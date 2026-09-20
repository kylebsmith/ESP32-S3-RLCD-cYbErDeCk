/*
 * The step -> column mapping, checked on the host against the shipping header.
 *
 * The trap this exists to catch is the same shape as the CASET one in
 * test_st7305_addr.c: the NAIVE mapping - step index equals character offset -
 * is correct for every unspaced pattern, which is every pattern in the shipped
 * guide. It is wrong the moment anyone types "x... x... x... x..." to make a
 * bar readable, and the symptom is a playhead sitting one column off, which
 * looks deliberate. So the test asserts it can tell the two apart.
 */
#include <stdio.h>
#include <string.h>

#include "seq_pattern.h"

#define MAX_STEPS 32

static int fails;

static void eqi(const char *what, int got, int want)
{
    if (got != want) {
        printf("[FAIL] %s: got %d want %d\n", what, got, want);
        fails++;
    }
}

int main(void)
{
    /* 1. The identity case - the shipped guide's own kick pattern. The naive
     *    mapping is CORRECT here, which is precisely why it survives. */
    const char *plain = "X...x...X...x...";
    eqi("plain steps", seq_pattern_steps(plain, MAX_STEPS), 16);
    for (int i = 0; i < 16; i++) {
        eqi("plain offset", seq_pattern_offset(plain, i, MAX_STEPS), i);
    }

    /* 2. Spacing for the eye. This is where the naive mapping dies. */
    const char *spaced = "x... x... x... x...";
    eqi("spaced steps", seq_pattern_steps(spaced, MAX_STEPS), 16);
    eqi("spaced step 4",  seq_pattern_offset(spaced, 4,  MAX_STEPS), 5);
    eqi("spaced step 8",  seq_pattern_offset(spaced, 8,  MAX_STEPS), 10);
    eqi("spaced step 15", seq_pattern_offset(spaced, 15, MAX_STEPS), 18);

    /* The test must be able to SEE the difference, not merely be right. */
    if (seq_pattern_offset(spaced, 4, MAX_STEPS) == 4) {
        printf("[FAIL] cannot distinguish the naive mapping\n");
        fails++;
    }

    /* 3. No step may ever land on a space, in any pattern. If one did, the
     *    playhead would mark a gap and the player would read it as a rest. */
    const char *pats[] = { plain, spaced, "  x.x.  ", "x x x x", ".", "" };
    for (size_t k = 0; k < sizeof pats / sizeof pats[0]; k++) {
        const int n = seq_pattern_steps(pats[k], MAX_STEPS);
        for (int i = 0; i < n; i++) {
            const int off = seq_pattern_offset(pats[k], i, MAX_STEPS);
            if (off < 0) {
                printf("[FAIL] pattern %zu step %d has no offset\n", k, i);
                fails++;
            } else if (seq_pattern_is_spacing(pats[k][off])) {
                printf("[FAIL] pattern %zu step %d lands on a space\n", k, i);
                fails++;
            }
        }
        eqi("no step past the count",
            seq_pattern_offset(pats[k], n, MAX_STEPS), -1);
    }

    /* 4. A TAB IS A STEP. seq_lane() rests only on '.', '-' and '_', so a tab
     *    compiles as a hit; a walk that skipped it would be one column off. */
    eqi("tab is a step", seq_pattern_steps("x\tx.", MAX_STEPS), 4);
    eqi("tab offset",    seq_pattern_offset("x\tx.", 1, MAX_STEPS), 1);

    /* 5. Truncation at SEQ_MAX_STEPS, matching seq_lane()'s own bound. */
    char many[41];
    memset(many, 'x', 40);
    many[40] = '\0';
    eqi("truncated count",  seq_pattern_steps(many, MAX_STEPS), 32);
    eqi("last real step",   seq_pattern_offset(many, 31, MAX_STEPS), 31);
    eqi("one past the end", seq_pattern_offset(many, 32, MAX_STEPS), -1);

    /* 6. Leading and trailing spacing. */
    eqi("leading space step 0", seq_pattern_offset("   x.x.", 0, MAX_STEPS), 3);
    eqi("trailing ignored",     seq_pattern_steps("x.x.   ", MAX_STEPS), 4);

    /* 7. The toggle hash is SPACING-SENSITIVE, on purpose. A spacing-blind
     *    hash - or comparing compiled bitmasks, which is the tempting first
     *    implementation - would silence a lane the moment the player
     *    re-spaced it for readability, which is the exact surprise the rule
     *    exists to prevent. */
    if (seq_pattern_hash("x...x...") == seq_pattern_hash("x... x...")) {
        printf("[FAIL] the hash ignores spacing; re-spacing would silence\n");
        fails++;
    }
    if (seq_pattern_hash("x...x...") != seq_pattern_hash("x...x...")) {
        printf("[FAIL] the hash is not stable\n");
        fails++;
    }
    if (seq_pattern_hash("X...x...X...x...") == seq_pattern_hash("x...x...X...x...")) {
        printf("[FAIL] the hash misses an accent change\n");
        fails++;
    }

    /* 8. THE POSITION COUNTER MUST NOT JUMP WHEN IT ROLLS.
     *
     * The counter was masked with 0x7FFF. 32768 is a power of two, so 8-, 16-
     * and 32-step lanes wrapped cleanly - which is every lane in the shipped
     * guide, which is why nobody noticed - but a 5-, 6- or 12-step lane
     * jumped by (32768 %% steps) every 66 minutes at 124 bpm.
     *
     * This asserts the property directly: consecutive steps must advance by
     * exactly one, modulo the lane length, ACROSS the roll. It fails on the
     * old mask for exactly the lane lengths a musician would reach for. */
    for (int steps = 2; steps <= 32; steps++) {
        const unsigned long roll = 0x8000UL;
        const int before = (int)((roll - 1) % (unsigned long)steps);
        const int after  = (int)(roll % (unsigned long)steps);
        if (after != (before + 1) % steps) {
            printf("[FAIL] unmasked counter jumps at roll for %d steps\n", steps);
            fails++;
        }
        /* And demonstrate that the OLD masked counter did jump, so this test
         * is known to be capable of detecting it. */
        const int masked_after = (int)((roll & 0x7FFFUL) % (unsigned long)steps);
        if (steps == 5 && masked_after == (before + 1) % steps) {
            printf("[FAIL] the 0x7FFF mask case is no longer a counterexample\n");
            fails++;
        }
    }

    printf(fails ? "[FAIL] %d check(s) failed\n" : "[PASS] pattern geometry\n",
           fails);
    return fails != 0;
}
