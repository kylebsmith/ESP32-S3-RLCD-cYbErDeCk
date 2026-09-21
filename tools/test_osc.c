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

static int fails;
static void eqi(const char *w, int got, int want)
{
    if (got != want) { printf("[FAIL] %s: got %d want %d\n", w, got, want); fails++; }
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

    printf(fails ? "[FAIL] %d check(s) failed\n" : "[PASS] OSC packing\n", fails);
    return fails != 0;
}
