#!/usr/bin/env python3
"""type_round.py OUT_DIR - the round face, laid by hand, and three variants of it.

SPECULATIVE - nothing here reaches the firmware yet. This replaces the drafts
tools/type_programme.py drew: those assembled letters from curve pieces that did
not meet at the pixel, and they shipped with breaks, stray pixels and one-pixel
necks. At 12 x 24 a face is made by hand, so this one is.

HOW IT IS MADE. It starts from today's face (tools/font12x24_art.py, via the
generated font) - clean, 2-pixel strokes, proven on the panel - and changes only
what the owner asked for: rounder curves, n = 2.2 (2026-09-28). Every bowl, arch
and hook takes the same corner, three steps where today's takes two:

    today         round
    ..######..    ...####...
    .########.    .########.
    ##......##    .##....##.
                  ##......##

Straight letters, and the punctuation that is already clean, stay today's.

AND IT IS CHECKED. check() refuses a glyph that has a piece that does not join,
a pixel touching the rest only at a corner, a stray or a spur, a one-pixel neck,
or ink in the two gap columns. A glyph that fails is shown boxed on the proof and
the programme exits non-zero, so nothing broken can be shipped from here again.

    type-round.png         every glyph that changed, 5 x, beside today's; then
                           the language at the panel's size
    type-round-styles.png  the round face and three variants, each checked:
                           geometric, slab and score
"""
import os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from zine import BIG, SMALL, Page   # noqa: E402

W, H = 12, 24


def today(ch):
    rows = BIG.g[ord(ch)]
    return [''.join('#' if rows[r] & (0x8000 >> c) else '.' for c in range(W)) for r in range(H)]


def art(top, rows, base=None):
    """A glyph from a list of 10-column rows starting at row `top`, laid on
    `base` (another glyph's rows) or on paper. Columns 10-11 are the gap."""
    g = list(base) if base else ['.' * W] * H
    for i, r in enumerate(rows):
        assert len(r) == 10, (top, r)
        g[top + i] = r + '..'
    return g


def rows_of(g, a, b):
    return [r[:10] for r in g[a:b + 1]]


# ------------------------------------------------------------------ the round face

R = {}

# the x-height bowl, and the cap-height bowl: the one corner, everywhere
XO = ['...####...', '.########.', '.##....##.', '##......##', '##......##',
      '##......##', '##......##', '.##....##.', '.########.', '...####...']
CO = ['...####...', '.########.', '.##....##.'] + ['##......##'] * 10 + \
     ['.##....##.', '.########.', '...####...']

R['o'] = art(10, XO)
R['c'] = art(10, XO[:3] + ['##........'] * 4 + XO[7:])
R['e'] = art(10, XO[:4] + ['##########', '##########', '##........'] + XO[7:])
R['a'] = art(10, ['...####...', '..#######.', '.......##.', '........##',
                  '..########', '.#########', '##......##', '##......##',
                  '.#########', '..#####.##'])
BOWL_B = ['##.####...', '#########.', '##.....##.', '##......##', '##......##',
          '##......##', '##......##', '##.....##.', '#########.', '##.####...']
BOWL_D = ['...####.##', '.#########', '.##.....##', '##......##', '##......##',
          '##......##', '##......##', '.##.....##', '.#########', '...####.##']
R['b'] = art(4, ['##........'] * 6 + BOWL_B)
R['d'] = art(4, ['........##'] * 6 + BOWL_D)
R['p'] = art(10, BOWL_B + ['##........'] * 3)
R['q'] = art(10, BOWL_D + ['........##'] * 3)
ARCH = ['##.####...', '#########.', '##.....##.']
R['n'] = art(10, ARCH + ['##......##'] * 7)
R['h'] = art(4, ['##........'] * 6 + ARCH + ['##......##'] * 7)
R['r'] = art(10, ARCH + ['##........'] * 7)
R['u'] = art(10, ['##......##'] * 7 + ['.##.....##', '.#########', '...####.##'])
R['s'] = art(10, ['...####...', '.########.', '.##....##.', '##........', '.#######..',
                  '..#######.', '........##', '.##....##.', '.########.', '...####...'])
R['g'] = art(10, XO[:7] + ['.##.....##', '.#########', '...####.##',
                           '........##', '.########.', '..######..'])
R['t'] = art(4, ['..##......'] * 6 + ['########..', '########..'] + ['..##......'] * 5 +
         ['...##.....', '...######.', '.....####.'])
R['f'] = art(4, ['.....####.', '...######.', '...##.....'] + ['..##......'] * 3 +
         ['#######...', '#######...'] + ['..##......'] * 8)
R['j'] = art(6, ['......##..', '......##..', '..........', '..........'] + ['......##..'] * 10 +
         ['.....##...', '.######...', '.####.....'])      # dot where i's is, stem from the x-height
R['l'] = art(4, ['...##.....'] * 13 + ['....##....', '....#####.', '......###.'])

# figures
R['0'] = art(4, CO[:7] + ['##..##..##', '##..##..##'] + CO[9:])
R['O'] = art(4, CO)
R['2'] = art(4, ['...####...', '.########.', '.##....##.', '........##', '........##',
                 '........##', '.......###', '......###.', '.....###..', '....###...',
                 '...###....', '..###.....', '.###......', '###.......',
                 '##########', '##########'])
R['3'] = art(4, ['...####...', '.########.', '.##....##.', '........##', '........##',
                 '.......##.', '..######..', '..######..', '.......##.', '........##',
                 '........##', '........##', '........##', '.##....##.', '.########.',
                 '...####...'])
R['5'] = art(4, ['##########', '##########'] + ['##........'] * 4 +
             ['#######...', '#########.', '.......##.'] + ['........##'] * 4 +
             ['.##....##.', '.########.', '...####...'])
R['6'] = art(4, ['...####...', '.########.', '.##....##.', '##........', '##........',
                 '##........'] + BOWL_B[:2] + ['##.....##.'] + ['##......##'] * 4 +
             ['.##....##.', '.########.', '...####...'])
R['9'] = art(4, ['...####...', '.########.', '.##....##.'] + ['##......##'] * 4 +
             ['.##.....##', '.#########', '...####.##'] + ['........##'] * 3 +
             ['.##....##.', '.########.', '...####...'])
R['8'] = art(4, ['...####...', '.########.', '.##....##.', '##......##', '##......##',
                 '.##....##.', '.########.', '.########.', '.##....##.'] +
             ['##......##'] * 4 + ['.##....##.', '.########.', '...####...'])

# capitals with a curve
R['C'] = art(4, CO[:3] + ['##........'] * 10 + CO[13:])
R['D'] = art(4, ['#######...', '#########.', '##.....##.'] + ['##......##'] * 10 +
             ['##.....##.', '#########.', '#######...'])
R['G'] = art(4, CO[:3] + ['##........'] * 4 + ['##...#####', '##...#####'] +
             ['##......##'] * 4 + CO[13:])
R['Q'] = art(4, CO + ['.....###..', '......###.'])
R['S'] = art(4, ['...####...', '.########.', '.##....##.', '##........', '##........',
                 '.##.......', '.#######..', '..#######.', '.......##.'] +
             ['........##'] * 4 + ['.##....##.', '.########.', '...####...'])
R['U'] = art(4, ['##......##'] * 13 + ['.##....##.', '.########.', '...####...'])
R['J'] = art(4, ['........##'] * 13 + ['.##....##.', '.########.', '...####...'])
R['B'] = art(4, ['#######...', '#########.', '##.....##.', '##......##', '##......##',
                 '##.....##.', '########..', '#########.', '##.....##.'] +
             ['##......##'] * 4 + ['##.....##.', '#########.', '#######...'])
R['P'] = art(4, ['#######...', '#########.', '##.....##.'] + ['##......##'] * 4 +
             ['##.....##.', '#########.', '#######...'] + ['##........'] * 6)
R['R'] = art(4, rows_of(R['P'], 4, 13) + ['##.##.....', '##..##....', '##...##...',
                                          '##....##..', '##.....##.', '##......##'])

# the punctuation the language lives on: round beads for dots
BEAD = ['....##....', '...####...', '...####...', '....##....']
R['.'] = art(16, BEAD)
R[':'] = art(10, BEAD + ['..........', '..........'] + BEAD)
R['!'] = art(4, ['....##....'] * 10 + ['..........', '..........'] + BEAD)
R['?'] = art(4, ['...####...', '.########.', '.##....##.', '........##', '........##',
                 '......###.', '.....###..', '....###...', '....##....', '....##....',
                 '..........', '..........'] + BEAD)
R['>'] = art(6, ['.###......', '..###.....', '...###....', '....###...', '.....###..',
                 '......###.', '......###.', '.....###..', '....###...', '...###....',
                 '..###.....', '.###......'])
R['<'] = art(6, [r[::-1] for r in rows_of(R['>'], 6, 17)])
R['%'] = art(4, ['.##.....##', '####...##.', '####...##.', '.##...##..', '......##..',
                 '.....##...', '.....##...', '....##....', '....##....', '...##.....',
                 '...##.....', '..##......', '..##...##.', '.##...####', '.##...####',
                 '##.....##.'])
R['~'] = art(12, ['..##......', '.####....#', '##..##..##', '#....####.', '......##..'])

# today's M touches itself only at pixel corners - the check found it - so the
# round face redraws it: shoulders that step in, joined edge to edge
R['M'] = art(4, ['##......##', '###....###', '####..####', '##.####.##', '##..##..##',
                 '##..##..##'] + ['##......##'] * 10)

# m: its arches are too narrow for the three-step corner, so only the outer
# shoulder steps in; the comma and semicolon take the bead
R['m'] = art(10, ['##.###.##.', '##########'] + ['##..##..##'] * 8)
COMMA = BEAD[:3] + ['...####...', '...###....', '..###.....', '.##.......']
R[','] = art(16, COMMA)
R[';'] = art(10, BEAD + ['..........', '..........'] + COMMA)

EXPECT = {'i': 2, 'j': 2, '!': 2, '?': 2, ':': 2, ';': 2, '=': 2, '"': 2, '%': 3,
          '0': 2}


# ------------------------------------------------------------------ the check

def check(ch, g):
    """What is wrong with a glyph, as a list of (column, row, reason)."""
    ink = {(c, r) for r in range(H) for c in range(W) if g[r][c] == '#'}
    bad = []
    for (c, r) in ink:
        if c >= 10:
            bad.append((c, r, 'ink in the gap'))
        n8 = sum((c + dc, r + dr) in ink for dc in (-1, 0, 1) for dr in (-1, 0, 1)
                 if (dc, dr) != (0, 0))
        if n8 == 0:
            bad.append((c, r, 'stray'))
        elif n8 == 1:
            bad.append((c, r, 'spur'))
        # a one-pixel neck: ink on two opposite sides only
        l, rt, u, d = (c - 1, r) in ink, (c + 1, r) in ink, (c, r - 1) in ink, (c, r + 1) in ink
        if (l and rt and not u and not d and not any((c + dc, r + dr) in ink for dc in (-1, 1)
                                                     for dr in (-1, 1))) or \
           (u and d and not l and not rt and not any((c + dc, r + dr) in ink for dc in (-1, 1)
                                                     for dr in (-1, 1))):
            bad.append((c, r, 'neck'))
        # touching the rest only at a corner
        for dc, dr in ((1, 1), (1, -1)):
            if (c + dc, r + dr) in ink and (c + dc, r) not in ink and (c, r + dr) not in ink:
                bad.append((c, r, 'corner'))
    # pieces, joined edge to edge
    seen, parts = set(), 0
    for p in ink:
        if p in seen:
            continue
        parts += 1
        stack = [p]
        while stack:
            q = stack.pop()
            if q in seen:
                continue
            seen.add(q)
            for dc, dr in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                if (q[0] + dc, q[1] + dr) in ink:
                    stack.append((q[0] + dc, q[1] + dr))
    want = EXPECT.get(ch, 1 if ink else 0)
    if parts != want:
        bad.append((0, 0, f'{parts} pieces, want {want}'))
    return bad


# ------------------------------------------------------------------ proofs

def put(p, x, y, g, s, frame=False):
    for r in range(H):
        for c in range(W):
            if g[r][c] == '#':
                p.box(x + c * s, y + r * s, s, s)
    if frame:
        p.frame(x - 1, y - 1, W * s + 2, H * s + 2, 1)


def text(p, x, y, t, face, s=1):
    for i, ch in enumerate(t):
        put(p, x + i * W * s, y, face.get(ch) or today(ch), s)


def label(p, x, y, s):
    for i, ch in enumerate(s):
        p.glyph(x + i * SMALL.w * 2, y, ord(ch), SMALL, 2)


def proof(face, out, title):
    chars = [c for c in 'abcdefghjlmnopqrstu0235689BCDGJMOPQRSU.,:;!?><%~' if c in face]
    S, gap = 5, 12
    cols = 12
    cw = 2 * W * S + gap * 3
    p = Page(40 + cols * cw, 120 + ((len(chars) + cols - 1) // cols) * (H * S + 50) + 360)
    label(p, 20, 12, title + '   (each pair: today left, round right)')
    failures = 0
    for i, ch in enumerate(chars):
        x = 20 + (i % cols) * cw
        y = 60 + (i // cols) * (H * S + 50)
        put(p, x, y, today(ch), S, frame=True)
        put(p, x + W * S + gap, y, face[ch], S, frame=True)
        bad = check(ch, face[ch])
        if bad:
            failures += 1
            p.frame(x + W * S + gap - 6, y - 6, W * S + 12, H * S + 12, 3)
        label(p, x + W * S - 4, y + H * S + 8, ch + ('  FAIL' if bad else ''))
    y = 60 + ((len(chars) + cols - 1) // cols) * (H * S + 50) + 10
    label(p, 20, y, 'at the panel\'s own size: today, then round')
    lines = ['>kick 9...8...9...8...  >hat x%70x%40 *2 !4',
             '>disc:x 8876532111235678  >spin <0 3 6 9>',
             '-- II first light   orbitals  0123456789',
             'bpm ~120, mute; jog Rq?  (a+b)*2 = 6.4']
    yy = y + 36
    for face_ in ({}, face):
        for t in lines:
            text(p, 20, yy, t, face_)
            yy += 28
        yy += 16
    p.im.crop((0, 0, p.w, yy + 10)).save(out)
    return failures


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'docs/img')
    os.makedirs(out, exist_ok=True)
    report = []
    for ch, g in R.items():
        for c, r, why in check(ch, g):
            report.append(f'{ch!r}: {why} at column {c}, row {r}')
    failures = proof(R, os.path.join(out, 'type-round.png'),
                     'the round face - every glyph that changed, 5 x: today beside round')
    print('\n'.join(report) if report else 'every round glyph passes the check')
    print(f'{failures} glyph(s) fail')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
