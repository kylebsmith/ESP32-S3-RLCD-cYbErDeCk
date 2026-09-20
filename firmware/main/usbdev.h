/*
 * The deck as a USB device: CDC (console + serial keyboard) and MIDI, at once.
 */
#pragma once

#include <stdbool.h>
#include "esp_err.h"

/* Called once from app_main, BEFORE serialkbd_init().
 *
 * Returns true when USB MIDI mode was entered, in which case the caller must
 * NOT start the USB-Serial-JTAG keyboard - the PHY no longer belongs to it.
 * Returns false in every other case, including every failure, so the default
 * path is always the one that works. */
bool usbdev_boot(void);

/* Is a host actually there? Not "did we try" - tud_mounted(). */
bool usbdev_mounted(void);

/* MIDI out. Silent when no host is mounted. */
void usbdev_midi_send(uint8_t status, uint8_t d1, uint8_t d2);
void usbdev_midi_flush(void);

/* Ask for the mode to change. Both reboot; neither can lose a document. */
esp_err_t usbdev_want(bool on);
bool      usbdev_wanted(void);
unsigned  usbdev_tries(void);

/* Messages sent, and how many USB packets carried them. */
void usbdev_packing(uint32_t *msgs, uint32_t *packets);
