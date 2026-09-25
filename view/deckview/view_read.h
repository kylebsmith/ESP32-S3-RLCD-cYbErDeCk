/*
 * The view node's reader for docs/VIEW.md, as one pure state machine so the
 * sketch and tools/test_view_wire.c run the same code - the host check packs
 * frames with the deck's firmware/main/view_wire.h and feeds them through this.
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

#define VIEW_MAX_W 120
#define VIEW_MAX_H 60
#define VIEW_RAW_MAX (4 + 6 + VIEW_MAX_W * VIEW_MAX_H + 1)

typedef struct {
    int      state, got;
    uint8_t  head[6];
    uint8_t  sum;
    uint8_t  cells[VIEW_MAX_W * VIEW_MAX_H];
    uint8_t  raw[VIEW_RAW_MAX];          /* every byte since the magic       */
    int      nraw;
    uint8_t  q[2 * VIEW_RAW_MAX];        /* bytes to read again, then input */
    uint8_t  t[2 * VIEW_RAW_MAX];
    /* the last good frame */
    uint32_t tick;
    uint8_t  w, h;
    uint32_t frames, refused;
} view_reader_t;

enum { VR_MAGIC = 0, VR_HEAD, VR_CELLS, VR_SUM };

/* One step. 1 = a good frame, -1 = refused, 0 = nothing yet. */
static inline int view_read_step(view_reader_t *r, uint8_t b)
{
    static const uint8_t M[4] = { 'D', 'K', 'V', '1' };
    if (r->state != VR_MAGIC && r->nraw < VIEW_RAW_MAX) {
        r->raw[r->nraw++] = b;
    }
    switch (r->state) {
    case VR_MAGIC:
        if (b == M[r->got]) {
            if (++r->got == 4) {
                r->state = VR_HEAD;
                r->got = 0;
                r->sum = 0;
                memcpy(r->raw, M, 4);
                r->nraw = 4;
            }
        } else {
            r->got = (b == M[0]) ? 1 : 0;
        }
        return 0;
    case VR_HEAD:
        r->head[r->got++] = b;
        r->sum ^= b;
        if (r->got == 6) {
            const int w = r->head[4], h = r->head[5];
            r->got = 0;
            if (w < 1 || h < 1 || w > VIEW_MAX_W || h > VIEW_MAX_H) {
                return -1;
            }
            r->state = VR_CELLS;
        }
        return 0;
    case VR_CELLS:
        r->cells[r->got++] = b;
        r->sum ^= b;
        if (r->got == r->head[4] * r->head[5]) { r->state = VR_SUM; }
        return 0;
    default:
        if (b != r->sum) {
            return -1;
        }
        r->state = VR_MAGIC;
        r->got = 0;
        r->nraw = 0;
        r->tick = (uint32_t)r->head[0] | ((uint32_t)r->head[1] << 8) |
                  ((uint32_t)r->head[2] << 16) | ((uint32_t)r->head[3] << 24);
        r->w = r->head[4];
        r->h = r->head[5];
        r->frames++;
        return 1;
    }
}

/* One byte in. Returns how many good frames it completed - usually 0 or 1, more
 * only when a refusal gives back bytes that hold frames of their own. */
static inline int view_read_byte(view_reader_t *r, uint8_t b)
{
    int qh = 0, qn = 1, done = 0;
    r->q[0] = b;
    while (qn > 0) {
        const uint8_t c = r->q[qh++];
        qn--;
        const int e = view_read_step(r, c);
        if (e > 0) {
            done++;
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
