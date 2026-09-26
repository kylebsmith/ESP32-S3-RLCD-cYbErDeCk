/*
 * The previous run's last words, as the lines '>jitter' prints. Pure, so
 * tools/test_vitals.c runs this exact code against every outcome - which the
 * deck itself cannot: two of them need a hang in USB MIDI mode or a hand on the
 * power switch.
 *
 * FOUR OUTCOMES, NOT THREE. A record is written once a minute only in USB MIDI
 * mode (the only place the hang has been seen), and on a deliberate restart. So a
 * run in serial mode left nothing, and the report went on describing whichever run
 * last wrote a record: both decks said "last run: 67s, serial ... restarted on
 * purpose" through dozens of flashes on 2026-09-25. Now every boot marks its start,
 * and a serial run that ends without saying goodbye - flashed, unplugged - is
 * reported as what it is: not watched. Calling it STOPPED DEAD would send the hunt
 * after every flash.
 */
#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "vitals.h"

#define VITALS_COLS 31

/* `power_on`: this boot came from a power cycle. */
static inline int vitals_verdict(const vitals_t *p, bool power_on,
                                 char out[][VITALS_COLS], int max)
{
    int n = 0;
    if (max < VITALS_LINES) {
        return 0;
    }
    if (!p->valid) {
        snprintf(out[n++], VITALS_COLS, "no previous run recorded");
        return n;
    }
    if (!p->deliberate && !p->usb_mode) {
        snprintf(out[n++], VITALS_COLS, "last run: serial, not watched");
        snprintf(out[n++], VITALS_COLS, "  only USB MIDI runs are");
        return n;
    }
    /* LOOPS PER SECOND is the discriminator: a healthy rate in the last record
     * means the deck was fine and then stopped abruptly. */
    const unsigned rate = p->uptime_s ? (unsigned)(p->loops / p->uptime_s) : 0u;
    snprintf(out[n++], VITALS_COLS, "last run: %us, %s",
             (unsigned)p->uptime_s, p->usb_mode ? "USB MIDI" : "serial");
    snprintf(out[n++], VITALS_COLS, "  %u loops/s, %uK heap",
             rate, (unsigned)(p->heap_free / 1024));
    /* A goodbye is written BEFORE the restart, so it records an intention, not
     * an outcome: when esp_restart() itself deadlocked, the record said
     * "restarted on purpose" about a deck that never restarted. A restart that
     * worked arrives as a software or USB reset; one that hung was recovered by
     * a power cycle. */
    if (!p->deliberate) {
        snprintf(out[n++], VITALS_COLS, "  STOPPED DEAD - see OS.md");
    } else if (power_on) {
        snprintf(out[n++], VITALS_COLS, "  RESTART HUNG - see OS.md");
    } else {
        snprintf(out[n++], VITALS_COLS, "  restarted on purpose");
    }
    return n;
}
