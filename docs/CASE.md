# Carry case

A sleeve the whole deck slides into, vertically, like a side bag. Front, back,
both flanks and the bottom; open only at the top. Every port is buried.
**Two halves, bolted.** `cad/carrycase.scad`, checked by `tools/check_case.py`.

![The case, assembled](img/case-iso.png)

| | |
|---|---|
| Outside | **150.7 × 181.8 × 38.2 mm**, 166.6 over the strap pads |
| Proportion | **1.2065** — the deck's own, to four figures |
| Material | 554 cm³ → **≈ 378 g** printed, both halves |
| Walls | 4.0 front and back, **16.0 all the way round**, 28.3 floor, 12.0 rim |
| Hardware | **13 × M5 × 35 socket cap, 13 × M5 nut**, 4 × Ø5×2 disc |
| Flock allowance | 0.80 mm per surface, + 0.40 clearance |
| Bed needed | **167 × 182 mm** per half |

## It is two parts, and not for printing

One piece could be printed. It could not be *reached into* — the magnet pockets
opened into a cavity 155 mm deep with nothing to get at them by, which is the
fault that started this revision (C-41). A plane parallel to the face fixes
that and pays four more times: both halves print flat and face-down, the magnet
pockets open upward on the bed, the inside becomes two open trays to flock, and
the back stops needing a spine to stand on.

Thirteen M5 socket screws round the perimeter into **hex nuts trapped at the
parting face**, so it assembles with one key and no spanner — nothing reaches down a
16 mm wall with a socket. The seam is not hidden: a 0.6 mm chamfer each side
makes it a 1.2 mm shadow gap, which is the only honest thing to do with a joint
you cannot fill.

The wall is **16 mm the whole way round**, and that is what lets the fasteners
go round with it. An M5 nut is 9.47 mm across corners; 16 mm leaves 3.27 mm of
metal either side, and the ring runs down the middle of it.

## The back is flat, and that is a print finding

The obvious back is a slab with a raised spine carrying the cowl channel.
Printed back-down the spine crown is the first layer and the slab bottom sits
11.00 mm above it — **3,100 mm² a side of downward-facing flat face**, needing
support. Flaring the spine out to meet the slab does not rescue it: 11 mm of
rise over 28 mm of run is **21°**, half of what FDM holds.

So the back drops to the channel floor everywhere. It costs 11 mm of depth and
returns a part with nothing under it.

## The fasteners are the outline

There is no list of screw coordinates anywhere in this project. `ring_path()`
takes the case's own outline, insets it to the middle of the wall, and samples
it at even arc length — so the fasteners follow the superellipse **round the
bottom corners** instead of stopping where a straight rail would have to.

Measured on the rendered part: **12 gaps between 32.1 and 35.8 mm**, spread
10.3 %. The low end is geometry, not error — a chord under-reads an arc.

**The ring is a U, and that is physics.** A fastener parallel to Z needs
material through the whole depth, and across the mouth there is none: the
deck's own cross-section has to pass through there. So it runs as far up both
flanks as it can and stops. What it does *not* do is stop at the corners, and
that is the check — 5 fasteners below the deck, 2 of them out past the cavity
in both bottom corners.

## Three numbers pinned to the deck, none to taste

| | |
|---|---|
| corner | `case_r = corner_blend × case_w / body_w` = **14.51** |
| height | `case_floor` solved so `case_h / case_w` = `body_h / body_w` = **28.30** |
| exponent | `form_n` = 3.2, the deck's own |

Holding the proportion fixes the case's **height**. It does not say where to
spend it, and that is the real decision. Spent at the mouth, the deck sits
24 mm down a hole and needs a scallop cut in the front to reach — and on a
150 mm face that scallop is not a detail, it is the silhouette. Spent at the
**floor** it is invisible, needs no scallop, and puts 28 mm of solid PLA on the
end you actually drop the thing on.

So the mouth is **12 mm**, which is a finger pad, and the floor takes the rest.

## Strap pads

Five goes at this (C-38 through C-42). The one that rules out a whole family is
worth knowing: **tangency between two parallel faces 8 mm apart can only be made
by a semicircle of radius 4** — no larger radius is tangent to both — so a
tangent boss always ends in a tight 4 mm turn, and a tight turn at each end is
what an ear looks like.

The escape would be a long shallow swell, and that is geometrically unavailable:
the slot has to sit high on the flank for a bag to hang flat, a swell centred
there runs out of straight flank within about 44 mm, and a 44 mm swell needs r34
ends — a 40° junction, *worse* than the stadium it replaced.

So the boss stays local and stops fighting the outline. It **speaks it**: a
46 mm pad, 8 mm proud, on the deck's own superelliptical corners and the deck's
own exponent. The complaint was that a rectangle and a squircle met at a weird
angle; there is no rectangle now. The slot gets **7.50 mm of metal either side
× 38.25 mm deep = 287 mm² in shear**, and the fastener above it and the one
below it take the strap load into the joint.

## The cowl is the guide

The deck's battery cowl is **83.93 × 28.23 mm at its foot and stands 11.00 mm
off the back** — measured on the rendered plate, not assumed. A channel that
width running the **full height** of the cavity lets the deck slide in and keys
it in X and in rotation at the same time.

The bump stops being a problem to accommodate and becomes the location feature.
It does not read on the outside at all: the back is flat (see above), so the
channel is a recess in a solid block rather than a spine raised off one.

**Measured: the deck slides its whole travel at 0.000 mm³ of interference**,
sampled at 70, 55, 40, 25, 10 and 0 mm above seated. A fit that only works when
seated is not a sleeve, it is a puzzle.

## How it holds

Three things, none of them a snap feature:

- **the cowl channel**, which keys the deck for its whole travel and bottoms out
  on the floor
- **the flock**, which is a compliant interference fit once it is in
- **gravity**, because it is carried mouth-up

The four magnets add a positive seat at the end of the travel. They are **not**
the retention and are not asked to be — at a 2 mm gap through the flock they are
worth a couple of newtons each, which is a click, not a hold.

## Print it face down

Front half on its face, back half on its back. Each is then **one flat bed face
and one open tray** — no bridge, no support, and the two surfaces anyone looks
at are the two that touch glass.

Measured on the rendered halves, in each one's own print frame:

| | front | back | budget |
|---|---|---|---|
| near-flat ceiling (< 15°) | **0 mm²** | **0 mm²** | 20 |
| shallow face (< 44°) | 206 mm² | 206 mm² | 250 |

The 206 mm² is the 45° seam and edge chamfers caught by tessellation on the
superellipse corners — chamfers at exactly the limit, not ledges. **Zero
near-flat ceiling is the number that matters**, because a flat face pointing at
the bed is the defect that killed the spined back.

Both halves need a **167 × 182 mm** bed.

## Finishing

The inside is meant to be flocked, which is why every internal surface carries
0.80 mm of pile allowance on top of clearance. **Without it the deck binds once
the flock is in**, which is a mistake you only get to make once. Two open trays
is the only sane way to do it.

The outside is meant to be filled, sanded and polished — but **not across the
seam**, which is a joint, not a blemish. Fill and sand each half, then assemble;
the 1.2 mm shadow gap, the thirteen screw heads and the two strap pads are the
only things that break the surface, and all three are meant to be seen.

## Assembly

1. Glue the four discs into the front half's pockets, flush with the tray face,
   **polarity matched to the deck** — check with the deck before the glue grabs.
2. Flock both trays.
3. Mate the halves on the four printed locating pins.
4. Drop thirteen M5 nuts into the hex wells in the **back face** and drive
   thirteen M5 × 35 from the front. Each nut is caught by its screw and drawn
   up onto its seat; the hex stops it turning. Snug, not gorilla — the bearing
   annulus is 35 mm² of PLA.

## Open

1. **The mouth is open, so the deck's top edge faces it.** The buttons and
   microphones sit on that edge and are reachable down an 8 mm recess. That is
   inherent to a sleeve; closing it needs a flap or a cap, which is a different
   object.
2. **The magnets are a seat, not a latch.** Four of them across a 2.00 mm gap
   are comparable to the deck's weight, not a multiple of it, and they are only
   fighting it in shear. Retention is the cowl channel, the flock, and carrying
   it mouth-up. The arithmetic is in C-41 and in `parameters.scad`.
3. **378 g is computed**, at 55 % of solid, not weighed. It is a heavy object;
   that was the brief.
4. **Flock pile is assumed at 0.80 mm.** Adhesive thickness varies by
   application; check a test coupon before committing the whole inside.
