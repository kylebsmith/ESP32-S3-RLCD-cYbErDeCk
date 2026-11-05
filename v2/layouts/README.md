# The layout studies

Two arrangements, both passing every check in `v2/tools/layout.py`, both drawn
from the measured part dimensions in `v2/parts/parts.md` rather than sketched.

Render either with:

```
python3 -I v2/tools/layout.py v2/layouts/<spec>.py v2/layouts/out/<spec>.svg
```

The checks read the emitted SVG, not the spec, so a bug in the drawing shows up
instead of passing quietly. Exit code is 1 if anything fails.

## What is fixed in both

The keyboard sets the width and nothing else gets near it: `body_w = max(kbd
pocket 288.35, board pocket 93.90) + 2 × 3.20 = 294.75`. So **the deck is
294.75 mm wide whatever the control surface does**, and the only variable is
depth.

Both put the encoder bay on the centreline, because that is the bay the screen
labels: four encoders at the vendor's 19.05 mm pitch span 76.200 mm inside an
84.80 mm display, and 15 characters of text (19.0800 mm) match the pitch to
0.030 mm.

## A — three bands, three pots

```
pot row        78.13 mm from the keys     beyond thumb reach, deliberately
encoder row    43.21                       within
key row        14.00                       within
deck           294.75 × 272.92
```

Three pots get their own row, one per column, each directly above a bay centre.
An odd count is symmetric for free: one on the axis, two mirrored.

## B — two bands, four pots beside the display

```
encoder row    43.21 mm from the keys      within thumb reach
key row        14.00                       within
pots           92.35                       beyond, deliberately
deck           294.75 × 233.55             39.37 mm shallower than A
```

The keyboard forces the deck to 294.75 mm wide while the board pocket is only
93.90, so there is about 97 mm of dead shell either side of the display —
created by the keyboard and used by nothing. Four pots go there. No extra
depth, no plate involvement (they mount through the 2.40 mm shell wall, which
a short Alpha bushing accepts), and there is empty interior volume behind them
for the bodies and the ADS1115.

The cost is a fourth pot, and it is not avoidable: an odd count needs one
control on the axis, and beside the display the axis is the display.

## Why three pots cannot fit two bands on the plate

Four Pattern B bays are needed (Quad Rotary, two NeoSliders, NeoKey) and only
three fit side by side in 288.35 mm, so two bay rows are forced. Stacking a pot
row onto a bay row instead pushes the bay row's centreline to 52.7 mm from the
keyboard — past the 45 mm thumb limit — and that row holds the encoders, the
most-played control on the panel. Pots beside a three-bay row do not fit either:
the margin is 20.35 mm and a 25.40 mm knob needs 24.13 mm of it plus its own
radius.

So the pots leave the plate, or they get their own row. That is the whole
choice between A and B.

## Spare bays

Both layouts leave two bays empty. That is the point rather than an oversight:
a bay does not know whether it holds a slider, a key strip or four encoders,
because all three are the same 76.200 × 21.590 outline on the same 38.10 ×
16.51 hole quad. Reconfiguring is four M2.5 screws.
