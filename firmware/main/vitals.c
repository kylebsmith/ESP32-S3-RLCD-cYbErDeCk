#include "vitals.h"

#include <stdio.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs.h"

static const char *TAG = "vitals";

#define NVS_NS   "deck"
#define NVS_KEY  "vitals"
#define WRITE_EVERY_S 60

static vitals_t s_prev;
static uint32_t s_loops;
static int64_t  s_last_write_us;
static char     s_line[VITALS_LINES][31];
static int      s_nlines;

static void write_record(bool usb_mode, bool running, uint32_t ticks,
                         bool deliberate)
{
    const vitals_t v = {
        .uptime_s     = (uint32_t)(esp_timer_get_time() / 1000000),
        .loops        = s_loops,
        .ticks        = ticks,
        .heap_free    = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        .usb_mode     = usb_mode ? 1u : 0u,
        .running      = running ? 1u : 0u,
        .reset_reason = (uint8_t)esp_reset_reason(),
        .valid        = 1u,
        .deliberate   = deliberate ? 1u : 0u,
    };
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
        return;
    }
    /* No nvs_commit() error handling and no retry: this is a diagnostic, and a
     * diagnostic that can fail the thing it is diagnosing is worse than no
     * diagnostic. A lost record costs one minute of resolution. */
    if (nvs_set_blob(h, NVS_KEY, &v, sizeof v) == ESP_OK) {
        nvs_commit(h);
    }
    nvs_close(h);
}

void vitals_begin(void)
{
    nvs_handle_t h;
    size_t len = sizeof s_prev;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        if (nvs_get_blob(h, NVS_KEY, &s_prev, &len) != ESP_OK ||
            len != sizeof s_prev) {
            memset(&s_prev, 0, sizeof s_prev);
        }
        nvs_close(h);
    }
    s_last_write_us = esp_timer_get_time();

    if (!s_prev.valid) {
        snprintf(s_line[0], sizeof s_line[0], "no previous run recorded");
        s_nlines = 1;
        return;
    }

    /* THE VERDICT IS THE WHOLE POINT, and it has to fit.
     *
     * A record saying the loop was advancing and the clock was running, written
     * one minute before a run that ended with no reset reason to explain it, is
     * the difference between "the editor stopped" and "everything stopped" - and
     * that is the question three rounds of testing from the outside could not
     * answer. LOOPS PER SECOND is the discriminator: a healthy rate in the last
     * record means the deck was fine and then stopped abruptly, which rules out
     * slow degradation. */
    const unsigned rate = s_prev.uptime_s
                        ? (unsigned)(s_prev.loops / s_prev.uptime_s) : 0u;
    int n = 0;
    snprintf(s_line[n++], sizeof s_line[0], "last run: %us, %s",
             (unsigned)s_prev.uptime_s, s_prev.usb_mode ? "USB MIDI" : "serial");
    snprintf(s_line[n++], sizeof s_line[0], "  %u loops/s, %uK heap",
             rate, (unsigned)(s_prev.heap_free / 1024));
    /* THREE OUTCOMES, NOT TWO, and the third is the one that cost a session.
     *
     * A goodbye is written immediately BEFORE the restart, so it records an
     * intention rather than an outcome - and when esp_restart() itself deadlocked
     * the record said "restarted on purpose" about a deck that never restarted at
     * all. The first version of this reported exactly that, confidently.
     *
     * The discriminator is the reset reason of the boot that reads it. A
     * deliberate restart that WORKED arrives here as a software or USB reset; one
     * that hung is recovered by a power cycle, so it arrives as POWERON. Said
     * goodbye and then needed a human to press the button means the restart never
     * completed.
     *
     * A deliberate power cycle straight after a successful '>usb off' reads as the
     * same thing. That is a false positive worth having: it says the restart may
     * not have completed, which is cheap to dismiss, where missing a real one cost
     * three sessions of blaming the transport. */
    const esp_reset_reason_t now = esp_reset_reason();
    if (!s_prev.deliberate) {
        /* The run never closed its record, so it did not choose to stop. */
        snprintf(s_line[n++], sizeof s_line[0], "  STOPPED DEAD - see OS.md");
    } else if (now == ESP_RST_POWERON) {
        snprintf(s_line[n++], sizeof s_line[0], "  RESTART HUNG - see OS.md");
    } else {
        snprintf(s_line[n++], sizeof s_line[0], "  restarted on purpose");
    }
    s_nlines = n;

    for (int i = 0; i < n; i++) {
        ESP_LOGW(TAG, "%s", s_line[i]);
    }
    ESP_LOGW(TAG, "  ticks %u, reset then %d, now %d",
             (unsigned)s_prev.ticks, (int)s_prev.reset_reason, (int)now);
}

const vitals_t *vitals_previous(void) { return &s_prev; }

void vitals_loop(bool usb_mode, bool running, uint32_t ticks)
{
    s_loops++;
    if (!usb_mode) {
        /* Serial mode has never shown the bug, so it pays nothing. */
        return;
    }
    const int64_t now = esp_timer_get_time();
    if (now - s_last_write_us < (int64_t)WRITE_EVERY_S * 1000000) {
        return;
    }
    s_last_write_us = now;
    write_record(usb_mode, running, ticks, false);
}

void vitals_goodbye(bool usb_mode, bool running, uint32_t ticks)
{
    write_record(usb_mode, running, ticks, true);
}

int vitals_report(const char *out[VITALS_LINES])
{
    for (int i = 0; i < s_nlines; i++) {
        out[i] = s_line[i];
    }
    return s_nlines;
}
