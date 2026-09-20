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

/* The clock runs at 24 pulses per quarter note and a step is six of them.
 *
 * Not because the step needs the resolution - it does not - but because 24
 * PPQN IS MIDI clock. Running the timer at the rate the protocol already
 * speaks means tempo sync to a DAW is a message on an existing tick rather
 * than a second timer to keep in phase with the first, and it buys swing for
 * free: a sixth of a step is the unit a shuffle is expressed in anyway. */
#define SEQ_PPQN           24
#define SEQ_TICKS_PER_STEP 6

typedef struct {
    char     name[SEQ_NAME_MAX];
    uint32_t mask;          /* one bit per step; the realtime core reads this */
    uint32_t accent;        /* 'X' - louder                                   */
    uint32_t ghost;         /* ',' - quieter                                  */
    uint8_t  deg[SEQ_MAX_STEPS];  /* scale degree per step, 0xFF = fixed note */
    uint8_t  steps;         /* how many of them are in play                   */
    uint8_t  note;          /* MIDI note number, for a fixed-pitch lane       */
    uint8_t  chan;          /* 0-15                                           */
    uint8_t  vel;
    int8_t   octave;        /* melodic lanes only                             */
    uint32_t src;           /* hash of the pattern AS TYPED - see the toggle  */
    uint16_t gate_ms;
    bool     used;
    bool     muted;
    bool     melodic;
} seq_lane_t;

esp_err_t seq_init(void);

/* Compile one lane. `steps` is a string where '.', '-' and '_' are rests and
 * anything else is a hit, so "x...x...x...x..." and "o---o---o---o---" both
 * work and nobody has to remember which character is correct.
 *
 * Three characters mean more than "a hit", and they were chosen so that the
 * pattern still reads as a picture of itself:
 *
 *   X   accent - louder. One shift key, and it stands up off the line.
 *   ,   ghost  - quieter. Small on the page, small in the mix.
 *   0-9 on a MELODIC lane, the scale degree. 0 is the root.
 *
 * A digit on a drum lane is just a hit; a lane knows which kind it is. This
 * matters because it means there is ONE pattern grammar, not two: the same
 * line of text, the same length, the same rests, whether it is a kick or a
 * bassline. */
esp_err_t seq_lane(const char *name, const char *steps);

/* The key. One word: a root, optionally '#' or 'b', then a mode -
 * "dmin", "c", "f#mix", "apent", "ebblues", "chrom".
 *
 * WHY DEGREES AND NOT NOTE NAMES. On this keyboard every character costs, and
 * a wrong note costs more. A degree cannot be out of key, so the player picks
 * SHAPE - which is the musical decision - and the key is one word they change
 * once. Changing it mid-performance transposes every melodic lane on the next
 * step, because the degrees are what is stored and the note is resolved at
 * the moment it sounds. That is a feature, and it is why the resolution is
 * not done at compile time. */
esp_err_t   seq_scale(const char *spec);
const char *seq_scale_name(void);

/* A melodic lane reads its pattern as scale degrees. Octave is in the usual
 * convention where middle C is C4 = 60. */
esp_err_t seq_lane_melodic(const char *name, int octave, int chan,
                           int gate_ms);

/* Shuffle, as a percentage: 50 is straight, 67 is triplet swing, 75 is as
 * far as this goes before it stops being a groove. Every ODD sixteenth is
 * delayed; the downbeats never move, which is what keeps it danceable. */
void seq_swing(int percent);
int  seq_get_swing(void);

/* MIDI clock out - 0xF8 every tick, start and stop around it. This is how the
 * deck drives a DAW's tempo rather than fighting it. Off by default: it is
 * 48 messages a second at 120 bpm and nobody should pay for it unmeasured. */
void seq_sync(bool on);
bool seq_get_sync(void);

/* Find a lane by name without creating one. `len` < 0 means NUL-terminated;
 * otherwise it is a span, because the editor holds a name inside a line of
 * text rather than a string of its own.
 *
 * The span form compares the terminator too. Without that test "kick" would
 * match a lane named "kickdrum", and the playhead would sweep the wrong line.
 */
const seq_lane_t *seq_lane_find(const char *name, int len);

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

/* WHERE EVENTS GO, AND WHY IT IS A TABLE.
 *
 * Max/MSP splits its world down the middle: `~` objects are audio, `jit.`
 * objects are video, and the two halves have different rules, different
 * scheduling and, in practice, different users. That split is not a law of
 * nature - it is an artefact of how the two subsystems were built - and it is
 * the single biggest reason a patch that makes sound cannot easily make a
 * picture.
 *
 * So there is no audio path and no visual path here. There is a LANE, which
 * is a row of characters and an intensity, and there are DESTINATIONS, which
 * decide what a lane means. The same 'x...x...x...x...' is a kick drum on a
 * MIDI destination and a frame trigger on a network one, and nothing in the
 * lane, the clock or the editor knows the difference. Adding live visuals is
 * adding a destination, not adding a second half of the system.
 *
 * Each destination carries an enable flag, because a radio that is on is a
 * radio that is drawing current. Turning one off is a command, not a rebuild.
 */
typedef void (*seq_sink_t)(uint8_t status, uint8_t d1, uint8_t d2);

#define SEQ_MAX_DESTS 4

/* Register a destination. `name` is what the player types. Registering does
 * not enable it: a destination that switched itself on at boot would be a
 * radio nobody asked for. */
esp_err_t seq_dest_add(const char *name, seq_sink_t fn, const char *help);

/* Turn one on or off by name. Returns ESP_ERR_NOT_FOUND for a name that was
 * never registered, so a typo is reported rather than silently doing
 * nothing - which is how a performer ends up on stage wondering why. */
esp_err_t seq_dest_enable(const char *name, bool on);
bool      seq_dest_is_on(const char *name);

/* Enumerate, for '>send' and for the status line. */
int         seq_dest_count(void);
const char *seq_dest_name(int i);
const char *seq_dest_help(int i);
bool        seq_dest_on(int i);

/* Silence everything, immediately, from any context. */
void seq_all_notes_off(void);

/* Events dropped because the transport could not keep up. A late note is
 * worse than a lost one, so the clock never blocks - but the count must be
 * visible or the loss is silent. */
uint32_t seq_dropped(void);
