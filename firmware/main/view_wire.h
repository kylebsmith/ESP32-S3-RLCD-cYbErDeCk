/*
 * The view node's wire format - docs/VIEW.md - as one pure function, so the
 * deck, the host check and the relay all build the same bytes.
 *
 *   'D' 'K' 'V' '1'   magic
 *   tick  u32, little-endian: the deck's pulse this frame belongs to
 *   w, h  u8 each: the frame in cells
 *   cells w*h bytes, row by row: 32-126 text, 128-155 the deck's tiles
 *   sum   u8: XOR of every byte after the magic
 *
 * The reader is view/deckview/deckview.ino. The tick rides in every frame so
 * that a node which joins the ensemble can show a frame when it was meant to be
 * shown, instead of when it happened to arrive.
 */
#ifndef VIEW_WIRE_H
#define VIEW_WIRE_H

#include <stddef.h>
#include <stdint.h>

#define VIEW_MAGIC_LEN 4
#define VIEW_HEAD_LEN  (VIEW_MAGIC_LEN + 6)

/* Frame `cells` (w*h of them) into `out`. Returns the length, or 0 if `max`
 * is too small or the frame is empty. */
static inline size_t view_wire_pack(uint8_t *out, size_t max, uint32_t tick,
                                    int w, int h, const uint8_t *cells)
{
    if (w < 1 || h < 1 || w > 255 || h > 255) {
        return 0;
    }
    const size_t n = (size_t)w * (size_t)h;
    const size_t len = VIEW_HEAD_LEN + n + 1;
    if (len > max) {
        return 0;
    }
    out[0] = 'D'; out[1] = 'K'; out[2] = 'V'; out[3] = '1';
    out[4] = (uint8_t)tick;
    out[5] = (uint8_t)(tick >> 8);
    out[6] = (uint8_t)(tick >> 16);
    out[7] = (uint8_t)(tick >> 24);
    out[8] = (uint8_t)w;
    out[9] = (uint8_t)h;
    uint8_t sum = 0;
    for (int i = 4; i < VIEW_HEAD_LEN; i++) {
        sum ^= out[i];
    }
    for (size_t i = 0; i < n; i++) {
        out[VIEW_HEAD_LEN + i] = cells[i];
        sum ^= cells[i];
    }
    out[len - 1] = sum;
    return len;
}

/* Base64, for carrying a frame inside a line of text: the console today.
 * Returns the length written, NUL-terminated, or 0 if it does not fit. */
static inline size_t view_wire_base64(char *out, size_t max, const uint8_t *in,
                                      size_t n)
{
    static const char A[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const size_t need = (n + 2) / 3 * 4 + 1;
    if (need > max) {
        return 0;
    }
    size_t o = 0;
    for (size_t i = 0; i < n; i += 3) {
        const uint32_t v = ((uint32_t)in[i] << 16) |
                           ((i + 1 < n) ? (uint32_t)in[i + 1] << 8 : 0) |
                           ((i + 2 < n) ? (uint32_t)in[i + 2] : 0);
        out[o++] = A[(v >> 18) & 63];
        out[o++] = A[(v >> 12) & 63];
        out[o++] = (i + 1 < n) ? A[(v >> 6) & 63] : '=';
        out[o++] = (i + 2 < n) ? A[v & 63] : '=';
    }
    out[o] = '\0';
    return o;
}

#endif /* VIEW_WIRE_H */
