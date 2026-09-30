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

    /* 5. The largest frame the deck can draw (VIZ_W x VIZ_H, 80 x 30) fits the
     *    reader. */
    static uint8_t big[80 * 30], bf[80 * 30 + 16];
    for (size_t i = 0; i < sizeof big; i++) { big[i] = (uint8_t)(128 + i % 28); }
    const size_t bn = view_wire_pack(bf, sizeof bf, 7, 80, 30, big);
    memset(&R, 0, sizeof R);
    CHECK(bn > 0 && feed(bf, bn) == 1 && R.w == 80 && R.h == 30 &&
          memcmp(R.cells, big, sizeof big) == 0, "an 80x30 frame round-trips");

    /* 7. The control frame: the mode, and the poster's lines with their spans.
     *    The deck and the node must agree on the limits, or a long poster is a
     *    refused frame and a black screen. */
    CHECK(VIEW_READ_TEXT_MAX == VIEW_TEXT_MAX && VIEW_READ_LINES_MAX == VIEW_LINES_MAX,
          "deck and node agree on the control frame's limits");
    const view_line_t lines[3] = {
        { "ORBITALS", 0, 0 },
        { "kick 9...8...9...8...", 9, 10 },
        { "hat ..3...3...3...4.", 30, 31 },         /* a span past the end */
    };
    uint8_t c[256];
    const size_t cn = view_wire_pack_ctl(c, sizeof c, 0x0a0b0c0du, VIEW_MODE_POSTER,
                                         lines, 3);
    CHECK(cn > 0 && memcmp(c, "DKC1", 4) == 0 && c[4] == 0x0d && c[8] == VIEW_MODE_POSTER &&
          c[9] == 3, "a control frame is magic, tick, mode and a count of lines");
    const size_t tl = (size_t)(c[10] | (c[11] << 8));
    CHECK(tl == (2 + 8 + 1) + (2 + 21 + 1) + (2 + 20 + 1) && cn == 12 + tl + 1,
          "then the text's length, the text, and the sum (%zu bytes)", cn);
    memset(&R, 0, sizeof R);
    CHECK(feed(c, cn) == VR_CONTROL && R.mode == VIEW_MODE_POSTER && R.nlines == 3 &&
          R.ctl_tick == 0x0a0b0c0du, "the node reads the mode and the count");
    const uint8_t *ch = NULL;
    int from = 0, to = 0;
    int k = view_read_line(&R, 1, &ch, &from, &to);
    CHECK(k == 21 && memcmp(ch, "kick 9...8...9...8...", 21) == 0 && from == 9 && to == 10,
          "and each line with the span it lights");
    k = view_read_line(&R, 2, &ch, &from, &to);
    CHECK(k == 20 && from == 0 && to == 0, "a span past the end of its line lights nothing");
    CHECK(view_read_line(&R, 3, &ch, &from, &to) < 0, "there is no fourth line");

    /* 8. As the deck sends them - a control frame, then its picture - and after
     *    junk, a torn control frame and a torn picture. */
    memset(&R, 0, sizeof R);
    static uint8_t both[2048];
    size_t bo = 0;
    memcpy(both + bo, "log line DKC", 12); bo += 12;
    memcpy(both + bo, c, cn / 2); bo += cn / 2;       /* a control frame cut short */
    memcpy(both + bo, f, n / 2); bo += n / 2;         /* a picture cut short */
    memcpy(both + bo, c, cn); bo += cn;
    memcpy(both + bo, f, n); bo += n;
    CHECK(feed(both, bo) == (VR_CONTROL | VR_PICTURE) && R.mode == VIEW_MODE_POSTER &&
          R.tick == 0x01020304u && R.nlines == 3, "both kinds are read after junk and tears");

    /* 9. A control frame with a bit wrong is refused, and the last good one
     *    stands - the node keeps drawing in the mode it was given. */
    uint8_t cb[256];
    const size_t cbn = view_wire_pack_ctl(cb, sizeof cb, 1, VIEW_MODE_SCAN, lines, 1);
    cb[13] ^= 0x40;
    const uint32_t before = R.refused;
    CHECK(feed(cb, cbn) == 0 && R.refused > before && R.mode == VIEW_MODE_POSTER,
          "a corrupted control frame is refused and the mode stands");

    /* 10. Modes the deck cannot name, and lines that do not fit, are not sent. */
    CHECK(view_wire_pack_ctl(c, sizeof c, 0, VIEW_MODES, NULL, 0) == 0,
          "an unknown mode is not packed");
    CHECK(view_wire_pack_ctl(c, sizeof c, 0, VIEW_MODE_RISO, NULL, 0) == 12 + 1,
          "a mode alone is thirteen bytes");
    view_line_t many[VIEW_LINES_MAX + 1];
    for (int i = 0; i <= VIEW_LINES_MAX; i++) { many[i].text = "x"; many[i].from = many[i].to = 0; }
    CHECK(view_wire_pack_ctl(c, sizeof c, 0, 0, many, VIEW_LINES_MAX + 1) == 0,
          "more than %d lines is not packed", VIEW_LINES_MAX);
    char longl[200];
    memset(longl, 'y', sizeof longl - 1);
    longl[sizeof longl - 1] = '\0';
    const view_line_t cut = { longl, VIEW_LINE_MAX + 10, VIEW_LINE_MAX + 12 };
    const size_t ln = view_wire_pack_ctl(c, sizeof c, 0, 0, &cut, 1);
    memset(&R, 0, sizeof R);
    feed(c, ln);
    k = view_read_line(&R, 0, &ch, &from, &to);
    CHECK(k == VIEW_LINE_MAX && from == 0 && to == 0,
          "a long line is cut at %d characters and its span past the cut dropped", VIEW_LINE_MAX);

    /* 11. DKC2 carries the view's parameters - controllers 1-8 on channel 16,
     *     255 for unset - and a DKC1 frame still reads, every parameter unset. */
    const uint8_t par[VIEW_PARAMS] = { 0, 64, 127, VIEW_PARAM_UNSET, 1, 2, 3, 4 };
    const size_t c2n = view_wire_pack_ctl2(c, sizeof c, 7, VIEW_MODE_PLAIN, par, lines, 1);
    CHECK(c2n > 0 && memcmp(c, "DKC2", 4) == 0 && c2n == VIEW_CTL2_HEAD_LEN + (2 + 8 + 1) + 1,
          "a DKC2 frame is DKC1 with eight parameters (%zu bytes)", c2n);
    memset(&R, 0, sizeof R);
    CHECK(feed(c, c2n) == VR_CONTROL && memcmp(R.params, par, VIEW_PARAMS) == 0 &&
          R.nlines == 1 && R.mode == VIEW_MODE_PLAIN, "the node reads the parameters");
    k = view_read_line(&R, 0, &ch, &from, &to);
    CHECK(k == 8 && memcmp(ch, "ORBITALS", 8) == 0, "and the lines after them");
    uint8_t c1[64];
    const size_t c1n = view_wire_pack_ctl(c1, sizeof c1, 8, VIEW_MODE_SCAN, NULL, 0);
    CHECK(feed(c1, c1n) == VR_CONTROL && R.mode == VIEW_MODE_SCAN &&
          R.params[0] == VIEW_PARAM_UNSET && R.params[7] == VIEW_PARAM_UNSET,
          "a DKC1 frame leaves every parameter unset");
    const size_t c3n = view_wire_pack_ctl2(c, sizeof c, 9, VIEW_MODE_RISO, par, NULL, 0);
    c[VIEW_CTL_HEAD_LEN + 2] ^= 0x01;
    const uint32_t was = R.refused;
    CHECK(feed(c, c3n) == 0 && R.refused > was && R.mode == VIEW_MODE_SCAN,
          "a parameter with a bit wrong is refused, and the mode stands");
    static uint8_t mix[512];
    size_t mo = 0;
    const size_t c4n = view_wire_pack_ctl2(mix + 40, sizeof mix - 40, 10, VIEW_MODE_POSTER, par, NULL, 0);
    memcpy(mix, mix + 40, c4n / 2); mo = c4n / 2;                /* torn */
    memmove(mix + mo, mix + 40, c4n); mo += c4n;                 /* whole */
    memset(&R, 0, sizeof R);
    CHECK(feed(mix, mo) == VR_CONTROL && R.mode == VIEW_MODE_POSTER && R.params[2] == 127,
          "a torn DKC2 costs itself and the next is read");
    CHECK(VIEW_CTL2_HEAD_LEN + VIEW_TEXT_MAX + 1 <= VIEW_RAW_MAX,
          "the largest control frame fits the node's buffer");

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
