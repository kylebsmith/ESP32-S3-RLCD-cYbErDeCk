/*
 * A lane's NAME, and what a name is defined as. Pure, dependency-free, and
 * compiled into both the firmware and tools/test_lane_name.c - the same
 * arrangement as seq_pattern.h.
 *
 * ===================================================================== *
 * ONE ADDRESS GRAMMAR  (docs/MANIFESTO.md §3.3)
 * ===================================================================== *
 *
 *     disc          a lane
 *     disc:2        a second lane on the same binding - an INSTANCE
 *     disc:x        a PART of that lane: its position, not the circle
 *     disc:2:x      the second circle's position
 *
 * It was three notations for one idea: '[]' selected a part ('disc[x]'), a
 * trailing digit made an instance ('disc2'), and '[]' also grouped inside a
 * pattern. The argument that moved probability off the bracket - "a property OF
 * a step is not a step that CONTAINS steps" - convicted 'disc[x]' exactly, and
 * the trailing digit quietly reserved every name's last character for ever and
 * put '>disc 2' (a small circle) one space from '>disc2' (the second one).
 *
 * ':' is the separator docs/SUBSTRATE.md already uses - 'lullaby.md:27',
 * 'din:1'. It is lexically disjoint from the pattern grammar, which never sees a
 * name. And it is legal in an OSC address, where '[' and ']' are not: they are
 * pattern-matching characters in OSC 1.0, so '/deck/disc[x]' was never a valid
 * address to send.
 *
 * ===================================================================== *
 * A NAME IS DEFINED, NOT BUILT IN  (docs/MANIFESTO.md §3.8)
 * ===================================================================== *
 *
 *     >kick = note 36               a drum: note 36 on channel 10
 *     >bass = voice 2 ch 1 gate 180 a voice: octave 2, 180 ms notes
 *     >cut = cc 74                  a controller
 *     >circle = disc                another name for a picture
 *     >conga =                      forget the name
 *
 * The seventeen sound names were verbs with their numbers compiled in, so a
 * player could not add a conga and could not move a kick to note 35 for a drum
 * machine that wants it there. They are lines in the boot document now, which
 * runs at startup, and a line anywhere else defines a name the moment it is run.
 */
#ifndef LANE_NAME_H
#define LANE_NAME_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define LANE_BASE_MAX  8      /* letters in a name                            */
#define LANE_PART_MAX  5      /* letters in a part                            */
#define LANE_INST_MAX  99
/* base, ":99", ":part", and the terminator - seq.h's SEQ_NAME_MAX holds it */
#define LANE_NAME_MAX  (LANE_BASE_MAX + 3 + 1 + LANE_PART_MAX + 1)

enum {
    LN_OK = 0,
    LN_EMPTY,
    LN_BASE,        /* a name starts with a letter and holds letters, digits  */
    LN_LONG,        /* more than LANE_BASE_MAX                                */
    LN_INST,        /* an instance is 2-99                                    */
    LN_PART,        /* a part is up to LANE_PART_MAX letters                  */
    LN_ORDER,       /* the instance comes before the part, and each once      */
    LN_BRACKET,     /* the old 'disc[x]' spelling                             */
};

typedef struct {
    char base[LANE_BASE_MAX + 1];
    int  inst;                     /* 1 is the plain name                  */
    char part[LANE_PART_MAX + 1];  /* "" when it is the lane itself        */
    char canon[LANE_NAME_MAX];     /* base[:inst][:part], with inst 1 left out */
} lane_name_t;

static inline int lane_is_lower(char c) { return c >= 'a' && c <= 'z'; }
static inline int lane_is_digit(char c) { return c >= '0' && c <= '9'; }

/* Parse the first word of a line as a lane address. `n` is its length. */
static inline int lane_name_parse(const char *w, size_t n, lane_name_t *o)
{
    memset(o, 0, sizeof *o);
    o->inst = 1;
    if (w == NULL || n == 0) {
        return LN_EMPTY;
    }
    size_t i = 0;
    if (!lane_is_lower(w[0])) {
        return LN_BASE;
    }
    while (i < n && w[i] != ':') {
        if (w[i] == '[') {
            return LN_BRACKET;
        }
        if (!lane_is_lower(w[i]) && !lane_is_digit(w[i])) {
            return LN_BASE;
        }
        if (i >= LANE_BASE_MAX) {
            return LN_LONG;
        }
        o->base[i] = w[i];
        i++;
    }
    int seg = 0;
    while (i < n) {
        i++;                                       /* past the ':' */
        const size_t s = i;
        while (i < n && w[i] != ':') { i++; }
        const size_t len = i - s;
        if (len == 0) {
            return LN_ORDER;                       /* 'disc:' or 'disc::x' */
        }
        int all_digit = 1, all_lower = 1;
        for (size_t k = s; k < i; k++) {
            all_digit &= lane_is_digit(w[k]);
            all_lower &= lane_is_lower(w[k]);
        }
        if (all_digit) {
            if (seg != 0 || o->part[0] != '\0') {
                return LN_ORDER;                   /* 'disc:x:2', 'disc:2:3' */
            }
            int v = 0;
            for (size_t k = s; k < i && v <= LANE_INST_MAX; k++) {
                v = v * 10 + (w[k] - '0');
            }
            if (v < 1 || v > LANE_INST_MAX) {
                return LN_INST;
            }
            o->inst = v;
        } else if (all_lower) {
            if (o->part[0] != '\0') {
                return LN_ORDER;                   /* 'disc:x:y' */
            }
            if (len > LANE_PART_MAX) {
                return LN_PART;
            }
            memcpy(o->part, w + s, len);
            o->part[len] = '\0';
        } else {
            return LN_PART;
        }
        seg++;
    }
    int k = snprintf(o->canon, sizeof o->canon, "%s", o->base);
    if (o->inst > 1) {
        k += snprintf(o->canon + k, sizeof o->canon - (size_t)k, ":%d", o->inst);
    }
    if (o->part[0] != '\0') {
        snprintf(o->canon + k, sizeof o->canon - (size_t)k, ":%s", o->part);
    }
    return LN_OK;
}

/* Why a name was refused, in at most 30 columns. `w` is the word as typed. */
static inline void lane_name_error_text(int err, char *out, size_t n)
{
    switch (err) {
    case LN_BASE:    snprintf(out, n, "a name is letters: conga"); break;
    case LN_LONG:    snprintf(out, n, "a name is %d letters at most",
                              LANE_BASE_MAX); break;
    case LN_INST:    snprintf(out, n, "a second one is :2 to :%d",
                              LANE_INST_MAX); break;
    case LN_PART:    snprintf(out, n, "a part is a word: disc:x"); break;
    case LN_ORDER:   snprintf(out, n, "number, then part: disc:2:x"); break;
    case LN_BRACKET: snprintf(out, n, "a part is :x now, not [x]"); break;
    default:         snprintf(out, n, "not a name"); break;
    }
}

/* ===================================================================== *
 * DEFINITIONS
 * ===================================================================== */

enum {
    LD_REMOVE = 0,    /* '>conga ='                        */
    LD_NOTE,          /* a fixed pitch: a drum              */
    LD_VOICE,         /* degrees in the key: a melodic part */
    LD_CC,            /* a controller                       */
    LD_DRAW,          /* another name for a picture         */
    LD_KNOB,          /* an input holding a value (seq.h)    */
    LD_PAD,           /* an input that fires on a press      */
    LD_ERROR,
};

typedef struct {
    int  kind;        /* LD_*                                           */
    int  num;         /* note 0-127, octave 0-8, or controller 0-127   */
    int  chan;        /* 1-16, as a person counts them                 */
    int  gate;        /* ms                                             */
    char draw[LANE_BASE_MAX + 1];   /* LD_DRAW: the picture's own name */
    char why[32];     /* LD_ERROR: the reason, 30 columns at most       */
} lane_def_t;

/* The next whitespace-separated word of `*p`, into `w`. Returns its length. */
static inline size_t lane_word(const char **p, char *w, size_t n)
{
    while (**p == ' ' || **p == '\t') { (*p)++; }
    size_t k = 0;
    while (**p != '\0' && **p != ' ' && **p != '\t') {
        if (k + 1 < n) { w[k++] = **p; }
        (*p)++;
    }
    w[k] = '\0';
    return k;
}

static inline int lane_num(const char *w, int lo, int hi, int *v)
{
    if (w[0] == '\0') { return 0; }
    int x = 0;
    for (const char *q = w; *q != '\0'; q++) {
        if (!lane_is_digit(*q) || x > 10000) { return 0; }
        x = x * 10 + (*q - '0');
    }
    if (x < lo || x > hi) { return 0; }
    *v = x;
    return 1;
}

/* Parse what follows the '='. The defaults are the ones the built-in names
 * always had: a drum on channel 10 at 40 ms, a voice on channel 1 at 150 ms, a
 * controller on channel 1. */
static inline int lane_def_parse(const char *arg, lane_def_t *d)
{
    memset(d, 0, sizeof *d);
    char w[16];
    const char *p = arg;
    if (lane_word(&p, w, sizeof w) == 0) {
        return d->kind = LD_REMOVE;
    }
    if (strcmp(w, "note") == 0 || strcmp(w, "voice") == 0 || strcmp(w, "cc") == 0) {
        const int note = (w[0] == 'n'), voice = (w[0] == 'v');
        d->kind = note ? LD_NOTE : voice ? LD_VOICE : LD_CC;
        d->chan = note ? 10 : 1;
        d->gate = note ? 40 : 150;
        lane_word(&p, w, sizeof w);
        if (!lane_num(w, 0, voice ? 8 : 127, &d->num)) {
            snprintf(d->why, sizeof d->why, voice ? "voice takes an octave 0-8"
                     : note ? "note takes 0-127" : "cc takes 0-127");
            return d->kind = LD_ERROR;
        }
        while (lane_word(&p, w, sizeof w) > 0) {
            char v[16];
            lane_word(&p, v, sizeof v);
            if (strcmp(w, "ch") == 0) {
                if (!lane_num(v, 1, 16, &d->chan)) {
                    snprintf(d->why, sizeof d->why, "ch is 1-16");
                    return d->kind = LD_ERROR;
                }
            } else if (strcmp(w, "gate") == 0 && d->kind != LD_CC) {
                if (!lane_num(v, 1, 5000, &d->gate)) {
                    snprintf(d->why, sizeof d->why, "gate is 1-5000 ms");
                    return d->kind = LD_ERROR;
                }
            } else {
                snprintf(d->why, sizeof d->why, "'%.10s'? ch N or gate N", w);
                return d->kind = LD_ERROR;
            }
        }
        return d->kind;
    }
    /* AN INPUT: a name whose value comes from outside - OSC to /deck/<name>
     * today, a satellite's control later (docs/NEXT.md §5, §8). Nothing else
     * goes on the line: where it is fed from is the name itself. */
    if (strcmp(w, "knob") == 0 || strcmp(w, "pad") == 0) {
        d->kind = (w[0] == 'k') ? LD_KNOB : LD_PAD;
        if (lane_word(&p, w, sizeof w) > 0) {
            snprintf(d->why, sizeof d->why, "%s takes nothing else",
                     d->kind == LD_KNOB ? "knob" : "pad");
            return d->kind = LD_ERROR;
        }
        return d->kind;
    }
    /* Anything else is the name of a picture; the caller knows which exist. */
    for (const char *q = w; *q != '\0'; q++) {
        if (!lane_is_lower(*q)) {
            snprintf(d->why, sizeof d->why, "note, voice, cc or a picture");
            return d->kind = LD_ERROR;
        }
    }
    if (lane_word(&p, d->why, sizeof d->why) > 0) {
        snprintf(d->why, sizeof d->why, "a picture takes nothing after");
        return d->kind = LD_ERROR;
    }
    const size_t wl = strlen(w);
    if (wl > LANE_BASE_MAX) {
        snprintf(d->why, sizeof d->why, "no picture is that long");
        return d->kind = LD_ERROR;
    }
    memcpy(d->draw, w, wl + 1);
    return d->kind = LD_DRAW;
}

#endif /* LANE_NAME_H */
