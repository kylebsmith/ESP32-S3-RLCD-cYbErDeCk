#include "cmd.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "docstore.h"
#include "seq.h"
#include "esp_log.h"

static const char *TAG = "cmd";

/* What each caller is permitted to reach. The owner's hands are unrestricted;
 * a guide line is the owner one step removed and may not change pairing or
 * power state without them meaning to; an agent may read, edit and store but
 * may not touch the radio or the device's own configuration. Tightening this
 * later is easy, and loosening it is a decision somebody has to make on
 * purpose - which is the point. */
static uint32_t caller_caps(cmd_caller_t who)
{
    switch (who) {
    case CMD_BY_HANDS: return 0xFFFFFFFFu;
    case CMD_BY_GUIDE: return CMD_CAP_READ | CMD_CAP_EDIT | CMD_CAP_STORE |
                              CMD_CAP_NET;
    case CMD_BY_AGENT: return CMD_CAP_READ | CMD_CAP_EDIT | CMD_CAP_STORE;
    default:           return CMD_CAP_READ;
    }
}

static int s_out_lines;

int cmd_last_output_lines(void) { return s_out_lines; }

static const cmd_t *s_table;
static int          s_count;

void cmd_register(const cmd_t *table, int count)
{
    s_table = table;
    s_count = count;
}

const cmd_t *cmd_table(int *count)
{
    if (count != NULL) {
        *count = s_count;
    }
    return s_table;
}

void cmd_out(cmd_ctx_t *ctx, const char *fmt, ...)
{
    char line[160];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);

    /* One sink, one call site - and it is a BUFFER, which is the whole point.
     * Command output that went to a log or a scrollback would be the one
     * thing on this device that is not text in the substrate: not editable,
     * not searchable, not pipeable, not undoable. docs/SUBSTRATE.md claims
     * there is one data structure; output has to be in it or the claim is
     * false. */
    ESP_LOGI(TAG, "%s", line);
    /* A RULE BETWEEN COMMANDS.
     *
     * Output accumulates in one buffer, so several runs ran together into an
     * unreadable wall - the owner described these pages as jumbled. One thin
     * line per command is enough to see where one answer ends and the next
     * begins, and it costs one row. */
    if (s_out_lines == 0) {
        doc_buf_append(doc_buf_ensure("+out"), "------------------------------");
    }
    doc_buf_append(doc_buf_ensure("+out"), line);
    s_out_lines++;
    if (ctx != NULL && ctx->msg[0] == '\0') {
        snprintf(ctx->msg, sizeof ctx->msg, "%.*s",
                 (int)(sizeof ctx->msg - 1), line);
    }
}

/* A TRAILING DIGIT MAKES ANOTHER ONE.
 *
 * '>disc2 x...' is a second circle. '>kick2' is a second kick. The name is the
 * lane, so a name that differs is a lane that differs - and the only thing the
 * command table needs to know is which BINDING it is, which is the name with the
 * digit taken off.
 *
 * Done here rather than with a table row per instance because the alternative is
 * 'disc1' through 'disc9' for thirteen primitives and seventeen sounds: a hundred
 * and thirty rows to say something a single rule says. docs/MAP.md refuses names
 * that delete nothing, and this one buys instances of everything for free.
 *
 * It is a suffix and not a separate argument so that the LANE keeps its own name.
 * '>route grow disc2' has to be able to say which circle, and 'disc 2' could not.
 *
 * Returns the length of the base name, which is the whole name when there is no
 * digit, and never strips a name that is all digits. */
static size_t base_len(const char *name, size_t n)
{
    size_t b = n;
    /* A TRAILING '[part]' SELECTS A PARAMETER. '>disc[x] 0..9..' is a lane whose
     * events are the circle's position, and the command table only needs to know
     * that it is a circle. The bracket is the referential mark everywhere else in
     * this language, which is why it is the one here too.
     *
     * Stripped before the digit, so 'disc2[x]' is the x of the second circle. */
    if (b > 2 && name[b - 1] == ']') {
        size_t k = b - 1;
        while (k > 0 && name[k] != '[') { k--; }
        if (k > 0) { b = k; }
    }
    while (b > 1 && name[b - 1] >= '0' && name[b - 1] <= '9') {
        b--;
    }
    return b;
}

const cmd_t *cmd_recognise(const char *line, int *word_at, int *word_len)
{
    if (line == NULL) {
        return NULL;
    }
    const char *p = line;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p != '>') {
        return NULL;
    }
    p++;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    const char *start = p;
    while (*p != '\0' && *p != ' ' && *p != '\t') {
        p++;
    }
    const size_t n = (size_t)(p - start);
    if (n == 0) {
        return NULL;
    }
    const size_t b = base_len(start, n);
    for (int pass = 0; pass < 2; pass++) {
        const size_t want = (pass == 0) ? n : b;
        if (pass == 1 && b == n) { break; }
        for (int i = 0; i < s_count; i++) {
            if (strlen(s_table[i].name) == want &&
                strncmp(s_table[i].name, start, want) == 0) {
                /* The MARK covers the whole name, digit included, so the editor
                 * underlines '>disc2' and not just '>disc'. */
                if (word_at != NULL)  { *word_at = (int)(start - line); }
                if (word_len != NULL) { *word_len = (int)n; }
                return &s_table[i];
            }
        }
    }
    return NULL;
}

cmd_status_t cmd_run_line(const char *line, cmd_caller_t caller,
                          char *msg_out, size_t msg_max)
{
    cmd_ctx_t ctx = { .arg = "", .caller = caller };
    ctx.msg[0] = '\0';
    s_out_lines = 0;

    /* The sigil. A command line is marked, so a document can hold prose and
     * runnable lines side by side without either pretending to be the other -
     * which is what makes documentation executable rather than merely
     * illustrative. docs/SUBSTRATE.md already assigns '>' to commands. */
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    if (*line != '>') {
        return CMD_DONE;            /* prose: not addressed to the machine */
    }
    line++;

    if (line == NULL) {
        return CMD_ERROR;
    }
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    if (*line == '\0' || *line == '#') {
        return CMD_DONE;            /* blank, or a comment in a guide file */
    }

    /* First word is the name; the remainder is one unparsed argument. */
    const char *sp = line;
    while (*sp != '\0' && *sp != ' ' && *sp != '\t') {
        sp++;
    }
    const size_t namelen = (size_t)(sp - line);
    const char *arg = sp;
    while (*arg == ' ' || *arg == '\t') {
        arg++;
    }
    ctx.arg = arg;

    const size_t baselen = base_len(line, namelen);
    for (int pass = 0; pass < 2; pass++) {
      const size_t want = (pass == 0) ? namelen : baselen;
      if (pass == 1 && baselen == namelen) { break; }
      for (int i = 0; i < s_count; i++) {
        if (strlen(s_table[i].name) == want &&
            strncmp(s_table[i].name, line, want) == 0) {

            if ((s_table[i].caps & ~caller_caps(caller)) != 0) {
                cmd_out(&ctx, "%s: not permitted here", s_table[i].name);
                if (msg_out != NULL) { snprintf(msg_out, msg_max, "%s", ctx.msg); }
                return CMD_ERROR;
            }
            /* THE LANE'S NAME IS WHAT WAS TYPED, digit and all - that is the
             * whole point of the instance. The table entry only decided which
             * binding to use. */
            static char typed[SEQ_NAME_MAX];
            snprintf(typed, sizeof typed, "%.*s", (int)namelen, line);
            ctx.name = typed;
            const cmd_status_t st = s_table[i].fn(&ctx);
            if (msg_out != NULL) {
                snprintf(msg_out, msg_max, "%s", ctx.msg);
            }
            return st;
        }
      }
    }

    /* Unknown names must say so rather than failing silently - a guide file
     * with a typo in it is otherwise indistinguishable from one that worked. */
    cmd_out(&ctx, "%.*s? try: help", (int)namelen, line);
    if (msg_out != NULL) {
        snprintf(msg_out, msg_max, "%s", ctx.msg);
    }
    return CMD_ERROR;
}

static cmd_announce_t s_announce;

void cmd_set_announce(cmd_announce_t fn) { s_announce = fn; }

void cmd_announce(const char *line)
{
    if (s_announce != NULL) {
        s_announce(line);
    }
}
