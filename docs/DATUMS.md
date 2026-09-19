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

**109.22 × 58.42 × 10.16 mm**, 64.8 g. `[VENDOR]` S3 (`4.3 × 2.3 × 0.4 in`,
converted exactly).

Corroborated four ways:

| Line of evidence | Value | Implied clearance |
|---|---|---|
| Manufacturer (S3) | 109.22 × 58.42 × 10.16 | — |
| `stl/ata/Caseback.stl` tray | 109.200 × 59.200 × 11.400 | **−0.02** / +0.78 / +1.24 |
| `stl/poc_top.stl` bay | 110.498 × 60.600 × 10.750 | +1.28 / +2.18 / +0.59 |
| Official product-photo silhouette ratio | 1.8531 | brackets 4.3/2.3 = 1.8696 |

The ATA tray is **109.200 mm — 4.3 in to within 0.02 mm**, i.e. a deliberate
zero-clearance press fit, and its own build guide hedges with "if the print
tolerance permits it". That independently validates the manufacturer's figure as
a real dimension rather than marketing rounding, and it is why this design does
**not** reuse 109.200: see [D-05](#d-05--keyboard-pocket).

Traps, all confirmed and all avoided:

- **150 × 100 × 20 mm / 120 g is the retail box**, repeated by many marketplace
  listings as if it were the product.
- **The Rii K18 is not this keyboard.** It is 325 × 122 × 18.3 mm with a
  trackpad — three times the length.
- **Retailer titles containing "Touchpad" are wrong for the 518BT.** It has no
  touchpad; mouse control is Fn + key. No pocket area is allocated for one.
- A figure of `4.09 × 2.28 × 0.43 in` circulates on review sites. It has no
  traceable primary source, contradicts the manufacturer, and contradicts the
  ATA tray by 5.3 mm. Rejected.

## D-05 — Keyboard pocket

**110.2 × 59.4 × 11.0 mm** `[DESIGN]`, sitting deliberately between the two
proven designs: 0.49 mm per side in plane, where the ATA is a press fit and the
proof-of-concept rattles enough that its build guide tells you to shim it with
tape.

Because this design's pocket is open front-to-back — the keyboard loads from
behind and is held forward by a pad on the back plate — there is no internal
floor-to-wall corner for a print fillet to interfere with, so the base relief
the ATA needs (a 6.62 mm fillet, to clear the keyboard's strongly radiused
underside) is not required here.

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
| Aperture used here | 86.8 × 65.6 | `[MEASURED]` S5 — a 1.00 mm reveal per side |

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

### O-01 — Keyboard dimensions are quoted only to 0.1 in

`riitek.com` publishes millimetre figures for most of its models but **inches
only** for the 518BT, rounded to 0.1 in = 2.54 mm. No dimensioned drawing, CAD
file or manual exists in Riitek's download centre, and FCC test report
MOST110422F1 contains no EUT dimensions (verified by full-text search).

*Mitigation*: the pocket is set from the intersection of the vendor figure and
two measured reference pockets, not from the vendor figure alone.
*To close*: calipers on a physical keyboard, or the GrabCAD model
`rii-bluetooth-mini-keyboard-518bt-1`, which requires authentication to download.

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

### O-04 — Edge-feature heights disagree with the reference by ~2.5 mm

Waveshare places the button centres at W = −0.70 and the microphones at
W = −0.50, i.e. 4.45 mm and 4.25 mm below the display glass front. Measuring the
same apertures in `stl/ata/Caseback.stl` and referencing them to that design's
bezel underside gives 6.9 mm and 6.55 mm — a consistent ≈2.5 mm offset on both
features.

The offset being *consistent across two independent features* implies a
systematic cause, most likely that the reference's board sits deeper in its
pocket than assumed because it retains the 2.75 mm stand base.

*Resolution*: the factory position is used, because it is where the switch
physically is and because this design does not retain the stand base. The
apertures are cut 5.4 × 4.4 mm around a 4.553 × 2.203 mm switch body, which
absorbs about ±1.1 mm of error in either direction.
*To close*: calipers on a physical board. **This is the first thing to check on
a test print.**

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

1. **Button and microphone heights below the display glass front** — O-04, the
   one place where two sources genuinely disagree.
2. **Keyboard outline**, all three axes — O-01, quoted only to 0.1 in.
3. **PCB outline and the standoff seating plane** — confirms D-02 and D-03 and
   the decision to discard the stand base.
4. **18650 holder protrusion past the standoff plane** — sets the cowl rise.

`tools/validate.py` proves internal consistency. It cannot prove that the datums
match reality; only calipers can do that.
