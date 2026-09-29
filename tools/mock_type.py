#!/usr/bin/env python3
"""mock_type.py OUT_DIR - the type mock-up in docs/wiki/pictures-and-type.md.

A MOCK-UP. Everything here is the deck's own 12 x 24 and 6 x 12 faces, read
from the generated font the firmware builds, except one glyph: the narrower
zero the legibility research asks for (research.md §3), drawn here from the
current zero with two interior columns taken out. Nothing in the firmware
changes until the reading test says so.

    type-proposal.png   today's 0 beside O; the narrower 0, with and without
                        its dot; a line of the confusion sets as the panel
                        draws them; and hierarchy by scale - a section line at
                        twice the size above the code it heads.
"""
import os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from zine import BIG, SMALL, Page, INK   # noqa: E402


def narrow_zero(dot=True):
    """Today's 12 x 24 zero is ten pixels wide, the same outline as O. Take out
    two interior columns - 3 and 6 - so it is eight wide, centred in the
    cell, stems and dot untouched; optionally drop the dot."""
    rows = []
    for bits in BIG.g[ord('0')]:
        cols = [bool(bits & (0x8000 >> c)) for c in range(12)]
        keep = [cols[c] for c in range(10) if c not in (3, 6)]
        row = [False] + keep + [False] * 3            # centred: 1 + 8 + 3 = 12
        rows.append(row)
    if not dot:
        for r in (11, 12):                             # the dot's rows, cleared
            rows[r] = [v if c in (1, 2, 7, 8) else False for c, v in enumerate(rows[r])]
    return rows


def draw_rows(p, x, y, rows, s):
    for r, row in enumerate(rows):
        for c, on in enumerate(row):
            if on:
                p.box(x + c * s, y + r * s, s, s)


def glyph_rows(face, ch):
    return [[bool(face.g[ord(ch)][r] & (0x8000 >> c)) for c in range(face.w)]
            for r in range(face.h)]


def text(p, x, y, s, face, scale=1):
    for i, ch in enumerate(s):
        p.glyph(x + i * face.w * scale, y, ord(ch), face, scale)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'docs/img')
    os.makedirs(out, exist_ok=True)
    p = Page(1200, 1060)
    S = 6                                              # pixels per font pixel, for the big specimen

    # 1. The zero: today, and the proposal - big, then at the panel's own size.
    text(p, 20, 12, 'the zero - 6 x enlarged, then at the panel\'s own size', SMALL, 2)
    y = 60
    cols = [('0 today', glyph_rows(BIG, '0')), ('O', glyph_rows(BIG, 'O')),
            ('0 narrow', narrow_zero(True)), ('no dot', narrow_zero(False))]
    x = 20
    for label, rows in cols:
        p.frame(x - 2, y - 2, 12 * S + 4, 24 * S + 4, 1)
        draw_rows(p, x, y, rows, S)
        text(p, x, y + 24 * S + 10, label, SMALL, 2)
        x += 12 * S + 110
    # in context, doubled and then at the panel's own size: today's and the proposal's
    y2 = y + 24 * S + 60
    line = '>bpm 120  O0O0  x.0.'
    for k, sc in enumerate((2, 1)):
        yy = y2 + k * 130
        for j, (label, narrow) in enumerate((('today', False), ('narrow', True))):
            ly = yy + j * (24 * sc + 12)
            text(p, 20, ly + 6 * sc, label, SMALL, 2)
            xx = 150
            for ch in line:
                if ch == '0' and narrow:
                    draw_rows(p, xx, ly, narrow_zero(True), sc)
                else:
                    p.glyph(xx, ly, ord(ch), BIG, sc)
                xx += 12 * sc
    y2 += 150

    # 2. Proof in confusion sets, at the panel's size and doubled.
    y3 = y2 + 110
    text(p, 20, y3, 'proof in confusion sets, not glyph by glyph', SMALL, 2)
    sets = '0OD Q  1lI|  5S  8B  2Z  689  71  rn m'
    text(p, 20, y3 + 36, sets, BIG, 1)
    text(p, 20, y3 + 76, sets, BIG, 2)

    # 3. Hierarchy by scale: a section line at twice the size, then the code.
    y4 = y3 + 150
    text(p, 20, y4, 'hierarchy by scale - a section line doubled, the code as it is', SMALL, 2)
    text(p, 20, y4 + 36, '-- II first light', BIG, 2)
    text(p, 20, y4 + 96, '>kick 5...4...5...4...', BIG, 1)
    text(p, 20, y4 + 120, '>box 2...1...2...1...', BIG, 1)
    text(p, 20, y4 + 144, '>disc:x 8876532111235678', BIG, 1)

    # 4. The 6 x 12 as a label face: fine for a status line, too thin for code.
    y5 = y4 + 200
    text(p, 20, y5, 'the 6 x 12 as a label face - a status line, not a line of code', SMALL, 2)
    text(p, 20, y5 + 36, 'orbitals*  12:4  K S ####   124 dmin sw50 clk', SMALL, 1)
    text(p, 20, y5 + 56, 'orbitals*  12:4  K S ####   124 dmin sw50 clk', SMALL, 2)

    p.im.save(os.path.join(out, 'type-proposal.png'))
    print('type mock-up written to', out)


if __name__ == '__main__':
    main()
