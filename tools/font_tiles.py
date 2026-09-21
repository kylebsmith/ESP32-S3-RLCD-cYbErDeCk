#!/usr/bin/env python3
"""The tile glyphs: our own shapes, at codepoints 128..155.

THIS IS THE SOURCE for both faces. Unlike the letters, which are hand-laid art
in font12x24_art.py and in make_font.py's G table, a tile is GEOMETRY - so it is
generated exactly at whatever cell size is asked for rather than drawn twice and
kept in sync by hand. A 50% dither cannot survive being drawn once and
downsampled, because ORing a checkerboard together gives solid black; it has to
be computed at each size or it is not a tone.

WHY THESE AND NOT OTHERS. The visual system had five ink levels - space, '.',
':', '*', '#', '@' - borrowed from punctuation a typeface designer chose for
setting prose. They are uneven as a tonal ramp, they carry the letterforms'
sidebearings so a field of them is striped with white gutters, and there is
nothing in them that looks deliberate. These are chosen for the job:

    128..136   nine tones, an ordered dither from nothing to solid
    137..140   sparkles - a speck, a four-point star, an eight-point, a burst
    141..145   half blocks, four ways, and a centred square
    146..148   a diamond, a filled disc, a hollow ring
    149..151   diagonals, and their crossing
    152..155   quadrant arcs, which tile 2x2 into one large circle

The last group is the one that buys LARGER geometry: four cells make a circle
twice the size, and sixteen make one four times the size, because each arc meets
its neighbours exactly at the cell edge. That is a property of the generator, not
a hope about the drawing, and check_fonts.py asserts it.

A tone fills the whole cell INCLUDING the columns a letter leaves as its gap.
That is the point: a field of tone has to be seamless, and a letter has to not
touch its neighbour. Different jobs, different rules, which is why
check_fonts.py applies its gap rule only to the letters.
"""

TILE_FIRST = 128
TILE_LAST  = 155

# The same 4x4 ordered matrix the display world has used since one bit was all
# there was. It spreads set pixels as evenly as any 4x4 arrangement can, so the
# result reads as a tone at arm's length where a random choice shimmers.
BAYER = (
    ( 0,  8,  2, 10),
    (12,  4, 14,  6),
    ( 3, 11,  1,  9),
    (15,  7, 13,  5),
)


def _blank(w, h):
    return [['.'] * w for _ in range(h)]


def _rows(g):
    return [''.join(r) for r in g]


def tone(level, w, h):
    """Level 0..8 of an ordered dither. 0 is empty, 8 is solid."""
    g = _blank(w, h)
    # Nine levels over sixteen thresholds: level 8 must be every pixel, so the
    # threshold at 8 is 16 and nothing can fail it.
    t = level * 16 // 8
    for y in range(h):
        for x in range(w):
            if BAYER[y & 3][x & 3] < t:
                g[y][x] = '#'
    return _rows(g)


def _dot(g, cx, cy, r, w, h):
    for y in range(h):
        for x in range(w):
            # The cell is twice as tall as it is wide on every face here, so a
            # shape that is round on the GLASS is half as tall in pixels-per-
            # unit vertically. Counting y at half weight makes a circle round.
            dx, dy = (x - cx), (y - cy) / 2.0
            if dx * dx + dy * dy <= r * r:
                g[y][x] = '#'


def speck(w, h):
    g = _blank(w, h)
    _dot(g, (w - 1) / 2.0, (h - 1) / 2.0, max(1, w // 6), w, h)
    return _rows(g)


def star4(w, h):
    """A four-point star: two strokes crossing, tapered by length."""
    g = _blank(w, h)
    cx, cy = (w - 1) / 2.0, (h - 1) / 2.0
    arm_x, arm_y = w / 2.0, h / 2.0
    for y in range(h):
        for x in range(w):
            dx, dy = abs(x - cx) / arm_x, abs(y - cy) / arm_y
            # A four-pointed star is the set where |dx|+|dy| is small - a
            # rotated square - pulled in at the diagonals by the square root,
            # which is what makes the arms concave instead of straight.
            if (dx ** 0.5 + dy ** 0.5) <= 1.0:
                g[y][x] = '#'
    return _rows(g)


def star8(w, h):
    """Four straight arms plus four diagonal ones at half length.

    THE ARM THICKNESS IS IN PIXELS, not in fractions of the cell. Measuring it
    as a fraction made the horizontal arm four rows deep and the vertical one
    two columns wide, because the cell is twice as tall as it is wide - so the
    star had fat arms one way and thin arms the other. A stroke's weight is a
    pixel count; only its LENGTH belongs in cell-relative units.
    """
    g = _blank(w, h)
    cx, cy = (w - 1) / 2.0, (h - 1) / 2.0
    arm = max(2, w // 6)                         # stroke weight, in pixels
    for y in range(h):
        for x in range(w):
            dx, dy = abs(x - cx), abs(y - cy)
            if dx < arm / 2.0 or dy < arm / 2.0:
                g[y][x] = '#'
            # The diagonals run at the cell's own slope, so they reach the
            # corners rather than stopping short of them.
            elif abs(dx * h - dy * w) < arm * h / 2.0 and dx < w * 0.3:
                g[y][x] = '#'
    return _rows(g)


def burst(w, h):
    """A ring of specks around a centre: an explosion, one frame of it."""
    g = _blank(w, h)
    cx, cy = (w - 1) / 2.0, (h - 1) / 2.0
    _dot(g, cx, cy, max(1, w // 8), w, h)
    for y in range(h):
        for x in range(w):
            dx, dy = (x - cx) / (w / 2.0), (y - cy) / (h / 2.0)
            d = (dx * dx + dy * dy) ** 0.5
            if 0.62 < d < 0.95 and (abs(dx) < 0.2 or abs(dy) < 0.2 or
                                    abs(abs(dx) - abs(dy)) < 0.25):
                g[y][x] = '#'
    return _rows(g)


def half(which, w, h):
    g = _blank(w, h)
    for y in range(h):
        for x in range(w):
            keep = {'u': y < h // 2, 'd': y >= h // 2,
                    'l': x < w // 2, 'r': x >= w // 2}[which]
            if keep:
                g[y][x] = '#'
    return _rows(g)


def square(w, h):
    """A centred filled square - square on the GLASS, so half as tall in rows
    as it is wide in columns is wrong; it must be TWICE as tall."""
    g = _blank(w, h)
    sw = max(2, (w * 2) // 3)
    sh = sw * 2
    if sh > h:
        sh, sw = h, max(2, h // 2)
    x0, y0 = (w - sw) // 2, (h - sh) // 2
    for y in range(y0, y0 + sh):
        for x in range(x0, x0 + sw):
            g[y][x] = '#'
    return _rows(g)


def diamond(w, h):
    g = _blank(w, h)
    cx, cy = (w - 1) / 2.0, (h - 1) / 2.0
    for y in range(h):
        for x in range(w):
            if abs(x - cx) / (w / 2.0) + abs(y - cy) / (h / 2.0) <= 1.0:
                g[y][x] = '#'
    return _rows(g)


def disc(w, h):
    g = _blank(w, h)
    _dot(g, (w - 1) / 2.0, (h - 1) / 2.0, w / 2.0, w, h)
    return _rows(g)


def ring(w, h):
    g = _blank(w, h)
    _dot(g, (w - 1) / 2.0, (h - 1) / 2.0, w / 2.0, w, h)
    inner = _blank(w, h)
    _dot(inner, (w - 1) / 2.0, (h - 1) / 2.0, w / 2.0 - max(1, w // 6), w, h)
    for y in range(h):
        for x in range(w):
            if inner[y][x] == '#':
                g[y][x] = '.'
    return _rows(g)


def diagonal(down, w, h):
    """A stroke corner to corner, thick enough to survive one bit."""
    g = _blank(w, h)
    thick = max(2, w // 5)
    for y in range(h):
        # The line runs the full diagonal of the CELL, so two cells side by side
        # continue each other without a kink.
        x_at = (y * (w - 1)) / (h - 1) if h > 1 else 0
        if not down:
            x_at = (w - 1) - x_at
        for x in range(w):
            if abs(x - x_at) < thick / 2.0:
                g[y][x] = '#'
    return _rows(g)


def cross(w, h):
    g = [list(r) for r in diagonal(True, w, h)]
    b = diagonal(False, w, h)
    for y in range(h):
        for x in range(w):
            if b[y][x] == '#':
                g[y][x] = '#'
    return _rows(g)


def _dilate(g, w, h, radius):
    """Grow every set pixel by `radius` PIXELS, in both axes equally."""
    out = _blank(w, h)
    r2 = radius * radius
    span = int(radius) + 1
    for y in range(h):
        for x in range(w):
            if g[y][x] != '#':
                continue
            for oy in range(-span, span + 1):
                for ox in range(-span, span + 1):
                    if ox * ox + oy * oy > r2:
                        continue
                    yy, xx = y + oy, x + ox
                    if 0 <= yy < h and 0 <= xx < w:
                        out[yy][xx] = '#'
    return out


def arc(quadrant, w, h):
    """One quarter of a circle whose centre is a CELL CORNER and whose radius is
    the cell itself - so four of them tile 2x2 into one circle twice the size,
    and sixteen at 4x4 into one four times the size. Each arc meets its
    neighbours exactly at the cell edge, which is what makes the joins invisible.

    quadrant 0 has its centre at the bottom-right and draws the top-left
    quarter; 1, 2, 3 go round from there.

    THE STROKE IS DILATED, NOT THRESHOLDED. Testing |distance - radius| against
    a width gives a band that is thin where the curve is steep and fat where it
    is shallow - at the top of the arc, where the curve is nearly horizontal, it
    swallowed a third of the cell. Taking the true one-pixel boundary and then
    growing it by a radius in PIXELS gives an even weight all the way round,
    which is the only way a curve looks drawn rather than computed.
    """
    cx = (w - 0.5) if quadrant in (0, 3) else -0.5
    cy = (h - 0.5) if quadrant in (0, 1) else -0.5
    r = w - 0.5                      # in glass units: x counts 1, y counts 1/2

    def inside(x, y):
        dx, dy = (x - cx), (y - cy) / 2.0
        return dx * dx + dy * dy <= r * r

    edge = _blank(w, h)
    for y in range(h):
        for x in range(w):
            if not inside(x, y):
                continue
            # A boundary pixel: inside, with a neighbour outside - or against
            # the cell edge, so the arc reaches the join rather than stopping.
            if (not inside(x + 1, y) or not inside(x - 1, y) or
                    not inside(x, y + 1) or not inside(x, y - 1)):
                edge[y][x] = '#'
    return _rows(_dilate(edge, w, h, max(1.0, w / 6.0)))


def tiles(w, h):
    """Every tile glyph at this cell size, as {codepoint: [rows]}."""
    t = {}
    for lv in range(9):                          # 128..136
        t[128 + lv] = tone(lv, w, h)
    t[137] = speck(w, h)
    t[138] = star4(w, h)
    t[139] = star8(w, h)
    t[140] = burst(w, h)
    t[141] = half('u', w, h)
    t[142] = half('d', w, h)
    t[143] = half('l', w, h)
    t[144] = half('r', w, h)
    t[145] = square(w, h)
    t[146] = diamond(w, h)
    t[147] = disc(w, h)
    t[148] = ring(w, h)
    t[149] = diagonal(True, w, h)
    t[150] = diagonal(False, w, h)
    t[151] = cross(w, h)
    for q in range(4):                           # 152..155
        t[152 + q] = arc(q, w, h)
    assert sorted(t) == list(range(TILE_FIRST, TILE_LAST + 1)), \
        "the table and the declared range disagree"
    return t


if __name__ == "__main__":
    import sys
    w, h = (12, 24) if "--12x24" in sys.argv else (6, 12)
    for code, rows in sorted(tiles(w, h).items()):
        print("%d:" % code)
        for r in rows:
            print("  " + r.replace('.', ' '))
