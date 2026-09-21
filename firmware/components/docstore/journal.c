/*
 * The snapshot journal.
 *
 * Layout of the `notes` partition: records on 4 KB sector boundaries,
 *
 *     magic u32 | seq u32 | len u32 | crc32 u32 | name[24] | payload
 *
 * Records are SECTOR ALIGNED and their sectors are erased immediately before
 * the write. That alignment is correctness, not tidiness: NOR flash can only
 * clear bits, so writing over the remains of a half-written record ANDs the
 * two together and yields garbage for ever. Erasing the target first means
 * every write lands on known-blank flash, and a record torn by a power cut is
 * repaired by the next save rather than poisoning the slot.
 *
 * The NAME is what turns one document into an archive. Records are scanned
 * for the newest valid record PER NAME, so each buffer has its own history in
 * the same append-only log and no second data structure is needed. An empty
 * name is the scratch buffer. None of the power-cut properties depend on the
 * name - they come from the sector alignment, the erase-before-write and the
 * CRC, all unchanged.
 *
 * Power-cut behaviour, which is the entire point:
 *   - torn mid-payload      -> CRC fails, record ignored, previous one loads
 *   - torn before the header-> no magic, slot skipped, previous one loads
 *   - torn during the erase -> that slot is blank, the previous record is in
 *                              a DIFFERENT sector and untouched, so it loads
 * In every case the loss is bounded by one snapshot, never the document.
 */
#include "docstore.h"

#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_rom_crc.h"

static const char *TAG = "journal";

#define REC_MAGIC    0x334B4544u       /* "DEK3" - named, typed, extensible */
#define REC_MAGIC_V2 0x324B4544u       /* "DEK2" - named, untyped           */
#define REC_MAGIC_V1 0x4B434544u       /* "DECK" - pre-name, = scratch      */
#define SECTOR 4096u

/* The current header. The reserved bytes are deliberate: every previous
 * format change has cost a migration and a second reader, so this one carries
 * room to grow. Anything added there is ignored by an older build rather than
 * shifting the payload underneath it. */
typedef struct {
    uint32_t magic;
    uint32_t seq;
    uint32_t len;
    uint32_t crc;
    uint8_t  kind;
    uint8_t  reserved[7];
    char     name[DOC_NAME_MAX];
} rec_hdr_t;

typedef struct {
    uint32_t magic;
    uint32_t seq;
    uint32_t len;
    uint32_t crc;
    char     name[DOC_NAME_MAX];
} rec_hdr_v2_t;

/* The v1 header, kept so existing writing is not stranded by the format
 * change. It is read, never written. */
typedef struct {
    uint32_t magic;
    uint32_t seq;
    uint32_t len;
    uint32_t crc;
} rec_hdr_v1_t;

static const esp_partition_t *s_part;
static uint32_t s_seq;

/* WHICH SECTORS HOLD LIVE DATA.
 *
 * The journal used to keep a write cursor and wrap it to zero when it ran off
 * the end. That is live data loss: sector 0 holds whichever document was
 * written first, and on this device that is the guide - the owner's
 * live-coding preset file. Wrapping erased it to make room for a save of
 * something else, silently, at runtime, with no error anywhere.
 *
 * So allocate instead of wrapping. One bit per 4 KB sector says whether a
 * LIVE record occupies it; a save finds a free run, and the space belonging
 * to the previous version of that same document is released only after the
 * new one is safely down. Nothing that is still reachable is ever erased. */
#define MAX_SECTORS 256
static uint8_t s_used[MAX_SECTORS / 8];
static size_t  s_nsectors;

/* Where each named document's live record sits, so its old sectors can be
 * released once a newer one is written. */
typedef struct {
    char   name[DOC_NAME_MAX];
    size_t off;
    size_t sectors;
    bool   in_use;
} live_t;
/* Sized to the ARCHIVE, not to the working set.
 *
 * This was DOC_MAX_BUFFERS (8) while journal_scan tracked JOURNAL_MAX_NAMES
 * (24), so a ninth document's sectors could not be claimed - they were left
 * marked free and find_free_run handed them to the next save. Raising the
 * scan array without raising this one moved the bug rather than fixing it,
 * which is exactly the mistake that made DOC_MAX_BUFFERS mean two things in
 * the first place. Reproduced with a ten-name harness: "sector 8 ninth ***
 * MARKED FREE ***", then that sector reused by the next save. */
#define JOURNAL_MAX_NAMES 24

static live_t s_live[JOURNAL_MAX_NAMES];

static inline bool sec_used(size_t s)
{
    return (s_used[s >> 3] & (1u << (s & 7))) != 0;
}

static inline void sec_mark(size_t s, bool used)
{
    if (used) {
        s_used[s >> 3] |= (uint8_t)(1u << (s & 7));
    } else {
        s_used[s >> 3] = (uint8_t)(s_used[s >> 3] & ~(1u << (s & 7)));
    }
}

static void live_claim(const char *name, size_t off, size_t sectors)
{
    live_t *slot = NULL;
    for (int i = 0; i < JOURNAL_MAX_NAMES; i++) {
        if (s_live[i].in_use &&
            strncmp(s_live[i].name, name, DOC_NAME_MAX) == 0) {
            slot = &s_live[i];
            break;
        }
    }
    if (slot == NULL) {
        for (int i = 0; i < JOURNAL_MAX_NAMES; i++) {
            if (!s_live[i].in_use) { slot = &s_live[i]; break; }
        }
    }
    if (slot == NULL) {
        /* No room to TRACK this document's sectors, which means nothing is
         * holding them and the next save can overwrite it. Say so loudly
         * rather than returning quietly: this is the exact silence that made
         * a ninth document disappear without a word. */
        ESP_LOGE(TAG, "live table full - '%s' is unprotected", name);
        return;
    }
    /* Release the previous version's sectors, now that the new one is down. */
    if (slot->in_use) {
        for (size_t s = 0; s < slot->sectors; s++) {
            sec_mark(slot->off / SECTOR + s, false);
        }
    }
    slot->in_use = true;
    snprintf(slot->name, DOC_NAME_MAX, "%s", name);
    slot->off = off;
    slot->sectors = sectors;
    for (size_t s = 0; s < sectors; s++) {
        sec_mark(off / SECTOR + s, true);
    }
}

/* Find a run of free sectors. Returns (size_t)-1 when the journal genuinely
 * has no room, which is a real condition and must be reported rather than
 * papered over by erasing something. */
static size_t find_free_run(size_t sectors)
{
    size_t run = 0;
    for (size_t s = 0; s < s_nsectors; s++) {
        if (sec_used(s)) {
            run = 0;
            continue;
        }
        if (++run == sectors) {
            return s + 1 - sectors;
        }
    }
    return (size_t)-1;
}

esp_err_t gapbuf_init(void);
void      gapbuf_load(const char *data, size_t len);
void      gapbuf_mark_clean(void);
int       buffer_claim(const char *name, size_t rec_off, size_t rec_len,
                       uint32_t seq, uint8_t kind);
const char *buffer_current_name(void);
uint8_t     buffer_current_kind(void);
bool        buffer_is_transient(const char *name);

static inline size_t rec_total(uint32_t len, size_t hdr)
{
    const size_t raw = hdr + len;
    return ((raw + SECTOR - 1u) / SECTOR) * SECTOR;
}

uint32_t doc_save_seq(void) { return s_seq; }

size_t doc_journal_used(void)
{
    size_t n = 0;
    for (size_t s = 0; s < s_nsectors; s++) {
        if (sec_used(s)) { n++; }
    }
    return n * SECTOR;
}

/* Read one record's header, whichever version it is. Returns false when the
 * slot holds nothing usable. */
static bool read_hdr(size_t off, rec_hdr_t *out, size_t *payload_off,
                     size_t *hdr_size)
{
    rec_hdr_t h;
    if (esp_partition_read(s_part, off, &h, sizeof h) != ESP_OK) {
        return false;
    }
    if (h.magic == REC_MAGIC) {
        *out = h;
        *hdr_size = sizeof(rec_hdr_t);
        *payload_off = off + *hdr_size;
        return h.len <= DOC_CAPACITY;
    }
    if (h.magic == REC_MAGIC_V2) {
        const rec_hdr_v2_t *o = (const rec_hdr_v2_t *)&h;
        memset(out, 0, sizeof *out);
        out->magic = o->magic; out->seq = o->seq;
        out->len = o->len;     out->crc = o->crc;
        out->kind = DOC_KIND_PROSE;
        snprintf(out->name, DOC_NAME_MAX, "%.*s", DOC_NAME_MAX - 1, o->name);
        *hdr_size = sizeof(rec_hdr_v2_t);
        *payload_off = off + *hdr_size;
        return o->len <= DOC_CAPACITY;
    }
    if (h.magic == REC_MAGIC_V1) {
        const rec_hdr_v1_t *o = (const rec_hdr_v1_t *)&h;
        memset(out, 0, sizeof *out);
        out->magic = o->magic; out->seq = o->seq;
        out->len = o->len;     out->crc = o->crc;
        out->name[0] = '\0';           /* v1 records are the scratch buffer */
        *hdr_size = sizeof(rec_hdr_v1_t);
        *payload_off = off + *hdr_size;
        return o->len <= DOC_CAPACITY;
    }
    return false;
}

/* Fetch a record's payload. Used to load a buffer's text on demand. */
esp_err_t journal_load_into(size_t rec_off, char *out, size_t max, size_t *len)
{
    if (s_part == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    rec_hdr_t h;
    size_t poff, hsz;
    if (!read_hdr(rec_off, &h, &poff, &hsz) || h.len > max) {
        return ESP_ERR_NOT_FOUND;
    }
    if (esp_partition_read(s_part, poff, out, h.len) != ESP_OK) {
        return ESP_FAIL;
    }
    if (esp_rom_crc32_le(0, (const uint8_t *)out, h.len) != h.crc) {
        return ESP_ERR_INVALID_CRC;
    }
    *len = h.len;
    return ESP_OK;
}

/* HOW MANY DISTINCT NAMES THE ARCHIVE CAN HOLD.
 *
 * This used to be DOC_MAX_BUFFERS, which is 8 and means something else - how
 * many documents are resident in RAM at once. Conflating them was silent
 * document loss: a ninth distinct name found no slot, so its record was
 * skipped entirely, never claimed its sectors, and find_free_run handed them
 * to the next save. And because the walk is in PHYSICAL SECTOR ORDER rather
 * than by age, which eight names survived was arbitrary - not the newest,
 * not the oldest.
 *
 * It is reachable with shipped commands: name eight documents, '>close' one,
 * then '>new' and '>name' a ninth.
 *
 * The archive can be larger than the working set; the only cost here is stack
 * for the scan array, 40 bytes per entry. */
/* Walk the whole partition and register the newest valid record per name. */
static void journal_scan(char *scratch, size_t scratch_len)
{
    struct { char name[DOC_NAME_MAX]; uint32_t seq; size_t off; size_t len;
             uint8_t kind; } best[JOURNAL_MAX_NAMES];
    int nbest = 0;
    size_t off = 0;

    memset(s_used, 0, sizeof s_used);
    memset(s_live, 0, sizeof s_live);
    s_nsectors = s_part->size / SECTOR;
    if (s_nsectors > MAX_SECTORS) {
        s_nsectors = MAX_SECTORS;
    }

    while (off + sizeof(rec_hdr_t) <= s_part->size) {
        rec_hdr_t h;
        size_t poff, hsz;
        if (!read_hdr(off, &h, &poff, &hsz)) {
            off += SECTOR;
            continue;
        }
        if (off + rec_total(h.len, hsz) > s_part->size) {
            off += SECTOR;
            continue;
        }
        bool ok = false;
        if (h.len <= scratch_len &&
            esp_partition_read(s_part, poff, scratch, h.len) == ESP_OK) {
            ok = esp_rom_crc32_le(0, (const uint8_t *)scratch, h.len) == h.crc;
        }
        if (!ok) {
            ESP_LOGW(TAG, "record at %u fails CRC - ignoring (the torn-write "
                          "case, working as designed)", (unsigned)off);
            off += rec_total(h.len, hsz);
            continue;
        }

        int slot = -1;
        for (int i = 0; i < nbest; i++) {
            if (strncmp(best[i].name, h.name, DOC_NAME_MAX) == 0) { slot = i; break; }
        }
        if (slot < 0 && nbest >= JOURNAL_MAX_NAMES) {
            /* No room to TRACK this name - but its sectors are still live,
             * and handing them to the next write would overwrite a document
             * that merely cannot be listed. Losing the ability to open a
             * document is bad; overwriting it is unrecoverable. Claim the
             * sectors under a placeholder so find_free_run never returns
             * them, and say so, loudly, because this is data the owner can
             * no longer reach. */
            live_claim("", off, rec_total(h.len, hsz) / SECTOR);
            ESP_LOGE(TAG, "archive full: '%s' is held but unreachable "
                          "(%d names max)", h.name, JOURNAL_MAX_NAMES);
            if (h.seq >= s_seq) {
                s_seq = h.seq;
            }
            off += rec_total(h.len, hsz);
            continue;
        }
        if (slot < 0) {
            slot = nbest++;
            snprintf(best[slot].name, DOC_NAME_MAX, "%s", h.name);
            best[slot].seq = 0;
        }
        if (slot >= 0 && h.seq >= best[slot].seq) {
            best[slot].seq  = h.seq;
            best[slot].off  = off;
            best[slot].len  = h.len;
            best[slot].kind = h.kind;
        }
        if (h.seq >= s_seq) {
            s_seq = h.seq;
        }
        off += rec_total(h.len, hsz);
    }

    /* Only the SURVIVING record of each name holds its sectors. Everything
     * else in the partition is stale and is free to be reused. */
    for (int i = 0; i < nbest; i++) {
        rec_hdr_t h;
        size_t poff, hsz;
        if (read_hdr(best[i].off, &h, &poff, &hsz)) {
            live_claim(best[i].name, best[i].off,
                       rec_total(h.len, hsz) / SECTOR);
        }
    }

    if (nbest > DOC_MAX_BUFFERS) {
        ESP_LOGW(TAG, "%d documents archived, %d can be open at once",
                 nbest, DOC_MAX_BUFFERS);
    }

    /* Slot 0 is the scratch buffer and already exists; give it its record if
     * the journal has one, and claim a slot for every named document. */
    for (int i = 0; i < nbest; i++) {
        if (best[i].name[0] == '\0') {
            size_t n = 0;
            if (journal_load_into(best[i].off, scratch, DOC_CAPACITY, &n) == ESP_OK) {
                gapbuf_load(scratch, n);
                ESP_LOGI(TAG, "restored scratch, %u bytes, seq %u",
                         (unsigned)n, (unsigned)best[i].seq);
            }
        } else if (buffer_claim(best[i].name, best[i].off, best[i].len,
                                best[i].seq, best[i].kind) >= 0) {
            ESP_LOGI(TAG, "archive: '%s' %u bytes, seq %u", best[i].name,
                     (unsigned)best[i].len, (unsigned)best[i].seq);
        }
    }
    ESP_LOGI(TAG, "%d document(s), %u of %u sectors live",
             nbest, (unsigned)(doc_journal_used() / SECTOR),
             (unsigned)s_nsectors);
}

esp_err_t doc_init(void)
{
    esp_err_t err = gapbuf_init();
    if (err != ESP_OK) {
        return err;
    }

    s_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, "notes");
    if (s_part == NULL) {
        ESP_LOGE(TAG, "no 'notes' partition");
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "journal at 0x%06x, %u B", (unsigned)s_part->address,
             (unsigned)s_part->size);

    char *scratch = malloc(DOC_CAPACITY);
    if (scratch == NULL) {
        return ESP_ERR_NO_MEM;
    }
    journal_scan(scratch, DOC_CAPACITY);
    free(scratch);
    return ESP_OK;
}

esp_err_t doc_save(void)
{
    if (s_part == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    /* Machine-written buffers are never archived. */
    if (buffer_is_transient(buffer_current_name())) {
        gapbuf_mark_clean();
        return ESP_OK;
    }
    const size_t len = doc_len();
    const size_t need = rec_total((uint32_t)len, sizeof(rec_hdr_t));

    if (need > s_part->size) {
        ESP_LOGE(TAG, "document larger than the journal partition");
        return ESP_ERR_INVALID_SIZE;
    }

    const size_t want = need / SECTOR;
    const size_t start = find_free_run(want);
    if (start == (size_t)-1) {
        /* Genuinely full of LIVE documents. Report it; do not make room by
         * erasing something the owner can still reach. */
        ESP_LOGE(TAG, "journal full: %u of %u sectors hold live documents",
                 (unsigned)(doc_journal_used() / SECTOR),
                 (unsigned)s_nsectors);
        return ESP_ERR_NO_MEM;
    }
    const size_t s_cursor = start * SECTOR;

    /* INTERNAL RAM, NOT PSRAM, AND IT IS A TIMING DECISION.
     *
     * With SPIRAM_MALLOC_ALWAYSINTERNAL at 4096, a document over 4 KB lands
     * in PSRAM - and esp_flash's write path checks esp_ptr_in_dram() on its
     * source buffer. A PSRAM source is copied through a 32-byte stack bounce
     * buffer, so a single 5 KB write becomes ~160 separate flash operations,
     * each with its own cache-disable bracket. One stall becomes a hundred
     * and sixty, and the sequencer's clock cannot run through any of them.
     *
     * The buffer is transient and at most DOC_CAPACITY, so internal RAM can
     * afford it. */
    char *tmp = heap_caps_malloc(len > 0 ? len : 1,
                                 MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (tmp == NULL) {
        return ESP_ERR_NO_MEM;
    }
    doc_read(tmp, len);

    rec_hdr_t h = {
        .magic = REC_MAGIC,
        .seq   = s_seq + 1,
        .len   = (uint32_t)len,
        .crc   = esp_rom_crc32_le(0, (const uint8_t *)tmp, len),
    };
    h.kind = buffer_current_kind();
    snprintf(h.name, sizeof h.name, "%s", buffer_current_name());

    /* Erase the slot, then payload, then header. The header goes last so a
     * cut between payload and header leaves no magic and the scan skips the
     * slot instead of trusting a half-written record. */
    esp_err_t err = esp_partition_erase_range(s_part, s_cursor, need);
    if (err == ESP_OK && len > 0) {
        err = esp_partition_write(s_part, s_cursor + sizeof h, tmp, len);
    }
    if (err == ESP_OK) {
        err = esp_partition_write(s_part, s_cursor, &h, sizeof h);
    }
    free(tmp);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "save failed: %s", esp_err_to_name(err));
        return err;
    }

    s_seq = h.seq;
    live_claim(h.name, s_cursor, want);
    gapbuf_mark_clean();
    return ESP_OK;
}

/* ---------------------------------------------------------------- self-test
 * Deliberately damaging; only the self-test calls these.
 */

esp_err_t journal_erase_all(void)
{
    if (s_part == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    const esp_err_t err = esp_partition_erase_range(s_part, 0, s_part->size);
    if (err == ESP_OK) {
        memset(s_used, 0, sizeof s_used);
        memset(s_live, 0, sizeof s_live);
        s_seq = 0;
    }
    return err;
}

/* Simulate a write torn by a power cut: clear some payload bits in the newest
 * record. Writing zeros always succeeds on NOR flash, so this reproduces the
 * exact failure a cut mid-write leaves behind. Newest by SEQUENCE - picking
 * the last by offset corrupts an unrelated record once the journal has
 * wrapped, and the test then reports a CRC failure that says nothing. */
esp_err_t journal_corrupt_newest(void)
{
    if (s_part == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    size_t off = 0, last = 0, last_payload = 0;
    uint32_t best = 0;
    bool found = false;
    while (off + sizeof(rec_hdr_t) <= s_part->size) {
        rec_hdr_t h;
        size_t poff, hsz;
        if (read_hdr(off, &h, &poff, &hsz) &&
            off + rec_total(h.len, hsz) <= s_part->size) {
            if (!found || h.seq >= best) {
                best = h.seq; last = off; last_payload = poff; found = true;
            }
            off += rec_total(h.len, hsz);
        } else {
            off += SECTOR;
        }
    }
    if (!found) {
        return ESP_ERR_NOT_FOUND;
    }
    const uint8_t zeros[8] = {0};
    const esp_err_t err =
        esp_partition_write(s_part, last_payload, zeros, sizeof zeros);
    ESP_LOGW(TAG, "self-test: corrupted payload of record at %u", (unsigned)last);
    return err;
}
