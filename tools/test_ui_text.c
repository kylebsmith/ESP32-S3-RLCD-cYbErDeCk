/*
 * Every fixed on-device string must fit the narrowest grid the device renders.
 *
 * "not a command - start the line with >" is 37 characters against a 30-column
 * status bar, so it rendered as "not a command - start the line" and the half
 * that says what to do never reached the glass. The owner hit exactly that:
 * a line would not run and the deck would not say why.
 *
 * This includes the SHIPPING header, not a copy of it - the same arrangement
 * as test_st7305_addr.c and test_mirror_path.c, and for the same reason.
 */
#include <stdio.h>
#include <string.h>

/* The header's own _Static_asserts catch this at device-build time. Disable
 * them here so this program can REPORT every violation instead of dying on
 * the first one - a list is more useful than an abort. */
#define UI_TEXT_NO_ASSERTS
#include "ui_text.h"
#include "seq_pattern.h"

#include <stdlib.h>

static int fails;

static void fits(const char *what, const char *s)
{
    const size_t n = strlen(s);
    if (n > UI_NARROW_COLS) {
        printf("[FAIL] %s is %zu chars, %d columns: \"%s\"\n",
               what, n, UI_NARROW_COLS, s);
        printf("       renders as \"%.*s\"\n", UI_NARROW_COLS, s);
        fails++;
    }
}

/* Every line of the guide, split on newlines, must fit too - a guide line
 * that wraps is not wrong, but a COMMAND line that wraps is harder to run and
 * harder to read while it is playing. */
static void guide_lines(void)
{
    const char *g = GUIDE_TEXT;
    int ln = 1;
    while (*g != '\0') {
        const char *nl = strchr(g, '\n');
        const size_t n = nl ? (size_t)(nl - g) : strlen(g);
        if (n > UI_NARROW_COLS) {
            printf("[FAIL] GUIDE_TEXT line %d is %zu chars: \"%.*s\"\n",
                   ln, n, (int)n, g);
            fails++;
        }
        if (!nl) { break; }
        g = nl + 1;
        ln++;
    }
}

/* THE GUIDE MUST TEACH LINES THAT RUN.
 *
 * The guide went on teaching 'star', 'shake' and 'tile' after all three had been
 * removed, and 'X' and ',' after the step grammar changed would have been next:
 * a tutorial whose lines are refused is worse than none, because the owner
 * trusts it. So every command line in it must name a verb the firmware has - read
 * from the command table itself, as docs/MAP.md counts verbs - and every lane
 * line's pattern must compile under the compiler that ships. */
typedef struct { char name[16]; char fn[16]; int lane; } verb_t;
static verb_t s_verbs[128];
static int s_nverbs;

static void load_verbs(void)
{
    FILE *f = fopen("firmware/components/cmd/builtins.c", "r");
    if (f == NULL) {
        printf("[FAIL] cannot read the command table (run from the repo root)\n");
        fails++;
        return;
    }
    static char src[200000];
    const size_t n = fread(src, 1, sizeof src - 1, f);
    fclose(f);
    src[n] = '\0';
    const char *t = strstr(src, "static const cmd_t s_builtins[]");
    const char *end = t ? strstr(t, "};") : NULL;
    for (const char *p = t; p && p < end && s_nverbs < 128; ) {
        const char *q = strstr(p, "{ \"");
        if (q == NULL || q > end) { break; }
        verb_t *v = &s_verbs[s_nverbs];
        if (sscanf(q, "{ \"%15[a-z0-9]\", %15[a-z_]", v->name, v->fn) == 2) {
            const char *eol = strchr(q, '\n');
            v->lane = (eol && strstr(q, "CMD_CAP_LANE") && strstr(q, "CMD_CAP_LANE") < eol);
            s_nverbs++;
        }
        p = q + 3;
    }
}

static const verb_t *verb(const char *w, size_t n)
{
    /* exact, then the lane rules: a trailing '[part]', then trailing digits */
    for (int pass = 0; pass < 2; pass++) {
        size_t b = n;
        if (pass == 1) {
            if (b > 2 && w[b - 1] == ']') {
                while (b > 0 && w[b - 1] != '[') { b--; }
                if (b > 0) { b--; }
            }
            while (b > 1 && w[b - 1] >= '0' && w[b - 1] <= '9') { b--; }
        }
        for (int i = 0; i < s_nverbs; i++) {
            if (strlen(s_verbs[i].name) == b && strncmp(s_verbs[i].name, w, b) == 0 &&
                (pass == 0 || s_verbs[i].lane)) {
                return &s_verbs[i];
            }
        }
    }
    return NULL;
}

static void guide_runs(void)
{
    load_verbs();
    if (s_nverbs < 40) {
        printf("[FAIL] read only %d verbs from the table\n", s_nverbs);
        fails++;
        return;
    }
    static seq_comp_t c;
    const char *g = GUIDE_TEXT;
    int ln = 1, checked = 0;
    while (*g != '\0') {
        const char *nl = strchr(g, '\n');
        const size_t n = nl ? (size_t)(nl - g) : strlen(g);
        char line[64];
        snprintf(line, sizeof line, "%.*s", (int)n, g);
        if (line[0] == '>' && strstr(line, "   ") != NULL) {
            /* The cheat-sheet style - '>sync on    MIDI clock out' - made the
             * comment part of the argument, so the line failed when it was run:
             * c_sync compared "on    MIDI clock out" with "on". A comment
             * goes on its own line. */
            printf("[FAIL] guide line %d has a comment inside it: %s\n", ln, line);
            fails++;
        }
        if (line[0] == '>') {
            const char *w = line + 1;
            size_t wl = strcspn(w, " ");
            const verb_t *v = verb(w, wl);
            if (v == NULL) {
                printf("[FAIL] guide line %d runs '%.*s', which is not a verb\n",
                       ln, (int)wl, w);
                fails++;
            } else if (v->lane) {
                const char *arg = w + wl;
                while (*arg == ' ') { arg++; }
                if (strcmp(v->name, "cc") == 0) {       /* '>cc cut 0..9..' */
                    arg += strcspn(arg, " ");
                    while (*arg == ' ') { arg++; }
                }
                if (*arg != '\0') {
                    const int e = seq_pattern_compile(arg, &c);
                    if (e != SEQ_PAT_OK) {
                        char why[64];
                        seq_pattern_error_text(&c, arg, why, sizeof why);
                        printf("[FAIL] guide line %d is refused: %s (%s)\n",
                               ln, line, why);
                        fails++;
                    }
                    checked++;
                }
            }
        }
        if (!nl) { break; }
        g = nl + 1;
        ln++;
    }
    /* and every picture the firmware can draw is mentioned, so adding one
     * without teaching it fails here rather than going unnoticed */
    static const char guide[] = GUIDE_TEXT;
    for (int i = 0; i < s_nverbs; i++) {
        if (strcmp(s_verbs[i].fn, "c_prim") != 0) { continue; }
        const char *hit = strstr(guide, s_verbs[i].name);
        const size_t k = strlen(s_verbs[i].name);
        int word = 0;
        while (hit != NULL) {
            const char before = (hit == guide) ? ' ' : hit[-1];
            const char after = hit[k];
            if ((before == ' ' || before == '\n' || before == '>') &&
                (after == ' ' || after == '\n' || after == '.' || after == ',')) {
                word = 1;
                break;
            }
            hit = strstr(hit + 1, s_verbs[i].name);
        }
        if (!word) {
            printf("[FAIL] the guide never mentions the primitive '%s'\n",
                   s_verbs[i].name);
            fails++;
        }
    }
    if (checked < 5) {
        printf("[FAIL] only %d lane lines checked - the parse is wrong\n", checked);
        fails++;
    }
    if (strstr(GUIDE_TEXT, GUIDE_MARK) == NULL) {
        printf("[FAIL] the guide does not carry its mark '%s'\n", GUIDE_MARK);
        fails++;
    }
}

int main(void)
{
    fits("UI_NOT_A_COMMAND", UI_NOT_A_COMMAND);
    fits("UI_GUIDE_HINT",    UI_GUIDE_HINT);
    fits("UI_NO_GUIDE",      UI_NO_GUIDE);
    guide_lines();
    guide_runs();

    /* The hint must not claim Enter runs a line. It does not - Enter always
     * inserts so that a line can be added after a command, and Ctrl+Enter
     * runs. The old hint said the opposite, and an owner who believed it
     * would split their guide in half instead of running anything. */
    if (strstr(UI_GUIDE_HINT, "Ctrl+Enter") == NULL) {
        printf("[FAIL] the guide hint must name Ctrl+Enter, not Enter\n");
        fails++;
    }

    /* The message a stuck owner reads must tell them what to DO, not only
     * that something is wrong. */
    if (strchr(UI_NOT_A_COMMAND, '>') == NULL) {
        printf("[FAIL] 'not a command' must say what a command starts with\n");
        fails++;
    }

    printf(fails ? "[FAIL] %d string(s) do not fit or do not say enough\n"
                 : "[PASS] on-device strings fit %d columns\n",
           fails ? fails : UI_NARROW_COLS);
    return fails != 0;
}
