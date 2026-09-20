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
#include "cell_attr.h"
#include "ui_text.h"
#include "seq.h"
#include "seq_pattern.h"

#include <stdio.h>
#include <string.h>

#include "docstore.h"
#include "cmd.h"
#include "kbd.h"
#include "st7305.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "textgrid.h"

int64_t editor_now_ms(void);
static void line_at(size_t from, char *out, size_t max);

#define MARGIN_X     20
#define MARGIN_TOP   12
#define CELL_W       12
#define CELL_H       24

/* The grid is chosen at runtime, because both faces are already compiled in
 * and the right density is not the same for writing prose and for reading a
 * list of ten commands. Chunky is the default because the panel is
 * reflective with no backlight; dense is there when the screen is the
 * constraint rather than the eyes. */
static int TEXT_COLS  = 30;
static int TEXT_ROWS  = 10;
static int STATUS_ROW = 10;

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
/* Whether the playhead is passing through the cursor cell. The blink has to
 * know, or it erases the bar every time it repaints that one cell. */
static bool s_cur_under = false;

static char s_status_shown[64];
/* The result of the last command, shown in place of the status line for a
 * few seconds. A command that reports nothing is indistinguishable from one
 * that did not run. */
static char    s_msg[64];
/* Where we came from, so a jump to +out is never a one-way door. An unnamed
 * scratch buffer has no name for >open to match, so without this a command
 * run from fresh writing strands that writing with no key and no command to
 * get back to it. */
static int     s_prev_buf = -1;
static int64_t s_msg_until;
static bool s_chrome_dirty = true;
static uint32_t s_pushes, s_push_bytes, s_render_us, s_cells;
static uint32_t s_cells_total;

void editor_vitals(uint32_t *pushes, uint32_t *bytes,
                   uint32_t *render_us, uint32_t *cells)
{
    *pushes = s_pushes;
    *bytes  = s_push_bytes;
    *render_us = s_render_us;
    *cells = s_cells;
    s_pushes = 0;
    s_push_bytes = 0;
    s_render_us = 0;
    s_cells = 0;
}

uint32_t editor_cells_drawn(void) { return s_cells_total; }

void editor_message(const char *m)
{
    snprintf(s_msg, sizeof s_msg, "%s", m);
    s_msg_until = editor_now_ms() + 6000;
}

void editor_invalidate(void)
{
    s_chrome_dirty = true;
    s_status_shown[0] = '\0';
}

esp_err_t editor_set_density(int dense)
{
    const tg_font_t *face = dense ? &tg_font_6x12 : &tg_font_12x24;
    const int cw = dense ? 6 : 12;
    const int ch = dense ? 12 : 24;

    const int cols = (ST7305_WIDTH - 2 * MARGIN_X) / cw;
    /* One row of the grid is the status line; the rest is text. */
    const int rows = (ST7305_HEIGHT - MARGIN_TOP) / ch;

    const esp_err_t err = tg_set_layout(face, 1, MARGIN_X, MARGIN_TOP,
                                        cols, rows);
    if (err != ESP_OK) {
        return err;
    }
    TEXT_COLS  = cols;
    TEXT_ROWS  = rows - 1;
    STATUS_ROW = rows - 1;
    s_top_offset = 0;
    s_goal_col = -1;
    editor_invalidate();
    tg_invalidate();
    return ESP_OK;
}

esp_err_t editor_init(void)
{
    return editor_set_density(0);
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
static void line_at(size_t from, char *out, size_t max);

/* Messages must fit, and "fit" is checked at BUILD time now, not hoped for.
 *
 * tg_draw_text_px used to clip silently at the screen edge, so "not a command
 * - start the line with >" rendered as "not a command - start the line" and
 * lost the entire point of the sentence. That was "fixed" by moving the clip
 * into the snprintf below - which truncates just as silently, at exactly the
 * same column, and the owner hit it. Fixed messages now live in ui_text.h
 * behind _Static_assert. See the note at the top of that file. */
#define STATUS_MAX TEXT_COLS

static void status_bar(void)
{
    char wide[96];
    char s[STATUS_MAX + 1];

    if (s_msg[0] != '\0' && editor_now_ms() < s_msg_until) {
        snprintf(wide, sizeof wide, "%s", s_msg);
    } else {
        if (s_msg[0] != '\0') {
            s_msg[0] = '\0';
        }
        const char *nm = doc_buf_name(doc_buf_current());
        const uint8_t m = kbd_mods();
        char mod[5];
        int mi = 0;
        if (m & KBD_CTRL)     { mod[mi++] = '^'; }
        if (m & KBD_MOD_LALT) { mod[mi++] = 'A'; }
        if (m & KBD_MOD_RALT) { mod[mi++] = 'G'; }
        if (m & KBD_SHIFT)    { mod[mi++] = 'S'; }
        mod[mi] = '\0';
        snprintf(wide, sizeof wide, "%-11.11s%c %3d:%-3d %s%s%s",
                 nm[0] ? nm : "scratch",
                 doc_dirty() ? '*' : ' ',
                 s_cursor_line + 1, s_cursor_col + 1,
                 mod,
                 kbd_connected() ? "K" : "-",
                 doc_sd_present() ? "S" : "-");
    }
    snprintf(s, sizeof s, "%-*.*s", TEXT_COLS, TEXT_COLS, wide);

    /* THE BAR ITSELF SAYS WHICH KIND OF BUFFER YOU ARE IN.
     *
     * A command with more than one line of output moves you into '+out'. That
     * is deliberate - output is a buffer, not a scrollback - but the move was
     * unannounced: the block above SKIPS the name field entirely while a
     * message is live, and every command sets a message for several seconds.
     * So during the exact window in which you were just teleported, nothing
     * on the screen said where you had landed.
     *
     * The owner hit this. They ran a command, were moved into '+out', tried to
     * run the lines they could see - which were command OUTPUT, formatted with
     * a line-number prefix - and got "not a command" with no explanation.
     *
     * Inverting the whole bar costs no columns, cannot be missed, and does not
     * expire with the message. Dark bar: your document. Light bar: output. */
    const int att = doc_current_is_transient() ? TG_NORMAL : TG_INVERSE;
    for (int c = 0; c < TEXT_COLS; c++) {
        tg_put(c, STATUS_ROW, s[c] ? s[c] : ' ', att);
    }
}

/* Document offset of the step that is sounding on a lane line, or -1.
 *
 * WHY AN OFFSET AND NOT A COLUMN. A 32-step lane written out is 38 characters
 * against a 30-column grid, so it wraps - and a mark derived from a display
 * row's column would land on the wrong row the moment it did. That is exactly
 * the bug that was fixed once already in current_line(), which read the wrap
 * table instead of the document. Compute in document space, project through
 * the wrap table, never the other way round.
 *
 * THE PLAYHEAD MARKS THE STEP, NOT THE HIT. Three reasons. It reads as one
 * transport sweeping the document rather than four lamps blinking
 * independently; it shows where you are in the bar even on a lane that is
 * resting; and it stays truthful under editing, because changing 'x...' to
 * '..x.' changes which steps sound but not the step-to-column mapping. */
static int playhead_offset(int line_off, const char *lbuf, int at, int len)
{
    if (!seq_running()) {
        return -1;
    }
    const seq_lane_t *l = seq_lane_find(lbuf + at, len);
    /* Exactly fire_step's skip test. If it would not sound, it must not be
     * marked - that makes "the playhead is sweeping this line" and "this lane
     * is sounding" the same statement, which is what the toggle relies on. */
    if (l == NULL || l->muted || l->steps == 0) {
        return -1;
    }
    /* The argument, delimited the way the dispatcher delimits it. */
    int a = at + len;
    while (lbuf[a] == ' ' || lbuf[a] == '\t') {
        a++;
    }
    /* A lane compiled from text that has since been edited would put the mark
     * on a character that is not the one sounding. The step COUNT is the whole
     * of what the mapping depends on, so it is the whole of the test. */
    if (seq_pattern_steps(lbuf + a, SEQ_MAX_STEPS) != l->steps) {
        return -1;
    }
    /* The global step is not the lane's step the moment one lane is not the
     * same length as another - which is the point of having lanes. */
    const int off = seq_pattern_offset(lbuf + a, (int)(seq_position() % (uint32_t)l->steps),
                                       SEQ_MAX_STEPS);
    return (off < 0) ? -1 : line_off + a + off;
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
    s_cur_under = false;

    /* Declared outside the row loop ON PURPOSE: it is computed once per
     * LOGICAL line and must survive across that line's continuation rows, so
     * a wrapped lane keeps its playhead. Every logical-line start resets it. */
    int ph_off = -1;

    for (int r = 0; r < TEXT_ROWS; r++) {
        const int li = s_top_line + r;
        const int start = (li < s_line_count) ? s_line_start[li] : 0;
        const int end   = (li < s_line_count)
                          ? ((li + 1 < s_line_count) ? s_line_start[li + 1]
                                                     : (int)len)
                          : 0;

        /* Mark a recognised command. Only the display row that BEGINS a
         * logical line can carry the sigil, and only the command word itself
         * is marked - so the eye learns "inverse word = the machine knows
         * this" without a second colour, which a one-bit panel does not
         * have. An unrecognised command is left looking like prose, which is
         * how it will behave. */
        int mark_at = -1, mark_len = 0;
        if (li < s_line_count &&
            (start == 0 || doc_at((size_t)start - 1) == '\n')) {
            char lbuf[128];
            line_at((size_t)start, lbuf, sizeof lbuf);
            if (cmd_recognise(lbuf, &mark_at, &mark_len) == NULL) {
                mark_at = -1;
                ph_off  = -1;
            } else {
                ph_off = playhead_offset(start, lbuf, mark_at, mark_len);
            }
        } else if (r == 0 && li < s_line_count) {
            /* The top row can be the CONTINUATION of a line that begins above
             * the viewport. Walk back to its start once, so a wrapped lane
             * does not lose its playhead the moment it scrolls. */
            size_t s = (size_t)start;
            while (s > 0 && doc_at(s - 1) != '\n') {
                s--;
            }
            char lbuf[128];
            int a, n;
            line_at(s, lbuf, sizeof lbuf);
            ph_off = (cmd_recognise(lbuf, &a, &n) != NULL)
                     ? playhead_offset((int)s, lbuf, a, n) : -1;
        }

        for (int c = 0; c < TEXT_COLS; c++) {
            char ch = ' ';
            if (li < s_line_count) {
                const int idx = start + c;
                if (idx < end) {
                    const char d = doc_at((size_t)idx);
                    ch = (d == '\n') ? ' ' : d;
                }
            }
            /* The playhead gets its OWN attribute rather than sharing the
             * cursor's solid block. Two solid blocks on a one-ink panel are
             * two things that look identical, and the owner lost their cursor
             * inside a running lane because of it. They are independent bits,
             * so a cursor sitting on the playhead shows as both.
             *
             * Computed BEFORE the cursor capture, because the blink needs to
             * know whether the playhead is passing through the cursor cell. */
            const bool playing = ph_off >= 0 && li < s_line_count &&
                                 (start + c) == ph_off && (start + c) < end;
            const bool is_cursor = (li == s_cursor_line && c == s_cursor_col);
            if (is_cursor) {
                s_cur_col = c; s_cur_row = r; s_cur_ch = ch;
                s_cur_under = playing;
            }
            const bool marked = mark_at >= 0 &&
                                c >= mark_at && c < mark_at + mark_len;
            const bool inv = (is_cursor && s_cursor_on) != marked;
            tg_put(c, r, ch, cell_attr(inv, playing));
        }
    }

    /* Cells first, then sub-cell chrome on top of them. Note that this
     * RENDERS but does not PUSH: the caller pushes once, with editor_present,
     * after the chrome is in the framebuffer too. */
    status_bar();
    /* The CPU side has never been measured - only the bytes on the wire.
     * Drawing a 12x24 cell is 288 pixel writes, each of which is a coordinate
     * transform; that is the cost that competes with live coding, not the
     * SPI. */
    const int64_t t0 = esp_timer_get_time();
    { const uint32_t n = (uint32_t)tg_render(); s_cells += n; s_cells_total += n; }
    s_render_us += (uint32_t)(esp_timer_get_time() - t0);
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
    /* cell_attr, not a second copy of the expression. The copy that used to
     * live here dropped the playhead bit, so a blink erased the bar from the
     * one cell where the cursor and the playhead meet. */
    tg_put(s_cur_col, s_cur_row, s_cur_ch, cell_attr(on, s_cur_under));
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
                snprintf(s_msg, sizeof s_msg, "%s", UI_GUIDE_HINT);
                s_msg_until = editor_now_ms() + 4000;
                return;
            }
        }
        snprintf(s_msg, sizeof s_msg, "%s", UI_NO_GUIDE);
        s_msg_until = editor_now_ms() + 3000;
        return;
    }
    case 'o': {
        /* A toggle, not a jump. From anywhere it shows the output; from the
         * output it returns you to what you were writing. */
        const int here = doc_buf_current();
        int target;
        if (strcmp(doc_buf_name(here), "+out") == 0 && s_prev_buf >= 0) {
            target = s_prev_buf;
        } else {
            target = doc_buf_find("+out");
        }
        if (target >= 0 && target != here && doc_buf_select(target) == ESP_OK) {
            s_prev_buf = here;
            editor_invalidate();
            tg_invalidate();
            const char *nm = doc_buf_name(target);
            snprintf(s_msg, sizeof s_msg, "%s", nm[0] ? nm : "scratch");
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

/* Copy the logical line that begins at `from`. */
static void line_at(size_t from, char *out, size_t max)
{
    const size_t len = doc_len();
    size_t n = 0;
    for (size_t i = from; i < len && n + 1 < max; i++) {
        const char ch = doc_at(i);
        if (ch == '\n') {
            break;
        }
        out[n++] = ch;
    }
    out[n] = '\0';
}

/* Copy the LOGICAL line the cursor is on - the run between newlines in the
 * document, not the run between soft wraps on the screen.
 *
 * This used the wrap table, and a command longer than thirty columns was
 * therefore cut at the wrap and its PREFIX executed, silently:
 * ">name Rust Belt Lullaby and the" filed the document as "Rust Belt Lullaby
 * and". No error, no mark, wrong name in the archive. It is the same class of
 * defect as the trailing-whitespace trap docs/SUBSTRATE.md is proud of
 * engineering out of existence - destructive, silent, discovered later - and
 * every MIDI command in the brief is long enough to hit it.
 *
 * A logical line also means a wrapped command runs identically from any of
 * its display rows, which removes the confusing "not a command" error when
 * standing on a continuation row. */
static void current_line(char *out, size_t max)
{
    const size_t len = doc_len();
    size_t s = doc_cursor();
    if (s > len) {
        s = len;
    }
    while (s > 0 && doc_at(s - 1) != '\n') {
        s--;
    }
    size_t n = 0;
    for (size_t i = s; i < len && n + 1 < max; i++) {
        const char ch = doc_at(i);
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

    const char *p = line;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p != '>') {
        snprintf(s_msg, sizeof s_msg, "%s", UI_NOT_A_COMMAND);
        s_msg_until = editor_now_ms() + 3000;
        return;
    }

    /* Where the output buffer ended BEFORE the command, so the view can land
     * on the first line this command wrote rather than the last line of
     * everything ever written. `help` was scrolling to the bottom and hiding
     * its own first three entries with nothing to say so. */
    const int outb = doc_buf_find("+out");
    const size_t out_was = outb >= 0 ? doc_buf_len(outb) : 0;

    char msg[96] = "";
    cmd_run_line(line, CMD_BY_HANDS, msg, sizeof msg);
    const int lines = cmd_last_output_lines();

    /* A result of more than one line shows itself. Reporting "10 commands" at
     * the bottom of the screen and leaving the actual answer somewhere the
     * owner has to know to look for is not minimalism, it is hiding. */
    if (lines > 1) {
        const int out = doc_buf_find("+out");
        const int here = doc_buf_current();
        if (out >= 0 && out != here && doc_buf_select(out) == ESP_OK) {
            s_prev_buf = here;
            doc_move_to(out_was);
            s_top_offset = (int)out_was;
            s_goal_col = -1;
            snprintf(s_msg, sizeof s_msg, "%s", msg[0] ? msg : "output");
            s_msg_until = editor_now_ms() + 2500;
            editor_invalidate();
            tg_invalidate();
            return;
        }
    }
    snprintf(s_msg, sizeof s_msg, "%s", msg[0] ? msg : "ok");
    s_msg_until = editor_now_ms() + 4000;
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

    if (ev->type == KBD_EV_ENTER && (ev->mods & KBD_COMMAND_MODS)) {
        run_current_line();
        return;
    }

    if (ev->type == KBD_EV_CHAR && (ev->mods & KBD_COMMAND_MODS)) {
        handle_ctrl((char)(ev->ch >= 'A' && ev->ch <= 'Z'
                           ? ev->ch - 'A' + 'a' : ev->ch));
        return;
    }

    switch (ev->type) {
    case KBD_EV_CHAR:      doc_insert(ev->ch); break;
    case KBD_EV_ENTER:
        /* Enter ALWAYS inserts a newline. Always, in every buffer.
         *
         * It used to run the line in a guide buffer, which is what
         * docs/SUBSTRATE.md describes - and it made the guide uneditable:
         * standing at the end of a command with no way to add a line after
         * it, because the key that adds lines was busy running things. An
         * editor whose Enter key sometimes does not insert a line is not an
         * editor.
         *
         * The sigil made the kind redundant here anyway. A command is marked
         * by '>' in the text, so the machine can tell a command from prose
         * without a mode; the only remaining question is WHEN to run one, and
         * that is a modifier, not a property of the buffer. */
        doc_insert('\n');
        break;
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
