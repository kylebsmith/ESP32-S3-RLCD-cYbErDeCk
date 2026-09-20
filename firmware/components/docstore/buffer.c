/*
 * Buffers: the gap buffer, several of them, with the current one selected.
 *
 * This replaces the single global document. The gap buffer itself is
 * unchanged in behaviour - it is the same algorithm that has been running and
 * surviving power cuts - it simply lives in a struct now so there can be more
 * than one of it.
 *
 * Storage is allocated LAZILY. A buffer restored from the journal at boot
 * knows its name and its record offset but holds no text until it is
 * selected, so eight buffers cost eight small structs rather than a megabyte
 * of PSRAM that nobody asked for.
 */
#include "docstore.h"

#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "buffer";

typedef struct {
    char    *buf;              /* NULL until loaded                        */
    size_t   cap, gs, ge;      /* gap start is also the cursor             */
    char     name[DOC_NAME_MAX];
    uint8_t  kind;
    bool     dirty;
    bool     used;
    /* Where the newest journal record for this buffer sits, so the text can
     * be fetched on demand rather than at boot. */
    bool     have_rec;
    size_t   rec_off;
    size_t   rec_len;          /* length on flash, for an unloaded buffer */
    uint32_t rec_seq;
} buf_t;

static buf_t s_bufs[DOC_MAX_BUFFERS];
static int   s_cur;

/* journal.c */
esp_err_t journal_load_into(size_t rec_off, char *out, size_t max, size_t *len);

static buf_t *cur(void) { return &s_bufs[s_cur]; }

static esp_err_t ensure_storage(buf_t *b)
{
    if (b->buf != NULL) {
        return ESP_OK;
    }
    b->buf = heap_caps_malloc(DOC_CAPACITY, MALLOC_CAP_SPIRAM);
    if (b->buf == NULL) {
        b->buf = heap_caps_malloc(DOC_CAPACITY, MALLOC_CAP_DEFAULT);
    }
    if (b->buf == NULL) {
        ESP_LOGE(TAG, "cannot allocate %d B for a buffer", DOC_CAPACITY);
        return ESP_ERR_NO_MEM;
    }
    b->cap = DOC_CAPACITY;
    b->gs  = 0;
    b->ge  = b->cap;

    /* Fetch the text now that somebody actually wants it. */
    if (b->have_rec) {
        char *tmp = malloc(DOC_CAPACITY);
        if (tmp != NULL) {
            size_t n = 0;
            if (journal_load_into(b->rec_off, tmp, DOC_CAPACITY, &n) == ESP_OK) {
                if (n > b->cap) {
                    n = b->cap;
                }
                memcpy(b->buf, tmp, n);
                b->gs = n;
                ESP_LOGI(TAG, "loaded '%s' (%u bytes) on demand",
                         b->name[0] ? b->name : "(scratch)", (unsigned)n);
            }
            free(tmp);
        }
        b->have_rec = false;
    }
    return ESP_OK;
}

/* ---- buffer table --------------------------------------------------------- */

int doc_buf_count(void)
{
    int n = 0;
    for (int i = 0; i < DOC_MAX_BUFFERS; i++) {
        if (s_bufs[i].used) {
            n++;
        }
    }
    return n;
}

int doc_buf_current(void) { return s_cur; }

const char *doc_buf_name(int i)
{
    return (i >= 0 && i < DOC_MAX_BUFFERS && s_bufs[i].used) ? s_bufs[i].name : "";
}

size_t doc_buf_len(int i)
{
    if (i < 0 || i >= DOC_MAX_BUFFERS || !s_bufs[i].used) {
        return 0;
    }
    const buf_t *b = &s_bufs[i];
    /* An unloaded buffer must report the length it has ON FLASH. Reporting
     * zero because the text has not been paged in yet makes a filed document
     * look empty, which is indistinguishable from having lost it. */
    return b->buf != NULL ? b->cap - (b->ge - b->gs) : b->rec_len;
}

bool doc_buf_is_dirty(int i)
{
    return i >= 0 && i < DOC_MAX_BUFFERS && s_bufs[i].used && s_bufs[i].dirty;
}

esp_err_t doc_buf_select(int i)
{
    if (i < 0 || i >= DOC_MAX_BUFFERS || !s_bufs[i].used) {
        return ESP_ERR_INVALID_ARG;
    }
    const esp_err_t err = ensure_storage(&s_bufs[i]);
    if (err != ESP_OK) {
        return err;
    }
    s_cur = i;
    ESP_LOGI(TAG, "buffer %d '%s' selected", i,
             s_bufs[i].name[0] ? s_bufs[i].name : "(scratch)");
    return ESP_OK;
}

/* Claim a slot without loading anything: used by the journal at boot. */
doc_kind_t doc_buf_kind(int i)
{
    return (i >= 0 && i < DOC_MAX_BUFFERS && s_bufs[i].used)
           ? (doc_kind_t)s_bufs[i].kind : DOC_KIND_PROSE;
}

void doc_buf_set_kind(doc_kind_t kind)
{
    cur()->kind = (uint8_t)kind;
    cur()->dirty = true;
}

uint8_t buffer_current_kind(void) { return cur()->kind; }

int buffer_claim(const char *name, size_t rec_off, size_t rec_len,
                 uint32_t seq, uint8_t kind)
{
    for (int i = 0; i < DOC_MAX_BUFFERS; i++) {
        if (!s_bufs[i].used) {
            buf_t *b = &s_bufs[i];
            memset(b, 0, sizeof *b);
            b->used = true;
            snprintf(b->name, sizeof b->name, "%s", name != NULL ? name : "");
            b->have_rec = rec_off != (size_t)-1;
            b->rec_off  = rec_off;
            b->rec_len  = rec_len;
            b->rec_seq  = seq;
            b->kind     = kind;
            return i;
        }
    }
    return -1;
}

esp_err_t doc_buf_new(void)
{
    const int i = buffer_claim("", (size_t)-1, 0, 0, DOC_KIND_PROSE);
    if (i < 0) {
        return ESP_ERR_NO_MEM;
    }
    return doc_buf_select(i);
}

esp_err_t doc_buf_rename(const char *name)
{
    if (name == NULL || name[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    /* A name is unique: naming a buffer the same as an existing one would
     * make the journal's newest-per-name rule silently merge two documents. */
    for (int i = 0; i < DOC_MAX_BUFFERS; i++) {
        if (s_bufs[i].used && i != s_cur &&
            strncmp(s_bufs[i].name, name, DOC_NAME_MAX) == 0) {
            ESP_LOGW(TAG, "'%s' is already taken", name);
            return ESP_ERR_INVALID_STATE;
        }
    }
    snprintf(cur()->name, DOC_NAME_MAX, "%s", name);
    cur()->dirty = true;                 /* force a save under the new name */
    ESP_LOGI(TAG, "buffer %d is now '%s'", s_cur, cur()->name);
    return ESP_OK;
}

esp_err_t doc_buf_close(int i)
{
    if (i < 0 || i >= DOC_MAX_BUFFERS || !s_bufs[i].used) {
        return ESP_ERR_INVALID_ARG;
    }
    if (doc_buf_count() <= 1) {
        return ESP_ERR_INVALID_STATE;    /* there is always one buffer */
    }
    free(s_bufs[i].buf);
    memset(&s_bufs[i], 0, sizeof s_bufs[i]);
    if (s_cur == i) {
        for (int j = 0; j < DOC_MAX_BUFFERS; j++) {
            if (s_bufs[j].used) { doc_buf_select(j); break; }
        }
    }
    return ESP_OK;
}

int doc_buf_find(const char *name)
{
    for (int i = 0; i < DOC_MAX_BUFFERS; i++) {
        if (s_bufs[i].used && strncmp(s_bufs[i].name, name, DOC_NAME_MAX) == 0) {
            return i;
        }
    }
    return -1;
}

int doc_buf_ensure(const char *name)
{
    const int found = doc_buf_find(name);
    if (found >= 0) {
        return found;
    }
    const int i = buffer_claim(name, (size_t)-1, 0, 0, DOC_KIND_PROSE);
    if (i >= 0) {
        ensure_storage(&s_bufs[i]);
    }
    return i;
}

void doc_buf_append(int i, const char *text)
{
    if (i < 0 || i >= DOC_MAX_BUFFERS || !s_bufs[i].used || text == NULL) {
        return;
    }
    buf_t *b = &s_bufs[i];
    if (ensure_storage(b) != ESP_OK) {
        return;
    }
    /* Append at the very end, wherever the gap happens to be. */
    const size_t len = b->cap - (b->ge - b->gs);
    const size_t save_gs = b->gs;
    while (b->gs < len) { b->buf[b->gs++] = b->buf[b->ge++]; }
    for (const char *p = text; *p != '\0' && b->gs < b->ge; p++) {
        b->buf[b->gs++] = *p;
    }
    if (b->gs < b->ge) {
        b->buf[b->gs++] = '\n';
    }
    /* Leave the cursor where the owner had it if this is their buffer. */
    if (i != s_cur) {
        while (b->gs > save_gs) { b->buf[--b->ge] = b->buf[--b->gs]; }
    }
    b->dirty = true;
}

bool buffer_is_transient(const char *name)
{
    return name != NULL && name[0] == '+';
}

const char *buffer_current_name(void) { return cur()->name; }
void        buffer_mark_clean(void)   { cur()->dirty = false; }

/* ---- the gap buffer, now per-buffer --------------------------------------- */

esp_err_t gapbuf_init(void)
{
    memset(s_bufs, 0, sizeof s_bufs);
    s_cur = 0;
    s_bufs[0].used = true;
    s_bufs[0].name[0] = '\0';            /* the scratch buffer */
    return ensure_storage(&s_bufs[0]);
}

size_t doc_len(void)    { const buf_t *b = cur(); return b->cap - (b->ge - b->gs); }
size_t doc_cursor(void) { return cur()->gs; }
bool   doc_dirty(void)  { return cur()->dirty; }

void gapbuf_mark_clean(void) { cur()->dirty = false; }

char doc_at(size_t i)
{
    const buf_t *b = cur();
    if (i >= doc_len()) {
        return '\0';
    }
    return i < b->gs ? b->buf[i] : b->buf[i + (b->ge - b->gs)];
}

void undo_record_insert(size_t pos, char c);
void undo_record_delete(size_t pos, char c);

void doc_insert(char c)
{
    buf_t *b = cur();
    if (b->gs == b->ge) {
        return;                          /* full - drop rather than overwrite */
    }
    undo_record_insert(b->gs, c);
    b->buf[b->gs++] = c;
    b->dirty = true;
}

void doc_backspace(void)
{
    buf_t *b = cur();
    if (b->gs > 0) {
        b->gs--;
        undo_record_delete(b->gs, b->buf[b->gs]);
        b->dirty = true;
    }
}

void doc_left(void)
{
    buf_t *b = cur();
    if (b->gs > 0) {
        b->buf[--b->ge] = b->buf[--b->gs];
    }
}

void doc_right(void)
{
    buf_t *b = cur();
    if (b->ge < b->cap) {
        b->buf[b->gs++] = b->buf[b->ge++];
    }
}

void doc_move_to(size_t pos)
{
    buf_t *b = cur();
    const size_t len = doc_len();
    if (pos > len) {
        pos = len;
    }
    while (b->gs > pos) {
        b->buf[--b->ge] = b->buf[--b->gs];
    }
    while (b->gs < pos) {
        b->buf[b->gs++] = b->buf[b->ge++];
    }
}

size_t doc_read(char *out, size_t max)
{
    const buf_t *b = cur();
    const size_t left  = b->gs;
    const size_t right = b->cap - b->ge;
    size_t n = 0;
    if (left > 0 && n < max) {
        const size_t k = left < max - n ? left : max - n;
        memcpy(out + n, b->buf, k);
        n += k;
    }
    if (right > 0 && n < max) {
        const size_t k = right < max - n ? right : max - n;
        memcpy(out + n, b->buf + b->ge, k);
        n += k;
    }
    return n;
}

void gapbuf_load(const char *data, size_t len)
{
    buf_t *b = cur();
    if (len > b->cap) {
        len = b->cap;
    }
    memcpy(b->buf, data, len);
    b->gs = len;
    b->ge = b->cap;
    b->dirty = false;
}

void doc_set_text(const char *s)
{
    gapbuf_load(s, strlen(s));
    doc_undo_reset();
}
