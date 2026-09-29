#!/usr/bin/env python3
"""mock_pictures.py OUT_DIR - the mock-ups in docs/wiki/pictures-and-type.md.

A MOCK-UP, AND IT SAYS WHICH PART. Every frame is drawn by the deck's picture
engine, firmware/components/viz/viz.c, through tools/mock_frames.c - the same way
tools/zine_art.c draws the zine. Only two things are imagined here, and both are
labelled on the images:

  square dots   the engine built a second time from a COPY of viz.c with three
                edits: the frame may be larger (VIZ_W, VIZ_H), and vertical
                distance counts once instead of twice, because a 4 x 4 dot is
                square where a 12 x 24 cell is not. Nothing else in viz.c changes.
                The copy is checked edit by edit, so if viz.c moves on, this
                fails instead of mocking something else.
  stamp         type laid into the picture: the deck's own faces, a font pixel to
                a dot, composited here - there is no stamp in the engine.

One bit, at the panel's own pixel size, ordered dither from font_tiles.BAYER.

    python3 tools/mock_pictures.py docs/img        # needs a C compiler, Pillow
"""
import os, re, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from zine import BIG, SMALL, Page, INK   # noqa: E402
from font_tiles import BAYER             # noqa: E402

VIZ_C = os.path.join(REPO, 'firmware/components/viz/viz.c')
VIZ_H = os.path.join(REPO, 'firmware/components/viz/include/viz.h')
INCLUDES = ['-I', os.path.join(REPO, 'firmware/components/seq/include'),
            '-I', os.path.join(HERE, 'hostshim')]

# (pattern, replacement, how many times it must match) - the whole of the
# square-dot proposal as far as the engine is concerned.
SQUARE = [
    (r'#define VIZ_W 60\b', '#define VIZ_W 100', 1, 'h'),
    (r'#define VIZ_H 24\b', '#define VIZ_H 75', 1, 'h'),
    (r'-2 \* \(y - cy\)', '-(y - cy)', 1, 'c'),
    (r'2 \* \(y - cy\)', '(y - cy)', 4, 'c'),
    (r'\(s_w / 2 < s_h\) \? \(s_w / 2\) : s_h', '(s_w < s_h) ? (s_w / 2) : (s_h / 2)', 2, 'c'),
]


def build(tmp, square):
    src = os.path.join(tmp, 'sq' if square else 'real')
    inc = os.path.join(src, 'include')
    os.makedirs(inc, exist_ok=True)
    c, h = open(VIZ_C).read(), open(VIZ_H).read()
    if square:
        for pat, rep, n, which in SQUARE:
            text = h if which == 'h' else c
            text, k = re.subn(pat, rep, text)
            if k != n:
                sys.exit(f'viz.c has moved on: {pat!r} matched {k} times, expected {n}')
            if which == 'h':
                h = text
            else:
                c = text
    open(os.path.join(src, 'viz.c'), 'w').write(c)
    open(os.path.join(inc, 'viz.h'), 'w').write(h)
    exe = os.path.join(src, 'mock_frames')
    subprocess.run(['cc', '-std=c11', '-O1', '-I', inc] + INCLUDES +
                   ['-o', exe, os.path.join(HERE, 'mock_frames.c'),
                    os.path.join(src, 'viz.c')], check=True)
    return exe


def frames(exe, out, sizes):
    os.makedirs(out, exist_ok=True)
    for w, h in sizes:
        subprocess.run([exe, out, str(w), str(h)], check=True)


def load(d, scene, w, h):
    b = open(os.path.join(d, f'{scene}-{w}x{h}.cells'), 'rb').read()
    assert (b[0], b[1]) == (w, h)
    return [[b[2 + y * w + x] for x in range(w)] for y in range(h)]


def tone(code):
    """A cell's tone, 0..8. The one glyph a picture writes that is not a tone is
    the small disc, 147, and it is solid."""
    return code - 128 if 128 <= code <= 136 else (8 if code == 147 else 0)


# ---------------------------------------------------------------- renderers

def draw_tiles(p, ox, oy, g, face):
    """Today: each cell is a glyph from the face the text is set in."""
    for y, row in enumerate(g):
        for x, code in enumerate(row):
            p.glyph(ox + x * face.w, oy + y * face.h, code, face, 1)


def draw_dots(p, ox, oy, g, s):
    """Square dots, s x s pixels, tone by ordered dither in panel coordinates -
    so a field of one tone is seamless across dots, as the tiles are across
    cells. At s = 4 a dot is exactly one period of the 4 x 4 matrix."""
    for y, row in enumerate(g):
        for x, code in enumerate(row):
            t = tone(code) * 2
            if t == 0:
                continue
            for py in range(s):
                for px in range(s):
                    X, Y = ox + x * s + px, oy + y * s + py
                    if BAYER[Y & 3][X & 3] < t:
                        p.px[X, Y] = INK


# SCREENS - the order in which a cell's pixels ink as a tone rises. Bayer is what
# every one-bit system reaches for; print had screens: a dot that grows, lines
# that thicken. Each is a table, so each costs what Bayer costs.
def _line_screen():
    # Whole rows only: a grey is the weight of a line - one, two or three pixels
    # on a four-pixel pitch (rows 1, then 2, then 0) - never a part-filled row.
    # Part-filled rows were tried both ways: dashed looks like Morse, dotted
    # flips single pixels in motion (41 % of flips on the radar, against 0.3 %
    # for whole rows - lone_flips() below). Three greys, and they hold still.
    at = {1: 0, 2: 4, 0: 8, 3: 14}
    return [[at[r]] * 4 for r in range(4)]


def _diagonal_screen():
    return [[4 * ((c + r) % 4) + c for c in range(4)] for r in range(4)]


def _cross_screen():
    # horizontal lines first, then vertical ones: an engraver's hatch
    m = [[0] * 4 for _ in range(4)]
    k = 0
    for c in range(4):
        m[1][c] = k; k += 1
    for r in (0, 2, 3):
        m[r][1] = k; k += 1
    for r in (0, 2, 3):
        for c in (0, 2, 3):
            m[r][c] = k; k += 1
    return m


def _cluster(n):
    # a halftone dot: pixels ink from the cell's centre outward, rounded
    cx = cy = (n - 1) / 2.0
    cells = sorted(((r, c) for r in range(n) for c in range(n)),
                   key=lambda rc: ((rc[0] - cy) ** 2 + (rc[1] - cx) ** 2, rc[0], rc[1]))
    m = [[0] * n for _ in range(n)]
    for i, (r, c) in enumerate(cells):
        m[r][c] = i
    return m


# OUR OWN - the owner's verdict on the screens above, 2026-09-28: Bayer is the
# best by a large margin, so make one of our own. The constraint, plainly: in a
# 4 x 4 cell Bayer's order is the most even there is - at a quarter, a half and
# three quarters there is exactly one best pattern, and Bayer has it. So each of
# ours keeps as much of Bayer as it can and changes one thing on purpose.
def _grain():
    # Bayer is built from 2 x 2 blocks: which pixel in each block (the diagonal -
    # that is what makes half a checkerboard) and which block first. Bayer takes
    # the blocks diagonally too; grain takes them along the row. The quarter,
    # half and three quarters are Bayer's own; the greys between them grow along
    # rows, so a scan line shows only there.
    fine, blocks = [[0, 2], [3, 1]], [[0, 1], [2, 3]]
    return [[4 * fine[r % 2][c % 2] + blocks[r // 2][c // 2] for c in range(4)]
            for r in range(4)]


def _weave():
    # Bayer in every other dot, turned half a turn in the rest, like a
    # chessboard of dots. Half a turn keeps the checkerboard at a half, so there
    # is no seam; the lightest grey becomes almost hexagonal - 2.8 and 3.2
    # pixels apart where Bayer's are 2.8 and 4 - and the rest weaves.
    m = [[0] * 8 for _ in range(8)]
    for r in range(8):
        for c in range(8):
            turned = (r // 4 + c // 4) % 2
            b = BAYER[3 - r % 4][3 - c % 4] if turned else BAYER[r % 4][c % 4]
            m[r][c] = 4 * b + 2 * (r // 4) + c // 4      # 0..63; a dot's own order kept
    return m


def _wide():
    # Bayer on pixels two wide, as the C64 drew its colour modes: a pixel and
    # its neighbour always ink together, so almost nothing sparkles alone.
    return [[2 * BAYER[r][c // 2] + c % 2 for c in range(8)] for r in range(4)]


SCREENS = {
    'bayer': [list(row) for row in BAYER],
    'halftone': _cluster(4),
    'lines': _line_screen(),
    'diagonal': _diagonal_screen(),
    'hatch': _cross_screen(),
    'poster': _cluster(8),
    'grain': _grain(),
    'weave': _weave(),
    'wide': _wide(),
}
PRINT = ('bayer', 'halftone', 'lines', 'diagonal', 'hatch', 'poster')
OURS = ('bayer', 'grain', 'weave', 'wide')


def draw_banded(p, ox, oy, g, s, screen=None):
    """4 x 4 dots, banded - the chosen look. A grey dot is drawn from the tones
    at its four corners, interpolated across its pixels and cut into the nine
    tones, so greys meet in curves. A solid or an empty dot is drawn exactly as
    it is: banding a one-dot line with the paper beside it would grey it out."""
    h, w = len(g), len(g[0])
    t = [[tone(c) for c in row] for row in g]
    cor = [[0.0] * (w + 1) for _ in range(h + 1)]
    for j in range(h + 1):
        for i in range(w + 1):
            vals = [t[y][x] for (x, y) in ((i - 1, j - 1), (i, j - 1), (i - 1, j), (i, j))
                    if 0 <= x < w and 0 <= y < h]
            cor[j][i] = sum(vals) / len(vals) / 8.0
    for y in range(h):
        for x in range(w):
            own = t[y][x]
            a, b, d, e = cor[y][x], cor[y][x + 1], cor[y + 1][x], cor[y + 1][x + 1]
            for py in range(s):
                v = (py + 0.5) / s
                left, right = a + (d - a) * v, b + (e - b) * v
                for px in range(s):
                    X, Y = ox + x * s + px, oy + y * s + py
                    if own in (0, 8):
                        lv = own
                    else:
                        lv = int((left + (right - left) * (px + 0.5) / s) * 8 + 0.5)
                        lv = min(max(lv, 1), 7)          # a grey dot stays a grey
                    m = screen or SCREENS['bayer']
                    ny, nx = len(m), len(m[0])
                    if m[Y % ny][X % nx] < lv * nx * ny / 8.0:
                        p.px[X, Y] = INK


def draw_dual(p, ox, oy, g, cw, ch, crisp):
    """A renderer-only alternative, kept to show why it is not proposed: each
    cell drawn from the tones at its four corners (the mean of the cells that
    meet there), interpolated across the cell - banded into the nine tones, or
    cut at half for a crisp edge. It smooths edges; it cannot add resolution a
    four-row frame does not have."""
    h, w = len(g), len(g[0])
    t = [[tone(c) for c in row] for row in g]
    cor = [[0.0] * (w + 1) for _ in range(h + 1)]
    for j in range(h + 1):
        for i in range(w + 1):
            vals = [t[y][x] for (x, y) in ((i - 1, j - 1), (i, j - 1), (i - 1, j), (i, j))
                    if 0 <= x < w and 0 <= y < h]
            cor[j][i] = sum(vals) / len(vals) / 8.0
    for y in range(h):
        for x in range(w):
            a, b, d, e = cor[y][x], cor[y][x + 1], cor[y + 1][x], cor[y + 1][x + 1]
            for py in range(ch):
                v = (py + 0.5) / ch
                left, right = a + (d - a) * v, b + (e - b) * v
                for px in range(cw):
                    val = left + (right - left) * (px + 0.5) / cw
                    X, Y = ox + x * cw + px, oy + y * ch + py
                    ink = val >= 0.5 if crisp else BAYER[Y & 3][X & 3] < int(val * 8 + 0.5) * 2
                    if ink:
                        p.px[X, Y] = INK


# ---------------------------------------------------------------- sheets

def lone_flips(d, scene, n, name):
    """How a screen moves: of the pixels that change from one frame to the
    next, the share that change alone - no changed pixel beside them. That is
    sparkle; a dot that grows or a line that lengthens changes in a clump."""
    prev, flips, alone = None, 0, 0
    for k in range(n):
        p = Page(336, 96)
        draw_banded(p, 0, 0, load(d, f'{scene}{k:02d}', 84, 24), 4, SCREENS[name])
        cur = {(x, y) for y in range(96) for x in range(336) if p.px[x, y] == INK}
        if prev is not None:
            ch = cur ^ prev
            flips += len(ch)
            alone += sum(1 for (x, y) in ch if not ({(x + 1, y), (x - 1, y),
                                                      (x, y + 1), (x, y - 1)} & ch))
        prev = cur
    return 100.0 * alone / max(flips, 1)


def truth(g, name, sigma=1.2):
    """How true a screen is to the greys the engine meant: blur the dots and the
    intended greys alike, as the eye does at arm's length, and take the RMS
    difference. Lower is truer. The intended grey of a pixel is what
    draw_banded interpolates before it cuts the nine tones."""
    import numpy as np
    h, w, s = len(g), len(g[0]), 4
    t = [[tone(c) for c in row] for row in g]
    cor = np.zeros((h + 1, w + 1))
    for j in range(h + 1):
        for i in range(w + 1):
            v = [t[y][x] for (x, y) in ((i - 1, j - 1), (i, j - 1), (i - 1, j), (i, j))
                 if 0 <= x < w and 0 <= y < h]
            cor[j, i] = sum(v) / len(v) / 8.0
    want = np.zeros((h * s, w * s))
    for y in range(h):
        for x in range(w):
            if t[y][x] in (0, 8):
                want[y * s:y * s + s, x * s:x * s + s] = t[y][x] / 8.0
                continue
            a, b, d, e = cor[y, x], cor[y, x + 1], cor[y + 1, x], cor[y + 1, x + 1]
            for py in range(s):
                v = (py + 0.5) / s
                left, right = a + (d - a) * v, b + (e - b) * v
                for px in range(s):
                    want[y * s + py, x * s + px] = min(max(
                        left + (right - left) * (px + 0.5) / s, 1 / 8), 7 / 8)
    p = Page(w * s, h * s)
    draw_banded(p, 0, 0, g, s, SCREENS[name])
    got = np.array([[1.0 if p.px[x, y] == INK else 0.0 for x in range(w * s)]
                    for y in range(h * s)])
    r = int(3 * sigma) + 1
    k = np.exp(-np.arange(-r, r + 1) ** 2 / (2 * sigma * sigma))
    k /= k.sum()

    def blur(a):
        a = np.pad(a, r, mode='edge')
        a = np.apply_along_axis(lambda v: np.convolve(v, k, 'valid'), 0, a)
        return np.apply_along_axis(lambda v: np.convolve(v, k, 'valid'), 1, a)
    return float(np.sqrt(np.mean((blur(got) - blur(want)) ** 2)))


def motion(path, d, scene, n, names, ms=120):
    """Every frame the engine draws for a scene, a step at a time, in each of
    the named screens: a moving picture, two by two, at twice the panel's pixel.
    A frame every 120 ms, a sixteenth at 125 bpm - a GIF counts in hundredths."""
    from PIL import Image
    shots = []
    for k in range(n):
        g = load(d, f'{scene}{k:02d}', 84, 24)
        p = Page(2 * 336 + 3 * 16, 2 * (96 + 28) + 12)
        for i, name in enumerate(names):
            x, y = 16 + (i % 2) * (336 + 16), 24 + (i // 2) * (96 + 28)
            p.text(x, y - 18, name, SMALL, 1)
            p.frame(x - 2, y - 2, 336 + 4, 96 + 4, 1)
            draw_banded(p, x, y, g, 4, SCREENS[name])
        shots.append(p.im.resize((p.w * 2, p.h * 2), Image.NEAREST))
    shots[0].save(path, save_all=True, append_images=shots[1:], duration=ms, loop=0)


def sheet(path, scenes, cols, gap=16, label=24, scale=1):
    """cols: (title, pixel w, pixel h, draw(p, ox, oy, scene)). scale > 1 saves it
    enlarged, a panel pixel to scale x scale, for patterns of single pixels."""
    pw = [c[1] for c in cols]
    ph = max(c[2] for c in cols)
    p = Page(sum(pw) + gap * (len(cols) + 1), label + len(scenes) * (ph + gap) + gap)
    x = gap
    for title, w, h, draw in cols:
        p.text(x, 4, title, SMALL, 1)
        for i, sc in enumerate(scenes):
            y = label + gap + i * (ph + gap)
            p.frame(x - 2, y - 2, w + 4, h + 4, 1)
            draw(p, x, y, sc)
        x += w + gap
    if scale > 1:
        from PIL import Image
        p.im.resize((p.w * scale, p.h * scale), Image.NEAREST).save(path)
    else:
        p.im.save(path)


def stamp(g, text, face, x0, y0, xor=False):
    """Type into a tone grid, a font pixel to a dot: solid, or knocked out."""
    for i, ch in enumerate(text):
        rows = face.g[ord(ch)]
        for r in range(face.h):
            for c in range(face.w):
                if rows[r] & (0x8000 >> c):
                    x, y = x0 + i * face.w + c, y0 + r
                    if 0 <= y < len(g) and 0 <= x < len(g[0]):
                        g[y][x] = (136 - (g[y][x] - 128 if g[y][x] >= 128 else 0)) if xor else 136


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, 'docs/img')
    os.makedirs(out, exist_ok=True)
    tmp = tempfile.mkdtemp(prefix='mock_pictures_')
    try:
        real, sq = build(tmp, False), build(tmp, True)
        rd, sd = os.path.join(tmp, 'rf'), os.path.join(tmp, 'sf')
        frames(real, rd, [(28, 4), (58, 17)])
        frames(sq, sd, [(56, 16), (84, 24), (87, 51)])
        R = lambda sc, w, h: load(rd, sc, w, h)          # noqa: E731
        Q = lambda sc, w, h: load(sd, sc, w, h)          # noqa: E731

        # 1. The default pane - 28 x 4 cells of 12 x 24, 336 x 96 pixels - today,
        #    and the same pixels as square dots.
        scenes = ['disc', 'discmask', 'ring', 'box', 'turn', 'radar', 'orbit']
        sheet(os.path.join(out, 'pictures-dots.png'), scenes, [
            ('today: 28 x 4 cells of 12 x 24', 336, 96,
             lambda p, x, y, s: draw_tiles(p, x, y, R(s, 28, 4), BIG)),
            ('6 x 6 dots: 56 x 16', 336, 96,
             lambda p, x, y, s: draw_dots(p, x, y, Q(s, 56, 16), 6)),
            ('4 x 4 dots: 84 x 24 (proposed)', 336, 96,
             lambda p, x, y, s: draw_dots(p, x, y, Q(s, 84, 24), 4)),
        ])

        # 1b. The choice, 2026-09-28: 4 x 4 dots, BANDED - each dot drawn from
        #     the tones at its four corners, interpolated across its sixteen
        #     pixels and cut into the nine tones. A 4 x 4 dot is one period of
        #     the dither, so on the deck this is a lookup, not arithmetic.
        sheet(os.path.join(out, 'pictures-banded.png'), scenes, [
            ('today: 28 x 4 cells of 12 x 24', 336, 96,
             lambda p, x, y, s: draw_tiles(p, x, y, R(s, 28, 4), BIG)),
            ('4 x 4 dots, flat', 336, 96,
             lambda p, x, y, s: draw_dots(p, x, y, Q(s, 84, 24), 4)),
            ('4 x 4 dots, banded (chosen)', 336, 96,
             lambda p, x, y, s: draw_banded(p, x, y, Q(s, 84, 24), 4)),
        ])

        # 1c. Screens: the same banded dots, inked the ways print did it - at the
        #     panel's own pixel. Set aside: Bayer is better by a large margin.
        four = ['disc', 'turn', 'radar', 'orbit']

        def screened(name):
            return lambda p, x, y, sc: draw_banded(p, x, y, Q(sc, 84, 24), 4, SCREENS[name])
        sheet(os.path.join(out, 'pictures-screens.png'), four,
              [(name, 336, 96, screened(name)) for name in PRINT])

        # 1d. Our own, beside Bayer, at twice the panel's pixel.
        sheet(os.path.join(out, 'pictures-own.png'), four,
              [(name, 336, 96, screened(name)) for name in OURS], scale=2)

        # 1e. The same four moving - every frame the engine draws for the radar
        #     from ORBITALS, a step at a time, as the panel would show it.
        md = os.path.join(tmp, 'mf')
        os.makedirs(md)
        subprocess.run([sq, md, '84', '24', 'motion'], check=True)
        motion(os.path.join(out, 'pictures-motion.gif'), md, 'radar', 32, OURS)
        print('screen     truth (RMS, lower is truer)   changing alone: radar  orbit')
        for name in SCREENS:
            tr = sum(truth(Q(sc, 84, 24), name) for sc in four) / len(four)
            print(f'  {name:9} {tr:.3f}                         '
                  f'{lone_flips(md, "radar", 32, name):5.1f} %'
                  f'  {lone_flips(md, "orbit", 48, name):5.1f} %')

        # 2. Smoothing the cells instead: it rounds edges, it cannot add rows.
        sheet(os.path.join(out, 'pictures-smoothing.png'), ['disc', 'ring', 'orbit'], [
            ('today', 336, 96, lambda p, x, y, s: draw_tiles(p, x, y, R(s, 28, 4), BIG)),
            ('corners, banded', 336, 96,
             lambda p, x, y, s: draw_dual(p, x, y, R(s, 28, 4), 12, 24, False)),
            ('corners, cut at half', 336, 96,
             lambda p, x, y, s: draw_dual(p, x, y, R(s, 28, 4), 12, 24, True)),
        ])

        # 3. The largest picture the editor gives today - the compact face, split
        #    at 17 rows: 58 x 17 cells of 6 x 12, 348 x 204 pixels - and 4 x 4 dots.
        sheet(os.path.join(out, 'pictures-panel.png'), ['disc', 'ring', 'radar', 'orbit'], [
            ('today, compact face: 58 x 17 cells of 6 x 12', 348, 204,
             lambda p, x, y, s: draw_tiles(p, x, y, R(s, 58, 17), SMALL)),
            ('4 x 4 dots: 87 x 51, the same pixels', 348, 204,
             lambda p, x, y, s: draw_dots(p, x, y, Q(s, 87, 51), 4)),
        ])

        # 4. Type as picture, on 4 x 4 dots in the default pane.
        blank = [[128] * 84 for _ in range(24)]
        panels = []
        g = [r[:] for r in blank]; stamp(g, 'kick', BIG, 18, 0)
        panels.append(('the 12 x 24 face, a font pixel a dot: a word fills the pane', g))
        g = Q('disc', 84, 24); stamp(g, 'x...x...', SMALL, 18, 6, xor=True)
        panels.append(('a disc, and type knocked out of it', g))
        g = [r[:] for r in blank]
        for k, x0 in enumerate(range(4, 56, 9)):
            g = [[max(128, c - 1) for c in row] for row in g]     # echo: a tone fainter a step
            stamp(g, 'deck', SMALL, x0, 6 + (k % 2) * 4)
        panels.append(('a word moving right under echo', g))
        g = Q('ring', 84, 24); stamp(g, 'o', BIG, 36, 0)
        panels.append(('a ring, and the letter o in it', g))
        p = Page(84 * 4 + 32, len(panels) * (96 + 40) + 20)
        for i, (lab, g) in enumerate(panels):
            oy = 20 + i * (96 + 40)
            p.text(16, oy - 16, lab, SMALL, 1)
            p.frame(14, oy - 2, 84 * 4 + 4, 96 + 4, 1)
            draw_dots(p, 16, oy, g, 4)
        p.im.save(os.path.join(out, 'pictures-stamp.png'))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print('mock-ups written to', out)


if __name__ == '__main__':
    main()
