/*
 * ASCII visuals, as lanes.
 *
 * THE WHOLE IDEA. A visual here is not a second language and not a second
 * environment - it is a lane, exactly like a drum lane, whose destination
 * happens to be a rectangle of characters instead of a synth. So live visual
 * coding is the same Ctrl+Enter on the same line in the same document as the
 * music, and the two can sit four lines apart driving each other.
 *
 * FOUR GENERATORS, AND NO MORE WITHOUT AN ARGUMENT. Complexity is meant to
 * come from primitives combining, the way a 3D model comes out of cubes and
 * spheres - so the generators are deliberately dumb and the interesting part
 * is that eight lanes can drive them at once, at different rates, with
 * probability, routed from each other:
 *
 *   noise   random glyphs, density from the step value
 *   bar     a column whose height is the step value
 *   dot     a single glyph that walks with the step
 *   wave    a sine row whose amplitude is the step value
 *
 * ROUTING is the sidechain. '>route noise bass' means the noise lane takes its
 * intensity from whatever the bass lane last played instead of from its own
 * digits. One idea - a lane can read another lane's output - and it works
 * between any two lanes, which is why it is not called sidechaining: the same
 * mechanism sends a kick to a bar height or a filter sweep to a wave.
 *
 * FRAME BY FRAME. viz_tick() is called from the sequencer's step, so the
 * animation runs on the same clock as the music and every frame is a step. The
 * frame is a character buffer; the editor draws it in a split, and '>frame'
 * sends the same buffer over OSC to whatever is rendering it elsewhere.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define VIZ_W 32
#define VIZ_H 12

/* Compile a visual lane. `gen` is one of the four names; `pattern` is the
 * ordinary lane grammar, and a step's digit is its intensity 0-9. */
esp_err_t viz_lane(const char *gen, const char *pattern);

/* Take intensity from another lane's last output rather than from the digits.
 * `src` NULL or empty unroutes. */
esp_err_t viz_route(const char *gen, const char *src);

/* Told by the sequencer: this lane just played this value. Feeds routing. */
void viz_lane_played(const char *lane, uint8_t value);

/* Advance one frame. Called on the sequencer's step, so the animation and the
 * music share a clock by construction rather than by being synchronised. */
void viz_tick(uint32_t step);

/* The frame, as VIZ_H rows of VIZ_W characters plus a terminator per row. */
const char *viz_row(int y);
bool viz_active(void);

/* The whole frame as one newline-separated string, for '>frame'. */
int viz_text(char *out, int max);

/* Split-screen preview: how many columns the visual gets, or 0 for off. */
void viz_split(bool on);
bool viz_split_on(void);
