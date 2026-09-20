#include "seq.h"

#include <ctype.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "seq";

static seq_lane_t s_lanes[SEQ_MAX_LANES];
static int        s_bpm = 120;
static bool       s_running;
static int        s_pos;
static esp_timer_handle_t s_clock;
static seq_sink_t s_sink;

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
    if (s_sink != NULL) {
        s_sink(status, d1, d2);
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

/* The clock. A hardware timer, never a task delay - docs/OS.md: "Never
 * sequence from a task delay. Use a hardware timer." A 16th note at 120 bpm
 * is 125,000 us exactly. */
static void tick(void *arg)
{
    (void)arg;
    const int64_t now = esp_timer_get_time();
    service_offs(now);

    if (!s_running) {
        return;
    }
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        const seq_lane_t *l = &s_lanes[i];
        if (!l->used || l->muted || l->steps == 0) {
            continue;
        }
        const int step = s_pos % l->steps;
        if (l->mask & (1u << step)) {
            emit((uint8_t)(0x90 | (l->chan & 0x0F)), l->note, l->vel);
            schedule_off(l->chan, l->note, l->gate_ms);
        }
    }
    s_pos = (s_pos + 1) & 0x7FFF;
}

static uint64_t period_us(void)
{
    /* Sixteenth notes. 60,000,000 / bpm / 4. */
    return (uint64_t)(60000000.0 / (double)s_bpm / 4.0);
}

esp_err_t seq_init(void)
{
    memset(s_lanes, 0, sizeof s_lanes);
    memset(s_offs, 0, sizeof s_offs);
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

static seq_lane_t *find(const char *name, bool create)
{
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        if (s_lanes[i].used && strncmp(s_lanes[i].name, name, SEQ_NAME_MAX) == 0) {
            return &s_lanes[i];
        }
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
    uint32_t mask = 0;
    int n = 0;
    for (const char *p = steps; *p != '\0' && n < SEQ_MAX_STEPS; p++) {
        if (*p == ' ') {
            continue;                /* spacing for the eye, ignored */
        }
        /* Anything that is not a rest is a hit. Nobody should have to
         * remember whether the hit character is x, o or *. */
        if (*p != '.' && *p != '-' && *p != '_') {
            mask |= (1u << n);
        }
        n++;
    }
    if (n == 0) {
        l->used = false;             /* an empty pattern removes the lane */
        return ESP_OK;
    }
    /* Compiled. The clock callback never sees this string again. */
    l->mask = mask;
    l->steps = (uint8_t)n;
    return ESP_OK;
}

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
}

int  seq_get_bpm(void)  { return s_bpm; }
bool seq_running(void)  { return s_running; }
int  seq_position(void) { return s_pos; }

void seq_play(void)
{
    s_pos = 0;
    s_running = true;
}

void seq_stop(void)
{
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

void seq_set_sink(seq_sink_t sink) { s_sink = sink; }
