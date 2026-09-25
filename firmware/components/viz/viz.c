#include "viz.h"

#include <stdio.h>
#include <string.h>

#include "seq.h"
#include "seq_pattern.h"

#define NGEN 16
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
    "echo", "move", "spin", "warp", "shake",   /* history, then motion */
    "noise", "disc", "box", "star", "ramp", "grid",  /* the sources    */
    "grow", "thin", "flip",                    /* shaping             */
    "tile", "fold",                            /* repetition          */
};

/* WHAT THE CLOCK LEFT FOR THE MAIN LOOP.
 *
 * One slot per primitive: was it marked this frame, and with what. That is all
 * that used to require a thirteen-lane table with its own patterns, rates,
 * probabilities, mutes and routes - every one of which was a second copy of
 * something seq already had. The lanes live in seq now; this is the handoff.
 *
 * Volatile because the clock callback writes it and the main loop reads it. A
 * second mark before the frame is drawn overwrites the first, which is correct:
 * the picture is a monitor and the newest value is the only one worth showing. */
typedef struct {
    volatile uint8_t prim;
    volatile uint8_t amt;
    volatile char    dir;
} mark_t;

/* ONE SLOT PER LANE, NOT PER PRIMITIVE.
 *
 * It was one slot per primitive, which silently collapsed instances: '>disc' and
 * '>disc2' both marked the same slot and the second overwrote the first, so two
 * circles drew one. A mark belongs to the lane that made it.
 *
 * Replayed in PRIMITIVE order regardless of arrival order - see viz_service - so
 * the pipeline still runs history, motion, sources, repetition whatever order the
 * lanes were typed in. Instances of the same primitive keep their arrival order
 * among themselves, which is the only order they could sensibly have. */
#define MARK_MAX 16
static mark_t  s_mark[MARK_MAX];
static volatile uint8_t s_nmark;

/* WHERE EACH PRIMITIVE DRAWS. -1 means centred, which is what everything did
 * before positions existed and is what a primitive with no '[x]' lane still does.
 * In cells, not amounts, so the draw functions read a coordinate rather than
 * re-deriving one. */
typedef struct {
    volatile int8_t x, y;        /* -1 = centre */
} place_t;
/* -1 in every slot at startup: a primitive with no '[x]' lane is centred, which is
 * what everything did before positions existed. Zero would be a real column. */
static place_t s_place[NGEN] = {
    [0 ... NGEN - 1] = { .x = -1, .y = -1 },
};

/* Pending parameter marks, applied before anything draws. Separate from the draw
 * marks because a position has to be in place BEFORE the shape that uses it -
 * within one frame the order of the two lanes in the table must not matter. */
typedef struct {
    volatile uint8_t prim, param, amt;
} pmark_t;
static pmark_t s_pmark[MARK_MAX];
static volatile uint8_t s_npmark;
static volatile uint32_t s_mark_tick;
static volatile bool     s_pending;

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
    /* A TRAILING DIGIT IS AN INSTANCE, NOT A DIFFERENT PRIMITIVE. 'disc2' draws a
     * circle; what differs is the lane, not the shape. Same rule as cmd.c's
     * dispatch, and it has to agree with it or '>disc2' would find a command and
     * then fail to find a primitive. */
    size_t n = strlen(g);
    while (n > 1 && g[n - 1] >= '0' && g[n - 1] <= '9') { n--; }
    if (n == strlen(g)) { return -1; }
    for (int i = 0; i < NGEN; i++) {
        if (strlen(s_names[i]) == n && strncmp(s_names[i], g, n) == 0) {
            return i;
        }
    }
    return -1;
}

/* Anything drawing? True while a primitive has been marked and not yet blanked,
 * which is what the split needs to know. The LANES are seq's business. */
static bool s_live;

bool viz_active(void) { return s_live; }

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

void viz_forget_all(void)
{
    s_nmark = 0;
    s_npmark = 0;
    for (int i = 0; i < NGEN; i++) { s_place[i].x = -1; s_place[i].y = -1; }
    s_pending = false;
    s_live = false;
    memset(s_prev, TONE_0, sizeof s_prev);
    clear_frame();
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

/* WHERE THIS PRIMITIVE DRAWS, or the centre when no '[x]' lane has said. The
 * index is passed in rather than looked up because a draw function does not know
 * its own slot - the dispatch table does. */
static int s_drawing;                /* the primitive currently drawing */

static int place_x(void)
{
    const int8_t v = s_place[s_drawing].x;
    return (v < 0) ? (s_w / 2) : (int)v;
}

static int place_y(void)
{
    const int8_t v = s_place[s_drawing].y;
    return (v < 0) ? (s_h / 2) : (int)v;
}

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
    const int cx = place_x(), cy = place_y();
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

/* shake: DISPLACE AT RANDOM, a row at a time. Where warp bends along a smooth
 * curve, this tears - the difference between water and a bad signal, and the
 * two read as completely different material over the same source. */
static void draw_shake(int amt, char dir, uint32_t step)
{
    if (amt <= 0) { return; }
    char tmp[VIZ_H][VIZ_W + 1];
    for (int y = 0; y < s_h; y++) { memcpy(tmp[y], s_fb[y], (size_t)s_w + 1); }
    for (int y = 0; y < s_h; y++) {
        const int sh = (int)(rng() % (uint32_t)(2 * amt + 1)) - amt;
        for (int x = 0; x < s_w; x++) {
            s_fb[y][x] = tmp[y][((x - sh) % s_w + s_w) % s_w];
        }
    }
}

/* grid: a lattice. The amount is how often a line falls, so it goes from a
 * frame around the edge to a dense mesh - and through warp or fold it stops
 * looking like a grid at all, which is the point of having one. */
static void draw_grid(int amt, char dir, uint32_t step)
{
    const int every = 10 - (amt < 1 ? 1 : amt);      /* 9 apart .. 1 apart */
    const char ch = (char)(TONE_0 + TONE_TOP);
    for (int y = 0; y < s_h; y++) {
        for (int x = 0; x < s_w; x++) {
            if ((x % every) == 0 || (y % every) == 0) { s_fb[y][x] = ch; }
        }
    }
}

/* grow: DILATE. Every inked cell spreads to its neighbours, one tone down, so
 * a single speck becomes a bloom and a thin line becomes a stroke. With echo
 * this is how a trail thickens as it fades instead of just dimming. */
static void draw_grow(int amt, char dir, uint32_t step)
{
    if (amt <= 0) { return; }
    char tmp[VIZ_H][VIZ_W + 1];
    for (int y = 0; y < s_h; y++) { memcpy(tmp[y], s_fb[y], (size_t)s_w + 1); }
    for (int y = 0; y < s_h; y++) {
        for (int x = 0; x < s_w; x++) {
            if (tone_of(tmp[y][x]) > 0) { continue; }
            int best = 0;
            if (x > 0)       { const int t = tone_of(tmp[y][x - 1]); if (t > best) best = t; }
            if (x < s_w - 1) { const int t = tone_of(tmp[y][x + 1]); if (t > best) best = t; }
            if (y > 0)       { const int t = tone_of(tmp[y - 1][x]); if (t > best) best = t; }
            if (y < s_h - 1) { const int t = tone_of(tmp[y + 1][x]); if (t > best) best = t; }
            const int lv = best - (10 - amt) / 3 - 1;
            if (lv > 0) { s_fb[y][x] = (char)(TONE_0 + lv); }
        }
    }
}

/* thin: ERODE, the other half of grow. An inked cell with an empty neighbour
 * goes. Run both and you get an outline; run thin alone under echo and a solid
 * shape eats itself from the edges inward. */
static void draw_thin(int amt, char dir, uint32_t step)
{
    if (amt <= 0) { return; }
    char tmp[VIZ_H][VIZ_W + 1];
    for (int y = 0; y < s_h; y++) { memcpy(tmp[y], s_fb[y], (size_t)s_w + 1); }
    for (int y = 0; y < s_h; y++) {
        for (int x = 0; x < s_w; x++) {
            if (tone_of(tmp[y][x]) == 0) { continue; }
            const bool edge =
                (x == 0 || tone_of(tmp[y][x - 1]) == 0) ||
                (x == s_w - 1 || tone_of(tmp[y][x + 1]) == 0) ||
                (y == 0 || tone_of(tmp[y - 1][x]) == 0) ||
                (y == s_h - 1 || tone_of(tmp[y + 1][x]) == 0);
            if (edge) { s_fb[y][x] = (char)TONE_0; }
        }
    }
}

/* flip: INVERT. Ink becomes empty and empty becomes ink, at a tone set by the
 * amount. One line, and the whole frame reads as a negative - which under a
 * pattern is the cheapest strobe there is. */
static void draw_flip(int amt, char dir, uint32_t step)
{
    const int lv = (amt <= 0) ? TONE_TOP : (amt * TONE_TOP + 8) / 9;
    for (int y = 0; y < s_h; y++) {
        for (int x = 0; x < s_w; x++) {
            const int t = tone_of(s_fb[y][x]);
            s_fb[y][x] = (t > 0) ? (char)TONE_0 : (char)(TONE_0 + lv);
        }
    }
}

/* spin: ROTATE. Quarter turns, and the amount says how many.
 *
 * Nothing else rotated. move translates, fold mirrors, warp displaces along an
 * axis - all of them leave orientation alone, so a shape could never turn. That
 * is the axis this brings, and it is why it earns a name where 'edge' and 'dots'
 * below did not.
 *
 * Quarter turns only, deliberately. An arbitrary angle needs interpolation, and
 * on a grid of characters where a cell is twice as tall as it is wide there is no
 * interpolation that does not smear - a 30-degree rotation of ASCII is mush. A
 * quarter turn is exact: it is a transpose and a flip, every cell lands on a cell.
 */
static void draw_spin(int amt, char dir, uint32_t step)
{
    const int turns = (amt < 0 ? 1 : amt) / 3;      /* 0..3 quarter turns */
    if (turns == 0) { return; }
    char tmp[VIZ_H][VIZ_W + 1];
    for (int y = 0; y < s_h; y++) { memcpy(tmp[y], s_fb[y], (size_t)s_w + 1); }
    /* THE FRAME IS NOT SQUARE, so a quarter turn cannot be a straight transpose:
     * a 58x10 picture rotated into a 58x10 window has to be scaled back into it.
     * Sampling the source at the transposed position does that in one pass and
     * costs nothing a bigger buffer would have bought. */
    for (int y = 0; y < s_h; y++) {
        for (int x = 0; x < s_w; x++) {
            int sx, sy;
            switch (turns) {
            case 1:  sx = y * s_w / s_h;             sy = (s_h - 1) - x * s_h / s_w; break;
            case 2:  sx = (s_w - 1) - x;             sy = (s_h - 1) - y;             break;
            default: sx = (s_w - 1) - y * s_w / s_h; sy = x * s_h / s_w;             break;
            }
            if (sx < 0) { sx = 0; } if (sx >= s_w) { sx = s_w - 1; }
            if (sy < 0) { sy = 0; } if (sy >= s_h) { sy = s_h - 1; }
            s_fb[y][x] = tmp[sy][sx];
        }
    }
}

/* box: a RECTANGLE OUTLINE from the centre, size from the amount.
 *
 * Hard corners, where disc is round and grid is a lattice. It was in the first
 * set, folded away during the collapse on the grounds that a disc through warp is
 * nearly a box - and that was wrong: a warped disc has no corners, and a corner
 * is the thing a box is for. */
static void draw_box(int amt, char dir, uint32_t step)
{
    /* THE FULL-SIZE BOX HAS TO FIT. At 1 + amt*(w/2 - 1)/9 the half-width is w/2
     * exactly, so x1 == s_w and the right edge falls off the frame - a rectangle
     * with three sides, which looks like a drawing bug rather than an arithmetic
     * one. Two off the half-width leaves room for both edges at every amount. */
    const int hw = 1 + amt * (s_w / 2 - 2) / 9;
    const int hh = amt * (s_h / 2 - 1) / 9;
    const int x0 = place_x() - hw, x1 = place_x() + hw;
    const int y0 = place_y() - hh, y1 = place_y() + hh;
    const char c = (char)(TONE_0 + TONE_TOP);
    for (int x = (x0 < 0 ? 0 : x0); x <= x1 && x < s_w; x++) {
        if (y0 >= 0)  { s_fb[y0][x] = c; }
        if (y1 < s_h) { s_fb[y1][x] = c; }
    }
    for (int y = (y0 < 0 ? 0 : y0); y <= y1 && y < s_h; y++) {
        if (x0 >= 0)  { s_fb[y][x0] = c; }
        if (x1 < s_w) { s_fb[y][x1] = c; }
    }
}

/* star: SPOKES from the centre, count from the amount.
 *
 * Radial LINES, where disc is a radial area and grid is orthogonal lines. Nothing
 * else draws anything at an angle, which is the axis it brings - and through spin
 * it turns, which is the pair of primitives this set was missing. */
static void draw_star(int amt, char dir, uint32_t step)
{
    const int spokes = 3 + (amt < 0 ? 9 : amt);      /* 3..12 */
    const int cx = place_x(), cy = place_y();
    const int reach = (s_w / 2 < s_h ? s_w / 2 : s_h);
    const char c = (char)(TONE_0 + TONE_TOP);
    for (int k = 0; k < spokes; k++) {
        /* A sine and cosine without either: walk the perimeter of a diamond and
         * draw to each vertex. Even spacing in perimeter is not even in angle,
         * which on a twelve-row picture nobody can tell apart from even in angle.
         */
        const int per = 4 * reach;
        int t = k * per / spokes;
        int ex, ey;
        if (t < reach)          { ex =  reach - t;       ey = -t; }
        else if (t < 2 * reach) { ex = -(t - reach);     ey = -(2 * reach - t); }
        else if (t < 3 * reach) { ex = -(3 * reach - t); ey =  t - 2 * reach; }
        else                    { ex =  t - 3 * reach;   ey =  4 * reach - t; }
        /* Vertical distance counts double, so halve it to keep the star round. */
        const int steps = reach;
        for (int i = 0; i <= steps; i++) {
            const int x = cx + ex * i / steps;
            const int y = cy + (ey / 2) * i / steps;
            if (x >= 0 && x < s_w && y >= 0 && y < s_h) { s_fb[y][x] = c; }
        }
    }
}

/* One row per primitive, in the same order as s_names - so the name the owner
 * types and the code that runs cannot drift apart. */
typedef void (*draw_fn)(int amt, char dir, uint32_t step);
static const draw_fn s_draw[NGEN] = {
    draw_echo,  draw_move, draw_spin, draw_warp, draw_shake,
    draw_noise, draw_disc, draw_box,  draw_star, draw_ramp, draw_grid,
    draw_grow,  draw_thin, draw_flip,
    draw_tile,  draw_fold,
};

/* What the clock leaves for the main loop: a step number and a flag. Written in
 * the callback, read and cleared in viz_service. */

/* ---- the handoff from the clock ---------------------------------------- */

int viz_prim_count(void) { return NGEN; }

const char *viz_prim_name(int i)
{
    return (i >= 0 && i < NGEN) ? s_names[i] : "";
}

int viz_prim_index(const char *name)
{
    return gen_index(name);
}

int viz_param_index(const char *name)
{
    if (name == NULL) { return VIZ_PARAM_NONE; }
    if (strcmp(name, "x") == 0) { return VIZ_PARAM_X; }
    if (strcmp(name, "y") == 0) { return VIZ_PARAM_Y; }
    return VIZ_PARAM_NONE;
}

void viz_mark_param(int prim, int param, int amt)
{
    if (prim < 0 || prim >= NGEN || param == VIZ_PARAM_NONE) { return; }
    const uint8_t n = s_npmark;
    if (n >= MARK_MAX) { return; }
    s_pmark[n].prim  = (uint8_t)prim;
    s_pmark[n].param = (uint8_t)param;
    s_pmark[n].amt   = (uint8_t)(amt < 0 ? 0 : (amt > 9 ? 9 : amt));
    s_npmark = (uint8_t)(n + 1);
    s_pending = true;
}

void viz_mark(int prim, int amt, char dir, uint32_t tick)
{
    if (prim < 0 || prim >= NGEN) {
        return;
    }
    /* RECORD ONLY. This runs in the clock callback. Generating a frame is a
     * pass over the whole picture for every primitive that fired, and doing
     * that between two ticks is the coupling docs/OS.md exists to forbid - it
     * has had to be undone in four other places in this firmware. */
    const uint8_t n = s_nmark;
    if (n >= MARK_MAX) {
        return;                     /* more lanes than marks: cannot happen */
    }
    s_mark[n].prim = (uint8_t)prim;
    s_mark[n].amt  = (uint8_t)(amt < 0 ? 0 : (amt > 9 ? 9 : amt));
    s_mark[n].dir  = dir;
    s_nmark = (uint8_t)(n + 1);
    s_mark_tick = tick;
    s_pending = true;
}

bool viz_service(void)
{
    if (!s_pending) {
        return false;
    }
    s_pending = false;
    const uint32_t tick = s_mark_tick;

    /* Keep this frame before it is wiped: echo needs the one before it, and a
     * copy taken here is the only place it is guaranteed to be complete. */
    memcpy(s_prev, s_fb, sizeof s_prev);
    clear_frame();

    /* PRIMITIVE ORDER, NOT ARRIVAL ORDER, and the order is a pipeline:
     * history, then motion, then sources, then repetition. echo lays the last
     * frame down dimmer, move shifts it so the history streaks while this
     * frame's source lands fresh, then the sources draw, then tile and fold
     * repeat what is there. Replaying in the order the lanes happened to fire
     * would make the same three lines mean something different depending on
     * which order they were typed in. */
    /* POSITIONS FIRST. A '>disc[x]' lane and a '>disc' lane are two lanes in one
     * table, and which comes first there is whichever the player typed first -
     * so the position is applied before anything draws, and the typing order
     * cannot change the picture. */
    const int np = (int)s_npmark;
    s_npmark = 0;
    for (int i = 0; i < np; i++) {
        const int prim = s_pmark[i].prim;
        const int amt  = s_pmark[i].amt;
        if (s_pmark[i].param == VIZ_PARAM_X) {
            s_place[prim].x = (int8_t)(amt * (s_w - 1) / 9);
        } else {
            s_place[prim].y = (int8_t)(amt * (s_h - 1) / 9);
        }
    }

    const int nm = (int)s_nmark;
    s_nmark = 0;
    bool drew = false;
    for (int prim = 0; prim < NGEN; prim++) {
        for (int m = 0; m < nm; m++) {
            if (s_mark[m].prim != (uint8_t)prim) { continue; }
            s_drawing = prim;
            s_draw[prim]((int)s_mark[m].amt, s_mark[m].dir, tick);
            drew = true;
        }
    }
    s_live = drew || s_live;
    return true;
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
