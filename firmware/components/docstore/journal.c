/*
 * The snapshot journal.
 *
 * Layout of the `notes` partition: a run of records, each starting on a 4 KB
 * sector boundary,
 *
 *     magic u32 | seq u32 | len u32 | crc32 u32 | payload
 *
 * Records are SECTOR ALIGNED and their sectors are erased immediately before
 * the write. That alignment is not tidiness, it is correctness: NOR flash can
 * only clear bits, so writing a record over the remains of a half-written one
 * ANDs the two together and yields garbage for ever. Erasing the target first
 * means every write lands on known-blank flash, and a record torn by a power
 * cut is repaired by the next save rather than poisoning the slot.
 *
 * Power-cut behaviour, which is the entire point:
 *   - torn mid-payload      -> CRC fails, record ignored, previous one loads
 *   - torn before the header-> no magic, scan stops, previous one loads
 *   - torn during the erase -> that slot is blank, previous record is in a
 *                              DIFFERENT sector and untouched, so it loads
 * In every case the loss is bounded by one snapshot, never the document.
 */
#include "docstore.h"

#include <string.h>

#include "esp_log.h"
#include "esp_partition.h"
#include "esp_rom_crc.h"

static const char *TAG = "journal";

#define REC_MAGIC 0x4B434544u          /* "DECK" */

typedef struct {
    uint32_t magic;
    uint32_t seq;
    uint32_t len;
    uint32_t crc;
} rec_hdr_t;

static const esp_partition_t *s_part;
static size_t   s_cursor;              /* where the next record goes */
static uint32_t s_seq;

/* Forward declarations into gapbuf.c */
esp_err_t gapbuf_init(void);
void      gapbuf_load(const char *data, size_t len);
void      gapbuf_mark_clean(void);

#define SECTOR 4096u

/* A record occupies whole sectors, so the next one always starts on blank
 * flash that can be erased without touching its predecessor. */
static inline size_t rec_total(uint32_t len)
{
    const size_t raw = sizeof(rec_hdr_t) + len;
    return ((raw + SECTOR - 1u) / SECTOR) * SECTOR;
}

uint32_t doc_save_seq(void)      { return s_seq; }
size_t   doc_journal_used(void)  { return s_cursor; }

/* Walk the journal and hand back the newest record that validates. */
static esp_err_t journal_scan(char *scratch, size_t scratch_len,
                              size_t *best_len)
{
    size_t off = 0;
    size_t best_off = 0;
    uint32_t best_seq = 0;
    bool found = false;

    size_t next_free = 0;
    while (off + sizeof(rec_hdr_t) <= s_part->size) {
        rec_hdr_t h;
        if (esp_partition_read(s_part, off, &h, sizeof h) != ESP_OK) {
            break;
        }
        if (h.magic != REC_MAGIC ||
            h.len > scratch_len ||
            off + rec_total(h.len) > s_part->size) {
            off += SECTOR;              /* blank or unusable slot - step on */
            continue;
        }
        if (esp_partition_read(s_part, off + sizeof h, scratch, h.len) == ESP_OK &&
            esp_rom_crc32_le(0, (const uint8_t *)scratch, h.len) == h.crc) {
            if (!found || h.seq >= best_seq) {
                best_seq = h.seq;
                best_off = off;
                found = true;
                /* Follow the NEWEST record, not the last one encountered.
                 * A wrapped journal has older records at higher offsets, so
                 * taking the last by offset put the write cursor immediately
                 * after a STALE record - and the next save would erase the
                 * sector holding the newest one. */
                next_free = off + rec_total(h.len);
            }
        } else {
            ESP_LOGW(TAG, "record at %u fails CRC - ignoring (the torn-write "
                          "case, working as designed)", (unsigned)off);
        }
        off += rec_total(h.len);
    }

    s_cursor = next_free < s_part->size ? next_free : 0;

    if (!found) {
        s_seq = 0;
        *best_len = 0;
        return ESP_ERR_NOT_FOUND;
    }

    rec_hdr_t h;
    esp_partition_read(s_part, best_off, &h, sizeof h);
    esp_partition_read(s_part, best_off + sizeof h, scratch, h.len);
    s_seq = h.seq;
    *best_len = h.len;
    return ESP_OK;
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

    size_t len = 0;
    err = journal_scan(scratch, DOC_CAPACITY, &len);
    if (err == ESP_OK) {
        gapbuf_load(scratch, len);
        ESP_LOGI(TAG, "restored %u bytes, seq %u, cursor at %u",
                 (unsigned)len, (unsigned)s_seq, (unsigned)s_cursor);
    } else {
        ESP_LOGI(TAG, "journal empty - starting a new document");
    }
    free(scratch);
    return ESP_OK;
}

esp_err_t doc_save(void)
{
    if (s_part == NULL) {
        return ESP_ERR_INVALID_STATE;   /* doc_init failed; do not deref */
    }
    const size_t len = doc_len();
    const size_t need = rec_total((uint32_t)len);

    if (need > s_part->size) {
        ESP_LOGE(TAG, "document larger than the journal partition");
        return ESP_ERR_INVALID_SIZE;
    }
    if (s_cursor + need > s_part->size) {
        s_cursor = 0;            /* wrap; the seq number keeps ordering us */
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

    /* Erase the slot, then payload, then header. The header goes last so a
     * cut between payload and header leaves no magic and the scan stops at
     * the previous record instead of trusting a half-written one. */
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
 * These exist so the torn-write recovery path can be exercised deliberately
 * rather than hoped about. A check that never runs is not a check.
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
 * exact failure a cut mid-write leaves behind. */
esp_err_t journal_corrupt_newest(void)
{
    if (s_part == NULL || s_cursor < SECTOR) {
        return ESP_ERR_INVALID_STATE;
    }
    /* Newest by SEQUENCE. Picking the last by offset corrupts an unrelated
     * record once the journal has wrapped, and the test then reports a CRC
     * failure that says nothing about the CRC gate. */
    size_t off = 0, last = 0;
    uint32_t best = 0;
    bool found = false;
    while (off + sizeof(rec_hdr_t) <= s_part->size) {
        rec_hdr_t h;
        if (esp_partition_read(s_part, off, &h, sizeof h) != ESP_OK) break;
        if (h.magic == REC_MAGIC && h.len <= DOC_CAPACITY &&
            off + rec_total(h.len) <= s_part->size) {
            if (!found || h.seq >= best) { best = h.seq; last = off; found = true; }
            off += rec_total(h.len);
        } else {
            off += SECTOR;
        }
    }
    if (!found) {
        return ESP_ERR_NOT_FOUND;
    }
    const uint8_t zeros[8] = {0};
    const esp_err_t err =
        esp_partition_write(s_part, last + sizeof(rec_hdr_t), zeros, sizeof zeros);
    ESP_LOGW(TAG, "self-test: corrupted payload of record at %u", (unsigned)last);
    return err;
}
