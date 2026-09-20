#include "cmd.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

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

    /* One sink, one call site. Today it is the log plus the context's short
     * message; when an output buffer exists this is the only function that
     * has to learn about it. */
    ESP_LOGI(TAG, "%s", line);
    if (ctx != NULL && ctx->msg[0] == '\0') {
        snprintf(ctx->msg, sizeof ctx->msg, "%.*s",
                 (int)(sizeof ctx->msg - 1), line);
    }
}

cmd_status_t cmd_run_line(const char *line, cmd_caller_t caller,
                          char *msg_out, size_t msg_max)
{
    cmd_ctx_t ctx = { .arg = "", .caller = caller };
    ctx.msg[0] = '\0';

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

    for (int i = 0; i < s_count; i++) {
        if (strlen(s_table[i].name) == namelen &&
            strncmp(s_table[i].name, line, namelen) == 0) {

            if ((s_table[i].caps & ~caller_caps(caller)) != 0) {
                cmd_out(&ctx, "%s: not permitted here", s_table[i].name);
                if (msg_out != NULL) { snprintf(msg_out, msg_max, "%s", ctx.msg); }
                return CMD_ERROR;
            }
            const cmd_status_t st = s_table[i].fn(&ctx);
            if (msg_out != NULL) {
                snprintf(msg_out, msg_max, "%s", ctx.msg);
            }
            return st;
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
