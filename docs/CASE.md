# Carry case

A sleeve the whole deck slides into, vertically, like a side bag. Front, back,
both flanks and the bottom; open only at the top. Every port is buried.
**Two halves, bolted.** `cad/carrycase.scad`, checked by `tools/check_case.py`.

![The case, assembled](img/case-iso.png)

| | |
|---|---|
| Outside | **130.7 × 166.4 × 37.4 mm** at the waist, **151.6 × 177.1** over the swells |
| Seam | **1:1.88**, low on the flank — not in the middle |
| Material | 448 cm³ model → **≈ 430 g of filament** (94 % of it perimeter shell) |
| Walls | **3.6 front and back**, 6.0 flank at the waist → 16.5 at each fastener, 13.0 floor, 12.0 rim |
| Edge | **no chamfer, no flat** — the section rolls 4.0 mm in at each face, continuously |
| Hardware | **7 × M5 × 16 socket cap + 7 × M5 × 10 heat-set insert**, 4 × Ø5×2 disc |
| Heads | flush: Ø9.0 × 5.0 counterbore under an r11 dish, 1.0 deep |
| Flock allowance | 0.80 mm per surface, + 0.40 clearance |
| Bed needed | **152 × 177 mm** per half |

## It is two parts, and not for printing

One piece could be printed. It could not be *reached into* — the magnet pockets
opened into a cavity 155 mm deep with nothing to get at them by, which is the
fault that started this revision (C-41). A plane parallel to the face fixes
that and pays four more times: both halves print flat and face-down, the magnet
pockets open upward on the bed, the inside becomes two open trays to flock, and
the back stops needing a spine to stand on.

Seven M5 socket screws into **heat-set inserts in the back half**, so it
assembles with one key and no spanner — nothing reaches down a 16 mm wall with a
socket, and nothing has to hold a nut you cannot see. Thirteen screws through to
captive hex nuts was the version before, and it clamped nothing: the pockets
opened at the parting face with no roof, so every nut rose and bore on the front
half's own face instead of the back's (C-43).

The seam is a **0.30 mm hairline**, not a shadow gap. The version that made a
feature of it — 0.6 mm chamfered each side — belonged to a case with planes; this
one has none for a gap to sit in.

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
takes the **cavity** and outsets it by 6.50 mm, so the fasteners follow the
superellipse **round the bottom corners** instead of stopping where a straight
rail would have to. Outsetting the cavity rather than insetting the outline is
the fix for the clash that made the first ring read as a rectangle pasted onto a
squircle: the bolts now sit on a curve the case itself is built from.

Measured on the rendered part: **6 gaps between 57.6 and 59.6 mm**, spread
3.3 %.

**The ring is a U, and that is physics.** A fastener parallel to Z needs
material through the whole depth, and across the mouth there is none: the
deck's own cross-section has to pass through there. So it runs as far up both
flanks as it can and stops. What it does *not* do is stop at the corners, and
that is the check — 3 fasteners below the deck, 2 of them out past the cavity
in both bottom corners.

## The heads lie flush

A socket cap head is 4.80 mm thick and 8.50 across. It drops into a **Ø9.00
counterbore 5.00 deep**, which puts the crown 0.20 mm *below* the face, and an
**r11.00 sphere cuts 1.00 mm** into the face around it so the transition from
skin to head is a dish rather than a step.

The dish is the part that has to be computed. A sphere of radius *R* cutting *d*
deep leaves a footprint `2·sqrt(2Rd − d²)` wide — r30 at 1.2 mm is **16.80 mm
across**, nearly twice the counterbore it is blending, and it cut straight out
through the rolled edge. r11 at 1.00 is 9.17 across and leaves a 1.42 mm rim
(C-48).

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

## A river rock, not a rounded box

Two things make it a rock, and neither is applied afterwards.

**The section rolls the whole way.** `case_roll = 0.50` means the draw-in is
falling from one face to the middle and rising again to the other — there is no
flat band anywhere and no arris. An earlier version softened each face by 3 mm
and left the middle straight, which reads as a curved rectangle, and the one
before that gave each half its own roll and put a crease down the seam.

**The wall is 6 mm and swells to 16.5 where it has to be.** Not by adding a pad
— by pushing the plan *outline* outward at each of the nine sites with a
cos-squared falloff, so there is no junction to crease at. Swells combine as
`1 − Π(1 − f)` rather than summing, so two near each other blend instead of
stacking to twice the amplitude.

It is built as **one polyhedron**, 45 layers of 84 points. A stack of hulls
cannot do it: the outline dips back between swells, so it is not convex and
`hull()` would fill the dips.

**The counterbore, not the bolt, sizes the swell.** A flush Ø9.00 counterbore
plus a rim has to sit inside a front face that the roll pulls in by 4.00 mm.
That chain is what sets 16.50, and it is an assert, not a taste.

## Where the filament actually goes

Model volume is not filament. At 0.8 mm nozzle, 3 perimeters, 15 % infill, the
shell is 2.4 mm thick and this part is thin-walled enough that **94 % of it is
shell** — the front half is *entirely* perimeter. Hollowing saves nothing; the
infill is 6 % of the total.

So the levers are not where four revisions of argument put them:

| | saves |
|---|---|
| perimeters 3 → 2 | **130 g** — slicer setting |
| a 0.6 mm nozzle | **98 g** — slicer setting |
| floor 21.1 → 13.0 | 30 g |
| wall 4.0 → 3.6 | 25 g |

**And thinning the flank did not save anything.** The 13 mm flank came from a
short chain: the mouth has to pass the deck, so there is no full-depth material
across the top; fasteners therefore live in the flanks and the floor; an M5
insert is Ø7.00 and wants 3 mm of metal a side. The way out was an external
boss, and the swells are it — the flank is 6.00 at the waist now, and 16.50 only
where an insert sits, with 4.75 mm of metal a side measured on the part.

It costs more, not less:

| | volume | surface | filament |
|---|---|---|---|
| 13 mm wall, flat slab | 396 cm³ | 1310 cm² | **390 g** |
| 6 mm wall + 9 swells | 448 cm³ | 1371 cm² | **430 g** |

Both went up, because at 94 % shell **surface is what you pay for**, and nine
swells overlap until the wall is thick nearly everywhere anyway. Pulling the
reach in from 26 to 18 mm recovered about 12 g of the 48; 14 mm would recover
another 4 and start to look like warts. So the rock is a form decision that
costs roughly 10 % more filament than the slab it replaced, and that is the
honest price of it (C-48).

## The strap lug is a hole

Eight versions. The last trapped a D-ring's bar in a bore straddling the parting
plane — clever, fiddly to print, fiddly to assemble, and it made the strap depend
on two 6 mm windows.

This is a hole. **Ø7.00, front to back through the flank**, so a cord or split
ring wraps the full wall and hangs outward, and the load goes into the whole
height of the flank above it. The parting plane cuts across it, so each half
prints it as a plain vertical bore with nothing overhanging. It sits in a swell
of its own: **4.75 mm of metal a side**, measured, and 21.3 mm clear of the
nearest fastener.

## Print it face down

Front half on its face, back half on its back. Each is then **one flat bed face
and one open tray** — no bridge, no support, and the two surfaces anyone looks
at are the two that touch glass.

Measured on the rendered halves, in each one's own print frame:

| | front | back | budget |
|---|---|---|---|
| near-flat ceiling (< 15°) | **0 mm²** | **0 mm²** | 20 |
| shallow face (< 44°) | **0 mm²** | **0 mm²** | 250 |

Both zero, because the edge is **rolled rather than chamfered**. A 45° chamfer
sits exactly on the FDM limit and shows up in this measurement; a roll whose
derivative vanishes at both ends is *vertical* where it meets each face and only
reaches about 28° in the middle. The smoother edge is also the safer print.

The only ceiling left is the counterbore roof: **1.80 mm of annulus** round a
5.40 mm bore, seven of them, every one opening on the bed. That is a ledge, not
a bridge.

Both halves need a **152 × 177 mm** bed. The front is 13.00 mm of body plus
3.00 mm of locating pin — 16.00 overall.

## Finishing

The inside is meant to be flocked, which is why every internal surface carries
0.80 mm of pile allowance on top of clearance. **Without it the deck binds once
the flock is in**, which is a mistake you only get to make once. Two open trays
is the only sane way to do it.

The outside is meant to be filled, sanded and polished — but **not across the
seam**, which is a joint, not a blemish. Fill and sand each half, then assemble;
the 0.30 mm hairline and seven dished bolt heads are the only things that break
the surface, and both are meant to be seen.

## Assembly

1. Press the seven M5 × 10 inserts into the back half's Ø6.20 bores with a
   soldering iron, square and flush.
2. Glue the four discs into the front half's pockets, flush with the tray face,
   **polarity matched to the deck** — check with the deck before the glue grabs.
3. Flock both trays.
4. Mate the halves on the four printed locating pins.
5. Drive seven M5 × 16 from the front with a 4 mm key. **8.00 mm of thread in
   the insert, 1.60 diameters.** Snug, not gorilla — the bearing annulus is PLA.

## Open

1. **The mouth is open, so the deck's top edge faces it.** The buttons and
   microphones sit on that edge and are reachable down an 8 mm recess. That is
   inherent to a sleeve; closing it needs a flap or a cap, which is a different
   object.
2. **The magnets are a seat, not a latch.** Four of them across a 2.00 mm gap
   are comparable to the deck's weight, not a multiple of it, and they are only
   fighting it in shear. Retention is the cowl channel, the flock, and carrying
   it mouth-up. The arithmetic is in C-41 and in `parameters.scad`.
3. **430 g is computed**, from surface area and a 2.4 mm shell, not weighed. It
   is a heavy object; that was the brief. Two perimeters instead of three takes
   it to about 300 g and is a slicer setting, not a redesign.
4. **Flock pile is assumed at 0.80 mm.** Adhesive thickness varies by
   application; check a test coupon before committing the whole inside.
