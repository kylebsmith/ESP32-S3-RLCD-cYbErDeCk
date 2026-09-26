/*
 * The pre-turned face draws exactly what the per-row path draws.
 *
 * textgrid.c draws a cell from glyph bytes turned once into the framebuffer's
 * own order, with the attributes as XOR masks - 36 stores where the per-row
 * path set 288 pixels. That is a claim about the ST7305's byte layout in all
 * four orientations, and if it is wrong the screen shows plausible glyphs in
 * the wrong places, or right glyphs with a bar in the wrong row: legible enough
 * to look like a font bug.
 *
 * So this compiles textgrid.c ITSELF, gives it a framebuffer whose pixels are
 * set by st7305_addr.h's own definition of a pixel, and for both faces, the
 * editor's layout and a flush one, and all four orientations, draws every
 * glyph with every combination of attributes both ways - onto framebuffers
 * full of the same junk - and demands identical bytes, including every byte
 * outside the cells. Then it breaks the turned face on purpose, two ways, and
 * demands the comparison notice.
 *
 *   cc -std=c11 -I firmware/components/textgrid/include \
 *      -I firmware/components/st7305/include -I tools/hostshim \
 *      tools/test_textgrid.c firmware/components/textgrid/font12x24.c \
 *      firmware/components/textgrid/font6x12.c -o /tmp/t && /tmp/t
 */
#include <stdio.h>
#include <string.h>

#include "../firmware/components/textgrid/textgrid.c"

/* ---- the panel, as st7305_addr.h defines a pixel ------------------------ */

static uint8_t FB_A[ST7305_FB_SIZE], FB_B[ST7305_FB_SIZE];
static uint8_t *s_host_fb = FB_A;
static int s_host_orient;
static long s_row_blits, s_damage_sum, s_damage_n;

uint8_t *st7305_framebuffer(void) { return s_host_fb; }
st7305_orient_t st7305_orientation(void) { return (st7305_orient_t)s_host_orient; }

void st7305_pixel_raw(int x, int y, bool on)
{
    if ((unsigned)x >= ST7305_WIDTH || (unsigned)y >= ST7305_HEIGHT) {
        return;
    }
    int nx, ny;
    st7305_to_native(s_host_orient, x, y, &nx, &ny);
    uint8_t *p = &s_host_fb[st7305_fb_index_n(nx, ny)];
    const uint8_t m = st7305_fb_mask_n(nx, ny);
    if (on) { *p |= m; } else { *p = (uint8_t)(*p & ~m); }
}

void st7305_fill_raw(int x, int y, int w, int h, bool on)
{
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            st7305_pixel_raw(xx, yy, on);
        }
    }
}

/* The driver's row blit is checked against this same per-pixel definition
 * by tools/test_st7305_addr.c. */
void st7305_row_bits_raw(int x, int y, int w, uint32_t bits)
{
    s_row_blits++;
    for (int i = 0; i < w; i++) {
        st7305_pixel_raw(x + i, y, ((bits >> (31 - i)) & 1u) != 0);
    }
}

void st7305_damage(int x, int y, int w, int h)
{
    s_damage_n++;
    s_damage_sum = s_damage_sum * 31 + x * 7 + y * 131 + w * 1031 + h * 7919;
}

esp_err_t st7305_flush(size_t *bytes) { if (bytes) { *bytes = 0; } return ESP_OK; }

/* ---- the check ----------------------------------------------------------- */

static int fails, cases;

static void junk(uint32_t seed)
{
    uint32_t v = seed * 2654435761u + 1;
    for (size_t i = 0; i < sizeof FB_A; i++) {
        v = v * 1664525u + 1013904223u;
        FB_A[i] = FB_B[i] = (uint8_t)(v >> 24);
    }
}

/* Every cell: glyph i mod the face, attribute p + i / glyphs, so over eight
 * passes every glyph meets every one of the eight attribute combinations. */
static void fill(int pass, int glyphs, int first)
{
    int i = 0;
    for (int r = 0; r < tg_rows(); r++) {
        for (int c = 0; c < tg_cols(); c++, i++) {
            tg_put(c, r, (char)(first + i % glyphs), (pass + i / glyphs) & 7);
        }
    }
}

/* Draw the grid both ways onto the same junk. 0 = the same bytes. */
static int both_ways(uint32_t seed, long *blits_turned)
{
    junk(seed);
    s_host_fb = FB_A;
    tg_set_turned(false);
    tg_invalidate();
    s_damage_sum = s_damage_n = 0;
    tg_render();
    const long dsum = s_damage_sum, dn = s_damage_n;

    s_host_fb = FB_B;
    tg_set_turned(true);
    tg_invalidate();
    s_damage_sum = s_damage_n = 0;
    const long before = s_row_blits;
    tg_render();
    *blits_turned = s_row_blits - before;
    s_host_fb = FB_A;
    if (dsum != s_damage_sum || dn != s_damage_n) {
        return -1;
    }
    return memcmp(FB_A, FB_B, sizeof FB_A) != 0;
}

static void layout(const tg_font_t *f, int scale, int ox, int oy, int cols,
                   int rows, const char *what)
{
    if (tg_set_layout(f, scale, ox, oy, cols, rows) != ESP_OK) {
        printf("[FAIL] %s: the layout was refused\n", what);
        fails++;
        return;
    }
    const int glyphs = f->last - f->first + 1;
    int bad = 0, bad_damage = 0, not_turned = 0;
    for (int o = 0; o < 4; o++) {           /* orientation changes mid-layout */
        s_host_orient = o;
        for (int pass = 0; pass < 8; pass++) {
            fill(pass, glyphs, f->first);
            long blits = 0;
            const int e = both_ways((uint32_t)(o * 8 + pass + 1), &blits);
            cases++;
            if (e < 0) { bad_damage++; }
            if (e > 0) {
                if (bad < 3) {
                    printf("[FAIL] %s orientation %d pass %d: bytes differ\n",
                           what, o, pass);
                }
                bad++;
            }
            if (scale == 1 && blits != 0) { not_turned++; }
        }
    }
    if (bad == 0 && bad_damage == 0 && not_turned == 0) {
        printf("[ ok ] %s: both ways the same bytes, 4 orientations x 8 passes\n",
               what);
    } else {
        if (bad_damage) {
            printf("[FAIL] %s: damage declared differently %d times\n", what,
                   bad_damage);
        }
        if (not_turned) {
            printf("[FAIL] %s: the turned face was not used (%d passes drew by rows)\n",
                   what, not_turned);
        }
        fails++;
    }
}

int main(void)
{
    /* The editor's two layouts (editor_set_density) and the flush ones. */
    layout(&tg_font_12x24, 1, 20, 12, 30, 12, "12x24 at the editor's margins");
    layout(&tg_font_12x24, 1, 0, 0, 33, 12, "12x24 flush");
    layout(&tg_font_6x12, 1, 20, 12, 60, 24, "6x12 at the editor's margins");
    layout(&tg_font_6x12, 1, 0, 0, 66, 25, "6x12 flush");
    /* A face the turned path does not take falls back, and still draws. */
    layout(&tg_font_6x12, 2, 20, 12, 30, 12, "6x12 at scale 2 (per-row path)");

    /* ---- and the comparison can tell a wrong face apart ---------------- */
    tg_set_layout(&tg_font_12x24, 1, 20, 12, 30, 12);
    int caught = 0;
    for (int o = 0; o < 4; o++) {
        /* A textgrid that forgot to re-turn when KEY cycled the orientation:
         * the last orientation's bytes, drawn at this one's places. */
        const int next = (o + 1) & 3;
        s_host_orient = o;
        fill(0, 124, 32);
        long blits;
        both_ways(99, &blits);                 /* turned for o */
        s_host_orient = next;
        s_turn_orient = next;                  /* ...and told it is fine */
        if (both_ways(100, &blits) != 0) { caught++; }
        s_turn_orient = -1;
    }
    printf(caught == 4 ? "[ ok ] a stale orientation is caught, 4 of 4\n"
                       : "[FAIL] a stale orientation passed: caught %d of 4\n",
           caught);
    if (caught != 4) { fails++; }

    /* One bit wrong in one turned glyph. */
    s_host_orient = 1;
    fill(0, 124, 32);
    long blits;
    both_ways(7, &blits);
    s_turned[('A' - 32) * 36 + 17] ^= 0x10;
    const int one_bit = both_ways(8, &blits);
    printf(one_bit ? "[ ok ] one wrong bit in one glyph is caught\n"
                   : "[FAIL] one wrong bit in one glyph passed\n");
    if (!one_bit) { fails++; }

    if (fails) {
        printf("[FAIL] %d check(s) failed\n", fails);
    } else {
        printf("[PASS] the turned face is the per-row path, %d cases\n", cases);
    }
    return fails != 0;
}
