/*
 * The visual primitives, on the host.
 *
 * WHY THIS EXISTS. Four things about viz.c are promises rather than mechanics,
 * and each one breaks silently on the device - you get a frame that looks a bit
 * wrong on a 30-column panel and no error anywhere:
 *
 *   1. Every name in the help text resolves to a primitive. A typo in the
 *      table means '>viz ring' says "no generator" to someone reading the
 *      help, which is the worst kind of wrong.
 *   2. s_names and s_draw are in the SAME order. They are two literals in two
 *      places; nothing in C makes them agree. If they slip, '>viz box' draws
 *      rain and the owner has no way to tell it from a bug in their pattern.
 *   3. mirror runs AFTER the generators, because the table order is the draw
 *      order. If it ran first it would fold an empty frame and do nothing -
 *      a primitive that silently does nothing at all.
 *   4. rain remembers between frames. It is the only primitive with state, and
 *      state that quietly stops advancing looks like a static texture.
 *
 * This compiles viz.c itself, not a copy of it, so the thing under test is the
 * thing that ships.
 */
#include <stdio.h>
#include <string.h>

#include "viz.h"

static int fails;
#define CHECK(cond, ...) do {                                             \
    if (!(cond)) { printf("  [FAIL] " __VA_ARGS__); printf("\n"); fails++; } \
    else         { printf("  [ ok ] " __VA_ARGS__); printf("\n"); }        \
} while (0)

/* Ink anywhere in the frame.
 *
 * ONLY OVER THE LIVE RECTANGLE. VIZ_W and VIZ_H are the buffer's maximum, not
 * the frame's size, and reading past a row's terminator finds whatever a larger
 * layout left in the buffer - the very confusion this file exists to pin down. */
static int frame_ink(void)
{
    int n = 0;
    for (int y = 0; y < viz_rows(); y++) {
        const char *row = viz_row(y);
        for (int x = 0; x < viz_cols() && row[x] != '\0'; x++) {
            n += row[x] != ' ';
        }
    }
    return n;
}

/* A TRULY EMPTY START. Removing the lanes is not enough: the frame buffer
 * still holds the last picture, and echo would resurrect it - which is correct
 * behaviour and wrong for a test that means "from nothing". */
static void clear_all_lanes(void)
{
    viz_forget_all();
    static const char *all[] = { "echo", "move", "warp",
                                 "noise", "disc", "ramp", "tile", "fold" };
    for (unsigned i = 0; i < sizeof all / sizeof *all; i++) {
        viz_lane(all[i], "");
    }
}

int main(void)
{
    /* A stated size, not the default: every count below is a fraction of the
     * rectangle, so the rectangle has to be part of the test. */
    viz_size(32, 12);

    /* 1. THE NAMES IN THE HELP TEXT. These are the exact strings c_viz prints
     *    when '>viz' is typed with no argument. */
    static const char *named[] = { "echo", "move", "warp",
                                   "noise", "disc", "ramp", "tile", "fold" };
    printf("-- every name the help offers resolves --\n");
    for (unsigned i = 0; i < sizeof named / sizeof *named; i++) {
        CHECK(viz_lane(named[i], "9") == ESP_OK, "viz %s", named[i]);
        viz_lane(named[i], "");
    }
    CHECK(viz_lane("sparkle", "9") != ESP_OK, "an unknown name is refused");

    /* 2. THE SOURCES DRAW, AND EACH DRAWS ITS OWN SHAPE. A name that resolves
     *    and then puts nothing in the frame is the failure a lookup test
     *    cannot see. */
    printf("\n-- each source draws --\n");
    static const char *sources[] = { "noise", "disc", "ramp" };
    for (unsigned i = 0; i < sizeof sources / sizeof *sources; i++) {
        clear_all_lanes();
        viz_lane(sources[i], "9");
        viz_tick(0);
        const int ink = frame_ink();
        CHECK(ink > 0, "%s put %d cells down", sources[i], ink);
    }

    /* disc at full amount is FILLED and centred: its middle is inked. That is
     * what tells it apart from ramp if the table rows ever swap. */
    clear_all_lanes();
    viz_lane("disc", "9");
    viz_tick(0);
    CHECK(viz_row(viz_rows() / 2)[viz_cols() / 2] != ' ', "disc is filled");

    /* A ramp runs along its axis: 'd' inks whole ROWS from the top, so the top
     * row is full and the bottom is empty at a partial amount. */
    clear_all_lanes();
    viz_lane("ramp", "d 4");          /* explicit: downward, part way */
    viz_tick(0);
    {
        int top = 0, bot = 0;
        for (int x = 0; x < viz_cols(); x++) {
            top += viz_row(0)[x] != ' ';
            bot += viz_row(viz_rows() - 1)[x] != ' ';
        }
        CHECK(top > 0 && bot == 0, "ramp 4 fills from the top (%d/%d)", top, bot);
    }

    /* 3. THE OPERATORS CHANGE WHAT IS THERE AND DRAW NOTHING THEMSELVES. An
     *    operator that inks an empty frame is not an operator. */
    printf("\n-- operators alone leave the frame empty --\n");
    static const char *ops[] = { "echo", "move", "warp", "tile", "fold" };
    for (unsigned i = 0; i < sizeof ops / sizeof *ops; i++) {
        clear_all_lanes();
        viz_lane(ops[i], "9");
        viz_tick(0);
        viz_tick(24);                 /* twice, so echo has a past to work on */
        CHECK(frame_ink() == 0, "%s alone draws nothing", ops[i]);
    }

    /* fold, on a source, makes the frame symmetric - and it has to run AFTER
     * the source, which is what the table order buys. */
    printf("\n-- fold mirrors what the sources drew --\n");
    clear_all_lanes();
    viz_lane("noise", "5");
    viz_tick(0);
    int asym = 0;
    for (int y = 0; y < viz_rows(); y++) {
        for (int x = 0; x < viz_cols() / 2; x++) {
            asym += viz_row(y)[x] != viz_row(y)[viz_cols() - 1 - x];
        }
    }
    CHECK(asym > 0, "noise alone is not symmetric (%d cells differ)", asym);

    viz_lane("fold", "1");
    viz_tick(0);
    int sym = 1;
    for (int y = 0; y < viz_rows(); y++) {
        for (int x = 0; x < viz_cols() / 2; x++) {
            if (viz_row(y)[x] != viz_row(y)[viz_cols() - 1 - x]) { sym = 0; }
        }
    }
    CHECK(sym, "noise + fold is symmetric left to right");

    viz_lane("fold", "9");
    viz_tick(0);
    int quad = 1;
    for (int y = 0; y < viz_rows() / 2; y++) {
        if (memcmp(viz_row(y), viz_row(viz_rows() - 1 - y),
                   (size_t)viz_cols()) != 0) { quad = 0; }
    }
    CHECK(quad, "fold 9 folds top to bottom too");

    /* 4. ECHO IS FEEDBACK, AND IT FADES. One frame of a source, then the
     *    source removed: the ink must persist and then die out. Without the
     *    fade, a trail never clears and the frame fills up for good. */
    printf("\n-- echo keeps the last frame, one step dimmer --\n");
    clear_all_lanes();
    viz_lane("disc", "9");
    viz_lane("echo", "9999");         /* every step */
    viz_tick(0);
    const int lit = frame_ink();
    CHECK(lit > 0, "the disc drew %d cells", lit);
    viz_lane("disc", "");             /* source removed: only echo remains */
    viz_tick(24);                     /* step 1: no disc, echo only */
    const int after1 = frame_ink();
    CHECK(after1 > 0, "one step later %d cells survive", after1);
    CHECK(after1 <= lit, "and no more than were there (%d <= %d)", after1, lit);
    int prev = after1;
    for (int k = 2; k < 8; k++) {
        viz_tick((uint32_t)k * 24u);
        const int now = frame_ink();
        if (now > prev) { prev = -1; break; }
        prev = now;
    }
    CHECK(prev >= 0, "it only ever fades, never grows");
    CHECK(prev == 0, "and reaches nothing (%d left)", prev);

    /* 5. MOTION RUNS BEFORE THE SOURCES, which is the whole reason echo plus
     *    move reads as a trail: the history is shifted and the new source
     *    lands fresh at its own place. If move ran after, the source would be
     *    dragged along with the trail and nothing would streak.
     *
     *    Proof: a source that only draws on step 0, plus echo, plus move down.
     *    By step 2 there must be ink BELOW where the source draws. */
    printf("\n-- move shifts the history, not the source --\n");
    clear_all_lanes();
    viz_size(16, 8);
    viz_lane("ramp", "u 1...");       /* one row at the BOTTOM, step 0 of 4 */
    viz_lane("echo", "9999");
    viz_lane("move", "uuuu");         /* history travels upward */
    viz_tick(0);
    int first_row = -1;
    for (int y = 0; y < viz_rows(); y++) {
        for (int x = 0; x < viz_cols(); x++) {
            if (viz_row(y)[x] != ' ') { first_row = y; break; }
        }
        if (first_row >= 0) { break; }
    }
    CHECK(first_row >= 0, "step 0 inked row %d", first_row);
    viz_tick(24);
    viz_tick(48);
    viz_lane("ramp", "");             /* no new ink: only the trail moves */
    int highest = 99;
    for (int y = 0; y < viz_rows(); y++) {
        for (int x = 0; x < viz_cols(); x++) {
            if (viz_row(y)[x] != ' ' && y < highest) { highest = y; }
        }
    }
    CHECK(highest < first_row,
          "two steps on, the trail reached row %d, above row %d",
          highest, first_row);
    viz_size(32, 12);

    /* tile repeats: a single column of ink becomes several. */
    printf("\n-- tile repeats the frame --\n");
    clear_all_lanes();
    viz_lane("ramp", "r 1");          /* a thin column at the left */
    viz_tick(0);
    const int one = frame_ink();
    viz_lane("tile", "9");            /* four copies */
    viz_tick(0);
    CHECK(frame_ink() > one, "tile turned %d cells into %d", one, frame_ink());

    /* 6. THE PANE. Two failures the owner actually saw, both invisible to a
     *    lookup test: a side-by-side split on a thirty-column grid left twelve
     *    columns for the code and wrapped every pattern line, and a frame
     *    shorter than its pane left the cells underneath holding whatever the
     *    last layout drew - which read as static junk in the bottom right. */
    printf("\n-- the preview pane --\n");
    viz_split(true);

    /* THE SAME SHAPE AT EITHER DENSITY, and always landscape. A cell is twice
     * as tall as it is wide, so a pane is square on the glass when it is twice
     * as many columns as rows - and a pane that is TALLER than it is wide in
     * cells is a portrait sliver on the panel, which is the stretched picture
     * the owner saw. Every pane here must be wider than it is tall. */
    const viz_pane_t wide = viz_pane(60, 24);
    CHECK(wide.w > 0, "60x24 gets a pane");
    CHECK(wide.w == 60, "it is full width (%d)", wide.w);
    CHECK(wide.y >= 4, "and leaves %d rows for code, >= 4", wide.y);
    CHECK(wide.y + wide.h <= 24, "it fits on the grid");
    CHECK(wide.w > wide.h * 2, "%dx%d is landscape on the glass",
          wide.w, wide.h);

    const viz_pane_t narrow = viz_pane(30, 11);
    CHECK(narrow.w > 0, "30x11 gets a pane");
    CHECK(narrow.w == 30, "it is full width (%d)", narrow.w);
    CHECK(narrow.y >= 4, "and leaves %d rows for code, >= 4", narrow.y);
    CHECK(narrow.y + narrow.h <= 11, "it fits on the grid");
    CHECK(narrow.w > narrow.h * 2, "%dx%d is landscape on the glass",
          narrow.w, narrow.h);

    /* A request the grid cannot honour shrinks the PICTURE, never the code. */
    viz_split_rows(40);
    const viz_pane_t greedy = viz_pane(30, 11);
    CHECK(greedy.y >= 4, "'split 40' still leaves %d rows of code", greedy.y);
    viz_split_rows(0);

    viz_split(false);
    CHECK(viz_pane(60, 24).w == 0, "split off means no pane");
    viz_split(true);

    /* 7. NO STALE INK. Draw big, shrink, and the rows the frame no longer
     *    covers must read empty - the caller has no other way to know where
     *    the picture stops. */
    printf("\n-- shrinking the frame leaves nothing behind --\n");
    clear_all_lanes();
    viz_size(40, 20);
    viz_lane("noise", "9");
    viz_tick(0);
    CHECK(viz_row(19)[0] != '\0', "at 40x20 row 19 exists");
    viz_size(20, 6);
    CHECK(viz_cols() == 20 && viz_rows() == 6, "resized to %dx%d",
          viz_cols(), viz_rows());
    int leaked = 0;
    for (int y = 6; y < VIZ_H; y++) {
        if (viz_row(y)[0] != '\0') { leaked++; }
    }
    CHECK(leaked == 0, "rows 6..%d read empty (%d leaked)", VIZ_H - 1, leaked);

    /* And the generators respect the new rectangle: full-intensity noise must
     * not put a single cell outside it. */
    viz_tick(0);
    int outside = 0, inside = 0;
    for (int y = 0; y < VIZ_H; y++) {
        const char *row = viz_row(y);
        for (int i = 0; row[i] != '\0'; i++) {
            if (row[i] != ' ') { (i >= 20 || y >= 6) ? outside++ : inside++; }
        }
    }
    CHECK(inside > 0 && outside == 0,
          "noise drew %d cells inside and %d outside", inside, outside);

    printf("\n%s (%d problems)\n", fails ? "VIZ CHECKS FAILED" : "ALL VIZ CHECKS PASS",
           fails);
    return fails ? 1 : 0;
}
