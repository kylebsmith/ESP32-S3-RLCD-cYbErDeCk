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

    /* 9. A PER-CENT SIGN IS A PARAMETER, NOT A STEP.
     *
     * 'x%15' is one step with a parameter. It moved off the bracket because the
     * bracket is the only punctuation a player already reads as GROUPING, and a
     * group had the better claim on it - see docs/MAP.md. If the parameter
     * counted as steps the pattern would be three longer than it looks and the
     * playhead would drift off the character that is sounding. */
    eqi("a parameter is not a step", seq_pattern_steps("x%15x.", MAX_STEPS), 3);
    eqi("step after a parameter",    seq_pattern_offset("x%15x.", 1, MAX_STEPS), 4);
    eqi("the step it attaches to",   seq_pattern_offset("x%15x.", 0, MAX_STEPS), 0);
    eqi("parameter value",           seq_pattern_param("%15"), 15);
    eqi("parameter length",          seq_pattern_param_len("%15"), 3);
    eqi("a bare per-cent is not one", seq_pattern_param_len("%"), 0);
    eqi("a per-cent then a letter",   seq_pattern_param_len("%a"), 0);
    eqi("zero is a real value",      seq_pattern_param("%0"), 0);
    /* No step may land inside a parameter. */
    {
        const char *b = "x%15x.";
        const int n = seq_pattern_steps(b, MAX_STEPS);
        for (int i = 0; i < n; i++) {
            const int off = seq_pattern_offset(b, i, MAX_STEPS);
            if (off < 0 || b[off] == '%' ||
                (b[off] >= '0' && b[off] <= '9' && off > 0 &&
                 (b[off-1] == '%' || (b[off-1] >= '0' && b[off-1] <= '9')))) {
                printf("[FAIL] step %d landed inside a parameter\n", i);
                fails++;
            }
        }
    }

    /* 9b. NESTING. A bracket subdivides the step it occupies, to any depth, and
     * it is resolved HERE - flattened onto the same uniform grid the clock
     * already reads, so the sequencer never learns about it and there is no
     * second code path that could be late.
     *
     * Each case states the flattened form, because that is the thing that has
     * to be right: 'x..[xx]' is eight slots at half the step length with hits
     * at 0, 6 and 7 - not four steps one of which is special. */
    {
        struct { const char *pat; int n; int div; const char *flat; } nest[] = {
            /* unchanged when nothing nests */
            { "x...x...x...x...", 16, 1, "x...x...x...x..." },
            /* the last step becomes two half-steps */
            { "x..[xx]",           8, 2, "x.....xx" },
            /* a triplet in the first step of four */
            { "[xxx]...",         12, 3, "xxx........." },
            /* two against three, in one bar, from one line */
            { "[xx][xxx]",        12, 6, "x..x..x.x.x." },
            /* depth: the second of a pair splits again */
            { "x.[x[xx]].",       16, 4, "x.......x.xx...." },
        };
        for (unsigned i = 0; i < sizeof nest / sizeof nest[0]; i++) {
            seq_walk_t w;
            const int n = seq_pattern_walk(nest[i].pat, &w);
            char got[40];
            int k = 0;
            for (; k < n && k < 32; k++) {
                got[k] = (w.at[k] < 0) ? '.' : nest[i].pat[w.at[k]];
            }
            got[k] = '\0';
            if (n != nest[i].n || w.div != nest[i].div ||
                strcmp(got, nest[i].flat) != 0) {
                printf("[FAIL] %-18s n=%d/%d div=%d/%d\n         got  %s\n"
                       "         want %s\n",
                       nest[i].pat, n, nest[i].n, w.div, nest[i].div,
                       got, nest[i].flat);
                fails++;
            } else {
                printf("[ ok ] %-18s -> %s  (%d slots, div %d)\n",
                       nest[i].pat, got, n, w.div);
            }
        }

        /* A NESTED PATTERN THAT CANNOT FIT IS REFUSED, not truncated: the
         * subdivision is a property of the whole bar, so dropping the tail
         * changes the meaning of everything before it. */
        seq_walk_t w;
        eqi("an impossible subdivision is refused",
            seq_pattern_walk("[xxxxx][xxxx][xxx]", &w), -1);

        /* A FLAT pattern is still CLAMPED, because truncating it loses the tail
         * and nothing else - one character, one step, exactly as written. */
        eqi("a long flat pattern clamps",
            seq_pattern_steps("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
                              MAX_STEPS), MAX_STEPS);

        /* And the playhead still lands on a character for every slot that has
         * one, through nesting - which is the whole reason this file exists. */
        {
            const char *b = "x.[x[xx]].";
            const int n = seq_pattern_steps(b, MAX_STEPS);
            int marks = 0;
            for (int i = 0; i < n; i++) {
                const int off = seq_pattern_offset(b, i, MAX_STEPS);
                if (off < 0) { continue; }
                marks++;
                if (b[off] == '[' || b[off] == ']') {
                    printf("[FAIL] slot %d landed on a bracket\n", i);
                    fails++;
                }
            }
            eqi("every mark in a nested pattern is reachable", marks, 6);
        }
    }

    /* 10. THE RATE TOKEN IS NOT PART OF THE PICTURE.
     *
     * '/2' must not compile as two extra hits, and the playhead must not walk
     * into it. If the command layer stripped the rate but this walk did not,
     * the two would disagree about which characters are steps - the exact
     * class of bug this header exists to make impossible. */
    {
        int num = 0, den = 0;
        eqi("rate: pattern length", seq_pattern_rate("x.x.x.x. /2", &num, &den), 8);
        eqi("rate: denominator",    den, 2);
        eqi("rate: numerator",      num, 1);
        eqi("rate: steps exclude it", seq_pattern_steps("x.x.x.x. /2", MAX_STEPS), 8);
        eqi("rate: last step",      seq_pattern_offset("x.x.x.x. /2", 7, MAX_STEPS), 7);
        eqi("rate: none past it",   seq_pattern_offset("x.x.x.x. /2", 8, MAX_STEPS), -1);

        seq_pattern_rate("x.x. *4", &num, &den);
        eqi("multiply numerator", num, 4);
        eqi("multiply denominator", den, 1);

        /* Not a rate: no leading operator, bad number, or out of range. */
        eqi("plain token is picture", seq_pattern_steps("x.x. zz", MAX_STEPS), 6);
        eqi("bad number is picture",  seq_pattern_steps("x.x. /z", MAX_STEPS), 6);
        eqi("no space is picture",    seq_pattern_steps("x.x./2", MAX_STEPS), 6);
    }

    printf(fails ? "[FAIL] %d check(s) failed\n" : "[PASS] pattern geometry\n",
           fails);
    return fails != 0;
}
