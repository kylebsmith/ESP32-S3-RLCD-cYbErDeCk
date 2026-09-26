/*
 * OSC 1.0 packet READING - the other half of osc_pack.h, for '>osc in'.
 * Pure, no sockets: tools/test_osc.c runs this exact code, including against
 * datagrams packed by osc_pack.h, so the deck reads what the deck writes.
 *
 * docs/NEXT.md §8: an OSC endpoint is a lane source. '/deck/knob1' from a phone
 * or a laptop - or from another deck's '>osc' - sets the input called knob1, and
 * whatever is routed from knob1 follows it. So all this has to produce is an
 * address and a number.
 *
 * THE NUMBER IS THE LAST NUMERIC ARGUMENT. A phone sends '/deck/knob1 f 0.73';
 * another deck sends '/deck/knob1 i 74 i 93' - the controller, then its value;
 * a button may send nothing at all. The last number is the value in all three.
 *
 * A malformed datagram is refused whole. Everything is bounds-checked against
 * the datagram's length, because it arrives from anyone on the network.
 */
#ifndef OSC_PARSE_H
#define OSC_PARSE_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define OSC_ADDR_MAX 48
#define OSC_BUNDLE_DEPTH 4

typedef struct {
    char   addr[OSC_ADDR_MAX];
    int    numbers;           /* how many numeric arguments it carried */
    double last;              /* the last of them                       */
    bool   last_float;        /* ...and whether it was a float/double   */
} osc_msg_t;

typedef void (*osc_msg_fn)(const osc_msg_t *m, void *arg);

static inline uint32_t osc_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

/* A padded string at `*at`, inside [0, len). Returns its length, or -1. */
static inline int osc_rd_str(const uint8_t *b, int len, int *at)
{
    const int s = *at;
    int e = s;
    while (e < len && b[e] != '\0') {
        e++;
    }
    if (e >= len) {
        return -1;                      /* no terminator inside the datagram */
    }
    const int n = e - s;
    const int adv = ((n + 1) + 3) & ~3;
    if (s + adv > len) {
        return -1;
    }
    *at = s + adv;
    return n;
}

/* One message at [at, end); *next is where it ends. Returns 1, or -1 if it is
 * not one. */
static inline int osc_rd_msg(const uint8_t *b, int at, int end, osc_msg_t *m,
                             int *next)
{
    memset(m, 0, sizeof *m);
    if (at >= end || b[at] != '/') {
        return -1;
    }
    const int a0 = at;
    const int an = osc_rd_str(b, end, &at);
    if (an < 1 || an >= OSC_ADDR_MAX) {
        return -1;
    }
    memcpy(m->addr, b + a0, (size_t)an);
    m->addr[an] = '\0';
    *next = at;
    if (at >= end || b[at] == '/') {
        return 1;                       /* no type tags: no arguments */
    }
    if (b[at] != ',') {
        return -1;
    }
    const int t0 = at;
    const int tn = osc_rd_str(b, end, &at);
    if (tn < 1) {
        return -1;
    }
    for (int i = 1; i < tn; i++) {
        const char t = (char)b[t0 + i];
        switch (t) {
        case 'i': case 'c': case 'r': case 'm': case 'f': {
            if (at + 4 > end) { return -1; }
            const uint32_t v = osc_be32(b + at);
            at += 4;
            if (t == 'i') {
                m->last = (double)(int32_t)v; m->last_float = false; m->numbers++;
            } else if (t == 'f') {
                float f;
                memcpy(&f, &v, sizeof f);
                m->last = (double)f; m->last_float = true; m->numbers++;
            }
            break;
        }
        case 'h': case 'd': case 't': {
            if (at + 8 > end) { return -1; }
            const uint64_t v = ((uint64_t)osc_be32(b + at) << 32) |
                               (uint64_t)osc_be32(b + at + 4);
            at += 8;
            if (t == 'h') {
                m->last = (double)(int64_t)v; m->last_float = false; m->numbers++;
            } else if (t == 'd') {
                double d;
                memcpy(&d, &v, sizeof d);
                m->last = d; m->last_float = true; m->numbers++;
            }
            break;
        }
        case 's': case 'S':
            if (osc_rd_str(b, end, &at) < 0) { return -1; }
            break;
        case 'b': {
            if (at + 4 > end) { return -1; }
            const uint32_t n = osc_be32(b + at);
            at += 4;
            if (n > (uint32_t)(end - at)) { return -1; }
            at += (int)((n + 3) & ~3u);
            if (at > end) { return -1; }
            break;
        }
        case 'T': m->last = 1; m->last_float = false; m->numbers++; break;
        case 'F': m->last = 0; m->last_float = false; m->numbers++; break;
        case 'N': case 'I': case '[': case ']':
            break;
        default:
            return -1;                  /* a type we cannot step over */
        }
    }
    *next = at;
    return 1;
}

static inline int osc_rd_packet(const uint8_t *b, int at, int end, int depth,
                                osc_msg_fn fn, void *arg)
{
    if (end - at >= 16 && memcmp(b + at, "#bundle", 8) == 0) {
        if (depth >= OSC_BUNDLE_DEPTH) {
            return -1;
        }
        at += 16;                       /* the tag and the time tag */
        int total = 0;
        while (at < end) {
            if (at + 4 > end) { return -1; }
            const uint32_t n = osc_be32(b + at);
            at += 4;
            if (n == 0 || (n & 3) != 0 || n > (uint32_t)(end - at)) {
                return -1;
            }
            const int got = osc_rd_packet(b, at, at + (int)n, depth + 1, fn, arg);
            if (got < 0) {
                return -1;
            }
            total += got;
            at += (int)n;
        }
        return total;
    }
    /* MESSAGES BACK TO BACK. OSC 1.0 wants a bundle for more than one, but the
     * deck's own '>osc' puts a step's messages in one datagram end to end, and
     * this has to read another deck. Each one starts with '/', so there is no
     * ambiguity in reading them that way. */
    int total = 0;
    while (at < end) {
        osc_msg_t m;
        int next = at;
        if (osc_rd_msg(b, at, end, &m, &next) < 0 || next <= at) {
            return -1;
        }
        if (fn != NULL) {
            fn(&m, arg);
        }
        total++;
        at = next;
    }
    return total > 0 ? total : -1;
}

/* Walk a datagram - one message, or a bundle of them - and call `fn` for each
 * message. Returns how many, or -1 when it is malformed. Walked twice, the first
 * time only to check it, so a bundle with one bad message delivers none of the
 * good ones before it: refused whole, as the header says. */
static inline int osc_parse(const uint8_t *buf, int len, osc_msg_fn fn, void *arg)
{
    if (buf == NULL || len <= 0 || (len & 3) != 0) {
        return -1;
    }
    if (osc_rd_packet(buf, 0, len, 0, NULL, NULL) < 0) {
        return -1;
    }
    return osc_rd_packet(buf, 0, len, 0, fn, arg);
}

/* The value a message carries, 0-127: nothing is a press at full value; a
 * float from 0 to 1 is a fraction (what a phone's fader sends); any other number
 * is taken as 0-127 and clamped. */
static inline int osc_value_127(const osc_msg_t *m)
{
    if (m->numbers == 0) {
        return 127;
    }
    double v = m->last;
    if (m->last_float && v >= 0.0 && v <= 1.0) {
        v = v * 127.0;
    }
    if (v < 0.0) { v = 0.0; }
    if (v > 127.0) { v = 127.0; }
    return (int)(v + 0.5);
}

#endif /* OSC_PARSE_H */
