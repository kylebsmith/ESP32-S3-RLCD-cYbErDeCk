/* The gap buffer itself. Lives in PSRAM: documents and scrollback go there,
 * only the framebuffer has to be internal (docs/HARDWARE.md). */
#include "docstore.h"

#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "gapbuf";

static char  *s_buf;
static size_t s_cap;
static size_t s_gs;      /* gap start - also the cursor position */
static size_t s_ge;      /* gap end, exclusive */
static bool   s_dirty;

esp_err_t gapbuf_init(void)
{
    s_buf = heap_caps_malloc(DOC_CAPACITY, MALLOC_CAP_SPIRAM);
    if (s_buf == NULL) {
        s_buf = heap_caps_malloc(DOC_CAPACITY, MALLOC_CAP_DEFAULT);
    }
    if (s_buf == NULL) {
        ESP_LOGE(TAG, "cannot allocate %d B", DOC_CAPACITY);
        return ESP_ERR_NO_MEM;
    }
    s_cap = DOC_CAPACITY;
    s_gs  = 0;
    s_ge  = s_cap;
    s_dirty = false;
    return ESP_OK;
}

size_t doc_len(void)    { return s_cap - (s_ge - s_gs); }
size_t doc_cursor(void) { return s_gs; }
bool   doc_dirty(void)  { return s_dirty; }

void gapbuf_mark_clean(void) { s_dirty = false; }

char doc_at(size_t i)
{
    if (i >= doc_len()) {
        return '\0';
    }
    return i < s_gs ? s_buf[i] : s_buf[i + (s_ge - s_gs)];
}

void undo_record_insert(size_t pos, char c);
void undo_record_delete(size_t pos, char c);

void doc_insert(char c)
{
    if (s_gs == s_ge) {
        return;                       /* full - drop rather than overwrite */
    }
    undo_record_insert(s_gs, c);
    s_buf[s_gs++] = c;
    s_dirty = true;
}

void doc_backspace(void)
{
    if (s_gs > 0) {
        s_gs--;
        undo_record_delete(s_gs, s_buf[s_gs]);
        s_dirty = true;
    }
}

void doc_left(void)
{
    if (s_gs > 0) {
        s_buf[--s_ge] = s_buf[--s_gs];
    }
}

void doc_right(void)
{
    if (s_ge < s_cap) {
        s_buf[s_gs++] = s_buf[s_ge++];
    }
}

void doc_move_to(size_t pos)
{
    const size_t len = doc_len();
    if (pos > len) {
        pos = len;
    }
    while (s_gs > pos) {
        s_buf[--s_ge] = s_buf[--s_gs];
    }
    while (s_gs < pos) {
        s_buf[s_gs++] = s_buf[s_ge++];
    }
}

size_t doc_read(char *out, size_t max)
{
    const size_t left  = s_gs;
    const size_t right = s_cap - s_ge;
    size_t n = 0;
    if (left > 0 && n < max) {
        const size_t k = left < max - n ? left : max - n;
        memcpy(out + n, s_buf, k);
        n += k;
    }
    if (right > 0 && n < max) {
        const size_t k = right < max - n ? right : max - n;
        memcpy(out + n, s_buf + s_ge, k);
        n += k;
    }
    return n;
}

/* Replace the whole document, used by the journal on load. */
void gapbuf_load(const char *data, size_t len)
{
    if (len > s_cap) {
        len = s_cap;
    }
    memcpy(s_buf, data, len);
    s_gs = len;                       /* cursor lands at the end */
    s_ge = s_cap;
    s_dirty = false;
}

void doc_set_text(const char *s)
{
    gapbuf_load(s, strlen(s));
    doc_undo_reset();
}
