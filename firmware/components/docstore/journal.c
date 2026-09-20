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
static size_t   s_cursor;
static uint32_t s_seq;

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

uint32_t doc_save_seq(void)     { return s_seq; }
size_t   doc_journal_used(void) { return s_cursor; }

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

/* Walk the whole partition and register the newest valid record per name. */
static void journal_scan(char *scratch, size_t scratch_len)
{
    struct { char name[DOC_NAME_MAX]; uint32_t seq; size_t off; size_t len;
             uint8_t kind; } best[DOC_MAX_BUFFERS];
    int nbest = 0;
    size_t next_free = 0;
    size_t off = 0;

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
        if (slot < 0 && nbest < DOC_MAX_BUFFERS) {
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
            /* Follow the NEWEST record, not the last by offset: a wrapped
             * journal has older records at higher offsets, and writing after
             * one of those would erase the sector holding the newest. */
            next_free = off + rec_total(h.len, hsz);
        }
        off += rec_total(h.len, hsz);
    }

    s_cursor = next_free < s_part->size ? next_free : 0;

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
    ESP_LOGI(TAG, "%d document(s) in the journal, cursor at %u",
             nbest, (unsigned)s_cursor);
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
    if (s_cursor + need > s_part->size) {
        s_cursor = 0;                   /* wrap; the seq keeps the ordering */
    }

    char *tmp = malloc(len > 0 ? len : 1);
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
    s_cursor += need;
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
        s_cursor = 0;
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
    if (s_part == NULL || s_cursor < SECTOR) {
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
