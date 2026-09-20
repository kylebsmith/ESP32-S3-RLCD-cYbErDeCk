/*
 * Host-side check for the ST7305 window arithmetic.
 *
 * It includes the firmware's own header, so it exercises the shipping code
 * rather than a reimplementation of it. docs/METHODOLOGY.md: a check that
 * computes from a copy instead of interrogating the artefact is worse than no
 * check.
 *
 *   cc -I firmware/components/st7305/include -o /tmp/t tools/test_st7305_addr.c
 *   /tmp/t
 *
 * The case that matters is NARROW_LEFT. Every full-frame driver gets the full
 * width right by accident, because at full width the mirroring is an
 * identity. This check fails on that naive implementation and passes on ours,
 * which is the standard this repository holds itself to.
 */
#include <stdio.h>
#include <string.h>

#include "st7305_addr.h"

static int failures;

static void check(const char *what, int got, int want)
{
    if (got != want) {
        printf("  [FAIL] %-46s got %d, want %d\n", what, got, want);
        failures++;
    } else {
        printf("  [ ok ] %-46s %d\n", what, got);
    }
}

/* What a driver that never discovered the mirroring would emit. */
static void naive_caset(int nx0, int nx1, uint8_t out[2])
{
    out[0] = (uint8_t)(ST7305_ADDR_START + nx0 / 12);
    out[1] = (uint8_t)(ST7305_ADDR_START + nx1 / 12);
}

int main(void)
{
    st7305_window_t w;
    uint8_t naive[2];

    printf("-- geometry --\n");
    check("framebuffer bytes",          ST7305_FB_SIZE,   15000);
    check("bytes per controller row",   ST7305_ROW_BYTES, 75);
    check("row addresses",              ST7305_ROW_ADDRS, 200);
    check("address mirror base",        ST7305_ADDR_MIRROR_BASE, 0x3C);

    printf("\n-- full width: the mirroring is an identity --\n");
    st7305_window(0, ST7305_NATIVE_W - 1, 0, ST7305_NATIVE_H - 1, &w);
    check("CASET start", w.caset[0], 0x12);
    check("CASET end",   w.caset[1], 0x2A);
    check("bytes",       w.bytes,    15000);
    naive_caset(0, ST7305_NATIVE_W - 1, naive);
    printf("  note  a naive driver emits {0x%02X, 0x%02X} here - IDENTICAL,\n"
           "        which is exactly why this bug hides from everyone\n",
           naive[0], naive[1]);

    printf("\n-- leftmost 12 px alone: where the naive version breaks --\n");
    st7305_window(0, 11, 0, ST7305_NATIVE_H - 1, &w);
    check("CASET start", w.caset[0], 0x2A);
    check("CASET end",   w.caset[1], 0x2A);
    naive_caset(0, 11, naive);
    if (naive[0] == w.caset[0] && naive[1] == w.caset[1]) {
        printf("  [FAIL] the naive implementation agrees here, so this check\n"
               "         cannot tell the two apart and proves nothing\n");
        failures++;
    } else {
        printf("  [ ok ] naive would emit {0x%02X, 0x%02X} and be WRONG -\n"
               "         this check distinguishes them\n", naive[0], naive[1]);
    }

    printf("\n-- rightmost 12 px alone --\n");
    st7305_window(ST7305_NATIVE_W - 12, ST7305_NATIVE_W - 1,
                  0, ST7305_NATIVE_H - 1, &w);
    check("CASET start", w.caset[0], 0x12);
    check("CASET end",   w.caset[1], 0x12);

    printf("\n-- cost of one character, measured on hardware as 9 and 36 --\n");
    /* A 6x12 logical cell at the origin, orientation 0: logical y -> native x
     * spans 12 px (one column address), logical x -> native y spans 6 px
     * (three row addresses). 1 * 3 * 3 = 9. */
    st7305_window(0, 11, 0, 5, &w);
    check("6x12 cell bytes",  w.bytes, 9);
    st7305_window(0, 23, 0, 11, &w);
    check("12x24 cell bytes", w.bytes, 36);
    st7305_window(0, 11, 0, ST7305_NATIVE_H - 1, &w);
    check("one full-width 12 px line", w.bytes, 600);

    printf("\n-- RASET is 2-line quantised --\n");
    st7305_window(0, 11, 0, 1, &w);
    check("two lines is one row address", w.rows, 1);
    st7305_window(0, 11, 0, 3, &w);
    check("four lines is two row addresses", w.rows, 2);

    printf("\n-- the byte is 4 x 2 px, smaller-y pixel in the higher bit --\n");
    check("mask at native (0,0)", st7305_fb_mask_n(0, 0), 0x80);
    check("mask at native (0,1)", st7305_fb_mask_n(0, 1), 0x40);
    check("mask at native (1,0)", st7305_fb_mask_n(1, 0), 0x20);
    check("mask at native (3,1)", st7305_fb_mask_n(3, 1), 0x01);
    check("index at native (0,0)",   (int)st7305_fb_index_n(0, 0), 0);
    check("index at native (4,0)",   (int)st7305_fb_index_n(4, 0), 1);
    check("index at native (0,2)",   (int)st7305_fb_index_n(0, 2), 75);

    printf("\n-- every logical pixel maps inside the framebuffer --\n");
    int worst = 0, oob = 0;
    for (int o = 0; o < 4; o++) {
        for (int y = 0; y < ST7305_HEIGHT; y++) {
            for (int x = 0; x < ST7305_WIDTH; x++) {
                int nx, ny;
                st7305_to_native(o, x, y, &nx, &ny);
                if (nx < 0 || nx >= ST7305_NATIVE_W ||
                    ny < 0 || ny >= ST7305_NATIVE_H) { oob++; continue; }
                const int i = (int)st7305_fb_index_n(nx, ny);
                if (i < 0 || i >= ST7305_FB_SIZE) { oob++; }
                if (i > worst) { worst = i; }
            }
        }
    }
    check("out-of-range mappings across all 4 orientations", oob, 0);
    check("highest framebuffer index touched", worst, ST7305_FB_SIZE - 1);

    printf("\n-- each orientation is a bijection (no pixel collides) --\n");
    static unsigned char seen[ST7305_FB_SIZE * 8];
    int collisions = 0;
    for (int o = 0; o < 4; o++) {
        memset(seen, 0, sizeof seen);
        for (int y = 0; y < ST7305_HEIGHT; y++) {
            for (int x = 0; x < ST7305_WIDTH; x++) {
                int nx, ny;
                st7305_to_native(o, x, y, &nx, &ny);
                const size_t byte = st7305_fb_index_n(nx, ny);
                const uint8_t m = st7305_fb_mask_n(nx, ny);
                int bit = 0;
                while ((m >> bit) != 1) { bit++; }
                const size_t slot = byte * 8 + (size_t)bit;
                if (seen[slot]) { collisions++; }
                seen[slot] = 1;
            }
        }
    }
    check("pixel collisions across all 4 orientations", collisions, 0);

    printf("\n%s  (%d failure%s)\n",
           failures == 0 ? "ALL CHECKS PASS" : "CHECKS FAILED",
           failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
