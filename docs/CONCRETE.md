# Concrete jacket

A cast concrete outer shell over the printed chassis. `cad/concrete.scad`,
`variant = 3`.

## The architecture, and why it is this one

**The printed chassis is not replaced.** It stays exactly as validated and
becomes the permanent core. Concrete is cast around it as a jacket.

Two hard constraints force this and nothing else:

- **You cannot heat-set an M2 insert into stone.** Every threaded fastening in
  this design is a brass insert melted into plastic. Cast the enclosure and
  every one of them has to become something else.
- **Plain concrete has no useful tensile strength**, and the wall here is
  3.2 mm. Cast at that section it crazes on the first drop. Thin cast concrete
  only works reinforced, and reinforcement at 3 mm is not a thing.

Both problems vanish if the plastic stays. The concrete then does only what
concrete is genuinely good at — **mass, face and edge** — and the plastic keeps
doing tolerance, threads and precision. The jacket is also **captive by
geometry**: it wraps the front face and the sides, and its apertures are
smaller than the body, so it cannot be slid off in any direction. Cast in place
is the entire fixing; there is no adhesive to fail.

| | |
|---|---|
| Jacket wall | **7.0 mm** |
| Outer size | 131.9 × 148.1 × 23.9 mm |
| Concrete volume | 98.2 cm³ |
| **Concrete mass** | **206 g** |
| All-in (jacket + skeleton + board + keyboard + cell) | **≈ 556 g** |

Below 6 mm GFRC chips at the edges; above 8 mm the object passes 600 g and
stops being a handheld. 7 is the middle of that.

## The top edge stays plastic

The three buttons and both microphones open through the **top edge**. Burying
them under 7 mm would mean lengthening every button cap and tunnelling both
mic ports. The jacket stops short of that edge instead, leaving a plastic
control strip along the top.

This is a better object than the alternative — a concrete body with a visible
functional band — and it removes five features from the mould. `conc_top_open`
turns it off if you ever want the other version.

## The mix — what to actually put in the bucket

### Your acrylic instinct is right; the implementation matters

**Do not pour two-part epoxy into wet concrete.** Epoxy is hydrophobic. It will
not emulsify into the mix water — you get a weak marbled mess with cured
epoxy lumps and unhydrated cement around them.

The correct version of that same idea is an **acrylic polymer admixture**
(sold as acrylic fortifier or polymer modifier). It replaces **10–20 % of the
mix water**, and it does exactly what you were reaching for: higher flexural
strength, far better adhesion to the plastic core, much lower water
absorption, and edges that chip less.

### The full recipe

| Component | Proportion | Why |
|---|---|---|
| White or grey Portland cement | 1 part by weight | binder |
| **Fine** sand, ≤ 1 mm | 1–2 parts | at a 7 mm wall, aggregate above ~1.75 mm bridges and leaves voids |
| **AR glass fibre**, 12–13 mm | **2–3 % of total weight** | the thing that stops it cracking |
| Acrylic polymer admixture | 10–20 % of the water | see above |
| Superplasticiser | per label | lets you drop the water |
| Water | w/c **0.32–0.38** | water is what makes concrete weak and porous |
| Iron-oxide pigment | ≤ 10 % of cement weight | optional |

**The fibre must be alkali-resistant (AR).** Ordinary E-glass is slowly eaten
by the alkaline cement and the part loses its strength over a year or two.
This is the single most important line in the table.

If you would rather buy one bag: a commercial **GFRC face mix** or a
**countertop casting mix** is formulated for precisely this and already has the
fibre, the polymer and the plasticiser balanced.

## Release, and getting a smooth face

**The mould face finish is the object's finish, exactly.** Concrete records
everything, including every print layer line.

1. **Print `mold_face` with its moulding surface upward**, so that face is the
   top skin rather than a stack of layer edges.
2. **Seal it** — brush on the two-part clear epoxy you already have, let it
   cure, then wet-sand 400 → 1000. This buries the layer lines and gives a
   glass-hard, non-porous surface. This is where a brutalist-but-smooth face
   comes from; nothing later recovers it.
3. **Wax it** — carnauba paste wax or a proper mould release, buffed thin. Not
   petroleum jelly neat; it leaves a film that shows.
4. **Vibrate.** Non-negotiable. Without it you get bugholes across the show
   face. An orbital sander with no paper on it, held against the mould side for
   two or three minutes per lift, is enough.

## Procedure

1. Print `mold_face`, `mold_collar` and `cores`. Seal and wax the mould faces.
2. Bolt the collar down onto the face plate (M4, seven bolts).
3. Push the three **loose cores** in through the collar wall until each butts
   against the skeleton flank — USB-C, microSD, keyboard service window. These
   are the features a straight-pull mould cannot make.
4. Drop the printed chassis in, located on the dowels.
5. Pour in **two lifts**, vibrating each. Do not fill in one go.
6. Screed the back flat against the collar's freeboard.
7. Cover and leave **24 h**. Withdraw the cores sideways *first*, then unbolt
   and lift the collar, then lift off the face plate.
8. **Wet cure 7 days** under plastic. This is most of the final strength.
9. Seal, if you want it to resist finger oils.

A PLA mould is good for perhaps three to five pours before the faces degrade.

## What changed, and what did not

**Nothing in the validated enclosure changed.** No geometry, no tolerances, no
fasteners. All 115 checks still describe the part that goes inside. The
concrete work is entirely additive and lives in its own file.

## Open items

1. **The back face is the screeded face** — the one surface not formed against
   the mould. It will be the roughest. Either accept it, or grind and polish
   the back ring flat after curing.
2. **Cast-in is permanent.** The skeleton cannot be recovered from a bad
   casting. Print a spare before pouring.
3. **Thermal and drop behaviour are unmodelled.** Concrete around a plastic
   core with different expansion coefficients, dropped on a corner, is not
   something this repository has any evidence about.
4. **556 g is an estimate**, not a weighing. The concrete figure is computed
   from the rendered mesh at 2.1 g/cm³; the rest is assumed.
