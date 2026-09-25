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

#include "driver/usb_serial_jtag.h"
#include "seq.h"
#include "view_wire.h"
#include "viz.h"

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
static void view_emit(const uint8_t *frame, size_t n)
{
    /* Static, both: a whole frame in base64 is two kilobytes, which is not
     * something to put on the main task's stack. */
    static char line[(VIZ_W * VIZ_H + VIEW_HEAD_LEN + 1 + 2) / 3 * 4 + 16];
    static char b64[sizeof line];
    if (view_wire_base64(b64, sizeof b64, frame, n) == 0) {
        return;
    }
    const int k = snprintf(line, sizeof line, "\x1b]view;%s\x07\n", b64);
    if (k <= 0 || k >= (int)sizeof line) {
        return;
    }
    if (usb_serial_jtag_is_driver_installed()) {
        if (usb_serial_jtag_write_bytes(line, (size_t)k, 0) != k) {
            view_dropped++;
            return;
        }
    } else {
        fwrite(line, 1, (size_t)k, stdout);
        fflush(stdout);
    }
    view_frames++;
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
    const size_t n = view_wire_pack(frame, sizeof frame, tick, w, h, cells);
    if (n > 0) {
        view_emit(frame, n);
    }
}
