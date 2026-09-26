#!/usr/bin/env python3
"""Generate the 6x12 bitmap face for the cYbErDeCk text grid.

docs/OS.md: at 1 bpp there is no antialiasing, so the face is hand-laid rather
than rasterised. The art below IS the source; firmware/components/textgrid/
font6x12.c is generated from it and should never be edited by hand.

Cell is 6 px wide x 12 px tall. Glyphs occupy columns 0..4; column 5 is the
inter-character gap. Row 0 is leading, rows 2..8 are the cap height, the
baseline sits under row 8, rows 9..10 take descenders, row 11 is blank.

    python3 tools/make_font.py > firmware/components/textgrid/font6x12.c
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from font_tiles import TILE_FIRST, TILE_LAST, tiles

CELL_W, CELL_H = 6, 12
CAP, XH, DESC = 2, 4, 4          # start rows for the three zones

# (start_row, lines).  '#' is ink.
G = {
    ' ': (CAP, []),
    '!': (CAP, ["..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.."]),
    '"': (CAP, [".#.#.", ".#.#."]),
    '#': (CAP, [".#.#.", ".#.#.", "#####", ".#.#.", "#####", ".#.#.", ".#.#."]),
    '$': (CAP, ["..#..", ".####", "#.#..", ".###.", "..#.#", "####.", "..#.."]),
    '%': (CAP, ["##..#", "##..#", "...#.", "..#..", ".#...", "#..##", "#..##"]),
    '&': (CAP, [".##..", "#..#.", "#.#..", ".#...", "#.#.#", "#..#.", ".##.#"]),
    "'": (CAP, ["..#..", "..#.."]),
    '(': (CAP, ["...#.", "..#..", ".#...", ".#...", ".#...", "..#..", "...#."]),
    ')': (CAP, [".#...", "..#..", "...#.", "...#.", "...#.", "..#..", ".#..."]),
    '*': (CAP, [".....", "#.#.#", ".###.", "#####", ".###.", "#.#.#", "....."]),
    '+': (CAP, [".....", "..#..", "..#..", "#####", "..#..", "..#..", "....."]),
    ',': (8,   ["..##.", "..#..", ".#..."]),
    '-': (5,   ["#####"]),
    '.': (7,   ["..##.", "..##."]),
    '/': (CAP, ["....#", "....#", "...#.", "..#..", ".#...", "#....", "#...."]),
    ':': (4,   ["..##.", "..##.", ".....", "..##.", "..##."]),
    ';': (4,   ["..##.", "..##.", ".....", "..##.", "..#..", ".#..."]),
    '<': (CAP, ["...#.", "..#..", ".#...", "#....", ".#...", "..#..", "...#."]),
    '=': (CAP, [".....", ".....", "#####", ".....", "#####", ".....", "....."]),
    '>': (CAP, [".#...", "..#..", "...#.", "....#", "...#.", "..#..", ".#..."]),
    '?': (CAP, [".###.", "#...#", "....#", "...#.", "..#..", ".....", "..#.."]),
    '@': (CAP, [".###.", "#...#", "#.###", "#.#.#", "#.###", "#....", ".###."]),
    '[': (CAP, [".###.", ".#...", ".#...", ".#...", ".#...", ".#...", ".###."]),
    '\\': (CAP, ["#....", "#....", ".#...", "..#..", "...#.", "....#", "....#"]),
    ']': (CAP, [".###.", "...#.", "...#.", "...#.", "...#.", "...#.", ".###."]),
    '^': (CAP, ["..#..", ".#.#.", "#...#", ".....", ".....", ".....", "....."]),
    '_': (10,  ["#####"]),
    '`': (CAP, [".#...", "..#.."]),
    '{': (CAP, ["..##.", ".#...", ".#...", "#....", ".#...", ".#...", "..##."]),
    '|': (CAP, ["..#..", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."]),
    '}': (CAP, [".##..", "...#.", "...#.", "....#", "...#.", "...#.", ".##.."]),
    '~': (4,   [".##..", "#..##"]),

    '0': (CAP, [".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."]),
    '1': (CAP, ["..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."]),
    '2': (CAP, [".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"]),
    '3': (CAP, ["#####", "...#.", "..#..", "...#.", "....#", "#...#", ".###."]),
    '4': (CAP, ["...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."]),
    '5': (CAP, ["#####", "#....", "####.", "....#", "....#", "#...#", ".###."]),
    '6': (CAP, ["..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###."]),
    '7': (CAP, ["#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."]),
    '8': (CAP, [".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."]),
    '9': (CAP, [".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.."]),

    'A': (CAP, [".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"]),
    'B': (CAP, ["####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."]),
    'C': (CAP, [".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."]),
    'D': (CAP, ["####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."]),
    'E': (CAP, ["#####", "#....", "#....", "####.", "#....", "#....", "#####"]),
    'F': (CAP, ["#####", "#....", "#....", "####.", "#....", "#....", "#...."]),
    'G': (CAP, [".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".####"]),
    'H': (CAP, ["#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"]),
    'I': (CAP, [".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."]),
    'J': (CAP, ["....#", "....#", "....#", "....#", "#...#", "#...#", ".###."]),
    'K': (CAP, ["#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"]),
    'L': (CAP, ["#....", "#....", "#....", "#....", "#....", "#....", "#####"]),
    'M': (CAP, ["#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"]),
    'N': (CAP, ["#...#", "##..#", "##..#", "#.#.#", "#..##", "#..##", "#...#"]),
    'O': (CAP, [".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."]),
    'P': (CAP, ["####.", "#...#", "#...#", "####.", "#....", "#....", "#...."]),
    'Q': (CAP, [".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"]),
    'R': (CAP, ["####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"]),
    'S': (CAP, [".###.", "#...#", "#....", ".###.", "....#", "#...#", ".###."]),
    'T': (CAP, ["#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."]),
    'U': (CAP, ["#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."]),
    'V': (CAP, ["#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."]),
    'W': (CAP, ["#...#", "#...#", "#...#", "#.#.#", "#.#.#", "##.##", "#...#"]),
    'X': (CAP, ["#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"]),
    'Y': (CAP, ["#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."]),
    'Z': (CAP, ["#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"]),

    'a': (XH,  [".###.", "....#", ".####", "#...#", ".####"]),
    'b': (CAP, ["#....", "#....", "####.", "#...#", "#...#", "#...#", "####."]),
    'c': (XH,  [".###.", "#...#", "#....", "#...#", ".###."]),
    'd': (CAP, ["....#", "....#", ".####", "#...#", "#...#", "#...#", ".####"]),
    'e': (XH,  [".###.", "#...#", "#####", "#....", ".###."]),
    'f': (CAP, ["..##.", ".#..#", ".#...", "####.", ".#...", ".#...", ".#..."]),
    'g': (DESC,[".####", "#...#", "#...#", ".####", "....#", "#...#", ".###."]),
    'h': (CAP, ["#....", "#....", "####.", "#...#", "#...#", "#...#", "#...#"]),
    'i': (CAP, ["..#..", ".....", ".##..", "..#..", "..#..", "..#..", ".###."]),
    'j': (CAP, ["...#.", ".....", "...#.", "...#.", "...#.", "...#.", "...#.",
                "#..#.", ".##.."]),
    'k': (CAP, ["#....", "#....", "#...#", "#..#.", "###..", "#..#.", "#...#"]),
    'l': (CAP, [".##..", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."]),
    'm': (XH,  ["##.#.", "#.#.#", "#.#.#", "#...#", "#...#"]),
    'n': (XH,  ["####.", "#...#", "#...#", "#...#", "#...#"]),
    'o': (XH,  [".###.", "#...#", "#...#", "#...#", ".###."]),
    'p': (DESC,["####.", "#...#", "#...#", "####.", "#....", "#....", "#...."]),
    'q': (DESC,[".####", "#...#", "#...#", ".####", "....#", "....#", "....#"]),
    'r': (XH,  ["#.##.", "##..#", "#....", "#....", "#...."]),
    's': (XH,  [".####", "#....", ".###.", "....#", "####."]),
    't': (CAP, [".#...", ".#...", "####.", ".#...", ".#...", ".#..#", "..##."]),
    'u': (XH,  ["#...#", "#...#", "#...#", "#...#", ".####"]),
    'v': (XH,  ["#...#", "#...#", "#...#", ".#.#.", "..#.."]),
    'w': (XH,  ["#...#", "#...#", "#.#.#", "#.#.#", ".#.#."]),
    'x': (XH,  ["#...#", ".#.#.", "..#..", ".#.#.", "#...#"]),
    'y': (DESC,["#...#", "#...#", "#...#", ".####", "....#", "#...#", ".###."]),
    'z': (XH,  ["#####", "...#.", "..#..", ".#...", "#####"]),
}


def rows_for(ch):
    start, art = G[ch]
    rows = [0] * CELL_H
    for i, line in enumerate(art):
        r = start + i
        if not 0 <= r < CELL_H:
            raise SystemExit("glyph %r row %d out of the 12-row cell" % (ch, r))
        if len(line) != 5:
            raise SystemExit("glyph %r row %d is %d wide, want 5" % (ch, r, len(line)))
        bits = 0
        for c, px in enumerate(line):
            if px == '#':
                bits |= 1 << (7 - c)      # bit7 = leftmost column
            elif px != '.':
                raise SystemExit("glyph %r: %r is not '#' or '.'" % (ch, px))
        rows[r] = bits
    return rows


def main():
    missing = [chr(c) for c in range(32, 127) if chr(c) not in G]
    if missing:
        raise SystemExit("no art for: %r" % missing)

    out = sys.stdout.write
    out("/* GENERATED by tools/make_font.py - do not edit.\n"
        " * The art this is built from lives in that script; edit it there and\n"
        " * regenerate, so the shapes stay reviewable.\n"
        " */\n"
        '#include "font6x12.h"\n'
        '#include "tgfont.h"\n\n'
        "const uint8_t font6x12[(FONT_LAST - FONT_FIRST + 1) * FONT_H] = {\n")
    for code in range(32, 127):
        ch = chr(code)
        rows = rows_for(ch)
        label = {'\\': "backslash", "'": "apostrophe"}.get(ch, ch)
        out("    /* %3d %-9s */ " % (code, "'" + label + "'"))
        out(", ".join("0x%02X" % b for b in rows))
        out(",\n")

    # THE GAP BETWEEN THE LETTERS AND THE TILES. Codepoints 127 up to the first
    # tile have no art and must still occupy their slots, or every tile after
    # them is off by the size of the hole.
    T = tiles(CELL_W, CELL_H)
    for code in range(127, TILE_FIRST):
        out("    /* %3d unused    */ " % code)
        out(", ".join("0x00" for _ in range(CELL_H)))
        out(",\n")
    out("\n    /* THE TILES - our own shapes, generated by tools/font_tiles.py.\n"
        "     * Geometry rather than letterforms, so they are computed at this\n"
        "     * cell size rather than drawn and downsampled: a 50%% dither ORed\n"
        "     * down to half scale is solid black, which is not a tone. */\n")
    for code in range(TILE_FIRST, TILE_LAST + 1):
        out("    /* %3d tile      */ " % code)
        out(", ".join("0x%02X" % rowbyte(r) for r in T[code]))
        out(",\n")
    out("};\n\n")
    out("const tg_font_t tg_font_6x12 = {\n"
        '    .name   = "6x12",\n'
        "    .w      = FONT_W,\n"
        "    .h      = FONT_H,\n"
        "    .stride = 1,\n"
        "    .first  = FONT_FIRST,\n"
        "    .last   = FONT_LAST,\n"
        "    .data   = font6x12,\n"
        "};\n")


def rowbyte(line):
    """One row of a <=8px-wide glyph as a byte, MSB leftmost."""
    b = 0
    for i, px in enumerate(line):
        if px == '#':
            b |= 1 << (7 - i)
    return b


def main_12x24():
    """Emit the 12x24 face from tools/font12x24_art.py."""
    import os
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from font12x24_art import ART
    out = sys.stdout.write
    out("/* GENERATED by tools/make_font.py --12x24 - do not edit. */\n")
    out('#include "font12x24.h"\n#include "tgfont.h"\n\n')
    def emit(line):
        b0 = b1 = 0
        for i, px in enumerate(line):
            if px == '#':
                if i < 8:
                    b0 |= 1 << (7 - i)
                else:
                    b1 |= 1 << (7 - (i - 8))
        out("    0x%02X, 0x%02X,  /* %s */\n" % (b0, b1, line))

    out("const uint8_t font12x24[(%d - 32 + 1) * 24 * 2] = {\n" % TILE_LAST)
    for code in range(32, 127):
        for line in ART[code]:
            emit(line)
    # The hole between the letters and the tiles has to be occupied, or every
    # tile after it is off by the size of the hole.
    for code in range(127, TILE_FIRST):
        for _ in range(24):
            emit('.' * 12)
    out("\n    /* THE TILES - our own shapes, generated by tools/font_tiles.py.\n"
        "     * Geometry rather than letterforms, so they are computed at this\n"
        "     * cell size rather than drawn once and scaled. */\n")
    T = tiles(12, 24)
    for code in range(TILE_FIRST, TILE_LAST + 1):
        for line in T[code]:
            emit(line)
    out("};\n\n")
    out("const tg_font_t tg_font_12x24 = {\n"
        '    .name = "12x24", .w = 12, .h = 24, .stride = 2,\n'
        "    .first = 32, .last = %d, .data = font12x24,\n};\n" % TILE_LAST)


def main_view():
    """Emit both faces as ONE standalone header for the HDMI view node
    (view/deckview/deckfont.h). Same art, same tile generator: a tile on the
    screen is the tile on the panel, bit for bit, because it cannot be anything
    else. CI regenerates it and diffs, exactly as it does the panel's faces."""
    import os
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from font12x24_art import ART
    out = sys.stdout.write
    out("/* GENERATED by tools/make_font.py --view - do not edit.\n"
        " * The deck's own two faces, for the view node: the art is in\n"
        " * tools/make_font.py and tools/font12x24_art.py, the tiles in\n"
        " * tools/font_tiles.py. */\n"
        "#pragma once\n#include <stdint.h>\n\n"
        "#define DECKFONT_FIRST 32\n#define DECKFONT_LAST  %d\n\n" % TILE_LAST)
    # 12x24: two bytes a row, MSB leftmost
    out("static const uint8_t deckfont_12x24[(%d - 32 + 1) * 24 * 2] = {\n" % TILE_LAST)
    T12 = tiles(12, 24)
    for code in range(32, TILE_LAST + 1):
        lines = (ART[code] if code < 127 else
                 T12[code] if code >= TILE_FIRST else ['.' * 12] * 24)
        vals = []
        for line in lines:
            b0 = b1 = 0
            for i, px in enumerate(line):
                if px == '#':
                    if i < 8:
                        b0 |= 1 << (7 - i)
                    else:
                        b1 |= 1 << (7 - (i - 8))
            vals += ["0x%02X" % b0, "0x%02X" % b1]
        out("    " + ",".join(vals) + ",  /* %d */\n" % code)
    out("};\n\n")
    # 6x12: one byte a row, MSB leftmost
    missing = [chr(c) for c in range(32, 127) if chr(c) not in G]
    if missing:
        raise SystemExit("no art for: %r" % missing)
    out("static const uint8_t deckfont_6x12[(%d - 32 + 1) * 12] = {\n" % TILE_LAST)
    T6 = tiles(CELL_W, CELL_H)
    for code in range(32, TILE_LAST + 1):
        if code < 127:
            rows = rows_for(chr(code))
        elif code >= TILE_FIRST:
            rows = [rowbyte(r) for r in T6[code]]
        else:
            rows = [0] * CELL_H
        out("    " + ",".join("0x%02X" % b for b in rows) + ",  /* %d */\n" % code)
    out("};\n")


if __name__ == "__main__":
    if "--12x24" in sys.argv:
        main_12x24()
    elif "--view" in sys.argv:
        main_view()
    else:
        main()
