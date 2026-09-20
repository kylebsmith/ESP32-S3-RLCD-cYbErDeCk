# Dimensional datum sheet

Every dimension this enclosure is built from, with its provenance, its
corroboration, and its known weaknesses.

The rule is simple: **no number enters `cad/parameters.scad` without a source
that a third party can check.** Where a number could not be sourced it is listed
in [Open items](#open-items) rather than guessed, and the design is shaped so
that the gap cannot cause a part to clash.

| Tag | Meaning |
|---|---|
| `[VENDOR]` | published by the component manufacturer, in a drawing, CAD file or spec table |
| `[MEASURED]` | numerically recovered from a reference artefact by `tools/measure_reference.py` |
| `[DERIVED]` | arithmetic on a `[VENDOR]` or `[MEASURED]` value |
| `[STANDARD]` | an ISO/IEC standard or a manufacturer's fitting specification |
| `[DESIGN]` | a choice made by this project, with rationale |
| `[PROVISIONAL]` | not corroborated by a second independent source |

---

## Sources

| # | Source | Kind | Used for |
|---|---|---|---|
| S1 | `ESP32-S3-RLCD-4.2-3dFile.rar` — Creo STEP assembly, dimensioned DXF, dimensioned PDF, published by Waveshare at `files.waveshare.com/wiki/ESP32-S3-RLCD-4.2/` | Manufacturer CAD | All board geometry |
| S2 | `docs.waveshare.com/ESP32-S3-RLCD-4.2` and `waveshare.com/esp32-s3-rlcd-4.2.htm` | Manufacturer spec text | Panel type, resolution, connector types |
| S3 | `riitek.com/product/259.html` — "Mini Keyboard 518BT" | Manufacturer spec text | Keyboard outline and mass |
| S4 | FCC ID `YIZRT-RII518` (Shenzhen Riitek), granted 2011-05-23 | Regulatory filing | Keyboard identity, control/port faces |
| S5 | `github.com/nilseuropa/solar_term` — SolarTerm enclosure, two independent designs (`stl/ata/`, `stl/poc_*`) | Third-party reference artefact | Independent corroboration; keyboard pocket |
| S6 | `makerworld.com/models/3251467` — "SolarTerm: ShareWave ESP32 S3 RLCD4.2 Case", BY-NC-SA, derived from S5 | Third-party reference | Entry point; no geometry taken |
| S7 | ISO 273, ISO 4762, ISO 10642 | Standards | Fastener fits |

S5 is used as a **measuring instrument, not as a design**. See
[PROVENANCE.md](PROVENANCE.md).

---

## D-01 — Board mounting-hole pattern

**85.500 × 62.100 mm**, four positions, 3.50 mm inset from every PCB edge.

This is the single most load-bearing datum in the project: it fixes where the
board sits, and everything else is referenced to it. It is therefore established
three times, independently:

| Derivation | Method | Result |
|---|---|---|
| A | `stl/poc_bottom.stl` sectioned at Z = 0.05, four Ø2.700 bores | 62.100 × 85.500 |
| B | `stl/ata/Caseback.stl` sectioned at Y = −11.323, four Ø2.700 bores | 62.100 × 85.500 |
| C | Waveshare DXF `DIMENSION` entities #9 and #11 | 85.5 and 62.1 |

A and B are two unrelated enclosure designs by different authors. They agree
with each other to **0.001 mm** and with the factory drawing exactly.
`tools/measure_reference.py` re-derives A and B on every run and asserts the
cross-check; `tools/validate.py` asserts the value is not `[PROVISIONAL]`.

**The holes are not plain holes.** The PCB carries four Ø4.20 through-holes,
each with a surface-mount **SMTSO-M2.5-7ET** standoff on the back face: Ø5.50
body, 7.00 mm tall, female M2.5 thread. The enclosure drives M2.5 screws *into
the board's own standoffs*; it supplies no bosses of its own, and it must leave
the standoff seating plane clear.

## D-02 — Board outline

**92.50 × 69.10 × 1.60 mm**, R0.50 corners. `[VENDOR]` S1, drawing text
`92.5 PCB OD` / `69.1 PCB OD`.

See [C-01](#c-01--the-701-mm-figure-is-the-stand-base-not-the-pcb).

## D-03 — Board depth stack

All in Waveshare's `W` frame: `W = 0` at the PCB back (component) face,
`+W` toward the display.

| Feature | W | Note |
|---|---|---|
| Display glass front | +3.75 | what the front-face lip bears on |
| PCB front face | +1.60 | |
| PCB back face | 0.00 | datum |
| SMTSO standoff end | −7.00 | what the back plate bears on |
| 18650 holder end | −15.20 | the deepest feature on the board |

Clear depth the enclosure must provide: **10.75 mm** (+3.75 to −7.00).
Holder protrusion past the standoff plane: **8.20 mm** — this, not the cell
diameter, is what the battery cowl has to swallow.

## D-04 — Keyboard outline

**108.5 × 58.2 × 10.2 mm**, 75 g. `[VENDOR]`

Source: the manufacturer's own user manual, filed as the **User Manual exhibit
(attachment 1470433) of FCC ID YIZRT-RII518**, page 13, section
*6. Technical Specifications*:

> Dimension：108.5mmx58.2mmx10.2mm
> Weight：75g

This closes the ±2.54 mm rounding band the project carried for most of its life.
108.5 × 58.2 × 10.2 is 4.272 × 2.291 × 0.402 in, **every axis of which rounds
correctly to riitek.com's published `4.3*2.3*0.4in`** — so the vendor's inch
figure is a rounded restatement of this same value rather than a second
independent source.

**Why it stayed hidden.** The manual's text is converted to vector outlines —
the PDF title contains `转曲`, Chinese for *convert to curves*. `pdftotext`
returns 2 139 characters from the 14-page file, all of it FCC boilerplate; the
specification table has no text layer at all and appears only when the page is
rasterised. Any full-text search of these exhibits returns the confident, wrong
answer that they carry no dimensions — which is exactly what an earlier pass of
this project concluded.

### Corroborated by the manufacturer's own drawing

Riitek's current product page carries a dimensioned product-overview image which
prints **both units on every axis, in one string per dimension**:

> `108.5mm/4.3inch`   `58.2mm/2.3inch`   `10.2mm/0.4inch`

That is decisive on the direction of the conversion. 4.3 in is 109.22 mm, not
108.5, so the millimetre figure cannot be derived from the inch figure — the
inch figure must be derived from the millimetre one. The drawing also labels the
**mini-USB charging port** and the **power switch**, both on the same short
edge, and shows a 68-key field with no pointing device.

So two independent manufacturer documents twelve years apart — the 2011 FCC
manual and the current drawing — give the same outline to the digit. Whatever
changed between the 75 g and 64.8 g samples, **the outline did not.**

What is left is ordinary moulding tolerance. Neither document states one, so the
design assumes ±0.30 mm in plane and +0.40 / −0.00 mm on thickness, the latter
because the drawing does not say whether 10.2 mm is to the moulding's top face
or to the keycap crowns. Both are tagged `[DESIGN]` in `parameters.scad` rather
than dressed up as measurements.

### Traps, all confirmed from primary sources

- **109.22 × 58.42 × 10.16 is not a body.** It is this project's own round trip
  through 0.1-inch granularity — see [C-07](#corrections).
- **150 × 100 × 20 mm / 120 g is the retail box.** 38 % longer and 72 % wider
  than the certified body; the 120 g includes the cable and printed manual.
- **The Rii K18 is not this keyboard, and neither is the RT518/RT518S.** The
  RT518 manual gives 317.2 × 123.6 × 18.3 mm, 342 g — three times the length,
  with a touchpad.
- **`4.09 × 2.28 × 0.43 in` circulates widely.** Now traced: it is Amazon's own
  structured *Product Dimensions* field on ASIN B0B46F8RS6, which contradicts
  the *Keyboard Description* field on the same page (`4.3*2.3*0.4in`). Its
  length of 103.9 mm is physically impossible — all three measured third-party
  pockets are larger. Rii's own storefront ships the sentence *"Keyboard
  measures"* with the value **missing**, in both the rendered HTML and its
  JSON-LD; that empty slot is the likely thing a third party back-filled.
- **riimall.com's own storefront claims an integrated touchpad.** The FCC
  external and internal photographs show there is none. Marketing copy for this
  model is unreliable even from the brand's own shop.

## D-05 — Keyboard pocket

**109.85 × 59.15 × 11.0 mm** `[DESIGN]` — stated, not derived from the body plus a
clearance, so the envelope does not move every time a tolerance assumption is
revisited. What it must satisfy is asserted instead.

| Clears | per side, in plane | depth |
|---|---|---|
| nominal body 108.5 × 58.2 × 10.2 | 0.675 / 0.475 | +0.80 |
| body at tolerance 108.8 × 58.5 × 10.6 | 0.525 / 0.325 | +0.40 |
| ATA tray, a working device, 109.200 × 59.200 | +0.325 / −0.025 | — |
| grip case, measured, 109.406 × 59.005 | +0.222 / +0.073 | — |

The pocket was **110.2 × 59.4** until the front lip was checked with the
keyboard anywhere other than centred, and went negative at a corner — see
[O-09](#o-09--the-keyboard-lip-goes-negative-at-a-corner--closed). Tightening
it to the floor both assertions allow recovered about two thirds of that; the
rest came from **eight half-round locating ribs**, four on the long walls at
±X and four on the short walls at ±Y.

| | bare pocket | ribbed | largest body | smallest body |
|---|---|---|---|---|
| width | 109.85 | 108.95 | 108.80 → +0.075/side | 108.20 → ±0.375 travel |
| height | 59.15 | 58.65 | 58.50 → +0.075/side | 57.90 → ±0.375 travel |

Each rib is a cylinder centred **on** the wall plane, so half of it is buried
in material already there and only the inboard half is new — it fuses to the
wall with no seam to delaminate — and each is tapered over its first 1.6 mm at
the entry end, because the keyboard loads from behind. They are only 2*r wide,
so they clear both the service window and the corner blends easily. The ribs
are load-bearing for the lip guarantee, so `validate.py` measures the ribbed
span **from the rendered chassis** rather than trusting the parameters: a rib
that was specified and not built fails the lip check exactly as a bare pocket
does.

The height row against the ATA tray is **0.025 mm negative**, and that is
deliberate. The tray's own clearance over the 58.2 body is 0.50 per side; this
pocket's is 0.475. The 0.6 mm margin this design asserts against a working
reference is applied to the width, where the certified body is 108.5 and the
tray's 109.200 is the only physical evidence of what a real unit needs.

The depth sits **exactly** on its assertion at the top of the thickness band:
11.0 mm against 10.6 + 0.4. That is the one axis with no slack left, and it is
also the axis whose nominal is least certain.

Because this design's pocket is open front-to-back — the keyboard loads from
behind and is held forward by a pad on the back plate — there is no internal
floor-to-wall corner for a print fillet to interfere with, so the base relief
the ATA needs is not required here.

## D-06 — Board edge features

Positions relative to the mounting-pattern centre, which coincides with the PCB
centre at `(U, V) = (46.25, 34.55)`.

| Feature | Offset | Size | Source |
|---|---|---|---|
| Buttons ×3 (PWR/BOOT/KEY) | X = −10, 0, +10 | body 4.553 × 2.203 | `[VENDOR]` S1 |
| Microphones ×2 | X = **±32.500** | 4.00 × 3.00 × 1.00 | `[VENDOR]` S1 |
| 18650 holder | Y = **−19.400** | 77.80 × 22.10 | `[VENDOR]` S1 |
| Speaker grille | Y = **+16.000** | 14.70 × 10.45 | `[VENDOR]` S1 |
| USB-C | Y = 0.00, W = −1.170 | shell 9.582 × 4.163 | `[VENDOR]` S1 |
| microSD | Y = +19.15, W = −0.625 | socket 16.103 × 2.452 | `[VENDOR]` S1 |
| 2×8 expansion header | X = −0.10, Y = −0.70 | body 21.003 × 6.603 × 8.603 | `[VENDOR]` S1 |

**Independent confirmation.** Before S1 was located, three of these were
measured from S5 by sectioning `stl/ata/Caseback.stl` normal to the board's long
axis. They match the factory drawing exactly:

| Datum | Measured from S5 | Factory drawing S1 |
|---|---|---|
| Microphone offsets | ±32.500 | ±32.500 |
| Battery offset | −19.400 | −19.400 |
| Button pitch | 10.000 | 10.000 |
| Speaker grille height (4 slots × 3.05 pitch + 1.3) | 10.45 | 10.45 |
| microSD offset | +19.25 | +19.15 |

Two independent measurement paths converging to three decimal places is the
strongest evidence in this document.

## D-07 — Display

| Datum | Value | Source |
|---|---|---|
| TFT module outline | 91.00 ±0.10 × 67.60 ±0.10 × 1.50 | `[VENDOR]` S1 |
| Active area | **84.80 ±0.10 × 63.60 ±0.10** | `[VENDOR]` S1 |
| Pixel pitch | 0.2120 mm square, 400 × 300 | `[DERIVED]` |
| True diagonal | 106.00 mm = 4.173 in | `[DERIVED]` |
| Active-area offset from PCB centre | **−1.60 mm along U**, centred along V | `[VENDOR]` S1 |
| ... independently measured | −1.604 mm along U | `[MEASURED]` S5 `poc_top.stl` |
| Aperture used here | 86.8 × 65.6 | `[MEASURED]` S5 — a 1.00 mm reveal per side |

The offset is confirmed independently: the proof-of-concept enclosure's display
aperture centre sits at (34.5640, 45.6460) against a mounting-pattern centre of
(34.550, 47.250) — an offset of **−1.604 mm** along the same axis. That design's
aperture is also *exactly* 63.600 × 84.800, i.e. the active area itself with
zero reveal; the 1.00 mm per-side reveal used here comes from the later ATA
bezel and is the more forgiving of the two.

See [C-02](#c-02--the-active-area-is-not-42-inches-and-is-not-centred).

## D-08 — Reference enclosure, as-built

Measured from S5 for comparison, not used as design input.

| Datum | ATA design | Proof-of-concept |
|---|---|---|
| Outer envelope | 151.650 × 116.770 × 18.000 | 142.100 × 112.500 × 15.248 |
| Wall thickness | 2.900 | 1.000 |
| Board pocket | 71.116 × 94.519 × 13.000 | — |
| Keyboard tray | 59.200 × 109.200 × 11.400 | 60.600 × 110.498 × 10.750 |
| Back wall over the board | 3.000 | 3.500 |
| M3 insert bore | Ø4.000, 8.1 deep | — |
| Bezel | separate, 2.000 thick | separate top shell |
| Battery cover | separate, 83.0 × 27.0, 12.0 proud | — |

## D-09 — Acrylic display window

**70.8163 × 94.2193 mm**, ~1 mm chamfered corners, with a 1.0 × 29.3 mm relief
step on one long edge. `[MEASURED]` from `stl/ata/plexiglass.dxf`, a single
closed `LWPOLYLINE` of 26 vertices, DXF AC1015, `$INSUNITS = 4` (millimetres).

A 2D template is an exact, unambiguous statement of intent, so this is among the
highest-confidence datums available. It is carried as an optional part.

---

## Corrections

Recorded rather than quietly fixed, because each was believed and acted on
before being caught.

### C-01 — The 70.1 mm figure is the stand base, not the PCB

Early revisions carried `board_h = 70.1` from a distributor spec line, tagged
`[PROVISIONAL]`. The factory drawing shows that "92.5 × 70.1 × 13.5 mm"
conflates three different objects:

- **92.50** is the PCB length — correct;
- **70.10** is the moulded **stand base**, which overhangs the PCB by exactly
  1.00 mm on one long edge. The PCB is 69.10;
- **13.50** is the display-front-to-base-rear stack, and it **excludes the
  battery holder**, which protrudes a further 5.45 mm. Real maximum: 18.95.

A pocket cut to 70.1 leaves a 1 mm gap on one long edge. Consequence here: this
design discards the stand base and kickstand entirely, which removes the 70.1
dimension, saves 2.75 mm of thickness and 1.0 mm of width, and costs nothing
mechanically because the standoffs are on the PCB.

**The drawing can be read either way, and one reading is a trap.** Chaining the
hole offsets gives `4.50 + 62.10 + 3.50 = 70.10`, which appears to confirm the
70.1 figure — and a second, independent pass over the same drawing reached
exactly that conclusion. It is wrong: the 4.50 is dimensioned from the **stand
base** edge while the 3.50 is dimensioned from the **PCB** edge, so the chain
mixes two datums. Removing the base's 1.00 mm overhang gives the self-consistent
`3.50 + 62.10 + 3.50 = 69.10`, which matches the front view's own `69.1 PCB OD`
annotation and the 3.50 mm inset quoted on all four sides.

A third line settles it independently of the drawing: the reference enclosure's
board pocket is 71.1163 × 94.5193 against a bare PCB of 69.1098 × 92.5098 —
**1.0032 / 1.0047 mm per side, symmetric**. Against a 70.10 mm base the same
pocket gives a lopsided 1.01 / 0.508.

### C-02 — The active area is not 4.2 inches, and is not centred

An earlier revision derived the active area from the nominal "4.2 inch"
diagonal and a 400 × 300 matrix, giving 85.344 × 64.008 mm centred on the board.
The drawing gives **84.80 × 63.60** — about 0.5 mm smaller in each axis — and
the area sits **1.60 mm toward the U = 0 edge**, with margins of 2.25 mm and
5.45 mm rather than equal ones. The true diagonal is 4.173 in.

A frame cut to the derived figure would have been oversized *and* 1.6 mm off the
panel on one side. `tools/validate.py` now asserts both the value and the
offset.

### C-03 — The "expansion access window" was part of the carry hanger

A 24.004 × 29.382 mm tapered recess in `stl/ata/Caseback.stl` was recorded as a
`[MEASURED]` expansion-header window. It is not. It sits at board-relative
(X +45.2, Y +36.1) — diagonally off the corner of the PCB, largely outside the
board outline — and it is part of the reference's **carry hanger**, which is
precisely the feature that differs between its `Caseback.stl` and
`Caseback_left.stl` variants.

The window is now placed from S1 instead, at the header's real centre.

### C-04 — Waveshare's own header window is narrower than the header

Waveshare's stand base cuts 21.60 × 5.60 for a header whose insulator is
21.003 × **6.603**. Their window exposes the pin field, with the body sitting in
a wider recess behind it. This back plate is 3.2 mm thick and the header stands
8.603 mm off the PCB back — **1.60 mm proud of the standoff plane** — so the
body itself must pass through. The window here is sized from the body.

### C-32 — The plate's bed face was a pad over open air

`docs/ASSEMBLY.md` prints the back plate **cowl up**, so its inner face is the
bed face. That face was not flat: the 0.25 mm keyboard keeper stood proud of it,
so the pad's **6233 mm²** was the only thing touching the bed and the remaining
**5777 mm² — 48% of the underside —** printed as a flat sheet over 0.25 mm of
open air.

The keeper's *thickness* was right and derived (`board_depth − kbd_depth`); the
wrong part was which of the two parts carried it. The chassis's keyboard bay now
stops 0.25 mm short of the front panel, leaving a band the keyboard bears on,
and the aperture is carried down through that band at the same section as the
flared aperture's narrow end so the two meet tangentially. The plate's inner
face is one plane at z = 3.20 with **11,888 mm² flat on the bed** and 166 mm²
within a millimetre of it. Nothing about the keyboard's clear depth, bearing
area or retaining lip changes.

### C-31 — Two countersinks were breaking out through the plate's rim — BLOCKING

The upper pair of shell-screw countersinks was positioned at
`plate_half_h − head/2 − margin`. `plate_half_h` is the plate's **mid-thickness**
section, and the outer *face* is rolled in from it by `plate_edge_soft`. Worse,
`boss_cx` = 51.24 lands inside the face's corner blend, which starts at
x = 48.63 — and there the outline runs diagonally, so a vertical extent is not a
clearance. The Ø4.0 countersink had **0.022 mm** of plate to bite on and opened
a notch in the rim at both top corners.

The give-away is topological, not dimensional: a countersink that has merged
with the outline is no longer an interior ring. `validate.py` now checks exactly
that, and it is also why the first attempt at a fix went wrong — measuring the
"highest usable y" against the *rendered* plate returns 58.5, because the
outline being measured already contains the breakout. Solved against the
analytic face outline instead: **62.25**, which also clears the microSD tunnel's
61.425 floor by 0.825 mm.

`boss_cx` cannot move inboard to escape the blend: the board pocket floors it at
50.05 and the blend starts at 48.63.

### C-30 — The tongue groove's wall was 0.48 mm, and three places said 1.60

`back_t − tongue_depth` is not the wall left outboard of the groove. That
arithmetic assumes the outer face sits at the nominal envelope; `rse_soft`'s edge
roll withdraws it by up to 1.2 mm over the back 4.7 mm of the thickness, and the
groove sat inside that band. The rendered chassis carried **0.480 mm** — 1.2
extrusions — while the source comment, the documentation and
`validate.py`'s own "groove leaves a printable bottom wall" check all said 1.60.
All three computed from parameters; none touched the mesh.

`tongue_depth` 1.6 → 1.3, `tongue_z` `back_t/2` → 2.2, and `tongue_len` gains
`fit_slide` so shortening the groove does not shorten the tongue. Measured
result: **0.943 mm** (2.36 extrusions), engagement unchanged at 0.90 mm, tongue
tip unmoved. The check now ray-casts the rendered chassis and reports both the
measurement and what the old arithmetic claimed.

### C-29 — The button cap was the reference sprue's bounding box

`button_cap_w/h` were 5.2 × 4.0, taken as the reference cap's cross-section.
They are its **bounding box**: 5.2 is the flange skirt at the rear face and 4.0
is the sprue's axial depth. The cap's own prismatic section is a constant
4.800 × 3.800 swept along the cap axis.

At 5.2 in a 5.400 aperture the caps had **0.10 mm per side** against this
project's own sliding fit of `fit_slide` = 0.30 — they would have bound.
`button_flange` was wrong in the same way: 0.4 was the past-*aperture* figure
used as a past-*cap* one, leaving 0.200 of overhang against 0.200 of float, so a
cap could walk out of its own aperture. Now 4.8 × 3.8 with a 0.7 flange:
0.30 mm per side of clearance, 0.40 mm per side of overhang.

### C-28 — The fix for C-25 opened a slit into the battery cavity — BLOCKING

C-25 found the battery cowl lying on top of two M2.5 board-screw countersinks
and fixed it two ways at once: by deriving the cowl's plan form (82.80 × 27.10)
and by opening its outer plan corner from **6.0 to 12.6** to pull the flank off
the screws.

The second half of that fix was itself blocking. A fuller outer corner draws the
surface **in** at 45°, while the cavity's near-square R2.2 corner stays where it
is. The wall between them went from 2.07 mm on the flats to **0.0385 mm at the
corner** — an open slit into the 18650 cavity, roughly 0.6 mm of arc by 3.8 mm
tall, on all four corners. The part was watertight and printed as a single body,
so nothing downstream objected.

| | flats | corner |
|---|---|---|
| declared `batt_cowl_wall` | 2.00 | 2.00 |
| at `batt_cowl_base_r` 12.6 | 2.07 | **0.0385** |
| at `batt_cowl_base_r` 6.0 | 2.07 | **1.0246** |

**Why nothing caught it.** Four assertions guard this cowl. Three are
one-dimensional — `rse_x_at()` horizontal extents — and the fourth measures the
cavity against the **holder**. Not one of them measures the outer surface
against the cavity, which is the only pair of surfaces that defines a wall. The
`OBSTRUCTION` class added in C-25 has the same blind spot: it asks what lies
*over* a hole, never how thin something got beside one. This is the project's
recurring failure in its purest form — *every defect found has been in geometry
no check interrogated*, and the corner is where one-dimensional checks go blind.

**Why no single corner radius fixes both.** The holder's square corner is at
(38.90, 11.05) and the screw axis at (42.75, 11.65) — 3.896 mm apart. Less the
Ø5.0 head radius, 1.396 mm remains for clearance *and* wall, against the
0.50 + 2.00 = 2.50 the design asks for. Every value of `batt_cowl_base_r` trades
about a millimetre of screw clearance for about a millimetre of corner wall and
the sum never exceeds ~1.1 mm.

**Resolved by decoupling them.** The corner goes back to 6.0, and the two lower
screws get their own Ø5.2 relief bores straight through the cowl — local access
instead of reshaping the whole shell to reach around. The bores live entirely at
z < 0, so nothing in the plate is touched, and they leave a 1.02 mm web to the
cavity.

`validate.py` gained a `COWL` class that reads the wall off the rendered plate
as a **distance** between the outer ring and the cavity ring, at every height —
not as an extent — and re-probes a Ø5.0 driver column to both screws.

**This defect shipped.** It was present in `v1.0` (`6a9ded1`) and in the STLs
issued from it. `v1.1` is the first release without it.

### C-27 — The back plate's stiffening ribs were not there

`backplate()` added three 3.0 mm ribs across the keyboard bay, with a comment
saying they "run in the plate's weakest direction: the long span between the
bottom tongue and the spine". They contributed **nothing**.

`cube(..., center = true)` centres in Z as well as X and Y — the same reading
error as C-14's tongue and groove. At a z-centre of
`back_t + kbd_keeper_t/2 - 0.01` and a height of `kbd_keeper_t` they occupied
z 3.19…3.44, entirely inside the keeper pad at z 3.19…3.45. Sectioning the
rendered plate at z = 3.30 and z = 3.44 gives 6275.2 and 6280.2 mm², identical
to a bare pad, and deleting the ribs changes the part's volume by
**0.000000 mm³**.

Nor could they have worked as written: `kbd_keeper_t` is
`board_depth - kbd_depth` = **0.25 mm**, so any rib standing proud of the pad
would be a fraction of a millimetre tall and would come out of the keyboard's
0.40 mm of float. Deleted rather than rescued. `tools/structure.py` measures
section properties from the mesh, so none of its published figures move.

This one was harmless — it wasted no filament and trapped nothing. It is
recorded because a feature that claims structure it does not provide is how a
design stops being checkable.

### C-25 — The cowl was lying on top of two screws and a window

Every check in the suite asked whether a hole had been **cut**. None asked
whether anything was **lying on it**. Both classes of defect below passed 76
checks, and both are the same mistake: the battery cowl's plan form was copied
from the reference's separate clip-on cover — which sits on a blank panel — and
this cowl is integral to a back plate that is already crowded.

`batt_cowl_w/h` were 83.0 × 30.0 and `batt_cowl_foot` was 3.2. `blend` in
`rse_blob()` is an outward offset of the **whole section**, so the cowl's
footprint **at the panel** was 89.4 × 36.4, not 83 × 30. It covered:

| Feature | Position | What was over it |
|---|---|---|
| lower M2.5 board screws | (±42.75, +0.25) | **1.11 mm** of cowl foot across the whole Ø5.0 countersink — measured 17/17 probes blocked |
| expansion-header window | y 26.50 … 34.70 | **3.60 mm** of its 8.20 mm height, up to **5.84 mm** deep; clear mouth 4.70 mm against a 6.60 mm header body |

Consequence: two of the four screws that carry the board and the cell could not
be inserted at all, and the expansion header could not be mated. Measured by
ray-casting along +Z through the rendered plate, which is the method the fix is
asserted with.

**The geometry is over-constrained and no single number fixes it.** The holder
is 77.80 wide and the countersink rim reaches x = 40.25, leaving 1.35 mm for a
clearance plus a wall that together need at least 1.9 — so the cowl *must*
overlap the screws in plan unless its corner retreats off them. The resolution
is four coupled changes, all now derived rather than chosen:

- **`batt_cowl_w/h` derived** from `batt_bay_w/h + 2*(clear + wall)`:
  82.80 × 27.10 instead of 83.0 × 30.0. The height was the accidental part —
  the cavity had 1.95 mm per side over the holder in V where the design's own
  declared `batt_cowl_clear` is 0.50. It is now 0.50, as declared.
- **`batt_cowl_foot` 3.2 → 0.6.** It was never a tangent fillet: 3.2 mm of
  flare decayed over `foot_f` = 0.20 of a 10 mm rise, i.e. 2.0 mm of height.
  The bound is the expansion window's lower edge.
- **`batt_cowl_base_r` 6.0 → 12.6.** This is what pulls the cowl's lower flank
  off the screws: on an R6 corner the footprint still reaches x = 41.61 at
  y = +0.25, which is 1.36 mm inside the countersink. At 12.6 it reaches 39.36
  and clears the rim by 0.89 mm.
- **`batt_cowl_base_ri` 3.0 → 2.2.** Shrinking the cavity re-opened the
  `board_pocket_r` trap: at 3.0 the holder's square corner at (38.90, 11.05)
  fell 0.98 **outside** the cavity outline. At 2.2 it is 0.88 inside.

`batt_cowl_crown` was wrong at the same time and for an unrelated reason — see
[C-26](#c-26--the-crown-started-a-millimetre-above-the-holder).

New check class **OBSTRUCTION** in `tools/validate.py` now probes every
fastener head footprint and every window in the plate along its access axis.
New check class **STACK** measures the cowl cavity as a *section* at the
holder's deepest plane rather than as a depth.

### C-26 — The crown started a millimetre above the holder

`batt_cowl_crown` was 0.50 with a comment that already stated the right rule —
"the sides must stay parallel until they are clear of it". They did not. The
holder reaches `batt_protrusion - back_t` = 5.00 mm below the panel and the
cavity's rise is 8.00, so 0.50 began the crown at 4.00 mm, a **millimetre too
early**. Measured on the rendered plate the cavity closed to **77.592 mm** at
the holder's deepest plane against a **77.80 mm** holder — 0.104 mm of
interference per side, which is also the 0.17 mm³ that the clash test reported
and passed, because its threshold is 1.0 mm³ for mesh-faceting noise.

The check that covered it was one-dimensional: *"the cell reaches z = −5.00,
the cowl inner face is at z = −8.00, clearance 3.00 mm"*. That is a depth. The
crown closes the **section**, and a depth cannot see a section.

`batt_cowl_crown` is now derived from the holder's own protrusion. Measured
after the fix, the cavity holds 78.800 × 23.100 at every height down to the
holder's deepest plane: +0.500 mm per side in both axes, exactly the declared
`batt_cowl_clear`, and the board-to-plate clash volume went from 0.17 mm³
to 0.00.

### C-21 — Three values whose stated reasons were wrong

All three survived adversarial refutation in the re-derivation pass. Two keep
their value and lose their justification; one moves.

- **`expansion_body_h` 8.603 → 8.700.** 8.603 is the header component's own
  bounding-box height. Waveshare's assembly seats it insulator 8.500 + 0.100
  lead + 0.100 seating below the PCB back plane. **The bbox of a part in
  isolation is not where the assembly puts it** — and the dependent figure,
  how far it stands proud of the standoff plane, was 1.60 and is 1.70.
- **`batt_cowl_base_ri` stayed 3.0, for a different reason** (it is now 2.2 —
  see [C-25](#c-25--the-cowl-was-lying-on-top-of-two-screws-and-a-window)). It was justified
  by "the holder's corners are R2.0". They are not: in the STEP the holder's
  plan form is exactly square at every height through the body — 0.0000 mm
  deviation from its bounding rectangle, 504 of 576 edges straight, every
  circle in the part on the cell axis. The real constraint is that a cavity
  corner must not bite into a square-cornered body inside it.
- **C-11's replica tangency figures.** "6.99 (tangency 6.81 / 6.88)" is a fit
  artefact; on the replica's authored vertex ring at z = 7.000 each flat is a
  single segment, so the tangent points are **exact** and the figure is
  7.000 / 7.000.

### C-20 — There are two keyboard revisions, and the corner is not an arc

**The charge port.** The owner's unit is **Type-C**. The drawing this project
works from labels "USB Charging Port" and draws a mini-USB connector. Both
sheets are otherwise identical — same layout, same `108.5mm/4.3inch`,
`58.2mm/2.3inch`, `10.2mm/0.4inch` callouts, same 68-key field, same model
number 518BT.

So there **are** two revisions, and [C-07](#corrections) was wrong to say the
"revised to USB-C" claim was false — it was right, and the mini-USB drawing is
simply the older sheet. What C-07 got right stands: **the outline did not
change across the revision**, which is the part the geometry depends on. The
Type-C sheet could not be sourced at full resolution; the analysis below uses
the mini-USB sheet, whose body silhouette is the same rendering.

Nothing in the model moves. The 34 × 8 mm service window on the left flank
clears either connector.

**The corner is not a constant radius.** A sub-pixel trace of the rear view
gives a circle-fit radius that depends on how much of the corner is included:

| span | circle R | rms |
|---|---|---|
| ±5 mm | 5.87 | 0.034 |
| ±11 mm | 7.30 | 0.164 |

A true arc does not do that. The four corners agree with **each other** to
0.08 mm, so the moulding is uniform corner to corner — what it is not is
circular. A symmetric superellipse fits the two uncorrupted corners at
n ≈ 1.74, but that sits *below* 2, meaning more cut away than an arc, which is
also exactly what a lit render's shading would produce at 45°.

**So the honest limit: this is a marketing render, not an orthographic
projection.** Its silhouette carries the edge roll and its own shading, and no
amount of curve fitting separates moulding shape from rendering artefact. The
dimension callouts are authoritative; the outline is not.

The band therefore stays set by the two **authored** CAD sources — a Shapr3D
STEP with `CIRCLE` entities of exactly 6.5, and the replica's exact tangency at
7.000 — and the design does not depend on resolving it further: the retaining
lip holds a full 1.00 mm for any real corner from 4.0 to 7.0, 0.98 at 7.3, and
degrades only past 8.0.

### C-12 to C-19 — the measured-hardware pass, and a full re-derivation

The owner put calipers on a real board, and a 24-agent re-derivation went back
to Waveshare's own DXF, STEP and drawing, the FCC exhibits and the reference
meshes: **212 findings, 50 disputed, 14 upheld** after adversarial refutation.

**C-12 · The board screws were getting a counterbore, not a countersink.**
The screws supplied with the board have a sloped head; the back plate was
cutting a flat-bottomed 4.6 mm recess. A countersunk head in a counterbore
lands on the shoulder edge instead of on a cone: it does not seat, it sits
proud, and it wedges the bore. Now ISO 10642, 5.0 × 1.50. The stock screw is
4.90 mm overall, which through a 3.20 mm plate leaves **1.70 mm biting the
standoff — 0.68 × diameter** where 1 × is the usual minimum, so the BOM now
specifies M2.5 × 8 countersunk instead.

**C-13 · Shell fasteners M3 → M2.** On the owner's instruction, and the better
fit for the space: the flank strip is 8.35 mm and an M3 boss at 7.4 nearly
filled it where an M2 boss at 6.6 leaves room. Every `m3_*` name was renamed
`shell_screw_*` rather than left holding M2 values under an M3 name. The trade
is recorded: four M2 screws close a 110 g shell easily but are a weaker
accessory anchor, so DESIGN.md no longer advertises them as rigging points.

**C-14 · The bottom tongue-and-groove did not hold — blocking.**
Both the groove and the tongue are `cube(..., center = true)`, which centres in
**Z** as well as X and Y. `tongue_z` was written as if it were the groove's
base, so the groove sat at z 0.000–1.600 — open to the chassis's outer face,
with no lip beneath it. The tongue was not captured in Z at all and the plate's
bottom edge could simply lift away. Separately `tongue_depth = 3.0` left
**0.20 mm** of bottom wall outboard of the groove, half an extrusion.

Now `tongue_z = back_t/2` (groove 0.800–2.400, 0.8 mm of chassis above and
below) and `tongue_depth = 1.6` (1.6 mm of wall, four extrusions, 1.2 mm of
engagement). Two new checks assert the tongue has chassis material **above and
below** it and that the wall survives; both fail on the old geometry.

**C-15 · The two service windows were not mirrored.** Both sat the same
distance from the spine-side edge, so a keyboard turned end-for-end met solid
wall, and the deck was asymmetric for no reason. The second is now the first
reflected about the bay centreline.

**C-16 · `display_aper_draft` 1.45 → 1.50.** `measure_reference.py` sectioned
0.05 mm below the bezel's outer face. On a 45° draft that inset shrinks the
opening by exactly 0.05 per side, so a 1.50 flare read as 1.45. The tool was
sectioning inside the very feature it was measuring.

**C-17 · The "physically excluded" argument is withdrawn.** C-07 claimed a
109.22 mm body could not enter the reference's 109.200 mm pocket. It does not
follow — 0.02 mm is inside injection tolerance and a pocket is not a gauge. The
unit-conversion argument stands without it.

**C-18 · The battery bay is disputed, and the calipers win.** The STEP says the
holder body is 21.10 along V; the owner measured a real board and confirmed
22.10. Both are recorded, because they may be measuring different things (body
versus footprint including the skirt) and because 22.10 is conservative in
**both** roles it plays — it makes the clearance cut larger and the component
mock larger.

**C-19 · Two values kept, their justifications deleted.** `kbd_keycap_tol` was
justified by a replica that is a featureless envelope, not a measurement of a
keyboard. `kbd_access_w/h` were tagged `[MEASURED]` when they are a chosen
envelope *containing* the reference notch, not a reading of it — retagged
`[DESIGN]`.

**And one the audit got wrong**, recorded because it nearly moved a datum: that
the reference tray's R6.100 sets a hard *upper* bound of 6.1 mm on the
keyboard's corner. It sets a **lower** bound. A rounder body has *more* corner
clearance, not less — its corner retreats further from the box corner than the
pocket's does. Brute-forced to 4.66 mm before anything was changed.

### C-11 — The keyboard corner radius was an artefact of the fitting window

[C-08](#c-08--a-guessed-corner-radius-hid-a-defect-that-a-1-d-check-could-not-see)
replaced a guessed 5.0 mm corner with a "measured" 10.0 mm, band 9.5–11.2, from
two sources. **Both were the same methodological error, and the error was mine.**

A least-squares circle fitted over a window that also contains the straight
edges is no longer fitting an arc, and it inflates with the window size. The
same corner on the replica STL reads:

| window | radius | rms |
|---|---|---|
| ±4 mm | 7.00 | 0.003 |
| ±8 mm | 6.99 | 0.004 |
| ±15 mm | 8.78 | 0.227 |

and on the manufacturer's drawing, 5.87 (rms 0.034) at ±5 mm against 12.46
(rms 0.812) at ±20. I used ±14 to ±16 and reported the result to two decimal
places. **The rms column is the tell, and I was printing it without reading
it** — a fit whose residual grows forty-fold is not measuring the thing you
named.

Re-measured arc-only, four sources converge:

| Source | Corner radius |
|---|---|
| Manufacturer drawing, rear view, arc only | 5.87 – 5.96 |
| Third-party CAD replica STL, arc only | 7.000 (tangency 7.000 / 7.000, exact) |
| A Shapr3D STEP carrying authored `CIRCLE` entities | 6.5 exactly |
| Reference ATA tray R6.100 requires a body of | ≥ 4.66 |

Now **6.5 nominal, band 5.9 – 7.0**. The aperture corner derives from it, so it
moved 10.2 → 6.0, which holds the full 1.00 mm lip across the whole corrected
band **and recovers 58.4 mm² of opening**.

One claim in the audit that produced this correction was itself wrong and is
worth recording beside it: that the reference tray's R6.100 sets a *hard upper
bound* of 6.1 mm on the keyboard's corner. It does the opposite. A rounder body
has **more** corner clearance, not less — its corner retreats further from the
box corner than the pocket's does — so the tray sets a **lower** bound, measured
at 4.66 mm. Verified by brute force before the datum was changed.

### C-10 — Every side button and both microphones were sealed inside the wall

The chassis had **no opening at all** for the three side buttons or the two
microphones, and the control dish removed no material. Sixty-four checks passed.

Two independent orientation faults:

- **`rbox()` extrudes from z = 0 to +t, not centred on z = 0.** After
  `rotate([90,0,0])` that runs *inward* from the translate point, so starting
  the cutter at `top_wall_y - wall/2` reached the inner face at y = 66.35 and
  stopped 1.60 mm short of the outer face at 69.55. Five blind pockets on the
  inside of a solid wall.
- **The dish was outside the part.** `rotate([90,0,0])` maps local +Z to global
  −Y, so an aperture built narrow-at-z=0 widens as it goes *inward* — backwards
  for a dish cut from outside. `mirror([0,0,1])` was used to correct that and
  instead placed the entire cutter at y > `top_wall_y`, clear of the wall,
  removing nothing. It is now `rotate([-90,0,0])` with the narrow end at the
  dish floor.

Ray casting proves it: before the fix, 181 rays fired inward at button height
returned a first hit at y = 69.5500 at **every** x — the top face was dead flat.

**Why nothing caught it.** Every one of the 64 checks measured a dimension.
Not one asked whether a hole was a hole. A new `OPENING` class now fires a ray
at each stated aperture and asserts it crosses **no** surface — and asserts the
wall between the buttons still crosses two, so the test cannot be satisfied by
deleting the wall. Verified against the original geometry: it reports six
failures.

Its first version was wrong twice over, both worth stating: it probed the side
ports at y = 0 when the board bay sits at y = 31.3, reporting two perfectly good
tunnels as sealed; and it sampled the dish only across ±15 mm when the dish
spans ±15.9, so every sample sat on the dish floor and max − min was zero
however deep it was.

### C-09 — Two defects the dimensional checks could not see

Both were found by eye, in a render, after being committed and published — which
is the part worth recording, because this repository's whole argument is that
numerical gating beats inspection.

**The apertures were nearly square-cornered.** `rse_aperture()` called

```
rse_plate(w + 2*flare, h + 2*flare, cr + flare, n, q)
```

into a module declared `(w, h, t, cr, n, q = 16)`. The thickness argument was
missing, so every argument after the second shifted left: `cr+flare` landed in
`t`, `n` landed in `cr`, `q` landed in `n`, and `q` fell back to its default.
The top plate of the loft was extruded **10.8 mm tall with a 2.0 mm corner at
exponent 16** — very nearly square.

The apertures still opened to the correct width and height, so the envelope
checks, the clearance checks, the lip check and the whole reference audit
passed. What it got wrong was the corner: **10.2 mm and 4.2 mm collapsed to
about 1.3 mm by the visible face**, and the cutter was 13.219 mm tall for a
2.42 mm cut.

OpenSCAD cannot warn about this. Omitting a positional argument is legal.

An argument-arity checker was written first and **does not catch it** — the call
passes five positional arguments into five required parameters, so it is
arity-legal; the error is a wrong value in the right slot. That checker was
deleted rather than kept as reassurance. What replaced it is
`tools/test_primitives.py`, which renders each primitive on its own and measures
what it actually produced. Its `rse_aperture` case asserts the cut is exactly as
tall as asked and that the corner **grows** with the flare instead of collapsing.

Its first version then produced a *spurious* failure — `rbox` reading 0.116 mm
under nominal — because the test did not inherit `$fn = 64` from
`parameters.scad` and so measured OpenSCAD's default resolution. The test was
wrong, not the model. It now sets the resolution explicitly and states the
faceting budget (0.0096 mm across at `$fn = 64`) rather than hiding it in a
loose tolerance.

**The acrylic window was an orphan.** `window.stl` was exported as a part,
listed in the BOM and shipped, and it fits nowhere. Its outline is the
*reference's* `plexiglass.dxf`, and the reference's board pocket is 13.0 mm —
board 10.75 + acrylic 2.0 + 0.25 clearance. This design's pocket is 11.0 mm:
board 10.75 plus a 0.25 squeeze on a foam gasket. **Dropping the acrylic is
where 2 mm of this deck's thinness came from**, and the part was simply never
deleted with it. It was also 0.72 mm larger than this pocket in both axes, so it
could not have been fitted even flat.

Removed as a part. The outline stays in `parameters.scad` as provenance — it is
a good measurement of somebody else's component — with an assert that now states
the contradiction out loud instead of letting a future edit re-adopt it. The
display needs no window: it sits 2.65 mm below the outer face behind a 2.4 mm
panel.

**Both checks that should have caught these were one-dimensional.** The panel
check compared `display_aper_w >= display_active_w` — widths and heights, which
only ever measure the flats. The active area of an LCD is square-cornered and
the aperture is not, so the reveal is narrowest at the corners: **0.298 mm
there against 1.00 on the flats**, with the corner bounded above at about 5.0.
That is now measured around the whole ring from the rendered mesh, exactly as
the keyboard lip already was after [C-08](#c-08--a-guessed-corner-radius-hid-a-defect-that-a-1-d-check-could-not-see).

### C-08 — A guessed corner radius hid a defect that a 1-D check could not see

`mock_keyboard()` carried `body_r = 5.0` tagged `[PROVISIONAL]` — a guess, never
measured, and the only description this project had of the keyboard's corners.

The real radius is about **10 mm**. Two independent sources agree:

| Source | Method | Result |
|---|---|---|
| Riitek product drawing, rear view | circle fit to the silhouette, scaled on the printed 108.5 mm | 10.8 – 11.2 mm |
| Third-party CAD replica STL | circle fit by section height | 9.5 – 10.6 mm |

Neither is a dimensioned callout, so it is `[MEASURED]` with a band rather than
`[VENDOR]`. **The band matters in both directions, and they are not the same
direction for every check** — a squarer body is the worst case for getting into
the pocket, a rounder one is the worst case for the lip holding it. Anything
checking a corner has to say which end it is using.

**What the guess was hiding.** At 5.0 mm the keyboard's corner stayed well
inside the aperture and every check passed with a healthy 1.00 mm lip. At the
real 10 mm the corner retreats far enough that the superelliptical aperture
corner — which bulges *toward* the box corner — no longer covered it. The lip
went **negative: −0.84 mm at nominal, −1.34 mm at the top of the band.** Four
open gaps into the pocket, and no retention at any corner.

**Why nothing caught it.** The lip check compared `kbd_aper_w < kbd_pocket_w`
and the same in height. That is a one-dimensional test of a two-dimensional
problem: widths and heights only ever measure the flats, and the lip fails at
the corners. It is replaced by one that walks the whole aperture ring, taken
from the rendered mesh, against the body at the worst end of the corner band.

**A second defect surfaced while measuring it.** Both component pockets were cut
`+ 1` proud instead of `+ 0.01` — the only two epsilons in the file that were not
0.01. That is not an epsilon, it is a millimetre taken off the thickness of the
retaining lip, leaving 1.4 mm of a 2.4 mm panel, and it put the bearing plane
inside the aperture's draft flare where the opening has already widened. Fixed
to 0.01. It also moved the structure: the weakest section improved from 294 to
332 mm³ and mean bending stiffness from 1.97× to 2.07× the reference, because
the material came back exactly at the weakest station.

The lesson is the one this file keeps relearning, in a new costume: a
`[PROVISIONAL]` value that gates a check does not fail loudly. It passes,
against itself.

### C-07 — The "retail body" at 109.22 mm was this project's own round trip

For one day this file carried two rival keyboard bodies: the drawing/manual
figure 108.5 × 58.2 × 10.2, and a "current retail" body at
109.22 × 58.42 × 10.16 taken from riitek.com's `4.3 × 2.3 × 0.4 in`. The fit
mocks were built from the **larger of the two**, on the reasoning that a
correction must never loosen a safety margin.

The reasoning was right. The second body was not real.

The manufacturer's own product drawing prints both units on every axis in a
single string — `108.5mm/4.3inch` — which fixes the direction of the
conversion. 4.3 in is 109.22 mm, so the millimetre figure cannot come from the
inch figure; the inch figure comes from the millimetre one. **109.22 was this
project converting 108.5 to 0.1-inch granularity and back, then treating the
result as evidence.** It is also physically excluded: the reference ATA tray is
a built, working device with a 109.200 mm pocket, and a 109.22 mm body does not
go into it.

Two further claims fell with it. The drawing is the *current* product page and
labels a **mini-USB** charging port, so "revised to USB-C" was false. And the
2011 manual and the current drawing agree to the digit, so whatever explains the
mass discrepancy, it is not a change of outline.

Nothing in the geometry moved: the pocket was stated, not derived, and it clears
the corrected body by more than it cleared the phantom. What changed is that a
fabricated datum is gone, and the worst case is now an *assumed tolerance*,
tagged as an assumption, rather than a number dressed as a vendor figure.

This is the third correction to this one datum — [C-06](#c-06--the-ata-tray-is-not-a-press-fit-and-the-002-mm-agreement-was-luck)
withdrew a press fit, O-01 withdrew a rounding band — and the pattern is worth
naming: **every one came from treating a derived or rounded restatement as an
independent source.** Two numbers agreeing is not corroboration until you know
which of them came first.

### C-06 — The ATA tray is not a press fit, and the 0.02 mm agreement was luck

For most of this project's life D-04 argued that the reference ATA tray's
109.200 mm was *"4.3 in to within 0.02 mm, i.e. a deliberate zero-clearance
press fit"*, and leaned on that agreement as strong corroboration of the
vendor's figure — it was described here as "the single strongest datum in this
report".

With the true body now known to be **108.5 mm**, that tray is a **+0.70 mm
clearance fit** and the 0.02 mm agreement was coincidence between two rounded
numbers: 4.3 in converted to mm happens to land near a pocket that was sized
with ordinary clearance over a different figure.

Nothing in the geometry changes — the pocket was already generous enough — but
the *reasoning* was wrong, and it was wrong in the most seductive way: a
suspiciously exact agreement that felt like evidence. Recorded because the
lesson generalises. An agreement to two decimal places between a rounded figure
and a measured one is not corroboration; it is two numbers that happen to be
close.

### C-05 — Corner radii that bite

Twice, a generously rounded pocket corner left material exactly where a
component's much sharper corner needed to be:

- the board pocket's R3.0 corners fouled the PCB's R0.50 corners by 0.33 mm;
- the battery cowl cavity's R8 corners fouled the holder's R2.0 corners by
  1.39 mm.

Both radii are now explicit parameters, bounded and asserted. The general rule:
with `c` mm of per-side clearance around a component of corner radius `r`, a
pocket corner radius much above `r + c/(√2 − 1)` will interfere.

---

## Open items

Gaps that remain. Each is stated with what it would take to close it, and with
how the design is shaped so the gap cannot cause a clash.

### Measured once, not re-audited

Every keyboard-facing dimension in this file is re-derived from the reference
meshes by `tools/audit_reference.py` on each run — **except the service
window** (`kbd_access_*`). The reference tray is open to the exterior along the
same short edge that carries the notch, so the notch cannot be separated
reliably from the surrounding opening; an extraction that looked like it worked
returned the 37–43 mm open edge rather than the 34 mm window.

It is recorded here as measured-once rather than counted among the audited rows,
because a datum that silently stops being checked is how the other corrections
in this file happened.

It gates nothing: the manufacturer's drawing confirms the charge port and the
power switch share one short edge, and the 34 × 8 mm window on the **left**
flank clears either connector type — the owner's Type-C unit and the mini-USB
one on the drawing alike. (This design cut a window on both flanks until
[C-22](#corrections); the keys only read one way up, so the second window
reached nothing and the mirrored pair put the useful one off the features.)

### O-01 — Keyboard outline — CLOSED. Its tolerance is not.

The outline is settled by two independent manufacturer documents twelve years
apart; see [D-04](#d-04--keyboard-outline). **108.5 × 58.2 × 10.2 mm.**

What remains open is narrower. Neither document states a **moulding tolerance**,
and neither says whether the 10.2 mm thickness is measured to the moulding's top
face or to the keycap crowns. A third-party CAD replica of this keyboard
measures 10.600 mm overall, which is where the assumed +0.40 mm band comes from.

The 75 g / 64.8 g mass discrepancy between the 2011 sample and current retail is
real but does not touch the outline. The charge port **does** differ between
revisions — the owner's unit is Type-C, the drawing on file is mini-USB — see
[C-20](#corrections). It drives no geometry: the service window is 34 × 8 mm on
the left flank and clears either connector.

*Mitigation*: the pocket clears an assumed ±0.30 mm in plane and +0.40 mm on
thickness, and the 34 × 8 mm service window clears either connector type, so
the port revision does not drive geometry.
*To close*: calipers. **Thickness first** — it is the only axis with no slack
left against its assertion.

### O-02 — Keyboard charging-port connector type

The FCC-certified body (2011) is micro-USB. One review claims current production
is USB-C. Riitek says only "USB Charging Cable".

*Mitigation*: irrelevant to this design. Both reference enclosures, and this
one, cut a 34 × 8 mm access window rather than a close-fitting port hole, so the
connector type does not drive geometry.

### O-03 — Which switch is PWR, which is BOOT, which is KEY

The STEP designators are KEY4 (U 36.25), KEY3 (U 46.25), KEY1 (U 56.25), but the
function-to-designator mapping is not stated in the STEP, the drawing or the
wiki text.

*Mitigation*: all three apertures are identical and the button sprue is
symmetric, so the enclosure is indifferent. Only the label in
[ASSEMBLY.md](ASSEMBLY.md) is affected.
*To close*: read the official schematic PDF.

### O-04 — Edge-feature heights vs the reference — RESOLVED

*Recorded because it was an open item for most of this design's life, and
because the resolution is the useful part.*

Waveshare places the button centres at W = −0.70 and the microphones at
W = −0.50, i.e. 4.45 mm and 4.25 mm below the display glass front. Measuring the
same apertures in `stl/ata/Caseback.stl` and referencing them to that design's
bezel underside gives 6.9 mm and 6.55 mm — a consistent ≈2.4 mm offset on both
features, which ruled out a one-off error but left the cause unknown.

**Cause found.** The reference's board pocket is **13.000 mm deep** while the
board's actual stack — display glass front to standoff seating plane — is
**10.75 mm**. Its pocket carries about 2.25 mm of slack, and its apertures are
cut for a board resting on the pocket *floor*. This design locates the board
against the *front lip* instead, so the two are measuring from opposite ends of
a stack with slack in it. The offset is 2.25 mm; the discrepancy was ≈2.4 mm.

A second measurement confirms the reference also discards Waveshare's stand
base: its pocket is 71.1163 × 94.5193 against a bare PCB of 69.1098 × 92.5098,
which is **1.0032 / 1.0047 mm per side — symmetric**. Against the 70.10 mm stand
base the clearance would be an implausibly lopsided 1.01 / 0.508.

*Resolution*: the factory position is used, referenced to the display glass
front, which is the plane this enclosure locates against. The apertures are
5.4 × 4.4 mm around a 4.553 × 2.203 mm switch body, absorbing about ±1.1 mm
either way regardless.

### O-07 — Board mounting-hole diameter, two readings

Two independent passes over Waveshare's CAD package disagree: the STEP `BOARD`
solid yields Ø4.20 through-holes (24 CIRCLE instances at radius 2.1000), while
the DXF front view reads as a Ø2.10 hole inside a Ø4.70 pad.

*Mitigation*: it does not matter here. This enclosure puts nothing in those
holes — the M2.5 screws thread into the SMTSO-M2.5-7ET standoffs, and the back
plate's own clearance is Ø2.70. Both readings are consistent with M2.5.

### O-05 — PCB outline tolerance

The drawing gives explicit tolerances only for the TFT (±0.10) and the active
area (±0.10). `92.5 PCB OD` and `69.1 PCB OD` carry none. Ordinary PCB routing
tolerance is about ±0.13 to ±0.2 mm.

*Mitigation*: the pocket carries 0.50 mm per side, comfortably more than routing
tolerance.

### O-06 — Keyboard corner radius

Modelled as R5.0 `[PROVISIONAL]` for the fit-check mock only. Inferred as ≈6.6
from the ATA pocket's base fillet, but that fillet is the ATA designer's choice
and the proof-of-concept has none, so the two references disagree.

*Mitigation*: the pocket corner radius is 6.0 and the pocket is 0.49 mm per side
larger than the keyboard, so a smaller real radius only adds clearance.

---

### O-08 — The button sprue did not fit behind the wall — CLOSED

Measured inward from the chassis's top outer face, on the rendered parts:

| Plane | Distance in | Space behind the wall |
|---|---|---|
| outer face | 0.00 | |
| control-dish floor | 0.90 | |
| top wall, inner face | 3.20 | 0.00 |
| PCB top edge (pocket clearance 0.50/side) | 3.70 | **0.50** |
| switch actuator face (0.19 inside the PCB edge) | 3.89 | **0.69** |

A flange behind the wall has to live in that 0.50 mm, and something has to
reach 0.69 mm to press the switch. **Both cannot be satisfied at once**, and
the sprue failed it from both directions before it was fixed:

- **Too long.** The original was 6.59 mm overall — cap 3.80, flange 1.20, post
  1.60 — needing 3.39 mm behind the wall. Seated with its flange on the wall
  the flange ran 0.70 mm into the PCB's edge and the post a further 1.60 mm
  through it; seated with its post on the switch, all three caps stood 2.70 mm
  proud and held PWR, BOOT and KEY permanently pressed.
- **Too short.** Cut back to cap + a 0.40 mm flange it fitted, with its deepest
  feature 0.35 mm behind the wall — and **0.34 mm short of the actuator**. The
  caps rattled and never reached the switches.

The 0.40 mm flange is measured from the reference enclosure, whose board pocket
carries **1.00 mm per side**. This one carries 0.50. The retention scheme did
not come across with the number.

**Resolved by moving the flange inside the wall.** A 1.00 mm counterbore behind
the apertures — one continuous slot, so the sprue's connecting webs recess with
the flanges — takes the flange, so at rest nothing protrudes past the wall's
inner face and the whole 0.50 mm is left for the plunger. The flange is 0.40
thick, so the counterbore also supplies **0.60 mm of free travel**, bought from
the wall's own thickness instead of from the board's clearance. The plunger is
offset in Y to −0.20 on the button axis, spanning −1.00 to +0.60 inside the
switch body's ±1.10 band, so it lands on the switch and misses the PCB's edge
entirely.

**And then the depth was still wrong, for a third reason.** 0.690 mm is where
the actuator is if the board is exactly where the drawing says. It is not: the
board is located by four M2.5 screws in ISO 273 close-fit 2.7 mm holes, so it
floats ±0.10 mm and the actuator is anywhere in **0.590 to 0.790**. A plunger
cut to 0.690 would hold a switch permanently pressed on any build that floated
toward the wall. The plunger is therefore cut to **0.540** — the near end of
the band, less a 0.05 mm margin — so the gap at rest is 0.05 to 0.25 mm and is
never zero, and the 0.60 mm of travel crosses the worst gap and still delivers
the switch's 0.25 mm throw with 0.10 mm to spare.

That last error was found by a different instrument than the ones that found
the first two: putting the sprue where the assembly puts it and intersecting it
with the chassis as a solid. The depth arithmetic was self-consistent and still
wrong, because it had no opinion about the board's mounting float. `validate.py`
now runs that boolean at rest, mid-travel and fully pressed.

**Why neither version was caught for so long.** `buttons` is a separate part,
exported laid flat for printing, and it was never placed in assembly space by
a fit check — only its bounding box was measured, and a bounding box was never
going to say whether 6.59 mm of anything can exist in 3.89 mm of space. The
`STACK` class now measures the sprue against the two planes above.

### O-09 — The keyboard lip goes negative at a corner — CLOSED

`_corner_lip()` measured the front-face lip with the keyboard **centred**.
Nothing centred it. With the body at its low tolerance (108.20 × 57.90) pushed
hard into a corner of what was then a 110.20 × 59.40 pocket — ±1.00 in X and
±0.75 in Y — the narrowest lip was **−0.361 mm** at that corner against
**+0.985 mm** centred. You would have seen a sliver into the pocket past one
corner. The keyboard was never at risk of escaping: every edge and the other
three corners held ≥ 1 mm against a 106.5 × 55.8 aperture.

Closed by taking the play out rather than by moving the aperture: the pocket is
now 109.85 × 59.15 with locating ribs, and the same worst case measures
**+0.368 mm**. `tools/validate.py` asserts it from the rendered chassis over
the body's whole tolerance band *and* every position the pocket allows.

This was trap 5 from the project's own list — *a part's bounding box in
isolation is not where the assembly puts it* — applied to the header
(`expansion_body_h`, C-21) and not to the keyboard.

---

## Measure these before a final print

The design is asserted consistent, not proven against hardware. Nobody has held
these two parts against a printed chassis. Before committing filament to a final
build, put calipers on:

1. **Keyboard THICKNESS** — O-01. The outline is settled, but 11.0 mm of pocket
   sits exactly on its assertion against a 10.6 mm upper band, and the drawing
   does not say whether its 10.2 mm includes the keycaps.
2. **PCB outline and the standoff seating plane** — confirms D-02 and D-03 and
   the decision to discard the stand base.
3. **18650 holder protrusion past the standoff plane** — sets the cowl rise.
4. **Button and microphone heights below the display glass front** — O-04 is
   resolved on paper but rests on a chain of three inferences; it is cheap to
   confirm with the board in hand.

`tools/validate.py` proves internal consistency. It cannot prove that the datums
match reality; only calipers can do that.
