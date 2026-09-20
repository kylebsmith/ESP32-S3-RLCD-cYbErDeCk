/*
 * The sequencer: lanes, a clock, and a destination per lane.
 *
 * WHY LANES AND NOT A GRID. ORCA puts the program, the performance and the
 * display on one surface, and that is what earns its 2D layout - you watch
 * bangs propagate and that IS the piece. This device's output is somewhere
 * else: a synth over MIDI, lights over OSC, a Pi driving a projector. The
 * spatial payoff lands on a different machine, so all that is left of a
 * matrix here is its cost - constant 2D cursor movement on a thumb keyboard,
 * which is the input this hardware is worst at.
 *
 * A lane is one line of text. Arrow keys move between lanes, Home and End
 * move within one, and every gesture is something the Rii does well. The
 * grammar is in the spirit of ixi lang, which was designed for readability
 * and few keystrokes, rather than of ORCA, which is deliberately esoteric.
 *
 *     kick   x...x...x...x...      36 on channel 10
 *     snare  ....x.......x...      38
 *     hat    x.x.x.x.x.x.x.x.      42
 *
 * THE REALTIME CORE NEVER PARSES TEXT. A lane compiles to a bitmask and a
 * few bytes; the clock callback reads those and nothing else. Editing is a
 * compile step, so a slow redraw or a long line can never make a note late -
 * which is the whole reason docs/OS.md wants the two-core split.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define SEQ_MAX_LANES 8
#define SEQ_MAX_STEPS 32
#define SEQ_NAME_MAX  12

typedef struct {
    char     name[SEQ_NAME_MAX];
    uint32_t mask;          /* one bit per step; the realtime core reads this */
    uint8_t  steps;         /* how many of them are in play                   */
    uint8_t  note;          /* MIDI note number                               */
    uint8_t  chan;          /* 0-15                                           */
    uint8_t  vel;
    uint16_t gate_ms;
    bool     used;
    bool     muted;
} seq_lane_t;

esp_err_t seq_init(void);

/* Compile one lane. `steps` is a string where anything other than '.' or ' '
 * is a hit, so "x...x...x...x..." and "o---o---o---o---" both work and
 * nobody has to remember which character is correct. */
esp_err_t seq_lane(const char *name, const char *steps);

/* A lane's destination. Names are remembered from the drum table. */
esp_err_t seq_lane_note(const char *name, int note, int chan);
esp_err_t seq_mute(const char *name, bool mute);

void seq_bpm(int bpm);
int  seq_get_bpm(void);
void seq_play(void);
void seq_stop(void);
bool seq_running(void);
int  seq_position(void);

const seq_lane_t *seq_lanes(int *count);

/* Where notes go. Set by whichever transport is up - the sequencer does not
 * know or care whether this is BLE MIDI, USB MIDI, a UART or a log line. */
typedef void (*seq_sink_t)(uint8_t status, uint8_t d1, uint8_t d2);
void seq_set_sink(seq_sink_t sink);

/* Silence everything, immediately, from any context. */
void seq_all_notes_off(void);

/* Events dropped because the transport could not keep up. A late note is
 * worse than a lost one, so the clock never blocks - but the count must be
 * visible or the loss is silent. */
uint32_t seq_dropped(void);
