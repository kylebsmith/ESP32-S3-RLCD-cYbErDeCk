/*
 * The editor view.
 *
 * LAYOUT, in logical pixels on the 400 x 300 landscape panel. Every vertical
 * figure is a multiple of 12 and every horizontal one is even, because those
 * are the CASET and RASET quanta - a margin that breaks them turns a
 * one-character redraw into a three-character one.
 *
 *     y   0..11    top margin                          12
 *     y  12..251   text, 10 rows of 24                240
 *     y 252..263   gap                                 12
 *     y 264..287   status bar, one row                 24
 *     y 288..299   bottom margin                       12
 *
 *     x   0..19    left margin                         20
 *     x  20..379   text, 30 columns of 12             360
 *     x 380..399   right margin                        20
 *
 * 30 x 10 is 300 characters on screen. Narrower than the 66 x 25 that
 * docs/HARDWARE.md planned for, but that grid was unreadable on the real
 * panel, and margins are most of what makes a page look composed rather than
 * dumped.
 */
#include "editor.h"

#include <stdio.h>
#include <string.h>

#include "docstore.h"
#include "kbd.h"
#include "st7305.h"
#include "textgrid.h"

#define MARGIN_X     20
#define MARGIN_TOP   12
#define TEXT_COLS    30
#define TEXT_ROWS    10
#define CELL_W       12
#define CELL_H       24

#define STATUS_Y    264
#define STATUS_H     24
#define RULE_Y      254        /* a hairline between text and status */

#define MAX_LINES  2048

static int  s_line_start[MAX_LINES];
static int  s_line_count;
static int  s_cursor_line;
static int  s_cursor_col;
static int  s_top_line;
static bool s_cursor_on = true;

/* Where the cursor cell landed at the last draw, so a blink can repaint one
 * cell instead of the frame. */
static int  s_cur_col = -1, s_cur_row = -1;
static char s_cur_ch  = ' ';

static char s_status_shown[64];

esp_err_t editor_init(void)
{
    return tg_set_layout(&tg_font_12x24, 1,
                         MARGIN_X, MARGIN_TOP, TEXT_COLS, TEXT_ROWS);
}

/* Greedy word wrap. Records where each display line starts and where the
 * cursor lands. Prose kind: Enter splits and what follows reflows. */
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
    int last_space = -1;

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
            int brk = (last_space > line_begin) ? last_space + 1 : (int)i;
            if (brk <= line_begin) {
                brk = (int)i;
            }
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

/* The status bar is chrome, not part of the document grid, so it is drawn at
 * its own pixel row and only when its text actually changes. */
static void status_bar(void)
{
    char s[64];
    const char *net = kbd_connected() ? "KEYBOARD" : kbd_state_name();

    snprintf(s, sizeof s, "%-8.8s %4u%c u%-2d %s",
             net,
             (unsigned)doc_len(),
             doc_dirty() ? '*' : ' ',
             doc_undo_depth(),
             doc_sd_present() ? "SD" : "  ");

    if (strcmp(s, s_status_shown) == 0) {
        return;                       /* nothing changed - do not touch flash */
    }
    snprintf(s_status_shown, sizeof s_status_shown, "%s", s);

    st7305_fill(0, STATUS_Y, ST7305_WIDTH, STATUS_H, true);      /* ink bar */
    tg_draw_text_px(MARGIN_X, STATUS_Y, s, TG_INVERSE);
}

void editor_draw(void)
{
    wrap(TEXT_COLS);

    if (s_cursor_line < s_top_line) {
        s_top_line = s_cursor_line;
    }
    if (s_cursor_line >= s_top_line + TEXT_ROWS) {
        s_top_line = s_cursor_line - TEXT_ROWS + 1;
    }
    if (s_top_line < 0) {
        s_top_line = 0;
    }

    const size_t len = doc_len();
    s_cur_col = s_cur_row = -1;

    for (int r = 0; r < TEXT_ROWS; r++) {
        const int li = s_top_line + r;
        for (int c = 0; c < TEXT_COLS; c++) {
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
            const bool is_cursor = (li == s_cursor_line && c == s_cursor_col);
            if (is_cursor) {
                s_cur_col = c; s_cur_row = r; s_cur_ch = ch;
            }
            tg_put(c, r, ch,
                   (is_cursor && s_cursor_on) ? TG_INVERSE : TG_NORMAL);
        }
    }

    tg_render();

    /* Chrome, drawn over the grid. */
    st7305_fill(MARGIN_X, RULE_Y, ST7305_WIDTH - 2 * MARGIN_X, 2, true);
    status_bar();
}

/* Repaint only the cursor cell. One 12 x 24 cell is 36 bytes on the wire
 * against 15,000 for a frame, so a blink is close to free on the bus.
 *
 * docs/OS.md bans cursor blink outright, on the grounds that a slow panel
 * must never animate. The real cost is not the bus, it is that a blink keeps
 * kicking the panel into HPM and the idle-LPM policy never fires. So the
 * blink is bounded: main stops calling this once typing has paused, the
 * cursor is left solid, and the panel is allowed to drop to 1 Hz.
 */
void editor_blink(bool on)
{
    if (s_cur_col < 0 || on == s_cursor_on) {
        return;
    }
    s_cursor_on = on;
    tg_put(s_cur_col, s_cur_row, s_cur_ch, on ? TG_INVERSE : TG_NORMAL);
    tg_render();
}

void editor_cursor_solid(void)
{
    editor_blink(true);
}

/* Move the cursor to a display line and column, preserving the column where
 * the target line is long enough. This needs the wrap table, so it is rebuilt
 * first - the table from the last draw is stale the moment anything is
 * inserted. */
static void goto_line_col(int line, int want_col)
{
    wrap(TEXT_COLS);
    if (line < 0) {
        line = 0;
    }
    if (line >= s_line_count) {
        line = s_line_count - 1;
    }
    const int start = s_line_start[line];
    const int end   = (line + 1 < s_line_count)
                      ? s_line_start[line + 1] : (int)doc_len();
    int width = end - start;
    /* A hard newline is part of the line but not a column you can sit past. */
    while (width > 0 && doc_at((size_t)(start + width - 1)) == '\n') {
        width--;
    }
    int col = want_col;
    if (col > width) {
        col = width;
    }
    doc_move_to((size_t)(start + col));
}

static void line_bounds(int *start, int *end)
{
    wrap(TEXT_COLS);
    const int line = s_cursor_line;
    *start = s_line_start[line];
    *end   = (line + 1 < s_line_count)
             ? s_line_start[line + 1] : (int)doc_len();
    while (*end > *start && doc_at((size_t)(*end - 1)) == '\n') {
        (*end)--;
    }
}

static bool is_word_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

/* Ctrl chords. Chosen from readline rather than invented, because they are
 * already in a lot of people's fingers and cost no new keys on a thumb
 * keyboard - docs/OS.md is explicit that one held modifier plus one letter is
 * the ceiling for a thumb. */
static void handle_ctrl(char c)
{
    switch (c) {
    case 'z': doc_undo(); break;
    case 'y': doc_redo(); break;
    case 'a': { int s, e; line_bounds(&s, &e); doc_move_to((size_t)s); break; }
    case 'e': { int s, e; line_bounds(&s, &e); doc_move_to((size_t)e); break; }
    case 'b': doc_left();  break;
    case 'f': doc_right(); break;
    case 'p': wrap(TEXT_COLS); goto_line_col(s_cursor_line - 1, s_cursor_col); break;
    case 'n': wrap(TEXT_COLS); goto_line_col(s_cursor_line + 1, s_cursor_col); break;
    case 'k': {                      /* kill to end of line */
        int s, e;
        line_bounds(&s, &e);
        const size_t cur = doc_cursor();
        doc_move_to((size_t)e);
        while (doc_cursor() > cur) {
            doc_backspace();
        }
        break;
    }
    case 'w': {                      /* delete the word before the cursor */
        while (doc_cursor() > 0 && !is_word_char(doc_at(doc_cursor() - 1))) {
            doc_backspace();
        }
        while (doc_cursor() > 0 && is_word_char(doc_at(doc_cursor() - 1))) {
            doc_backspace();
        }
        break;
    }
    default: break;
    }
}

void editor_handle(const kbd_event_t *ev)
{
    if (ev->type == KBD_EV_CHAR && (ev->mods & (KBD_CTRL | KBD_ALT))) {
        handle_ctrl((char)(ev->ch >= 'A' && ev->ch <= 'Z'
                           ? ev->ch - 'A' + 'a' : ev->ch));
        return;
    }

    switch (ev->type) {
    case KBD_EV_CHAR:      doc_insert(ev->ch); break;
    case KBD_EV_ENTER:     doc_insert('\n');   break;
    case KBD_EV_TAB:       doc_insert(' '); doc_insert(' '); break;
    case KBD_EV_BACKSPACE: doc_backspace();   break;
    case KBD_EV_LEFT:      doc_left();        break;
    case KBD_EV_RIGHT:     doc_right();       break;
    case KBD_EV_UP:
        wrap(TEXT_COLS);
        goto_line_col(s_cursor_line - 1, s_cursor_col);
        break;
    case KBD_EV_DOWN:
        wrap(TEXT_COLS);
        goto_line_col(s_cursor_line + 1, s_cursor_col);
        break;
    case KBD_EV_HOME: { int s, e; line_bounds(&s, &e); doc_move_to((size_t)s); break; }
    case KBD_EV_END:  { int s, e; line_bounds(&s, &e); doc_move_to((size_t)e); break; }
    default: break;
    }
}
