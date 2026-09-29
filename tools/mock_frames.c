/*
 * Frames for the picture mock-ups in docs/wiki/pictures-and-type.md.
 *
 * Like tools/zine_art.c this draws nothing itself: it links the picture engine,
 * marks primitives the way the clock does when a lane fires, and saves the
 * cells. tools/mock_pictures.py builds it twice - once against the real
 * firmware/components/viz/viz.c, and once against a copy with square dots (the
 * proposal being mocked) - and sets both side by side.
 *
 *   mock_frames OUTDIR W H          one frame of each scene
 *   mock_frames OUTDIR W H motion   every frame of the radar, the night and the
 *                                   orbit, a step at a time, as the panel shows them
 *   mock_frames OUTDIR W H script F every frame of a score: F has a line a step,
 *                                   each the picture events tools/test_pieces -s
 *                                   printed for it - 'disc:#9', 'ramp:d3',
 *                                   'disc:x:#8', 'turn:2:u2' - played in order
 *
 * OUTDIR/<scene>-<W>x<H>.cells is 2 bytes (w, h) then w*h cells.
 */
#include <stdbool.h>
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

/* THE RADAR FROM ORBITALS, one step of it, as the piece is now written:
 *
 *   >echo 8
 *   >turn u 2...............
 *   >turn:2 l ....2...........
 *   >turn:3 d ........2.......
 *   >turn:4 r ............2...
 *
 * Four beams taking turns, one to a beat, stepping anticlockwise - a turn fades
 * clockwise from its leading edge, so anticlockwise leaves each trail behind its
 * beam - and echo fading them where they were drawn. It was '>turn 2' under
 * '>spin <0 3 6 9>', and spin turns the history, never this step's source, so the
 * fresh wedge landed in the same quadrant every step: a corner that never moved,
 * which the owner saw on the screen as broken. */
static void radar(int s)
{
    static const char heading[4] = { 'u', 'l', 'd', 'r' };
    mark("echo", 8, 0);
    if (s % 4 == 0) { mark("turn", 2, heading[(s / 4) % 4]); }
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
    const bool motion = argc == 5 && strcmp(argv[4], "motion") == 0;
    const bool script = argc == 6 && strcmp(argv[4], "script") == 0;
    if (argc != 4 && !motion && !script) {
        fprintf(stderr, "usage: mock_frames OUTDIR W H [motion | script FILE]\n");
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

    if (script) {
        /* A SCORE, A STEP A LINE. A way the step does not name is the lane's
         * default, down, as the deck's fire_event gives it; an instance ('turn:2')
         * is its primitive; anything that is not a picture is someone else's. */
        FILE *f = fopen(argv[5], "r");
        if (f == NULL) { fprintf(stderr, "cannot read %s\n", argv[5]); return 2; }
        char line[4096], name[32];
        int k = 0;
        fresh();
        while (fgets(line, sizeof line, f) != NULL) {
            for (char *t = strtok(line, " \t\r\n"); t != NULL; t = strtok(NULL, " \t\r\n")) {
                char *part[4] = { 0 };
                int np = 0;
                for (char *q = t; np < 4; ) {
                    part[np++] = q;
                    q = strchr(q, ':');
                    if (q == NULL) { break; }
                    *q++ = '\0';
                }
                if (np < 2 || viz_prim_index(part[0]) < 0) { continue; }
                const char *last = part[np - 1];
                const char *axis = (np >= 3 && (strcmp(part[np - 2], "x") == 0 ||
                                                strcmp(part[np - 2], "y") == 0)) ? part[np - 2] : NULL;
                const char way = last[0];
                const int amt = atoi(last + 1);
                if (axis != NULL) {
                    at(part[0], axis[0], amt);
                } else if (way == '#' || way == 'u' || way == 'd' || way == 'l' || way == 'r') {
                    mark(part[0], amt, way == '#' ? 'd' : way);
                }
            }
            step();
            snprintf(name, sizeof name, "s%03d", k++);
            bad |= save(name);
        }
        fclose(f);
        return bad;
    }

    if (motion) {
        /* The engine draws a frame when a lane fires and not between, so a frame
         * a step is the whole of the motion: the radar, then the orbit. */
        char name[32];
        fresh();
        for (int s = 0; s < 32; s++) {
            radar(s);
            step();
            snprintf(name, sizeof name, "radar%02d", s);
            bad |= save(name);
        }
        /* ORBITALS' first section as written - the radar and >noise 1, whose
         * sparkles are the deck's own glyphs and have to reach the screen as
         * glyphs, not as greys */
        fresh();
        for (int s = 0; s < 32; s++) {
            radar(s);
            mark("noise", 1, 0);
            step();
            snprintf(name, sizeof name, "night%02d", s);
            bad |= save(name);
        }
        static const char xs[] = "8876532111235678", ys[] = "568886531113";
        static const char sun[] = "2...1...2...1...";
        fresh();
        for (int s = 0; s < 48; s++) {
            mark("echo", 8, 0);
            if (sun[s % 16] != '.') { mark("box", sun[s % 16] - '0', 0); }
            at("disc", 'x', xs[s % 16] - '0');
            at("disc", 'y', ys[s % 12] - '0');
            mark("disc", 3, 0);
            step();
            snprintf(name, sizeof name, "orbit%02d", s);
            bad |= save(name);
        }
        return bad;
    }

    fresh(); mark("disc", 8, 0); step(); bad |= save("disc");
    fresh(); mark("disc", 9, 0); mark("mask", 5, 0); step(); bad |= save("discmask");
    fresh(); mark("disc", 8, 0); mark("edge", 1, 0); step(); bad |= save("ring");
    fresh(); mark("box", 7, 0); step(); bad |= save("box");
    fresh(); mark("turn", 9, 'u'); step(); bad |= save("turn");

    /* the radar from ORBITALS, two beams in: the third beat of its bar */
    fresh();
    for (int s = 0; s < 10; s++) {
        radar(s);
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
