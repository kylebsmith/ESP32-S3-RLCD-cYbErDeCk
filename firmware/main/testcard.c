/*
 * The display test card.
 *
 * Two jobs at once: a composition worth looking at, and every element proving
 * something. The orientation tell-tales are words rather than marks, because
 * "TOP LEFT appears at the top right and reads backwards" is unambiguous in a
 * way that four small shapes were not.
 */
#include "testcard.h"

#include <stdio.h>

#include "st7305.h"
#include "textgrid.h"

static void rule(int row, int y_in_row, int h)
{
    const int x = tg_cell_w();
    st7305_fill(x, row * tg_cell_h() + y_in_row,
                ST7305_WIDTH - 2 * x, h, true);
}

void testcard_draw(const char *build_id)
{
    const int cols = tg_cols();
    const int rows = tg_rows();
    char line[80];

    tg_clear();

    /* ---- orientation tell-tales, in words ---- */
    tg_puts(0, 0, "TOP LEFT", TG_NORMAL);
    tg_puts_right(cols - 1, rows - 1, "BOTTOM RIGHT", TG_NORMAL);

    /* ---- masthead: solid inverse block, flush left ---- */
    const int block_w = cols > 24 ? 19 : cols - 2;
    for (int r = 2; r <= 3; r++) {
        tg_fill(1, r, block_w, ' ', TG_INVERSE);
    }
    tg_puts(2, 2, "CYBERDECK", TG_INVERSE);
    tg_puts(2, 3, "O S", TG_INVERSE);

    /* ---- specification ---- */
    snprintf(line, sizeof line, "ST7305 %dx%d 1BIT",
             ST7305_WIDTH, ST7305_HEIGHT);
    tg_puts(1, 5, line, TG_NORMAL);

    snprintf(line, sizeof line, "GRID %dx%d  CELL %dx%d",
             cols, rows, tg_cell_w(), tg_cell_h());
    tg_puts(1, 6, line, TG_NORMAL);

    snprintf(line, sizeof line, "ORIENT %d - KEY ROTATES",
             (int)st7305_orientation());
    tg_puts(1, 7, line, TG_NORMAL);

    /* ---- specimen ---- */
    tg_puts(1, 8,  "ABCDEFGHIJKLMNOPQRSTUVWXYZ", TG_NORMAL);
    tg_puts(1, 9,  "abcdefghijklmnopqrstuvwxyz", TG_NORMAL);
    tg_puts(1, 10, "0123456789 !?.,:;-+=*/#@&", TG_NORMAL);

    (void)build_id;

    tg_render();
    rule(4, tg_cell_h() - 5, 3);        /* heavy rule under the masthead */
    st7305_flush_full();
}
