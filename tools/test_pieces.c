/*
 * The pieces in pieces/, checked line by line the way the deck reads them.
 *
 * A piece is a document: the lines a performer runs, top to bottom. A line
 * the deck refuses is a piece that stops in front of an audience, and the
 * grammar moves - '!4', the step decision and the names each changed what a
 * line may say - so every line of every piece goes through the shipping
 * compiler (seq_pattern.h), the name grammar (lane_name.h), the key
 * (seq_scale.h) and the picture table (viz.c) here, with the command layer's
 * own rules for what a lane, a definition and a route may be. It starts from
 * the boot document's names, as the deck does.
 *
 *   test_pieces BUILTINS_C [-s] [-b BARS] FILE...
 *
 * BUILTINS_C is firmware/components/cmd/builtins.c, read for the verb table
 * so a renamed verb fails here. -s prints, at the end of every section (a
 * line starting '--'), the notes the lanes play: every step, every lane, the
 * degrees resolved in the piece's key. It is how the pieces were written.
 */
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lane_name.h"
#include "seq.h"
#include "seq_pattern.h"
#include "seq_scale.h"
#include "ui_text.h"
#include "viz.h"

/* viz.c reads the lane table for its draw order; nothing here draws. */
const seq_lane_t *seq_lanes(int *count)
{
    if (count != NULL) { *count = 0; }
    return NULL;
}

#define MAX_VERBS  64
#define ALIASES    32        /* builtins.c ALIAS_MAX */
#define LINE_MAX_RUN 127     /* c_run's line buffer, less its terminator */
#define WIDE       30        /* the chunky face's columns */

static char s_verbs[MAX_VERBS][12];
static int  s_nverbs;

typedef struct {
    char name[LANE_BASE_MAX + 1];
    int  kind, num, chan, gate;
} alias_t;
static alias_t s_alias[ALIASES];

typedef struct {
    bool       used, muted;
    char       name[LANE_NAME_MAX];
    int        kind;            /* LD_NOTE, LD_VOICE, LD_CC, LD_DRAW      */
    int        num, chan, gate, prim, part;   /* part: 0, 'v', 'o', 'x', 'y' */
    char       route[LANE_NAME_MAX + 4];
    seq_comp_t c;
    bool       has_pat;
    char       text[LINE_MAX_RUN + 1];   /* the pattern as last run */
    /* the clock's state, for the score - seq.c's names */
    bool       trig, idle, done;
    int        trig_val, vel, oct, rank;
    uint32_t   origin;
} lane_t;
static lane_t s_lane[SEQ_MAX_LANES];

typedef struct { char name[LANE_BASE_MAX + 1]; int kind; } input_t;
static input_t s_input[SEQ_MAX_INPUTS];

static int s_bpm = 124, s_swing = 50, s_root = 2;
static const seq_mode_t *s_mode = &seq_modes[SEQ_MODE_MIN];
static int fails, s_wide, s_lines;
static const char *s_file;
static int s_lineno;

static void fail(const char *fmt, const char *a, const char *b)
{
    printf("[FAIL] %s:%d: ", s_file, s_lineno);
    printf(fmt, a ? a : "", b ? b : "");
    printf("\n");
    fails++;
}

/* ---------------------------------------------------------------- tables */

static void load_verbs(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        printf("[FAIL] cannot read %s\n", path);
        exit(2);
    }
    static char src[256 * 1024];
    const size_t n = fread(src, 1, sizeof src - 1, f);
    fclose(f);
    src[n] = '\0';
    const char *t = strstr(src, "static const cmd_t");
    const char *end = t ? strstr(t, "};") : NULL;
    for (const char *p = t; p != NULL && p < end; p++) {
        if (*p != '{') { continue; }
        const char *q = p + 1;
        while (*q == ' ') { q++; }
        if (*q != '"') { continue; }
        q++;
        const char *e = strchr(q, '"');
        const char *c = e ? e + 1 : NULL;
        while (c && (*c == ',' || *c == ' ')) { c++; }
        if (e && c && c[0] == 'c' && c[1] == '_' && e - q < 12 && s_nverbs < MAX_VERBS) {
            memcpy(s_verbs[s_nverbs], q, (size_t)(e - q));
            s_verbs[s_nverbs][e - q] = '\0';
            s_nverbs++;
        }
    }
    if (s_nverbs < 20) {
        printf("[FAIL] found %d verbs in %s - the table moved\n", s_nverbs, path);
        exit(2);
    }
}

static bool is_verb(const char *w)
{
    for (int i = 0; i < s_nverbs; i++) {
        if (strcmp(s_verbs[i], w) == 0) { return true; }
    }
    return false;
}

static alias_t *alias_find(const char *name)
{
    for (int i = 0; i < ALIASES; i++) {
        if (s_alias[i].name[0] != '\0' && strcmp(s_alias[i].name, name) == 0) {
            return &s_alias[i];
        }
    }
    return NULL;
}

static lane_t *lane_find(const char *name)
{
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        if (s_lane[i].used && strcmp(s_lane[i].name, name) == 0) {
            return &s_lane[i];
        }
    }
    return NULL;
}

static input_t *input_find(const char *name)
{
    for (int i = 0; i < SEQ_MAX_INPUTS; i++) {
        if (s_input[i].name[0] != '\0' && strcmp(s_input[i].name, name) == 0) {
            return &s_input[i];
        }
    }
    return NULL;
}

static void forget_base(const char *base)
{
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        lane_name_t o;
        if (s_lane[i].used &&
            lane_name_parse(s_lane[i].name, strlen(s_lane[i].name), &o) == LN_OK &&
            strcmp(o.base, base) == 0) {
            s_lane[i].used = false;
        }
    }
}

/* ---------------------------------------------------------------- binding */

/* builtins.c binding_of(), on this file's tables. */
static bool bind(const lane_name_t *ln, lane_t *l, char *why, size_t wn)
{
    const alias_t *a = alias_find(ln->base);
    int prim = -1;
    l->part = 0;
    if (a != NULL) {
        if (a->kind == LD_KNOB || a->kind == LD_PAD) {
            snprintf(why, wn, "%s is an input: route from it", ln->base);
            return false;
        }
        if (a->kind == LD_DRAW) {
            prim = a->num;
        } else {
            l->kind = a->kind;
            l->num = a->num;
            l->chan = a->chan;
            l->gate = a->gate;
        }
    } else {
        prim = viz_prim_index(ln->base);
        if (prim < 0) {
            snprintf(why, wn, "%s? try: help", ln->base);
            return false;
        }
    }
    if (prim >= 0) {
        l->kind = LD_DRAW;
        l->prim = prim;
        const int p = viz_param_index(ln->part);
        if (ln->part[0] != '\0' && p == VIZ_PARAM_NONE) {
            snprintf(why, wn, "no :%s - a picture has x, y", ln->part);
            return false;
        }
        l->part = ln->part[0] ? ln->part[0] : 0;
        return true;
    }
    if (ln->part[0] != '\0') {
        const bool note = (l->kind == LD_NOTE || l->kind == LD_VOICE);
        if (note && strcmp(ln->part, "vel") == 0) {
            l->part = 'v';
        } else if (l->kind == LD_VOICE && strcmp(ln->part, "oct") == 0) {
            l->part = 'o';
        } else {
            snprintf(why, wn, "%s has no :%s", ln->base, ln->part);
            return false;
        }
    }
    return true;
}

static lane_t *lane_slot(const char *name)
{
    lane_t *l = lane_find(name);
    for (int i = 0; i < SEQ_MAX_LANES && l == NULL; i++) {
        if (!s_lane[i].used) {
            l = &s_lane[i];
            memset(l, 0, sizeof *l);
            snprintf(l->name, sizeof l->name, "%s", name);
            l->used = true;
        }
    }
    return l;
}

/* ---------------------------------------------------------------- lines */

static void do_lane(const char *word, size_t n, const char *pat)
{
    lane_name_t ln;
    char why[48];
    const int e = lane_name_parse(word, n, &ln);
    if (e != LN_OK) {
        lane_name_error_text(e, why, sizeof why);
        fail("%s", why, NULL);
        return;
    }
    lane_t probe;
    memset(&probe, 0, sizeof probe);
    if (!bind(&ln, &probe, why, sizeof why)) {
        fail("%s", why, NULL);
        return;
    }
    if (pat[0] == '\0') {
        lane_t *l = lane_find(ln.canon);
        if (l != NULL) { l->used = false; }
        return;
    }
    static seq_comp_t c;
    const int ce = seq_pattern_compile(pat, &c);
    if (ce != SEQ_PAT_OK) {
        char msg[64];
        seq_pattern_error_text(&c, pat, msg, sizeof msg);
        fail("'%s': %s", pat, msg);
        return;
    }
    const bool turns = probe.kind == LD_DRAW && probe.part == 0 &&
                       viz_prim_turns(probe.prim);
    bool dir = (c.dir != 0);
    for (int i = 0; i < c.n; i++) { dir |= (c.leaf[i].dir != 0); }
    if (dir && !turns) {
        fail("'%s': u d l r: move warp ramp turn", pat, NULL);
        return;
    }
    /* RUN UNCHANGED, A LINE SILENCES ITS LANE - by hand, while playing
     * (builtins.c rerun_silences). In a piece that is a trap: the same line
     * twice, or a sketch that repeats the one before it, mutes where it meant
     * to play. A piece mutes with '>mute'. */
    lane_t *was = lane_find(ln.canon);
    if (was != NULL && was->has_pat && !was->muted && strcmp(was->text, pat) == 0) {
        fail("'%s' is what %s already plays - by hand it silences it", pat, ln.canon);
        return;
    }
    lane_t *l = lane_slot(ln.canon);
    if (l == NULL) {
        fail("%s: 16 lanes is all there is", ln.canon, NULL);
        return;
    }
    char keep[sizeof l->route];
    memcpy(keep, l->route, sizeof keep);
    *l = probe;
    l->used = true;
    snprintf(l->name, sizeof l->name, "%s", ln.canon);
    memcpy(l->route, keep, sizeof keep);
    l->c = c;
    l->has_pat = true;
    snprintf(l->text, sizeof l->text, "%.127s", pat);
}

static void do_define(const char *word, size_t n, const char *arg)
{
    lane_name_t ln;
    char why[48];
    const int e = lane_name_parse(word, n, &ln);
    if (e != LN_OK) {
        lane_name_error_text(e, why, sizeof why);
        fail("%s", why, NULL);
        return;
    }
    if (ln.inst != 1 || ln.part[0] != '\0') {
        fail("define the plain name: %s", ln.base, NULL);
        return;
    }
    if (is_verb(ln.base)) {
        fail("%s is a command", ln.base, NULL);
        return;
    }
    if (viz_prim_index(ln.base) >= 0) {
        fail("%s is a picture already", ln.base, NULL);
        return;
    }
    while (*arg == ' ') { arg++; }
    if (*arg == '=') { arg++; }
    lane_def_t d;
    lane_def_parse(arg, &d);
    if (d.kind == LD_ERROR) {
        fail("%s: %s", ln.base, d.why);
        return;
    }
    alias_t *a = alias_find(ln.base);
    input_t *in = input_find(ln.base);
    if (in != NULL && d.kind != in->kind) {
        in->name[0] = '\0';
    }
    if (d.kind == LD_REMOVE) {
        if (a != NULL) {
            forget_base(ln.base);
            a->name[0] = '\0';
        }
        return;
    }
    if (d.kind == LD_KNOB || d.kind == LD_PAD) {
        if (input_find(ln.base) == NULL) {
            input_t *slot = NULL;
            for (int i = 0; i < SEQ_MAX_INPUTS && slot == NULL; i++) {
                if (s_input[i].name[0] == '\0') { slot = &s_input[i]; }
            }
            if (slot == NULL) {
                fail("%s: 16 inputs is all there is", ln.base, NULL);
                return;
            }
            snprintf(slot->name, sizeof slot->name, "%s", ln.base);
            slot->kind = d.kind;
        }
        forget_base(ln.base);
    }
    int prim = -1;
    if (d.kind == LD_DRAW && (prim = viz_prim_index(d.draw)) < 0) {
        fail("no picture called %s", d.draw, NULL);
        return;
    }
    for (int i = 0; i < ALIASES && a == NULL; i++) {
        if (s_alias[i].name[0] == '\0') { a = &s_alias[i]; }
    }
    if (a == NULL) {
        fail("%s: 32 names is all there is", ln.base, NULL);
        return;
    }
    snprintf(a->name, sizeof a->name, "%s", ln.base);
    a->kind = d.kind;
    a->num = (d.kind == LD_DRAW) ? prim : d.num;
    a->chan = d.chan > 0 ? d.chan : 1;
    a->gate = d.gate;
    /* rebind what is playing under the name, as builtins.c does */
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        lane_name_t o;
        if (s_lane[i].used &&
            lane_name_parse(s_lane[i].name, strlen(s_lane[i].name), &o) == LN_OK &&
            strcmp(o.base, ln.base) == 0) {
            char why2[48];
            lane_t probe = s_lane[i];
            if (bind(&o, &probe, why2, sizeof why2)) {
                s_lane[i].kind = probe.kind;
                s_lane[i].num = probe.num;
                s_lane[i].chan = probe.chan;
                s_lane[i].gate = probe.gate;
                s_lane[i].prim = probe.prim;
            }
        }
    }
}

static bool whole(const char *s, long lo, long hi)
{
    if (*s == '\0') { return false; }
    char *end = NULL;
    const long v = strtol(s, &end, 10);
    while (end && *end == ' ') { end++; }
    return end && *end == '\0' && v >= lo && v <= hi;
}

static void do_route(const char *arg)
{
    char g[40] = "", s[40] = "";
    sscanf(arg, "%39s %39s", g, s);
    lane_name_t gl, sl;
    char why[48];
    int e = lane_name_parse(g, strlen(g), &gl);
    if (e == LN_OK && s[0] != '\0') {
        e = lane_name_parse(s, strlen(s), &sl);
    }
    if (g[0] == '\0' || e != LN_OK) {
        if (e != LN_OK) { lane_name_error_text(e, why, sizeof why); }
        fail("route: %s", g[0] ? why : "route <lane> <lane it follows>", NULL);
        return;
    }
    lane_t *l = lane_find(gl.canon);
    if (l == NULL) {
        lane_t probe;
        memset(&probe, 0, sizeof probe);
        if (!bind(&gl, &probe, why, sizeof why)) {
            fail("route: %s", why, NULL);
            return;
        }
        l = lane_slot(gl.canon);
        if (l == NULL) {
            fail("%s: 16 lanes is all there is", gl.canon, NULL);
            return;
        }
        char name[sizeof l->name];
        memcpy(name, l->name, sizeof name);
        *l = probe;
        l->used = true;
        memcpy(l->name, name, sizeof name);
    }
    if (s[0] == '\0') {
        l->route[0] = '\0';
        return;
    }
    if (strcmp(sl.canon, gl.canon) == 0) {
        fail("%s cannot follow itself", gl.canon, NULL);
        return;
    }
    if (!l->has_pat) {
        seq_pattern_compile("x", &l->c);
        l->has_pat = true;
    }
    snprintf(l->route, sizeof l->route, "%s", sl.canon);
    /* A PIECE ROUTES FROM WHAT IS THERE: the deck allows a source written on a
     * later line and says "silent until", which is right while typing and
     * wrong in a piece - it means the order of the lines is the bug. */
    if (strcmp(sl.part, "end") == 0) {
        char base[40];
        snprintf(base, sizeof base, "%.*s", (int)(strlen(sl.canon) - 4), sl.canon);
        const lane_t *src = lane_find(base);
        if (src == NULL) {
            fail("route %s: no lane %s yet", gl.canon, base);
        } else if (src->c.count == 0) {
            fail("route %s: %s never ends - give it !n", gl.canon, base);
        }
    } else if (lane_find(sl.canon) == NULL && input_find(sl.canon) == NULL) {
        fail("route %s: nothing called %s yet", gl.canon, sl.canon);
    }
}

/* The verbs a piece may use, and what their arguments must be. The rest -
 * the radio, the documents, the reboots - have no place in a piece: '>new'
 * inside a document being run would switch the page out from under the run. */
static void do_verb(const char *w, const char *arg)
{
    if (strcmp(w, "bpm") == 0) {
        if (!whole(arg, 20, 300)) { fail("bpm '%s': 20-300", arg, NULL); }
    } else if (strcmp(w, "swing") == 0) {
        if (!whole(arg, 50, 75)) { fail("swing '%s': 50-75", arg, NULL); }
        else { s_swing = atoi(arg); }
    } else if (strcmp(w, "scale") == 0) {
        int r;
        const seq_mode_t *m;
        if (seq_scale_parse(arg, &r, &m) != 0) { fail("scale '%s' is not a key", arg, NULL); }
        else { s_root = r; s_mode = m; }
    } else if (strcmp(w, "route") == 0) {
        do_route(arg);
    } else if (strcmp(w, "mute") == 0 || strcmp(w, "solo") == 0) {
        char buf[160];
        snprintf(buf, sizeof buf, "%.159s", arg);
        for (char *t = strtok(buf, " "); t != NULL; t = strtok(NULL, " ")) {
            bool any = false;
            for (int i = 0; i < SEQ_MAX_LANES; i++) {
                lane_name_t o;
                if (s_lane[i].used &&
                    lane_name_parse(s_lane[i].name, strlen(s_lane[i].name), &o) == LN_OK &&
                    (strcmp(s_lane[i].name, t) == 0 || strcmp(o.base, t) == 0)) {
                    any = true;
                    s_lane[i].muted = (w[0] == 'm');
                }
            }
            if (!any) { fail("%s %s: no such lane playing", w, t); }
        }
        if (arg[0] == '\0') {
            for (int i = 0; i < SEQ_MAX_LANES; i++) { s_lane[i].muted = false; }
        }
    } else if (strcmp(w, "play") == 0 || strcmp(w, "stop") == 0 ||
               strcmp(w, "panic") == 0 || strcmp(w, "lanes") == 0 ||
               strcmp(w, "split") == 0 || strcmp(w, "jitter") == 0 ||
               strcmp(w, "sync") == 0) {
        /* nothing to check */
    } else {
        fail("'%s' does not belong in a piece", w, NULL);
        return;
    }
    if (strcmp(w, "bpm") == 0 && whole(arg, 20, 300)) { s_bpm = atoi(arg); }
}

static void run_line(const char *line)
{
    const char *p = line;
    while (*p == ' ' || *p == '\t') { p++; }
    if (*p != '>') { return; }
    p++;
    while (*p == ' ' || *p == '\t') { p++; }
    if (*p == '\0' || *p == '#') { return; }
    const char *w = p;
    while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '=') { p++; }
    const size_t n = (size_t)(p - w);
    const char *arg = p;
    while (*arg == ' ' || *arg == '\t') { arg++; }
    char word[32];
    snprintf(word, sizeof word, "%.*s", (int)n, w);
    char a[160];
    snprintf(a, sizeof a, "%s", arg);
    for (size_t k = strlen(a); k > 0 && (a[k - 1] == ' ' || a[k - 1] == '\r'); k--) {
        a[k - 1] = '\0';
    }
    if (a[0] == '=') {
        do_define(w, n, a);
        return;
    }
    if (is_verb(word)) {
        do_verb(word, a);
        return;
    }
    char base[LANE_BASE_MAX + 2];
    size_t k = 0;
    while (k < n && w[k] != ':' && k <= LANE_BASE_MAX) { base[k] = w[k]; k++; }
    base[k < sizeof base ? k : sizeof base - 1] = '\0';
    if (alias_find(base) == NULL && viz_prim_index(base) < 0) {
        fail("%s? try: help", word, NULL);
        return;
    }
    do_lane(w, n, a);
}

/* ---------------------------------------------------------------- score */

static char s_ev[2048];
static double s_now_ms;              /* the tick being scored, in ms      */
static double s_end_ms[16][128];     /* when each channel's note ends     */
static int s_cuts;

static void say(const lane_t *l, const char *what, bool maybe)
{
    const size_t u = strlen(s_ev);
    snprintf(s_ev + u, sizeof s_ev - u, "  %s:%s%s", l->name, what, maybe ? "?" : "");
}

static void published(const lane_t *src, int value)
{
    for (int j = 0; j < SEQ_MAX_LANES; j++) {
        lane_t *d = &s_lane[j];
        if (d->used && d->route[0] != '\0' && strcmp(d->route, src->name) == 0) {
            d->trig = true;
            d->trig_val = value;
        }
    }
}

static void publish_end(const lane_t *src)
{
    char end[LANE_NAME_MAX + 8];
    snprintf(end, sizeof end, "%s:end", src->name);
    for (int j = 0; j < SEQ_MAX_LANES; j++) {
        lane_t *d = &s_lane[j];
        if (d->used && strcmp(d->route, end) == 0) {
            d->trig = true;
            d->trig_val = 127;
        }
    }
}

static lane_t *parent_of(const lane_t *part)
{
    const char *colon = strrchr(part->name, ':');
    if (colon == NULL) { return NULL; }
    const size_t n = (size_t)(colon - part->name);
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        lane_t *l = &s_lane[i];
        if (l->used && l != part && strncmp(l->name, part->name, n) == 0 &&
            l->name[n] == '\0') {
            return l;
        }
    }
    return NULL;
}

static int vel_of(const lane_t *l, int val)
{
    if (val == SEQ_VAL_X) { return l->vel; }
    const int v = (val * 127 + 4) / 9;
    return v < 1 ? 1 : v;
}

/* seq.c fire_event(), printing what the wire would carry. */
static void fire(lane_t *l, const seq_leaf_t *e, bool routed, bool maybe)
{
    char t[48];
    const int val = e ? e->val : SEQ_VAL_X;
    if (l->part == 'v' || l->part == 'o') {
        const int amt = routed ? (l->trig_val * 9 + 63) / 127
                               : (val == SEQ_VAL_X ? -1 : val);
        lane_t *p = parent_of(l);
        if (p != NULL) {
            if (l->part == 'v') { p->vel = amt < 0 ? 100 : vel_of(p, amt); }
            else { p->oct = amt < 0 ? p->num : (amt > 8 ? 8 : amt); }
        }
        snprintf(t, sizeof t, "%d", amt);
        say(l, t, maybe);
        published(l, (amt < 0 ? 9 : amt) * 127 / 9);
        return;
    }
    if (l->kind == LD_DRAW) {
        int amt = routed ? (l->trig_val * 9 + 63) / 127 : (val == SEQ_VAL_X ? 9 : val);
        if (amt > 9) { amt = 9; }
        snprintf(t, sizeof t, "%c%d", (e && e->dir) ? e->dir : '#', amt);
        say(l, t, maybe);
        published(l, amt * 127 / 9);
        return;
    }
    if (l->kind == LD_CC) {
        if (!routed && val == SEQ_VAL_X) { return; }
        const int v = routed ? (l->trig_val & 0x7F) : (val * 127) / 9;
        snprintf(t, sizeof t, "=%d", v);
        say(l, t, maybe);
        published(l, v);
        return;
    }
    const bool melodic = (l->kind == LD_VOICE);
    int vel = routed ? l->trig_val : (melodic ? l->vel : vel_of(l, val));
    if (vel < 1) { vel = 1; }
    if (vel > 127) { vel = 127; }
    if (melodic) {
        const int n = seq_degree_note(s_root, s_mode, val == SEQ_VAL_X ? 0 : (uint8_t)val,
                                      l->oct);
        static const char *nm[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G",
                                    "G#", "A", "A#", "B" };
        const int held = (e && l->c.div) ? (e->len * l->c.rden * 10) /
                                           (l->c.div * l->c.rnum) : 10;
        if (held >= 20) {
            snprintf(t, sizeof t, "%s%d~%d", nm[n % 12], n / 12 - 1, held / 10);
        } else {
            snprintf(t, sizeof t, "%s%d", nm[n % 12], n / 12 - 1);
        }
        /* A NOTE-OFF IS SCHEDULED, NOT PAIRED (seq.c schedule_off): a held
         * note whose gate runs past the next note of the same pitch on the
         * same channel ends THAT one, early. A gate is milliseconds, so this
         * depends on the tempo, and only the score can see it. */
        const int ch = (l->chan - 1) & 15;
        if (s_end_ms[ch][n] > s_now_ms + 0.5) {
            const size_t u = strlen(t);
            snprintf(t + u, sizeof t - u, "!cut%.0fms", s_end_ms[ch][n] - s_now_ms);
            s_cuts++;
        }
        const double step_ms = 60000.0 / s_bpm / 4.0;
        const double hold = (e && l->c.div) ? (double)(e->len - e->width) * l->c.rden /
                                              ((double)l->c.div * l->c.rnum) : 0.0;
        s_end_ms[ch][n] = s_now_ms + l->gate + hold * step_ms;
    } else {
        snprintf(t, sizeof t, "%d", vel);
    }
    say(l, t, maybe);
    published(l, vel);
}

static void rerank(void)
{
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        lane_t *l = &s_lane[i];
        int hops = 0;
        const char *up = l->route;
        while (l->used && up[0] != '\0' && hops <= SEQ_MAX_LANES) {
            char src[LANE_NAME_MAX + 8];
            snprintf(src, sizeof src, "%s", up);
            const size_t n = strlen(src);
            if (n > 4 && strcmp(src + n - 4, ":end") == 0) { src[n - 4] = '\0'; }
            const lane_t *s = lane_find(src);
            if (s == NULL) { break; }
            hops++;
            up = s->route;
        }
        l->rank = hops;
    }
}

/* seq.c fire_lanes(), one tick: in route order, parts first; a sidechain fires
 * on its source's hit, a count plays its passes and then is a source, a cue is
 * started by its source every time. Swing 0 - the score is the grid - and odds
 * are marked, not rolled. */
static void tick_lanes(uint32_t tick)
{
    int maxrank = 0;
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        if (s_lane[i].used && s_lane[i].rank > maxrank) { maxrank = s_lane[i].rank; }
    }
    for (int pass = 0; pass < (maxrank + 1) * 2; pass++)
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        lane_t *l = &s_lane[i];
        if (!l->used || l->muted || !l->has_pat || l->c.slots == 0) { continue; }
        const bool part = (l->part == 'v' || l->part == 'o');
        if (pass != l->rank * 2 + (part ? 0 : 1)) { continue; }
        const bool routed = l->route[0] != '\0';
        if (routed && l->c.count == 0) {
            if (!l->trig) { continue; }
            l->trig = false;
            const seq_leaf_t *first = NULL;
            for (int k = 0; k < l->c.n && first == NULL; k++) {
                if (l->c.leaf[k].kind == SEQ_LEAF_HIT) { first = &l->c.leaf[k]; }
            }
            fire(l, first, true, false);
            continue;
        }
        int s = 0;
        uint32_t cy = 0;
        if (!seq_pattern_slot_at(tick, l->c.slots, l->c.div, l->c.rnum, l->c.rden, 0,
                                 &s, &cy)) {
            continue;
        }
        if (l->c.count != 0) {
            const uint32_t g = cy * (uint32_t)l->c.slots + (uint32_t)s;
            if (routed) {
                if (l->trig) { l->trig = false; l->idle = false; l->origin = g; }
            } else if (l->idle) {
                if (s != 0) { continue; }
                l->idle = false;
                l->origin = g;
            }
            if (l->idle) { continue; }
            const uint32_t local = g - l->origin;
            cy = local / (uint32_t)l->c.slots;
            s = (int)(local % (uint32_t)l->c.slots);
            if (cy >= (uint32_t)l->c.count) {
                l->idle = true;
                if (!routed) { l->done = true; l->muted = true; }
                say(l, "end", false);
                publish_end(l);
                continue;
            }
        }
        for (int k = 0; k < l->c.n; k++) {
            const seq_leaf_t *e = &l->c.leaf[k];
            if (e->slot != s || e->kind != SEQ_LEAF_HIT) { continue; }
            if (e->per > 1 && (int)(cy % e->per) != e->ph) { continue; }
            fire(l, e, false, e->prob != SEQ_PROB_ALWAYS);
        }
    }
}

static void score(int bars, const char *title)
{
    static const char *nm[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G",
                                "G#", "A", "A#", "B" };
    printf("\n=== %s: %d bpm, %s%s, swing %d\n", title, s_bpm, nm[s_root],
           s_mode->name, s_swing);
    /* a copy, so scoring a section changes nothing the next lines see */
    static lane_t saved[SEQ_MAX_LANES];
    memcpy(saved, s_lane, sizeof saved);
    int nl = 0;
    printf("   ");
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        lane_t *l = &s_lane[i];
        if (!l->used) { continue; }
        nl++;
        l->trig = false;
        l->idle = (l->c.count != 0);
        l->done = false;
        l->vel = 100;
        l->oct = l->num;
        printf(" %s%s%s%s", l->name, l->muted ? "(muted)" : "",
               l->route[0] ? "<" : "", l->route);
    }
    printf("   [%d of %d lanes]\n", nl, SEQ_MAX_LANES);
    rerank();
    memset(s_end_ms, 0, sizeof s_end_ms);
    for (uint32_t tick = 0; tick < (uint32_t)bars * 384; tick++) {
        s_ev[0] = '\0';
        s_now_ms = tick * 60000.0 / s_bpm / 96.0;
        tick_lanes(tick);
        if (s_ev[0] == '\0') { continue; }
        const uint32_t step = tick / 24, sub = tick % 24;
        char at[16];
        if (sub == 0) {
            snprintf(at, sizeof at, "%u.%u", step / 16 + 1, step % 16 + 1);
        } else {
            snprintf(at, sizeof at, "%u.%u+%u", step / 16 + 1, step % 16 + 1, sub);
        }
        printf("  %-8s%s\n", at, s_ev);
    }
    memcpy(s_lane, saved, sizeof saved);
    if (s_cuts > 0) {
        printf("  ** %d note(s) cut short by an earlier note-off of the same pitch\n", s_cuts);
        s_cuts = 0;
    }
}

/* ---------------------------------------------------------------- main */

static void run_text(const char *text, const char *file, int want_score, int bars)
{
    s_file = file;
    s_lineno = 0;
    char section[128] = "(top)";
    const char *p = text;
    while (*p != '\0') {
        const char *e = strchr(p, '\n');
        const size_t n = e ? (size_t)(e - p) : strlen(p);
        char line[512];
        snprintf(line, sizeof line, "%.*s", (int)(n < sizeof line - 1 ? n : sizeof line - 1), p);
        s_lineno++;
        if (want_score && strncmp(line, "--", 2) == 0) {
            score(bars, section);
            snprintf(section, sizeof section, "%.120s", line);
        }
        if (line[0] == '>' || (line[0] == ' ' && strchr(line, '>'))) {
            s_lines++;
            if (n > LINE_MAX_RUN) {
                fail("line is %s characters; >run reads %s", "long", "127");
            }
            if (n > WIDE) { s_wide++; }
        }
        run_line(line);
        p = e ? e + 1 : p + n;
    }
    if (want_score) { score(bars, section); }
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage: test_pieces BUILTINS_C [-s] [-b BARS] FILE...\n");
        return 2;
    }
    load_verbs(argv[1]);
    int want_score = 0, bars = 2, files = 0;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) { want_score = 1; continue; }
        if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) { bars = atoi(argv[++i]); continue; }
        FILE *f = fopen(argv[i], "r");
        if (f == NULL) {
            printf("[FAIL] cannot read %s\n", argv[i]);
            fails++;
            continue;
        }
        static char text[64 * 1024];
        const size_t n = fread(text, 1, sizeof text - 1, f);
        fclose(f);
        text[n] = '\0';
        /* every piece starts where the deck does: the boot names, nothing
         * playing, the boot document's key and tempo */
        memset(s_alias, 0, sizeof s_alias);
        memset(s_lane, 0, sizeof s_lane);
        memset(s_input, 0, sizeof s_input);
        s_bpm = 124; s_swing = 50; s_root = 2; s_mode = &seq_modes[SEQ_MODE_MIN];
        run_text(BOOT_NAMES, "boot", 0, 0);
        const int before = fails;
        s_lines = 0;
        s_wide = 0;
        run_text(text, argv[i], want_score, bars);
        files++;
        printf("[%s] %s: %d lines, %d wider than %d columns\n",
               fails == before ? " ok " : "FAIL", argv[i], s_lines, s_wide, WIDE);
    }
    if (fails) {
        printf("[FAIL] %d line(s) the deck would refuse\n", fails);
    } else {
        printf("[PASS] %d piece(s): every line is one the deck runs\n", files);
    }
    return fails != 0;
}
