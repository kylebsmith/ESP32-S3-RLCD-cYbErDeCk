/*
 * A bitmap face. One row of a glyph is `stride` bytes, MSB = leftmost column,
 * so a face up to 8 px wide costs 1 byte a row and up to 16 px costs 2.
 *
 * Monospace is structural, not a property of the art: every glyph occupies the
 * same w x h cell and advances the same distance. ASCII art therefore lines up
 * exactly, which matters because the grid is the substrate (docs/SUBSTRATE.md).
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char    *name;
    uint8_t        w;        /* glyph cell width in pixels  */
    uint8_t        h;        /* glyph cell height in pixels */
    uint8_t        stride;   /* bytes per glyph row         */
    uint8_t        first;    /* first codepoint present     */
    uint8_t        last;     /* last codepoint present      */
    const uint8_t *data;     /* (last-first+1) * h * stride */
} tg_font_t;

static inline const uint8_t *tg_font_glyph(const tg_font_t *f, int c)
{
    if (c < f->first || c > f->last) {
        c = '?';
        if (c < f->first || c > f->last) {
            c = f->first;
        }
    }
    return &f->data[(size_t)(c - f->first) * f->h * f->stride];
}

/* Is a pixel of a glyph row set? */
static inline int tg_font_bit(const tg_font_t *f, const uint8_t *row, int gx)
{
    return (row[gx >> 3] >> (7 - (gx & 7))) & 1;
}

extern const tg_font_t tg_font_6x12;     /* compact: 66 x 25 */
extern const tg_font_t tg_font_8x24;     /* middle:  45 x 12 - the chunky art
                                          * squeezed to 9 columns, so it keeps
                                          * the weight the panel needs while
                                          * fitting half again as much code.
                                          * HEIGHT must be a multiple of 12 and
                                          * WIDTH must be even - the CASET and
                                          * RASET quanta - so a middle size is
                                          * a narrower cell, never a shorter
                                          * one, and 9x18 and 9x24 are both
                                          * unbuildable on this panel. */
extern const tg_font_t tg_font_12x24;    /* chunky:  33 x 12 - the default,
                                          * because a reflective panel with no
                                          * backlight needs the weight */
