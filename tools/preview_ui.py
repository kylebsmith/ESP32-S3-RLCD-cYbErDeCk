#!/usr/bin/env python3
"""Render the deck's screen on the host, from the real font art and the real
layout constants parsed out of the firmware.

The panel cannot be photographed from here and reflective LCDs photograph
badly anyway, so this is how a layout gets looked at before it is flashed. It
is a mock of the renderer, not the renderer, so it proves the *design* and not
the driver - the driver is covered by tools/test_st7305_addr.c.

    python3 tools/preview_ui.py out.png
"""

import os
import re
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from font12x24_art import ART            # noqa: E402

W, H = 400, 300


def layout():
    """Read the layout constants out of editor.c so this cannot drift."""
    src = open(os.path.join(ROOT, "firmware/main/editor.c")).read()
    out = {}
    for name in ("MARGIN_X", "MARGIN_TOP", "TEXT_COLS", "TEXT_ROWS",
                 "CELL_W", "CELL_H", "STATUS_Y", "STATUS_H", "RULE_Y"):
        m = re.search(r"#define\s+%s\s+(\d+)" % name, src)
        if not m:
            raise SystemExit("cannot find %s in editor.c" % name)
        out[name] = int(m.group(1))
    return out


class Screen:
    def __init__(self):
        self.px = [[0] * W for _ in range(H)]

    def set(self, x, y, on):
        if 0 <= x < W and 0 <= y < H:
            self.px[y][x] = 1 if on else 0

    def fill(self, x, y, w, h, on):
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                self.set(xx, yy, on)

    def glyph(self, x, y, ch, inv=False):
        art = ART.get(ord(ch), ART[ord('?')])
        for gy in range(24):
            for gx in range(12):
                on = art[gy][gx] == '#'
                if inv:
                    on = not on
                self.set(x + gx, y + gy, on)

    def text(self, x, y, s, inv=False):
        for i, ch in enumerate(s):
            self.glyph(x + i * 12, y, ch, inv)

    def save(self, path, scale=2):
        img = Image.new("1", (W, H), 1)
        pix = img.load()
        for y in range(H):
            for x in range(W):
                pix[x, y] = 0 if self.px[y][x] else 1   # ink is dark
        if scale != 1:
            img = img.resize((W * scale, H * scale), Image.NEAREST)
        img.save(path)


def wrap(text, cols):
    lines, cur = [], ""
    for para in text.split("\n"):
        if para == "":
            lines.append("")
            continue
        cur = ""
        for word in para.split(" "):
            if cur == "":
                cur = word
            elif len(cur) + 1 + len(word) <= cols:
                cur += " " + word
            else:
                lines.append(cur)
                cur = word
        lines.append(cur)
    return lines


SAMPLE = ("Good morning.\n"
          "\n"
          "The bass should feel late - always behind the beat, never on it.\n"
          "\n"
          "Measured with a loopback, not guessed.")


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "preview.png"
    L = layout()
    s = Screen()

    lines = wrap(SAMPLE, L["TEXT_COLS"])
    for r in range(L["TEXT_ROWS"]):
        if r >= len(lines):
            break
        s.text(L["MARGIN_X"], L["MARGIN_TOP"] + r * L["CELL_H"],
               lines[r][:L["TEXT_COLS"]])

    # cursor: block at the end of the last written line
    last = min(len(lines), L["TEXT_ROWS"]) - 1
    col = min(len(lines[last]), L["TEXT_COLS"] - 1)
    s.glyph(L["MARGIN_X"] + col * L["CELL_W"],
            L["MARGIN_TOP"] + last * L["CELL_H"], " ", inv=True)

    # hairline, then the status bar
    s.fill(L["MARGIN_X"], L["RULE_Y"], W - 2 * L["MARGIN_X"], 2, True)
    s.fill(0, L["STATUS_Y"], W, L["STATUS_H"], True)
    s.text(L["MARGIN_X"], L["STATUS_Y"], "KEYBOARD   142* SD", inv=True)

    s.save(out)
    print("wrote %s   layout: %d x %d cells at (%d,%d), margins %d/%d"
          % (out, L["TEXT_COLS"], L["TEXT_ROWS"], L["MARGIN_X"],
             L["MARGIN_TOP"], L["MARGIN_X"], L["MARGIN_TOP"]))


if __name__ == "__main__":
    main()
