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

static int s_err_col = -1;

int cmd_last_error_col(void) { return s_err_col; }

static int s_secret_col = -1;

int cmd_last_secret_col(void) { return s_secret_col; }

static cmd_asker_t s_asker;

void cmd_set_asker(cmd_asker_t fn) { s_asker = fn; }

bool cmd_ask_secret(const char *question, cmd_secret_fn fn)
{
    return s_asker != NULL && fn != NULL && s_asker(question, fn);
}

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

/* WHAT A LINE'S FIRST WORD IS: a command, a definition, or a lane.
 *
 * A command is an exact word in the table - no instance digits, no parts: the
 * rule that let '>disc2' find 'disc' also let '>bpm140' find 'bpm' and quietly
 * report the tempo it had not set, and it is gone with the digits. A definition
 * is any name followed by '='. A lane is a defined name or a picture, with its
 * address - 'disc:2:x' - which lane_name.h parses and builtins.c resolves.
 *
 * The first word ends at a space or at '=', so '>conga=note 63' is a definition
 * like '>conga = note 63'. */
static const char *first_word(const char *p, size_t *n)
{
    const char *start = p;
    while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '=') {
        p++;
    }
    *n = (size_t)(p - start);
    return start;
}

/* After the first word: is this a definition? */
static bool is_definition(const char *after)
{
    while (*after == ' ' || *after == '\t') {
        after++;
    }
    return *after == '=';
}

/* The pseudo-entries the recogniser returns for things that are not rows of the
 * table, so the editor can mark them the way it marks a command. */
static const cmd_t s_lane_entry   = { "lane", NULL, CMD_CAP_EDIT, "a lane" };
static const cmd_t s_define_entry = { "=",    NULL, CMD_CAP_EDIT, "a name" };

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
    size_t n = 0;
    const char *start = first_word(p, &n);
    if (n == 0) {
        return NULL;
    }
    const cmd_t *hit = NULL;
    for (int i = 0; i < s_count && hit == NULL; i++) {
        if (strlen(s_table[i].name) == n && strncmp(s_table[i].name, start, n) == 0) {
            hit = &s_table[i];
        }
    }
    if (hit == NULL && is_definition(start + n)) {
        hit = &s_define_entry;
    }
    if (hit == NULL && cmd_lane_known(start, n)) {
        hit = &s_lane_entry;
    }
    if (hit != NULL) {
        /* The MARK covers the whole address, so the editor marks '>disc:2:x'
         * and not just '>disc'. */
        if (word_at != NULL)  { *word_at = (int)(start - line); }
        if (word_len != NULL) { *word_len = (int)n; }
    }
    return hit;
}

/* AN OLD SPELLING GETS THE NEW ONE. Documents written before the address grammar
 * say 'disc2' and 'disc[x]', and the boards on the owner's desk carry them. An
 * unknown word that is one of those says what it is now, instead of "try: help". */
static void old_spelling(cmd_ctx_t *ctx, const char *w, size_t n)
{
    size_t b = n;
    const char *br = memchr(w, '[', n);
    if (br != NULL) {
        b = (size_t)(br - w);
    } else {
        while (b > 1 && w[b - 1] >= '0' && w[b - 1] <= '9') { b--; }
    }
    if (b < n && b > 0 && cmd_lane_known(w, b)) {
        if (br != NULL) {
            const char *close = memchr(br, ']', n - b);
            const int pl = close ? (int)(close - br - 1) : (int)(n - b - 1);
            cmd_out(ctx, "%.*s[%.*s] is %.*s:%.*s now", (int)b, w, pl, br + 1,
                    (int)b, w, pl, br + 1);
        } else {
            cmd_out(ctx, "%.*s is %.*s:%.*s now", (int)n, w, (int)b, w,
                    (int)(n - b), w + b);
        }
        return;
    }
    if (n >= 2 && w[0] == 'c' && w[1] == 'c') {
        cmd_out(ctx, "cc is a kind now: >fx = cc 74");
        return;
    }
    cmd_out(ctx, "%.*s? try: help", (int)n, w);
}

cmd_status_t cmd_run_line(const char *line, cmd_caller_t caller,
                          char *msg_out, size_t msg_max)
{
    cmd_ctx_t ctx = { .arg = "", .caller = caller, .err_at = -1,
                      .secret_at = -1 };
    ctx.msg[0] = '\0';
    s_out_lines = 0;
    s_err_col = -1;
    s_secret_col = -1;
    const char *const line0 = line;

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
    size_t namelen = 0;
    first_word(line, &namelen);
    const char *arg = line + namelen;
    while (*arg == ' ' || *arg == '\t') {
        arg++;
    }
    ctx.arg = arg;
    const uint32_t caps = caller_caps(caller);
    /* '=' DECIDES FIRST. A definition of a command's name has to reach
     * cmd_define() to be refused - found as a verb instead, '>bpm = note 3' ran
     * bpm with "= note 3", atoi made it 0, and the tempo fell to 20 without a
     * word. */
    const bool def = (*arg == '=');

    for (int i = 0; i < s_count && !def; i++) {
        if (strlen(s_table[i].name) == namelen &&
            strncmp(s_table[i].name, line, namelen) == 0) {
            if ((s_table[i].caps & ~caps) != 0) {
                cmd_out(&ctx, "%s: not permitted here", s_table[i].name);
                if (msg_out != NULL) { snprintf(msg_out, msg_max, "%s", ctx.msg); }
                return CMD_ERROR;
            }
            ctx.name = s_table[i].name;
            const cmd_status_t st = s_table[i].fn(&ctx);
            if (ctx.err_at >= 0) {
                /* ctx.arg may have been advanced past a first word, but it
                 * still points into this line, so the column is exact. */
                s_err_col = (int)(ctx.arg - line0) + ctx.err_at;
            }
            if (ctx.secret_at >= 0) {
                s_secret_col = (int)(ctx.arg - line0) + ctx.secret_at;
            }
            if (msg_out != NULL) {
                snprintf(msg_out, msg_max, "%s", ctx.msg);
            }
            return st;
        }
    }

    /* A DEFINITION OR A LANE. Both change what plays, so both need EDIT - the
     * same authority the lane verbs had when they were verbs. */
    cmd_status_t st = CMD_ERROR;
    if (def || cmd_lane_known(line, namelen)) {
        if ((CMD_CAP_EDIT & ~caps) != 0) {
            cmd_out(&ctx, "%.*s: not permitted here", (int)namelen, line);
        } else {
            st = def ? cmd_define(&ctx, line, namelen)
                     : cmd_lane(&ctx, line, namelen);
            if (ctx.err_at >= 0) {
                s_err_col = (int)(ctx.arg - line0) + ctx.err_at;
            }
        }
        if (msg_out != NULL) {
            snprintf(msg_out, msg_max, "%s", ctx.msg);
        }
        return st;
    }

    /* Unknown names must say so rather than failing silently - a guide file
     * with a typo in it is otherwise indistinguishable from one that worked. */
    old_spelling(&ctx, line, namelen);
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
