/*
 * The SD mirror. An export medium, never the source of truth.
 *
 * Written .tmp-then-rename so a cut during the write leaves either the old
 * complete file or an orphan .tmp, and never a half-written notes.txt.
 */
#include "docstore.h"

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
        ESP_LOGW(TAG, "no SD card mounted (%s) - journal only",
                 esp_err_to_name(err));
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

    const size_t len = doc_len();
    char *buf = malloc(len > 0 ? len : 1);
    if (buf == NULL) {
        return ESP_ERR_NO_MEM;
    }
    doc_read(buf, len);

    FILE *f = fopen(MOUNT "/notes.tmp", "wb");
    if (f == NULL) {
        free(buf);
        ESP_LOGW(TAG, "cannot open notes.tmp");
        return ESP_FAIL;
    }
    const size_t wrote = len > 0 ? fwrite(buf, 1, len, f) : 0;
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    free(buf);

    if (wrote != len) {
        ESP_LOGW(TAG, "short write %u of %u", (unsigned)wrote, (unsigned)len);
        return ESP_FAIL;
    }

    remove(MOUNT "/notes.txt");
    if (rename(MOUNT "/notes.tmp", MOUNT "/notes.txt") != 0) {
        ESP_LOGW(TAG, "rename failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}
