/*
 * OSC 1.0 packet building. Pure, no sockets, host-testable.
 *
 * Extracted for the same reason st7305_addr.h and seq_pattern.h were: the wire
 * format is exact arithmetic that is easy to get subtly wrong and impossible to
 * debug from the far end. A missing pad byte or a little-endian int produces a
 * datagram that a receiver silently discards, and the symptom is "the visuals
 * don't work" with nothing in any log. tools/test_osc.c interrogates THIS
 * header, so the check exercises the shipping code.
 *
 * Spec: opensoundcontrol.org/spec-1_0. Strings are null-terminated and padded
 * with nulls to a multiple of four; integers are 32-bit big-endian; the type
 * tag string starts with a comma.
 */
#ifndef OSC_PACK_H
#define OSC_PACK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct { uint8_t *buf; int len, cap; } osc_t;

static inline void osc_init(osc_t *o, uint8_t *buf, int cap)
{
    o->buf = buf; o->len = 0; o->cap = cap;
}

static inline bool osc_str(osc_t *o, const char *s)
{
    const int n = (int)strlen(s) + 1;
    const int pad = (4 - (n % 4)) % 4;
    if (o->len + n + pad > o->cap) {
        return false;
    }
    memcpy(&o->buf[o->len], s, (size_t)n);
    o->len += n;
    memset(&o->buf[o->len], 0, (size_t)pad);
    o->len += pad;
    return true;
}

static inline bool osc_i32(osc_t *o, int32_t v)
{
    if (o->len + 4 > o->cap) {
        return false;
    }
    o->buf[o->len++] = (uint8_t)((uint32_t)v >> 24);
    o->buf[o->len++] = (uint8_t)((uint32_t)v >> 16);
    o->buf[o->len++] = (uint8_t)((uint32_t)v >> 8);
    o->buf[o->len++] = (uint8_t)(uint32_t)v;
    return true;
}

/* A whole message, atomically: on failure the buffer is left exactly as it was
 * so a half-written message can never reach the wire. A malformed datagram is
 * worse than a dropped one, because a receiver may reject all of it and lose
 * the messages that were fine. */
static inline bool osc_msg_ii(osc_t *o, const char *addr, int32_t a, int32_t b)
{
    const int mark = o->len;
    if (osc_str(o, addr) && osc_str(o, ",ii") && osc_i32(o, a) && osc_i32(o, b)) {
        return true;
    }
    o->len = mark;
    return false;
}

static inline bool osc_msg_i(osc_t *o, const char *addr, int32_t a)
{
    const int mark = o->len;
    if (osc_str(o, addr) && osc_str(o, ",i") && osc_i32(o, a)) {
        return true;
    }
    o->len = mark;
    return false;
}

static inline bool osc_msg_s(osc_t *o, const char *addr, const char *s)
{
    const int mark = o->len;
    if (osc_str(o, addr) && osc_str(o, ",s") && osc_str(o, s)) {
        return true;
    }
    o->len = mark;
    return false;
}

#endif /* OSC_PACK_H */
