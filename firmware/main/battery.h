#pragma once

#include <stddef.h>

#include "esp_err.h"
/*
 * Battery, honestly.
 *
 * THE SENSE CIRCUIT IS NOT DOCUMENTED. Nothing in this repository - not
 * HARDWARE.md, not DATUMS.md, not the Waveshare material under reference/ -
 * says which pin the battery voltage is divided onto, or whether it is brought
 * out at all. So this does not guess a GPIO: guessing would mean driving or
 * reading a pin that may belong to something else, and reporting a number
 * derived from a guess is worse than reporting none.
 *
 * battery_percent() returns -1 until the pin is known, and callers must handle
 * that rather than printing a zero. '>battery' scans the free ADC1 channels
 * and prints what each one reads, which is how the pin gets identified: plug
 * and unplug USB, and the channel that moves with the cell is the one.
 */
int  battery_percent(void);      /* 0..100, or -1 when unknown */
int  battery_mv(void);           /* cell millivolts, or -1 */
void battery_scan(char *out, size_t max);

/* Tell the deck which channel the cell is on, and what the divider is.
 * Persisted, so it is set once. `gpio` 0 forgets it again.
 *
 * This exists so the owner can finish the job without a reflash: run
 * '>battery', unplug USB, run it again, and whichever channel moved is the
 * one. That is a measurement they can make and I cannot. */
esp_err_t battery_use(int gpio, int divider_x10);
