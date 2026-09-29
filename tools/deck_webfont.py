#!/usr/bin/env python3
"""The deck's two faces as fonts a browser can use, for the wiki book.

  python3 tools/deck_webfont.py OUT_DIR     writes Deck.ttf and DeckSmall.ttf

Built from the firmware's own font sources, so the book is set in exactly what
the panel draws: the round 12 x 24 face as "Deck" and the 6 x 12 as "Deck
Small", every pixel a square in the outline. ASCII keeps its codes; the
deck's tiles, 128-155 (the nine Bayer tones, the sparkles, blocks and arcs),
go to the Private Use Area at U+E080-U+E09B, so a page can set them as text;
and the characters the panel never needed - dashes, arrows, box drawing - come
from tools/deck_extra.py, drawn on the same grids.

One em is one cell: at font-size 24px, Deck is 12 x 24 CSS pixels a
character and Deck Small, pixel-doubled, the same. Line-height 1 stacks the
cells as the panel does.
"""
import os
import sys

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from zine import BIG_SRC, SMALL_SRC, load_face   # noqa: E402
import deck_extra                               # noqa: E402

UPM = 2400
TILE_BASE = 0xE000                              # tile 128 -> U+E080


def bitmap_rows(glyph, gw):
    return [''.join('#' if r & (0x8000 >> x) else '.' for x in range(gw)) for r in glyph]


def outline(rows, px, base_row):
    """A glyph from pixel rows: one rectangle per horizontal run, clockwise."""
    pen = TTGlyphPen(None)
    for y, row in enumerate(rows):
        x = 0
        while x < len(row):
            if row[x] != '#':
                x += 1
                continue
            x0 = x
            while x < len(row) and row[x] == '#':
                x += 1
            top, bottom = (base_row - y) * px, (base_row - y - 1) * px
            pen.moveTo((x0 * px, bottom))
            pen.lineTo((x0 * px, top))
            pen.lineTo((x * px, top))
            pen.lineTo((x * px, bottom))
            pen.closePath()
    return pen.glyph()


def build(name, glyphs, gw, gh, base_row, extra, path):
    """glyphs: {code: rows of '#'/'.'}; base_row: rows above the baseline."""
    px = UPM // gh
    cmap, outlines, order = {}, {'.notdef': outline([], px, base_row)}, ['.notdef']
    def add(cp, rows):
        gname = 'uni%04X' % cp
        outlines[gname] = outline(rows, px, base_row)
        cmap[cp] = gname
        order.append(gname)
    for code, rows in sorted(glyphs.items()):
        cp = code if code < 128 else TILE_BASE + code
        add(cp, rows)
    for cp, rows in sorted(extra.items()):
        add(cp, rows)
    add(0xA0, ['.' * gw] * gh)                    # no-break space
    fb = FontBuilder(UPM, isTTF=True)
    fb.setupGlyphOrder(order)
    fb.setupCharacterMap(cmap)
    fb.setupGlyf(outlines)
    adv = gw * px
    fb.setupHorizontalMetrics({g: (adv, 0) for g in order})
    asc, desc = base_row * px, -(gh - base_row) * px
    fb.setupHorizontalHeader(ascent=asc, descent=desc, lineGap=0)
    fb.setupNameTable({'familyName': name, 'styleName': 'Regular'})
    fb.setupOS2(version=4, sTypoAscender=asc, sTypoDescender=desc, sTypoLineGap=0,
                usWinAscent=asc, usWinDescent=-desc, fsSelection=0x80,
                achVendID='DECK', xAvgCharWidth=adv)
    fb.setupPost(isFixedPitch=1)
    fb.save(path)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else '.'
    os.makedirs(out, exist_ok=True)
    big = {c: bitmap_rows(g, 12) for c, g in load_face(BIG_SRC, 12, 24).items()}
    small = {c: bitmap_rows(g, 6) for c, g in load_face(SMALL_SRC, 6, 12).items()}
    build('Deck', big, 12, 24, 20, deck_extra.EXTRA, os.path.join(out, 'Deck.ttf'))
    build('Deck Small', small, 6, 12, 9, deck_extra.SMALL, os.path.join(out, 'DeckSmall.ttf'))


if __name__ == '__main__':
    main()
