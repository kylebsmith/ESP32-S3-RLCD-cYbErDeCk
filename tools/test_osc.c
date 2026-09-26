/*
 * The OSC wire format, checked against the shipping header.
 *
 * A missing pad byte or a little-endian integer produces a datagram a receiver
 * silently discards - the symptom is "the visuals don't work" with nothing in
 * any log on either side. This is the cheapest possible place to catch that.
 */
#include <stdio.h>
#include <string.h>

#include "osc_pack.h"
#include "osc_parse.h"

static int fails;
static void eqi(const char *w, int got, int want)
{
    if (got != want) { printf("[FAIL] %s: got %d want %d\n", w, got, want); fails++; }
}

/* ---- reading ------------------------------------------------------------ */

static osc_msg_t seen[8];
static int nseen;

static void keep(const osc_msg_t *m, void *arg)
{
    (void)arg;
    if (nseen < 8) { seen[nseen] = *m; }
    nseen++;
}

static int parse(const uint8_t *b, int n)
{
    nseen = 0;
    return osc_parse(b, n, keep, NULL);
}

/* A message with one float, packed by hand: osc_pack.h has no float. */
static int msg_f(uint8_t *b, const char *addr, float f)
{
    osc_t o;
    osc_init(&o, b, 64);
    osc_str(&o, addr);
    osc_str(&o, ",f");
    uint32_t v;
    memcpy(&v, &f, sizeof v);
    osc_i32(&o, (int32_t)v);
    return o.len;
}

static void check(const char *what, int ok)
{
    if (!ok) { printf("[FAIL] %s\n", what); fails++; }
    else     { printf("[ ok ] %s\n", what); }
}

static void reading(void)
{
    uint8_t b[256];
    osc_t o;

    /* Another deck's '>osc' output reads back as what it said. */
    osc_init(&o, b, sizeof b);
    osc_msg_ii(&o, "/deck/knob1", 74, 93);
    check("another deck's /deck/knob1 i 74 i 93 reads as knob1 = 93",
          parse(b, o.len) == 1 && strcmp(seen[0].addr, "/deck/knob1") == 0 &&
          seen[0].numbers == 2 && osc_value_127(&seen[0]) == 93);

    /* A phone's fader: a float from 0 to 1. */
    int n = msg_f(b, "/deck/knob2", 0.5f);
    check("a phone's fader at 0.5 is 64",
          parse(b, n) == 1 && osc_value_127(&seen[0]) == 64);
    n = msg_f(b, "/deck/knob2", 1.0f);
    check("and at 1.0 is 127", parse(b, n) == 1 && osc_value_127(&seen[0]) == 127);
    n = msg_f(b, "/deck/knob2", 64.0f);
    check("a float past 1 is taken as 0-127", parse(b, n) == 1 &&
          osc_value_127(&seen[0]) == 64);

    /* A button with nothing to say: a press, at full value. */
    osc_init(&o, b, sizeof b);
    osc_str(&o, "/deck/pad1");
    osc_str(&o, ",");
    check("a message with no arguments is 127",
          parse(b, o.len) == 1 && seen[0].numbers == 0 &&
          osc_value_127(&seen[0]) == 127);

    /* Clamped, both ways. */
    osc_init(&o, b, sizeof b);
    osc_msg_i(&o, "/deck/knob1", -5);
    osc_msg_i(&o, "/deck/knob1", 300);
    check("-5 is 0 and 300 is 127, and two messages end to end are two",
          parse(b, o.len) == 2 && osc_value_127(&seen[0]) == 0 &&
          osc_value_127(&seen[1]) == 127);

    /* A bundle of two, and a bundle inside a bundle. */
    uint8_t m1[64], m2[64];
    osc_t a, c;
    osc_init(&a, m1, sizeof m1);
    osc_msg_i(&a, "/deck/knob1", 10);
    osc_init(&c, m2, sizeof m2);
    osc_msg_i(&c, "/deck/pad1", 1);
    osc_init(&o, b, sizeof b);
    memcpy(b, "#bundle\0", 8); o.len = 8;
    osc_i32(&o, 0); osc_i32(&o, 1);                 /* time tag: immediately */
    osc_i32(&o, a.len); memcpy(b + o.len, m1, (size_t)a.len); o.len += a.len;
    osc_i32(&o, c.len); memcpy(b + o.len, m2, (size_t)c.len); o.len += c.len;
    const int flat = o.len;
    check("a bundle of two is two messages", parse(b, flat) == 2 &&
          strcmp(seen[1].addr, "/deck/pad1") == 0);
    uint8_t outer[320];
    osc_init(&o, outer, sizeof outer);
    memcpy(outer, "#bundle\0", 8); o.len = 8;
    osc_i32(&o, 0); osc_i32(&o, 1);
    osc_i32(&o, flat); memcpy(outer + o.len, b, (size_t)flat); o.len += flat;
    check("and inside another bundle, still two", parse(outer, o.len) == 2);

    /* REFUSED WHOLE. A datagram arrives from anyone on the network. */
    osc_init(&o, b, sizeof b);
    osc_msg_i(&o, "/deck/knob1", 10);
    osc_msg_ii(&o, "/deck/knob1", 20, 30);
    check("a second message cut short refuses the first one too",
          parse(b, o.len - 4) == -1 && nseen == 0);
    check("a length that is not whole words is refused", parse(b, 6) == -1);
    memset(b, 'a', 64); b[0] = '/';
    check("an address with no end is refused", parse(b, 64) == -1 && nseen == 0);
    osc_init(&o, b, sizeof b);
    osc_msg_i(&o, "/deck/an-address-far-too-long-for-any-name-at-all", 1);
    check("an address past 47 characters is refused", parse(b, o.len) == -1);
    osc_init(&o, b, sizeof b);
    memcpy(b, "#bundle\0", 8); o.len = 8;
    osc_i32(&o, 0); osc_i32(&o, 1); osc_i32(&o, 4096);
    check("a bundle element longer than the datagram is refused",
          parse(b, o.len) == -1);
    osc_init(&o, b, sizeof b);
    osc_str(&o, "/deck/x"); osc_str(&o, ",x"); osc_i32(&o, 1);
    check("a type it cannot step over is refused", parse(b, o.len) == -1);
    osc_init(&o, b, sizeof b);
    osc_str(&o, "deck/x"); osc_str(&o, ",i"); osc_i32(&o, 1);
    check("a message that does not start with / is refused", parse(b, o.len) == -1);
}

int main(void)
{
    uint8_t b[256];
    osc_t o;

    /* Every message must be a multiple of four bytes long. A receiver walks
     * the datagram in words; anything else desynchronises it. */
    const char *addrs[] = { "/deck/kick", "/deck/a", "/deck/hihat", "/x",
                            "/deck/twelvechar" };
    for (size_t i = 0; i < sizeof addrs / sizeof addrs[0]; i++) {
        osc_init(&o, b, sizeof b);
        if (!osc_msg_ii(&o, addrs[i], 36, 100)) {
            printf("[FAIL] %s would not pack\n", addrs[i]); fails++; continue;
        }
        if (o.len % 4 != 0) {
            printf("[FAIL] %s packs to %d bytes, not a multiple of 4\n",
                   addrs[i], o.len);
            fails++;
        }
    }

    /* Exact layout for a known message: "/deck/kick" is 10 chars, so 11 with
     * the null, padded to 12. ",ii" is 3, padded to 4. Two ints are 8. */
    osc_init(&o, b, sizeof b);
    osc_msg_ii(&o, "/deck/kick", 36, 100);
    eqi("known message length", o.len, 12 + 4 + 8);
    if (memcmp(b, "/deck/kick\0\0", 12) != 0) {
        printf("[FAIL] address is not null-padded to 12\n"); fails++;
    }
    if (memcmp(b + 12, ",ii\0", 4) != 0) {
        printf("[FAIL] type tag is not \",ii\" padded to 4\n"); fails++;
    }
    /* BIG endian. This is the one that produces a silently-ignored packet. */
    eqi("int is big-endian b0", b[16], 0);
    eqi("int is big-endian b3", b[19], 36);
    eqi("second int", b[23], 100);

    /* A string whose length is already a multiple of four still gets a whole
     * pad word, because the null must be there. "/abc" is 4 chars -> 8 bytes. */
    osc_init(&o, b, sizeof b);
    osc_str(&o, "/abc");
    eqi("4-char string pads to 8", o.len, 8);

    /* Overflow must leave the buffer untouched, not half-written. */
    uint8_t tiny[8];
    osc_init(&o, tiny, sizeof tiny);
    if (osc_msg_ii(&o, "/deck/kick", 1, 2)) {
        printf("[FAIL] packed a message into a buffer too small\n"); fails++;
    }
    eqi("failed pack leaves nothing", o.len, 0);

    /* Several messages concatenate with no separator - that is how a receiver
     * reads more than one from a single datagram. */
    osc_init(&o, b, sizeof b);
    osc_msg_i(&o, "/deck/step", 7);
    const int after_first = o.len;
    osc_msg_ii(&o, "/deck/kick", 36, 100);
    eqi("two messages concatenate", o.len, after_first + 24);

    osc_init(&o, b, sizeof b);
    osc_msg_s(&o, "/deck/frame", "hi");
    eqi("string arg message", o.len % 4, 0);

    /* ---- reading, for '>osc in' (osc_parse.h) ---------------------------- */
    reading();

    printf(fails ? "[FAIL] %d check(s) failed\n" : "[PASS] OSC packing and reading\n",
           fails);
    return fails != 0;
}
