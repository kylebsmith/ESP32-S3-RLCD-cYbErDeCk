/*
 * The picture, out to the view node (docs/NEXT.md §6, docs/VIEW.md).
 *
 * A DESTINATION, NOT A VERB. '>send view on' streams frames and '>send view off'
 * stops - listed beside mon, din, osc and usb, because the view node is one more
 * place the lanes go. It sends a frame each time the picture changes, from the
 * main loop, never the clock.
 *
 * TODAY'S TRANSPORT IS THE CONSOLE, and says so. The deck will drive the view
 * node over its own USB; until that link exists a computer relays it
 * (tools/viewrelay.py). The frame goes out as a terminal escape - ESC ] view;
 * <base64> BEL - which a terminal swallows and the relay lifts out, so the
 * console stays readable while it carries pictures. When the direct link
 * arrives, only view_emit() changes: the bytes are already the wire format.
 */
#include "view.h"

#include <stdio.h>
#include <string.h>

#include "docstore.h"
#include "driver/usb_serial_jtag.h"
#include "lane_name.h"
#include "seq.h"
#include "seq_pattern.h"
#include "usbdev.h"
#include "view_wire.h"
#include "viz.h"

_Static_assert((int)VIZ_OUT_MODES == (int)VIEW_MODES &&
               (int)VIZ_OUT_POSTER == (int)VIEW_MODE_POSTER &&
               (int)VIZ_OUT_CODE == (int)VIEW_MODE_CODE,
               "the engine and the wire number the view's modes the same way");

/* THE VIEW'S PARAMETERS live in the picture engine (viz_out_param): the last
 * value of controllers 1-8 on MIDI channel 16, or what '>send view day 9' set,
 * sent to the node with every control frame (view_wire.h). */
static void view_sink(const char *lane, uint8_t status, uint8_t d1, uint8_t d2,
                      uint32_t when_us)
{
    /* The view node takes pictures, not notes; the notes are someone else's.
     * But a controller on channel 16 is the performer playing the screen:
     * '>ink = cc 1 ch 16', '>ink 0123456789 /16'. */
    (void)lane; (void)when_us;
    if ((status & 0xF0) == 0xB0 && (status & 0x0F) == VIEW_PARAM_CHANNEL - 1 &&
        d1 >= 1 && d1 <= VIEW_PARAMS) {
        viz_out_param_set(d1 - 1, d2 & 0x7F);
    }
}

void view_init(void)
{
    seq_dest_add("view", view_sink, NULL, "the picture to an HDMI node");
}

uint32_t view_frames, view_dropped;

/* ONE WRITE, AND NEVER A WAIT.
 *
 * Through stdio the console driver takes a frame a character at a time - a
 * mutex and a ring-buffer send for each of 1,436 bytes - and blocks whenever
 * its ring is full. That was 31 ms of every frame on the editor's loop,
 * measured: a quarter of its time with the view on, and the loop fell from 199
 * turns a second to 142. So the frame goes to the driver whole, and if the
 * ring has no room for all of it, it is not sent - the picture drops a frame
 * rather than the editor dropping keystrokes, and a frame is never torn.
 *
 * In USB MIDI mode the console is on the CDC interface and this driver is not
 * installed; there the frame goes through stdio, as it always did. */
static bool view_emit(const uint8_t *frame, size_t n)
{
    /* Static, both: a whole frame in base64 is three kilobytes, which is not
     * something to put on the main task's stack. */
    static char line[(VIZ_W * VIZ_H + VIEW_HEAD_LEN + 1 + 2) / 3 * 4 + 16];
    static char b64[sizeof line];
    if (view_wire_base64(b64, sizeof b64, frame, n) == 0) {
        return false;
    }
    const int k = snprintf(line, sizeof line, "\x1b]view;%s\x07\n", b64);
    if (k <= 0 || k >= (int)sizeof line) {
        return false;
    }
    if (usb_serial_jtag_is_driver_installed()) {
        if (usb_serial_jtag_write_bytes(line, (size_t)k, 0) != k) {
            return false;
        }
        return true;
    }
    /* USB MIDI mode: the console is the CDC. Whole, or dropped - never a wait. */
    return usbdev_console_write_whole(line, (size_t)k);
}

/* THE POSTER'S LINES: the piece's name, the section the cursor is in, the tempo
 * and scale, and the lanes in play with the span of the step each is on - the
 * same span the editor lights (seq_pattern_mark), so the poster and the panel
 * never disagree about where the music is.
 *
 * SEVEN LANES OF 32 CHARACTERS, because the node shows seven of 25 and because
 * the picture and this must go out together through a 4,000-byte ring: at most
 * about 600 bytes here, against the picture's 3,225. */
#define POSTER_LANES 7
#define POSTER_COLS  32

static int poster_lines(view_line_t *out, char text[][VIEW_LINE_MAX + 1])
{
    static char doc[8192];
    const size_t len = doc_read(doc, sizeof doc - 1);
    doc[len] = '\0';
    const size_t cur = doc_cursor() < len ? doc_cursor() : len;
    int n = 0;
    /* the name: the first line with something on it */
    size_t a = 0;
    while (a < len && (doc[a] == '\n' || doc[a] == ' ')) { a++; }
    size_t b = a;
    while (b < len && doc[b] != '\n') { b++; }
    snprintf(text[n], POSTER_COLS + 1, "%.*s", (int)(b - a), doc + a);
    n++;
    /* the section: the last '--' line at or above the cursor */
    text[n][0] = '\0';
    for (size_t i = 0; i <= cur && i < len; ) {
        size_t e = i;
        while (e < len && doc[e] != '\n') { e++; }
        if (e - i >= 2 && doc[i] == '-' && doc[i + 1] == '-') {
            size_t s = i + 2;
            while (s < e && doc[s] == ' ') { s++; }
            snprintf(text[n], POSTER_COLS + 1, "%.*s", (int)(e - s), doc + s);
        }
        i = e + 1;
    }
    n++;
    snprintf(text[n], VIEW_LINE_MAX + 1, "%d bpm  %s", seq_get_bpm(), seq_scale_name());
    n++;
    for (int i = 0; i < n; i++) {
        out[i].text = text[i];
        out[i].from = out[i].to = 0;
    }
    int count = 0;
    const seq_lane_t *l = seq_lanes(&count);
    static seq_comp_t comp;                     /* the main task only */
    for (int i = 0; i < SEQ_MAX_LANES && n < 3 + POSTER_LANES; i++) {
        if (!l[i].used || l[i].muted || l[i].slots == 0) {
            continue;
        }
        const int k = snprintf(text[n], POSTER_COLS + 1, "%s %s", l[i].name, l[i].text);
        out[n].text = text[n];
        out[n].from = out[n].to = 0;
        int slot = 0, f = 0, t = 0;
        uint32_t cycle = 0;
        if (k > 0 && seq_running() && seq_lane_now(&l[i], &slot, &cycle) &&
            seq_pattern_compile(l[i].text, &comp) == SEQ_PAT_OK &&
            seq_pattern_mark(&comp, slot, cycle, &f, &t)) {
            const int off = (int)strlen(l[i].name) + 1;
            if (off + t <= POSTER_COLS && f < t) {
                out[n].from = (uint8_t)(off + f);
                out[n].to = (uint8_t)(off + t);
            }
        }
        n++;
    }
    return n;
}

/* THE SPAN OF THE STEP A LINE'S LANE IS ON, in the line: the same test the
 * editor's playhead makes (editor.c, playhead_span) - the lane is playing, not
 * muted, and playing exactly what this line says - so the screen lights what the
 * panel lights and nothing else. Lines longer than CODE_COLS light only within
 * what is sent. */
#define CODE_COLS 40

static void line_span(const char *line, size_t n, uint8_t *from, uint8_t *to)
{
    *from = *to = 0;
    if (!seq_running() || n < 2 || line[0] != '>') {
        return;
    }
    size_t a = 1;
    while (a < n && line[a] != ' ' && line[a] != '\t') { a++; }
    lane_name_t ln;
    if (lane_name_parse(line + 1, a - 1, &ln) != LN_OK) {
        return;
    }
    const seq_lane_t *l = seq_lane_find(ln.canon, -1);
    if (l == NULL || l->muted || l->slots == 0) {
        return;
    }
    while (a < n && (line[a] == ' ' || line[a] == '\t')) { a++; }
    static char pat[SEQ_TEXT_MAX];
    const size_t m = (n - a < sizeof pat - 1) ? n - a : sizeof pat - 1;
    memcpy(pat, line + a, m);
    pat[m] = '\0';
    if (seq_pattern_hash(pat) != l->src) {
        return;
    }
    int slot = 0, f = 0, t = 0;
    uint32_t cycle = 0;
    static seq_comp_t comp;                     /* the main task only */
    if (!seq_lane_now(l, &slot, &cycle) || seq_pattern_compile(pat, &comp) != SEQ_PAT_OK ||
        !seq_pattern_mark(&comp, slot, cycle, &f, &t)) {
        return;
    }
    if (f < t && a + (size_t)t <= CODE_COLS) {
        *from = (uint8_t)(a + (size_t)f);
        *to = (uint8_t)(a + (size_t)t);
    }
}

/* THE CODE'S LINES, for '>send view code': the document's name, the tempo and
 * scale, and ten lines round the cursor - three above it, the rest below - each
 * with the span its lane is on. Forty characters a line, so the picture and this
 * still go out together through the console's ring. */
static int code_lines(view_line_t *out, char text[][VIEW_LINE_MAX + 1])
{
    /* THE DOCUMENT IS READ WHERE IT LIES, a character at a time round the
     * cursor: ten lines are wanted. The first version copied the whole
     * document out every step, into 8 KB of internal RAM held for as long as
     * the deck ran - the free heap fell from 48 to 39 KB (2026-09-29) - and a
     * document past 8 KB showed the wrong lines. The walks are bounded, so a
     * document with no line breaks costs a bounded look, not all of it. */
    enum { LOOK = 4096 };
    const size_t len = doc_len();
    const size_t cur = doc_cursor() < len ? doc_cursor() : len;
    int n = 0;
    const char *nm = doc_buf_name(doc_buf_current());
    snprintf(text[n], CODE_COLS + 1, "%s", (nm && nm[0]) ? nm : "scratch");
    n++;
    snprintf(text[n], CODE_COLS + 1, "%d bpm  %s", seq_get_bpm(), seq_scale_name());
    n++;
    for (int i = 0; i < n; i++) {
        out[i].text = text[i];
        out[i].from = out[i].to = 0;
    }
    const size_t floor = cur > LOOK ? cur - LOOK : 0;
    size_t ls = cur;
    while (ls > floor && doc_at(ls - 1) != '\n') { ls--; }
    for (int back = 0; back < 3 && ls > floor; back++) {
        ls--;
        while (ls > floor && doc_at(ls - 1) != '\n') { ls--; }
    }
    char line[128];                    /* a line the editor runs is 127 at most */
    while (n < 2 + 10 && n < VIEW_LINES_MAX && ls <= len) {
        size_t le = ls, k = 0;
        while (le < len && le - ls < LOOK) {
            const char c = doc_at(le);
            if (c == '\n') {
                break;
            }
            if (k + 1 < sizeof line) {
                line[k++] = c;
            }
            le++;
        }
        line[k] = '\0';
        snprintf(text[n], CODE_COLS + 1, "%s", line);
        out[n].text = text[n];
        line_span(line, k, &out[n].from, &out[n].to);
        n++;
        if (le >= len) {
            break;
        }
        ls = le + 1;
    }
    return n;
}

void view_frame(void)
{
    /* OFF CLEARS THE COLOURS: the parameters go back to unset, so the next
     * '>send view' starts from the node's own defaults whatever a set before it
     * played - each act of a set begins clean. */
    static bool was_on;
    if (!seq_dest_is_on("view")) {
        if (was_on) {
            viz_out_params_clear();
        }
        was_on = false;
        return;
    }
    was_on = true;
    static uint8_t cells[VIZ_W * VIZ_H];
    static uint8_t frame[VIZ_W * VIZ_H + VIEW_HEAD_LEN + 1];
    int w = 0, h = 0;
    uint32_t tick = 0;
    if (viz_frame(cells, (int)sizeof cells, &w, &h, &tick) <= 0) {
        return;
    }
    /* THE MODE GOES AHEAD OF EVERY PICTURE, and the poster's lines with it, so
     * a node that joins late or loses one is right again a step later. */
    static uint8_t ctl[VIEW_CTL2_HEAD_LEN + VIEW_TEXT_MAX + 1];
    static view_line_t lines[VIEW_LINES_MAX];
    static char text[VIEW_LINES_MAX][VIEW_LINE_MAX + 1];
    const int mode = viz_out_mode_now();
    const int nl = (mode == VIZ_OUT_POSTER) ? poster_lines(lines, text)
                 : (mode == VIZ_OUT_CODE)   ? code_lines(lines, text) : 0;
    uint8_t par[VIEW_PARAMS];
    viz_out_params_land(tick / 24);           /* a colour waiting for its one */
    for (int i = 0; i < VIEW_PARAMS; i++) {
        par[i] = viz_out_param(i);
    }
    const size_t cn = view_wire_pack_ctl2(ctl, sizeof ctl, tick, mode, par, lines, nl);
    if (cn > 0) {
        view_emit(ctl, cn);
    }
    const size_t n = view_wire_pack(frame, sizeof frame, tick, w, h, cells);
    if (n > 0) {
        if (view_emit(frame, n)) {
            view_frames++;
        } else {
            view_dropped++;
        }
    }
}
