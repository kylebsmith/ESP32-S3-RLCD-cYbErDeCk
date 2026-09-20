/*
 * The built-in commands.
 *
 * Every one of these is reachable by name, which is the discipline
 * docs/SUBSTRATE.md imposes and the reason the palette, the guide file and a
 * future agent all work without new plumbing.
 */
#include "cmd.h"

#include <stdio.h>
#include <string.h>

#include "docstore.h"

static cmd_status_t c_help(cmd_ctx_t *ctx)
{
    int n = 0;
    const cmd_t *t = cmd_table(&n);
    for (int i = 0; i < n; i++) {
        cmd_out(ctx, "%-8s %s", t[i].name, t[i].help);
    }
    snprintf(ctx->msg, sizeof ctx->msg, "%d commands", n);
    return CMD_DONE;
}

static cmd_status_t c_list(cmd_ctx_t *ctx)
{
    int shown = 0;
    for (int i = 0; i < DOC_MAX_BUFFERS; i++) {
        const char *nm = doc_buf_name(i);
        if (nm[0] == '\0' && doc_buf_len(i) == 0 && i != doc_buf_current()) {
            continue;
        }
        cmd_out(ctx, "%c%d %-16s %5u%s",
                i == doc_buf_current() ? '*' : ' ', i,
                nm[0] ? nm : "(scratch)",
                (unsigned)doc_buf_len(i),
                doc_buf_is_dirty(i) ? " *" : "");
        shown++;
    }
    snprintf(ctx->msg, sizeof ctx->msg, "%d buffer%s", shown, shown == 1 ? "" : "s");
    return CMD_DONE;
}

static cmd_status_t c_new(cmd_ctx_t *ctx)
{
    if (doc_buf_new() != ESP_OK) {
        cmd_out(ctx, "no free buffer");
        return CMD_ERROR;
    }
    snprintf(ctx->msg, sizeof ctx->msg, "new scratch buffer %d", doc_buf_current());
    return CMD_DONE;
}

/* Promote the current scratch buffer into the archive. This is the whole of
 * "save": the text was already durable, naming it is what files it. */
static cmd_status_t c_name(cmd_ctx_t *ctx)
{
    if (ctx->arg[0] == '\0') {
        cmd_out(ctx, "name what? e.g. name lullaby");
        return CMD_ERROR;
    }
    if (doc_buf_rename(ctx->arg) != ESP_OK) {
        cmd_out(ctx, "cannot use that name");
        return CMD_ERROR;
    }
    doc_save();
    snprintf(ctx->msg, sizeof ctx->msg, "filed as %s", ctx->arg);
    return CMD_DONE;
}

static cmd_status_t c_open(cmd_ctx_t *ctx)
{
    /* >list already prints an index against every buffer, and an unnamed
     * scratch buffer has no name to match - so the index has to work, or the
     * listing is showing something that cannot be acted on. */
    if (ctx->arg[0] >= '0' && ctx->arg[0] <= '9') {
        const int n = ctx->arg[0] - '0';
        if (doc_buf_select(n) == ESP_OK) {
            const char *nm = doc_buf_name(n);
            snprintf(ctx->msg, sizeof ctx->msg, "%s", nm[0] ? nm : "scratch");
            return CMD_DONE;
        }
        cmd_out(ctx, "no buffer %d", n);
        return CMD_ERROR;
    }
    for (int i = 0; i < DOC_MAX_BUFFERS; i++) {
        if (strcmp(doc_buf_name(i), ctx->arg) == 0 && ctx->arg[0] != '\0') {
            if (doc_buf_select(i) != ESP_OK) {
                cmd_out(ctx, "cannot open %s", ctx->arg);
                return CMD_ERROR;
            }
            snprintf(ctx->msg, sizeof ctx->msg, "%s", ctx->arg);
            return CMD_DONE;
        }
    }
    cmd_out(ctx, "no document called '%s'", ctx->arg);
    return CMD_ERROR;
}

static cmd_status_t c_save(cmd_ctx_t *ctx)
{
    const esp_err_t e = doc_save();
    if (e != ESP_OK) {
        cmd_out(ctx, "save failed");
        return CMD_ERROR;
    }
    doc_mirror_sd();
    snprintf(ctx->msg, sizeof ctx->msg, "saved %u bytes", (unsigned)doc_len());
    return CMD_DONE;
}

static cmd_status_t c_close(cmd_ctx_t *ctx)
{
    if (doc_buf_close(doc_buf_current()) != ESP_OK) {
        cmd_out(ctx, "cannot close the last buffer");
        return CMD_ERROR;
    }
    snprintf(ctx->msg, sizeof ctx->msg, "closed");
    return CMD_DONE;
}

/* The kind no longer decides whether Enter runs a line - the '>' sigil marks
 * a command and Ctrl+Enter runs it, in any buffer. The kind is kept because
 * it still has to decide prose-versus-grid reflow for Orca patches, which is
 * the distinction docs/SUBSTRATE.md actually cares about. */
static cmd_status_t c_guide(cmd_ctx_t *ctx)
{
    doc_buf_set_kind(DOC_KIND_GUIDE);
    snprintf(ctx->msg, sizeof ctx->msg, "kind: guide");
    return CMD_DONE;
}

static cmd_status_t c_prose(cmd_ctx_t *ctx)
{
    doc_buf_set_kind(DOC_KIND_PROSE);
    snprintf(ctx->msg, sizeof ctx->msg, "kind: prose");
    return CMD_DONE;
}

static cmd_status_t c_out(cmd_ctx_t *ctx)
{
    const int i = doc_buf_ensure("+out");
    if (i < 0 || doc_buf_select(i) != ESP_OK) {
        cmd_out(ctx, "no output buffer");
        return CMD_ERROR;
    }
    snprintf(ctx->msg, sizeof ctx->msg, "output");
    return CMD_DONE;
}

esp_err_t editor_set_density(int dense);

/* Both faces are compiled in, so this costs no flash and no assets. Dense is
 * also CHEAPER to draw - a 6x12 cell is 9 bytes on the wire against 36 for a
 * 12x24 one - so the readable default is the expensive one, which is the
 * right way round. */
static cmd_status_t c_density(cmd_ctx_t *ctx)
{
    /* Strict, not defaulting. A command that silently does the opposite of
     * what a typo asked for is worse than one that refuses. */
    int dense;
    if (ctx->arg[0] == 'd' || ctx->arg[0] == '6') {
        dense = 1;
    } else if (ctx->arg[0] == 'c' || ctx->arg[0] == '1') {
        dense = 0;
    } else {
        cmd_out(ctx, "density chunky | density dense");
        return CMD_ERROR;
    }
    if (editor_set_density(dense) != ESP_OK) {
        cmd_out(ctx, "density: layout refused");
        return CMD_ERROR;
    }
    snprintf(ctx->msg, sizeof ctx->msg, dense ? "dense 60x20" : "chunky 30x11");
    return CMD_DONE;
}

static const cmd_t s_builtins[] = {
    { "help",  c_help,  CMD_CAP_READ,                   "list the commands" },
    { "list",  c_list,  CMD_CAP_READ,                   "list open buffers" },
    { "new",   c_new,   CMD_CAP_EDIT,                   "a fresh scratch buffer" },
    { "name",  c_name,  CMD_CAP_EDIT | CMD_CAP_STORE,   "file this buffer under a name" },
    { "open",  c_open,  CMD_CAP_READ,                   "switch to a named document" },
    { "save",  c_save,  CMD_CAP_STORE,                  "write this buffer now" },
    { "close", c_close, CMD_CAP_EDIT,                   "forget this buffer" },
    { "guide", c_guide, CMD_CAP_EDIT,                   "mark as a guide" },
    { "prose", c_prose, CMD_CAP_EDIT,                   "mark as prose" },
    { "out",   c_out,   CMD_CAP_READ,                   "read command output" },
    { "density", c_density, CMD_CAP_SYSTEM,             "chunky | dense" },
};

void cmd_register(const cmd_t *table, int count);

void cmd_init(void)
{
    cmd_register(s_builtins, (int)(sizeof s_builtins / sizeof s_builtins[0]));
}
