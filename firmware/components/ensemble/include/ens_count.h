/*
 * Where two decks are in the COUNT, not only in the pulse. Pure, so
 * tools/test_clock.c runs this exact code.
 *
 * THE PHASE CANNOT SEE A WHOLE PULSE. The ensemble folded every disagreement
 * into plus or minus half a pulse and said so - "a whole-pulse disagreement is a
 * different bar, not a phase error" - and then never dealt with the different
 * bar. Each deck counts pulses from its own '>play', a step is 24 of them, so two
 * decks started by two players shared a tempo and a pulse, within tens of
 * microseconds, while their sixteenths fell wherever the second '>play' happened
 * to land: 3, 4 and 14 pulses apart on three tries, 2026-09-25 - up to 71 ms,
 * and the Mac heard the kicks 77 ms apart.
 */
#pragma once

#include <stdint.h>

/* The leader's pulse `their_tick` is due at `their_due`, in our clock; our pulse
 * `our_tick` is due at `our_due`. Returns how many pulses the leader's count is
 * ahead of ours at the same instant. `phase` is the sub-pulse part already folded
 * into +/- half a pulse, which is what the phase correction uses. */
static inline int32_t ens_count_ahead(uint32_t their_tick, int64_t their_due,
                                      uint32_t our_tick, int64_t our_due,
                                      int64_t per, int64_t phase)
{
    const int64_t between = (their_due - our_due - phase) / per;
    return (int32_t)((int64_t)their_tick - (int64_t)our_tick - between);
}

/* The sub-pulse part, folded - the phase correction's own arithmetic. */
static inline int64_t ens_phase(int64_t their_due, int64_t our_due, int64_t per)
{
    int64_t off = (their_due - our_due) % per;
    if (off >  per / 2) { off -= per; }
    if (off < -per / 2) { off += per; }
    return off;
}
