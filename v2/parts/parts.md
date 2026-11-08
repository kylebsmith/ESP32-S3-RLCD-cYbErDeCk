# v2 parts, dimensions and tolerances

*An enclosure input. Every number below carries its source tag. Where a number
could not be traced to a primary source it is marked **NOT SOURCED** and listed
in §10 as a caliper job — it is not estimated.*

---

## 0.0 Status — which device this is about, and what changed on 2026-11-08

**This document is about v2, the live-coding instrument in `v2/`.** The
repository root — `cad/`, `tools/`, `export/`, `docs/`, `firmware/` — is **v1,
and v1 is still a real device**: the original cYbErDeCk, less built out, with
its own parameters, its own check suite and its own 2.40 mm printed front wall.
Nothing here retires v1. Where a claim below is about one device and not the
other it now says so, because several of them were written against v1's thin
front panel and read as live advice for v2.

**This file predates the control surface v2 actually has.** The research is all
kept: a survey of a part that was considered and rejected is still the record of
*why* it was rejected, and deleting it would lose the argument. What has changed
is that every **recommendation** for a part the design dropped is struck, dated
and moved under a "considered and not chosen" heading, so nothing here can be
mistaken for the current choice. Fixed in this pass: §0 items 2, 3, 4, 7; §2.2,
§2.3, §2.4; §3 (count, travel, cutout); §4; §5.1–§5.5; §8.1–§8.4; §10; §11.

**What v2 carries, measured off the exported artefacts rather than off the
source that made them** (procedure in §12):

| In the v2 build | Qty | Evidence |
|---|---|---|
| Rotary encoder **5880**, individual breakouts | **3** | three 20.320 × 20.320 M2 boss quads at x = −1.905 / 36.195 / 74.295, all at y = 68.0125 `[CAD-MESH]` |
| **NeoSlider 5295** | **1** | one 38.100 × 16.510 boss quad at (126.365, 48.9625) and one 12.000 × 52.000 slot — **one of each, not two** `[CAD-MESH; DXF]` |
| Nut-mount pot, 10 kΩ linear taper | **3** | three 24.130 × 31.750 carrier webs, each with a Ø7.200 bore, at y = 29.9125 `[CAD-MESH]` |
| Printed knob | **6** | `knob_encoder.stl` ×3 and `knob_pot.stl` ×3 — 19.050 tall, 25.247 across the fluted brim `[CAD-MESH]` |
| Printed fader cap | **1** | `fader_cap.stl` — a 10.000 square plug under a 16.000 × 25.400 grip `[CAD-MESH]` |
| **NOT in the build** | **0** | NeoKey 1×4 **4980** (§4) · Quad Rotary **5752** (§2.4) · ADS1115 **1085** (§8.4) · Adafruit knobs **5527/5528/5530** (§5.5) · slide-pot nubbin **5621** (§3) |

Two further things this file is silent about and a reader should not infer from
its silence: the keyboard is a **stripped Keychron K9 Pro**, not a key breakout
(`v2/parts/keyboard-strip.md`), and the I²C bus is **four devices**, not nine
(`v2/parts/wiring.md` §2.3).

---

## 0. Verdict up front

1. **"Neo-Encoder" resolves to two PIDs, and one of the user's is wrong.**
   **4991** is the breakout *with NeoPixel, no encoder fitted*; **5880** is the
   same board *with a Bourns PEC11 encoder pre-soldered* (it has the NeoPixel
   too). **5881 is not an encoder — it is a pack of 100 anti-static bags.**
   `[AF prod-5881]` For ×3, buy **5880** unless you want a specific encoder feel.
   **Still current, and checked again in this pass.** `v2/parts/knobs.md` §0
   item 1 reaches the same place from the other end — *"Do not buy 4991 boards,
   do not buy 25 mm PEC11s, do not hand-solder three encoders"* `[REPO]` — and
   the carrier is drawn for three of them (§0.0).

2. **Three encoders on one bus do not collide. The pitch argument that
   followed was read the wrong way round, and it is retired.** The address
   jumpers give eight addresses (0x36–0x3D) `[AF pinouts]`, so three is
   trivial — that half stands and `v2/parts/wiring.md` §2.3 implements it.

   > ~~"The real constraint is that each 5880 is a **25.4 mm square board** whose
   > knob must land on a 19 mm-ish musical pitch. If the three rotaries sit in a
   > row, **5752 Quad Rotary** (four encoders on one 76.2 × 21.59 mm board, one
   > I2C address, 19.05 mm pitch) is the better buy."~~

   **Superseded 2026-11-08, with the measurement.** The knob never had to land
   on a 19 mm pitch; that premise came from the MX key unit, which the control
   band does not use. The band is laid out on **2 × U = 38.100** — the centre
   spacing of all six bores in `slab.dxf` `[DXF]` and of the three encoder boss
   quads in `carrier.stl` `[CAD-MESH]`. At 38.100 three 25.400 mm square boards
   leave **12.700 mm of clear air between neighbours**, and the carrier's own
   board pockets, 26.670 square on a 38.100 pitch, leave **11.430 mm of web**
   `[DERIVED from CAD-MESH]`. So the 25.4 mm board was never the binding
   constraint. The Quad Rotary's *fixed* 19.050 mm pitch is the constraint, and
   it bites the knob: `v2/cad/knob.scad`'s header records the trade in its own
   words — *"hence 25.400 rather than the 13.000 the Quad Rotary's 19.050 pitch
   would have forced. Individual encoders on a 38.100 pitch are what bought the
   diameter."* `[REPO]` The knob that exists measures **25.247 across the
   flutes, Ø25.400 at the intact rim** `[CAD-MESH knob_encoder.stl]`.

   **Current choice: three individual 5880s.** §2.4 keeps the Quad Rotary survey
   as the record of what was compared, no longer as a recommendation.

3. **Two PCB outlines carry the whole Adafruit range — and the v2 build uses
   one bay of the first and three of the second.** The NeoSlider (5295), NeoKey
   (4980) and Quad Rotary (5752) are **all 76.200 × 21.590 mm with the same
   38.10 × 16.510 mm, Ø2.50 mm hole pattern** `[STEP ×3]`; the sensors, ADC and
   single encoders are all on the 25.40 mm-wide breakout grid. That survey is
   unchanged and still useful. **What has changed is the consequence.** The
   carrier does not carry an interchangeable rack of Pattern-B bays: it carries
   **one** 38.100 × 16.510 quad, rotated 90°, for the NeoSlider, and **three**
   20.320 × 20.320 quads for the individual encoders `[CAD-MESH]`. The NeoKey
   and the Quad Rotary — the other two Pattern-B parts — are **not in the
   build** (§4, §2.4), so the "swap bands without re-cutting bosses"
   interchangeability below is a property of Adafruit's range, not an option
   this deck still has.

4. **NOTHING PANEL-MOUNTS, so the long-bushing hunt is off. The pot's nut
   lands on a 3.810 mm web in the printed carrier, and a stock bushing is
   enough.**

   > ~~"A stock 16 mm Alpha bushing is M7 × 0.75 × **6.5 mm**, which allows a
   > panel of only ≈4.0 mm … The 10 mm option (suffix **B6** or **E6**) permits
   > a ≈7.5 mm panel — that is the part that makes a recessed pocket possible
   > **without a counterbore**. With anything else, a recess deeper than ~1.5 mm
   > **must** be a counterbore."~~

   **Superseded 2026-11-08.** The arithmetic was right and the question was
   wrong: v2 does not mount the pot to the deck surface at all. The 12.700 mm
   acrylic is not a panel the pot could be nutted to, and it was never going to
   be. What carries the pot is a **web in the 3D-printed carrier**, measured in
   the exported part as **material from z 5.085 to 8.885 — 3.800 thick** on a
   0.005 mm sampling step, i.e. the drawn 3.810, inside a 24.130 × 31.750 collar
   with a **Ø7.200** bore through it `[CAD-MESH carrier.stl]`, three of them at
   a 38.100 pitch. Against the 562's **6.600 mm** bushing — measured
   independently here as a Ø6.2846 thread section running z 2.000 to 8.600
   `[HOUSE-MESH]` — a 3.810 web plus a 2.000 nut is **5.810**, leaving 0.790 for
   the washer and the thread's bite. **A stock M7 × 0.75 × 6.5 bushing is
   sufficient and the Alpha RD901F's 10 mm option buys nothing here.** §5 keeps
   the RD901F survey as an alternative, not as the recommendation.

5. **IMU: BNO085 (4754).** It is the only one of the four that does the fusion
   on-chip and hands you a quaternion. §6. **Status: the comparison stands, the
   part is not placed.** The v2 carrier has **no boss quad and no pocket for
   it** — the only breakout quads in `carrier.stl` are the three encoder
   patterns, the one NeoSlider pattern and the Feather's two M2.5 bosses
   `[CAD-MESH]`. An IMU needs no aperture, so it could still be added on the
   wiring plane; it is **not budgeted in the current geometry** and §8.3's
   address map counts it as though it were.

6. **ToF: VL53L4CD (5396).** 18° FoV, 1 mm short-range linearity, 100 Hz —
   "accurate and constrained" is literally its spec sheet against the others. §7.
   **Status: the comparison stands, the part is not placed, and unlike the IMU it
   cannot be added silently** — a ToF has to see out. `slab.dxf` carries
   **sixteen loops and no sensor window**: the outline, the keyboard void, six
   Ø22.000 bores, the fader slot, six fastener holes and one fiducial `[DXF]`.
   Cutting one is a slab change, not a wiring change.

7. ~~**No I2C multiplexer is needed.** Nine devices, all unique, map cleanly.~~
   **The conclusion survives; the premise does not.** No multiplexer is needed —
   but not because nine devices map cleanly. **The bus is four devices**: three
   5880s and one NeoSlider, at 0x36/0x37/0x38/0x30, two reversible solder
   jumpers, no knife work. `v2/parts/wiring.md` §2.3 is the current map and it
   supersedes §8.3 here. §8 is kept as the nine-device survey it was.

---

## 0.1 How these numbers were produced

Source tags used throughout:

| Tag | Meaning |
|---|---|
| `[STEP]` | **Measured** by me from the vendor STEP file with an OpenCASCADE kernel — per-solid optimal bounding boxes and cylindrical-face radii. Highest confidence. |
| `[FAB]` | Adafruit fab print (vendor board drawing), read directly. |
| `[DS]` | Manufacturer datasheet PDF. |
| `[AF]` | Adafruit product page or Learn guide text. |
| `[DXF]` | **Measured by me** off `v2/export/dxf/slab.dxf`, the emitted cut file. OpenSCAD writes circles as polylines, so a diameter is a 96-gon's extent: read as ±0.01, never as analytic. |
| `[CAD-MESH]` | **Measured by me** off an exported mesh in `v2/export/stl/`, by plane sections and by inside/outside sampling at a 0.005 mm step. Carries ±0.005 from the sampling. |
| `[HOUSE-MESH]` | **Measured by me** off the house vendor mesh in `/home/user/kylebsmith/cmtl-3d-assets/02_controls/` — the same files `v2/cad/parameters.scad` §7 cites. |
| `[REPO]` | A value or a statement read out of this repository's own source, named by file. |

The STEP measurements are reproducible: the models are listed in §11, and the
measuring script is not committed (it is a throwaway) but the method is just
`BRepBndLib::AddOptimal` per solid plus a cylinder census per face.

**Frame convention.** For each board I call the face the user touches the
**front** and the face carrying the STEMMA QT connectors the **back**. Vendor
models differ in which way is +Z; all numbers below are restated in this
front/back frame, not the model's raw Z.

**One correction worth recording,** because it nearly went in wrong: the
Adafruit render of the 5880 appears to show the encoder square-on to the board.
It is not. Every planar face of the encoder body has a normal off all three
axes, and the faces span equal X and Y ranges — the encoder is **rotated 45°**
to the PCB. The render is isometric, so a 45°-rotated part looks square-on
while the board looks like a diamond. The 4991 fab print corroborates it: the
encoder pad rows run diagonally. `[STEP 5880 face normals; FAB 4991]`

---

## 1. Master table

Dimensions in mm. "Above / below PCB" is measured from the PCB faces, PCB
excluded. All these boards are **1.570 mm thick** `[STEP, every board measured]`.

| Part | PID | PCB outline | Mount holes | Front (above) | Back (below) | I2C |
|---|---|---|---|---|---|---|
| QT Rotary Encoder, no encoder | 4991 | 25.400 × 25.400 | 4 × Ø2.50 on 20.32 × 20.32 | — (you fit it) | 2.96 | 0x36 |
| QT Rotary Encoder + encoder | 5880 | 25.400 × 25.400 | 4 × Ø2.50 on 20.32 × 20.32 | **21.40** to shaft tip | 2.96 | 0x36 |
| Quad Rotary ("NeoRotary 4") | 5752 | 76.200 × 21.590 | 4 × Ø2.50 on 38.10 × **16.510** | **21.40** (encoders not supplied) | **4.30** (right-angle QT) | 0x49 |
| NeoSlider | 5295 | 76.200 × 21.590 | 4 × Ø2.50 on 38.10 × **16.510** | **15.50** to shank tip | 2.96 | 0x30 |
| NeoKey 1×4 QT | 4980 | 76.200 × 21.590 | 4 × Ø2.50 on 38.10 × 16.51 | 0 bare; **11.6** w/ MX `[DS]` | 2.96 | 0x30 |
| BNO085 IMU | 4754 | 25.400 × 22.860 | 4 × Ø2.50 on 20.32 × 17.78 | 0 | 2.96 | 0x4A |
| LSM6DSOX + LIS3MDL | 4517 | 25.400 × 17.780 | 4 × Ø2.50 on 20.32 × 12.70 | 0 | 2.96 | 0x6A + 0x1C |
| VL53L4CD ToF | 5396 | 25.400 × 17.780 | 4 × Ø2.50 on 20.32 × 12.70 | 0 | 2.96 | 0x29 |
| VL53L1X ToF | 3967 | 25.400 × 17.780 | 4 × Ø2.50 on 20.32 × 12.70 | 0 | 2.96 | 0x29 |
| VL6180X ToF | 3316 | 25.400 × 17.780 | 4 × Ø2.50 on 20.32 × 12.70 | 0 | 2.96 | 0x29 |
| ADS1115 ADC | 1085 | 25.400 × 17.780 | 4 × Ø2.50 on 20.32 × 12.70 | 0 | 2.96 | 0x48 |

All `[STEP]`. Every board has a **2.54 mm corner radius** and every mounting
hole sits **2.54 mm in from both adjacent edges** `[STEP]`.

**The 16.52 / 16.51 wobble is settled at 16.510.** This table used to give the
Pattern-B hole span as 16.52 on two rows and 16.51 on a third — the same hole
pattern, written two ways, from two STEP reads that rounded differently. Three
things agree on **16.510**: the fab print's **0.650 in** `[FAB]`, §1.1's own
reading, and the only Pattern-B boss quad that actually exists, whose four M2
insert bores sit at x 118.110 and 134.620 — a span of **16.510** — and y 29.9125
and 68.0125, a span of 38.100 `[CAD-MESH carrier.stl]`. `v2/cad/parameters.scad`
carries `pat_b_holes = [38.100, 16.510]` `[REPO]`. The 0.010 is recorded rather
than hidden: it is a STEP-read rounding, not a vendor tolerance.

**Which rows are in the v2 build.** Rows 5880 (×3) and 5295 (×1) are in it. Row
562 (§5.2) is the pot class it is drawn around. Rows 4991, 5752, 4980, 4754,
4517, 5396, 3967, 3316 and 1085 are **engineering record**: measured, kept, and
not bought for this device — see §0.0. They are still the right numbers if any
of them is ever wanted.

### 1.1 The two repeating patterns

**Pattern A — "Adafruit 1.0 inch breakout".** 25.400 mm wide, 1.570 thick,
4 × Ø2.50 holes inset 2.54 from each edge, R2.54 corners, one 0.1″ header row
of Ø1.00 holes at 2.54 pitch along one long edge, and **two JST-SH 4-pin
STEMMA QT sockets on the back face at the two short ends**, body
**4.95 (along board) × 6.00 (across) × 2.96 (tall)**, centred on the board's
long centreline, openings facing outward along the long axis. Depth varies:
17.780 (sensors, ADC), 20.320 (PCA9546), 22.860 (BNO085), 25.400 (encoder).
`[STEP, measured identically on 4517 / 5396 / 3967 / 3316 / 1085]`

**Pattern B — "3 × 0.85 inch control strip".** 76.200 × 21.590 × 1.570,
4 × Ø2.50 holes on **38.10 × 16.51**, R2.54 corners. Used by 5295, 4980 and
5752. `[STEP ×3, corroborated by FAB: 3.00 × 0.85 in, 1.50 × 0.65 in]`

**Pattern B — "3 × 0.85 inch control strip" — correction to the span.** The
hole pattern is **38.100 × 16.510**, as this subsection already had it; the
master table's two 16.52 entries are brought to it above.

~~**Design consequence:** one 38.10 × 16.51 boss quad accepts the slider, the key
strip *and* the quad encoder interchangeably. Lay the deck out on that module
and the arrangement study in `v2/docs/envelope.md` can swap bands without
re-cutting bosses.~~

**Superseded as a design consequence, 2026-11-08 — true of Adafruit's range,
not of this deck.** Interchangeability only pays if there is more than one
candidate for the bay, and there is not: the NeoKey is out of the design (§4)
and the Quad Rotary was not chosen (§2.4), which leaves the NeoSlider as the
only Pattern-B part in the build. The carrier has **exactly one** Pattern-B boss
quad, at (126.365, 48.9625), rotated 90° so the board stands on end along Y
`[CAD-MESH]`, and `v2/cad/parameters.scad` names the band after it —
`band = 4 * U; // [DERIVED] 76.200, the NeoSlider board on end` `[REPO]`. The
*fact* stays on the record because it is the reason one boss drawing served
three candidate parts while the layout was still open.

---

## 2. Rotary encoders ×3

### 2.1 Which PID

| PID | Name | Encoder fitted? | NeoPixel? |
|---|---|---|---|
| **4991** | I2C STEMMA QT Rotary Encoder Breakout **with NeoPixel** | **No** — "This version of the board DOES NOT have rotary encoder already soldered in" `[AF]` | Yes |
| **5880** | I2C STEMMA QT Rotary Encoder Breakout **with Encoder** | **Yes** — "has a rotary encoder already soldered in" `[AF]` | Yes `[STEP: NEO3535_REVERSE:LED1]` |
| **5881** | **Pink Anti-Static Zip Top Bag – 2 × 3.5 – 100 Pack** `[AF prod-5881]` | n/a | n/a |

Both carry the same seesaw MCU, same addresses, same board. **5880 = 4991 +
a pre-soldered encoder.** Buy 5880 ×3 unless you are choosing your own detent feel.

### 2.2 Board geometry (identical for 4991 and 5880) `[STEP]`

- PCB **25.400 × 25.400 × 1.570**, corner R**2.54**
- 4 × Ø**2.50** mounting holes on a **20.32 × 20.32** square, 2.54 in from each edge
- 0.1″ header: 6 × Ø1.00 at 2.54 pitch along one edge (INT, SCL, SDA, GND, 3Vo, VIN) `[FAB]`
- **STEMMA QT: two sockets on the BACK face**, at the midpoints of the two
  edges *perpendicular to the header edge*, centres **20.32 mm apart**, each
  2.54 mm in from its edge; body 6.00 (along the edge) × 4.95 (into the board)
  × **2.96 tall**; openings face **outward, in line with each other**.
  → a daisy chain runs **straight through** the board on one axis.
- **NeoPixel is reverse-mounted**: the package sits on the back face and emits
  **through the board to the front (knob) side**. Centre is on the board
  centreline, **10.41 mm from the board centre** (2.29 mm in from the edge),
  on the axis parallel to the header edge. Emitting aperture Ø**2.40**.
  → ~~**the panel needs a Ø2.5–3.0 mm light hole 10.41 mm off the knob axis**,
  or a light pipe.~~ **Not in v2 — this was written for a printed, opaque front
  wall, which is v1.** v2's deck surface is **12.700 mm of CLEAR acrylic**
  (`acrylic_t = 12.700; // [DESIGN] 0.5 in clear, laser cut. The brief.`
  `[REPO]`), so the NeoPixel is read straight through the sheet and **no
  aperture is cut**: `slab.dxf` holds sixteen loops and none of them is a light
  hole `[DXF]`. The geometric fact below it is unaffected and still matters for
  anything that sits on top of the board: the pixel is **not** under the knob,
  it is 10.41 mm off the knob axis, and the knob's Ø25.400 brim has a radius of
  12.700 — so the brim **covers** it. Whether the glow reads through the acrylic
  from under a knob brim is a **look-at-the-hardware question, not a cutting
  one** (§10).
- Encoder pin tails protrude **2.13 mm** past the back face — less than the
  2.96 mm connectors, so **back-face clearance is governed by the QT sockets at 2.96 mm**.

### 2.3 The fitted encoder (5880) — Bourns PEC11

Adafruit's loose encoder for the 4991 is **PID 377 = Bourns PEC11-4215F-S24**
`[AF prod-377 datasheet link]`, i.e. 24 detents / 24 pulses per revolution,
15 mm flatted metal shaft, with push switch. The 5880's modelled part is the
same family (`PEC11+SWITCH` in the STEP).

| Property | Value | Source |
|---|---|---|
| In-plane rotation on the PCB | **45° to the board edges** | `[STEP face normals]` |
| Envelope (square keep-out, board-aligned) | **16.70 × 16.70** | `[STEP bbox]` |
| Body thickness, PCB front face → panel mounting surface | **6.50** | `[STEP]` |
| Bushing thread | **M7 × 0.75** | `[DS PEC11 p2]` |
| ~~Bushing length~~ | ~~**5.0** for a 15 mm shaft; **7.0** for 20/25/30~~ — **REFUTED on the part, 2026-11-08. The 5880's modelled bushing is 7.000.** Measured on the house vendor mesh with the axis taken from the bushing's own bbox centre (12.700, 12.700, which is the board centre): a thread section of **Ø6.800 major / Ø5.717 minor running z −6.500 to −13.500 below the PCB front face — 7.000 long**. `v2/cad/parameters.scad` records the same number and the same cause: *"The datasheet's 5.000 came from the PEC11 'LB' row and is wrong for this part by 2.000."* The 5.0/7.0 row is a datasheet **option table** for shaft lengths and was read as if it described this part. | `[HOUSE-MESH]`, `[REPO]` |
| ~~Bushing length~~ | ~~6.5~~ — **WRONG, corrected (and this correction was itself superseded).** 6.5 is the BODY height from the PCB face to the mounting surface. The drawing carries 6.5 as a standalone leader for the body and `LB` for the threaded portion; 6.5 was read twice, once correctly as the body and once incorrectly as the bushing. ~~Max panel the stock encoder can be nutted to is therefore **2.60**, not 4.10.~~ — the 2.60 followed from the 5.000 refuted in the row above, and no panel figure applies in v2 anyway (below). | `[DS pec11 p3]` |
| Shaft | **Ø6.0, flatted to 4.5** (D-shaft, Bourns "F" type) | `[DS PEC11 p2]` **— and the flat is DATASHEET-ONLY. It is not in the model.** The 5880 mesh draws the shaft as a plain **Ø6.000 cylinder over its full 7.948 mm** of exposed length; the only sub-Ø6 features are a 0.600 mm relief at the shaft root (z −14.100 .. −13.500, which reads 4.750 across if mistaken for a flat) and a chamfer at the tip. So **nothing about the flat may be tagged `[STEP]`**, and the knob's D-bore rests entirely on the datasheet. §10. `[HOUSE-MESH]` |
| Bushing + shaft from mounting surface (L1) | **15.0** (also 20 / 25 / 30 available) | `[DS PEC11 p2]` |
| Shaft tip above PCB front face | **21.40** ( = 6.50 + 14.90) | `[STEP]` — reconciles with L1 = 15.0 to within 0.1 mm |
| Mechanical angle | 360° continuous | `[DS p1]` |
| Detent / resolution | 12 / 18 / 24 detents; 12 / 18 / 24 PPR | `[DS p1]` |
| Switch | SPST momentary, travel **0.5 ± 0.2**, force **610 ± 306 gf** | `[DS p1]` |
| Rotational life | 30,000 cycles (switch 20,000) | `[DS p1]` |
| Hardware supplied | one flat washer + one mounting nut | `[DS p1]` |
| Max RPM | 60 — "hand rotated mechanical encoders only" | `[DS p1; AF 4991]` |

**None of the following three paragraphs describes v2. They are kept, struck,
because they are a correct account of the v1-style flat-panel mounting this file
was first written against, and v1 still exists.**

> ~~**Panel cutout:** Ø**7.0–7.2** clearance for the M7 × 0.75 bushing.~~
>
> ~~**Maximum panel thickness at the encoder: ≈4.0 mm.** Bushing 6.5 − nut ~2.0 −
> washer ~0.5. v1's front wall is 2.40 mm (`v2/docs/envelope.md`), so a flush
> panel is comfortable; a recess deeper than ~1.5 mm beyond that must be a
> **counterbore**.~~
>
> ~~**Knob height above panel:** with a 2.40 mm panel, 15.0 − 2.40 = **12.60 mm**
> of bushing+shaft protrudes, of which the top ~8.5 mm is Ø6 D-shaft.~~

**What v2 actually does with the encoder, measured.** The encoder is never
nutted to anything: it arrives soldered to the 5880, and **the 5880 bolts to the
carrier** on a 20.320 × 20.320 M2 boss quad, its rear face on the wiring plane
at z = 0 with the STEMMA QT sockets hanging into a pocket whose floor is at
**z = −3.460** `[CAD-MESH]`, which is the 2.960 connector plus 0.500. So there
is **no panel cutout and no panel thickness at the encoder.** The bushing and
the nut are swallowed instead by the **printed knob's counterbore**, measured in
the exported knob as a **Ø12.000 bore from the knob's underside up to z = 6.350**
with the D-bore above it reading **6.100 × 4.600** `[CAD-MESH knob_encoder.stl]`.
The knob fills a Ø22.000 slab bore and stands 6.350 proud. §5.5.

The PEC11 nut and washer dimensions are still **NOT SOURCED** — the datasheet
says they are supplied but does not dimension them — and that now matters for a
different reason: the counterbore is Ø12.000, drawn 2.000 over a nominal M7
nut's Ø10 across the flats `[REPO]`, so an oversized supplied nut would stand
the knob off. §10.

### 2.4 CONSIDERED AND NOT CHOSEN: 5752 Quad Rotary Breakout

> **Retired as a recommendation 2026-11-08.** This subsection used to carry the
> heading *"Alternative: 5752 Quad Rotary Breakout — recommended for a row of
> three"* and a "why it wins" argument. The build uses **three individual
> 5880s** (§0.0, §0 item 2). **The measurements below are unchanged and are kept
> deliberately** — they are the record of what the Quad Rotary was compared on,
> and they are the only place in this repository that holds them.

| Property | Value | Source |
|---|---|---|
| PCB | **76.200 × 21.590 × 1.570**, R2.54 | `[STEP]` |
| Mount holes | 4 × Ø**2.50** on **38.10 × 16.510** (was written 16.52; §1) | `[STEP]`, `[FAB]` |
| Encoder positions | 4, at **19.05 mm pitch** on the board centreline (X = ±9.525, ±28.575 from centre) | `[STEP]` |
| Encoder rotation | **axis-aligned** (unlike 4991/5880), body envelope **12.10 × 12.30** each | `[STEP]` |
| Encoder height | **21.40** above front face — same PEC11 part as 5880 | `[STEP]` |
| STEMMA QT | **RIGHT-ANGLE** sockets (JST_SH4_RA), 2.90 × 6.00 × **4.25**, on the back face at the two ends, facing outward along the long axis; **4.30 mm above the back face** | `[STEP]` |
| Encoders supplied? | **No** — "does not come with any encoders soldered on". Budget 4 × PEC11 — one of the reasons it lost, see below | `[AF pinouts]` |
| I2C | 0x49, eight addresses 0x49–0x50 | `[AF pinouts]` |

~~**Why it wins for three-in-a-row:** one board, one address, one QT cable, one
boss quad shared with the slider and key strip, and a guaranteed 19.05 mm
pitch. Three separate 5880s need three 25.4 mm squares — a minimum 25.4 mm
pitch unless you overlap boards, three addresses and three cables.~~

**Why it was not chosen, and every advantage above answered.** The costs it
named are all real and all were paid cheaply:

| Its advantage | What the build did instead |
|---|---|
| one board | three — but on **38.100 mm centres**, measured off `slab.dxf` and `carrier.stl`, which leaves 12.700 of air between 25.400 boards and 11.430 of web between their pockets `[DXF; CAD-MESH]`. The 25.4 mm square was never the binding dimension. |
| one address | three — **0x36 / 0x37 / 0x38, by two solder-closed jumpers**, which are reversible. The Quad Rotary's are **cut**, which is not. `v2/parts/wiring.md` §2.3 `[REPO]` |
| one QT cable | three — on a chain that was going to exist anyway, with a channel cut for it in the carrier floor `[REPO]` |
| one boss quad shared with the slider and key strip | the key strip is **out of the design** (§4), so there was nothing left to share with |
| a guaranteed 19.05 mm pitch | **this is the reason it lost.** A fixed 19.050 pitch caps the knob; `v2/cad/knob.scad` puts the cap at Ø13.000 and the deck's knob is Ø25.400 `[REPO; CAD-MESH]`. §0 item 2. |

One further cost that was never in its favour: the 5752 ships with **no
encoders**, so it is one board **plus four PEC11s to hand-solder**, against
three 5880s that arrive assembled — which is `v2/parts/knobs.md` §0 item 1's
point, reached from the knob end `[REPO]`.

**Flag — measured, and it reads oddly:** the four reverse-mount NeoPixels on
the 5752 sit at **exactly the encoder centres** (same X, same Y) `[STEP]`, i.e.
*underneath* the encoder bodies. On the NeoSlider the same arrangement works
because the pot has a full-length centre slot for the light to escape through
(§3); an EC11 body has no such slot. **If per-encoder lighting matters to you,
verify on hardware before committing panel apertures.** The 4991/5880 do not
have this problem — their NeoPixel is clear of the encoder (§2.2).

---

## 3. NeoSlider — PID 5295, **×1**

I2C QT Slide Potentiometer with 4 NeoPixels. Default **0x30**, sixteen
addresses 0x30–0x3F `[AF pinouts]`.

> **Count corrected 2026-11-08: ×1, not ×2.** This heading read `×2`, §8.1 read
> Qty 2 and §11 read Qty 2, and all three were wrong against the geometry.
> `carrier.stl` holds **one** Pattern-B boss quad — four M2 bores at x 118.110 /
> 134.620 and y 29.9125 / 68.0125, centred on (126.365, 48.9625) — and **one**
> board seat pocket, 22.860 × 77.470, at the same centre. `slab.dxf` holds
> **one** slot. `[CAD-MESH; DXF]` `v2/parts/wiring.md` §6.4 already had it as
> *"1 NeoSlider — one, not two"*; this file had not caught up `[REPO]`.

### Board `[STEP]`
- PCB **76.200 × 21.590 × 1.570**, R2.54
- 4 × Ø**2.50** on **38.10 × 16.510** (X = ±19.050, Y = ±**8.255** from board
  centre — was written 16.52 / ±8.26; §1)
- **Back face:** two JST-SH QT sockets at **X = ±15.24** (30.48 apart), on the
  board centreline, 4.95 × 6.00 × **2.96**, facing outward along the long axis.
  Note these are **inboard, not at the ends** — unlike the NeoKey.
- **Back face:** 4 reverse-mount NeoPixels at **X = ±9.53 and ±26.67, Y = 0**,
  emitting through the board to the front. They sit on the pot's centreline and
  light escapes through the pot's own full-length slot — which is why they are
  "under the slide potentiometer" `[AF pinouts]` and still visible.
- Pot solder lugs protrude **2.75 mm** above the back face (less than the 2.96 connectors).

### Slide potentiometer `[STEP]`
- Plan envelope **75.0 × 9.5**, centred along X, offset **0.24 mm** off the
  board's Y centreline.
- **Projects 15.50 mm from the PCB front face**, made up of a **6.50 mm** pot
  body (matching Adafruit's "Height: 6.5mm" for the bare pot, PID 4219 `[AF]`)
  plus a **9.00 mm** actuator shank clear of the body.
- Shank is a stepped blade: lower section ≈5.14 long × 1.8 thick, upper
  ≈4.14 × 1.2, tip 2.64 × 0.80. `[STEP face geometry]`

### Slab slot — what is actually cut, and the v1-panel version it replaces

> ~~**Panel cutout.** A slot at least **76 × 10 mm** to clear the pot body, or —
> better — a **75 × 2.5 mm** slot for the shank only, with the pot body below
> the panel. With the body under a 2.40 mm panel, the shank stands
> **15.50 − 6.50 − 2.40 = 6.60 mm** proud. **The nubbin then has to reach down
> to the shank**, so check its bore depth before fixing the panel thickness.~~
>
> **Superseded 2026-11-08 — again a 2.40 mm v1 front wall, not a v2 slab.**

**What the cut file says.** `slab.dxf` carries **one** slot, **12.000 × 52.000,
centred at (126.365, 48.9625)**, long axis along Y `[DXF]`. It is not sized to
the pot body or to the shank: it is sized to the **printed cap**, by
`slider_slot_w = slider_cap_w + 2.000` and
`slider_slot_l = slider_travel + slider_cap_w + 2.000` `[REPO]`, and 12.000 =
10.000 + 2.000 while 52.000 = **40.000** + 10.000 + 2.000 — so **the emitted
slot still encodes the retired 40 mm travel figure**, see below.

**And there is no nubbin to check the bore depth of.** The cap is printed:
`fader_cap.stl` measures **16.000 × 25.400 × 15.875** overall, a **10.000 square
plug** through the full 12.700 of acrylic with **3.175 proud**, and a blind
**1.700 × 5.200** socket 8.000 deep that grips the pot's blade — the blade
measures **1.786 thick** `[HOUSE-MESH]`, so the 1.700 socket is an interference
fit and the grip *is* the fixing `[CAD-MESH; REPO fader_cap.scad]`. Adafruit's
5621 nubbin is off the BOM (§11).

### Travel: **67.000** `[MEASURED, owner's calipers 2026-11-08]` — CLOSED

**The owner put calipers on the real part and it is 67 mm.** That closes the
single most important missing number in this document (§10 item 1 was it) and
retires every earlier figure and aside about it.

| | mm | |
|---|---|---|
| **travel** | **67.000** | `[MEASURED, owner's calipers]` |
| ~~travel~~ | ~~NOT SOURCED~~ / ~~"commonly 60 mm"~~ / ~~40.000~~ | **all retired.** See the three notes below. |
| shank tip from the PCB front face | **15.500** | `[STEP]`, and confirmed `[HOUSE-MESH]` |
| slide-pot body front face | **6.500** — *was 6.00 here, which is a secondary step* | `[HOUSE-MESH]`, `[REPO]` |
| **shank proud of its body** | **9.000** — *was 9.50 here* | `[DERIVED]` |
| shank, widest exposed section | **5.141** wide × **1.786** thick, centred x +0.071 | `[HOUSE-MESH]` |
| pot body plan envelope | **75.000 × 9.500** | `[HOUSE-MESH]`, confirms `[STEP]` |

**Note 1 — the three retired figures, and where each came from.**
*NOT SOURCED* was correct when written: the vendor model draws the pot's cover
as solid across the whole body with the shank passing through it, so scanning
every z level between cover and tip returns one run — the shank — and no gap
anywhere, which is why travel could not be derived from the file. That reasoning
is kept because it is still the reason the model cannot be used. *"Commonly
60 mm"* was flagged as a guess in its own sentence and is now simply wrong.
**40.000** never appeared in this document: it is
`v2/cad/parameters.scad`'s `slider_travel = 40.000; // [PROVISIONAL]`, whose own
comment says *"Calipers on the real part settle it"* `[REPO]`. They have.
**Changing that parameter is not this document's job and has not been done
here** — a separate thread owns it. This section's job was to stop contradicting
the measurement, and it no longer does.

**Note 2 — 67.000 is consistent with the body, and 40.000 was not.** The pot
body measures **75.000** long and the blade's widest exposed section **5.141**
wide `[HOUSE-MESH]`. A 67.000 travel puts blade-plus-stroke at
67.000 + 5.141 = **72.141** inside 75.000, leaving **1.4295 of cover beyond the
blade at each end** `[DERIVED]` — which is how a full-length fader is built. At
40.000 the same arithmetic leaves **14.930 of dead cover at each end** of a
75 mm body, which no slide pot is made like. **The artefact actively supports
the caliper reading.**

**Note 3 — what 67.000 costs the slab, stated because it is not free.** By the
slot formula `[REPO]` the slot becomes **67.000 + 10.000 + 2.000 = 79.000**
long, against the **52.000** presently emitted `[DXF]`. Centred on the same
(126.365, 48.9625) that is y **9.4625 … 88.4625**, and the control band it has
to live in is y **10.8625 … 87.0625** — the carrier's own limits at that X,
measured as material ending at y 10.860 on the spine side and resuming at
**y 87.065** at the rear rim's inner face `[CAD-MESH carrier.stl, z 5.000,
x 126.365]`. So a 79.000 slot **overruns the band by 1.3975 at each end**, and
the cap's plug is the full 12.700 of acrylic thickness, so at the rear it would
meet the carrier rim rather than air. It also exceeds the **76.200** board it
serves. **Flagged, not solved: it belongs to whoever changes the parameter**,
and the choices are to shorten the cap, move the slot, or grow the band.

### Flag — the vendor's own figures disagree
Adafruit lists the 5295 as **76.2 × 29.8 × 21.5 mm** `[AF]`. The STEP totals
**76.20 × 21.59 × 20.03** — independently confirmed here on the house mesh as a
**76.200 × 21.590 × 20.030** bounding box `[HOUSE-MESH]`. The 21.5 is the board
width and the 76.2 the length, so the **29.8 is the height — 9.8 mm more than
the model.** That gap is almost exactly the height of Adafruit's slide-pot
nubbin (**5621**, 11 × 7.1 × **9.3 mm**, shaft slot 5 × 2.2 mm `[AF]`), which is
**not in the STEP**.

~~Assume a fitted nubbin adds up to ~9.8 mm over the bare shank until measured.~~
**No longer an assumption anyone has to make: the 5621 is not used.** v2 prints
its own cap (above), so the vendor's 29.8 figure is kept here only as the
explanation for the model/page discrepancy, which is what it was always worth.
The one thing the 5621's dimensions are still good for is a sanity bound: its
**5 × 2.2 mm** shaft slot against the blade measured at **5.141 × 1.786**
`[HOUSE-MESH]` says the vendor nubbin is a loose fit on width, which is part of
why the printed cap grips on thickness instead.

---

## 4. CONSIDERED AND REMOVED FROM THE DESIGN: NeoKey 1×4 QT — PID 4980

> **Removed from the v2 design. Retired as a component 2026-11-08; retained in
> full as engineering record.** This section's heading read *"NeoKey 1×4 QT —
> PID 4980, ×1"* and §0 item 3 and §11 carried it as a first-class part. It is
> **not in the build**: `carrier.stl` has no boss quad for it and `slab.dxf` has
> no MX cutouts — the slab's sixteen loops are the outline, the keyboard void,
> six Ø22.000 bores, the fader slot, six fastener holes and a fiducial `[DXF;
> CAD-MESH]`.
>
> **What replaced it is not a bigger key breakout — it is a whole keyboard.**
> v2's keys are a **stripped Keychron K9 Pro**: 61 switches on an exact
> **19.000 mm** lattice (not 19.050), retaining the aluminium top frame, which
> **is** the switch plate at **1.200 mm**. `v2/parts/keyboard-strip.md` owns all
> of it, and `v2/cad/parameters.scad` §8 carries the datums `[REPO]`.
>
> **Everything below is kept because it is measured and it is nowhere else in
> this repository** — the 4980's hole pattern, its hot-swap sockets, and the
> Cherry MX datasheet block, which is the only place the MX stack is written
> down. The *advice* in it has been struck where it is no longer advice.

Default **0x30**, sixteen addresses 0x30–0x3F `[AF pinouts]`.

### Board `[STEP]`
- PCB **76.200 × 21.590 × 1.570**, R2.54
- 4 × Ø**2.50** on **38.10 × 16.51** (at X = 19.05 / 57.15, Y = 2.54 / 19.05)
- **Key centres: X = 9.525, 28.575, 47.625, 66.675 → 19.05 mm pitch**
  (standard 0.750″ MX), all on the board centreline Y = 10.795.
- Per-key hole pattern, relative to the key centre:
  - central post Ø**3.90** at (0, 0) — Cherry nominal Ø3.99 `[DS p2]`
  - two fixation-pin holes Ø**1.81** at (**±5.08**, 0) — Cherry nominal Ø1.70 `[DS p2]`
  - two contact holes Ø**3.06** at (−2.54, +5.08) and (+3.81, +2.54)
    — oversized vs Cherry's Ø1.50 pin holes because of the sockets, below.
- **Back face:** four **Kailh-style hot-swap sockets**, **10.90 × 5.90 × 3.00**,
  reaching 1.83 mm above the back face.
- **Back face:** two QT sockets **at the two short ends**, 4.95 × 6.00 × **2.96**,
  facing outward along the long axis → the chain runs straight through.
- **Back face:** 4 reverse NeoPixels at Y = 5.715 (5.08 mm off the key
  centreline), at the key X positions, emitting to the front.
- Tallest thing on the back face: the **QT sockets at 2.96 mm**.

### Switches are NOT included, and not soldered
"There are four Cherry MX or compatible key switch **sockets** … Simply press
any compatible key switch into the socket from the top of the board" `[AF pinouts]`.
~~**Budget 4 × MX switches separately. No soldering.**~~ — **not for this
build.** The K9 Pro arrives with its own 61 switches and 61 hot-swap sockets
(`v2/parts/keyboard-strip.md` §0), so there are **no loose MX switches or
keycaps in the v2 BOM** (§11). The statement is correct about the 4980 itself
and stays on the record for that reason.

### Switch and keycap stack — Cherry MX datasheet `[DS cherry-mx p1, p3]`
| Property | Value |
|---|---|
| Switch housing footprint | **15.6 × 15.6** |
| Height, PCB face → top of stem | **11.6** |
| Stem above housing | 3.6 |
| Pins below PCB | 5.0; plastic locating post 3.30 |
| Total travel | **4.0 mm** (0.16″); actuation at **2.0 mm** pretravel |
| Operating force (linear) | 2.1 oz ± 0.7 ≈ 60 gf |
| **Plate / panel cutout** | **14.00 ± 0.05 mm square**, corner R ≤ 0.3 |
| Plate thickness the clips expect | **1.5 mm** (0.06″ ± 0.004) |
| Keycap, standard 8 mm type | top ≈**17.5 mm** (0.69″) above the PCB |
| Keycap, cylindrical rows C–E | 17.9 – 20.8 mm (0.704″ – 0.82″) |

~~**Panel consequence:** the MX panel cutout is **14.00 mm square at 19.05 mm
pitch**, and the plate should be **1.5 mm** where the switch clips engage. A
3D-printed plate thicker than that stops the clips latching — the switch then
hangs on the hot-swap socket alone, which is not a mechanical mount. If the key
band needs to be recessed, **make the 1.5 mm plate a separate inset and recess
around it**, exactly the counterbore logic that applies to the pots in §5.~~

**Superseded 2026-11-08 — and the problem it describes is solved by a bought
part, not by a printed inset.** v2 cuts **no MX plate at all**: the K9 Pro's
**aluminium top frame is the switch plate**, measured at **1.200 mm**
`[CAD-EXACT, keyboard-strip.md]`, and it is one of the parts the build keeps. So
the clip-latching trap this paragraph correctly identified cannot arise — the
clips engage a plate Keychron made for them, and the carrier's job is to seat
the assembly, not to be a plate. The MX *keycap* and *plate* dimensions in the
table above stay valid as the Cherry datasheet record.

Keycap heights above are Cherry's own for Cherry's caps. **Aftermarket profiles
(OEM, DSA, XDA, SA) differ by several millimetres and are NOT SOURCED here** —
measure whatever you actually buy (§10).

---

## 5. Nut-mount potentiometers ×3 — and in v2 the nut lands on a CARRIER WEB

**Scope.** Standard nut-mount pots with a threaded bushing, taking a knob. Not
PCB-mount, not a breakout. **The part class is unchanged and this section is
still first-class — but the thing the nut clamps is not a panel.**

> ~~"**Panel-mount** potentiometers … held by the panel with a nut … The
> governing number is the **bushing length**, because it sets the maximum panel
> thickness at that point."~~

**Corrected 2026-11-08. Nothing on this deck panel-mounts.** Every control is
carried by the 3D-printed carrier and recessed; the knob sits down inside a bore
through the acrylic with only its top proud (`v2/parts/knobs.md` §0 `[REPO]`).
The bushing length is still the governing number, but what it governs is the
**carrier web**, and that web is **3.810 mm** — measured in the exported part as
material from z **5.085 to 8.885** at 0.005 mm sampling, inside a
**24.130 × 31.750** collar with a **Ø7.200** bore, three of them on a 38.100
pitch at y 29.9125 `[CAD-MESH carrier.stl]`. `v2/cad/parameters.scad` states the
reasoning in the same terms this section used to: *"the slab is 12.700 and the
bushing is 6.600, so the pot cannot be carried by the slab — which is why the
brief's 'the potentiometers should be panel mounted' has to mean mounted to a
web in the CARRIER, with the knob filling the slab's bore above it."* `[REPO]`

The pot also hangs **below** the web into a blind body pocket, measured
**19.050 × 26.670** with **2.220 mm of floor left under it** `[CAD-MESH]` — see
§5.4, which is where a real problem with that pocket is now recorded.

### 5.1 The bushing-length arithmetic — still correct, now applied to the web

> **The question this subsection was written to answer is no longer asked.** It
> was *"counterbore or thick panel?"*, and v2's answer is neither: the pot is
> nutted to a 3.810 mm carrier web and the knob carries its own counterbore.
> **The arithmetic itself is sound and is kept** — it is what tells you a stock
> bushing is sufficient. Read "panel thickness" below as "thickness of whatever
> the nut clamps".

```
usable bushing length  =  panel thickness  +  washer  +  nut  (+ 1 thread or so to bite)
```

With a typical supplied **M7/M9 nut at 2.0 mm thick** and a **0.4 mm washer**
`[DS PTV09 p3 mounting hardware]`:

| Bushing length | Max panel thickness at the pot |
|---|---|
| 5.0 mm | **≈2.6 mm** |
| 6.5 mm (16 mm Alpha default) | **≈4.0 mm** |
| 7.0 mm | **≈4.5 mm** |
| 8.0 mm | **≈5.5 mm** |
| **10.0 mm** | **≈7.5 mm** |

~~**So: anything up to ~4 mm of panel is fine with stock parts. A recess that
makes the panel locally thicker than that is only possible with a 10 mm
bushing — otherwise the recess must be a COUNTERBORE: thin (≤4 mm) where the
bushing passes, opened out above it to clear the knob.**~~

**What it comes to in v2, 2026-11-08.** The 562's bushing measures **6.600**
(a Ø6.2846 thread section running z 2.000 to 8.600 on the house mesh
`[HOUSE-MESH]`, matching `pot_bushing = 6.600` `[REPO]`), which by the table
above allows **4.200** of clamped material on this table's 2.000 nut and 0.400
washer — `parameters.scad` says 4.100, counting a 0.5 mm washer, and the
difference is the washer, not a disagreement. The web is **3.810**
`[CAD-MESH]`, so 3.810 + a 2.000 nut = **5.810**, leaving **0.790** for the
0.400 washer and the thread's bite. **It fits with a stock bushing, with 0.390
left for the bite**, and `v2/cad/parameters.scad` chose 3.810 for exactly
that reason — *"6 × grid, inside the 4.100 of panel the bushing allows once a
nut and washer are counted"* `[REPO]`. The counterbore the old conclusion
demanded does exist, but it is cut in the **printed knob**, not in the deck:
**Ø12.000 × 6.350 deep**, measured `[CAD-MESH knob_encoder.stl]`, sized 2.000
over a nominal M7 nut's Ø10 across the flats. §5.5.

### 5.2 Candidates with real datasheet numbers

#### Alpha **RD901F** — 9 mm, single metal shaft, nut mount `[DS alpha-RD901F]`
~~**The recommendation, because of the bushing table.**~~ **An alternative, and
a good one — but no longer the recommendation.** Its whole advantage was a 10 mm
bushing for a thick panel, and v2 clamps 3.810 mm of carrier web (§5, §5.1), so
that advantage buys nothing. Its real remaining advantages are a **smaller body
behind the web** and a datasheet that actually specifies a **T18 spline**, which
the 562's vendor model does not (below). Survey kept in full.

| | |
|---|---|
| Bushing options (order-code suffix) | **blank** = Ø7 × 5 mm · **B3** = Ø7 × 7 mm · **B6** = Ø7 × **10 mm** · **E1** = Ø9 × 5 mm · **E3** = Ø9 × 7 mm · **E6** = Ø9 × **10 mm** |
| Thread | **M7 × 0.75** (Ø7 variants) / **M9 × 0.75** (Ø9 variants) |
| Shaft | **Ø6.0, 18-tooth knurled (T18)**, C0.5 chamfer; length *L* per order code (e.g. 15F = 15 mm) |
| Rotational angle | **300° ± 5°** |
| Taper | **B = linear** (also A log, C rev-log); 1 KΩ–500 KΩ, 1 MΩ |
| Rotational torque | 20–250 gf·cm |
| Rotation life | 15,000 cycles |
| Terminals | 3, horizontal (`-20`) or vertical (`-30`); Ø1.0 PCB holes, 2.5 / 5.25 spacing |
| Bracket option | `-00D` adds a mounting bracket |

~~**This is the extra-long-bushing part the brief asked for.** `RD901F-20B6-15F-B10K`
(Ø7 × 10 mm bushing, 15 mm T18 shaft, 10 KΩ linear) gives a **≈7.5 mm panel
budget** — a genuine recessed pocket with a plain through-hole, no counterbore.~~
**Superseded 2026-11-08: the brief changed and the panel went away.** The part
number is kept because it is correct and it is the thing to order *if* a future
revision ever clamps more than ~4 mm. For the geometry as drawn, the shorter
`-20`/`-B3` bushings are the ones that suit a 3.810 web.

Panel cutout: **Ø7.2** for the Ø7 bushings, **Ø9.2** for the Ø9 bushings.

#### Alpha **RV16AF** — 16 mm, metal shaft, panel mount `[DS alpha-RV16AF]`
**The better *feel*, and what Adafruit already stocks** (their related-product
lists carry `RV16AF-41-15R1-…` part numbers `[AF prod-562/3391]`).

| | |
|---|---|
| Bushing options | **A** = M6 × 0.75 · **B** = M7 × 0.75 · **C** = M8 × 0.75 · **D** = M3/8 × 0.75; lengths **1** = 5.0 · **2** = 6.5 · **3** = 7.0 · **4** = **8.0 mm**; blank = M7 × 0.75, 6.5 mm |
| Rotational angle | **300° ± 5°** |
| **Detent options** | code **3** = no click; **3C / 3D / 3E / 3F** = centre click, **11, 21 or 41 clicks** |
| Rotational torque | 20–200 gf·cm; click torque 30–250 gf·cm |
| Taper | A log / **B linear** / C rev-log; 1 KΩ–500 KΩ, 1 MΩ |
| Rotation life | 15,000 cycles |
| Push/pull strength | 8 kgf for 3 s; rotational stop 6 kgf·cm min |
| Front boss | Ø17 |

Longest bushing here is **8.0 mm → ≈5.5 mm panel**. Detented 21- or 41-click
variants are genuinely attractive for an instrument surface.

#### Adafruit **562** (10 K linear) / **3391** (10 K log) — 16 mm, breadboard-friendly
Adafruit's own stock item, and an RV16AF-class part. **Measured from the vendor
STEP** `[STEP 562]`, which is the most reliable data in this section:

| Feature | Measured |
|---|---|
| Front flange | Ø**17.0**, 1.5 thick, trimmed to **11.0 mm** across one axis |
| Shoulder below flange | Ø**12.0** |
| Threaded bushing | modelled Ø**6.27** major / Ø5.41 minor over **6.6 mm** — consistent with **M7 × 0.75 × 6.5 mm** per the Alpha datasheet (models routinely draw threads undersize) |
| Shaft | Ø**6.000**, and **the model shows NO knurl and NO flat — a plain cylinder.** ~~T18 knurled (bbox 5.90 × 5.93 — not a D-flat), 6.5 mm of knurl~~ **— refuted on the artefact 2026-11-08.** Over z 11…16 every one of the 64 shaft vertices sits at **r = 3.0000 exactly, one distinct radius**, clustered at 20° stations: that is a tessellated cylinder, and the 5.90 × 5.93 "bbox" is simply an inscribed polygon's shortfall under Ø6.000, not evidence of a spline. `v2/cad/parameters.scad` says the same — *"the house model of the 562 shows a PERFECT Ø6.000 cylinder over z 11..16 — no flat, no spline — which is a vendor model and not a measurement of the real shaft's form."* **So the 562's shaft form is NOT SOURCED** (§10); T18 is a *datasheet* claim for the Alpha RD901F/RV16AF, not a measurement of this part. `[HOUSE-MESH]`, `[REPO]` |
| Shaft tip above panel seat | **15.6** |
| Body behind the panel seat | **9.2** total — a Ø**16.0** × 6.0 can plus a 17.5 × 20.75 × 1.7 base plate |
| Terminals | 3 at **5.08 mm pitch**, right-angle, extending **16.6 mm** sideways from the shaft axis at 3.45 mm behind the seat |
| Rotational travel | **300°** `[AF]` |
| Anti-rotation lug | **none modelled** — the front boss is Ø17 with flats; a plain round panel hole is sufficient |

**Panel budget: ≈4.0 mm.** Cutout Ø**7.2**.
**Enclosure budget behind the panel: 9.2 mm deep, Ø16 — plus 16.6 mm of
sideways terminal reach, which is the dimension that actually bites in a
crowded deck.**

#### Bourns **PTV09A** — 9 mm `[DS bourns-PTV09]`
Named in the brief. Of its eight variants **only `PTV09A-6` is nut-mount**
(horizontal/rear mount with a **metal threaded bushing, M9 × 0.75**); -1 and
-7/-8 use a plain *sleeve* bushing (no thread, no nut) and -2/-3/-4 are
bushingless. Specs: mechanical angle **280° ± 10°**, **centre-detent option**,
rotational torque 10–100 gf·cm, detent torque 20–200 gf·cm, 10,000 cycles,
shaft **Ø6.0** in F (flat, **4.5 across the flat**), S (18-tooth knurl) or
U (40-tooth knurl), lengths **15 / 20 / 25 / 30 mm**, terminals 3 at 2.5 mm
pitch. IP40.

**Supplied hardware, dimensioned** `[DS p3]` — the only datasheet here that
dimensions it, so use these as the reference for the whole class:
- **Nut M9 × 0.75, across-flats 11.0 ± 0.5, thickness 2.0 ± 0.2**
- **Washer ID 9.1 ± 0.3, OD 15.0 ± 1.0, thickness 0.4**

→ **Panel cutout Ø9.1–9.3; allow Ø16 of flat around it for the washer, and an
11.5 mm wrench clearance for the nut.**

**Caveat, stated rather than guessed:** the PTV09A-6 drawing carries both a
`5.0` and a `5.9` dimension in the bushing region and I could not unambiguously
assign either as the thread length. **Treat the usable bushing as ≤5.0 mm**
(→ **≈2.6 mm clamped**) until verified. That makes the PTV09A the *tightest*
option here ~~and the reason the Alpha RD901F is recommended instead~~ — **and
on a 3.810 mm web it is the one option in §5.2 that would genuinely fail.**
2.6 < 3.810 + 2.000 nut, so a PTV09A-6 could not be clamped to the carrier web
at all. The RD901F is no longer the recommendation either (§5.3); what rules the
PTV09A out is the web, not a comparison.

#### Bourns **PTD90** — 9 mm multi-ganged `[DS bourns-PTD90]`
**M7 × 0.75** bushing, knurled or plain shaft, **300° ± 10°**, 15,000 cycles,
IP40, washer + nut supplied, PTD901 body depth **7.05 ± 0.5**. Ganged versions
(PTD902/904/906/908) put 2–8 sections on one shaft — only relevant if you want
one knob driving several parameters. Bushing length **NOT SOURCED** from this
drawing (§10).

#### Alps **RK09**
**NOT SOURCED.** `tech.alpsalpine.com` and `alpsalpine.com` both return **403**
from this environment on every product and datasheet path tried. RK09 is the
direct competitor to the RD901F/PTV09 (9 mm, M7 × 0.75, 300°) but I will not
put numbers in this file that I could not fetch. If you want RK09 specifically,
pull the PDF from a distributor and I will fold it in.

#### Alpha **RV09** — named in the brief, but not a panel-mount part
Alpha's RV09 series is listed as "9 mm, **snap-in**, insulated shaft"
`[Alpha catalogue]` — PCB-mount with a plastic shaft, **no threaded bushing**.
For the brief's "nut-mounted through the front panel", the 9 mm Alpha part is
**RD901F**, not RV09.

### 5.3 Summary — which to buy

> **Rewritten 2026-11-08. The table below used to rank parts by "max panel", a
> figure v2 no longer has.** The previous first row recommended the Alpha
> RD901F's 10 mm bushing for a "recessed, no counterbore" mount; the recess is
> now in the knob and the clamped thickness is a 3.810 mm carrier web, so that
> row has nothing to recommend. It is struck below, kept.

**What the geometry requires of the pot — all of it measured, so this is a
specification and not a preference:**

| Requirement | Figure | Source |
|---|---|---|
| bushing thread | **M7 × 0.75**, cutout **Ø7.200** | `[CAD-MESH]` the three web bores measure Ø7.200 |
| bushing length | **≥ 6.210** = web 3.810 + washer 0.400 + nut 2.000, plus a thread's bite; the 562's **6.600** leaves **0.390** for the bite | `[CAD-MESH; HOUSE-MESH]` |
| body behind the web | fits **19.050 × 26.670** × 9.200 deep — but see **§5.4**, the terminals do not | `[CAD-MESH]` |
| shaft | **Ø6.000**, **NON-D** (splined/knurled or plain round); the printed knob's six ribs grip either | `[REPO]` `knob.scad` |
| shaft length above the clamping face | **≥ 15.600**, so the tip reaches the knob's bore | `[HOUSE-MESH]` 562 measures 15.600 |
| taper | **B = linear**, **10 kΩ** | `[REPO]` `wiring.md` §3.5 sizes the 1.5 kΩ top leg on 10 kΩ |
| rotation | 300° nominal | `[DS]` |

**Buy:** **Adafruit 562** (10 kΩ linear) is the part every v2 figure above was
measured from and the safe default. Any **M7 × 0.75 nut-mount 10 kΩ linear
pot with a ≥6 mm bushing, a Ø6 non-D shaft ≥15.6 mm long, and terminals
inside §5.4's envelope** substitutes for it.

**Retired row, for the record:** ~~"Recessed, no counterbore — Alpha
RD901F-xxB6 or -xxE6, 10 KΩ linear (B), Ø7 or Ø9 × 10 mm bushing, ≈7.5 mm max
panel."~~

**Alternatives, unchanged as survey:** Alpha **RV16AF** for the detented
21/41-click option, which is still genuinely attractive on an instrument;
Alpha **RD901F** for the smallest body behind the web.

For a **control surface, buy linear (taper B)**, not audio/log — log taper
crushes resolution at one end of a parameter sweep. **This is the one
recommendation in §5 that has never changed**, and `v2/parts/wiring.md` §3.5
depends on it.

### 5.4 What has to fit behind the web — and the terminals currently do not

| Part | Diameter / section behind the clamping face | Depth behind it | Terminal reach from the shaft axis |
|---|---|---|---|
| Adafruit 562 / RV16AF | Ø**16.000** can (measured ±8.000 in both axes) | **9.200** | **16.5956 mm sideways**, three tabs each ≈1.17 × 0.50, at 5.08 pitch, on a 17.500-wide base plate 1.700 thick |
| Alpha RD901F (9 mm) | ~9.5 × 9.5 body | ~7.5 `[DS, body 7.5]` | 3 pins, 2.5 / 5.25 spacing |
| Bourns PTD901 (9 mm) | — | **7.05 ± 0.5** `[DS]` | — |

Row 1 re-measured independently for this pass `[HOUSE-MESH]`, and it confirms
both §5.2's `[STEP]` figures and `v2/cad/parameters.scad`'s — **which are not
the same measurement, and conflating them is the trap.** `parameters.scad` has
`pot_lug_span = 25.336`, this section has 16.6, and both are right: **25.3357 is
the part's overall span across the Y axis** (lug tip −16.5956 to body edge
+8.7401) while **16.5956 is the reach from the shaft axis to the lug tips**. The
first is symmetric about nothing; the second is what a pocket centred on the
axis has to clear.

**The 16 mm pot's 16.6 mm of sideways terminal reach is easy to miss and is
usually what collides. It is colliding now.** `parameters.scad`'s comment calls
`pot_lug_span` *"what the floor's pocket actually has to clear"* and sizes
`pot_pocket_sz = [19.050, 26.670]` against it — 26.670 > 25.336, which is true
of a **centred** span and false of this part. Placed as the carrier intends —
shaft axis on the measured bore centre (36.195, 29.9125), clamping face on the
web's underside at z 5.080, terminals toward the wiring trunk — **all three
terminal tabs pass clean through the web collar**, and a boolean against the
exported part puts the figure on it:

| | |
|---|---|
| tab z in the carrier frame | **1.380 … 1.880** |
| carrier material the tabs occupy | y **14.0375 … 16.5775**, i.e. the collar's 2.540 mm wall |
| interference | **1.4808 mm³ per tab, 4.4425 mm³ total** |
| tab tip | **0.7206 beyond the collar's OUTER face** |
| at 90° / 270° / any 45° step | **also clashes** — the collar's X half-extent is 12.065, and the worst-case corner reach is 16.388, still under 16.5956. **There is no rotation that clears it.** |

`[CAD-MESH, boolean intersection against v2/export/stl/carrier.stl, procedure in
§12]`

**This is a geometry finding, not a parts finding, and it is flagged rather than
fixed — this document owns no `.scad`.** The outs are: lengthen the pocket and
the collar to 2 × 16.5956 + clearance (≈34), offset the pocket ≈3.93 in −Y, or
record that the tabs are bent down on assembly, in which case say so and give
the bend. **Do not treat the 26.670 as sufficient on the strength of the 25.336
figure** — that is the error that produced it.

### 5.5 Knobs — THE KNOB IS PRINTED IN-HOUSE. The bought-knob survey below is record only

> **Retired as a recommendation 2026-11-08.** v2 does not buy knobs. It prints
> **six**: `knob_encoder.stl` ×3 with a D-bore and `knob_pot.stl` ×3 with a
> plain bore and six internal ribs, from one file, `v2/cad/knob.scad`, which
> may also be **cast in acrylic** from the printed mould
> `knob_mould_brim.stl` / `knob_mould_plug.stl` `[REPO]`.

**The knob that exists, measured off the exported meshes:**

| | knob_encoder | knob_pot | |
|---|---|---|---|
| overall height | **19.050** | **19.050** | `[CAD-MESH]` |
| plug Ø, inside the slab bore | **18.000** over the lower 12.700 | **18.000** | `[CAD-MESH]` |
| brim | **25.247** across the flutes, Ø**25.400** at the intact rim, **6.350** proud | same | `[CAD-MESH]` |
| nut/bushing counterbore | **Ø12.000**, floor at z **6.350** | same | `[CAD-MESH]` |
| shaft bore | **6.100 × 4.600** — a D | **Ø6.100 with 6 ribs**, crest radius 2.750 | `[CAD-MESH]` |
| indicator | one **2.000 × 1.000** groove across the top face — *the only legend on the deck* | same | `[CAD-MESH; REPO]` |

`knob_h − knob_proud == acrylic_t` is an identity in `knob.scad`, not a
coincidence: **19.050 − 6.350 = 12.700** `[REPO]`, which is what "the knob's
underside is flush with the acrylic's underside" means.

**And this is where "non-D pots, D-shaft encoders" is actually implemented.**
Two bores exist because the two shafts differ: the PEC11 is a D (datasheet, and
**only** the datasheet — §2.3), while the pot is specified non-D, so its knob
gets six ribs that *"bite a round shaft AND a T18 spline, which makes the knob
correct either way while the shaft's real form is NOT SOURCED"* `[REPO]`. A grub
screw was not an option and the reason is geometric rather than stylistic: the
pot's shaft tip lands at z **20.680** and the deck's top face is at **22.225**,
so the shaft stops **1.545 below the surface** and there is nowhere on the knob
that is both outside the acrylic and over the shaft `[REPO]`.

#### 5.5.1 Bought-knob survey — kept as record, not as a recommendation

All Adafruit; all `[AF]` product pages.

| PID | Knob | Ø | Height | Bore | Fits |
|---|---|---|---|---|---|
| **5527 / 5528 / 5530** | Anodized aluminium machined (black / silver / red) | **20.0** (inner 15.0) | **12.5** | **Ø6 + 2 mm hex set screw** | **T18, round AND D-shaft** |
| 2058 | Slim metal, T18 | 10.0 | 10.0 | T18 splines | T18 pots only — **not** a D-shaft |
| 2046 / 2047 / 2048 | Soft-touch T18 (red / white / blue) | 11.0 top, 15.0 base | **NOT SOURCED** | T18 splines | T18 pots only |
| 2056 | Solid machined metal | 25.0 | 15.5 | Ø6 slot, 11 mm keyway | D-shaft |
| 2055 | Scrubber knob | 34.7 | 10.5 | — | encoder |
| 5093 | Slim rubber encoder knob | 11.5 × 14.5 (axes not stated) | — | **Ø6 D-shaft** | PEC11 encoder |

~~**Recommendation: the set-screw knobs (5527/5528/5530).** They are the only
ones that fit **both** the encoder's Ø6 D-shaft and the pot's Ø6 T18 knurl, so
one knob part number serves the whole surface and the recess geometry is
identical everywhere: **Ø20 knob, 12.5 mm tall.**~~

~~**Recess sizing:** a counterbore must clear Ø20 plus finger room — **Ø26–28 is
a sensible pocket** — while the bushing passes through a **Ø7.2** hole in the
≤4 mm floor of that pocket. With a 2.40 mm floor and a 15 mm shaft, the shaft
stands 12.6 mm proud, the 12.5 mm knob sits essentially flush with the pocket
floor, and the pocket depth is then purely an aesthetic choice.~~

**Both retired 2026-11-08, and the first one rested on a claim that has since
been refuted.** The "fits both" argument depended on the pot having a **Ø6 T18
knurl**, which §5.2 now shows is *not* a measured property of the 562 — the
vendor model draws a plain Ø6.000 cylinder — so "one knob serves both shafts"
was never established on the pot side. The printed knob reaches the same goal
honestly: **one file, one outside geometry, two bores**, with the pot's bore
ribbed so it is correct on a round shaft *or* a spline. And the "recess sizing"
paragraph is a v1-panel calculation throughout (2.40 mm floor, Ø26–28 pocket,
knob standing on top of the panel); v2 has a **Ø22.000 bore straight through
12.700 of acrylic** with the knob down inside it `[DXF]`, and the counterbore is
**in the knob**. §5.1.

**What the bought knobs are still good for:** a fallback if printing or casting
the knob fails, and a reference for how tall a commercial knob of this diameter
is. **5527/5528/5530** remain the only ones in the table with a set screw, so
they remain the only ones that would fit a D-shaft *and* a round shaft — on a
deck that let a knob sit above its panel.

---

## 6. STEMMA QT IMU — recommendation: **BNO085, PID 4754**

> **Status 2026-11-08: the recommendation stands, the part is not in the build.**
> The v2 carrier provides **no boss quad and no pocket** for an IMU — the only
> breakout quads in `carrier.stl` are three encoder patterns, one NeoSlider
> pattern and the Feather's two M2.5 bosses `[CAD-MESH]` — and it is not in §11.
> An IMU needs no aperture, so adding one is a wiring and boss change, not a
> slab change. Read this section as *"if an IMU is fitted, fit this one"*.

| | 4754 BNO085 | 4517 LSM6DSOX+LIS3MDL | 4554 ICM-20948 | 4503 LSM6DS3TR-C |
|---|---|---|---|---|
| DoF | 9 | 9 | 9 | **6 — no magnetometer** |
| **On-chip fusion** | **Yes — CEVA. Rotation Vector / quaternion straight off the chip** `[AF]` | No | DMP on silicon, but not exposed by Adafruit's drivers | No |
| Host cost for orientation | **~0 — read a quaternion** | Full AHRS on the ESP32-S3 | Raw in practice | Full AHRS, and no heading reference |
| Extras | Game/Geomagnetic rotation vectors, tap, step, shake | FSM + machine-learning core, tap, pedometer | — | — |
| PCB | **25.400 × 22.860** | 25.400 × 17.780 | 25.7 × 17.7 `[AF]` | 25.3 × 17.8 `[AF]` |
| Holes | Ø2.50 on 20.32 × 17.78 | Ø2.50 on 20.32 × 12.70 | — | — |
| I2C | **0x4A** (0x4B via DI) | 0x6A/0x6B **and** 0x1C/0x1E | 0x69 (0x68 via jumper) | 0x6A/0x6B |
| STEP model | **yes** | **yes** | **no** | **no** |

**Justification.** The brief is *gestural control*, which needs **absolute
orientation, continuously, with low latency, without eating ESP32-S3 cycles**.
Only the BNO085 gives that: it runs the fusion internally and reports a
quaternion. On the 4517 or 4503 the host has to run a Madgwick/Mahony filter at
a few hundred Hz forever — on a device whose CPU is already carrying a live
display, BLE HID, BLE MIDI and (per `STATUS.md`) WiFi/OSC and SSH. The
ICM-20948's DMP exists but Adafruit's libraries do not expose it, so in practice
you are back to host-side fusion.

**Costs, stated honestly:** the 4754 is the **largest** of the four
(25.400 × 22.860 — 5.08 mm deeper than the sensor-grid boards) and the BNO085
is the fussiest to talk to (SH-2 protocol over I2C, and it is known to be
timing-sensitive). Both are acceptable prices for free orientation.

**Second choice: 4517.** Pick it only if you want the machine-learning core for
on-sensor gesture classification *and* you are willing to spend host cycles on
orientation. Note it occupies **two** I2C addresses.

Geometry for both is in §1 and both have STEP models (§11). Neither has
anything below the PCB; tallest feature is the QT sockets at 2.96 mm.

---

## 7. STEMMA QT time-of-flight — recommendation: **VL53L4CD, PID 5396**

> **Status 2026-11-08: the recommendation stands, the part is not in the build,
> and unlike the IMU it cannot be added quietly.** A ToF has to see out, and
> `slab.dxf` carries **no sensor window**: its sixteen loops are the outline,
> the keyboard void, six Ø22.000 bores, the fader slot, six fastener holes and
> one fiducial `[DXF]`. Fitting one is a **slab change** — a re-cut sheet — and
> the 18° FoV argument below is also an argument about where that window can go.
> Read this section as *"if a ToF is fitted, fit this one"*. It is not in §11.

| | 5396 **VL53L4CD** | 5425 VL53L4CX | 3967 VL53L1X | 3316 VL6180X |
|---|---|---|---|---|
| Range | **~1 – 1300 mm**; 0–1200 at full FoV | ~1 – 6000 mm | ~30 – 4000 mm | 5 – 100 mm (150–200 in good light) |
| **Minimum distance** | **~1 mm** | ~1 mm | **~30 mm** | 5 mm |
| Short-range linearity | **down to 1 mm** | down to 10 mm | — | — |
| **FoV** | **18° diagonal** | 18° | **27°** (programmable ROI can narrow it) | wide, short |
| **Update rate** | **up to 100 Hz** | — | up to 50 Hz | — |
| Multi-target | no | yes | no | no |
| PCB | 25.400 × 17.780 | — | 25.400 × 17.780 | 25.400 × 17.780 |
| I2C | 0x29, software-settable | 0x29 | 0x29, software-settable | **0x29, "you can't change it"** `[AF guide]` |
| STEP model | **yes** | **no** | **yes** | **yes** |

All figures `[AF]` product pages / Learn guides.

**Justification against "theremin-style but accurate and constrained":**

- **Constrained** — 18° is the narrowest FoV on offer. At a 300 mm hand height
  that is a ~95 mm-wide cone; the VL53L1X's 27° is ~145 mm and will pick up
  your other hand, the keyboard and your chest. Narrow FoV is what stops a ToF
  theremin feeling possessed.
- **Accurate** — 1 mm linearity down to 1 mm is the headline. A theremin
  gesture lives in the 20–400 mm band, which is exactly where the VL53L1X is
  *blind* (its floor is ~30 mm) and where the L4CD is at its best.
- **Fast** — 100 Hz vs the L1X's 50 Hz. For a continuous musical controller the
  difference between 10 ms and 20 ms of sensor latency is audible as lag.
- **Not flaky** — these are FlightSense parts with ambient-light rejection; the
  L4CD's 1300 mm ceiling means it is not straining at the top of its range for
  a hand 300 mm away, which is where cheap IR rangefinders go noisy.

**Rejected:** the **VL53L4CX** buys 6 m you do not want and *loses* short-range
linearity (10 mm vs 1 mm) — and it has **no 3D model**. The **VL53L1X** cannot
see closer than ~30 mm. The **VL6180X** tops out around 100 mm, which is too
short for a hand-above-the-deck gesture, **and its address cannot be changed**.

**If you want a second ToF axis,** take a second 5396 and sequence them with
XSHUT — both sit at 0x29 (§8).

**Flag — stale vendor dimension.** Adafruit's 3316 page states
**20.5 × 18.0 × 3.0 mm**, but the vendor STEP measures **25.400 × 17.780 ×
1.570** with QT sockets 2.96 above. The page figure looks like the
pre-STEMMA-QT board revision. Trust the STEP; verify on receipt. `[AF vs STEP]`

---

## 8. I2C bus audit

> **Superseded as the bus plan, 2026-11-08 — `v2/parts/wiring.md` owns it now.**
> This section audits the **nine-device surface this document was written for**.
> The build has **four** I²C devices: three 5880 encoders at 0x36/0x37/0x38 and
> one NeoSlider at 0x30, by **two solder-closed jumpers and no knife work**
> (`wiring.md` §2.3). The audit method, the jumper semantics and the
> cross-family overlap finding are all still correct and worth keeping — that
> overlap is exactly why the four addresses were allocated deliberately rather
> than by default. What is **not** current is the device list, the counts, and
> the §8.3 map. `wiring.md` §7 already records three corrections to this
> section; they are folded in below.

### 8.1 Addresses as shipped

**Qty column corrected 2026-11-08.** The struck figures are what this document
surveyed; the bold figures are what the v2 carrier provides for (§0.0).

| Device | Qty | Default | Available range | How to change |
|---|---|---|---|---|
| Rotary encoder 4991 / 5880 | 3 | **0x36** | **0x36 – 0x3D** (8) | 3 jumpers A0/A1/A2, **soldered CLOSED** to add 1/2/4 `[AF]` |
| NeoSlider 5295 | ~~2~~ **1** | **0x30** | **0x30 – 0x3F** (16) | 4 jumpers A0–A3, **CUT** to add 1/2/4/8 `[AF]` — **and at ×1 it never has to be cut: it stays at its shipped 0x30** |
| NeoKey 1×4 4980 | ~~1~~ **0** | **0x30** | **0x30 – 0x3F** (16) | 4 jumpers A0–A3, **soldered CLOSED** to add 1/2/4/8 `[AF]` — **removed from the design, §4** |
| BNO085 4754 | ~~1~~ **0** *(not placed)* | **0x4A** | 0x4A / 0x4B | DI pin high `[AF]` — §0 item 5 |
| VL53L4CD 5396 | ~~1~~ **0** *(not placed)* | **0x29** | any, in software | set at runtime; needs XSHUT to sequence duplicates `[AF]` — §0 item 6 |
| ADS1115 1085 | ~~1~~ **0** | **0x48** | 0x48 – 0x4B (4) | ADDR jumper `[AF]` — **not needed, §8.4** |
| *(alt)* Quad Rotary 5752 | **0** | **0x49** | **0x49 – 0x50** (8) | 3 jumpers, **CUT** to add `[AF]` — not chosen, §2.4 |

### 8.2 What actually collides, out of the box

**Items 2, 4 and 5 below were about parts that are no longer in the build;
items 1 and 3 are live and item 3 is the one that mattered.**

1. **Three encoders all arrive at 0x36.** Three-way collision. Solved by
   jumpers — eight addresses for three devices, no contest. **Live, and this is
   what `wiring.md` §2.3 does.**
2. ~~**NeoSlider ×2 and NeoKey ×1 all arrive at 0x30.** Three-way collision.~~
   **Gone: one slider, no NeoKey.** With a single 5295 nothing else wants 0x30,
   so the slider is never jumpered — which matters, because its jumpers are
   **cut** and cutting is one-way (§8.3's own warning).
3. **Cross-family:** the encoders' 0x36–0x3D band sits **inside** the
   0x30–0x3F band the sliders and key strip use. Six seesaw devices are
   competing for one 16-address window. It fits, but only if you allocate
   deliberately — this is the one that bites people.
4. **ADS1115 (0x48–0x4B) vs BNO085 (0x4A–0x4B)** — they overlap. Leave the
   ADS1115 at 0x48 and do not jumper it.
5. **ADS1115 vs Quad Rotary (0x49)** — fine at defaults; do not move the
   ADS1115 to 0x49 if you use the 5752.

### 8.3 Proposed map — SUPERSEDED by `v2/parts/wiring.md` §2.3

**The current map, four devices** `[REPO wiring.md §2.3]`:

| Address | Device | Jumper action |
|---|---|---|
| 0x30 | NeoSlider 5295 | **none — as shipped, and never cut** |
| 0x36 | Encoder 5880 #1 | none |
| 0x37 | Encoder 5880 #2 | **close A0** (solder, reversible) |
| 0x38 | Encoder 5880 #3 | **close A1** (solder, reversible) |

**The nine-device map this section proposed, retired 2026-11-08:**

| Address | Device | Jumper action |
|---|---|---|
| ~~0x29~~ | ~~VL53L4CD ToF~~ | ~~none~~ — not placed |
| 0x30 | NeoSlider A → **the only slider** | **none — as shipped** |
| ~~0x31~~ | ~~NeoSlider B~~ | ~~**cut A0**~~ — there is no slider B, and this cut would have been irreversible |
| ~~0x32~~ | ~~NeoKey 1×4~~ | ~~**close A1**~~ — removed, §4 |
| 0x36 | Encoder 1 | none |
| 0x37 | Encoder 2 | **close A0** |
| 0x38 | Encoder 3 | **close A1** |
| ~~0x48~~ | ~~ADS1115 (3 pots)~~ | ~~none~~ — not needed, §8.4 |
| ~~0x4A~~ | ~~BNO085 IMU~~ | ~~none~~ — not placed |

Note what the correction **removes**: a **cut** jumper on a slider that does not
exist. That was the one destructive, one-way operation in the whole plan.

**One address hazard this section did not know about, and it is not in either
map:** the Waveshare ESP32-S3-RLCD-4.2 puts **four devices of its own** on the
same SDA/SCL pair — a PCF85063 RTC, an SHTC3, an ES8311 and an ES7210 — and
`wiring.md` §2.2 places the RTC at **0x51** and the SHTC3 at **0x70**, clear of
0x29–0x4A but *unverified* `[REPO]`. **Check the board's own four before
jumpering anything.**

~~Nine devices, nine unique addresses, four jumper operations, **no PCA9546/9548
needed**. If you take the **5752 Quad Rotary** instead of three 5880s, drop
0x36–0x38 and add **0x49** — eight devices, two jumper operations.~~
**Four devices, four unique addresses, two reversible jumper operations, and
still no multiplexer.** The conclusion was right for a reason that no longer
applies.

**Two practical warnings:**

- **NeoSlider jumpers are CUT, not soldered.** That is destructive and
  one-way. **Decide the slider addresses before touching a knife.** The NeoKey
  and the encoders use solder-closed jumpers, which are reversible. The quad
  encoder is cut-to-change like the slider.
- ~~**Bus loading.** Adafruit STEMMA QT boards generally carry their own SDA/SCL
  pull-ups; nine boards in parallel divides the effective pull-up resistance by
  roughly nine and adds nine connectors' worth of capacitance plus cable. I did
  **not** confirm the resistor value on each board. Check the schematics before
  committing to 400 kHz, and be ready to run the chain at 100 kHz or lift
  pull-ups on all but one or two boards.~~ `[NOT SOURCED — verify]` →
  **NOW SOURCED, and half refuted.** `v2/parts/wiring.md` §2.4 read the vendor
  schematics: the **Waveshare board already fits 2.2 kΩ** (R20/R22), which
  dominates everything else, so with three encoders and the slider the effective
  pull-up is **1170 Ω sinking 2.48 mA at V_OL = 0.4 V** — inside the 3 mA I²C
  limit. **Cut nothing, lift nothing.** The headroom is exactly one more 10 kΩ
  board. But **100 kHz is still the right speed**, for a different reason this
  section did not find: the **BSS138 level shifters on the encoders** put
  135–270 pF on each line before any cable (`wiring.md` §0 item 7, §2.5)
  `[REPO]`. Right answer, wrong reason — recorded that way deliberately.

### 8.4 The three pots are ANALOGUE — what reads them: **(a), REVERSED to native ADC1**

> **This subsection recommended (b), the ADS1115. The decision went the other
> way and the ADS1115 is off the BOM.** `v2/parts/wiring.md` §3.3 reversed it
> *"with reasons"*, and `v2/parts/wiring.md` §7 records the reversal against this
> very subsection `[REPO]`. The live geometry agrees: `v2/cad/carrier.scad`
> cuts an analogue trunk below the pot row and three stubs up to it, commented
> **"GPIO1/2/3 = ADC1_CH0/1/2, the ADC unit that keeps working while Wi-Fi is
> up. No ADS1115."**, and `parameters.scad` §9 says the same `[REPO]`.
>
> **Why (b) lost, in one line:** the destination is **7-bit MIDI CC**, and the
> S3's 12 bits with ±4 LSB DNL is **32 counts per CC step** — so the resolution
> argument, which was the whole case for 16 bits, collapses. The ADS1115 would
> also have added a fifth 10 kΩ pull-up pair to a bus with room for exactly one
> more (§8.3), and it does **not** remove the three analogue runs; it only
> relocates them.
>
> **This subsection's ADC2/Wi-Fi warning was correct and is the reason the
> answer is GPIO1/2/3 specifically** — `wiring.md` §7 says so in those words.
> That is the part of (a) below that was load-bearing, and it survived.
>
> **What (a) costs, carried forward as a live risk and not waved away:** the
> SAR ADC's noise and non-linearity are real, and the mitigation is **one
> 1.5 kΩ resistor in each pot's top leg plus one 100 nF to ground at the header
> end** — three of each, the only passives in the build (`wiring.md` §3.5).
> Without the 1.5 kΩ the top **36° of a 300° sweep** reads full-scale.

**(a) ESP32-S3 native ADC — THE CHOICE.** Three ADC1 channels (GPIO1–GPIO10).
Free, no extra board. But ADC2 is unusable while the radio is up — and this
device runs WiFi, BLE HID and BLE MIDI (`STATUS.md`) — so you are confined to
ADC1, and the S3's SAR ADC is noisy and non-linear enough that a slow knob sweep
visibly steps. It also means **three analogue runs across the enclosure** to the
main board, picking up noise from the display and the radio.

**(b) ADS1115 over STEMMA QT — ~~recommended~~ NOT CHOSEN.** 16-bit, 4 channels
(one spare), programmable gain, up to 860 SPS `[AF]`. The three pots wire
locally to the ADS1115's 0.1″ header, and **one QT cable** joins it to the chain
that already carries everything else. Survey kept; see the banner.

| ADS1115 breakout, PID 1085 | Value | Source |
|---|---|---|
| PCB | **25.400 × 17.780 × 1.570**, R2.54 | `[STEP]` |
| Mount holes | 4 × Ø**2.50** on **20.32 × 12.70** | `[STEP]` |
| Above front face / below back face | 0 / **2.96** (QT sockets) | `[STEP]` |
| STEMMA QT | 2 sockets, two short ends, back face, facing outward | `[STEP]` |
| I2C | **0x48**, four addresses 0x48–0x4B | `[AF]` |
| Vendor-stated size | 25.4 × 17.8 × 4.6 — agrees with the STEP | `[AF]` |

~~**Which is more coherent: (b).**~~ **The coherence argument, and why it did not
survive.** Everything else on this deck is STEMMA QT.
The ADS1115 keeps the pots on the same chain, the same cable type and the same
`i2cdetect` map as the rest of the surface, replaces three noise-prone analogue
runs with one differential-ish local cluster, and gives 16 bits instead of a
noisy 12. The cost is one more 25.40 × 17.78 board inside the enclosure and one
more address — ~~both already budgeted in §8.3~~ **and neither is budgeted any
more: there is no boss quad for it in `carrier.stl` and no 0x48 in the live map**
`[CAD-MESH; REPO]`. The 16-bits-for-free trade is what §3.3 of `wiring.md`
weighs against 7-bit MIDI, and it is the step that decided it.

---

## 9. 3D models — what exists

| Part | PID | STEP | STL | Saved to `v2/parts/models/` |
|---|---|---|---|---|
| QT Rotary Encoder | 4991 | ✅ | ✅ | `4991 QT Rotary Encoder.step` / `.stl` |
| Rotary Encoder + encoder | 5880 | ✅ | — (3MF) | `5880 Rotary Encoder I2C STEMMA QT.step` |
| Quad Rotary Breakout | 5752 | ✅ | ✅ | `5752 Quad Rotary Breakout.step` |
| NeoSlider | 5295 | ✅ | ✅ | `5295 NeoSlider Stemma.step` |
| NeoKey 1×4 QT | 4980 | ✅ | ✅ | `4980 NeoKey 1x4 QT.step` / `.stl` |
| BNO085 IMU | 4754 | ✅ | ✅ | `4754 BNO085 STEMMA QT.step` |
| LSM6DSOX + LIS3MDL | 4517 | ✅ | ✅ | `4517 LSM6DSOX-LIS3MDL.step` |
| VL53L4CD ToF | 5396 | ✅ | ✅ | `5396 Adafruit VL53L4CD.step` |
| VL53L1X ToF | 3967 | ✅ | ✅ | `3967 VL5SL1X TOF Sensor.step` |
| VL6180X ToF | 3316 | ✅ | — (3MF) | `3316 VL6180 ToF Sensor.step` |
| ADS1115 ADC | 1085 | ✅ | — (3MF) | `1085 ADS1115 ADC.step` |
| 16 mm panel pot | 562 | ✅ | — | `562 10K Potentiometer.step` |
| **ICM-20948 IMU** | 4554 | ❌ | ❌ | **NO MODEL — needs measurement** |
| **LSM6DS3TR-C IMU** | 4503 | ❌ | ❌ | **NO MODEL — needs measurement** |
| **VL53L4CX ToF** | 5425 | ❌ | ❌ | **NO MODEL — needs measurement** |
| **MX keyswitch** | — | ❌ | ❌ | **NO MODEL** — but fully dimensioned by the Cherry datasheet (§4) |
| **Slide-pot nubbin** | 5621 / 5741 | ❌ | ❌ | **NO MODEL — needs measurement** |
| **9 / 16 mm panel pots** | Alpha | ❌ | ❌ | **NO MODEL** — dimensioned by datasheet (§5) |

Source: `github.com/adafruit/adafruit_cad_parts` (shallow clone at
`/home/user/adafruit/adafruit_cad_parts`). All three missing PIDs — 4554, 4503,
5425 — were confirmed absent from that library by direct search.

**Two additions to this table's scope, 2026-11-08.** (i) `v2/parts/models/` now
also holds the models three *other* documents depend on — `5710 Feather RP2040
DVI`, `keychron-k9pro` and `k9pro-internals`, and `waveshare-rlcd42` — owned by
`hdmi-feather.md`, `keyboard-strip.md` and `board-models.md` respectively, not
by this file. (ii) The v2 figures added to this document in this pass were
measured off **meshes**, either the exported parts in `v2/export/` or the house
vendor meshes in `/home/user/kylebsmith/cmtl-3d-assets/02_controls/` — the same
files `v2/cad/parameters.scad` §7 cites. §12 gives the procedure.

**`v2/parts/models/` is gitignored** (".gitignore: *v2: downloaded vendor CAD
is reference material, not ours to redistribute*"), so those files are local
working copies only. **That is why every dimension this document depends on is
written out above rather than left in the model** — a fresh clone has the
numbers even with no models present. Re-fetch with:

```
git clone --depth 1 https://github.com/adafruit/adafruit_cad_parts
```

Datasheets are saved beside them in `v2/parts/models/datasheets/` (also
ignored): Bourns PEC11 and PEC11L, Bourns PTV09, Cherry MX series.
Bourns PTD90 and the Alpha RD901F / RV16AF / RD1601F PDFs were fetched from
`bourns.com/docs/product-datasheets/` and
`taiwanalpha.com/downloads?target=products&id=…`.

---

## 10. NOT SOURCED — get the calipers

Stated loudly, per the project's rule that an unsourced number is a bug.

**Reviewed 2026-11-08.** The numbering below is unchanged, because other
documents cite these by number (`v2/parts/wiring.md` §7 cites item 7). Each item
now carries its status: **CLOSED**, **MOOT** (the part is not in the build) or
**STILL OPEN**. Items 10–12 are new in this pass.

1. **CLOSED — 67.000 mm `[MEASURED, owner's calipers 2026-11-08]`.**
   ~~**NeoSlider (5295) travel** — electrical and mechanical, in mm. Not on the
   product page, not in the Learn guide, not derivable from the STEP (the model
   does not cut the actuator slot). **This is the single most important missing
   number in this document** — it sets the slider band's length and the usable
   resolution of two of the deck's primary controls.~~ §3. It was correctly the
   most important missing number: it sets the slot length, and at 67.000 the
   slot wants to be 79.000 against the 52.000 presently cut, which overruns the
   control band by 1.3975 at each end (§3 Note 3). The reasoning about *why the
   model cannot give it* is kept in §3 and is still right.
2. **MOOT — the cap is printed.** ~~**NeoSlider nubbin (5621 / 5741)** — bore
   depth, and therefore how far the finished knob stands above the shank tip.~~
   `fader_cap.stl` replaces it (§3), so there is no vendor nubbin bore to
   measure. The 29.8-vs-20.03 discrepancy the item was derived from is still
   explained in §3 and still worth knowing.
3. **STILL OPEN, for a new reason.** **PEC11 encoder nut and washer** —
   across-flats and thickness. The datasheet says they are supplied but does not
   dimension them. Until measured, use the Bourns PTV09 hardware as a proxy
   (nut 11.0 A/F × 2.0, washer 0.4). ~~and treat the 4.0 mm panel budget as
   provisional~~ — there is no panel budget in v2. What it gates now is the
   **printed knob's Ø12.000 counterbore**, drawn 2.000 over a nominal M7 nut's
   Ø10 across the flats `[REPO]`: a supplied nut wider than Ø12.000 across the
   flats stands the knob off the slab, visibly. *Procedure: calipers across the
   flats and the thickness of the nut and washer that arrive with the 5880.*
4. **MOOT for v2, open for a 4980.** ~~**Keycap height** for whatever caps you
   actually buy.~~ v2 buys no keycaps: the K9 Pro brings its own, measured at
   **9.7572** above the plate top `[REPO keyboard-strip.md]`. Cherry's figures
   (§4) still apply to Cherry caps, and OEM/DSA/XDA/SA still differ by
   millimetres, for anyone fitting caps to a 4980.
5. **MOOT — the knobs are printed (§5.5).** ~~**Soft-touch knob height
   (2046/2047/2048)** — Adafruit gives both diameters but no height.~~ Still a
   real hole in Adafruit's data; kept as such.
6. **MOOT — the knobs are printed (§5.5).** ~~**5093 encoder knob** — the page
   says "11.5 mm × 14.5 mm" without saying which is diameter and which is
   height.~~ Also still a real hole in Adafruit's data.
7. **CLOSED for the four boards in the build.** ~~**Per-board I2C pull-up values**
   across all nine boards (§8.3).~~ Read off the vendor schematics in
   `v2/parts/wiring.md` §2.4: the Waveshare's own 2.2 kΩ dominates, the
   effective pull-up is 1170 Ω at 2.48 mA, and **nothing should be lifted**
   `[REPO]`. §8.3.
8. **MOOT — the 5752 was not chosen (§2.4).** ~~**5752 NeoPixel visibility** with
   an encoder fitted.~~ The live equivalent of this question is item 12.
9. **STILL OPEN, but it gates nothing.** **Adafruit 562 bushing thread** — the
   model gives **Ø6.2846 major / Ø5.3938 minor over 6.600** `[HOUSE-MESH]`,
   undersize for the M7 × 0.75 the Alpha datasheet specifies, which is normal
   for a vendor model. ~~Check with a thread gauge before cutting a panel hole,
   or just drill Ø7.2 and let the nut decide.~~ The carrier's web bore is
   already cut at **Ø7.200** `[CAD-MESH]`, which is the "drill Ø7.2" branch, so
   the decision is made. *Procedure: thread gauge on the real bushing.*
10. **NEW — the PEC11's D-flat width, and it is on one datasheet line.**
    `[DS PEC11 p2]` says Ø6.0 flatted to **4.5**; the printed knob's bore is cut
    to **4.600** = 4.5 + 0.100 `[REPO]`. The **vendor model does not model the
    flat at all** — the 5880's shaft is a plain Ø6.000 cylinder over its full
    7.948 mm, with only a 0.600 mm relief at the root and a tip chamfer
    `[HOUSE-MESH]`, §2.3. So the knob's one key feature has **no independent
    confirmation**, and if the real flat is shallower than 4.500 the knob will
    not seat. *Procedure: calipers across the flat of a real 5880 shaft, plus
    how far up the shaft the flat runs — the knob's D section spans z
    6.350…14.350 of the bore and needs flat across that window.*
11. **NEW — the 562's real shaft form: knurl, spline or plain.** §5.2's "T18
    knurled" was read off a tessellation artefact and is withdrawn; the model
    shows a perfect Ø6.000 cylinder `[HOUSE-MESH]`. The printed pot knob's six
    ribs were chosen precisely so this need not be known — but it should be.
    *Procedure: look at the shaft, count the teeth, caliper the crest diameter.*
12. **NEW — whether a NeoPixel reads through 12.700 mm of clear acrylic from
    under a Ø25.400 knob brim.** v2 cuts no light holes (§2.2) and the brim's
    12.700 mm radius covers the 5880's pixel, which sits 10.41 mm off the knob
    axis. *Procedure: power one 5880 under an offcut of the real sheet with a
    knob fitted, in room light. A look-at-it test, not a measurement.*

**Fetch from a distributor (blocked from this environment):**

13. **Alps RK09** — alpsalpine.com returns 403 on every path tried.
    *(Was item 10 before this pass; renumbered because items 10–12 above are
    new. Nothing cites it by number.)*
14. **Bourns PTV09A-6 and PTD901 bushing lengths** — present on the drawings
    but not legible enough to assign with confidence at the resolution
    available. *(Was item 11.)* **And note the PEC11 precedent from §2.3: a
    bushing length read off an option table rather than off the part was wrong
    by 2.000 mm there, so these two should be measured, not re-read.**

---

## 11. Shopping list — v2 control surface

**Rebuilt 2026-11-08.** The previous list is kept below it, struck, because it
is the record of what was researched and five of its lines are now wrong in a
way a reader would have spent money on.

### 11.1 Buy this

| Qty | Part | PID / PN | Why this qty |
|---|---|---|---|
| **3** | I²C STEMMA QT Rotary Encoder **with Encoder** | Adafruit **5880** | three 20.320 × 20.320 boss quads at y 68.0125 `[CAD-MESH]` |
| **1** | NeoSlider | Adafruit **5295** | one Pattern-B boss quad and one slot `[CAD-MESH; DXF]` |
| **3** | Nut-mount pot, **10 kΩ linear (taper B)**, M7 × 0.75 bushing ≥6 mm, **Ø6 non-D shaft ≥15.6 mm**, terminals inside §5.4 | Adafruit **562** (10 kΩ linear) — the part every v2 figure was measured from | three Ø7.200 carrier webs at y 29.9125 `[CAD-MESH]` |
| **1** | STEMMA QT JST-SH 4-pin **to male headers, 150 mm** | Adafruit **4209** | the Waveshare board has **no QT socket**: this is the whole transition from its 2 × 8 header to the chain `[REPO wiring.md §5]` |
| **2** | STEMMA QT cable, **50 mm** | Adafruit **4399** | ENC1→ENC2 and ENC2→ENC3: **12.700 mm** board-edge to board-edge each `[DERIVED from CAD-MESH]` |
| **1** | STEMMA QT cable, **100 mm** | Adafruit **4210** | ENC3→slider: the carrier routes it **87.630 mm** — 48.260 in Y then 39.370 in X `[DERIVED from CAD-MESH + REPO route]` |
| **3** | Resistor **1.5 kΩ**, 1 % | — | pot top leg; without it the top **36° of a 300° sweep** reads full-scale `[REPO wiring.md §3.5]` |
| **3** | Capacitor **100 nF** | — | one per ADC input, at the header end `[REPO wiring.md §6.1]` |
| **1** | Header, **2 × 8 male 2.54 mm** | — | mates with the Waveshare P1 `[REPO wiring.md §6.3]` |

**Printed or cast, not bought:** 3 × `knob_encoder`, 3 × `knob_pot`,
1 × `fader_cap` (§5.5, §3), plus the carrier, slab, back panel, cowl and feet —
`v2/docs/fabrication.md` owns those and the fasteners and heat-set inserts.

**Supplied with the keyboard, not bought:** the Keychron K9 Pro arrives with its
own 61 switches, 61 hot-swap sockets, keycaps, aluminium frame and cell
(`v2/parts/keyboard-strip.md`).

**Four I²C devices, one chain, no multiplexer, two reversible solder jumpers, no
knife work.**

### 11.2 The previous list, and what each struck line would have cost you

| Qty | Part | PID / PN | Verdict |
|---|---|---|---|
| 3 | I2C STEMMA QT Rotary Encoder **with Encoder** | Adafruit **5880** | **unchanged — correct** |
| ~~*or* 1~~ | ~~I2C Quad Rotary Breakout + 4 × Bourns PEC11~~ | ~~Adafruit **5752** + **377** ×4~~ | **not chosen** — its 19.050 pitch caps the knob; §2.4 |
| ~~2~~ **1** | NeoSlider | Adafruit **5295** | **one too many.** §3 |
| ~~2–6~~ **0** | ~~Slide-pot nubbin~~ | ~~Adafruit **5621** (3-pack)~~ | **the cap is printed.** §3 |
| ~~1~~ **0** | ~~NeoKey 1×4 QT~~ | ~~Adafruit **4980**~~ | **removed from the design.** §4 |
| ~~4~~ **0** | ~~Cherry MX-compatible keyswitches + keycaps~~ | — | **the K9 Pro brings 61 of each.** §4 |
| 3 | Panel-mount pot, **10 KΩ linear** | ~~**Alpha RD901F-20B6-15F-B10K** (10 mm bushing)~~ *or* Adafruit **562** | **the 10 mm bushing is pointless here** — the nut clamps a 3.810 web. §5.1, §5.3 |
| ~~3+~~ **0** | ~~Knob, Ø20 × 12.5, Ø6 set-screw~~ | ~~Adafruit **5527 / 5528 / 5530**~~ | **six knobs are printed**, and the "fits both shafts" case rested on a refuted T18 claim. §5.2, §5.5 |
| ~~1~~ **0** | ~~ADS1115 4-ch 16-bit ADC~~ | ~~Adafruit **1085**~~ | **not needed** — native ADC1 on GPIO1/2/3. §8.4 |
| ~~1~~ **0** *(not placed)* | 9-DoF IMU with on-chip fusion | Adafruit **4754** (BNO085) | **the comparison stands; no mounting exists.** §0 item 5 |
| ~~1~~ **0** *(not placed)* | Time-of-flight sensor | Adafruit **5396** (VL53L4CD) | **the comparison stands; and it needs a slab window that is not cut.** §0 item 6 |
| ~~~10~~ **4** | STEMMA QT cables | ~~50/100/200 mm, Adafruit 4399 / 4210 / 4401~~ → **4209 ×1 + 4399 ×2 + 4210 ×1** | ~10 was a nine-device estimate. §11.1 |

~~Nine I2C devices, one chain, no multiplexer.~~

**One discrepancy left open rather than decided:** `v2/parts/wiring.md` §6.4
budgets **4** daisy-chain QT cables on top of the 4209, while its own
connection list in §6.1 names **three** chain hops (ENC1→ENC2, ENC2→ENC3,
ENC3→slider). §11.1 buys the three the connection list needs; buy a fourth as a
spare if you want wiring.md's figure. Not resolved here because `wiring.md`
owns it.

---

## 12. How the v2 figures in this document were measured

Every v2 figure added on 2026-11-08 was taken off an **emitted artefact**, never
off the source that produced it — a figure computed from the same parameters as
the thing it describes would check nothing. Re-runnable, with
`/usr/bin/python3.11`:

| Tag | File read | Method |
|---|---|---|
| `[DXF]` | `v2/export/dxf/slab.dxf` | `ezdxf.recover.readfile`, then every entity's layer, vertex count and vertex bounding box. 16 loops: `CUT_OUTER` ×1, `CUT_INNER` ×14, `FIDUCIAL` ×1. OpenSCAD emits circles as 96-vertex polylines, so a diameter is that polygon's extent — read as ±0.01. |
| `[CAD-MESH]` | `v2/export/stl/*.stl` | `trimesh.load(..., process=False)` then **`merge_vertices()` first** — without it every part here reports `is_watertight False`. Plan features from `mesh.section(plane_origin=[0,0,z], plane_normal=[0,0,1])`, mapped back to world XY through the section's own 4 × 4 transform, each region reported as outer bbox plus hole bboxes. Z extents from `mesh.contains()` along a sampled column at **0.005 mm**, so these carry ±0.005. Interference from `trimesh.boolean.intersection(..., engine='manifold')` and the intersection's volume. |
| `[HOUSE-MESH]` | `/home/user/kylebsmith/cmtl-3d-assets/02_controls/adafruit_{562,5295,5880}_*/*.stl` | Same loader. Radii taken about an axis **derived from the part**, not assumed: for the 5880 the bushing band's bbox centre, which lands on (12.700, 12.700) — the board centre. Shaft form tested by the **set of distinct vertex radii** in a z band, not by a bounding box. |

**Two traps worth recording, because the first one nearly went into this file as
a finding.**

1. **`mesh.contains(vertices)` is not an interference test.** Asked whether the
   562 fouls the carrier, a vertex-containment test returned **0 of 5976
   vertices inside** at the intended orientation — and the answer is wrong. The
   pot's terminals are flat tabs whose only vertices are at their ends, so a tab
   crosses a 2.540 mm wall with no vertex ever landing inside it. The real
   answer, from a boolean, is **4.4425 mm³ of interference** (§5.4). *A
   containment test on vertices measures the vertices, not the solid.*
2. **A bounding box is not a shaft form.** A tessellated Ø6.000 cylinder has a
   bbox *under* 6.000, because an inscribed polygon's extremes fall short — which
   is how 5.90 × 5.93 got written down as evidence of a T18 knurl (§5.2). The
   test that works is the **set of distinct radii**: one value means a cylinder,
   two or more mean a form.
