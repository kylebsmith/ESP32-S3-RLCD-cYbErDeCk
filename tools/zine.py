#!/usr/bin/env python3
"""zine.py ART_DIR OUT_DIR - set the hello zine in the deck's own faces.

Sixteen pages, half letter (5.5 x 8.5 in) at 300 dpi, in two inks: black, and a
red for what the deck lights - the step you are on, its replies, the numbers.
Each ink is one bit deep, like the panel, and each is its own plate, so the
PDFs are two stencils a page, black over red, and the plates also come apart
for a riso print shop. Body text is the deck's 12x24 face at twice its size - thirty columns,
the chunky screen's line length, as one narrow column on a wide page - and
notes are its 6x12 face, sixty columns, the dense screen's. The pictures are
the deck's own: cells drawn by firmware/components/viz/viz.c through
tools/zine_art.c, set in the same tiles the panel uses. Nothing here is a
typeface from anywhere else.

Writes OUT_DIR/hello-pages.pdf (reading order), OUT_DIR/hello-booklet.pdf
(US Letter landscape, two pages a side: print both sides, flip on the short
edge, fold, staple), the booklet's two plates as hello-plate-black.pdf and
hello-plate-red.pdf, and a PNG per page.
"""
import os, re, sys, zlib
from PIL import Image, ImageDraw

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIG_SRC = os.path.join(REPO, 'firmware/components/textgrid/font12x24.c')
SMALL_SRC = os.path.join(REPO, 'firmware/components/textgrid/font6x12.c')

W, H = 1650, 2550           # a page: 5.5 x 8.5 in at 300 dpi
M = 200                     # the left margin: the column starts here
T = 250                     # the top margin
INK, PAPER, RED = 0, 1, 2
RGB_RED = (227, 6, 19)          # a Swiss red; the plate is printed in whatever red the shop has
PALETTE = [0, 0, 0, 255, 255, 255, *RGB_RED] + [0] * (256 * 3 - 9)


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
        self.im = Image.new('P', (w, h), PAPER)
        self.im.putpalette(PALETTE)
        self.px = self.im.load()

    def dot(self, x, y, ink):
        if 0 <= x < self.w and 0 <= y < self.h:
            if ink == -RED:                      # red under: only where it is paper
                if self.px[x, y] == PAPER:
                    self.px[x, y] = RED
            else:
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

    def frame(self, x, y, w, h, t=3, ink=INK):
        self.box(x, y, w, t, ink); self.box(x, y + h - t, w, t, ink)
        self.box(x, y, t, h, ink); self.box(x + w - t, y, t, h, ink)

    def tone(self, x, y, w, h, level, sc=1, ink=INK):
        """Fill with the deck's own dither tile at this level (0-8)."""
        gw, gh = BIG.w * sc, BIG.h * sc
        for cy in range(y, y + h, gh):
            for cx in range(x, x + w, gw):
                self.glyph(cx, cy, 128 + level, BIG, sc, ink,
                           screen=lambda X, Y: X < x + w and Y < y + h)

    def cells(self, x, y, path, sc=1, crop=None, ink=INK):
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
                               code, BIG, sc, ink)
        return (x1 - x0) * BIG.w * sc, (y1 - y0) * BIG.h * sc

    def rotated(self, x, y, s, face=SMALL, sc=2, angle=90):
        """A line set sideways, for the margins."""
        s = cite(s)
        tmp = Page(len(s) * face.w * sc, face.h * sc)
        tmp.text(0, 0, s, face, sc)
        r = tmp.im.rotate(angle, expand=True)
        mask = r.convert('L').point(lambda v: 255 if v == 0 else 0).convert('1')
        self.im.paste(INK, (x, y), mask)

    def marker(self, n, total=16, ink=INK):
        """The page as a step: page five of sixteen is ....x........... - and
        the step it is on is red, as the deck lights a lane's step."""
        x = M
        for i in range(total):
            on = (i == n - 1)
            x = self.text(x, H - 190, 'x' if on else '.', SMALL, 2, RED if on else ink)

    def numeral(self, n, right, ink_under=True):
        """The page's number, huge and red, at the foot of the outer edge and
        a little off it: the deck's own 12 x 24 face at eighteen times, under
        the black where they cross, and clear of the titles' red replies."""
        s, digits = 18, f'{n:02d}'
        wdt = len(digits) * BIG.w * s
        x = self.w - wdt * 88 // 100 if right else -wdt * 12 // 100
        self.text(x, self.h - 190 - BIG.h * s - 24, digits, BIG, s, -RED if ink_under else RED)


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

def plates(im):
    """A page's two stencils, '1' images with the ink black: red, then black."""
    red = im.point(lambda v: 0 if v == RED else 255, 'L').convert('1')
    black = im.point(lambda v: 0 if v == INK else 255, 'L').convert('1')
    return red, black


def write_pdf(path, images, w, h, inks=(RGB_RED, (0, 0, 0))):
    """Each page two image masks, painted in their inks one over the other -
    lossless, one bit a plate, FlateDecode. `images` are lists of '1' plates in
    the order of `inks`."""
    objs, pages = [], []
    def add(b):
        objs.append(b)
        return len(objs)
    pw, ph = w * 72 / 300, h * 72 / 300
    catalog = add(None)
    tree = add(None)
    for plates_ in images:
        names, draw = [], []
        for k, (plate, ink) in enumerate(zip(plates_, inks)):
            raw = zlib.compress(plate.tobytes(), 9)
            xo = add(b'<< /Type /XObject /Subtype /Image /Width %d /Height %d /ImageMask true '
                     b'/BitsPerComponent 1 /Filter /FlateDecode /Length %d >>\nstream\n' %
                     (plate.width, plate.height, len(raw)) + raw + b'\nendstream')
            names.append(b'/P%d %d 0 R' % (k, xo))
            draw.append(b'q %.3f %.3f %.3f rg %.2f 0 0 %.2f 0 0 cm /P%d Do Q' %
                        (ink[0] / 255, ink[1] / 255, ink[2] / 255, pw, ph, k))
        content = b'\n'.join(draw)
        c = add(b'<< /Length %d >>\nstream\n' % len(content) + content + b'\nendstream')
        pages.append(add(b'<< /Type /Page /Parent %d 0 R /MediaBox [0 0 %.2f %.2f] '
                         b'/Resources << /XObject << %s >> >> /Contents %d 0 R >>' %
                         (tree, pw, ph, b' '.join(names), c)))
    objs[catalog - 1] = b'<< /Type /Catalog /Pages %d 0 R >>' % tree
    objs[tree - 1] = b'<< /Type /Pages /Kids [%s] /Count %d >>' % (
        b' '.join(b'%d 0 R' % p for p in pages), len(pages))
    out = bytearray(b'%PDF-1.4\n%\xe2\xe3\xcf\xd3\n')
    offs = []
    for i, o in enumerate(objs, 1):
        offs.append(len(out))
        out += b'%d 0 obj\n' % i + o + b'\nendobj\n'
    xref = len(out)
    out += b'xref\n0 %d\n0000000000 65535 f \n' % (len(objs) + 1)
    for off in offs:
        out += b'%010d 00000 n \n' % off
    out += b'trailer\n<< /Size %d /Root %d 0 R >>\nstartxref\n%d\n%%%%EOF\n' % (
        len(objs) + 1, catalog, xref)
    open(path, 'wb').write(bytes(out))


def build(art, out):
    from zine_pages import PAGES
    pages = []
    for i, fn in enumerate(PAGES, 1):
        p = Page()
        fn(p, art)
        if i not in (1, len(PAGES)):
            p.numeral(i, right=(i % 2 == 1), ink_under=(fn.__name__ != 'room'))
        p.marker(i, len(PAGES), ink=PAPER if fn.__name__ == 'room' else INK)
        pages.append(p.im)
        p.im.save(os.path.join(out, f'hello-{i:02d}.png'), dpi=(300, 300))
    write_pdf(os.path.join(out, 'hello-pages.pdf'), [plates(im) for im in pages], W, H)
    return impose(pages, out)


def impose(pages, out):
    """A saddle-stitched booklet: US Letter landscape, two pages a side. For
    sixteen pages the sides are 16|1, 2|15, 14|3, 4|13, 12|5, 6|11, 10|7,
    8|9 - print both sides flipping on the short edge, stack in order, fold
    and staple at the fold."""
    n = len(pages)
    assert n % 4 == 0, 'a booklet is a multiple of four pages'
    sides = []
    lo, hi = 1, n
    while lo < hi:
        sides.append((hi, lo)); sides.append((lo + 1, hi - 1))
        lo += 2; hi -= 2
    sheets = []
    for left, right in sides:
        sheet = Image.new('P', (2 * W, H), PAPER)
        sheet.putpalette(PALETTE)
        sheet.paste(pages[left - 1], (0, 0))
        sheet.paste(pages[right - 1], (W, 0))
        sheets.append(sheet)
    both = [plates(sh) for sh in sheets]
    write_pdf(os.path.join(out, 'hello-booklet.pdf'), both, 2 * W, H)
    # the plates apart, each printed black, for a riso drum apiece
    write_pdf(os.path.join(out, 'hello-plate-red.pdf'), [(r,) for r, _ in both], 2 * W, H, inks=((0, 0, 0),))
    write_pdf(os.path.join(out, 'hello-plate-black.pdf'), [(b,) for _, b in both], 2 * W, H, inks=((0, 0, 0),))
    return sheets


if __name__ == '__main__':
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    build(sys.argv[1], sys.argv[2])
