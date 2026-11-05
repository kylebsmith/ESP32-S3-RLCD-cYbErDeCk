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

## The grid is the keyboard's

The module is the **key unit, 19.05 mm**, and the control field is **15 u =
285.75 mm — the K9 Pro's own switch field**. The band above the keys and the
keys themselves are on one grid. The field divides into three equal 5 u columns
with nothing left over:

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

## The keyboard seats flat

Measured from Keychron's own STEP file by sectioning the solid at 26 depths:

```
top surface     y = 0.00 at every depth          FLAT
bottom          -13.15 at the rear -> -7.69 at the front
slope           3.374 deg, linear to 0.000 mm at every sample
extrapolated    rear 13.25, front 7.60   (Keychron publishes 13.3 / 7.6)
```

**The bottom is the wedge, not the top** — which is the inverse of what the
research inferred from the published heights. Because the bottom is a single
inclined plane, it sits flat on a flat floor and keeps its own 3.374° typing
angle. **No ramp is needed anywhere.** A flat bay floor is the correct seat,
and the zero deviation across all 26 samples also means no feet are modelled.

## Still open

- Whether the NeoKey and the panel-mount pots are really gone, or only gone
  from this arrangement.
- NeoSlider travel is still **NOT SOURCED** (`parts.md` §10); the 50.80 mm slot
  is a placeholder and the fader's slot length will move when it is measured.
- Port and toggle positions on the keyboard's rear edge, which face into the
  slab and need a relief in the spine.
