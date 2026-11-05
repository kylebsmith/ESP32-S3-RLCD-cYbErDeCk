# The v2 envelope budget

Everything below is measured out of v1's `cad/parameters.scad`, which is frozen
and traceable. These are the numbers v2 inherits before a single new part is
placed.

## What v1 actually is

| | mm |
|---|---|
| deck body | 116.25 × 140.25 × 16.85 |
| front wall / back / side | 2.40 / 3.20 / 3.20 |
| display active | 84.80 × 63.60, centred (−1.60, +31.78) |
| — spans y | **−0.03 … +63.58** |
| board | 11.25 deep, 11.00 stack, centred y +31.78 |
| battery cowl | 82.80 × 27.10, rising 11.00 off the back |
| keyboard bay | 109.85 × 59.15 × **11.00 deep**, centred y −36.15 |
| — spans y | **−65.72 … −6.57** |
| keyboard aperture | 106.50 × 55.80 |

## The one fact that shapes v2

**The keyboard bay is the control surface.** Taking the Rii out frees a
pocket **109.85 × 59.15 and 11.00 mm deep**, in the lower half of the front
face, that is *already inside the proven envelope*. Panel area for the controls
therefore costs **nothing** in footprint — the device does not have to grow to
gain a control surface, it only has to stop wasting that volume on a bad
keyboard.

What it may cost is **thickness**, and that is the whole design problem.

## The depth problem, stated before the parts arrive

The bay gives 11.00 mm between the front face and the bay floor. Every control
has to fit a stack of:

```
keycap / knob / slider cap   ──┐
actuator body (switch, EC11, pot)   ├── must fit 11.00 mm, or the body grows
breakout PCB                 ──┘
```

A Cherry MX switch alone is about 11.6 mm above its PCB before a keycap, and an
EC11 encoder body is about 12–13 mm with its shaft. So the naive stack does
**not** fit, and that is exactly why the user's instinct to **recess** the
controls is the right one: recessing is not styling here, it is what buys the
depth back. A knob whose top sits 2 mm below the panel is a knob whose body
starts 2 mm deeper into a bay that has none to spare.

The real budget is therefore:

```
available = kbd_depth (11.00) + front_t (2.40) + whatever the body grows
needed    = PCB + actuator + (cap − recess)
```

and the layouts have to be costed against it, not drawn and hoped for.

## The area problem

59.15 mm of bay height has to hold whatever rows the control complement needs.
A first pass at row heights, before the real part numbers land:

| row | height needed |
|---|---|
| 3 rotary encoders with knobs | ~25 mm |
| 3 potentiometers | ~25 mm |
| 2 sliders, if laid across | ~15 mm |
| 4 keys at 19.05 mm pitch | ~20 mm |

That is already past 59 mm for four rows. So one of these is true, and the
layout studies have to pick:

1. the body grows in **y** (it has room to: the deck is 140.25 and the mouth
   end is mostly collar);
2. some controls move off the front face — the **top edge** above the display,
   or the flanks;
3. the complement shrinks.

No guessing: this gets decided against measured part dimensions and against
the control complement the precedents research argues for.

## What is NOT up for negotiation

The display, the board, the battery and the port positions are proven and
stay. v2 refines the shell around them; it does not re-derive them.
