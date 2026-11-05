# The 12.70 mm clear acrylic stack

*An enclosure input, written to be cut and printed from. The user has fixed the
construction: a **laser-cut clear acrylic front panel, 0.500 in = 12.700 mm**, a
**3D-printed carrier plate** beneath it holding every PCB and all the wiring on
its **top face, facing the user, seen through the acrylic**, and a **laser-cut
back panel**. This document decides whether that works, and what has to change
so that it does.*

*Every number carries a source tag. Numbers that could not be traced to a
primary source are marked **NOT SOURCED** and listed in §7. Nothing here is
estimated.*

---

## 0. Verdict up front

1. **`parts.md` §2.3 has the PEC11 bushing length wrong, and the error is the
   centre of this whole problem.** It records "Bushing length 6.5" — but 6.5 is
   the *body* height from the PCB face to the mounting surface. The Bourns
   drawing's own table gives the bushing **LB = 5.0 mm** for the 15 mm shaft and
   **7.0 mm** for 20/25/30 `[DS pec11 p3]`. So the stock encoder's threaded
   bushing is **5.00 mm**, which permits a panel of **2.60 mm**, not 4.10. §1.1.

2. **A uniform 12.70 mm panel defeats both stock controls, and it is not close.**
   With the panel's underside resting on each part's own shoulder — the lowest it
   can physically go — a **5880 leaves 2.30 mm of shaft above the panel**, and a
   **5295 NeoSlider's shank tip finishes 3.70 mm *below* the panel's top face**.
   `[DERIVED §2.2, §2.4]` The acrylic is not a window problem. It is a
   shaft-length problem, and the shafts are 10 mm too short.

3. **The encoders are fixable for the price of one component, and the fix is
   already half-bought.** Adafruit **4991** is the identical board *without* an
   encoder fitted (`parts.md` §2.1). Solder a **Bourns PEC11-4225F-S0024**
   (25 mm shaft, 24 detents, with switch) to it and the shaft tip moves from
   21.50 to **31.50 mm above the PCB face**, giving **12.30 mm of Ø6 shaft above
   a 12.70 mm panel** — 2.05 × the shaft diameter, a proper knob. §2.3.
   Cost: you solder three encoders, and Bourns says hand soldering is "**Not
   recommended**" `[DS pec11 p1]`.

4. **The NeoSlider cannot be fixed by buying anything.** Its 9.00 mm of shank is
   moulded into the part, no long-lever variant exists in the Adafruit
   catalogue, and the two long-lever slide pots with real datasheets stop at
   **H = 10.0 mm** (Bourns PTA, 15 mm metal lever) `[DS pta]` — 1 mm better than
   the part you already own. **No fader on the market stands 12.70 mm above its
   own body.** Faders live under 1.5–2 mm panels because that is what the
   geometry allows. §2.4.

5. **So the fader does not go under the acrylic — it goes *in* it.** Cut a
   through-window in the 12.70 slab and stand the 5295 on a printed plinth
   inside it, with the slide pot's body top **3.00 mm below the slab's top
   face**. The nubbin then has the full 9.00 mm of shank, sits in a 3 mm
   protective well, and still stands 6.30 mm proud. One laser cut, no pocket, no
   lamination, no bond line. §2.4.4.

6. **Do not laminate 2 × 6.35 to fake a pocket.** Optically a void-free
   index-matched cement joint does disappear — PMMA's n<sub>D</sub><sup>20</sup> is
   **1.491** `[RÖHM gsxt]` and the cements are PMMA — but every trapped bubble is
   two 4 %-reflecting surfaces `[RÖHM gsxt]`, TAP's own bulletin says
   "**bubble-free laminations are difficult but possible and generally require
   experimentation**" `[3P tap]`, and the joint is irreversible on a device whose
   brief is "can be taken apart". A **12.70 mm slab plus a separate 6.35 mm
   plate**, held apart by a drawn reveal, gets you every local thickness you want
   with no bond line at all. §3.4.

7. **At 12.70 mm, laser taper alone can close an M7 bushing hole.** 1.5° of
   taper over 12.70 mm is **0.67 mm across the diameter** `[DERIVED]`, so a hole
   drawn Ø7.40 finishes **Ø6.73 at the narrow face** — the Ø7.0 bushing will not
   pass. Draw **Ø8.20** for any M7 bushing through 12.70 mm, or stop using small
   holes at this thickness and use **Ø25.400 wells** instead. §3.2, §3.5.

8. **The 38.10 mm control pitch is not a taste decision — it is Röhm's rule.**
   For Ø25.400 wells, Röhm requires hole centres at least **1.5 × the hole
   diameter** from any edge `[RÖHM kb]`, i.e. **38.10 mm**, which is also exactly
   2 key units and leaves a **12.70 mm web — equal to the sheet thickness**,
   which is the minimum-web guidance `[3P fab-guides]`. Three constraints, one
   number. §3.3.

9. **Buy cast, not extruded — this reverses `acrylic.md` §0.2 for this part, and
   for a good reason.** `acrylic.md` chose extruded because a *flush functional
   plate* lives on thickness tolerance. Nothing in this stack is flush, so
   thickness becomes a free variable, and the two things that are *not* free —
   **laser edge quality and crazing resistance** — both favour cast
   `[3P cast-v-extruded]`. The price is brutal: a "half-inch" cast sheet is in
   spec anywhere from **10.79 to 13.59 mm** `[3P eplastics]`. §4.1, §4.2.

10. **Clarity is not a problem at this thickness and the datasheet says why.**
    PMMA's absorption in the visible is **< 0.05 %** and its transmittance is
    **~92 % for *both* GS and XT** `[RÖHM gsxt]` — the 8 % loss is the two
    surfaces at 4 % each, not the path. Doubling the thickness costs nothing.
    There is no green edge because there is almost nothing to absorb. §4.1.

11. **Gloss, and a cleaning rule — not a frosted back.** Frosting the back face
    is the obvious anti-fingerprint move and it is the one move that destroys the
    brief: it is the face the wiring is seen through. A matte *front* scatters
    too, and the wiring is ~19 mm behind it. **Keep both faces polished, and
    accept that this instrument is cleaned with warm water and a drop of
    detergent, never dry, never isopropyl** `[RÖHM mach §9]`. §4.4.

12. **The one thing nobody asked about: the display.** 12.70 mm of clear acrylic
    over a **reflective, backlight-less** LCD costs **15.1 %** of its light
    (four 4 % surface crossings, in and out) `[DERIVED from RÖHM gsxt]` plus a
    full-strength specular reflection off the front face, on a display that has
    no backlight to fight it with. **Put the display in an aperture, not under
    glazing.** §4.5.

---

## 0.1 Source tags

| Tag | Meaning |
|---|---|
| `[DS pec11]` | Bourns **PEC11** 12 mm incremental encoder datasheet. Read directly from `v2/parts/models/datasheets/bourns-PEC11-12mm-encoder.pdf`. |
| `[DS ptv09]` | Bourns **PTV09** 9 mm potentiometer datasheet, same directory; dimension pages rendered and read. |
| `[DS pta]` | Bourns **PTA** low-profile slide potentiometer datasheet, fetched from `bourns.com/docs/product-datasheets/pta.pdf`. |
| `[RÖHM gsxt]` | POLYVANTIS/Röhm **PLEXIGLAS GS/XT Technical Information**, Ref. 211-1, 04/25. Primary material data. |
| `[RÖHM anneal]` | ACRYLITE **Annealing ACRYLITE acrylic sheet** fabrication manual. |
| `[RÖHM kb]`, `[RÖHM mach]` | ACRYLITE knowledge base / Röhm *Machining PLEXIGLAS* — as cited and quoted in `v2/parts/acrylic.md` §5, §7. |
| `[TROTEC]` | Trotec **Acrylic Processing Guide: How to Achieve Flame Polished Edges**. A laser maker's own guide. |
| `[STEP]` | Measured from the vendor STEP file; the numbers live in `parts.md` §1. |
| `[AF]` | Adafruit product page, read directly. |
| `[SVC …]` | A fabrication service's own published specification. |
| `[3P …]` | Third party — distributor listing, vendor blog, community practice. Weakest; always flagged. |
| `[DIST]` | A distributor's spec table, not a manufacturer drawing. |
| `[DERIVED]` | Arithmetic from the above, shown in full. |

URLs in §6.4.

---

# 1. The shaft-length problem

## 1.1 First, a correction to `parts.md`

`parts.md` §2.3 records, for the PEC11 fitted to the 5880:

| | `parts.md` says | the drawing says |
|---|---|---|
| Body, PCB front face → mounting surface | 6.50 `[STEP]` | **6.5** `[DS pec11 p3]` ✔ |
| Bushing length | 6.5 `[DS PEC11 p2]` | **5.0** for L1 = 15 `[DS pec11 p3]` ✘ |

The PEC11 dimension drawing carries **`6.5` as a standalone leader at the far
left** (the body) and then **`L1`** spanning from the mounting surface to the
shaft tip, with **`LB`** calling out the threaded portion inside it. The table
beneath it, quoted verbatim:

```
            L1        LB         l
            15       5.0       7.0
            20       7.0      10.0
            25       7.0      12.0
            30       7.0      12.0        [DS pec11 p3, F-type flatted shaft]
```

`6.5` was read twice — once correctly as the body, once incorrectly as the
bushing. The consequences:

| | was | is |
|---|---|---|
| Ø6 shaft available on a 15 mm encoder | 8.40 (implied) | **10.00** `[DERIVED]` |
| Max panel the stock encoder can be nutted to | 4.10 | **2.60** `[DERIVED: 5.0 − 0.4 washer − 2.0 nut]` |
| Shaft tip above PCB front face | 21.40 `[STEP]` | 6.5 + 15.0 = **21.50** `[DS]` — agrees to 0.1 mm ✔ |

`acrylic.md` §1's bushing table inherits the error: its row
"6.5 mm — PEC11 encoder … max panel **4.10**" should read **5.0 mm → 2.60**.
The 16 mm Alpha and Adafruit 562 rows are unaffected (`[STEP 562]` measured the
bushing at 6.6 mm directly).

**The washer-and-nut allowance of 2.40 mm throughout is the Bourns PTV09
hardware, used as a class proxy and now read off the drawing directly:
nut M9 × 0.75, 11.0 ± 0.5 across flats, 2.0 ± 0.2 thick; washer ID 9.1 ± 0.3,
OD 15.0 ± 1.0, 0.4 thick** `[DS ptv09 p3]`. The PEC11 datasheet says "One flat
washer and one mounting nut supplied with each encoder" `[DS pec11 p1]` and does
not dimension them — still NOT SOURCED (§7.1).

## 1.2 The governing equation

Every part here is carried by the **carrier plate**, not by the acrylic, because
12.70 mm is thicker than any bushing on the market. The acrylic is a window the
shaft passes through. So the question is never "will the bushing reach through
the panel" — it is:

```
protrusion above the panel's top face
     = (shaft length above the part's own shoulder) − (local panel thickness T)

                                                                    [DERIVED]
```

…and the panel's underside can descend no lower than the part's shoulder,
because below that the part's body is in the way and a laser cannot cut a
pocket. **T is therefore the *local* acrylic thickness at that control, and it is
the only free variable.**

Shoulder-to-tip, from the vendor models and datasheets:

| part | shoulder | above the shoulder | source |
|---|---|---|---|
| 5880 / 4991 + PEC11, **L1 = 15** | body top, 6.50 above PCB front | **15.00** (5.00 bushing + 10.00 Ø6 shaft) | `[DS pec11 p3]`, `[STEP]` |
| 4991 + PEC11, **L1 = 20** | ditto | **20.00** (7.00 + 13.00) | `[DS pec11 p3]` |
| 4991 + PEC11, **L1 = 25** | ditto | **25.00** (7.00 + 18.00) | `[DS pec11 p3]` |
| 4991 + PEC11, **L1 = 30** | ditto | **30.00** (7.00 + 23.00) | `[DS pec11 p3]` |
| 5295 NeoSlider | pot body top, 6.50 above PCB front | **9.00** (shank only) | `[STEP 5295]` via `parts.md` §3 |
| panel pot, shaft code *L* | its own mounting surface | ***L*** | `[DS ptv09 p2]`, corroborated §1.3 |

## 1.3 Where a pot's "shaft length" is measured from

Bourns prints `L ± 0.5` on the shaft-style drawings with the shaft broken off to
the left of the extension line `[DS ptv09 p2]`, which is the convention for "from
the mounting surface" but is not stated in words. It is confirmed empirically
inside this repo: `parts.md` §5.2 measured the **Adafruit 562** — an Alpha
RV16AF-class 16 mm pot with the standard **15 mm** shaft code — and found
**"Shaft tip above panel seat: 15.6"** `[STEP 562]`.

**15.6 measured against a 15 code. *L* is from the mounting surface, to within
0.6 mm.** Used that way throughout.

---

# 2. The arithmetic, control by control

Datum for everything below: **Z = 0 at the carrier plate's top face** — the
visible wiring plane. The acrylic's underside sits at Z = **A**; its top face at
**A + T**.

## 2.1 Why the recommended A is 6.350

The wiring has to live somewhere, and that somewhere is the gap between the
carrier's top face and the acrylic's underside. Set it at **6.350 mm = ¼ in =
5 × 1.27 mm**, the project module from `acrylic.md` §10. Then:

```
carrier top face (wiring plane)   Z =  0.000
acrylic underside                 Z =  6.350      = ¼ in
acrylic top face                  Z = 19.050      = ¾ in = 1 key unit = 15 units

wiring volume : acrylic : total   =  6.35 : 12.70 : 19.05   =   1 : 2 : 3

                                                                    [DERIVED]
```

Three numbers, one ratio, every one of them an integer multiple of the 1.27 mm
unit, and the total equals the keyboard's own 19.05 mm key pitch. That is the
"mathematically flawless ratio" the brief asks for, and it falls out of the
construction rather than being imposed on it.

The breakouts sit with their **front faces at Z = −0.15**, sunk 1.72 mm into the
carrier so that each encoder's 6.50 mm body top lands exactly on **Z = 6.350** —
the acrylic's underside. Two things follow for free:

- The boards read as **flush** with the wiring plane (0.15 mm proud of nothing),
  so cables leave them in the same plane they travel in.
- **The encoder's own shoulder bears on the acrylic's underside.** Finger push
  goes shaft → body → shoulder → acrylic → panel screws → carrier, and *not*
  into four solder joints. `acrylic.md` §2.6 argued for exactly this; here it is
  free rather than engineered, because 6.50 and 6.35 differ by 0.15 mm.

## 2.2 The 5880, as bought, under 12.70 mm

```
PCB front face          Z = -0.15
encoder body            -0.15 -> 6.35        16.7 sq keep-out  [STEP]
M7x0.75 bushing, LB      6.35 -> 11.35       5.00 long         [DS pec11]
O6 flatted shaft        11.35 -> 21.35      10.00 long         [DERIVED]
acrylic                  6.35 -> 19.05      12.70 thick

protrusion = 21.35 - 19.05 = 2.30 mm                           [DERIVED]
nut?         5.00 bushing - 12.70 panel  = -7.70   impossible  [DERIVED]
```

**2.30 mm.** A Ø20 × 12.5 mm knob on 2.30 mm of a Ø6 shaft is a lever with a
5.4 : 1 mechanical disadvantage against its own grip. It is not a control; it is
a thing that will come off in your hand. The 5880 is out.

The same table for every thickness worth considering, protrusion in mm
`[DERIVED]`:

| local T | PEC11 **15** | PEC11 **20** | PEC11 **25** | PEC11 **30** | NeoSlider |
|---|---|---|---|---|---|
| 0 (no acrylic) | 15.00 | 20.00 | 25.00 | 30.00 | 9.00 |
| 3.00 | 12.00 | 17.00 | 22.00 | 27.00 | **6.00** |
| 4.50 | 10.50 | 15.50 | 20.50 | 25.50 | 4.50 |
| 6.35 | 8.65 | 13.65 | 18.65 | 23.65 | 2.65 |
| **12.70** | **2.30** | 7.30 | **12.30** | 17.30 | **−3.70** |

## 2.3 The fix: 4991 + a 25 mm PEC11

`parts.md` §2.1 already established that **4991 and 5880 are the same board**;
the 5880 simply arrives with a PEC11 soldered to it, and Adafruit's loose
encoder (PID 377) is a **PEC11-4215F-S0024** `[AF prod-377]`. The ordering code
is open `[DS pec11 p2]`:

```
PEC11 - 4   2   25   F  -  S   0024
         |   |    |   |     |     |
         |   |    |   |     |     +-- 24 pulses / 360 deg
         |   |    |   |     +-------- push momentary switch
         |   |    |   +-------------- metal FLATTED shaft
         |   |    +------------------ 25.0 mm shaft
         |   +----------------------- 24 detents
         +--------------------------- PC pin, horizontal / rear facing
```

**`PEC11-4225F-S0024`** — same detent count, same switch, same footprint, same
45°-on-the-board orientation as the part Adafruit fits. Only the shaft is longer.

```
PCB front face          Z = -0.15
encoder body            -0.15 ->  6.35
M7x0.75 bushing, LB      6.35 -> 13.35       7.00 long         [DS pec11 p3]
O6 flatted shaft        13.35 -> 31.35      18.00 long, flat l = 12.00
acrylic                  6.35 -> 19.05

protrusion = 31.35 - 19.05 = 12.30 mm   = 2.05 x shaft diameter  [DERIVED]
of which, inside the acrylic:  bushing 7.00, O6 shaft 5.70
```

**12.30 mm of Ø6 shaft, with a 12.00 mm flat on it.** The knob's set screw lands
on the flat, not on a round shaft, over its whole engagement.

Costs of this route, stated plainly:

- **You solder three encoders**, and Bourns says of the PEC11: "Wave Soldering…
  260 °C max for 3–5 seconds. **Hand Soldering… Not recommended**" `[DS pec11 p1]`.
  People hand-solder these constantly; Bourns will not endorse it. The failure
  mode is a melted body, not a bad joint.
- The 25 mm variant is **not** a stock Adafruit line item; it is a Bourns order
  from a distributor.
- The **K (knurled) shaft style is not available at 25 mm with a switch** — the
  datasheet restricts knurled-with-switch to 15 and 20 mm `[DS pec11 p2]`. The
  F (flatted) style has no such note. Use F.

### 2.3.1 Does anything grip 12.30 mm?

I could not find a published engineering rule for **set-screw hub engagement as
a multiple of shaft diameter** — the rules that exist are about the *screw*, not
the hub (Machine Design: minimum thread engagement 1–1.5 × the set screw's major
diameter in aluminium `[3P machinedesign]`; setscrew ≈ ½ shaft diameter
`[3P machinedesign]`). **That rule is therefore NOT SOURCED (§7.3) and the
engagement figures below are stated as ratios, not as pass/fail.**

What *is* sourced is that torque is a non-issue. The PEC11's running torque is
**50–200 gf·cm = 0.005–0.020 N·m** `[DS pec11 p1]`; an Elma M2.5 knob set screw
is rated to **0.2 N·m** `[DS elma-k1]`. Ten times the margin. The question is
purely wobble, i.e. leverage.

Knob bore depths, as far as they can be sourced:

| knob | Ø | height | bore | **bore depth** | source |
|---|---|---|---|---|---|
| Adafruit **5527 / 5528 / 5530** | 20.0 (inner 15.0) | 12.5 | Ø6 + 2 mm hex set screw | **not stated** | `[AF prod-5527]` |
| Adafruit **2056** | 25.0 | 15.5 | Ø6 slot | **11 mm keyway** | `[AF]` via `parts.md` §5.5 |
| RS PRO Ø20 black, white indicator | 20.0 | 12.5 | 6.4 | **"Depth 9 mm"** | `[DIST rs-pro]` — the field is not defined on the listing |
| Elma **K1 Metal** (Shape/Pure/etc.) | various | various | 6 mm standard | **not stated** | `[DS elma-k1]` |

**None of the four knob datasheets dimensions the bore.** The two numbers that
exist — 11 mm and 9 mm — are both *less* than the 12.30 mm available, which
settles the design question even though it leaves the datasheet question open:

```
float above the panel  =  12.30  -  engagement          [DERIVED]

knob bottomed on the shaft tip, bore 11.0  ->  float 1.30, engagement 11.30
knob bottomed on the shaft tip, bore  9.0  ->  float 3.30, engagement  9.00
knob set deliberately to float 1.00        ->  engagement 11.30 (bore permitting)
```

> **The shaft stops being the binding dimension.** With 12.30 mm available you
> get to *choose* the float: set every knob with the same 1.00 mm shim so all six
> sit in one plane, and the set screw holds it there on a 12.00 mm flat.

That is the *opposite* of the 5880 case, where the knob would have rested on the
panel with 2.30 mm in the bore and no choice about anything. **The 25 mm shaft
moves the design from shaft-limited to bore-limited, which is where you want
it.** Measure the actual bore on receipt (§7.2).

## 2.4 The NeoSlider, which is the one that cannot be bought out of

### 2.4.1 The number

```
NeoSlider 5295, measured from the vendor STEP          [STEP 5295, parts.md §3]
   pot body            6.50 above the PCB front face
   actuator shank      9.00 clear of the body
   total               15.50 to the shank tip

acrylic underside resting on the pot body top:
   protrusion = 9.00 - 12.70 = -3.70 mm                           [DERIVED]
```

**The shank tip finishes 3.70 mm below the top surface of the acrylic.** It is
not short of a knob; it is *underwater*. And a slide actuator has a second
requirement a rotary does not: it must **travel** along a slot, so the slot
cannot be opened out into a knob well — it has to stay narrow for its whole
length, and at 12.70 mm deep a 2.5 mm slot is a 5 : 1 aspect-ratio canyon no
finger reaches into.

### 2.4.2 There is no long-lever fader to buy

| part | lever above its mounting plane | source |
|---|---|---|
| Adafruit 5295 / 4219 slide pot | **9.00** | `[STEP 5295]` |
| Bourns **PTA**, 10 mm lever (`DP` metal) | **H = 7.0** | `[DS pta p1]` |
| Bourns **PTA**, 15 mm lever (`DP` metal) | **H = 10.0** | `[DS pta p1]` |

The Bourns PTA is the industry's long-lever slide pot — 15 to 60 mm travel,
metal/insulated lever options, 15,000 cycles, 30–250 gf operating force
`[DS pta p1]` — and its tallest standard lever stands **10.0 mm**. One millimetre
more than the part already in the parts bin, against a 12.70 mm panel.
(The PTA is also a **PCB part**, "PC Pins Down Facing", with two M2 × 0.4 mounting
holes `[DS pta p2]`, so swapping to it means hand-wiring three terminals and
losing the four NeoPixels and the I²C interface. It would, however, land on the
**spare fourth ADS1115 channel** that `parts.md` §8.3 already allocated.)

**Faders have short levers because panels are thin.** A 12.70 mm panel is outside
the entire product category. This is not a sourcing failure; it is a fact about
faders.

### 2.4.3 The four options, with the arithmetic

| # | option | number | verdict |
|---|---|---|---|
| **a** | **Local pocket in the acrylic.** Mill 8–9 mm deep × 76 × 12. | Needs T ≤ 4.00 locally for a nubbin with ~5 mm of engagement `[DERIVED]`. | **Not available.** A laser cuts through or rasters; it does not pocket. This is a CNC operation, and the user chose laser. |
| **b** | **Lamination:** a 6.35 upper lamina cut away over the fader, over a 6.35 lower lamina. | Fader under 6.35: protrusion **2.65 mm** `[DERIVED]`. Nubbin is 9.3 mm tall `[AF 5621]`. | **Fails anyway**, and buys a bond line (§3.4). |
| **c** | **Raised sub-panel / plinth under the acrylic.** Lift the 5295 so the shank clears. | To get 6 mm proud: board bottom at Z = 7.98, acrylic underside at 6.35 → **the board is 3.2 mm inside the acrylic** `[DERIVED]`. | **Fails.** Requires the pocket of option (a). |
| **d** | **A step: the fader is *in* the slab, not under it.** | See below. | **This one.** |

### 2.4.4 The recommendation: a window, and a plinth inside it

Cut a through-window in the 12.70 mm slab and stand the NeoSlider in it on a
printed plinth, with the **slide pot's body top 3.00 mm below the slab's top
face**:

```
acrylic top face                     Z = 19.050
pot body top (3.00 mm recessed)      Z = 16.050
pot body                             Z = 16.050 - 6.50 = 9.550   <- PCB front face
NeoSlider PCB front face             Z =  9.550
NeoSlider PCB bottom                 Z =  7.980      -> plinth height 7.980
shank tip                            Z =  9.550 + 15.50 = 25.050

nubbin engagement available          25.050 - 16.050 = 9.00 mm   (the whole shank)
nubbin top (9.3 tall, seated at 16.050)  Z = 25.350
nubbin top above the slab            25.350 - 19.050 = 6.30 mm, in a 3.00 mm well
                                                                    [DERIVED]
```

What this buys:

- **The full 9.00 mm of shank**, which is every millimetre the part has. The
  Adafruit **5621** nubbin is 11 × 7.1 × **9.3 mm** with a **5 × 2.2 mm shaft
  slot** `[AF prod-5621]`; its slot *depth* is not published (§7.4) but cannot
  exceed 9.3, so the shank is now the long partner rather than the short one.
- **A 3.00 mm lip around the fader**, which is knob protection the rest of the
  panel does not need and the fader does — it is the part your palm lands on.
- **No pocket, no bond, one laser cut.** The window is an internal contour in a
  12.70 mm sheet, and at 88.90 × 25.40 mm it is comfortably above every
  minimum-feature limit in §3.3.
- The four reverse-mounted NeoPixels light the pot's own full-length slot
  `[STEP 5295]`, and with the pot body standing in open air inside the window,
  that light now escapes sideways into the acrylic's cut edge as well.

Window size: the board is 76.200 × 21.590 `[STEP]`; the plinth needs a wall;
**cut 88.90 × 25.40 (3½ in × 1 in, 70 × 20 units)**, centred, leaving 6.35 mm of
plinth wall at each end and 1.905 mm at each side. `[DERIVED]`

Costs, stated: the fader is **no longer behind the acrylic**, so it is the one
control that can be touched directly, it is the one place dust gets in, and the
plinth's two STEMMA QT cables have to climb 7.98 mm from the wiring plane. §5.2
routes them.

### 2.4.5 If the fader must stay under glazing

Then the only remaining move is to **delete the 5295** and fit a Bourns
**PTA6043-2015DPB103** (60 mm travel, 15 mm metal lever, H = 10.0, linear
10 kΩ) `[DS pta]` with a **custom printed cap** whose socket reaches down through
the slab — a 2.2 × 5 mm socket ~15 mm deep over a slot wide enough to pass the
cap's spine (≈8 mm, not 2.5). It works geometrically. It costs you the
NeoPixels, the I²C interface, three hand-soldered wires, the spare ADS1115
channel, and a part you have to design and print to tolerance. **Option (d) is
better.**

## 2.5 The three potentiometers

These are yours to choose, and they mount to the carrier, so the only question
is: how long a shaft, and how high a boss?

**Design rule: every knob on this instrument sits in the same plane.** The
encoder sets it, because the encoder's shaft length comes in 5 mm steps and
cannot be shimmed. With the 25 mm PEC11 the shaft tip is at **Z = 31.350**, so
every pot shaft tip must also reach **Z = 31.350**, and the pot's mounting
surface therefore sits at **Z = 31.350 − L**:

| shaft code *L* | mounting surface Z | what that means | nut (0.4 + 2.0) occupies | clears the acrylic at 6.35? |
|---|---|---|---|---|
| 15 | **16.35** | 10 mm *inside* the acrylic | 16.35 → 18.75 | ✘ impossible |
| 20 | **11.35** | 5 mm inside the acrylic | 11.35 → 13.75 | ✘ impossible |
| **25** | **6.35** | flange exactly at the acrylic underside, on a **6.350 mm printed boss** | 6.35 → 8.75 | **only inside a ≥Ø16 well** |
| **30** | **1.35** | on a **1.270 mm boss** (1 unit; tip then lands at 31.270, 0.08 mm under the encoders) | 1.27 → 3.67 | **✔ 2.68 mm of air** |

`[DERIVED]`

**Recommendation: *L* = 30, on a 1.270 mm boss.** The nut and washer then sit
**on the wiring plane, in full view through the acrylic** — three nickel M9 nuts
and three Ø6 shafts crossing the visible interior, which is the brief's "the
interior is as much the design as the front panel" made literal rather than
decorative. Wrench clearance is 11.5 mm `[DS ptv09 p3]`, and nothing is within
that at a 38.10 pitch.

**The penalty, and it is sourced:** the PTV09 specifies *play of shaft* as
**0.8 × L/20 mm peak-to-peak** under a 250 gf·cm end thrust `[DS ptv09 §2.3]` —
play scales *linearly with shaft length*:

| *L* | play at the shaft tip |
|---|---|
| 15 | 0.60 mm P-P |
| 20 | 0.80 mm P-P |
| **25** | **1.00 mm P-P** |
| **30** | **1.20 mm P-P** |

1.2 mm of wobble at the knob is perceptible. If that is unacceptable, take
*L* = 25 on a 6.350 mm boss and put the nut inside a Ø25.400 well (§3.3) — 1.00
mm, and a visible nut under a floating knob, which is arguably the better-looking
answer anyway.

### 2.5.1 Which pot, specifically

Requirements: **Ø6.0 shaft, 25 or 30 mm long, threaded metal bushing, 9 mm or
16 mm body, linear (taper B)** — `parts.md` §5.3's reasoning on linear-vs-log
stands.

| candidate | 25/30 mm shaft? | bushing | shaft material | verdict |
|---|---|---|---|---|
| **Bourns PTV09A-6** `[DS ptv09]` | **Yes — 15 / 20 / 25 / 30, documented** | **M9 × 0.75 metal threaded**, length 5.0 `[DS ptv09 p3 — see §7.5]` | **insulated (plastic)**: F flat, S 18-tooth knurl, U 40-tooth knurl | **The only datasheet-confirmed long shaft here.** Order `PTV09A-6030S-B103` = rear mount/metal bushing, no detent, 30 mm, 18-tooth knurl, linear 10 kΩ. |
| Alpha **RD901F** | **NOT SOURCED** — `parts.md` §5.2 documents the *bushing* options (Ø7/Ø9 × 5/7/10 mm) but not the shaft-length ladder | M7 or M9 × 0.75, up to **10 mm** | **metal**, Ø6 T18 knurl | The better part *if* a 25/30 mm shaft exists. Ask the distributor. §7.6 |
| Alpha **RV16AF** / Adafruit **562** | **NOT SOURCED** — length codes not in `parts.md` | M6–M8 × 0.75, 5 / 6.5 / 7 / 8 mm | **metal**, Ø6 T18 | Best feel, detent options (11/21/41 clicks) `[DS RV16AF via parts.md]`, but also **16.6 mm of sideways terminal reach** `[STEP 562]` — the dimension that actually collides at a 38.10 pitch. |
| Bourns **PTA** | n/a — slide | n/a | n/a | Not a rotary. |

**A plastic shaft and a set-screw knob are a poor pairing.** On the
`PTV09A-6030S` the shaft is an **18-tooth knurl**, which gives the set screw
something to bite into and makes it a push-fit part by design — so either use a
push-on T18 knob (Adafruit 2046/2047/2048/2058 `[AF]`) or accept that the Ø20
set-screw knob (5527) will emboss the plastic the first time it is tightened and
is thereafter a one-position fit. **If the pots must feel like the encoders, the
metal-shaft RD901F is the part and its shaft ladder has to be confirmed first
(§7.6).**

Bushing hole in the carrier at the pot: **Ø9.4 finished** for M9 × 0.75 (washer
ID is 9.1 ± 0.3 `[DS ptv09 p3]`); carrier local thickness ≤ 5.0 − 2.4 =
**2.60 mm** at the pot `[DERIVED]` — which the 1.270 mm boss satisfies with
1.33 mm to spare.

## 2.6 The whole stack, in one block

```
                                                   Z (mm)   source
  knob top (O20 x 12.5, floating 1.00 above panel)  32.55   [AF 5527] + [DERIVED]
  shaft tip, encoders AND pots                      31.35   [DERIVED, design rule]
  nubbin top (fader)                                25.35   [DERIVED]
  fader shank tip                                   25.05   [DERIVED]
  --------------------------------------------------------
  acrylic TOP face                                  19.05   = 3/4 in = 1 key unit
  pot body top inside the fader window              16.05   (3.00 recessed)
  acrylic UNDERSIDE / encoder shoulder               6.35   = 1/4 in
  --------------------------------------------------------
  NeoSlider PCB front face                           9.55   (on a 7.98 plinth)
  pot nut + washer                              1.27-3.67
  pot mounting boss top                              1.27
  CARRIER TOP FACE = wiring plane                    0.00   <- datum
  breakout PCB front faces                          -0.15   (sunk flush)
  breakout PCB bottoms                              -1.72
  wiring channel floors                             -3.00
  STEMMA QT socket bottoms                          -4.68   [STEP: 2.96 below PCB]
  carrier bottom face                               -7.62   = 6 units
  --------------------------------------------------------
  back panel, 3.00 laser-cut              (set by the back-shell thread)
```

Total above the wiring plane: **19.050 mm**. Carrier: **7.620 mm**. Overall
front-to-carrier-back: **26.670 mm = 21 units = 1.050 in.** `[DERIVED]`

---

# 3. Can a laser cut 12.70 mm acrylic?

## 3.1 Power, speed, focus — from a laser maker

Trotec's own acrylic guide, read directly `[TROTEC]`:

| question | answer, verbatim |
|---|---|
| power | "a good rule of thumb for achieving a polished cut and good production speed is **10 watts per 0.04 inches (1 mm)**" → **127 W for 12.70 mm** `[DERIVED]` |
| minimum | "**Sixty watts can process 0.5 inches (12 mm), however it will be slower and the quality will not be as nice**" |
| the other end | "400 watt for 1.0 inches (25 mm) = best quality" |
| frequency | "start **high on cast acrylic (10 to 20 kHz)**; start **lower on extruded (2 to 5 kHz)**" |
| speed | "To achieve a flame polished cut, you should set your speed to **less than 0.4 in/sec… (600 mm/min)**" |
| lens | "For materials with a thickness of **0.75″ (20 mm) or less, use the 2.5″ lens**" |
| **focus** | "Above 0.25″ (6 mm) **defocus 1/3 into the material**. EX: **Cutting 0.5″ Z-Offset should be −0.17″**" |
| table | "For parts larger than 4″ (100 mm) **or thicknesses above 0.25″ (6 mm), a cutting table with acrylic lamellas** will generally produce the best results" |
| exhaust | "Negative pressure is more important than air flow… Airflow through cut kerf lets acrylic melt from top to bottom. **This gives flame polished cut. Otherwise you will receive wavy horizontal lines**" |
| air assist | "maximum **1.5 PSI (0.1 bar)**" |
| masking | "leave the film on… When processing acrylics with paper backing, at a minimum, **always remove the paper from the top side**… the paper contains **paraffin, which causes flaming and reduces the cutting edge quality**" |

**Read the focus line again: at 12.70 mm you are deliberately cutting out of
focus, 4.3 mm down into the sheet.** That is the single reason taper is
unavoidable at this thickness — the beam waist is in the middle and the kerf
opens above and below it.

**So: 12.70 mm is cuttable, and it is specifically the thickness at which a
hobby machine stops being adequate.** A 60 W desktop tube will get through it,
slowly, with a wavy edge. You want **≥100 W, a lamella table, real downdraft and
a 2.5″ lens** — which means a service, not a makerspace.

## 3.2 Taper, and the hole that closes

No primary measurement of acrylic kerf taper could be retrieved (`acrylic.md`
§11.4 reached the same wall). The community figure is **1–2°, "more noticeable
with thicker materials (¼″ and thicker)"** `[3P taper]`, and an academic study of
laser-cut acrylic found that "**kerf width and taper will increase considerably
with the increase of laser power and nozzle gap and decreasing the cutting
speed**" `[3P ump]` — all three of which are exactly what §3.1 tells you to do at
12.70 mm.

`acrylic.md` §4.3 computed taper as *t* · tan θ per side. At 12.70 mm:

| taper | per side | **across a diameter** | a hole drawn Ø7.40 finishes, at its narrow face | a hole drawn Ø8.20 finishes |
|---|---|---|---|---|
| 1.0° | 0.222 | **0.443** | **Ø6.96** | Ø7.76 |
| 1.5° | 0.333 | **0.665** | **Ø6.73** | Ø7.53 |
| 2.0° | 0.443 | **0.887** | **Ø6.51** | Ø7.31 |

`[DERIVED]`

**At 1° of taper, a Ø7.40 hole has already closed below the Ø7.0 M7 bushing.**
This is a genuinely new constraint that does not exist at 3 mm, where the same
taper costs 0.08–0.16 mm and nobody notices.

Two responses, use both:

1. **Draw Ø8.20 for any M7 bushing through 12.70 mm** (finished Ø8.40 at the
   wide face after a 0.2 mm kerf, Ø7.3–7.8 at the narrow face). The bushing
   passes at every taper in the range.
2. **Stop putting small holes in 12.70 mm sheet.** §3.3.

And the procedural rule `acrylic.md` §4.3 already states, which matters ten times
more here: **mark the reference face on the drawing, cut one coupon, note which
way the taper leans, and keep every subsequent part the same way up. Put the
visible face up**, so the micro-chamfer lands on the edge you look at.

## 3.3 Minimum feature, minimum web — and why the pitch is 38.100

The usual guidance `[3P fab-guides]`, applied at both thicknesses in this stack:

| | guidance | **at 12.70 mm** | at 6.35 mm |
|---|---|---|---|
| minimum hole Ø | 0.5 – 1.5 × *t* | **6.35 – 19.05 mm** | 3.18 – 9.53 mm |
| minimum web / gap | ≥ 1 × *t* | **≥ 12.70 mm** | ≥ 6.35 mm |
| minimum feature width | ≥ 1 × *t* | **≥ 12.70 mm** | ≥ 6.35 mm |
| internal corners | fillet 0.5 – 1.3 mm `[SVC sendcutsend]` | fillet everything | fillet everything |
| hole centre to any edge | **≥ 1.5 × hole Ø** `[RÖHM kb]` | — | — |

**A Ø8.20 bushing hole is at the very bottom of the achievable range in 12.70 mm
stock, and three of them 38.10 apart leave 29.9 mm of web — fine. But the real
move is to stop fighting it:**

> **Give each rotary control a Ø25.400 well instead of a Ø8.20 hole.**

- Ø25.400 = 1 in = 20 units, comfortably inside the 6.35–19.05 minimum-hole band
  — actually above it, which is the safe side.
- The PEC11 body is **12.5 × 13.2 mm** `[DS pec11 p3]`, mounted at **45° to the
  board** `[STEP 5880]`, giving a board-aligned keep-out of 16.7 mm `[STEP]` to
  18.2 mm `[DERIVED from DS]` — a Ø25.400 well clears it on either reading, so
  **the acrylic can descend past the bushing to the board if you ever want it to**.
- The Ø20 knob over a Ø25.400 well leaves a **2.70 mm annular reveal**, through
  which you see the encoder body, the PCB, and — critically — the 5880/4991's
  reverse-mount NeoPixel, which sits **10.41 mm off the knob axis** `[STEP 4991]`,
  i.e. **inside a Ø25.400 (R12.70) well.** The per-encoder indicator lights its
  own well, with no light hole to cut and no light pipe. `acrylic.md` §7.5 could
  not have this because its face layer was opaque.
- Röhm's point-fastening rule — "the distance from the hole center to the edge of
  the sheet should be **at least 1.5 times the diameter of the drilled hole**"
  `[RÖHM kb]` — gives **1.5 × 25.400 = 38.100 mm** as the minimum centre spacing
  and minimum centre-to-edge.

```
   38.100 = 1.5 x 25.400   Rohm's hole rule          [ROHM kb]
   38.100 - 25.400 = 12.700 web = 1 x thickness      [3P fab-guides]
   38.100 = 2 x 19.050 = 2 key units                 [parts.md / layouts]
   38.100 = 30 x 1.27 mm                             [acrylic.md §10]
```

**Four independent constraints land on the same number.** For the user's 2 × 3
grid — three encoders above three pots — at 38.100 pitch:

```
centre-to-centre field   76.200 x 38.100     = the Pattern-B board length [STEP]
outer field, well to well 101.600 x 63.500   = 4.000 in x 2.500 in       [DERIVED]
```

**63.500 is the encoder-grid height `v2/layouts/README.md` already aligns to the
display's 63.60 mm active height**, to 0.10 mm. The pitch that the material
demands is the pitch the layout already wanted.

The layout thread owns where that grid sits; this document owns the fact that
**38.100 is the smallest pitch the material permits** and that going tighter
means giving up the wells.

## 3.4 One 12.70 sheet, or two 6.35 laminated?

The whole appeal of lamination is that it makes local thickness free — a pocket
becomes a hole in the upper lamina. `acrylic.md` §2 built its entire plate on
that logic. **For a clear panel, do not.**

**Does a good cement joint disappear?** Optically, yes — in principle:

| | | source |
|---|---|---|
| PMMA refractive index n<sub>D</sub><sup>20</sup> | **1.491** | `[RÖHM gsxt]` |
| Reflection loss, each surface | **4 %** | `[RÖHM gsxt]` |
| Absorption in the visible | **< 0.05 %** | `[RÖHM gsxt]` |

A solvent or reaction cement for PMMA *is* PMMA once cured, so a void-free joint
has no refractive-index step and no reflection. A **dry** (screwed, unbonded)
lamination has two new air interfaces and therefore **8 % loss and a visible
plane** `[DERIVED from RÖHM gsxt]` — so for clear acrylic, screwing the laminae
together, which is what `acrylic.md` §2.5 recommends for the opaque plate, is
**not** available.

**And in practice:**

- TAP Plastics' own product bulletin: "**bubble-free laminations are difficult
  but possible and generally require experimentation**", and it advises cutting
  oversize so you can "trim the laminated piece to a bubble-free edge after
  curing" `[3P tap]`. You cannot trim a laser-cut panel to a bubble-free edge —
  the edge is the design.
- Every bubble is **two 4 % mirrors** in the middle of the window `[DERIVED]`.
- Röhm: "**Adhesive and adhesive fumes must be prevented from entering the
  cavities, since this may lead to stress cracking**" `[RÖHM fab via acrylic.md]`.
  This panel is nothing but cavities.
- Trotec: before joining, "**temper your material for a few hours at 80 degrees
  Celsius**" to avoid micro cracks `[TROTEC]`. ACRYLITE: let cemented parts sit
  **≥ 6 hours** before annealing, or rapid solvent evaporation bubbles the joint
  `[RÖHM anneal]`.
- It is **irreversible**, on an object whose brief says it must come apart.

The right products, if you do it anyway: **Weld-On 40** — two-part reactive
acrylic, **20 : 1 mix**, tack-free ~20 min, hard in **45–60 min**, explicitly sold
for "**laminating thick sheets of acrylic together to create heavy blocks**"
`[3P weldon40]`; or Röhm **ACRIFIX 1R 9019**, whose listed typical application is
"**bonds of laser-cut elements**" `[RÖHM bonding via acrylic.md]`.

**What to do instead: two separate plates at two levels.** A 12.70 mm slab and,
where you need it, a 6.35 mm plate, both seating on the same plane in the
carrier, separated by a drawn **reveal** rather than a bond line. The step *is*
the detail. This is `acrylic.md` §6.3's "the reveal is the tolerance, made into a
line", applied in Z instead of in plan — and it has the property that **nothing
in the stack is flush with anything**, which is what makes §4.2 survivable.

In the recommended build you do not even need the second plate: the long-shaft
encoders work under the full 12.70 and the fader is in a window. The 6.35 plate
stays available as the fallback if the 25 mm PEC11 cannot be sourced.

## 3.5 Who will cut it, and what it costs

| service | 12.70 mm clear? | tolerance | notes |
|---|---|---|---|
| **SendCutSend** (US) | **Yes — .500″ listed, cast only** | **± .009″ = ±0.23 mm** (a blanket figure, not thickness-qualified) | 2–4 day production; **$39 order minimum**, parts "from $0.50" `[3P sendcutsend-pricing]`. `[SVC sendcutsend]` |
| **Ponoko** | **No — 1.5 to 9 mm** `[3P ponoko-cat]` | ±0.13 mm | Out. The best one-off laser service for this project cannot cut the user's chosen thickness. |
| **Visionteksystems** (UK) | **Yes — "1 mm to 15 mm", "a polished edge without flame or diamond polishing"** | not published | `[3P visiontek]` |
| **Schaeffer / Front Panel Express** | Perspex 2/3/4 mm only; **milled parts to 10 mm** | not published | Mills — the escape hatch, but not at 12.70 and not laser. `[SVC schaeffer via acrylic.md]` |
| local acrylic fabricator | usually | theirs | The only route that will also laminate (§3.4) or diamond-polish (§4.3). |

**Material cost anchor:** Professional Plastics clear acrylic, **0.500″ × 12 × 24
in: $50.79** `[3P zoro]`. A 295 × 190 mm piece is 11.6 × 7.5 in = 87 in², against
288 in² for that sheet → **≈ $15 of material**, before cutting. Add the service
minimum and a one-off 12.70 mm panel is realistically a **$40–90** part, not a
£15 one. Budget for cutting **two** — the coupon in §6.3 and the panel.

**Standard sheet sizes** `[RÖHM gsxt]`:

| | thickness range | standard sheet |
|---|---|---|
| PLEXIGLAS **GS** (cast) | **2 – 160 mm** solid sheet/block | 3050 × 2030 mm up to 25 mm |
| PLEXIGLAS **XT** (extruded) | **1.5 – 25 mm** | 3050 × 2050 mm |

Both cover 12.70. US stock is 48 × 96 in.

---

# 4. Clear acrylic, specifically

## 4.1 Clarity at 12.70 mm — the question answers itself

From Röhm's own property table `[RÖHM gsxt, DIN 5036 Part 3 / ISO 489]`:

| | PLEXIGLAS **GS** (cast) | PLEXIGLAS **XT** (extruded) |
|---|---|---|
| Transmittance τ<sub>D65</sub> | **~92 %** | **~92 %** |
| Reflection loss, **each** surface | **4 %** | **4 %** |
| **Absorption in the visible range** | **< 0.05 %** | **< 0.05 %** |
| Refractive index n<sub>D</sub><sup>20</sup> | 1.491 | 1.491 |
| Max. permanent service temperature | **80 °C** | **70 °C** |

Three conclusions, all from that one table:

1. **Cast and extruded are optically identical.** ~92 % both, 4 % both,
   < 0.05 % both. There is no clarity argument between them, in either direction.
2. **Thickness is irrelevant to clarity.** 4 % + 4 % = 8 % of surface reflection
   accounts for essentially the entire 8 % shortfall from 100 %. The *path* costs
   under 0.05 %. A 12.70 mm sheet transmits what a 3 mm sheet transmits.
3. **There is no green edge, and the reason is the < 0.05 %.** A green cast in
   thick float glass is path-length absorption from its iron content; PMMA has
   nothing to absorb. (I did not retrieve a primary glass datasheet to put a
   number on the comparison — §7.7 — but the PMMA half of it is primary and
   decisive on its own.)

**The user's instinct is right: a 12.70 mm clear slab really is as transparent as
a thin one, and the thick edge really will read as a solid colourless bar.**

## 4.2 Cast, and the price of it

This reverses `acrylic.md` §0.2 for *this* part, and the reversal is principled.
`acrylic.md` chose extruded because a flush plate carrying bushings lives or dies
on thickness tolerance. **Nothing in this stack is flush and nothing bears on
the acrylic's thickness** (§2.1 sets the knob plane from the acrylic's *top*
face, which you measure on the part you actually receive). So thickness becomes a
free variable, and the two things that are not free decide it:

| | cast (GS) | extruded (XT) |
|---|---|---|
| **laser-cut edge quality at 12 mm** | "**cast acrylic tends to produce a smoother and more flame-polished edge directly from the laser**"; "**extruded edges never match cast edges on a laser**" `[3P cast-v-extruded]` | contested; one source calls extruded better for speed but notes it "can leave a burr on the edge" `[3P cast-v-extruded]` |
| **internal stress** | lower | Röhm's own causes-of-tensile-stress list includes "**internal stress in PLEXIGLAS XT… due to manufacturing technique**" `[RÖHM mach §8]` |
| laser frequency | start **high, 10–20 kHz** `[TROTEC]` | start **low, 2–5 kHz** `[TROTEC]` |
| service availability at .500″ | **SendCutSend: cast only** `[SVC sendcutsend]` | .500″ is not an extruded nominal — see below |
| **thickness tolerance** | **catastrophic** — below | tight |

**And the price, which has to be said out loud:**

```
cast, 0.500 in nominal (12.70 mm)
   ePlastics table:  0.425 - 0.535 in  =  10.79 - 13.59 mm   [3P eplastics]
   Rohm GS formula  +-(0.4 + 0.1 x t)  =  +-1.67 mm          [ROHM tol via acrylic.md]
                                       ->  11.03 - 14.37 mm

extruded, 0.472 in nominal (12.0 mm)
   ePlastics table:  0.448 - 0.496 in  =  11.38 - 12.60 mm   [3P eplastics]
   Rohm XT: +-5% at >=3 mm             =  +-0.60 mm          [ROHM tol via acrylic.md]
                                                                    [DERIVED]
```

**A sheet sold to you as "half-inch" may arrive at 10.79 mm and be entirely
within spec.** That is 1.9 mm thinner than nominal — 15 % — and it is why §2.1
references the knob plane to the measured top face rather than to a drawing.

Note also, and it matters for the user's exact words: **0.500 in is a cast-only
nominal.** The ePlastics extruded table has no .500″ row at all; its nearest is
0.472″ = 12.0 mm `[3P eplastics]`, and ACRYLITE's US extruded line stops at .472″
`[RÖHM satinice-us via acrylic.md]`.

> **If you want exactly 0.500 inch, you are buying cast. If you want extruded's
> tolerance, you are buying 12 mm and giving up the half-inch.**

Given that nothing is flush, and that the edge is on display: **buy cast,
0.500 in.**

## 4.3 The edge, which is half the object

At 12.70 mm the cut face is a **12.70 mm wide polished bar running round the
whole perimeter** — on a 295 × 190 panel that is roughly 12,300 mm² of visible
edge, comparable to a tenth of the front face. It is not a detail.

**What the laser gives you.** Trotec's guide is titled *How to Achieve Flame
Polished Edges* and is explicit that the flame-polished laser edge is a function
of exhaust, not power: "Airflow through cut kerf lets acrylic melt from top to
bottom. **This gives flame polished cut. Otherwise you will receive wavy
horizontal lines**" `[TROTEC]`. With ≥100 W, < 600 mm/min, a lamella table and
real downdraft, the laser polishes the edge as it cuts — this is the one thing
acrylic does that no other sheet material does.

**Where it stops.** One trade source puts the limit at exactly our thickness:
"**Acrylic ¼″ or less can be laser cut for a polished edge**" and "below about
12 mm thickness in cast material, laser-cut edges are often good enough to ship
without secondary finishing. Above that… touch-up work is usually needed"
`[3P cast-v-extruded]`. **12.70 mm is on the wrong side of that line by 0.7 mm.**
Expect faint horizontal striations and the taper of §3.2.

**Flame polish vs frosted vs diamond, at 12.70 mm:**

| finish | at this thickness | source |
|---|---|---|
| **as-lasered** | semi-polished, with striations; free; the taper is visible as a very slight bevel on one face | `[TROTEC]`, `[3P taper]` |
| **torch flame polish** | "traditional flame polished acrylic panels **tend to be slightly irregular and do not achieve the same levels of clarity** as the diamond edge polisher"; "flame polishing acrylic… **sets up heat stresses that very easily fail into micro cracks**" | `[3P simplyplastics]`, `[3P signs101]` |
| **diamond polish** | the best optical edge, but "will only work on **straight or sloping sides**… **cannot be used to produce rounded corners**" — so a radiused panel cannot be fully diamond-polished | `[3P simplyplastics]` |
| **deliberately frosted** | a raster pass or a bead blast; kills the bar-of-glass look entirely; also the one finish that hides taper | `[DERIVED]` |

**Recommendation: as-lasered, cast, on a ≥100 W machine with lamellas, and
square corners rather than radii** — because square corners are the only ones a
diamond polisher could rescue if the laser edge disappoints, and because a 12.70
mm bar with a 12.70 mm corner radius looks like a lozenge, not an instrument.
If you radius the outline, you have chosen the laser edge permanently.

**Anneal after cutting, and understand what that costs.** ACRYLITE's schedule
`[RÖHM anneal]`:

```
temperature   180 F = 80 C, "just below the deflection temperature"
heating       1 hour per millimetre of thickness, minimum 2 hours
cooling       thickness(mm) / 4  hours, for sheet above 8 mm
cooling rate  28 F/hr thin, 22 F/hr at 9.5 mm  (~12 C/hr)
remove        below 140 F = 60 C
oven          forced air, 150-250 ft/min, control +-10 F (+-6 C)

for 12.70 mm:  heat 12.7 h  +  cool 3.2 h  =  ~16 hours          [DERIVED]
and: "For parts with holes drilled entirely through the sheet, position the
part so heated air flows through the hole."                      [ROHM anneal]
```

**A sixteen-hour oven cycle in a forced-air oven held to ±6 °C.** A domestic oven
cannot do it. This is a service operation, or a reflow oven with a thermocouple,
and it is the single biggest hidden cost in the whole panel. The payoff, in
ACRYLITE's words: "**Annealing reduces to a minimum the likelihood that crazing
or large-scale cracking will occur**", and it "can increase bond strength by more
than 50 %".

## 4.4 Crazing, whitening, yellowing — and fingerprints

**Crazing.** Röhm's mechanism, quoted in full in `acrylic.md` §5.1, is that PMMA
"withstand[s] considerable tensile stress **as long as they are not exposed to
corrosive media at the same time**", and the list of corrosive media includes
"**monomer vapors during laser cutting**" `[RÖHM mach §8]`. A laser-cut edge is
*by definition* a stressed surface that has just been bathed in monomer vapour.
Röhm adds: "**Stress generated in the immediate vicinity of cut edges may have to
be relieved by subsequent annealing.**"

**On a transparent panel this is not a strength problem, it is a visibility
problem.** Crazing is "**numerous tiny cracks in the material**" `[RÖHM anneal]`.
On an opaque plate you would never see them. On a 12.70 mm clear slab, a craze
field around a bushing hole is a **white scatter halo** you look straight through
— and it grows. It would appear exactly where the hand is, around the knobs, and
exactly in the deep bores where the material is most stressed.

**This is the strongest argument in this document for the Ø25.400 wells and
against Ø8.20 bores**, independently of §3.2's taper argument: a big, gently
curved, low-stress-concentration aperture crazes far less readily than a small
deep bore, and Röhm's own advice for recesses is "**holes should be predrilled at
the corners to avoid notch effects**… the drill holes should be slightly
chamfered or countersunk" `[RÖHM mach §2.4, §3.1]` — for a laser, **fillet every
internal corner; a sharp internal corner in acrylic is a designed-in crack.**

**Stress whitening around a tightened nut** — the other classic failure — does
not arise in this stack, because **nothing is nutted to the acrylic.** Every nut
lands on the printed carrier. That is a real benefit of the window-not-panel
construction and it should be stated: §2.5's three M9 nuts are on PETG, not PMMA.

**Yellowing.** Röhm's own summary for both grades: "**unequalled resistance to
weathering and aging**" `[RÖHM gsxt]`, and the standard grades are "largely
UV-absorbing" (GS 0F00) / "largely UV-absorbing" (XT 0A000). A desk instrument
indoors will not yellow. A specific Δ-yellowness-index figure is **NOT SOURCED**
(§7.8); the qualitative claim is from the material maker.

**Fingerprints, and the trap in the obvious answer.** This is an instrument that
people put their hands on, and gloss PMMA is a fingerprint catalogue. The usual
fixes, each tested against *this* brief:

| option | effect on fingerprints | **effect on seeing the wiring** |
|---|---|---|
| **frosted/satin BACK face** | none (wrong face) | **Fatal.** The back face is the window. Satinice's own selling point is that it is "insensitive to traces of wear and finger marks" `[RÖHM satinice-eu via acrylic.md]` — which is a selling point for the face you *touch*, not the one you *look through*. |
| **matte / non-glare FRONT face** (ACRYLITE Gallery non-glare **P99**, "subtly textured non-glare surface", retailers quoting 90–91.9 % transmission `[3P acrylite-p99]`) | good | **Bad.** A scattering surface blurs by roughly (scatter angle × distance), and the wiring plane is **19.05 mm behind the front face** `[DERIVED §2.1]`. The wires become soft shapes. Plaskolite's own non-glare data also shows haze climbing **0 → 11.2 after 10 abrasion cycles** `[3P plaskolite]` — it wears. |
| **hard-coated matt** — PLEXIGLAS **Optical HCM**: "solid sheets with a **non-scratch coating** and very good abrasion and chemical resistance… high-gloss (**HC**) or **matt (HCM)**" `[RÖHM gsxt]` | good, and durable | same blur problem as above, but the **HC (gloss) variant** is genuinely interesting: scratch resistance without scatter. Availability at 12.70 mm is **NOT SOURCED** (§7.9). |
| **matte front with polished windows** | partial | Achievable only by mask-and-blast or mask-and-etch, not by laser. A real option for a one-off; a two-process part. |
| **gloss, and a cleaning rule** | none | **Perfect clarity.** |

**Recommendation: gloss, both faces, and treat cleaning as a design constraint
rather than a care note.** Röhm `[RÖHM mach §9]`:

> "**Only clear water is needed** to clean and preserve PLEXIGLAS®. If the dirt
> pickup is more pronounced, the water should be **warm and contain a mild
> household detergent**. **Dry rubbing must be avoided at all cost.**"

And ACRYLITE: the sheet "is **subject to crazing, cracking or discoloration if
brought into contact with incompatible materials**… **cleaners, polishes,
adhesives, sealants…**" `[RÖHM satinice-us via acrylic.md]`.

> **This instrument may not be wiped with isopropyl alcohol.** IPA is the first
> thing any engineer reaches for, it crazes PMMA, and §4.4's first paragraph
> explains why a laser-cut panel is the worst possible candidate for it.
> Warm water, a drop of washing-up liquid, a damp microfibre, never dry.

If that is unacceptable, the panel wants the HC hard coat, or it wants to not be
acrylic.

## 4.5 The display, which nobody asked about

The board is a **Waveshare ESP32-S3-RLCD-4.2: a 4.2 in reflective LCD, no
backlight** (`docs/HARDWARE.md`). A reflective display works by sending ambient
light in and bouncing it back out. Put 12.70 mm of clear acrylic over it and the
light crosses **two surfaces going in and two coming out**, at 4 % each
`[RÖHM gsxt]`:

```
0.96^4 = 0.849   ->  15.1 % of the display's light is gone      [DERIVED]
```

Plus — and this is the bigger problem — the acrylic's **front surface reflects
4 % of every overhead light straight back at the user**, as a sharp specular
image, with no backlight behind the display to overpower it. On a glossy phone
you don't notice because the panel is emissive. On a reflective LCD, 4 % of a
ceiling light can be brighter than the display itself.

**Options, in order:**

1. **An aperture.** The 12.70 mm slab becomes a bezel; the display sits in a
   through-window like the fader. Zero loss, zero veiling glare, and the 12.70 mm
   cut edge frames the screen as a polished reveal. **Do this.**
2. Anti-glare / non-glare acrylic **over the display region only** — kills the
   specular image, costs contrast to haze, and §4.4 explains why you do not want
   it over the wiring.
3. Glaze it and accept 15 % plus glare. Only if the slab must be continuous.

This belongs in the layout thread's decision, but it is a *stack* fact and it
would be negligent to leave it out: **the one part of the panel that most looks
like it wants glazing is the one part that must not have it.**

---

# 5. The carrier plate — the real design

The wiring is on the top face, facing the user, seen through the acrylic. That
makes the interior a finished surface with a manufacturing process, and the
process is FDM, which has opinions.

## 5.1 Channels: width, depth, radius

**What has to fit.** Two populations:

| cable | OD | source |
|---|---|---|
| Adafruit silicone stranded hook-up, **30 AWG** | **0.8 mm** | `[3P core-electronics / AF 2519]` |
| Adafruit silicone stranded hook-up, **26 AWG** | **1.5 mm ± 0.1** | `[3P core-electronics / AF 2519]` |
| **STEMMA QT cable** (4 conductors, JST SH 1 mm) | **NOT SOURCED** — Adafruit publishes no gauge or OD (§7.10) | `[AF cat-619]` |

**Channel sizing rule** `[DERIVED]`, with the FDM tolerance that actually
governs — Protolabs: **±0.5 % with a ±0.5 mm minimum** for prototyping FDM,
**±0.25 % / ±0.25 mm** industrial `[SVC protolabs via acrylic.md]`:

```
channel width  = cable OD  +  2 x FDM tolerance  +  0.3 mm clearance
channel depth  >= cable OD + 0.5        (so the cable sits BELOW the plane)
```

| what | drawn width | drawn depth | holds |
|---|---|---|---|
| single 30 AWG | **2.00** | **1.50** | one 0.8 mm wire with slop to spare |
| single 26 AWG | **2.60** | **2.00** | one 1.5 mm wire |
| **a STEMMA QT cable, 4 wires side by side** | **4.00** | **2.00** | until the OD is measured, this is sized for 4 × 0.8 = 3.2 mm `[DERIVED, see §7.10]` |
| **a pot's 3 wires** | **3.00** | **2.00** | three 26 AWG at 1.0 pitch |
| **trunk** (two QT cables stacked) | **4.00** | **3.00** | the §2.6 channel floor at Z = −3.00 |

**Rib between channels: 1.270 mm (1 unit).** FDM minimum wall is 2 × nozzle
width — 0.8 mm on a 0.4 mm nozzle — so 1.270 is three extrusions wide and prints
solid with no infill, which is also what makes its top surface iron cleanly.

**Bend radius.** No primary standard could be retrieved (§7.11). The secondary
guidance is **≈3 × OD stationary, ≈10 × OD flexing** for stranded wire
`[3P pcbsync]`, with aerospace practice at 10 × `[3P lectromec]`. Nothing here
flexes in use. Taking 3 × on the largest cable:

```
3 x 1.5 mm = 4.5 mm        ->  every channel corner R >= 5.00 mm   [DERIVED]
```

**R5.00 at the centreline inside a 12.70 mm cell** leaves an inner wall radius of
3.75 and an outer of 6.25 for a 2.5 mm channel — it fits the module. Use **R6.35
(½ unit of the module)** where there is room, because a radius that is a module
fraction reads as deliberate and a radius that isn't reads as a fillet.

**Keeping it orthogonal.** Three rules, and they are the whole aesthetic:

1. **Every channel centreline lies on the 6.350 mm module**, in X or in Y. No
   diagonals. A diagonal run through a grid of orthogonal runs is the single
   fastest way to make a wiring plane look accidental.
2. **Every turn is 90°, at R6.35.** No 45° cuts, no splines.
3. **Every crossing is a bridge, and the bridge is drawn.** Two channels that
   meet must not merge into a puddle. The lower run drops to a second depth and
   passes under a printed bridge:

```
   upper channel floor      Z = -3.00
   bridge roof              Z = -3.20      (0.20 clearance to the upper cable)
   bridge thickness                1.20    (3 layers at 0.4, or 10 at 0.12)
   lower channel floor      Z = -6.00      (bridge underside at -4.40,
                                            1.60 of clear height for the lower cable)
   carrier bottom face      Z = -7.62      -> 1.62 mm of floor under the deepest channel
                                                                    [DERIVED]
```

A 1.20 mm bridge spanning a 4.00 mm channel is a **3.3 : 1 bridge** — well inside
what FDM spans unsupported, and it prints as part of the ironed top surface
because its roof is only 0.20 mm below the plane. **Crossings become a visible,
countable feature rather than a tangle.** Make all of them the same width and put
them on the grid.

## 5.2 STEMMA QT cable lengths — the thing that decides whether this looks good

Adafruit's actual catalogue, read off the product pages `[AF cat-619]`,
`[AF prod-4912]`:

| length | PID | price (1-off) | note |
|---|---|---|---|
| **50 mm** | **4399** | **$0.95** ($0.86 @10, $0.76 @100) | socket-to-socket |
| **100 mm** | **4210** | — | socket-to-socket; **out of stock** at time of reading |
| **200 mm** | **4401** | **$1.25** | socket-to-socket |
| **300 mm** | **5384** | **$1.25** | socket-to-socket |
| **400 mm** | **5385** | **$1.50** | socket-to-socket |
| 150 mm | 4209 | $0.95 | JST SH → **male headers** |
| 150 mm | 4397 | — | JST SH → **female sockets**; out of stock |
| 200 mm | 4424 | $0.95 | **JST PH** → JST SH |

**There is no 150 mm socket-to-socket cable and there is nothing between 100 and
200.** The usable ladder is **50 / 100 / 200 / 300 / 400**, and `parts.md` §11's
"50/100/200 mm — 4399 / 4210 / 4401" is correct as far as it goes.

**Why this matters more than it looks.** A fixed-length cable spanning a fixed
distance has exactly one degree of freedom: the path. Under a clear panel, slack
you did not design is slack everyone can see.

```
slack  =  cable length  -  socket-to-socket path length
jog    =  slack / 2     (an orthogonal out-and-back detour adds 2 x its depth)
                                                                    [DERIVED]
```

| hop (socket to socket) | with a 50 mm cable | with a 100 mm cable |
|---|---|---|
| 25.40 (1 in) | slack **+24.60** → jog **12.30** | — |
| **38.10 (the control pitch)** | slack **+11.90** → **jog 5.95 mm** | slack +61.90 → jog 30.95 |
| 50.80 (2 in) | **−0.80 → DOES NOT REACH** | slack +49.20 → jog 24.60 |
| **76.20 (a Pattern-B board)** | **−26.20 → DOES NOT REACH** | slack **+23.80** → **jog 11.90 mm** |
| 114.30 | — | — (use 200 mm: slack +85.70) |

`[DERIVED. Path length is socket-centre to socket-centre plus the allowance for
leaving each socket along the board's long axis — the QT sockets face outward in
line with each other `[STEP]`, so a straight-through chain adds only the board
overhang, 2.54 mm per end `[STEP]`.]`

**The rules that fall out:**

1. **A 50 mm cable will not cross a 76.200 mm board.** Any hop of 50.80 mm or
   more needs a 100 mm cable — and the 100 mm cable (4210) was **out of stock**.
   Check stock before fixing the layout, and **buy the 100 mm cables first**,
   because they are the ones the geometry demands and the ones that run out.
2. **At the 38.100 mm control pitch, a 50 mm cable has 11.90 mm of slack and the
   channel must absorb all of it as a 5.95 mm orthogonal jog.** Draw the jog. On
   a 2 × 3 grid, six identical jogs in the same direction is a pattern; six
   random loops is a mess.
3. **Design the channel path to the cable, not the cable to the channel.** This
   is the inversion that makes the interior work: the channel's *path length*
   is the specification, and its shape is how you hit it.
4. **Always round the jog DOWN to the module, never up.** 5.95 mm is 4.7 units;
   **take 5.080 mm (4 units)**, which adds 10.16 mm of path and leaves 1.74 mm of
   residual slack for the connector's strain relief and the channel's own slop.
   Rounding *up* to 6.350 would add 12.70 mm of path against 11.90 mm of cable —
   **0.80 mm short, and a cable that does not reach is not a cosmetic problem.**
   Slack forgives; shortfall does not. `[DERIVED]`
5. The fader's plinth is 7.980 mm tall (§2.4.4). Its two cables enter it through
   **3.00 × 5.00 mm windows cut in the plinth's end walls at the wiring-plane
   level**, so the cable stays flat on the plane and climbs only inside the
   plinth where it is hidden by the plinth itself. `[DERIVED]`

## 5.3 Retaining the cables

FDM tolerance (±0.5 mm prototyping `[SVC protolabs]`) is larger than the
interference a snap-fit groove needs, so **a channel that grips by being
slightly narrow than the wire will not work reliably.** Three options that do:

| method | geometry | verdict |
|---|---|---|
| **printed combs** | a separate printed strip, 1.27 mm thick, spanning several channels, screwed down at each end into M2 inserts | **Recommended.** Removable, repeatable, and it reads as a deliberate repeating element through the acrylic — a run of combs at a constant pitch looks like a loom, which is the right metaphor. |
| **printed cantilever clips** | a 1.6 mm finger with a 0.6 mm lip over the channel, printed as part of the carrier | Prints without support if the lip overhangs ≤ 0.6 mm; fatigues in PLA, acceptable in PETG; cannot be replaced if it snaps. Use for single wires, not trunks. |
| **cover strips** | a 1.5 mm clear acrylic strip over a run, screwed down | Adds a third material and two more 4 % surfaces over the wire. No. |
| adhesive | — | Röhm: adhesives and their fumes cause stress cracking in PMMA `[RÖHM fab via acrylic.md]`, and this is a part you will open. No. |

**Comb pitch: 25.400 mm (1 in, 20 units).** Long enough that the combs are
countable, short enough that no cable sags between them.

## 5.4 Mounting the breakouts — and a correction to the brief

The task asks for M2.5 heat-set insert sizes. **M2.5 will not fit.**

```
breakout mounting holes:   O2.50          [STEP, every board in parts.md §1]
M2.5 screw clearance:      O2.7 - O2.9    (close/medium fit)
M2   screw clearance:      O2.4           -> passes a O2.50 hole with 0.10 to spare
                                                                    [DERIVED]
```

**Use M2 for the boards.** M2.5 is correct only for the panel screws, where you
drill the hole yourself.

| | M2 (boards) | M2.5 (panel) |
|---|---|---|
| part | ruthex **RX-M2x4** | ruthex **RX-M2,5x5.7** / `GE-M25x57-001` |
| ⌀D1 (knurl max) | **NOT SOURCED** (§7.12) | **4.6** `[3P ruthex-3djake]` |
| ⌀D2 | — | **3.9** |
| ⌀D3 (recommended bore) | — | **4.0** |
| L | **4** (from the part name) | **5.7** |
| W min (wall) | — | **1.6** |
| corroboration | — | Insertsdirect M2.5 Heatfit: "**Recommended Hole Size: 4.00 mm**" `[3P insertsdirect]` |

The ruthex listing gives the table but **does not define which symbol is which**
`[3P ruthex-3djake]`; D3 = 4.0 as the bore is inferred from the convention and
corroborated independently by Insertsdirect. Flagged in §7.12.

**Boss geometry** `[DERIVED from the above]`:

```
M2.5 insert boss:   bore O4.00 x 6.20 deep (L + 0.5 clearance)
                    boss OD >= 4.00 + 2 x 1.60 = 7.20 mm
                    -> draw O7.62 (6 units)
M2   insert boss:   bore per the M2 datasheet (NOT SOURCED), boss OD 6.35 (5 units)
```

**Standoff height is zero.** §2.1 sinks the boards into the carrier so their
front faces sit at Z = −0.15, which means the insert bosses are *below* the
wiring plane, inside the board pocket, and **nothing stands proud of the plane
except the parts themselves.** That is worth protecting: the moment a standoff
pillar appears on the visible plane, the plane stops being a plane.

Pocket geometry per board: **outline + 0.50 mm all round, 1.72 mm deep**, with
local **2.96 mm relief under each STEMMA QT socket** `[STEP: sockets are 4.95 ×
6.00 × 2.96 on the back face]` — and those reliefs are the channel mouths, so
**the connector drops into the channel it feeds.** No transition, no step, no
cable climbing out of a socket and over a lip. `[DERIVED]`

## 5.5 Printing the visible face

The carrier's top face is seen through 12.70 mm of polished acrylic from 150 mm
away. Layer lines are a finish decision, not an artefact.

| decision | choice | why |
|---|---|---|
| **orientation** | **visible face up, flat on the bed** | Ironing cannot rescue an angled or stepped surface; the fix is orientation — "rotate the model so that surface becomes a flat top… even if it means adding supports elsewhere" `[3P ironing-guides]`. Printing face-**down** on a smooth plate would give a better face still, but the channels are *in* that face and would become unsupported voids off the bed. Face up. |
| **layer height** | **0.12 mm**, 0.4 mm nozzle | Three layers to a 0.36 mm feature; the 1.20 mm bridge roof is 10 layers. |
| **ironing** | **on, for top surfaces only** | Flow is printer-specific and the published values disagree wildly — 10 % / 7 % `[3P ironing-guides]`, 15 %, 23 %, 35 %, 43 %. **Start at 10 % and step up on a test patch.** Line spacing **0.15–0.25 mm**; slower = glossier `[3P ironing-guides]`. |
| **two ironed planes** | the wiring plane at Z = 0 **and** every channel floor at Z = −3.00 | Both are flat tops at a single Z, so both iron in one pass each. Keeping *all* channel floors at one depth (and all bridge-unders at a second) is what makes that possible — a varying channel depth costs you the finish. `[DERIVED]` |
| **material** | **PETG** | PLA irons best `[3P ironing-guides]` but creeps under screw preload and goes brittle; PETG needs lower ironing flow and lower temperature and will string, but survives a part that is screwed together and carried. If the part is a one-off display piece, PLA. |
| **colour** | **matte black or very dark grey** | Through clear acrylic a dark matte plane reads as a void, and the coloured wires and nickel nuts become the only things with brightness — maximum contrast for the thing the brief wants seen. White reads as a lightbox and diffuses the NeoPixels nicely but shows every scuff and flattens the wiring into it. |
| **warp** | rib the underside | ~295 × 190 × 7.62 of PETG is a large flat part and it will cup. The channels already rib the top; add a 1.27 mm cross-rib grid on the **underside** on the same 25.400 mm module as the combs. `[DERIVED]` |

**NOT SOURCED:** every ironing figure above is from slicer documentation and
community guides, not from a tested spec for this geometry (§7.13).

---

# 6. Assembly

## 6.1 How the acrylic is fixed

**A laser cannot counterbore 12.70 mm.** `acrylic.md` §2.5's beautiful trick —
a Ø5.2 hole in the upper lamina over a Ø3.2 hole in the lower one *is* a
counterbore — is only available if you laminate, and §3.4 says do not. So the
screw heads are either proud or they are not there.

**Proud, and owned.** Six **M4 button-head** screws from the front, through
**Ø5.60 drawn** holes in the acrylic (finishing Ø5.8 wide face / ~Ø5.1 narrow
face at 1.5° taper `[DERIVED §3.2]` — M4 passes either way), into **M4 heat-set
inserts** in the carrier. ISO 7380 M4 button head is Ø7.6 × 2.2
`[3P fastener-tables]` — it sits 2.2 mm above a 12.70 mm slab, 17 % of the
thickness, which at this scale is a detail and not a blemish, and it puts six
visible fasteners on a panel whose entire thesis is that the fastening is the
design.

Rejected:

| | why not |
|---|---|
| screws from **behind**, through the carrier, tapped into the acrylic | Threads in PMMA under preload are Röhm's own crazing case: "deformation during fastening (clamping, drilling, **screw union**)" `[RÖHM mach §8]`. No. |
| screws from behind into **inserts bonded into blind holes** in the acrylic | A laser cuts no blind holes. |
| **captured by the back panel** — carrier pulled up against the acrylic's underside by the back shell | Works, but then removing the acrylic means dismantling the whole object, which is exactly what the brief forbids. |
| countersunk heads | needs a counterbore. |

**One datum hole, five slots.** `acrylic.md` §6.5's rule applies unchanged and
matters more here, because cast's thickness tolerance (§4.2) is joined by its
length/width tolerance (±0.5 mm cut-to-size `[RÖHM tol via acrylic.md]`): **one
round Ø5.60 hole sets position; the other five are 5.60 × 8.60 slots** giving
±1.5 mm of float so the panel never fights the printed part. And Röhm's reason
for oversizing bolt holes — thermal and moisture movement `[RÖHM kb]` — is real:
PLEXIGLAS's "possible expansion due to heat and moisture" is **5 mm/m**
`[RÖHM gsxt]`, which on a 295 mm panel is **1.48 mm**. The slots are not
optional. `[DERIVED]`

Screw positions: **inside the control wells wherever possible**, where the knob
hides them, and otherwise on the 38.100 grid. Röhm's hole-to-edge rule — centre
≥ 1.5 × Ø `[RÖHM kb]` — puts an M4 screw centre no closer than **8.40 mm** to any
edge or window; use 12.700 (1 module) and it is also on the grid.

## 6.2 The order

```
 1  CARRIER, bare.    Print it. Deburr the channel mouths. Check flatness on glass.
 2                    Install all heat-set inserts (M2 board bosses, M2.5 comb
                      bosses, M4 panel bosses) with a temperature-controlled iron.
                      Inserts first, always: you cannot press one in next to a
                      populated board.
 3  POTS.             Three pots up through the 1.27 mm bosses from below.
                      Washer, nut, on the TOP face. Tighten to "stops turning"
                      and no further - the pot only has to resist 20-250 gf-cm
                      of operating torque [DS ptv09], against a 7 kgf-cm bushing
                      rating [DS RD901F via acrylic.md]. Solder three wires each.
 4  ENCODERS.         Solder PEC11-4225F-S0024 to each 4991 OFF the carrier,
                      on a bench, with a jig that holds the encoder square.
                      (Bourns does not recommend hand soldering - [DS pec11].)
                      Set the three I2C addresses NOW: 0x36 / 0x37 / 0x38 by
                      closing A0 / A1 - solder jumpers, reversible [parts.md 8.3].
 5  ADS1115.          Address 0x48, no jumper. Wire the three pots to A0-A2 and
                      leave A3 free.
 6  BOARDS DOWN.      Drop every breakout into its pocket, M2 screws.
                      Sockets land in the channel mouths.
 7  WIRING.           QT chain, hop by hop, laying each cable into its channel
                      and its designed jog. Do NOT fit the combs yet.
 8  FADER PLINTH.     Thread the fader's two QT cables through the plinth's end
                      windows, seat the 5295, M2 screws.
 9  TEST.             Power up. i2cdetect. Nine addresses. This is the last
                      moment the wiring is free.
10  COMBS.            Fit the combs. Dress each run. This is a finishing
                      operation, not an assembly one - treat it like one.
11  ACRYLIC.          Peel the masking (§6.3). Lower the slab. Datum screw
                      first, then the five slotted ones, finger tight, then
                      quarter-turn each in a cross pattern.
12  KNOBS.            Three encoder knobs and three pot knobs, set screws on the
                      flats. Set the height with a shim so all six match.
                      The fader nubbin pushes on.
13  BACK.             Carrier into the back shell, back panel on.
```

## 6.3 What can be serviced, and what cannot

| | access |
|---|---|
| **back panel off** | the carrier's underside: pot bodies and terminals, the ESP32-S3 board, power. Nothing on the wiring plane. |
| **6 screws + 6 knobs, acrylic off** | **everything on the wiring plane**: every board, every cable, every address jumper, the ADS1115, the fader. Five minutes. This is the whole point of screwing rather than bonding. |
| **a comb off** | one run of cable, without disturbing its neighbours. |
| **the fader nubbin** | push fit, off with fingers. |
| **an encoder** | **not serviceable.** It is soldered to the 4991. This is the cost of §2.3 and it should be stated in the build notes, not discovered. Keep a spare 4991 and a spare PEC11. |
| **the acrylic** | replaceable as a unit; the carrier is untouched. Since §4.4 means the panel is the part most likely to be damaged (crazing, a solvent accident, a scratch), **the fact that it is a single bought-again part is a feature.** |

**The test coupon, cut from the same sheet on the same job.** This closes five
items in §7 for the price of ~500 mm of cut length `[DERIVED from acrylic.md
§8.3]`:

```
- a 25.00 mm square               -> measure kerf and taper, both faces
- bores at O7.80 / O8.20 / O8.60  -> which one passes an M7x0.75 bushing
- one O25.40 well                 -> the finished diameter and the edge quality
- a 12.70 mm web between two wells
- a 2.50 x 80 mm slot             -> can the machine hold it at this thickness
- 50 mm of outline                -> the edge finish you are actually buying
```

## 6.4 Documents used

- Bourns, **PEC11 Series 12 mm Incremental Encoder** — `[DS pec11]` — local:
  `v2/parts/models/datasheets/bourns-PEC11-12mm-encoder.pdf`
- Bourns, **PTV09 Series 9 mm Potentiometer** — `[DS ptv09]` — local:
  `v2/parts/models/datasheets/bourns-PTV09-9mm-pot.pdf`
- Bourns, **PTA Series Low Profile Slide Potentiometer** — `[DS pta]` —
  https://www.bourns.com/docs/product-datasheets/pta.pdf
- POLYVANTIS/Röhm, **PLEXIGLAS Solid sheet, block, multi-skin sheet… (GS/XT)**,
  Ref. No. 211-1, 04/25 — `[RÖHM gsxt]` —
  https://www.voskunststoffen.nl/files/166151/datasheet%20plexiglas%20XT%20GS.pdf
- ACRYLITE, **Annealing ACRYLITE acrylic sheet** — `[RÖHM anneal]` —
  https://www.acrylite.co/resources/fabrication-manuals/annealing-acrylite-acrylic-sheet
- Trotec Laser, **Acrylic Processing Guide: How to Achieve Flame Polished Edges**
  — `[TROTEC]` —
  https://www.troteclaser.com/fileadmin/content/images/News/EN_US/New_Acrylic_Processing_Guide/acrylicprocessingguide.pdf
- Elma, **K1 Metal knobs datasheet**, 06|20 — `[DS elma-k1]` —
  https://static.chipdip.ru/lib/985/DOC025985920.pdf
- ePlastics, **Thickness tolerance of plexiglass/acrylic sheet** — `[3P eplastics]`
  — https://www.eplastics.com/blog/thickness-tolerance-plexiglass-acrylic-sheet
- Adafruit **5527** knob, **5621** nubbin, **4912** cable family, **category 619**
  — `[AF]` — adafruit.com/product/5527, /5621, /4912, /category/619
- ruthex M2.5 insert spec table — `[3P ruthex-3djake]` —
  https://www.3djake.com/ruthex/threaded-insert-m25-70-pieces ; corroborated by
  https://www.insertsdirect.com/product/heatfit-headed-m25-30 `[3P insertsdirect]`
- Adafruit silicone stranded wire OD — `[3P core-electronics]` —
  core-electronics.com.au silicone-cover-stranded-core-wire listings, and
  https://www.mouser.com/datasheet/2/737/Adafruit_Industries_2519_Web-3318622.pdf
- SendCutSend **acrylic** — `[SVC sendcutsend]` — https://sendcutsend.com/materials/acrylic/
- Visiontek Systems **laser cutting services** (1–15 mm acrylic) — `[3P visiontek]`
  — https://www.visionteksystems.co.uk/laser_cutting_services.htm
- Simply Plastics, **diamond & flame polishing** — `[3P simplyplastics]` —
  https://www.simplyplastics.com/fabrication-services/diamond-flame-polishing
- TAP Plastics **Product Bulletin 2** (laminating) — `[3P tap]` —
  https://www.tapplastics.com/image/pdf/Product_Bulletin_2.pdf
- Weld-On 40 properties — `[3P weldon40]` —
  https://www.acplasticsinc.com/media/Weld-On%2040%20Properties.pdf
- Machine Design, **Setscrews** — `[3P machinedesign]` —
  https://www.machinedesign.com/fasteners/setscrews
- Plaskolite **OPTIX 99 / NG** data — `[3P plaskolite]` —
  https://www.curbellplastics.com/wp-content/uploads/2022/11/Plaskolite-OPTIX-99-Acrylic-Data-Sheet.pdf
- ACRYLITE **Gallery non-glare P99** — `[3P acrylite-p99]` —
  https://www.acrylite.co/non-glare-p99-colorless-0a000-ar.html
- Cast vs extruded for laser — `[3P cast-v-extruded]` —
  https://omtechlaser.com/blogs/news/acrylic-laser-cutting ,
  https://www.thunderlaser.com/laser-wiki/material-applications/extruded-acrylic-vs-cast-acrylic
- Kerf/taper study — `[3P ump]` — https://umpir.ump.edu.my/id/eprint/2372/
- Bend radius secondary guidance — `[3P pcbsync]`, `[3P lectromec]` —
  https://pcbsync.com/?p=11427 ,
  https://lectromec.com/minimum-bend-radius-applied-wire-system-degradation/
- Ironing practice — `[3P ironing-guides]` — https://help.prusa3d.com/en/article/ironing_177488 ,
  https://printago.io/guides/orca-slicer-ironing
- Zoro, Professional Plastics clear acrylic .500″ × 12 × 24 — `[3P zoro]` —
  zoro.com item G700907453 *(price read from a search result; the page returns
  403 from this environment — §7.14)*
- `[RÖHM tol]`, `[RÖHM mach]`, `[RÖHM kb]`, `[RÖHM fab]`, `[RÖHM bonding]`,
  `[SVC protolabs]`, `[3P taper]`, `[3P fab-guides]`, `[3P ponoko-cat]`,
  `[SVC schaeffer]` — all as cited, quoted and URL-listed in
  `v2/parts/acrylic.md` §8.4.

---

# 7. NOT SOURCED

Stated loudly, per the project's rule that an unsourced number is a bug.

**Blocking — these change the design if they come back wrong:**

1. **PEC11 nut and washer dimensions.** The datasheet says "One flat washer and
   one mounting nut supplied with each encoder" `[DS pec11 p1]` and does not
   dimension either. The 0.4 + 2.0 = 2.40 mm allowance used throughout is the
   **Bourns PTV09 hardware** `[DS ptv09 p3]` as a class proxy. It governs the
   "max panel with a nut" column in §1.1 and nothing else in the recommended
   build, because nothing is nutted to the acrylic.
2. **Knob bore depth — all four candidates.** Adafruit 5527/5528/5530, Elma K1
   and the Adafruit 2056 all publish height and bore *diameter*; **none publishes
   bore depth.** The only figures found are "11 mm keyway" for the 2056
   `[AF via parts.md §5.5]` and an RS PRO listing's undefined "**Depth 9 mm**"
   field for a Ø20 × 12.5 knob `[DIST rs-pro]`. §2.3.1 shows the design is
   insensitive to this *provided* the bore is ≥ 9 mm. **Measure one on receipt.**
3. **A published rule for set-screw hub engagement vs shaft diameter.** Searched
   and not found. The rules that exist govern the *screw's* thread engagement
   (1–1.5 × major Ø in aluminium `[3P machinedesign]`), not the hub's grip on the
   shaft. The "2.05 × diameter" and "1.4 × diameter" ratios in §2.2–2.3 are
   stated as ratios, not as pass/fail criteria.
4. **NeoSlider 5295 travel**, still — carried forward from `parts.md` §10.1,
   unchanged by anything here. And the **5621 nubbin's slot depth**: the page
   gives "Shaft slot size: 5 mm × 2.2 mm" and no depth `[AF prod-5621]`. §2.4.4
   works because 9.00 mm of shank exceeds any depth a 9.3 mm nubbin can have, but
   the number is still missing.
5. **Bourns PTV09A-6 bushing length.** `parts.md` §10.11 flagged "both a 5.0 and
   a 5.9 dimension in the bushing region". Reading the rendered A-6 drawing, the
   `5.0 (.197)` sits on the dimension chain after `3.5 ± 0.3` in the bushing
   region, and `5.9 (.232)` is a separate leader lower down — consistent with
   **5.0 = threaded bushing length**, but **I will not call it certain from a
   rendered drawing.** 5.0 is used; a 1.270 mm carrier boss works at either value.
6. **Alpha RD901F shaft-length options.** `parts.md` §5.2 documents the *bushing*
   ladder (Ø7/Ø9 × 5/7/10 mm) but not the shaft ladder. The Tayda specification
   for `RD901F-40-15R1-B10K` (SP26020021), fetched and read in full, contains
   only the electrical/mechanical spec and the lead-free soldering sheet — **no
   dimension drawing**. Whether a 25 or 30 mm metal shaft is orderable is
   unresolved, and it is the one thing that would let the pots match the encoders
   in feel. **Ask the distributor.**
7. **The green-edge comparison with glass.** The PMMA half is primary
   (absorption < 0.05 % `[RÖHM gsxt]`). The glass half — iron content, path-length
   absorption, a measured green shift — is general knowledge and **no primary
   glass datasheet was retrieved.** §4.1 is written so the conclusion rests only
   on the PMMA number.
8. **A yellowness-index figure for PMMA over time.** Röhm's "unequalled
   resistance to weathering and aging" `[RÖHM gsxt]` is qualitative. No ΔYI /
   hours-of-QUV number was found.
9. **PLEXIGLAS Optical HC/HCM availability at 12.70 mm.** The grade exists and is
   described in the GS/XT technical information `[RÖHM gsxt]`; its thickness
   range is not given there.
10. **STEMMA QT cable wire gauge and outer diameter.** Adafruit's category page
    and product pages give length, connector and colour code, and **no gauge, no
    OD, no bundle dimension** `[AF cat-619]`, `[AF prod-4912]`. §5.1's 4.00 mm
    channel is sized from 4 × the 30 AWG silicone OD (0.8 mm) as a stand-in.
    **Measure a cable before cutting the carrier's channels** — this is the
    single most load-bearing unmeasured number in §5.
11. **Minimum bend radius, from a standard.** IPC/WHMA-A-620 is paywalled and was
    not read. The 3 × OD (stationary) / 10 × OD (flexing) figures are from a
    vendor summary of IPC-D-620 `[3P pcbsync]` and an aerospace article citing
    AS50881 `[3P lectromec]`. The R5.00 minimum in §5.1 is derived from the
    conservative end of secondary guidance.
12. **ruthex M2 insert dimensions**, and the **definition of ruthex's D1/D2/D3/W
    symbols**. The M2.5 table is published `[3P ruthex-3djake]` but unlabelled;
    D3 = 4.0 as the recommended bore is inferred and separately corroborated
    `[3P insertsdirect]`. The M2 part (`RX-M2x4`) was not found with a dimension
    table.
13. **Every ironing figure in §5.5.** Flow percentages in the published guides
    range from 7 % to 43 % for the same materials. They are starting points.
14. **The $50.79 price for 0.500″ × 12 × 24 in acrylic.** Read from a search
    result; zoro.com returns **403** from this environment, so the page itself was
    not read. Treat as an order-of-magnitude anchor.
15. **Laser hole tolerance specifically at 12.70 mm.** SendCutSend's ±.009″
    `[SVC sendcutsend]` is a blanket acrylic figure, not qualified by thickness,
    and no service publishes a thickness-dependent table. **The coupon in §6.3
    is the only way to know.**
16. **Acrylic kerf taper, measured.** Carried forward from `acrylic.md` §11.4 and
    still open. The 1–2° range is community practice `[3P taper]`, supported in
    direction but not in magnitude by `[3P ump]`. **At 3 mm this was a footnote;
    at 12.70 mm it decides whether a bushing hole exists (§3.2), so it is now a
    blocking unknown and the coupon must answer it.**
17. **5880/4991 encoder body footprint.** The STEP measures a 16.70 mm square
    board-aligned keep-out `[STEP]`; the datasheet's 12.5 × 13.2 body rotated 45°
    computes to 18.2 mm `[DERIVED from DS pec11 p3]`. **A 1.5 mm disagreement.**
    Immaterial against a Ø25.400 well; material if anyone ever tries a tighter
    aperture.
