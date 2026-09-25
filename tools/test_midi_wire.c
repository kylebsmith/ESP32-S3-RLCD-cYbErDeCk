/*
 * What goes on the wire.
 *
 * Every sink that leaves this device - USB, BLE, DIN - has to agree about two
 * things, and both fail silently when they do not:
 *
 *   1. HOW LONG EACH MESSAGE IS. A receiver counts data bytes. One too few and
 *      it waits, swallows the next status byte as data, and everything after
 *      that is garbage until a Reset. Nothing reports an error; the drum machine
 *      just stops responding, which reads as a wiring fault and is not one.
 *
 *   2. THAT 0xF9 NEVER LEAVES. It is the deck's own step marker, undefined in
 *      MIDI, and it is how a destination that cares about the bar learns where
 *      the bar is. Sending it would put an undefined realtime byte in front of a
 *      parser with no reason to expect one.
 *
 * This is a host check because the failure is not observable from the device:
 * the deck cannot see that its receiver's parser has desynchronised.
 */
#include <stdio.h>
#include "midi_len.h"

static int fails;

static void want(uint8_t status, int expect, const char *what)
{
    const int got = midi_datalen(status);
    if (got != expect) {
        printf("  [FAIL] 0x%02X %-18s %d data bytes, want %d\n",
               status, what, got, expect);
        fails++;
    } else {
        printf("  [ ok ] 0x%02X %-18s %d\n", status, what, got);
    }
}

int main(void)
{
    printf("-- every status byte has the right length --\n");
    /* Channel voice. The two one-byte ones are the whole reason this needs a
     * table rather than a constant. */
    want(0x80, 2, "note off");
    want(0x90, 2, "note on");
    want(0xA0, 2, "poly pressure");
    want(0xB0, 2, "control change");
    want(0xC0, 1, "program change");
    want(0xD0, 1, "channel pressure");
    want(0xE0, 2, "pitch bend");
    /* The same again on a high channel, because the table must mask the
     * channel off and not test the byte whole. */
    want(0x9F, 2, "note on ch16");
    want(0xCF, 1, "program ch16");

    /* System Common. SPP is the one that was wrong, and it is the one that
     * begins a synchronised performance. */
    printf("\n-- system common --\n");
    want(0xF1, 1, "MTC quarter frame");
    want(0xF2, 2, "song position");
    want(0xF3, 1, "song select");
    want(0xF6, 0, "tune request");

    /* System Realtime: single byte, all of them. */
    printf("\n-- system realtime is one byte --\n");
    want(0xF8, 0, "timing clock");
    want(0xFA, 0, "start");
    want(0xFB, 0, "continue");
    want(0xFC, 0, "stop");
    want(0xFE, 0, "active sensing");
    want(0xFF, 0, "reset");

    printf("\n-- the deck's own marker never leaves --\n");
    if (!midi_is_internal(0xF9)) {
        printf("  [FAIL] 0xF9 is not marked internal\n");
        fails++;
    } else {
        printf("  [ ok ] 0xF9 is internal\n");
    }
    /* And nothing else is, or a real message would be dropped. */
    int wrong = 0;
    for (int s = 0x80; s <= 0xFF; s++) {
        if (s != 0xF9 && midi_is_internal((uint8_t)s)) { wrong++; }
    }
    if (wrong) {
        printf("  [FAIL] %d other statuses marked internal\n", wrong);
        fails++;
    } else {
        printf("  [ ok ] and nothing else is\n");
    }

    printf("\n%s (%d problems)\n",
           fails ? "MIDI WIRE CHECKS FAILED" : "ALL MIDI WIRE CHECKS PASS", fails);
    return fails ? 1 : 0;
}
