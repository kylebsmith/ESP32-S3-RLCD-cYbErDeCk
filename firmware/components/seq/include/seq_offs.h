/*
 * A NOTE-OFF ENDS ITS OWN NOTE, NEVER A LATER ONE.
 *
 * Offs are scheduled when a note starts (seq.c), so none can be forgotten. But
 * MIDI has no idea which note an off belongs to: it ends whatever is sounding
 * at that pitch on that channel. A note held past the next note of the same
 * pitch - a pad chord ringing into the next chord's common tone, a long-gated
 * bass rolling sixteenths - had its off land in the MIDDLE of the newer note
 * and end it early, at a point set by the gate and not the grid. Counted in
 * the EP as it stood on 2026-09-29: ORBITALS lost 573 notes that way, GRID 60,
 * OFFSET 172 - which is what "the timing just feels like it kinda falls
 * apart" sounds like when every clock edge is on time.
 *
 * So a note that starts while its pitch is still pending an off sends that off
 * first: the old note ends at the new one's start, and the new one keeps its
 * whole gate. A retrigger, as a keyboard does it.
 *
 * Pure, so tools/test_offs.c checks the rule the firmware runs.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int64_t     due_us;
    const char *lane;       /* who scheduled it, for whatever reads the event */
    uint8_t     status, d1;
    bool        armed;
} seq_off_t;

/* Before a note-on: the off still pending for this pitch on this channel,
 * disarmed so the caller sends it now - or -1 when there is none. */
static inline int seq_off_take(seq_off_t *offs, int n, uint8_t chan, uint8_t note)
{
    const uint8_t status = (uint8_t)(0x80 | (chan & 0x0F));
    for (int i = 0; i < n; i++) {
        if (offs[i].armed && offs[i].status == status && offs[i].d1 == note) {
            offs[i].armed = false;
            return i;
        }
    }
    return -1;
}

/* Schedule an off; -1 when the table is full and the caller must send it
 * at once rather than lose it. */
static inline int seq_off_arm(seq_off_t *offs, int n, int64_t due, const char *lane,
                              uint8_t chan, uint8_t note)
{
    for (int i = 0; i < n; i++) {
        if (!offs[i].armed) {
            offs[i].armed  = true;
            offs[i].due_us = due;
            offs[i].lane   = lane;
            offs[i].status = (uint8_t)(0x80 | (chan & 0x0F));
            offs[i].d1     = note;
            return i;
        }
    }
    return -1;
}
