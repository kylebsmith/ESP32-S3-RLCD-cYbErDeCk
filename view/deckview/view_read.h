/*
 * The view node's reader for docs/VIEW.md, as one pure state machine so the
 * sketch and tools/test_view_wire.c run the same code - the host check packs
 * frames with the deck's firmware/main/view_wire.h and feeds them through this.
 *
 * TWO KINDS OF FRAME, ONE READER. A picture ('DKV1': the cells) and a control
 * frame ('DKC1': the mode, and the poster's lines) share a magic up to its third
 * byte and a checksum, so one scan finds both and one resync serves both.
 *
 * ONE LOST BYTE COSTS ONE FRAME. A reader that goes back to scanning when a
 * frame is refused loses the NEXT frame too: a torn frame's header and cells are
 * read out of the frame after it, whose magic is then already eaten. So every
 * byte since a candidate's magic is kept, and when the candidate is refused they
 * go back in front of the input, less the first - so the scan resumes one byte
 * later and finds the magic it passed over. It is a loop over a queue, never a
 * recursion, because a run of bad frames must not be able to take the stack; and
 * it terminates because every refusal gives back at least one byte fewer than it
 * took.
 */
#pragma once
#include <stdint.h>
#include <string.h>

/* The largest picture the deck sends is 80 x 30 (VIZ_W x VIZ_H); this leaves
 * room above it without spending the node's RAM on a size nothing draws. */
#define VIEW_MAX_W 100
#define VIEW_MAX_H 40
/* The deck's view_wire.h says the same; tools/test_view_wire.c holds them to it. */
#define VIEW_READ_TEXT_MAX  1024
#define VIEW_READ_LINES_MAX 12
#define VIEW_RAW_MAX (4 + 6 + VIEW_MAX_W * VIEW_MAX_H + 1)

enum { VR_PICTURE = 1, VR_CONTROL = 2 };   /* what view_read_byte() completed */

typedef struct {
    int      state, got, kind, need;
    uint8_t  head[8];
    uint8_t  sum;
    uint8_t  cells[VIEW_MAX_W * VIEW_MAX_H];
    uint8_t  body[VIEW_READ_TEXT_MAX];        /* a control frame's text, as it comes */
    uint8_t  raw[VIEW_RAW_MAX];          /* every byte since the magic       */
    int      nraw;
    uint8_t  q[2 * VIEW_RAW_MAX];        /* bytes to read again, then input */
    uint8_t  t[2 * VIEW_RAW_MAX];
    /* the last good picture */
    uint32_t tick;
    uint8_t  w, h;
    uint32_t frames, refused;
    /* the last good control frame */
    uint32_t ctl_tick, controls;
    uint8_t  mode, nlines;
    uint16_t text_len;
    uint8_t  text[VIEW_READ_TEXT_MAX];
} view_reader_t;

enum { VR_MAGIC = 0, VR_HEAD, VR_BODY, VR_SUM };

/* One step. VR_PICTURE or VR_CONTROL for a good frame, -1 refused, 0 not yet. */
static inline int view_read_step(view_reader_t *r, uint8_t b)
{
    if (r->state != VR_MAGIC && r->nraw < VIEW_RAW_MAX) {
        r->raw[r->nraw++] = b;
    }
    switch (r->state) {
    case VR_MAGIC: {
        const int g = r->got;
        const int ok = (g == 0 && b == 'D') || (g == 1 && b == 'K') ||
                       (g == 2 && (b == 'V' || b == 'C')) || (g == 3 && b == '1');
        if (!ok) {
            r->got = (b == 'D') ? 1 : 0;
            return 0;
        }
        if (g == 2) {
            r->kind = (b == 'C');
        }
        if (++r->got == 4) {
            r->state = VR_HEAD;
            r->got = 0;
            r->sum = 0;
            r->raw[0] = 'D'; r->raw[1] = 'K'; r->raw[2] = r->kind ? 'C' : 'V';
            r->raw[3] = '1';
            r->nraw = 4;
        }
        return 0;
    }
    case VR_HEAD:
        r->head[r->got++] = b;
        r->sum ^= b;
        if (r->got == (r->kind ? 8 : 6)) {
            r->got = 0;
            if (r->kind) {
                const int n = r->head[5], len = r->head[6] | (r->head[7] << 8);
                if (n > VIEW_READ_LINES_MAX || len > VIEW_READ_TEXT_MAX) {
                    return -1;
                }
                r->need = len;
            } else {
                const int w = r->head[4], h = r->head[5];
                if (w < 1 || h < 1 || w > VIEW_MAX_W || h > VIEW_MAX_H) {
                    return -1;
                }
                r->need = w * h;
            }
            r->state = r->need > 0 ? VR_BODY : VR_SUM;
        }
        return 0;
    case VR_BODY:
        if (r->kind) {
            r->body[r->got++] = b;
        } else {
            r->cells[r->got++] = b;
        }
        r->sum ^= b;
        if (r->got == r->need) { r->state = VR_SUM; }
        return 0;
    default: {
        if (b != r->sum) {
            return -1;
        }
        const uint32_t tick = (uint32_t)r->head[0] | ((uint32_t)r->head[1] << 8) |
                              ((uint32_t)r->head[2] << 16) | ((uint32_t)r->head[3] << 24);
        r->state = VR_MAGIC;
        r->got = 0;
        r->nraw = 0;
        if (r->kind) {
            r->ctl_tick = tick;
            r->mode = r->head[4];
            r->nlines = r->head[5];
            r->text_len = (uint16_t)r->need;
            memcpy(r->text, r->body, (size_t)r->need);
            r->controls++;
            return VR_CONTROL;
        }
        r->tick = tick;
        r->w = r->head[4];
        r->h = r->head[5];
        r->frames++;
        return VR_PICTURE;
    }
    }
}

/* One byte in. Returns VR_PICTURE and/or VR_CONTROL for what it completed -
 * usually nothing or one, both only when a refusal gives back bytes holding
 * frames of their own. */
static inline int view_read_byte(view_reader_t *r, uint8_t b)
{
    int qh = 0, qn = 1, done = 0;
    r->q[0] = b;
    while (qn > 0) {
        const uint8_t c = r->q[qh++];
        qn--;
        const int e = view_read_step(r, c);
        if (e > 0) {
            done |= e;
        } else if (e < 0) {
            /* Refused: read again everything after the candidate's first byte,
             * then whatever was still waiting. */
            r->refused++;
            const int n = r->nraw;
            int k = 0;
            for (int i = 1; i < n; i++) { r->t[k++] = r->raw[i]; }
            for (int i = 0; i < qn; i++) { r->t[k++] = r->q[qh + i]; }
            memcpy(r->q, r->t, (size_t)k);
            qh = 0;
            qn = k;
            r->state = VR_MAGIC;
            r->got = 0;
            r->nraw = 0;
        }
    }
    return done;
}

/* Line `i` of the last control frame: its characters and span. Returns the
 * length, or -1 if there is no such line. */
static inline int view_read_line(const view_reader_t *r, int i, const uint8_t **chars,
                                 int *from, int *to)
{
    int o = 0;
    for (int k = 0; k < r->nlines && o + 2 <= r->text_len; k++) {
        const int f = r->text[o], t = r->text[o + 1];
        int e = o + 2;
        while (e < r->text_len && r->text[e] != '\n') { e++; }
        if (k == i) {
            *chars = r->text + o + 2;
            *from = f;
            *to = t;
            return e - (o + 2);
        }
        o = e + 1;
    }
    return -1;
}
