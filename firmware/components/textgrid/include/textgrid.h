/*
 * The text grid.
 *
 * The cell is not a style choice (docs/HARDWARE.md): cell HEIGHT must be a
 * multiple of 12, because 12 px is the CASET quantum in landscape, but cell
 * WIDTH only has to be even, because the RASET quantum is 2 px. That asymmetry
 * is what buys a chunky face - width is free, height comes in 12s.
 *
 *   6 x 12   ->  66 x 25   the compact grid, close to the classical measure
 *  12 x 24   ->  33 x 12   the chunky grid: 4x the glyph area, which is what a
 *                          reflective panel with no backlight actually needs
 *
 * Damage is per cell, so a keystroke redraws a character and not a frame.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "st7305.h"
#include "tgfont.h"

/* Upper bounds; the live grid is tg_cols()/tg_rows(). */
#define TG_MAX_COLS 66
#define TG_MAX_ROWS 25

enum {
    TG_NORMAL  = 0,
    TG_INVERSE = 1,
};

/* Select the face and integer scale. cell = font->w * scale by font->h * scale.
 * Returns ESP_ERR_INVALID_ARG if the cell height is not a multiple of 12 or
 * the cell width is odd - the two alignment rules above, enforced rather than
 * documented. */
esp_err_t tg_set_font(const tg_font_t *font, int scale);

int tg_cols(void);
int tg_rows(void);
int tg_cell_w(void);
int tg_cell_h(void);
const char *tg_font_name(void);

void tg_clear(void);
void tg_put(int col, int row, char ch, int attr);
void tg_puts(int col, int row, const char *s, int attr);
void tg_fill(int col, int row, int count, char ch, int attr);

/* Right-align s so it ends at column `end` inclusive. */
void tg_puts_right(int end, int row, const char *s, int attr);

/* Draw dirty cells into the framebuffer without pushing, so a caller can add
 * sub-cell decoration and send one window. */
int tg_render(void);

/* Render, then push the damage. *bytes is what went over the wire. */
int tg_flush(size_t *bytes);

void tg_invalidate(void);
