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
#include "cmd.h"
#include "kbd.h"
#include "st7305.h"
#include "esp_log.h"
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
/* How far back from the cursor the wrap window starts. The old code wrapped
 * from byte zero, so a document past MAX_LINES display lines stopped growing
 * the table, the cursor line was never found, and the view froze with every
 * further keystroke invisible. Wrapping a bounded window around the cursor
 * removes the cliff and makes the cost independent of document length. */
#define WRAP_BACK  6000

static int  s_line_start[MAX_LINES];
static int  s_line_count;
static int  s_cursor_line;
static int  s_cursor_col;
/* The view is anchored to a document OFFSET, not a line index. wrap() builds
 * its table from a window starting at cursor-WRAP_BACK, so the origin moves
 * as the document grows and every line index shifts underneath it. A line
 * number was a coordinate in a sliding frame; an offset is not. */
static int  s_top_offset;

/* The sticky goal column. Vertical motion CONSUMES it but never overwrites
 * it: goto_line_col clamps to the target line's width and the next wrap()
 * re-derives the column from that clamped position, so without this a single
 * short line passed through en route loses the column for good.
 * -1 means "take it from wherever the cursor is now". */
static int  s_goal_col = -1;

static bool s_cursor_on = true;

/* Where the cursor cell landed at the last draw, so a blink can repaint one
 * cell instead of the frame. */
static int  s_cur_col = -1, s_cur_row = -1;
static char s_cur_ch  = ' ';

static char s_status_shown[64];
/* The result of the last command, shown in place of the status line for a
 * few seconds. A command that reports nothing is indistinguishable from one
 * that did not run. */
static char    s_msg[64];
static int64_t s_msg_until;
static bool s_chrome_dirty = true;
static uint32_t s_pushes, s_push_bytes;

void editor_vitals(uint32_t *pushes, uint32_t *bytes)
{
    *pushes = s_pushes;
    *bytes  = s_push_bytes;
    s_pushes = 0;
    s_push_bytes = 0;
}

void editor_invalidate(void)
{
    s_chrome_dirty = true;
    s_status_shown[0] = '\0';
}

esp_err_t editor_init(void)
{
    return tg_set_layout(&tg_font_12x24, 1,
                         MARGIN_X, MARGIN_TOP, TEXT_COLS, TEXT_ROWS);
}

/* Greedy word wrap. Records where each display line starts and where the
 * cursor lands. Prose kind: Enter splits and what follows reflows. */
static int s_wrap_origin;       /* document offset the table starts at */

static void wrap(int cols)
{
    const size_t len = doc_len();
    const size_t cur = doc_cursor();

    /* Start a bounded distance before the cursor, then advance to the first
     * character after a newline so the window begins on a real line boundary
     * - wrapping is independent of anything before that point. */
    size_t origin = cur > WRAP_BACK ? cur - WRAP_BACK : 0;
    if (origin > 0) {
        while (origin < cur && doc_at(origin - 1) != '\n') {
            origin++;
        }
    }
    s_wrap_origin = (int)origin;

    s_line_count  = 0;
    s_cursor_line = 0;
    s_cursor_col  = 0;

    size_t i = origin;
    int line_begin = (int)origin;
    int col = 0;
    int last_space = -1;

    s_line_start[s_line_count++] = (int)origin;

    while (i <= len) {
        if (i == cur) {
            s_cursor_line = s_line_count - 1;
            s_cursor_col  = col;
        }
        if (i == len || s_line_count >= MAX_LINES) {
            break;
        }
        const char ch = doc_at(i);

        if (ch == '\n') {
            i++;
            line_begin = (int)i;
            col = 0;
            last_space = -1;
            s_line_start[s_line_count++] = line_begin;
            continue;
        }
        if (ch == ' ') {
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
            s_line_start[s_line_count++] = line_begin;
        }
    }
}

/* The status bar is chrome, not part of the document grid, so it is drawn at
 * its own pixel row and only when its text actually changes. */
int64_t editor_now_ms(void);

static void status_bar(void)
{
    char s[64];
    if (s_msg[0] != '\0' && editor_now_ms() < s_msg_until) {
        if (strcmp(s_msg, s_status_shown) == 0) {
            return;
        }
        snprintf(s_status_shown, sizeof s_status_shown, "%s", s_msg);
        st7305_fill(0, STATUS_Y, ST7305_WIDTH, STATUS_H, true);
        tg_draw_text_px(MARGIN_X, STATUS_Y, s_msg, TG_INVERSE);
        return;
    }
    if (s_msg[0] != '\0' && editor_now_ms() >= s_msg_until) {
        s_msg[0] = '\0';
        s_status_shown[0] = '\0';      /* force the real status back */
    }
    const char *net = kbd_connected() ? "KEYBOARD" : kbd_state_name();

    /* Cursor position earns its place: a text editor that cannot tell you
     * where the cursor is makes every navigation bug invisible, and on a
     * 30-column screen the cursor is easy to lose. */
    snprintf(s, sizeof s, "%-3.3s%5u%c %2d:%-2d u%-2d %s",
             kbd_connected() ? "KBD" : "...",
             (unsigned)doc_len(),
             doc_dirty() ? '*' : ' ',
             s_cursor_line + 1, s_cursor_col + 1,
             doc_undo_depth(),
             doc_sd_present() ? "SD" : "  ");
    (void)net;

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

    /* Recover the top line from the anchored offset, then scroll to keep the
     * cursor on screen, then re-anchor. */
    int top = 0;
    for (int i = 0; i < s_line_count; i++) {
        if (s_line_start[i] <= s_top_offset) {
            top = i;
        } else {
            break;
        }
    }
    if (s_cursor_line < top) {
        top = s_cursor_line;
    }
    if (s_cursor_line >= top + TEXT_ROWS) {
        top = s_cursor_line - TEXT_ROWS + 1;
    }
    if (top < 0) {
        top = 0;
    }
    s_top_offset = s_line_start[top];
    const int s_top_line = top;

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

    /* Cells first, then sub-cell chrome on top of them. Note that this
     * RENDERS but does not PUSH: the caller pushes once, with editor_present,
     * after the chrome is in the framebuffer too. */
    tg_render();

    /* Chrome, drawn over the grid. The rule is static, so it is painted only
     * when the whole screen is being repainted anyway - redrawing it every
     * frame dragged the damage rectangle down across the entire panel. */
    if (s_chrome_dirty) {
        st7305_fill(MARGIN_X, RULE_Y, ST7305_WIDTH - 2 * MARGIN_X, 2, true);
        s_chrome_dirty = false;
    }
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

/* Push whatever editor_draw/editor_blink put in the framebuffer.
 *
 * This exists because the alternative was a silent no-op. tg_flush() renders
 * AND pushes, but editor_draw has already rendered - so tg_flush saw zero
 * dirty cells, concluded there was nothing to send, and skipped the SPI push
 * entirely. Every edit was drawn perfectly into the framebuffer and never
 * reached the glass. Splitting render from present makes that mistake
 * impossible to make again by accident. */
void editor_present(size_t *bytes)
{
    size_t n = 0;
    st7305_flush(&n);
    if (bytes != NULL) {
        *bytes = n;
    }
    /* Counted, not logged. A capped one-shot probe went quiet after 24 pushes
     * and made a perfectly healthy device look hung - the screen had stopped
     * blinking by design at the same moment, so both signals died together.
     * The heartbeat reports these instead, so quiet always means idle and
     * never means broken. */
    if (n > 0) {
        s_pushes++;
        s_push_bytes += n;
    }
}

void editor_cursor_solid(void)
{
    editor_blink(true);
}

/* Move the cursor to a display line and column, preserving the column where
 * the target line is long enough. This needs the wrap table, so it is rebuilt
 * first - the table from the last draw is stale the moment anything is
 * inserted. */
/* Callers wrap first; this does not, because an arrow press was walking the
 * gap buffer up to three times per keystroke. */
static void goto_line_col(int line, int want_col)
{
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

static void move_vertical(int delta)
{
    if (s_goal_col < 0) {
        s_goal_col = s_cursor_col;
    }
    goto_line_col(s_cursor_line + delta, s_goal_col);
}

/* Ctrl chords. Chosen from readline rather than invented, because they are
 * already in a lot of people's fingers and cost no new keys on a thumb
 * keyboard - docs/OS.md is explicit that one held modifier plus one letter is
 * the ceiling for a thumb. */
static void handle_ctrl(char c)
{
    switch (c) {
    case 'g': {
        /* Jump to the guide. Everything is reachable by name, but reaching
         * the place where names are typed cannot itself require typing a
         * name - that circle has to be broken by a gesture. */
        for (int i = 0; i < DOC_MAX_BUFFERS; i++) {
            if (strcmp(doc_buf_name(i), "guide") == 0) {
                doc_buf_select(i);
                doc_move_to(0);
                s_goal_col = -1;
                s_top_offset = 0;
                editor_invalidate();
                tg_invalidate();
                snprintf(s_msg, sizeof s_msg, "guide - Enter runs a line");
                s_msg_until = editor_now_ms() + 4000;
                return;
            }
        }
        snprintf(s_msg, sizeof s_msg, "no guide buffer");
        s_msg_until = editor_now_ms() + 3000;
        return;
    }
    case 'o': {                      /* jump to command output */
        const int i = doc_buf_find("+out");
        if (i >= 0 && doc_buf_select(i) == ESP_OK) {
            editor_invalidate();
            tg_invalidate();
            snprintf(s_msg, sizeof s_msg, "+out");
            s_msg_until = editor_now_ms() + 2500;
        }
        return;
    }
    case 'z': doc_undo(); break;
    case 'y': doc_redo(); break;
    case 'a': { int s, e; line_bounds(&s, &e); doc_move_to((size_t)s); break; }
    case 'e': { int s, e; line_bounds(&s, &e); doc_move_to((size_t)e); break; }
    case 'b': doc_left();  break;
    case 'f': doc_right(); break;
    case 'p': move_vertical(-1); break;
    case 'n': move_vertical(+1); break;
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

/* Motion is logged so navigation can be checked from the bench without
 * anyone having to read the panel over someone's shoulder. */
#ifndef EDITOR_TRACE_MOTION
#define EDITOR_TRACE_MOTION 0
#endif

/* Off by default. Each line costs the USB-serial VFS a mutex take and a
 * ring-buffer send PER CHARACTER, which is real latency on the keystroke path
 * for a trace nobody is reading during normal use. */
static void log_motion(const char *what)
{
#if EDITOR_TRACE_MOTION
    /* Re-wrap before reporting. editor_handle wraps once up front, so by the
     * time a motion has run the cached line/col describe where the cursor
     * WAS - which made a working goal column look broken in the trace while
     * the offsets proved it correct. Only compiled in for tracing, so the
     * extra wrap costs nothing in a normal build. */
    wrap(TEXT_COLS);
    ESP_LOGI("editor", "%s -> line %d col %d (offset %u of %u)",
             what, s_cursor_line + 1, s_cursor_col + 1,
             (unsigned)doc_cursor(), (unsigned)doc_len());
#else
    (void)what;
#endif
}

/* Copy the display line the cursor is on into out. */
static void current_line(char *out, size_t max)
{
    int s, e;
    line_bounds(&s, &e);
    size_t n = 0;
    for (int i = s; i < e && n + 1 < max; i++) {
        const char ch = doc_at((size_t)i);
        if (ch == '\n') {
            break;
        }
        out[n++] = ch;
    }
    out[n] = '\0';
}

/* The Run verb from docs/SUBSTRATE.md. In a guide buffer this will be plain
 * Enter; until buffer kinds exist, Ctrl+Enter runs the line under the cursor
 * from anywhere, which is the same gesture without the mode. */
static void run_current_line(void)
{
    char line[128];
    current_line(line, sizeof line);
    if (line[0] == '\0') {
        return;
    }
    char msg[96] = "";
    const cmd_status_t st = cmd_run_line(line, CMD_BY_HANDS, msg, sizeof msg);
    snprintf(s_msg, sizeof s_msg, "%s", msg[0] ? msg : (st == CMD_DONE ? "ok" : "?"));
    s_msg_until = editor_now_ms() + 4000;
    /* A command may have switched buffers entirely. */
    editor_invalidate();
    tg_invalidate();
}

void editor_handle(const kbd_event_t *ev)
{
    /* One wrap per event. Everything below reads the table; nothing below
     * rebuilds it. */
    wrap(TEXT_COLS);

    /* Any motion that is not vertical, and any edit, drops the goal column. */
    switch (ev->type) {
    case KBD_EV_UP:
    case KBD_EV_DOWN:
        break;
    default:
        s_goal_col = -1;
        break;
    }

    if (ev->type == KBD_EV_CHAR && (ev->mods & (KBD_CTRL | KBD_ALT))) {
        handle_ctrl((char)(ev->ch >= 'A' && ev->ch <= 'Z'
                           ? ev->ch - 'A' + 'a' : ev->ch));
        return;
    }

    switch (ev->type) {
    case KBD_EV_CHAR:      doc_insert(ev->ch); break;
    case KBD_EV_ENTER: {
        /* One bit of interpretation, exactly as docs/SUBSTRATE.md says: the
         * kind decides only what Enter does. In a guide, Enter runs the line
         * and Ctrl+Enter inserts one; in prose it is the other way round, so
         * the Run verb is reachable from anywhere without a mode. */
        const bool guide =
            doc_buf_kind(doc_buf_current()) == DOC_KIND_GUIDE;
        const bool ctrl = (ev->mods & KBD_CTRL) != 0;
        if (guide != ctrl) {
            run_current_line();
        } else {
            doc_insert('\n');
        }
        break;
    }
    case KBD_EV_TAB:       doc_insert(' '); doc_insert(' '); break;
    case KBD_EV_BACKSPACE: doc_backspace();   break;
    case KBD_EV_LEFT:      doc_left();  log_motion("left");  break;
    case KBD_EV_RIGHT:     doc_right(); log_motion("right"); break;
    case KBD_EV_UP:        move_vertical(-1); log_motion("up");    break;
    case KBD_EV_DOWN:      move_vertical(+1); log_motion("down");  break;
    case KBD_EV_HOME: { int s, e; line_bounds(&s, &e); doc_move_to((size_t)s); break; }
    case KBD_EV_END:  { int s, e; line_bounds(&s, &e); doc_move_to((size_t)e); break; }
    default: break;
    }
}
