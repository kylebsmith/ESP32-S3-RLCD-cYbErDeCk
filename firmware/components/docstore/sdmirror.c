/*
 * The SD mirror. An export medium, never the source of truth.
 *
 * Written .tmp-then-rename so a cut during the write leaves either the old
 * complete file or an orphan .tmp, and never a half-written document.
 *
 * TWO CORRECTIONS, both found on hardware by watching the log while driving
 * the sequencer, and both of them silent data loss:
 *
 *  1. Every buffer was mirrored to the SAME FILE, notes.txt. Switching
 *     documents therefore overwrote the previous document's backup with the
 *     current one, so the card held exactly one document - whichever was
 *     edited last - while appearing to hold a backup of the work. The mirror
 *     is now one file per document name.
 *
 *  2. doc_save() refuses to journal a transient '+' buffer, but the caller
 *     went on to mirror it anyway, so command output - the contents of +out -
 *     was written to the card as though it were a document. docs/OS.md is
 *     explicit that machine-written buffers are not archived, and the owner's
 *     reason is concrete: these files are copied to a DGX for semantic
 *     analysis, and a corpus salted with command transcripts is a corpus that
 *     has been quietly poisoned. The guard belongs in both places, because
 *     the two paths have failed apart once already.
 */
#include "docstore.h"
#include "mirror_path.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "driver/sdmmc_host.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

static const char *TAG = "sdmirror";

/* solar_term.toml: 1-bit SDMMC, CLK 38, CMD 21, D0 39. */
#define PIN_CLK 38
#define PIN_CMD 21
#define PIN_D0  39

#define MOUNT "/sdcard"

/* One fixed scratch name, renamed onto the target. Rename within a directory
 * is the atomic step; the temp file never needs to be per-document. */
#define MIRROR_TMP MOUNT "/mirror.tmp"

/* The internal contract with buffer.c, declared the same way journal.c
 * declares it - these are docstore internals, not public API. */
const char *buffer_current_name(void);

static sdmmc_card_t *s_card;

bool doc_sd_present(void) { return s_card != NULL; }

esp_err_t sdmirror_init(void)
{
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    /* Clear the wide-bus and DDR capability bits rather than replacing the
     * whole flags word - the default also carries DEINIT_ARG, which the host
     * needs to tear itself down correctly. */
    host.flags &= ~(uint32_t)(SDMMC_HOST_FLAG_4BIT | SDMMC_HOST_FLAG_8BIT |
                              SDMMC_HOST_FLAG_DDR);
    host.max_freq_khz = SDMMC_FREQ_PROBING;   /* 400 kHz; be conservative */

    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;
    slot.clk   = PIN_CLK;
    slot.cmd   = PIN_CMD;
    slot.d0    = PIN_D0;
    slot.d1    = GPIO_NUM_NC;
    slot.d2    = GPIO_NUM_NC;
    slot.d3    = GPIO_NUM_NC;
    slot.cd    = SDMMC_SLOT_NO_CD;
    slot.wp    = SDMMC_SLOT_NO_WP;
    slot.flags = SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    const esp_vfs_fat_sdmmc_mount_config_t mcfg = {
        .format_if_mount_failed = false,
        .max_files              = 4,
        .allocation_unit_size   = 16 * 1024,
    };

    esp_err_t err = esp_vfs_fat_sdmmc_mount(MOUNT, &host, &slot, &mcfg, &s_card);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "mount at 400 kHz failed (%s), retrying at default speed",
                 esp_err_to_name(err));
        host.max_freq_khz = SDMMC_FREQ_DEFAULT;
        err = esp_vfs_fat_sdmmc_mount(MOUNT, &host, &slot, &mcfg, &s_card);
    }
    if (err != ESP_OK) {
        /* A missing or unreadable card is not an error worth stopping for -
         * the journal is the truth and the deck must work without a card. */
        /* A timeout on the first command is what an EMPTY SLOT looks like,
         * and this board routes no card-detect line, so the firmware cannot
         * tell that apart from a wiring fault. Say both rather than implying
         * a defect - most of a session was spent debugging a bus that was
         * fine because this message only mentioned failure. */
        if (err == ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "no SD card responded - the slot is probably EMPTY "
                          "(no card-detect pin on this board); journal only");
        } else {
            ESP_LOGW(TAG, "no SD card mounted (%s) - journal only",
                     esp_err_to_name(err));
        }
        s_card = NULL;
        return err;
    }
    ESP_LOGI(TAG, "SD mounted: %s, %llu MB", s_card->cid.name,
             ((uint64_t)s_card->csd.capacity * s_card->csd.sector_size) >> 20);
    return ESP_OK;
}

esp_err_t doc_mirror_sd(void)
{
    if (s_card == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Machine-written buffers are not archived - see the note at the top. */
    const char *name = buffer_current_name();
    if (mirror_is_transient(name)) {
        return ESP_OK;
    }

    char path[sizeof(MOUNT) + DOC_NAME_MAX + 8];
    mirror_path(path, sizeof path, MOUNT, name);

    const size_t len = doc_len();
    char *buf = malloc(len > 0 ? len : 1);
    if (buf == NULL) {
        return ESP_ERR_NO_MEM;
    }
    doc_read(buf, len);

    FILE *f = fopen(MIRROR_TMP, "wb");
    if (f == NULL) {
        free(buf);
        ESP_LOGW(TAG, "cannot open " MIRROR_TMP);
        return ESP_FAIL;
    }
    const size_t wrote = len > 0 ? fwrite(buf, 1, len, f) : 0;
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    free(buf);

    if (wrote != len) {
        ESP_LOGW(TAG, "short write %u of %u", (unsigned)wrote, (unsigned)len);
        remove(MIRROR_TMP);
        return ESP_FAIL;
    }

    remove(path);
    if (rename(MIRROR_TMP, path) != 0) {
        ESP_LOGW(TAG, "rename to %s failed", path);
        remove(MIRROR_TMP);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "mirrored %u bytes to %s", (unsigned)len, path);
    return ESP_OK;
}
