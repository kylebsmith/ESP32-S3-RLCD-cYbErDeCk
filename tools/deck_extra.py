"""The characters the deck's two faces lack, for print and the web only.

The panel's faces cover ASCII and the deck's own tiles; the wiki and the zine
also set dashes, arrows, box drawing, a few maths signs and accented names.
Rather than fall back to someone else's typeface, they are drawn here on the
12 x 24 face's own grid - strokes two pixels wide, capitals on rows 4-19, the
x-height from row 10, the baseline under row 19, the maths axis on rows 13-14 -
and the 6 x 12 versions are the same drawings halved, a pixel kept where any of
its four was inked. The firmware never sees these: the panel's faces stay as
they are.

  python3 tools/deck_extra.py OUT.png     a proof sheet, every glyph at 4x
"""
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from font12x24_art import ART   # noqa: E402

W, H = 12, 24


def sparse(rows):
    """{row: '..##..'} -> 24 rows of 12, '.' and '#'."""
    out = ['.' * W] * H
    for y, r in rows.items():
        out[y] = (r + '.' * W)[:W]
    return out


def base(ch):
    return list(ART[ord(ch)])


def over(a, b):
    return [''.join('#' if x == '#' or y == '#' else '.' for x, y in zip(r, s))
            for r, s in zip(a, b)]


def shift(rows, dy):
    blank = '.' * W
    if dy < 0:
        return rows[-dy:] + [blank] * -dy
    return [blank] * dy + rows[:H - dy]


def mirror_h(rows):
    return [r[:10][::-1] + r[10:] for r in rows]


def mirror_v(rows):
    return rows[::-1]


def hbar(y0, y1, x0, x1):
    return sparse({y: '.' * x0 + '#' * (x1 - x0 + 1) for y in range(y0, y1 + 1)})


def vbar(x0, x1, y0, y1):
    return sparse({y: '.' * x0 + '#' * (x1 - x0 + 1) for y in range(y0, y1 + 1)})


# BOX DRAWING runs through the middle of the cell and out to its edges, so it
# joins across cells: across on rows 11-12, down on columns 5-6.
def box(l, r, u, d):
    g = sparse({})
    if l: g = over(g, hbar(11, 12, 0, 6))
    if r: g = over(g, hbar(11, 12, 5, 11))
    if u: g = over(g, vbar(5, 6, 0, 12))
    if d: g = over(g, vbar(5, 6, 11, 23))
    return g


RIGHT = sparse({
    10: '.....##',
    11: '......##',
    12: '.......##',
    13: '##########',
    14: '##########',
    15: '.......##',
    16: '......##',
    17: '.....##',
})
DOWN = sparse({
    4: '....##', 5: '....##', 6: '....##', 7: '....##', 8: '....##',
    9: '....##', 10: '....##', 11: '....##', 12: '....##', 13: '....##',
    14: '.#..##..#', 15: '.##.##.##', 16: '..######', 17: '...####', 18: '....##',
})

ACUTE = sparse({6: '.....##', 7: '....##', 8: '...##'})
GRAVE = sparse({6: '...##', 7: '....##', 8: '.....##'})
DIAER = sparse({7: '..##..##', 8: '..##..##'})
BREVE = sparse({6: '..#....#', 7: '..##..##', 8: '...####'})
COMMA_BELOW = sparse({21: '....##', 22: '....##', 23: '...##'})

EXTRA = {
    0x2014: hbar(13, 14, 0, 11),                       # em dash: joins
    0x2013: hbar(13, 14, 1, 8),                        # en dash
    0x2212: hbar(13, 14, 0, 9),                        # minus, as wide as +
    0x2192: RIGHT,                                     # right arrow
    0x2190: mirror_h(RIGHT),
    0x2193: DOWN,
    0x2191: mirror_v(shift(DOWN, 1)),
    0x2194: over(over(hbar(13, 14, 0, 9), sparse({10: '..##...##', 11: '.##.....##'[:10], 12: '##.......#'[:10]})),
                 sparse({15: '##.......#'[:10], 16: '.##.....##'[:10], 17: '..##...##'})),
    0x00B7: sparse({12: '....##', 13: '...####', 14: '...####', 15: '....##'}),
    0x00D7: sparse({10: '.##....##', 11: '..##..##', 12: '...####', 13: '....##',
                    14: '....##', 15: '...####', 16: '..##..##', 17: '.##....##'}),
    0x2026: sparse({17: '##..##..##', 18: '##..##..##'}),
    0x00A7: sparse({
        3: '...####', 4: '..##..##', 5: '..##', 6: '...##', 7: '..####',
        8: '.##..##', 9: '.##...##', 10: '.##...##', 11: '..##..##', 12: '...####',
        13: '.....##', 14: '......##', 15: '..##..##', 16: '...####'}),
    0x00B5: over(base('u'), sparse({20: '##', 21: '##', 22: '##', 23: '##'})),
    0x2264: over(shift(base('<'), -3), hbar(18, 19, 1, 8)),
    0x2265: over(shift(base('>'), -3), hbar(18, 19, 1, 8)),
    0x2261: over(over(hbar(9, 10, 0, 9), hbar(13, 14, 0, 9)), hbar(17, 18, 0, 9)),
    0x2248: sparse({9: '.###...##', 10: '##.##.##', 11: '##..###',
                    14: '.###...##', 15: '##.##.##', 16: '##..###'}),
    0x00B0: sparse({3: '...####', 4: '..##..##', 5: '..##..##', 6: '...####'}),
    0x00B1: over(over(vbar(4, 5, 7, 16), hbar(11, 12, 0, 9)), hbar(18, 19, 0, 9)),
    0x00F7: over(over(hbar(13, 14, 0, 9), sparse({9: '....##', 10: '....##'})),
                 sparse({17: '....##', 18: '....##'})),
    0x2713: sparse({8: '.........#', 9: '........##', 10: '.......##', 11: '......##',
                    12: '.....##', 13: '#...##', 14: '##.##', 15: '.###', 16: '..#'}),
    0x25BA: sparse({7: '.#', 8: '.###', 9: '.#####', 10: '.#######', 11: '.#########',
                    12: '.#########', 13: '.#######', 14: '.#####', 15: '.###', 16: '.#'}),
    0x25BC: sparse({9: '##########', 10: '.########', 11: '.########', 12: '..######',
                    13: '..######', 14: '...####', 15: '...####', 16: '....##'}),
    0x2500: box(1, 1, 0, 0), 0x2502: box(0, 0, 1, 1),
    0x250C: box(0, 1, 0, 1), 0x2510: box(1, 0, 0, 1),
    0x2514: box(0, 1, 1, 0), 0x2518: box(1, 0, 1, 0),
    0x251C: box(0, 1, 1, 1), 0x2524: box(1, 0, 1, 1),
    0x252C: box(1, 1, 0, 1), 0x2534: box(1, 1, 1, 0),
    0x253C: box(1, 1, 1, 1),
    0x230A: over(vbar(2, 3, 4, 21), hbar(20, 21, 2, 7)),        # floor
    0x230B: over(vbar(6, 7, 4, 21), hbar(20, 21, 2, 7)),
    0x2126: sparse({4: '..######', 5: '.##....##', 6: '##......##', 7: '##......##',
                    8: '##......##', 9: '##......##', 10: '##......##', 11: '##......##',
                    12: '##......##', 13: '.##....##', 14: '..##..##', 15: '..##..##',
                    16: '..##..##', 17: '..##..##', 18: '###....###', 19: '###....###'}),
    0x0394: sparse({4: '....##', 5: '....##', 6: '...####', 7: '...####', 8: '..##..##',
                    9: '..##..##', 10: '..##..##', 11: '.##....##', 12: '.##....##',
                    13: '.##....##', 14: '##......##', 15: '##......##', 16: '##......##',
                    17: '##......##', 18: '##########', 19: '##########'}),
    0x03C3: sparse({10: '..########', 11: '.##########', 12: '##....##', 13: '##.....##',
                    14: '##......##', 15: '##......##', 16: '##......##', 17: '##......##',
                    18: '.##....##', 19: '..######'}),
    0x00D8: over(base('O'), sparse({3: '.........#', 4: '........##', 6: '.......##',
                                    8: '......##', 10: '.....##', 12: '....##', 14: '...##',
                                    16: '..##', 18: '.##', 19: '##', 20: '#'})),
    0x00E9: over(base('e'), ACUTE), 0x00E8: over(base('e'), GRAVE),
    0x00E1: over(base('a'), ACUTE), 0x0103: over(base('a'), BREVE),
    0x00FC: over(base('u'), DIAER), 0x00FF: over(base('y'), DIAER),
    0x0219: over(base('s'), COMMA_BELOW),
}

# Filling in the Oslash's diagonal: every row from 3 to 20 gets its step.
_o = [list(r) for r in EXTRA[0x00D8]]
for y in range(3, 21):
    x = max(0, min(9, 9 - (y - 3) * 9 // 17))
    for dx in (0, 1):
        if 0 <= x - dx < W:
            _o[y][x - dx] = '#'
EXTRA[0x00D8] = [''.join(r) for r in _o]

# Superscripts are the deck's small face, raised: the compact digits on the
# big face's grid, top-aligned with the capitals.
def _small_rows():
    sys.path.insert(0, HERE)
    from zine import SMALL_SRC, load_face
    return load_face(SMALL_SRC, 6, 12)

def _sup(ch):
    g = _small_rows()[ord(ch)]
    rows = ['.' * W] * H
    for y in range(12):
        bits = ''.join('#' if g[y] & (0x8000 >> x) else '.' for x in range(6))
        if y - 1 >= 0:
            rows[y - 1] = ('...' + bits + '...')[:W]
    return rows

for cp, ch in ((0x00B9, '1'), (0x00B2, '2'), (0x00B3, '3'), (0x2074, '4'), (0x207F, 'n')):
    EXTRA[cp] = _sup(ch)

# What is not drawn is written out: rare enough that words serve better.
SPELL = {'½': '1/2', '¼': '1/4', '⅛': '1/8', '⅔': '2/3',
         '♭': 'b', '⏎': 'Enter', '″': '"', '✱': '*',
         '‘': "'", '’': "'", '“': '"', '”': '"'}


# THE 6 x 12 VERSIONS, drawn for that grid rather than halved: halving turned
# the three bars of an identity into one block. The small face's own metrics:
# capitals on rows 2-8, x-height from row 4, the baseline under row 8, the
# maths axis on row 5, one-pixel strokes in columns 0-4.
SW, SH = 6, 12


def ssparse(rows):
    out = ['.' * SW] * SH
    for y, r in rows.items():
        out[y] = (r + '.' * SW)[:SW]
    return out


def sover(a, b):
    return [''.join('#' if x == '#' or y == '#' else '.' for x, y in zip(r, t))
            for r, t in zip(a, b)]


def _small_base(ch):
    g = _small_rows()[ord(ch)]
    return [''.join('#' if g[y] & (0x8000 >> x) else '.' for x in range(SW)) for y in range(SH)]


def sbox(l, r, u, d):
    g = ssparse({})
    if l: g = sover(g, ssparse({5: '###'}))
    if r: g = sover(g, ssparse({5: '..####'}))
    if u: g = sover(g, ssparse({y: '..#' for y in range(0, 6)}))
    if d: g = sover(g, ssparse({y: '..#' for y in range(5, 12)}))
    return g


def _small_extra():
    sup = {'1': ['.#.', '##.', '.#.', '.#.', '###'], '2': ['##.', '..#', '.#.', '#..', '###'],
           '3': ['##.', '..#', '.#.', '..#', '##.'], '4': ['#.#', '#.#', '###', '..#', '..#'],
           'n': ['...', '...', '##.', '#.#', '#.#']}
    def ssup(ch):
        return ssparse({y + 1: '.' + r for y, r in enumerate(sup[ch])})
    acute, grave = ssparse({2: '...#', 3: '..#'}), ssparse({2: '.#', 3: '..#'})
    return {
        0x2014: ssparse({5: '######'}), 0x2013: ssparse({5: '#####'}), 0x2212: ssparse({5: '#####'}),
        0x2192: ssparse({3: '..#', 4: '...#', 5: '#####', 6: '...#', 7: '..#'}),
        0x2190: ssparse({3: '..#', 4: '.#', 5: '#####', 6: '.#', 7: '..#'}),
        0x2193: ssparse({2: '..#', 3: '..#', 4: '..#', 5: '..#', 6: '#.#.#', 7: '.###', 8: '..#'}),
        0x2191: ssparse({2: '..#', 3: '.###', 4: '#.#.#', 5: '..#', 6: '..#', 7: '..#', 8: '..#'}),
        0x2194: ssparse({4: '.#..#', 5: '######', 6: '.#..#'}),
        0x00B7: ssparse({4: '..##', 5: '..##'}),
        0x00D7: ssparse({3: '#...#', 4: '.#.#', 5: '..#', 6: '.#.#', 7: '#...#'}),
        0x2026: ssparse({8: '#.#.#'}),
        0x00A7: ssparse({1: '.###', 2: '#', 3: '.###', 4: '#...#', 5: '.###', 6: '....#', 7: '.###'}),
        0x00B5: sover(_small_base('u'), ssparse({9: '#', 10: '#'})),
        0x2264: ssparse({2: '...#', 3: '..#', 4: '.#', 5: '..#', 6: '...#', 8: '####'}),
        0x2265: ssparse({2: '.#', 3: '..#', 4: '...#', 5: '..#', 6: '.#', 8: '####'}),
        0x2261: ssparse({3: '#####', 5: '#####', 7: '#####'}),
        0x2248: ssparse({3: '.##.#', 4: '#.##', 6: '.##.#', 7: '#.##'}),
        0x00B0: ssparse({1: '.##', 2: '#..#', 3: '.##'}),
        0x00B1: ssparse({2: '..#', 3: '..#', 4: '#####', 5: '..#', 6: '..#', 8: '#####'}),
        0x00F7: ssparse({3: '..#', 5: '#####', 7: '..#'}),
        0x2713: ssparse({3: '.....#', 4: '....#', 5: '#..#', 6: '.##'}),
        0x25BA: ssparse({2: '#', 3: '##', 4: '###', 5: '####', 6: '###', 7: '##', 8: '#'}),
        0x25BC: ssparse({3: '#####', 4: '.###', 5: '..#'}),
        0x2500: sbox(1, 1, 0, 0), 0x2502: sbox(0, 0, 1, 1),
        0x250C: sbox(0, 1, 0, 1), 0x2510: sbox(1, 0, 0, 1),
        0x2514: sbox(0, 1, 1, 0), 0x2518: sbox(1, 0, 1, 0),
        0x251C: sbox(0, 1, 1, 1), 0x2524: sbox(1, 0, 1, 1),
        0x252C: sbox(1, 1, 0, 1), 0x2534: sbox(1, 1, 1, 0), 0x253C: sbox(1, 1, 1, 1),
        0x230A: ssparse({y: '.#' for y in range(2, 9)} | {9: '.###'}),
        0x230B: ssparse({y: '...#' for y in range(2, 9)} | {9: '.###'}),
        0x2126: ssparse({2: '.###', 3: '#...#', 4: '#...#', 5: '#...#', 6: '.#.#', 7: '.#.#', 8: '##.##'}),
        0x0394: ssparse({2: '..#', 3: '..#', 4: '.#.#', 5: '.#.#', 6: '#...#', 7: '#...#', 8: '#####'}),
        0x03C3: ssparse({4: '.####', 5: '#..#', 6: '#...#', 7: '#...#', 8: '.###'}),
        0x00D8: sover(_small_base('O'), ssparse({1: '....#', 3: '...#', 5: '..#', 7: '.#', 9: '#'})),
        0x00E9: sover(_small_base('e'), acute), 0x00E8: sover(_small_base('e'), grave),
        0x00E1: sover(_small_base('a'), acute),
        0x0103: sover(_small_base('a'), ssparse({2: '#...#', 3: '.###'})),
        0x00FC: sover(_small_base('u'), ssparse({2: '.#.#'})),
        0x00FF: sover(_small_base('y'), ssparse({2: '.#.#'})),
        0x0219: sover(_small_base('s'), ssparse({9: '..#', 10: '.#'})),
        0x00B9: ssup('1'), 0x00B2: ssup('2'), 0x00B3: ssup('3'), 0x2074: ssup('4'), 0x207F: ssup('n'),
    }


def half(rows):
    """The 6 x 12 version: a pixel for every 2 x 2, inked if any of them is."""
    out = []
    for y in range(0, H, 2):
        out.append(''.join('#' if '#' in (rows[y][x:x + 2] + rows[y + 1][x:x + 2]) else '.'
                           for x in range(0, W, 2)))
    return out


def proof(path, scale=4):
    from PIL import Image, ImageDraw
    cps = sorted(EXTRA)
    cols = 12
    cw, ch = (W + 4) * scale, (H + 18) * scale
    img = Image.new('L', (cols * cw, ((len(cps) + cols - 1) // cols) * ch * 2), 255)
    d = ImageDraw.Draw(img)
    for i, cp in enumerate(cps):
        ox, oy = (i % cols) * cw, (i // cols) * ch * 2
        g = EXTRA[cp]
        d.rectangle([ox, oy, ox + W * scale - 1, oy + H * scale - 1], outline=200)
        for y in range(H):
            for x in range(W):
                if g[y][x] == '#':
                    d.rectangle([ox + x * scale, oy + y * scale,
                                 ox + x * scale + scale - 1, oy + y * scale + scale - 1], fill=0)
        s = SMALL[cp]
        for y in range(12):
            for x in range(6):
                if s[y][x] == '#':
                    d.rectangle([ox + x * scale, oy + (H + 4 + y) * scale,
                                 ox + x * scale + scale - 1, oy + (H + 4 + y) * scale + scale - 1], fill=0)
        d.text((ox, oy + (H + 17) * scale), 'U+%04X' % cp, fill=0)
    img.save(path)


SMALL = _small_extra()
assert set(SMALL) == set(EXTRA), set(EXTRA) ^ set(SMALL)


if __name__ == '__main__':
    for cp, g in EXTRA.items():
        assert len(g) == H and all(len(r) == W for r in g), hex(cp)
    proof(sys.argv[1] if len(sys.argv) > 1 else '/tmp/deck_extra.png')
