/*
 * The zine's pictures, drawn by the deck's own picture engine.
 *
 * zine/ is set in the deck's faces and its pictures are the deck's: this links
 * firmware/components/viz/viz.c - the file the firmware builds - marks
 * primitives the way the clock does when a lane fires, and writes each frame's
 * cells (codes 32-155, the tiles included) for tools/zine.py to set on the
 * page. Nothing here draws; it only asks viz.c to.
 *
 *   zine_art OUTDIR
 *
 * Each scene is the lines a performer would type, played step by step:
 * OUTDIR/<scene>-<n>.cells is 2 bytes (w, h) then w*h cells.
 */
#include <stdio.h>
#include <string.h>

#include "seq.h"
#include "viz.h"

/* viz.c reads the lane table for its draw order; no lane is routed here. */
const seq_lane_t *seq_lanes(int *count)
{
    if (count != NULL) { *count = 0; }
    return NULL;
}

static const char *s_dir;
static uint32_t s_tick;

static int P(const char *name)
{
    const int p = viz_prim_index(name);
    if (p < 0) {
        fprintf(stderr, "no primitive %s\n", name);
    }
    return p;
}

static void mark(const char *name, int amt, char dir)
{
    viz_mark(P(name), amt, dir, s_tick);
}

static void at(const char *name, char axis, int amt)
{
    viz_mark_param(P(name), viz_param_index(axis == 'x' ? "x" : "y"), amt);
}

static void save(const char *scene, int n)
{
    static uint8_t cells[VIZ_W * VIZ_H];
    int w = 0, h = 0;
    uint32_t tick = 0;
    const int k = viz_frame(cells, (int)sizeof cells, &w, &h, &tick);
    char path[256];
    snprintf(path, sizeof path, "%s/%s-%d.cells", s_dir, scene, n);
    FILE *f = fopen(path, "wb");
    if (f == NULL || k <= 0) {
        fprintf(stderr, "cannot write %s\n", path);
        return;
    }
    const uint8_t wh[2] = { (uint8_t)w, (uint8_t)h };
    fwrite(wh, 1, 2, f);
    fwrite(cells, 1, (size_t)w * (size_t)h, f);
    fclose(f);
}

/* One step of the clock: the marks made, then the frame drawn. */
static void step(void)
{
    viz_service();
    s_tick += 24;
}

static void fresh(void)
{
    viz_forget_all();
    viz_size(VIZ_W, VIZ_H);
    s_tick = 0;
    /* an empty frame, so echo has nothing left from the last scene */
    step();
}

int main(int argc, char **argv)
{
    s_dir = argc > 1 ? argv[1] : ".";

    /* >disc 8   >edge 1   - a ring: a field through a contour */
    fresh();
    mark("disc", 8, 0); mark("edge", 1, 0); step();
    save("ring", 0);

    /* >disc 9   - the round field alone, solid then falling off */
    fresh();
    mark("disc", 9, 0); step();
    save("disc", 0);

    /* >ramp d   >edge 9   - a contour map */
    fresh();
    mark("ramp", 9, 'd'); mark("edge", 9, 0); step();
    save("contour", 0);

    /* >noise 2   >mask 7   - a few hard stars in a lot of dark */
    fresh();
    mark("noise", 3, 0); step();
    save("stars", 0);

    /* >echo 8   >turn 2   >spin <0 3 6 9>   - the radar from ORBITALS, night */
    fresh();
    for (int s = 0; s < 12; s++) {
        const int spin[4] = { 0, 3, 6, 9 };
        mark("echo", 8, 0); mark("turn", 2, 0); mark("spin", spin[s % 4], 0);
        step();
    }
    save("radar", 0);

    /* ORBITALS as it is written: >echo 8; a sun that beats on the beat,
     * >box 2...1...2...1...; a planet, >disc 3, whose x runs at sixteen
     * steps and y at twelve (>disc:x 8876532111235678, >disc:y
     * 568886531113). Echo smears the planet into a comet: the orbit. */
    {
        static const char xs[] = "8876532111235678";
        static const char ys[] = "568886531113";
        static const char sun[] = "2...1...2...1...";
        for (int scene = 0; scene < 3; scene++) {
            fresh();
            int n = 0;
            for (int s = 0; s < 48; s++) {
                mark("echo", 8, 0);
                const char b = sun[s % 16];
                if (scene == 2) {
                    mark("box", 6, 0);                    /* eclipse: >box 6 */
                } else if (b != '.') {
                    mark("box", b - '0', 0);
                }
                at("disc", 'x', xs[s % 16] - '0');
                at("disc", 'y', ys[s % 12] - '0');
                mark("disc", 3, 0);
                if (scene == 1) { mark("fold", 9, 0); }   /* day: >fold 9 */
                if (scene == 2) { mark("edge", 1, 0); }   /* >edge 1 */
                step();
                if (s % 8 == 7) {
                    save(scene == 0 ? "orbit" : scene == 1 ? "day" : "eclipse", n++);
                }
            }
        }
    }

    /* >echo 9 >move u >noise 1 - trails that rise, the olivia jack sketch */
    fresh();
    for (int s = 0; s < 20; s++) {
        mark("echo", 9, 0); mark("move", 9, 'u'); mark("noise", 1, 0);
        if (s % 4 == 0) { mark("disc", 4, 0); }
        step();
    }
    save("smoke", 0);

    /* THE SIXTEEN, one specimen each, for docs/CMF.md: a field alone, or an
     * operator doing its one thing to a field - the lines are in the name. */
    fresh(); mark("disc", 8, 0); step(); save("spec-disc", 0);
    fresh(); mark("box", 8, 0); step(); save("spec-box", 0);
    fresh(); mark("turn", 9, 'u'); step(); save("spec-turn", 0);
    fresh(); mark("ramp", 9, 'r'); step(); save("spec-ramp", 0);
    fresh(); mark("grid", 5, 0); step(); save("spec-grid", 0);
    fresh(); mark("noise", 3, 0); step(); save("spec-noise", 0);
    fresh(); mark("disc", 9, 0); mark("mask", 7, 0); step(); save("spec-mask", 0);
    fresh(); mark("disc", 8, 0); mark("edge", 1, 0); step(); save("spec-edge", 0);
    fresh();
    for (int s = 0; s < 8; s++) {                    /* >echo 8, a disc moving */
        mark("echo", 8, 0); at("disc", 'x', 1 + s); mark("disc", 3, 0); step();
    }
    save("spec-echo", 0);
    fresh();
    for (int s = 0; s < 10; s++) {                   /* >move u under echo */
        mark("echo", 9, 0); mark("move", 9, 'u'); mark("noise", 1, 0); step();
    }
    save("spec-move", 0);
    fresh();
    for (int s = 0; s < 12; s++) {                   /* >spin <0 3 6 9> on a turn */
        const int spin[4] = { 0, 3, 6, 9 };
        mark("echo", 8, 0); mark("turn", 2, 0); mark("spin", spin[s % 4], 0); step();
    }
    save("spec-spin", 0);
    /* warp bends the history, before anything draws: a ramp, then echo+warp */
    fresh(); mark("ramp", 9, 'd'); step();
    mark("echo", 9, 0); mark("warp", 9, 'd'); step(); save("spec-warp", 0);
    fresh(); mark("noise", 1, 0); mark("grow", 9, 0); step(); save("spec-grow", 0);
    fresh(); mark("box", 8, 0); mark("thin", 9, 0); step(); save("spec-thin", 0);
    fresh(); mark("disc", 6, 0); mark("flip", 9, 0); step(); save("spec-flip", 0);
    fresh(); mark("turn", 4, 'l'); mark("fold", 9, 0); step(); save("spec-fold", 0);

    printf("scenes written to %s\n", s_dir);
    return 0;
}
