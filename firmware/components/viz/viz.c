#include "viz.h"

#include <stdio.h>
#include <string.h>

#include "seq.h"
#include "seq_pattern.h"

#define NGEN 8
#define NAME_MAX 8

/* THREE SOURCES AND FIVE OPERATORS, in a flat table like every other name in
 * this system.
 *
 * WHY THIS SHAPE. The first attempt was eight SOURCES - noise, bar, dot, wave,
 * ring, rain, box - which is a bag of shapes, and a bag of shapes is what a
 * system looks like when nobody decided what the operations are. Adding a
 * ninth shape adds one picture. What a node graph actually gives you is few
 * sources and a set of operators every source can be fed through, so the
 * vocabulary multiplies instead of accumulating: feedback, transform,
 * replicate, mirror, displace. Three sources through five operators is a far
 * larger space than eight sources, out of the same eight names.
 *
 * Every shape that was lost is reachable as a combination. Rain is noise that
 * moves down and echoes. A bar is a ramp. A wave is a ramp that warps. A box
 * is a disc that warps. That is the trade being made on purpose.
 *
 * TABLE ORDER IS DRAW ORDER, and the order is a pipeline:
 *
 *   history   echo   lays the last frame back down, one ink step dimmer
 *   motion    move   shifts what is there, so the history streaks and the
 *             warp   sources land fresh - which is what makes a trail
 *   sources   noise  disc  ramp
 *   repeat    tile   fold
 *
 * So '>viz echo 9' plus '>viz move d' plus '>viz noise 2' is falling rain with
 * a dissolving tail, and none of those three lines knows about the others. */
static const char *s_names[NGEN] = {
    "echo", "move", "warp",          /* history, then motion  */
    "noise", "disc", "ramp",         /* the sources           */
    "tile", "fold",                  /* repetition            */
};

typedef struct {
    bool    used;
    uint32_t mask, chance;
    uint8_t  val[SEQ_MAX_STEPS];   /* 0-9 amount, 255 = no digit given  */
    /* The step's own character. A digit says HOW MUCH; a letter says WHICH
     * WAY, and the two primitives that point somewhere - move and warp - read
     * it as u, d, l or r. Storing the character rather than translating it at
     * compile time means one grammar: any non-rest character is a hit, and
     * what it MEANS is the primitive's business. */
    char     chr[SEQ_MAX_STEPS];
    /* The lane's direction, from a 'u', 'd', 'l' or 'r' written before the
     * pattern. A step's own letter overrides it for that step. */
    char     dir;
    uint8_t  prob[SEQ_MAX_STEPS];
    uint8_t  steps;
    uint16_t tps;                  /* this lane's ticks per step */
    char     src[NAME_MAX + 4];    /* routed from this lane, or empty */
    uint8_t  routed_val;           /* what that lane last played, 0-9 */
} vlane_t;

static vlane_t s_l[NGEN];
static char    s_fb[VIZ_H][VIZ_W + 1];
/* The frame before this one, for echo. Feedback is the single technique that
 * turns a still picture into an animation, so it gets the memory it needs. */
static char    s_prev[VIZ_H][VIZ_W + 1];
static bool    s_split;
static uint32_t s_rng = 0x1234567u;

/* OUR OWN GLYPHS, NOT PUNCTUATION.
 *
 * The picture used to be drawn with ' ', '.', ':', '*', '#' and '@' - six
 * shapes a typeface designer chose for setting prose, pressed into service as
 * a tonal ramp. They are uneven as tones, they carry the letters' sidebearings
 * so a field of them is striped with white gutters, and none of them was drawn
 * for this. The tiles at 128..155 were: see tools/font_tiles.py.
 *
 *   128..136   nine tones, an ordered dither from nothing to solid
 *   137..140   sparkles: a speck, a four-point star, an eight-point, a burst
 *   141..145   half blocks four ways, and a centred square
 *   146..148   a diamond, a filled disc, a hollow ring
 *   149..151   diagonals and their crossing
 *   152..155   quadrant arcs, which tile 2x2 into one circle twice the size
 *
 * NINE TONES IS THE POINT. Six uneven steps could not fade; nine even ones can,
 * so echo dissolves a trail through real greys instead of jumping ':' to '.' to
 * gone. And the field is seamless, because a tone fills its cell edge to edge
 * where a letter must not touch its neighbour. */
#define TONE_0   128                 /* an empty cell, as a tone       */
#define TONE_TOP 8                   /* 136 is solid                   */
#define SOLID    (char)(TONE_0 + TONE_TOP)

#define SPARK_FIRST 137              /* speck, star4, star8, burst     */
#define SPARK_N     4
#define ARC_FIRST   152              /* top-left, top-right, br, bl    */

static inline int tone_of(char ch)
{
    const unsigned u = (unsigned char)ch;
    if (u >= TONE_0 && u <= TONE_0 + TONE_TOP) { return (int)(u - TONE_0); }
    return (u == ' ' || u == 0) ? 0 : TONE_TOP;   /* anything else is solid */
}

static void clear_frame(void);
static void viz_frame(uint32_t tick);

/* The live frame size. Defaults to something drawable so a frame exists before
 * any layout has been set - viz_tick() can be called from the clock the moment
 * a lane compiles, which is before the editor has drawn anything. */
static int s_w = 28, s_h = 10;

int viz_cols(void) { return s_w; }
int viz_rows(void) { return s_h; }

void viz_size(int w, int h)
{
    if (w < 4)     { w = 4; }
    if (h < 2)     { h = 2; }
    if (w > VIZ_W) { w = VIZ_W; }
    if (h > VIZ_H) { h = VIZ_H; }
    if (w == s_w && h == s_h) { return; }
    s_w = w;
    s_h = h;
    clear_frame();          /* the old frame is the wrong shape - do not show it */
}

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

static int s_split_h;                /* 0 means "about half the rows" */

void viz_split(bool on) { s_split = on; }
bool viz_split_on(void) { return s_split; }

void viz_split_rows(int rows)
{
    s_split_h = (rows > 0) ? rows : 0;
}

/* CODE_ROWS_MIN is the whole argument for who loses when the two do not fit.
 * Four visible lines is enough to hold a lane and its neighbours in view while
 * editing; below that the editor stops being usable, and an unusable editor
 * with a beautiful picture next to it is not a live-coding instrument. */
#define CODE_ROWS_MIN 4

viz_pane_t viz_pane(int cols, int rows)
{
    viz_pane_t p = { .x = 0, .y = 0, .w = 0, .h = 0 };
    if (!s_split || cols < 8 || rows < CODE_ROWS_MIN + 3) {
        return p;
    }

    /* Half the rows, rounded UP: a picture three cells tall reads as nothing,
     * and the code side is still legible at five lines. Plus two for the
     * border, which is part of the pane and has to be paid for out of it. */
    int h = (s_split_h > 0) ? s_split_h + 2 : (rows + 1) / 2;
    if (rows - h < CODE_ROWS_MIN) { h = rows - CODE_ROWS_MIN; }
    if (h > VIZ_H + 2)            { h = VIZ_H + 2; }
    if (h < 3)                    { return p; }

    p.w = cols;
    p.h = h;
    p.x = 0;
    p.y = rows - h;
    return p;
}

const char *viz_row(int y)
{
    return (y >= 0 && y < s_h) ? s_fb[y] : "";
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

    /* A DIRECTION IN FRONT OF THE PATTERN, BECAUSE A STEP CANNOT SAY BOTH.
     *
     * One character per step means a step holds an amount or a direction, and
     * the primitives that point somewhere need both at once - '>viz ramp 4'
     * has no way to say which way, and '>viz ramp u' has no way to say how
     * far. So a lone u, d, l or r before the pattern sets the lane's direction
     * and is not a step:
     *
     *     >viz ramp u 4.6.9.6.      up, at those amounts
     *     >viz move d....d...       down, twice a bar, full
     *
     * Both idioms work and neither needs explaining twice: a letter in front is
     * the lane's direction, a letter in a step is that step's direction, and
     * the default is down because falling is what ASCII does first. */
    char dir = 'd';
    if ((pattern[0] == 'u' || pattern[0] == 'd' ||
         pattern[0] == 'l' || pattern[0] == 'r') &&
        (pattern[1] == ' ' || pattern[1] == '\t')) {
        dir = pattern[0];
        pattern++;
        while (*pattern == ' ' || *pattern == '\t') { pattern++; }
        if (*pattern == '\0') {
            /* 'ramp u' on its own: the direction, at full amount, every step.
             * Refusing here would make the shortest useful line an error. */
            pattern = "9";
        }
    }

    /* THE SAME WALK AS A MUSIC LANE. seq_pattern.h owns which characters are
     * steps, where a bracket attaches, and what a trailing rate means - so a
     * visual line and a drum line cannot drift apart about their own grammar,
     * and the playhead lands correctly on both. */
    int rnum = 1, rden = 1;
    const int plen = seq_pattern_rate(pattern, &rnum, &rden);
    uint32_t mask = 0, chance = 0;
    uint8_t val[SEQ_MAX_STEPS], prob[SEQ_MAX_STEPS];
    char    chr[SEQ_MAX_STEPS];
    memset(val, 255, sizeof val);
    memset(prob, 255, sizeof prob);
    memset(chr, 0, sizeof chr);
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
            chr[n] = *p;
            if (*p == '?') { chance |= (1u << n); }
            if (*p >= '0' && *p <= '9') { val[n] = (uint8_t)(*p - '0'); }
        }
        n++;
    }
    if (n == 0) { l->used = false; return ESP_OK; }

    l->mask = mask; l->chance = chance; l->steps = (uint8_t)n;
    memcpy(l->val, val, sizeof l->val);
    memcpy(l->prob, prob, sizeof l->prob);
    memcpy(l->chr, chr, sizeof l->chr);
    long t = (long)SEQ_TICKS_PER_STEP * rden / (rnum > 0 ? rnum : 1);
    if (t < 1) { t = 1; }
    l->tps = (uint16_t)(t > 32767 ? 32767 : t);
    l->dir = dir;
    l->used = true;
    return ESP_OK;
}

void viz_forget_all(void)
{
    for (int i = 0; i < NGEN; i++) {
        s_l[i].used = false;
        s_l[i].src[0] = '\0';
        s_l[i].routed_val = 0;
    }
    clear_frame();
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
    for (int y = 0; y < s_h; y++) {
        memset(s_fb[y], TONE_0, (size_t)s_w);
        s_fb[y][s_w] = '\0';
    }
}

/* THE AMOUNT IS ALWAYS THE SAME IDEA: 0 is none and 9 is full.
 *
 * This is the one rule that makes the digits readable, and it was missing. The
 * old set had a digit mean density in one primitive, height in another, a
 * position in a third and a radius in a fourth, so '0..3..6..9' meant four
 * unrelated things depending on which line it was on and there was nothing to
 * learn. Now every primitive answers the same question - HOW MUCH of you? -
 * and what that scales is the primitive's one-line description:
 *
 *   echo   how much of the last frame survives   0 none .. 9 a long tail
 *   move   how far it shifts, in cells           direction from u d l r
 *   warp   how far rows are displaced            axis from u d l r
 *   noise  how much of the field is inked
 *   disc   the radius                            0 a dot .. 9 fills
 *   ramp   how far the gradient has swept        direction from u d l r
 *   tile   how many copies                       1 .. 4
 *   fold   how many mirrors                      1 .. 3
 *
 * SPEED IS THE PATTERN, NOT A NUMBER. '>viz move dddddddd' shifts every step,
 * '>viz move d.......' once a bar, '>viz move d /2' at half rate. Rate is
 * already in the grammar for the drums, so movement borrows it rather than
 * inventing a parameter - which is why none of these needs a direction word or
 * a second bracket field. */

/* A direction letter as a delta. Defaults to down, because that is the one
 * every ASCII animation needs first. */
static void delta_of(char c, int *dx, int *dy)
{
    switch (c) {
    case 'u': *dx =  0; *dy = -1; break;
    case 'l': *dx = -1; *dy =  0; break;
    case 'r': *dx =  1; *dy =  0; break;
    default:  *dx =  0; *dy =  1; break;         /* d, and anything unnamed */
    }
}

/* ---- history ---------------------------------------------------------- */

/* echo: FEEDBACK. Lay the previous frame back down, every cell one or more
 * steps dimmer on the ink ramp.
 *
 * This is the technique the whole set was missing. On its own it does nothing
 * visible; under any source it is the difference between a blinking shape and
 * an animation, and it costs one line of pattern. A high amount fades slowly
 * and leaves a long tail; a low one is gone in two frames. */
static void draw_echo(int amt, char dir, uint32_t step)
{
    /* One tone step at nine, four at zero. Nine tones means a trail can
     * actually FADE - eight visible stages between solid and gone - which is
     * the whole reason the ramp is nine and not six.
     *
     * It still has to reach nothing. At no fall at all the frame would fill
     * with everything ever drawn and never clear again, which is feedback with
     * the gain at unity, and a trail that does not end is not a trail. */
    const int fall = 1 + (9 - amt) / 3;          /* 1 step at 9, 4 at 0 */
    for (int y = 0; y < s_h; y++) {
        for (int x = 0; x < s_w; x++) {
            const int lv = tone_of(s_prev[y][x]) - fall;
            if (lv > 0) { s_fb[y][x] = (char)(TONE_0 + lv); }
        }
    }
}

/* ---- motion ----------------------------------------------------------- */

/* move: TRANSFORM. Shift the whole frame, wrapping at the edges.
 *
 * Placed before the sources so that what moves is the HISTORY: the trail
 * streaks away and this frame's source lands fresh at its own position. That
 * ordering is the entire reason echo + move reads as a comet and not as a
 * juddering picture. */
static void draw_move(int amt, char dir, uint32_t step)
{
    int dx, dy;
    delta_of(dir, &dx, &dy);
    const int n = (amt == 9 || amt < 0) ? 1 : (amt + 2) / 3;   /* 1..3 cells */
    dx *= n; dy *= n;
    if (dx == 0 && dy == 0) { return; }

    char tmp[VIZ_H][VIZ_W + 1];
    for (int y = 0; y < s_h; y++) {
        memcpy(tmp[y], s_fb[y], (size_t)s_w + 1);
    }
    for (int y = 0; y < s_h; y++) {
        for (int x = 0; x < s_w; x++) {
            const int sy = ((y - dy) % s_h + s_h) % s_h;
            const int sx = ((x - dx) % s_w + s_w) % s_w;
            s_fb[y][x] = tmp[sy][sx];
        }
    }
}

/* warp: DISPLACE. Slide each line of the frame along the axis by an amount
 * that runs up and down across the frame, so straight things bend.
 *
 * A ramp through warp is a wave; a disc through warp is a lens; noise through
 * warp is water. One primitive, and its output depends entirely on what was
 * drawn before it - which is what an operator is for. The phase advances with
 * the step, so it moves on its own. */
static void draw_warp(int amt, char dir, uint32_t step)
{
    int dx, dy;
    delta_of(dir, &dx, &dy);
    const int span = (amt < 0 ? 9 : amt);
    if (span == 0) { return; }

    char tmp[VIZ_H][VIZ_W + 1];
    for (int y = 0; y < s_h; y++) {
        memcpy(tmp[y], s_fb[y], (size_t)s_w + 1);
        memset(s_fb[y], TONE_0, (size_t)s_w);
    }
    for (int y = 0; y < s_h; y++) {
        for (int x = 0; x < s_w; x++) {
            /* A triangle along the axis being displaced, folded to 0..8..0 so
             * there is no floating point and nothing to look up. */
            const int along = (dy != 0) ? x : y;
            const int t   = (along + (int)step) % 16;
            const int tri = (t < 8) ? t : (16 - t);
            const int sh  = ((tri - 4) * span) / 9;
            const int sy = (dy != 0) ? ((y - sh) % s_h + s_h) % s_h : y;
            const int sx = (dy != 0) ? x : ((x - sh) % s_w + s_w) % s_w;
            s_fb[y][x] = tmp[sy][sx];
        }
    }
}

/* ---- sources ---------------------------------------------------------- */

/* noise: a stochastic field. The amount is how much of it is inked. */
static void draw_noise(int amt, char dir, uint32_t step)
{
    /* SPARKLES, NOT SPECKLE. Scattering tones gives grey mush; scattering the
     * four sparkle glyphs gives a field with things IN it, which is what makes
     * a noise lane worth looking at rather than merely worth measuring. The
     * brighter sparkles are rarer, so the field has a few bright points in a
     * lot of small ones instead of being uniformly loud. */
    const int cells = s_w * s_h * amt / 9;
    for (int i = 0; i < cells; i++) {
        const uint32_t r = rng();
        const uint32_t pick = (r >> 16) % 8u;
        const char ch = (pick < 4u) ? (char)SPARK_FIRST            /* speck  */
                      : (pick < 6u) ? (char)(SPARK_FIRST + 1)      /* star4  */
                      : (pick < 7u) ? (char)(SPARK_FIRST + 2)      /* star8  */
                                    : (char)(SPARK_FIRST + 3);     /* burst  */
        s_fb[r % (uint32_t)s_h][(r >> 8) % (uint32_t)s_w] = ch;
    }
}

/* disc: a filled circle from the centre, radius from the amount. Route this
 * from a kick and the frame breathes on the beat, which is the clearest thing
 * routing does. Through warp it is a box; through fold, a flower. */
static void draw_disc(int amt, char dir, uint32_t step)
{
    const int cx = s_w / 2, cy = s_h / 2;
    /* THE SHORT AXIS BOUNDS THE RADIUS, or it is not a circle.
     *
     * This scaled off the width alone, and the pane is a wide letterbox - 58 by
     * 10 at the compact face - so 'disc 9' asked for a radius of 29 in a frame
     * 10 tall and drew a filled RECTANGLE with four rounded corners. A circle
     * has to fit in both directions. Vertical distance counts double because a
     * cell is twice as tall as it is wide, so the height's reach is s_h, not
     * s_h/2. */
    const int lim = (s_w / 2 < s_h) ? (s_w / 2) : s_h;
    const int r  = amt * lim / 9;
    /* A SMALL DISC IS ONE GLYPH. At radius nothing there is a drawn circle to
     * use - 147 - which reads as a circle where a single '@' read as a blob.
     * This is what the tiles are for: the shape at the size it is wanted. */
    /* A SMALL DISC IS ONE DRAWN GLYPH. At a radius of a cell or less there is
     * nothing to rasterise, and 147 is a circle somebody drew - which reads as
     * a circle where a single solid block read as a blob. This is what having
     * our own shapes buys: the shape at the size it is wanted. */
    if (r <= 1) { s_fb[cy][cx] = (char)147; return; }
    for (int y = 0; y < s_h; y++) {
        /* A CELL IS TWICE AS TALL AS IT IS WIDE on both faces - 12x24 and
         * 6x12 - so a circle that is round in CELLS is a squashed ellipse on
         * the glass. Counting vertical distance double makes it round to the
         * eye, which is the only measure that matters here. */
        const int dy = 2 * (y - cy);
        for (int x = 0; x < s_w; x++) {
            const int dx = x - cx;
            const int d2 = dx * dx + dy * dy;
            if (d2 > r * r) { continue; }
            /* SOLID INSIDE, A LIGHTER TONE ON THE EDGE.
             *
             * The arc glyphs were tried here and are wrong for this: an arc's
             * curvature is one cell, so it only matches a circle about two
             * cells across. On a bigger one every boundary cell got a tight
             * curve the circle does not have, and the result was a double
             * contour rather than an edge. A step down the tone ramp softens
             * the staircase without claiming a curve that is not there - which
             * is what an edge tone is for on a panel with one ink. */
            if (d2 <= (r - 1) * (r - 1)) { s_fb[y][x] = SOLID; continue; }
            s_fb[y][x] = (char)(TONE_0 + TONE_TOP - 2);
        }
    }
}

/* ramp: a gradient across the frame along the given axis, swept to the amount.
 *
 * This is the one that replaces bar, dot and wave, and it replaces all three
 * because all three were the same idea - a mark whose extent is the value -
 * drawn three ways. A ramp at amount 3 is a short bar; through warp it is a
 * wave; through tile it is a row of bars. */
static void draw_ramp(int amt, char dir, uint32_t step)
{
    int dx, dy;
    delta_of(dir, &dx, &dy);
    const int len = (dy != 0) ? s_h : s_w;
    int reach = amt * len / 9;
    /* ANY AMOUNT ABOVE ZERO HAS TO SHOW. In a short frame - eight rows, which
     * is what a stacked split gives at chunky - amt*len/9 floors to zero for
     * every amount below two, so '>viz ramp 1' drew nothing at all and looked
     * like a dead lane. Zero means none; one means the least there is. */
    if (amt > 0 && reach < 1) { reach = 1; }
    for (int i = 0; i < reach; i++) {
        /* Solid at the leading edge, thinning to the faintest tone at the tail.
         * Nine tones make this a gradient; six punctuation marks made it a
         * staircase, and the direction was only visible in the motion. */
        int lv = TONE_TOP - (i * TONE_TOP) / (reach > 1 ? reach - 1 : 1);
        if (lv < 1) { lv = 1; }
        const char ch = (char)(TONE_0 + lv);
        const int at = (dx < 0 || dy < 0) ? (len - 1 - i) : i;
        if (dy != 0) {
            for (int x = 0; x < s_w; x++) { s_fb[at][x] = ch; }
        } else {
            for (int y = 0; y < s_h; y++) { s_fb[y][at] = ch; }
        }
    }
}

/* ---- repetition ------------------------------------------------------- */

/* tile: REPLICATE. Take the leftmost slice of the frame and repeat it across.
 * Instant density from a small source, which is what a replicator is for. */
static void draw_tile(int amt, char dir, uint32_t step)
{
    const int n = 1 + (amt < 0 ? 9 : amt) / 3;   /* 1..4 copies */
    if (n < 2) { return; }
    const int seg = s_w / n;
    if (seg < 1) { return; }
    for (int y = 0; y < s_h; y++) {
        for (int x = seg; x < s_w; x++) {
            s_fb[y][x] = s_fb[y][x % seg];
        }
    }
}

/* fold: MIRROR, one to three times. Left onto right, then top onto bottom,
 * then the left half again - a kaleidoscope out of one line of pattern.
 * It draws nothing of its own, which is the point. */
static void draw_fold(int amt, char dir, uint32_t step)
{
    const int folds = 1 + (amt < 0 ? 9 : amt) / 4;   /* 1..3 */
    for (int f = 0; f < folds; f++) {
        if (f % 2 == 0) {
            for (int y = 0; y < s_h; y++) {
                for (int x = 0; x < s_w / 2; x++) {
                    s_fb[y][s_w - 1 - x] = s_fb[y][x];
                }
            }
        } else {
            for (int y = 0; y < s_h / 2; y++) {
                memcpy(s_fb[s_h - 1 - y], s_fb[y], (size_t)s_w);
            }
        }
    }
}

/* One row per primitive, in the same order as s_names - so the name the owner
 * types and the code that runs cannot drift apart. */
typedef void (*draw_fn)(int amt, char dir, uint32_t step);
static const draw_fn s_draw[NGEN] = {
    draw_echo, draw_move, draw_warp,
    draw_noise, draw_disc, draw_ramp,
    draw_tile, draw_fold,
};

/* What the clock leaves for the main loop: a step number and a flag. Written in
 * the callback, read and cleared in viz_service. */
static volatile uint32_t s_pending_tick;
static volatile bool     s_pending;

/* IS ANY LANE DUE ON THIS TICK? Cheap enough for the callback - eight modulos
 * and no memory traffic - and it is what makes the frame rate right.
 *
 * The clock runs at 96 PPQN, so this is called about 200 times a second at
 * 124 bpm, while a sixteenth-note lane fires eight times a second. Flagging
 * every tick meant the frame was regenerated two hundred times a second and
 * the pane redrawn with it: eight seconds of rendering in every ten, eighty
 * per cent of a core, for twenty-four identical pictures in a row.
 *
 * It was also WRONG, not merely wasteful. The frame is cleared before the
 * lanes are drawn, so on a tick where nothing fires the old code cleared the
 * picture and drew nothing back - the frame was blank between steps, and it
 * only ever looked right because the editor happened to sample it on step
 * boundaries. One frame per step, which is what the device is for, is also
 * the only version that is correct. */
static bool any_lane_due(uint32_t tick)
{
    for (int i = 0; i < NGEN; i++) {
        const vlane_t *l = &s_l[i];
        if (!l->used || l->steps == 0) { continue; }
        const uint32_t tps = l->tps ? l->tps : SEQ_TICKS_PER_STEP;
        if ((tick % tps) == 0) { return true; }
    }
    return false;
}

void viz_tick(uint32_t tick)
{
    if (!any_lane_due(tick)) { return; }
    s_pending_tick = tick;
    s_pending = true;
}

bool viz_service(void)
{
    if (!s_pending) { return false; }
    s_pending = false;
    if (!viz_active()) { return false; }
    viz_frame(s_pending_tick);
    return true;
}

static void viz_frame(uint32_t tick)
{
    if (!viz_active()) { return; }
    /* Keep this frame before it is wiped: echo needs the one before it, and a
     * copy taken here is the only place it is guaranteed to be complete. */
    for (int y = 0; y < s_h; y++) {
        memcpy(s_prev[y], s_fb[y], (size_t)s_w + 1);
    }
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

        /* A letter in the step wins over the lane's direction; a digit or an
         * 'x' leaves the lane's direction alone. */
        const char ch = l->chr[s];
        const char dir = (ch == 'u' || ch == 'd' || ch == 'l' || ch == 'r')
                         ? ch : l->dir;
        s_draw[i](amt, dir, step);
    }
}

int viz_text(char *out, int max)
{
    /* THE WIRE GETS ASCII, THE GLASS GETS THE TILES.
     *
     * The frame is drawn with our own glyphs at 128..155, which mean nothing to
     * anything that is not this device - a receiver on a Pi, an OSC monitor,
     * the host check. So '>frame' translates: each tone back to a step of
     * " .:*#@", each sparkle to '*', an arc to '#'. That is a downsample and is
     * stated as one; the panel is not affected.
     *
     * When there is a receiver that wants the real thing - the RP2040 with the
     * HDMI output - the format to send it is the glyph bytes, and this is the
     * function to add that to. It is not guessed at now, because a wire format
     * invented before its reader is a wire format nobody implements. */
    int n = 0;
    for (int y = 0; y < s_h && n < max - 1; y++) {
        char line[VIZ_W + 2];
        int k = 0;
        for (; k < s_w && k < VIZ_W; k++) {
            const unsigned u = (unsigned char)s_fb[y][k];
            if (u >= TONE_0 && u <= TONE_0 + TONE_TOP) {
                static const char RAMP[] = " ..::*#@@";      /* nine to six */
                line[k] = RAMP[u - TONE_0];
            } else if (u >= SPARK_FIRST && u < SPARK_FIRST + SPARK_N) {
                line[k] = '*';
            } else if (u >= ARC_FIRST && u < ARC_FIRST + 4) {
                line[k] = '#';
            } else if (u >= 128) {
                line[k] = '#';
            } else {
                line[k] = s_fb[y][k];
            }
        }
        line[k] = '\0';
        const int w = snprintf(out + n, (size_t)(max - n), "%s\n", line);
        if (w <= 0) { break; }
        n += w;
    }
    return n;
}
