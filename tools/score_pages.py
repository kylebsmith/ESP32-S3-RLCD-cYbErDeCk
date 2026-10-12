"""Zine #1, the score book: the set's six tracks as their scores, a spread each.

The right page of a spread is the score: the whole document as the deck holds
it, read from pieces/ at build time, in the deck's 60-column face - headings
red, because a heading is what you press. The left page is the same score read
out: what it is in, what the voice says, and the one figure that makes the
track, drawn from the lines on the right. Nothing on a left page is not in the
score beside it.

Sixteen pages, one bar: a cover, how to walk a score, the sounds (one patch a
track, one voice a bank), six spreads in set order, and the back.
"""
import os
import re
from PIL import Image

from zine import BIG, SMALL, M, T, W, H, INK, PAPER, RED, Page
from zine_pages import title, para, note, art

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
S2 = 24     # a row of the 6x12 face at twice its size: the deck's 60 columns
LEAD = 36   # a score's row on the page: half a row more, to read it by
FOOT = 1720  # where a left page's facts begin, low, beside its number


def score(name):
    return open(os.path.join(REPO, 'pieces', name + '.txt')).read()


def big(p, x, y, s, sc, ink=INK):
    return p.text(x, y, s, BIG, sc, ink)


def mirrored(p, x, y, s, sc, ink=INK):
    """A word set backwards, the way a record runs when it is pulled back."""
    tmp = Page(len(s) * BIG.w * sc, BIG.h * sc)
    tmp.text(0, 0, s, BIG, sc)
    m = tmp.im.transpose(Image.FLIP_LEFT_RIGHT)
    mask = m.convert('L').point(lambda v: 255 if v == 0 else 0).convert('1')
    p.im.paste(ink, (x, y), mask)


def facts(p, y, lines, ink=INK):
    """The score's numbers, plainly, in the body face."""
    for ln in lines:
        p.text(M, y, ln, BIG, 2, ink)
        y += 48
    return y


def keys(p, y, names, ink=INK):
    """What F1-F8 play, from the score's own >keys line."""
    x = M
    for i, n in enumerate(names, 1):
        x = p.text(x, y, 'f%d' % i, SMALL, 2, RED)
        x = p.text(x + 6, y, n, SMALL, 2, ink) + 30
        if i == 4:
            x, y = M, y + S2 + 6
    return y + S2


def low(p, lines, names, ink=INK):
    """The facts and the keys, set so they end just above the page's number."""
    y = 1880 - (len(lines) * 48 + 30 + 2 * S2 + 6)
    keys(p, facts(p, y, lines, ink) + 30, names, ink)


def keys_of(name):
    return re.search(r'>keys ([^\n]+)', score(name)).group(1).split()


def opener(p, name, ink=INK):
    return title(p, 'open ' + name, name, ink=ink)


# ------------------------------------------------------------------ the score

def wrap(line, cols=60):
    """A line longer than the column wraps as the deck wraps it, with the rows
    after the first indented two past the line's own indent."""
    if len(line) <= cols:
        return [line]
    ind = len(line) - len(line.lstrip(' '))
    rows, rest = [line[:cols]], line[cols:]
    while rest:
        pad = ' ' * (ind + 2)
        rows.append(pad + rest[:cols - len(pad)])
        rest = rest[cols - len(pad):]
    return rows


def score_page(p, name):
    """The whole document in two columns, broken at the heading nearest half."""
    p.no_numeral = True
    rows = []
    for ln in score(name).rstrip('\n').split('\n'):
        head = ln.lstrip().startswith('--')
        for i, r in enumerate(wrap(ln)):
            rows.append((r, head, i == 0))
    heads = [i for i, (r, h, first) in enumerate(rows) if h and first]
    cut = min(heads, key=lambda i: abs(i - len(rows) / 2))
    for col, part in ((0, rows[:cut]), (1, rows[cut:])):
        x, y = 85 + col * 760, 170
        for r, head, first in part:
            if head:
                p.text(x, y, r, SMALL, 2, RED)
            else:
                k = r.find('>')
                if first and k >= 0 and r[:k].strip() == '':
                    p.text(x, y, r[:k], SMALL, 2)
                    p.text(x + k * 12, y, '>', SMALL, 2, RED)
                    p.text(x + (k + 1) * 12, y, r[k + 1:], SMALL, 2)
                else:
                    p.text(x, y, r, SMALL, 2)
            y += LEAD


# ------------------------------------------------------------------ 1

def cover(p, d):
    # THE GRID FIELD, '>grid 5', as the deck's engine draws it: a lattice, red,
    # twice its size, the top of the page - staff paper for a box that plays
    # text. The black type prints over it.
    p.cells(-60, -260, art(d, 'spec-grid-0'), 2, ink=RED)
    p.text(M, T, 'cYbErDeCk', SMALL, 2)
    p.text(M, T + S2 + 6, 'zine #1', SMALL, 2)
    y = 1500
    x = p.text(M - 36, y, '>', BIG, 10, ink=RED)
    p.text(x, y, 'score', BIG, 10)
    for k, ln in enumerate(('six tracks, as the deck', 'holds them, and read out.')):
        p.text(M + k * 96, y + 240 + 72 + k * 60, ln, BIG, 2)


# ------------------------------------------------------------------ 2

def walk(p, d):
    y = opener(p, 'set')
    y = para(p, y,
             'a score is a document.\n'
             '\n'
             'walk down it:\n'
             'f10, the next heading.\n'
             'ctrl+enter runs it.')
    # two bars: pressed on step 9, it starts on the next one
    y += 90
    for bar in range(2):
        for s in range(16):
            x = M + bar * 16 * 36 + s * 36 + bar * 24
            on = (bar == 1 and s == 0)
            code = 136 if on else (147 if s % 4 == 0 else 137)
            p.glyph(x, y, code, BIG, 3 if on else 3, RED if on else INK)
    p.glyph(M + 8 * 36, y + 72 + 12, 146, BIG, 3, RED)
    p.text(M + 8 * 36 - 36, y + 72 + 12 + 84, 'you', SMALL, 2, RED)
    p.text(M + 16 * 36 + 24, y + 72 + 12 + 84, 'it starts here', SMALL, 2, RED)
    y += 72 + 12 + 84 + S2 + 90
    para(p, y,
         'everything lands on the\n'
         'one: a part, a change, a\n'
         'mute. run it any time in\n'
         'the bar before.\n'
         '\n'
         'f1-f8: a part out, a part\n'
         'back. a heading again: its\n'
         'lanes out. again: back.')


# ------------------------------------------------------------------ 3

BAY = (('1', 'bass1', (1, 6)), ('2', 'lead1', (1, 2, 3, 4, 6)), ('3', 'keys', (1, 2, 3, 4, 6)),
       ('4', 'stab', (5,)), ('5', 'bass2', (2,)), ('6', '808', (3, 4)),
       ('9', 'rumble', (5,)),
       ('10', 'drums', (1, 2, 3, 4, 5, 6)), ('11', 'bell', (3, 6)),
       ('12', 'samples', (1, 2, 3, 4, 5, 6)), ('13', 'acid', (5,)))


def sounds(p, d):
    y = title(p, 'patch 4', 'patch 4: cc 3, ch 1-15')
    y = para(p, y, 'one channel a sound, one\nchain a track. the patch\npicks the chain.')
    y += 50
    x0 = M + 10 * 24
    for c, name in enumerate(('grid', 'offset', 'weight', 'heat', 'press', 'orbit')):
        p.text(x0 + c * 72, y, str(c + 1), SMALL, 2, RED)
    y += S2 + 16
    for ch, name, used in BAY:
        p.text(M, y, '%2s %s' % (ch, name), SMALL, 2)
        for c in range(6):
            on = (c + 1) in used
            p.glyph(x0 + c * 72, y - 12, 136 if on else 137, BIG, 2, RED if (on and name == 'samples') else INK)
        y += 52
    p.text(M, y + 4, '1 grid  2 offset  3 weight  4 heat  5 pressure  6 orbitals', SMALL, 2, RED)
    y += 80
    y = para(p, y, 'a sample is a note: drop it\non its pad, channel 12.')
    y += 30
    pads = (('c1', 'one two three four'), ('c#1', 'pull up'), ('d1', 'very demure, very mindful'),
            ('d#1', 'oof'), ('e1', 'shadow wizard money gang'), ('f1', 'we love casting spells'),
            ('g1', 'we live in a society'), ('g#1', 'bass boosted'),
            ('a1', "that's here. that's home."), ('a#1', 'the xp shutdown'))
    for i, (pad, w_) in enumerate(pads):
        p.text(M, y + i * (S2 + 8), pad, SMALL, 2, RED)
        p.text(M + 60, y + i * (S2 + 8), w_, SMALL, 2)


# ------------------------------------------------------------------ 4, 5: GRID

def grid_read(p, d):
    y = opener(p, 'grid')
    # the voice counts the grid in: four steps of sixteen, said
    for s in range(16):
        on = s % 4 == 0
        p.glyph(M + s * 72, y, 136 if on else 137, BIG, 6, INK)
    for k, w_ in enumerate(('one', 'two', 'three', 'four')):
        p.text(M + k * 4 * 72, y + 144 + 20, w_, BIG, 3, RED)
    p.text(M, y + 144 + 20 + 72 + 16, '>count 9............... !1   a computer voice, once', SMALL, 2)
    y += 144 + 20 + 72 + 16 + S2 + 110
    # one hand, four chords: the stab's notes, the same four over four basses
    stab = ('ab', 'eb', 'c', 'ab')          # [2,4,6,9] in f minor, top down
    for k, (bass, ch) in enumerate((('f', 'fm9'), ('db', 'dbmaj9#11'), ('bb', 'bb9sus'), ('c', 'cm(b13)'))):
        x = M + k * 320
        for j, n in enumerate(stab):
            p.text(x, y + j * 84, n, BIG, 3, RED)
        p.text(x, y + 4 * 84 + 10, bass, BIG, 8)
        p.text(x, y + 4 * 84 + 10 + 192 + 14, ch, SMALL, 2)
    low(p, ('128 bpm, f minor.', 'one organ stab over a moving', 'bass: four chords, one hand.'), keys_of('grid'))


def grid_score(p, d):
    score_page(p, 'grid')


# ------------------------------------------------------------------ 6, 7: OFFSET

def offset_read(p, d):
    y = opener(p, 'offset')
    # straight, then swung 62: every odd sixteenth late by a quarter of a step
    for row, (label, sw) in enumerate((('straight', 0), ('swing 62', 0.24))):
        p.text(M, y, label, SMALL, 2, RED if sw else INK)
        for s in range(16):
            late = s % 2 == 1
            x = M + 240 + int((s + (sw if late else 0)) * 62)
            p.glyph(x, y - 12, 147, BIG, 2, RED if (late and sw) else INK)
        y += 110
    y += 120
    # the MC's word, set backwards: a rewind
    mirrored(p, M - 20, y, 'pull up', 12)
    p.text(M, y + 288 + 30, '>pullup 9............... !1', SMALL, 2, RED)
    p.text(M, y + 288 + 30 + S2 + 8, '>demure 9............... !1   on the drop', SMALL, 2)
    low(p, ('132 bpm, a dorian.', 'am9 d9sus cmaj7 g6: an organ', 'stab that moves in the bar,', 'and a plucked hook over it.'), keys_of('offset'))


def offset_score(p, d):
    score_page(p, 'offset')


# ------------------------------------------------------------------ 8, 9: WEIGHT

def weight_read(p, d):
    y = opener(p, 'weight')
    # the four chords as weights: each voicing a stack, standing on its 808 note
    chords = (('dmaj9', 'd', (54, 57, 61, 64)), ('c#m7', 'c#', (56, 59, 61, 64)),
              ('bm9', 'b', (54, 57, 61, 62)), ('f#m9', 'f#', (56, 57, 61, 64)))
    top = y + 60
    for k, (name, root, notes) in enumerate(chords):
        x = M + k * 320
        for n in notes:                       # a semitone is 36 px: the gaps show
            p.box(x, top + (65 - n) * 36, 230, 26, RED)
        p.text(x, top + 12 * 36 + 40, root, BIG, 8)
        p.text(x, top + 12 * 36 + 40 + 192 + 14, name, SMALL, 2)
    # the slides: a tie into the next note glides - d down to c#, b down to f#
    yb = top + 12 * 36 + 40 + 96
    for k in (0, 2):
        for i in range(7):
            p.box(M + k * 320 + 200 + i * 16, yb + i * 12, 14, 14, INK)
    p.text(M, top + 12 * 36 + 40 + 192 + 14 + S2 + 30, 'the 808: a tie into the next note slides.', SMALL, 2)
    low(p, ('150 bpm, f# minor.', 'the bridge: amaj9 e6/9', 'dmaj9 c#7sus. each drop', 'lands on an oof.'), keys_of('weight'))


def weight_score(p, d):
    score_page(p, 'weight')


# ------------------------------------------------------------------ 10, 11: HEAT

def heat_read(p, d):
    y = opener(p, 'heat')
    # the royal road, and the one note all four chords keep
    for k, ch in enumerate(('amaj7#11', 'b9', 'g#m7', 'c#m9')):
        p.text(M + k * 320, y, ch, BIG, 2)
        p.glyph(M + k * 320, y + 60, 136, BIG, 2, RED)
    p.text(M, y + 60 + 60, 'd#, held through all four', SMALL, 2, RED)
    y += 60 + 60 + S2 + 110
    # the 808 in the drop: >bass <3__7_3_7 ...> - every note tied into the
    # next, so it never lands: a fifth up and back, four roots, all glide
    bars = ((33, 40), (35, 42), (32, 39), (37, 44))
    pts = []
    for b, (lo, hi) in enumerate(bars):
        for e, n in ((0, lo), (3, hi), (5, lo), (7, hi)):
            pts.append((M + (b * 8 + e) * 40, n))
    pts.append((M + 32 * 40, 33))
    base = y + 15 * 22
    for (x0, n0), (x1, n1) in zip(pts, pts[1:]):
        steps = max(1, (x1 - x0) // 8, abs(n1 - n0) * 2)
        for i in range(steps):
            fx = x0 + (x1 - x0) * i // steps
            fy = base - (n0 + (n1 - n0) * i / steps - 30) * 22
            p.box(fx, int(fy), 14, 14, RED)
    for b, name in enumerate(('a', 'b', 'g#', 'c#')):
        p.text(M + b * 320, base + 40, name, BIG, 3)
    y = base + 40 + 72 + 40
    p.text(M, y, '>bass <3__7_3_7 4__8_4_8 2__6_2_6 5__9_5_9> /2', SMALL, 2)
    y += S2 + 80
    p.text(M, y, 'shadow wizard money gang', BIG, 2)
    p.text(M, y + 52, 'we love casting spells', BIG, 2, RED)
    p.text(M, y + 52 + 56, 'once, in the chaos. the second starts as the first ends:', SMALL, 2)
    p.text(M, y + 52 + 56 + S2 + 6, '>route spells gang:end', SMALL, 2, RED)
    low(p, ('160 bpm, e major.', 'an 808 that never lands,', 'a bright hook that glides,', 'four on the floor, crushed.'), keys_of('heat'))


def heat_score(p, d):
    score_page(p, 'heat')


# ------------------------------------------------------------------ 12, 13: PRESSURE

def pressure_read(p, d):
    # the room goes dark for this one
    p.box(0, 0, W, H)
    p.dark = True
    y = title(p, 'open pressure', 'pressure', ink=PAPER)
    p.text(M, y, 'we live in', BIG, 6, PAPER)
    p.text(M, y + 160, 'a society', BIG, 6, PAPER)
    y += 160 + 144 + 50
    p.text(M, y, '>society 9............... !1   alone, in a bar of nothing', SMALL, 2, RED)
    p.text(M, y + S2 + 8, '>boosted 9............... !1   on the drop', SMALL, 2, RED)
    y += 2 * S2 + 90
    # the stab in threes over the kick in fours, home every three bars
    for row, (label, n, ink) in enumerate((('kick', 4, PAPER), ('stab', 3, RED))):
        p.text(M, y, label, SMALL, 2, ink)
        for s in range(48):
            p.glyph(M + 90 + s * 24, y, 147 if s % n == 0 else 137, SMALL, 2, ink)
        y += 60
    p.text(M + 90 + 47 * 24 - 24, y, 'home', SMALL, 2, RED)
    low(p, ('140 bpm, e phrygian.', 'em(b9) every three', 'sixteenths. a kick and its', 'rumble, acid, then the kick', 'thrown off the grid.'), keys_of('pressure'), ink=PAPER)


def pressure_score(p, d):
    score_page(p, 'pressure')


# ------------------------------------------------------------------ 14, 15: ORBITALS

def orbitals_read(p, d):
    # the orbit, as the deck draws it: a planet round a sun, red, off the edge
    p.cells(W - 1100, 520, art(d, 'orbit-5'), 2, crop=(0, 1, 60, 22), ink=RED)
    y = opener(p, 'orbitals')
    y += 520
    # one voice through the eclipse, and the set shutting down after it
    for k, ln in enumerate(("that's here.", "that's home.", "that's us.")):
        p.text(M, y + k * 130, ln, BIG, 5)
    y += 3 * 130 + 40
    p.text(M, y, '>sagan 9............... !1   carl sagan, in the eclipse', SMALL, 2)
    p.text(M, y + S2 + 8, '>bye 9............... !1   the xp shutdown, the last sound', SMALL, 2, RED)
    low(p, ('124 bpm, d minor.', 'dm9 bbmaj7 fmaj7 c6 under', 'glass, a slow tune, an', 'eclipse in half time, a lift.'), keys_of('orbitals'))


def orbitals_score(p, d):
    score_page(p, 'orbitals')


# ------------------------------------------------------------------ 16

def back(p, d):
    y = title(p, 'help', '39 commands - the one line here it knows')
    for n, (name, bpm, key) in enumerate((('grid', 128, 'f minor'), ('offset', 132, 'a dorian'),
                                          ('weight', 150, 'f# minor'), ('heat', 160, 'e major'),
                                          ('pressure', 140, 'e phrygian'), ('orbitals', 124, 'd minor')), 1):
        x = p.text(M, y, str(n), BIG, 2, RED)
        p.text(x + 24, y, '%-9s %d  %s' % (name, bpm, key), BIG, 2)
        y += 64
    y += 60
    para(p, y,
         'nothing here starts when you\n'
         'press it. it starts on the\n'
         'one.')
    note(p, 2010,
         "the right pages are the documents the deck runs, pieces/,\n"
         "set in its own 6x12 face; the left pages read them out in\n"
         "its 12x24. two inks, one bit each. the samples go on their\n"
         "pads: pieces/ABLETON.md. copy it, fold it, pass it on.")


PAGES = [cover, walk, sounds, grid_read, grid_score, offset_read, offset_score,
         weight_read, weight_score, heat_read, heat_score, pressure_read,
         pressure_score, orbitals_read, orbitals_score, back]
