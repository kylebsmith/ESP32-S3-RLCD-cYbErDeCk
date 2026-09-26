/*
 * The pattern compiler, checked on the host against the shipping header.
 *
 * Three jobs, and each one is the kind of mistake that looks deliberate when it
 * happens on the glass:
 *
 *  - WHERE a step is: the playhead has to sit on the characters that are
 *    sounding. The naive mapping - step index equals character offset - is
 *    right for every unspaced pattern and wrong the moment anyone types
 *    "x... x... x... x..." to make a bar readable. So the test asserts it can
 *    tell the two apart.
 *  - WHAT a step is: after docs/MANIFESTO.md §3.6 a step is a head character
 *    plus modifiers, and anything that is not a step is REFUSED with its
 *    position (§3.2) instead of being played as a hit.
 *  - WHEN a step is: every slot must land where the arithmetic says, including
 *    the five- and seven-way splits the old integer ticks-per-slot got wrong.
 */
#include <stdio.h>
#include <string.h>

#include "seq_pattern.h"

static int fails;

static void eqi(const char *what, int got, int want)
{
    if (got != want) {
        printf("[FAIL] %s: got %d want %d\n", what, got, want);
        fails++;
    }
}

static seq_comp_t C;

/* Compile and render one cycle as a string of what starts on each slot:
 * the head character, '.' for a rest, '_' for a tie, ' ' for nothing. */
static const char *render(const char *pat, uint32_t cycle)
{
    static char out[SEQ_PATTERN_MAX_SLOTS + 1];
    if (seq_pattern_compile(pat, &C) != SEQ_PAT_OK) {
        snprintf(out, sizeof out, "ERR%d@%d", C.err, C.err_at);
        return out;
    }
    memset(out, ' ', (size_t)C.slots);
    out[C.slots] = '\0';
    for (int i = 0; i < C.n; i++) {
        const seq_leaf_t *L = &C.leaf[i];
        if (L->per > 1 && (int)(cycle % L->per) != L->ph) { continue; }
        out[L->slot] = pat[L->at];
    }
    return out;
}

static void render_is(const char *pat, uint32_t cycle, const char *want)
{
    const char *got = render(pat, cycle);
    if (strcmp(got, want) != 0) {
        printf("[FAIL] %-22s cycle %u\n         got  '%s'\n         want '%s'\n",
               pat, (unsigned)cycle, got, want);
        fails++;
    } else {
        printf("[ ok ] %-22s c%u -> '%s'\n", pat, (unsigned)cycle, got);
    }
}

/* The error a pattern is refused with, and the character it points at. */
static void refused(const char *pat, int err, int at)
{
    const int e = seq_pattern_compile(pat, &C);
    char msg[64];
    seq_pattern_error_text(&C, pat, msg, sizeof msg);
    if (e != err || C.err_at != at) {
        printf("[FAIL] refuse %-18s got err %d at %d, want %d at %d (%s)\n",
               pat, e, C.err_at, err, at, msg);
        fails++;
    } else {
        printf("[ ok ] refuse %-18s at %2d: %s\n", pat, at, msg);
    }
}

static const seq_leaf_t *leaf_at_char(const char *pat, int at)
{
    for (int i = 0; i < C.n; i++) {
        if (C.leaf[i].at == at) { return &C.leaf[i]; }
    }
    printf("[FAIL] %s: no leaf at character %d\n", pat, at);
    fails++;
    return &C.leaf[0];
}

int main(void)
{
    /* 0. The header's tick count must be seq.h's. It is duplicated so the
     *    header can stay dependency-free; this is what keeps the copies equal. */
    eqi("ticks per step matches seq.h", SEQ_PATTERN_TICKS_PER_STEP, 24);

    /* 1. The identity case. The naive mapping is CORRECT here, which is
     *    precisely why it survives. */
    render_is("x...x...x...x...", 0, "x...x...x...x...");
    eqi("plain slots", seq_pattern_steps("x...x...x...x...", 64), 16);

    /* 2. Spacing for the eye. This is where the naive mapping dies. */
    {
        const char *s = "x... x... x... x...";
        seq_pattern_compile(s, &C);
        eqi("spaced slots", C.slots, 16);
        int a, b;
        seq_pattern_mark(&C, 4, 0, &a, &b);
        eqi("spaced step 4 is character 5", a, 5);
        seq_pattern_mark(&C, 15, 0, &a, &b);
        eqi("spaced step 15 is character 18", a, 18);
        if (a == 15) {
            printf("[FAIL] cannot distinguish the naive mapping\n");
            fails++;
        }
    }

    /* 3. No step may land on a space, in any pattern. */
    {
        const char *pats[] = { "x... x...", "  x.x.  ", "x x x x", "[x x] . 3" };
        for (size_t k = 0; k < sizeof pats / sizeof pats[0]; k++) {
            seq_pattern_compile(pats[k], &C);
            for (int i = 0; i < C.n; i++) {
                if (seq_pattern_is_spacing(pats[k][C.leaf[i].at])) {
                    printf("[FAIL] %s leaf %d lands on a space\n", pats[k], i);
                    fails++;
                }
            }
        }
    }

    /* 4. A TAB IS SPACING NOW. It used to compile as a hit, because anything
     *    that was not a rest was a hit - the rule §3.2 retires. */
    eqi("tab is spacing", seq_pattern_steps("x\tx.", 64), 3);

    /* 5. THE DIGIT IS THE STEP'S AMOUNT, and 'x' is the lane's own level. */
    {
        seq_pattern_compile("9..3x", &C);
        eqi("9 is 9", leaf_at_char("9..3x", 0)->val, 9);
        eqi("3 is 3", leaf_at_char("9..3x", 3)->val, 3);
        eqi("x is the lane's level", leaf_at_char("9..3x", 4)->val, SEQ_VAL_X);
    }

    /* 6. A MODIFIER IS PART OF THE STEP. 'x%15' is one step, the playhead
     *    lights all four characters, and '%0' means never. */
    {
        const char *b = "x%15x.";
        seq_pattern_compile(b, &C);
        eqi("a modifier is not a step", C.slots, 3);
        eqi("its odds", leaf_at_char(b, 0)->prob, 15);
        eqi("no odds on the next", leaf_at_char(b, 4)->prob, SEQ_PROB_ALWAYS);
        int a = -1, e = -1;
        seq_pattern_mark(&C, 0, 0, &a, &e);
        eqi("the span starts on the x", a, 0);
        eqi("and ends after the 5", e, 4);
        seq_pattern_mark(&C, 1, 0, &a, &e);
        eqi("the next step starts after it", a, 4);
        seq_pattern_compile("x%0", &C);
        eqi("zero is a real value", C.leaf[0].prob, 0);
        /* Odds on a group apply to what is inside it, and multiply. */
        seq_pattern_compile("[x%50x]%50", &C);
        eqi("odds multiply", C.leaf[0].prob, 25);
        eqi("a group's odds reach its members", C.leaf[1].prob, 50);
    }

    /* 7. NESTING, unchanged in meaning. Each case states what starts on each
     *    slot, because that is the thing that has to be right. */
    render_is("x..[xx]",      0, "x . . xx");
    render_is("[xxx]...",     0, "xxx.  .  .  ");
    render_is("[xx][xxx]",    0, "x  x  x x x ");
    render_is("x.[x[xx]].",   0, "x   .   x xx.   ");

    /* 8. A CHORD. ',' stacks inside brackets: every member sounds at once, in
     *    the same step. It used to be a ghost note, which is why a pad could
     *    not play a triad. */
    {
        const char *p = "[0,4,7]...";
        seq_pattern_compile(p, &C);
        eqi("a chord is one step", C.steps, 4);
        eqi("of one slot each", C.slots, 4);
        int at0 = 0;
        for (int i = 0; i < C.n; i++) {
            if (C.leaf[i].kind == SEQ_LEAF_HIT && C.leaf[i].slot == 0) { at0++; }
        }
        eqi("three notes start together", at0, 3);
        int a, b;
        seq_pattern_mark(&C, 0, 0, &a, &b);
        eqi("the playhead lights from the first note", a, 1);
        eqi("to the last", b, 6);
        /* Members are sequences, so a stack can move inside its step: 0 and 4
         * start together, then 2 and 5. */
        seq_pattern_compile("[02,45]", &C);
        eqi("[02,45] is two slots", C.slots, 2);
        int first = 0, second = 0;
        for (int i = 0; i < C.n; i++) {
            const char ch = "[02,45]"[C.leaf[i].at];
            if (C.leaf[i].slot == 0) { first  += (ch == '0' || ch == '4'); }
            if (C.leaf[i].slot == 1) { second += (ch == '2' || ch == '5'); }
        }
        eqi("0 and 4 together", first, 2);
        eqi("then 2 and 5", second, 2);
    }

    /* 9. A TIE. '_' holds the note before it, so note length is finally
     *    expressible: '0__.' is one note three steps long. */
    {
        const char *p = "0__.3_.5";
        seq_pattern_compile(p, &C);
        eqi("the 0 lasts three steps", leaf_at_char(p, 0)->len, 3);
        eqi("the 3 lasts two",          leaf_at_char(p, 4)->len, 2);
        eqi("the 5 lasts one",          leaf_at_char(p, 7)->len, 1);
        /* the playhead still walks the tie: it marks the STEP */
        int a, b;
        seq_pattern_mark(&C, 1, 0, &a, &b);
        eqi("the playhead sits on the tie", a, 1);
        /* a tie after a group holds the group's last note, not the group */
        seq_pattern_compile("[xx]_", &C);
        eqi("[xx]_ holds the second x", C.leaf[1].len, 3);
        eqi("and not the first", C.leaf[0].len, 1);
        /* a tie after a chord holds all of it */
        seq_pattern_compile("[0,4]_", &C);
        eqi("a tied chord's first note", C.leaf[0].len, 2);
        eqi("and its second", C.leaf[1].len, 2);
        /* a tie inside one member of a stack holds only that member */
        seq_pattern_compile("[03,4_]", &C);
        eqi("the other member is not held", leaf_at_char("[03,4_]", 1)->len, 1);
        eqi("its own member is",            leaf_at_char("[03,4_]", 4)->len, 2);
        /* after a rest a tie holds nothing, and is a rest */
        seq_pattern_compile("x._", &C);
        eqi("x before a rest is not held", C.leaf[0].len, 1);
    }

    /* 10. ALTERNATION costs no slots now: it is a cycle class on each leaf.
     *     A sixteen-step lane that alternates is still sixteen slots. */
    render_is("x<3 5>", 0, "x3");
    render_is("x<3 5>", 1, "x5");
    render_is("<x .>",  1, ".");
    render_is("0...<3 5>...", 1, "0...5...");
    render_is("[x<x .>]", 1, "x.");
    render_is("<[xx] x>", 0, "xx");
    render_is("<[xx] x>", 1, "x ");
    render_is("<a b><c d e>", 0, "ERR2@1");      /* a, b, c, e are not steps */
    render_is("<0 1><2 3 4>", 4, "03");
    /* NESTED ALTERNATION ADVANCES ONLY WHEN IT IS CHOSEN - a b a c, not a c a c.
     * The old flattening passed the cycle down unchanged and played a c a c,
     * which is not what Strudel does; the corpus in tools/corpus found it. */
    render_is("<0 <1 2>>", 0, "0");
    render_is("<0 <1 2>>", 1, "1");
    render_is("<0 <1 2>>", 2, "0");
    render_is("<0 <1 2>>", 3, "2");
    {
        seq_pattern_compile("<0 <1 2>>", &C);
        eqi("<0 <1 2>> repeats every four", C.per, 4);
        /* THE CLIFF IS GONE (§3.9). This needed 66 slots flattened and was
         * refused at 64; it is 33 slots and two cycles now. */
        const char *cliff = "x...x...x...x...x...x...x...x...<3 5>";
        eqi("a long lane may alternate", seq_pattern_compile(cliff, &C), SEQ_PAT_OK);
        eqi("and costs its own length", C.slots, 33);
        /* odds travel with the alternative */
        const char *b = "<x%15 x%90>";
        seq_pattern_compile(b, &C);
        eqi("first alternative's odds",  leaf_at_char(b, 1)->prob, 15);
        eqi("second alternative's odds", leaf_at_char(b, 6)->prob, 90);
        /* A tie that would hold a note on only some of its bars is refused. */
        refused("0<_ .>", SEQ_PAT_TIE_ALT, 2);
        /* ...but a tie OUTSIDE holds whichever alternative played */
        seq_pattern_compile("<0 3>_", &C);
        eqi("<0 3>_ holds the 0", C.leaf[0].len, 2);
        eqi("and the 3",          C.leaf[1].len, 2);
    }

    /* 11. REFUSED, WITH THE CHARACTER. Every one of these used to play:
     *     anything that was not a rest was a hit, and an unclosed bracket was
     *     absorbed into a nonsense subdivision. */
    refused("x...x...x;..",   SEQ_PAT_BAD_CHAR, 9);
    refused("[x.x.x.x.",      SEQ_PAT_OPEN, 0);
    refused("x.x.]",          SEQ_PAT_CLOSE, 4);
    refused("[x.x.>",         SEQ_PAT_MISMATCH, 5);
    refused("x[]x",           SEQ_PAT_EMPTY_GROUP, 2);
    refused("[x,]",           SEQ_PAT_EMPTY_GROUP, 3);
    refused("0,4,7",          SEQ_PAT_COMMA, 1);
    refused("x%",             SEQ_PAT_PERCENT, 1);
    refused("x%101",          SEQ_PAT_PERCENT, 1);
    refused("x%1000",         SEQ_PAT_PERCENT, 1);
    refused("_x..",           SEQ_PAT_TIE_START, 0);
    refused("[[[[[x]]]]]",    SEQ_PAT_DEEP, 4);
    refused("[xxxxx][xxxx][xxx]", SEQ_PAT_SLOTS, 0);
    /* the marks this decision deleted say where they went */
    refused("X...x...",       SEQ_PAT_BAD_CHAR, 0);
    refused("x?x?",           SEQ_PAT_BAD_CHAR, 1);
    refused("x---",           SEQ_PAT_BAD_CHAR, 1);
    refused("x,x,",           SEQ_PAT_COMMA, 1);
    refused("o---o---",       SEQ_PAT_BAD_CHAR, 0);
    /* Strudel habits: told where they belong */
    refused("x*2 x",          SEQ_PAT_BAD_CHAR, 1);
    refused("x!3",            SEQ_PAT_BAD_CHAR, 1);
    refused("x@3",            SEQ_PAT_BAD_CHAR, 1);
    refused("x ~ x ~",        SEQ_PAT_BAD_CHAR, 2);
    /* trailers that are not numbers in range */
    refused("x.x. /0",        SEQ_PAT_TRAILER, 5);
    refused("x.x. /z",        SEQ_PAT_TRAILER, 5);
    refused("x.x. *99",       SEQ_PAT_TRAILER, 5);
    refused("x.x. !0",        SEQ_PAT_TRAILER, 5);
    refused("x.x. /2 /3",     SEQ_PAT_TWICE, 5);
    refused("x.x. !2 !3",     SEQ_PAT_TWICE, 5);
    refused("x.x./2",         SEQ_PAT_BAD_CHAR, 4);
    refused("[xxxxxxx] *4",   SEQ_PAT_FINE, 0);
    eqi("a pattern of spaces is empty", seq_pattern_compile("   ", &C), SEQ_PAT_EMPTY);

    /* Every message fits the status bar, for every character that can be
     * refused. The bar is 30 columns at its narrowest. */
    for (int err = SEQ_PAT_BAD_CHAR; err <= SEQ_PAT_FINE; err++) {
        for (int ch = 1; ch < 127; ch++) {
            char pat[4] = { (char)ch, 0, 0, 0 };
            memset(&C, 0, offsetof(seq_comp_t, leaf));
            C.err = err;
            C.err_at = 0;
            C.err_num = 999;
            char msg[80];
            seq_pattern_error_text(&C, pat, msg, sizeof msg);
            if (strlen(msg) > 30) {
                printf("[FAIL] error %d for '%c' is %zu wide: %s\n",
                       err, ch, strlen(msg), msg);
                fails++;
                break;
            }
        }
    }

    /* 12. THE TRAILING TOKENS. The rate and the count are not part of the
     *     picture, in any order, and the playhead must not walk into them. */
    {
        seq_pattern_compile("x.x.x.x. /2", &C);
        eqi("rate: slots", C.slots, 8);
        eqi("rate: denominator", C.rden, 2);
        eqi("rate: numerator", C.rnum, 1);
        eqi("rate: pattern length", C.plen, 8);
        seq_pattern_compile("x.x. *4 !3", &C);
        eqi("count after rate", C.count, 3);
        eqi("rate before count", C.rnum, 4);
        seq_pattern_compile("x.x. !3 *4", &C);
        eqi("count before rate", C.count, 3);
        eqi("rate after count", C.rnum, 4);
        seq_pattern_compile("x.x.", &C);
        eqi("no count is for ever", C.count, 0);
        /* the direction in front is not a step */
        seq_pattern_compile("u 4.4.", &C);
        eqi("a direction in front", C.dir, 'u');
        eqi("is not a step", C.slots, 4);
        seq_pattern_compile("u", &C);
        eqi("a lone u is a step", C.slots, 1);
    }

    /* 13. TIME. Slot g starts on tick floor(g * 24 * rden / (div * rnum)).
     *
     *     The old code divided 24 by the subdivision in integers, so a five-way
     *     split got 4 ticks where it needed 4.8 and the lane looped in 80 ticks
     *     instead of 96 - a quintuplet drifted against every other lane. This
     *     walks a whole bar and checks every lane lands back on the beat. */
    {
        struct { const char *pat; int ticks; } bar[] = {
            { "x...x...x...x...", 16 * 24 },
            { "[xxxxx]...",        4 * 24 },
            { "[xxxxxxx]...",      4 * 24 },
            { "[xx][xxx]",         2 * 24 },
            { "x.x. /2",           4 * 48 },
            { "x.x. *2",           4 * 12 },
            { "[xxx]. /3",         2 * 72 },
        };
        for (unsigned i = 0; i < sizeof bar / sizeof bar[0]; i++) {
            seq_pattern_compile(bar[i].pat, &C);
            int fired = 0, last_slot = -1, first_tick_cycle1 = -1;
            for (uint32_t t = 0; t < (uint32_t)bar[i].ticks * 2; t++) {
                int s; uint32_t cy;
                if (seq_pattern_slot_at(t, C.slots, C.div, C.rnum, C.rden, 0,
                                        &s, &cy)) {
                    if (cy == 0) { fired++; last_slot = s; }
                    if (cy == 1 && first_tick_cycle1 < 0) {
                        first_tick_cycle1 = (int)t;
                    }
                }
            }
            char what[64];
            snprintf(what, sizeof what, "%s fires every slot", bar[i].pat);
            eqi(what, fired, C.slots);
            snprintf(what, sizeof what, "%s ends on its last slot", bar[i].pat);
            eqi(what, last_slot, C.slots - 1);
            snprintf(what, sizeof what, "%s's next bar starts on the beat", bar[i].pat);
            eqi(what, first_tick_cycle1, bar[i].ticks);
        }
        /* EACH NOTE ON THE NEAREST TICK: within half a tick of g * Q / D, for
         * every split up to eight at every rate up to four either way. */
        for (int div = 1; div <= 8; div++) {
            for (int rn = 1; rn <= 4; rn++) {
                for (int rd = 1; rd <= 4; rd++) {
                    if (div * rn > 24 * rd) { continue; }
                    const double per = 24.0 * rd / (div * rn);
                    int g = 0;
                    for (uint32_t t = 0; t < 24u * 16u * (uint32_t)rd && g < 64; t++) {
                        int s; uint32_t cy;
                        if (!seq_pattern_slot_at(t, 64, div, rn, rd, 0, &s, &cy)) {
                            continue;
                        }
                        const double err = (double)t - g * per;
                        if (err > 0.5 || err < -0.5 || s != g % 64) {
                            printf("[FAIL] div %d *%d/%d slot %d on tick %u, "
                                   "ideal %.2f\n", div, rn, rd, g, (unsigned)t, g * per);
                            fails++;
                            div = 99; rn = 99; rd = 99;
                            break;
                        }
                        g++;
                    }
                }
            }
        }
        /* The test can SEE the old bug: integer ticks-per-slot for a five-way
         * split puts the next bar at 80, not 96. */
        const int old_tps = 24 / 5;
        if (old_tps * 5 * 4 == 4 * 24) {
            printf("[FAIL] the old arithmetic is no longer a counterexample\n");
            fails++;
        }
    }

    /* 14. SWING BENDS TIME, NOT A LANE'S STEPS: the second sixteenth of every
     *     eighth starts late, and everything inside a sixteenth goes with it.
     *
     *     This test used to assert that swing delays a lane's odd SLOTS, which
     *     is what the code did - and it is why one roll straightened a whole
     *     line and why 'xxxxxxxx /2' swung where 'x.x.x.x.x.x.x.x.' did not.
     *     Every half below fails on that arithmetic. */
    {
        /* a) A plain sixteenth lane lands exactly where it always did. */
        for (int sw = 0; sw <= 22; sw++) {
            int s; uint32_t cy;
            const int ok = seq_pattern_slot_at((uint32_t)(24 + sw), 4, 1, 1, 1,
                                               sw, &s, &cy);
            if (!ok || s != 1) {
                printf("[FAIL] swing %d: the offbeat sixteenth is not at %d\n",
                       sw, 24 + sw);
                fails++;
            }
        }
        /* b) One roll does not move the other fifteen steps. */
        for (int pc = 50; pc <= 75; pc++) {
            const int sw = (pc * 2 * 24) / 100 - 24;
            uint32_t plain[16], rolled[16];
            for (int k = 0; k < 2; k++) {
                seq_pattern_compile(k ? "xxxxxxxxxxxxxxx[xx]" : "xxxxxxxxxxxxxxxx", &C);
                uint32_t *at = k ? rolled : plain;
                for (uint32_t t = 0; t < 16 * 24; t++) {
                    int s; uint32_t cy;
                    if (seq_pattern_slot_at(t, C.slots, C.div, C.rnum, C.rden, sw,
                                            &s, &cy) && s % C.div == 0) {
                        at[s / C.div] = t;
                    }
                }
            }
            for (int i = 0; i < 15; i++) {
                if (plain[i] != rolled[i]) {
                    printf("[FAIL] swing %d: a roll on step 15 moved step %d "
                           "from %u to %u\n", pc, i, (unsigned)plain[i],
                           (unsigned)rolled[i]);
                    fails++;
                    break;
                }
            }
            /* ...and the roll is inside its own sixteenth: an odd one, so from
             * its swung start to the end of the eighth. */
            seq_pattern_compile("xxxxxxxxxxxxxxx[xx]", &C);
            int in = 0;
            for (uint32_t t = 0; t < 16 * 24; t++) {
                int s; uint32_t cy;
                if (seq_pattern_slot_at(t, C.slots, C.div, C.rnum, C.rden, sw,
                                        &s, &cy) && s >= 30) {
                    in += (t >= (uint32_t)(15 * 24 + sw) && t < 16 * 24);
                }
            }
            eqi("the roll's two notes are inside its sixteenth", in, 2);
        }
        /* c) One rhythm, two spellings, one groove: eighths do not swing,
         *    whether they are written x.x. or at half speed. */
        {
            const int sw = 8;                               /* 67: triplet */
            const char *pats[] = { "x.x.x.x.x.x.x.x.", "xxxxxxxx /2",
                                   "[xx][xx][xx][xx] /4",
                                   "x...x...x...x...x...x...x...x... *2" };
            for (size_t p = 0; p < sizeof pats / sizeof pats[0]; p++) {
                seq_pattern_compile(pats[p], &C);
                int hits = 0, off = 0;
                for (uint32_t t = 0; t < 16 * 24; t++) {
                    int s; uint32_t cy;
                    if (!seq_pattern_slot_at(t, C.slots, C.div, C.rnum, C.rden,
                                             sw, &s, &cy)) {
                        continue;
                    }
                    for (int i = 0; i < C.n; i++) {
                        if (C.leaf[i].kind == SEQ_LEAF_HIT && C.leaf[i].slot == s) {
                            hits++;
                            off += (t % 48 != 0);
                        }
                    }
                }
                if (hits != 8 || off != 0) {
                    printf("[FAIL] swing 67: '%s' played %d eighths, %d off the "
                           "eighth\n", pats[p], hits, off);
                    fails++;
                } else {
                    printf("[ ok ] swing 67: '%s' - eight eighths, none moved\n",
                           pats[p]);
                }
            }
        }
        /* d) Every slot of every lane starts exactly once, in order, at every
         *    swing the verb allows, wherever a slot is two ticks or more. */
        {
            const int divs[] = { 1, 2, 3, 4, 5, 6, 7, 8, 12 };
            int bad = 0;
            for (int pc = 50; pc <= 75 && !bad; pc++) {
                const int sw = (pc * 2 * 24) / 100 - 24;
                for (size_t di = 0; di < sizeof divs / sizeof divs[0] && !bad; di++)
                for (int rn = 1; rn <= 4 && !bad; rn *= 2)
                for (int rd = 1; rd <= 4 && !bad; rd *= 2) {
                    if (divs[di] * rn > 12 * rd) { continue; }
                    uint64_t want = 0;
                    uint32_t last = 0;
                    for (uint32_t t = 0; t < 8 * 96 && !bad; t++) {
                        int s; uint32_t cy;
                        if (!seq_pattern_slot_at(t, 64, divs[di], rn, rd, sw, &s,
                                                 &cy)) {
                            continue;
                        }
                        const uint64_t g = (uint64_t)cy * 64 + (uint64_t)s;
                        if (g != want || (want > 0 && t <= last)) {
                            printf("[FAIL] swing %d div %d *%d/%d: slot %llu on "
                                   "tick %u, wanted slot %llu\n", pc, divs[di],
                                   rn, rd, (unsigned long long)g, (unsigned)t,
                                   (unsigned long long)want);
                            fails++;
                            bad = 1;
                        }
                        want = g + 1;
                        last = t;
                    }
                }
            }
            if (!bad) {
                printf("[ ok ] swing 50-75: every slot once, in order\n");
            }
        }
        /* e) With no swing, nothing moves at all: every slot starts on the
         *    nearest tick to g*Q/D, rounded exactly as it always was. */
        {
            int moved = 0;
            for (int div = 1; div <= 12 && !moved; div++)
            for (int rn = 1; rn <= 8 && !moved; rn *= 2)
            for (int rd = 1; rd <= 8 && !moved; rd *= 2) {
                if (div * rn > 24 * rd) { continue; }
                const uint64_t Q = 24u * (uint64_t)rd, D = (uint64_t)div * (uint64_t)rn;
                uint64_t want = 0;
                for (uint32_t t = 0; t < 4 * 96 * (uint32_t)rd && !moved; t++) {
                    int s; uint32_t cy;
                    if (!seq_pattern_slot_at(t, 64, div, rn, rd, 0, &s, &cy)) {
                        continue;
                    }
                    const uint64_t g = (uint64_t)cy * 64 + (uint64_t)s;
                    if (g != want || t != (2 * g * Q + D) / (2 * D)) {
                        printf("[FAIL] unswung div %d *%d/%d: slot %llu on tick "
                               "%u\n", div, rn, rd, (unsigned long long)g,
                               (unsigned)t);
                        fails++;
                        moved = 1;
                    }
                    want = g + 1;
                }
            }
            if (!moved) {
                printf("[ ok ] no swing: every slot on the tick it always had\n");
            }
        }
    }

    /* 15. THE PLAYHEAD MOVES AT THE LANE'S OWN SPEED.
     *
     *     The editor used seq_position() % steps - the global SIXTEENTH count -
     *     so a '/2' lane's mark ran twice as fast as its sound and a nested
     *     lane's ran at the wrong speed entirely. At tick 48 a '/2' lane is on
     *     its second step, not its third. */
    {
        seq_pattern_compile("x.x.x.x. /2", &C);
        int s; uint32_t cy;
        seq_pattern_slot_now(48, C.slots, C.div, C.rnum, C.rden, &s, &cy);
        eqi("/2 at tick 48 is on step 1", s, 1);
        const int old = (48 / 24) % C.slots;
        if (old == s) {
            printf("[FAIL] cannot distinguish the old playhead\n");
            fails++;
        }
        seq_pattern_compile("x..[xx]", &C);
        seq_pattern_slot_now(12, C.slots, C.div, C.rnum, C.rden, &s, &cy);
        eqi("a nested lane at tick 12 is on slot 1", s, 1);
        seq_pattern_slot_now(24 * 4, C.slots, C.div, C.rnum, C.rden, &s, &cy);
        eqi("and a bar later it is back at the start", s, 0);
        eqi("of the next cycle", (int)cy, 1);
    }

    /* 16. The toggle hash is SPACING-SENSITIVE, on purpose - a spacing-blind
     *     hash would silence a lane the moment the player re-spaced it. */
    if (seq_pattern_hash("x...x...") == seq_pattern_hash("x... x...")) {
        printf("[FAIL] the hash ignores spacing; re-spacing would silence\n");
        fails++;
    }
    if (seq_pattern_hash("9...x...") == seq_pattern_hash("x...x...")) {
        printf("[FAIL] the hash misses a velocity change\n");
        fails++;
    }

    /* 17. THE POSITION COUNTER MUST NOT JUMP WHEN IT ROLLS. The counter was
     *     masked with 0x7FFF, so a 5-, 6- or 12-step lane jumped every 66
     *     minutes. Asserted directly, and the old mask shown to be caught. */
    for (int steps = 2; steps <= 32; steps++) {
        const unsigned long roll = 0x8000UL;
        const int before = (int)((roll - 1) % (unsigned long)steps);
        const int after  = (int)(roll % (unsigned long)steps);
        if (after != (before + 1) % steps) {
            printf("[FAIL] unmasked counter jumps at roll for %d steps\n", steps);
            fails++;
        }
        const int masked_after = (int)((roll & 0x7FFFUL) % (unsigned long)steps);
        if (steps == 5 && masked_after == (before + 1) % steps) {
            printf("[FAIL] the 0x7FFF mask case is no longer a counterexample\n");
            fails++;
        }
    }

    printf(fails ? "[FAIL] %d check(s) failed\n" : "[PASS] pattern compiler\n",
           fails);
    return fails != 0;
}
