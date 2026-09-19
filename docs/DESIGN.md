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

Where the width is concerned there is nothing left to take: the keyboard pocket
is 110.2 mm — wide enough to clear both the certified 108.5 mm body and the
rounded retail figure, since the vendor has revised the unit at least once
without publishing new dimensions — and the walls are 3.2 mm, so 116.6 mm is the
floor. The board is 17 mm narrower than the keyboard pocket, and that mismatch is the single biggest
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
  form a torsion box; the reference's open tray is a channel.
- **The spine is a shear web.** The 3.2 mm rib between the two bays runs the
  full depth and the full width, right across the middle — which is where a
  device of this aspect ratio wants to fold. The reference's equivalent region
  is a 6.44 mm partition that stops short of the bezel.

### The numbers, and the one they contradict

`tools/structure.py` computes section properties from the rendered mesh — second
moment of area, section modulus, and Bredt's formula for the closed cell — at
90 stations along the axis the device folds about. It is classical section
analysis, not FEA, so it is trustworthy for **ratios and weak-point location**
and says nothing about absolute stress.

| | reference deck | this design |
|---|---|---|
| mean second moment `I` | 9 553 mm⁴ | **18 791 mm⁴** |
| mean section modulus `Z` | 891 mm³ | **1 559 mm³** |
| worst-section `Z` | 212 mm³ | **294 mm³** |

**1.97× the mean bending stiffness and 1.38× at the worst section, in a
smaller envelope.** Fitting the back plate is worth 3.41× on its own, which is
the monocoque argument in one number.

Moving the joint to the back is worth more than expected. At a typical station
the closed section's neutral axis sits at z = 2.22 mm and the joint plane at
z = 3.20 — 0.98 mm from the neutral axis, against 14.38 mm for the front face.
Bending stress varies with distance from the neutral axis, so **the joint at the
back carries about 93 % less bending stress than the same joint across the
face.** That was reasoned qualitatively before it was computed; the computation
made it stronger, not weaker.

The claim it *contradicts* is the "closed torsion box" one, and the correction
is worth stating plainly. Only **7 of 90 stations enclose a genuine cell — 8 %
of the length.** Everywhere else the front face is absent, because that is what
an aperture is, and the section is a U closed only by the back plate. The deck
is not a torsion box with two holes in it; it is two open channels joined by one
short closed cell at the spine. Bredt's formula at that cell gives J = 54 381 mm⁴
against roughly 2 910 mm⁴ for the same outline left open — about **19×**.

So the spine is not merely *a* shear web, it is the only closed cell in the
device, and it is carrying the torsional stiffness of the whole deck. That is a
better argument for it than the one originally made, and it is also a warning:
thinning the spine or opening it up for cable routing would cost far more than
its cross-section suggests.

### The weak point, named

The worst section is at **Y = −36.4 mm, mid-keyboard-bay** — 31 mm from the
spine, which does not cover it. `Z` there is 294 mm³ against a mean of 1 559.

The cause is not an oversight and cannot be designed out: the keyboard aperture
is 106.5 mm across a 116.6 mm body, so there is almost no front face left over
that bay — about 5 mm each side. Any material added there covers keys. The board
bay does not have the problem, because the display aperture is 86.8 mm wide and
leaves ~15 mm of face on each flank; its `Z` runs 1 300–3 200.

The reference deck is weakest in the same place for the same reason (212 mm³),
which is what makes the comparison like for like. This design is 38 % better at
the point that governs. **If this deck fails in a drop, that is where, and the
honest answer is that the keyboard's own width sets it.**

Wall thicknesses are integer multiples of a 0.4 mm nozzle (3.2 = 8 extrusions,
2.4 front = 6, 3.2 back = 8) so every wall prints as solid perimeters with no
sparse infill anywhere in the load path. `validate.py` asserts this.

## Form language

The brief is Dieter Rams read organically: his discipline and restraint, but
with the surfaces of something grown rather than extruded. Those two pull in
opposite directions, and where they conflict the resolution is always the same —
**the constraint wins and the form is honest about it.**

### Continuous curvature, not fillets

A conventional rounded rectangle is an arc tangent to a line. Position and
tangent match at the join; **curvature does not** — it jumps from 1/r to zero.
The eye reads that discontinuity as a hard corner however large the radius is,
which is why a fillet so often looks applied rather than grown.

Every visible corner here is instead a **superelliptical quadrant** of a larger
corner size. At `n = 3.2` with a corner size of 11.2 mm the curve tracks an R8
arc to within 0.2 mm, so the silhouette is unchanged — but the corner now runs
11.2 mm along each edge instead of 8, with curvature ramping in from zero at the
tangent point. That is the whole difference between a radius and a form.

A pure Lamé curve over the entire outline was tried first and is wrong at this
scale: it cuts about 10 mm off each corner, eats the wall, and loses the
straight edges the design is disciplined by. Straight edges with a continuous
corner is the Rams reading; a global superellipse is a lozenge.

### Edges roll, they do not chamfer

The previous revision broke the front and back arrises with a 1.0 mm chamfer —
a 45° cut, which is two more curvature discontinuities. Instead the shell is
lofted through a **smoothstep roll**: each face is inset 1.2 mm from the widest
section and arrives there with zero slope, so there is no arris to catch light.

This costs nothing in envelope. The widest section is at mid-thickness and is
*exactly* `body_w × body_h` — the softening is taken out of the faces inward,
never added outward — and the middle 44 % of the thickness stays at full wall,
so nothing is thinned where it carries load. The roll is bounded by the shell's
thinnest point, at the arris, which measures `wall − edge_soft`. At 1.2 mm that
leaves 2.0 mm of rim around the back opening, and `validate.py` asserts it.

### The cowl grows out of the panel

The battery swelling is the object's one large organic gesture, so it must not
look bolted on. It is lofted with a **tangent foot fillet** reached in the first
20 % of its rise, which removes the base line entirely — there is no edge where
it meets the back panel for light to break on — and it crowns with the same
smoothstep as the shell edges, so the whole object shares one curvature law.

The middle band of its rise is deliberately **straight-sided**. That is not
styling: the cell occupies the lower half of the cavity, and a blob that starts
crowning immediately narrows faster than what it has to contain. The first
attempt did exactly that and the cavity punched out through its own wall — the
back plate rendered as two disconnected bodies. `validate.py` now walks the
wall thickness along the full rise and asserts it never drops below 4
extrusions.

### One perforated element

There is exactly one perforated field on the object: the speaker grille, five
1.2 mm slots on a 2.3125 mm pitch spanning 10.45 mm — which is Waveshare's own
grille height to 0.00 mm. Finer perforation on a tighter pitch is the Braun
grille idiom, and confining it to one place is what lets it read as a considered
detail rather than as venting.

An earlier revision also carried a five-slot vent field, justified as insurance
for sustained Wi-Fi transmit. **It is removed.** The RLCD has no backlight, so
the only meaningful dissipation is the ESP32-S3 itself; the reference design — a
built, working device — has no ventilation beyond its grille and header slot;
and the field sat at an offset that could not be aligned with anything without
fouling the cowl. An element that is insurance rather than requirement, and that
cannot be composed, is the first thing *as little design as possible* removes.

### Controls recessed, not applied

The three buttons sit in **one shallow dish**, 0.9 mm deep with a soft-cornered
flared rim, rather than in three bare holes. The cluster reads as a single
element; the caps sit below the surrounding surface so nothing protrudes to
catch. This is the ET66 keypad move, and it costs 0.9 mm of a 3.2 mm wall.

### Where the language breaks, and why

The **display aperture's corner is smaller than the form wants** — 4.2 mm where
everything else is 9 to 11.2. It is not a compromise of taste. The aperture
height is trapped between the active area it must not clip (63.60 mm) and the
module edge it must still bear on (67.60 mm), leaving exactly 1.0 mm per side. A
superelliptical corner consumes `blend × 0.1948` of that at 45°, so anything
above about 4.6 clips the panel's square corner. 4.2 leaves 0.18 mm.

The outward flare opens it to an effective 5.65 mm at the visible face, which
softens it considerably. But the honest statement is that the panel sets this
corner, not the designer, and `validate.py` asserts the bound rather than
letting a later edit quietly violate it.

The **internal cavity** is likewise not given the treatment. A continuous corner
is tighter at the corner itself than an arc of the same visual line; applied to
the back opening at the shell's own 11.2 mm it undercuts the keyboard bay by
10.96 mm² and traps the keyboard. Measured across candidates, 6.0 mm is the
largest that reaches zero. The cavity is hidden, so it takes 6.0 and the wall
simply runs thicker at the corners (3.7 mm against 3.2 nominal) — which is where
a shell wants material anyway.

## Material and finish

The form is only half of biophilic; the surface is the other half, and it is
chosen at the printer rather than in the CAD.

**Preferred:** a wood-, stone- or hemp-filled PLA, or a matte PETG in a warm
neutral — bone, oat, clay, moss. These read as material rather than as plastic,
they hide layer lines, and they age by dulling rather than by scuffing bright.

**Avoid** gloss black and saturated colour. Gloss turns the rolled edges into
hard specular lines, which undoes the entire edge treatment, and a saturated
shell fights the display — the RLCD is a reflective monochrome panel whose
legibility depends on ambient light, so the body should return light to it, not
absorb it.

**Texture.** Enable the slicer's fuzzy-skin on the *outer walls only*, amplitude
0.15–0.25 mm, point distance 0.6 mm. On the rolled side walls this produces a
fine stochastic grain that catches light like a mineral surface, while the front
face — printed against the bed — stays smooth where the hand and eye actually
land. It costs nothing and it is the single highest-value finish decision.

**Inserts** should be brass and left visible. Rams never hid a fastener that was
doing work; four brass rings on the back are honest and they are the only metal
on the object.

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
- **Expansion header access**, sized from the header body rather than from
  Waveshare's own narrower window, so a SolarLink-class card can mate.
- **Two presets.** `overbuilt` (3.2 mm walls, default) and `compact` (2.4 mm,
  about 1.6 mm narrower and 2.4 mm shorter, noticeably less rigid).

## What is not proven

Internal consistency is asserted by 70 automated checks, and the structural
claims by classical section analysis in `tools/structure.py`. **Fit against
physical hardware is not, and neither is drop survival** — section analysis
gives ratios, not absolute stress, and nothing here models layer adhesion,
strain rate or impact. There is no FEA and no drop test. Nobody has held these two components against a printed
chassis. [DATUMS.md](DATUMS.md#measure-these-before-a-final-print) lists the four
measurements to take first, and O-04 — a ~2.5 mm disagreement between the
factory drawing and the reference on button height — is the one to check before
anything else.

Print the chassis first, on its own, and offer the parts up to it before
committing to a full set.
