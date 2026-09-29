/*
 * '>toggle' lands on the one - checked with firmware/components/seq/include/
 * seq_toggle.h, the rule the clock runs (seq.c, tick). A lane model plays bars
 * of ticks; a toggle pressed mid-bar must change nothing until the next bar's
 * first tick, a second press before then must take it back, and an A/B pair
 * swapped with one toggle must swap on the same tick, both ways.
 */
#include <stdio.h>
#include "seq_toggle.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf("[FAIL] " __VA_ARGS__); printf("\n"); fails++; } \
                           else { printf("[ ok ] " __VA_ARGS__); printf("\n"); } } while (0)

typedef struct { int muted, pend; } lane_t;

static void press(lane_t *l, int running)
{
    const int next = seq_toggle_next(l->muted, l->pend);
    if (!running) { l->muted = (next == SEQ_PEND_MUTE); l->pend = SEQ_PEND_NONE; }
    else l->pend = next;
}

static void tick(lane_t *ls, int n, unsigned t)
{
    if (!seq_toggle_at_bar(t)) return;
    for (int i = 0; i < n; i++) {
        ls[i].muted = seq_toggle_land(ls[i].muted, ls[i].pend);
        ls[i].pend = SEQ_PEND_NONE;
    }
}

int main(void)
{
    CHECK(seq_toggle_at_bar(0) && seq_toggle_at_bar(384) && seq_toggle_at_bar(768),
          "a bar is 16 steps of 24 ticks: 0, 384, 768");
    CHECK(!seq_toggle_at_bar(383) && !seq_toggle_at_bar(385) && !seq_toggle_at_bar(24),
          "and nothing between");

    /* 1. A press at tick 100 changes nothing until 384. */
    lane_t a = { 0, SEQ_PEND_NONE };
    int changed_at = -1;
    for (unsigned t = 0; t < 1000; t++) {
        if (t == 100) press(&a, 1);
        const int was = a.muted;
        tick(&a, 1, t);
        if (a.muted != was && changed_at < 0) changed_at = (int)t;
    }
    CHECK(changed_at == 384 && a.muted, "pressed at tick 100, the lane goes silent at 384 (%d)", changed_at);

    /* 2. Pressed twice in one bar: nothing happens. */
    lane_t b = { 0, SEQ_PEND_NONE };
    int moved = 0;
    for (unsigned t = 400; t < 1200; t++) {
        if (t == 500 || t == 700) press(&b, 1);
        const int was = b.muted;
        tick(&b, 1, t);
        moved |= (b.muted != was);
    }
    CHECK(!moved && !b.muted, "two presses in a bar take each other back");

    /* 3. An A/B pair, B silent: one toggle swaps them on the same tick, and
     *    the same toggle swaps them back a bar later. */
    lane_t ab[2] = { { 0, SEQ_PEND_NONE }, { 1, SEQ_PEND_NONE } };
    int swap1 = -1, swap2 = -1;
    for (unsigned t = 0; t < 2000; t++) {
        if (t == 50 || t == 900) { press(&ab[0], 1); press(&ab[1], 1); }
        const int a0 = ab[0].muted, b0 = ab[1].muted;
        tick(ab, 2, t);
        if (ab[0].muted != a0 || ab[1].muted != b0) {
            CHECK(ab[0].muted != a0 && ab[1].muted != b0 && ab[0].muted != ab[1].muted,
                  "at tick %u both change together, one on and one off", t);
            if (swap1 < 0) swap1 = (int)t; else swap2 = (int)t;
        }
    }
    CHECK(swap1 == 384 && swap2 == 1152, "swapped at 384 and back at 1152 (%d, %d)", swap1, swap2);

    /* 4. Stopped, a toggle acts at once, so a piece sets itself up before play. */
    lane_t c = { 0, SEQ_PEND_NONE };
    press(&c, 0);
    CHECK(c.muted && c.pend == SEQ_PEND_NONE, "stopped, it is silent at once");
    press(&c, 0);
    CHECK(!c.muted, "and back at once");

    /* 5. A lane made after the press never inherits it: a new slot is zeroed. */
    CHECK(SEQ_PEND_NONE == 0, "no change pending is zero, what a new lane starts with");

    printf(fails ? "[FAIL] %d check(s) failed\n" : "[PASS] toggle lands on the one\n", fails);
    return fails != 0;
}
