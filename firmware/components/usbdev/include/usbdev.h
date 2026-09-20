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

/* Called from the main loop, every pass. Performs any work a timer callback
 * decided on but must not do itself.
 *
 * THIS EXISTS BECAUSE THE SAME BUG STRANDED THIS DEVICE TWICE IN ONE SESSION.
 * esp_timer dispatch callbacks run on a shared task with a 3.5 KB stack that
 * must not block, and both the revert path and the reboot path were calling
 * NVS writes, flash writes and SD card I/O from there. Each takes a lock and
 * can block indefinitely; when it did, the timer task stopped - taking the
 * sequencer clock, the editor and the console with it - while TinyUSB's own
 * task carried on enumerating, so the deck looked perfectly alive to the host
 * and answered nothing. Nothing tripped the task watchdog either, because it
 * watches the idle tasks and those were still running.
 *
 * So: timer callbacks set a flag. This does the work, in task context, where
 * blocking is allowed. */
void usbdev_poll(void);

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
