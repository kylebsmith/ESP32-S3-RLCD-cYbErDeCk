/*
 * BLE MIDI, the deck as a peripheral.
 *
 * Chosen as the FIRST transport for three reasons, none of them elegance:
 *  - it needs no new hardware, unlike the DIN path;
 *  - it does not touch the USB PHY, so the console, the serial keyboard and
 *    button-free flashing all survive - and this board has no RESET switch
 *    (docs/ASSEMBLY.md: PWR, BOOT, KEY), so losing the console is not a
 *    recoverable mistake;
 *  - macOS speaks it natively through Audio MIDI Setup, with no driver.
 *
 * What it is NOT good for is being the clock. Round-trip is around 19 ms and
 * jittery; docs/OS.md is explicit that BLE MIDI must never be the timing
 * master. The deck runs its own clock and BLE MIDI carries the notes.
 *
 * Spec: BLE-MIDI 1.0. Service 03B80E5A-EDE8-4B33-A751-6CE34EC4C700,
 * characteristic 7772E5DB-3868-4112-A1A9-F2669D106BF3, notify + write.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Register the GATT service. Must run inside kbd's `gatt` hook. */
void blemidi_register(void);

/* Begin advertising. Runs from kbd's `synced` hook. */
void blemidi_start(void);

/* Send one MIDI message. Safe to call when nothing is connected. */
void blemidi_send(uint8_t status, uint8_t d1, uint8_t d2);

bool blemidi_connected(void);
