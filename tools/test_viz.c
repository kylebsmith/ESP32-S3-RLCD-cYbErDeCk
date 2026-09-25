/*
 * The drawing half, on the host.
 *
 * WHAT CHANGED AND WHY THIS FILE SHRANK. viz used to own a lane table -
 * patterns, rates, probabilities, mutes, routes - every field of which was a
 * second copy of something seq already had. That table is gone: a drawing lane
 * lives in seq's one lane table with SEQ_BIND_VIZ, and what is left here is the
 * drawing. So the tests for patterns, mutes and routing left with it, and what
 * remains tests the picture.
 *
 * The lane behaviour those tests covered is not untested, it is tested
 * elsewhere: seq.c pulls in esp_timer, FreeRTOS queues and a pinned task, so it
 * cannot be compiled on the host, and its lane logic is verified on the device.
 * Saying that plainly is better than leaving a file here that tests a copy of
 * the sequencer nobody runs.
 *
 * Four things below are promises rather than mechanics, and each one breaks
 * silently - you get a picture that looks a bit wrong on a 30-column panel and
 * no error anywhere:
 *
 *   1. Every name in the help text resolves to a primitive.
 *   2. s_names and s_draw are in the SAME order. Two literals in two places;
 *      nothing in C makes them agree, and if they slip then '>box' draws rain.
 *   3. The pipeline runs in TABLE order, not the order the lanes fired. This is
 *      the new one, and it is what the collapse has to guarantee: three lines
 *      of a document must mean the same thing whatever order they were typed.
 *   4. echo only ever fades, and reaches nothing.
 */
#include <stdio.h>
#include <string.h>

#include "seq.h"
#include "viz.h"

static int fails;
#define CHECK(cond, ...) do {                                             \
    if (!(cond)) { printf("  [FAIL] " __VA_ARGS__); printf("\n"); fails++; } \
    else         { printf("  [ ok ] " __VA_ARGS__); printf("\n"); }        \
} while (0)

/* THE PUBLISHED FRAME, NOT THE INTERNAL ONE. viz draws with our own glyphs at
 * 128..155, where an EMPTY cell is the empty TONE rather than a space - so a
 * test looking for ' ' would find ink in every cell and pass nothing.
 * viz_text() is the frame as the rest of the world receives it, in plain ASCII,
 * so checking that also covers the translation instead of trusting it. */
static char g_rows[VIZ_H][VIZ_W + 2];
static int  g_nrows;

static void snap(void)
{
    static char buf[VIZ_H * (VIZ_W + 2) + 4];
    const int n = viz_text(buf, (int)sizeof buf);
    g_nrows = 0;
    int k = 0;
    for (int i = 0; i < n && g_nrows < VIZ_H; i++) {
        if (buf[i] == '\n') {
            g_rows[g_nrows][k] = '\0';
            g_nrows++;
            k = 0;
        } else if (k < VIZ_W) {
            g_rows[g_nrows][k++] = buf[i];
        }
    }
}

static const char *row_at(int y)
{
    return (y >= 0 && y < g_nrows) ? g_rows[y] : "";
}

static int frame_ink(void)
{
    int n = 0;
    for (int y = 0; y < viz_rows(); y++) {
        const char *r = row_at(y);
        for (int x = 0; x < viz_cols() && r[x] != '\0'; x++) {
            n += r[x] != ' ';
        }
    }
    return n;
}

static void blank(void)
{
    viz_forget_all();
    snap();
}

/* Fire a primitive exactly the way the sequencer does: the clock marks it, the
 * main loop draws. Two calls, because that separation is the point - generating
 * a frame inside a timer callback is what docs/OS.md forbids. */
static void mark(const char *prim, int amt, char dir)
{
    viz_mark(viz_prim_index(prim), amt, dir, 0);
}

/* THE LANE TABLE, FAKED - which is not a compromise, it is the only way to test
 * the thing it is here for.
 *
 * viz.c asks seq for the lanes so it can read their ROUTES and draw a routed lane
 * after the lane it follows. This test links viz.c on its own, so it has to provide
 * that table; and providing it means the route order can be stated exactly and the
 * resulting picture checked, which is not possible on the device without two hands
 * and a camera. */
static seq_lane_t g_lanes[SEQ_MAX_LANES];
static int        g_nlanes;

/* EXACTLY WHAT THE REAL ONE RETURNS: the whole array, and the count of lanes in
 * USE. The array is sparse - forgetting a lane clears `used` in place - so the
 * count is not a bound on the index. This shim used to return a dense table and
 * its length, which is the one shape in which a loop bounded by the count is
 * correct; that is how chain_ranks() got away with one. */
const seq_lane_t *seq_lanes(int *count)
{
    if (count != NULL) {
        int n = 0;
        for (int i = 0; i < SEQ_MAX_LANES; i++) { n += g_lanes[i].used; }
        *count = n;
    }
    return g_lanes;
}

static void no_lanes(void)
{
    memset(g_lanes, 0, sizeof g_lanes);
    g_nlanes = 0;
}

/* One drawing lane on `prim`, optionally following `follows`. */
static void lane(const char *prim, const char *follows)
{
    if (g_nlanes >= SEQ_MAX_LANES) { return; }
    seq_lane_t *l = &g_lanes[g_nlanes++];
    memset(l, 0, sizeof *l);
    snprintf(l->name, sizeof l->name, "%s", prim);
    l->used  = true;
    l->bind  = SEQ_BIND_VIZ;
    l->prim  = (uint8_t)viz_prim_index(prim);
    l->param = 0;
    if (follows != NULL) { snprintf(l->route, sizeof l->route, "%s", follows); }
}

/* A slot a forgotten lane left behind: not in use, and whatever it held. */
static void hole(const char *was)
{
    if (g_nlanes >= SEQ_MAX_LANES) { return; }
    seq_lane_t *l = &g_lanes[g_nlanes++];
    memset(l, 0, sizeof *l);
    snprintf(l->name, sizeof l->name, "%s", was);
    l->bind = SEQ_BIND_VIZ;
    l->prim = (uint8_t)viz_prim_index(was);
}

int main(void)
{
    /* A stated size, not the default: every count below is a fraction of the
     * rectangle, so the rectangle has to be part of the test. */
    viz_size(32, 12);
    snap();

    /* 1. THE NAMES IN THE HELP TEXT, and the table behind them. */
    static const char *named[] = { "echo", "move", "spin", "warp",
                                   "noise", "disc", "box", "turn", "ramp", "grid",
                                   "mask", "edge",
                                   "grow", "thin", "flip", "fold" };
    const int N = (int)(sizeof named / sizeof *named);
    printf("-- every name the help offers resolves --\n");
    CHECK(viz_prim_count() == N, "%d primitives, %d names", viz_prim_count(), N);
    for (int i = 0; i < N; i++) {
        CHECK(viz_prim_index(named[i]) >= 0, "%s", named[i]);
    }
    CHECK(viz_prim_index("sparkle") < 0, "an unknown name is refused");

    /* A TRAILING DIGIT IS AN INSTANCE. 'disc2' is a second circle, not a second
     * primitive, so it resolves to the same index - and two instances must not
     * collapse into one mark, which is what a per-primitive mark table did. */
    CHECK(viz_prim_index("disc2") == viz_prim_index("disc"),
          "disc2 is an instance of disc");
    CHECK(viz_prim_index("echo9") == viz_prim_index("echo"),
          "echo9 is an instance of echo");
    CHECK(viz_prim_index("2") < 0, "a bare number is not a primitive");

    /* The index a name resolves to must be the slot that name occupies, or the
     * two tables have slipped and every primitive draws its neighbour. */
    int slipped = 0;
    for (int i = 0; i < N; i++) {
        if (strcmp(viz_prim_name(i), named[i]) != 0) { slipped++; }
        if (viz_prim_index(named[i]) != i)           { slipped++; }
    }
    CHECK(slipped == 0, "name and index agree for all %d (%d slipped)", N, slipped);

    /* 2. THE SOURCES DRAW. A name that resolves and then puts nothing in the
     *    frame is the failure a lookup test cannot see. */
    printf("\n-- each source draws --\n");
    static const char *sources[] = { "noise", "disc", "box", "turn", "ramp",
                                     "grid" };
    for (unsigned i = 0; i < sizeof sources / sizeof *sources; i++) {
        blank();
        mark(sources[i], 9, 'd');
        viz_service();
        snap();
        CHECK(frame_ink() > 0, "%s put %d cells down", sources[i], frame_ink());
    }

    /* disc at full amount is FILLED and centred, which is what tells it from
     * ramp if the table rows ever swap. */
    blank();
    mark("disc", 9, 'd');
    viz_service(); snap();
    CHECK(row_at(viz_rows() / 2)[viz_cols() / 2] != ' ', "disc is filled");

    /* A ramp runs along its axis: 'd' inks whole ROWS from the top. */
    blank();
    mark("ramp", 4, 'd');
    viz_service(); snap();
    {
        int top = 0, bot = 0;
        for (int x = 0; x < viz_cols(); x++) {
            top += row_at(0)[x] != ' ';
            bot += row_at(viz_rows() - 1)[x] != ' ';
        }
        CHECK(top > 0 && bot == 0, "ramp 4 fills from the top (%d/%d)", top, bot);
    }

    /* 3. THE OPERATORS CHANGE WHAT IS THERE AND DRAW NOTHING THEMSELVES. flip
     *    is absent on purpose: inverting an empty frame FILLS it, which is
     *    correct, and it is the one operator that draws on nothing. */
    printf("\n-- operators alone leave the frame empty --\n");
    static const char *ops[] = { "echo", "move", "spin", "warp",
                                 "grow", "thin", "mask", "edge", "fold" };
    for (unsigned i = 0; i < sizeof ops / sizeof *ops; i++) {
        blank();
        mark(ops[i], 9, 'd');
        viz_service(); snap();
        mark(ops[i], 9, 'd');
        viz_service(); snap();          /* twice, so echo has a past to work on */
        CHECK(frame_ink() == 0, "%s alone draws nothing", ops[i]);
    }

    /* 4. THE PIPELINE IS TABLE ORDER, NOT MARK ORDER.
     *
     *    This is the guarantee the collapse has to make. The marks arrive in
     *    whatever order the lanes happen to sit in seq's table, which is the
     *    order the player typed them - and the picture must not depend on that.
     *    So: mark fold FIRST and noise SECOND, the reverse of the table, and
     *    fold must still run last and leave the frame symmetric. If it ran when
     *    it was marked it would fold an empty frame and do nothing. */
    printf("\n-- the pipeline runs in table order, not mark order --\n");
    blank();
    mark("noise", 5, 'd');
    viz_service(); snap();
    int asym = 0;
    for (int y = 0; y < viz_rows(); y++) {
        for (int x = 0; x < viz_cols() / 2; x++) {
            asym += row_at(y)[x] != row_at(y)[viz_cols() - 1 - x];
        }
    }
    CHECK(asym > 0, "noise alone is not symmetric (%d cells differ)", asym);

    blank();
    mark("fold", 1, 'd');               /* marked FIRST, sits LAST in the table */
    mark("noise", 5, 'd');              /* marked SECOND, sits EARLIER          */
    viz_service(); snap();
    int sym = 1, ink = frame_ink();
    for (int y = 0; y < viz_rows(); y++) {
        for (int x = 0; x < viz_cols() / 2; x++) {
            if (row_at(y)[x] != row_at(y)[viz_cols() - 1 - x]) { sym = 0; }
        }
    }
    CHECK(ink > 0, "both drew (%d cells)", ink);
    CHECK(sym, "fold ran after noise despite being marked before it");

    /* 5. ECHO FADES, AND REACHES NOTHING. At no decay the frame would fill with
     *    everything ever drawn and never clear - feedback with the gain at
     *    unity - and a trail that does not end is not a trail. */
    printf("\n-- echo keeps the last frame, one tone dimmer --\n");
    blank();
    mark("disc", 9, 'd');
    viz_service(); snap();
    const int lit = frame_ink();
    CHECK(lit > 0, "the disc drew %d cells", lit);

    mark("echo", 9, 'd');               /* echo only: no source any more */
    viz_service(); snap();
    const int after1 = frame_ink();
    CHECK(after1 > 0, "one frame later %d cells survive", after1);
    CHECK(after1 <= lit, "and no more than were there (%d <= %d)", after1, lit);

    int prev = after1, grew = 0, k;
    for (k = 2; k < 200 && prev > 0; k++) {
        mark("echo", 9, 'd');
        viz_service(); snap();
        const int now = frame_ink();
        if (now > prev) { grew = 1; break; }
        prev = now;
    }
    CHECK(!grew, "it only ever fades, never grows");
    CHECK(prev == 0, "and empties after %d frames (%d left)", k, prev);

    /* 6. MOTION RUNS BEFORE THE SOURCES, which is the whole reason echo plus
     *    move reads as a trail: the history is shifted and the new source lands
     *    fresh at its own place. */
    printf("\n-- move shifts the history, not the source --\n");
    blank();
    viz_size(16, 8);
    mark("ramp", 1, 'u');               /* one row at the BOTTOM */
    viz_service(); snap();
    int first_row = -1;
    for (int y = 0; y < viz_rows() && first_row < 0; y++) {
        for (int x = 0; x < viz_cols(); x++) {
            if (row_at(y)[x] != ' ') { first_row = y; break; }
        }
    }
    CHECK(first_row >= 0, "the ramp inked row %d", first_row);

    for (int f = 0; f < 2; f++) {       /* history travels up, no new ink */
        mark("echo", 9, 'd');
        mark("move", 9, 'u');
        viz_service(); snap();
    }
    int highest = 99;
    for (int y = 0; y < viz_rows(); y++) {
        for (int x = 0; x < viz_cols(); x++) {
            if (row_at(y)[x] != ' ' && y < highest) { highest = y; }
        }
    }
    CHECK(highest < first_row, "two frames on, the trail reached row %d, above %d",
          highest, first_row);
    viz_size(32, 12);
    snap();

    /* 6b. THE FIELDS AND THE THRESHOLDS.
     *
     *     The set used to be shapes, and a shape is one picture: 'box' drew a
     *     rectangle outline and could draw nothing else, 'star' drew spokes and its
     *     amount was a COUNT rather than a magnitude. They are fields now, and the
     *     claim being tested is the one that justified the change - that a field
     *     through a threshold reaches shapes no name in the old set could.
     *
     *     Each of these fails on the old code by construction, because the names it
     *     uses did not resolve there. */
    printf("\n-- fields and thresholds --\n");
    no_lanes();
    CHECK(viz_prim_index("turn") >= 0, "turn resolves");
    CHECK(viz_prim_index("mask") >= 0, "mask resolves");
    CHECK(viz_prim_index("edge") >= 0, "edge resolves");
    CHECK(viz_prim_index("star") < 0,  "star is gone");
    CHECK(viz_prim_index("shake") < 0, "shake is gone");
    CHECK(viz_prim_index("tile") < 0,  "tile is gone");

    /* A RING, which is the headline: a filled disc traced by 'edge' has ink, and
     * has NO ink at its own centre. The old set could not draw this at all - 'ring'
     * was removed in the first collapse and nothing replaced it. */
    blank();
    mark("disc", 8, 'd');
    viz_service(); snap();
    const int solid = frame_ink();
    CHECK(row_at(viz_rows() / 2)[viz_cols() / 2] != ' ', "the disc has a centre");

    blank();
    mark("disc", 8, 'd');
    mark("edge", 1, 'd');
    viz_service(); snap();
    const int ring = frame_ink();
    CHECK(ring > 0, "disc through edge draws something (%d cells)", ring);
    CHECK(ring < solid / 2, "and it is an outline: %d of %d cells", ring, solid);
    CHECK(row_at(viz_rows() / 2)[viz_cols() / 2] == ' ',
          "the middle is EMPTY - that is what makes it a ring");

    /* A LEVEL. Masking a field keeps its core and drops its rim, so the same disc
     * comes out smaller - which is how a mask lane resizes a shape whose own amount
     * never changed. */
    blank();
    mark("disc", 9, 'd');
    mark("mask", 9, 'd');
    viz_service(); snap();
    const int masked = frame_ink();
    CHECK(masked > 0 && masked < solid,
          "mask 9 kept %d cells of a %d-cell field", masked, solid);

    /* THE ANGLE IS A MAGNITUDE, which is the whole complaint about 'star'. More
     * sweep is more ink, and the sweep starts where the direction says. */
    blank(); mark("turn", 2, 'u'); viz_service(); snap();
    const int wedge = frame_ink();
    blank(); mark("turn", 9, 'u'); viz_service(); snap();
    const int full_sweep = frame_ink();
    CHECK(wedge > 0, "turn 2 draws a wedge (%d cells)", wedge);
    CHECK(full_sweep > wedge * 2, "turn 9 is much more than turn 2 (%d vs %d)",
          full_sweep, wedge);

    blank(); mark("turn", 2, 'u'); viz_service(); snap();
    char up_row[VIZ_W + 2];
    snprintf(up_row, sizeof up_row, "%s", row_at(1));
    blank(); mark("turn", 2, 'd'); viz_service(); snap();
    CHECK(strcmp(up_row, row_at(1)) != 0, "a sweep up and a sweep down differ");

    /* 6c. ROUTE SETS DRAW ORDER, which was the ceiling on all of this.
     *
     *     thin-then-grow despeckles; grow-then-thin closes gaps. Both were one
     *     table entry away and only one was reachable. The check: the same two marks
     *     in the same frame, with the route reversed, must produce DIFFERENT
     *     pictures. On the old code they could not, by construction - the table
     *     decided - so this fails there whatever it is given. */
    printf("\n-- route sets the draw order --\n");
    blank();
    no_lanes();
    lane("noise", NULL);
    lane("grow", "noise");              /* noise, then grow */
    lane("thin", "grow");               /* then thin: a close */
    mark("grid", 2, 'd'); mark("grow", 9, 'd'); mark("thin", 9, 'd');
    viz_service(); snap();
    char closed[VIZ_H][VIZ_W + 2];
    for (int y = 0; y < viz_rows(); y++) {
        snprintf(closed[y], sizeof closed[y], "%s", row_at(y));
    }
    const int closed_ink = frame_ink();

    blank();
    no_lanes();
    lane("noise", NULL);
    lane("thin", "noise");              /* noise, then thin */
    lane("grow", "thin");               /* then grow: a despeckle */
    mark("grid", 2, 'd'); mark("grow", 9, 'd'); mark("thin", 9, 'd');
    viz_service(); snap();
    const int opened_ink = frame_ink();

    int rows_differ = 0;
    for (int y = 0; y < viz_rows(); y++) {
        if (strcmp(closed[y], row_at(y)) != 0) { rows_differ++; }
    }
    CHECK(rows_differ > 0,
          "grow-then-thin and thin-then-grow differ (%d rows, %d vs %d cells)",
          rows_differ, closed_ink, opened_ink);

    /* AND A HOLE IN THE TABLE MUST NOT CHANGE IT. After a lane is forgotten its
     * slot is empty and the table is sparse. chain_ranks() walked lanes[0..count)
     * where count is the lanes IN USE, so with a hole at the front it never
     * reached the last lane - 'thin' fell to rank 0 and drew first, and the close
     * became a despeckle. The same three routes, one hole: the same picture. */
    blank();
    no_lanes();
    hole("flip");
    lane("noise", NULL);
    lane("grow", "noise");
    lane("thin", "grow");
    mark("grid", 2, 'd'); mark("grow", 9, 'd'); mark("thin", 9, 'd');
    viz_service(); snap();
    int holed_differ = 0;
    for (int y = 0; y < viz_rows(); y++) {
        if (strcmp(closed[y], row_at(y)) != 0) { holed_differ++; }
    }
    CHECK(holed_differ == 0,
          "a forgotten lane's hole does not change the draw order (%d rows off)",
          holed_differ);

    /* AND AN UNROUTED DOCUMENT IS STILL ORDER-INDEPENDENT. Promise 3 at the top of
     * this file, which the route ranking must not have cost: with no routes every
     * lane is rank 0 and the table decides, exactly as before. */
    no_lanes();
    blank();
    /* A DETERMINISTIC SOURCE, because this compares two cell COUNTS. 'noise' was
     * used here first and the check failed on correct code: two runs of a random
     * field are two different fields, so the counts differed for a reason that has
     * nothing to do with order. */
    mark("grid", 2, 'd'); mark("grow", 9, 'd'); mark("thin", 9, 'd');
    viz_service(); snap();
    const int a_order = frame_ink();
    blank();
    mark("thin", 9, 'd'); mark("grow", 9, 'd'); mark("grid", 2, 'd');
    viz_service(); snap();
    CHECK(frame_ink() == a_order,
          "unrouted, marking order does not matter (%d == %d)",
          a_order, frame_ink());

    /* 7. THE PANE. Two failures the owner saw, both invisible to a lookup test:
     *    a side-by-side split on a thirty-column grid left twelve columns for
     *    the code and wrapped every pattern line, and a frame shorter than its
     *    pane left the cells underneath holding the last layout's glyphs.
     *
     *    A cell is twice as tall as it is wide, so a pane that is taller than it
     *    is wide IN CELLS is a portrait sliver on the glass. Every pane must be
     *    wider than it is tall. */
    printf("\n-- the preview pane --\n");
    viz_split(true);

    const viz_pane_t wide = viz_pane(60, 24);
    CHECK(wide.w == 60, "60x24 gets a full-width pane (%d)", wide.w);
    CHECK(wide.y >= 4, "and leaves %d rows for code, >= 4", wide.y);
    CHECK(wide.y + wide.h <= 24, "it fits on the grid");
    CHECK(wide.w > wide.h * 2, "%dx%d is landscape on the glass", wide.w, wide.h);

    const viz_pane_t narrow = viz_pane(30, 11);
    CHECK(narrow.w == 30, "30x11 gets a full-width pane (%d)", narrow.w);
    CHECK(narrow.y >= 4, "and leaves %d rows for code, >= 4", narrow.y);
    CHECK(narrow.y + narrow.h <= 11, "it fits on the grid");
    CHECK(narrow.w > narrow.h * 2, "%dx%d is landscape", narrow.w, narrow.h);

    viz_split_rows(40);                 /* more than the grid has */
    CHECK(viz_pane(30, 11).y >= 4, "'split 40' still leaves code rows");
    viz_split_rows(0);

    viz_split(false);
    CHECK(viz_pane(60, 24).w == 0, "split off means no pane");
    viz_split(true);

    /* 8. NO STALE INK. Draw big, shrink, and the rows the frame no longer
     *    covers must read empty - the caller has no other way to know where the
     *    picture stops. */
    printf("\n-- shrinking the frame leaves nothing behind --\n");
    blank();
    viz_size(40, 20);
    mark("noise", 9, 'd');
    viz_service(); snap();
    CHECK(row_at(19)[0] != '\0', "at 40x20 row 19 exists");

    viz_size(20, 6);
    snap();
    CHECK(viz_cols() == 20 && viz_rows() == 6, "resized to %dx%d",
          viz_cols(), viz_rows());
    int leaked = 0;
    for (int y = 6; y < VIZ_H; y++) {
        if (row_at(y)[0] != '\0') { leaked++; }
    }
    CHECK(leaked == 0, "rows 6..%d read empty (%d leaked)", VIZ_H - 1, leaked);

    mark("noise", 9, 'd');
    viz_service(); snap();
    int outside = 0, inside = 0;
    for (int y = 0; y < VIZ_H; y++) {
        const char *r = row_at(y);
        for (int i = 0; r[i] != '\0'; i++) {
            if (r[i] != ' ') { (i >= 20 || y >= 6) ? outside++ : inside++; }
        }
    }
    CHECK(inside > 0 && outside == 0,
          "noise drew %d cells inside and %d outside", inside, outside);

    printf("\n%s (%d problems)\n",
           fails ? "VIZ CHECKS FAILED" : "ALL VIZ CHECKS PASS", fails);
    return fails ? 1 : 0;
}
