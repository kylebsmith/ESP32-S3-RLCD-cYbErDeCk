# Form study — two planes, and what moving the acrylic actually buys

*A read-only study of the owner's proposal: a near-horizontal upper panel still
capped by the 12.70 acrylic, and a slanted lower bed carrying the stripped K9
Pro, not covered by acrylic, with the print rising around the keyboard to meet
the acrylic's edge. **No geometry and no parameter was changed to write this.**
Every figure below was measured off an exported artefact — `v2/export/dxf/slab.dxf`,
`v2/export/stl/*.stl`, `v2/export/render/.bodies/*.stl`,
`v2/export/render/.work/manifest.json` — with tools written for this study and
listed in §6.2. Nothing was read out of `parameters.scad` and then reported as a
measurement.*

---

## 0. Verdict up front

**DO IT, WITH THESE CHANGES.** Three numbers decide it, and none of them is the
one the proposal was argued on.

1. **The well is real, and it fails the standard this repository already uses on
   the knob.** A front-row key centre stands **9.6925 mm** behind a **12.7000 mm**
   square acrylic wall whose top face is **3.5778 mm** above the keycap it is
   trying to reach `[MEASURED §1.1]`. MIL-STD-1472H figure 57 asks for a
   **Ø32.000** bare-hand access opening; applied exactly as `knobs.md` applies it
   to the bore, **24 of the 61 keys fail it, the worst by 6.3075 mm**, and the
   front row's minimum straight-line approach angle over the arris is
   **20.26°** `[MEASURED §1.2]`. The forearm meets that arris **37.4650 mm**
   above the desk `[MEASURED §1.3]`.

2. **Moving the acrylic costs 0.18 % of the visible wiring plane and 0 % of the
   wire channels.** The carrier's exposed top face at z 0 totals
   **7659.2572 mm²**, of which **7645.5078 mm² (99.82 %)** already lies rearward
   of the keyboard frame's rear edge, and **100 % of the 1410.9436 mm² of wire-
   channel floor** lies rearward of it — the channels' plan bbox is
   y 13.8075 … 85.3875 `[MEASURED §3.1]`. **The brief's "wiring visible through
   clear acrylic" requirement is not in tension with this proposal at all.**
   That was the orchestrator's headline cost and it does not exist.

3. **The tilt is free at exactly one angle, and that angle is the keyboard's
   own.** Pivot the bed about the plate top's rear edge and tilt it
   **3.37914°** — the plane family the whole K9 Pro is built on `[CAD-EXACT,
   keyboard-strip.md §3.5]` — and the frame's inclined skirt rim becomes a
   **single horizontal plane at z −3.4050, level to 0.0001 mm**, which is
   *exactly* the z the carrier's annulus already clears, with the same
   **2.9450 mm** of floor under it `[DERIVED §3.4]`. The rear ports do not move,
   so USB-C service clearance stays at **4.2850 mm**. The deck's front edge drops
   from 37.4650 to **27.8424 mm** above the desk. **Nothing in the Z stack has to
   be re-derived.**

**And the things that are not true, said plainly.**

4. **`fabrication.md` row 12 is stale, not unresolved.** The orchestrator's brief
   calls it "the hardest unresolved item" and quotes "**FAIL in Y if the frame
   passes the slab … whether the frame passes is unresolved — §5.1**". §5.1 was
   closed by `parameters.scad` §8a, which fixed `kbd_seat_z = 5.190` and chose
   **RECESSED**. The exported void measures **285.7500 × 95.8850**, which is
   **smaller than the 287.000 × 97.500 frame by 0.6250 per side in X and 0.8075
   in Y** `[MEASURED §2.1]`. The frame cannot pass the slab. Row 12 describes a
   288.350 × 98.450 void that is not in the export. **The change does not delete
   an unresolved item; it deletes a resolved one, and the deletion is still worth
   having** — §2.1.

5. **"The front rail is the narrowest web in the part" is wrong by 1.8845 mm.**
   `parameters.scad` §8a item 5 and §9b both assert it. Measured contour-to-contour
   on the exported DXF, the narrowest web is **3.7980** — outline to the −x middle
   screw slot — and the front rail is **5.6825**, seventh in the list
   `[MEASURED §2.3]`. The slot family is the governing web and nothing in the repo
   says so.

6. **8:5 survives, bit-exact, because it governs the PLAN and nothing in the plan
   moves.** 5 × 304.6000 − 8 × 190.3750 = **0.000000000** off the exported outline
   `[MEASURED §3.6]`. It never governed the silhouette. The one term that could
   break it is `front_wall`, and the solve is brutal: **w = 4·front_wall − 9.475**,
   so **every 1.000 mm of front wall costs 4.000 mm of side wall and 8.000 mm of
   overall width** `[DERIVED §3.6]`. The chunky front panel has to be bought in
   **z**, which the slant gives for nothing, and must not be bought in **y**.

7. **`corner_r = 0` is affordable on the acrylic and it costs 0.8286 mm of corner
   reveal.** With the forearm no longer on the acrylic, the only objection left is
   the arris itself. Measured: the slab's outermost corner vertex stands
   **2.2797** outboard of the carrier rim today; square it and the same corner
   stands **3.1083** `[MEASURED §3.7]`. The reveal stops being constant and the
   corner becomes a chipping edge on PMMA. **Square the PRINT instead** — it is
   tough, it is the part your arm now touches, and it is where the brutalism is
   visible from the front.

8. **One live collision, and it is not in this proposal — it is in the owner's own
   new measurement.** At the measured **67 mm** NeoSlider travel the fader slot
   becomes 79.000 long, **overruns the 4U band by 1.4000 at each end, opens over
   the carrier's rear bearing rim**, and leaves a **0.5725 mm** web to the
   proposed panel's front edge `[MEASURED §3.8]`. The slot is drawn 52.000 today.
   **This has to be settled before the panel is re-cut**, in either form.

9. **The exported keyboard-frame reference body has its skirt wedge cut on the
   wrong side of zero.** `k9pro_internals.scad`'s `k9_skirt()` rotates a cut
   polygon through `rotate([0,90,0])`, which maps x → −z, so a wedge written for
   z −2.838 … −8.595 lands at **+2.838 … +8.595**. Measured on
   `ref_kbd_frame.stl`: **every down-facing face has normal exactly (0, 0, −1)**,
   the skirt rim is flat at **−8.595 at both y 0 and y 97.5**, and the frame's
   outer ring is truncated at the FRONT, **max z 2.8728 at y 0 against 3.7000**
   `[MEASURED §2.4]`. So the one artefact that would confirm the inclined rim does
   not carry it. **§0.3's level-rim result therefore rests on
   `keyboard-strip.md` §3.5's `[CAD-EXACT]` plane family, not on a measurement I
   took.** Said loudly because it is the weakest joint in this study.

---

## 0.1 Source tags

Same meanings as `fabrication.md` §0.1. Two used heavily here:

| tag | means |
|---|---|
| `[MEASURED]` | measured **today**, by me, off a named artefact in `v2/export`. Reproducible with §6.2. |
| `[DERIVED]` | arithmetic on `[MEASURED]` values. The arithmetic is shown. |
| `[LIT]` | peer-reviewed literature, cited with journal, volume and DOI. **Not** a house number and not treated as one. |

---

# 1. Is the well an ergonomic defect? Quantified.

## 1.1 The well, measured

Datums, all read off artefacts, none off `parameters.scad`:

| | value | artefact |
|---|---|---|
| carrier rim top (= the acrylic's underside; it bears there and nowhere else) | **9.5250** | `export/stl/carrier.stl` z max |
| slab thickness | **12.7000** | `export/stl/slab.stl` z extent |
| **acrylic top face** | **22.2250** | derived from the two above |
| deck underside (carrier bed −6.3500 less back panel 3.1750) | **−9.5250** | `carrier.stl`, `backpanel.stl` |
| foot height | **5.7150** | `export/stl/foot.stl` z extent |
| keyboard placement | **(−143.5000, −90.3125, 5.1900)** | `render/.work/manifest.json` |
| plate top, placed | **8.8900** | `ref_kbd_frame.stl` z max + placement |
| keycap tops, placed, 61 bodies | **18.4659 … 18.6472** | `ref_kbd_keycaps.stl` + placement |
| void front straight edge | **y −89.5050** | `export/dxf/slab.dxf` |
| nearest keycap edge | **y −88.9125** | `ref_kbd_keycaps.stl` + placement |

The placement is confirmed three independent ways before anything is built on it:
the frame's placed span is **x ±143.5000, y −90.3125 … +7.1875, z −3.4050 …
8.8900** — i.e. 287.000 × 97.500 at the plate top 8.890 and the skirt rim
−3.405; and the placed cap field is **284.3250 × 94.2000** centred **+0.0625 /
−0.2500** on the frame. All three agree with `parameters.scad`'s independent
literals, which is the control.

So:

```
WELL DEPTH   = 22.2250 - 18.6472 = 3.5778   at the tallest cap (the 6.25u space)
             = 22.2250 - 18.5030 = 3.7220   at a 1u front-row cap
             = 22.2250 - 18.4659 = 3.7591   at the shallowest cap in the field
REACH PAST THE WALL, to the nearest cap EDGE   = -88.9125 - (-89.5050) = 0.5925
REACH PAST THE WALL, to a front-row key CENTRE =                         9.6925
WALL HEIGHT                                     = 22.2250 - 9.5250     = 12.7000
ACRYLIC LAND IN FRONT OF THE WALL (the front rail)                     =  5.6825
```

`[MEASURED]`. **The finger descends 3.5778 … 3.7591 mm and reaches 9.6925 mm
inboard of a 12.7000 mm square-cut wall to put its pad on a key centre.** The
pad lands 0.5925 mm from that wall at the cap's near edge.

## 1.2 Against the standard the repo already uses

`knobs.md` §2 settles the knob by quoting **MIL-STD-1472H figure 57, *Minimal
Finger Access to First Joint*: "Push Button Access: Bare hand — 32 mm (1.25 in)
dia"** `[MIL fig 57, quoted verbatim in knobs.md]`, and uses it to conclude
"nothing reaches down the bore". Applied the same way — a Ø32.000 circle centred
on each key top, asked whether it fits inside the void:

| | |
|---|---|
| keys whose centre is nearer than 16.000 to the acrylic wall | **24 of 61** |
| worst wall distance | **9.6925** (the whole front row, all 8 keys) |
| **worst deficit** | **6.3075 mm** |
| circle area blocked by acrylic, worst key | **166.972 mm² of 804.248 (20.76 %)** |

`[MEASURED §6.2]`. **Honest limit, stated because it matters:** figure 57 is
written for a *circular* opening in a panel, and the keyboard void is a long
slot approached from one side. The circle test is therefore conservative. The
assumption-free version of the same fact needs no standard at all:

```
minimum straight-line approach angle to the front key row, over the arris
  tallest front-row cap   atan((22.2250 - 18.6472) / 9.6925) = 20.2607 deg
  a 1u front-row cap      atan((22.2250 - 18.5030) / 9.6925) = 21.0072 deg
```

`[DERIVED]`. **Every approach shallower than ~20.3° to the front row is blocked
by solid acrylic.** A finger's distal phalanx can be that steep. The metacarpals
behind it cannot.

## 1.3 Where the palm lands

The forearm and the heel of the hand meet the deck's front arris. Measured:

| layer | thickness | z |
|---|---|---|
| back panel, PETG | 3.1750 | −9.5250 … −6.3500 |
| carrier, PETG | 15.8750 | −6.3500 … +9.5250 |
| **acrylic, PMMA** | **12.7000** | +9.5250 … +22.2250 |
| **total** | **31.7500** | **acrylic is exactly 40.00 % of the front elevation** |

`[MEASURED]`. Plus `foot.stl`'s 5.7150, so **the arris stands 37.4650 mm above
the desk**, is 12.7000 mm of PMMA deep, and carries `corner_r = 2.000` — which
`parameters.scad` §11 justifies as "less kind to a forearm" being the thing it
is avoiding. **The forearm rests on the one material in the build that chips,
at the one height in the build that is highest, and then the fingers drop
3.6 mm into a slot.** That is the defect, stated geometrically.

## 1.4 Is a keyboard slant sourced in this repository? Partly.

I grepped `v2/research/`, `v2/parts/`, `v2/docs/` and `v2/layouts/` for slope,
tilt, wedge, wrist, ergonomic, forearm, elbow, home row, ISO 9241 and HFES.

**What IS sourced, and it is good:**

| figure | tag | where |
|---|---|---|
| Keychron's three published typing angles **3.4° / 5° / 8°** | `[VENDOR]` | `keyboard-k9pro.md` §8.2 |
| K9 Pro case slope derived from the published 7.6 / 13.3 heights, **3.346°** | `[DERIVED]` | `keyboard-k9pro.md` §3.1 — agrees with the vendor minimum to **0.05°** |
| the tray / shield / skirt plane family, **3.37914°**, off the plane normals | `[CAD-EXACT]` | `keyboard-strip.md` §2.1, §3.5 |
| keycap-plane slope is **not** the case slope, and must not be used to set a ramp | `[DERIVED]` | `keyboard-k9pro.md` §3.4 |

Three routes, one angle. **If this deck is going to have a typing slant at all,
3.37914° is the only angle in the repository with a provenance.**

**What is NOT sourced:** any wrist-angle, keyboard-height or slope-range figure.
`parameters.scad` §8b sets `kbd_tilt_deg = 0` on the argument that *"positive
tilt EXTENDS the wrist, and wrist extension is the posture that hurts… which is
why the ergonomic market sells negative tilt and nobody sells positive"*. The
first clause is true and the repo has no citation for it; the second is a market
claim with no source at all. **That line is `[DESIGN]` wearing the clothes of
`[STANDARD]`.** Here is what the literature actually says, cited, with no house
number invented:

| study | slopes tested | result |
|---|---|---|
| Marklin & Simoneau, *Human Factors* **43**(2):287–298, 2001, [doi:10.1518/001872001775900940](https://doi.org/10.1518/001872001775900940) | **+15°, +7.5°, 0°, −7.5°, −15°** | mean wrist extension falls from ≈**22°** at +15° to ≈**9°** at −15° — **≈0.43° of wrist extension per degree of slope** `[LIT]` |
| Simoneau, Marklin & Berman, *Physical Therapy* **83**(9):816–830, 2003, [doi:10.1093/ptj/83.9.816](https://doi.org/10.1093/ptj/83.9.816) | +7.5° … −15° | ≈12° of extension at +7.5°, ≈3° of **flexion** at −15° `[LIT]` |
| Nathan-Roberts et al., 2009 (split/gabled) | 0° … −4° | least wrist extension, pronation and elbow height in that band `[LIT]` |
| AHFE 2025 discomfort study | 0° vs −13° | **wrist discomfort significantly HIGHER at −13°** (M 6.30 vs 2.10) — the counter-evidence, and it is included on purpose `[LIT]` |

**So: the literature's working range is +15° to −15°, the extension optimum is
0° to −7.5°, and the comfort evidence disagrees with the extension evidence at
large negative angles.** The gradient — 0.43°/° — is the only number I will use
downstream, and it is used as a *cost*, never as a licence.

**At 3.37914° of positive slant the predicted wrist-extension penalty over flat
is 3.37914 × 0.4333 = +1.46°** `[DERIVED from LIT]`. Against a 20.26° approach
cone and a 3.6 mm well, that is not the thing that hurts.

---

# 2. What the change deletes

## 2.1 Row 12 — already dead, and worth burying anyway

`fabrication.md` §3 row 12 reads: *"void **288.350 × 98.450** `[parameters.scad]`
… **FAIL in Y if the frame passes the slab.** Draw 288.925 × 99.695 … **Whether
the frame passes is unresolved — §5.1.**"* and §5.1 names it the first blocking
unknown: *"the deciding number is the keyboard's Z seat, and it is in no file in
this repo."*

**It is in a file now.** `parameters.scad` §8a writes `kbd_seat_z = 5.190`,
`kbd_mount = "recessed"`, and re-sizes the void to `kbd_void = [285.750,
95.885]`. Measured on the artefact:

```
void, export/dxf/slab.dxf       285.7500 x 95.8850   (bbox x -142.8750..142.8750,
                                                           y  -89.5050..  6.3800)
frame, ref_kbd_frame.stl placed 287.0000 x 97.5000
overlap                         0.6250 per side in X, 0.8075 per side in Y
```

`[MEASURED]`. The void is **smaller** than the frame. The frame cannot pass the
slab; the slab overlaps its rim. **Row 12's premise is not the geometry that is
exported, and §5.1 item 1 is closed.** Both documents need re-recording — §4.4.

What the live row-12 risk actually is, and this is what the proposal deletes:
the overlap has to survive the laser's ±0.230 outboard excursion, and
`parameters.scad` computes it at **0.2640 worst case at the corner**, chosen at
the max-min of a three-way bracket between the measured keycap envelope, the
frame's R4.400 and the void's own corner radius. **Delete the void and all of
that goes: one corner-radius bracket, one 0.264 mm worst case, one
cap-envelope dependency, and `kbd_void`, `kbd_void_r`, `kbd_void_y`,
`kbd_cap_clear` and `kbd_overlap` with them.**

## 2.2 Row 2 — the perimeter mismatch. Slightly worse in kind, better in count.

Row 2 FAILS as drawn and is fixed by `carrier_inset = 1.905`: the acrylic always
overhangs the print, so the ±1.753 mm of possible mismatch becomes a reveal.
Measured on the artefacts, the reveal is **1.9050 on all four flats**
(slab ±152.3000 / ±95.1875 against carrier ±150.3950 / ±93.2825) `[MEASURED]`.

Shrink the panel and **three** of the four edges keep that fix unchanged — rear
and both sides. The fourth stops being a free perimeter edge and becomes an
**interior joint** at the keyboard's rear, which is a pair row 2 has never had.
Its closing requirement:

```
laser size tolerance on the panel depth                 +/- 0.230   [SVC sendcutsend]
print's own +/-0.5 % over the 8.890 from its own datum  +/- 0.0445  [SVC protolabs]
minimum gap that always closes                              0.2745
one grid (0.635) is 2.31 x that
```

`[DERIVED]`. **So the joint is affordable at one grid and reads as a deliberate
0.635 shadow line.** The genuine cost is elsewhere and must be said: **a reveal
needs something to overhang.** Where there is no acrylic, the print is the outer
surface, so the print's sides must come out to ±152.300 forward of the joint and
stay at ±150.395 behind it. **A 1.905 mm step appears in each side elevation at
the joint.** Either it is designed as the brutalist reveal line that turns the
corner, or it is a defect. There is no third option, and pretending the step can
be removed is how row 2 failed the first time.

## 2.3 Row 14 — improves where it does not matter, unchanged where it does

Row 14's budget is **0.770 mm for slab flatness plus carrier bow combined, at
the display**. Measured against the artefacts:

```
the 6 slab fasteners      x columns -144.7800 / +144.7800  (span 289.5600)
                          y rows     17.2125 / 48.9625 / 80.7125 (span 63.5000)
slab front edge  -95.1875 -> forward cantilever past the nearest fastener row 112.4000
slab rear  edge   95.1875 -> rearward cantilever                              14.4750
display pocket floor (carrier z -3.1750), plan x -136.9500..-43.1800,
                                                y   13.7775.. 84.1475
```

`[MEASURED]`. **Shrinking the panel deletes the 112.4000 mm forward cantilever
and leaves a 10.0250 mm one** — a factor of **11.2 in length**, and a
self-weight cantilever deflection goes as L⁴, so that term falls by ≈15,800×.
**But the display is not on that cantilever.** It sits between the fastener rows
in Y (overhanging each by only 3.4350) and in the **289.5600 mm unsupported X
span between the two fastener columns**, which does not change.

And the X span's self-weight sag is not the problem either:

```
12.700 PMMA strip, simply supported over 289.560, self weight
  d = 5qL^4 / 384EI = 0.0241 mm      E = 3300 MPa [ROHM xt-gs-ds]
                                     rho = 1190 kg/m3  NOT SOURCED (section 5)
```

`[DERIVED]`. **0.0241 against a 0.770 budget.** Row 14 is a *manufactured-flatness*
problem, not a bending problem, and manufactured flatness is `fabrication.md`
§5.2's blocking unknown. What the proposal does buy there is the **governing
length**: the panel's diagonal falls **359.1988 → 316.5887 mm, −11.86 %**, and
its depth **190.3750 → 86.2975, −54.67 %** `[MEASURED]`. If the eventual
flatness spec is per unit length — the usual form — the allowance falls
proportionally. **That improvement cannot be banked until §5.2 is closed, and I
am not banking it.**

PMMA volume falls **350.043 → 295.308 cm³, −15.64 %**, and the panel's outline
area to **45.33 %** of today's `[MEASURED]`.

## 2.4 A defect found in passing, which weakens one of my own arguments

`v2/cad/parts/k9pro_internals.scad` `k9_skirt()` cuts the rim back to the
inclined plane with

```
translate([-1,0,0]) rotate([0,90,0]) linear_extrude(height = frame_w + 2)
    polygon([[rim_z_front,0],[rim_z_rear,frame_d],
             [rim_z_rear-20,frame_d],[rim_z_front-20,0]]);
```

`rotate([0,90,0])` maps (x,y,z) → (z,y,−x), so the polygon's first coordinate —
the negative rim z values — comes out **positive**. Measured on the exported
body:

| | measured | intended |
|---|---|---|
| down-facing face normals | **every one is exactly (0, 0, −1)**, 6506 faces | a family at 3.37914° |
| skirt rim z at y 0 (front) | **−8.5950** | −2.8380 |
| skirt rim z at y 97.5 (rear) | **−8.5950** | −8.5950 |
| frame outer ring max z at y 0 | **+2.8728** | +3.7000 |
| distinct z values in the body | include **2.8380 … 3.7000**, i.e. the wedge landed above the part | — |

`[MEASURED §6.2]`. Two consequences: the reference body **overstates the skirt
depth at the front by 5.7570 mm**, and it **removes up to 0.8620 mm from the
frame's front bezel**. No check can see it: `check_render.py`'s gate is on the
keycap mesh, and the skirt's incline is not a quantity any artefact check reads.
**It is not mine to fix in a read-only study, and §0.9 records that my level-rim
result therefore leans on `keyboard-strip.md` §3.5's `[CAD-EXACT]` plane family
rather than on a measurement.**

---

# 3. What it costs — argued against the proposal

## 3.1 The visible wiring — the cost that is not there

The brief requires the wiring to be seen through the clear acrylic and laid out
intentionally on the carrier's top face. Measured off `carrier.stl`, by taking
every exactly-up-facing planar face at each named z and unioning its plan:

| | total | rearward of the frame's rear edge y +7.1875 | forward |
|---|---|---|---|
| exposed wiring plane, z 0.0000 | **7659.2572 mm²** | **7645.5078 (99.82 %)** | 13.7494 |
| wire-channel floors, z −2.0000 | **1410.9436 mm²** | **1410.9436 (100.00 %)** | **0.0000** |

and the channels' own plan bbox is **x −43.3866 … 114.9350, y 13.8075 …
85.3875** `[MEASURED]` — entirely inside the band. At the spine's front face
(y +8.8900, the sensible joint line) the plane is still **95.16 %** rearward and
the channels still 100 %.

**Every board is rearward too**, by the exporter's own placements: display
y 14.4125, Feather y 61.6625, the three encoders y 80.7125, the three pots
y 29.9125, the NeoSlider y 48.9625 `[MEASURED, manifest.json]`.

**So: shrinking the panel to the keyboard's rear edge removes acrylic from
13.7494 mm² of exposed wiring plane and 0.0000 mm² of wire channel.** The
proposal does not trade the wiring requirement away. It does not touch it.

*Refutation of my own point:* what IS lost is seeing the **aluminium frame and
the keycap field through clear 12.70** — the flush-inlay reading. That is real
and it is aesthetic, and the owner has asked for the opposite, so it is a
preference being exercised, not an error.

## 3.2 The six slab screws — the pattern survives unchanged

All six fasteners sit at y ∈ {17.2125, 48.9625, 80.7125} — **every one of them
rearward of the keyboard** `[MEASURED]`. Against a new front edge:

| fastener | bbox | centre → new edge, y +7.1875 | → y +8.8900 | slot edge → new edge |
|---|---|---|---|---|
| (+144.7800, +80.7125) | 4.4440 × 7.4450 | 73.5250 | 71.8225 | 68.1000 |
| (−144.7800, +80.7125) | 7.4260 × 4.7709 | 73.5250 | 71.8225 | 69.4370 |
| (+144.7800, +48.9625) **datum, round** | 4.4450 × 4.4450 | 41.7750 | 40.0725 | 37.8500 |
| (−144.7800, +48.9625) | 7.4440 × 4.4450 | 41.7750 | 40.0725 | 37.8500 |
| (+144.7800, +17.2125) | 4.4440 × 7.4450 | **10.0250** | **8.3225** | **4.6000** |
| (−144.7800, +17.2125) | 7.4260 × 4.7709 | 10.0250 | 8.3225 | 5.9370 |

`[MEASURED]`. Röhm's rule of record — hole **centre** to free edge ≥ 1.5 × hole Ø
= **6.6675** for Ø4.445 — is met at both candidate edges: **+3.3575** at
y +7.1875, **+1.6550** at y +8.8900. **The datum-plus-five-slots pattern needs no
change at all**, and the slots' radial directions are unaffected because the
datum does not move.

**And the sub-minimum-web count falls.** Measured contour-to-contour against
`[3P fab-guides]`'s ≥1 × *t* = 12.700:

| | pairs under 12.700 | narrowest |
|---|---|---|
| slab as exported | **13** | **3.7980** (outline ↔ −x middle slot) |
| proposed panel, front edge y +8.8900 | **10** | **3.7980** (same pair) |

`[MEASURED]`. The three that go are the front rail (5.6825) and the two
void-to-lower-slot webs (8.1023, 9.2212); three new outline-to-bore webs appear
at 10.0225 and one slot web tightens to 4.6000. **Net: three fewer DFM
violations and the governing web is untouched, because the governing web was
never the front rail** — §0.5.

**Does the new front edge reach sound bearing?** The carrier's rim at z 9.5250
runs continuously across the spine except for the **20.000 mm USB-C corridor
notch at x −9.5000 … +10.5000** `[MEASURED]`. So a panel edge at y +8.8900 lands
on the spine's full 3.200 width everywhere but a single 20.000 mm span — against
6.220 of bearing on the sides and rear. **Acceptable, and it must be stated
rather than discovered.**

## 3.3 The K9 Pro on a wedge — what has to hold it up

The plate seat is **flat and horizontal, everywhere, no slope** `[CAD-EXACT,
keyboard-strip.md §3]` — 7 cored bosses, 7 PCB-boss pads, 33 seat pads. Tilting
does not change any of that **relative to the frame**; it rotates the whole seat
family about X. **The carrier's plinth top becomes a single inclined plane
instead of a horizontal one, and every boss and pad keeps its height above that
plane.** There is no wedge to design under the aluminium: the plinth *is* the
wedge, and it is one plane.

What it costs the floor, pivoting about the plate top's rear edge at the
keyboard's own 3.37914°:

```
plate top      rear  8.8900 (unchanged)        front  3.1331   (-5.7569)
cap tops       rear 18.6472 (unchanged)        front 12.8903
skirt rim      rear -3.4050                    front -3.4049   LEVEL to 0.0001
floor under the skirt rim                             2.9450   (unchanged)
socket-pocket floor, at the keyboard front           -2.6579, with 3.6921 of floor under it
                                               (9.4490 today)
rear port aperture top                                5.2400   (unchanged)
  -> clear of the acrylic underside by                4.2850   (unchanged)
deck front edge                                      12.6024 -> 27.8424 above the desk
                                               (37.4650 today, -9.6226)
front keycap swings forward 9.7572 x sin(3.37914) =   0.5751
  -> cap leading edge y -89.4876 against the carrier pocket face -90.7875: +1.2999 clear
```

`[DERIVED from the measured datums of §1.1]`.

**The skirt rim going level is the whole argument.** It is level because the rim
already lies in the 3.37914° plane; tilting the part by its own plane angle
makes that plane horizontal. The carrier's annulus floor, today a flat plane
0.300 below the rim's *rear*, becomes a flat plane 0.300 below the rim
*everywhere* — at the same z, with the same 2.9450 of floor, deleting **5.757 mm
of dead air** at the front of the annulus. `kbd_skirt_z` and `kbd_annulus_z` do
not change value.

**The USB-C and power-switch edge still clears**, and it clears *because the
pivot is at the rear*. Put the pivot anywhere else and the ports move. If
instead the caps are made flush with the acrylic's top face — variant V1, §4.1 —
the whole keyboard rises 3.5778 and **the port aperture's clearance to the
acrylic underside collapses from 4.2850 to 0.7072 mm** `[DERIVED]`, which is
less than a USB-C overmould. **That is what kills the fully-flush variant, not
taste.**

The hard upper brackets on the angle, solved:

| bracket | θ |
|---|---|
| 3.175 of floor left under the front skirt rim (flush variant) | **5.3350°** |
| 1.270 of floor left under it | **6.4426°** |
| zero floor under it | **7.1784°** |
| the skirt rim at the deck's own underside | **9.0071°** |
| **the owner's "front edge seamless with the bottom"** | **15.3862°** (rear-step variant) / **17.2305°** (flush variant) |

`[DERIVED]`. **The literal "seamless with the bottom" needs 15.39°, which is at
or past the steepest slope any of the cited studies tested, and costs +6.67° of
wrist extension `[DERIVED from LIT]`.** It is not available. What IS available
is a front edge **9.6226 mm lower**, and a front elevation that is **100 % PETG
instead of 60 % PETG and 40 % PMMA** — which is the "seamless" the sentence is
really after.

## 3.4 Print volume, orientation and overhangs

```
carrier as exported            300.7900 x 186.5650 x 15.8750   footprint diagonal 353.9507
carrier proposed (rises to the acrylic's top face at the joint)
                               300.7900 x 186.5650 x 28.5750   unchanged in plan
```

`[MEASURED / DERIVED]`. **The binding dimension is 300.7900 in X and it does not
move.** The height goes 15.8750 → 28.5750, which no printer that can already
take 300.790 × 186.565 will notice. **The part needs a ≥305 mm bed today and it
needs a ≥305 mm bed after** — Prusa XL (360³) or a 400-class machine; a 256 or
300 mm bed does not take it either way.

**Overhangs.** The slanted bed is an **up-facing** plane at 3.37914°; an
up-facing plane is not an overhang at any angle. The printed walls that rise
around the keyboard are vertical. Two places do need naming:

1. **The acrylic's bearing land at the joint** is up-facing at z 9.5250 with a
   vertical wall in front of it rising to the cap plane — no down-facing face,
   provided **no lip is drawn over the acrylic's edge**. Draw one and it is a 0°
   overhang across 304.600 mm. Don't.
2. **The 20.000 mm USB-C corridor** already needs a bridged roof and the roof
   gets taller, not thinner. `fabrication.md` §4.5 item 7's ≤4:1 span-to-roof
   check covers it and will have to be re-run.

## 3.5 Is the acrylic's front edge structural, and can `corner_r` go to 0?

**It is not structural today and it is less so afterwards.** `knobs.md` §12 took
every load and every fit out of the acrylic; `parameters.scad` §9b states the
wrist load is "compression on a 2.495 × 9.525 PETG web". But the acrylic front
rail *is* the stiffest member across the front, and the arithmetic should be on
the record:

| | b × h | I |
|---|---|---|
| acrylic front rail as drawn | 5.6825 × 12.7000 PMMA | **969.99 mm⁴** |
| PETG front rim under it | 2.4950 × 9.5250 | 179.67 mm⁴ |
| V2 PETG front wall, 1.905 reveal **kept** | 2.4950 × 18.9524 | 1415.41 mm⁴ |
| V2 PETG front wall, reveal **released** (no acrylic to overhang) | 4.4000 × 18.9524 | **2496.11 mm⁴** |

`[DERIVED]`. **2.573 × the acrylic rail's second moment, in a wall that is
monolithic with the 6.350 floor instead of a loose plate resting on a rim.**
PETG's E is **NOT SOURCED** (§5), so this is a geometric ratio and not a
stiffness ratio, and it is quoted as such.

**`corner_r = 0` on the acrylic:** affordable, and here is the price.

```
slab as drawn, outermost corner vertex (151.7140, 94.6017) -> 2.2797 outboard of the rim
slab with corner_r 0, corner          (152.3000, 95.1875) -> 3.1083 outboard
flats everywhere else                                     -> 1.9050
```

`[MEASURED]`. **The reveal stops being constant: 1.9050 on the flats, 3.1083 at
the corners, +0.8286 on today's 2.2797.** Plus a 90° PMMA arris on a 12.700 slab
is a chipping edge that cannot be repaired, which is the thing `corner_r = 2.000`
was written to avoid. **So: keep `corner_r` on the acrylic, and square the
PRINT.** The print is tough, it is what the arm now touches, it is the whole
front elevation, and `carrier_inset_r = 1.000` exists only because offsetting an
R2.000 acrylic corner inward by 1.905 gives a 0.095 stress riser — an argument
that evaporates the moment the print is the outer surface at the front.

## 3.6 Does 8:5 survive? Yes — it governs the plan, and the plan does not move.

```
exported outline 304.6000 x 190.3750
  5 x 304.6000 = 1523.0000      8 x 190.3750 = 1523.0000      delta +0.000000000
```

`[MEASURED]`. It was derived on `body_w × body_h`, both plan dimensions, by
solving for the wall. **It never described the silhouette**, and the silhouette
is the only thing this proposal changes. Re-deriving the solve from the terms:

```
body_w = 288.350 + 2w
body_h = front_wall + 98.450 + 3.200 + 76.200 + w
5*body_w == 8*body_h   ->   w = 4*front_wall - 9.475
```

| front_wall | → wall | body | 5w − 8h |
|---|---|---|---|
| 3.175 | 3.225 | 294.800 × 184.250 | 0.000000 |
| **4.400** | **8.125** | **304.600 × 190.375** | **0.000000** |
| 5.000 | 10.525 | 309.400 × 193.375 | 0.000000 |
| 6.350 | 15.925 | 320.200 × 200.125 | 0.000000 |
| 8.125 | 23.025 | 334.400 × 209.000 | 0.000000 |
| 12.700 | 41.325 | 371.000 × 231.875 | 0.000000 — **too wide for any bed in reach** |

`[DERIVED]`. **Every 1.000 mm of front wall costs 4.000 mm of side wall and
8.000 mm of overall width.** The "chunky front panel" must therefore be bought
in Z — where the slant gives 18.9524 mm of PETG for nothing — and never in Y.
**This is the single most load-bearing constraint in the whole proposal and it
is not obvious from reading the file.**

One caveat with teeth: the plan **bounding box** stays 304.600 × 190.375 only if
the print's front comes out to y −95.1875 where the acrylic no longer overhangs
it. If the print stays inset 1.905 at the front, the deck's plan depth becomes
188.470 and **8:5 breaks**. §4.3 carries the parameter.

## 3.7 Recorded against myself

Three ways this study could be wrong, with the measurement that would settle
each:

- **The skirt-rim-goes-level result leans on a parameter, not an artefact**,
  because the artefact that carries it is broken (§2.4). *Settle it:* fix the
  `rotate([0,90,0])` sign, re-export `ref_kbd_frame.stl`, and re-measure the
  down-facing normals — they should be (0, ±0.058943, ∓0.998261).
- **Figure 57 is a circular-opening clause applied to a slot.** *Settle it:*
  §1.2's 20.26° approach-angle figure needs no standard and does not depend on
  it; if the circle test is rejected, the argument survives on that alone.
- **The 0.43°/° wrist gradient is from one study's means across five slopes**,
  and a 2025 study contradicts its sign at large negative angles. *Settle it:*
  it is used only to price a +3.38° slant at +1.46°, which is well inside any
  reading of the literature, and nothing else in this document depends on it.

## 3.8 One live collision this study ran into

The owner measured the NeoSlider at **67 mm** of travel. `slider_travel` is
**40.000 `[PROVISIONAL]`**. `slider_slot_l = travel + cap + 2.000`, so:

| travel | slot | y span | vs the 4U band 10.8625…87.0625 | vs the carrier's interior rear face 87.0625 | vs a panel front edge at +8.8900 |
|---|---|---|---|---|---|
| 40.000 | 52.000 | 22.9625 … 74.9625 | +12.1000 / +12.1000 | **CLEAR** | web 14.0725, ok |
| **67.000** | **79.000** | **9.4625 … 88.4625** | **−1.4000 / −1.4000** | **OPENS OVER THE REAR BEARING RIM** | **web 0.5725, under 1 × t** |

`[MEASURED, slot bbox read off `export/dxf/slab.dxf` as 12.0000 × 52.0000 centred
y 48.9625]`. **At 67 mm the fader slot no longer fits the 4U band in either
form, and in the shrunken panel it leaves a 0.5725 mm web to the front edge.**
Since `band = 4 * U` is one of the three terms in the 8:5 solve, this is a
footprint question, not a slot question. **It must be settled before any panel
is re-cut.** Not mine to settle here; recorded in §5.

---

# 4. Recommendation

## 4.1 The three variants, costed

| | **V1 FLUSH** caps level with the acrylic's top face | **V2 REAR STEP** plate top unchanged, step moves to the far edge | **V3 FLAT** acrylic moved, no tilt |
|---|---|---|---|
| well at the front row | **0.0000** | **0.0000** | **0.0000** |
| fig-57 failures | 0 | 0 | 0 |
| rear port clear of the acrylic underside | **0.7072 ✘** | **4.2850** (unchanged) | 4.2850 |
| skirt rim | level at +0.1728 | **level at −3.4050 = today's z** | unchanged, inclined |
| floor under the skirt rim | 6.5228 | **2.9450** (unchanged) | 2.9450 |
| deck front edge above the desk | 31.4202 | **27.8424** | 33.8872 |
| step at the acrylic joint | 0.0000 | **3.5778**, facing away from the hands | 3.5778 |
| keycaps recessed? (the brief's own rule) | **no ✘** | yes, by the printed wall | yes |
| wrist-extension penalty `[LIT]` | +1.46° | **+1.46°** | **0.00°** |
| keyboard Z re-derived? (seat, plinth, port corridor) | **yes — seat +3.5778, plinth 13.0268 thick, ports up 3.5778** | **no — every Z datum keeps its value** | no |

`[DERIVED §6.2]`.

## 4.2 **DO IT WITH THESE CHANGES — V2, at 3.37914°, joint at y +8.8900**

The three numbers:

1. **4.2850 → 4.2850.** Pivoting about the plate top's rear edge leaves the rear
   ports exactly where they are. V1 takes that to 0.7072 and is therefore not
   available at any angle.
2. **−3.4050, level to 0.0001, with 2.9450 of floor.** At the keyboard's own
   3.37914° the skirt rim becomes horizontal at precisely the z the carrier
   already clears. No Z datum in the deck changes. That is what makes the
   slant cost nothing structural, and it is the literal reading of "the most
   fundamental form possible with that geometry".
3. **99.82 % / 100 %.** The wiring requirement is untouched, so the only thing
   the proposal actually trades away is the flush-inlay look of the aluminium
   under clear acrylic — which is the thing the owner asked to remove.

**And the one change to the owner's description:** the front edge cannot be
seamless with the bottom. That needs 15.39°, which is at the edge of the
literature's tested range and costs +6.67° of wrist extension. **It comes down
9.6226 mm, to 27.8424 above the desk, and the front elevation becomes 100 %
PETG — one material, one unbroken face, square-cornered.** That is the
"seamless with the bottom" that is available, and it is available at +1.46°.

## 4.3 The exact parameter-level change list

All names in `v2/cad/parameters.scad` unless stated.

**New datums (all would be `[DESIGN]` unless noted):**

| name | value | note |
|---|---|---|
| `kbd_tilt_deg` | **3.37914** — change from 0 | `[CAD-EXACT]` by provenance: it is the keyboard's own plane family. §8b's whole paragraph must be rewritten; it is currently a `[DESIGN]` zero argued from an unsourced market claim. |
| `kbd_tilt_pivot_y` | **`kbd_frame_rear_y` = 7.1875** | the pivot is the plate top's REAR edge. This is the parameter that keeps the ports still; it must exist and be named, not implied. |
| `acrylic_y0` | **`kbd_spine_y0` = 8.8900** | the panel's front edge. Lands on the spine's full 3.200 top. |
| `acrylic_d` | **`body_h/2 − acrylic_y0` = 86.2975** | the panel's depth. |
| `acrylic_joint_gap` | **0.635** (1 grid) | `[DERIVED]` — 2.31 × the 0.2745 that must always close. §2.2. |
| `bed_plane_z0` | **`kbd_cap_top_z` = 18.6472 at `kbd_tilt_pivot_y`** | plane B's rear datum. Everything printed forward of the joint is flush with this plane. |
| `front_wall_t_eff` | **4.400** (print flush to `body_h/2` at the front) | not a new wall thickness — it is `front_wall` with the 1.905 reveal **released** because there is no acrylic to overhang. **Required, or the plan bbox drops to 188.470 and 8:5 breaks** (§3.6). |
| `print_corner_r` | **0.000** at the front and sides forward of the joint | the brutalist corner, on the tough material. |

**Changed:** `kbd_tilt_deg`; `carrier_inset` becomes piecewise (1.905 rearward of
`acrylic_y0`, 0.000 forward of it); `corner_r` **unchanged at 2.000** on the
acrylic (§3.5).

**Deleted outright:** `kbd_void`, `kbd_void_r`, `kbd_void_y`, `kbd_cap_clear`,
`kbd_overlap`, and the three-way corner-radius bracket in §8c that they support —
about **80 lines** of the hardest-won reasoning in the file, all of it made moot
rather than wrong.

**Re-expressed, not deleted:** `kbd_plate_gap = 0.635` loses its meaning — there
is no slab over the plate to gap from — but it is what currently *sets*
`kbd_plate_top_z = acrylic_z0 − kbd_plate_gap = 8.8900`. **`kbd_plate_top_z` must
become a primary `[DESIGN]` datum at 8.8900**, with the note that its value is
inherited from the recessed branch and that keeping it is what holds every
downstream Z still (§3.3). Deleting the gap without promoting the datum silently
undefines the keyboard's Z, and OpenSCAD will render and export that.

**Unchanged, and each one is a result rather than an assumption:** `body_w`,
`body_h`, `wall`, `front_wall`, `spine`, `band`, `interior_w`, `acrylic_t`,
`acrylic_z0`, `carrier_t`, `back_t`, `screw_x`, `screw_dy`, `slab_screw_pts()`,
`back_screw_pts()`, `kbd_skirt_z`, `kbd_annulus_z`, `kbd_ports`,
`kbd_corridor_w`, every board position, every knob datum.

**Unsourced, and how to close each:**

1. **`kbd_tilt_deg` = 3.37914** is `[CAD-EXACT]` for the *plane*, but the claim
   that the rim goes level depends on the exported body, which is broken (§2.4).
   *Close it:* fix the `rotate([0,90,0])` sign in `k9pro_internals.scad`
   `k9_skirt()`, re-export, and assert in `check_cad.py` that the frame body's
   down-facing normals are (0, ±0.058943, ∓0.998261) and that min-z(y) is a line
   of slope 0.0590456.
2. **The wrist-extension cost of a positive slant** is `[LIT]`, not `[STANDARD]`.
   *Close it:* nothing in this repo can. Record the citations and the +1.46°, and
   stop writing §8b's market claim as if it were sourced.
3. **PETG's elastic modulus**, needed before §3.5's 2.573 is a stiffness number
   rather than a geometry number. *Close it:* the filament's own TDS.
4. **PMMA density**, needed before any mass figure. *Close it:* Röhm's PLEXIGLAS
   GS datasheet, ρ per ISO 1183.
5. **`slider_travel`** — §3.8. *Close it:* calipers on the real NeoSlider, both
   end stops, as `parameters.scad` §10 already asks.

## 4.4 What has to be re-recorded

| file | what |
|---|---|
| `v2/tools/check_cad.py` | every baseline that reads `kbd_void*`, `kbd_overlap`, `kbd_cap_clear`, `kbd_plate_gap`; the acrylic-perimeter checks (3 edges, not 4); a **new** check that the printed bed plane and the cap plane are parallel and that the cap tops sit at the declared offset below it; a **new** check on the joint gap |
| `v2/tools/check_cad.py` | the "front rail is the narrowest web" assertion, which is **wrong today** (§0.5) and should be replaced by a measured min-web inventory with the governing pair named |
| `v2/tools/render/check_render.py` | all 28 gates re-shot; the acrylic-transmission gate C4 currently works over the full footprint and must be re-scoped to the panel; a **new** gate on the skirt rim once §2.4 is fixed |
| `v2/tools/render/render.py` | **`mask_elev` and `mask_elev_deck` are the views that change most** and are the ones that would catch a wrong slant; `plan` and `plan_noacrylic` change least. Add a **side elevation** — there isn't one, and the silhouette is now the point |
| `v2/docs/fabrication.md` | **row 12 deleted** (§2.1); **§5.1 item 1 closed** (§2.1); **row 2 restated for 3 edges + 1 joint** (§2.2); **§1.5's web table re-measured** — it lists the front rail at 4.400 and the artefact says 5.6825, and the narrowest web at all is 3.7980 (§0.5); **row 14's cantilever column updated** (§2.3) |
| `v2/cad/parameters.scad` | §8a's two-branch FLUSH/RECESSED argument becomes a three-branch one and the chosen branch changes; §8b is rewritten with the `[LIT]` citations; §9b's `rim_front_w` note no longer describes the front |
| `v2/layouts/README.md` | still quotes 5.639 for the wedge cost where §8b corrected it to 5.757, and still says "no ramp is needed anywhere" |

---

# 5. NOT SOURCED

Each with the procedure that closes it. Nothing here was estimated.

1. **Whether the K9 Pro's skirt rim really lies in the 3.37914° plane.**
   `keyboard-strip.md` §3.5 says `[CAD-EXACT]`; the exported body says the rim is
   flat (§2.4). *Procedure:* fix `k9_skirt()`'s rotate sign, re-export, measure
   the down-facing normals. **Blocking — §0.3 and the whole V2 recommendation
   rest on it.**
2. **`slider_travel`.** `[PROVISIONAL] 40.000` in the file, 67 mm measured by the
   owner. §3.8 shows the two answers are not compatible with the same footprint.
   *Procedure:* calipers on both end stops of the real part. **Blocking.**
3. **Any wrist-angle, keyboard-height or slope-range figure of record for this
   project.** The literature in §1.4 is cited and is `[LIT]`; there is no house
   number and this study does not invent one. *Procedure:* if a house number is
   wanted, it is a decision to be written down as `[DESIGN]` with the citations
   beside it, exactly as `knobs.md` does with the 6.350 proud height.
4. **PETG's elastic modulus** (§3.5) and **PMMA's density** (§2.3). *Procedure:*
   the filament TDS and Röhm's PLEXIGLAS GS datasheet.
5. **The cast slab's flatness** — unchanged from `fabrication.md` §5.2, and it is
   still the thing that decides whether §2.3's −11.86 % of diagonal is worth
   anything.
6. **Whether a 20.000 mm unsupported span of the panel's front edge over the
   USB-C corridor notch is acceptable** (§3.2). It almost certainly is; nothing
   measures it. *Procedure:* once the panel exists, dial-indicate the edge at the
   notch's centre with the deck assembled.

---

# 6. Reproduce it

## 6.1 Read

`v2/cad/parameters.scad` §§2, 3, 4, 6, 7, 8, 8a–8e, 9, 9b, 10, 11, 12 ·
`v2/cad/parts/k9pro_internals.scad` · `v2/tools/render/bodies.scad` ·
`v2/docs/fabrication.md` §§0, 1.2, 1.4, 1.5, 3, 4, 5 · `v2/docs/envelope.md` ·
`v2/parts/keyboard-strip.md` §§2.1, 3.5 · `v2/parts/knobs.md` §2 ·
`v2/parts/acrylic.md` §9 · `v2/research/keyboard-k9pro.md` §§3, 8 ·
`v2/research/precedents.md` §6 · `v2/layouts/README.md`

## 6.2 Measured — the artefacts and the method

Every figure in this document came from one of these, with
`/usr/bin/python3.11`, `ezdxf`, `shapely`, `trimesh`, `numpy`:

```
# the slab: loops by layer, bboxes, inscribed diameters, every pairwise web
ezdxf.readfile('v2/export/dxf/slab.dxf')   -> 1 CUT_OUTER, 14 CUT_INNER, 1 FIDUCIAL
  outline 304.6000 x 190.3750 ; void 285.7500 x 95.8850 at y -89.5050..6.3800
  13 contour pairs under 12.700, narrowest 3.7980 (outline <-> -x middle slot)

# the solids: bbox, and the PLAN of every exactly-horizontal planar face by z
trimesh.load(..., process=False); m.merge_vertices()      # is_watertight needs this
  carrier.stl  300.7900 x 186.5650 x 15.8750, z -6.3500..9.5250
  up-face plan at z  0.0000 -> 7659.2572 mm2 ; at z -2.0000 -> 1410.9436 mm2
  up-face plan at z  9.5250 -> 5396.1272 mm2 (the bearing rim, one piece,
                               notched 20.000 at x -9.5000..+10.5000)
  slab.stl 304.6000 x 190.3750 x 12.7000 ; foot.stl h 5.7150 ; backpanel.stl t 3.1750

# the keyboard, placed by the EXPORTER's own record, not by me
json.load('v2/export/render/.work/manifest.json')['placements']['keyboard']
  -> (-143.5000, -90.3125, 5.1900)
  ref_kbd_frame.stl   + placement -> x +/-143.5000, y -90.3125..7.1875, z -3.4050..8.8900
  ref_kbd_keycaps.stl + placement -> 61 bodies, cap tops 18.4659..18.6472,
                                     five rows on a 19.000 lattice
  (np.ptp, not ndarray.ptp -- numpy 2 removed the method)

# fig 57: a O32.000 circle on each key centre, differenced against the void polygon
  24 of 61 fail ; worst deficit 6.3075 ; worst blocked area 166.972 of 804.248 mm2

# the frame's skirt defect
  m.face_normals[m.face_normals[:,2] < -0.5]  -> 6506 faces, EVERY one (0,0,-1)
  min z per y slice -> -8.5950 at both ends ; max z at y 0 -> +2.8728 against 3.7000
```

The scripts are throwaway and live outside the tree; each one is ~40 lines and
the output above is the whole of what they produce. **Nothing in this document
was computed from the same parameters as the thing it describes**: every
geometric claim is a measurement of an emitted file, and where that was not
possible — the skirt's incline — §0.9 and §5.1 say so in those words.

## 6.3 The five checkers, run unchanged after this study

```
/usr/bin/python3.11 v2/tools/check_cad.py            ->   PASS
/usr/bin/python3.11 v2/tools/test_layout.py          ->   ALL PASS
/usr/bin/python3.11 v2/tools/test_dxf_post.py        ->   every check fires
/usr/bin/python3.11 v2/tools/export.py               ->   all exported
/usr/bin/python3.11 v2/tools/render/check_render.py  ->   28 passed, 0 FAILED, 0 skipped
```

---

# 7. One-line summary

**The well is a real 3.5778 mm defect behind a 12.7000 mm wall that 24 of 61
keys fail MIL-STD-1472H figure 57 against; moving the acrylic off the keyboard
costs 0.18 % of the visible wiring plane and no screw, and tilting the bed by
the keyboard's own 3.37914° about the plate top's rear edge makes the frame's
skirt rim level at exactly the z the carrier already clears — so do it, keep the
plate top where it is, and accept that the front edge comes down 9.6226 mm
rather than to the table.**
