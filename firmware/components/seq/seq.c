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

static seq_lane_t s_lanes[SEQ_MAX_LANES];
static int        s_bpm = 120;
static bool       s_running;
/* volatile: written by the clock on the esp_timer task, read by the editor
 * task to place the playhead. Aligned 32-bit does not tear on Xtensa, so this
 * is about the compiler not hoisting the read out of the draw loop. */
static volatile uint32_t s_pos;
static uint32_t   s_tick;            /* 24 PPQN pulses since play */
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
    uint8_t  status, d1, d2;
    uint32_t queued_us;      /* for the transport-latency statistic */
} midi_ev_t;

/* The ideal grid. Re-anchored on play and on any tempo change, so the
 * statistic measures dispatch jitter and never accumulated tempo error -
 * those are different problems with different fixes. */
static int64_t    s_grid_t0;
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
                        s_dests[i].fn(ev.status, ev.d1, ev.d2, ev.queued_us);
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
    int64_t due_us;
    uint8_t status, d1;
    bool    armed;
} pending_off_t;
static pending_off_t s_offs[SEQ_MAX_LANES * 4];

static void emit(uint8_t status, uint8_t d1, uint8_t d2)
{
    if (s_midiq == NULL) {
        return;                      /* before seq_init; nowhere to put it */
    }
    const midi_ev_t ev = { status, d1, d2, (uint32_t)esp_timer_get_time() };
    if (xQueueSend(s_midiq, &ev, 0) != pdTRUE) {
        midi_ev_t drop;
        (void)xQueueReceive(s_midiq, &drop, 0);   /* make room */
        (void)xQueueSend(s_midiq, &ev, 0);
        s_dropped++;
    }
}

static void schedule_off(uint8_t chan, uint8_t note, uint16_t ms)
{
    const int64_t due = esp_timer_get_time() + (int64_t)ms * 1000;
    for (size_t i = 0; i < sizeof s_offs / sizeof s_offs[0]; i++) {
        if (!s_offs[i].armed) {
            s_offs[i].armed  = true;
            s_offs[i].due_us = due;
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
            emit(s_offs[i].status, s_offs[i].d1, 0);
        }
    }
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

static void fire_step(int step)
{
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        const seq_lane_t *l = &s_lanes[i];
        if (!l->used || l->muted || l->steps == 0) {
            continue;
        }
        const int s = step % l->steps;
        if (!(l->mask & (1u << s))) {
            continue;
        }
        /* '?' - maybe. Half, because half is the only ratio that needs no
         * number after it, and a number after it would be the start of the
         * syntax this instrument is trying not to have. */
        if (l->chance & (1u << s)) {
            /* '?' alone is half; '?[15]' is fifteen per cent. Half is the
             * default because it is the only ratio that needs no number, and
             * the bracket is there for when the player wants a different one
             * rather than a different character. */
            /* 255 means the step carried no bracket, so use the default of
             * a half. Everything else is taken literally, including zero. */
            const uint32_t pct = (l->prob[s] == 255u) ? 50u : l->prob[s];
            if ((rng_next() % 100u) >= pct) {
                continue;
            }
        }
        /* Accent and ghost are a ratio of the lane's own velocity, not fixed
         * numbers, so setting a lane quiet keeps its accents in proportion
         * instead of flattening the whole pattern against a ceiling. */
        int vel = l->vel;
        if (l->accent & (1u << s)) { vel = vel + (127 - vel) * 3 / 4; }
        if (l->ghost  & (1u << s)) { vel = vel / 3; }
        if (vel < 1)   { vel = 1; }
        if (vel > 127) { vel = 127; }

        /* On a melodic lane an 'x' - or an 'X', or a ',' - is the ROOT, not
         * MIDI note 0. Falling through to l->note here would emit C-1 at the
         * bottom of the range, which on most synths is inaudible and on a few
         * is a thump nobody asked for, and the player would reasonably
         * conclude the lane was broken. */
        if (l->ctrl) {
            /* Digits are values: 0 is 0 and 9 is 127. A step with no digit
             * holds the last value rather than jumping to zero, because a
             * controller that snaps to silence on every unmarked step is a
             * stutter, not a sweep. */
            /* A step with no digit HOLDS, it does not emit zero. The code
             * here sent 0 while the comment beside it claimed otherwise - a
             * controller that snaps to silence between steps is a stutter,
             * and the comment was describing the intention rather than the
             * behaviour. Nothing is sent at all on a hold, which is also one
             * fewer message on the wire. */
            if (l->deg[s] == 0xFF) {
                continue;
            }
            const uint8_t v = (uint8_t)((l->deg[s] * 127) / 9);
            emit((uint8_t)(0xB0 | (l->chan & 0x0F)), l->cc, v);
            continue;
        }
        const uint8_t note = l->melodic
            ? degree_note(l->deg[s] == 0xFF ? 0 : l->deg[s], l->octave)
            : l->note;
        emit((uint8_t)(0x90 | (l->chan & 0x0F)), note, (uint8_t)vel);
        schedule_off(l->chan, note, l->gate_ms);
    }
}

/* The clock. A hardware timer, never a task delay - docs/OS.md: "Never
 * sequence from a task delay. Use a hardware timer."
 *
 * It ticks at 24 PPQN, not at the step rate, because that is the rate MIDI
 * clock is defined at: sync costs one message on a tick that already exists.
 * At 120 bpm a tick is 20,833 us and a sixteenth is six of them, 125,000 us
 * exactly. */
static void tick(void *arg)
{
    (void)arg;
    const int64_t now = esp_timer_get_time();
    service_offs(now);

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
    }
    {
        const int64_t ideal = s_grid_t0 + (int64_t)s_tick * (int64_t)period_us();
        int64_t d = now - ideal;
        if (d >  1000000) { d =  1000000; }
        if (d < -1000000) { d = -1000000; }
        stat_add(&s_clock_stat, (int32_t)d);
    }
    if (s_sync && (s_tick % SEQ_CLOCK_EVERY) == 0) {
        emit(0xF8, 0, 0);            /* timing clock, no data bytes */
    }

    const int step  = (int)(s_tick / SEQ_TICKS_PER_STEP);
    const int phase = (int)(s_tick % SEQ_TICKS_PER_STEP);
    const int want  = (step & 1) ? swing_ticks() : 0;
    if (phase == want) {
        /* NO MASK. It used to be `step & 0x7FFF`, and 32768 is a power of
         * two: 8-, 16- and 32-step lanes wrapped cleanly, but a 5-, 6- or
         * 12-step lane took a PHASE JUMP of (32768 % steps) every time the
         * counter rolled - once every 66 minutes at 124 bpm. A pattern
         * silently moving off the beat, once an hour, in the middle of a set.
         *
         * The counter is 32 bits and advances at the step rate, so it now
         * rolls after about eight years of continuous playing. */
        s_pos = step;
        fire_step(s_pos);
    }
    s_tick++;
}

static uint64_t period_us(void)
{
    /* One internal tick. 60,000,000 / bpm / 96. At 124 bpm that is 5,040 us,
     * and a sixteenth is 24 of them - 120,967 us. */
    return (uint64_t)(60000000.0 / (double)s_bpm / (double)SEQ_PPQN);
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

esp_err_t seq_lane(const char *name, const char *steps)
{
    seq_lane_t *l = find(name, true);
    if (l == NULL) {
        return ESP_ERR_NO_MEM;
    }
    uint32_t mask = 0, accent = 0, ghost = 0, chance = 0;
    uint8_t  deg[SEQ_MAX_STEPS];
    uint8_t  prob[SEQ_MAX_STEPS];
    memset(deg, 0xFF, sizeof deg);
    memset(prob, 255, sizeof prob);   /* 255 = no bracket on this step */
    int n = 0;
    for (const char *p = steps; *p != '\0' && n < SEQ_MAX_STEPS; p++) {
        /* A bracket is a parameter on the step just placed, not a step. */
        const int plen = seq_pattern_param_len(p);
        if (plen > 0) {
            const int v = seq_pattern_param(p);
            if (v >= 0 && v <= 100 && n > 0) {
                /* 0 means NEVER. It used to be clamped to 1%, which made
                 * '?[0]' a step that fires once every few minutes - the one
                 * value whose meaning is obvious was the one value that lied.
                 * 255 is the sentinel for "no parameter"; 0 is a real zero. */
                prob[n - 1] = (uint8_t)v;
                chance |= (1u << (n - 1));   /* a per-cent implies maybe */
            }
            p += plen - 1;
            continue;
        }
        if (seq_pattern_is_spacing(*p)) {
            continue;    /* spacing for the eye - seq_pattern.h owns this rule */
        }
        /* Anything that is not a rest is a hit. Nobody should have to
         * remember whether the hit character is x, o or *. */
        if (*p != '.' && *p != '-' && *p != '_') {
            mask |= (1u << n);
            if (*p == 'X') { accent |= (1u << n); }
            if (*p == ',') { ghost  |= (1u << n); }
            if (*p == '?') { chance |= (1u << n); }
            if (*p >= '0' && *p <= '9') { deg[n] = (uint8_t)(*p - '0'); }
        }
        n++;
    }
    if (n == 0) {
        l->used = false;             /* an empty pattern removes the lane */
        return ESP_OK;
    }
    /* Compiled. The clock callback never sees this string again. */
    l->mask   = mask;
    l->accent = accent;
    l->ghost  = ghost;
    l->chance = chance;
    memcpy(l->prob, prob, sizeof l->prob);
    memcpy(l->deg, deg, sizeof l->deg);
    l->steps  = (uint8_t)n;
    /* The text this lane was compiled from, so a later press can tell "run
     * this again unchanged" from "I edited it". After the empty-pattern early
     * return above, so a removed lane carries no source. */
    l->src    = seq_pattern_hash(steps);
    return ESP_OK;
}

esp_err_t seq_lane_ctrl(const char *name, int cc, int chan)
{
    seq_lane_t *l = find(name, true);
    if (l == NULL) {
        return ESP_ERR_NO_MEM;
    }
    l->ctrl    = true;
    l->melodic = false;
    if (cc >= 0 && cc < 128)   { l->cc = (uint8_t)cc; }
    if (chan >= 0 && chan < 16) { l->chan = (uint8_t)chan; }
    return ESP_OK;
}

esp_err_t seq_lane_melodic(const char *name, int octave, int chan, int gate_ms)
{
    seq_lane_t *l = find(name, true);
    if (l == NULL) {
        return ESP_ERR_NO_MEM;
    }
    l->melodic = true;
    l->octave  = (int8_t)octave;
    if (chan >= 0 && chan < 16) { l->chan = (uint8_t)chan; }
    if (gate_ms > 0)            { l->gate_ms = (uint16_t)gate_ms; }
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

esp_err_t seq_lane_note(const char *name, int note, int chan)
{
    seq_lane_t *l = find(name, true);
    if (l == NULL) {
        return ESP_ERR_NO_MEM;
    }
    if (note >= 0 && note < 128) { l->note = (uint8_t)note; }
    if (chan >= 0 && chan < 16)  { l->chan = (uint8_t)chan; }
    return ESP_OK;
}

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
    /* Re-anchor: the grid is a different grid now, and measuring the new
     * clock against the old one would report a tempo change as jitter. */
    s_grid_t0 = 0;
    s_tick    = 0;
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
