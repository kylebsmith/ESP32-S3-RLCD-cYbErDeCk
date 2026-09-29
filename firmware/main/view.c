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
#include "seq.h"
#include "seq_pattern.h"
#include "view_wire.h"
#include "viz.h"

_Static_assert((int)VIZ_OUT_MODES == (int)VIEW_MODES &&
               (int)VIZ_OUT_POSTER == (int)VIEW_MODE_POSTER,
               "the engine and the wire number the view's modes the same way");

static void view_sink(const char *lane, uint8_t status, uint8_t d1, uint8_t d2,
                      uint32_t when_us)
{
    /* The view node takes pictures, not notes. It is a destination so that
     * '>send' can list it and turn it on; the notes are someone else's. */
    (void)lane; (void)status; (void)d1; (void)d2; (void)when_us;
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
    } else {
        fwrite(line, 1, (size_t)k, stdout);
        fflush(stdout);
    }
    return true;
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

void view_frame(void)
{
    if (!seq_dest_is_on("view")) {
        return;
    }
    static uint8_t cells[VIZ_W * VIZ_H];
    static uint8_t frame[VIZ_W * VIZ_H + VIEW_HEAD_LEN + 1];
    int w = 0, h = 0;
    uint32_t tick = 0;
    if (viz_frame(cells, (int)sizeof cells, &w, &h, &tick) <= 0) {
        return;
    }
    /* THE MODE GOES AHEAD OF EVERY PICTURE, and the poster's lines with it, so
     * a node that joins late or loses one is right again a step later. */
    static uint8_t ctl[VIEW_CTL_HEAD_LEN + VIEW_TEXT_MAX + 1];
    static view_line_t lines[VIEW_LINES_MAX];
    static char text[VIEW_LINES_MAX][VIEW_LINE_MAX + 1];
    const int mode = viz_out_mode_now();
    const int nl = (mode == VIZ_OUT_POSTER) ? poster_lines(lines, text) : 0;
    const size_t cn = view_wire_pack_ctl(ctl, sizeof ctl, tick, mode, lines, nl);
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
