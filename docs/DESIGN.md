# Design rationale

What this enclosure is, why it is shaped the way it is, and what was traded
away to get there.

## Brief

A single-form, overbuilt, minimum-footprint handheld terminal built around a
Waveshare ESP32-S3-RLCD-4.2 and a Rii 518BT keyboard, intended for live
performance use — musical live coding, control surfaces, and as a portable node
for a local network — rather than for a desk.

The reference design this project measures solves the same problem as a keyboard
tray and a board case joined together. The brief was explicitly to make it read
as one object instead.

## The footprint is component-bound, and the numbers say so

The honest headline first: **most of the size of this thing is not a design
choice.** Two rigid rectangles have to sit side by side.

```
width  = wall + max(keyboard 110.2, board 93.5) + wall
height = wall + keyboard 59.4 + spine + board 70.1 + wall
depth  = back plate + board stack 11.0 + front face
```

With 3.2 mm walls that is **116.6 × 139.1 × 16.6 mm**, and
`tools/validate.py` asserts each of those three equalities on every run. The
enclosure cannot be made smaller without thinning a wall or crushing a part.

Against the reference:

| | Reference (ATA) | This design | Δ |
|---|---|---|---|
| Height | 151.65 | 139.10 | **−12.55** |
| Width | 116.77 | 116.60 | −0.17 |
| Thickness | 18.00 | 16.60 | **−1.40** |
| Battery bulge | +12.0 | +9.0 | −3.0 |
| Wall | 2.90 | 3.20 | +0.30 |
| Footprint area | 17 708 mm² | 16 218 mm² | **−8.4 %** |

Shorter, marginally narrower, thinner, *and* thicker-walled. The height comes
from deleting the reference's top rail; the thickness comes from discarding
Waveshare's stand base.

Where the width is concerned there is nothing left to take: the keyboard is
109.22 mm wide and the walls are 3.2 mm, so 116.6 mm is the floor. The board is
23 mm narrower than the keyboard, and that mismatch is the single biggest
constraint in the design — it is also, usefully, the only free space in it.

## One form, not two cases

The reference splits along the **front face**: a bezel screws onto a tray. That
is the worst available plane for the joint. A drop onto a corner loads the shell
in bending, bending stress peaks at the outer fibres, and the outer fibre on the
display side is exactly where the joint is. Four M3 screws end up carrying it in
shear.

Here the shell is a **monocoque**: the entire front face, all four side walls
and the inter-bay spine are one continuous body, and the joint moves to the
back. Three things follow:

- **The face the user touches has no joint across it.** There is no bezel line,
  no step, and no fastener heads on the front.
- **The assembled shell is a closed section.** Front face, walls and back plate
  form a torsion box; the reference's open tray is a channel, which is an order
  of magnitude less stiff in twist for the same wall.
- **The spine is a shear web.** The 3.2 mm rib between the two bays runs the
  full depth and the full width, right across the middle — which is where a
  device of this aspect ratio wants to fold. The reference's equivalent region
  is a 6.44 mm partition that stops short of the bezel.

Wall thicknesses are integer multiples of a 0.4 mm nozzle (3.2 = 8 extrusions,
2.4 front = 6, 3.2 back = 8) so every wall prints as solid perimeters with no
sparse infill anywhere in the load path. `validate.py` asserts this.

## Everything loads from the back

The board and the keyboard both drop in from behind and are captured by lips in
the front face:

- the **display aperture** (86.8 × 65.6) is 3.35 mm smaller per side than the
  board pocket, so the board cannot move forward;
- the **keyboard aperture** (106.5 × 55.8) is 1.85 / 1.80 mm smaller per side
  than the keyboard pocket, so the keyboard cannot fall through.

The back plate then holds both forward. Over the keyboard bay it carries a
raised **keeper pad** whose thickness is *derived*, not chosen:
`board_depth − kbd_depth` — the keyboard bay is shallower than the board bay by
exactly that much, so the pad takes up the difference. A 0.5 mm adhesive foam
gasket around the display aperture absorbs the tolerance stack and doubles as
shock isolation for a panel the reference's own documentation calls fragile.

Service access is genuinely good: four screws release the back plate, and the
board, battery and back plate come out as one module.

## The battery has to stick out, so it was made useful

An 18650 is 18.4 mm in diameter (18.6 for protected cells). This deck is 16.6 mm
thick. The cell cannot be contained — it reaches 8.20 mm past the standoff
plane and must project through the back.

The reference handles this with a separate clip-on cover standing 12 mm proud.
Here the cowl is **integral to the back plate**, which removes a part and two
clips that can break, and shapes the unavoidable bulge into a **grip ridge**. It
runs across the deck just above the spine — where the fingers wrap when a
portrait handheld is held in two hands.

Its rise is derived, not copied: `(protrusion − back plate) + clearance + wall +
cap radius` = 9.0 mm, against the reference's 12.0, because discarding
Waveshare's 2.75 mm stand base moves the whole stack forward.

**The trade-off, stated plainly:** an integral cowl means swapping a cell takes
four screws instead of popping a door. That is the right call for a device meant
to survive a gig bag, and USB-C powers the board anyway — but if hot-swapping
matters more than robustness for your use, `batt_cowl_enable` turns it off and a
separate door becomes the obvious variant.

## The board is narrower than the keyboard, which is where the screws live

Because the keyboard sets the width, the board bay leaves a solid **8.35 mm
flank** down each side. That dead space carries both the back-plate fastener
bosses and the side-port tunnels — at zero cost to the footprint.

It also makes them fight. The USB-C and microSD tunnels cross the same strip the
bosses need, so the two fastener rows are *derived* from three constraints and
asserted, not placed by eye:

- clear of the port band as actually cut (measured opening plus clearance);
- inboard of the back plate edge by enough that an ISO 10642 countersink does
  not break out;
- as far apart as those two allow.

The margins are under a millimetre. Earlier revisions failed both constraints in
turn — the upper countersink broke out of the plate edge, and before that the
ports were cut through the outer wall only, which would have left both
connectors buried behind 8 mm of plastic. Both were caught by assertions, not by
inspection, which is the argument for having them.

A **full-width tongue and groove** along the bottom edge captures the plate
mechanically where there is no room for screws.

## Fasteners

Four M3 into brass heat-set inserts, countersunk.

Socket caps were the first choice and were wrong: an ISO 4762 M3 head is 3.0 mm
tall, which was the entire back plate thickness, so counterboring left *no*
material under the head to take preload. The plate went to 3.2 mm (8 extrusions,
matching the wall) and the screws to ISO 10642 countersunk — 1.86 mm deep,
bearing on a cone rather than a thin annulus, and flush.

Insert bosses are Ø7.4 around a Ø4.0 bore. 1.7 mm of wall is thinner than the
usual 2.0 mm guidance, chosen deliberately because every 0.1 mm of boss diameter
comes straight off the port clearance above — and because each boss merges into
the side wall, so its outboard side is far thicker than that figure suggests.

## Accessory mounting: the four screws are the rig points

There is no 1/4"-20 socket and no moulded strap lug. Both were designed,
modelled, and removed, because neither fits without growing the envelope or
cutting into a component bay:

- a 1/4"-20 insert needs ~8 mm of bore depth, and the only material at the
  deck's centroid is the 3.2 mm spine — a 10.5 mm bore does not fit in it.
  Anywhere else means a boss protruding from the back, which stops the deck
  lying flat;
- strap lugs want the corner voids, but the keyboard is full-width, so the only
  corner material is the gusset, and a 12 mm webbing slot does not fit inside a
  6 mm fillet without breaking into the keyboard bay.

Instead the four M3 back-plate screws **are** the accessory mounting points.
They thread into brass, not plastic, which makes them the strongest anchors on
the device. Fit M3 × 12 in place of M3 × 8 and clamp a bracket, strap yoke or
stand clamp under the heads. `accessory_pattern()` in `cyberdeck.scad` publishes
the hole pattern so a bracket can be drilled to match.

This is a real constraint honestly resolved, not a feature quietly dropped.

## Performance-oriented details

- **Keyboard service windows on both sides.** The 518BT's power slide switch and
  charging port share one short edge. Without a window the deck cannot be
  switched on. Both reference designs cut one; this cuts one on *each* side, so
  the keyboard can be installed either way round. The window is the intersection
  of both reference notches — the envelope two proven designs agree on.
- **Both microphone ports.** The board has a dual-mic array with echo
  cancellation, and blocking one would break beamforming. Both are at ±32.500 mm,
  a figure that came out of an independent measurement and the factory drawing
  identically.
- **Ventilation over the ESP32-S3 module.** The RLCD has no backlight so thermal
  load is low; these are insurance for sustained Wi-Fi transmit.
- **Expansion header access**, sized from the header body rather than from
  Waveshare's own narrower window, so a SolarLink-class card can mate.
- **Two presets.** `overbuilt` (3.2 mm walls, default) and `compact` (2.4 mm,
  about 1.6 mm narrower and 2.4 mm shorter, noticeably less rigid).

## What is not proven

Internal consistency is asserted by 52 automated checks. **Fit against physical
hardware is not.** Nobody has held these two components against a printed
chassis. [DATUMS.md](DATUMS.md#measure-these-before-a-final-print) lists the four
measurements to take first, and O-04 — a ~2.5 mm disagreement between the
factory drawing and the reference on button height — is the one to check before
anything else.

Print the chassis first, on its own, and offer the parts up to it before
committing to a full set.
