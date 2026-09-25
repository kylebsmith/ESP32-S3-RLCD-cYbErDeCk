#include "viz.h"

#include <stdio.h>
#include <string.h>

#include "seq.h"
#include "seq_pattern.h"

#define NGEN 16
#define NAME_MAX 8

/* SIX FIELDS AND TEN OPERATORS, and the word FIELD is the whole of the third
 * design and the reason there was a third.
 *
 * The first set was eight SHAPES - bar, dot, wave, ring, rain, box, mirror,
 * noise - which is what a system looks like when nobody has decided what the
 * operations are: a ninth shape buys one more picture and nothing else. The
 * second set fixed half of that, few sources through many operators, but the
 * sources stayed shapes, and the owner put a finger on exactly the right one:
 * "star? wtf is that?" A star is not a primitive. It is one picture, its amount
 * is a COUNT OF SPOKES where every other amount in this language is a magnitude,
 * and nothing composes with it - grow makes it a blob, thin erases it, spin on a
 * four-spoke star does nothing at all.
 *
 * So the sources are not shapes now, they are FIELDS. Each one answers "how far
 * is this cell from the thing" in its own geometry, and lays that answer down as
 * a tone:
 *
 *   disc   round distance from the point      euclidean
 *   box    square distance from the point     chebyshev - the corners disc cannot
 *   turn   the ANGLE around the point         the axis star was reaching for
 *   ramp   distance along one direction       linear
 *   grid   distance to the nearest lattice    periodic
 *   noise  no geometry at all                 the entropy, irreducible
 *
 * A shape is then a field plus a THRESHOLD, and the threshold is what was missing:
 *
 *   mask   keep only what is at least this bright    - a LEVEL through the field
 *   edge   keep only where the field changes fast    - a CONTOUR of it
 *
 * Those two are why this set is smaller and does more. A ring is disc + mask. A
 * rectangle outline is box + edge. Spokes are turn + edge. A radar sweep is turn
 * under spin, which the old set could not make at all, and a contour map is ramp
 * + edge, which it could not either. Every one of those used to need its own
 * name, or was simply unreachable.
 *
 * WHAT IT COST, because a set that only grows is a set nobody pruned. Out: star
 * (a count, not a magnitude, and compositionally a dead end), shake (warp with a
 * random displacement instead of a smooth one - reachable by routing warp from
 * noise, which is the same idea stated once instead of twice) and tile (repetition
 * of the frame, where fold already mirrors it and grid now supplies periodicity as
 * a field). Three out, three in, sixteen names either way.
 *
 * TABLE ORDER IS THE DEFAULT DRAW ORDER; 'route' OVERRIDES IT. The table below is
 * a sensible pipeline - memory, motion, fields, shaping - and a document whose
 * lanes are unrouted draws in it regardless of the order the lines were typed,
 * which is a promise tools/test_viz.c checks. But a fixed order is also a ceiling:
 * thin-then-grow despeckles and grow-then-thin closes gaps, and only one of the
 * two was ever reachable. So a routed lane draws AFTER the lane it follows. The
 * order becomes something the document states - '>route thin disc' - rather than
 * something the table decided years earlier. See order_marks(). */
static const char *s_names[NGEN] = {
    "echo", "move", "spin", "warp",            /* memory, then motion  */
    "noise", "disc", "box", "turn", "ramp", "grid",   /* the FIELDS     */
    "mask", "edge",                            /* the THRESHOLDS       */
    "grow", "thin", "flip",                    /* shaping              */
    "fold",                                    /* repetition           */
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

/* ---- the field helpers -------------------------------------------------- */
/* Integer only: there is no floating point in the draw path and there is not going
 * to be, because the whole frame is redrawn every step. */

/* An integer square root, so a distance field can carry a DISTANCE rather than a
 * distance squared - the tone ramp has to be even in distance or the falloff
 * bunches up against the rim. */
static int isqrt_i(int v)
{
    if (v <= 0) { return 0; }
    int r = 0, b = 1 << 14;
    while (b > v) { b >>= 2; }
    while (b != 0) {
        if (v >= r + b) { v -= r + b; r = (r >> 1) + b; }
        else            { r >>= 1; }
        b >>= 2;
    }
    return r;
}

/* THE ANGLE, WITHOUT TRIGONOMETRY. 1024 units to the full turn, clockwise from
 * straight up. This is the "diamond angle": inside each quadrant it interpolates
 * along the perimeter of a square rather than a circle, so it is not the true angle
 * but it is strictly MONOTONIC in it - and monotonic is the entire requirement here,
 * because every use is a comparison. On a twelve-row picture the difference from a
 * real atan2 is not resolvable, and this costs one divide. */
static int turn_of(int dx, int dy)
{
    const int adx = dx < 0 ? -dx : dx;
    const int ady = dy < 0 ? -dy : dy;
    const int sum = adx + ady;
    if (sum == 0) { return 0; }
    if (dy <  0 && dx >= 0) { return   0 + 256 * adx / sum; }   /* up    -> right */
    if (dx >  0 && dy >= 0) { return 256 + 256 * ady / sum; }   /* right -> down  */
    if (dy >  0 && dx <= 0) { return 512 + 256 * adx / sum; }   /* down  -> left  */
    return                          768 + 256 * ady / sum;      /* left  -> up    */
}

/* A FIELD LAID DOWN AS TONES: solid out to two thirds of the reach, then falling to
 * the faintest tone at the rim.
 *
 * Why not a straight gradient from the centre: a field that is grey everywhere is a
 * field nobody wants to look at, and what was asked for was full black squares and
 * sparkles, not a wash. Solid in the middle keeps the punch; the outer third is what
 * 'mask' slices and 'edge' traces, which is what makes a field better than a flat
 * fill rather than merely softer. */
static void ink_field(int d, int reach, int y, int x)
{
    if (reach < 1 || d > reach) { return; }
    const int hard = reach * 2 / 3;
    if (d <= hard) { s_fb[y][x] = SOLID; return; }
    const int span = reach - hard;
    int lv = TONE_TOP - ((d - hard) * (TONE_TOP - 1)) / (span < 1 ? 1 : span);
    if (lv < 1) { lv = 1; }
    s_fb[y][x] = (char)(TONE_0 + lv);
}

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

/* disc: THE ROUND FIELD - euclidean distance from the point, as a tone.
 *
 * The one the owner kept ("diss is fine"), and the only change is that it is now a
 * field rather than a fill: solid out to two thirds of the radius, falling to the
 * faintest tone at the rim. Alone it looks the same as it did. Through 'mask' it is
 * a disc at any size; through 'edge' it is a ring, which used to be its own name
 * and then was unreachable for a while. */
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
    /* A SMALL DISC IS ONE DRAWN GLYPH. At a radius of a cell or less there is
     * nothing to rasterise, and 147 is a circle somebody drew - which reads as
     * a circle where a single solid block read as a blob. This is what having
     * our own shapes buys: the shape at the size it is wanted. */
    if (r <= 1) { s_fb[cy][cx] = (char)147; return; }
    for (int y = 0; y < s_h; y++) {
        const int dy = 2 * (y - cy);
        for (int x = 0; x < s_w; x++) {
            const int dx = x - cx;
            ink_field(isqrt_i(dx * dx + dy * dy), r, y, x);
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

/* box: THE SQUARE FIELD - chebyshev distance from the point, as a tone.
 *
 * The corners disc cannot have. It was an OUTLINE, which is one picture; as a field
 * it is the outline through 'edge', a filled square on its own, a smaller square
 * through 'mask', and the thing that puts a right angle into anything it is masked
 * against. The old version also had an arithmetic bug worth remembering - the right
 * edge fell off the frame at full size, a rectangle with three sides - and a field
 * cannot have that bug, because it never draws an edge in the first place.
 *
 * Distance is the LARGER of the two axes, which is what makes it square where disc
 * takes the root of their sum and comes out round. One line apart; two geometries. */
static void draw_box(int amt, char dir, uint32_t step)
{
    const int cx = place_x(), cy = place_y();
    const int lim = (s_w / 2 < s_h) ? (s_w / 2) : s_h;
    const int r = amt * lim / 9;
    if (r < 1) { s_fb[cy][cx] = SOLID; return; }
    for (int y = 0; y < s_h; y++) {
        const int ady = 2 * (y - cy) < 0 ? -2 * (y - cy) : 2 * (y - cy);
        for (int x = 0; x < s_w; x++) {
            const int adx = (x - cx) < 0 ? -(x - cx) : (x - cx);
            ink_field(adx > ady ? adx : ady, r, y, x);
        }
    }
}

/* turn: THE ANGLE AROUND THE POINT, swept from the given direction.
 *
 * This is what 'star' was reaching for and could not hold. Star's amount was a COUNT
 * OF SPOKES, three to twelve - the only amount in the language that was not a
 * magnitude - and it composed with nothing: grow made it a blob, thin erased it, spin
 * on a four-spoke star did nothing at all. An angle field is a magnitude. The amount
 * is how much of the circle the sweep covers, solid at its leading edge and fading
 * behind it, which is exactly what 'ramp' does along a straight line.
 *
 * What that buys, none of which the old set could make:
 *   turn 2  under spin           a radar sweep - and with echo, one with a tail
 *   turn 9  through edge         spokes, at whatever count the contour finds
 *   turn 3  through mask         a hard-edged wedge
 *   turn    masked against disc  a pie slice */
static void draw_turn(int amt, char dir, uint32_t step)
{
    if (amt <= 0) { return; }
    const int cx = place_x(), cy = place_y();
    const int start = (dir == 'r') ? 256 : (dir == 'd') ? 512
                    : (dir == 'l') ? 768 : 0;
    int sweep = amt * 1024 / 9;
    if (sweep < 1) { sweep = 1; }
    for (int y = 0; y < s_h; y++) {
        /* Vertical distance counts double, as everywhere else here: a cell is twice
         * as tall as it is wide, so an angle measured in cells is not the angle the
         * eye sees. */
        const int dy = 2 * (y - cy);
        for (int x = 0; x < s_w; x++) {
            const int dx = x - cx;
            if (dx == 0 && dy == 0) { s_fb[y][x] = SOLID; continue; }
            const int rel = (turn_of(dx, dy) - start + 1024) & 1023;
            if (rel >= sweep) { continue; }
            int lv = TONE_TOP - (rel * TONE_TOP) / sweep;
            if (lv < 1) { lv = 1; }
            s_fb[y][x] = (char)(TONE_0 + lv);
        }
    }
}

/* mask: A LEVEL THROUGH WHATEVER IS THERE. Keep the cells at least this bright and
 * clear the rest.
 *
 * The operator every field was missing, and the reason fields are worth having at
 * all. A source used to bake its own hard edge in, so the only shape it could make
 * was the one its author chose. With a level, the same disc is a disc at any size
 * the mask picks, a ramp becomes a hard bar wherever the level crosses it, and a
 * noise field becomes sparse specks instead of grey mush - and the level is a lane,
 * so it moves. */
static void draw_mask(int amt, char dir, uint32_t step)
{
    /* THE AMOUNT IS A DIGIT; THE RAMP HAS NINE STEPS. Nine tones means tone 8 is
     * solid, so a level taken straight from the digit asked for "at least 9" and
     * kept nothing at all - 'mask 9' cleared the frame. Map the digit onto the ramp
     * instead: 1 keeps anything inked, 9 keeps only solid. */
    const int d = amt < 1 ? 1 : (amt > 9 ? 9 : amt);
    const int keep = 1 + (d - 1) * (TONE_TOP - 1) / 8;
    for (int y = 0; y < s_h; y++) {
        for (int x = 0; x < s_w; x++) {
            if (tone_of(s_fb[y][x]) < keep) { s_fb[y][x] = (char)TONE_0; }
        }
    }
}

/* edge: A CONTOUR OF WHATEVER IS THERE. Keep a cell only where the tone beside it
 * drops off sharply.
 *
 * The other half of the threshold idea, and the one that makes the shapes that used
 * to need their own names. A rectangle outline is 'box edge' - which is all 'box'
 * ever was, so box could become a field and lose nothing. A ring is 'disc edge'.
 * Spokes are 'turn edge'. A contour map is 'ramp edge', which nothing in the old set
 * could draw at all.
 *
 * The amount is how steep a drop counts: 9 finds every tone step and gives dense
 * contours, 1 finds only ink against nothing and gives one crisp outline. More is
 * more, which is what a digit means everywhere else in this language. */
static void draw_edge(int amt, char dir, uint32_t step)
{
    /* HOW STEEP A DROP COUNTS, and it has to fit in the ramp: at 10 - amt the
     * gentlest setting asked for a fall of nine tones where the ramp only has eight,
     * so 'edge 1' found nothing anywhere. 9 - amt spans the ramp exactly. */
    int drop = 9 - (amt < 1 ? 1 : (amt > 9 ? 9 : amt));
    if (drop < 1) { drop = 1; }
    char tmp[VIZ_H][VIZ_W + 1];
    for (int y = 0; y < s_h; y++) { memcpy(tmp[y], s_fb[y], (size_t)s_w + 1); }
    for (int y = 0; y < s_h; y++) {
        for (int x = 0; x < s_w; x++) {
            const int t = tone_of(tmp[y][x]);
            if (t <= 0) { s_fb[y][x] = (char)TONE_0; continue; }
            int lo = t;
            if (x > 0)       { const int n = tone_of(tmp[y][x - 1]); if (n < lo) { lo = n; } }
            if (x < s_w - 1) { const int n = tone_of(tmp[y][x + 1]); if (n < lo) { lo = n; } }
            if (y > 0)       { const int n = tone_of(tmp[y - 1][x]); if (n < lo) { lo = n; } }
            if (y < s_h - 1) { const int n = tone_of(tmp[y + 1][x]); if (n < lo) { lo = n; } }
            /* OFF THE FRAME COUNTS AS EMPTY, so a fill reaching the border still gets
             * an outline along it rather than vanishing there. */
            if (x == 0 || y == 0 || x == s_w - 1 || y == s_h - 1) { lo = 0; }
            /* INK AGAINST NOTHING IS ALWAYS AN EDGE, whatever the amount.
             *
             * Without this there is no crisp outline at any setting, because a field
             * fades to its faintest tone at the rim: the outermost inked cell differs
             * from the emptiness beyond it by one step, exactly like every internal
             * step, so a threshold that keeps the outline keeps the whole rim with it
             * and a disc through edge came out as a thick band rather than a ring.
             * The boundary of the inked region is a different KIND of edge and is
             * treated as one; the amount then decides how much of the interior joins
             * it. So 1 is one outline and 9 is a contour map. */
            const bool against_nothing = (lo == 0 && t > 0);
            s_fb[y][x] = (against_nothing || t - lo >= drop) ? SOLID : (char)TONE_0;
        }
    }
}

/* One row per primitive, in the same order as s_names - so the name the owner
 * types and the code that runs cannot drift apart. */
typedef void (*draw_fn)(int amt, char dir, uint32_t step);
static const draw_fn s_draw[NGEN] = {
    draw_echo,  draw_move, draw_spin, draw_warp,
    draw_noise, draw_disc, draw_box,  draw_turn, draw_ramp, draw_grid,
    draw_mask,  draw_edge,
    draw_grow,  draw_thin, draw_flip,
    draw_fold,
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

/* WHERE 'route' BECAME THE PIPELINE.
 *
 * The draw order used to be the order of the table above, full stop, and that was a
 * ceiling on the whole visual half of this instrument. thin-then-grow despeckles a
 * noisy frame; grow-then-thin closes the gaps in a broken line. They are different
 * pictures and only one of them was ever reachable, because the table put grow
 * before thin and nothing a performer could type changed it. Sixteen primitives in a
 * frozen chain is not composition - it is a mixer with sixteen mute buttons.
 *
 * 'route' already states order in the document: '>route thin disc' says the erosion
 * follows the circle. So a lane's RANK is how many route hops it is from a lane that
 * follows nothing, and a frame draws rank 0 first, then rank 1, and so on. Inside a
 * rank the table order still decides, which keeps the promise tools/test_viz.c
 * checks - a document of unrouted lanes draws the same whatever order the lines were
 * typed in. Order is now something a performer can state and could not state before,
 * and nothing that worked before behaves differently.
 *
 * Cycles cannot happen - seq_route refuses a self-route and the chain is walked at
 * most SEQ_MAX_LANES times - but the walk is bounded anyway, because a draw loop that
 * can spin is worse than a wrong order. */
static void chain_ranks(int *rank)
{
    for (int i = 0; i < NGEN; i++) { rank[i] = 0; }
    int n = 0;
    const seq_lane_t *lanes = seq_lanes(&n);
    if (lanes == NULL) { return; }
    /* THE WHOLE TABLE, TESTING `used`. `n` is how many lanes are in use, not an
     * index bound: forgetting a lane empties its slot in place, so after one '>disc'
     * the table has a hole and a loop to `n` stopped short of the last lane - which
     * then drew at rank 0, first, and a close became a despeckle. seq.c's own
     * lane_find() carries the same warning; this loop had not read it. */
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        if (!lanes[i].used) { continue; }
        if (lanes[i].bind != SEQ_BIND_VIZ || lanes[i].param != 0) { continue; }
        const int prim = lanes[i].prim;
        if (prim < 0 || prim >= NGEN) { continue; }
        /* Walk up this lane's route chain, counting hops. */
        int hops = 0;
        const char *up = lanes[i].route;
        while (up != NULL && up[0] != '\0' && hops < SEQ_MAX_LANES + 1) {
            int next = -1;
            for (int j = 0; j < SEQ_MAX_LANES; j++) {
                if (lanes[j].used && strcmp(lanes[j].name, up) == 0) {
                    next = j;
                    break;
                }
            }
            if (next < 0) { break; }            /* follows a lane that is gone */
            hops++;
            up = lanes[next].route;
        }
        /* An instance deeper in a chain pulls its primitive down with it: two lanes
         * on one primitive draw together, and the later position is the safe one. */
        if (hops > rank[prim]) { rank[prim] = hops; }
    }
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
    /* HOW FAR DOWN A ROUTE CHAIN EACH PRIMITIVE SITS, and then the table order
     * within a rank. See order_marks: this is where 'route' became the thing that
     * states the pipeline. */
    int rank[NGEN];
    chain_ranks(rank);
    for (int r = 0; r <= NGEN; r++) {
        for (int prim = 0; prim < NGEN; prim++) {
            if (rank[prim] != r) { continue; }
            for (int m = 0; m < nm; m++) {
                if (s_mark[m].prim != (uint8_t)prim) { continue; }
                s_drawing = prim;
                s_draw[prim]((int)s_mark[m].amt, s_mark[m].dir, tick);
                drew = true;
            }
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
