#!/usr/bin/env python3
"""mock_pictures.py OUT_DIR - the mock-ups in docs/wiki/pictures-and-type.md.

A MOCK-UP, AND IT SAYS WHICH PART. Every frame is drawn by the deck's picture
engine, firmware/components/viz/viz.c, through tools/mock_frames.c - the same way
tools/zine_art.c draws the zine. Only two things are imagined here, and both are
labelled on the images:

  square dots   the engine built a second time from a COPY of viz.c with three
                edits: the frame may be larger (VIZ_W, VIZ_H), and vertical
                distance counts once instead of twice, because a 4 x 4 dot is
                square where a 12 x 24 cell is not. Nothing else in viz.c changes.
                The copy is checked edit by edit, so if viz.c moves on, this
                fails instead of mocking something else.
  stamp         type laid into the picture: the deck's own faces, a font pixel to
                a dot, composited here - there is no stamp in the engine.

One bit, at the panel's own pixel size, ordered dither from font_tiles.BAYER.

    python3 tools/mock_pictures.py docs/img        # needs a C compiler, Pillow
"""
import os, re, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from zine import BIG, SMALL, Page, INK   # noqa: E402
from font_tiles import BAYER             # noqa: E402

VIZ_C = os.path.join(REPO, 'firmware/components/viz/viz.c')
VIZ_H = os.path.join(REPO, 'firmware/components/viz/include/viz.h')
INCLUDES = ['-I', os.path.join(REPO, 'firmware/components/seq/include'),
            '-I', os.path.join(HERE, 'hostshim')]

# (pattern, replacement, how many times it must match) - the whole of the
# square-dot proposal as far as the engine is concerned.
SQUARE = [
    (r'#define VIZ_W 60\b', '#define VIZ_W 100', 1, 'h'),
    (r'#define VIZ_H 24\b', '#define VIZ_H 75', 1, 'h'),
    (r'-2 \* \(y - cy\)', '-(y - cy)', 1, 'c'),
    (r'2 \* \(y - cy\)', '(y - cy)', 4, 'c'),
    (r'\(s_w / 2 < s_h\) \? \(s_w / 2\) : s_h', '(s_w < s_h) ? (s_w / 2) : (s_h / 2)', 2, 'c'),
]


def build(tmp, square):
    src = os.path.join(tmp, 'sq' if square else 'real')
    inc = os.path.join(src, 'include')
    os.makedirs(inc, exist_ok=True)
    c, h = open(VIZ_C).read(), open(VIZ_H).read()
    if square:
        for pat, rep, n, which in SQUARE:
            text = h if which == 'h' else c
            text, k = re.subn(pat, rep, text)
            if k != n:
                sys.exit(f'viz.c has moved on: {pat!r} matched {k} times, expected {n}')
            if which == 'h':
                h = text
            else:
                c = text
    open(os.path.join(src, 'viz.c'), 'w').write(c)
    open(os.path.join(inc, 'viz.h'), 'w').write(h)
    exe = os.path.join(src, 'mock_frames')
    subprocess.run(['cc', '-std=c11', '-O1', '-I', inc] + INCLUDES +
                   ['-o', exe, os.path.join(HERE, 'mock_frames.c'),
                    os.path.join(src, 'viz.c')], check=True)
    return exe


def frames(exe, out, sizes):
    os.makedirs(out, exist_ok=True)
    for w, h in sizes:
        subprocess.run([exe, out, str(w), str(h)], check=True)


def load(d, scene, w, h):
    b = open(os.path.join(d, f'{scene}-{w}x{h}.cells'), 'rb').read()
    assert (b[0], b[1]) == (w, h)
    return [[b[2 + y * w + x] for x in range(w)] for y in range(h)]


def tone(code):
    """A cell's tone, 0..8. The one glyph a picture writes that is not a tone is
    the small disc, 147, and it is solid."""
    return code - 128 if 128 <= code <= 136 else (8 if code == 147 else 0)


# ---------------------------------------------------------------- renderers

def draw_tiles(p, ox, oy, g, face):
    """Today: each cell is a glyph from the face the text is set in."""
    for y, row in enumerate(g):
        for x, code in enumerate(row):
            p.glyph(ox + x * face.w, oy + y * face.h, code, face, 1)


def draw_dots(p, ox, oy, g, s):
    """Square dots, s x s pixels, tone by ordered dither in panel coordinates -
    so a field of one tone is seamless across dots, as the tiles are across
    cells. At s = 4 a dot is exactly one period of the 4 x 4 matrix."""
    for y, row in enumerate(g):
        for x, code in enumerate(row):
            t = tone(code) * 2
            if t == 0:
                continue
            for py in range(s):
                for px in range(s):
                    X, Y = ox + x * s + px, oy + y * s + py
                    if BAYER[Y & 3][X & 3] < t:
                        p.px[X, Y] = INK


def draw_banded(p, ox, oy, g, s):
    """4 x 4 dots, banded - the chosen look. A grey dot is drawn from the tones
    at its four corners, interpolated across its pixels and cut into the nine
    tones, so greys meet in curves. A solid or an empty dot is drawn exactly as
    it is: banding a one-dot line with the paper beside it would grey it out."""
    h, w = len(g), len(g[0])
    t = [[tone(c) for c in row] for row in g]
    cor = [[0.0] * (w + 1) for _ in range(h + 1)]
    for j in range(h + 1):
        for i in range(w + 1):
            vals = [t[y][x] for (x, y) in ((i - 1, j - 1), (i, j - 1), (i - 1, j), (i, j))
                    if 0 <= x < w and 0 <= y < h]
            cor[j][i] = sum(vals) / len(vals) / 8.0
    for y in range(h):
        for x in range(w):
            own = t[y][x]
            a, b, d, e = cor[y][x], cor[y][x + 1], cor[y + 1][x], cor[y + 1][x + 1]
            for py in range(s):
                v = (py + 0.5) / s
                left, right = a + (d - a) * v, b + (e - b) * v
                for px in range(s):
                    X, Y = ox + x * s + px, oy + y * s + py
                    if own in (0, 8):
                        lv = own
                    else:
                        lv = int((left + (right - left) * (px + 0.5) / s) * 8 + 0.5)
                        lv = min(max(lv, 1), 7)          # a grey dot stays a grey
                    if BAYER[Y & 3][X & 3] < lv * 2:
                        p.px[X, Y] = INK


def draw_dual(p, ox, oy, g, cw, ch, crisp):
    """A renderer-only alternative, kept to show why it is not proposed: each
    cell drawn from the tones at its four corners (the mean of the cells that
    meet there), interpolated across the cell - banded into the nine tones, or
    cut at half for a crisp edge. It smooths edges; it cannot add resolution a
    four-row frame does not have."""
    h, w = len(g), len(g[0])
    t = [[tone(c) for c in row] for row in g]
    cor = [[0.0] * (w + 1) for _ in range(h + 1)]
    for j in range(h + 1):
        for i in range(w + 1):
            vals = [t[y][x] for (x, y) in ((i - 1, j - 1), (i, j - 1), (i - 1, j), (i, j))
                    if 0 <= x < w and 0 <= y < h]
            cor[j][i] = sum(vals) / len(vals) / 8.0
    for y in range(h):
        for x in range(w):
            a, b, d, e = cor[y][x], cor[y][x + 1], cor[y + 1][x], cor[y + 1][x + 1]
            for py in range(ch):
                v = (py + 0.5) / ch
                left, right = a + (d - a) * v, b + (e - b) * v
                for px in range(cw):
                    val = left + (right - left) * (px + 0.5) / cw
                    X, Y = ox + x * cw + px, oy + y * ch + py
                    ink = val >= 0.5 if crisp else BAYER[Y & 3][X & 3] < int(val * 8 + 0.5) * 2
                    if ink:
                        p.px[X, Y] = INK


# ---------------------------------------------------------------- sheets

def sheet(path, scenes, cols, gap=16, label=24):
    """cols: (title, pixel w, pixel h, draw(p, ox, oy, scene))"""
    pw = [c[1] for c in cols]
    ph = max(c[2] for c in cols)
    p = Page(sum(pw) + gap * (len(cols) + 1), label + len(scenes) * (ph + gap) + gap)
    x = gap
    for title, w, h, draw in cols:
        p.text(x, 4, title, SMALL, 1)
        for i, sc in enumerate(scenes):
            y = label + gap + i * (ph + gap)
            p.frame(x - 2, y - 2, w + 4, h + 4, 1)
            draw(p, x, y, sc)
        x += w + gap
    p.im.save(path)


def stamp(g, text, face, x0, y0, xor=False):
    """Type into a tone grid, a font pixel to a dot: solid, or knocked out."""
    for i, ch in enumerate(text):
        rows = face.g[ord(ch)]
        for r in range(face.h):
            for c in range(face.w):
                if rows[r] & (0x8000 >> c):
                    x, y = x0 + i * face.w + c, y0 + r
                    if 0 <= y < len(g) and 0 <= x < len(g[0]):
                        g[y][x] = (136 - (g[y][x] - 128 if g[y][x] >= 128 else 0)) if xor else 136


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, 'docs/img')
    os.makedirs(out, exist_ok=True)
    tmp = tempfile.mkdtemp(prefix='mock_pictures_')
    try:
        real, sq = build(tmp, False), build(tmp, True)
        rd, sd = os.path.join(tmp, 'rf'), os.path.join(tmp, 'sf')
        frames(real, rd, [(28, 4), (58, 17)])
        frames(sq, sd, [(56, 16), (84, 24), (87, 51)])
        R = lambda sc, w, h: load(rd, sc, w, h)          # noqa: E731
        Q = lambda sc, w, h: load(sd, sc, w, h)          # noqa: E731

        # 1. The default pane - 28 x 4 cells of 12 x 24, 336 x 96 pixels - today,
        #    and the same pixels as square dots.
        scenes = ['disc', 'discmask', 'ring', 'box', 'turn', 'radar', 'orbit']
        sheet(os.path.join(out, 'pictures-dots.png'), scenes, [
            ('today: 28 x 4 cells of 12 x 24', 336, 96,
             lambda p, x, y, s: draw_tiles(p, x, y, R(s, 28, 4), BIG)),
            ('6 x 6 dots: 56 x 16', 336, 96,
             lambda p, x, y, s: draw_dots(p, x, y, Q(s, 56, 16), 6)),
            ('4 x 4 dots: 84 x 24 (proposed)', 336, 96,
             lambda p, x, y, s: draw_dots(p, x, y, Q(s, 84, 24), 4)),
        ])

        # 1b. The choice, 2026-09-28: 4 x 4 dots, BANDED - each dot drawn from
        #     the tones at its four corners, interpolated across its sixteen
        #     pixels and cut into the nine tones. A 4 x 4 dot is one period of
        #     the dither, so on the deck this is a lookup, not arithmetic.
        sheet(os.path.join(out, 'pictures-banded.png'), scenes, [
            ('today: 28 x 4 cells of 12 x 24', 336, 96,
             lambda p, x, y, s: draw_tiles(p, x, y, R(s, 28, 4), BIG)),
            ('4 x 4 dots, flat', 336, 96,
             lambda p, x, y, s: draw_dots(p, x, y, Q(s, 84, 24), 4)),
            ('4 x 4 dots, banded (chosen)', 336, 96,
             lambda p, x, y, s: draw_banded(p, x, y, Q(s, 84, 24), 4)),
        ])

        # 2. Smoothing the cells instead: it rounds edges, it cannot add rows.
        sheet(os.path.join(out, 'pictures-smoothing.png'), ['disc', 'ring', 'orbit'], [
            ('today', 336, 96, lambda p, x, y, s: draw_tiles(p, x, y, R(s, 28, 4), BIG)),
            ('corners, banded', 336, 96,
             lambda p, x, y, s: draw_dual(p, x, y, R(s, 28, 4), 12, 24, False)),
            ('corners, cut at half', 336, 96,
             lambda p, x, y, s: draw_dual(p, x, y, R(s, 28, 4), 12, 24, True)),
        ])

        # 3. The largest picture the editor gives today - the compact face, split
        #    at 17 rows: 58 x 17 cells of 6 x 12, 348 x 204 pixels - and 4 x 4 dots.
        sheet(os.path.join(out, 'pictures-panel.png'), ['disc', 'ring', 'radar', 'orbit'], [
            ('today, compact face: 58 x 17 cells of 6 x 12', 348, 204,
             lambda p, x, y, s: draw_tiles(p, x, y, R(s, 58, 17), SMALL)),
            ('4 x 4 dots: 87 x 51, the same pixels', 348, 204,
             lambda p, x, y, s: draw_dots(p, x, y, Q(s, 87, 51), 4)),
        ])

        # 4. Type as picture, on 4 x 4 dots in the default pane.
        blank = [[128] * 84 for _ in range(24)]
        panels = []
        g = [r[:] for r in blank]; stamp(g, 'kick', BIG, 18, 0)
        panels.append(('the 12 x 24 face, a font pixel a dot: a word fills the pane', g))
        g = Q('disc', 84, 24); stamp(g, 'x...x...', SMALL, 18, 6, xor=True)
        panels.append(('a disc, and type knocked out of it', g))
        g = [r[:] for r in blank]
        for k, x0 in enumerate(range(4, 56, 9)):
            g = [[max(128, c - 1) for c in row] for row in g]     # echo: a tone fainter a step
            stamp(g, 'deck', SMALL, x0, 6 + (k % 2) * 4)
        panels.append(('a word moving right under echo', g))
        g = Q('ring', 84, 24); stamp(g, 'o', BIG, 36, 0)
        panels.append(('a ring, and the letter o in it', g))
        p = Page(84 * 4 + 32, len(panels) * (96 + 40) + 20)
        for i, (lab, g) in enumerate(panels):
            oy = 20 + i * (96 + 40)
            p.text(16, oy - 16, lab, SMALL, 1)
            p.frame(14, oy - 2, 84 * 4 + 4, 96 + 4, 1)
            draw_dots(p, 16, oy, g, 4)
        p.im.save(os.path.join(out, 'pictures-stamp.png'))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print('mock-ups written to', out)


if __name__ == '__main__':
    main()
