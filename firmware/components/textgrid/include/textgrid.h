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
    /* A solid inverted block. THE EDIT CURSOR OWNS THIS and nothing else
     * should take it: on a one-bit panel there is no second colour, so the
     * loudest treatment must belong to the thing you can least afford to lose
     * track of. */
    TG_INVERSE = 1,
    /* A bar across the bottom of the cell, leaving the glyph readable.
     *
     * The sequencer playhead used TG_INVERSE and the owner's report was
     * immediate: "it being full black is clashing with the cursor, so when
     * you're trying to edit something while it's running it's very easy to
     * lose your cursor". Two solid blocks on a screen with one ink is two
     * things that look identical.
     *
     * This is the tracker convention, and the terminal one: the playing
     * position is marked differently from the thing you are editing. The bits
     * are a MASK, so TG_INVERSE|TG_UNDER is a cursor sitting on the playhead
     * and is legible as both - the bar inverts back out of the solid block. */
    TG_UNDER   = 2,
    /* A bar across the TOP of the cell, for a recognised command word.
     *
     * The command word was TG_INVERSE - the same solid block as the cursor.
     * The cursor XORs against it, so a cursor sitting on a command word turns
     * that cell back to normal: technically visible, practically lost, and
     * the owner reported exactly that ("the cursor gets lost navigating those
     * as well unless it's flashing").
     *
     * Three marks, three distinct treatments, all composable because they are
     * separate bits: solid block is the cursor and only the cursor, bottom bar
     * is the playhead, top bar is "the machine recognises this word". */
    TG_OVER    = 4,
};

/* Place a grid at an explicit pixel origin with an explicit size, so the UI
 * can have real margins instead of starting hard against the glass edge.
 *
 * The origin is constrained the same way the cell is: origin_y and cell height
 * must be multiples of 12, origin_x and cell width must be even. Break either
 * and one cell's damage window quantises outward into its neighbour, which
 * turns a one-character redraw into a three-character one. Enforced, not
 * documented. */
esp_err_t tg_set_layout(const tg_font_t *font, int scale,
                        int origin_x, int origin_y, int cols, int rows);

/* Convenience: the largest grid that fits, flush at the origin. */
esp_err_t tg_set_font(const tg_font_t *font, int scale);

int tg_origin_x(void);
int tg_origin_y(void);

/* Draw a string at an arbitrary pixel position, straight into the
 * framebuffer, for chrome that does not live on the document grid - the
 * status bar, rules, labels. Damages what it touches. */
void tg_draw_text_px(int px, int py, const char *s, int attr);
int  tg_text_width_px(const char *s);

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

/* Draw cells from the face pre-turned into framebuffer bytes (the default), or
 * by the per-row path. The two draw the same bytes - tools/test_textgrid.c -
 * and the switch exists so the deck can time one against the other in the
 * same boot. */
void tg_set_turned(bool on);
