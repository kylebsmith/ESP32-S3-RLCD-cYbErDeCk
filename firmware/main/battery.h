#pragma once

#include <stddef.h>
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
void battery_scan(char *out, size_t max);  /* one line per free ADC channel */
