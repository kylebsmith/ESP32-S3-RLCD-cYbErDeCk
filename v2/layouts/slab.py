"""The slab -- a desk instrument, one band, three equal columns.

Not handheld. It sits on a desk or a lap, so thumb reach stops being the
constraint and the whole panel is in play with two hands.

The module is the KEYBOARD'S OWN key unit, 19.05 mm, and the control field is
15 u -- exactly the K9 Pro's switch field, 285.75 mm. So the band above the
keys and the keys themselves are laid out on one grid, and the field divides
into three equal 5 u columns with nothing left over:

        5 u              5 u              5 u
    +------------+   +------------+   +------------+
    |   SCREEN   |   |   FADER    |   |  ENCODERS  |
    +------------+   +------------+   +------------+
         95.25            95.25            95.25      = 285.75

The air inside a column is the column. That is the difference between
structured space and a gap -- three elements on a 285.75 mm field are sparse
however they are placed, so the field is divided first and the parts sit in the
divisions, rather than the parts being placed and the leftovers called design.

The screen board is 93.90 wide and its column is 95.25: it fills its column to
within 1.35 mm, which is why the screen is the left column and not a choice.
Its left edge lands 1.975 mm off the interior wall, so the board's side ports
reach that wall directly and need no channel across the slab.

Everything aligns to something, which is the difference between a grid and
three centred objects. The encoder grid is 38.10 + 25.40 = 63.50 tall and the
display's active area is 63.60: the knobs stand exactly as tall as the screen.
The fader board is 76.20 long, which is the band height, so stood on end it
meets both edges of the band and is the element that declares its depth.
"""
import _common as C
import parts as P

U = 19.050                  # the keyboard's key unit, and the panel's module
FIELD = 15 * U              # 285.75, the K9 Pro's switch field exactly
COL = 5 * U                 # 95.25, one third of the field
BAND = 4 * U                # 76.200, and the vertical fader board fills it

SCREEN_X, FADER_X, ENC_X = -COL, 0.0, COL
ENC_PITCH = 2 * U           # 38.100; half of it is 19.050, on the grid
ENC_KNOB = 25.400           # 1 in. Chosen so that pitch + knob = 63.50, which
                            # is the display's 63.60 active height to within
                            # 0.10 mm: the four knobs stand exactly as tall as
                            # the screen. Knob gap is 12.70.


def build(Panel, ctl, display, plate, bay, keyboard):
    import layout as L
    p = C.frame(L, BAND, "SLAB  screen / fader / encoders", board_band=False)
    d = p.datums
    mid = d["band_lo"] + BAND / 2

    p.add(C.kbd(L, d))
    p.add(plate(0.0, mid, FIELD, BAND))
    p.add(display(SCREEN_X, mid))

    # the fader runs vertically: its 76.200 board is exactly the band height,
    # so it meets the band's top and bottom edges and is the one element that
    # declares the band's full depth
    p.add(bay(FADER_X, mid, "_fader board", pattern="Bv", mirror=FADER_X))
    p.add(ctl("slider", FADER_X, mid, P.SLIDER_SLOT[1], P.SLIDER_SLOT[0],
              "FADER"))

    for i, (sx, sy) in enumerate(((-1, 1), (1, 1), (-1, -1), (1, -1))):
        x = ENC_X + sx * ENC_PITCH / 2
        y = mid + sy * ENC_PITCH / 2
        p.add(bay(x, y, f"_enc board {i+1}", pattern="A", mirror=ENC_X))
        p.add(ctl("encoder", x, y, ENC_KNOB, ENC_KNOB, f"E{i+1}", mirror=ENC_X))

    p.note(f"deck {p.bw:.2f} x {p.bh:.2f} mm, a flat slab")
    p.note(f"field {FIELD:.2f} = 15 u = the keyboard's switch field; "
           f"three columns of {COL:.2f}")
    p.note("bay floor FLAT: the case bottom is one inclined plane at 3.374 deg "
           "(measured from Keychron's STEP), so it seats flat and keeps its "
           "own typing angle")
    return p
