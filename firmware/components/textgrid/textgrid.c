#include "textgrid.h"

#include <string.h>

#include "esp_log.h"
#include "font6x12.h"

static const char *TAG = "textgrid";

static const tg_font_t *s_font;
static int s_scale = 1;
static int s_cols, s_rows, s_cw, s_ch;
static int s_ox, s_oy;

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

esp_err_t tg_set_layout(const tg_font_t *font, int scale,
                        int origin_x, int origin_y, int cols, int rows)
{
    if (font == NULL || scale < 1) {
        return ESP_ERR_INVALID_ARG;
    }
    const int cw = font->w * scale;
    const int ch = font->h * scale;

    /* The two alignment rules, enforced - on the cell AND on the origin. */
    if (ch % 12 != 0 || origin_y % 12 != 0) {
        ESP_LOGE(TAG, "cell height %d / origin y %d must be multiples of 12",
                 ch, origin_y);
        return ESP_ERR_INVALID_ARG;
    }
    if (cw % 2 != 0 || origin_x % 2 != 0) {
        ESP_LOGE(TAG, "cell width %d / origin x %d must be even", cw, origin_x);
        return ESP_ERR_INVALID_ARG;
    }
    if (cols < 1 || rows < 1 || cols > TG_MAX_COLS || rows > TG_MAX_ROWS ||
        origin_x + cols * cw > ST7305_WIDTH ||
        origin_y + rows * ch > ST7305_HEIGHT) {
        ESP_LOGE(TAG, "grid %dx%d at (%d,%d) does not fit",
                 cols, rows, origin_x, origin_y);
        return ESP_ERR_INVALID_ARG;
    }

    s_font = font; s_scale = scale;
    s_cw = cw; s_ch = ch; s_cols = cols; s_rows = rows;
    s_ox = origin_x; s_oy = origin_y;

    for (int r = 0; r < TG_MAX_ROWS; r++) {
        for (int c = 0; c < TG_MAX_COLS; c++) {
            s_txt[r][c] = ' ';
            s_att[r][c] = TG_NORMAL;
        }
    }
    tg_invalidate();
    ESP_LOGI(TAG, "font %s x%d -> cell %dx%d, grid %dx%d at (%d,%d)",
             font->name, scale, cw, ch, cols, rows, origin_x, origin_y);
    return ESP_OK;
}

esp_err_t tg_set_font(const tg_font_t *font, int scale)
{
    if (font == NULL || scale < 1) {
        return ESP_ERR_INVALID_ARG;
    }
    return tg_set_layout(font, scale, 0, 0,
                         ST7305_WIDTH / (font->w * scale),
                         ST7305_HEIGHT / (font->h * scale));
}

int tg_origin_x(void) { return s_ox; }
int tg_origin_y(void) { return s_oy; }

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

/* Blit one glyph at an arbitrary pixel position. */
static void blit_glyph(int px, int py, char ch, bool inv)
{
    const uint8_t *g = tg_font_glyph(s_font, (unsigned char)ch);
    for (int gy = 0; gy < s_font->h; gy++) {
        const uint8_t *rowbits = &g[(size_t)gy * s_font->stride];
        for (int gx = 0; gx < s_font->w; gx++) {
            bool on = tg_font_bit(s_font, rowbits, gx) != 0;
            if (inv) {
                on = !on;
            }
            if (s_scale == 1) {
                st7305_pixel_raw(px + gx, py + gy, on);
            } else {
                st7305_fill_raw(px + gx * s_scale, py + gy * s_scale,
                                s_scale, s_scale, on);
            }
        }
    }
    /* One rectangle for the whole glyph, declared once. */
    st7305_damage(px, py, s_font->w * s_scale, s_font->h * s_scale);
}

int tg_text_width_px(const char *s)
{
    return (int)strlen(s) * s_cw;
}

void tg_draw_text_px(int px, int py, const char *s, int attr)
{
    const bool inv = attr == TG_INVERSE;
    for (; *s != '\0'; s++, px += s_cw) {
        if (px + s_cw > ST7305_WIDTH) {
            break;
        }
        blit_glyph(px, py, *s, inv);
    }
}

/* How tall the TG_UNDER bar is, in font pixels before scaling. A single row
 * disappears on a low-contrast reflective panel at arm's length; a third of
 * the cell would read as a block again. Proportional to the face so the
 * 6x12 and 12x24 fonts look like the same idea. */
static int under_px(void)
{
    const int n = s_font->h / 6;
    return n < 2 ? 2 : n;
}

static void draw_cell(int col, int row)
{
    const uint8_t *g = tg_font_glyph(s_font, (unsigned char)s_txt[row][col]);
    const uint8_t att = s_att[row][col];
    const bool inv   = (att & TG_INVERSE) != 0;
    const bool under = (att & TG_UNDER) != 0;
    const int  ubar  = s_font->h - under_px();
    const int x0 = s_ox + col * s_cw;
    const int y0 = s_oy + row * s_ch;
    const int stride = s_font->stride;

    for (int gy = 0; gy < s_font->h; gy++) {
        const uint8_t *rowbits = &g[(size_t)gy * stride];
        /* The bar is applied AFTER the inverse, so it flips back out of a
         * solid block. That is what makes cursor-on-playhead readable as
         * both rather than as a slightly different block. */
        const bool bar = under && gy >= ubar;
        for (int gx = 0; gx < s_font->w; gx++) {
            bool on = tg_font_bit(s_font, rowbits, gx) != 0;
            if (inv) {
                on = !on;
            }
            if (bar) {
                on = !on;
            }
            if (s_scale == 1) {
                st7305_pixel_raw(x0 + gx, y0 + gy, on);
            } else {
                st7305_fill_raw(x0 + gx * s_scale, y0 + gy * s_scale,
                                s_scale, s_scale, on);
            }
        }
    }
    st7305_damage(x0, y0, s_cw, s_ch);
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
