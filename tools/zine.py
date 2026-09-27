#!/usr/bin/env python3
"""zine.py ART_DIR OUT_DIR - set the hello zine in the deck's own faces.

The zine is eight pages of a one-sheet mini zine: 2.75 x 4.25 in at 300 dpi,
one bit deep, like the panel. Body text is the deck's 12x24 face at twice its
size - thirty columns, the chunky screen's line length - and notes are its 6x12
face, sixty columns, the dense screen's. The pictures are the deck's own: cells
drawn by firmware/components/viz/viz.c through tools/zine_art.c, set in the
same tiles the panel uses. Nothing here is a typeface from anywhere else.

Writes OUT_DIR/hello-pages.pdf (reading order), OUT_DIR/hello-print.pdf (the
imposed sheet, US Letter landscape, fold and cut) and a PNG per page.
"""
import os, re, sys
from PIL import Image, ImageDraw

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIG_SRC = os.path.join(REPO, 'firmware/components/textgrid/font12x24.c')
SMALL_SRC = os.path.join(REPO, 'firmware/components/textgrid/font6x12.c')

W, H = 825, 1275            # a panel: 2.75 x 4.25 in at 300 dpi
M = 40                      # the margin, flush left
INK, PAPER = 0, 1


# ------------------------------------------------------------------ faces

def load_face(path, gw, gh):
    """{code: rows} from a generated font source; each row an int, MSB left."""
    src = open(path).read()
    body = src[src.index('] = {') + 5: src.index('};')]
    body = re.sub(r'/\*.*?\*/', '', body, flags=re.S)
    vals = [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{2})', body)]
    bpr = 2 if gw > 8 else 1
    per = gh * bpr
    glyphs = {}
    for i in range(len(vals) // per):
        rows = vals[i * per:(i + 1) * per]
        if bpr == 2:
            glyphs[32 + i] = [(rows[2 * r] << 8) | rows[2 * r + 1] for r in range(gh)]
        else:
            glyphs[32 + i] = [rows[r] << 8 for r in range(gh)]
    return glyphs


class Face:
    def __init__(self, path, gw, gh):
        self.g, self.w, self.h = load_face(path, gw, gh), gw, gh


BIG = Face(BIG_SRC, 12, 24)
SMALL = Face(SMALL_SRC, 6, 12)


# ------------------------------------------------------------------ page

class Page:
    def __init__(self, w=W, h=H):
        self.w, self.h = w, h
        self.im = Image.new('1', (w, h), PAPER)
        self.px = self.im.load()

    def dot(self, x, y, ink):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[x, y] = ink

    def glyph(self, x, y, code, face, s=2, ink=INK, screen=None):
        rows = face.g.get(code)
        if rows is None:
            return
        for r, bits in enumerate(rows):
            if not bits:
                continue
            for c in range(face.w):
                if bits & (0x8000 >> c):
                    for dy in range(s):
                        for dx in range(s):
                            X, Y = x + c * s + dx, y + r * s + dy
                            if screen is None or screen(X, Y):
                                self.dot(X, Y, ink)

    def text(self, x, y, s, face=BIG, sc=2, ink=INK, screen=None):
        """Set a line; returns the x after it. Bytes >= 128 are the tiles, and
        [@key] is a citation, numbered from zine_refs."""
        if isinstance(s, str):
            s = cite(s)
        for ch in s:
            code = ch if isinstance(ch, int) else ord(ch)
            self.glyph(x, y, code, face, sc, ink, screen)
            x += face.w * sc
        return x

    def lines(self, x, y, block, face=BIG, sc=2, lead=None, ink=INK):
        lead = lead or face.h * sc
        for i, ln in enumerate(block.split('\n')):
            self.text(x, y + i * lead, ln, face, sc, ink)
        return y + len(block.split('\n')) * lead

    def box(self, x, y, w, h, ink=INK):
        for Y in range(max(0, y), min(self.h, y + h)):
            for X in range(max(0, x), min(self.w, x + w)):
                self.px[X, Y] = ink

    def frame(self, x, y, w, h, t=3):
        self.box(x, y, w, t); self.box(x, y + h - t, w, t)
        self.box(x, y, t, h); self.box(x + w - t, y, t, h)

    def tone(self, x, y, w, h, level, sc=1):
        """Fill with the deck's own dither tile at this level (0-8)."""
        gw, gh = BIG.w * sc, BIG.h * sc
        for cy in range(y, y + h, gh):
            for cx in range(x, x + w, gw):
                self.glyph(cx, cy, 128 + level, BIG, sc,
                           screen=lambda X, Y: X < x + w and Y < y + h)

    def cells(self, x, y, path, sc=1, crop=None):
        """A frame from the deck's picture engine, in the deck's tiles."""
        d = open(path, 'rb').read()
        w, h = d[0], d[1]
        c = d[2:]
        x0, y0, x1, y1 = crop or (0, 0, w, h)
        for cy in range(y0, y1):
            for cx in range(x0, x1):
                code = c[cy * w + cx]
                if code not in (32, 128):
                    self.glyph(x + (cx - x0) * BIG.w * sc, y + (cy - y0) * BIG.h * sc,
                               code, BIG, sc)
        return (x1 - x0) * BIG.w * sc, (y1 - y0) * BIG.h * sc

    def rotated(self, x, y, s, face=SMALL, sc=2, angle=90):
        """A line set sideways, for the margins."""
        s = cite(s)
        tmp = Page(len(s) * face.w * sc, face.h * sc)
        tmp.text(0, 0, s, face, sc)
        r = tmp.im.rotate(angle, expand=True)
        mask = r.point(lambda v: 255 if v == 0 else 0).convert('1')
        self.im.paste(0, (x, y), mask)

    def marker(self, n):
        """The page as a step: page three of eight is ..x....."""
        steps = ''.join('x' if i == n - 1 else '.' for i in range(8))
        self.text(M, H - M - 24, steps, SMALL, 2)
        self.text(W - M - 12 * 3, H - M - 24, f'{n}/8', SMALL, 2)


def halftone(X, Y):
    return (X + Y) % 3 == 0


def cite(s):
    """[@toplap] -> [17]: numbers follow the order of zine_refs.REFS."""
    from zine_refs import REFS
    keys = [k for k, _ in REFS]
    def one(m):
        k = m.group(1)
        if k not in keys:
            raise KeyError(f'no reference {k}')
        return f'[{keys.index(k) + 1}]'
    return re.sub(r'\[@([a-z0-9]+)\]', one, s)


# ------------------------------------------------------------------ build

def build(art, out):
    from zine_pages import PAGES
    pages = []
    for i, fn in enumerate(PAGES, 1):
        p = Page()
        fn(p, art)
        p.marker(i)
        pages.append(p.im)
        p.im.save(os.path.join(out, f'hello-{i}.png'), dpi=(300, 300))
    pages[0].save(os.path.join(out, 'hello-pages.pdf'), save_all=True,
                  append_images=pages[1:], resolution=300)
    return impose(pages, out)


def impose(pages, out):
    """One US Letter sheet, landscape: four panels by two, the top row turned
    upside down, so one fold, one cut and one more fold make the booklet."""
    sheet = Image.new('1', (3300, 2550), PAPER)
    # see zine_pages.IMPOSITION for the order and its source
    from zine_pages import IMPOSITION
    for row, panels in enumerate(IMPOSITION):
        for col, (page, flip) in enumerate(panels):
            im = pages[page - 1]
            if flip:
                im = im.rotate(180)
            sheet.paste(im, (col * W, row * H))
    d = ImageDraw.Draw(sheet)
    for col in range(1, 4):                                   # fold lines, dotted
        for y in range(0, 2550, 24):
            d.line([(col * W, y), (col * W, y + 8)], fill=0)
    for x in range(0, 3300, 24):
        d.line([(x, H), (x + 8, H)], fill=0)
    d.line([(W, H), (3 * W, H)], fill=0, width=3)             # the one cut
    sheet.save(os.path.join(out, 'hello-print.pdf'), resolution=300)
    return sheet


if __name__ == '__main__':
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    build(sys.argv[1], sys.argv[2])
