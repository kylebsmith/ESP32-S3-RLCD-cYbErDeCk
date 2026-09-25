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

/* SIXTEEN, BECAUSE THERE IS ONE BUDGET NOW.
 *
 * It was eight music lanes AND thirteen visual primitives, counted separately,
 * which meant a piece that leaned visual was penalised for it while eight drum
 * slots sat empty. One table means the player decides the mix, and sixteen is
 * what the owner's own first piece wanted: seven sounding lanes and six
 * drawing ones. */
#define SEQ_MAX_LANES 16
/* SIXTY-FOUR, BECAUSE ALTERNATION SPENDS SLOTS.
 *
 * It was 32, which is a bar of thirty-seconds and felt generous - until '<a b>'
 * arrived. Alternation is resolved by laying the pattern down once per cycle with
 * the group resolved differently each time (see seq_pattern.h), so a sixteen-step
 * lane with one two-way alternation needs 32 slots and had nothing left. At 64 the
 * same lane can alternate three ways, or nest and alternate together.
 *
 * The cost is the mask type: one bit per slot, so the four bitmasks become 64-bit.
 * That is eight bytes a lane more, and the realtime core reads them exactly as
 * before. */
#define SEQ_MAX_STEPS 64
#define SEQ_NAME_MAX  12

/* The clock runs at 24 pulses per quarter note and a step is six of them.
 *
 * Not because the step needs the resolution - it does not - but because 24
 * PPQN IS MIDI clock. Running the timer at the rate the protocol already
 * speaks means tempo sync to a DAW is a message on an existing tick rather
 * than a second timer to keep in phase with the first, and it buys swing for
 * free: a sixth of a step is the unit a shuffle is expressed in anyway. */
/* 96 PPQN internally, MIDI clock still emitted at 24.
 *
 * At 24 PPQN a sixteenth was six ticks, so swing - which delays the offbeat by
 * a whole number of ticks - had exactly FOUR distinct settings between 50 and
 * 75. Measured by running swing_ticks(): 50 and 58 produce byte-identical
 * output, as do 60 through 66. A control with 26 positions and 4 effects is a
 * control that lies to the player.
 *
 * Four times the resolution gives 13 distinct settings across the same range,
 * which is finer than the ear resolves at 124 bpm. MIDI clock is emitted every
 * fourth tick, so the wire protocol is unchanged at its required 24 PPQN. */
#define SEQ_PPQN           96
#define SEQ_TICKS_PER_STEP 24
#define SEQ_CLOCK_EVERY    4      /* 96 / 24 = MIDI clock divisor */

/* WHERE A LANE'S EVENTS GO.
 *
 * This enum is the whole point of the collapse. A lane compiles text into WHEN
 * and HOW MUCH; the binding says WHERE, and the language never mentions it. So
 * '>kick x...x...' and '>disc x...x...' are the same sentence with different
 * destinations, and a new output medium - an RP2040 over HDMI, a light rig, a
 * plotter - is a new value here rather than a change to the grammar.
 *
 * Before this there were two lane structs, field for field the same, with two
 * compile loops over the same pattern walk, two mute mechanisms, two budgets
 * and two listings. Every bug in that area was found twice, or found in one
 * half and left standing in the other for days. */
typedef enum {
    SEQ_BIND_NOTE = 0,   /* a MIDI note: drums and the melodic voices */
    SEQ_BIND_CC,         /* a controller number: digits are values    */
    SEQ_BIND_VIZ,        /* a drawing primitive: digits are amounts   */
} seq_bind_t;

typedef struct {
    char     name[SEQ_NAME_MAX];
    uint64_t mask;          /* one bit per slot; the realtime core reads this */
    uint64_t accent;        /* 'X' - louder                                   */
    uint64_t ghost;         /* ',' - quieter                                  */
    uint64_t chance;        /* '?' - maybe                                    */
    uint8_t  prob[SEQ_MAX_STEPS];  /* per-step chance %, 0 = use the default  */
    uint8_t  deg[SEQ_MAX_STEPS];  /* scale degree per step, 0xFF = fixed note */
    uint8_t  steps;         /* how many of them are in play                   */
    /* THIS LANE'S OWN TIME BASE, in internal ticks per step.
     *
     * Default SEQ_TICKS_PER_STEP. '/2' doubles it, '*2' halves it. Every lane
     * keeping its own clock is what makes polyrhythm, half-time and ratchets
     * the same mechanism rather than three features - and it is why the rate
     * lives on the lane and not on the transport. */
    uint16_t tps;
    uint8_t  note;          /* MIDI note number, for a fixed-pitch lane       */
    uint8_t  chan;          /* 0-15                                           */
    uint8_t  vel;
    int8_t   octave;        /* melodic lanes only                             */
    uint32_t src;           /* hash of the pattern AS TYPED - see the toggle  */
    uint16_t gate_ms;
    bool     used;
    bool     muted;
    bool     melodic;
    bool     ctrl;          /* a controller lane: digits are VALUES, not notes */
    uint8_t  cc;

    /* ---- the binding, and what only some bindings need ---- */
    seq_bind_t bind;
    uint8_t  prim;          /* SEQ_BIND_VIZ: which drawing primitive           */
    /* WHICH PART OF IT. Zero means the primitive itself - how much of it to draw.
     * Anything else is a parameter of that primitive, and the value goes there
     * instead: '>disc[x] 0..9..' is a lane whose events are positions.
     *
     * seq does not know what a parameter means, exactly as it does not know what
     * a primitive is. It carries a number and hands it over. */
    uint8_t  param;
    char     chr[SEQ_MAX_STEPS]; /* the step's own character, verbatim         */
    char     dir;           /* 'u','d','l','r' - a direction written in front  */

    /* ---- routing: a lane may read another lane's output ---- */
    /* THE SAME MECHANISM FOR EVERY PAIR. A kick can drive a circle's radius, a
     * filter sweep can drive a wave's amplitude, and a circle can drive a
     * bloom - because there is one table and one last_val, where before there
     * were two tables and routing existed in only one of them. */
    char     route[SEQ_NAME_MAX];  /* driven by this lane, or empty            */
    uint8_t  last_val;      /* what this lane last played, 0-9                 */
    volatile bool    trig;  /* the source fired; set in the clock callback      */
    volatile uint8_t trig_val;
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
 *   ?   maybe  - plays about half the time. A question mark is what it is.
 *
 * And ONE optional parameter, in brackets, attached to the step before it:
 *
 *   ?[15]  this step plays fifteen per cent of the time
 *
 * A bracket is not a step. It occupies no column in the step count, so the
 * playhead still lands on the character that is sounding. There are no nested
 * brackets and there never will be: a bracket may follow a step and contain a
 * number, and that is the whole of it.
 *
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

/* Make a lane a CONTROLLER lane. Its pattern digits become CC values rather
 * than scale degrees: 0 is 0 and 9 is 127, so '0..4..8..4..' is a sweep up and
 * back. Same grammar as every other lane - same rests, same '?' probability,
 * same playhead - which is the point. */
esp_err_t seq_lane_ctrl(const char *name, int cc, int chan);

/* Bind a lane to a drawing primitive. `prim` is an index into viz's own table;
 * seq does not know what a primitive is, only that it has a number. */
esp_err_t seq_lane_viz(const char *name, int prim, int param);

/* ROUTING, FOR ANY PAIR OF LANES.
 *
 * A routed lane fires WHEN its source fires, at that hit's value, and ignores
 * its own steps. Routing used to set only an amount, and a drum's velocity
 * barely varies - so a routed lane sat at full value forever and nothing about
 * the source's timing reached it.
 *
 * A lane cannot drive itself: every lane publishes what it played, so a
 * self-route would re-trigger for ever with nothing in the clock able to stop
 * it. Refused with ESP_ERR_INVALID_ARG. `src` NULL or empty unroutes. */
esp_err_t seq_route(const char *name, const char *src);

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

/* FORGET A LANE, WHICH IS NOT THE SAME AS MUTING ONE.
 *
 * Eight lanes is the budget, and until this existed there was no way to give a
 * slot back: muting leaves the lane allocated, so a session that had tried ten
 * names was full and said "no room" about a lane the document did not even
 * mention. Mute is a performance gesture and belongs on a lane that is still
 * part of the piece; forget is an editing one and means the lane is gone.
 *
 * A note already sounding still gets its note-off - the gate is scheduled
 * independently of the lane - so forgetting cannot leave a note stuck on. */
esp_err_t seq_forget(const char *name);

/* Every lane, for a blank slate. What plays should be what the document says,
 * and a new document says nothing yet. */
void seq_forget_all(void);

void seq_bpm(int bpm);
int  seq_get_bpm(void);
void seq_play(void);
void seq_stop(void);
bool seq_running(void);
/* The global step counter since play. UNMASKED: a lane finds its own step
 * with `seq_position() % lane->steps`, and any mask that is not a multiple of
 * every possible lane length introduces a phase jump when it rolls. */
uint32_t seq_position(void);

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
/* `when_us` is the moment the CLOCK decided this event happens, not the
 * moment the transport got round to it. BLE-MIDI puts a millisecond timestamp
 * in every packet precisely so a receiver can reconstruct the intended
 * timing, and stamping it at send time throws that away: the packet then
 * says "now", which is whenever the queue, the scheduler and the radio
 * happened to converge. Passing the tick time means a transport that honours
 * timestamps sees a grid as tight as the clock's own - measured here at
 * under 100 us for 99.6 % of ticks. */
typedef void (*seq_sink_t)(const char *lane, uint8_t status, uint8_t d1,
                           uint8_t d2, uint32_t when_us);

/* EIGHT, AND THE COUNT IS CHECKED AT COMPILE TIME.
 *
 * This was four, which was exactly the number of destinations that existed -
 * and then a fifth was added. seq_dest_add() returned an error, every call site
 * ignored it, and the deck came up in USB MIDI mode with a host attached, the
 * heartbeat reporting 'act1 dev1 midi1', '>usb' reporting "usb is on, host
 * attached", and NO 'usb' destination to route anything to. Everything said
 * yes and nothing played.
 *
 * The number is not the real fix. The real fix is that adding a destination can
 * no longer fail quietly: main.c now asserts what it registered, so the next
 * transport either fits or refuses to build. */
#define SEQ_MAX_DESTS 8

/* Register a destination. `name` is what the player types. Registering does
 * not enable it: a destination that switched itself on at boot would be a
 * radio nobody asked for. */
/* A destination may also declare a FLUSH. The MIDI task drains everything the
 * clock queued for a step, hands each event to every enabled destination, and
 * then flushes - so a transport that can batch knows exactly where the step
 * ends. Pass NULL when there is nothing to batch; the log destination, for
 * instance, has nothing to gain. */
typedef void (*seq_flush_t)(void);

esp_err_t seq_dest_add(const char *name, seq_sink_t fn, seq_flush_t flush,
                       const char *help);

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

/* SELF-MEASUREMENT.
 *
 * The owner perceives timing jitter, and the first rule of this project is
 * that nothing is claimed without evidence. The sequencer knows exactly when
 * each tick SHOULD have fired - the grid is t0 + n*period - and exactly when
 * it did, so it can measure its own clock without any external instrument.
 *
 * Two separate numbers, because they have different causes and different
 * fixes, and conflating them would send the optimisation in the wrong
 * direction:
 *
 *   CLOCK   dispatch time minus the ideal grid position. This is esp_timer,
 *           FreeRTOS scheduling, and anything that stalls the CPU - notably a
 *           flash write, which disables the instruction cache.
 *   XPORT   how long a note waited between being queued by the clock and
 *           being handed to the transport by the MIDI task.
 *
 * Microseconds throughout. Reading these perturbs nothing; the accumulation
 * is three integer operations on a systimer value the tick already has. */
typedef struct {
    uint32_t n;
    int64_t  sum;        /* of deviations, us            */
    int64_t  sumsq;      /* for the standard deviation   */
    int32_t  min, max;
    int32_t  base;       /* the FIRST sample; buckets are relative to this   */
    bool     based;
    uint32_t late;       /* samples more than LATE_US from the tightest    */
    /* A HISTOGRAM, NOT JUST MIN AND MAX.
     *
     * min/max says the worst tick was 12 ms late. It does not say whether
     * that happened once in forty seconds or fifty times a second, and those
     * are completely different instruments. Seven buckets, relative to the
     * tightest sample: <100us, <250, <500, <1ms, <2ms, <5ms, >=5ms. */
    uint32_t bucket[7];
    uint32_t worst_ms;   /* uptime at which `max` happened, for correlating
                          * an excursion against the log - which is how the
                          * CAUSE gets found rather than guessed */
} seq_stat_t;

/* What counts as audibly late. A 16th at 124 bpm is 121,000 us; a millisecond
 * is under 1% of that and well inside what the perception literature treats
 * as inaudible for a percussive onset. It is a counter, not a verdict. */
#define SEQ_LATE_US 1000
#define SEQ_NBUCKETS 7

void seq_stats(seq_stat_t *clock_out, seq_stat_t *xport_out);
void seq_stats_reset(void);

/* Called on every internal tick, and on every note a lane plays.
 *
 * The visuals hang off these rather than seq calling into viz directly,
 * because the sequencer must not depend on a display: the same hooks would
 * serve a coprocessor over pogo pins without seq learning about it. */
typedef void (*seq_tick_hook_t)(uint32_t tick);
typedef void (*seq_play_hook_t)(const char *lane, uint8_t value);
void seq_set_hooks(seq_tick_hook_t on_tick, seq_play_hook_t on_play);

/* A DRAWING LANE FIRED. Called from the clock callback, so the implementation
 * must only RECORD - see SEQ_BIND_VIZ. `prim` is the index passed to
 * seq_lane_viz; seq never learns what it means. */
typedef void (*seq_draw_hook_t)(int prim, int amt, char dir, uint32_t tick);
void seq_set_draw_hook(seq_draw_hook_t fn);

/* A lane bound to a PARAMETER of a primitive fired. Same contract as the draw
 * hook: called from the clock callback, so it may only record. */
typedef void (*seq_param_hook_t)(int prim, int param, int amt);
void seq_set_param_hook(seq_param_hook_t fn);

/* Events dropped because the transport could not keep up. A late note is
 * worse than a lost one, so the clock never blocks - but the count must be
 * visible or the loss is silent. */
uint32_t seq_dropped(void);
