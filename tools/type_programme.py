#!/usr/bin/env python3
"""type_programme.py OUT_DIR - a speculative face for the deck, drawn by a programme.

SPECULATIVE. Nothing here reaches the firmware. The deck's 12 x 24 face is laid by
hand in tools/font12x24_art.py; this asks what it becomes if every curve in it is
one curve - the superellipse |x/a|^n + |y/b|^n = 1 the enclosure's corners are cut
with (docs/CMF.md) - drawn by rule instead of by hand. Change n and every round
letter changes with it: Gerstner's "designing programmes" (1964), in pixels.

PASS 1 found that at the panel's 12 x 24 the exponent moves a pixel or two, and
that today's hand-laid bowls already sit at about n = 3.2 - the face and the case
were drawn to the same corner without anyone deciding it. The curves show when the
same rule draws LARGER: a section line at twice the size, or the view node's screen.
I chose n = 2.2, the round one (2026-09-28).

PASS 2 keeps n = 2.2 and asks for more than a rounder corner. Every glyph - the
punctuation the language lives on included (> . % : [ ] < ! /) - is drawn under a
STYLE, a handful of rules layered on the one curve:

    round     the base: n = 2.2, round dots, so a rest is a bead: x...x...
    open      wider apertures, and i j l t spread across the cell - the change
              Beier & Larson (2010) found helps frequently misread letters
    traps     a notch where strokes meet, to keep joins crisp on a dim panel
    stencil   bowls and joins broken - industrial, after Crouwel's grids
    slab      typewriter serifs: the terminal's own tradition, and the
              strongest 1 / l / I separation there is
    oblique   the round face slanted one pixel in eight - a voice for comments
    inline    hollow strokes, for section lines at twice the size only

None of it costs the deck anything to run: whatever draws a glyph, the deck keeps
a bitmap - 36 bytes and 3.8 us a cell at 12 x 24, 144 bytes at 24 x 48.

Rules every style keeps: the 12 x 24 cell (every size a multiple of it); the body
in columns 0-9, the gap in 10-11; cap height rows 4-19, x-height 10-19, descenders
to 22; strokes of 2 units; the dotted zero, full width (my choice).
Anything a style does not draw is today's glyph, scaled.

    type-programme.png   pass 1: today against n = 2.2, 3.2 and 6
    type-variants.png    pass 2: the styles on n = 2.2, at the panel's size, at
                         twice it, and setting the language
"""
import math, os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from zine import BIG, SMALL, Page   # noqa: E402

W, H, ST = 12, 24, 2                     # the cell and the stroke, in units
CAP, XH, BASE, DESC = 4, 10, 19, 22      # top of caps, top of x-height, last row on the
                                         # baseline, last descender row - in units


class Style:
    def __init__(self, name, n=2.2, **flags):
        self.name, self.n = name, n
        self.open = flags.get('open', False)
        self.traps = flags.get('traps', False)
        self.stencil = flags.get('stencil', False)
        self.slab = flags.get('slab', False)
        self.oblique = flags.get('oblique', False)
        self.inline = flags.get('inline', False)
        self.plain = flags.get('plain', False)   # pass 1: round letters only


_CIRCLE = [((ST - 0.02) * math.cos(t * math.pi / 16), (ST - 0.02) * math.sin(t * math.pi / 16))
           for t in range(32)]


class Glyph:
    """A cell drawn at k pixels a unit. Every call takes unit coordinates; curves
    are sampled at pixel centres, so a larger k draws a finer curve."""

    def __init__(self, k):
        self.k = k
        self.px = [[False] * (W * k) for _ in range(H * k)]

    def _span(self, x0, y0, x1, y1):
        k = self.k
        return (range(max(0, y0 * k), min(H * k, (y1 + 1) * k)),
                range(max(0, x0 * k), min(W * k, (x1 + 1) * k)))

    def ring(self, x0, y0, x1, y1, n, keep=None, solid=False):
        """A superellipse ring of stroke ST in the unit box x0..x1, y0..y1
        (inclusive) - or, solid, the whole shape. keep(dx, dy), from the centre
        in units, may refuse a pixel: that is how apertures are cut. The inner
        curve is squarer (n + 2): an offset of a superellipse is not one, and
        the same n inside thickens every corner to three pixels."""
        k = self.k
        cx, cy = (x0 + x1 + 1) / 2.0, (y0 + y1 + 1) / 2.0
        a, b = (x1 - x0 + 1) / 2.0, (y1 - y0 + 1) / 2.0
        rows, cols = self._span(x0, y0, x1, y1)
        for r in rows:
            for c in cols:
                dx, dy = (c + 0.5) / k - cx, (r + 0.5) / k - cy
                u, v = abs(dx / a), abs(dy / b)
                F = u ** n + v ** n
                if F > 1.0:
                    continue
                if not solid:
                    # A pixel is stroke if it lies within ST of the edge: if a
                    # circle of radius ST round it reaches outside the curve.
                    # Exact for any convex shape, so the stroke is even at every
                    # exponent - a second, shrunken curve is not an offset curve.
                    if not any(abs((dx + ox) / a) ** n + abs((dy + oy) / b) ** n > 1.0
                               for ox, oy in _CIRCLE):
                        continue
                if keep is None or keep(dx, dy):
                    self.px[r][c] = True

    def stroke(self, x0, y0, x1, y1, w=ST):
        """A straight stroke of width w between two unit points, round-ended:
        every pixel whose centre lies within w/2 of the segment."""
        k = self.k
        vx, vy = x1 - x0, y1 - y0
        L2 = vx * vx + vy * vy or 1e-9
        for r in range(H * k):
            for c in range(W * k):
                px_, py_ = (c + 0.5) / k, (r + 0.5) / k
                t = max(0.0, min(1.0, ((px_ - x0) * vx + (py_ - y0) * vy) / L2))
                qx, qy = x0 + t * vx, y0 + t * vy
                if (px_ - qx) ** 2 + (py_ - qy) ** 2 <= (w / 2.0) ** 2:
                    self.px[r][c] = True

    def fill(self, x0, y0, x1, y1, on=True):
        rows, cols = self._span(x0, y0, x1, y1)
        for r in rows:
            for c in cols:
                self.px[r][c] = on

    def clear(self, x0, y0, x1, y1):
        self.fill(x0, y0, x1, y1, False)

    def notch(self, ux, uy):
        """One pixel cut out at the unit corner point (ux, uy): an ink trap."""
        k = self.k
        c, r = int(ux * k), int(uy * k)
        for rr in (r - 1, r):
            for cc in (c - 1, c):
                if 0 <= rr < H * k and 0 <= cc < W * k and k == 1 and (rr, cc) == (r - 1, c - 1):
                    self.px[rr][cc] = False
        if k > 1:
            self.px[r - 1][c - 1] = False


def today(ch, k):
    """Today's hand-laid glyph, each pixel drawn k x k."""
    rows = BIG.g.get(ord(ch)) or BIG.g[ord('?')]
    return [[bool(rows[r // k] & (0x8000 >> (c // k))) for c in range(W * k)]
            for r in range(H * k)]


def draw(ch, st, k):
    """The programme's glyph for ch in style st, or None where it keeps today's."""
    g = Glyph(k)
    n = st.n
    ap = 1.0 if st.open else 0.0                 # how much wider an aperture opens

    def R(x0, y0, x1, y1, keep=None, closed=False, solid=False):
        kp = keep
        if st.stencil and closed and not solid:
            half_b = (y1 - y0 + 1) / 2.0
            def kp(dx, dy, keep=keep, hb=half_b):
                if abs(dx) < 1.0 and abs(dy) > hb - ST - 0.5:
                    return False                 # the stencil's bridge, top and bottom
                return keep is None or keep(dx, dy)
        g.ring(x0, y0, x1, y1, n, kp, solid)

    def dot(x0, y0, x1, y1):
        R(x0, y0, x1, y1, solid=True)

    def slab_foot(sx, y=BASE, left=2, right=2):
        if st.slab:
            g.fill(max(0, sx - left), y - 1, min(9, sx + 1 + right), y)

    def slab_head(sx, y, left=2):
        if st.slab:
            g.fill(max(0, sx - left), y, sx + 1, y + 1)

    def bridge(x0, y0, x1, y1):
        if st.stencil:
            g.clear(x0, y0, x1, y1)

    def trap(ux, uy):
        if st.traps:
            g.notch(ux, uy)

    # ---- the round lowercase
    if ch == 'o':
        R(0, XH, 9, BASE, closed=True)
    elif ch == 'c':
        R(0, XH, 9, BASE, keep=lambda dx, dy: not (dx > 0 and abs(dy) < 2.5 + ap))
    elif ch == 'e':
        R(0, XH, 9, BASE, closed=True, keep=lambda dx, dy: not (dx > 0 and 0.5 < dy < 3.2 + ap))
        g.fill(1, 14, 9, 15)
        bridge(0, 14, 0, 15)
    elif ch in 'bdpq':
        R(0, XH, 9, BASE, closed=True)
        sx = 0 if ch in 'bp' else 8
        top = CAP if ch in 'bd' else XH
        bot = DESC if ch in 'pq' else BASE
        g.fill(sx, top, sx + 1, bot)
        inner = 2 if ch in 'bp' else 7
        bridge(inner, XH, inner, XH + 1)
        bridge(inner, BASE - 1, inner, BASE)
        trap(2 if ch in 'bp' else 8, XH + 2)
        trap(2 if ch in 'bp' else 8, BASE - 1)
        if ch in 'pq':
            slab_foot(sx, DESC)
        if ch == 'b':
            slab_head(0, CAP)
        if ch == 'd':
            slab_foot(8, BASE, left=0)
    elif ch in 'nh':
        g.fill(0, CAP if ch == 'h' else XH, 1, BASE)
        R(0, XH, 9, XH + 9, keep=lambda dx, dy: dy < 0)
        g.fill(8, XH + 5, 9, BASE)
        bridge(2, XH, 2, XH + 1)
        trap(2, XH + 2)
        slab_foot(0); slab_foot(8)
        slab_head(0, CAP if ch == 'h' else XH)
    elif ch == 'm':
        g.fill(0, XH, 1, BASE)
        R(0, XH, 5, XH + 7, keep=lambda dx, dy: dy < 0)
        R(4, XH, 9, XH + 7, keep=lambda dx, dy: dy < 0)
        g.fill(4, XH + 3, 5, BASE)
        g.fill(8, XH + 3, 9, BASE)
        bridge(2, XH, 2, XH + 1); bridge(6, XH, 6, XH + 1)
        if st.slab:
            g.fill(0, BASE - 1, 9, BASE); g.clear(2, BASE - 1, 3, BASE); g.clear(6, BASE - 1, 7, BASE)
    elif ch == 'u':
        R(0, BASE - 9, 9, BASE, keep=lambda dx, dy: dy > 0)
        g.fill(0, XH, 1, BASE - 5)
        g.fill(8, XH, 9, BASE)
        bridge(7, BASE - 1, 7, BASE)
        trap(8, BASE - 1)
        slab_head(0, XH); slab_head(8, XH); slab_foot(8, BASE, left=0)
    elif ch == 'r':
        g.fill(0, XH, 1, BASE)
        R(0, XH, 9, XH + 9, keep=lambda dx, dy: dy < -1.5 and dx < 3.5)
        bridge(2, XH, 2, XH + 1)
        trap(2, XH + 2)
        slab_foot(0, BASE, left=0, right=4); slab_head(0, XH)
    elif ch == 'a':
        R(0, XH, 9, XH + 9, keep=lambda dx, dy: dy < -1 and not (dx < -1.5 and dy > -3.5 - ap))
        g.fill(8, XH + 4, 9, BASE)
        R(0, 14, 9, BASE, closed=True)
        trap(8, 15)
        slab_foot(8, BASE, left=0)
    elif ch == 's':
        R(0, XH, 9, 15, keep=lambda dx, dy: dy < 0 or dx < 0)
        R(0, 14, 9, BASE, keep=lambda dx, dy: dy > 0 or dx > 0)
        g.clear(8, 12, 9, 13 + int(ap)); g.clear(0, 16 - int(ap), 1, 17)
        bridge(4, 14, 5, 15)
    elif ch == 'g':
        R(0, XH, 9, 18, closed=True)
        g.fill(8, XH, 9, 20)
        R(0, 16, 9, DESC, keep=lambda dx, dy: dy > 1.2)
        trap(8, XH + 2)
    elif ch == 't':
        g.fill(3, 6, 4, 16)
        g.fill(0 if st.open else 0, XH, 9 if st.open else 8, XH + 1)
        R(3, 12, 9, BASE, keep=lambda dx, dy: dy > 1.2 and dx < 2.4 + ap)
        bridge(5, XH, 5, XH + 1)
    elif ch == 'f':
        g.fill(3, 8, 4, BASE)
        g.fill(0, XH, 9 if st.open else 8, XH + 1)
        R(3, CAP, 9, 12, keep=lambda dx, dy: dy < -1.2 and dx < 2.4 + ap)
        slab_foot(3, BASE, left=2, right=2)
    elif ch == 'j':
        dot(5, 5, 8, 7)
        g.fill(6, XH, 7, 20)
        R(0, 14, 7, DESC, keep=lambda dx, dy: dy > 1.2)
        slab_head(6, XH, left=3)
    elif ch == 'i':
        dot(3, 5, 6, 7)
        g.fill(4, XH, 5, BASE)
        if st.open or st.slab:
            g.fill(1, XH, 5, XH + 1)             # the flag spreads it across the cell
            g.fill(1, BASE - 1, 8, BASE)         # and the foot
    elif ch == 'l':
        if st.open or st.slab:
            g.fill(3, CAP, 4, BASE - 3)
            g.fill(1, CAP, 4, CAP + 1)
            R(3, 12, 9, BASE, keep=lambda dx, dy: dy > 1.2 and dx < 2.4)
            g.fill(3, 12, 4, 16)
        else:
            return None
    # ---- capitals and figures on the same curve
    elif ch == 'O':
        R(0, CAP, 9, BASE, closed=True)
    elif ch == '0':
        R(0, CAP, 9, BASE, closed=True)
        dot(4, 11, 5, 12)                            # the dot stays
    elif ch == 'C':
        R(0, CAP, 9, BASE, keep=lambda dx, dy: not (dx > 0 and abs(dy) < 3.5 + ap))
    elif ch == 'G':
        R(0, CAP, 9, BASE, keep=lambda dx, dy: not (dx > 0 and -3.5 - ap < dy < 0.5))
        g.fill(5, 12, 9, 13); g.fill(8, 12, 9, 16)
    elif ch == 'D':
        R(0, CAP, 9, BASE, closed=True)
        g.fill(0, CAP, 1, BASE); g.fill(0, CAP, 4, CAP + 1); g.fill(0, BASE - 1, 4, BASE)
    elif ch == 'Q':
        R(0, CAP, 9, BASE, closed=True)
        g.fill(6, 17, 7, 18); g.fill(8, 19, 9, 21)
    elif ch == 'S':
        R(0, CAP, 9, 12, keep=lambda dx, dy: dy < 0 or dx < 0)
        R(0, 11, 9, BASE, keep=lambda dx, dy: dy > 0 or dx > 0)
        g.clear(8, 7, 9, 8 + int(ap)); g.clear(0, 15 - int(ap), 1, 16)
    elif ch == 'U':
        R(0, BASE - 11, 9, BASE, keep=lambda dx, dy: dy > 0)
        g.fill(0, CAP, 1, BASE - 6); g.fill(8, CAP, 9, BASE - 6)
    elif ch == '8':
        R(1, CAP, 8, 12, closed=True); R(0, 11, 9, BASE, closed=True)
    elif ch == '6':
        R(0, 10, 9, BASE, closed=True)
        R(0, CAP, 9, 17, keep=lambda dx, dy: dy < 0 and not (dx > 1 and dy > -4 - ap))
        g.fill(0, 9, 1, 14)
    elif ch == '9':
        R(0, CAP, 9, 13, closed=True)
        R(0, 6, 9, BASE, keep=lambda dx, dy: dy > 0 and not (dx < -1 and dy < 4 + ap))
        g.fill(8, 9, 9, 14)
    elif ch == '3':
        R(0, CAP, 9, 12, keep=lambda dx, dy: dy < 0 or dx > 0)
        R(0, 11, 9, BASE, keep=lambda dx, dy: dy > 0 or dx > 0)
        g.clear(0, 7, 1, 8); g.clear(0, 11, 3, 12); g.clear(0, 15, 1, 16)
    elif ch == '2':
        R(0, CAP, 9, 13, keep=lambda dx, dy: dy < 0 or (dx > 0 and dy < 2))
        g.clear(0, 8, 1, 9)
        g.stroke(7.5, 12.5, 1.0, 18.0)
        g.fill(0, 18, 9, BASE)
    elif ch == '5':
        g.fill(0, CAP, 9, CAP + 1); g.fill(0, CAP, 1, 12)
        R(0, 10, 9, BASE, keep=lambda dx, dy: not (dx < -1 and -2.5 < dy < 2.0 + ap))
    elif ch == '1' and st.slab:
        g.fill(4, CAP, 5, BASE); g.fill(2, CAP, 5, CAP + 1); g.fill(1, BASE - 1, 8, BASE)
    elif ch == 'I' and st.slab:
        g.fill(4, CAP, 5, BASE); g.fill(1, CAP, 8, CAP + 1); g.fill(1, BASE - 1, 8, BASE)
    # ---- the punctuation the language lives on
    elif st.plain:
        return None
    elif ch == '.':
        dot(3, 16, 6, 19)                        # a rest is a bead
    elif ch == ':':
        dot(3, 10, 6, 13); dot(3, 16, 6, 19)
    elif ch == ';':
        dot(3, 10, 6, 13); dot(3, 16, 6, 19); g.stroke(5.5, 18.5, 3.5, 22.0)
    elif ch == ',':
        dot(3, 16, 6, 19); g.stroke(5.5, 18.5, 3.5, 22.0)
    elif ch == '>':
        g.stroke(1.5, 6.0, 8.0, 12.0, 3.0); g.stroke(8.0, 12.0, 1.5, 18.0, 3.0)
    elif ch == '<':
        g.stroke(8.5, 6.0, 2.0, 12.0, 3.0); g.stroke(2.0, 12.0, 8.5, 18.0, 3.0)
    elif ch == '%':
        R(0, CAP, 4, 9, closed=False); R(5, 14, 9, BASE, closed=False)
        g.stroke(8.5, 5.0, 1.5, 18.5)
    elif ch == '(':
        R(3, CAP - 1, 12, BASE + 2, keep=lambda dx, dy: dx < -1.5)
    elif ch == ')':
        R(-3, CAP - 1, 6, BASE + 2, keep=lambda dx, dy: dx > 1.5)
    elif ch == '{':
        R(3, CAP - 1, 12, 10, keep=lambda dx, dy: dx < 0 and dy < 0)
        g.fill(3, 7, 4, 10); g.fill(1, 11, 4, 12); g.fill(3, 13, 4, 16)
        R(3, 13, 12, BASE + 1, keep=lambda dx, dy: dx < 0 and dy > 0)
    elif ch == '}':
        R(-3, CAP - 1, 6, 10, keep=lambda dx, dy: dx > 0 and dy < 0)
        g.fill(5, 7, 6, 10); g.fill(5, 11, 8, 12); g.fill(5, 13, 6, 16)
        R(-3, 13, 6, BASE + 1, keep=lambda dx, dy: dx > 0 and dy > 0)
    elif ch == '!':
        g.stroke(4.5, CAP + 0.5, 4.5, 14.0, 2.4); dot(3, 16, 6, 19)
    elif ch == '?':
        R(0, CAP, 9, 12, keep=lambda dx, dy: dy < 0 or dx > 1)
        g.clear(0, 8, 1, 9)
        g.fill(4, 12, 5, 14); dot(3, 16, 6, 19)
    elif ch == '~':
        R(0, 11, 5, 16, keep=lambda dx, dy: dy < 0)
        R(4, 13, 9, 18, keep=lambda dx, dy: dy > 0)
    elif ch == '@':
        R(0, CAP, 9, BASE, keep=lambda dx, dy: not (dx > 1 and dy > 2))
        R(3, 9, 7, 15, closed=False)
        g.fill(6, 9, 7, 16)
    elif ch == '$':
        R(0, CAP + 1, 9, 12, keep=lambda dx, dy: dy < 0 or dx < 0)
        R(0, 11, 9, BASE - 1, keep=lambda dx, dy: dy > 0 or dx > 0)
        g.clear(8, 7, 9, 8); g.clear(0, 14, 1, 15)
        g.fill(4, CAP - 1, 5, BASE + 1)
    elif ch == '/':
        g.stroke(8.5, CAP, 1.5, BASE + 0.5)
    else:
        return None
    return g.px


def shear(px, k):
    """Slant: each row moved right one pixel in eight above the baseline."""
    base = (BASE + 1) * k
    out = [[False] * (W * k) for _ in range(H * k)]
    for r, row in enumerate(px):
        s = max(0, (base - r) // (8 * 1))
        s = s * k // k
        for c, on in enumerate(row):
            if on and 0 <= c + s // k < W * k:
                out[r][min(W * k - 1, c + (base - r) // 8)] = True
    return out


def _erode(px):
    h, w = len(px), len(px[0])
    return [[px[r][c] and all(0 <= r + dr < h and 0 <= c + dc < w and px[r + dr][c + dc]
                              for dr, dc in ((1, 0), (-1, 0), (0, 1), (0, -1)))
             for c in range(w)] for r in range(h)]


def hollow(px):
    """Inline: each stroke keeps a two-pixel edge and loses its core. It needs a
    stroke of six pixels or more to have a core - three times the size."""
    core = _erode(_erode(px))
    return [[px[r][c] and not core[r][c] for c in range(len(px[0]))] for r in range(len(px))]


def glyph(ch, st, k):
    px = draw(ch, st, k) or today(ch, k)
    if st.oblique:
        px = shear(px, k)
    if st.inline and k >= 3:
        px = hollow(px)
    return px


# ------------------------------------------------------------------ figures

def put(p, x, y, px, s=1):
    for r, row in enumerate(px):
        for c, on in enumerate(row):
            if on:
                p.box(x + c * s, y + r * s, s, s)


def line(p, x, y, text, st, k, s=1):
    for i, ch in enumerate(text):
        put(p, x + i * W * k * s, y, glyph(ch, st, k) if st else today(ch, k), s)


def label(p, x, y, s):
    for i, ch in enumerate(s):
        p.glyph(x + i * SMALL.w * 2, y, ord(ch), SMALL, 2)


def pass1(out):
    faces = [('today, drawn by hand', None),
             ('n = 2.2 - round (chosen)', Style('round', 2.2, plain=True)),
             ('n = 3.2 - the case\'s corner', Style('case', 3.2, plain=True)),
             ('n = 6 - square', Style('square', 6.0, plain=True))]
    letters = 'abcdefghjnopqrstu 0235689 CDGOQSU'
    p = Page(1560, 4200)
    y = 20
    label(p, 20, y, 'at the panel\'s size, 12 x 24, shown 2 x: the exponent moves a pixel or two')
    y += 40
    for name, st in faces:
        label(p, 20, y, name)
        line(p, 20, y + 26, letters, st, 1, 2)
        y += 24 * 2 + 44
    y += 16
    label(p, 20, y, 'the same rule drawn at 24 x 48 - twice the size, not pixel-doubled - where the curve shows')
    y += 40
    for name, st in faces:
        label(p, 20, y, name + ('  (today\'s, pixel-doubled)' if st is None else ''))
        line(p, 20, y + 26, 'orbitals 0268', st, 2, 1)
        y += 48 + 44
    y += 16
    label(p, 20, y, 'drawn at 48 x 96 - the size a view node could draw it on a big screen')
    y += 40
    x = 20
    for name, st in faces:
        label(p, x, y, 'today' if st is None else name.split(' - ')[0])
        line(p, x, y + 26, 'o0g8S', st, 4, 1)
        x += 5 * W * 4 + 20
    y += 96 + 60
    p.im.crop((0, 0, p.w, y)).save(os.path.join(out, 'type-programme.png'))


def pass2(out):
    styles = [Style('round'), Style('open', open=True), Style('traps', traps=True),
              Style('stencil', stencil=True), Style('slab', slab=True),
              Style('oblique', oblique=True), Style('inline', inline=True)]
    lower = 'abcdefghijklmnopqrstuvwxyz'
    figs = '0123456789  > < % . : ; , ( ) { } ! ? ~ @ $ / *'
    code = ['>kick 9...8...9...8...', '>hat x%70x%40 *2 !4', '>disc:x 8876532111235678',
            '>spin <0 3 6 9>  >echo 8']
    p = Page(1760, 5200)
    y = 20
    label(p, 20, y, 'pass 2 - seven styles on n = 2.2, at the panel\'s size (shown 2 x), the '
                    'language set in each, and a section line drawn at twice the size')
    y += 44
    rows = [('today, drawn by hand', None)] + [(s.name, s) for s in styles]
    for name, st in rows:
        label(p, 20, y, name)
        line(p, 20, y + 26, lower, st, 1, 2)
        line(p, 20, y + 26 + 52, figs, st, 1, 2)
        for i, t in enumerate(code):
            line(p, 20 + (i % 2) * 560, y + 26 + 112 + (i // 2) * 28, t, st, 1, 1)
        big = 3 if (st is not None and st.inline) else 2
        line(p, 1150, y + 26 + 104 - (24 if big == 3 else 0),
             'orbitals' if big == 3 else '-- first light', st, big, 1)
        y += 26 + 112 + 60 + 40
    p.im.crop((0, 0, p.w, y)).save(os.path.join(out, 'type-variants.png'))


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'docs/img')
    os.makedirs(out, exist_ok=True)
    pass1(out)
    pass2(out)
    print('type programme written to', out)


if __name__ == '__main__':
    main()
