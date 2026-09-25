/*
 * The Strudel conformance corpus, checked against the shipping compiler.
 *
 * docs/NEXT.md §11. Each entry in tools/corpus/strudel.txt is a Strudel
 * mini-notation string, its translation into this deck's step grammar, and the
 * events Strudel itself played for it (tools/corpus/gen.mjs). This compiles the
 * translation with seq_pattern.h - the header the firmware builds - and checks
 * that the deck plays the same notes, at the same times, for the same lengths,
 * cycle by cycle.
 *
 * Times are compared as fractions of a cycle. A cycle is one bar to Strudel and
 * one pass of the lane to the deck - its own length in sixteenths - so the two
 * agree on everything inside a bar, which is what the notation says. '?' is
 * compared as the notes that CAN play: Strudel's randomness is a function of
 * time and the deck's is a seeded roll, so the same odds do not give the same
 * notes, and that is not a disagreement about the notation.
 *
 *   test_corpus FILE [FILE...]
 */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "seq_pattern.h"

typedef struct { int64_t b_num, b_den, d_num, d_den; char v; } ev_t;

#define MAX_EV 4096

static int cmp_frac(int64_t an, int64_t ad, int64_t bn, int64_t bd)
{
    const int64_t l = an * bd, r = bn * ad;
    return (l > r) - (l < r);
}

static int cmp_ev(const void *pa, const void *pb)
{
    const ev_t *a = pa, *b = pb;
    int c = cmp_frac(a->b_num, a->b_den, b->b_num, b->b_den);
    if (c != 0) { return c; }
    if (a->v != b->v) { return (a->v > b->v) - (a->v < b->v); }
    /* two notes may start together with different lengths - '[x x, x x x]' */
    return cmp_frac(a->d_num, a->d_den, b->d_num, b->d_den);
}

static void parse_frac(const char *s, int64_t *num, int64_t *den)
{
    char *end = NULL;
    *num = strtoll(s, &end, 10);
    *den = 1;
    if (end && *end == '/') {
        *den = strtoll(end + 1, NULL, 10);
    }
}

/* "0:1/4:x 1/2:1/4:x ..." */
static int parse_events(char *line, ev_t *ev)
{
    int n = 0;
    for (char *tok = strtok(line, " "); tok && n < MAX_EV; tok = strtok(NULL, " ")) {
        char *c1 = strchr(tok, ':');
        char *c2 = c1 ? strchr(c1 + 1, ':') : NULL;
        if (!c1 || !c2) { continue; }
        *c1 = *c2 = '\0';
        parse_frac(tok, &ev[n].b_num, &ev[n].b_den);
        parse_frac(c1 + 1, &ev[n].d_num, &ev[n].d_den);
        ev[n].v = c2[1];
        n++;
    }
    return n;
}

static seq_comp_t C;

/* What the deck plays for `deck` over `cycles` cycles, every note that can. */
static int deck_events(const char *deck, int cycles, ev_t *ev, char *why, size_t wn)
{
    const int e = seq_pattern_compile(deck, &C);
    if (e != SEQ_PAT_OK) {
        seq_pattern_error_text(&C, deck, why, wn);
        return -1;
    }
    int n = 0;
    for (int c = 0; c < cycles; c++) {
        for (int i = 0; i < C.n && n < MAX_EV; i++) {
            const seq_leaf_t *L = &C.leaf[i];
            if (L->kind != SEQ_LEAF_HIT) { continue; }
            if (L->per > 1 && c % L->per != L->ph) { continue; }
            ev[n].b_num = (int64_t)c * C.slots + L->slot;
            ev[n].b_den = C.slots;
            ev[n].d_num = L->len;
            ev[n].d_den = C.slots;
            ev[n].v = (L->val == SEQ_VAL_X) ? 'x' : (char)('0' + L->val);
            n++;
        }
    }
    return n;
}

static ev_t s_want[MAX_EV], s_got[MAX_EV];

typedef struct {
    int in_scope, pass, fail, out;
    char reasons[64][96];
    int reason_n[64];
    int nreasons;
} tally_t;

static void count_reason(tally_t *t, const char *why)
{
    for (int i = 0; i < t->nreasons; i++) {
        if (strcmp(t->reasons[i], why) == 0) { t->reason_n[i]++; return; }
    }
    if (t->nreasons < 64) {
        snprintf(t->reasons[t->nreasons], sizeof t->reasons[0], "%s", why);
        t->reason_n[t->nreasons++] = 1;
    }
}

/* Check one entry. `rhythm` compares onsets and lengths only - the real-world
 * set, where a word is a hit. */
static void check(tally_t *t, const char *src, const char *deck, int cycles,
                  char *events, int rhythm, int verbose)
{
    t->in_scope++;
    char why[64];
    const int nw = parse_events(events, s_want);
    const int ng = deck_events(deck, cycles, s_got, why, sizeof why);
    if (ng < 0) {
        printf("[FAIL] '%s' -> '%s' does not compile: %s\n", src, deck, why);
        t->fail++;
        return;
    }
    if (rhythm) {
        /* values are not compared, so they must not decide the order either */
        for (int i = 0; i < nw; i++) { s_want[i].v = 'x'; }
        for (int i = 0; i < ng; i++) { s_got[i].v = 'x'; }
    }
    qsort(s_want, (size_t)nw, sizeof s_want[0], cmp_ev);
    qsort(s_got, (size_t)ng, sizeof s_got[0], cmp_ev);
    int bad = (nw != ng);
    int at = -1;
    for (int i = 0; i < nw && i < ng && !bad; i++) {
        const ev_t *w = &s_want[i], *g = &s_got[i];
        if (cmp_frac(w->b_num, w->b_den, g->b_num, g->b_den) != 0 ||
            cmp_frac(w->d_num, w->d_den, g->d_num, g->d_den) != 0 ||
            (!rhythm && w->v != g->v)) {
            bad = 1;
            at = i;
        }
    }
    if (bad) {
        t->fail++;
        printf("[FAIL] '%s' -> '%s': %d notes, Strudel %d\n", src, deck, ng, nw);
        if (at >= 0) {
            const ev_t *w = &s_want[at], *g = &s_got[at];
            printf("       first difference, note %d: deck %" PRId64 "/%" PRId64
                   "+%" PRId64 "/%" PRId64 " '%c', Strudel %" PRId64 "/%" PRId64
                   "+%" PRId64 "/%" PRId64 " '%c'\n", at,
                   g->b_num, g->b_den, g->d_num, g->d_den, g->v,
                   w->b_num, w->b_den, w->d_num, w->d_den, w->v);
        }
        return;
    }
    t->pass++;
    if (verbose) {
        printf("[ ok ] %-28s %-24s %d notes\n", src, deck, ng);
    }
}

static int run(const char *path, int verbose)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        printf("[FAIL] cannot read %s\n", path);
        return 1;
    }
    static char line[1 << 20];
    static char src[4096], deck[4096];
    static char events[1 << 20];
    tally_t feat = { 0 }, world = { 0 };
    tally_t *t = &feat;
    int cycles = 0, have = 0, rhythm = 0;
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\n")] = '\0';
        if (strncmp(line, "## world", 8) == 0) { t = &world; continue; }
        if (line[0] == 'S' || line[0] == 'W') {
            snprintf(src, sizeof src, "%s", line + 2);
            rhythm = (line[0] == 'W');
            have = 0;
        } else if (line[0] == 'D') {
            snprintf(deck, sizeof deck, "%s", line + 2);
        } else if (line[0] == 'N') {
            cycles = atoi(line + 2);
        } else if (line[0] == 'E') {
            snprintf(events, sizeof events, "%s", line + 2);
            have = 1;
            check(t, src, deck, cycles, events, rhythm, verbose && t == &feat);
        } else if (line[0] == 'X') {
            t->out++;
            count_reason(t, line + 2);
            if (verbose && t == &feat) {
                printf("[out ] %-28s %s\n", src, line + 2);
            }
        }
        (void)have;
    }
    fclose(f);
    printf("\nfeatures: %d in scope, %d match Strudel, %d differ; %d out of scope\n",
           feat.in_scope, feat.pass, feat.fail, feat.out);
    if (world.in_scope + world.out > 0) {
        const int all = world.in_scope + world.out;
        printf("world:    %d patterns; %d in scope (%d%%), %d match Strudel's rhythm,"
               " %d differ; %d out of scope\n", all, world.in_scope,
               100 * world.in_scope / all, world.pass, world.fail, world.out);
        for (int i = 0; i < world.nreasons; i++) {
            printf("          %3d  %s\n", world.reason_n[i], world.reasons[i]);
        }
    }
    return (feat.fail + world.fail) != 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: test_corpus FILE [FILE...]\n");
        return 2;
    }
    int bad = 0;
    for (int i = 1; i < argc; i++) {
        bad |= run(argv[i], 1);
    }
    printf(bad ? "[FAIL] the deck and Strudel disagree\n"
               : "[PASS] the deck plays what Strudel plays, for all of it in scope\n");
    return bad;
}
