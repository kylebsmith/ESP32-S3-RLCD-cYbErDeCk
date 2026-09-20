/*
 * The editor view.
 *
 * One buffer, soft-wrapped to the grid, with the last row given to a status
 * line. docs/SUBSTRATE.md calls the grid the substrate and this is the first
 * instance of it: a rectangle of characters with a cursor in it.
 *
 * Wrapping is greedy word wrap - "prose" kind, where Enter splits and the
 * text below reflows. The "grid" kind, where position is the meaning and
 * reflow is destruction, is the same array with one bit of interpretation
 * changed; it is not implemented tonight but nothing here forecloses it.
 */
#include "editor.h"

#include <stdio.h>
#include <string.h>

#include "docstore.h"
#include "kbd.h"
#include "st7305.h"
#include "textgrid.h"

#define MAX_LINES 2048

static int    s_line_start[MAX_LINES];
static int    s_line_count;
static int    s_cursor_line;
static int    s_cursor_col;
static int    s_top_line;       /* first visible line */

/* Greedy word wrap over the document. Records where each display line starts
 * and where the cursor lands. */
static void wrap(int cols)
{
    const size_t len = doc_len();
    const size_t cur = doc_cursor();

    s_line_count  = 0;
    s_cursor_line = 0;
    s_cursor_col  = 0;

    size_t i = 0;
    int line_begin = 0;
    int col = 0;
    int last_space = -1;          /* index of the last space on this line */

    s_line_start[s_line_count++] = 0;

    while (i <= len) {
        if (i == cur) {
            s_cursor_line = s_line_count - 1;
            s_cursor_col  = col;
        }
        if (i == len) {
            break;
        }
        const char c = doc_at(i);

        if (c == '\n') {
            i++;
            line_begin = (int)i;
            col = 0;
            last_space = -1;
            if (s_line_count < MAX_LINES) {
                s_line_start[s_line_count++] = line_begin;
            }
            continue;
        }

        if (c == ' ') {
            last_space = (int)i;
        }

        col++;
        i++;

        if (col >= cols) {
            /* Break after the last space if there was one, so words stay whole. */
            int brk = (last_space > line_begin) ? last_space + 1 : (int)i;
            if (brk <= line_begin) {
                brk = (int)i;
            }
            /* The cursor may belong to the line we just closed. */
            if ((size_t)brk > cur && cur >= (size_t)line_begin) {
                s_cursor_line = s_line_count - 1;
                s_cursor_col  = (int)(cur - line_begin);
            }
            i = (size_t)brk;
            line_begin = brk;
            col = 0;
            last_space = -1;
            if (s_line_count < MAX_LINES) {
                s_line_start[s_line_count++] = line_begin;
            }
        }
    }
}

static void status_line(int row, int cols)
{
    char s[80];
    const char *net = kbd_connected() ? "KB" : kbd_state_name();

    snprintf(s, sizeof s, " %-9.9s %4uc s%-3u %s",
             net,
             (unsigned)doc_len(),
             (unsigned)doc_save_seq(),
             doc_sd_present() ? "SD" : "--");

    tg_fill(0, row, cols, ' ', TG_INVERSE);
    tg_puts(0, row, s, TG_INVERSE);

    /* A dirty marker on the right edge: this is the only signal that a save
     * is still owed, and it costs nothing to hold on a reflective panel. */
    tg_put(cols - 2, row, doc_dirty() ? '*' : ' ', TG_INVERSE);
}

void editor_draw(void)
{
    const int cols = tg_cols();
    const int rows = tg_rows();
    const int text_rows = rows - 1;

    wrap(cols);

    /* Keep the cursor on screen. */
    if (s_cursor_line < s_top_line) {
        s_top_line = s_cursor_line;
    }
    if (s_cursor_line >= s_top_line + text_rows) {
        s_top_line = s_cursor_line - text_rows + 1;
    }
    if (s_top_line < 0) {
        s_top_line = 0;
    }

    const size_t len = doc_len();

    for (int r = 0; r < text_rows; r++) {
        const int li = s_top_line + r;
        for (int c = 0; c < cols; c++) {
            char ch = ' ';
            if (li < s_line_count) {
                const int start = s_line_start[li];
                const int end   = (li + 1 < s_line_count)
                                  ? s_line_start[li + 1] : (int)len;
                const int idx = start + c;
                if (idx < end) {
                    const char d = doc_at((size_t)idx);
                    ch = (d == '\n') ? ' ' : d;
                }
            }
            /* The cursor is drawn as an inverse cell - no blink, because a
             * slow panel must never animate (docs/OS.md). */
            const bool is_cursor = (li == s_cursor_line && c == s_cursor_col);
            tg_put(c, r, ch, is_cursor ? TG_INVERSE : TG_NORMAL);
        }
    }

    status_line(rows - 1, cols);
}

void editor_handle(const kbd_event_t *ev)
{
    switch (ev->type) {
    case KBD_EV_CHAR:      doc_insert(ev->ch); break;
    case KBD_EV_ENTER:     doc_insert('\n');   break;
    case KBD_EV_TAB:       doc_insert(' '); doc_insert(' '); break;
    case KBD_EV_BACKSPACE: doc_backspace();   break;
    case KBD_EV_LEFT:      doc_left();        break;
    case KBD_EV_RIGHT:     doc_right();       break;
    case KBD_EV_UP:
    case KBD_EV_DOWN:
        /* Vertical motion needs the wrap table, which editor_draw rebuilds.
         * Moving by a display line is a column-preserving walk; for tonight a
         * whole-line jump is close enough and never loses the cursor. */
        for (int i = 0; i < tg_cols(); i++) {
            if (ev->type == KBD_EV_UP) { doc_left(); } else { doc_right(); }
        }
        break;
    default: break;
    }
}
