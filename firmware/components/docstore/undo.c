/*
 * Undo, as an operation log.
 *
 * A snapshot ring is the obvious design and the wrong one here: the document
 * is up to 128 KB and a useful ring would cost megabytes. An operation log
 * costs a few bytes per edit, and it is what makes undo affordable on a part
 * with 512 KB of internal SRAM.
 *
 * Consecutive typing coalesces into one entry, so undo moves by words rather
 * than letter by letter - which is what a person means by "undo". The run is
 * broken by moving the cursor, by a newline, or by switching between
 * inserting and deleting, because those are the points where the intent
 * changed.
 *
 * Redo is the same log walked the other way, and is discarded as soon as a
 * new edit is made, which is the behaviour every editor has taught people to
 * expect.
 */
#include "docstore.h"

#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "undo";

#define UNDO_STEPS     128
#define UNDO_RUN_MAX    48      /* characters coalesced into one step */

enum { OP_NONE = 0, OP_INSERT, OP_DELETE };

typedef struct {
    uint8_t  type;
    uint8_t  closed;            /* no more characters join this run */
    uint8_t  buf;               /* which buffer this edit belongs to */
    uint32_t pos;               /* document offset the run starts at */
    uint16_t len;
    char     text[UNDO_RUN_MAX];
} undo_op_t;

static undo_op_t *s_ops;
static int s_count;             /* entries currently in the log      */
static int s_cursor;            /* how many of them are "done"       */
static bool s_applying;         /* suppress recording while undoing  */

static void ensure(void)
{
    if (s_ops == NULL) {
        s_ops = heap_caps_calloc(UNDO_STEPS, sizeof(undo_op_t), MALLOC_CAP_SPIRAM);
        if (s_ops == NULL) {
            s_ops = calloc(UNDO_STEPS, sizeof(undo_op_t));
        }
        if (s_ops == NULL) {
            ESP_LOGE(TAG, "no memory for the undo log - undo is disabled");
        }
    }
}

void doc_undo_reset(void)
{
    ensure();
    s_count = 0;
    s_cursor = 0;
}

int doc_undo_depth(void) { return s_cursor; }

/* Drop everything after the cursor: a new edit invalidates the redo tail. */
static void drop_redo(void)
{
    s_count = s_cursor;
}

/* The log is global but an edit belongs to ONE buffer. Without this, undo in
 * a document could apply an operation recorded in another one - the offsets
 * are meaningless there, so it corrupted text at whatever position happened
 * to match. Silent, and in the redo direction it wrote into the wrong file. */
static undo_op_t *last_op(void)
{
    if (s_cursor == 0) {
        return NULL;
    }
    undo_op_t *op = &s_ops[s_cursor - 1];
    return op->buf == (uint8_t)doc_buf_current() ? op : NULL;
}

static undo_op_t *push(void)
{
    if (s_cursor >= UNDO_STEPS) {
        /* Oldest step falls off the front. */
        memmove(&s_ops[0], &s_ops[1], (UNDO_STEPS - 1) * sizeof(undo_op_t));
        s_cursor = UNDO_STEPS - 1;
    }
    undo_op_t *op = &s_ops[s_cursor++];
    memset(op, 0, sizeof *op);
    op->buf = (uint8_t)doc_buf_current();
    s_count = s_cursor;
    return op;
}

/* Called by the document after an insertion of one character at `pos`. */
void undo_record_insert(size_t pos, char c)
{
    ensure();
    if (s_ops == NULL || s_applying) {
        return;
    }
    drop_redo();

    undo_op_t *op = last_op();
    const bool continues = op != NULL &&
                           op->type == OP_INSERT &&
                           !op->closed &&
                           op->len < UNDO_RUN_MAX &&
                           op->pos + op->len == pos;
    if (!continues) {
        op = push();
        op->type = OP_INSERT;
        op->pos  = (uint32_t)pos;
        op->len  = 0;
    }
    op->text[op->len++] = c;
    /* Whitespace ends the run, so undo steps back a word at a time. A run
     * that swallowed a whole sentence would make undo useless for the thing
     * it is mostly wanted for: taking back the last word. */
    if (c == ' ' || c == '\n' || c == '\t' || op->len >= UNDO_RUN_MAX) {
        op->closed = 1;
    }
}

/* Called after deleting the character that was at `pos`. */
void undo_record_delete(size_t pos, char c)
{
    ensure();
    if (s_ops == NULL || s_applying) {
        return;
    }
    drop_redo();

    undo_op_t *op = last_op();
    /* Backspace walks leftwards, so a run grows at its front. */
    const bool continues = op != NULL &&
                           op->type == OP_DELETE &&
                           !op->closed &&
                           op->len < UNDO_RUN_MAX &&
                           op->pos == pos + 1;
    if (!continues) {
        op = push();
        op->type = OP_DELETE;
        op->pos  = (uint32_t)pos;
        op->len  = 0;
    }
    memmove(&op->text[1], &op->text[0], op->len);
    op->text[0] = c;
    op->len++;
    op->pos = (uint32_t)pos;
    if (op->len >= UNDO_RUN_MAX) {
        op->closed = 1;
    }
}

static void apply_inverse(const undo_op_t *op)
{
    s_applying = true;
    if (op->type == OP_INSERT) {
        doc_move_to(op->pos + op->len);
        for (int i = 0; i < op->len; i++) {
            doc_backspace();
        }
    } else if (op->type == OP_DELETE) {
        doc_move_to(op->pos);
        for (int i = 0; i < op->len; i++) {
            doc_insert(op->text[i]);
        }
    }
    s_applying = false;
}

static void apply_forward(const undo_op_t *op)
{
    s_applying = true;
    if (op->type == OP_INSERT) {
        doc_move_to(op->pos);
        for (int i = 0; i < op->len; i++) {
            doc_insert(op->text[i]);
        }
    } else if (op->type == OP_DELETE) {
        doc_move_to(op->pos + op->len);
        for (int i = 0; i < op->len; i++) {
            doc_backspace();
        }
    }
    s_applying = false;
}

bool doc_undo(void)
{
    ensure();
    if (s_ops == NULL || s_cursor == 0) {
        return false;
    }
    if (s_ops[s_cursor - 1].buf != (uint8_t)doc_buf_current()) {
        ESP_LOGW(TAG, "nothing to undo in this buffer");
        return false;
    }
    const undo_op_t *op = &s_ops[--s_cursor];
    apply_inverse(op);
    ESP_LOGI(TAG, "undo: %s %u char(s) at %u -> doc %u, depth %d",
             op->type == OP_INSERT ? "removed" : "restored",
             op->len, (unsigned)op->pos, (unsigned)doc_len(), s_cursor);
    return true;
}

bool doc_redo(void)
{
    ensure();
    if (s_ops == NULL || s_cursor >= s_count) {
        return false;
    }
    if (s_ops[s_cursor].buf != (uint8_t)doc_buf_current()) {
        ESP_LOGW(TAG, "nothing to redo in this buffer");
        return false;
    }
    const undo_op_t *op = &s_ops[s_cursor++];
    apply_forward(op);
    ESP_LOGI(TAG, "redo: %s %u char(s) at %u -> doc %u, depth %d",
             op->type == OP_INSERT ? "restored" : "removed",
             op->len, (unsigned)op->pos, (unsigned)doc_len(), s_cursor);
    return true;
}
