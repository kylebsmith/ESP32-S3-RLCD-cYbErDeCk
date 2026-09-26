/*
 * The previous run's last words - every outcome, including the two the deck
 * cannot produce on the bench (docs/NEXT.md §2: "written but never exercised").
 *
 * STOPPED DEAD needs a hang in USB MIDI mode and RESTART HUNG needs a hand on
 * the power switch, so on the deck they have never been seen. This runs
 * firmware/main/vitals_verdict.h - the code the deck prints from - through all
 * of them, and through the one it got wrong: a serial run that ended without a
 * goodbye. The rule this replaced called that STOPPED DEAD; before every boot
 * marked its start it was not reachable, and the report described an older run
 * instead - "last run: 67s, serial ... restarted on purpose", through a day of
 * flashes.
 */
#include <stdio.h>
#include <string.h>

#include "vitals_verdict.h"

static int fails;

#define CHECK(cond, ...) do { if (!(cond)) { printf("[FAIL] " __VA_ARGS__); \
    printf("\n"); fails++; } else { printf("[ ok ] " __VA_ARGS__); printf("\n"); } } while (0)

static char L[VITALS_LINES][VITALS_COLS];

static int said(int n, const char *what)
{
    for (int i = 0; i < n; i++) {
        if (strstr(L[i], what) != NULL) {
            return 1;
        }
    }
    return 0;
}

/* The rule before this one, for the case it got wrong. */
static const char *old_rule(const vitals_t *p, int power_on)
{
    if (!p->deliberate) { return "STOPPED DEAD"; }
    return power_on ? "RESTART HUNG" : "restarted on purpose";
}

int main(void)
{
    vitals_t v;
    int n;

    memset(&v, 0, sizeof v);
    n = vitals_verdict(&v, false, L, VITALS_LINES);
    CHECK(n == 1 && said(n, "no previous run"), "no record: '%s'", L[0]);

    /* A serial run marks its start and writes nothing else; it ended by a flash
     * or a cable, not by choice, and nobody was watching it. */
    memset(&v, 0, sizeof v);
    v.valid = 1; v.usb_mode = 0; v.uptime_s = 5;
    n = vitals_verdict(&v, false, L, VITALS_LINES);
    CHECK(said(n, "serial, not watched") && !said(n, "STOPPED DEAD"),
          "a serial run with no goodbye: '%s'", L[0]);
    CHECK(strcmp(old_rule(&v, 0), "STOPPED DEAD") == 0,
          "and the old rule would have called it STOPPED DEAD");

    /* The hunt: a USB MIDI run whose record was never closed. */
    memset(&v, 0, sizeof v);
    v.valid = 1; v.usb_mode = 1; v.uptime_s = 600; v.loops = 118800;
    v.heap_free = 131072;
    n = vitals_verdict(&v, false, L, VITALS_LINES);
    CHECK(said(n, "STOPPED DEAD") && said(n, "198 loops/s") && said(n, "USB MIDI"),
          "a USB MIDI run that stopped: '%s' / '%s' / '%s'", L[0], L[1], L[2]);

    /* It said goodbye, and then a hand had to cycle the power. */
    v.deliberate = 1;
    n = vitals_verdict(&v, true, L, VITALS_LINES);
    CHECK(said(n, "RESTART HUNG"), "a goodbye, then a power cycle: '%s'", L[2]);
    n = vitals_verdict(&v, false, L, VITALS_LINES);
    CHECK(said(n, "restarted on purpose"), "a goodbye that worked: '%s'", L[2]);
    v.usb_mode = 0;
    n = vitals_verdict(&v, false, L, VITALS_LINES);
    CHECK(said(n, "restarted on purpose") && said(n, "serial"),
          "a deliberate restart from serial mode is still reported");

    /* The panel and '>jitter' are thirty columns; the verdict is the last line,
     * so a cut line loses exactly the part worth reading. */
    /* snprintf would cut silently, so the check is that each line still ENDS
     * the way it should - with a month of uptime and the largest counters. */
    memset(&v, 0, sizeof v);
    v.valid = 1; v.usb_mode = 1; v.uptime_s = 30u * 24 * 3600;
    v.loops = 4294967295u; v.heap_free = 4294967295u; v.deliberate = 1;
    n = vitals_verdict(&v, true, L, VITALS_LINES);
    const size_t l0 = strlen(L[0]), l1 = strlen(L[1]);
    CHECK(n <= VITALS_LINES && l0 >= 8 && strcmp(L[0] + l0 - 8, "USB MIDI") == 0 &&
          l1 >= 6 && strcmp(L[1] + l1 - 6, "K heap") == 0 &&
          strcmp(L[2], "  RESTART HUNG - see OS.md") == 0,
          "a month's uptime and the largest counters are not cut: '%s' / '%s'",
          L[0], L[1]);

    if (fails) {
        printf("[FAIL] %d check(s) failed\n", fails);
    } else {
        printf("[PASS] every verdict, including the two the bench cannot reach\n");
    }
    return fails != 0;
}
