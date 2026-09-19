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

**110.2 × 59.4 × 11.0 mm** `[DESIGN]` — stated, not derived from the body plus a
clearance, so the envelope does not move every time a tolerance assumption is
revisited. What it must satisfy is asserted instead.

| Clears | per side, in plane | depth |
|---|---|---|
| nominal body 108.5 × 58.2 × 10.2 | 0.85 / 0.60 | +0.80 |
| body at tolerance 108.8 × 58.5 × 10.6 | 0.70 / 0.45 | +0.40 |
| ATA tray, a working device, 109.200 × 59.200 | 0.50 / 0.10 | — |
| grip case, measured, 109.406 × 59.005 | 0.40 / 0.20 | — |

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

### O-01 — Keyboard outline — CLOSED. Its tolerance is not.

The outline is settled by two independent manufacturer documents twelve years
apart; see [D-04](#d-04--keyboard-outline). **108.5 × 58.2 × 10.2 mm.**

What remains open is narrower. Neither document states a **moulding tolerance**,
and neither says whether the 10.2 mm thickness is measured to the moulding's top
face or to the keycap crowns. A third-party CAD replica of this keyboard
measures 10.600 mm overall, which is where the assumed +0.40 mm band comes from.

The 75 g / 64.8 g mass discrepancy between the 2011 sample and current retail is
real but does not touch the outline, and the charge port is mini-USB in both
(an earlier note here claiming USB-C was wrong — [C-07](#corrections)).

*Mitigation*: the pocket clears an assumed ±0.30 mm in plane and +0.40 mm on
thickness, and the service window is 34 × 8 mm on **both** short edges, so the
port edge does not drive geometry.
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
