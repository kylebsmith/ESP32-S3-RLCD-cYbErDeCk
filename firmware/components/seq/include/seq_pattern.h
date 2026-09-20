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

static inline int seq_pattern_is_spacing(char c)
{
    return c == ' ';
}

/* How many steps a pattern compiles to. Must match seq_lane()'s count. */
static inline int seq_pattern_steps(const char *pat, int max_steps)
{
    int n = 0;
    if (pat == NULL) {
        return 0;
    }
    for (const char *p = pat; *p != '\0' && n < max_steps; p++) {
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
    for (const char *p = pat; *p != '\0'; p++) {
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
