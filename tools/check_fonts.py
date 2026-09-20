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


def check_12x24():
    from font12x24_art import ART
    rows = byte_rows(os.path.join(ROOT, "firmware/components/textgrid/font12x24.c"),
                     95 * 24, 2)
    bad = 0
    for i, code in enumerate(range(32, 127)):
        for r in range(24):
            got = decode(rows[i * 24 + r], 12)
            want = ART[code][r]
            if got != want:
                if bad < 8:
                    print("  [FAIL] code %d row %d\n     art %s\n     c   %s"
                          % (code, r, want, got))
                bad += 1
    print("  %s 12x24: %d glyphs, %d row mismatches"
          % ("[ ok ]" if bad == 0 else "[FAIL]", 95, bad))
    return bad


def check_6x12():
    from make_font import rows_for, G
    rows = byte_rows(os.path.join(ROOT, "firmware/components/textgrid/font6x12.c"),
                     95 * 12, 1)
    bad = 0
    for i, code in enumerate(range(32, 127)):
        want = rows_for(chr(code))
        for r in range(12):
            if rows[i * 12 + r][0] != want[r]:
                if bad < 8:
                    print("  [FAIL] code %d row %d: c 0x%02X, art 0x%02X"
                          % (code, r, rows[i * 12 + r][0], want[r]))
                bad += 1
    print("  %s 6x12:  %d glyphs, %d row mismatches"
          % ("[ ok ]" if bad == 0 else "[FAIL]", 95, bad))
    return bad


def check_metrics():
    """The invariants that make the face a family rather than 95 drawings."""
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


def main():
    print("-- fonts decode back to their art --")
    bad = check_12x24() + check_6x12() + check_metrics()
    print("\n%s (%d problem%s)"
          % ("ALL FONT CHECKS PASS" if bad == 0 else "FONT CHECKS FAILED",
             bad, "" if bad == 1 else "s"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
