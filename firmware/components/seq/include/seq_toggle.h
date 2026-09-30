/*
 * THE PERFORMANCE SWITCH WAITS FOR THE ONE.
 *
 * '>toggle bass bass:2' swaps two versions of a part, '>toggle kick hat'
 * takes a block out and brings it back - and a switch that lands mid-bar
 * lurches. So a toggle does not act when it is pressed: it leaves a change
 * pending on each lane, and the clock makes it on the first tick of the next
 * bar, before any lane fires (seq.c, tick). Pressing it again before the bar
 * takes the change back. Stopped, it acts at once, so a piece can set itself
 * up before '>play'. I said, 2026-09-29: variations to "switch back and
 * forth between live", and transitions that do not come in "at not correct
 * timing".
 *
 * Pure, so tools/test_toggle.c checks the rule the firmware runs.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define SEQ_TOGGLE_TICKS_PER_BAR (24 * 16)   /* SEQ_TICKS_PER_STEP x 16 */

/* A lane's pending change. Zero is none, so a new lane starts with none. */
enum { SEQ_PEND_NONE = 0, SEQ_PEND_UNMUTE = 1, SEQ_PEND_MUTE = 2 };

static inline bool seq_toggle_at_bar(uint32_t tick)
{
    return tick % SEQ_TOGGLE_TICKS_PER_BAR == 0;
}

/* Where a lane is going: pending, if anything is, else where it is. */
static inline bool seq_toggle_will_mute(bool muted, int pend)
{
    return pend == SEQ_PEND_NONE ? muted : pend == SEQ_PEND_MUTE;
}

/* What a toggle asks for: the opposite of where the lane is going. */
static inline int seq_toggle_next(bool muted, int pend)
{
    return seq_toggle_will_mute(muted, pend) ? SEQ_PEND_UNMUTE : SEQ_PEND_MUTE;
}

/* The clock, on a bar's first tick: make the change, and clear it. */
static inline bool seq_toggle_land(bool muted, int pend)
{
    return pend == SEQ_PEND_NONE ? muted : pend == SEQ_PEND_MUTE;
}
