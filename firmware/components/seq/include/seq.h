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
/* TWENTY, because a name is an address now: 'crash:12:vel' is a base of up to
 * eight letters, an instance and a part - lane_name.h's LANE_NAME_MAX, which
 * builtins.c asserts fits. It was twelve when a name was a word and a digit. */
#define SEQ_NAME_MAX  20

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
/* The parts of a sound lane (seq_lane_t.param on SEQ_BIND_NOTE). */
#define SEQ_PART_NONE 0
#define SEQ_PART_VEL  1      /* how hard: 'x' is 100, a digit as on a drum     */
#define SEQ_PART_OCT  2      /* which octave, 0-8: 'x' is the name's own       */

typedef enum {
    SEQ_BIND_NOTE = 0,   /* a MIDI note: drums and the melodic voices */
    SEQ_BIND_CC,         /* a controller number: digits are values    */
    SEQ_BIND_VIZ,        /* a drawing primitive: digits are amounts   */
} seq_bind_t;

/* ONE EVENT OF A COMPILED LANE: something happens on a slot of the cycle.
 *
 * This replaced four bitmasks and three per-slot tables (accent, ghost, chance,
 * odds, degree, the step character) when a step stopped being one character -
 * docs/MANIFESTO.md §3.6. A chord is several events on one slot; a tie is
 * `hold`; alternation is the cycle class (per, ph) rather than extra slots, so
 * '<a b>' no longer doubles a lane's length. See seq_pattern.h for how the text
 * becomes these. */
typedef struct {
    uint8_t slot;           /* within one cycle                               */
    uint8_t val;            /* 0-9, or SEQ_VAL_X for 'x' - the lane's level   */
    uint8_t hold;           /* extra slots a tie keeps it sounding            */
    uint8_t prob;           /* 0-100, or SEQ_PROB_ALWAYS                      */
    uint8_t per, ph;        /* plays on cycles where cycle % per == ph        */
    char    dir;            /* 'u' 'd' 'l' 'r' written as the step, or 0      */
    uint8_t spare;
} seq_ev_t;

/* NINETY-SIX EVENTS A LANE. A full sixty-four slot bar is 64; a sixteen-step
 * pad of three-note chords is 48; a four-way alternation of sixteen steps is 64.
 * More than that is refused with the number, like every other limit here. */
#define SEQ_MAX_EVENTS 96
/* The pattern as typed, so '>lanes' prints what the player wrote. With a step
 * that is more than one character, rebuilding the text from the compiled form
 * would print something they did not type - the thing docs/COMMANDS.md forbids. */
#define SEQ_TEXT_MAX   100

typedef struct {
    char     name[SEQ_NAME_MAX];
    /* THE COMPILED LANE. Events sorted by slot, and where each slot's run of
     * them starts, so the clock looks at exactly the events of the slot it is
     * on and nothing else. */
    seq_ev_t ev[SEQ_MAX_EVENTS];
    uint8_t  first[SEQ_MAX_STEPS + 1];
    uint8_t  nev;
    uint8_t  slots;         /* per cycle; 0 means not in play                 */
    uint8_t  div;           /* slots per step                                 */
    uint8_t  steps;         /* top-level steps: its length in sixteenths      */
    /* THIS LANE'S OWN TIME BASE: '*rnum' and '/rden'.
     *
     * Every lane keeping its own clock is what makes polyrhythm, half-time and
     * ratchets the same mechanism rather than three features - and it is why
     * the rate lives on the lane and not on the transport. The slot a tick falls
     * on is computed exactly from these by seq_pattern_slot_at(); it used to be
     * an integer ticks-per-slot, which a five-way split cannot be. */
    uint8_t  rnum, rden;
    char     text[SEQ_TEXT_MAX];
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
    /* WHICH PART OF IT. Zero means the lane itself. On a picture anything else
     * is a parameter of the primitive and the value goes there instead -
     * '>disc:x 0..9..' is a lane whose events are positions - and seq does not
     * know what it means, exactly as it does not know what a primitive is.
     *
     * On a SOUND lane a part is SEQ_PART_VEL or SEQ_PART_OCT, and that one seq
     * does act on: '>bass:vel 9..3' sets the level of the lane called 'bass' and
     * '>bass:oct <2 3>' its octave, each until the next event. A parameter is a
     * lane, for sound as it already was for pictures. */
    uint8_t  param;
    int8_t   oct0;          /* the octave the NAME defines, which 'x' restores */
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
    /* HOW MANY ROUTE HOPS from a lane that follows nothing. The clock fires
     * lower ranks first, so a routed lane hears its source on the same tick
     * whatever order the lines were typed in. */
    uint8_t  rank;

    /* ---- a count: play n passes, then stop (docs/NEXT.md §4) ---- */
    uint8_t  count;         /* '!n', 0 = for ever                              */
    bool     idle;          /* counted, and not started: waits for its own
                             * downbeat, or - routed - for its source           */
    bool     done;          /* finished; muted by that, and '>play' re-arms it */
    uint32_t origin;        /* the global slot its first pass began on         */
} seq_lane_t;

esp_err_t seq_init(void);

/* Compile one lane. The grammar is seq_pattern.h's, in one place:
 *
 *   x      a hit at the lane's own level
 *   0-9    a hit with an amount - velocity on a drum, degree on a voice, value
 *          on a controller, how much on a picture. ONE meaning per binding,
 *          where a digit on a drum used to be compiled and never read.
 *   .      rest       _   tie: the note before keeps sounding
 *   [ab]   subdivide  [0,4,7]  a chord   <ab>  one per cycle   x%15  odds
 *   /2 *2  rate, !4 play four cycles then stop - at the end, after a space
 *
 * It used to be "'.', '-' and '_' are rests and anything else is a hit, so
 * nobody has to remember which character is correct" - a kindness that also
 * made every typo a note and every unclosed bracket a septuplet. A pattern that
 * is not the grammar is REFUSED now: ESP_ERR_INVALID_ARG, with the reason and
 * the character from seq_lane_error(). Nothing is changed when it is refused,
 * so a typo mid-performance leaves the lane playing what it played before.
 *
 * `steps` is kept verbatim for '>lanes'. ESP_ERR_NO_MEM means no lane is free. */
esp_err_t seq_lane(const char *name, const char *steps);

/* Why the last seq_lane() refused its pattern, in at most thirty columns, and
 * the offset of the character it is about (-1 if none). */
const char *seq_lane_error(int *at);

/* Which slot of a lane is sounding now, and in which of its cycles - for the
 * playhead. The lane's own clock, not the global sixteenth: a '/2' lane moves at
 * half speed and a nested one at its subdivision. Returns false when stopped. */
bool seq_lane_now(const seq_lane_t *l, int *slot, uint32_t *cycle);

/* WHERE A COUNTED LANE IS: its pass, 0-based, while it plays; SEQ_PASS_WAITS
 * before it starts; SEQ_PASS_DONE when it has finished. -3 for a lane with no
 * count. For '>lanes', which says so rather than leaving a silent lane to be
 * guessed at. */
#define SEQ_PASS_WAITS (-1)
#define SEQ_PASS_DONE  (-2)
#define SEQ_PASS_NONE  (-3)
int seq_lane_pass(const seq_lane_t *l);

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

/* WHERE A LANE GOES, all of it at once.
 *
 *   SEQ_BIND_NOTE, melodic false   a fixed pitch - a drum. `note`, `chan`.
 *   SEQ_BIND_NOTE, melodic true    scale degrees. `octave` in the convention
 *                                  where middle C is C4 = 60; `chan`, `gate_ms`.
 *   SEQ_BIND_CC                    a controller: 0 is 0 and 9 is 127. `cc`.
 *   SEQ_BIND_VIZ                   a picture. `prim` is an index into viz's own
 *                                  table and `param` a part of it; seq never
 *                                  learns what either means.
 *
 * ONE CALL, NOT FOUR. There were four - note, melodic, controller, picture -
 * and each set only its own fields. That was harmless while a name's binding
 * was compiled in and could never change; once a name is DEFINED
 * (lane_name.h), '>kick = voice 2' turns a drum into a voice, and a lane that
 * kept `melodic` from one binding and `ctrl` from another would play something
 * nobody asked for. This sets every field the binding owns and clears the rest. */
typedef struct {
    seq_bind_t bind;
    bool       melodic;
    uint8_t    note;        /* NOTE, fixed pitch                */
    int8_t     octave;      /* NOTE, melodic                    */
    uint8_t    chan;        /* 0-15                             */
    uint16_t   gate_ms;     /* NOTE                             */
    uint8_t    cc;          /* CC                               */
    uint8_t    prim;        /* VIZ                              */
    uint8_t    param;       /* 0, a part of the picture, or SEQ_PART_* */
} seq_binding_t;

esp_err_t seq_lane_bind(const char *name, const seq_binding_t *b);

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
/* The global sixteenth counter since play. UNMASKED: any mask that is not a
 * multiple of every possible lane length introduces a phase jump when it rolls.
 * NOT a lane's own position - a '/2' or nested lane moves at its own speed, and
 * the playhead used to take this modulo the lane's length and run at the wrong
 * one. Use seq_lane_now() for that. */
uint32_t seq_position(void);

/* ---- sharing time with another deck ------------------------------------- *
 *
 * A SHARED CLOCK IS NOT A SHARED TICK. Sending every pulse over a radio would
 * inherit the radio's jitter directly - several milliseconds, which is audible.
 * So each deck runs its own timer and is told, a few times a second, where the
 * ensemble thinks it should be; it then corrects SLOWLY by trimming its own
 * period rather than jumping. A jump is a glitch; a trim is a drift nobody hears.
 *
 * This is the same model Ableton Link uses, which is deliberate: if Link is ever
 * licensed and ported, it replaces the transport under these two functions and
 * nothing above them changes.
 */

/* Where this deck is: the absolute pulse since play, and when that pulse was
 * due, so a receiver can work out the offset without guessing the flight time. */
void seq_timebase(uint32_t *tick, int64_t *tick_due_us, int *bpm);

/* The ensemble says this deck should be at `tick` at `due_us`. Nudges the local
 * clock toward it. Ignored when the clock is not running - a follower that is
 * stopped stays stopped, because starting a deck is a decision the player makes.
 *
 * Returns the phase error in microseconds BEFORE the correction, which is what a
 * player wants to see to know whether the ensemble is together. */
int32_t seq_nudge(uint32_t tick, int64_t due_us, int bpm);

/* Slide the grid by a known amount, and follow a tempo. Used when the caller has
 * already measured the error itself and filtered it - which it must, because the
 * raw per-packet error is biased late by transport delay and correcting on every
 * sample steers the clock into that bias. */
void seq_nudge_by(int32_t err_us, int bpm);

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
