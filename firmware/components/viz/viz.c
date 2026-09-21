#include "viz.h"

#include <stdio.h>
#include <string.h>

#include "seq.h"
#include "seq_pattern.h"

#define NGEN 4
#define NAME_MAX 8

/* The generators, in a flat table like every other name in this system. */
static const char *s_names[NGEN] = { "noise", "bar", "dot", "wave" };

typedef struct {
    bool    used;
    uint32_t mask, chance;
    uint8_t  val[SEQ_MAX_STEPS];   /* 0-9 intensity, 255 = no digit */
    uint8_t  prob[SEQ_MAX_STEPS];
    uint8_t  steps;
    uint16_t tps;                  /* this lane's ticks per step */
    char     src[NAME_MAX + 4];    /* routed from this lane, or empty */
    uint8_t  routed_val;           /* what that lane last played, 0-9 */
} vlane_t;

static vlane_t s_l[NGEN];
static char    s_fb[VIZ_H][VIZ_W + 1];
static bool    s_split;
static uint32_t s_rng = 0x1234567u;

static inline uint32_t rng(void)
{
    s_rng ^= s_rng << 13; s_rng ^= s_rng >> 17; s_rng ^= s_rng << 5;
    return s_rng;
}

static int gen_index(const char *g)
{
    for (int i = 0; i < NGEN; i++) {
        if (strcmp(s_names[i], g) == 0) { return i; }
    }
    return -1;
}

bool viz_active(void)
{
    for (int i = 0; i < NGEN; i++) { if (s_l[i].used) { return true; } }
    return false;
}

static int s_split_w;                /* 0 means "a third of whatever we have" */

void viz_split(bool on) { s_split = on; }
bool viz_split_on(void) { return s_split; }

void viz_split_width(int cols)
{
    s_split_w = (cols > 0) ? cols : 0;
}

int viz_split_cols(int total)
{
    /* A third, rounded down, and never so much that the code side cannot hold
     * a pattern line without wrapping. Sixteen columns is '>kick ' plus
     * sixteen steps, which is the shortest line worth looking at. */
    int w = (s_split_w > 0) ? s_split_w : total / 3;
    const int keep = 18;
    if (w > total - keep) { w = total - keep; }
    if (w > VIZ_W)        { w = VIZ_W; }
    if (w < 4)            { w = 4; }
    return w;
}
const char *viz_row(int y)
{
    return (y >= 0 && y < VIZ_H) ? s_fb[y] : "";
}

esp_err_t viz_lane(const char *gen, const char *pattern)
{
    const int gi = gen_index(gen);
    if (gi < 0) { return ESP_ERR_NOT_FOUND; }
    vlane_t *l = &s_l[gi];

    if (pattern == NULL || pattern[0] == '\0') {
        l->used = false;                 /* an empty pattern removes it */
        return ESP_OK;
    }

    /* THE SAME WALK AS A MUSIC LANE. seq_pattern.h owns which characters are
     * steps, where a bracket attaches, and what a trailing rate means - so a
     * visual line and a drum line cannot drift apart about their own grammar,
     * and the playhead lands correctly on both. */
    int rnum = 1, rden = 1;
    const int plen = seq_pattern_rate(pattern, &rnum, &rden);
    uint32_t mask = 0, chance = 0;
    uint8_t val[SEQ_MAX_STEPS], prob[SEQ_MAX_STEPS];
    memset(val, 255, sizeof val);
    memset(prob, 255, sizeof prob);
    int n = 0;
    const char *stop = pattern + plen;
    for (const char *p = pattern; p < stop && *p && n < SEQ_MAX_STEPS; p++) {
        const int pl = seq_pattern_param_len(p);
        if (pl > 0) {
            const int v = seq_pattern_param(p);
            if (v >= 0 && v <= 100 && n > 0) {
                prob[n - 1] = (uint8_t)v;
                chance |= (1u << (n - 1));
            }
            p += pl - 1;
            continue;
        }
        if (seq_pattern_is_spacing(*p)) { continue; }
        if (*p != '.' && *p != '-' && *p != '_') {
            mask |= (1u << n);
            if (*p == '?') { chance |= (1u << n); }
            if (*p >= '0' && *p <= '9') { val[n] = (uint8_t)(*p - '0'); }
        }
        n++;
    }
    if (n == 0) { l->used = false; return ESP_OK; }

    l->mask = mask; l->chance = chance; l->steps = (uint8_t)n;
    memcpy(l->val, val, sizeof l->val);
    memcpy(l->prob, prob, sizeof l->prob);
    long t = (long)SEQ_TICKS_PER_STEP * rden / (rnum > 0 ? rnum : 1);
    if (t < 1) { t = 1; }
    l->tps = (uint16_t)(t > 32767 ? 32767 : t);
    l->used = true;
    return ESP_OK;
}

esp_err_t viz_route(const char *gen, const char *src)
{
    const int gi = gen_index(gen);
    if (gi < 0) { return ESP_ERR_NOT_FOUND; }
    snprintf(s_l[gi].src, sizeof s_l[gi].src, "%s", src ? src : "");
    return ESP_OK;
}

void viz_lane_played(const char *lane, uint8_t value)
{
    if (lane == NULL || lane[0] == '\0') { return; }
    for (int i = 0; i < NGEN; i++) {
        if (s_l[i].src[0] != '\0' && strcmp(s_l[i].src, lane) == 0) {
            /* MIDI velocity and CC are both 0-127; the visuals think in 0-9,
             * which is the same resolution the pattern digits have. Mapping
             * here rather than at every use keeps one scale in the system. */
            s_l[i].routed_val = (uint8_t)((value * 9 + 63) / 127);
        }
    }
}

static void clear_frame(void)
{
    for (int y = 0; y < VIZ_H; y++) {
        memset(s_fb[y], ' ', VIZ_W);
        s_fb[y][VIZ_W] = '\0';
    }
}

/* The four primitives. Each is a handful of lines on purpose: what makes this
 * expressive is eight lanes at different rates driving them, not any one of
 * them being clever. */
static void draw_noise(int amt)
{
    const int cells = VIZ_W * VIZ_H * amt / 9;
    static const char ink[] = ".:*#@";
    for (int i = 0; i < cells; i++) {
        const uint32_t r = rng();
        s_fb[r % VIZ_H][(r >> 8) % VIZ_W] = ink[(r >> 16) % 5];
    }
}

static void draw_bar(int amt, uint32_t step)
{
    const int x = (int)(step % VIZ_W);
    const int h = amt * VIZ_H / 9;
    for (int y = VIZ_H - h; y < VIZ_H; y++) {
        if (y >= 0) { s_fb[y][x] = '#'; }
    }
}

static void draw_dot(int amt, uint32_t step)
{
    const int x = (int)(step % VIZ_W);
    const int y = (VIZ_H - 1) - (amt * (VIZ_H - 1) / 9);
    static const char ink[] = ".oO@";
    s_fb[y < 0 ? 0 : y][x] = ink[amt > 6 ? 3 : (amt > 3 ? 2 : (amt > 1 ? 1 : 0))];
}

/* A sine without floating point or a table: a triangle folded twice is close
 * enough at twelve rows, and it costs nothing on the clock. */
static void draw_wave(int amt, uint32_t step)
{
    const int mid = VIZ_H / 2;
    for (int x = 0; x < VIZ_W; x++) {
        const int t = (x + (int)step) % 16;
        const int tri = (t < 8) ? t : (16 - t);          /* 0..8..0 */
        const int y = mid + ((tri - 4) * amt * mid) / (4 * 9);
        if (y >= 0 && y < VIZ_H) { s_fb[y][x] = '-'; }
    }
}

void viz_tick(uint32_t tick)
{
    if (!viz_active()) { return; }
    clear_frame();

    for (int i = 0; i < NGEN; i++) {
        vlane_t *l = &s_l[i];
        if (!l->used || l->steps == 0) { continue; }
        const uint32_t tps = l->tps ? l->tps : SEQ_TICKS_PER_STEP;
        if ((tick % tps) != 0) { continue; }
        const uint32_t step = tick / tps;
        const int s = (int)(step % l->steps);
        if (!(l->mask & (1u << s))) { continue; }
        if (l->chance & (1u << s)) {
            const uint32_t pct = (l->prob[s] == 255u) ? 50u : l->prob[s];
            if ((rng() % 100u) >= pct) { continue; }
        }
        /* Routed intensity wins over the digit. That is the point of routing:
         * the line says WHEN and another lane says HOW MUCH. */
        int amt = (l->src[0] != '\0') ? l->routed_val
                                     : ((l->val[s] == 255) ? 9 : l->val[s]);
        if (amt < 0) { amt = 0; }
        if (amt > 9) { amt = 9; }

        switch (i) {
        case 0: draw_noise(amt);        break;
        case 1: draw_bar(amt, step);    break;
        case 2: draw_dot(amt, step);    break;
        default: draw_wave(amt, step);  break;
        }
    }
}

int viz_text(char *out, int max)
{
    int n = 0;
    for (int y = 0; y < VIZ_H && n < max - 1; y++) {
        const int w = snprintf(out + n, (size_t)(max - n), "%s\n", s_fb[y]);
        if (w <= 0) { break; }
        n += w;
    }
    return n;
}
