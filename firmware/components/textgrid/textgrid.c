#include "textgrid.h"

#include <string.h>

#include "esp_log.h"
#include "font6x12.h"

static const char *TAG = "textgrid";

static const tg_font_t *s_font;
static int s_scale = 1;
static int s_cols, s_rows, s_cw, s_ch;

static char    s_txt[TG_MAX_ROWS][TG_MAX_COLS];
static uint8_t s_att[TG_MAX_ROWS][TG_MAX_COLS];
static uint8_t s_dirty[(TG_MAX_ROWS * TG_MAX_COLS + 7) / 8];

static inline void mark(int col, int row)
{
    const int i = row * TG_MAX_COLS + col;
    s_dirty[i >> 3] |= (uint8_t)(1u << (i & 7));
}

static inline bool is_dirty(int col, int row)
{
    const int i = row * TG_MAX_COLS + col;
    return (s_dirty[i >> 3] & (1u << (i & 7))) != 0;
}

esp_err_t tg_set_font(const tg_font_t *font, int scale)
{
    if (font == NULL || scale < 1) {
        return ESP_ERR_INVALID_ARG;
    }
    const int cw = font->w * scale;
    const int ch = font->h * scale;

    /* The two alignment rules, enforced. A cell height that is not a multiple
     * of 12 makes one line's damage window spill into its neighbours; an odd
     * width does the same on the other axis. */
    if (ch % 12 != 0) {
        ESP_LOGE(TAG, "cell height %d is not a multiple of 12", ch);
        return ESP_ERR_INVALID_ARG;
    }
    if (cw % 2 != 0) {
        ESP_LOGE(TAG, "cell width %d is odd", cw);
        return ESP_ERR_INVALID_ARG;
    }

    const int cols = ST7305_WIDTH / cw;
    const int rows = ST7305_HEIGHT / ch;
    if (cols < 1 || rows < 1 || cols > TG_MAX_COLS || rows > TG_MAX_ROWS) {
        ESP_LOGE(TAG, "grid %dx%d out of range", cols, rows);
        return ESP_ERR_INVALID_ARG;
    }

    s_font = font; s_scale = scale;
    s_cw = cw; s_ch = ch; s_cols = cols; s_rows = rows;

    for (int r = 0; r < TG_MAX_ROWS; r++) {
        for (int c = 0; c < TG_MAX_COLS; c++) {
            s_txt[r][c] = ' ';
            s_att[r][c] = TG_NORMAL;
        }
    }
    tg_invalidate();
    ESP_LOGI(TAG, "font %s x%d -> cell %dx%d, grid %dx%d",
             font->name, scale, cw, ch, cols, rows);
    return ESP_OK;
}

int tg_cols(void)   { return s_cols; }
int tg_rows(void)   { return s_rows; }
int tg_cell_w(void) { return s_cw; }
int tg_cell_h(void) { return s_ch; }
const char *tg_font_name(void) { return s_font != NULL ? s_font->name : "none"; }

void tg_invalidate(void)
{
    memset(s_dirty, 0xFF, sizeof s_dirty);
}

void tg_clear(void)
{
    for (int r = 0; r < s_rows; r++) {
        for (int c = 0; c < s_cols; c++) {
            if (s_txt[r][c] != ' ' || s_att[r][c] != TG_NORMAL) {
                s_txt[r][c] = ' ';
                s_att[r][c] = TG_NORMAL;
                mark(c, r);
            }
        }
    }
}

void tg_put(int col, int row, char ch, int attr)
{
    if ((unsigned)col >= (unsigned)s_cols || (unsigned)row >= (unsigned)s_rows) {
        return;
    }
    if (s_txt[row][col] == ch && s_att[row][col] == (uint8_t)attr) {
        return;                                /* nothing actually changed */
    }
    s_txt[row][col] = ch;
    s_att[row][col] = (uint8_t)attr;
    mark(col, row);
}

void tg_puts(int col, int row, const char *s, int attr)
{
    for (; *s != '\0' && col < s_cols; s++, col++) {
        tg_put(col, row, *s, attr);
    }
}

void tg_puts_right(int end, int row, const char *s, int attr)
{
    const int len = (int)strlen(s);
    tg_puts(end - len + 1, row, s, attr);
}

void tg_fill(int col, int row, int count, char ch, int attr)
{
    for (int i = 0; i < count; i++) {
        tg_put(col + i, row, ch, attr);
    }
}

static void draw_cell(int col, int row)
{
    const uint8_t *g = tg_font_glyph(s_font, (unsigned char)s_txt[row][col]);
    const bool inv = s_att[row][col] == TG_INVERSE;
    const int x0 = col * s_cw;
    const int y0 = row * s_ch;
    const int stride = s_font->stride;

    for (int gy = 0; gy < s_font->h; gy++) {
        const uint8_t *rowbits = &g[(size_t)gy * stride];
        for (int gx = 0; gx < s_font->w; gx++) {
            bool on = tg_font_bit(s_font, rowbits, gx) != 0;
            if (inv) {
                on = !on;
            }
            if (s_scale == 1) {
                st7305_pixel(x0 + gx, y0 + gy, on);
            } else {
                st7305_fill(x0 + gx * s_scale, y0 + gy * s_scale,
                            s_scale, s_scale, on);
            }
        }
    }
}

int tg_render(void)
{
    int redrawn = 0;
    for (int r = 0; r < s_rows; r++) {
        for (int c = 0; c < s_cols; c++) {
            if (is_dirty(c, r)) {
                draw_cell(c, r);
                redrawn++;
            }
        }
    }
    memset(s_dirty, 0, sizeof s_dirty);
    return redrawn;
}

int tg_flush(size_t *bytes)
{
    const int redrawn = tg_render();
    if (redrawn > 0) {
        st7305_flush(bytes);
    } else if (bytes != NULL) {
        *bytes = 0;
    }
    return redrawn;
}
