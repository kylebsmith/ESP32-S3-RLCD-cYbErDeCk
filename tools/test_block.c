/*
 * Ctrl+Enter runs a line and its block - checked with firmware/main/
 * editor_block.h, the rule the editor runs: which lines are a line's block,
 * where it ends, blank lines inside it, tabs against spaces, nesting.
 */
#include <stdio.h>
#include <string.h>
#include "editor_block.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf("[FAIL] " __VA_ARGS__); printf("\n"); fails++; } \
                           else { printf("[ ok ] " __VA_ARGS__); printf("\n"); } } while (0)

static const char *s_text;
static char at(size_t i) { return s_text[i]; }

/* The block under the line numbered `ln` (0 first), joined with '|'. */
static int block_of(const char *text, int ln, char *out, size_t max)
{
    s_text = text;
    size_t s = 0;
    for (int k = 0; k < ln; k++) { s = strchr(text + s, '\n') - text + 1; }
    static char lines[BLOCK_MAX_LINES][BLOCK_LINE_MAX];
    const int n = block_collect(at, strlen(text), s, lines, BLOCK_MAX_LINES);
    out[0] = '\0';
    for (int i = 0; i < n; i++) {
        if (i) strncat(out, "|", max - strlen(out) - 1);
        strncat(out, lines[i], max - strlen(out) - 1);
    }
    return n;
}

int main(void)
{
    char b[1024];
    const char *doc =
        "-- set\n"             /* 0 */
        "  >bpm 124\n"         /* 1 */
        "  >kick 9...\n"       /* 2 */
        "\n"                   /* 3 */
        "  >play\n"            /* 4 */
        "-- drop\n"            /* 5 */
        "  >toggle kick\n"     /* 6 */
        "    >hat ..7.\n"      /* 7 */
        "  >lines 0 /16\n"     /* 8 */
        ">clear\n";            /* 9 */
    int n = block_of(doc, 0, b, sizeof b);
    CHECK(n == 3 && strcmp(b, "  >bpm 124|  >kick 9...|  >play") == 0,
          "a heading's block is the lines under it, blank lines inside it too: %s", b);
    n = block_of(doc, 5, b, sizeof b);
    CHECK(n == 3 && strcmp(b, "  >toggle kick|    >hat ..7.|  >lines 0 /16") == 0,
          "nested lines are part of the outer block: %s", b);
    n = block_of(doc, 6, b, sizeof b);
    CHECK(n == 1 && strcmp(b, "    >hat ..7.") == 0, "and a line has its own: %s", b);
    n = block_of(doc, 8, b, sizeof b);
    CHECK(n == 0, "a block ends at a line indented no deeper");
    n = block_of(doc, 9, b, sizeof b);
    CHECK(n == 0, "the last line has none");
    const char *tabs = "-- a\n\t>kick 9...\n  >hat ..7.\n-- b\n";
    n = block_of(tabs, 0, b, sizeof b);
    CHECK(n == 2, "a tab is two spaces, what the Tab key types");
    CHECK(block_indent("\t  x") == 4 && block_indent("x") == 0, "indents count");
    CHECK(block_command("   >toggle kick") != NULL && block_command("  -- drop") == NULL &&
          block_command("") == NULL, "a line runs if it starts '>' after its indent");
    const char *text = "   explained\n     >kick 9...\n";
    n = block_of(text, 0, b, sizeof b);
    CHECK(n == 1, "a line of words has a block too, and the block runs");

    /* RUN AGAIN, A BLOCK IS A SWITCH: out when every lane line is as it plays
     * and one of them is sounding, back when none is, as written otherwise. */
    const int all_on[] = { BLOCK_LANE_ON, BLOCK_LANE_OTHER, BLOCK_LANE_ON };
    const int half[]   = { BLOCK_LANE_OFF, BLOCK_LANE_ON };
    const int all_off[] = { BLOCK_LANE_OFF, BLOCK_LANE_OTHER, BLOCK_LANE_OFF };
    const int edited[] = { BLOCK_LANE_ON, BLOCK_LANE_CHANGE };
    const int no_lane[] = { BLOCK_LANE_OTHER, BLOCK_LANE_OTHER };
    CHECK(block_plan(all_on, 3, true) == BLOCK_OFF, "a scene run again, playing: out on the one");
    CHECK(block_plan(half, 2, true) == BLOCK_OFF, "partly playing: out, the part that plays");
    CHECK(block_plan(all_off, 3, true) == BLOCK_ON, "run once more: back");
    CHECK(block_plan(edited, 2, true) == BLOCK_RUN, "an edited line: the block runs as written");
    CHECK(block_plan(no_lane, 2, true) == BLOCK_RUN, "toggles and colours alone: they run");
    CHECK(block_plan(all_on, 3, false) == BLOCK_RUN, "stopped, a block always runs: set up twice");
    printf(fails ? "[FAIL] %d check(s) failed\n" : "[PASS] a line runs its block\n", fails);
    return fails != 0;
}
