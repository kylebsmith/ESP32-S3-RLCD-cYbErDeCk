// deckview - the deck's picture on HDMI, from an Adafruit Feather RP2040 DVI.
//
// docs/NEXT.md §6: the visualization node, and docs/VIEW.md: its wire format.
// The deck sends a picture a step, and ahead of each a control frame naming
// how to draw it. The deck only names the mode; everything below happens here,
// so none of it costs the deck anything, and all of it is worked out from the
// frames and their ticks alone - the same performance draws the same pictures.
//
//   plain     the deck's picture, bit for bit, light on black
//   scan      each row drawn as one line, lifted by its greys (Rutt/Etra, 1972)
//   phosphor  a green tube: what the beam lit glows and fades over about a beat
//   feedback  the last frame, zoomed and turned, with the new one on top
//   riso      two inks out of register: pink is this step, blue the one before last
//   poster    a live Swiss poster of the piece, from the lines the deck sends
//   code      the code itself, round the cursor, over the picture dimmed
//
// THE DECK'S OWN GLYPHS STAY GLYPHS. The greys are the deck's dots; every cell
// the engine wrote as a glyph - the four sparkles noise scatters, the small disc,
// the arcs, any letter - is drawn as itself on top, in the deck's compact 6 x 12
// face, which on the screen is 12 x 24: the size the panel draws them. A sparkle
// turned into a grey is a speck of nothing, and they are half the picture's voice.
//
// 320 x 240, eight bits a pixel through a palette, doubled to 640 x 480 at 60 Hz
// - the mode every HDMI screen takes. Two framebuffers: the next picture is drawn
// while the last is shown, and phosphor and feedback read the one on screen.
// Mock-ups of each, from the engine's real frames: tools/mock_view.py.
//
// Frames arrive over USB serial. Today the deck's frames are relayed by a
// computer (tools/viewrelay.py); the same bytes are what the deck will send
// itself when it drives this board directly.
//
//   arduino-cli compile -b rp2040:rp2040:adafruit_feather_dvi view/deckview
//   arduino-cli upload  -b rp2040:rp2040:adafruit_feather_dvi -p <port> view/deckview

#include <PicoDVI.h>
#include <math.h>
#include "deckfont.h"
#include "view_read.h"

DVIGFX8 display(DVI_RES_320x240p60, true, adafruit_feather_dvi_cfg);

#define W 320
#define H 240

// The modes, numbered as the deck's firmware/main/view_wire.h numbers them.
enum { PLAIN, SCAN, PHOSPHOR, FEEDBACK, RISO, POSTER, CODE, MODES };
static const char *const NAMES[MODES] = { "plain", "scan", "phosphor", "feedback",
                                          "riso", "poster", "code" };

static view_reader_t s_rd;
static uint8_t *s_base;                 // the first of the two framebuffers
static int s_mode = PLAIN, s_pal_dirty = 2;
static uint32_t s_label_until;          // show the mode's name until then

static const uint8_t BAYER[4][4] = {
  { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };

// ---- the picture as greys ------------------------------------------------
//
// A cell is twice as tall as it is wide, so it is two square dots, one above
// the other: 80 x 30 cells are 80 x 60 dots, a dot 4 x 4 pixels.

#define DW_MAX 80
#define DH_MAX 60
static uint8_t s_tone_of[256];
static bool    s_glyph[256];                  // drawn as itself, not as a grey
static uint8_t s_hist[3][DH_MAX][DW_MAX];    // this step and the two before
static uint8_t s_code[3][DH_MAX / 2][DW_MAX]; // and their cells, for the glyphs
static uint8_t s_hdw[3], s_hdh[3];
static int s_cur;
static int16_t s_cor[DH_MAX + 1][DW_MAX + 1];  // greys at dot corners, 0..256
static uint8_t s_ink[H][W / 8];                // this step, banded, bit for bit
// where each screen column and row falls in the grid
static int16_t s_cdot[W], s_rdot[H];
static uint8_t s_cfrac[W], s_rfrac[H];
static int s_dw, s_dh, s_s, s_ox, s_oy;

// A tile's grey: the tones are 128..136; the small disc, 147, is solid; any
// other glyph is as grey as it is inked.
static void tones_init(void)
{
  for (int c = 0; c < 256; c++) {
    int ink = 0;
    if (c >= DECKFONT_FIRST && c <= DECKFONT_LAST) {
      const int g = c - DECKFONT_FIRST;
      for (int r = 0; r < 24; r++) {
        uint16_t bits = (deckfont_12x24[(g * 24 + r) * 2] << 8) | deckfont_12x24[(g * 24 + r) * 2 + 1];
        while (bits) { ink += bits & 1; bits >>= 1; }
      }
    }
    s_tone_of[c] = (uint8_t)((ink * 8 + 144) / 288);
  }
  for (int t = 0; t <= 8; t++) s_tone_of[128 + t] = (uint8_t)t;
  s_tone_of[' '] = 0;
  // a glyph is drawn as itself, so it is no grey in the field under it
  for (int c = 0; c < 256; c++) {
    s_glyph[c] = (c > ' ' && c < 127) || (c >= 137 && c <= DECKFONT_LAST);
    if (s_glyph[c]) s_tone_of[c] = 0;
  }
}

static void grid_from_cells(const uint8_t *cells, int w, int h)
{
  const int dw = w < DW_MAX ? w : DW_MAX;
  const int dh = 2 * h < DH_MAX ? 2 * h : DH_MAX;
  s_cur = (s_cur + 1) % 3;
  for (int j = 0; j < dh; j++) {
    const int cy = (j * 2 * h / dh) / 2;
    for (int i = 0; i < dw; i++) {
      const uint8_t c = cells[cy * w + i * w / dw];
      s_hist[s_cur][j][i] = s_tone_of[c];
      if ((j & 1) == 0) s_code[s_cur][j / 2][i] = c;
    }
  }
  s_hdw[s_cur] = (uint8_t)dw;
  s_hdh[s_cur] = (uint8_t)dh;
}

// Place a dw x dh grid of s-pixel dots at (ox, oy), and tabulate where every
// column and row lands, so nothing below divides per pixel.
static void place(int dw, int dh, int s, int ox, int oy)
{
  s_dw = dw; s_dh = dh; s_s = s; s_ox = ox; s_oy = oy;
  for (int x = 0; x < W; x++) {
    const int g = x - ox;
    if (g < 0 || g >= dw * s) { s_cdot[x] = -1; continue; }
    s_cdot[x] = (int16_t)(g / s);
    s_cfrac[x] = (uint8_t)(((2 * (g % s) + 1) * 256) / (2 * s));
  }
  for (int y = 0; y < H; y++) {
    const int g = y - oy;
    if (g < 0 || g >= dh * s) { s_rdot[y] = -1; continue; }
    s_rdot[y] = (int16_t)(g / s);
    s_rfrac[y] = (uint8_t)(((2 * (g % s) + 1) * 256) / (2 * s));
  }
}

static void place_fit(int dw, int dh)
{
  int s = W / dw < H / dh ? W / dw : H / dh;
  if (s < 1) s = 1;
  place(dw, dh, s, (W - dw * s) / 2, (H - dh * s) / 2);
}

// The greys at the dots' corners: each the mean of the dots that meet there.
static void corners(const uint8_t *g, int pitch, int dw, int dh)
{
  for (int j = 0; j <= dh; j++) {
    for (int i = 0; i <= dw; i++) {
      int sum = 0, n = 0;
      for (int k = 0; k < 4; k++) {
        const int x = i - 1 + (k & 1), y = j - 1 + (k >> 1);
        if (x >= 0 && x < dw && y >= 0 && y < dh) { sum += g[y * pitch + x]; n++; }
      }
      s_cor[j][i] = (int16_t)((sum * 32 + n / 2) / n);
    }
  }
}

// The grey a pixel was meant to have, 0..256, between its dot's corners.
static inline int grey_at(int dx, int dy, int fx, int fy)
{
  const int a = s_cor[dy][dx], b = s_cor[dy][dx + 1];
  const int d = s_cor[dy + 1][dx], e = s_cor[dy + 1][dx + 1];
  const int left = a * 256 + (d - a) * fy, right = b * 256 + (e - b) * fy;
  return (left * 256 + (right - left) * fx) >> 16;
}

// THE DECK'S OWN RULE (docs/wiki/pictures-and-type.md): a grey dot is banded
// between its corners and cut into the nine tones; a solid or an empty dot is
// exactly what it is. Bayer, at the pixel's place in the grid.
static inline bool banded(const uint8_t *g, int pitch, int x, int y)
{
  if (x < 0 || x >= W || y < 0 || y >= H) return false;
  const int dx = s_cdot[x], dy = s_rdot[y];
  if (dx < 0 || dy < 0) return false;
  const int t = g[dy * pitch + dx];
  if (t == 0) return false;
  if (t >= 8) return true;
  int lv = (grey_at(dx, dy, s_cfrac[x], s_rfrac[y]) + 16) >> 5;
  if (lv < 1) lv = 1;
  if (lv > 7) lv = 7;
  return BAYER[(y - s_oy) & 3][(x - s_ox) & 3] < 2 * lv;
}

static inline bool ink(int x, int y) { return s_ink[y][x >> 3] & (0x80 >> (x & 7)); }

static void ink_this_step(void)
{
  const uint8_t *g = &s_hist[s_cur][0][0];
  corners(g, DW_MAX, s_dw, s_dh);
  memset(s_ink, 0, sizeof s_ink);
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
      if (banded(g, DW_MAX, x, y)) s_ink[y][x >> 3] |= 0x80 >> (x & 7);
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

static void fill(uint8_t *fb, int x, int y, int w, int h, uint8_t c)
{
  for (int Y = y; Y < y + h; Y++)
    if (Y >= 0 && Y < H)
      for (int X = x; X < x + w; X++)
        if (X >= 0 && X < W) fb[Y * W + X] = c;
}

// ---- the deck's glyphs, drawn as themselves ---------------------------------
//
// Each glyph cell's 6 x 12 glyph, centred on the cell. `how` is the mode's ink:
// a palette index to set, or (riso) a plate bit to add; phosphor sets its full
// glow on the line's own half of the palette. In scan a glyph sits on the line of
// its row, since the rows are lines there.

enum { SET, PLATE, GLOW };

static void glyphs(uint8_t *fb, int slot, int how, uint8_t ink, int ddx, int ddy)
{
  const int dw = s_hdw[slot], rows = s_hdh[slot] / 2;
  const int lines = rows > 0 ? rows : 1;
  for (int cy = 0; cy < rows; cy++) {
    for (int cx = 0; cx < dw; cx++) {
      const uint8_t c = s_code[slot][cy][cx];
      if (!s_glyph[c]) continue;
      int x0 = s_ox + cx * s_s + s_s / 2 - 3 + ddx;
      int y0 = s_oy + cy * 2 * s_s + s_s - 6 + ddy;
      if (s_mode == SCAN) y0 = 44 + ((cy + 1) * (H - 10 - 44)) / lines - 12;
      const int g = c - DECKFONT_FIRST;
      for (int r = 0; r < 12; r++) {
        const uint8_t bits = deckfont_6x12[g * 12 + r];
        const int Y = y0 + r;
        if (!bits || Y < 0 || Y >= H) continue;
        for (int k = 0; k < 6; k++) {
          const int X = x0 + k;
          if (!(bits & (0x80 >> k)) || X < 0 || X >= W) continue;
          uint8_t *p = &fb[Y * W + X];
          if (how == PLATE) *p |= ink;
          else if (how == GLOW) *p = (uint8_t)(127 | ((Y & 1) << 7));
          else *p = ink;
        }
      }
    }
  }
}

// ---- palettes ------------------------------------------------------------

static uint16_t rgb(int r, int g, int b)
{
  r = r < 0 ? 0 : r > 255 ? 255 : r;
  g = g < 0 ? 0 : g > 255 ? 255 : g;
  b = b < 0 ? 0 : b > 255 ? 255 : b;
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static void palette(int mode)
{
  uint16_t *p = display.getPalette();
  for (int i = 0; i < 256; i++) p[i] = 0;
  switch (mode) {
  case PLAIN: case SCAN:
    p[1] = rgb(236, 236, 228);
    break;
  case PHOSPHOR:                          // a green tube, and its dimmer odd lines
    for (int i = 0; i < 128; i++) {
      const float v = i / 127.0f;
      const int r = (int)(170 * powf(v, 2.6f)), g = (int)(255 * fminf(1.0f, 1.08f * powf(v, 0.75f)));
      const int b = (int)(120 * powf(v, 2.2f));
      p[i] = rgb(r, g, b);
      p[128 + i] = rgb(r * 45 / 100, g * 45 / 100, b * 45 / 100);
    }
    break;
  case FEEDBACK:                          // black, through red and orange, to white
    for (int i = 0; i < 256; i++) {
      const float v = i / 255.0f;
      p[i] = rgb((int)(255 * fminf(1.0f, 1.6f * v)), (int)(255 * fmaxf(0.0f, v - 0.55f) * 2.0f),
                 (int)(255 * fmaxf(0.0f, v - 0.8f) * 4.0f));
    }
    break;
  case RISO:                              // paper, pink, blue, and both
    p[0] = rgb(244, 240, 229);
    p[1] = rgb(255, 72, 176);
    p[2] = rgb(0, 120, 191);
    p[3] = rgb(0, 34, 133);
    break;
  case POSTER:                            // paper, ink, a red, and a grey
    p[0] = rgb(246, 244, 238);
    p[1] = rgb(18, 18, 18);
    p[2] = rgb(228, 0, 43);
    p[3] = rgb(150, 148, 142);
    break;
  case CODE:                              // black, the picture dimmed, text, red, grey
    p[0] = rgb(0, 0, 0);
    p[1] = rgb(58, 58, 54);
    p[2] = rgb(236, 236, 228);
    p[3] = rgb(228, 0, 43);
    p[4] = rgb(128, 128, 122);
    break;
  }
}

// ---- the modes -----------------------------------------------------------

static uint32_t step_of(uint32_t tick) { return tick / 24; }   // 96 to the beat

static void draw_plain(uint8_t *fb)
{
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) fb[y * W + x] = ink(x, y) ? 1 : 0;
}

// Each row of cells as one line across the screen, lifted by its greys; each
// line hides what is behind it. A line only ever rises from its own base, and
// the lines behind it sit on higher bases, so hiding them means clearing from
// the line down to its base and no further.
static void draw_scan(uint8_t *fb)
{
  memset(fb, 0, W * H);
  const int lines = s_dh / 2 > 0 ? s_dh / 2 : 1;
  const int top = 44, bottom = H - 10, pad = 16, lift = 38;
  const int span = W - 2 * pad;
  static int16_t ys[W];
  for (int k = 0; k < lines; k++) {
    const uint8_t *row = s_hist[s_cur][k * 2 < s_dh ? k * 2 : s_dh - 1];
    const int base = top + ((k + 1) * (bottom - top)) / lines;
    for (int x = pad; x < W - pad; x++) {
      int pos = (((x - pad) * 2 + 1) * s_dw * 256) / (2 * span) - 128;
      if (pos < 0) pos = 0;
      int u0 = pos >> 8, f = pos & 255;
      if (u0 >= s_dw - 1) { u0 = s_dw - 1; f = 0; }
      const int u1 = u0 + 1 < s_dw ? u0 + 1 : u0;
      const int t = row[u0] * (256 - f) + row[u1] * f;          // 0..2048
      ys[x] = (int16_t)(base - (lift * t) / 2048);
    }
    for (int x = pad; x < W - pad; x++)
      for (int y = ys[x] + 1; y <= base && y < H; y++) fb[y * W + x] = 0;
    for (int x = pad; x < W - pad - 1; x++) {
      const int a = ys[x] < ys[x + 1] ? ys[x] : ys[x + 1];
      const int b = ys[x] < ys[x + 1] ? ys[x + 1] : ys[x];
      for (int y = a; y <= b; y++) if (y >= 0 && y < H) fb[y * W + x] = 1;
    }
  }
}

// What the beam lit glows, and fades to about half each step; odd lines are
// the dim half of the palette.
static void draw_phosphor(uint8_t *fb, const uint8_t *front)
{
  for (int y = 0; y < H; y++) {
    const uint8_t odd = (y & 1) << 7;
    for (int x = 0; x < W; x++) {
      int b = front[y * W + x] & 127;
      b = (b * 70) >> 7;
      if (ink(x, y)) b = 127;
      fb[y * W + x] = (uint8_t)(b | odd);
    }
  }
}

// The frame on screen, zoomed and turned about the centre and a little dimmer,
// with the new picture's greys on top. A full turn every two bars; a zoom that
// kicks on each beat. It feeds back greys, not dots: a dither turned by a few
// degrees is noise.
static void draw_feedback(uint8_t *fb, const uint8_t *front, uint32_t tick)
{
  const float z = (step_of(tick) % 4 == 0) ? 1.10f : 1.035f;
  const float th = 2.0f * (float)M_PI / 32.0f;
  const int C = (int)(cosf(th) / z * 65536.0f), S = (int)(sinf(th) / z * 65536.0f);
  const int cx2 = W - 1, cy2 = H - 1;                 // twice the centre
  for (int y = 0; y < H; y++) {
    // source = centre + R * (p - centre), in 16.16
    const int ry = y * 2 - cy2, rx = -cx2;           // doubled offsets from the centre
    int sx = (cx2 << 15) + (C * rx + S * ry) / 2;
    int sy = (cy2 << 15) + (-S * rx + C * ry) / 2;
    const int dy = s_rdot[y];
    for (int x = 0; x < W; x++, sx += C, sy -= S) {
      int X = (sx + 32768) >> 16, Y = (sy + 32768) >> 16;
      X = X < 0 ? 0 : X >= W ? W - 1 : X;
      Y = Y < 0 ? 0 : Y >= H ? H - 1 : Y;
      int v = (front[Y * W + X] * 215) >> 8;
      const int dx = s_cdot[x];
      if (dx >= 0 && dy >= 0) {
        const int n = (grey_at(dx, dy, s_cfrac[x], s_rfrac[y]) * 255) >> 8;
        if (n > v) v = n;
      }
      fb[y * W + x] = (uint8_t)v;
    }
  }
}

// Pink is this step; blue is the step before last, its plate drifting with the
// bar - and a plate carries its own screen with it.
static void draw_riso(uint8_t *fb, uint32_t tick)
{
  const int old = (s_cur + 1) % 3;
  const uint8_t *g = &s_hist[old][0][0];
  corners(g, DW_MAX, s_hdw[old], s_hdh[old]);
  const int step = (int)step_of(tick);
  const int ddx = (int)lroundf(4.0f * sinf(2.0f * (float)M_PI * (step % 16) / 16.0f)), ddy = 3;
  const bool have = s_hdw[old] == s_dw && s_hdh[old] == s_dh;
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) {
      const uint8_t a = ink(x, y) ? 1 : 0;
      const uint8_t b = (have && banded(g, DW_MAX, x - ddx, y - ddy)) ? 2 : 0;
      fb[y * W + x] = a | b;
    }
  glyphs(fb, s_cur, PLATE, 1, 0, 0);
  if (have) glyphs(fb, old, PLATE, 2, ddx, ddy);
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

// This step's picture resampled to dw x dh dots of 4 pixels at (x0, y0), in `ink`.
static void picture_at(uint8_t *fb, int dw, int dh, int x0, int y0, int x1, int y1, uint8_t ink)
{
  static uint8_t pic[DH_MAX][DW_MAX];
  for (int j = 0; j < dh; j++)
    for (int i = 0; i < dw; i++)
      pic[j][i] = s_hist[s_cur][j * s_dh / dh][i * s_dw / dw];
  const int dw0 = s_dw, dh0 = s_dh, s0 = s_s, ox0 = s_ox, oy0 = s_oy;
  place(dw, dh, 4, x0, y0);
  corners(&pic[0][0], DW_MAX, dw, dh);
  for (int y = y0 < 0 ? 0 : y0; y < y1 && y < H; y++)
    for (int x = x0 < 0 ? 0 : x0; x < x1 && x < W; x++)
      if (banded(&pic[0][0], DW_MAX, x, y)) fb[y * W + x] = ink;
  place(dw0, dh0, s0, ox0, oy0);
}

// THE POSTER, the owner's brief (2026-09-29): the title and the tempo took half
// the screen, and it should be far more Swiss punk. So the picture takes the
// page, bleeding off the top and the right; the section's number is huge, red,
// printed over it; the piece's name runs up a black spine; the section's name is
// reversed out of a black bar; tempo, key, bar and step are set small and tight,
// over sixteen blocks for the bar; and the lanes stand in a column, the step
// each is on lit red.
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
  for (char *c = meta; *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32;
  const uint32_t step = step_of(s_rd.tick);

  picture_at(fb, 56, 37, W - 224, 0, W, 148, 1);          // bleeds top and right
  fill(fb, 0, 0, 22, H, 1);                               // the spine
  for (int i = 0; title[i] && 8 + i * 12 < H; i++) glyph12up(fb, 5, H - 8 - i * 12, (uint8_t)title[i], 0);
  const int nl = (int)strlen(num);
  if (nl) text12(fb, 28, 4, num, nl, nl <= 3 ? 5 : 4, 2); // over the picture
  const int nn = (int)strlen(name) < 11 ? (int)strlen(name) : 11;
  if (nn) {
    fill(fb, 28, 156, nn * 12 + 10, 26, 1);
    text12(fb, 33, 157, name, nn, 1, 0);
  }
  text6(fb, 28, 190, meta, 1);
  char bar[24];
  snprintf(bar, sizeof bar, "BAR %lu  %2lu/16", (unsigned long)(step / 16 + 1),
           (unsigned long)(step % 16 + 1));
  text6(fb, 28, 203, bar, 1);
  for (int st = 0; st < 16; st++)
    fill(fb, 28 + st * 9, 220, 7, 14, st == (int)(step % 16) ? 2 : (st % 4 == 0 ? 1 : 3));
  for (int i = 0; i < 7; i++) {
    const uint8_t *ch;
    const int n = view_read_line(&s_rd, 3 + i, &ch, &from, &to);
    if (n <= 0) break;
    const int y = 154 + i * 12;
    for (int k = 0; k < n && k < 23; k++) {
      const int x = W - 142 + k * 6;
      const bool lit = k >= from && k < to;
      if (lit) fill(fb, x, y, 6, 11, 2);
      glyph6(fb, x, y - 1, ch[k], lit ? 0 : 1);
    }
  }
}

// THE CODE, the owner's ask (2026-09-29): a mode that is just the code, the way
// live coders put their screens up (TOPLAP: "show us your screens"). The lines
// round the cursor, in the deck's compact face, over the picture dimmed behind
// them: a section heading red, a line whose lane is playing bright with its step
// lit red, everything else grey. The deck sends the lines; it draws nothing.
static void draw_code(uint8_t *fb, uint32_t tick)
{
  (void)tick;
  memset(fb, 0, W * H);                                   // black
  picture_at(fb, 80, 60, 0, 0, W, H, 1);                  // the picture, dim
  int from, to;
  char title[40], meta[40];
  line_of(0, title, 24, &from, &to);
  line_of(1, meta, 30, &from, &to);
  for (char *c = meta; *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32;
  const uint32_t step = step_of(s_rd.tick);
  char right[48];
  snprintf(right, sizeof right, "%s  BAR %lu  %2lu/16", meta, (unsigned long)(step / 16 + 1),
           (unsigned long)(step % 16 + 1));
  text12(fb, 12, 6, title, (int)strlen(title), 1, 2);
  text6(fb, W - 12 - 6 * (int)strlen(right), 12, right, 4);
  fill(fb, 12, 36, W - 24, 2, 3);
  for (int i = 0; i < 10; i++) {
    const uint8_t *ch;
    const int n = view_read_line(&s_rd, 2 + i, &ch, &from, &to);
    if (n < 0) break;
    const int y = 46 + i * 18;
    const bool head = n >= 2 && ch[0] == '-' && ch[1] == '-';
    const bool live = from < to;
    for (int k = 0; k < n && k < 49; k++) {
      const int x = 12 + k * 6;
      const bool lit = k >= from && k < to;
      if (lit) fill(fb, x, y, 6, 12, 3);
      glyph6(fb, x, y, ch[k], lit ? 0 : head ? 3 : live ? 2 : 4);
    }
  }
}

// ---- the screen ------------------------------------------------------------

static const uint8_t LABEL_INK[MODES] = { 1, 1, 127, 255, 3, 2, 2 };

static void show(uint32_t tick)
{
  if (s_pal_dirty > 0) { palette(s_mode); s_pal_dirty--; }
  uint8_t *fb = display.getBuffer();
  const uint8_t *front = (fb == s_base) ? s_base + W * H : s_base;
  place_fit(s_dw, s_dh);
  ink_this_step();
  switch (s_mode) {
  case SCAN:     draw_scan(fb);                  glyphs(fb, s_cur, SET, 1, 0, 0);   break;
  case PHOSPHOR: draw_phosphor(fb, front);       glyphs(fb, s_cur, GLOW, 0, 0, 0);  break;
  case FEEDBACK: draw_feedback(fb, front, tick); glyphs(fb, s_cur, SET, 255, 0, 0); break;
  case RISO:     draw_riso(fb, tick);            break;             // both plates, inside
  case POSTER:   draw_poster(fb, tick);          break;
  case CODE:     draw_code(fb, tick);            break;
  default:       draw_plain(fb);                 glyphs(fb, s_cur, SET, 1, 0, 0);   break;
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
  s_base = display.getBuffer();
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
        grid_from_cells(s_rd.cells, s_rd.w, s_rd.h);
        s_dw = s_hdw[s_cur];
        s_dh = s_hdh[s_cur];
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
