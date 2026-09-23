# Carry case

A sleeve the whole deck slides into, vertically, like a side bag. Front, back,
both flanks and the bottom; open only at the top. Every port is buried.
`cad/carrycase.scad`, checked by `tools/check_case.py`.

| | |
|---|---|
| Outside | **138.7 × 155.4 × 38.2 mm** |
| Material | 273 cm³ → **≈ 186 g** printed |
| Walls | 4.0 front and back, **10.0 flanks**, 6.0 floor |
| Flock allowance | 0.80 mm per surface, + 0.40 clearance |

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

## Strap lugs

No ears, no bosses, no hardware. Each lug is a **4 × 16 mm slot cut straight
through the flank**, front to back.

The flank is 10 mm of solid wall and 27 mm deep, so the material either side of
the slot carries **82 mm² in shear per side** — far past anything a strap
applies — while **nothing protrudes at all**. From outside there is only a hole
in a continuous surface.

## Print it mouth-up

Standing on the closed floor. Built that way **nothing in the part bridges**:
the floor is the first layer, the walls rise around the cavity, the mouth is the
last layer. Printed on its back instead, the front wall would have to bridge
118 mm of open cavity.

**The base is flat, and that is a print decision before it is a style one.** A
superellipse outline curves *outward* as it leaves the bed — a 0° overhang at
the very bottom, 1,216 mm² of it, measured. Squaring the base removed all of it
and gave the object a plinth to stand on. The top keeps the full soft corner,
where curving inward as it rises is free.

What remains is **290 mm² of shallow downward face**, about a third of it the
two strap-slot roofs — each a 4 mm bridge, which prints without support.

## Finishing

The inside is meant to be flocked, which is why every internal surface carries
0.80 mm of pile allowance on top of clearance. **Without it the deck binds once
the flock is in**, which is a mistake you only get to make once.

The outside is meant to be filled, sanded and polished, so nothing on it
protrudes, clips or hinges, and the spine is flared into the slab rather than
stepped onto it — a hard step is where filler cracks.

## Open

1. **The mouth is open, so the deck's top edge faces it.** The buttons and
   microphones sit on that edge and are reachable down an 8 mm recess. That is
   inherent to a sleeve; closing it needs a flap or a cap, which is a different
   object.
2. **186 g is computed**, at 55 % of solid, not weighed.
3. **Flock pile is assumed at 0.80 mm.** Adhesive thickness varies by
   application; check a test coupon before committing the whole inside.
