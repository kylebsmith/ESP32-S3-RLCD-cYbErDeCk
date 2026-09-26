/*
 * The ticks follow the grid - checked by simulation, with the same
 * firmware/components/seq/include/seq_clock.h the clock schedules with.
 *
 * Two decks at 124 bpm: a leader, and a follower whose crystal runs 2 ppm fast,
 * which is what two of these boards measured against each other on 2026-09-25.
 * The follower starts a millisecond out of phase and corrects its grid toward
 * the leader's as seq_nudge does - an eighth of the error per packet, eight
 * packets a second - measuring perfectly, so only the clock is on trial. The
 * timer is modelled as firing exactly when armed.
 *
 * THE OLD CLOCK, periodic, never moved its ticks: the corrections moved only the
 * grid. Its follower's grid agrees with the leader's to a microsecond while its
 * ticks drift away at 2 us a second. The check below fails on that model, by
 * name, so it can tell the two apart; then it demands the new one stay close.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "seq_clock.h"
#include "ens_count.h"

static int fails;

#define CHECK(cond, ...) do { if (!(cond)) { printf("[FAIL] " __VA_ARGS__); \
    printf("\n"); fails++; } else { printf("[ ok ] " __VA_ARGS__); printf("\n"); } } while (0)

#define PER      5040           /* period_us() at 124 bpm */
#define PPM      2.0
#define HOUR_US  3600000000.0
#define PACKET   125000.0       /* leader microseconds between corrections */

/* Leader time to follower time and back: the follower's microseconds are
 * shorter by PPM. */
static double to_local(double t)  { return t * (1.0 + PPM * 1e-6); }
static double to_leader(double u) { return u / (1.0 + PPM * 1e-6); }

typedef struct {
    double worst_tick;          /* |follower tick - leader tick|, leader us */
    double worst_grid;          /* |follower grid - leader grid|            */
} result_t;

/* One hour. `on_grid` = the new clock; otherwise the old periodic one. */
static result_t run(int on_grid)
{
    result_t r = { 0, 0 };
    const double start = 1234.0;               /* follower's play, leader us */
    const double first = to_local(start);      /* its first tick, local us   */
    int64_t grid = (int64_t)llround(first);    /* anchored on the first tick */
    double tick_at = first;                    /* local time of tick k       */
    double next_packet = PACKET;
    for (uint32_t k = 0; ; k++) {
        const double lead = (double)k * PER;   /* the leader's tick k */
        if (lead > HOUR_US) {
            break;
        }
        /* The follower's tick k, and how far it is from the leader's. */
        const double at = to_leader(tick_at);
        if (lead > 10e6) {                     /* after ten seconds to settle */
            const double e = fabs(at - lead);
            if (e > r.worst_tick) { r.worst_tick = e; }
            const double ge = fabs(to_leader((double)grid + (double)k * PER) - lead);
            if (ge > r.worst_grid) { r.worst_grid = ge; }
        }
        /* Corrections that arrive before the next tick. */
        while (next_packet <= to_leader(tick_at)) {
            const uint32_t K = (uint32_t)(next_packet / PER);
            const double due_local = to_local((double)K * PER);
            double err = due_local - ((double)grid + (double)K * PER);
            while (err >  PER / 2) { err -= PER; }
            while (err < -PER / 2) { err += PER; }
            grid += (int64_t)(err / 8);
            next_packet += PACKET;
        }
        /* The next tick. */
        if (on_grid) {
            const int64_t now = (int64_t)llround(tick_at);
            tick_at += (double)seq_clock_wait(now, now, true, grid, k + 1, PER);
        } else {
            tick_at = first + (double)(k + 1) * PER;
        }
    }
    return r;
}

int main(void)
{
    const result_t old = run(0);
    const result_t now = run(1);
    printf("       periodic timer:  grid within %.1f us, ticks within %.1f us\n",
           old.worst_grid, old.worst_tick);
    printf("       ticks on grid:   grid within %.1f us, ticks within %.1f us\n",
           now.worst_grid, now.worst_tick);
    CHECK(old.worst_grid < 20.0 && old.worst_tick > 5000.0,
          "the old clock fails as measured: grids agree, ticks end the hour %.1f ms apart",
          old.worst_tick / 1000.0);
    CHECK(now.worst_tick < 20.0,
          "ticks on the grid stay within %.1f us of the leader's for an hour",
          now.worst_tick);

    /* A new tempo takes over from the last tick: the next one is due exactly
     * one new period after it, whatever the old grid was. */
    const int64_t last = 987654321, np = 4807;           /* 130 bpm */
    const uint32_t next = 12345;
    const int64_t g = seq_clock_reanchor(last, next, np);
    CHECK(g + (int64_t)next * np == last + np,
          "after a tempo change the next tick is due one new period after the last");
    /* The anchor this replaced said "due now" and restarted a periodic timer,
     * which fired a period later: the tick a whole pulse off its own grid. */
    const int64_t pressed = last + 1800;
    const int64_t old_g = pressed - (int64_t)next * np;
    const int64_t old_fires = pressed + np;
    CHECK(old_fires - (old_g + (int64_t)next * np) == np,
          "the old anchor put the tick %lld us off its grid (4821 measured at 130)",
          (long long)np);

    /* A grid slid past now fires at once and late, never early or not at all. */
    CHECK(seq_clock_wait(1000, 1000, true, 0, 0, PER) == SEQ_WAIT_MIN_US,
          "a tick already due fires in %d us", SEQ_WAIT_MIN_US);
    CHECK(seq_clock_wait(1000, 900, false, 0, 7, PER) == 900 + PER - 1000,
          "a stopped clock keeps its period, for the note-offs");

    /* ---- the count, which the phase cannot see (ens_count.h) ------------ */
    /* The leader is at pulse T at `their`; this deck's count is D behind, and
     * its pulses sit `jit` microseconds off the leader's. */
    const int64_t their = 5000000000LL;
    const uint32_t T = 100000;
    const int32_t cases[] = { 14, 3, 4, 0, -5000, 383, 24 };
    const int64_t jits[] = { 0, 37, -37, PER / 2 - 1, -(PER / 2 - 1) };
    int right = 0, total = 0;
    for (size_t c = 0; c < sizeof cases / sizeof cases[0]; c++) {
        for (size_t j = 0; j < sizeof jits / sizeof jits[0]; j++) {
            const uint32_t t = 7;                 /* our next pulse */
            const int64_t ours = their - (int64_t)(T - cases[c] - t) * PER + jits[j];
            const int64_t ph = ens_phase(their, ours, PER);
            right += ens_count_ahead(T, their, t, ours, PER, ph) == cases[c];
            total++;
        }
    }
    CHECK(right == total, "the count between two decks is exact, %d of %d, "
          "with the pulse up to half a pulse off", right, total);
    const int64_t o14 = their - (int64_t)(T - 14 - 7) * PER + 37;
    const int64_t o38 = their - (int64_t)(T - 38 - 7) * PER + 37;
    CHECK(ens_phase(their, o14, PER) == ens_phase(their, o38, PER),
          "and the phase alone - all the ensemble used - cannot tell 14 pulses "
          "from 38");

    if (fails) {
        printf("[FAIL] %d check(s) failed\n", fails);
    } else {
        printf("[PASS] the ticks follow the grid, in the leader's count\n");
    }
    return fails != 0;
}
