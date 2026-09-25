// deckview - the deck's picture on HDMI, from an Adafruit Feather RP2040 DVI.
//
// docs/NEXT.md §6: the visualization node. viz.c left the wire format unwritten
// "because a format invented before its reader is a format nobody implements";
// this is the reader, and docs/VIEW.md is the format.
//
// It draws the deck's own glyphs - deckfont.h is generated from the same art as
// the panel's faces (tools/make_font.py --view) - one bit per pixel, because the
// deck's picture is one ink and a screen that added colour would be showing
// something the performer never made. Each frame is drawn at the largest whole
// scale that fits 640x480, with whichever of the two faces fills it best, and
// centred: a cell is always a square block of the tile, never smeared.
//
// Frames arrive over USB serial. Today the deck's frames are relayed by a
// computer (tools/viewrelay.py); the same bytes are what the deck will send
// itself when it drives this board directly.
//
//   arduino-cli compile -b rp2040:rp2040:adafruit_feather_dvi view/deckview
//   arduino-cli upload  -b rp2040:rp2040:adafruit_feather_dvi -p <port> view/deckview

#include <PicoDVI.h>
#include "deckfont.h"

DVIGFX1 display(DVI_RES_640x480p60, true, adafruit_feather_dvi_cfg);

#define SCREEN_W 640
#define SCREEN_H 480
#define ROW_BYTES (SCREEN_W / 8)

// ---- the wire format: docs/VIEW.md, read by view_read.h ------------------
#include "view_read.h"
static view_reader_t s_rd;

// ---- drawing ------------------------------------------------------------

static inline void set_run(uint8_t *row, int x, int n)
{
  for (int i = 0; i < n; i++, x++) {
    row[x >> 3] |= (uint8_t)(0x80 >> (x & 7));
  }
}

// One frame of cells into the back buffer, in face `big` (12x24) or not
// (6x12), `s` screen pixels to a glyph pixel, top-left at (x0, y0).
static void draw_cells(const uint8_t *cells, int w, int h, bool big, int s,
                       int x0, int y0)
{
  uint8_t *fb = display.getBuffer();
  const int fw = big ? 12 : 6, fh = big ? 24 : 12;
  for (int cy = 0; cy < h; cy++) {
    for (int gr = 0; gr < fh; gr++) {
      for (int sy = 0; sy < s; sy++) {
        const int y = y0 + (cy * fh + gr) * s + sy;
        if (y < 0 || y >= SCREEN_H) continue;
        uint8_t *row = fb + y * ROW_BYTES;
        for (int cx = 0; cx < w; cx++) {
          int code = cells[cy * w + cx];
          if (code < DECKFONT_FIRST || code > DECKFONT_LAST) code = ' ';
          const int g = code - DECKFONT_FIRST;
          uint16_t bits;
          if (big) {
            bits = (uint16_t)((deckfont_12x24[(g * 24 + gr) * 2] << 8) |
                              deckfont_12x24[(g * 24 + gr) * 2 + 1]);
          } else {
            bits = (uint16_t)(deckfont_6x12[g * 12 + gr] << 8);
          }
          if (bits == 0) continue;
          const int xb = x0 + cx * fw * s;
          for (int gc = 0; gc < fw; gc++) {
            if (bits & (0x8000 >> gc)) {
              const int x = xb + gc * s;
              if (x >= 0 && x + s <= SCREEN_W) set_run(row, x, s);
            }
          }
        }
      }
    }
  }
}

// THE LARGEST WHOLE SCALE THAT FITS, of either face. Whole, because a cell is
// a tile and a tile scaled by 1.7 is not the tile any more; either face,
// because a small frame at the big face's scale 1 wastes most of the screen
// that the small face at scale 3 fills.
static void layout(int w, int h, bool *big, int *s)
{
  int best = 0;
  *big = true;
  *s = 1;
  for (int f = 0; f < 2; f++) {
    const int fw = f == 0 ? 12 : 6, fh = f == 0 ? 24 : 12;
    for (int k = 8; k >= 1; k--) {
      if (w * fw * k <= SCREEN_W && h * fh * k <= SCREEN_H) {
        const int area = w * fw * k * h * fh * k;
        if (area > best) { best = area; *big = (f == 0); *s = k; }
        break;
      }
    }
  }
}

static void show_frame(const uint8_t *cells, int w, int h)
{
  memset(display.getBuffer(), 0, ROW_BYTES * SCREEN_H);
  bool big;
  int s;
  layout(w, h, &big, &s);
  const int fw = big ? 12 : 6, fh = big ? 24 : 12;
  const int x0 = (SCREEN_W - w * fw * s) / 2;
  const int y0 = (SCREEN_H - h * fh * s) / 2;
  draw_cells(cells, w, h, big, s, x0, y0);
  display.swap();
}

// Before the first frame, say what this is and what it is waiting for, in the
// deck's own face - a black screen is indistinguishable from a broken one.
static void show_waiting(void)
{
  static const char *lines[] = { "cYbErDeCk view", "", "waiting for", "the deck" };
  static uint8_t cells[4 * 16];
  memset(cells, ' ', sizeof cells);
  for (int r = 0; r < 4; r++) {
    for (int c = 0; lines[r][c] && c < 16; c++) cells[r * 16 + c] = lines[r][c];
  }
  show_frame(cells, 16, 4);
}

void setup()
{
  Serial.begin(115200);
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
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
      if (view_read_byte(&s_rd, buf[i])) show_frame(s_rd.cells, s_rd.w, s_rd.h);
    }
    n = Serial.available();
  }
  // A one-line account every two seconds, for whoever is listening: frames
  // drawn, frames refused, and the last tick - which is how the far end knows
  // this node is showing the picture it sent and when it belonged.
  static uint32_t last;
  if (millis() - last > 2000) {
    last = millis();
    Serial.printf("view: %lu frames, %lu refused, tick %lu, %ux%u\r\n",
                  (unsigned long)s_rd.frames, (unsigned long)s_rd.refused,
                  (unsigned long)s_rd.tick, s_rd.w, s_rd.h);
  }
}
