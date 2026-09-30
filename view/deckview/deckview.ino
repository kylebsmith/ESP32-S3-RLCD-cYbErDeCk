// deckview - the deck's picture on HDMI, from an Adafruit Feather RP2040 DVI.
//
// docs/NEXT.md §6: the visualization node, and docs/VIEW.md: its wire format.
// The deck sends a picture a step, and ahead of each a control frame naming
// how to draw it. The deck only names the mode; everything below happens here,
// so none of it costs the deck anything, and all of it is worked out from the
// frames and their ticks alone - the same performance draws the same pictures.
//
// EVERY PIXEL IS THE DECK'S (2026-09-29). Each cell is drawn with the deck's own
// 6 x 12 tile, bit for bit, and each of its pixels is a 2 x 2 block on the HDMI
// screen: 53 x 20 cells are the whole screen. Nothing is blended, resampled or
// re-dithered. me, looking at the old modes: the smoothing "blurred and
// mushed" the "chunky blocks, rigid ass lovely pixels"; phosphor and feedback,
// which only worked by resampling, are retired, and their numbers draw plain.
//
//   plain     the deck's cells, tile for tile
//   scan      each row of cells one line across the screen, stepped up by its greys
//   riso      two plates out of register: this step, and the step before last
//   poster    a live Swiss poster of the piece, from the lines the deck sends
//   code      the code itself, round the cursor, over the picture dimmed
//
// COLOUR IS PLAYED, NOT PICKED: controllers 1-8 on MIDI channel 16, which the
// deck forwards with every control frame (view_wire.h) - ink, paper, saturation,
// day, invert, the glyphs' accent, drift, and the grid. See params().
//
// 320 x 240, eight bits a pixel through a palette, doubled to 640 x 480 at 60 Hz
// - the mode every HDMI screen takes. Two framebuffers: the next picture is drawn
// while the last is shown.
//
// Frames arrive over USB serial. Today the deck's frames are relayed by a
// computer (tools/viewrelay.py); the same bytes are what the deck will send
// itself when it drives this board directly (docs/VIEW.md, "The wire").
//
//   arduino-cli compile -b rp2040:rp2040:adafruit_feather_dvi view/deckview
//   arduino-cli upload  -b rp2040:rp2040:adafruit_feather_dvi -p <port> view/deckview

#include <PicoDVI.h>
#include <math.h>
#include "deckfont.h"
#include "tinyfont.h"
#include "view_read.h"

typedef struct { int r, g, b; } col_t;       // a colour, 0-255 a channel
static inline col_t C(int r, int g, int b) { col_t c = { r, g, b }; return c; }

DVIGFX8 display(DVI_RES_320x240p60, true, adafruit_feather_dvi_cfg);

#define W 320
#define H 240
#define CW 6                                   // a cell: the deck's compact tile
#define CH 12
#define COLS (W / CW)                          // 53
#define ROWS (H / CH)                          // 20

// The modes, numbered as the deck's firmware/main/view_wire.h numbers them.
enum { PLAIN, SCAN, SORT, LATENT, RISO, POSTER, CODE, MODES };
static const char *const NAMES[MODES] = { "plain", "scan", "sort", "latent",
                                          "riso", "poster", "code" };

static view_reader_t s_rd;
static int s_mode = PLAIN, s_pal_dirty = 2;
static uint32_t s_label_until;          // show the mode's name until then
static uint8_t s_par_seen[8];            // the parameters the palette was made from

// ---- the cells, as the deck sent them ------------------------------------
//
// This step and the two before, for riso's second plate.

static uint8_t s_cells[3][VIEW_MAX_H][VIEW_MAX_W];
static uint8_t s_cw[3], s_ch[3];
static int s_cur;
static uint8_t s_tone_of[256];           // 0..8: how grey a cell is, for scan
static bool    s_glyph[256];             // a letter or a sign, not a tone

static void tones_init(void)
{
  for (int c = 0; c < 256; c++) {
    int ink = 0;
    if (c >= DECKFONT_FIRST && c <= DECKFONT_LAST) {
      const int g = c - DECKFONT_FIRST;
      for (int r = 0; r < CH; r++) {
        uint8_t bits = deckfont_6x12[g * CH + r];
        while (bits) { ink += bits & 1; bits >>= 1; }
      }
    }
    s_tone_of[c] = (uint8_t)((ink * 8 + 36) / 72);
    if (s_tone_of[c] > 8) s_tone_of[c] = 8;
  }
  for (int t = 0; t <= 8; t++) s_tone_of[128 + t] = (uint8_t)t;
  s_tone_of[' '] = 0;
  for (int c = 0; c < 256; c++) s_glyph[c] = (c > ' ' && c < 127) || (c >= 137 && c <= DECKFONT_LAST);
}

static void keep_cells(const uint8_t *cells, int w, int h)
{
  s_cur = (s_cur + 1) % 3;
  for (int y = 0; y < h; y++) memcpy(s_cells[s_cur][y], cells + y * w, w);
  s_cw[s_cur] = (uint8_t)w;
  s_ch[s_cur] = (uint8_t)h;
}

// ONE CELL, BIT FOR BIT: the deck's 6 x 12 tile at (x, y). A tone is drawn in
// `ink`, a glyph in `accent`; paper is left alone. `plate` ORs instead of
// setting, for riso's two inks.
static void cell(uint8_t *fb, int x, int y, uint8_t c, uint8_t ink, uint8_t accent, bool plate)
{
  if (c < DECKFONT_FIRST || c > DECKFONT_LAST || c == ' ') return;
  const int g = c - DECKFONT_FIRST;
  const uint8_t v = s_glyph[c] ? accent : ink;
  for (int r = 0; r < CH; r++) {
    const int Y = y + r;
    if (Y < 0 || Y >= H) continue;
    const uint8_t bits = deckfont_6x12[g * CH + r];
    if (!bits) continue;
    for (int k = 0; k < CW; k++) {
      const int X = x + k;
      if (!(bits & (0x80 >> k)) || X < 0 || X >= W) continue;
      if (plate) fb[Y * W + X] |= v; else fb[Y * W + X] = v;
    }
  }
}

// A slot's cells into the box [x0, x0 + cols*6) x [y0, y0 + rows*12), cropped
// about the frame's centre when the frame is larger than the box.
static void cells_at(uint8_t *fb, int slot, int x0, int y0, int cols, int rows,
                     uint8_t ink, uint8_t accent, bool plate)
{
  const int w = s_cw[slot], h = s_ch[slot];
  const int nc = w < cols ? w : cols, nr = h < rows ? h : rows;
  const int cx0 = (w - nc) / 2, cy0 = (h - nr) / 2;
  const int bx = x0 + (cols - nc) * CW / 2, by = y0 + (rows - nr) * CH / 2;
  for (int j = 0; j < nr; j++)
    for (int i = 0; i < nc; i++)
      cell(fb, bx + i * CW, by + j * CH, s_cells[slot][cy0 + j][cx0 + i], ink, accent, plate);
}

// The whole screen's box: the frame centred, 53 x 20 at most.
static void screen_box(int slot, int *x0, int *y0, int *cols, int *rows)
{
  *cols = s_cw[slot] < COLS ? s_cw[slot] : COLS;
  *rows = s_ch[slot] < ROWS ? s_ch[slot] : ROWS;
  *x0 = (W - *cols * CW) / 2;
  *y0 = (H - *rows * CH) / 2;
}

// ---- type, in the deck's own faces ---------------------------------------

static void glyph12(uint8_t *fb, int x, int y, int ch, int s, uint8_t c)
{
  if (ch < DECKFONT_FIRST || ch > DECKFONT_LAST) ch = ' ';
  const int g = ch - DECKFONT_FIRST;
  for (int r = 0; r < 24; r++) {
    const uint16_t bits = (deckfont_12x24[(g * 24 + r) * 2] << 8) | deckfont_12x24[(g * 24 + r) * 2 + 1];
    if (!bits) continue;
    for (int k = 0; k < 12; k++) {
      if (!(bits & (0x8000 >> k))) continue;
      for (int yy = 0; yy < s; yy++)
        for (int xx = 0; xx < s; xx++) {
          const int X = x + k * s + xx, Y = y + r * s + yy;
          if (X >= 0 && X < W && Y >= 0 && Y < H) fb[Y * W + X] = c;
        }
    }
  }
}

static void glyph6(uint8_t *fb, int x, int y, int ch, uint8_t c)
{
  if (ch < DECKFONT_FIRST || ch > DECKFONT_LAST) ch = ' ';
  const int g = ch - DECKFONT_FIRST;
  for (int r = 0; r < 12; r++) {
    const uint8_t bits = deckfont_6x12[g * 12 + r];
    for (int k = 0; k < 6; k++) {
      const int X = x + k, Y = y + r;
      if ((bits & (0x80 >> k)) && X >= 0 && X < W && Y >= 0 && Y < H) fb[Y * W + X] = c;
    }
  }
}

static int text12(uint8_t *fb, int x, int y, const char *t, int n, int s, uint8_t c)
{
  for (int i = 0; i < n && t[i]; i++) glyph12(fb, x + i * 12 * s, y, (uint8_t)t[i], s, c);
  return x + n * 12 * s;
}

static void text6(uint8_t *fb, int x, int y, const char *t, uint8_t c)
{
  for (int i = 0; t[i]; i++) glyph6(fb, x + i * 6, y, (uint8_t)t[i], c);
}

// The small face, tinyfont.h: 3 x 5 in a 4 x 6 cell, for a whole pattern on a line.
static void tiny(uint8_t *fb, int x, int y, int ch, uint8_t c)
{
  if (ch < 32 || ch > 126) ch = '?';
  const uint16_t g = tinyfont_3x5[ch - 32];
  for (int r = 0; r < 5; r++)
    for (int k = 0; k < 3; k++)
      if (g & (0x4000 >> (r * 3 + k))) {
        const int X = x + k, Y = y + r;
        if (X >= 0 && X < W && Y >= 0 && Y < H) fb[Y * W + X] = c;
      }
}

static void tiny_text(uint8_t *fb, int x, int y, const char *t, int n, uint8_t c)
{
  for (int i = 0; i < n && t[i]; i++) tiny(fb, x + i * 4, y, (uint8_t)t[i], c);
}

static void fill(uint8_t *fb, int x, int y, int w, int h, uint8_t c)
{
  for (int Y = y; Y < y + h; Y++)
    if (Y >= 0 && Y < H)
      for (int X = x; X < x + w; X++)
        if (X >= 0 && X < W) fb[Y * W + X] = c;
}

// ---- colour: the performer's, from channel 16 ----------------------------
//
// A parameter is 0-127, or 255 when the deck has sent nothing for it. With all
// of them unset, plain and scan are what they always were: warm light on black.
//
//   cc 1  ink hue            cc 5  invert, 64 and up
//   cc 2  paper hue          cc 6  the glyphs' own hue: sparkles, letters, the disc
//   cc 3  saturation         cc 7  drift (riso) or lift (scan)
//   cc 4  day: 0 light on black, 127 black on paper, and every grey between
//   cc 8  the deck's cell grid, shown, this bright

static int par(int i) { return s_rd.params[i]; }
static bool set(int i) { return s_rd.params[i] != 255; }

static uint16_t rgb(int r, int g, int b)
{
  r = r < 0 ? 0 : r > 255 ? 255 : r;
  g = g < 0 ? 0 : g > 255 ? 255 : g;
  b = b < 0 ? 0 : b > 255 ? 255 : b;
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// A hue 0-127 round the wheel, saturation and value 0-255.
static col_t hsv(int h, int s, int v)
{
  const int hh = ((h % 128) + 128) % 128 * 6, sx = hh / 128, f = hh % 128;
  const int p = v * (255 - s) / 255;
  const int q = v * (255 - s * f / 127) / 255;
  const int t = v * (255 - s * (127 - f) / 127) / 255;
  switch (sx) {
  case 0: return C(v, t, p);
  case 1: return C(q, v, p);
  case 2: return C(p, v, t);
  case 3: return C(p, q, v);
  case 4: return C(t, p, v);
  default: return C(v, p, q);
  }
}

static col_t mix(col_t a, col_t b, int k)            // k 0..256 of b
{
  return C(a.r + (b.r - a.r) * k / 256, a.g + (b.g - a.g) * k / 256,
                  a.b + (b.b - a.b) * k / 256);
}

static void put(uint16_t *p, int i, col_t c) { p[i] = rgb(c.r, c.g, c.b); }

// Paper, ink and the glyphs' colour from the parameters: 0 paper, 1 ink,
// 2 glyphs, 3 both of riso's plates, 5 the grid.
static void palette(int mode)
{
  uint16_t *p = display.getPalette();
  for (int i = 0; i < 256; i++) p[i] = 0;
  const bool riso = (mode == RISO);
  const int day = set(3) ? par(3) : (riso ? 127 : 0);
  const int sat = set(2) ? par(2) * 2 : (riso ? 255 : 0);
  col_t paper, ink, ink2, glyph;
  if (riso && !set(0) && !set(1) && !set(2) && !set(3)) {
    paper = C(244, 240, 229);                  // paper, pink, blue, and both
    ink = C(255, 72, 176);
    ink2 = C(0, 120, 191);
  } else {
    paper = hsv(set(1) ? par(1) : 10, sat * 35 / 100, 8 + day * 237 / 127);
    const int light = day >= 64;
    ink = (!set(0) && !set(2)) ? (light ? C(18, 18, 18) : C(236, 236, 228))
                               : hsv(set(0) ? par(0) : 0, sat, light ? 40 + sat * 150 / 255 : 245);
    ink2 = hsv((set(5) ? par(5) : (set(0) ? par(0) : 116) + 64), riso ? 255 : sat,
               light ? 60 + sat * 120 / 255 : 230);
  }
  glyph = set(5) ? hsv(par(5), 255, day >= 64 ? 200 : 255) : ink;
  if (set(4) && par(4) >= 64) { col_t t = paper; paper = ink; ink = t; }
  put(p, 0, paper);
  put(p, 1, ink);
  put(p, 2, riso ? ink2 : glyph);
  put(p, 3, C(ink.r < ink2.r ? ink.r : ink2.r, ink.g < ink2.g ? ink.g : ink2.g,
                     ink.b < ink2.b ? ink.b : ink2.b));
  if (riso && day < 64) put(p, 3, mix(ink, ink2, 128));
  put(p, 5, mix(paper, ink, set(7) ? 16 + par(7) : 0));
  if (mode == POSTER) {                                // paper, ink, a red, and a grey
    p[0] = rgb(246, 244, 238);
    p[1] = rgb(18, 18, 18);
    p[2] = set(5) ? p[2] : rgb(228, 0, 43);
    p[3] = rgb(150, 148, 142);
    if (set(5)) { const col_t a = hsv(par(5), 255, 220); put(p, 2, a); }
  } else if (mode == SORT) {                           // a ramp, ink to paper, for the streaks
    for (int k = 0; k < 8; k++) put(p, 6 + k, mix(ink, paper, 32 + k * 28));
  } else if (mode == CODE || mode == LATENT) {         // black, a grey, text, red, grey
    p[0] = rgb(0, 0, 0);
    p[1] = rgb(58, 58, 54);
    p[2] = rgb(236, 236, 228);
    p[3] = rgb(228, 0, 43);
    p[4] = rgb(128, 128, 122);
    if (set(5)) { const col_t a = hsv(par(5), 255, 255); put(p, 3, a); }
  }
}

// ---- the modes -----------------------------------------------------------

static uint32_t step_of(uint32_t tick) { return tick / 24; }   // 96 to the beat

// The deck's cell grid itself, the lines between its cells: the Swiss reveal.
static void grid_lines(uint8_t *fb, int x0, int y0, int cols, int rows)
{
  if (!set(7) || par(7) == 0) return;
  for (int j = 0; j <= rows; j++) {
    const int y = y0 + j * CH - (j == rows);
    if (y >= 0 && y < H) for (int x = x0; x < x0 + cols * CW && x < W; x++) fb[y * W + x] = 5;
  }
  for (int i = 0; i <= cols; i++) {
    const int x = x0 + i * CW - (i == cols);
    if (x >= 0 && x < W) for (int y = y0; y < y0 + rows * CH && y < H; y++) fb[y * W + x] = 5;
  }
}

static void draw_plain(uint8_t *fb)
{
  memset(fb, 0, W * H);
  int x0, y0, cols, rows;
  screen_box(s_cur, &x0, &y0, &cols, &rows);
  grid_lines(fb, x0, y0, cols, rows);
  cells_at(fb, s_cur, x0, y0, cols, rows, 1, 2, false);
}

// Each row of cells as one line across the screen, stepped up by each cell's
// grey in whole pixels - no slope between cells, the steps are the cells - and
// each line hides what is behind it, down to its own base. A glyph rides its
// line, drawn as itself (Rutt and Etra's scan processor, on the deck's grid).
static void draw_scan(uint8_t *fb)
{
  memset(fb, 0, W * H);
  int x0, y0, cols, rows;
  screen_box(s_cur, &x0, &y0, &cols, &rows);
  grid_lines(fb, x0, y0, cols, rows);
  const int lift = set(6) ? par(6) * 48 / 127 : 30;
  const int w = s_cw[s_cur], h = s_ch[s_cur];
  const int cx0 = (w - cols) / 2, cy0 = (h - rows) / 2;
  for (int j = 0; j < rows; j++) {
    const int base = y0 + (j + 1) * CH - 1;
    int prev = -1;
    for (int i = 0; i < cols; i++) {
      const uint8_t c = s_cells[s_cur][cy0 + j][cx0 + i];
      const int y = base - (s_glyph[c] ? 0 : s_tone_of[c] * lift / 8);
      const int xa = x0 + i * CW;
      for (int X = xa; X < xa + CW && X < W; X++) {
        for (int Y = y + 1; Y <= base && Y < H; Y++) if (Y >= 0) fb[Y * W + X] = 0;
        if (y >= 0 && y < H) fb[y * W + X] = 1;
      }
      if (prev >= 0 && prev != y) {
        const int lo = prev < y ? prev : y, hi = prev < y ? y : prev;
        for (int Y = lo; Y <= hi; Y++) if (Y >= 0 && Y < H) fb[Y * W + xa] = 1;
      }
      prev = y;
      if (s_glyph[c]) cell(fb, xa, y - CH + 1, c, 1, 2, false);
    }
  }
}

// Two plates out of register: this step in the first ink, the step before last
// in the second, where they cross both, which the palette makes darker. The
// second plate drifts with the bar in whole pixels, as far as cc 7 says.
static void draw_riso(uint8_t *fb, uint32_t tick)
{
  memset(fb, 0, W * H);
  int x0, y0, cols, rows;
  screen_box(s_cur, &x0, &y0, &cols, &rows);
  const int old = (s_cur + 1) % 3;
  const int amt = set(6) ? par(6) * 8 / 127 : 4;
  const int ph = (int)(step_of(tick) % 16), tri = ph < 8 ? ph : 16 - ph;   // 0..8..0
  const int ddx = (tri - 4) * amt / 4, ddy = (amt + 1) / 2;
  cells_at(fb, s_cur, x0, y0, cols, rows, 1, 1, true);
  if (s_cw[old] == s_cw[s_cur] && s_ch[old] == s_ch[s_cur])
    cells_at(fb, old, x0 + ddx, y0 + ddy, cols, rows, 2, 2, true);
}

// Type turned a quarter anticlockwise, reading upward from (x, y): the spine.
static void glyph12up(uint8_t *fb, int x, int y, int ch, uint8_t c)
{
  if (ch < DECKFONT_FIRST || ch > DECKFONT_LAST) ch = ' ';
  const int g = ch - DECKFONT_FIRST;
  for (int r = 0; r < 24; r++) {
    const uint16_t bits = (deckfont_12x24[(g * 24 + r) * 2] << 8) | deckfont_12x24[(g * 24 + r) * 2 + 1];
    for (int k = 0; k < 12; k++) {
      const int X = x + r, Y = y - k;
      if ((bits & (0x8000 >> k)) && X >= 0 && X < W && Y >= 0 && Y < H) fb[Y * W + X] = c;
    }
  }
}

// Line i of the last control frame into a C string, cut at `max`; returns its length.
static int line_of(int i, char *out, int max, int *from, int *to)
{
  const uint8_t *ch;
  int n = view_read_line(&s_rd, i, &ch, from, to);
  if (n <= 0) { out[0] = 0; return n; }
  if (n > max) n = max;
  memcpy(out, ch, n);
  out[n] = 0;
  return n;
}

// THE POSTER, my brief (2026-09-29, and again 2026-09-30: "all
// clumped", the picture "this random rectangle in the top"). The picture is the
// page: full bleed, fourteen cells deep, the deck's own pixels. The section's
// number is printed over its lower left, huge and red. One hairline under it;
// the section's name and the bar's sixteen steps on it; then every lane, a
// whole pattern to a line in the small face, left to right, the step each is
// on lit red. The piece's name and its tempo are tags at the picture's top
// corners. One grid: 6 pixels across, and every line of type on it.
static void draw_poster(uint8_t *fb, uint32_t tick)
{
  (void)tick;
  memset(fb, 0, W * H);                                   // paper
  int from, to;
  char title[40], sect[40], meta[40], num[12] = "", name[40] = "";
  line_of(0, title, 30, &from, &to);
  if (!title[0]) strcpy(title, "cYbErDeCk");
  const int sn = line_of(1, sect, 39, &from, &to);
  line_of(2, meta, 30, &from, &to);
  {
    int sp = 0;
    while (sp < sn && sect[sp] != ' ') sp++;
    if (sp < sn && sp < 5) { memcpy(num, sect, sp); num[sp] = 0; strcpy(name, sect + sp + 1); }
    else strcpy(name, sect);
  }
  for (char *c = title; *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32;
  for (char *c = meta; *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32;
  for (char *c = name; *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32;
  const uint32_t step = step_of(s_rd.tick);

  cells_at(fb, s_cur, 1, 0, COLS, 14, 1, 2, false);      // the picture: the page
  const int nl = (int)strlen(num);                        // the number, over it
  if (nl) {
    const int sc = nl <= 2 ? 3 : 2;
    text12(fb, 6, 168 - 20 * sc, num, nl, sc, 2);
  }
  const int tl = (int)strlen(title);                      // the name: a tag, top left
  fill(fb, 0, 0, tl * 4 + 9, 11, 1);
  tiny_text(fb, 5, 3, title, tl, 0);
  char tag[48];                                           // tempo and bar: top right
  snprintf(tag, sizeof tag, "%s  BAR %lu", meta, (unsigned long)(step / 16 + 1));
  const int gl = (int)strlen(tag);
  fill(fb, W - gl * 4 - 9, 0, gl * 4 + 9, 11, 1);
  tiny_text(fb, W - gl * 4 - 4, 3, tag, gl, 0);

  fill(fb, 0, 168, W, 1, 1);                              // the hairline
  const int nn = (int)strlen(name) < 30 ? (int)strlen(name) : 30;
  if (nn) text6(fb, 6, 172, name, 1);
  for (int st = 0; st < 16; st++)                         // the bar, right
    fill(fb, W - 6 - (16 - st) * 8 + 2, 175, 6, 6,
         st == (int)(step % 16) ? 2 : (st % 4 == 0 ? 1 : 3));
  for (int i = 0; i < 7; i++) {                           // the lanes, whole
    const uint8_t *ch;
    const int n = view_read_line(&s_rd, 3 + i, &ch, &from, &to);
    if (n <= 0) break;
    const int y = 190 + i * 7;
    for (int k = 0; k < n && k < 78; k++) {
      const int x = 6 + k * 4;
      const bool lit = k >= from && k < to;
      if (lit) fill(fb, x - 1, y - 1, 4, 7, 2);
      tiny(fb, x, y, ch[k], lit ? 0 : 1);
    }
  }
}

// THE CODE, my ask (2026-09-29): a mode that is just the code, the way
// live coders put their screens up (TOPLAP: "show us your screens"). The lines
// round the cursor, in the deck's compact face, over the picture dimmed behind
// them: a section heading red, a line whose lane is playing bright with its step
// lit red, everything else grey. The deck sends the lines; it draws nothing.
static void draw_code(uint8_t *fb, uint32_t tick)
{
  (void)tick;
  memset(fb, 0, W * H);                                   // black, and only the code
  int from, to;
  char title[40], meta[40];
  line_of(0, title, 24, &from, &to);
  line_of(1, meta, 30, &from, &to);
  for (char *c = meta; *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32;
  const uint32_t step = step_of(s_rd.tick);
  char right[48];
  snprintf(right, sizeof right, "%s  BAR %lu  %2lu/16", meta, (unsigned long)(step / 16 + 1),
           (unsigned long)(step % 16 + 1));
  text12(fb, 6, 6, title, (int)strlen(title), 1, 2);
  text6(fb, W - 6 - 6 * (int)strlen(right), 12, right, 4);
  fill(fb, 6, 36, W - 12, 1, 3);
  for (int i = 0; i < 10; i++) {
    const uint8_t *ch;
    const int n = view_read_line(&s_rd, 2 + i, &ch, &from, &to);
    if (n < 0) break;
    const int y = 46 + i * 18;
    const bool head = n >= 2 && ch[0] == '-' && ch[1] == '-';
    const bool live = from < to;
    for (int k = 0; k < n && k < 51; k++) {
      const int x = 6 + k * 6;
      const bool lit = k >= from && k < to;
      if (lit) fill(fb, x, y, 6, 12, 3);
      glyph6(fb, x, y, ch[k], lit ? 0 : head ? 3 : live ? 2 : 4);
    }
  }
}

// LATENT, my ask (2026-09-30): the code "translated and animated and
// jumbled like encoded latent space". Every glyph of the code is drawn with
// its rows turned by a hash of where it is and of the beat, so the page is
// the code's own marks, scrambled - and where a lane is playing, its step is
// drawn plain, in red: the music decodes what it plays. The picture's tone
// under a letter says how far its rows are turned, so the texture moves with
// the music too. The same lines as the code; nothing is invented.
static void draw_latent(uint8_t *fb, uint32_t tick)
{
  memset(fb, 0, W * H);
  const uint32_t step = step_of(tick), beat = step / 4;
  int x0, y0, cols, rows;
  screen_box(s_cur, &x0, &y0, &cols, &rows);
  int from, to;
  for (int i = 0; i < 12; i++) {
    const uint8_t *ch;
    const int n = view_read_line(&s_rd, 2 + i, &ch, &from, &to);
    if (n < 0) break;
    const int y = 6 + i * 19;
    for (int k = 0; k < n && k < 51; k++) {
      const int x = 6 + k * 6;
      const uint8_t c = ch[k];
      if (c < DECKFONT_FIRST || c > DECKFONT_LAST || c == ' ') continue;
      const bool lit = k >= from && k < to;
      if (lit) { fill(fb, x, y, 6, 12, 3); glyph6(fb, x, y, c, 0); continue; }
      const int cx = (x - x0) / CW, cy = (y - y0) / CH;
      const int tone = (cx >= 0 && cx < cols && cy >= 0 && cy < rows)
                       ? s_tone_of[s_cells[s_cur][(s_ch[s_cur] - rows) / 2 + cy]
                                              [(s_cw[s_cur] - cols) / 2 + cx]] : 0;
      uint32_t h = (uint32_t)(k * 73856093u) ^ (uint32_t)(i * 19349663u) ^ (beat * 83492791u);
      const int g = c - DECKFONT_FIRST;
      for (int r = 0; r < 12; r++) {
        h = h * 1103515245u + 12345u;
        const int turn = (int)((h >> 16) % (1 + tone)) - (int)(tone / 2);
        const uint8_t bits = deckfont_6x12[g * 12 + ((r + (int)(h >> 29)) % 12)];
        for (int b = 0; b < 6; b++) {
          const int bb = ((b + turn) % 6 + 6) % 6;
          if (bits & (0x80 >> bb)) {
            const int X = x + b, Y = y + r;
            if (X >= 0 && X < W && Y >= 0 && Y < H) fb[Y * W + X] = (h >> 27) & 1 ? 2 : 4;
          }
        }
      }
    }
  }
}

// SORT, my ask for "a pixel sorter or something more glitched"
// (2026-09-30). The deck's pixels, cell for cell, then some rows sorted: every
// inked pixel in the row gathered to one end, so the row becomes a bar as long
// as the ink it held - the picture as its own histogram - with its tail fading
// down a ramp in the palette. Which rows sort is a hash of the row and the
// step, how many is cc 7, so it moves with the music and never twice alike.
static void draw_sort(uint8_t *fb, uint32_t tick)
{
  draw_plain(fb);
  const uint32_t step = step_of(tick);
  const int amt = set(6) ? par(6) : 48;
  for (int y = 0; y < H; y++) {
    const uint32_t h = ((uint32_t)(y / 3) * 2654435761u) ^ (step * 40503u) ^ (step >> 4) * 97u;
    if ((int)((h >> 24) & 127) >= amt) continue;
    uint8_t *r = fb + y * W;
    int n = 0;
    for (int x = 0; x < W; x++) n += (r[x] == 1 || r[x] == 2);
    if (n == 0) continue;
    const bool left = (h >> 9) & 1;
    for (int x = 0; x < W; x++) {
      const int d = left ? x : W - 1 - x;
      uint8_t v = 0;
      if (d < n) v = 1;
      else if (d < n + 16) v = (uint8_t)(6 + (d - n) / 2);
      r[x] = v;
    }
  }
}

// ---- the screen ------------------------------------------------------------

static const uint8_t LABEL_INK[MODES] = { 1, 1, 1, 1, 1, 1, 2 };

static void show(uint32_t tick)
{
  if (memcmp(s_par_seen, s_rd.params, sizeof s_par_seen) != 0) {
    memcpy(s_par_seen, s_rd.params, sizeof s_par_seen);
    s_pal_dirty = 2;
  }
  if (s_pal_dirty > 0) { palette(s_mode); s_pal_dirty--; }
  uint8_t *fb = display.getBuffer();
  switch (s_mode) {
  case SCAN:   draw_scan(fb);         break;
  case SORT:   draw_sort(fb, tick);   break;
  case LATENT: draw_latent(fb, tick); break;
  case RISO:   draw_riso(fb, tick);   break;
  case POSTER: draw_poster(fb, tick); break;
  case CODE:   draw_code(fb, tick);   break;
  default:     draw_plain(fb);        break;      // plain, and the two retired
  }
  if ((int32_t)(s_label_until - millis()) > 0) {
    fill(fb, 4, H - 18, 6 * (int)strlen(NAMES[s_mode]) + 4, 14, 0);
    text6(fb, 6, H - 17, NAMES[s_mode], LABEL_INK[s_mode]);
  }
  display.swap(false, false);
}

// Before the first frame, say what this is and what it is waiting for, in the
// deck's own face - a black screen is indistinguishable from a broken one.
static void show_waiting(void)
{
  for (int k = 0; k < 2; k++) {
    palette(PLAIN);
    uint8_t *fb = display.getBuffer();
    memset(fb, 0, W * H);
    text12(fb, (W - 14 * 12) / 2, 84, "cYbErDeCk view", 14, 1, 1);
    text6(fb, (W - 20 * 6) / 2, 126, "waiting for the deck", 1);
    display.swap(false, false);
  }
}

void setup()
{
  Serial.begin(115200);
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  tones_init();
  show_waiting();
}

void loop()
{
  // Drain what USB has, in one go, so a large frame is not held up behind the
  // per-byte overhead of Serial.read().
  static uint8_t buf[512];
  int n = Serial.available();
  while (n > 0) {
    const int k = Serial.readBytes(buf, n > (int)sizeof buf ? (int)sizeof buf : n);
    for (int i = 0; i < k; i++) {
      const int got = view_read_byte(&s_rd, buf[i]);
      if (got & VR_CONTROL) {
        const int m = s_rd.mode < MODES ? s_rd.mode : PLAIN;
        if (m != s_mode) {
          s_mode = m;
          s_pal_dirty = 2;
          s_label_until = millis() + 1500;
        }
      }
      if (got & VR_PICTURE) {
        keep_cells(s_rd.cells, s_rd.w, s_rd.h);
        show(s_rd.tick);
      }
    }
    n = Serial.available();
  }
  // A one-line account every two seconds, for whoever is listening: frames
  // drawn, frames refused, the last tick, the size and the mode.
  static uint32_t last;
  if (millis() - last > 2000) {
    last = millis();
    Serial.printf("view: %lu frames, %lu refused, tick %lu, %ux%u, %s, %u lines\r\n",
                  (unsigned long)s_rd.frames, (unsigned long)s_rd.refused,
                  (unsigned long)s_rd.tick, s_rd.w, s_rd.h, NAMES[s_mode], s_rd.nlines);
  }
}
