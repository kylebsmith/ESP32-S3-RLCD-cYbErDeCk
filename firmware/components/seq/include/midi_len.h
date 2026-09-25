/*
 * How many data bytes a MIDI status byte carries.
 *
 * THIS TABLE IS THE WHOLE PROTOCOL, AND GETTING IT WRONG IS SILENT. A receiver
 * counts data bytes. Send one too few and it waits, absorbing the next status
 * byte as data, and everything after that is garbage until a Reset - with no
 * error anywhere. The drum machine simply stops responding, which reads as a
 * wiring fault or a firmware fault and is neither.
 *
 * It lives in a header with no dependencies, compiled into the firmware and
 * into tools/test_midi_wire.c, for the same reason seq_pattern.h and
 * st7305_addr.h do: a rule that two pieces of code must agree about is a rule
 * that has to exist once.
 *
 * The first version of this was inside the DIN sink and returned 0 for every
 * status from 0xF0 up. That is right for System Realtime and WRONG for System
 * Common: Song Position Pointer carries two data bytes and is exactly what a
 * DAW or a drum machine needs in order to start on the right bar. It would have
 * gone out as a bare 0xF2 and hung the receiver's parser on the one message
 * that begins a synchronised performance.
 */
#ifndef MIDI_LEN_H
#define MIDI_LEN_H

#include <stdbool.h>
#include <stdint.h>

static inline int midi_datalen(uint8_t status)
{
    if (status >= 0xF8) {
        return 0;          /* System Realtime: clock, start, stop - one byte */
    }
    switch (status) {
    case 0xF1: return 1;   /* MTC quarter frame                             */
    case 0xF2: return 2;   /* Song Position Pointer                         */
    case 0xF3: return 1;   /* Song Select                                   */
    case 0xF4:
    case 0xF5:
    case 0xF6:
    case 0xF7: return 0;   /* undefined, tune request, end of exclusive     */
    default: break;
    }
    switch (status & 0xF0) {
    case 0xC0:             /* program change   */
    case 0xD0: return 1;   /* channel pressure */
    default:   return 2;   /* note, poly pressure, CC, pitch bend           */
    }
}

/* 0xF9 IS NOT MIDI. It is the deck's own step marker - undefined in the spec -
 * and it is how a destination that cares about the bar learns where the bar is.
 * Every sink that talks to something outside this device has to drop it, and
 * saying so once is how they stay in agreement. */
static inline bool midi_is_internal(uint8_t status)
{
    return status == 0xF9;
}

#endif /* MIDI_LEN_H */
