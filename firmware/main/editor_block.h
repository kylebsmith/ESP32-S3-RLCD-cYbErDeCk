/*
 * A LINE AND ITS BLOCK.
 *
 * Tab indents (two spaces), and the lines indented deeper than a line are its
 * block. Ctrl+Enter runs the line and then every line of its block that starts
 * with '>', in order - so a section's heading, run, is the whole section: a
 * scene, one key. A '>toggle' in it lands on the next bar's one
 * (seq_toggle.h), so a scene of toggles lands together. The owner, 2026-09-29:
 * "run code blocks maybe simply via tabs".
 *
 *   -- drop                 <- ctrl+enter here runs the two lines under it
 *     >toggle kick hat
 *     >lines 0 /16
 *   >fill ....9..9 !1       <- or here: just this line, as always
 *
 * A block runs to the first line that is not blank and not indented deeper;
 * blank lines inside it are part of it. A tab counts as two spaces, what the
 * Tab key types. Pure, so tools/test_block.c checks the rule the editor runs.
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>

#define BLOCK_MAX_LINES 32
#define BLOCK_LINE_MAX  128

/* The document a character at a time: the editor passes doc_at. */
typedef char (*block_char_fn)(size_t i);

static inline int block_indent(const char *line)
{
    int n = 0;
    for (; *line == ' ' || *line == '\t'; line++) {
        n += (*line == '\t') ? 2 : 1;
    }
    return n;
}

static inline bool block_blank(const char *line)
{
    for (; *line != '\0'; line++) {
        if (*line != ' ' && *line != '\t') {
            return false;
        }
    }
    return true;
}

/* The line starting at `s` into `out` (cut at `max`); returns where the next
 * line starts, or `len`. */
static inline size_t block_line(block_char_fn at, size_t len, size_t s,
                                char *out, size_t max)
{
    size_t n = 0, i = s;
    for (; i < len; i++) {
        const char ch = at(i);
        if (ch == '\n') {
            i++;
            break;
        }
        if (n + 1 < max) {
            out[n++] = ch;
        }
    }
    out[n] = '\0';
    return i;
}

/* The block under the line that starts at `head`: up to `cap` lines copied into
 * `lines`, blank ones left out. Returns how many. */
static inline int block_collect(block_char_fn at, size_t len, size_t head,
                                char (*lines)[BLOCK_LINE_MAX], int cap)
{
    char buf[BLOCK_LINE_MAX];
    size_t next = block_line(at, len, head, buf, sizeof buf);
    const int depth = block_indent(buf);
    int n = 0;
    while (next < len && n < cap) {
        const size_t after = block_line(at, len, next, buf, sizeof buf);
        if (!block_blank(buf)) {
            if (block_indent(buf) <= depth) {
                break;
            }
            for (size_t k = 0; k < BLOCK_LINE_MAX; k++) {
                lines[n][k] = buf[k];
                if (buf[k] == '\0') {
                    break;
                }
            }
            n++;
        }
        next = after;
    }
    return n;
}

/* Does this line run? A '>' after its indent. */
static inline const char *block_command(const char *line)
{
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    return (*line == '>') ? line : NULL;
}
