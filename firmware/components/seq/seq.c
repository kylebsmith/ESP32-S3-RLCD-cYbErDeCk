#include "seq.h"
#include "seq_pattern.h"

#include <ctype.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "seq";

static seq_tick_hook_t s_on_tick;
static seq_play_hook_t s_on_play;
static seq_draw_hook_t s_on_draw;
static seq_param_hook_t s_on_param;

static seq_lane_t s_lanes[SEQ_MAX_LANES];
static int        s_bpm = 120;
static bool       s_running;
/* volatile: written by the clock on the esp_timer task, read by the editor
 * task to place the playhead. Aligned 32-bit does not tear on Xtensa, so this
 * is about the compiler not hoisting the read out of the draw loop. */
static volatile uint32_t s_pos;
static uint32_t   s_tick;            /* 96 PPQN pulses since play */

_Static_assert(SEQ_MAX_STEPS == SEQ_PATTERN_MAX_SLOTS,
               "a lane holds exactly the slots the compiler can produce");
_Static_assert(SEQ_TICKS_PER_STEP == SEQ_PATTERN_TICKS_PER_STEP,
               "the compiler and the clock agree on the sixteenth");
static int        s_swing = 50;      /* per cent; 50 is straight   */
static bool       s_sync;            /* send MIDI clock            */
static esp_timer_handle_t s_clock;
typedef struct {
    char        name[12];
    const char *help;
    seq_sink_t  fn;
    seq_flush_t flush;
    bool        on;
} dest_t;
static dest_t s_dests[SEQ_MAX_DESTS];
static int    s_ndests;

/* The clock callback must not call the transport directly.
 *
 * esp_timer dispatches on its own task with a 3.5 KB stack, and NimBLE's
 * notify path is not a 3.5 KB guest. Worse, that task is stalled by every
 * flash write - so an autosave would delay the clock, which is exactly the
 * class of coupling docs/OS.md's two-core split exists to forbid.
 *
 * So the clock only enqueues. A dedicated task with a real stack drains the
 * queue and talks to the transport, and a full queue drops the oldest event
 * rather than blocking the clock: a late note is worse than a lost one. */
typedef struct {
    uint8_t     status, d1, d2;
    uint32_t    queued_us;   /* for the transport-latency statistic */
    const char *lane;        /* into s_lanes[].name - static, never freed */
} midi_ev_t;

/* The ideal grid. Re-anchored on play and on any tempo change, so the
 * statistic measures dispatch jitter and never accumulated tempo error -
 * those are different problems with different fixes. */
static int64_t    s_grid_t0;

/* Ticks skipped by the spread statistic after the grid is anchored. Forty
 * milliseconds at any tempo this device runs - long enough for one autosave to
 * finish, short enough that nothing about the performance is unmeasured. */
#define SEQ_SETTLE_TICKS 8
static int        s_settle;
static seq_stat_t s_clock_stat, s_xport_stat;

static void stat_add(seq_stat_t *s, int32_t v)
{
    if (s->n == 0) {
        s->min = s->max = v;
    } else {
        if (v < s->min) { s->min = v; }
        if (v > s->max) {
            s->max = v;
            s->worst_ms = (uint32_t)(esp_timer_get_time() / 1000);
        }
    }
    /* Relative to a FIXED baseline - the first sample - not to the running
     * minimum. Bucketing against a running minimum means a late-arriving new
     * minimum retroactively invalidates every bucket counted before it, so
     * the histogram describes a distribution that was never measured. The
     * standard deviation and the spread are unaffected (both are computed
     * from absolute sums and are shift-invariant); only the histogram and the
     * late count were wrong, and those are the two numbers anyone would
     * quote. */
    if (!s->based) {
        s->base  = v;
        s->based = true;
    }
    const int32_t rel = v - s->base;
    if (rel > SEQ_LATE_US) {
        s->late++;
    }
    static const int32_t edge[SEQ_NBUCKETS - 1] = { 100, 250, 500, 1000, 2000, 5000 };
    int b = SEQ_NBUCKETS - 1;
    for (int i = 0; i < SEQ_NBUCKETS - 1; i++) {
        if (rel < edge[i]) { b = i; break; }
    }
    s->bucket[b]++;
    s->n++;
    s->sum   += v;
    s->sumsq += (int64_t)v * (int64_t)v;
}

void seq_stats(seq_stat_t *c, seq_stat_t *x)
{
    if (c != NULL) { *c = s_clock_stat; }
    if (x != NULL) { *x = s_xport_stat; }
}

void seq_set_draw_hook(seq_draw_hook_t fn) { s_on_draw = fn; }
void seq_set_param_hook(seq_param_hook_t fn) { s_on_param = fn; }

void seq_set_hooks(seq_tick_hook_t t, seq_play_hook_t p)
{
    s_on_tick = t;
    s_on_play = p;
}

void seq_stats_reset(void)
{
    memset(&s_clock_stat, 0, sizeof s_clock_stat);
    memset(&s_xport_stat, 0, sizeof s_xport_stat);
}
/* How many events one drain may take before flushing. The queue is 64 deep
 * and a step on eight lanes is at most sixteen messages with note-offs, so
 * this is a backstop against a pathological burst holding the flush open,
 * not a limit anything normal reaches. */
#define SEQ_BURST_MAX 32
static QueueHandle_t s_midiq;
static uint32_t      s_dropped;

static uint64_t period_us(void);

static void midi_task(void *arg)
{
    (void)arg;
    midi_ev_t ev;
    uint32_t said = 0;
    while (1) {
        if (xQueueReceive(s_midiq, &ev, portMAX_DELAY) == pdTRUE) {
            /* DRAIN THE WHOLE STEP BEFORE FLUSHING.
             *
             * The clock queues every lane's note for a step in one pass, so
             * by the time this task runs they are all sitting here. Handing
             * them over together lets a transport that batches send them in
             * one go - which on BLE is the difference between a kick, a hat
             * and a bass note arriving in one connection event or spread
             * across three. The loop is bounded by the queue depth and never
             * waits, so it cannot delay anything. */
            int burst = 0;
            do {
                /* How long this event sat in the queue. Measured HERE, where
                 * the transport is actually about to be called, so it
                 * includes the task switch the queue costs. */
                stat_add(&s_xport_stat,
                         (int32_t)((uint32_t)esp_timer_get_time() - ev.queued_us));
                for (int i = 0; i < s_ndests; i++) {
                    if (s_dests[i].on && s_dests[i].fn != NULL) {
                        s_dests[i].fn(ev.lane ? ev.lane : "", ev.status,
                                      ev.d1, ev.d2, ev.queued_us);
                    }
                }
                burst++;
            } while (burst < SEQ_BURST_MAX &&
                     xQueueReceive(s_midiq, &ev, 0) == pdTRUE);

            for (int i = 0; i < s_ndests; i++) {
                if (s_dests[i].on && s_dests[i].flush != NULL) {
                    s_dests[i].flush();
                }
            }
            /* Report the loss from HERE, never from emit(): emit() runs on
             * the clock, and a log call on the clock to announce that the
             * clock is overloaded makes it worse. */
            if (s_dropped != said) {
                said = s_dropped;
                ESP_LOGW(TAG, "%u events dropped - transport behind",
                         (unsigned)said);
            }
        }
    }
}

uint32_t seq_dropped(void) { return s_dropped; }

/* THE KEY, AND WHY A SCALE IS A TABLE AND NOT A PARSER.
 *
 * A degree is resolved to a note by indexing this table and adding. That is
 * the entire pitch system: no note names in the realtime path, no string
 * anywhere near the clock, and a wrong note is not expressible. The modes are
 * the seven diatonic ones plus the three a performer actually reaches for
 * under pressure - minor pentatonic, which cannot sound wrong; blues, which
 * is pentatonic plus the flat five; and chromatic, for when the whole point
 * is to leave the key.
 *
 * Order matters: the longest names must be tested first or "maj" swallows
 * "maj5" and "b" swallows "blues". That is a real bug this table's layout is
 * chosen to prevent rather than a comment about one. */
typedef struct { const char *name; uint8_t n; uint8_t iv[12]; } mode_t;
static const mode_t s_modes[] = {
    { "chrom", 12, {0,1,2,3,4,5,6,7,8,9,10,11} },
    { "blues",  6, {0,3,5,6,7,10} },
    { "pent",   5, {0,3,5,7,10} },       /* minor pentatonic - the safe one */
    { "maj5",   5, {0,2,4,7,9} },        /* major pentatonic                */
    { "maj",    7, {0,2,4,5,7,9,11} },
    { "min",    7, {0,2,3,5,7,8,10} },
    { "dor",    7, {0,2,3,5,7,9,10} },
    { "phr",    7, {0,1,3,5,7,8,10} },
    { "lyd",    7, {0,2,4,6,7,9,11} },
    { "mix",    7, {0,2,4,5,7,9,10} },
    { "loc",    7, {0,1,3,5,6,8,10} },
};

static uint8_t     s_root = 0;                  /* pitch class, C = 0 */
static const mode_t *s_mode = &s_modes[5];      /* min */
static char        s_scale_name[12] = "cmin";

const char *seq_scale_name(void) { return s_scale_name; }

esp_err_t seq_scale(const char *spec)
{
    if (spec == NULL || spec[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    const char *p = spec;
    int pc;
    switch (*p | 0x20) {          /* tolerate either case; nobody should care */
    case 'c': pc = 0;  break;
    case 'd': pc = 2;  break;
    case 'e': pc = 4;  break;
    case 'f': pc = 5;  break;
    case 'g': pc = 7;  break;
    case 'a': pc = 9;  break;
    case 'b': pc = 11; break;
    default: return ESP_ERR_INVALID_ARG;
    }
    p++;
    if (*p == '#') { pc = (pc + 1) % 12; p++; }
    else if (*p == 'b') { pc = (pc + 11) % 12; p++; }

    const mode_t *m = &s_modes[5];              /* a bare root means minor */
    if (*p != '\0') {
        m = NULL;
        for (size_t i = 0; i < sizeof s_modes / sizeof s_modes[0]; i++) {
            if (strncmp(p, s_modes[i].name, strlen(s_modes[i].name)) == 0) {
                m = &s_modes[i];
                break;
            }
        }
        if (m == NULL) {
            return ESP_ERR_INVALID_ARG;
        }
    }
    s_root = (uint8_t)pc;
    s_mode = m;
    snprintf(s_scale_name, sizeof s_scale_name, "%s", spec);
    return ESP_OK;
}

/* Degree to MIDI note. Degrees past the top of the scale keep climbing into
 * the next octave, so "0123456789" is a run and not a wrap - which is what
 * anyone typing it expects, and the reason degrees go to 9 rather than to the
 * size of the mode. */
static uint8_t degree_note(uint8_t deg, int octave)
{
    const int n = s_mode->n;
    const int up = deg / n;
    const int idx = deg % n;
    int note = 12 * (octave + 1 + up) + s_root + s_mode->iv[idx];
    if (note < 0)   { note = 0; }
    if (note > 127) { note = 127; }
    return (uint8_t)note;
}

/* Note-offs are scheduled rather than sent with the note, so a lane can never
 * leave a note hanging: there is no "on" a performer could forget to pair.
 * docs/OS.md's own warning about stuck notes is that midi.on/midi.off as a
 * PAIR manufactures the problem that panic then exists to clean up. */
typedef struct {
    int64_t     due_us;
    const char *lane;       /* who scheduled it, for whatever reads the event */
    uint8_t     status, d1;
    bool        armed;
} pending_off_t;
static pending_off_t s_offs[SEQ_MAX_LANES * 4];

static const char *s_emitting;   /* the lane fire_lanes is currently serving */

static void emit(uint8_t status, uint8_t d1, uint8_t d2)
{
    if (s_midiq == NULL) {
        return;                      /* before seq_init; nowhere to put it */
    }
    const midi_ev_t ev = { status, d1, d2, (uint32_t)esp_timer_get_time(),
                           s_emitting };
    if (xQueueSend(s_midiq, &ev, 0) != pdTRUE) {
        /* Make room by dropping the OLDEST NON-CLOCK event.
         *
         * The old code dropped whatever was at the head, which could be a
         * 0xF8. A lost timing clock makes a DAW's tempo follower hunt - the
         * owner reported Ableton going "wonky" with sync on - and a late note
         * is a far smaller crime than a tempo that wobbles. Clock is the one
         * message whose VALUE is its regularity. */
        midi_ev_t drop;
        int rescued = 0;
        midi_ev_t keep[4];
        while (xQueueReceive(s_midiq, &drop, 0) == pdTRUE) {
            if (drop.status < 0xF8 || rescued >= (int)(sizeof keep / sizeof keep[0])) {
                break;                      /* this one is expendable */
            }
            keep[rescued++] = drop;         /* a clock: put it back after */
        }
        for (int i = 0; i < rescued; i++) {
            (void)xQueueSend(s_midiq, &keep[i], 0);
        }
        (void)xQueueSend(s_midiq, &ev, 0);
        s_dropped++;
    }
}

static void schedule_off(uint8_t chan, uint8_t note, uint32_t us)
{
    const int64_t due = esp_timer_get_time() + (int64_t)us;
    for (size_t i = 0; i < sizeof s_offs / sizeof s_offs[0]; i++) {
        if (!s_offs[i].armed) {
            s_offs[i].armed  = true;
            s_offs[i].due_us = due;
            s_offs[i].lane   = s_emitting;
            s_offs[i].status = (uint8_t)(0x80 | (chan & 0x0F));
            s_offs[i].d1     = note;
            return;
        }
    }
    /* Table full: send the off immediately rather than lose it. A dropped
     * note-off is a stuck note, which is the one failure an audience hears. */
    emit((uint8_t)(0x80 | (chan & 0x0F)), note, 0);
}

static void service_offs(int64_t now)
{
    for (size_t i = 0; i < sizeof s_offs / sizeof s_offs[0]; i++) {
        if (s_offs[i].armed && now >= s_offs[i].due_us) {
            s_offs[i].armed = false;
            /* An off belongs to the lane that played the note, not to whichever
             * lane the clock served last - which is who it was labelled with. */
            s_emitting = s_offs[i].lane;
            emit(s_offs[i].status, s_offs[i].d1, 0);
        }
    }
    s_emitting = NULL;
}

/* Swing, in ticks. A sixteenth is six ticks, so an eighth is twelve; a
 * shuffle puts the offbeat at `swing` per cent of the way through that
 * eighth instead of at the halfway point. 67 per cent lands on 8 of 12,
 * which is two ticks late - triplet swing, exactly.
 *
 * Only ODD sixteenths move. The downbeat staying put is the whole difference
 * between a groove and a tempo change. */
static int swing_ticks(void)
{
    int d = (s_swing * 2 * SEQ_TICKS_PER_STEP) / 100 - SEQ_TICKS_PER_STEP;
    if (d < 0) { d = 0; }
    if (d > SEQ_TICKS_PER_STEP - 2) { d = SEQ_TICKS_PER_STEP - 2; }
    return d;
}

/* A PRNG, not esp_random().
 *
 * This is called from the clock. esp_random() reads a hardware register that
 * is documented as requiring the RF subsystem to be active for full entropy,
 * and it is not somewhere to take a dependency from the realtime path. A step
 * that plays half the time does not need cryptographic randomness; it needs to
 * be cheap, bounded, and never blocking. xorshift32, seeded once.
 *
 * It is also a FEATURE that this is deterministic from the seed: a pattern
 * that sounds right is reproducible within a session, which is the difference
 * between a performance decision and a coin toss. */
static uint32_t s_rng = 0x9E3779B9u;

static inline uint32_t rng_next(void)
{
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return s_rng;
}

/* A LANE PLAYED. Tell anything routed from it.
 *
 * This is the whole of routing, and it is four lines because there is one lane
 * table. Before the collapse the music half published nothing and the visual
 * half kept its own copy of this loop, so a kick could drive a circle and a
 * circle could drive nothing. */
static void published(seq_lane_t *src, uint8_t value)
{
    src->last_val = (uint8_t)((value * 9 + 63) / 127);
    for (int j = 0; j < SEQ_MAX_LANES; j++) {
        seq_lane_t *d = &s_lanes[j];
        if (d->route[0] != '\0' && strcmp(d->route, src->name) == 0) {
            d->trig = true;
            d->trig_val = src->last_val;
        }
    }
    if (s_on_play != NULL) {
        s_on_play(src->name, value);
    }
}

/* A DIGIT ON A DRUM IS HOW HARD IT IS HIT: 9 is 127 and 1 is 14. It used to be
 * compiled and never read - '>kick 0...9...' was two identical hits - while
 * loudness was a three-value enum spelled 'X x ,'. One meaning per binding now
 * (docs/MANIFESTO.md §3.7).
 *
 * 0 is the quietest hit there is, not silence: a rest is '.', and a digit is
 * always an event, on every binding. MIDI would read velocity 0 as a note-off,
 * so it is 1. 'x' is the lane's own level - 100 unless something set it. */
static int vel_of(const seq_lane_t *l, uint8_t val)
{
    if (val == SEQ_VAL_X) {
        return l->vel;
    }
    const int v = (val * 127 + 4) / 9;
    return v < 1 ? 1 : v;
}

/* How long a note sounds: the voice's own gate, plus every slot a tie holds it.
 * A bass is 180 ms whether or not it is tied, and '0__' is that plus two
 * sixteenths - the tie ADDS the steps it spans rather than replacing the
 * voice's character with a different one. */
static uint32_t gate_us(const seq_lane_t *l, uint8_t hold)
{
    uint64_t us = (uint64_t)l->gate_ms * 1000u;
    if (hold > 0 && l->div > 0 && l->rnum > 0) {
        us += (uint64_t)hold * period_us() * SEQ_TICKS_PER_STEP * l->rden /
              ((uint64_t)l->div * l->rnum);
    }
    return us > 60000000u ? 60000000u : (uint32_t)us;
}

/* ONE EVENT, to wherever the lane is bound. `routed` means the source's value
 * decides how much, which is what routing has always meant. */
static void fire_event(seq_lane_t *l, const seq_ev_t *e, bool routed,
                       uint32_t tick)
{
    /* A DRAWING LANE. The value is an amount 0-9 and the destination is a
     * primitive; nothing about MIDI applies. Marked rather than drawn,
     * because generating a frame is a pass over the whole picture and this
     * is an esp_timer callback - docs/OS.md forbids acting here. The main
     * loop replays the marks in primitive order. */
    if (l->bind == SEQ_BIND_VIZ) {
        int amt = routed ? (int)l->trig_val
                         : (e->val == SEQ_VAL_X ? 9 : (int)e->val);
        if (amt < 0) { amt = 0; }
        if (amt > 9) { amt = 9; }
        const char dir = e->dir ? e->dir : l->dir;
        /* A PARAMETER LANE CARRIES A VALUE, NOT A SHAPE. It does not draw;
         * it says where the shape will. Same events, same clock, same
         * grammar - a different part of the destination. */
        if (l->param != 0) {
            if (s_on_param != NULL) {
                s_on_param((int)l->prim, (int)l->param, amt);
            }
        } else if (s_on_draw != NULL) {
            s_on_draw((int)l->prim, amt, dir, tick);
        }
        published(l, (uint8_t)(amt * 127 / 9));
        return;
    }
    if (l->ctrl) {
        /* A step with no digit HOLDS: nothing is sent. A controller that
         * snaps to zero between steps is a stutter, not a sweep, and holding
         * is also one fewer message on the wire. */
        if (e->val == SEQ_VAL_X) {
            return;
        }
        const uint8_t v = (uint8_t)((e->val * 127) / 9);
        emit((uint8_t)(0xB0 | (l->chan & 0x0F)), l->cc, v);
        published(l, v);
        return;
    }
    /* On a melodic lane an 'x' is the ROOT, not MIDI note 0 - which would emit
     * C-1 at the bottom of the range, inaudible on most synths and a thump on
     * a few. On a voice the digit is the degree, so its velocity is the lane's
     * own; a per-step velocity for a voice is a parameter lane, the way a
     * circle's position is. */
    const uint8_t note = l->melodic
        ? degree_note(e->val == SEQ_VAL_X ? 0 : e->val, l->octave)
        : l->note;
    int vel = l->melodic ? l->vel : vel_of(l, e->val);
    if (vel < 1)   { vel = 1; }
    if (vel > 127) { vel = 127; }
    emit((uint8_t)(0x90 | (l->chan & 0x0F)), note, (uint8_t)vel);
    published(l, (uint8_t)vel);
    schedule_off(l->chan, note, gate_us(l, e->hold));
}

static void fire_lanes(uint32_t tick)
{
    s_emitting = NULL;
    const int sw = swing_ticks();
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        /* NOT const: a routed lane clears its own trigger here. */
        seq_lane_t *l = &s_lanes[i];
        if (!l->used || l->muted || l->slots == 0 || l->nev == 0) {
            continue;
        }
        if (l->route[0] != '\0') {
            /* ROUTED: the source decides both when and how much, and this
             * lane's own steps are not consulted. A routed lane that also had
             * to agree with its own pattern fired only where the two happened
             * to coincide, which is most of the way to never. */
            if (!l->trig) {
                continue;
            }
            l->trig = false;
            s_emitting = l->name;
            fire_event(l, &l->ev[0], true, tick);
            continue;
        }
        /* Every lane is asked whether THIS tick starts one of its slots, from
         * its own subdivision and rate, exactly - see seq_pattern_slot_at. */
        int s = 0;
        uint32_t cy = 0;
        if (!seq_pattern_slot_at(tick, l->slots, l->div, l->rnum, l->rden, sw,
                                 &s, &cy)) {
            continue;
        }
        s_emitting = l->name;
        /* ONE ROLL PER SLOT. Everything that starts together shares it, so a
         * chord with odds plays whole or not at all - Strudel's randomness is a
         * function of time, which gives the same result - and '[0%30,4%60]'
         * plays the 4 whenever it plays the 0, because one number is compared
         * against both. Deterministic from the seed, cheap, never blocking. */
        int roll = -1;
        for (int k = l->first[s]; k < l->first[s + 1]; k++) {
            const seq_ev_t *e = &l->ev[k];
            if (e->per > 1 && (cy % e->per) != e->ph) {
                continue;               /* not this alternative's cycle */
            }
            if (e->prob != SEQ_PROB_ALWAYS) {
                if (roll < 0) {
                    roll = (int)(rng_next() % 100u);
                }
                if (roll >= e->prob) {
                    continue;
                }
            }
            fire_event(l, e, false, tick);
        }
    }
}

/* A COMPILED LANE IS HANDED TO THE CLOCK, NOT WRITTEN UNDER IT.
 *
 * The clock runs on CPU1 and the editor on CPU0, so the two genuinely overlap.
 * The old bitmasks were rewritten in place while the timer read them, and a
 * torn read cost at most one wrong step. An event list read through an index
 * cannot be torn safely, so the command layer compiles into this stage and the
 * clock copies it in at the top of a tick - the clock is then the only writer of
 * everything it reads. One stage, because one command runs at a time. */
static struct {
    seq_ev_t ev[SEQ_MAX_EVENTS];
    uint8_t  first[SEQ_MAX_STEPS + 1];
    uint8_t  nev, slots, div, steps, rnum, rden;
} s_stage;
static volatile int s_stage_for = -1;    /* the lane waiting for it, or -1 */

static void stage_apply(void)
{
    const int i = s_stage_for;
    if (i < 0 || i >= SEQ_MAX_LANES) {
        return;
    }
    __sync_synchronize();
    seq_lane_t *l = &s_lanes[i];
    memcpy(l->ev, s_stage.ev, sizeof l->ev);
    memcpy(l->first, s_stage.first, sizeof l->first);
    l->nev   = s_stage.nev;
    l->div   = s_stage.div;
    l->steps = s_stage.steps;
    l->rnum  = s_stage.rnum;
    l->rden  = s_stage.rden;
    l->slots = s_stage.slots;
    __sync_synchronize();
    s_stage_for = -1;
}

/* The clock. A hardware timer, never a task delay - docs/OS.md: "Never
 * sequence from a task delay. Use a hardware timer."
 *
 * It ticks at 96 PPQN, a multiple of the 24 MIDI clock is defined at, so sync
 * costs one message on every fourth tick that already exists. At 120 bpm a tick
 * is 5,208 us and a sixteenth is twenty-four of them, 125,000 us exactly. */
static void tick(void *arg)
{
    (void)arg;
    const int64_t now = esp_timer_get_time();
    service_offs(now);
    /* A lane compiled since the last tick takes effect HERE, stopped or not, so
     * a lane written before '>play' is in place when play starts. `now` is
     * already taken, so the copy cannot show up in the jitter statistic. */
    stage_apply();

    if (!s_running) {
        return;
    }

    /* Dispatch deviation from the ideal grid. This is the number the owner
     * is hearing when they say it feels jittery, and it is measured before
     * any note is emitted so the measurement cannot be blamed on the notes. */
    if (s_grid_t0 == 0) {
        /* Anchor on the first tick after play, not on the press. The timer is
         * free-running, so the gap between the two is an arbitrary constant
         * phase - real, but not jitter, and reporting it as jitter buries the
         * signal under a 6 ms offset. */
        s_grid_t0 = now - (int64_t)s_tick * (int64_t)period_us();
        s_settle  = SEQ_SETTLE_TICKS;
    }
    if (s_settle > 0) {
        /* THE FIRST FEW TICKS AFTER PLAY ARE NOT MEASURED, and the reason is
         * worth stating rather than hiding: '>play' is usually the last line of
         * a document, so the autosave lands right behind it, and a flash write
         * stalls the esp_timer task. That produced a single 11 ms sample at
         * t=0 which set 'worst' and doubled 'sd' for the rest of the session -
         * a statistic dominated by one event at the moment of pressing play,
         * describing nothing about how the clock then runs.
         *
         * It is excluded, not concealed: 'late' still counts every tick from
         * the first, so a real stall is still visible as a dropped or late
         * event. What is thrown away is only its contribution to the spread. */
        s_settle--;
    } else {
        const int64_t ideal = s_grid_t0 + (int64_t)s_tick * (int64_t)period_us();
        int64_t d = now - ideal;
        if (d >  1000000) { d =  1000000; }
        if (d < -1000000) { d = -1000000; }
        stat_add(&s_clock_stat, (int32_t)d);
    }
    if (s_sync && (s_tick % SEQ_CLOCK_EVERY) == 0) {
        emit(0xF8, 0, 0);            /* timing clock, no data bytes */
    }

    /* Every lane is asked whether THIS tick is one of its steps, so a lane at
     * '/2' or '*2' is not a special case anywhere - it simply divides the same
     * counter differently. That is what makes half-time, double-time and
     * polyrhythm one mechanism rather than three features.
     *
     * s_pos stays the sixteenth-note position, because that is what the
     * playhead and '>lanes' mean by "where we are". It is UNMASKED: it used to
     * be `& 0x7FFF`, and 32768 is a power of two, so 8- and 16-step lanes
     * wrapped cleanly while a 5-, 6- or 12-step lane took a phase jump every
     * 66 minutes.
     */
    fire_lanes(s_tick);
    /* The visuals advance on the same tick as the music, so the animation and
     * the beat share a clock by construction rather than by being kept in
     * step. Cheap: the hook returns immediately when nothing is drawing. */
    if (s_on_tick != NULL) {
        s_on_tick(s_tick);
    }
    if ((s_tick % SEQ_TICKS_PER_STEP) == 0) {
        s_pos = s_tick / SEQ_TICKS_PER_STEP;
        /* The step itself is an event. It travels through the same queue as
         * the notes, so a destination that cares about the bar gets it in the
         * same datagram as the notes of that step - and one that does not
         * care ignores it, exactly as the monitor ignores the lane name.
         * Status 0xF9 is undefined in MIDI, so no transport will act on it. */
        emit(0xF9, (uint8_t)(s_pos & 0x7F), 0);
    }
    s_tick++;
}

static uint64_t period_us(void)
{
    /* One internal tick. 60,000,000 / bpm / 96. At 124 bpm that is 5,040 us,
     * and a sixteenth is 24 of them - 120,967 us. */
    return (uint64_t)(60000000.0 / (double)s_bpm / (double)SEQ_PPQN);
}

void seq_timebase(uint32_t *tick, int64_t *tick_due_us, int *bpm)
{
    if (tick != NULL)        { *tick = s_tick; }
    if (bpm != NULL)         { *bpm  = s_bpm; }
    if (tick_due_us != NULL) {
        /* When the NEXT pulse is due on the ideal grid, not when the last one
         * happened to fire. The grid is the thing the two decks are agreeing
         * about; dispatch jitter is not. */
        *tick_due_us = s_grid_t0 + (int64_t)s_tick * (int64_t)period_us();
    }
}

/* THE TRIM, AND WHY IT IS NOT A JUMP.
 *
 * A follower that is a few milliseconds out could snap its tick counter and be
 * exactly right immediately - and every lane would skip or repeat a step, which
 * is the one thing a listener notices. Instead the ideal grid is slid by a
 * fraction of the error each time a packet arrives, so the deck converges over a
 * second or two and nothing is ever late or early by more than a fraction of a
 * pulse.
 *
 * An eighth of the error per packet, at eight packets a second, closes ninety per
 * cent of it in about two and a half seconds. Slower than that and a tempo change
 * takes too long to follow; faster and ordinary radio jitter starts steering the
 * clock. */
int32_t seq_nudge(uint32_t tick, int64_t due_us, int bpm)
{
    if (!s_running || s_grid_t0 == 0) {
        return 0;
    }
    if (bpm > 0 && bpm != s_bpm) {
        /* Tempo is followed outright: it is a decision somebody made, not an
         * error to converge on. seq_bpm re-anchors the grid, which is right -
         * the phase correction below then re-aligns it. */
        seq_bpm(bpm);
    }
    /* Where WE think that pulse was due, against where the ensemble says. */
    const int64_t ours = s_grid_t0 + (int64_t)tick * (int64_t)period_us();
    int64_t err = due_us - ours;

    /* A whole-pulse disagreement is not a phase error, it is a different bar -
     * which happens when a deck joins late. Fold the error into +/- half a pulse
     * so the deck slides to the nearest pulse boundary rather than trying to
     * travel a whole bar. */
    const int64_t per = (int64_t)period_us();
    if (per > 0) {
        while (err >  per / 2) { err -= per; }
        while (err < -per / 2) { err += per; }
    }
    s_grid_t0 += err / 8;
    return (int32_t)err;
}

void seq_nudge_by(int32_t err_us, int bpm)
{
    if (!s_running || s_grid_t0 == 0) {
        return;
    }
    if (bpm > 0 && bpm != s_bpm) {
        seq_bpm(bpm);
        return;              /* seq_bpm re-anchors; let the next one align it */
    }
    s_grid_t0 += err_us;
}

esp_err_t seq_init(void)
{
    memset(s_lanes, 0, sizeof s_lanes);
    memset(s_offs, 0, sizeof s_offs);

    s_midiq = xQueueCreate(64, sizeof(midi_ev_t));
    if (s_midiq == NULL) {
        return ESP_ERR_NO_MEM;
    }
    /* Priority above the editor, below the BLE host. 4 KB because NimBLE's
     * notify path is the deepest thing this task calls. */
    /* Pinned to core 1. NimBLE's host and controller are both on core 0
     * (CONFIG_BT_NIMBLE_PINNED_TO_CORE=0, CONFIG_BT_CTRL_PINNED_TO_CORE_0),
     * as is the editor, so core 1 is comparatively idle and the drain does
     * not queue behind the radio. Priority 6 stays above the editor and below
     * the BLE host: this task enqueues and blocks, so it hands the core
     * straight back. */
    if (xTaskCreatePinnedToCore(midi_task, "midi", 4096, NULL, 6, NULL, 1)
        != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    const esp_timer_create_args_t args = {
        .callback = tick,
        .name = "seq",
        .dispatch_method = ESP_TIMER_TASK,
    };
    const esp_err_t err = esp_timer_create(&args, &s_clock);
    if (err != ESP_OK) {
        return err;
    }
    /* The timer runs even when stopped, so scheduled note-offs still drain
     * after a stop and nothing is left sounding. */
    return esp_timer_start_periodic(s_clock, period_us());
}

static seq_lane_t *lane_find(const char *name, int len)
{
    if (name == NULL) {
        return NULL;
    }
    if (len < 0) {
        len = (int)strlen(name);
    }
    if (len <= 0 || len >= SEQ_NAME_MAX) {
        return NULL;
    }
    /* Iterate the whole array and test `used`. An empty pattern clears `used`
     * IN PLACE, so the array is sparse and a loop bounded by the lane COUNT
     * would stop at the first hole and miss every lane after it. */
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        if (s_lanes[i].used &&
            strncmp(s_lanes[i].name, name, (size_t)len) == 0 &&
            s_lanes[i].name[len] == '\0') {
            return &s_lanes[i];
        }
    }
    return NULL;
}

const seq_lane_t *seq_lane_find(const char *name, int len)
{
    return lane_find(name, len);
}

static seq_lane_t *find(const char *name, bool create)
{
    seq_lane_t *hit = lane_find(name, -1);
    if (hit != NULL) {
        return hit;
    }
    if (!create) {
        return NULL;
    }
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        if (!s_lanes[i].used) {
            seq_lane_t *l = &s_lanes[i];
            memset(l, 0, sizeof *l);
            l->used = true;
            snprintf(l->name, SEQ_NAME_MAX, "%s", name);
            l->chan = 9;             /* channel 10, where drums live */
            l->vel = 100;
            l->gate_ms = 40;
            return l;
        }
    }
    return NULL;
}

esp_err_t seq_forget(const char *name)
{
    seq_lane_t *l = find(name, false);
    if (l == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    /* Order matters: silence it before releasing it, so no step can fire out
     * of a slot that is being handed back.
     *
     * The name is left alone on purpose. A queued event holds `const char *`
     * into this very field, so zeroing it here would make an event already in
     * flight report an empty lane; find() memsets the slot when it hands it to
     * the next name, by which point the queue has long drained. */
    /* Gone means gone, including its route: keeping it meant turning a lane off
     * and on again brought the old routing back with it. */
    l->slots = 0;
    l->route[0] = '\0';
    l->trig = false;
    l->used = false;
    return ESP_OK;
}

void seq_forget_all(void)
{
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        s_lanes[i].slots = 0;
        s_lanes[i].route[0] = '\0';
        s_lanes[i].trig = false;
        s_lanes[i].used = false;
    }
}

/* Hand a compiled pattern to the clock and wait until it has taken it. The
 * clock takes it at the top of its next tick - at most one tick, five
 * milliseconds at 124 bpm and thirty at the slowest tempo - so this waits a
 * little over the slowest tick before deciding the clock is not there at all,
 * which is only true before seq_init(), and writing directly is then safe. */
static void stage_wait(void)
{
    for (int i = 0; i < 40 && s_stage_for >= 0; i++) {
        vTaskDelay(1);
    }
    if (s_stage_for >= 0) {
        stage_apply();
    }
}

static void stage(int lane, const seq_comp_t *c)
{
    stage_wait();
    /* Rests and ties are the playhead's business; the clock gets the hits,
     * sorted by slot, and where each slot's run of them begins. */
    int n = 0;
    for (int s = 0; s < c->slots; s++) {
        s_stage.first[s] = (uint8_t)n;
        for (int i = 0; i < c->n; i++) {
            const seq_leaf_t *L = &c->leaf[i];
            if (L->kind != SEQ_LEAF_HIT || L->slot != s) {
                continue;
            }
            seq_ev_t *e = &s_stage.ev[n++];
            e->slot  = L->slot;
            e->val   = L->val;
            e->hold  = (uint8_t)(L->len - L->width);
            e->prob  = L->prob;
            e->per   = L->per ? L->per : 1;
            e->ph    = L->ph;
            e->dir   = L->dir;
            e->spare = 0;
        }
    }
    for (int s = c->slots; s <= SEQ_MAX_STEPS; s++) {
        s_stage.first[s] = (uint8_t)n;
    }
    s_stage.nev   = (uint8_t)n;
    s_stage.slots = (uint8_t)c->slots;
    s_stage.div   = (uint8_t)c->div;
    s_stage.steps = (uint8_t)c->steps;
    s_stage.rnum  = (uint8_t)c->rnum;
    s_stage.rden  = (uint8_t)c->rden;
    __sync_synchronize();
    s_stage_for = lane;
    stage_wait();
}

/* One compile at a time - the command layer is the only caller - so the
 * compiler's working space is static rather than 1.8 KB on a task stack. */
static seq_comp_t s_comp;
static char       s_err[40];
static int        s_err_at = -1;

const char *seq_lane_error(int *at)
{
    if (at != NULL) {
        *at = s_err_at;
    }
    return s_err;
}

esp_err_t seq_lane(const char *name, const char *steps)
{
    /* COMPILE FIRST, AND TOUCH NOTHING UNTIL IT IS GOOD. A typo mid-performance
     * must leave the lane playing what it played, not silence it and not play
     * the typo. */
    const int err = seq_pattern_compile(steps, &s_comp);
    seq_lane_t *l = find(name, err != SEQ_PAT_EMPTY);
    if (err == SEQ_PAT_EMPTY) {
        /* an empty pattern removes the lane */
        if (l != NULL) {
            l->slots = 0;
            l->used = false;
        }
        return ESP_OK;
    }
    if (l == NULL) {
        return ESP_ERR_NO_MEM;
    }
    int hits = 0;
    for (int i = 0; i < s_comp.n; i++) {
        hits += (s_comp.leaf[i].kind == SEQ_LEAF_HIT);
    }
    const bool fresh = (l->slots == 0 && l->route[0] == '\0');
    if (err != SEQ_PAT_OK || hits > SEQ_MAX_EVENTS || s_comp.count != 0) {
        if (err != SEQ_PAT_OK) {
            seq_pattern_error_text(&s_comp, steps, s_err, sizeof s_err);
            s_err_at = s_comp.err_at;
        } else if (hits > SEQ_MAX_EVENTS) {
            snprintf(s_err, sizeof s_err, "%d notes: %d fit a lane", hits,
                     SEQ_MAX_EVENTS);
            s_err_at = -1;
        } else {
            /* '!n' parses - the owner chose the spelling - and what a lane does
             * when it has finished is docs/NEXT.md §4, designed together with
             * the ending. Until then it is refused rather than ignored: a lane
             * that plays for ever when told four times is a lie. */
            snprintf(s_err, sizeof s_err, "!n is not built yet");
            s_err_at = -1;
        }
        /* A lane the binding call created a moment ago for this very line has
         * never played; do not leave it holding one of the sixteen slots. */
        if (fresh) {
            l->used = false;
        }
        return (err != SEQ_PAT_OK) ? ESP_ERR_INVALID_ARG : ESP_ERR_INVALID_SIZE;
    }
    s_err[0] = '\0';
    s_err_at = -1;

    stage((int)(l - s_lanes), &s_comp);
    /* A DIRECTION IN FRONT OF THE PATTERN - '>ramp u 4' - is the lane's way;
     * a step can still say its own with u d l r. */
    l->dir = s_comp.dir ? s_comp.dir : 'd';
    snprintf(l->text, sizeof l->text, "%s", steps);
    /* The text this lane was compiled from, so a later press can tell "run
     * this again unchanged" from "I edited it". The WHOLE argument, direction
     * included: it hashed the text after the direction, while the command layer
     * hashed all of it, so '>ramp u 4' could never be silenced by running it
     * again. */
    l->src = seq_pattern_hash(steps);
    return ESP_OK;
}

bool seq_lane_now(const seq_lane_t *l, int *slot, uint32_t *cycle)
{
    if (!s_running || l == NULL || l->slots == 0) {
        return false;
    }
    seq_pattern_slot_now(s_tick, l->slots, l->div, l->rnum, l->rden, slot, cycle);
    return true;
}


esp_err_t seq_lane_bind(const char *name, const seq_binding_t *b)
{
    seq_lane_t *l = find(name, true);
    if (l == NULL) {
        return ESP_ERR_NO_MEM;
    }
    l->bind    = b->bind;
    l->melodic = (b->bind == SEQ_BIND_NOTE) && b->melodic;
    l->ctrl    = (b->bind == SEQ_BIND_CC);
    l->note    = b->note & 0x7F;
    l->octave  = b->octave;
    l->chan    = b->chan & 0x0F;
    l->gate_ms = b->gate_ms ? b->gate_ms : 40;
    l->cc      = b->cc & 0x7F;
    l->prim    = (b->bind == SEQ_BIND_VIZ) ? b->prim : 0;
    l->param   = (b->bind == SEQ_BIND_VIZ) ? b->param : 0;
    return ESP_OK;
}

esp_err_t seq_route(const char *name, const char *src)
{
    seq_lane_t *l = find(name, false);
    if (l == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    /* A LANE CANNOT DRIVE ITSELF. Every lane publishes what it played, so a
     * self-route would re-trigger every step for ever with nothing in the clock
     * able to stop it - a lane that plays on its own and ignores the transport,
     * which is not a lane. Refused rather than tolerated, because the line
     * reads perfectly sensibly and the failure does not. */
    if (src != NULL && strcmp(src, name) == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    snprintf(l->route, sizeof l->route, "%s", src ? src : "");
    l->trig = false;    /* no stale trigger from whatever it used to follow */

    /* A ROUTE IS A PATTERN. Making a lane live should not need a pattern
     * written first - and a routed lane ignores the one you write, so writing
     * it was a step that only made sense to whoever wrote the code. One step at
     * full value, which the source overrides on every hit. */
    if (l->route[0] != '\0' && l->slots == 0) {
        seq_comp_t *c = &s_comp;
        seq_pattern_compile("x", c);
        stage((int)(l - s_lanes), c);
        l->text[0] = '\0';
        l->src = 0;
        l->muted = false;
        l->used = true;
    }
    return ESP_OK;
}



void seq_swing(int percent)
{
    if (percent < 50) { percent = 50; }
    if (percent > 75) { percent = 75; }
    s_swing = percent;
}
int seq_get_swing(void) { return s_swing; }

void seq_sync(bool on)
{
    /* Start and stop are sent by seq_play/seq_stop, so turning sync on while
     * already running must announce the fact or the far end sits waiting. */
    if (on && !s_sync && s_running) { emit(0xFA, 0, 0); }
    if (!on && s_sync && s_running) { emit(0xFC, 0, 0); }
    s_sync = on;
}
bool seq_get_sync(void) { return s_sync; }


esp_err_t seq_mute(const char *name, bool mute)
{
    seq_lane_t *l = find(name, false);
    if (l == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    l->muted = mute;
    return ESP_OK;
}

void seq_bpm(int bpm)
{
    if (bpm < 20)  { bpm = 20; }
    if (bpm > 300) { bpm = 300; }
    s_bpm = bpm;
    if (s_clock != NULL) {
        esp_timer_stop(s_clock);
        esp_timer_start_periodic(s_clock, period_us());
    }
    /* RE-ANCHOR THE GRID, BUT KEEP THE POSITION.
     *
     * The grid has to move: it is a different grid now, and measuring the new
     * clock against the old one would report a tempo change as jitter. The BAR
     * does not. Setting s_tick to zero threw the musical position away, so
     * '>bpm 140' mid-performance restarted every lane at step one - audible, and
     * nothing asked for it.
     *
     * It showed up through the ensemble: a follower that takes the leader's tempo
     * also took a bar reset with it, which was the single worst phase outlier in
     * the measurements - about 2 ms, right after every tempo change, where the
     * steady state is under 600 us.
     *
     * Anchoring so that the CURRENT tick is due now preserves both: the bar
     * carries on where it was and the statistic measures the new grid. */
    if (s_running) {
        s_grid_t0 = esp_timer_get_time() - (int64_t)s_tick * (int64_t)period_us();
    } else {
        s_grid_t0 = 0;
        s_tick    = 0;
    }
    seq_stats_reset();
}

int  seq_get_bpm(void)  { return s_bpm; }
bool seq_running(void)  { return s_running; }
uint32_t seq_position(void) { return s_pos; }

void seq_play(void)
{
    s_pos  = 0;
    s_tick = 0;
    s_grid_t0 = 0;               /* anchored on the first tick */
    seq_stats_reset();
    s_running = true;
    if (s_sync) {
        /* Song-position-zero then start, which is what a DAW expects and what
         * makes the deck the master rather than a thing that drifts. */
        emit(0xF2, 0, 0);
        emit(0xFA, 0, 0);
    }
}

void seq_stop(void)
{
    if (s_sync && s_running) {
        emit(0xFC, 0, 0);
    }
    s_running = false;
    seq_all_notes_off();
}

void seq_all_notes_off(void)
{
    for (size_t i = 0; i < sizeof s_offs / sizeof s_offs[0]; i++) {
        if (s_offs[i].armed) {
            s_offs[i].armed = false;
            emit(s_offs[i].status, s_offs[i].d1, 0);
        }
    }
    /* Belt and braces: CC 123 on every channel. This is what an audience
     * needs when something has gone wrong, and it costs 48 bytes. */
    for (uint8_t ch = 0; ch < 16; ch++) {
        emit((uint8_t)(0xB0 | ch), 123, 0);
    }
}

const seq_lane_t *seq_lanes(int *count)
{
    if (count != NULL) {
        int n = 0;
        for (int i = 0; i < SEQ_MAX_LANES; i++) {
            if (s_lanes[i].used) { n++; }
        }
        *count = n;
    }
    return s_lanes;
}

esp_err_t seq_dest_add(const char *name, seq_sink_t fn, seq_flush_t flush,
                       const char *help)
{
    if (s_ndests >= SEQ_MAX_DESTS) {
        return ESP_ERR_NO_MEM;
    }
    dest_t *d = &s_dests[s_ndests++];
    snprintf(d->name, sizeof d->name, "%s", name);
    d->fn    = fn;
    d->flush = flush;
    d->help  = help;
    d->on    = false;
    return ESP_OK;
}

static dest_t *dest_find(const char *name)
{
    for (int i = 0; i < s_ndests; i++) {
        if (strcmp(s_dests[i].name, name) == 0) {
            return &s_dests[i];
        }
    }
    return NULL;
}

esp_err_t seq_dest_enable(const char *name, bool on)
{
    dest_t *d = dest_find(name);
    if (d == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    d->on = on;
    return ESP_OK;
}

bool seq_dest_is_on(const char *name)
{
    const dest_t *d = dest_find(name);
    return d != NULL && d->on;
}

int         seq_dest_count(void)      { return s_ndests; }
const char *seq_dest_name(int i)      { return (i >= 0 && i < s_ndests) ? s_dests[i].name : ""; }
const char *seq_dest_help(int i)      { return (i >= 0 && i < s_ndests) ? s_dests[i].help : ""; }
bool        seq_dest_on(int i)        { return (i >= 0 && i < s_ndests) && s_dests[i].on; }
