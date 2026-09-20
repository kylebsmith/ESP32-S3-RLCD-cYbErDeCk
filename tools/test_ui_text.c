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

int main(void)
{
    fits("UI_NOT_A_COMMAND", UI_NOT_A_COMMAND);
    fits("UI_GUIDE_HINT",    UI_GUIDE_HINT);
    fits("UI_NO_GUIDE",      UI_NO_GUIDE);
    guide_lines();

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
