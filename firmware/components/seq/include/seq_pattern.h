/*
 * Pattern geometry: the one walk over a pattern string.
 *
 * seq_lane() walks a pattern forward turning characters into bits. The editor
 * must walk the SAME string backward - from a step index to the character it
 * was compiled from - to put a playhead on the character that is sounding.
 *
 * Two walks that must agree about which characters are steps is two chances
 * to disagree, and the disagreement would be invisible: the mark would sit
 * one column off and look deliberate. So there is ONE walk, here, in a header
 * with no dependencies, compiled into both the firmware and the host check -
 * the same arrangement as st7305_addr.h and mirror_path.h.
 *
 * THE RULE: ' ' is spacing for the eye and is skipped. NOTHING ELSE IS.
 * A tab is not a rest - seq_lane() treats only '.', '-' and '_' as rests, so
 * a tab compiles as a hit - and a walk that skipped tabs would put the mark
 * one character left of what is sounding.
 */
#ifndef SEQ_PATTERN_H
#define SEQ_PATTERN_H

#include <stdint.h>
#include <string.h>

static inline int seq_pattern_is_spacing(char c)
{
    return c == ' ';
}

/* A PER-CENT SIGN IS A PARAMETER ON THE STEP BEFORE IT.
 *
 * 'x%15' is a fifteen-per-cent chance on that one step. It attaches to the
 * character on its left and occupies no step of its own.
 *
 * WHY IT MOVED OFF THE BRACKET. This used to be 'x[15]', which meant the
 * bracket was spent on a parameter - and the bracket is the only punctuation a
 * player already reads as GROUPING. A probability is a property OF a step; a
 * group is a step that CONTAINS steps. Those are different enough to deserve
 * different marks, and the structural one had the better claim on the bracket.
 * docs/MAP.md argued this and this is the change.
 *
 * Returns the character length of the parameter at `p`, or 0. */
static inline int seq_pattern_param_len(const char *p)
{
    if (p == NULL || *p != '%') {
        return 0;
    }
    int n = 1;
    while (p[n] >= '0' && p[n] <= '9' && n < 4) {
        n++;
    }
    return (n > 1) ? n : 0;          /* a bare '%' is not a parameter */
}

/* The number after the '%' at `p`, or -1. */
static inline int seq_pattern_param(const char *p)
{
    const int len = seq_pattern_param_len(p);
    if (len == 0) {
        return -1;
    }
    int v = 0;
    for (int i = 1; i < len; i++) {
        v = v * 10 + (p[i] - '0');
    }
    return v;
}

/* THE RATE TOKEN. The last whitespace-separated token, if it begins with '/'
 * or '*', sets this lane's speed and is not part of the picture:
 *
 *     >bass 0...3...5...3... /2      half speed, two bars
 *     >hat  x.x.x.x. *2             double speed
 *
 * It lives HERE, not in the command layer, for the same reason everything
 * else does: seq_lane() walks the string to build bits and the editor walks it
 * to place the playhead, and if only one of them knew about the rate they
 * would disagree about which characters are steps. '/2' would otherwise
 * compile as two extra hits.
 *
 * Returns the numerator and denominator of the rate, and the length of the
 * pattern before it. den > 1 is slower; num > 1 is faster. */
static inline int seq_pattern_rate(const char *pat, int *num, int *den)
{
    if (num != NULL) { *num = 1; }
    if (den != NULL) { *den = 1; }
    if (pat == NULL) { return 0; }

    const int len = (int)strlen(pat);
    int end = len;
    while (end > 0 && pat[end - 1] == ' ') { end--; }
    int start = end;
    while (start > 0 && pat[start - 1] != ' ') { start--; }
    if (start == 0 || end - start < 2) {
        return len;                       /* no token, or the whole string */
    }
    const char op = pat[start];
    if (op != '/' && op != '*') {
        return len;
    }
    int v = 0;
    for (int i = start + 1; i < end; i++) {
        if (pat[i] < '0' || pat[i] > '9') { return len; }
        v = v * 10 + (pat[i] - '0');
    }
    if (v < 1 || v > 32) {
        return len;                       /* nonsense: treat it as picture */
    }
    if (op == '/' && den != NULL) { *den = v; }
    if (op == '*' && num != NULL) { *num = v; }
    int plen = start;
    while (plen > 0 && pat[plen - 1] == ' ') { plen--; }
    return plen;
}

/* ===================================================================== *
 * NESTING, AND WHY THE REALTIME CORE NEVER LEARNS ABOUT IT
 * ===================================================================== *
 *
 * A bracket subdivides the step it occupies, to any depth:
 *
 *     x..[xx]          four steps; the last is two half-steps
 *     x.[x[xx]].       the second of that pair splits again
 *
 * One rule, recursive, and it is how a bar is already read on paper.
 *
 * THE TRICK IS THAT IT IS A COMPILE-TIME TRANSFORM. The clock reads a flat
 * bitmask at a uniform rate and knows nothing else - docs/SUBSTRATE.md: the
 * realtime core never parses text - so nesting is resolved here, by FLATTENING
 * the tree onto that same uniform grid. 'x..[xx]' becomes eight slots at half
 * the step length with hits at 0, 6 and 7. The sequencer is unchanged; there is
 * no second code path for a nested lane, and there is nothing new that can be
 * late.
 *
 * The arithmetic: each top-level step is given `div` slots, where div is the
 * least common multiple of what its members need, so every leaf in the tree
 * lands exactly on a slot boundary. A pattern whose flattened form does not fit
 * is REFUSED rather than truncated - a silently shortened bar is a bug that
 * sounds like a composition choice.
 */
#define SEQ_PATTERN_MAX_SLOTS 32
#define SEQ_PATTERN_MAX_DEPTH  4

typedef struct {
    int n;                                  /* slots in play, 0 if empty     */
    int div;                                /* slots per top-level step      */
    int16_t at[SEQ_PATTERN_MAX_SLOTS];      /* char offset of the slot's mark,
                                             * or -1 where nothing starts    */
} seq_walk_t;

/* One item: a step and its parameter, or a bracket group. Returns the position
 * just past it. */
static inline const char *seq_pattern_item_end(const char *p, const char *end)
{
    if (p >= end) {
        return p;
    }
    if (*p == '[') {
        int depth = 0;
        while (p < end) {
            if (*p == '[') { depth++; }
            else if (*p == ']') { depth--; if (depth == 0) { return p + 1; } }
            p++;
        }
        return p;                       /* unterminated: to the end */
    }
    p++;
    p += seq_pattern_param_len(p);
    return p;
}

static inline int seq_pattern_gcd(int a, int b)
{
    while (b != 0) { const int t = a % b; a = b; b = t; }
    return a < 1 ? 1 : a;
}

static inline int seq_pattern_lcm(int a, int b)
{
    if (a < 1) { a = 1; }
    if (b < 1) { b = 1; }
    return a / seq_pattern_gcd(a, b) * b;
}

/* Slots this level needs so every descendant lands on a boundary. */
static inline int seq_pattern_span(const char *p, const char *end, int depth)
{
    int k = 0, l = 1;
    while (p < end) {
        if (seq_pattern_is_spacing(*p)) { p++; continue; }
        const char *e = seq_pattern_item_end(p, end);
        int s = 1;
        if (*p == '[' && depth > 0 && e - 1 > p + 1) {
            s = seq_pattern_span(p + 1, e - 1, depth - 1);
        }
        l = seq_pattern_lcm(l, s);
        k++;
        if (k * l > SEQ_PATTERN_MAX_SLOTS) { return k * l; }  /* let it overflow */
        p = e;
    }
    return (k == 0) ? 1 : k * l;
}

/* Fill in where each slot's mark lives. `base` is the pattern's first
 * character, so the offsets are absolute. */
static inline void seq_pattern_place(const char *p, const char *end,
                                     const char *base, int start, int width,
                                     seq_walk_t *o, int depth)
{
    int k = 0;
    for (const char *q = p; q < end; ) {
        if (seq_pattern_is_spacing(*q)) { q++; continue; }
        q = seq_pattern_item_end(q, end);
        k++;
    }
    if (k == 0) { return; }
    /* A CLAMPED FLAT PATTERN HAS MORE ITEMS THAN SLOTS, and share would floor
     * to zero - which silently placed nothing at all and turned a 40-step line
     * into 32 empty slots. One slot each, and the items past the end fall off
     * the bottom of the bounds check below. */
    int share = width / k;
    if (share < 1) { share = 1; }

    int idx = 0;
    while (p < end) {
        if (seq_pattern_is_spacing(*p)) { p++; continue; }
        const char *e = seq_pattern_item_end(p, end);
        const int at = start + idx * share;
        if (*p == '[') {
            if (depth > 0 && e - 1 > p + 1) {
                seq_pattern_place(p + 1, e - 1, base, at, share, o, depth - 1);
            }
        } else if (at >= 0 && at < SEQ_PATTERN_MAX_SLOTS) {
            o->at[at] = (int16_t)(p - base);
        }
        idx++;
        p = e;
    }
}

/* Flatten a pattern. Returns the slot count, 0 for an empty pattern, or -1 if
 * the flattened form does not fit - which the caller must report rather than
 * quietly shorten. */
static inline int seq_pattern_walk(const char *pat, seq_walk_t *o)
{
    for (int i = 0; i < SEQ_PATTERN_MAX_SLOTS; i++) { o->at[i] = -1; }
    o->n = 0;
    o->div = 1;
    if (pat == NULL) {
        return 0;
    }
    const char *end = pat + seq_pattern_rate(pat, NULL, NULL);

    int k = 0, l = 1;
    for (const char *q = pat; q < end; ) {
        if (seq_pattern_is_spacing(*q)) { q++; continue; }
        const char *e = seq_pattern_item_end(q, end);
        int s = 1;
        if (*q == '[' && e - 1 > q + 1) {
            s = seq_pattern_span(q + 1, e - 1, SEQ_PATTERN_MAX_DEPTH - 1);
        }
        l = seq_pattern_lcm(l, s);
        k++;
        q = e;
        /* A NESTED PATTERN THAT DOES NOT FIT IS REFUSED; A FLAT ONE IS CLAMPED,
         * and the difference is not a compromise.
         *
         * A flat pattern truncated at 32 still plays its first 32 steps exactly
         * as written - one character, one step - so clamping loses the tail and
         * nothing else, which is the behaviour this instrument has always had.
         * A nested pattern cannot be shortened that way: the subdivision is a
         * property of the whole bar, so dropping the end changes the meaning of
         * everything before it. Refusing is the only honest answer there. */
        if (l > 1 && k * l > SEQ_PATTERN_MAX_SLOTS) { return -1; }
    }
    if (k == 0) {
        return 0;
    }
    if (l == 1 && k > SEQ_PATTERN_MAX_SLOTS) {
        k = SEQ_PATTERN_MAX_SLOTS;
    }
    o->div = l;
    o->n   = k * l;
    seq_pattern_place(pat, end, pat, 0, o->n, o, SEQ_PATTERN_MAX_DEPTH);
    return o->n;
}

/* How many slots a pattern compiles to. Must match seq_lane()'s count. */
static inline int seq_pattern_steps(const char *pat, int max_steps)
{
    seq_walk_t w;
    const int n = seq_pattern_walk(pat, &w);
    if (n <= 0) {
        return 0;
    }
    return (n > max_steps) ? max_steps : n;
}

/* Character offset within `pat` of slot `want`, or -1 if nothing starts there.
 * The inverse of the count above, and the reason this file exists: the editor
 * walks backward from a slot to the character that is sounding, and it must
 * agree with the forward walk exactly - including through a nested group, where
 * several slots share one bar and only one of them carries a mark. */
static inline int seq_pattern_offset(const char *pat, int want, int max_steps)
{
    if (pat == NULL || want < 0 || want >= max_steps ||
        want >= SEQ_PATTERN_MAX_SLOTS) {
        return -1;
    }
    seq_walk_t w;
    if (seq_pattern_walk(pat, &w) <= 0) {
        return -1;
    }
    return (want < w.n) ? (int)w.at[want] : -1;
}

/* A hash of the pattern AS TYPED, used to tell "I re-ran this line untouched"
 * from "I edited it and want it updated".
 *
 * It hashes the raw characters, spacing included - deliberately. A hash that
 * ignored spacing, or a comparison of the compiled bitmask, would treat
 * "x...x...x...x..." and "x... x... x... x..." as the same line and silence
 * the lane the moment the player re-spaced it for readability. That is
 * exactly the surprise the toggle rule exists to avoid, so the rule fails
 * toward compiling and never toward silence.
 *
 * FNV-1a, 32-bit. Not cryptographic and does not need to be: the cost of a
 * collision is one press that silences instead of recompiling, and the next
 * press undoes it. */
static inline uint32_t seq_pattern_hash(const char *pat)
{
    uint32_t h = 2166136261u;
    if (pat == NULL) {
        return h;
    }
    for (const char *p = pat; *p != '\0'; p++) {
        h ^= (uint32_t)(unsigned char)*p;
        h *= 16777619u;
    }
    return h;
}

#endif /* SEQ_PATTERN_H */
