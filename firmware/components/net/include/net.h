/*
 * The network, and OSC out.
 *
 * WHY THIS IS OFF UNTIL ASKED. The ESP32-S3 has ONE radio. Wi-Fi and BLE
 * coexist through a scheduler, and the deck already runs two BLE links - a HID
 * central for the keyboard and, when enabled, a MIDI peripheral. Adding Wi-Fi
 * is adding a third claimant to the same airtime, and the owner has already
 * heard what radio contention does to timing. So it is a command, not a
 * default, and USB MIDI remains the path with a measured 0.03 ms of jitter.
 *
 * TWO MODES, for two situations the owner named:
 *
 *   JOIN  a network that exists - a studio, a venue, home.
 *   HOST  its own - an installation with no infrastructure, or a performance
 *         where a Pi and a phone need something to attach to and the deck is
 *         the only thing on stage that can be relied on to be there.
 *
 * OSC rather than raw MIDI over the wire, for one reason: MIDI throws away
 * WHICH LANE fired. '/deck/kick' is a mapping a visual patch can bind to;
 * note 36 on channel 10 is a convention the receiver has to already know.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* Join an existing network. Credentials go to NVS, not to a document - a
 * password in a document would be mirrored to the SD card and copied to the
 * owner's DGX with the rest of their writing. */
esp_err_t net_join(const char *ssid, const char *pass);

/* Host one. Open if `pass` is NULL or shorter than 8 characters, because WPA2
 * requires 8 and silently falling back to open would be a lie. */
esp_err_t net_host(const char *ssid, const char *pass);

/* Stop the radio entirely and give the airtime back. */
void net_stop(void);

/* One line, 30 columns: mode, whether it is up, and the address. */
void net_status(char *out, size_t max);
bool net_up(void);

/* Where OSC goes. Setting a target enables the destination; port 0 disables. */
esp_err_t net_osc_target(const char *ip, int port);

/* The destination hooks, registered by the app. */
void net_osc_send(const char *lane, uint8_t status, uint8_t d1, uint8_t d2,
                  uint32_t when_us);
void net_osc_flush(void);

/* The transport position, once per step. A receiver's whole timebase in one
 * message - it needs no MIDI clock and no note to know where the bar is. */
void net_osc_step(int step);

/* A screenful of ASCII as '/deck/frame'. This is the visual primitive: the
 * deck's medium IS a rectangle of characters, so the most useful thing it can
 * hand a renderer is that rectangle, not a description of one. Its own
 * datagram, because it is up to a few hundred bytes. */
esp_err_t net_osc_frame(const char *text);

/* Datagrams sent, and messages packed into them. */
void net_osc_counts(uint32_t *msgs, uint32_t *packets);
