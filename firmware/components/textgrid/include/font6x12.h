/*
 * A hand-laid 6x12 bitmap face for the cYbErDeCk text grid.
 *
 * docs/OS.md: "1 bpp, strictly. No antialiasing, ever. Hand-hinted bitmap
 * faces only; any TTF rasteriser producing coverage values produces mush."
 *
 * Cell:  6 px wide x 12 px tall.
 * Glyph: 5 px wide in columns 0..4; column 5 is the inter-character gap.
 *        Row 0 is blank (leading), rows 2..8 are the cap height, the
 *        baseline sits under row 8, rows 9..10 carry descenders, row 11 is
 *        blank so adjacent lines never touch.
 *
 * One byte per row, bit 7 = leftmost column.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#define FONT_W       6
#define FONT_H      12
#define FONT_FIRST  32
#define FONT_LAST  126

/* 12 bytes per glyph, glyphs 32..126. */
extern const uint8_t font6x12[(FONT_LAST - FONT_FIRST + 1) * FONT_H];

static inline const uint8_t *font6x12_glyph(int c)
{
    if (c < FONT_FIRST || c > FONT_LAST) {
        c = '?';
    }
    return &font6x12[(size_t)(c - FONT_FIRST) * FONT_H];
}
