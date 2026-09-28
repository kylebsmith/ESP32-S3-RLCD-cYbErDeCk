/*
 * Frames for the picture mock-ups in docs/wiki/pictures-and-type.md.
 *
 * Like tools/zine_art.c this draws nothing itself: it links the picture engine,
 * marks primitives the way the clock does when a lane fires, and saves the
 * cells. tools/mock_pictures.py builds it twice - once against the real
 * firmware/components/viz/viz.c, and once against a copy with square dots (the
 * proposal being mocked) - and sets both side by side.
 *
 *   mock_frames OUTDIR W H
 *
 * OUTDIR/<scene>-<W>x<H>.cells is 2 bytes (w, h) then w*h cells.
 */
#include <stdio.h>
#include <stdlib.h>
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
static int s_w, s_h;

static int P(const char *name)
{
    const int p = viz_prim_index(name);
    if (p < 0) { fprintf(stderr, "no primitive %s\n", name); }
    return p;
}

static void mark(const char *name, int amt, char dir) { viz_mark(P(name), amt, dir, s_tick); }

static void at(const char *name, char axis, int amt)
{
    viz_mark_param(P(name), viz_param_index(axis == 'x' ? "x" : "y"), amt);
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
    viz_size(s_w, s_h);
    s_tick = 0;
    step();
}

static int save(const char *scene)
{
    static uint8_t cells[VIZ_W * VIZ_H];
    int w = 0, h = 0;
    uint32_t tick = 0;
    const int k = viz_frame(cells, (int)sizeof cells, &w, &h, &tick);
    char path[512];
    snprintf(path, sizeof path, "%s/%s-%dx%d.cells", s_dir, scene, s_w, s_h);
    FILE *f = fopen(path, "wb");
    if (f == NULL || k <= 0 || w != s_w || h != s_h) {
        fprintf(stderr, "cannot write %s (%dx%d)\n", path, w, h);
        if (f != NULL) { fclose(f); }
        return 1;
    }
    const uint8_t wh[2] = { (uint8_t)w, (uint8_t)h };
    fwrite(wh, 1, 2, f);
    fwrite(cells, 1, (size_t)w * (size_t)h, f);
    fclose(f);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: mock_frames OUTDIR W H\n");
        return 2;
    }
    s_dir = argv[1];
    s_w = atoi(argv[2]);
    s_h = atoi(argv[3]);
    if (s_w < 4 || s_w > VIZ_W || s_h < 2 || s_h > VIZ_H || s_w > 255 || s_h > 255) {
        fprintf(stderr, "%dx%d does not fit this engine (%dx%d)\n", s_w, s_h, VIZ_W, VIZ_H);
        return 2;
    }
    int bad = 0;

    fresh(); mark("disc", 8, 0); step(); bad |= save("disc");
    fresh(); mark("disc", 9, 0); mark("mask", 5, 0); step(); bad |= save("discmask");
    fresh(); mark("disc", 8, 0); mark("edge", 1, 0); step(); bad |= save("ring");
    fresh(); mark("box", 7, 0); step(); bad |= save("box");
    fresh(); mark("turn", 9, 'u'); step(); bad |= save("turn");

    /* >echo 8 >turn 2 >spin <0 3 6 9> - the radar from ORBITALS */
    fresh();
    for (int s = 0; s < 12; s++) {
        static const int spin[4] = { 0, 3, 6, 9 };
        mark("echo", 8, 0); mark("turn", 2, 0); mark("spin", spin[s % 4], 0);
        step();
    }
    bad |= save("radar");

    /* ORBITALS as written: a sun on the beat, a planet at sixteen steps by twelve */
    {
        static const char xs[] = "8876532111235678", ys[] = "568886531113";
        static const char sun[] = "2...1...2...1...";
        fresh();
        for (int s = 0; s < 40; s++) {
            mark("echo", 8, 0);
            if (sun[s % 16] != '.') { mark("box", sun[s % 16] - '0', 0); }
            at("disc", 'x', xs[s % 16] - '0');
            at("disc", 'y', ys[s % 12] - '0');
            mark("disc", 3, 0);
            step();
        }
        bad |= save("orbit");
    }
    return bad;
}
