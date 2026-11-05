# The v2 envelope

## The brief, stated correctly

v2 is **bigger than v1**. The footprint is not a constraint to squeeze into —
it grows to fit the instrument. Three things it has to carry:

1. the **display and board**, proven in v1 and carried over unchanged;
2. a **keyboard**, which stays — and gets *better*, not smaller;
3. a **control surface**: rotaries, pots, sliders, keys, and the gestural
   sensors.

**This is an enclosure exercise.** The firmware already works and is not being
touched. What the controls are *for* matters only insofar as it decides how
many there are, how big they are and where a performer's hands expect them.

## Decisions taken

| | decision | why |
|---|---|---|
| keyboard | **Keychron K9 Pro** — 60 %, ANSI, low-profile, wireless, aluminium frame | A 60 % keeps the **number row**, which the research established is the real discriminator: the language treats digits 0–9 as first-class values, so a 40 % board is the right size and the wrong layout. ANSI because `keymap.c` has no entry for HID usage `0x64`. |
| control plate | **flush laser-cut acrylic**, set into the shell face | Industrial, minimalist, brutalist, Scandinavian. The plate carries every control and all the hardware; the shell stays thin. |
| labelling | **none fixed** — the interface is agnostic | *"reconfiguration is implied by the simplicity."* Meaning lives in software and on the screen, not in engraving. |
| arrangement | **A** — display far, controls middle, keyboard near | Push, OP-1, Digitakt, Organelle and Octatrack all converge on it, and it is the only option the thumb envelope permits. |
| pots | **three standard panel-mount**, knobs | Confirmed deliberately. |

Still being confirmed: whether the K9 Pro shares the K7 Pro's footprint, and
whether its base plate comes off.

## What carries over from v1, unchanged

| | mm |
|---|---|
| display active | 84.80 × 63.60 |
| board | 11.25 deep, 11.00 stack |
| battery cowl | 82.80 × 27.10, rising 11.00 off the back |
| front wall / back / side | 2.40 / 3.20 / 3.20 |

v1's whole deck was 116.25 × 140.25 × 16.85, with the keyboard bay
109.85 × 59.15 × 11.00 at the bottom. Those are *reference* proportions, not a
cage.

## The grid is the vendor's. It was not invented.

The single most useful thing the parts study found. Three different controls
share one board outline:

```
Pattern B — "3 × 0.85 inch control strip"      76.200 × 21.590 × 1.570
  4 × Ø2.50 mounting holes on 38.10 × 16.51, R2.54 corners

  5295  NeoSlider        two linear pots
  4980  NeoKey 1×4       four MX keys
  5752  Quad Rotary      four encoders at 19.05 pitch
```

**One boss quad accepts any of the three, interchangeably.** Every sensor, and
the ADC, sits on a second pattern — Adafruit's 25.400 mm breakout grid, four
Ø2.50 holes inset 2.54 mm from each edge.

Two patterns carry the entire control surface. So a row of identical
76.20 × 21.59 bays is simultaneously the perfect grid that was asked for *and*
the agnosticism — a bay does not know whether it holds a slider, a key strip or
four encoders, and swapping them is unscrewing four M2.5s. Reconfiguration
really is implied by the geometry, not merely suggested by the styling.

Lay the deck out on that module and an arrangement can be re-cut without
re-cutting bosses.

### The grid module is 0.635 mm, and it took two wrong answers to get there

Adafruit lays boards out in inches, so the grid is imperial. Finding which
imperial grid took three attempts, and the first two are recorded because the
failure mode is instructive: each looked right and each was checked against too
few of the parts.

```
4.7625  (3/16 in)   divides  4 of 14 placement dimensions
1.2700  (0.050 in)  divides 12 of 14
0.6350  (0.025 in)  divides 12 of 14, and the half-pitches too
```

**4.7625 was wrong.** It came from 19.05 / 4 and it divides the 19.05 pitch
family and nothing else — not 21.590, not 25.400, not 16.510, not 20.320. It was
checked against the one dimension it was derived from, which is no check at all.

**1.27 — Adafruit's actual board grid, 0.050 in — fails on one case, and it is a
case this panel always produces.** An *even* number of items at 19.05 pitch,
centred on the axis, lands on half-pitches: ±9.525 and ±28.575. And
9.525 / 1.27 = 7.5. A four-encoder row is exactly that. Symmetry about the
centreline and an even control count together force a half-pitch.

**0.635 mm = 0.025 in** divides every board dimension *and* the half-pitches,
and still catches an arbitrary round metric number, which is what the check is
for — 14.25, −3.30 and 95.00 all fail it.

So there is a hierarchy, not a single number:

| | mm | in | what it is |
|---|---|---|---|
| placement | **0.635** | 0.025 | the snap every control is checked against |
| board | 1.27 | 0.050 | Adafruit's own layout grid |
| coarse | 6.35 | ¼ | the layout's visible rhythm |
| tile | 25.40 | 1 | the plate's tile |

**The display is the one honest exception.** Its 84.80 × 63.60 active area lands
on no inch grid at any of these moduli — it is a metric part in an imperial
world. It is exempted *by name* in the checker rather than by loosening the grid
for everything else, because the alternative is a grid that excuses whatever
does not fit it.

## Arrangement A, and the 45 mm that decides it

```
      ┌─────────────────────────┐
      │        display          │   far edge: eyes
      ├─────────────────────────┤
      │    control plate        │   ← centreline within ~45 mm of the keys
      ├─────────────────────────┤
      │       keyboard          │   near edge: hands
      └─────────────────────────┘
```

Not a preference. 90 % of adult thumbs reach 45.6–49.4 mm from the near edge
(means 51.8–61.4 mm), so **the control row's centreline has to sit within about
45 mm of the keyboard's top edge** or it cannot be reached without the hands
leaving the keys.

That one number disposes of the alternatives. Putting the controls *below* the
keys (B) puts them beyond the striking surface and two bands from the display —
no instrument does it. Putting them at the top (C) is what Push does, but Push
is a desk instrument with the arms free over a 300 mm panel; here the far edge
is 45–60 mm *beyond* the thumb envelope. C's good idea survives, relocated: the
**physical top edge** is reachable, and is where thumbwheels belong. Side by
side (D) is the Organelle, and it fails on an 84.80 mm display — it spends
prime area on two or three controls and destroys column alignment.

## What sets the thickness

The deck is 16.85 thick. Measured from the vendor STEP files, each control
adds this much:

| part | above its PCB | below it |
|---|---|---|
| 5752 Quad Rotary | 21.40 to shaft tip | 4.30 (right-angle QT) |
| 5295 NeoSlider | 15.50 to shank tip | 2.96 |
| 4980 NeoKey + MX | 11.6 with switch | 2.96 |

Every board is 1.570 thick. The encoder band is the deepest, and it is the one
to design to:

```
panel face
  │   encoder body behind the mounting surface      7.00   [DS PEC11R]
  │   PCB                                           1.570  [STEP]
  │   right-angle STEMMA QT below the PCB           4.30   [STEP]
  └── inner floor                                  ≈12.87
```

**≈12.9 mm against v1's 11.00 internal stack**, so the control band is about
2 mm deeper than the display band. Either the deck thickens there, or the band
gets its own cavity. *(Open: that 7.00 is from the Bourns datasheet while the
21.40 shaft-tip figure is measured from the STEP — they describe different
datums and want reconciling against one model before a pocket is cut.)*

Thickness, not footprint, is the number to defend.

## The pot bushing, and why a separate plate solves it

A panel-mount pot is carried by the panel, so **the bushing length is the
maximum panel thickness at that hole.** A stock 16 mm Alpha bushing is
M7 × 0.75 × 6.5 mm and permits about **4.0 mm** of panel once the supplied nut
and washer are counted. The front wall is 2.40.

This is exactly why the plate is the right answer rather than a styling choice:
**a separate plate can be as thick as the bushings allow, and no thicker, while
the shell goes on being whatever it needs to be.** A monolithic shell would have
needed a counterbore at every pot — thin where the bushing passes, wide above
it for the knob — which a 3D printer can do and a laser cannot.

And it can be avoided outright. The **Alpha RD901F** series sells the same pot
with a Ø7 or Ø9 bushing in 5 / 7 / 10 mm lengths; the 10 mm option (suffix
**B6** or **E6**) permits about **7.5 mm** of panel. That is the part that makes
a recessed pocket possible with no counterbore at all. With anything shorter, a
recess deeper than ~1.5 mm must be counterbored.

A laser cuts a profile through constant-thickness sheet, so varying thickness
has to come from **stacked laminae** — two or three bonded layers, each carrying
different features. That is how a flush acrylic plate gets a counterbore back
if it turns out to need one, and it is being specified rather than assumed.

Encoders, keys and sliders are the opposite problem: they arrive on breakout
PCBs behind the plate, standing off it, so their aperture is a simple cut and
their depth is PCB + standoff + actuator. Two mounting logics, one front face,
and the plate has to resolve both.

A reminder in the other direction: apertures must not shrink-wrap the hardware.
A hole sized to a knob's nominal diameter is a hole no real knob goes into.

## Open, and blocking geometry

1. **K9 Pro outside dimensions**, and whether its aluminium base plate is
   removable — the biggest single lever on deck thickness.
2. **The acrylic stack**: how many layers, what thickness, and how a sheet
   tolerance of ±10 % is absorbed so the plate still reads flush.
3. **Alignment to the display.** 84.80 / 4 = 21.2 mm per encoder, about half the
   ergonomic minimum, so four encoders *cannot* align to display columns. Push's
   legibility depends entirely on that alignment. Either the display grows or
   the panel earns its legibility another way.

## Settled by arithmetic: the encoder pitch

This was an open conflict — the Quad Rotary fixes four encoders at 19.05 mm
while MIL-STD-1472H asks for 25 mm minimum edge-to-edge separation for a 14 mm
knob, which is where the 39 mm figure came from. It is not a conflict, because
one side of it is impossible:

```
13 mm knob + 25 mm separation      = 38.00 mm pitch
four encoders at 38.00             = 127.00 mm span
the Quad Rotary board is             76.20 mm wide
                                     short by 50.80 mm — two thirds of the board
```

MIL's separation cannot be had on this part at any knob size. So:

```
19.05 pitch − 6.00 finger gap = 13.05 mm   largest knob the vendor pitch allows
MIL-STD-1472H minimum knob    = 13.00 mm   smallest knob it permits at all
                                ────────
window                          0.05 mm
```

**The encoder knob is 13.00 mm.** It is the only diameter that satisfies both,
and the margin is five hundredths of a millimetre — worth knowing before someone
picks a 16 mm knob because it feels nicer in the hand.

The resolution of the standard, stated so it is not re-litigated: MIL's 25 mm is
for controls that must be **found blind under stress**, and that is what the
three pots are — widely spaced, shape-coded, set and left. The encoder row is
played as a *row*, where the hand finds the group and counts across it, which is
what every instrument from the OP-1 to the Digitakt actually does at roughly this
pitch. Two different jobs, two different spacings, one standard correctly read.

## Not up for negotiation

Display, board, battery and port positions are proven. v2 refines the shell
around them; it does not re-derive them. The firmware is not touched.
