#!/usr/bin/env python3
"""cmf.py ART_DIR OUT_DIR - the specimens for docs/CMF.md.

Every image is drawn from the project's own sources: the glyphs from the
generated fonts the firmware builds (font12x24.c, font6x12.c), the pictures
from cells the deck's picture engine drew (tools/zine_art.c, viz.c), the
corner from the superellipse the enclosure is cut with. One bit, black on
white, like the zine.

    cmf-glyphs-12x24.png   every code, 32-155, in the chunky face
    cmf-glyphs-6x12.png    the same codes in the compact face, same size
    cmf-tiles.png          the 28 tiles, named
    cmf-metrics.png        both faces' zones: leading, cap, x, descender
    cmf-pictures.png       the sixteen primitives, as the engine draws them
    cmf-marks.png          cursor, playhead, refusal
    cmf-corner.png         the corner: superellipse n = 3.2 against its arc
"""
import math, os, sys
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from zine import BIG, SMALL, Page, INK, PAPER   # noqa: E402

TILE_NAMES = {128: 'tone 0', 129: 'tone 1', 130: 'tone 2', 131: 'tone 3',
              132: 'tone 4', 133: 'tone 5', 134: 'tone 6', 135: 'tone 7',
              136: 'tone 8', 137: 'speck', 138: 'star 4', 139: 'star 8',
              140: 'burst', 141: 'half up', 142: 'half down', 143: 'half left',
              144: 'half right', 145: 'square', 146: 'diamond', 147: 'disc',
              148: 'ring', 149: 'diagonal', 150: 'diagonal', 151: 'cross',
              152: 'arc', 153: 'arc', 154: 'arc', 155: 'arc'}


def label(p, x, y, s):
    p.text(x, y, s, SMALL, 2)


def library(face, sc, path):
    """Every code 32-155, sixteen to a row, each cell outlined, its number
    under it. Both faces come out the same size, so they can be compared."""
    cw, ch = face.w * sc, face.h * sc
    bw, bh = cw + 30, ch + 44
    cols, rows = 16, 8
    p = Page(cols * bw + 40, rows * bh + 40)
    for i, code in enumerate(range(32, 156)):
        x = 20 + (i % cols) * bw
        y = 20 + (i // cols) * bh
        p.frame(x + 8, y, cw + 8, ch + 8, 1)
        p.glyph(x + 12, y + 4, code, face, sc)
        label(p, x + 8, y + ch + 14, str(code))
    p.im.save(path)


def tiles(path):
    groups = [('the nine tones', range(128, 137)), ('sparkles', range(137, 141)),
              ('halves and a square', range(141, 146)),
              ('diamond, disc, ring', range(146, 149)),
              ('diagonals and cross', range(149, 152)),
              ('quadrant arcs', range(152, 156))]
    p = Page(1700, 1320)
    y = 30
    for name, codes in groups:
        p.text(30, y, name, SMALL, 2)
        y += 40
        x = 30
        for c in codes:
            p.glyph(x, y, c, BIG, 4)
            label(p, x, y + 104, str(c))
            label(p, x, y + 130, TILE_NAMES[c])
            x += 48 + 120
        if name == 'quadrant arcs':
            # the four arcs tile 2x2 into one circle
            for k, (dx, dy) in enumerate(((0, 0), (1, 0), (1, 1), (0, 1))):
                p.glyph(x + 40 + dx * 48, y - 40 + dy * 96, 152 + k, BIG, 4)
            label(p, x + 40, y + 160, '152-155, 2 x 2')
        y += 200
    p.im.save(path)


def metrics(path):
    p = Page(1700, 820)
    for f, sc, x0, zones, name in (
            (BIG, 10, 40, [(0, 'leading'), (4, 'cap'), (10, 'x-height'),
                           (20, 'baseline'), (23, 'descender')], '12 x 24'),
            (SMALL, 20, 820, [(0, 'leading'), (2, 'cap'), (4, 'x-height'),
                              (9, 'baseline'), (11, 'descender')], '6 x 12')):
        y0 = 60
        p.text(x0, 10, name, SMALL, 2)
        for i, ch in enumerate('Hxg'):
            p.glyph(x0 + i * f.w * sc, y0, ord(ch), f, sc)
        width = 3 * f.w * sc
        for row, zname in zones:
            yy = y0 + row * sc
            for xx in range(x0, x0 + width, 6):
                p.box(xx, yy, 3, 1)
            label(p, x0 + width + 10, yy - 12, zname)
    p.im.save(path)


def cells(p, x, y, path, face=SMALL, sc=1):
    d = open(path, 'rb').read()
    w, h = d[0], d[1]
    for cy in range(h):
        for cx in range(w):
            code = d[2 + cy * w + cx]
            if code not in (32, 128):
                p.glyph(x + cx * face.w * sc, y + cy * face.h * sc, code, face, sc)


def pictures(art, path):
    specs = [('disc', '>disc 8'), ('box', '>box 8'), ('turn', '>turn u 9'),
             ('ramp', '>ramp r 9'), ('grid', '>grid 5'), ('noise', '>noise 3'),
             ('mask', '>disc 9 >mask 7'), ('edge', '>disc 8 >edge 1'),
             ('echo', '>echo 8, a disc moving'), ('move', '>move u, echo, noise'),
             ('spin', '>spin <0 3 6 9> >turn 2'), ('warp', '>ramp d 9, then >echo 9 >warp d 9'),
             ('grow', '>noise 1 >grow 9'), ('thin', '>box 8 >thin 9'),
             ('flip', '>disc 6 >flip 9'), ('fold', '>turn l 4 >fold 9')]
    bw, bh = 360 + 30, 288 + 60
    p = Page(4 * bw + 30, 4 * bh + 30)
    for i, (name, line) in enumerate(specs):
        x = 30 + (i % 4) * bw
        y = 20 + (i // 4) * bh
        p.frame(x - 4, y - 4, 360 + 8, 288 + 8, 1)
        cells(p, x, y, os.path.join(art, f'spec-{name}-0.cells'))
        p.text(x, y + 288 + 12, name, SMALL, 2)
        p.text(x + 7 * 12, y + 288 + 18, line, SMALL, 1)
    p.im.save(path)


def marks(path):
    """What the deck draws over its own text: the cursor is a solid inverse
    block, the playhead a bar across the bottom sixth of each cell of the
    step that is sounding (applied after the inverse, so it reads through the
    cursor), and a refusal boxes the character."""
    p = Page(1300, 560)
    line = '>kick 9...8...9...8...'
    x0, y0 = 30, 40
    sc = 2
    under = BIG.h // 6
    for i, ch in enumerate(line):
        x = x0 + i * 24
        inv = (i == 10)                       # the cursor
        bar = (i == 10)                       # the step sounding: '8...'[0]
        if inv:
            p.box(x, y0, 24, 48)
        p.glyph(x, y0, ord(ch), BIG, sc, ink=PAPER if inv else INK)
        if bar:
            for yy in range(y0 + 48 - under * sc, y0 + 48):
                for xx in range(x, x + 24):
                    p.px[xx, yy] = 1 - p.px[xx, yy]
    label(p, x0, y0 + 64, 'the cursor on the playhead: an inverse block, and a bar')
    label(p, x0, y0 + 80, 'across the bottom sixth of the cell that flips back out of it')
    y1 = 220
    for i, ch in enumerate('>hat x...X...'):
        x = x0 + i * 24
        p.glyph(x, y1, ord(ch), BIG, sc)
        if ch == 'X':
            p.frame(x - 3, y1 - 3, 30, 54, 3)
    label(p, x0, y1 + 64, 'a refusal boxes the character; the bar says why:')
    p.text(x0, y1 + 90, 'X is gone: 9 is loud', SMALL, 2)
    p.im.save(path)


def corner(path):
    """The enclosure's corner: a superellipse quadrant, n = 3.2, corner size
    11.2 mm, against the R8 arc it tracks to within 0.2 mm - but with
    curvature ramping in from zero instead of jumping to 1/r."""
    s = 60                                    # pixels per millimetre
    size, n = 11.2, 3.2
    p = Page(int(14 * s) + 80, int(14 * s) + 120)
    ox, oy = 40, 40
    def dot(x, y, r=1):
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                p.dot(int(ox + x * s) + dx, int(oy + y * s) + dy, INK)
    # the superellipse quadrant, in the corner's own frame
    for k in range(4000):
        t = k / 3999 * math.pi / 2
        x = size * (1 - math.cos(t) ** (2 / n))
        y = size * (1 - math.sin(t) ** (2 / n))
        dot(x, y, 2)
    # the R8 arc it tracks, dotted
    for k in range(0, 2000, 14):
        t = k / 1999 * math.pi / 2
        x = 8 * (1 - math.cos(t))
        y = 8 * (1 - math.sin(t))
        dot(x, y, 1)
    # the straight edges it runs into
    for k in range(0, int(14 * s), 3):
        p.dot(ox + k, int(oy), INK) if k > size * s else None
        p.dot(int(ox), oy + k, INK) if k > size * s else None
    label(p, ox, int(oy + 12.5 * s), 'solid: superellipse, n = 3.2, 11.2 mm')
    label(p, ox, int(oy + 12.5 * s) + 18, 'dotted: the R8 arc it tracks to 0.2 mm')
    p.im.save(path)


def main():
    art, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    library(BIG, 2, os.path.join(out, 'cmf-glyphs-12x24.png'))
    library(SMALL, 4, os.path.join(out, 'cmf-glyphs-6x12.png'))
    tiles(os.path.join(out, 'cmf-tiles.png'))
    metrics(os.path.join(out, 'cmf-metrics.png'))
    pictures(art, os.path.join(out, 'cmf-pictures.png'))
    marks(os.path.join(out, 'cmf-marks.png'))
    corner(os.path.join(out, 'cmf-corner.png'))
    print('specimens written to', out)


if __name__ == '__main__':
    main()
