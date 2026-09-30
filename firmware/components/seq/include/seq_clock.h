/*
 * When the next tick fires. Pure, so tools/test_clock.c runs this exact code.
 *
 * THE GRID IS THE CLOCK. The timer was periodic, and everything that moved the
 * ideal grid - a follower's corrections toward the ensemble, a new tempo - moved
 * only the bookkeeping: the ticks went on firing at the timer's own phase, on
 * the deck's own crystal. Measured 2026-09-25 on two decks: the ensemble
 * reported the two grids 3 us apart while the follower's ticks sat 146-388 us
 * from its own grid and drifted 2 us a second - 7 ms an hour. And after '>bpm'
 * a deck's ticks sat a whole pulse (4.8 ms) behind the grid it broadcast.
 *
 * So each tick arms the next at the moment the grid says it is due. A grid that
 * never moves - a leader, a deck alone - makes that exactly the periodic timer
 * it replaced. A grid that moves takes the ticks with it, a fraction of an error
 * at a time: the "trimming its own period" that seq.h always described.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* A tick the grid wants sooner than this fires this late instead - and the
 * clock statistic, which measures against the grid, says so. */
#define SEQ_WAIT_MIN_US 50

#define SEQ_CLOCK_PPQN 96              /* seq.h SEQ_PPQN */

/* WHEN TICK n IS DUE, from the grid's origin: n x 60,000,000 / (bpm x 96)
 * microseconds, worked out whole every time and rounded once.
 *
 * It was n x a period rounded DOWN to a whole microsecond - 5,040 us at 124
 * bpm where the true period is 5,040.32 - so the deck played 124.008 bpm,
 * 132.024 at 132 and 165.038 at 165: 64, 179 and 232 parts per million fast,
 * 4, 11 and 14 ms a minute against a DAW at the tempo on the screen, and the
 * loops walked away from it as the track went on. The jitter statistic
 * measured against that same grid, and every host capture fitted its tempo
 * to the notes, so neither could see it (tools/clock_audit.py can). Now the
 * only error is the crystal's. */
static inline int64_t seq_clock_at(uint32_t n, int bpm)
{
    const int64_t den = (int64_t)(bpm > 0 ? bpm : 1) * SEQ_CLOCK_PPQN;
    return ((int64_t)n * 60000000LL + den / 2) / den;
}

/* How long from `now` to the next tick. On the grid when the clock is running
 * and the grid is anchored; otherwise one period after this tick began, which
 * keeps note-offs draining at the same rate while stopped. */
static inline int64_t seq_clock_wait(int64_t now, int64_t tick_began,
                                     bool on_grid, int64_t grid_t0,
                                     uint32_t next_tick, int bpm)
{
    const int64_t due = on_grid ? grid_t0 + seq_clock_at(next_tick, bpm)
                                : tick_began + seq_clock_at(1, bpm);
    const int64_t wait = due - now;
    return wait < SEQ_WAIT_MIN_US ? SEQ_WAIT_MIN_US : wait;
}

/* A new tempo, mid-bar: the grid is re-anchored so that the next tick is due one
 * NEW period after the last one fired. The bar carries on where it was, and the
 * only interval that changes at the boundary is the one the tempo change is. */
static inline int64_t seq_clock_reanchor(int64_t last_tick_us,
                                         uint32_t next_tick, int new_bpm)
{
    return last_tick_us + seq_clock_at(1, new_bpm) - seq_clock_at(next_tick, new_bpm);
}
