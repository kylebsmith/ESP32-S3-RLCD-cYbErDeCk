# The v2 envelope

## The brief, stated correctly

v2 is **bigger than v1**. The footprint is not a constraint to squeeze into —
it grows to fit the instrument. Three things it has to carry:

1. the **display and board**, proven in v1 and carried over unchanged;
2. a **keyboard**, which stays — and should get *better*, not disappear;
3. a **control surface**: rotaries, pots, sliders, keys, and the gestural
   sensors.

**This is an enclosure exercise.** The firmware already works and is not being
touched. What the controls are *for* matters only insofar as it decides how
many there are, how big they are and where a performer's hands expect them.

## What carries over from v1, unchanged

| | mm |
|---|---|
| display active | 84.80 × 63.60 |
| board | 11.25 deep, 11.00 stack |
| battery cowl | 82.80 × 27.10, rising 11.00 off the back |
| front wall / back / side | 2.40 / 3.20 / 3.20 |

v1's whole deck was 116.25 × 140.25 × 16.85, with the keyboard bay
109.85 × 59.15 × 11.00 at the bottom. Those are the *reference* proportions,
not a cage.

## The real problem: arrangement, then size

With the footprint free, the question stops being "what fits" and becomes
**what order do the bands go in, and how far apart**. A performer's hands have
to find the controls without looking, while their eyes are on the display and
their fingers are on the keys.

The candidate arrangements, to be drawn and judged rather than argued:

```
A   display  /  controls  /  keyboard        controls between eyes and hands
B   display  /  keyboard  /  controls        controls below, furthest reach
C   controls /  display   /  keyboard        controls at the top edge
D   display + controls side by side / keyboard     wider, shorter
```

Each has a different total height and a different hand journey. The study has
to cost all four, not pick one.

## What sets the thickness

The deck is 16.85 thick. Every control adds a stack behind the panel:

```
cap / knob / keycap      ──┐
actuator (switch, EC11, pot) ├── sets the panel-to-floor depth
breakout PCB             ──┘
```

A Cherry MX switch is about 11.6 mm above its PCB before a keycap; an EC11
body about 12–13 with its shaft. So the control band is **thicker than the
display band**, and the shell either steps or carries the thickest case
everywhere. Recessing the controls is what pulls the worst of it back: a knob
whose top sits below the panel is a knob whose body starts lower.

Thickness, not footprint, is the number to defend.

## Not up for negotiation

Display, board, battery and port positions are proven. v2 refines the shell
around them; it does not re-derive them. The firmware is not touched.

## The recess is a counterbore, not a thicker panel

The pots are **standard panel-mount**: a threaded bushing through the panel,
a nut on the front, a knob on the shaft. That single fact constrains the
recess more than anything else on the control band, because a panel-mount
part is carried by the panel, and **the bushing length is the maximum panel
thickness at that hole**. Bushings are a few millimetres. The front wall is
2.40.

So a recessed knob cannot be made by thickening the panel — the bushing would
not reach through. It has to be a **counterbore**: the panel stays thin where
the bushing passes, and the well around it is cut *into* a locally raised
boss, or the knob sits in a pocket formed by raising the panel's neighbours
rather than the panel itself.

```
        knob
     ┌─────────┐          well wall (raised neighbour)
  ───┤         ├───┐  ┌───
     └────┬────┘   │  │        panel stays 2.40 at the bushing
  ════════╪════════╪══╪════  ← front wall
          │ nut
       bushing  (length caps THIS thickness, not the well depth)
          │
        body (goes inside, sets enclosure depth here)
```

Three numbers decide whether a given pot can be recessed at all, and by how
much:

| | why it matters |
|---|---|
| bushing length | the hard cap on panel thickness at the hole |
| shaft length above the panel face | how tall the knob can be, hence how deep the well |
| body depth behind the panel | the enclosure depth under the control band |

Encoders, keys and sliders are the opposite problem — they arrive on breakout
PCBs, mounted *behind* the panel and standing off it, so their recess is a
simple pocket and their depth is PCB + standoff + actuator. Two different
mounting logics on one panel, and the panel has to resolve both at the same
front face.

One consequence for the parts research: a pot with an unusually **long
bushing** is worth more than a nicer-feeling one with a short bushing, because
it is the only kind that tolerates a thick panel. If none exists in the slim
9 mm class, the knobs sit in wells formed by their neighbours, which is a
layout constraint, not a detail — it means knobs cannot be isolated in open
panel space.

A reminder in the other direction: the wells must not shrink-wrap the knobs.
A well sized to the knob's nominal diameter is a well a printed part cannot
accept a real knob into.
