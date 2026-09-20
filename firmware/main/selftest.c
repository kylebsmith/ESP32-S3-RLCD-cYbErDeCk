/*
 * The persistence self-test.
 *
 * It drives itself across real chip resets using a stage counter in NVS, so
 * it exercises the thing that actually matters - does the document come back
 * after the machine stops - rather than a function call that pretends to.
 *
 *   stage 0  seed paragraph A, save
 *   stage 1  assert A came back; seed B, save, then CORRUPT B's payload
 *   stage 2  assert A came back again, because B no longer validates
 *   stage 3  wipe the journal and get out of the way
 *
 * Stage 1 is the interesting one: it is the torn-write case, reproduced on
 * purpose. The repository's standard is that a fix ships with a check proven
 * to fail on the old state, and this is that check for the journal.
 */
#include "selftest.h"

#include <string.h>

#include "docstore.h"
#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "selftest";

#define NVS_NS  "deck"
#define KEY     "tstage"

static const char PARA_A[] =
    "The quick brown fox jumps over the lazy dog. Sphinx of black quartz, "
    "judge my vow. This paragraph is snapshot A and it must survive a reset.";

static const char PARA_B[] =
    "This is snapshot B. Its payload is deliberately corrupted after it is "
    "written, so it must NOT come back - snapshot A must.";

static uint8_t stage_get(void)
{
    nvs_handle_t h;
    uint8_t v = 0;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u8(h, KEY, &v);
        nvs_close(h);
    }
    return v;
}

static void stage_set(uint8_t v)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, KEY, v);
        nvs_commit(h);
        nvs_close(h);
    }
}

static bool doc_equals(const char *want)
{
    const size_t n = strlen(want);
    if (doc_len() != n) {
        return false;
    }
    for (size_t i = 0; i < n; i++) {
        if (doc_at(i) != want[i]) {
            return false;
        }
    }
    return true;
}

bool selftest_run(void)
{
    const uint8_t stage = stage_get();

    /* Refuse to run against a document that is not the test's own. Stage 2
     * erases the whole journal; on a device that has been written on, that is
     * the owner's work. The test only ever starts on an empty journal, so a
     * non-empty one at stage 0 means this is a real device, not a bench. */
    if (stage == 0 && doc_len() != 0) {
        ESP_LOGW(TAG, "journal already has %u bytes - skipping the self-test "
                      "rather than erasing someone's writing",
                 (unsigned)doc_len());
        stage_set(3);
        return false;
    }

    switch (stage) {
    case 0:
        ESP_LOGW(TAG, "STAGE 0: seeding snapshot A (%u bytes) and saving",
                 (unsigned)strlen(PARA_A));
        doc_set_text(PARA_A);
        if (doc_save() != ESP_OK) {
            ESP_LOGE(TAG, "STAGE 0: save FAILED");
        }
        stage_set(1);
        ESP_LOGW(TAG, "STAGE 0 done - reset the board to continue");
        return true;

    case 1:
        if (doc_equals(PARA_A)) {
            ESP_LOGW(TAG, "PASS 1/2: snapshot A survived a chip reset "
                          "(%u bytes restored)", (unsigned)doc_len());
        } else {
            ESP_LOGE(TAG, "FAIL 1/2: expected snapshot A, got %u bytes",
                     (unsigned)doc_len());
        }
        ESP_LOGW(TAG, "STAGE 1: writing snapshot B, then corrupting it");
        doc_set_text(PARA_B);
        doc_save();
        journal_corrupt_newest();
        stage_set(2);
        ESP_LOGW(TAG, "STAGE 1 done - reset the board to continue");
        return true;

    case 2:
        if (doc_equals(PARA_A)) {
            ESP_LOGW(TAG, "PASS 2/2: the corrupted snapshot B was rejected and "
                          "snapshot A loaded instead - torn-write recovery works");
        } else if (doc_equals(PARA_B)) {
            ESP_LOGE(TAG, "FAIL 2/2: corrupted snapshot B was accepted - the "
                          "CRC gate is not working");
        } else {
            ESP_LOGE(TAG, "FAIL 2/2: neither snapshot loaded (%u bytes)",
                     (unsigned)doc_len());
        }
        ESP_LOGW(TAG, "STAGE 2: wiping the journal, self-test complete");
        journal_erase_all();
        doc_set_text("");
        stage_set(3);
        return true;

    default:
        return false;       /* normal operation */
    }
}

void selftest_reset(void)
{
    stage_set(0);
}
