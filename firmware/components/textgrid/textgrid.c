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

/* THE FACE, PRE-TURNED - see draw_cell_turned. Every glyph of the face in the
 * framebuffer's own byte order for one orientation, built the first time each
 * is drawn. 12x24 is 124 glyphs of 36 bytes; a face that does not fit here
 * draws by the per-row path instead. */
#define TURN_CELL_MAX 36
static uint8_t s_turned[124 * TURN_CELL_MAX];
static uint8_t s_turned_have[(124 + 7) / 8];
static uint8_t s_turn_mask[8][TURN_CELL_MAX];   /* by attribute, TG_* bits */
static int  s_turn_orient = -1;     /* what s_turned was built for; -1 = nothing */
static int  s_turn_rows, s_turn_cols, s_turn_bytes;
static bool s_turn_fits;
static bool s_turn_on = true;

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

    /* A cell is ch/4 bytes along native x by cw/2 rows along native y. */
    s_turn_cols  = ch / 4;
    s_turn_rows  = cw / 2;
    s_turn_bytes = s_turn_cols * s_turn_rows;
    s_turn_fits  = scale == 1 && s_turn_bytes <= TURN_CELL_MAX &&
                   (size_t)(font->last - font->first + 1) * (size_t)s_turn_bytes
                       <= sizeof s_turned;
    s_turn_orient = -1;

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

/* One row of a cell as the panel shows it: the glyph's bits, then the inverse,
 * then the bars. MSB = leftmost. Both ways of drawing a cell come through
 * here, so they cannot disagree about what a cell looks like. */
static uint32_t row_bits(const uint8_t *g, int gy, uint8_t att)
{
    const bool inv   = (att & TG_INVERSE) != 0;
    const bool under = (att & TG_UNDER) != 0;
    const bool over  = (att & TG_OVER) != 0;
    /* The bar is applied AFTER the inverse, so it flips back out of a solid
     * block. That is what makes cursor-on-playhead readable as both rather
     * than as a slightly different block. */
    const bool bar = (under && gy >= s_font->h - under_px()) ||
                     (over && gy < under_px());
    const uint8_t *rowbits = &g[(size_t)gy * s_font->stride];

    uint32_t bits = 0;
    for (int gx = 0; gx < s_font->w; gx++) {
        bool on = tg_font_bit(s_font, rowbits, gx) != 0;
        if (inv) { on = !on; }
        if (bar) { on = !on; }
        if (on)  { bits |= 1u << (31 - gx); }
    }
    return bits;
}

static void draw_cell(int col, int row)
{
    const uint8_t *g = tg_font_glyph(s_font, (unsigned char)s_txt[row][col]);
    const uint8_t att = s_att[row][col];
    const int x0 = s_ox + col * s_cw;
    const int y0 = s_oy + row * s_ch;

    /* ONE BLIT PER GLYPH ROW, NOT ONE CALL PER PIXEL.
     *
     * The old loop called st7305_pixel_raw for every pixel, and each call ran
     * the orientation switch, a bounds test and the full index and mask
     * arithmetic - 288 times for a 12x24 cell. But nx is constant along a
     * logical row in every orientation (see st7305_row_start), so all of that
     * hoists out and a row becomes one tight loop over its twelve bits.
     *
     * This was the path everything paid; draw_cell_turned below now takes
     * every cell it can, and this is what is left for the rest - a scaled face
     * - and the reference the turned face is checked against. */
    const int w = s_font->w;
    for (int gy = 0; gy < s_font->h; gy++) {
        const uint32_t bits = row_bits(g, gy, att);
        if (s_scale == 1) {
            st7305_row_bits_raw(x0, y0 + gy, w, bits);
        } else {
            for (int gx = 0; gx < w; gx++) {
                st7305_fill_raw(x0 + gx * s_scale, y0 + gy * s_scale,
                                s_scale, s_scale,
                                ((bits >> (31 - gx)) & 1u) != 0);
            }
        }
    }
    st7305_damage(x0, y0, s_cw, s_ch);
}

/* ---- THE FACE, PRE-TURNED --------------------------------------------------
 *
 * A CELL IS ALWAYS WHOLE FRAMEBUFFER BYTES. A framebuffer byte is 4 native-x
 * by 2 native-y pixels, and a logical row is a native column (st7305_addr.h),
 * so a cell's height runs along native x and its width along native y.
 * tg_set_layout holds the cell height and origin y to multiples of 12 and the
 * cell width and origin x to even numbers - so a cell covers whole bytes, in
 * every orientation, because the mirrored ones count from 299 and 399 and 300
 * and 400 are multiples of 4 and 2 as well. A 12x24 cell is 6 rows of 6 bytes,
 * a 6x12 cell 3 of 3; and where a pixel lands inside its cell's bytes does not
 * depend on where the cell is.
 *
 * So each glyph is turned into its bytes once, the first time it is drawn in
 * this orientation, and drawing a cell is 36 stores - where the per-row path
 * gathered 24 rows of bits and set 288 pixels one read-modify-write at a time.
 * The attributes are XOR masks over the same bytes, one for each of the
 * eight combinations, and the masks are not written down anywhere: each is the
 * per-row path's own cell with the attributes XORed against the same cell
 * without them, so the two paths cannot disagree about where a bar is, or
 * about what a bar does to an inverted cell.
 *
 * tools/test_textgrid.c compiles this file and draws every glyph with every
 * attribute, in both faces and all four orientations, both ways, and demands
 * the same framebuffer byte for byte. */

/* A cell's framebuffer origin: the smallest native x and y of its pixels. */
static void cell_base(int orient, int x0, int y0, int *bnx, int *bny)
{
    int ax, ay, bx, by;
    st7305_to_native(orient, x0, y0, &ax, &ay);
    st7305_to_native(orient, x0 + s_cw - 1, y0 + s_ch - 1, &bx, &by);
    *bnx = ax < bx ? ax : bx;
    *bny = ay < by ? ay : by;
}

/* The per-row path's cell, as bytes: row_bits placed pixel by pixel through
 * st7305_to_native, for a cell at the origin. */
static void turn(const uint8_t *g, uint8_t att, int orient, uint8_t *out)
{
    int bnx, bny;
    cell_base(orient, 0, 0, &bnx, &bny);
    memset(out, 0, (size_t)s_turn_bytes);
    for (int gy = 0; gy < s_ch; gy++) {
        const uint32_t bits = row_bits(g, gy, att);
        for (int gx = 0; gx < s_cw; gx++) {
            if ((bits >> (31 - gx)) & 1u) {
                int nx, ny;
                st7305_to_native(orient, gx, gy, &nx, &ny);
                out[((ny - bny) >> 1) * s_turn_cols + ((nx - bnx) >> 2)] |=
                    st7305_fb_mask_n(nx, ny);
            }
        }
    }
}

/* Ready to draw turned cells in this orientation? Builds the masks, and
 * forgets every turned glyph, whenever the orientation is not the one they
 * were turned for - KEY cycles it at any time. */
static bool turn_ready(int orient)
{
    if (!s_turn_on || !s_turn_fits) {
        return false;
    }
    if (orient != s_turn_orient) {
        /* One mask for each of the eight attribute combinations, so a cell
         * costs one XOR a byte whatever it carries. */
        uint8_t plain[TURN_CELL_MAX];
        const uint8_t *g = tg_font_glyph(s_font, ' ');
        turn(g, TG_NORMAL, orient, plain);
        for (int att = 0; att < 8; att++) {
            turn(g, (uint8_t)att, orient, s_turn_mask[att]);
            for (int i = 0; i < s_turn_bytes; i++) {
                s_turn_mask[att][i] ^= plain[i];
            }
        }
        memset(s_turned_have, 0, sizeof s_turned_have);
        s_turn_orient = orient;
    }
    return true;
}

static void draw_cell_turned(int col, int row, int orient)
{
    const uint8_t *g = tg_font_glyph(s_font, (unsigned char)s_txt[row][col]);
    const size_t gi = (size_t)(g - s_font->data) /
                      ((size_t)s_font->h * s_font->stride);
    uint8_t *src = &s_turned[gi * (size_t)s_turn_bytes];
    if ((s_turned_have[gi >> 3] & (1u << (gi & 7))) == 0) {
        turn(g, TG_NORMAL, orient, src);
        s_turned_have[gi >> 3] |= (uint8_t)(1u << (gi & 7));
    }

    const uint8_t *m = s_turn_mask[s_att[row][col] & 7];

    const int x0 = s_ox + col * s_cw;
    const int y0 = s_oy + row * s_ch;
    int bnx, bny;
    cell_base(orient, x0, y0, &bnx, &bny);
    uint8_t *dst = st7305_framebuffer() +
                   (size_t)(bny >> 1) * ST7305_ROW_BYTES + (size_t)(bnx >> 2);
    for (int r = 0; r < s_turn_rows; r++, dst += ST7305_ROW_BYTES) {
        for (int c = 0; c < s_turn_cols; c++) {
            dst[c] = (uint8_t)(*src++ ^ *m++);
        }
    }
    st7305_damage(x0, y0, s_cw, s_ch);
}

void tg_set_turned(bool on)
{
    s_turn_on = on;
}

int tg_render(void)
{
    const int orient = (int)st7305_orientation();
    const bool turned = turn_ready(orient);
    int redrawn = 0;
    for (int r = 0; r < s_rows; r++) {
        for (int c = 0; c < s_cols; c++) {
            if (is_dirty(c, r)) {
                if (turned) {
                    draw_cell_turned(c, r, orient);
                } else {
                    draw_cell(c, r);
                }
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
