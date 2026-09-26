/*
 * The key: a root, a mode, and a degree resolved to a note.
 *
 * Pure and dependency-free, compiled into the firmware and the host checks -
 * the arrangement seq_pattern.h has, for the same reason. tools/test_scale.c
 * checks the parser the deck runs, and tools/test_pieces.c reads every
 * '>scale' in the pieces with it and prints their notes with the table the
 * deck plays. A copy of either in a test would be a second opinion about
 * what the deck does, and the two would drift.
 */
#ifndef SEQ_SCALE_H
#define SEQ_SCALE_H

#include <stdint.h>
#include <string.h>

/* THE KEY, AND WHY A SCALE IS A TABLE AND NOT A PARSER.
 *
 * A degree is resolved to a note by indexing this table and adding. That is
 * the entire pitch system: no note names in the realtime path, no string
 * anywhere near the clock, and a wrong note is not expressible. The modes are
 * the seven diatonic ones plus the three a performer actually reaches for
 * under pressure - minor pentatonic, which cannot sound wrong; blues, which
 * is pentatonic plus the flat five; and chromatic, for when the whole point
 * is to leave the key.
 *
 * Order matters: the longest names must be tested first or "maj" swallows
 * "maj5". That is a real bug this table's layout is chosen to prevent rather
 * than a comment about one. */
typedef struct { const char *name; uint8_t n; uint8_t iv[12]; } seq_mode_t;

static const seq_mode_t seq_modes[] = {
    { "chrom", 12, {0,1,2,3,4,5,6,7,8,9,10,11} },
    { "blues",  6, {0,3,5,6,7,10} },
    { "pent",   5, {0,3,5,7,10} },       /* minor pentatonic - the safe one */
    { "maj5",   5, {0,2,4,7,9} },        /* major pentatonic                */
    { "maj",    7, {0,2,4,5,7,9,11} },
    { "min",    7, {0,2,3,5,7,8,10} },
    { "dor",    7, {0,2,3,5,7,9,10} },
    { "phr",    7, {0,1,3,5,7,8,10} },
    { "lyd",    7, {0,2,4,6,7,9,11} },
    { "mix",    7, {0,2,4,5,7,9,10} },
    { "loc",    7, {0,1,3,5,6,8,10} },
};
#define SEQ_MODE_COUNT ((int)(sizeof seq_modes / sizeof seq_modes[0]))
#define SEQ_MODE_MIN   5                  /* a bare root means minor */

/* The mode whose name starts `p`, or NULL. */
static inline const seq_mode_t *seq_mode_at(const char *p)
{
    for (int i = 0; i < SEQ_MODE_COUNT; i++) {
        if (strncmp(p, seq_modes[i].name, strlen(seq_modes[i].name)) == 0) {
            return &seq_modes[i];
        }
    }
    return NULL;
}

/* "dmin", "f#mix", "ebblues", "c": a root letter in either case, then '#' or
 * 'b', then a mode - none means minor. Returns 0 and sets *root (a pitch class,
 * C = 0) and *mode, or -1 and leaves both alone.
 *
 * 'b' IS A FLAT UNLESS IT BEGINS A MODE. It was always a flat, so "cblues" read
 * as C-flat followed by "lues", which is no mode, and was refused: blues after a
 * natural root could not be typed at all, only "ebblues" and the like. A mode's
 * name wins, and a flat still needs no space - "ebmaj", "bb", "abblues". */
static inline int seq_scale_parse(const char *spec, int *root,
                                  const seq_mode_t **mode)
{
    if (spec == NULL || spec[0] == '\0') {
        return -1;
    }
    const char *p = spec;
    int pc;
    switch (*p | 0x20) {          /* tolerate either case; nobody should care */
    case 'c': pc = 0;  break;
    case 'd': pc = 2;  break;
    case 'e': pc = 4;  break;
    case 'f': pc = 5;  break;
    case 'g': pc = 7;  break;
    case 'a': pc = 9;  break;
    case 'b': pc = 11; break;
    default: return -1;
    }
    p++;
    if (*p == '#') {
        pc = (pc + 1) % 12;
        p++;
    } else if (*p == 'b' && seq_mode_at(p) == NULL) {
        pc = (pc + 11) % 12;
        p++;
    }
    const seq_mode_t *m = &seq_modes[SEQ_MODE_MIN];
    if (*p != '\0') {
        m = seq_mode_at(p);
        if (m == NULL) {
            return -1;
        }
    }
    *root = pc;
    *mode = m;
    return 0;
}

/* Degree to MIDI note. Degrees past the top of the scale keep climbing into
 * the next octave, so "0123456789" is a run and not a wrap - which is what
 * anyone typing it expects, and the reason degrees go to 9 rather than to the
 * size of the mode. */
static inline uint8_t seq_degree_note(int root, const seq_mode_t *m, uint8_t deg,
                                      int octave)
{
    const int n = m->n;
    const int up = deg / n;
    const int idx = deg % n;
    int note = 12 * (octave + 1 + up) + root + m->iv[idx];
    if (note < 0)   { note = 0; }
    if (note > 127) { note = 127; }
    return (uint8_t)note;
}

#endif /* SEQ_SCALE_H */
