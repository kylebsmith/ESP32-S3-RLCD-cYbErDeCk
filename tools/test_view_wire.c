/*
 * The view node's wire format (docs/VIEW.md), checked end to end on the host:
 * frames packed by the DECK's firmware/main/view_wire.h, read by the NODE's
 * view/deckview/view_read.h - the two files the two boards actually build.
 *
 * What would go wrong without it is invisible from either side: a deck that
 * packs one byte differently and a node that refuses every frame both look like
 * "the screen is black", and the node's only report is a refusal count.
 */
#include <stdio.h>
#include <string.h>

#include "view_wire.h"
#include "view_read.h"

static int fails;

#define CHECK(cond, ...) do { if (!(cond)) { printf("[FAIL] " __VA_ARGS__); \
    printf("\n"); fails++; } else { printf("[ ok ] " __VA_ARGS__); printf("\n"); } } while (0)

static view_reader_t R;

static int feed(const uint8_t *b, size_t n)
{
    int got = 0;
    for (size_t i = 0; i < n; i++) {
        got += view_read_byte(&R, b[i]);
    }
    return got;
}

int main(void)
{
    /* 1. The layout, byte for byte. */
    const uint8_t cells[6] = { 'x', 128, 155, ' ', 131, '#' };
    uint8_t f[64];
    const size_t n = view_wire_pack(f, sizeof f, 0x01020304u, 3, 2, cells);
    CHECK(n == 4 + 6 + 6 + 1, "a 3x2 frame is %zu bytes", n);
    CHECK(memcmp(f, "DKV1", 4) == 0, "it starts with the magic");
    CHECK(f[4] == 0x04 && f[7] == 0x01, "the tick is little-endian");
    CHECK(f[8] == 3 && f[9] == 2, "then the size");
    uint8_t x = 0;
    for (size_t i = 4; i < n - 1; i++) { x ^= f[i]; }
    CHECK(f[n - 1] == x, "and the XOR of everything after the magic");

    /* 2. The node reads what the deck packs. */
    memset(&R, 0, sizeof R);
    CHECK(feed(f, n) == 1, "the node reads it");
    CHECK(R.tick == 0x01020304u && R.w == 3 && R.h == 2 &&
          memcmp(R.cells, cells, 6) == 0, "tick, size and every cell arrive");

    /* 3. It finds a frame after anything - console text, half a frame - so a
     *    byte lost on the way costs one frame and not the stream. */
    memset(&R, 0, sizeof R);
    uint8_t mess[256];
    size_t m = 0;
    const char *junk = "I (123) cmd: DK DKV DKV1";   /* even a false magic */
    memcpy(mess, junk, strlen(junk)); m += strlen(junk);
    memcpy(mess + m, f, n / 2); m += n / 2;           /* a frame cut short */
    memcpy(mess + m, f, n); m += n;                   /* then a whole one */
    CHECK(feed(mess, m) >= 1 && R.tick == 0x01020304u,
          "a frame after junk and a torn frame is still read");

    /* 4. A frame with one bit wrong is refused, not drawn. */
    memset(&R, 0, sizeof R);
    uint8_t bad[64];
    memcpy(bad, f, n);
    bad[12] ^= 0x01;
    CHECK(feed(bad, n) == 0 && R.refused == 1, "a corrupted frame is refused");

    /* 4b. A stream of nothing but torn frames cannot run the reader away: every
     *     byte is accounted for, and a good frame after it is still read. */
    memset(&R, 0, sizeof R);
    static uint8_t storm[20000];
    size_t sn = 0;
    while (sn + n < sizeof storm - n) {
        memcpy(storm + sn, f, n - 3);            /* each one short of its end */
        sn += n - 3;
    }
    memcpy(storm + sn, f, n); sn += n;
    CHECK(feed(storm, sn) >= 1 && R.tick == 0x01020304u,
          "after %zu bytes of torn frames the next good one is read", sn);

    /* 5. The largest frame the deck can draw fits the reader. */
    static uint8_t big[60 * 24], bf[60 * 24 + 16];
    for (size_t i = 0; i < sizeof big; i++) { big[i] = (uint8_t)(128 + i % 28); }
    const size_t bn = view_wire_pack(bf, sizeof bf, 7, 60, 24, big);
    memset(&R, 0, sizeof R);
    CHECK(bn > 0 && feed(bf, bn) == 1 && R.w == 60 && R.h == 24 &&
          memcmp(R.cells, big, sizeof big) == 0, "a 60x24 frame round-trips");

    /* 6. Base64 as the console carries it today: RFC 4648's own vectors. */
    static const char *in[] = { "", "f", "fo", "foo", "foob", "fooba", "foobar" };
    static const char *out[] = { "", "Zg==", "Zm8=", "Zm9v", "Zm9vYg==",
                                 "Zm9vYmE=", "Zm9vYmFy" };
    for (int i = 0; i < 7; i++) {
        char b64[32];
        view_wire_base64(b64, sizeof b64, (const uint8_t *)in[i], strlen(in[i]));
        CHECK(strcmp(b64, out[i]) == 0, "base64 '%s' is '%s'", in[i], b64);
    }

    printf(fails ? "[FAIL] %d check(s) failed\n" : "[PASS] the view wire format\n",
           fails);
    return fails != 0;
}
