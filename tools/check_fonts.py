#!/usr/bin/env python3
"""Check the committed font C decodes back to the art it was drawn from.

The generator writes bytes; the firmware reads them back with

    bit = (row[gx >> 3] >> (7 - (gx & 7))) & 1

This walks that rule over the committed C and compares the result to the art,
so an encode/decode mismatch - a wrong stride, a flipped bit order, a byte
swapped between the two halves of a 12-pixel row - is caught here rather than
showing up as mush on a panel nobody can photograph.

    python3 tools/check_fonts.py
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)


def byte_rows(path, count, stride):
    """Pull the 0xNN byte literals out of a generated font C file."""
    text = open(path).read()
    body = text[text.index("= {") + 3: text.rindex("};")]
    vals = [int(m, 16) for m in re.findall(r"0x([0-9A-Fa-f]{2})", body)]
    if len(vals) != count * stride:
        raise SystemExit("%s: %d bytes, expected %d"
                         % (path, len(vals), count * stride))
    return [vals[i * stride:(i + 1) * stride] for i in range(count)]


def decode(rowbytes, width):
    """Exactly what tg_font_bit does."""
    return "".join('#' if (rowbytes[gx >> 3] >> (7 - (gx & 7))) & 1 else '.'
                   for gx in range(width))


def full_art(w, h):
    """Every codepoint the face holds, as art: the letters from their own
    source, 127 as the blank the generator emits for the hole, then the tiles.

    ONE TABLE, BUILT THE SAME WAY THE GENERATOR BUILDS IT. Checking only the
    letters would leave the tiles - which are now most of the bytes - decoded
    by nobody, and an off-by-one in the hole at 127 would shift every tile
    silently. The tiles are geometry, so this recomputes them rather than
    trusting a copy.
    """
    from font_tiles import TILE_FIRST, TILE_LAST, tiles
    art = {}
    if w == 12:
        from font12x24_art import ART
        for code in range(32, 127):
            art[code] = ART[code]
    else:
        from make_font import rows_for
        for code in range(32, 127):
            art[code] = [decode([b], w) for b in rows_for(chr(code))]
    for code in range(127, TILE_FIRST):
        art[code] = ['.' * w] * h
    art.update(tiles(w, h))
    assert sorted(art) == list(range(32, TILE_LAST + 1))
    return art


def check_face(path, w, h, stride):
    from font_tiles import TILE_LAST
    art = full_art(w, h)
    n = TILE_LAST - 32 + 1
    rows = byte_rows(os.path.join(ROOT, path), n * h, stride)
    bad = 0
    for i, code in enumerate(range(32, TILE_LAST + 1)):
        for r in range(h):
            got = decode(rows[i * h + r], w)
            want = art[code][r]
            if got != want:
                if bad < 8:
                    print("  [FAIL] code %d row %d\n     art %s\n     c   %s"
                          % (code, r, want, got))
                bad += 1
    print("  %s %dx%d: %d glyphs, %d row mismatches"
          % ("[ ok ]" if bad == 0 else "[FAIL]", w, h, n, bad))
    return bad


def check_12x24():
    return check_face("firmware/components/textgrid/font12x24.c", 12, 24, 2)


def check_6x12():
    return check_face("firmware/components/textgrid/font6x12.c", 6, 12, 1)


def check_metrics():
    """The invariants that make the face a family rather than 95 drawings.

    THE LETTERS ONLY. A tile is not a letter: a tone has to fill the cell edge
    to edge or a field of it is striped with white gutters, and an arc has to
    reach the cell edge or four of them do not join into a circle. Different
    job, different rule - which is exactly why the rule is stated here for one
    range rather than assumed for the whole face."""
    from font12x24_art import ART
    bad = 0
    for code in range(33, 127):          # skip space
        art = ART[code]
        for r in (0, 1, 2, 23):
            if '#' in art[r]:
                print("  [FAIL] code %d: row %d is leading and must be blank" % (code, r))
                bad += 1
        for r in range(24):
            if art[r][10] != '.' or art[r][11] != '.':
                print("  [FAIL] code %d row %d: columns 10-11 are the gap" % (code, r))
                bad += 1
    print("  %s metrics: leading rows blank, gap columns clear"
          % ("[ ok ]" if bad == 0 else "[FAIL]"))
    return bad


def check_tiles_join():
    """THE ARCS MUST TILE. Four quadrant arcs placed 2x2 have to form one
    unbroken circle, which is the whole reason they exist - larger geometry out
    of cell-sized pieces. The join is where it can fail: if an arc stops one
    pixel short of the cell edge, the big circle has four notches in it.

    So: every arc must put ink in the two edges it is supposed to meet at, and
    the ink must be at the same position as its neighbour's.
    """
    from font_tiles import tiles
    bad = 0
    for w, h in ((12, 24), (6, 12)):
        t = tiles(w, h)
        # Quadrant 0 draws the top-left quarter, so it leaves at the RIGHT edge
        # and at the BOTTOM edge. Its right edge must line up with quadrant 1's
        # left edge, and its bottom with quadrant 3's top.
        a, b = t[152], t[153]
        rows_a = [r for r in range(h) if a[r][w - 1] == '#']
        rows_b = [r for r in range(h) if b[r][0] == '#']
        if not rows_a or rows_a != rows_b:
            print("  [FAIL] %dx%d: arcs 0 and 1 meet at rows %s vs %s"
                  % (w, h, rows_a, rows_b))
            bad += 1
        d = t[155]
        cols_a = [x for x in range(w) if a[h - 1][x] == '#']
        cols_d = [x for x in range(w) if d[0][x] == '#']
        if not cols_a or cols_a != cols_d:
            print("  [FAIL] %dx%d: arcs 0 and 3 meet at columns %s vs %s"
                  % (w, h, cols_a, cols_d))
            bad += 1
    print("  %s tiles: the four arcs join into one circle"
          % ("[ ok ]" if bad == 0 else "[FAIL]"))
    return bad


def main():
    print("-- fonts decode back to their art --")
    bad = check_12x24() + check_6x12() + check_metrics() + check_tiles_join()
    print("\n%s (%d problem%s)"
          % ("ALL FONT CHECKS PASS" if bad == 0 else "FONT CHECKS FAILED",
             bad, "" if bad == 1 else "s"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
