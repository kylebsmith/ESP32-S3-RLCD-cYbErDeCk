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
#include "lane_name.h"
#include "secret_line.h"

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
 * removed, and was playing its own comments - '>echo 8    then add:' compiled
 * nine more hits. A tutorial whose lines are refused is worse than none, because
 * the owner trusts it. So every command line in it must be a verb the firmware
 * has - read from the command table, as docs/MAP.md counts verbs - or a lane
 * whose NAME exists and whose pattern compiles under the compiler that ships.
 *
 * Names are not verbs any more (docs/MANIFESTO.md §3.8): they are definitions in
 * the boot document, or a picture's own name. So this reads the definitions out
 * of BOOT_TEXT and the pictures out of viz.c, and a definition in the guide adds
 * a name for the lines after it, exactly as running it would. */
typedef struct { char name[16]; char fn[16]; } verb_t;
static verb_t s_verbs[128];
static int s_nverbs;
static char s_names[96][LANE_BASE_MAX + 1];
static int s_nnames;

static char *slurp(const char *path, char *buf, size_t n)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        printf("[FAIL] cannot read %s (run from the repo root)\n", path);
        fails++;
        return NULL;
    }
    const size_t got = fread(buf, 1, n - 1, f);
    fclose(f);
    buf[got] = '\0';
    return buf;
}

static void load_verbs(void)
{
    static char src[200000];
    if (slurp("firmware/components/cmd/builtins.c", src, sizeof src) == NULL) {
        return;
    }
    const char *t = strstr(src, "static const cmd_t s_builtins[]");
    const char *end = t ? strstr(t, "};") : NULL;
    for (const char *p = t; p && p < end && s_nverbs < 128; ) {
        const char *q = strstr(p, "{ \"");
        if (q == NULL || q > end) { break; }
        verb_t *v = &s_verbs[s_nverbs];
        if (sscanf(q, "{ \"%15[a-z0-9]\", %15[a-z_]", v->name, v->fn) == 2) {
            s_nverbs++;
        }
        p = q + 3;
    }
}

static void add_name(const char *w, size_t n)
{
    if (s_nnames < 96 && n <= LANE_BASE_MAX) {
        memcpy(s_names[s_nnames], w, n);
        s_names[s_nnames][n] = '\0';
        s_nnames++;
    }
}

static int is_name(const char *w)
{
    for (int i = 0; i < s_nnames; i++) {
        if (strcmp(s_names[i], w) == 0) { return 1; }
    }
    return 0;
}

static const verb_t *verb(const char *w, size_t n)
{
    for (int i = 0; i < s_nverbs; i++) {
        if (strlen(s_verbs[i].name) == n && strncmp(s_verbs[i].name, w, n) == 0) {
            return &s_verbs[i];
        }
    }
    return NULL;
}

static void load_pictures(void)
{
    static char src[120000];
    if (slurp("firmware/components/viz/viz.c", src, sizeof src) == NULL) {
        return;
    }
    const char *t = strstr(src, "static const char *s_names[NGEN] = {");
    const char *end = t ? strstr(t, "};") : NULL;
    for (const char *p = t ? strchr(t, '{') : NULL; p && p < end; ) {
        const char *q = strchr(p, '"');
        if (q == NULL || q > end) { break; }
        const char *r = strchr(q + 1, '"');
        add_name(q + 1, (size_t)(r - q - 1));
        p = r + 1;
    }
}

/* One command line, checked the way cmd_run_line would dispatch it. `what` names
 * the text for the message. Returns 1 if it is a lane line that was compiled. */
static int check_line(const char *what, int ln, const char *line)
{
    static seq_comp_t c;
    const char *w = line + 1;
    size_t wl = 0;
    while (w[wl] != '\0' && w[wl] != ' ' && w[wl] != '=') { wl++; }
    const char *rest = w + wl;
    while (*rest == ' ') { rest++; }
    /* NO PASSWORD ON A LINE. "guide 2" taught '>wifi <ssid> <pass>' and
     * '>host deck 12345678': a password typed into a document, which is
     * journalled, mirrored to the card and copied to the owner's DGX. Both take
     * one word now and ask for the password (firmware/main/ask.h), so a line
     * that gives them a second word is teaching the old habit. */
    if (wl == 4 && (strncmp(w, "wifi", 4) == 0 || strncmp(w, "host", 4) == 0)) {
        char word[40];
        size_t n = 0;
        if (first_word_rest(rest, word, sizeof word, &n) >= 0) {
            printf("[FAIL] %s line %d puts a password on a line: %s\n",
                   what, ln, line);
            fails++;
        }
    }
    if (verb(w, wl) != NULL) {
        return 0;
    }
    lane_name_t nm;
    const int ne = lane_name_parse(w, wl, &nm);
    if (*rest == '=') {
        lane_def_t d;
        lane_def_parse(rest + 1, &d);
        if (ne != LN_OK || d.kind == LD_ERROR || d.kind == LD_REMOVE ||
            (d.kind == LD_DRAW && !is_name(d.draw))) {
            printf("[FAIL] %s line %d does not define a name: %s (%s)\n",
                   what, ln, line, d.kind == LD_ERROR ? d.why : "bad name");
            fails++;
            return 0;
        }
        add_name(nm.base, strlen(nm.base));
        return 0;
    }
    if (ne != LN_OK || !is_name(nm.base)) {
        printf("[FAIL] %s line %d runs '%.*s', which is neither a verb nor a "
               "name\n", what, ln, (int)wl, w);
        fails++;
        return 0;
    }
    if (*rest == '\0') {
        return 0;
    }
    const int e = seq_pattern_compile(rest, &c);
    if (e != SEQ_PAT_OK) {
        char why[64];
        seq_pattern_error_text(&c, rest, why, sizeof why);
        printf("[FAIL] %s line %d is refused: %s (%s)\n", what, ln, line, why);
        fails++;
        return 0;
    }
    return 1;
}

/* Walk a text, checking every command line in it. Returns lane lines checked. */
static int check_text(const char *what, const char *text)
{
    int ln = 1, lanes = 0;
    while (*text != '\0') {
        const char *nl = strchr(text, '\n');
        const size_t n = nl ? (size_t)(nl - text) : strlen(text);
        char line[64];
        snprintf(line, sizeof line, "%.*s", (int)n, text);
        if (line[0] == '>' && strstr(line, "   ") != NULL) {
            /* The cheat-sheet style - '>sync on    MIDI clock out' - made the
             * comment part of the argument, so the line failed when it was run:
             * c_sync compared "on    MIDI clock out" with "on". A comment
             * goes on its own line. */
            printf("[FAIL] %s line %d has a comment inside it: %s\n", what, ln, line);
            fails++;
        }
        if (line[0] == '>') {
            lanes += check_line(what, ln, line);
        }
        if (!nl) { break; }
        text = nl + 1;
        ln++;
    }
    return lanes;
}

static void guide_runs(void)
{
    load_verbs();
    load_pictures();
    if (s_nverbs < 30 || s_nnames != 16) {
        printf("[FAIL] read %d verbs and %d pictures - the parse is wrong\n",
               s_nverbs, s_nnames);
        fails++;
        return;
    }
    /* The boot document runs first, and it is what defines the sound names. */
    check_text("BOOT_TEXT", BOOT_TEXT);
    if (!is_name("kick") || !is_name("bass") || !is_name("cut")) {
        printf("[FAIL] the boot document does not define kick, bass and cut\n");
        fails++;
    }
    if (strstr(BOOT_TEXT, BOOT_MARK) == NULL ||
        strncmp(BOOT_NAMES, BOOT_MARK, sizeof BOOT_MARK - 1) != 0) {
        printf("[FAIL] the names block must start with its mark '%s'\n", BOOT_MARK);
        fails++;
    }
    const int checked = check_text("guide", GUIDE_TEXT);
    /* and every picture the firmware can draw is mentioned, so adding one
     * without teaching it fails here rather than going unnoticed */
    static const char guide[] = GUIDE_TEXT;
    for (int i = 0; i < 16; i++) {
        const char *hit = strstr(guide, s_names[i]);
        const size_t k = strlen(s_names[i]);
        int word = 0;
        while (hit != NULL) {
            const char before = (hit == guide) ? ' ' : hit[-1];
            const char after = hit[k];
            if ((before == ' ' || before == '\n' || before == '>') &&
                (after == ' ' || after == '\n' || after == '.' || after == ',' ||
                 after == ':')) {
                word = 1;
                break;
            }
            hit = strstr(hit + 1, s_names[i]);
        }
        if (!word) {
            printf("[FAIL] the guide never mentions the picture '%s'\n", s_names[i]);
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
