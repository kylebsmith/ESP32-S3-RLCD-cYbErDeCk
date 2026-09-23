# Carry case

A sleeve the whole deck slides into, vertically, like a side bag. Front, back,
both flanks and the bottom; open only at the top. Every port is buried.
**Two halves, bolted.** `cad/carrycase.scad`, checked by `tools/check_case.py`.

| | |
|---|---|
| Outside | **134.7 × 162.4 × 38.2 mm**, 162.6 over the rails |
| Proportion | **1.2065** — the deck's own, to four figures |
| Material | 460 cm³ → **≈ 314 g** printed, both halves |
| Walls | 4.0 front and back, 8.0 flanks, 6.0 floor, **14.0 rails** |
| Hardware | **8 × M5 × 25 socket cap, 8 × M5 nut**, 4 × Ø5×2 disc |
| Flock allowance | 0.80 mm per surface, + 0.40 clearance |
| Bed needed | **163 × 162 mm** per half |

## It is two parts, and not for printing

One piece could be printed. It could not be *reached into* — the magnet pockets
opened into a cavity 155 mm deep with nothing to get at them by, which is the
fault that started this revision (C-41). A plane parallel to the face fixes
that and pays four more times: both halves print flat and face-down, the magnet
pockets open upward on the bed, the inside becomes two open trays to flock, and
the back stops needing a spine to stand on.

Eight M5 socket screws down the flanks into **hex nuts trapped at the parting
face**, so it assembles with one key and no spanner — nothing reaches down a
16 mm flank with a socket. The seam is not hidden: a 0.6 mm chamfer each side
makes it a 1.2 mm shadow gap, which is the only honest thing to do with a joint
you cannot fill.

An M5 nut needs about 15 mm of flank to sit in. A 16 mm flank was tried and it
made the case **150 × 155 — square**, where the deck is plainly portrait; that
is the opposite of echoing it. So the flank stays at 8 mm and the fasteners move
out into the rails, which have 22 mm of material anyway.

## The back is flat, and that is a print finding

The obvious back is a slab with a raised spine carrying the cowl channel.
Printed back-down the spine crown is the first layer and the slab bottom sits
11.00 mm above it — **3,100 mm² a side of downward-facing flat face**, needing
support. Flaring the spine out to meet the slab does not rescue it: 11 mm of
rise over 28 mm of run is **21°**, half of what FDM holds.

So the back drops to the channel floor everywhere. It costs 11 mm of depth and
returns a part with nothing under it.

## Three numbers pinned to the deck, none to taste

| | |
|---|---|
| corner | `case_r = corner_blend × case_w / body_w` = **12.97** |
| height | `case_rim` solved so `case_h / case_w` = `body_h / body_w` = **15.00** |
| exponent | `form_n` = 3.2, the deck's own |

Offsetting the deck's 11.20 mm corner outward by the wall would give 20.40 mm on
a 135 mm body — proportionally almost twice as round as the deck, which is what
made v1 read as a pebble. Holding the **ratio** instead keeps the family.

The mouth height is not chosen either: it is whatever makes the case the deck's
proportion, and it lands on 15.00 mm — deep enough to swallow the top edge,
shallow enough to get the deck back out. Top and bottom are the same corner now,
because the flat plinth only existed to stand on the bed.

## The cowl is the guide

The deck's battery cowl is **83.93 × 28.23 mm at its foot and stands 11.00 mm
off the back** — measured on the rendered plate, not assumed. A channel that
width running the **full height** of the cavity lets the deck slide in and keys
it in X and in rotation at the same time.

The bump stops being a problem to accommodate and becomes the location feature.
On the outside that channel reads as a full-height spine, flared at its base so
it grows out of the slab rather than sitting on it.

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

## Strap rails

Three versions, each failing the way the last one was fixed. v1 cut a 4 mm slot
through a 10 mm flank **at the top corner**, where the outline is already
turning: a thin web in an awkward place. v2 grew a 32 mm pad on each flank —
strong, and it read as **two tabs stuck to a box**, the bolted-on look that got
the grip fingers thrown out back in C-38.

A tab is a local answer. So the rail is not local: **one squared band per flank,
14 mm proud, 134 mm long**, standing off with a hard step — no fillet, the way a
boss looks machined rather than grown. It carries all four fasteners *and* the
strap slot, so it is structure the whole length of the object rather than a
feature near one end. The slab stays slim; the weight is where the load is.

| | v1 | now |
|---|---|---|
| material each side of the slot | 3.00 mm | **6.50 mm** |
| depth behind it | 27.25 mm | **38.25 mm** |
| shear section a side | 82 mm² | **249 mm²** |

Both ends stop inside the **straight run** of the flank, so a termination is
always a clean step and never a step onto a curve. The slot sits 47 mm below the
mouth: lugs at the rim foul the hand drawing the deck out, and a bag hung from
its rim tips forward. The screws at y = 19 and y = 57 frame it and clamp the
joint exactly where the strap pulls.

## Print it face down

Front half on its face, back half on its back. Each is then **one flat bed face
and one open tray** — no bridge, no support, and the two surfaces anyone looks
at are the two that touch glass.

Measured on the rendered halves, in each one's own print frame:

| | front | back | budget |
|---|---|---|---|
| near-flat ceiling (< 15°) | **0 mm²** | **0 mm²** | 20 |
| shallow face (< 44°) | 110 mm² | 110 mm² | 200 |

The 110 mm² is the 45° seam and edge chamfers caught by tessellation on the
superellipse corners — chamfers at exactly the limit, not ledges. **Zero
near-flat ceiling is the number that matters**, because a flat face pointing at
the bed is the defect that killed the spined back.

Both halves need a **163 × 162 mm** bed.

## Finishing

The inside is meant to be flocked, which is why every internal surface carries
0.80 mm of pile allowance on top of clearance. **Without it the deck binds once
the flock is in**, which is a mistake you only get to make once. Two open trays
is the only sane way to do it.

The outside is meant to be filled, sanded and polished — but **not across the
seam**, which is a joint, not a blemish. Fill and sand each half, then assemble;
the 1.2 mm shadow gap, the eight screw heads and the two strap rails are the
only things that break the surface, and all three are meant to be seen.

## Assembly

1. Glue the four discs into the front half's pockets, flush with the tray face,
   **polarity matched to the deck** — check with the deck before the glue grabs.
2. Flock both trays.
3. Drop eight M5 nuts into the hex pockets at the back half's parting face.
   They are a 0.20 mm press fit and stay put while you close it.
4. Mate the halves on the four printed locating pins and drive eight
   M5 × 25 from the front.

## Open

1. **The mouth is open, so the deck's top edge faces it.** The buttons and
   microphones sit on that edge and are reachable down an 8 mm recess. That is
   inherent to a sleeve; closing it needs a flap or a cap, which is a different
   object.
2. **The magnets are a seat, not a latch.** Four of them across a 2.00 mm gap
   are comparable to the deck's weight, not a multiple of it, and they are only
   fighting it in shear. Retention is the cowl channel, the flock, and carrying
   it mouth-up. The arithmetic is in C-41 and in `parameters.scad`.
3. **314 g is computed**, at 55 % of solid, not weighed. It is a heavy object;
   that was the brief.
4. **Flock pile is assumed at 0.80 mm.** Adhesive thickness varies by
   application; check a test coupon before committing the whole inside.
