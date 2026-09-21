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

/* A BRACKET IS A PARAMETER ON THE STEP BEFORE IT, NOT A STEP.
 *
 * '?[15]' is a fifteen-per-cent chance on that one step. The bracket attaches
 * to the character to its left and occupies no step of its own, which is what
 * keeps step index and character offset a bijection - and that bijection is
 * what lets the playhead sit on the character that is sounding. Nested
 * brackets would destroy it, which is why there are none: a bracket may only
 * ever follow a step and contain a number.
 *
 * Returns the character length of the bracket group at `p`, or 0. */
static inline int seq_pattern_param_len(const char *p)
{
    if (p == NULL || *p != '[') {
        return 0;
    }
    int n = 1;
    while (p[n] != '\0' && p[n] != ']' && n < 6) {
        n++;
    }
    return (p[n] == ']') ? n + 1 : 0;   /* unterminated: not a parameter */
}

/* The number inside the bracket at `p`, or -1. */
static inline int seq_pattern_param(const char *p)
{
    if (seq_pattern_param_len(p) == 0) {
        return -1;
    }
    int v = 0, any = 0;
    for (const char *q = p + 1; *q != ']'; q++) {
        if (*q < '0' || *q > '9') {
            return -1;
        }
        v = v * 10 + (*q - '0');
        any = 1;
    }
    return any ? v : -1;
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

/* How many steps a pattern compiles to. Must match seq_lane()'s count. */
static inline int seq_pattern_steps(const char *pat, int max_steps)
{
    int n = 0;
    if (pat == NULL) {
        return 0;
    }
    const char *stop = pat + seq_pattern_rate(pat, NULL, NULL);
    for (const char *p = pat; p < stop && *p != '\0' && n < max_steps; p++) {
        const int plen = seq_pattern_param_len(p);
        if (plen > 0) { p += plen - 1; continue; }   /* a parameter, not a step */
        if (!seq_pattern_is_spacing(*p)) {
            n++;
        }
    }
    return n;
}

/* Character offset within `pat` of step `want`, or -1 if there is no such
 * step. The inverse of the count above, and the reason this file exists. */
static inline int seq_pattern_offset(const char *pat, int want, int max_steps)
{
    if (pat == NULL || want < 0 || want >= max_steps) {
        return -1;
    }
    int n = 0;
    const char *stop = pat + seq_pattern_rate(pat, NULL, NULL);
    for (const char *p = pat; p < stop && *p != '\0'; p++) {
        const int plen = seq_pattern_param_len(p);
        if (plen > 0) { p += plen - 1; continue; }
        if (seq_pattern_is_spacing(*p)) {
            continue;
        }
        if (n == want) {
            return (int)(p - pat);
        }
        if (++n >= max_steps) {
            break;
        }
    }
    return -1;
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
