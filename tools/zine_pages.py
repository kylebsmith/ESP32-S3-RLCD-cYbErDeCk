"""The hello zine's sixteen pages. Layout is code, as the instrument is text.

One idea a page, one narrow column - thirty columns, the deck's own line - on
a page that is mostly paper. Every claim is measured on the deck (and says
where), taken from the deck's own source - its replies are quoted from the
firmware, never paraphrased - or cited: [@key] resolves to a number in
zine_refs.REFS.

Each section is titled with a line you could type, and the deck's answer to
it - 'what? try: help', from firmware/components/cmd/cmd.c - is set beneath,
because a title is a command the instrument does not know. Sixteen pages are
one bar: the folio is the step.
"""
import os
from zine import BIG, SMALL, M, T, W, H, INK, PAPER

B = 48      # a line of body text: the 12x24 face at twice its size
S = 24      # a line of notes: the 6x12 face at twice its size


def art(d, name):
    return os.path.join(d, name + '.cells')


def title(p, cmd, reply, ink=INK, y=T):
    p.text(M, y, '>' + cmd, BIG, 3, ink=ink)
    p.glyph(M, y + 72 + 14, 137, SMALL, 2, ink=ink)
    p.text(M + 24, y + 72 + 14, reply, SMALL, 2, ink=ink)
    return y + 72 + 14 + S + 96


def para(p, y, text, ink=INK):
    return p.lines(M, y, text, BIG, 2, ink=ink)


def note(p, y, text, ink=INK):
    return p.lines(M, y, text, SMALL, 2, S, ink=ink)


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


# ------------------------------------------------------------------ 1

def cover(p, d):
    p.text(M, T, 'cYbErDeCk', SMALL, 2)
    p.text(M, T + S + 6, 'zine #0', SMALL, 2)
    # the deck's own ring, >disc 8 >edge 1, small and alone
    cells_small(p, W - M - 45 * 6, T, art(d, 'ring-0'), crop=(8, 1, 53, 23))
    y = 1640
    p.text(M, y, '>hello', BIG, 4)
    para(p, y + 96 + 48, 'a getting-started zine\nfor a box that plays text.')


# ------------------------------------------------------------------ 2

def epigraph(p, d):
    y = para(p, 980,
             'the engine\n'
             '"might compose elaborate\n'
             'and scientific pieces of\n'
             'music of any degree of\n'
             'complexity or extent."')
    p.text(M, y + 30, 'ada lovelace, 1843 [@lovelace]', SMALL, 2)


# ------------------------------------------------------------------ 3

def what(p, d):
    y = title(p, 'what', 'what? try: help')
    para(p, y, 'a keyboard, a screen that\nneeds daylight, a clock.')


# ------------------------------------------------------------------ 4

def lanes(p, d):
    para(p, T,
         'you type lines.\n'
         '\n'
         'a line is a lane.\n'
         '\n'
         'lanes play: drums, synths,\n'
         'filters, pictures.')
    para(p, 1500,
         'no mouse. no menu. no mode.\n'
         '\n'
         'the page you are editing\n'
         'is the page that plays.')


# ------------------------------------------------------------------ 5

def argument(p, d):
    para(p, 900,
         'laptop sets taught crowds\n'
         'to read projected code as\n'
         'proof that it was live\n'
         '[@toplap][@collins03][@burland].\n'
         '\n'
         'here the gesture and the\n'
         'sound are one thing to\n'
         'watch [@schloss][@reeves][@berthaut].')
    note(p, 2010,
         'an esp32-s3 and a 400x300 reflective panel, one bit, no\n'
         'backlight. 96 ticks a beat, 16 lanes, 34 verbs. usb midi\n'
         'jitter at the host: 0.03 ms, measured. it makes no sound\n'
         'itself: it sends midi, osc and pictures to what does.')


# ------------------------------------------------------------------ 6

def hello(p, d):
    y = title(p, 'hello', 'hello? try: help')
    p.text(M, y, '>kick 9...8...9...8...')
    y += B + 30
    # the same bar, velocity as ink: 9 solid, 8 a shade under, a rest faint
    tone = {'9': 136, '8': 134, '.': 129}
    p.text(M, y, [tone[c] for c in '9...8...9...8...'], BIG, 2)
    p.text(M, y + B + 18, 'velocity is ink', SMALL, 2)
    y += B + 18 + S + 90
    p.text(M, y, '>play')


# ------------------------------------------------------------------ 7

def keys(p, d):
    para(p, T,
         'ctrl+enter  runs the line\n'
         'enter       makes a line\n'
         '\n'
         'again, while it plays:\n'
         'quiet. again: back.')
    para(p, 1150,
         'it sends midi, not sound.\n'
         '\n'
         '>usb on  to a computer:\n'
         '         the deck restarts\n'
         '         as a midi device.\n'
         '>din 17  to a synth, with\n'
         '         no computer. build\n'
         '         the cable first.')
    note(p, 2010,
         'drums on channel 10, bass 1, lead 2, pad 3, arp 4: the\n'
         'names are lines in the boot document. change them there.')


# ------------------------------------------------------------------ 8

def grammar(p, d):
    y = title(p, 'grammar', 'grammar? try: help')
    for k, v in (('x', 'a hit'), ('.', 'a rest'), ('0-9', 'how much'),
                 ('_', 'hold the last one'), ('[xx]', 'two in one step'),
                 ('[0,4,7]', 'all at once'), ('<a b>', 'one each time round'),
                 ('x%30', '30 times in 100'), ('/2 *2', "this lane's speed"),
                 ('!4', 'four times, then end')):
        p.text(M, y, k)
        p.text(M + 9 * 24, y, v)
        y += B + 24


# ------------------------------------------------------------------ 9

def meter(p, d):
    para(p, T, 'a line is as long as it is.')
    # sixteen steps against twelve, for forty-eight: where each comes round
    y = 900
    for n in (16, 12):
        p.text(M, y, str(n), SMALL, 2)
        x = M + 3 * 12
        for s in range(49):
            p.glyph(x + s * 14, y, 147 if s % n == 0 else 137, SMALL, 2)
        y += 72
    p.text(M + 3 * 12 + 48 * 14 - 24, y, 'home', SMALL, 2)
    para(p, 1500,
         '16 against 12 comes home\n'
         'every 3 bars. that is all\n'
         'polyrhythm is.')


# ------------------------------------------------------------------ 10

def strudel(p, d):
    y = para(p, T,
             'strudel:  bd ~ sd ~\n'
             'here:     x.x.')
    note(p, y + 30,
         '51 of 51 corpus patterns play what strudel 1.2.6 plays,\n'
         'note for note [@strudel][@tidal].')
    y = note(p, 1300,
             'what the deck will not play, it refuses, and boxes the\n'
             'character:')
    y += 30
    for pat, why in (('x...X...', 'X is gone: 9 is loud'),
                     ('x...?...', '? is gone: x%50 is maybe'),
                     ('x.-.', '- is not a rest here: .')):
        x = p.text(M, y, '>hat ', SMALL, 2)
        for ch in pat:
            if ch in 'X?-':
                p.frame(x - 3, y - 3, 12 + 6, 24 + 6, 2)
            x = p.text(x, y, ch, SMALL, 2)
        p.text(M + 17 * 12, y, why, SMALL, 2)
        y += S + 14


# ------------------------------------------------------------------ 11

def names(p, d):
    y = title(p, 'names', 'names? try: help')
    y = para(p, y,
             'the names are yours:\n'
             '>conga = note 63\n'
             '>strings = voice 3 ch 5\n'
             '>fx = cc 20 ch 2\n'
             '>conga =         gone.')
    para(p, y + 150,
         'anything drives anything:\n'
         '>route cut kick\n'
         '>route disc kick\n'
         '>tom x.x.x.x. !2\n'
         '>route crash tom:end')


# ------------------------------------------------------------------ 12

def endings(p, d):
    para(p, T,
         'a lane that ends can start\n'
         'another. no timeline: an\n'
         'arrangement is endings.')
    note(p, 1000,
         '>knob1 = knob and >osc in 9000: a phone, a laptop or\n'
         'another deck sends /deck/knob1 and >route cut knob1\n'
         'follows it. the value lives here, so losing the phone\n'
         'loses nothing.')
    y = 1600
    p.text(M, y, 'the deck talks back:', SMALL, 2)
    y += S + 12
    for ln in ('_ needs a note before it', 'a chord goes in []: [0,4,7]',
               'a name is letters: conga', '16 lanes is all there is.',
               'run cannot run itself'):
        p.glyph(M, y, 146, SMALL, 2)
        p.text(M + 24, y, ln, SMALL, 2)
        y += S + 6


# ------------------------------------------------------------------ 13

def pictures(p, d):
    y = title(p, 'pictures', 'pictures? try: help')
    y = para(p, y, 'six fields, ten operators,\nno shapes.')
    y = note(p, y + 20,
             'fields  disc box turn ramp grid noise\n'
             'levels  mask edge\n'
             'bends   echo move spin warp grow thin flip fold')
    y = para(p, y + 40, 'a ring is a disc through an\nedge: >disc 8 >edge 1')
    y += 70
    p.cells(M, y, art(d, 'orbit-5'), 1, crop=(0, 1, 60, 22))
    y += 21 * 24 + 40
    for i, name in enumerate(('radar-0', 'day-5', 'eclipse-5')):
        cells_small(p, M + i * 246, y, art(d, name), crop=(11, 3, 49, 21))
    y += 216 + 40
    note(p, y,
         'orbitals, drawn by the deck: a planet, x at 16 steps and\n'
         'y at 12, round a sun on the beat; night, day, eclipse.\n'
         'echo keeps the trail: feedback is the trick [@hydra].')


# ------------------------------------------------------------------ 14

def band(p, d):
    para(p, 900,
         'for anyone who lost a\n'
         'night to a tracker, a\n'
         'patch cable or a\n'
         'semicolon.')
    para(p, 1500, 'x is a hit. . is a rest.\nnow form a band [@sideburns].')
    note(p, 2010,
         'on the deck: ground, lift, orbitals. open one, run it top\n'
         'to bottom. every set ends in silence.')


# ------------------------------------------------------------------ 15

def room(p, d):
    # the room is dark: the page reversed, the way >flip reverses a frame
    p.box(0, 0, W, H)
    y = title(p, 'room', 'room? try: help', ink=PAPER)
    for yr, who, what_ in (('1843', 'lovelace', 'an engine could compose [@lovelace]'),
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
                           ('2026', 'you', 'this box')):
        p.text(M, y, f'{yr}  {who:<16} {what_}', SMALL, 2, ink=PAPER)
        y += S + 12


# ------------------------------------------------------------------ 16

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
                line = '     ' + wd
            else:
                line = (line + ' ' + wd) if line else wd
        if line:
            p.text(M, y, line, SMALL, 2)
            y += S
        y += 8
    note(p, 2090,
         "set in the deck's own 12x24 and 6x12 faces; one bit; every\n"
         "picture drawn by its engine. a deck after gibson [@gibson], a\n"
         "grid after weingart, sometimes called swiss punk [@weingart].\n"
         "full references: zine/README.md. copy it, fold it, pass it on.")


PAGES = [cover, epigraph, what, lanes, argument, hello, keys, grammar,
         meter, strudel, names, endings, pictures, band, room, back]
