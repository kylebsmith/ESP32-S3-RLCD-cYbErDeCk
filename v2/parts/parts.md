# v2 parts, dimensions and tolerances

*An enclosure input. Every number below carries its source tag. Where a number
could not be traced to a primary source it is marked **NOT SOURCED** and listed
in §10 as a caliper job — it is not estimated.*

---

## 0. Verdict up front

1. **"Neo-Encoder" resolves to two PIDs, and one of the user's is wrong.**
   **4991** is the breakout *with NeoPixel, no encoder fitted*; **5880** is the
   same board *with a Bourns PEC11 encoder pre-soldered* (it has the NeoPixel
   too). **5881 is not an encoder — it is a pack of 100 anti-static bags.**
   `[AF prod-5881]` For ×3, buy **5880** unless you want a specific encoder feel.

2. **Three encoders on one bus do not collide — the panel does.** The address
   jumpers give eight addresses (0x36–0x3D) `[AF pinouts]`, so three is trivial.
   The real constraint is that each 5880 is a **25.4 mm square board** whose
   knob must land on a 19 mm-ish musical pitch. If the three rotaries sit in a
   row, **5752 Quad Rotary** (four encoders on one 76.2 × 21.59 mm board, one
   I2C address, 19.05 mm pitch) is the better buy — §2.3.

3. **The whole control surface shares two PCB outlines.** The NeoSlider
   (5295), NeoKey (4980) and Quad Rotary (5752) are **all 76.200 × 21.590 mm
   with the same 38.10 × 16.51 mm, Ø2.50 mm hole pattern** `[STEP ×3]`. The
   sensors, ADC and single encoders are all on the 25.40 mm-wide Adafruit
   breakout grid. **Two boss patterns carry the entire surface.**

4. **Panel-mount pots: the bushing is the binding constraint, and it is
   orderable.** A stock 16 mm Alpha bushing is M7 × 0.75 × **6.5 mm**, which
   allows a panel of only **≈4.0 mm** once the supplied nut and washer are
   accounted for. The **Alpha RD901F 9 mm** series sells the same pot with a
   **Ø7 or Ø9 bushing in 5 / 7 / 10 mm lengths** `[DS RD901F p1]`. The 10 mm
   option (suffix **B6** or **E6**) permits a **≈7.5 mm** panel — that is the
   part that makes a recessed pocket possible **without a counterbore**. With
   anything else, a recess deeper than ~1.5 mm **must** be a counterbore. §5.

5. **IMU: BNO085 (4754).** It is the only one of the four that does the fusion
   on-chip and hands you a quaternion. §6.

6. **ToF: VL53L4CD (5396).** 18° FoV, 1 mm short-range linearity, 100 Hz —
   "accurate and constrained" is literally its spec sheet against the others. §7.

7. **No I2C multiplexer is needed.** Nine devices, all unique, map cleanly. §8.

---

## 0.1 How these numbers were produced

Source tags used throughout:

| Tag | Meaning |
|---|---|
| `[STEP]` | **Measured** by me from the vendor STEP file with an OpenCASCADE kernel — per-solid optimal bounding boxes and cylindrical-face radii. Highest confidence. |
| `[FAB]` | Adafruit fab print (vendor board drawing), read directly. |
| `[DS]` | Manufacturer datasheet PDF. |
| `[AF]` | Adafruit product page or Learn guide text. |

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
| Quad Rotary ("NeoRotary 4") | 5752 | 76.200 × 21.590 | 4 × Ø2.50 on 38.10 × 16.52 | **21.40** (encoders not supplied) | **4.30** (right-angle QT) | 0x49 |
| NeoSlider | 5295 | 76.200 × 21.590 | 4 × Ø2.50 on 38.10 × 16.52 | **15.50** to shank tip | 2.96 | 0x30 |
| NeoKey 1×4 QT | 4980 | 76.200 × 21.590 | 4 × Ø2.50 on 38.10 × 16.51 | 0 bare; **11.6** w/ MX `[DS]` | 2.96 | 0x30 |
| BNO085 IMU | 4754 | 25.400 × 22.860 | 4 × Ø2.50 on 20.32 × 17.78 | 0 | 2.96 | 0x4A |
| LSM6DSOX + LIS3MDL | 4517 | 25.400 × 17.780 | 4 × Ø2.50 on 20.32 × 12.70 | 0 | 2.96 | 0x6A + 0x1C |
| VL53L4CD ToF | 5396 | 25.400 × 17.780 | 4 × Ø2.50 on 20.32 × 12.70 | 0 | 2.96 | 0x29 |
| VL53L1X ToF | 3967 | 25.400 × 17.780 | 4 × Ø2.50 on 20.32 × 12.70 | 0 | 2.96 | 0x29 |
| VL6180X ToF | 3316 | 25.400 × 17.780 | 4 × Ø2.50 on 20.32 × 12.70 | 0 | 2.96 | 0x29 |
| ADS1115 ADC | 1085 | 25.400 × 17.780 | 4 × Ø2.50 on 20.32 × 12.70 | 0 | 2.96 | 0x48 |

All `[STEP]`. Every board has a **2.54 mm corner radius** and every mounting
hole sits **2.54 mm in from both adjacent edges** `[STEP]`.

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

**Design consequence:** one 38.10 × 16.51 boss quad accepts the slider, the key
strip *and* the quad encoder interchangeably. Lay the deck out on that module
and the arrangement study in `v2/docs/envelope.md` can swap bands without
re-cutting bosses.

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
  → **the panel needs a Ø2.5–3.0 mm light hole 10.41 mm off the knob axis**,
  or a light pipe. It is *not* under the knob.
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
| Bushing length | **5.0** for a 15 mm shaft; **7.0** for 20/25/30 | `[DS pec11 p3]` |
| ~~Bushing length~~ | ~~6.5~~ — **WRONG, corrected.** 6.5 is the BODY height from the PCB face to the mounting surface. The drawing carries 6.5 as a standalone leader for the body and `LB` for the threaded portion; 6.5 was read twice, once correctly as the body and once incorrectly as the bushing. Max panel the stock encoder can be nutted to is therefore **2.60**, not 4.10. | `[DS pec11 p3]` |
| Shaft | **Ø6.0, flatted to 4.5** (D-shaft, Bourns "F" type) | `[DS PEC11 p2]` |
| Bushing + shaft from mounting surface (L1) | **15.0** (also 20 / 25 / 30 available) | `[DS PEC11 p2]` |
| Shaft tip above PCB front face | **21.40** ( = 6.50 + 14.90) | `[STEP]` — reconciles with L1 = 15.0 to within 0.1 mm |
| Mechanical angle | 360° continuous | `[DS p1]` |
| Detent / resolution | 12 / 18 / 24 detents; 12 / 18 / 24 PPR | `[DS p1]` |
| Switch | SPST momentary, travel **0.5 ± 0.2**, force **610 ± 306 gf** | `[DS p1]` |
| Rotational life | 30,000 cycles (switch 20,000) | `[DS p1]` |
| Hardware supplied | one flat washer + one mounting nut | `[DS p1]` |
| Max RPM | 60 — "hand rotated mechanical encoders only" | `[DS p1; AF 4991]` |

**Panel cutout:** Ø**7.0–7.2** clearance for the M7 × 0.75 bushing.

**Maximum panel thickness at the encoder: ≈4.0 mm.** Bushing 6.5 − nut ~2.0 −
washer ~0.5. v1's front wall is 2.40 mm (`v2/docs/envelope.md`), so a flush
panel is comfortable; a recess deeper than ~1.5 mm beyond that must be a
**counterbore**. The PEC11 nut and washer dimensions are **NOT SOURCED** — the
datasheet says they are supplied but does not dimension them (§10).

**Knob height above panel:** with a 2.40 mm panel, 15.0 − 2.40 = **12.60 mm**
of bushing+shaft protrudes, of which the top ~8.5 mm is Ø6 D-shaft. §5.5.

### 2.4 Alternative: 5752 Quad Rotary Breakout — recommended for a row of three

| Property | Value | Source |
|---|---|---|
| PCB | **76.200 × 21.590 × 1.570**, R2.54 | `[STEP]` |
| Mount holes | 4 × Ø**2.50** on **38.10 × 16.52** | `[STEP]` |
| Encoder positions | 4, at **19.05 mm pitch** on the board centreline (X = ±9.525, ±28.575 from centre) | `[STEP]` |
| Encoder rotation | **axis-aligned** (unlike 4991/5880), body envelope **12.10 × 12.30** each | `[STEP]` |
| Encoder height | **21.40** above front face — same PEC11 part as 5880 | `[STEP]` |
| STEMMA QT | **RIGHT-ANGLE** sockets (JST_SH4_RA), 2.90 × 6.00 × **4.25**, on the back face at the two ends, facing outward along the long axis; **4.30 mm above the back face** | `[STEP]` |
| Encoders supplied? | **No** — "does not come with any encoders soldered on" `[AF pinouts]`. Budget 4 × PEC11. |
| I2C | 0x49, eight addresses 0x49–0x50 | `[AF pinouts]` |

**Why it wins for three-in-a-row:** one board, one address, one QT cable, one
boss quad shared with the slider and key strip, and a guaranteed 19.05 mm
pitch. Three separate 5880s need three 25.4 mm squares — a minimum 25.4 mm
pitch unless you overlap boards, three addresses and three cables.

**Flag — measured, and it reads oddly:** the four reverse-mount NeoPixels on
the 5752 sit at **exactly the encoder centres** (same X, same Y) `[STEP]`, i.e.
*underneath* the encoder bodies. On the NeoSlider the same arrangement works
because the pot has a full-length centre slot for the light to escape through
(§3); an EC11 body has no such slot. **If per-encoder lighting matters to you,
verify on hardware before committing panel apertures.** The 4991/5880 do not
have this problem — their NeoPixel is clear of the encoder (§2.2).

---

## 3. NeoSlider — PID 5295, ×2

I2C QT Slide Potentiometer with 4 NeoPixels. Default **0x30**, sixteen
addresses 0x30–0x3F `[AF pinouts]`.

### Board `[STEP]`
- PCB **76.200 × 21.590 × 1.570**, R2.54
- 4 × Ø**2.50** on **38.10 × 16.52** (X = ±19.05, Y = ±8.26 from board centre)
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

### Panel cutout
A slot at least **76 × 10 mm** to clear the pot body, or — better — a
**75 × 2.5 mm** slot for the shank only, with the pot body below the panel.
With the body under a 2.40 mm panel, the shank stands **15.50 − 6.50 − 2.40 =
6.60 mm** proud. **The nubbin then has to reach down to the shank**, so check
its bore depth before fixing the panel thickness.

### Travel: **NOT SOURCED** — and the STEP cannot give it

Measured from `5295 NeoSlider Stemma.step` on the y = 0 centreline by point
classification, not from its face list:

| | mm | |
|---|---|---|
| shank tip from the PCB front face | **15.50** | `[STEP]` |
| slide-pot body front face | **6.00** | `[STEP]` |
| **shank proud of its body** | **9.50** | `[DERIVED]` |
| shank width | **5.00**, centred at x = +0.10 | `[STEP]` |

**The slot is not modelled.** The pot's cover is drawn as solid material across
the full 69 mm of the body and the shank simply passes through it, so there is
no opening in the model to measure and *travel cannot be derived from this
file*. Scanning every z level between the cover and the shank tip returns one
run — the shank — and no gap anywhere along the body.

So the 9.50 mm protrusion is now measured rather than quoted, and travel stays
open: calipers on the real part, or Adafruit's fab files. §10.
Neither the Adafruit page, the Learn guide, nor the STEP gives the electrical
or mechanical travel. The STEP models the pot with the shank at centre and does
not cut the slot, so travel cannot be derived from it. **Measure it.** (The
pot is 75 mm long; such parts are commonly 60 mm travel, but *commonly* is not
a source and this file does not carry guesses.)

### Flag — the vendor's own figures disagree
Adafruit lists the 5295 as **76.2 × 29.8 × 21.5 mm** `[AF]`. The STEP totals
**76.20 × 21.59 × 20.03**. The 21.5 is the board width and the 76.2 the length,
so the **29.8 is the height — 9.8 mm more than the model.** That gap is almost
exactly the height of Adafruit's slide-pot nubbin (**5621**, 11 × 7.1 × **9.3 mm**,
shaft slot 5 × 2.2 mm `[AF]`), which is **not in the STEP**. Assume a fitted
nubbin adds up to ~9.8 mm over the bare shank until measured.

---

## 4. NeoKey 1×4 QT — PID 4980, ×1

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
**Budget 4 × MX switches separately. No soldering.**

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

**Panel consequence:** the MX panel cutout is **14.00 mm square at 19.05 mm
pitch**, and the plate should be **1.5 mm** where the switch clips engage. A
3D-printed plate thicker than that stops the clips latching — the switch then
hangs on the hot-swap socket alone, which is not a mechanical mount. If the key
band needs to be recessed, **make the 1.5 mm plate a separate inset and recess
around it**, exactly the counterbore logic that applies to the pots in §5.

Keycap heights above are Cherry's own for Cherry's caps. **Aftermarket profiles
(OEM, DSA, XDA, SA) differ by several millimetres and are NOT SOURCED here** —
measure whatever you actually buy (§10).

---

## 5. Panel-mount potentiometers ×3 — first-class section

**Scope, as corrected by the brief:** standard panel-mount pots with a threaded
bushing, held by the panel with a nut, taking a standard knob. Not PCB-mount,
not a breakout. The governing number is the **bushing length**, because it sets
the maximum panel thickness at that point.

### 5.1 The arithmetic that decides counterbore vs thick panel

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

**So: anything up to ~4 mm of panel is fine with stock parts. A recess that
makes the panel locally thicker than that is only possible with a 10 mm
bushing — otherwise the recess must be a COUNTERBORE: thin (≤4 mm) where the
bushing passes, opened out above it to clear the knob.**

### 5.2 Candidates with real datasheet numbers

#### Alpha **RD901F** — 9 mm, single metal shaft, panel mount `[DS alpha-RD901F]`
**The recommendation, because of the bushing table.**

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

**This is the extra-long-bushing part the brief asked for.** `RD901F-20B6-15F-B10K`
(Ø7 × 10 mm bushing, 15 mm T18 shaft, 10 KΩ linear) gives a **≈7.5 mm panel
budget** — a genuine recessed pocket with a plain through-hole, no counterbore.

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
| Shaft | Ø**6.0**, **T18 knurled** (bbox 5.90 × 5.93 — not a D-flat), **6.5 mm** of knurl |
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
(→ **≈2.6 mm max panel**) until verified. That makes the PTV09A the *tightest*
option here and the reason the Alpha RD901F is recommended instead.

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

| Need | Part | Bushing | Max panel |
|---|---|---|---|
| **Recessed, no counterbore** | **Alpha RD901F-xxB6 or -xxE6**, 10 KΩ linear (B) | Ø7 or Ø9 × **10 mm** | **≈7.5 mm** |
| Best feel, detented, one-stop Adafruit | **Adafruit 562** (or Alpha RV16AF with a 21- or 41-click option) | M7 × 0.75 × 6.5 | ≈4.0 mm |
| Smallest behind the panel | Alpha RD901F (9 mm body) | per order code | per order code |

For a **control surface, buy linear (taper B)**, not audio/log — log taper
crushes resolution at one end of a parameter sweep.

### 5.4 What has to fit behind the panel

| Part | Diameter / section behind panel | Depth behind panel seat | Terminal reach |
|---|---|---|---|
| Adafruit 562 / RV16AF | Ø16.0 | **9.2** `[STEP]` | **16.6 mm sideways** `[STEP]` |
| Alpha RD901F (9 mm) | ~9.5 × 9.5 body | ~7.5 `[DS, body 7.5]` | 3 pins, 2.5 / 5.25 spacing |
| Bourns PTD901 (9 mm) | — | **7.05 ± 0.5** `[DS]` | — |

The 16 mm pot's **16.6 mm of sideways terminal reach** is easy to miss and is
usually what collides with a neighbouring board, not the body.

### 5.5 Knobs

All Adafruit; all `[AF]` product pages.

| PID | Knob | Ø | Height | Bore | Fits |
|---|---|---|---|---|---|
| **5527 / 5528 / 5530** | Anodized aluminium machined (black / silver / red) | **20.0** (inner 15.0) | **12.5** | **Ø6 + 2 mm hex set screw** | **T18, round AND D-shaft** |
| 2058 | Slim metal, T18 | 10.0 | 10.0 | T18 splines | T18 pots only — **not** a D-shaft |
| 2046 / 2047 / 2048 | Soft-touch T18 (red / white / blue) | 11.0 top, 15.0 base | **NOT SOURCED** | T18 splines | T18 pots only |
| 2056 | Solid machined metal | 25.0 | 15.5 | Ø6 slot, 11 mm keyway | D-shaft |
| 2055 | Scrubber knob | 34.7 | 10.5 | — | encoder |
| 5093 | Slim rubber encoder knob | 11.5 × 14.5 (axes not stated) | — | **Ø6 D-shaft** | PEC11 encoder |

**Recommendation: the set-screw knobs (5527/5528/5530).** They are the only
ones that fit **both** the encoder's Ø6 D-shaft and the pot's Ø6 T18 knurl, so
one knob part number serves the whole surface and the recess geometry is
identical everywhere: **Ø20 knob, 12.5 mm tall.**

**Recess sizing:** a counterbore must clear Ø20 plus finger room — **Ø26–28 is
a sensible pocket** — while the bushing passes through a **Ø7.2** hole in the
≤4 mm floor of that pocket. With a 2.40 mm floor and a 15 mm shaft, the shaft
stands 12.6 mm proud, the 12.5 mm knob sits essentially flush with the pocket
floor, and the pocket depth is then purely an aesthetic choice.

---

## 6. STEMMA QT IMU — recommendation: **BNO085, PID 4754**

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

### 8.1 Addresses as shipped

| Device | Qty | Default | Available range | How to change |
|---|---|---|---|---|
| Rotary encoder 4991 / 5880 | 3 | **0x36** | **0x36 – 0x3D** (8) | 3 jumpers A0/A1/A2, **soldered CLOSED** to add 1/2/4 `[AF]` |
| NeoSlider 5295 | 2 | **0x30** | **0x30 – 0x3F** (16) | 4 jumpers A0–A3, **CUT** to add 1/2/4/8 `[AF]` |
| NeoKey 1×4 4980 | 1 | **0x30** | **0x30 – 0x3F** (16) | 4 jumpers A0–A3, **soldered CLOSED** to add 1/2/4/8 `[AF]` |
| BNO085 4754 | 1 | **0x4A** | 0x4A / 0x4B | DI pin high `[AF]` |
| VL53L4CD 5396 | 1 | **0x29** | any, in software | set at runtime; needs XSHUT to sequence duplicates `[AF]` |
| ADS1115 1085 | 1 | **0x48** | 0x48 – 0x4B (4) | ADDR jumper `[AF]` |
| *(alt)* Quad Rotary 5752 | 1 | **0x49** | **0x49 – 0x50** (8) | 3 jumpers, **CUT** to add `[AF]` |

### 8.2 What actually collides, out of the box

1. **Three encoders all arrive at 0x36.** Three-way collision. Solved by
   jumpers — eight addresses for three devices, no contest.
2. **NeoSlider ×2 and NeoKey ×1 all arrive at 0x30.** Three-way collision.
3. **Cross-family:** the encoders' 0x36–0x3D band sits **inside** the
   0x30–0x3F band the sliders and key strip use. Six seesaw devices are
   competing for one 16-address window. It fits, but only if you allocate
   deliberately — this is the one that bites people.
4. **ADS1115 (0x48–0x4B) vs BNO085 (0x4A–0x4B)** — they overlap. Leave the
   ADS1115 at 0x48 and do not jumper it.
5. **ADS1115 vs Quad Rotary (0x49)** — fine at defaults; do not move the
   ADS1115 to 0x49 if you use the 5752.

### 8.3 Proposed map — **no multiplexer required**

| Address | Device | Jumper action |
|---|---|---|
| 0x29 | VL53L4CD ToF | none |
| 0x30 | NeoSlider A | **none — as shipped** |
| 0x31 | NeoSlider B | **cut A0** |
| 0x32 | NeoKey 1×4 | **close A1** |
| 0x36 | Encoder 1 | none |
| 0x37 | Encoder 2 | **close A0** |
| 0x38 | Encoder 3 | **close A1** |
| 0x48 | ADS1115 (3 pots) | none |
| 0x4A | BNO085 IMU | none |

Nine devices, nine unique addresses, four jumper operations, **no PCA9546/9548
needed**. If you take the **5752 Quad Rotary** instead of three 5880s, drop
0x36–0x38 and add **0x49** — eight devices, two jumper operations.

**Two practical warnings:**

- **NeoSlider jumpers are CUT, not soldered.** That is destructive and
  one-way. **Decide the slider addresses before touching a knife.** The NeoKey
  and the encoders use solder-closed jumpers, which are reversible. The quad
  encoder is cut-to-change like the slider.
- **Bus loading.** Adafruit STEMMA QT boards generally carry their own SDA/SCL
  pull-ups; nine boards in parallel divides the effective pull-up resistance by
  roughly nine and adds nine connectors' worth of capacitance plus cable. I did
  **not** confirm the resistor value on each board. Check the schematics before
  committing to 400 kHz, and be ready to run the chain at 100 kHz or lift
  pull-ups on all but one or two boards. `[NOT SOURCED — verify]`

### 8.4 The three pots are ANALOGUE — what reads them

The panel-mount pots in §5 have no I2C. Two options:

**(a) ESP32-S3 native ADC.** Three ADC1 channels (GPIO1–GPIO10). Free, no extra
board. But ADC2 is unusable while the radio is up — and this device runs WiFi,
BLE HID and BLE MIDI (`STATUS.md`) — so you are confined to ADC1, and the S3's
SAR ADC is noisy and non-linear enough that a slow knob sweep visibly steps.
It also means **three analogue runs across the enclosure** to the main board,
picking up noise from the display and the radio.

**(b) ADS1115 over STEMMA QT — recommended.** 16-bit, 4 channels (one spare),
programmable gain, up to 860 SPS `[AF]`. The three pots wire locally to the
ADS1115's 0.1″ header, and **one QT cable** joins it to the chain that already
carries everything else.

| ADS1115 breakout, PID 1085 | Value | Source |
|---|---|---|
| PCB | **25.400 × 17.780 × 1.570**, R2.54 | `[STEP]` |
| Mount holes | 4 × Ø**2.50** on **20.32 × 12.70** | `[STEP]` |
| Above front face / below back face | 0 / **2.96** (QT sockets) | `[STEP]` |
| STEMMA QT | 2 sockets, two short ends, back face, facing outward | `[STEP]` |
| I2C | **0x48**, four addresses 0x48–0x4B | `[AF]` |
| Vendor-stated size | 25.4 × 17.8 × 4.6 — agrees with the STEP | `[AF]` |

**Which is more coherent: (b).** Everything else on this deck is STEMMA QT.
The ADS1115 keeps the pots on the same chain, the same cable type and the same
`i2cdetect` map as the rest of the surface, replaces three noise-prone analogue
runs with one differential-ish local cluster, and gives 16 bits instead of a
noisy 12. The cost is one more 25.40 × 17.78 board inside the enclosure and one
more address — both already budgeted in §8.3.

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

**Measure on hardware:**

1. **NeoSlider (5295) travel** — electrical and mechanical, in mm. Not on the
   product page, not in the Learn guide, not derivable from the STEP (the model
   does not cut the actuator slot). **This is the single most important missing
   number in this document** — it sets the slider band's length and the
   usable resolution of two of the deck's primary controls.
2. **NeoSlider nubbin (5621 / 5741)** — bore depth, and therefore how far the
   finished knob stands above the shank tip. Adafruit's own product dimension
   for the 5295 (29.8 mm) exceeds the STEP (20.03 mm) by 9.8 mm and the nubbin
   is the only candidate for the difference.
3. **PEC11 encoder nut and washer** — across-flats and thickness. The datasheet
   says they are supplied but does not dimension them. Until measured, use the
   Bourns PTV09 hardware as a proxy (nut 11.0 A/F × 2.0, washer 0.4) and treat
   the 4.0 mm panel budget as provisional.
4. **Keycap height** for whatever caps you actually buy. Cherry's own figures
   (§4) apply to Cherry caps; OEM/DSA/XDA/SA profiles differ by millimetres.
5. **Soft-touch knob height (2046/2047/2048)** — Adafruit gives both diameters
   but no height.
6. **5093 encoder knob** — the page says "11.5 mm × 14.5 mm" without saying
   which is diameter and which is height.
7. **Per-board I2C pull-up values** across all nine boards (§8.3).
8. **5752 NeoPixel visibility** with an encoder fitted (§2.4).
9. **Adafruit 562 bushing thread** — the STEP models it Ø6.27/5.41, which is
   undersize for the M7 × 0.75 the Alpha datasheet specifies. Check with a
   thread gauge before cutting a panel hole, or just drill Ø7.2 and let the nut
   decide.

**Fetch from a distributor (blocked from this environment):**

10. **Alps RK09** — alpsalpine.com returns 403 on every path tried.
11. **Bourns PTV09A-6 and PTD901 bushing lengths** — present on the drawings
    but not legible enough to assign with confidence at the resolution available.

---

## 11. Shopping list, as researched

| Qty | Part | PID / PN |
|---|---|---|
| 3 | I2C STEMMA QT Rotary Encoder **with Encoder** | Adafruit **5880** |
| *or* 1 | I2C Quad Rotary Breakout + 4 × Bourns PEC11 | Adafruit **5752** + **377** ×4 |
| 2 | NeoSlider | Adafruit **5295** |
| 2–6 | Slide-pot nubbin | Adafruit **5621** (3-pack) |
| 1 | NeoKey 1×4 QT | Adafruit **4980** |
| 4 | Cherry MX-compatible keyswitches + keycaps | — |
| 3 | Panel-mount pot, **10 KΩ linear** | **Alpha RD901F-20B6-15F-B10K** (10 mm bushing) *or* Adafruit **562** |
| 3+ | Knob, Ø20 × 12.5, Ø6 set-screw | Adafruit **5527 / 5528 / 5530** |
| 1 | ADS1115 4-ch 16-bit ADC | Adafruit **1085** |
| 1 | 9-DoF IMU with on-chip fusion | Adafruit **4754** (BNO085) |
| 1 | Time-of-flight sensor | Adafruit **5396** (VL53L4CD) |
| ~10 | STEMMA QT cables, 50/100/200 mm | Adafruit 4399 / 4210 / 4401 |

Nine I2C devices, one chain, no multiplexer.
