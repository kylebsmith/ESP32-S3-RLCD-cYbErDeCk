#pragma once
/* The 8x24 middle face. Squeezed from the 12x24 art by tools/make_font.py
 * --8x24, so there is one set of letterforms rendered at three sizes rather
 * than three sets of opinions about the same letters.
 *
 * Eight and not nine: cell width must be EVEN - the RASET quantum in
 * landscape - and the device rejected a nine-wide cell outright. */
#include <stdint.h>

extern const uint8_t font8x24[(126 - 32 + 1) * 24 * 2];
