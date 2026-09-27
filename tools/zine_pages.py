"""The hello zine's eight pages. Layout is code, as the instrument is text.

Every claim here is measured on the deck (and says where), taken from the
deck's own source - its replies are quoted from the firmware, never
paraphrased - or cited: [@key] resolves to a number in zine_refs.REFS.

Each page is titled with a line you could type. The deck's answer to it -
from firmware/components/cmd/cmd.c, 'what? try: help' - is printed under it,
because a title is a command the instrument does not know.
"""
import os
from zine import BIG, SMALL, M, W, H, INK, PAPER, halftone, cite

B = 48      # a line of body text: the 12x24 face at twice its size
S = 24      # a line of notes: the 6x12 face at twice its size


def art(d, name):
    return os.path.join(d, name + '.cells')


def title(p, cmd, reply, ink=INK, y=M):
    """'>cmd' large, misregistered, and what the deck says to it."""
    p.text(M + 5, y + 5, '>' + cmd, BIG, 3, ink=ink, screen=halftone)
    p.text(M, y, '>' + cmd, BIG, 3, ink=ink)
    p.glyph(M, y + 72 + 6, 137, SMALL, 2, ink=ink)
    p.text(M + 24, y + 72 + 6, reply, SMALL, 2, ink=ink)
    return y + 72 + 6 + S + 20


def para(p, y, text, ink=INK):
    return p.lines(M, y, text, BIG, 2, ink=ink)


def note(p, y, text, ink=INK):
    return p.lines(M, y, text, SMALL, 2, S, ink=ink)


# ------------------------------------------------------------------ 1

def cover(p, d):
    # a column of the deck's nine tones down the right edge, bleeding off
    for lv in range(9):
        p.tone(W - 64, lv * (H // 9), 64 + 8, H // 9 + 1, 8 - lv, 1)
    p.text(M, M, 'cYbErDeCk / zine #0 / getting started', SMALL, 2)
    y = M + S + 50
    p.text(M + 7, y + 7, '>hell', BIG, 7, screen=halftone)
    p.text(M, y, '>hell', BIG, 7)
    # the o is the deck's own ring: >disc 8 >edge 1, drawn by viz.c
    ry = y + 168 + 10
    p.cells(M + 20, ry, art(d, 'ring-0'), 1, crop=(8, 1, 53, 23))
    p.text(W - 64 - 16 - 7 * 36, ry + 22 * 24 + 8, ', world', BIG, 3)
    y = ry + 22 * 24 + 8 + 72 + 36
    para(p, y, 'a getting-started zine\nfor a box that plays text.')
    p.rotated(W - 64 - 8 - S, M + 40,
              'x hit  . rest  0-9 how much  _ hold  [xx] split  '
              '<a b> alternate  x%30 maybe  /2 *2 speed  !4 end',
              SMALL, 2, 270)


# ------------------------------------------------------------------ 2

def what(p, d):
    y = title(p, 'what', 'what? try: help')
    y = para(p, y,
             'a keyboard, a screen that\n'
             'needs daylight, a clock.\n'
             '\n'
             'you type lines. a line is\n'
             'a lane. lanes play: drums,\n'
             'synths, filters, pictures.\n'
             '\n'
             'no mouse. no menu. no mode.\n'
             'the page you are editing\n'
             'is the page that plays.')
    y += 30
    blk = ('laptop sets taught crowds\n'
           'to read projected code as\n'
           'proof that it was live\n'
           '[@toplap][@collins03][@burland].\n'
           'here the gesture and the\n'
           'sound are one thing to\n'
           'watch [@schloss][@reeves][@berthaut].')
    p.box(M - 14, y - 14, 30 * 24 + 28, 7 * B + 28)
    para(p, y, blk, ink=PAPER)
    y += 7 * B + 44
    note(p, y,
         'an esp32-s3 and a 400x300 reflective panel, one bit, no\n'
         'backlight. 96 ticks a beat, 16 lanes, 34 verbs. usb midi\n'
         'jitter at the host: 0.03 ms, measured. it makes no sound\n'
         'itself: it sends midi, osc and pictures to what does.')


# ------------------------------------------------------------------ 3

def hello(p, d):
    y = title(p, 'hello', 'hello? try: help')
    p.text(M, y, '>kick 9...8...9...8...')
    y += B + 6
    # the same bar, with velocity as ink: 9 is solid, 8 a shade under,
    # a rest is the faintest tone there is
    tone = {'9': 136, '8': 134, '.': 129}
    p.text(M, y, [tone[c] for c in '9...8...9...8...'], BIG, 2)
    p.text(M + 16 * 24 + 12, y + 12, 'velocity is ink', SMALL, 2)
    y += B + 22
    p.text(M, y, '>play')
    y += B + 26
    y = para(p, y,
             'ctrl+enter  runs the line\n'
             'enter       makes a line\n'
             'again, while it plays:\n'
             'quiet. again: back.')
    y += 26
    y = para(p, y,
             'it sends midi, not sound.\n'
             '>usb on  to a computer:\n'
             '         the deck restarts\n'
             '         as a midi device.\n'
             '>din 17  to a synth, with\n'
             '         no computer. build\n'
             '         the cable first.')
    y += 22
    y = note(p, y,
             'drums on channel 10, bass 1, lead 2, pad 3, arp 4: the\n'
             'names are lines in the boot document. change them there.')
    y += 26
    para(p, y, 'x is a hit. . is a rest.\nnow form a band [@sideburns].')


# ------------------------------------------------------------------ 4

def grammar(p, d):
    y = title(p, 'grammar', 'grammar? try: help')
    rows = [('x', 'a hit'), ('.', 'a rest'), ('0-9', 'how much'),
            ('_', 'hold the last one'), ('[xx]', 'two in one step'),
            ('[0,4,7]', 'all at once'), ('<a b>', 'one each time round'),
            ('x%30', '30 times in 100'), ('/2 *2', "this lane's speed"),
            ('!4', 'four times, then end')]
    for k, v in rows:
        p.box(M - 6, y - 5, 8 * 24 + 12, B + 4)
        p.text(M, y, k, BIG, 2, ink=PAPER)
        p.text(M + 9 * 24, y, v)
        y += B + 8
    y += 10
    y = para(p, y,
             'a line is as long as it\n'
             'is. 16 against 12 comes\n'
             'home every 3 bars. that\n'
             'is all polyrhythm is.')
    y += 18
    y = note(p, y,
             'strudel writes bd ~ sd ~ where this writes x.x. and 51 of\n'
             '51 corpus patterns play what strudel 1.2.6 plays, note\n'
             'for note [@strudel][@tidal]. what the deck will not play,\n'
             'it refuses, and boxes the character:')
    y += 10
    for pat, why in (('x...X...', 'X is gone: 9 is loud'),
                     ('x...?...', '? is gone: x%50 is maybe'),
                     ('x.-.', '- is not a rest here: .')):
        x = p.text(M, y, '>hat ', SMALL, 2)
        for ch in pat:
            if ch in 'X?-':
                p.frame(x - 3, y - 3, 12 + 6, 24 + 6, 2)
            x = p.text(x, y, ch, SMALL, 2)
        p.text(M + 17 * 12, y, why, SMALL, 2)
        y += S + 8


# ------------------------------------------------------------------ 5

def names(p, d):
    y = title(p, 'names', 'names? try: help')
    y = para(p, y,
             'the names are yours:\n'
             '>conga = note 63\n'
             '>strings = voice 3 ch 5\n'
             '>fx = cc 20 ch 2\n'
             '>conga =         gone.')
    y += 26
    y = para(p, y,
             'anything drives anything:\n'
             '>route cut kick\n'
             '>route disc kick\n'
             '>tom x.x.x.x. !2\n'
             '>route crash tom:end')
    y += 22
    y = para(p, y,
             'a lane that ends can start\n'
             'another. no timeline: an\n'
             'arrangement is endings.')
    y += 22
    y = note(p, y,
             '>knob1 = knob and >osc in 9000: a phone, a laptop or\n'
             'another deck sends /deck/knob1 and >route cut knob1\n'
             'follows it. the value lives here, so losing the phone\n'
             'loses nothing.')
    y += 20
    p.text(M, y, 'the deck talks back:', SMALL, 2)
    y += S + 4
    for ln in ('_ needs a note before it', 'a chord goes in []: [0,4,7]',
               'a name is letters: conga', '16 lanes is all there is.',
               'run cannot run itself'):
        p.glyph(M, y, 146, SMALL, 2)
        p.text(M + 24, y, ln, SMALL, 2)
        y += S + 2


# ------------------------------------------------------------------ 6

def pictures(p, d):
    y = title(p, 'pictures', 'pictures? try: help')
    y = para(p, y, 'six fields, ten operators,\nno shapes.')
    y += 8
    y = note(p, y,
             'fields  disc box turn ramp grid noise\n'
             'levels  mask edge\n'
             'bends   echo move spin warp grow thin flip fold')
    y += 22
    # the orbit in orbitals, at the deck's own size: a 12x24 tile a cell
    p.cells(M, y, art(d, 'orbit-5'), 1, crop=(0, 1, 60, 22))
    p.frame(M - 8, y - 8, 60 * 12 + 16, 21 * 24 + 16, 2)
    y += 21 * 24 + 24
    # and three more, in the small tiles
    for i, name in enumerate(('radar-0', 'day-5', 'eclipse-5')):
        x0 = M + i * (240 + 12)
        cells_small(p, x0, y, art(d, name), crop=(10, 3, 50, 21))
        p.frame(x0 - 4, y - 4, 240 + 8, 216 + 8, 2)
    y += 216 + 14
    note(p, y,
         'orbitals, drawn by the deck: a planet, x at 16 steps and\n'
         'y at 12, round a sun on the beat; night, day, eclipse.\n'
         'echo keeps the trail: feedback is the trick [@hydra].')


def cells_small(p, x, y, path, crop=None):
    d = open(path, 'rb').read()
    w, h = d[0], d[1]
    c = d[2:]
    x0, y0, x1, y1 = crop or (0, 0, w, h)
    for cy in range(y0, y1):
        for cx in range(x0, x1):
            code = c[cy * w + cx]
            if code not in (32, 128):
                p.glyph(x + (cx - x0) * 6, y + (cy - y0) * 12, code, SMALL, 1)


# ------------------------------------------------------------------ 7

def room(p, d):
    # the room is dark: the page is reversed, the way >flip reverses a frame
    p.box(0, 0, W, H)
    y = title(p, 'room', 'room? try: help', ink=PAPER)
    rows = [('1843', 'lovelace', 'an engine could compose [@lovelace]'),
            ('1963', 'mathews', 'the computer, an instrument [@mathews]'),
            ('1964', 'riley', 'in c: 53 phrases, repeated [@riley]'),
            ('1968', 'reich', 'music as a gradual process [@reich]'),
            ('1971', 'xenakis', 'formalized music [@xenakis]'),
            ('1986', 'spiegel', 'music mouse [@spiegel]'),
            ('1987', 'obarski', 'the tracker [@obarski]'),
            ('1996', 'eno', 'generative music 1 [@eno]'),
            ('2002', 'mccartney', 'supercollider 3 [@mccartney]'),
            ('2003', 'collins et al', 'live coding [@collins03]'),
            ('2004', 'toplap', 'show us your screens [@toplap]'),
            ('2005', 'toussaint', 'euclid: open here still [@toussaint]'),
            ('2013', 'aaron, blackwell', 'sonic pi [@sonicpi]'),
            ('2014', 'mclean', 'tidal [@tidal]'),
            ('2014', 'collins, mclean', 'algorave [@algorave]'),
            ('2017', 'jack', 'hydra [@hydra]'),
            ('2023', 'roos, mclean', 'strudel [@strudel]'),
            ('2026', 'you', 'this box')]
    for yr, who, what_ in rows:
        p.text(M, y, f'{yr}  {who:<15} {what_}', SMALL, 2, ink=PAPER)
        y += S + 3
    y += 24
    y = para(p, y,
             'for anyone who lost a\n'
             'night to a tracker, a\n'
             'patch cable or a\n'
             'semicolon.', ink=PAPER)
    y += 24
    para(p, y,
         'on the deck: ground, lift,\n'
         'orbitals. open one, run it\n'
         'top to bottom. every set\n'
         'ends in silence.', ink=PAPER)


# ------------------------------------------------------------------ 8

def back(p, d):
    y = title(p, 'help', '34 commands - the one line here it knows')
    from zine_refs import REFS
    for n, (_, ref) in enumerate(REFS, 1):
        words = f'[{n}] {ref}'.split(' ')
        line = ''
        for wd in words:
            if len(line) + len(wd) + 1 > 60:
                p.text(M, y, line, SMALL, 2)
                y += S
                line = '    ' + wd
            else:
                line = (line + ' ' + wd) if line else wd
        if line:
            p.text(M, y, line, SMALL, 2)
            y += S
        y += 1
    y += 14
    note(p, y,
         "set in the deck's own 12x24 and 6x12 faces; one bit; every\n"
         "picture drawn by its engine. a deck after gibson [@gibson], a\n"
         "grid after weingart, sometimes called swiss punk [@weingart].\n"
         "full references: zine/README.md. copy it, fold it, pass it on.")


PAGES = [cover, what, hello, grammar, names, pictures, room, back]

# PAGE ORDER ON THE SHEET: the standard one-sheet, one-cut mini zine - top
# row upside down, 5 4 3 2; bottom 6 7 8 1 (Wikibooks, Zine Making/Putting
# pages together; Princeton University Library's single-sheet template).
IMPOSITION = [
    [(5, True), (4, True), (3, True), (2, True)],
    [(6, False), (7, False), (8, False), (1, False)],
]
