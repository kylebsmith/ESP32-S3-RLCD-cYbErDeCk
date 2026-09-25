/*
 * MIDI on a wire, at 31250 baud.
 *
 * WHY THIS EXISTS, and it is the most important sentence in this file: every
 * other transport this deck has needs a COMPUTER in the middle. USB MIDI makes
 * the deck a USB *device*, so it has to be plugged into a host; an SP404, a
 * Mutant Brain, a MIDI interface and every other piece of studio hardware is
 * either a USB device too - and two devices cannot talk to each other - or it
 * has no USB at all and wants five-pin DIN. BLE MIDI needs a host that speaks
 * BLE MIDI. OSC needs a network and something listening on it.
 *
 * So the deck could not be plugged into a single piece of hardware in the
 * owner's studio without a laptop sitting between them, which is exactly what
 * an instrument should not require. MIDI over a UART fixes that for all of it
 * at once, because DIN MIDI has been one protocol on one wire since 1983:
 * 31250 baud, 8N1, the same bytes already being written to the USB endpoint.
 *
 * THE ELECTRICAL SIDE IS NOT OPTIONAL and is not done in firmware. A MIDI
 * output is a current loop: pin 4 to +5 V through 220 ohms, pin 5 to the UART
 * TX through 220 ohms, pin 2 to ground. From 3.3 V logic the classic values
 * become 33 ohms and 10 ohms, or use a buffer to 5 V. TRS Type A puts tip on
 * pin 5, ring on pin 4, sleeve on pin 2. Wire it wrong and the receiver sees
 * nothing; there is no way for this code to tell the difference between wrong
 * wiring and a quiet part, which is why the pin is the owner's to declare.
 *
 * THE PIN IS NOT GUESSED. docs/ASSEMBLY.md records that guessing a pin on this
 * board has already cost an afternoon once - the battery sense line - and the
 * conclusion drawn there applies unchanged: a pin is a fact about a physical
 * object, and the only party who can see the object is the owner. '>din 17'
 * says which one.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* Start MIDI output on this GPIO, or stop it with gpio < 0. Starting twice on
 * the same pin is a no-op; starting on a different pin moves it. */
esp_err_t dinmidi_start(int gpio);
void      dinmidi_stop(void);

/* The pin in use, or -1. */
int  dinmidi_pin(void);
bool dinmidi_running(void);

/* A sequencer destination: three bytes, or two when the status says so. */
void dinmidi_send(const char *lane, uint8_t status, uint8_t d1, uint8_t d2,
                  uint32_t when_us);

/* Bytes written since the last call, so '>din' can prove the wire is busy
 * without the owner needing a scope. */
uint32_t dinmidi_bytes(void);
