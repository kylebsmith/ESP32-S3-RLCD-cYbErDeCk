/*
 * The ST7305 address arithmetic, as pure functions with no dependencies.
 *
 * It lives in its own header so that the firmware and the host-side test
 * exercise THE SAME CODE. A test that reimplements the arithmetic it is
 * checking proves only that someone can make the same mistake twice.
 *
 * The rule this exists to protect (docs/HARDWARE.md):
 *
 *     CASET = { 0x3C - addr_end , 0x3C - addr_start }    reversed and mirrored
 *
 * At full width that mirroring is an IDENTITY, which is why every full-frame
 * driver is silently correct and never discovers it. It only bites when a
 * window narrows - and a narrow window is exactly what redrawing one
 * character is.
 */
#ifndef ST7305_ADDR_H
#define ST7305_ADDR_H

#include <stddef.h>
#include <stdint.h>

#define ST7305_WIDTH   400
#define ST7305_HEIGHT  300
#define ST7305_NATIVE_W 300
#define ST7305_NATIVE_H 400

#define ST7305_ROW_BYTES  (((ST7305_NATIVE_W + 11) / 12) * 3)   /* 75  */
#define ST7305_ROW_ADDRS  (ST7305_NATIVE_H / 2)                 /* 200 */
#define ST7305_FB_SIZE    (ST7305_ROW_BYTES * ST7305_ROW_ADDRS) /* 15000 */

#define ST7305_ADDR_START       0x12
#define ST7305_ADDR_END         0x2A
#define ST7305_ADDR_MIRROR_BASE (ST7305_ADDR_START + ST7305_ADDR_END) /* 0x3C */

/* Logical (landscape) to native (the controller's own frame). */
static inline void st7305_to_native(int orient, int x, int y, int *nx, int *ny)
{
    switch (orient & 3) {
    case 0: *nx = y;                     *ny = x;                    break;
    case 1: *nx = y;                     *ny = ST7305_WIDTH - 1 - x; break;
    case 2: *nx = ST7305_HEIGHT - 1 - y; *ny = ST7305_WIDTH - 1 - x; break;
    default:*nx = ST7305_HEIGHT - 1 - y; *ny = x;                    break;
    }
}

static inline size_t st7305_fb_index_n(int nx, int ny)
{
    return (size_t)(ny >> 1) * ST7305_ROW_BYTES + (size_t)(nx >> 2);
}

/* One RAM byte is 4 native-x pixels by 2 native-y pixels; the top
 * (smaller native y) pixel is the higher bit. */
static inline uint8_t st7305_fb_mask_n(int nx, int ny)
{
    return (uint8_t)(1u << (7 - ((nx & 3) << 1) - (ny & 1)));
}

/* Resolve a native-frame rectangle to what actually goes on the wire. */
typedef struct {
    uint8_t caset[2];
    uint8_t raset[2];
    int     send_start;    /* byte offset into a framebuffer row */
    int     send_count;    /* bytes per row address               */
    int     rows;          /* number of row addresses             */
    int     bytes;         /* send_count * rows                   */
} st7305_window_t;

static inline void st7305_window(int nx0, int nx1, int ny0, int ny1,
                                 st7305_window_t *w)
{
    const int addr_start = ST7305_ADDR_START + nx0 / 12;
    const int addr_end   = ST7305_ADDR_START + nx1 / 12;
    const int row_first  = ny0 / 2;
    const int row_last   = ny1 / 2;

    w->caset[0] = (uint8_t)(ST7305_ADDR_MIRROR_BASE - addr_end);
    w->caset[1] = (uint8_t)(ST7305_ADDR_MIRROR_BASE - addr_start);
    w->raset[0] = (uint8_t)row_first;
    w->raset[1] = (uint8_t)row_last;

    w->send_start = (addr_start - ST7305_ADDR_START) * 3;
    w->send_count = (addr_end - addr_start + 1) * 3;
    w->rows       = row_last - row_first + 1;
    w->bytes      = w->send_count * w->rows;
}

#endif /* ST7305_ADDR_H */
