#!/usr/bin/env python3
"""mock_view.py OUT_DIR - what the view node could do with the deck's frames.

A MOCK-UP. The deck keeps Bayer and sends the view node what it sends today: a
frame a step, and the tick it belongs to. Everything below happens on the node.

Every frame is drawn by the deck's own picture engine, viz.c as flashed, at the
80 x 30 cells the deck sends, playing ORBITALS' night - the radar and >noise 1 -
and each cell is two square dots, as on the node (view/deckview). The greys are
the deck's dots; every cell the engine wrote as a glyph - the sparkles, the
small disc - is drawn as itself on top, in the deck's 6 x 12 face, as the node
draws it. Each mode uses only what an RP2040 on PicoDVI has, and each is a
function of the frames and their ticks and of nothing else, so the same
performance draws the same pictures every time.

    view-modes.png    the six modes, the same step of the radar in each
    view-motion.gif   four of them moving through the second bar, a frame a step

    python3 tools/mock_view.py docs/img        # needs a C compiler, Pillow, numpy
"""
import math, os, subprocess, sys, tempfile

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mock_pictures as M     # noqa: E402
import type_round as T        # noqa: E402
from zine import SMALL, Page  # noqa: E402

W, H = 640, 480
STEPS = 32                    # the night, a sixteenth a step
STILL = 20                    # the step every still shows


# ---------------------------------------------------------------- the deck's frames

def frames(tmp, w, h, square=True, scene='radar'):
    exe = M.build(tmp, square)
    d = os.path.join(tmp, f'm{w}x{h}{"s" if square else "r"}')
    os.makedirs(d)
    subprocess.run([exe, d, str(w), str(h), 'motion'], check=True)
    return [M.load(d, f'{scene}{k:02d}', w, h) for k in range(STEPS)]


def is_glyph(c):
    """Drawn as itself on the node: a letter, or a tile past the nine tones."""
    return (32 < c < 127) or (137 <= c <= 155)


def dots(cells):
    """The node's grid: each cell two square dots, one above the other; a glyph
    cell is no grey in it, because the glyph is drawn on top."""
    return [[128 if is_glyph(c) else c for c in row] for row in cells for _ in (0, 1)]


def sprites(cells, scan=False):
    """The glyph layer, 320 x 240: each glyph cell's 6 x 12 glyph from the deck's
    compact face, centred on the cell - or, in scan, sitting on its row's line."""
    out = np.zeros((240, 320), bool)
    rows = len(cells)
    for cy, row in enumerate(cells):
        for cx, c in enumerate(row):
            if not is_glyph(c):
                continue
            x0, y0 = cx * 4 + 2 - 3, cy * 8 + 4 - 6
            if scan:
                y0 = 44 + ((cy + 1) * (240 - 10 - 44)) // rows - 12
            for r, bits in enumerate(SMALL.g[c]):
                for k in range(6):
                    X, Y = x0 + k, y0 + r
                    if bits & (0x8000 >> k) and 0 <= X < 320 and 0 <= Y < 240:
                        out[Y, X] = True
    return out


def bayer(g):
    """The deck's own picture, bit for bit: 4 x 4 dots, banded, Bayer."""
    p = Page(len(g[0]) * 4, len(g) * 4)
    M.draw_banded(p, 0, 0, g, 4)
    return np.array(p.im, dtype=bool) == False  # noqa: E712 - ink is 0 in the page


def tones(g):
    return np.array([[M.tone(c) for c in row] for row in g], float) / 8.0


def smooth(g, w=320, h=240):
    """The frame's greys as greys, bilinear across the screen - a screen that
    can show grey has no need to dither."""
    t = tones(g)
    rows, cols = t.shape
    ys = np.clip((np.arange(h) + 0.5) / h * rows - 0.5, 0, rows - 1)
    xs = np.clip((np.arange(w) + 0.5) / w * cols - 0.5, 0, cols - 1)
    y0, x0 = np.floor(ys).astype(int), np.floor(xs).astype(int)
    y1, x1 = np.minimum(y0 + 1, rows - 1), np.minimum(x0 + 1, cols - 1)
    fy, fx = (ys - y0)[:, None], (xs - x0)[None, :]
    top = t[y0][:, x0] * (1 - fx) + t[y0][:, x1] * fx
    bot = t[y1][:, x0] * (1 - fx) + t[y1][:, x1] * fx
    return top * (1 - fy) + bot * fy


def up(a, n=2):
    return np.repeat(np.repeat(a, n, axis=0), n, axis=1)


def rgb(mask, ink, paper):
    out = np.empty(mask.shape + (3,), np.uint8)
    out[:] = paper
    out[mask] = ink
    return out


# ---------------------------------------------------------------- the modes

def plain(fr):
    """The deck's picture, light on black: its greys as dots, its glyphs as
    glyphs."""
    for g in fr:
        yield rgb(up(bayer(dots(g)) | sprites(g)), (236, 236, 228), (0, 0, 0))


def scan(fr):
    """Rutt and Etra's scan processor, 1972, and the pulsar plot Joy Division
    put on a record sleeve, 1979: each row of cells drawn as one line, lifted by
    its greys, each line hiding what is behind it; the glyphs sit on the lines.
    As the node draws it, at 320 x 240."""
    top, bottom, lift, pad = 44, 240 - 10, 38, 16
    span = 320 - 2 * pad
    for g in fr:
        t = np.array([[M.tone(c) for c in row] for row in dots(g)], float)
        rows = len(g)
        cols = t.shape[1]
        img = np.zeros((240, 320), bool)
        for k in range(rows):
            row = t[2 * k]
            base = top + ((k + 1) * (bottom - top)) // rows
            ys = np.zeros(320, int)
            for x in range(pad, 320 - pad):
                pos = (((x - pad) * 2 + 1) * cols * 256) // (2 * span) - 128
                pos = max(pos, 0)
                u0, f = pos >> 8, pos & 255
                if u0 >= cols - 1:
                    u0, f = cols - 1, 0
                u1 = min(u0 + 1, cols - 1)
                tt = row[u0] * (256 - f) + row[u1] * f
                ys[x] = base - int(lift * tt) // 2048
            for x in range(pad, 320 - pad):
                img[ys[x] + 1:base + 1, x] = False             # hide what is behind
            for x in range(pad, 320 - pad - 1):
                lo, hi = sorted((ys[x], ys[x + 1]))
                img[lo:hi + 1, x] = True
        yield rgb(up(img | sprites(g, scan=True)), (236, 236, 228), (0, 0, 0))


def phosphor(fr):
    """A green tube: what the beam lit keeps glowing and fades, about a beat to
    dark, and every other line of the screen is dimmer. The beam is the deck's
    own picture; its sparkles flare at full glow and fade like everything else."""
    lut = np.zeros((256, 3), np.uint8)
    for i in range(256):
        v = i / 255
        lut[i] = (int(170 * v ** 2.6), int(255 * min(1, 1.08 * v ** 0.75)), int(120 * v ** 2.2))
    glow = np.zeros((240, 320))
    for g in fr:
        glow = np.maximum(glow * 0.55, (bayer(dots(g)) | sprites(g)) * 1.0)
        img = up(lut[np.clip(glow * 255, 0, 255).astype(np.uint8)])
        img[1::2] = (img[1::2] * 0.45).astype(np.uint8)
        yield img


def feedback(fr):
    """A camera pointed at its own monitor, the way the video synthesists
    worked: each frame is the last one, zoomed a little and turned about the
    centre, a little dimmer, with the new picture on top. The turn is a full
    circle every two bars; the zoom kicks on each beat. It feeds back the greys,
    not the dots - turning a dither by a few degrees turns it to noise - and the
    glyphs at full white, so the stars are drawn into the tunnel."""
    lut = np.zeros((256, 3), np.uint8)
    for i in range(256):
        v = i / 255
        lut[i] = (int(255 * min(1, 1.6 * v)), int(255 * max(0, v - 0.55) * 2.0),
                  int(255 * max(0, v - 0.8) * 4.0))
    yy, xx = np.mgrid[0:240, 0:320].astype(float)
    cx, cy = 159.5, 119.5
    buf = np.zeros((240, 320))
    for k, g in enumerate(fr):
        z = 1.10 if k % 4 == 0 else 1.035
        th = 2 * math.pi / 32
        c, s = math.cos(th) / z, math.sin(th) / z
        sx = np.clip(np.round(cx + c * (xx - cx) + s * (yy - cy)).astype(int), 0, 319)
        sy = np.clip(np.round(cy - s * (xx - cx) + c * (yy - cy)).astype(int), 0, 239)
        buf = np.maximum(buf[sy, sx] * 0.84, smooth(dots(g)))
        buf[sprites(g)] = 1.0
        yield up(lut[np.clip(buf * 255, 0, 255).astype(np.uint8)])


def riso(fr):
    """Two inks, printed out of register: pink is this step, blue the step
    before last, and the blue plate drifts with the bar - its sparkles with it.
    Where they overlap the paper takes both. The deck's Bayer in each ink."""
    paper = np.array((244, 240, 229), float) / 255
    pink = np.array((255, 72, 176), float) / 255
    blue = np.array((0, 120, 191), float) / 255
    for k, g in enumerate(fr):
        a = bayer(dots(g)) | sprites(g)
        b = (bayer(dots(fr[k - 2])) | sprites(fr[k - 2])) if k >= 2 else np.zeros_like(a)
        dx, dy = round(4 * math.sin(2 * math.pi * k / 16)), 3
        b = np.roll(np.roll(b, dy, axis=0), dx, axis=1)
        img = np.ones((240, 320, 3)) * paper
        img[a] *= pink
        img[b] *= blue
        yield up((img * 255).astype(np.uint8))


# ---------------------------------------------------------------- the poster

PIECE = 'orbitals'
SECTION = ('II', 'first light')
LANES = ['kick 9...8...9...8...', 'box 2...1...2...1...', 'disc:x 8876532111235678',
         'disc:y 568886531113', 'hat ..3...3...3...4.', 'bass .000.000.000.000']


def glyph(ch):
    return T.R.get(ch) or T.today(ch)


def big(img, x, y, text, s, colour):
    """The round face, a font pixel to s x s."""
    for i, ch in enumerate(text):
        rows = glyph(ch)
        for r in range(24):
            for c in range(12):
                if rows[r][c] == '#':
                    X, Y = x + (i * 12 + c) * s, y + r * s
                    img[Y:Y + s, X:X + s] = colour


def small(img, x, y, text, colour):
    for i, ch in enumerate(text):
        bits = SMALL.g.get(ord(ch))
        if not bits:
            continue
        for r, b in enumerate(bits):
            for c in range(6):
                if b & (0x8000 >> c):
                    img[y + r, x + i * 6 + c] = colour


def poster(fr, pic):
    """A live Swiss poster of the piece: its name, the section in red, a rule,
    the section's lanes with the step each one is on, the picture, and the
    count - flush left, on a grid, redrawn each step. It needs the deck to
    send the lines as well as the frame."""
    paper, ink, red = (246, 244, 238), (18, 18, 18), (228, 0, 43)
    for k, g in enumerate(pic):
        img = np.empty((H, W, 3), np.uint8)
        img[:] = paper
        big(img, 32, 28, PIECE, 4, ink)
        big(img, W - 32 - 2 * 12 * 6, 12, SECTION[0], 6, red)
        img[196:204, 32:W - 32] = ink
        big(img, 32, 220, SECTION[1], 2, ink)
        for i, lane in enumerate(LANES):
            y = 290 + i * 28
            name, pat = lane.split(' ')
            big(img, 32, y, name, 1, ink)
            x0 = 32 + (len(name) + 1) * 12
            on = k % len(pat)
            for j, ch in enumerate(pat):
                if j == on:
                    img[y:y + 24, x0 + j * 12:x0 + j * 12 + 12] = red
                    big(img, x0 + j * 12, y, ch, 1, paper)
                else:
                    big(img, x0 + j * 12, y, ch, 1, ink)
        b = up(bayer(g))                                   # Bayer at twice the pitch
        px, py = W - 32 - b.shape[1], 220
        img[py:py + b.shape[0], px:px + b.shape[1]][b] = ink
        meta = f'124 bpm   dmin   bar {k // 16 + 1}   step {k % 16 + 1:>2}'
        for i, ch in enumerate(meta):
            bits = SMALL.g.get(ord(ch))
            for r, row in enumerate(bits or []):
                for c in range(6):
                    if row & (0x8000 >> c):
                        img[140 + 2 * r:142 + 2 * r, 32 + 12 * i + 2 * c:34 + 12 * i + 2 * c] = ink
        for st in range(16):
            x = W - 32 - 16 * 18 + st * 18 + 6
            img[172:184, x:x + 12] = red if st == k % 16 else ink
        yield img


# ---------------------------------------------------------------- sheets

MODES = [('plain - today, with the dots', plain), ('scan', scan), ('phosphor', phosphor),
         ('feedback', feedback), ('riso', riso), ('poster', None)]


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(HERE), 'docs/img')
    os.makedirs(out, exist_ok=True)
    tmp = tempfile.mkdtemp(prefix='mock_view_')
    fr = frames(tmp, 80, 30, square=False, scene='night')   # the deck's own cells
    pic = frames(tmp, 36, 27)
    shots = {}
    for name, mode in MODES:
        run = list(poster(fr, pic) if mode is None else mode(fr))
        shots[name] = run

    gap, label = 24, 28
    sheet = Image.new('RGB', (2 * W + 3 * gap, 3 * (H + label) + gap), (255, 255, 255))
    pg = Page(sheet.width, sheet.height)
    for i, (name, _) in enumerate(MODES):
        x, y = gap + (i % 2) * (W + gap), label + (i // 2) * (H + label)
        sheet.paste(Image.fromarray(shots[name][STILL]), (x, y))
        pg.text(x, y - 20, name, SMALL, 2)
    lab = pg.im.convert('RGB')
    sheet = Image.fromarray(np.minimum(np.array(sheet), np.array(lab)))
    sheet.save(os.path.join(out, 'view-modes.png'))

    moving = ['scan', 'phosphor', 'feedback', 'riso']
    film = []
    for k in range(16, STEPS):                       # the second bar: feedback has built up
        f = Image.new('RGB', (2 * W, 2 * H))
        for i, name in enumerate(moving):
            f.paste(Image.fromarray(shots[name][k]), ((i % 2) * W, (i // 2) * H))
        film.append(f.quantize(64))
    film[0].save(os.path.join(out, 'view-motion.gif'), save_all=True, append_images=film[1:],
                 duration=120, loop=0)
    print('view mock-ups written to', out)


if __name__ == '__main__':
    main()
