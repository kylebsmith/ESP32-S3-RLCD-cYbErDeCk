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

int main(void)
{
    /* A stated size, not the default: every count below is a fraction of the
     * rectangle, so the rectangle has to be part of the test. */
    viz_size(32, 12);
    snap();

    /* 1. THE NAMES IN THE HELP TEXT, and the table behind them. */
    static const char *named[] = { "echo", "move", "spin", "warp", "shake",
                                   "noise", "disc", "box", "star", "ramp", "grid",
                                   "grow", "thin", "flip", "tile", "fold" };
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
    static const char *sources[] = { "noise", "disc", "box", "star", "ramp",
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
    static const char *ops[] = { "echo", "move", "spin", "warp", "shake",
                                 "grow", "thin", "tile", "fold" };
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

    /* tile repeats: a thin column becomes several. */
    printf("\n-- tile repeats the frame --\n");
    blank();
    mark("ramp", 1, 'r');               /* a thin column at the left */
    viz_service(); snap();
    const int one = frame_ink();
    mark("ramp", 1, 'r');
    mark("tile", 9, 'd');
    viz_service(); snap();
    CHECK(frame_ink() > one, "tile turned %d cells into %d", one, frame_ink());

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
