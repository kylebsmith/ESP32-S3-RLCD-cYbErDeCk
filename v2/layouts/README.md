# The layout

`slab.py` — a desk instrument. Render it with:

```
python3 -I v2/tools/layout.py v2/layouts/slab.py v2/layouts/out/slab.svg
```

The checks read the emitted SVG, not the spec, so a bug in the drawing shows up
instead of passing quietly. Exit code is 1 if anything fails.

## What changed from the handheld studies

The earlier two-band and three-band layouts assumed a handheld object, which
made thumb reach the binding constraint and forced controls into a narrow band
within 45 mm of the keys. This is a **desk or lap slab**, two hands, so that
constraint is gone and the whole panel is in play. Those layouts are deleted
rather than kept — they were answering a question nobody is asking any more.

The part list also shrank:

| | |
|---|---|
| gone | the Quad Rotary. The encoders are individual STEMMA breakouts. |
| gone | the NeoKey 1×4. There is a keyboard. |
| gone | the panel-mount pots. |
| kept | one NeoSlider, four individual I²C rotary encoders, the display |

## The grid is the MX key unit's — and the keyboard's own is not that number

The module is the **MX key unit, 19.05 mm**, and the control field is **15 u =
285.75 mm**. This used to read "— the K9 Pro's own switch field". It is not.
All 61 of the K9 Pro's switch holes land on an exact **19.000 mm** lattice in
both axes, read off the analytic hole axes in Keychron's own assembly, so the
keyboard's switch field is **285.000 × 95.000** and the claim was 0.750 mm out
(`../parts/keyboard-strip.md` §1.6). `../cad/parameters.scad` now carries the two
separately, as `U = 19.050` and `kbd_U = 19.000`.

So the band above the keys and the keys themselves are **not** on one grid:
they agree to 0.050 mm per unit and drift to 0.750 mm across the field. Nothing
in this layout is datumed off the keyboard's lattice — the keyboard is a pocket,
not a placement — so the drift costs nothing here. But it is drift, not
alignment, and the first element that wants to line up with a key column will
have to pick a side. The 19.050 field below is the **control** field, and every
number in it is still right:

```
      5 u              5 u              5 u
  +------------+   +------------+   +------------+
  |   SCREEN   |   |   FADER    |   |  ENCODERS  |
  +------------+   +------------+   +------------+
       95.25            95.25            95.25     = 285.75
```

The air inside a column *is* the column. Three elements on a 285.75 mm field
are sparse however they are placed, so the field is divided first and the parts
sit in the divisions — rather than the parts being placed and the leftovers
called design.

## Everything aligns to something

That is the difference between a grid and three centred objects.

| | mm | aligns to |
|---|---|---|
| display active height | 63.60 | — |
| encoder grid height | 63.50 | the display, to 0.10 mm |
| fader board length | 76.20 | the band height exactly, both edges |
| display board width | 93.90 | its 95.25 column, to 1.35 mm |

The encoder grid is 38.10 pitch + 25.40 knob = 63.50. That is why the knob is
25.40 and not larger: at this pitch a bigger knob breaks the alignment with the
screen, and at a bigger pitch the grid outgrows the screen. The knob gap is
12.70.

## Why the screen is the left column

Not composition. The display board is 93.90 wide and fills its 95.25 column to
within 1.35 mm, and its left edge lands **1.975 mm off the interior wall**, so
the board's side ports reach that wall directly and need no channel across the
slab. The requirement that the ESP32's ports stay accessible puts the board at
an edge, and the board is behind the screen.

## The keyboard seats flat, and the typing angle is now ours

Read off the analytic surfaces in Keychron's own STEP file, not sampled
(`../parts/keyboard-strip.md` §1.4, §2.1):

```
frame top        y = 0.000, ONE flat planar face, normal exactly (0,+1,0)
tray underside   one inclined plane: -13.274 at the rear -> -7.573 at the front
slope            3.37914 deg, off that plane's own normal
                 Keychron publishes 13.3 / 7.6 — agreement to 0.027 and 0.026
```

**The bottom is the wedge, not the top** — which is the inverse of what the
research inferred from the published heights. An earlier pass sectioned the
solid at 26 depths and got the same geometry to a rounding (3.374°, rear 13.25,
front 7.60); the plane's normal settles it without sampling.

Two things this section used to say are **wrong**, and the same measurement
pass disproved both:

- **"no feet are modelled."** It came from the case-only STEP, which does not
  contain them. The full assembly does: `BIG_HOLDER` / `SMALL_HOLDER` ×4 and
  two `FOOT_STOOL_ASM`, one drawn **deployed**, and the deployed one reaches
  **y = −22.100** — the lowest point of anything in the file. They go with the
  tray, and good: a deployed foot would foul any bay floor.
- **"No ramp is needed anywhere."** True of the keyboard mounted *whole*, and
  it stops being true the moment the tray comes off — which is the decision
  (`../parts/keyboard-strip.md`). The wedge is in the ABS tray; strip it and
  the plate is **flat**, so the typing angle stops being the keyboard's and
  becomes the enclosure's. **It has been decided: ZERO**
  (`../cad/parameters.scad` §8b, `kbd_tilt_deg = 0`). Positive tilt extends the
  wrist, which is the posture that hurts; keeping the stock 3.37914° would also
  cost 5.639 mm of carrier depth at the rear.

A flat bay floor is still the correct seat — now because flat was chosen, not
because the keyboard brought its own angle.

## Still open

- Whether the NeoKey and the panel-mount pots are really gone, or only gone
  from this arrangement.
- NeoSlider travel is still **NOT SOURCED** (`parts.md` §10); the 50.80 mm slot
  is a placeholder and the fader's slot length will move when it is measured.
- ~~Port and toggle positions on the keyboard's rear edge~~ — **settled by
  measurement**, `../parts/keyboard-strip.md` §4.1: all four apertures are cut
  in the top frame's own rear wall at z = 8.000, a part the strip keeps, with
  the USB-C at X 123.500 (9.640 × 3.860), the mode toggle at 28.500, the OS
  toggle at 9.500 and the indicator at 140.520. The spine relief is now a
  drawing job, not an open question.
