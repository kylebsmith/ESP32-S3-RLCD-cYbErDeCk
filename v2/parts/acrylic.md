# The flush laser-cut acrylic control plate

*An enclosure input, written to be cut from. Every number carries a source tag.
Where a number could not be traced to a primary source it is marked
**NOT SOURCED** and listed in §11 as a caliper or purchase-order job — it is not
estimated.*

---

## 0. Verdict up front

1. **The plate is two laminae, not one sheet, and the reason is arithmetic
   rather than taste.** Three parts demand three different local thicknesses of
   the same panel: an MX switch needs **exactly 1.5 mm** at its clips
   `[DS cherry-mx]`, a PEC11 encoder bushing permits **≤2.6 mm** of panel
   `[DERIVED §1]`, and a 254 mm-wide plate wants **3 mm or more** to be a plate
   rather than a membrane. A laser cuts constant thickness. Two bonded or
   clamped laminae give all three. §2.

2. **Cast acrylic is the wrong material for this plate and it is not a close
   call.** PLEXIGLAS **GS (cast)** thickness tolerance is **±(0.4 + 0.1 × t) mm**
   — ±0.70 mm on a 3 mm sheet, 23 % of nominal `[RÖHM tol]`. PLEXIGLAS **XT
   (extruded)** is **±10 % below 3 mm and ±5 % at 3 mm and above**
   `[RÖHM tol]` — ±0.15 mm on the same sheet. A flush fit and a bushing-length
   budget both live on that number. **Specify extruded.** This reverses the
   usual laser-cutting default, and the usual reason for the default (cast
   engraves white, extruded does not) does not apply here because
   `envelope.md` deleted all engraving. §3.

3. **The 1.5 mm functional lamina should not be acrylic.** Röhm's own machining
   guide lists "**deformation during fastening (clamping, drilling, screw
   union)**" as a cause of the tensile stress that produces crazing, and names
   "**monomer vapors during laser cutting**" among the corrosive media that
   turn that stress into cracks `[RÖHM mach §8]`. A pot nut on a laser-cut hole
   in 1.5 mm acrylic is both of those at once. Make that lamina **1.6 mm FR4**
   — the same material and the same thickness as every breakout in
   `parts.md` (measured 1.570 mm `[STEP]`), inside Cherry's own plate tolerance
   of **1.524 ± 0.10 mm** `[DS cherry-mx]`, routed to **±0.2 mm**, drilled to a
   tighter tolerance than any laser `[SVC jlcpcb]`, and incapable of crazing.
   The visible face stays acrylic, so nothing about the look is given up. §5, §9.

4. **"Flush" cannot be specified; a one-sided step can.** Plate thickness
   ±0.31 mm against an FDM recess depth of ±0.5 mm `[SVC protolabs]` gives
   **±0.81 mm worst case** — a plate drawn flush lands anywhere from 0.8 proud
   to 0.8 sunk, and the sign reverses between builds. The fix is to stop
   designing for zero: **seat the plate 0.3 mm below the shell face on a
   shimmed seat, inside a drawn 0.4 mm reveal.** The reveal is the tolerance,
   made into a line. §6.

5. **One purchasing decision sets the plate's whole appearance.** Buy the
   **Alpha RD901F-xxB6/E6** with its **10 mm bushing** and the plate face is
   dead flat with three Ø7.4 holes in it. Buy anything with a 6.5 mm bushing
   and the face needs three Ø26–32 knob pockets, because 6.5 − 4.6 = 1.9 mm is
   less than the 2.4 mm the washer and nut need `[DERIVED §1]`. Flat face or
   three pockets. §2.2, §5.4.

6. **Do not bond the laminae. Screw them.** Six M2.5 button-head screws in
   Ø5.2 counterbores lasered into the face layer hold the stack together *and*
   hold it to the shell, sit 1.5 mm below the plate face, and leave the plate
   removable. A bonded plate is a single-use plate, which contradicts the whole
   brief. §2.5.

7. **The grid module is 1.27 mm, and `envelope.md`'s 4.7625 mm does not
   survive checking.** 4.7625 divides 76.200 and 19.050 but not 25.400,
   21.590, 16.510, 17.780, 20.320 or 22.860 — **12 of the 14 placement
   dimensions in `parts.md` §1 are not integer multiples of it.** 1.27 mm
   (0.050 in) divides **all fourteen**. The coarse layout module is **6.35 mm**
   (5 units, ¼ in) and the plate's tile is **25.40 mm** (4 modules, 1 in).
   Arithmetic in §10.

---

## 0.1 Source tags

| Tag | Meaning |
|---|---|
| `[RÖHM …]` | Röhm / POLYVANTIS (PLEXIGLAS, ACRYLITE) own technical publication, read directly. The material maker. Highest confidence on material behaviour. |
| `[DS …]` | Component manufacturer datasheet. |
| `[SVC …]` | A fabrication service's own published specification. Authoritative for *that service*, not for the process in general. |
| `[STEP]` | Measured from the vendor STEP file — `parts.md` §0.1. |
| `[DERIVED]` | Arithmetic from the above, shown in full. |
| `[3P]` | Third party / community practice. Weakest; used only where flagged as such. |

Documents used, with URLs, in §8.4.

---

## 1. What each part demands of the plate

The plate serves two incompatible mounting logics at once, which is the whole
problem. **Panel-mount parts** (the three pots) are *carried by* the plate, so
the plate's thickness is inside their bushing budget. **Breakout parts** (the
76.200 × 21.590 strips and the 25.400 mm sensor boards) stand *behind* the
plate on bosses, so the plate only has to let their actuators through.

```
usable bushing length = panel thickness + washer + nut (+ a thread to bite)
washer 0.4  +  nut 2.0  =  2.40 mm            [DS bourns-PTV09 p3]
```

| bushing | max panel |
|---|---|
| 5.0 mm | **2.60** |
| 5.0 mm — PEC11 encoder with a 15 mm shaft (**corrected**, was 6.5) | **2.60** |
| 6.5 mm — stock 16 mm Alpha, Adafruit 562 (`[STEP 562]` measured 6.6 directly) | **4.10** |
| 7.0 mm — RD901F `B3`/`E3` | **4.60** |
| 8.0 mm — RV16AF length code `4` | **5.60** |
| **10.0 mm — RD901F `B6`/`E6`** | **7.60** |

`[DERIVED from DS RD901F, DS PEC11, DS bourns-PTV09; the 2.0/0.4 nut and
washer are the Bourns PTV09 figures used as a class proxy — the PEC11 and
RD901F datasheets say hardware is supplied but do not dimension it, §11.1]`

Now the full demand list:

| feature | required local thickness | aperture | source |
|---|---|---|---|
| pot bushing, stock 6.5 mm | ≤ 4.10 mm | Ø7.4 finished | `[DERIVED]` |
| pot bushing, RD901F 10 mm | ≤ 7.60 mm | Ø7.4 (Ø9.4 for the E variants) | `[DERIVED]` |
| encoder bushing (PEC11, **5.0 mm**) **if nutted** | ≤ **2.60 mm** | Ø7.4 | `[DERIVED]` |
| **MX switch, clips engaged** | **1.524 ± 0.10 mm** | **14.00 ± 0.05 sq**, corner R ≤ 0.3 | `[DS cherry-mx p3]` |
| NeoSlider shank | any — pot body sits behind the plate | slot ≥ 2.5 wide × 75 long | `[STEP 5295]`, `parts.md` §3 |
| 5880 NeoPixel light hole | any | Ø2.5–3.0, **10.41 mm off the knob axis** | `[STEP 4991/5880]` |
| plate as a structure, ~254 mm wide | ≥ 3 mm | — | `[DERIVED]` |

**The binding pair is 1.5 mm at the MX clips against ≥3 mm for stiffness, with
a 4.10 mm ceiling at the bushings.** No single sheet satisfies it.
`parts.md` §4 already reached half of this conclusion for a printed plate — "if
the key band needs to be recessed, make the 1.5 mm plate a separate inset and
recess around it, exactly the counterbore logic that applies to the pots in §5."
This document is that inset, generalised.

---

## 2. The stack — the central deliverable

### 2.1 Why one sheet fails, in one block

```
single 4.5 mm sheet:
  MX clips          need 1.52 ± 0.10      get 4.50    FAIL — switch cannot latch
  PEC11 bushing     6.5 - 4.5 = 2.00      need 2.40   FAIL — nut will not fit
  stiffness         ok
single 1.5 mm sheet:
  MX clips          ok
  PEC11 bushing     6.5 - 1.5 = 5.00      need 2.40   ok
  stiffness         254 mm of 1.5 mm acrylic          FAIL — it is a membrane
single 3.0 mm sheet:
  MX clips          FAIL
  PEC11 bushing     6.5 - 3.0 = 3.50                  ok
  stiffness         ok
```

Two of the three requirements can be met by one sheet. Never all three.
`[DERIVED]`

### 2.2 The stack, layer by layer

**Two laminae. Total 4.60 mm ± 0.31 mm.** Numbered from the face, because that
is the datum that matters.

#### Layer 1 — the face. 3.0 mm extruded satin acrylic.

| | |
|---|---|
| material | PLEXIGLAS **Satinice 0M033 SC** (satin one side) or **WM513 SC** white-translucent, extruded, 3 mm `[RÖHM satinice-eu]`. In the Americas: ACRYLITE **0M001 SC** colourless, **WM530 SC** white, or **9M807 S5** black, all stocked at .118″ (3 mm) `[RÖHM satinice-us]`. |
| thickness tolerance | **±5 %** = ±0.15 mm `[RÖHM tol]` |
| satin side | **outward.** SC = one side satin, DC = both, DF = both plus diffuser beads through the body `[RÖHM satinice-eu]`. SC is enough and the gloss back face bonds and clamps better. |
| carries | the outer profile · the **bay troughs** (§2.3) · the three pot holes *or* three knob pockets · the Ø5.2 screw counterbores · the slider channels · any light apertures · the 0.4 mm reveal edge |
| does **not** carry | anything with a tolerance tighter than ±0.2 mm, and nothing a nut bears on |

#### Layer 2 — the function layer. 1.6 mm FR4.

| | |
|---|---|
| material | 1.6 mm FR4, black solder mask, no copper needed (or copper as a ground plane if you want it). Ordered as a PCB with a routed outline. |
| thickness tolerance | **±10 %** = ±0.16 mm for boards ≥1.0 mm `[SVC jlcpcb]` |
| outline tolerance | **±0.2 mm** routed `[SVC jlcpcb]` — *better than any acrylic laser*, and the drilled holes are tighter still |
| carries | the **Ø7.4 bushing holes** · the **14.00 mm MX cutouts** · the 2.5 mm slider slots · the M2.5 fixing holes and slots · the Pattern A / Pattern B hole quads · the NeoPixel light holes |
| why not acrylic | §5. A nut bears here, and 1.6 mm FR4 is also inside Cherry's plate spec of 1.524 ± 0.10 mm `[DS cherry-mx p3]`, which 1.5 mm acrylic only just is and which the laser cannot hold anyway (§4.6). |

**Two numbers make this layer almost suspiciously correct:** every Adafruit
breakout in `parts.md` §1 measures **1.570 mm** thick `[STEP, every board]` —
i.e. nominal 1.6 mm FR4. The function layer is therefore *the same material and
gauge as the parts it carries*, and the entire plate stack is one sheet of
acrylic plus one more of the boards' own substrate.

#### Totals

```
3.0 mm XT acrylic      ±0.15
1.6 mm FR4             ±0.16
───────────────────────────────
4.60 mm                ±0.31        [DERIVED]

bushing budget check, plain through-hole:
  RD901F 10 mm bushing   10.0 - 4.60 = 5.40 >= 2.40   OK, 3.0 mm spare
  PEC11    6.5 mm         6.5 - 4.60 = 1.90 <  2.40   FAILS
bushing budget check, face layer relieved (nut lands on layer 2):
  PEC11    6.5 mm         6.5 - 1.60 = 4.90 >= 2.40   OK, 2.5 mm spare
```

So: **the pots get plain through-holes if you buy the 10 mm bushing, and the
encoders are only ever nutted inside a relief.** That relief is the trough.

#### The all-acrylic variant, for the record

If the function layer must be acrylic after all: **3.0 mm + 1.5 mm XT**, total
**4.50 ± 0.30 mm**, every arithmetic line above unchanged to within 0.1 mm. It
works. It just puts a nut on a laser-cut hole in 1.5 mm of a brittle,
stress-crazing material (§5), and it cannot hold the MX cutout any better.
Use it for the first prototype, where the plate is a £15 experiment, and move
the function layer to FR4 for the one you play.

### 2.3 The trough — one feature that makes the plate bay-agnostic

The relief that the encoder nuts need is the same relief the recessed keys need
is the same channel the sliders need. Make it one shape and repeat it.

```
       ┌──────────────── 25.40 ────────────────┐   = 4 modules
   ────┤                                       ├────  layer 1, 3.0 mm
       │          76.20 long  (12 modules)     │
   ════╧═══════════════════════════════════════╧════  layer 2, 1.6 mm
```

**Pattern B bay trough: 76.20 × 25.40 mm, 3.0 mm deep (= layer 1 removed).**
**Pattern A bay trough: 25.40 × 25.40 mm.**

Both are whole numbers of the 6.35 mm module in *both* axes (§10) — 12 × 4 and
4 × 4 — which the PCB outline itself is not (21.590 mm = 17 units, a prime
multiple). The trough is 3.81 mm taller than the board it serves, centred,
giving 1.905 mm of air each side.

What this buys:

- **Layer 1 never changes.** It is bay-generic: N identical troughs on the
  tile grid. Layer 2 is the only lamina that knows whether a bay holds a
  slider, a key strip or four encoders. **Reconfiguring the instrument is
  re-cutting a 1.6 mm PCB panel**, which is the cheapest part of the whole
  object. That is `envelope.md`'s "reconfiguration is implied by the geometry"
  made literal.
- **It kills the key-band web problem.** Recessing MX keys individually in
  layer 1 needs a ≥16.0 mm square relief per key at 19.05 mm pitch, leaving
  **3.05 mm of acrylic between wells** — at or below the 1 × thickness minimum
  feature guidance `[3P fab-guides]` and the weakest line on the plate.
  One trough has no webs at all. `[DERIVED]`
- **It is the MIL-legal shape coding, for free.** MIL-STD-1472H 5.1.1.6 requires
  blind-operated controls to be shape coded or separated by 125 mm, which this
  panel cannot do (`precedents.md` §6.7). *Trough versus flat face* is a shape
  code the hand reads instantly, it costs no engraving, and it survives
  remapping — which is exactly what `precedents.md` §8.2 argued for and what
  the brief's "agnostic" requires.
- **It is the knob protection `precedents.md` §7.2 asked for** — "the land can
  be *raised* instead of the knob *sunk*" — with the shell's perimeter and the
  flat pot field as the high lands.

Actuator heights that follow, with the standoff set to **6.50 mm** so each
part's own shoulder lands on layer 2's back face (see §2.6):

| part | above the trough floor | above the plate face |
|---|---|---|
| PEC11 shaft tip (5752) | 21.40 − 6.50 − 1.60 = **13.30** | **10.30** |
| NeoSlider shank (5295) | 15.50 − 6.50 − 1.60 = **7.40** | **4.40** |
| MX stem (4980) | **NOT SOURCED — see §11.14** | — |

`[DERIVED from STEP 5752/5295]`. The MX row cannot be filled: an MX switch sets
its own plate-to-PCB distance through its clips, and `parts.md` §4 gives the
housing height above the PCB (11.6 to the stem tip, 3.6 of which is stem above
the housing) but **not the height at which the clips engage the plate.** That
number, not the 6.50 mm convenience, fixes the NeoKey bay's standoff — so the
key bay will almost certainly need **a different boss height from the encoder
and slider bays**, which is a shell question and a real one. §11.14.

### 2.4 Rejected alternatives, with the reason

| scheme | why not |
|---|---|
| **single 4.5 mm sheet** | §2.1. Keeps only as a *prototype shortcut*: it is a real stocked nominal (.177″ `[SVC sendcutsend]`) so you can cut the whole plate from one sheet to check the layout, losing the MX clips and the encoder nuts. |
| **three laminae, 1.5 + 1.5 + 1.5** | ±0.45 mm stacked tolerance, two more interfaces, three more alignment errors, and nothing needs a third feature level. |
| **3M VHB tape between the laminae** | VHB 5952 is 1.1 mm at **±10 %**, 4926 is 0.4 mm at **±15 %** `[DS 3m-vhb]`. Tape adds thickness *and* adds the worst tolerance in the stack to a budget that already has only 1.9 mm of slack at the PEC11 bushing. Also irreversible. No. |
| **bonding with solvent/reaction cement** | Works, and §2.5 names the right products — but it makes the plate single-use, and that is the opposite of the brief. |
| **machined (CNC) acrylic plate, one piece** | Solves everything geometrically and is the right answer if you are willing to pay for milling — but it is no longer laser-cut, and the user chose laser cut. Noted as the escape hatch, not recommended. |

### 2.5 How the laminae are held together: screws, not glue

**Six M2.5 button-head screws** pass from the **front**, through a Ø5.2
counterbore in layer 1, through a Ø3.2 hole in layer 2, into bosses in the
printed shell. They clamp the two laminae to each other *and* the stack to the
shell, in one operation, reversibly.

```
ISO 7380 button head M2.5:   head Ø4.5 max, height 1.5 max      [3P fastener-tables]
counterbore in layer 1:      Ø5.2 finished (draw Ø5.0)          -> 0.35 mm/side clearance
head top below plate face:   3.0 - 1.5 = 1.5 mm                 -> invisible from any normal angle
```

`[DERIVED]`. An ISO 4762 socket cap (Ø4.5 × 2.5) also disappears, with 0.5 mm
to spare — use it if you prefer a hex key.

**The counterbore is the entire argument for lamination, visible in one
feature:** a Ø5.2 hole in the upper lamina over a Ø3.2 hole in the lower one
*is* a counterbore, and a laser made it because it is just two different-sized
holes in two different sheets. Where screws fall inside a trough, layer 1 is
already gone and no counterbore is needed at all — so **put as many screws as
possible inside trough floors** and the face carries none.

**If you bond anyway**, these are the right products, from the material maker:

| | |
|---|---|
| **ACRIFIX 1R 9019** | 1-component, UV-curing, highly fluid, capillary. Application listed as "**area bonding for complex, colorless geometries**"; typical application listed as "**bonds of laser-cut elements**" `[RÖHM bonding]`. This is literally the product for this job. XT clear only. |
| **ACRIFIX 1R 0192** | 1-component, light-curing, viscous, gap-filling, GS **and** XT. Cures under ordinary fluorescent light, no special UV kit `[DS acrifix-1r0192]`. The practical choice. |
| **ACRIFIX 2R 0195** | 2-component, thixotropic, "**for satin GS and XT surfaces**", bond line is "bubble-free, translucent white, matte surface" `[RÖHM bonding]`. The matched adhesive for a Satinice plate — specified by Röhm for exactly this material. |

And the two rules that go with them, both from Röhm:
- **Anneal before and after bonding at approx. 70 °C** `[RÖHM fab]`; severely
  stressed bonds, 2–4 h at 70–80 °C immediately after curing
  `[DS acrifix-1r0192]`.
- **"Adhesive and adhesive fumes must be prevented from entering the cavities,
  since this may lead to stress cracking"** `[RÖHM fab]`. This plate is nothing
  but cavities — every bushing hole, every MX cutout. A capillary adhesive and
  a plate full of holes is a bad combination, which is a further argument for
  screws.

### 2.6 Set the encoder and slider standoff to 6.50 mm, and nut what you can

Both the 5752's PEC11 encoders and the 5295's slide pot have their shoulder
**6.50 mm above the PCB front face** `[STEP]`. Set those two bays' standoff to
exactly **6.50 mm** and those shoulders land flat on layer 2's back face. The
encoder's supplied nut then has 4.90 mm of bushing to grip and **finger load
goes into the plate instead of into solder joints** — which matters, because
`precedents.md` §7.5 collects the reviews of instruments whose encoders went
soft.

**One assembly warning.** At 19.05 mm pitch, four nuts of 11.0 mm across flats
(12.70 across corners, if the PEC11 hardware matches the PTV09 proxy) leave
**19.05 − 12.70 = 6.35 mm of air between adjacent nuts** `[DERIVED]`. A socket
or a nut driver will not fit once the second nut is on. A **thin open-ended
spanner, tightened in sequence**, or no nuts at all. Confirm the actual PEC11
nut dimensions before committing (§11.1).

---

## 3. Sheet thickness and tolerance, as actually sold

### 3.1 The tolerance table that decides the material

From Röhm's own technical information sheet "Tolerance of PLEXIGLAS sheets",
sourced to the PLEXIGLAS Sales Handbook 02/2004 `[RÖHM tol]`:

**PLEXIGLAS GS — cast. Thickness tolerance = ± (0.4 + 0.1 × thickness) mm.**

| nominal t (mm) | 2.0 | 3.0 | 4.0 | 5.0 | 6.0 | 8.0 | 10.0 | 12.0 | 15.0 |
|---|---|---|---|---|---|---|---|---|---|
| tolerance ± (mm) | 0.6 | 0.7 | 0.8 | 0.9 | 1.0 | 1.2 | 1.4 | 1.6 | 1.9 |
| as % of nominal | 30 % | **23 %** | 20 % | 18 % | 17 % | 15 % | 14 % | 13 % | 13 % |

**PLEXIGLAS XT — extruded.** < 3 mm: **±10 %**. ≥ 3 mm: **±5 %**.

| nominal t (mm) | 1.5 | 2.0 | 3.0 | 4.0 | 5.0 | 6.0 | 8.0 |
|---|---|---|---|---|---|---|---|
| tolerance ± (mm) | 0.15 | 0.20 | **0.15** | 0.20 | 0.25 | 0.30 | 0.40 |

Same document, same sheets: **cut-to-size length and width, ≤ 500 mm: ±0.5 mm**
(both grades). **Flatness: ≤ 3 mm deviation per 1000 mm of chord** (both
grades) — on a 254 mm plate that is up to 0.76 mm of bow, which the perimeter
screws flatten but which you should know about before you blame the printer.
**Squareness: ±1 mm per 1000 mm** (GS).

A US distributor's generic table for the same materials is looser still, and is
worth quoting because it is what you may actually receive if you buy from
stock rather than to a Röhm order code `[3P eplastics]`:

| nominal | cast, min–max | extruded, min–max |
|---|---|---|
| 0.118″ (3.0 mm) | 0.078–0.148″ = **1.98–3.76 mm** | 0.106–0.130″ = **2.69–3.30 mm** |
| 0.236″ (6.0 mm) | 0.186–0.266″ = 4.72–6.76 mm | 0.224–0.248″ = 5.69–6.30 mm |

**A "3 mm" cast sheet that arrives at 2.0 mm is inside spec.** That is the
sentence to remember.

### 3.2 Standard thicknesses actually sold

| route | stocked thicknesses |
|---|---|
| PLEXIGLAS **GS cast**, Röhm | **2 – 160 mm** solid sheet/block; standard sheet 3050 × 2030 mm up to 25 mm `[RÖHM xt-gs-ds]` |
| PLEXIGLAS **XT extruded**, Röhm | **1.5 – 25 mm**; standard sheet 3050 × 2050 mm `[RÖHM xt-gs-ds]` |
| PLEXIGLAS **Satinice** cast | 3 – 20 mm, 3050 × 2030 mm `[RÖHM satinice-eu]` |
| PLEXIGLAS **Satinice** extruded | **3 – 8 mm**, 3050 × 2050 mm `[RÖHM satinice-eu]` |
| ACRYLITE Satinice extruded, US | **.118″ (3), .177″ (4.5), .236″ (6), .354″ (9), .472″ (12)**, 48″ × 96″ `[RÖHM satinice-us]` |
| SendCutSend (cast only) | **.063″ (1.60), .118″ (3.00), .177″ (4.50), .214″ (5.40), .375″ (9.53), .500″ (12.7)**; colours in .118″ only `[SVC sendcutsend]` |
| Ponoko | 1.5, 2.0, 3.0, 4.5, 5.6, 6.0, 9.0 mm depending on colour and region `[3P ponoko-cat]` — the current catalogue is the authority, §11.7 |

Imperial nominals you will meet, converted: .060″ = 1.52 · .080″ = 2.03 ·
.098″ = 2.49 · **.118″ = 3.00** · .150″ = 3.81 · **.177″ = 4.50** ·
**.187″ = 4.75** · .220″ = 5.59 · **.236″ = 6.00** · .250″ = 6.35.

Two of those are worth noticing. **.187″ is a stocked cast thickness**
`[3P eplastics]`, and 3/16 in = **4.7625 mm** is `envelope.md`'s proposed module
— a coincidence, and a reminder that the imperial ladder and the vendor grid
come from the same place. And **.177″ = 4.50 mm equals the all-acrylic stack
total**, so the two-lamina plate and a single sheet are the same thickness,
which is what makes the single-sheet prototype in §2.4 worth cutting.

### 3.3 Consequence

```
3.0 XT  + 1.5 XT   stack:  4.50  +/- 0.30     [DERIVED]
3.0 XT  + 1.6 FR4  stack:  4.60  +/- 0.31     [DERIVED]
3.0 GS  + 1.5 GS   stack:  4.50  +/- 1.25     [DERIVED]  <-- 28 % of nominal
single 4.5 GS sheet:       4.50  +/- 0.85     [DERIVED]
```

A ±1.25 mm plate cannot be flush with anything, and it eats the PEC11's entire
1.9 mm of bushing slack on its own. **Extruded, or nothing.**

The one caveat, stated so it is not a surprise: Röhm's own list of causes of
tensile stress includes "**internal stress in PLEXIGLAS XT, especially tubes,
due to manufacturing technique**" `[RÖHM mach §8]`. Extruded sheet carries more
frozen-in stress than cast and is therefore *more* prone to solvent crazing.
With a screwed stack (§2.5) there is no solvent; with a bonded one, anneal.

---

## 4. Laser kerf and achievable tolerance — numbers for the CAD file

### 4.1 Kerf

| source | material | kerf |
|---|---|---|
| Ponoko `[SVC ponoko-kerf]` | 3 mm clear acrylic | **0.2 mm**, and "kerf is centered on cutting lines" |
| Ponoko, general | any | "starts at about 0.2 mm", widens with thickness, hardness and curvature |
| Sculpteo `[3P sculpteo]` | acrylic 3 mm / 5 mm | 0.105 mm / 0.17 mm |

Spread of roughly 2×, which is what you expect: kerf is a property of the
machine, the lens and the feed, not of the material. **Design to 0.2 mm and cut
a coupon.** Centred kerf means:

```
parts come out SMALLER by one kerf:  draw 40.2 to get 40.0
holes come out LARGER  by one kerf:  draw 39.8 to get 40.0     [SVC ponoko-kerf]
```

### 4.2 Machine tolerance, on top of kerf

| source | figure |
|---|---|
| Ponoko, CO₂ | **±0.005″ = ±0.13 mm** `[SVC ponoko-kerf]` |
| SendCutSend, acrylic | **±.009″ = ±0.23 mm** `[SVC sendcutsend]` |
| SendCutSend, slip-fit allowance | .006–.010″ = **0.15–0.25 mm** `[SVC sendcutsend]` |

So a hole drawn at Ø7.20 is nominally 7.20 + 0.20 = **7.40 finished**, and lands
somewhere in **Ø7.27 – Ø7.53** at Ponoko's ±0.13 mm or **Ø7.17 – Ø7.63** at
SendCutSend's ±0.23 mm. `[DERIVED]` The pessimistic figure is the one quoted in
§4.5, because it is the one that has to still clear a Ø7.0 thread.

### 4.3 Edge taper

The beam has a focal point, so the kerf is wider above and below it and the cut
face is not square. The commonly quoted figure is **1–2°**, "more noticeable
with thicker materials (¼″ and thicker)" and "almost unnoticeable in ⅛″"
`[3P taper]`. At 1.5° through 3.0 mm that is **0.08 mm of width change across
the thickness** `[DERIVED]` — negligible for a Ø7.4 clearance hole, not
negligible for a 14.00 ± 0.05 MX cutout.

I could not retrieve a primary measurement of acrylic kerf taper; the 1–2°
figure is community practice and is in §11.4. The direction of the taper
depends on focus and can go either way `[3P taper]`, so the only safe
instruction is procedural: **mark the reference face on the drawing, cut one
coupon, note which way it leans, and keep every subsequent part the same way
up.** Put the plate's *visible* face up, so the micro-chamfer falls on the edge
you look at.

### 4.4 Minimum features

No service publishes acrylic-specific numbers, and the guidance spread is wide
`[3P fab-guides]`:

| | guidance | for 3.0 mm layer 1 | for 1.6 mm layer 2 |
|---|---|---|---|
| minimum hole diameter | 0.5 – 1.5 × thickness | **1.5 – 4.5 mm** | 0.8 – 2.4 mm |
| minimum web / gap between cuts | ≥ 1 × thickness | **≥ 3.0 mm** | ≥ 1.6 mm |
| minimum feature width | ≥ 1 × thickness | **≥ 3.0 mm** | ≥ 1.6 mm |
| internal corners | fillet 0.02–0.05″ = 0.5–1.3 mm `[SVC sendcutsend]` | — | — |
| hole centre to sheet edge | **≥ 1.5 × hole diameter** `[RÖHM acrylite-kb]` | see §5.3 | — |

Röhm's machining guide adds, for the same reason: "**When cutting recesses,
holes should be predrilled at the corners to avoid notch effects and thus
possible breakage of the workpiece**", and "in order to rule out notch effects
altogether, the drill holes should be slightly chamfered or countersunk"
`[RÖHM mach §2.4, §3.1]`. For a laser the equivalent is: **fillet every internal
corner. A sharp internal corner in acrylic is a designed-in crack.**

**Checks against the plate:** the Ø2.5 NeoPixel light hole is below the 3.0 mm
minimum for layer 1 — put light holes in layer 2 and open layer 1 generously
above them. The merged key trough removes the 3.05 mm web (§2.3). The narrowest
remaining web is between two Ø5.2 screw counterbores, which the tile grid keeps
25.40 mm apart. `[DERIVED]`

### 4.5 The CAD table

Finished sizes derived in §1 and §2.5; drawn sizes are finished − 0.2 mm kerf.

| feature | **draw** | **expect finished** | why that finished size |
|---|---|---|---|
| pot / encoder bushing hole, M7 × 0.75 | **Ø 7.20** | 7.40 (7.17–7.63) | thread major Ø7.0; 7.17 still clears, 7.63 still hides under the Ø15 washer `[DS bourns-PTV09 p3]` |
| M9 × 0.75 bushing hole (RD901F `E` variants) | **Ø 9.20** | 9.40 | ditto, washer ID 9.1 |
| M2.5 screw, round (the datum hole) | **Ø 3.00** | 3.20 | free fit plus laser scatter |
| M2.5 screw, slot | **3.00 × 6.00** | 3.20 × 6.20 | ± 1.5 mm of float — §6.5 |
| M2.5 head counterbore, layer 1 only | **Ø 5.00** | 5.20 | ISO 7380 head Ø4.5 max |
| MX switch cutout, layer 2 only | **13.80 sq**, R0.3 | 14.00 (13.77–14.23) | Cherry 14.00 ± 0.05 — **not achievable, see §4.6** |
| slider shank slot, layer 2 | **2.30 × 74.80** | 2.50 × 75.00 | shank lower section 1.8 thick `[STEP 5295]` |
| slider channel, layer 1 | **nubbin width + 0.8** | nubbin + 1.0 | `precedents.md` §7.4; nubbin 11 × 7.1 `[AF 5621]` |
| NeoPixel light hole, layer 2 | **Ø 2.30** | 2.50 | emitter aperture Ø2.40 `[STEP 4991]` |
| bay trough, layer 1 | **76.40 × 25.60** | 76.20 × 25.40 | an internal cut-out is a hole: add kerf |
| plate outline | **nominal + 0.20** | nominal | a part is cut smaller by one kerf |

### 4.6 Where the laser cannot hold the tolerance — say it out loud

**Cherry's plate cutout is 14.00 ± 0.05 mm** `[DS cherry-mx p3]`. The best
published laser tolerance here is ±0.13 mm `[SVC ponoko-kerf]` — **2.6× the
whole allowed band**, before kerf variation and before taper. A laser cannot
make a Cherry-spec switch plate, and neither can anyone's laser.

In practice keyboard plates are laser-cut constantly and the switches hold,
because the clips tolerate 13.9–14.2. But it is a tolerance you are choosing to
miss, so miss it deliberately:

1. Cut a coupon in the real material at **13.70 / 13.80 / 13.90**, press a real
   switch into each, and pick the one that clicks in and does not rattle.
2. Put the MX cutouts in the **FR4** layer, where the published outline
   tolerance is ±0.2 mm `[SVC jlcpcb]` and the holes are drilled rather than
   burned.

This is the strongest single argument for §9's hybrid stack, and it is
independent of the crazing argument.

---

## 5. Mounting panel-mount pots in acrylic

### 5.1 What the material maker says, verbatim

Röhm's *Guidelines for Workshop Practice — Machining PLEXIGLAS*, §8 Annealing,
lists the causes of the tensile stress that leads to crazing `[RÖHM mach §8]`:

> Tensile stress may be caused, for example, by:
> • machining operations like sawing, milling, turning and sanding
> • thermoforming, especially line bending · • irregular heating
> • shrinkage of adhesives
> • **deformation during fastening (clamping, drilling, screw union)**
> • shrinkage after localized overheating due to incorrectly ground tools or
>   polishing · • impeded thermal expansion
> • internal stress in PLEXIGLAS XT … • external load.

And the mechanism:

> Plastics withstand considerable tensile stress **as long as they are not
> exposed to corrosive media at the same time.** If corrosive media are also
> present — for example, solvents and thinners during bonding, printing or
> painting, **monomer vapors during laser cutting**, plasticizers from PVC
> insulation material, sealants, films and aggressive cleaning agents — crazing
> may be the result, **with the parts becoming unfit for use, even though the
> same media do not cause damage to stress-free parts.**

And, on the cut edge specifically:

> **Stress generated in the immediate vicinity of cut edges may have to be
> relieved by subsequent annealing.**

**A pot nut tightened onto a laser-cut hole in acrylic is a screw union on a
stressed, monomer-exposed edge.** That is both halves of Röhm's own failure
mode, in one joint, on the part the performer grabs hardest. This is not a
"brittle material, be careful" hand-wave; it is the manufacturer describing
this exact assembly.

### 5.2 Will the nut torque crack it?

The pot gives you a ceiling but not a floor:

| | |
|---|---|
| **Bushing mount strength, min 7 kgf·cm = 0.69 N·m** | `[DS RD901F §2.6]` — the *pot's* limit |
| Shaft end stop strength, min 4 kgf·cm | `[DS RD901F §2.5]` |
| Axis push/pull, 10 kgf for 10 s, no damage | `[DS RD901F §2.4]` |
| Rotational torque in use, 20–250 gf·cm | `[DS RD901F §2.2]` |

So the part survives 0.69 N·m. **Nothing published says what 1.5 mm of
laser-cut acrylic survives, and I will not invent a number** (§11.3). What can
be said with sources:

- The nut only has to resist the 20–250 gf·cm of operating torque — **two to
  thirty times less than the 7 kgf·cm the bushing will take** `[DERIVED]`. The
  joint does not need to be tight; it needs to be *not loose*. Tighten to the
  point where the pot stops turning and stop.
- **The anti-rotation feature, not the nut, should carry the torque.** Standard
  practice is that the pot's tang must either dig into the panel or sit in an
  indexing hole, and that nothing may come between the pot and the panel for it
  to work `[3P pot-mounting]`. On acrylic, **a tang digging in is a deliberate
  crack initiator — use the indexing hole.** Adafruit's **562** has no
  anti-rotation lug modelled at all and a plain round hole is sufficient
  `[STEP 562]`, which makes it the easy option; whether the RD901F has one, and
  where, is **NOT SOURCED** (§11.2).
- **A washer is necessary and not sufficient.** The supplied washer is ID 9.1,
  **OD 15.0 ± 1.0, 0.4 thick** `[DS bourns-PTV09 p3]` — it spreads the load over
  Ø15 instead of Ø11, which is the right direction. Röhm's own suggestion for
  preventing cracking around a drilled hole in acrylic is **a rubber grommet**
  `[RÖHM acrylite-kb]`, which here means: a thin fibre or nylon washer between
  nut and acrylic, on top of the steel one.

### 5.3 Minimum material around the hole

Röhm's rule, for point fastening `[RÖHM acrylite-kb]`:

> the distance from the hole center to the edge of the sheet should be at least
> **1.5 times the diameter of the drilled hole**

| hole | centre ≥ from any edge | land, hole wall to edge |
|---|---|---|
| Ø7.4 (M7 bushing) | **11.1 mm** | **7.4 mm** |
| Ø9.4 (M9 bushing) | **14.1 mm** | **9.4 mm** |

`[DERIVED]`. SendCutSend's looser rule — "keep holes at least 1.5× material
thickness from the edge", 4.5 mm for 3 mm stock `[SVC sendcutsend]` — is a
service's DFM minimum, not a material limit. **Use Röhm's.** It means a pot may
not sit within 11.1 mm of the plate's own edge, of a trough wall, or of a screw
counterbore. On the tile grid (§10) that is satisfied automatically: the
nearest grid line is 12.70 mm away.

Röhm's second rule, "**the bolt hole diameter should be at least 2 times the
diameter of the bolt**", to allow for thermal and moisture movement
`[RÖHM acrylite-kb]`, is written for architectural glazing and is absurd here
(a Ø14.8 hole for a Ø7.4 bushing). The principle it encodes is real, and §6.5
honours it with slots instead.

Also reserve, on the face side: **Ø16 of flat for the washer** and **11.5 mm of
wrench clearance** for the nut `[DS bourns-PTV09 p3]`, and behind the plate,
for a 16 mm pot, **Ø16 × 9.2 mm of body plus 16.6 mm of sideways terminal
reach** `[STEP 562]` — which `parts.md` §5.4 correctly identifies as the
dimension that actually collides with a neighbour.

### 5.4 Verdict

**Acrylic alone at the bushing holes is marginal, and the fix is free.**
Ranked:

1. **Put the bushing holes in the FR4 layer** (§2.2, §9). FR4 does not craze,
   is 7× stiffer, and the nut lands on 1.6 mm of epoxy-glass with a Ø15 washer.
   Costs nothing in appearance, because layer 2 is invisible. **Do this.**
2. **Buy the 10 mm bushing** (`RD901F-20B6-15F-B10K` or the `E6` Ø9 variant,
   already identified in `parts.md` §0.4). This lets the bushing pass the whole
   4.60 mm stack, so the nut bears on the *back* of layer 2 and the acrylic face
   layer is never loaded at all. **Also do this.**
3. Fibre washer under the nut, steel washer under that, tighten to "stops
   turning", use the indexing hole rather than a digging tang.
4. **If the bushing hole must be in acrylic: anneal the plate after cutting.**
   Röhm's schedule `[RÖHM mach §8]`:
   - GS **80 °C** (unformed parts up to 100 °C); XT **70–80 °C** (unformed up to
     85 °C)
   - time in hours = **thickness ÷ 3**, minimum **2 hours**
   - cooling time in hours = **thickness ÷ 4**, cooling rate **≤ 15 °C/h**,
     remove from the oven below **60 °C**
   - For a 3 mm plate: 2 h at 80 °C (the minimum governs), then ≥ 45 min of
     controlled cooling. `[DERIVED]` A domestic oven will not hold 80 °C ± a
     few degrees; a reflow oven or a toaster oven with a thermocouple will.

---

## 6. Flush fit into the printed shell

### 6.1 The tolerance stack

```
plate thickness T  = 4.60 +/- 0.31                      [DERIVED, 3.0 XT + 1.6 FR4]
FDM recess depth D, if outsourced:
   prototyping / desktop FDM:  +/-0.5 %, min +/-0.5 mm  [SVC protolabs]
   industrial FDM:             +/-0.25 %, min +/-0.25 mm
   -> on a 4.60 mm depth the lower limit governs:  +/-0.50 mm  /  +/-0.25 mm
   -> on the 254 mm plate opening:                 +/-1.27 mm  /  +/-0.64 mm

step at the face, plate drawn flush:
   worst case  +/- (0.31 + 0.50) = +/- 0.81 mm
   RSS         +/- 0.59 mm
```

`[DERIVED]`. **A plate drawn flush is somewhere between 0.8 mm proud and 0.8 mm
sunk, and which one changes between builds.** A step that reverses sign reads as
a defect every time. A step that is always in the same direction reads as a
line.

Two honest caveats about that ±0.5 mm. It is a *service's bounding-box*
tolerance, measured with calipers over the whole part `[SVC protolabs]`, and it
is the number you must design to if you send the shell out. On a calibrated
machine you own, a recess **depth** is set by layer count — at 0.2 mm layers it
quantises to 0.2 mm and the real error is first-layer squish and Z repeatability,
both much smaller. The term that bites on your own printer is the **in-plane**
one: XY shrink and expansion on a 254 mm opening.

### 6.2 What to design instead of flush

**Design a one-sided step, and measure the plate.**

```
seat the plate 0.30 mm BELOW the shell face, on a shimmed seat:
   the T term is removed by measuring the delivered plate with calipers
   and choosing shims; only D's error remains.
   residual step = 0.30 +/- (printer Z error)
   on your own printer, +/-0.10 mm  ->  step 0.20 .. 0.40 mm   always sunk
   outsourced, +/-0.50 mm           ->  step -0.20 .. 0.80 mm  reverses: go to 0.60 mm nominal
```

`[DERIVED]`. Going *sunk* rather than *proud* is not arbitrary. `precedents.md`
§7.2 already asked for the surrounding land to be the high point so the deck can
be laid face-down without loading a shaft — "the knob's top sits ≥ 0.5 mm below
the plane of the highest surrounding land … and the land can be *raised* instead
of the knob *sunk*. Do this first." **The shell's perimeter is that land. The
0.3 mm step is not a compromise with the tolerance; it is the protection
feature, and the tolerance hides inside it.**

### 6.3 The scheme: floating plate, shimmed seat, drawn reveal

```
 shell face ──────┐                      ┌────── shell face
                  │  0.40 reveal         │
                  │ ┌──────────────────┐ │   <- 0.30 below the shell face
                  │ │ layer 1, 3.0 mm  │ │
                  │ ├──────────────────┤ │
                  │ │ layer 2, 1.6 mm  │ │
                  └─┤                  ├─┘
    recess wall     │   shim stack     │        <- 0.1 / 0.2 / 0.3 mm washers
    full height     ▓▓▓ boss ▓▓▓▓▓▓▓▓▓▓▓
```

| element | spec | reason |
|---|---|---|
| shell recess | plain rectangle, walls full height to the shell face, floor ≥ 3 mm below layer 2 | the plate is carried by bosses, not by the floor; a deep floor gives the breakout connectors room |
| bosses | 4–6, tops at `shell_face − T_measured − 0.30` | the one surface whose height you control, and you control it with shims |
| shims | M2.5 stainless shim washers, 0.1 / 0.2 / 0.3 / 0.5 mm | removes the ±0.31 mm sheet tolerance by measurement rather than by hope |
| reveal | **0.40 mm per side**, so the recess is `plate + 0.80` on each dimension | §6.4 |
| fasteners | 6 × M2.5 button head from the front, in Ø5.2 counterbores (§2.5) | clamps the laminae and the plate in one go; reversible |
| plate top edge | **0.3 mm chamfer or R0.3**, drawn | a laser leaves a square arris; a drawn chamfer makes the reveal read as intended |

**No adhesive anywhere.** The plate is a serviceable part: unscrew six screws
and the entire control surface, both laminae and all the hardware, lifts off the
shell.

### 6.4 The perimeter gap, and "don't shrink-wrap anything"

The brief's instruction from v1 — *"dont shrink wrap anything so agressively
itll be insane to work with once printed"* — has a number here:

```
plate outline tolerance    +/- 0.13 .. 0.23 mm        [SVC ponoko-kerf / sendcutsend]
recess opening, outsourced +/- 1.27 mm on 254 mm      [SVC protolabs]
recess opening, own printer, after XY calibration  +/- 0.2 .. 0.3 mm typical  [NOT SOURCED, 11.6]
```

A 0.25 mm per-side gap is inside the *laser's* error alone and will sometimes
not go in. **0.40 mm per side** (0.80 mm on each overall dimension) survives the
laser, survives a calibrated printer, is wide enough to assemble with bare
hands, and reads as a deliberate shadow line rather than a fit error. If the
shell is outsourced, the opening must be measured and the plate cut to it, not
the reverse — the plate is the part with the good tolerance, and it is the part
you can re-cut for £15.

**The reveal is the tolerance, drawn.** That one sentence is the answer to the
flush-fit question, and it is also, conveniently, the Scandinavian idiom: a
floating plane with a shadow gap, no filler, no fudging, the joint stated.

### 6.5 Screw holes: one datum, five slots

Movement to absorb, over a 228.60 mm screw span (12.70 mm in from each end of a
254.00 mm plate):

| term | magnitude | source |
|---|---|---|
| FDM boss-pitch error, ±0.5 % | **±1.14 mm** | `[SVC protolabs]`, `[DERIVED]` |
| FDM boss-pitch error, ±0.25 % | ±0.57 mm | ditto |
| laser hole position | ±0.13 … ±0.23 mm | `[SVC ponoko-kerf / sendcutsend]` |
| PMMA thermal, ΔT 30 K, α = 0.07 mm/m·K | 0.53 mm total over 254 mm | `[RÖHM xt-gs-ds]`, `[DERIVED]` |
| PMMA **differential** vs a PETG or PLA shell | **0.00 – 0.22 mm** | `[DERIVED]` — both materials move at ≈7 × 10⁻⁵/K, so they move together |
| Röhm's architectural heat + moisture allowance, 5 mm/m | 1.27 mm over 254 mm | `[RÖHM xt-gs-ds]` |

The dominant term is **not thermal — it is the printed boss pitch.** That is
worth knowing, because it changes the fix from "allow for expansion" to "allow
for the printer".

```
one round hole, Ø3.20 finished (draw Ø3.00), at the plate centre   = the datum
five slots,     3.20 x 6.20 finished (draw 3.00 x 6.00)            = +/-1.50 mm travel
slot long axis along the plate's long dimension
```

`[DERIVED]`. ±1.5 mm covers the ±1.14 mm worst-case boss error plus the laser
and the thermal terms together. Keep the slots in **layer 2** (FR4, drilled and
routed to ±0.2 mm) and plain Ø3.2 round holes in layer 1 under the counterbores
— layer 1's position is set by the reveal, not by the screws.

---

## 7. Finish and material

### 7.1 The named products, and what is orderable in small quantity

| want | product | sizes | source |
|---|---|---|---|
| **satin one side, extruded, EU** | PLEXIGLAS **Satinice 0M033 SC** (clear), **WM513 SC** (white-translucent) | 3050 × 2050, **3–8 mm** | `[RÖHM satinice-eu]` |
| satin both sides, extruded, EU | **0M033 DC**, **WM513 DC** | as above | `[RÖHM satinice-eu]` |
| satin + diffuser beads through the body | **0D010 DF** — meets food-contact requirements, so it is a well-stocked grade | 3 mm wall / 3–8 mm sheet | `[RÖHM satinice-eu]` |
| **matte black, extruded, US** | ACRYLITE Satinice **9M807 S5** (velvet, one side) or **9M807 D5** (two sides) | 48″ × 96″, **.118″ / .177″ / .236″ / .354″ / .472″** | `[RÖHM satinice-us]` |
| matte white, US | **WM539 S5** / **WM630 S5**; crystal-surface **WM530 SC** | as above | `[RÖHM satinice-us]` |
| satin cast, many colours, EU | **0F00 SC/DC**, **WH02 SC/DC**, **WH10 DC** + "many colors" | 3050 × 2030, 3–20 mm | `[RÖHM satinice-eu]` |
| opaque colour numbers you will see on quote forms | **2025** black · **2447** white · **7074** dark grey · **2124** light grey | .118″ only at SendCutSend | `[SVC sendcutsend]` |
| cut-to-order matte, no sheet to buy | Ponoko's catalogue carries **Black Matte**, **White Matte** and **Clear Matte** acrylic as cuttable stock | — | `[3P ponoko-cat]` |

**For this plate: an opaque black or dark-grey satin, extruded, satin side out,
3 mm.** `9M807 S5` in the Americas, `0M033 SC` dyed or a Satinice colour in
Europe. Gloss black on a hand-held performance object is a fingerprint
catalogue and a stage-light mirror; transparent is a wiring exhibition; white
satin is beautiful and shows every mark a venue can produce.

### 7.2 Why satin, in the manufacturer's words

> With its warm and soft appearance, the velvety surface of PLEXIGLAS® Satinice
> simply cries out to be touched. … **This surface makes the material
> insensitive to traces of wear and finger marks.** `[RÖHM satinice-eu]`

ACRYLITE says the same of the US line: "**Resistant to traces of wear and
finger marks**" `[RÖHM satinice-us]`. That is the maker's own claim for the one
property a handheld instrument needs most, and it is the whole reason to prefer
Satinice over sanding or vapour-frosting a gloss sheet yourself.

Two more properties that matter here, both listed for the **extruded** grades
specifically: "**low thickness tolerance**" and "**tough surface due to
diffuser beads**" `[RÖHM satinice-eu]`. The extruded satin grade is better on
exactly the axis §3 said was decisive.

### 7.3 Can a satin surface be laser-cut without marking it?

**Yes, commercially and routinely** — Ponoko sells Black Matte, White Matte and
Clear Matte acrylic as laser-cut materials `[3P ponoko-cat]`, and Röhm states
"PLEXIGLAS® Satinice can be fabricated just like standard PLEXIGLAS®"
`[RÖHM satinice-eu]`, with a dedicated matte-surface adhesive (**ACRIFIX 2R
0195**) supplied for the line `[RÖHM bonding]`.

But there is a real risk and it needs one specific instruction. Laser cutting
deposits soot and can scorch the surface near the cut; the standard mitigation
is **paper masking**, which "offers superior protection against scratches and
heat scorching, and reduces smoke residue" compared with plastic film, which
can curl, melt and leave residue `[3P masking]`. On a **gloss** surface the
soot wipes off. On a **satin** surface the soot sits in the texture — and §7.4
is about to forbid you from scrubbing it.

**So, on the order:** *extruded satin acrylic, paper masking, masking to remain
on the satin face through cutting and to be shipped on.* Then peel it at
assembly. Also specify fillets on internal corners (§4.4) and note the
reference face (§4.3).

### 7.4 Cleaning — a hard constraint, not a care note

This is a hand-held object that will be played with sweaty hands in a dark
room, so how it is cleaned is a design input. Röhm `[RÖHM mach §9]`:

> **Only clear water is needed** to clean and preserve PLEXIGLAS®. If the dirt
> pickup is more pronounced, the water should be **warm and contain a mild
> household detergent**. **Dry rubbing must be avoided at all cost.**

And, from the ACRYLITE sheet `[RÖHM satinice-us]`:

> Like other plastic materials, ACRYLITE® sheet is **subject to crazing,
> cracking or discoloration if brought into contact with incompatible
> materials.** These materials may include **cleaners, polishes, adhesives,
> sealants, gasketing or packaging materials, cutting emulsions**, etc.

**The instrument may not be wiped with isopropyl alcohol**, which is the first
thing any engineer reaches for and which crazes PMMA — and remember from §5.1
that the plate's laser-cut edges are *stressed*, which is precisely the
condition under which a corrosive medium turns into a crack. Warm water and a
drop of washing-up liquid, never dry, never solvent. If that is unacceptable,
the plate wants to be anodised aluminium (§9), which does not care.

### 7.5 Edge lighting and the NeoPixels

Three parts put light behind this plate: the 5880 emits through the PCB to the
front with a Ø2.40 aperture **10.41 mm off the knob axis** `[STEP 4991]`; the
5295's four NeoPixels light the slide pot's own slot `[STEP 5295]`; the 4980's
four sit 5.08 mm off the key centreline `[STEP 4980]`. With an **opaque** face
layer and an FR4 function layer, none of that light reaches the face except
through a deliberate aperture.

Three options, in increasing effort:
1. **Accept the vendor's intent.** The slider's light escapes up its own slot;
   the key strip's light escapes around the switch. Nothing to design.
2. **Drill for it.** Ø2.5 finished (draw Ø2.3) in layer 2, opened out above in
   layer 1 (§4.4 — Ø2.5 is below layer 1's minimum feature). On the grid,
   because an off-grid light hole will be the one thing that breaks the
   "insane symmetry".
3. **A diffusing insert.** A third, local lamina of a light-diffusing grade
   dropped into a trough. Transmission at 3 mm `[RÖHM satinice-us]`:
   **0D010 DF 86 %**, WD852 92 %, WD853 85 %, WD854 79 %, WD855 72 %,
   **WD008 DF white 60 %**. 0D010 DF at .118″ is the obvious candidate.

**Edge-lighting the plate itself is a trap** and should be ruled out now: it
needs a transparent body (giving up §7.1 and §7.2 and exposing the wiring), the
light leaks out of every one of the dozens of apertures, and the apertures are
the whole design. Noted and closed.

---

## 8. Fabrication sourcing

### 8.1 Who will cut one

| service | what it does | acrylic stock | file formats | tolerance | lead time | multi-layer / bonding? |
|---|---|---|---|---|---|---|
| **Ponoko** (US/NZ) | CO₂ laser, one-offs its whole business model | 1.5–9 mm, incl. **Black/White/Clear Matte**, mirror, two-colour engraving stock `[3P ponoko-cat]` | vector; the catalogue page does not state formats (§11.7) | kerf 0.2 mm, machine **±0.13 mm** `[SVC ponoko-kerf]` | same-day Bay Area before 11:00 PT, next-day mainland US `[3P ponoko-cat]` | **No.** Flat cutting only. |
| **SendCutSend** (US) | laser, **cast acrylic only**, ±.009″; also countersinking on "sheetmetal and plastic" `[SVC sendcutsend-cs]` | .063 / .118 / .177 / .214 / .375 / .500″; colours in .118″ only | not stated on the material page | **±0.23 mm** `[SVC sendcutsend]` | not stated | **No.** Its acrylic page lists laser cutting as the only add-on — which contradicts the countersinking page, §11.8. |
| **Schaeffer AG / Front Panel Express** (DE/US) | milled panels, the front-panel trade's default | **Perspex 2, 3, 4 mm** transparent, 3 mm red; aluminium 1.5–10 mm; **milled parts to 10 mm, inner and outer contours** `[SVC schaeffer]` | "all standard file formats"; free **Front Panel Designer** (FPD); DXF import `[SVC schaeffer]` | not published (§11.9) | not published | Mills, so it can make a **true counterbore in one piece** — the escape hatch from this whole document. **No production minimum.** |
| **JLCPCB / any PCB fab** | the FR4 function layer | 0.4 / 0.6 / 0.8 / 1.0 / 1.2 / **1.6** / 2.0 mm `[SVC jlcpcb]` | Gerber | outline **±0.2 mm**, thickness **±10 %** ≥1.0 mm `[SVC jlcpcb]` | days | n/a |
| **a local acrylic fabricator** | the only route that will *laminate* for you | whatever they stock | DXF/AI | theirs | days | **Yes** — display and shopfitting trade does area bonding daily. |

**The finding that matters: no mainstream online service will bond your
laminae.** Which is fine, because §2.5 says don't bond them — but if you decide
to, it is either your own bench with ACRIFIX or a local fabricator, not an
upload.

### 8.2 What it costs

Honestly: I could not source a current one-off price for a 254 × 51 mm acrylic
plate, and §11.10 says so. What I can anchor:

| | |
|---|---|
| Ponoko, one-off laser-cut acrylic | "**From $50 for just 1**" `[3P ponoko-cat]` |
| Schaeffer, 125 × 125 mm × 2 mm natural anodised aluminium, single piece | **€144.62** ex VAT `[3P schaeffer-gallery]` |
| Schaeffer, 444.5 × 158.75 mm × 3 mm anodised black + full-surface UV print, single piece | **€264.33** ex VAT `[3P schaeffer-gallery]` |
| raw material, for scale: 3 mm clear acrylic, 300 × 300 mm | ~£4–8 (2022 university price list) `[3P cambridge-prices]` |
| FR4 function layer, 254 × 51 mm, 1.6 mm, 5 off | order of **$10–30** `[NOT SOURCED, 11.10]` |

The shape of it: **the acrylic face layer is a £15–40 part and the FR4 function
layer is a £10–25 part.** Both are cheap enough that cutting three variants of
layer 2 and choosing by feel is the correct way to resolve every "measure it"
item in §11. Milled aluminium is roughly an order of magnitude more.

### 8.3 What to put on the order

1. Material by **Röhm order code** (`9M807 S5`, `0M033 SC`, …), not "3 mm matte
   black acrylic" — the code fixes cast-vs-extruded and therefore the tolerance.
2. **Extruded**, explicitly, and reject cast substitution (§3.1).
3. **Paper masking, satin face, left on** (§7.3).
4. Nominal thicknesses **drawn at finished size**; the drawing is kerf-
   compensated per §4.5, so tell them **not** to apply their own kerf offset.
5. The **reference face** marked (§4.3).
6. Fillets on all internal corners (§4.4).
7. One **test coupon** in the same material on the same sheet: MX cutouts at
   13.70 / 13.80 / 13.90, bushing holes at Ø7.0 / 7.2 / 7.4, a 3.0 mm web, and
   a 20 mm square for measuring kerf and taper. This coupon closes four of the
   §11 items for the price of 400 mm of cut length.

### 8.4 Documents used

- Röhm/POLYVANTIS, **Tolerance of PLEXIGLAS sheets** (Sales Handbook 02/2004) — `[RÖHM tol]` — https://www.zenitsk.sk/public/media/PlastySK/Tolerancia_dosiek_Plexiglas.pdf
- Röhm/POLYVANTIS, **Guidelines for Workshop Practice: Machining PLEXIGLAS** (No. 311-1) — `[RÖHM mach]` — https://www.zenit.cz/public/media/Plasty/Zenit_Montaz_Plasty_Plexiglas_Zpracovani_AJ.pdf
- Röhm/POLYVANTIS, **fabrication overview** (forming, bonding, sealing) — `[RÖHM fab]` — https://www.zenit.cz/public/media/Plasty/Zenit_Montaz_Plasty_Plexiglas_Zpracovani2_AJ.pdf
- Röhm/POLYVANTIS, **ACRIFIX adhesive selection guide** — `[RÖHM bonding]` — https://www.zenit.cz/public/media/Plasty/Zenit_Montaz_Plasty_Plexiglas_Lepeni_AJ.pdf
- Röhm/POLYVANTIS, **PLEXIGLAS GS / XT product datasheet** — `[RÖHM xt-gs-ds]` — https://www.voskunststoffen.nl/files/166151/datasheet%20plexiglas%20XT%20GS.pdf
- Evonik/Röhm, **PLEXIGLAS Satinice technical information**, July 2015 — `[RÖHM satinice-eu]` — https://igepa.ee/wp-content/uploads/2017/06/211-11-plexiglas-satinice-en.pdf
- POLYVANTIS, **ACRYLITE Satinice technical information** 3773G, 07/24 — `[RÖHM satinice-us]` — https://www.tapplastics.com/image/catalog/ACRYLITE-Satinice-Technical-Information.pdf
- ACRYLITE knowledge base, **hole edge distance and bolt clearance** — `[RÖHM acrylite-kb]` — https://www.acrylite.co/resources/knowledge-base/article/how-far-in-should-I-drill-a-hole-from-the-edge-of-acrylic-sheet
- Evonik, **ACRIFIX 1R 0192 datasheet** — `[DS acrifix-1r0192]` — https://www.plexiglas.nl/wp-content/uploads/2015/01/391-20-ACRIFIX-1R-0192-en.pdf
- Ponoko, **how much material does the laser burn away** — `[SVC ponoko-kerf]` — https://help.ponoko.com/en/articles/4442594-how-much-material-does-the-laser-burn-away
- Ponoko, **laser-cut acrylic catalogue** — `[3P ponoko-cat]` — https://www.ponoko.com/laser-cutting/acrylic
- SendCutSend, **acrylic** — `[SVC sendcutsend]` — https://sendcutsend.com/materials/acrylic/
- SendCutSend, **countersinking** — `[SVC sendcutsend-cs]` — https://sendcutsend.com/guidelines/countersinking/
- Protolabs Network (Hubs), **manufacturing standards** — `[SVC protolabs]` — https://www.hubs.com/manufacturing-standards/
- JLCPCB, **PCB capabilities** — `[SVC jlcpcb]` — https://jlcpcb.com/capabilities/pcb-capabilities
- Schaeffer AG / Front Panel Express — `[SVC schaeffer]` — https://www.schaeffer-ag.de/produkte?lang=en and https://frontpanelexpress.com/products ; price examples `[3P schaeffer-gallery]` https://www.schaeffer-ag.de/en/products/product-gallery
- Taiwan Alpha, **RD901F-(C) specification** — `[DS RD901F]` — https://www.taydaelectronics.com/datasheets/files/SP26020021_RD901F-40-15R1-B10K-0CD-1.pdf
- ePlastics, **acrylic thickness tolerance** — `[3P eplastics]` — https://www.eplastics.com/blog/thickness-tolerance-plexiglass-acrylic-sheet
- 3M, **VHB 5952 technical data** — `[DS 3m-vhb]` — https://multimedia.3m.com/mws/media/2369604O/3m-vhb-tape-5952-technical-data-sheet.pdf
- Sculpteo, **laser cutting materials design guidelines** — `[3P sculpteo]` — https://info.sculpteo.com/hubfs/Material%20comparisons/Laser%20cutting%20materials%20design%20guidelines.pdf
- Fastener tables — `[3P fastener-tables]` — https://westfieldfasteners.co.uk/Standards/ScrewBolt-SHBtn-M.html (ISO 7380), https://engineersedge.com/iso_socket_head_screw.htm (ISO 4762), https://monsterbolts.com/pages/m2-5-bolt-size
- Cut Laser Cut, **the laser cut edge** — `[3P taper]` — https://cutlasercut.com/drawing-resources/expert-tips/laser-cut-edge/ *(page body could not be retrieved from this environment; see §11.4)*
- Masking practice — `[3P masking]` — https://makerstock.com/blogs/makerstock-blog/acrylic-paper-or-plastic-masking
- Pot mounting practice — `[3P pot-mounting]` — https://forum.allaboutcircuits.com/threads/rings-for-potentiometer.143276/ *(community; used only where flagged)*
- Minimum-feature guidance, several makerspaces and services — `[3P fab-guides]` — see `[3P sculpteo]` above and https://sendcutsend.com/faq/what-are-best-practices-for-minimum-geometry/

---

## 9. Does every layer need to be acrylic? Honestly: no, and it costs nothing

The user chose acrylic **for the look**. Only layer 1 is visible. Layer 2 is a
hidden structural and dimensional part, and three materials can serve it:

| | **acrylic 1.5 mm** | **FR4 1.6 mm** | **aluminium 1.5 mm** |
|---|---|---|---|
| outline tolerance | ±0.13 … ±0.23 mm `[SVC]` | **±0.2 mm** routed, holes tighter `[SVC jlcpcb]` | ±0.13 mm class `[SVC]` |
| thickness tolerance | ±0.15 (XT) / ±0.55 (GS) `[RÖHM tol]` | ±0.16 (±10 %) `[SVC jlcpcb]` | gauge, tight |
| MX plate spec 1.524 ± 0.10 `[DS cherry-mx]` | 1.5 ✓ but laser can't hold the cutout | **1.6 ✓, and the cutout is routed** | 1.5 ✓ |
| crazes under a nut? | **yes — Röhm's own failure mode, §5.1** | no | no |
| stiffness, E | 3300 MPa `[RÖHM xt-gs-ds]` | ~24 GPa, ≈7× | 69 GPa, ≈21× |
| can be tapped / take threads | badly | yes (or press-fit inserts) | **yes, and services tap it** `[SVC sendcutsend-cs]` |
| countersinks available | laser: no | drilled: yes | **yes** `[SVC sendcutsend-cs]` |
| cost, this size | lowest | **low** | moderate |
| matches the parts it carries | no | **yes — every breakout is 1.570 mm FR4** `[STEP]` | no |
| visible | no | no | no |

**Recommendation: FR4.** It wins on every axis that layer 2 is for, it is the
same substrate and gauge as the nine boards it holds, and it is the cheapest of
the three to iterate — which matters because §11 has six items that only a
physical coupon can close.

**Aluminium** is the right answer if the plate ever needs real threads (a
threaded insert for a strap lug, a tripod socket, a stand mount), or if §7.4's
cleaning restriction turns out to be unacceptable in practice. Then reconsider
the whole plate: Schaeffer will mill a **true counterbore in one piece of
anodised aluminium 1.5–10 mm thick, with inner and outer contours, no production
minimum** `[SVC schaeffer]` — which makes the entire lamination problem vanish
for roughly an order of magnitude more money. That is the honest alternative and
it should be on the table, not buried.

**One note, flagged as outside this document's scope:** if layer 2 is a PCB,
it can carry the I²C chain between the nine breakouts as traces instead of as
nine STEMMA QT cables. That is an electronics decision, not an enclosure one,
and `parts.md` §8 owns the bus. It is recorded here only because the mechanical
part and the electrical part would be the same object, which is the kind of
thing worth noticing early.

---

## 10. The grid

### 10.1 The unit is 1.27 mm, and it is a fact rather than a choice

Every placement dimension in `parts.md` §1 is an exact integer multiple of
**1.27 mm = 0.050 in = 1/20 in**:

| dimension | units | | dimension | units |
|---|---|---|---|---|
| Pattern B length 76.200 | **60** | | Pattern A depth 17.780 | **14** |
| Pattern B width 21.590 | **17** | | Pattern A depth 20.320 | **16** |
| Pattern B hole X 38.100 | **30** | | Pattern A depth 22.860 | **18** |
| Pattern B hole Y 16.510 | **13** | | Pattern A hole X 20.320 | **16** |
| Pattern A width 25.400 | **20** | | Pattern A hole Y 12.700 | **10** |
| encoder / MX pitch 19.050 | **15** | | corner R, hole inset 2.540 | **2** |
| QT socket pitch 30.480 | **24** | | QT socket pitch 20.320 | **16** |

`[DERIVED, exact rational arithmetic over all 14 dimensions]`. The only number
in `parts.md` §1 that is *not* a multiple of 1.27 is the **1.570 mm board
thickness** — which is not a placement dimension at all, it is nominal 1.6 mm
FR4, and §2.2 makes use of it.

**And the unit resolves a 0.01 mm disagreement inside `parts.md` itself.** Its
master table measures the Pattern B hole pitch as **16.52** on the 5752 and
5295 and **16.51** on the 4980, and §1.1 calls the pattern 16.51. 13 × 1.27 =
**16.51 exactly**; 16.52 is 13.008 units. So **16.51 is the vendor's number and
the 16.52 is measurement noise in the third decimal** — which is a small thing
except that it is the kind of small thing a boss pattern is cut from. Use 16.51.
`[DERIVED]`

### 10.2 `envelope.md`'s 4.7625 mm does not survive the check

```
module 1.27    (0.050 in)   ALL 14 dimensions are integer multiples
module 2.54    (0.100 in)   fails on 21.590 (8.5), 16.510 (6.5), 19.050 (7.5)
module 4.7625  (3/16 in)    fails on 12 of 14:
                            21.590 = 4.533   25.400 = 5.333   16.510 = 3.467
                            17.780 = 3.733   20.320 = 4.267   22.860 = 4.800
                            12.700 = 2.667    2.540 = 0.533   30.480 = 6.400
module 6.35    (0.250 in)   fails on the cross-axis set: 21.590 (3.4), 16.510 (2.6),
                            17.780 (2.8), 20.320 (3.2), 22.860 (3.6)
module 12.70   (0.500 in)   fails on 11 of 14
module 25.40   (1.000 in)   fails on 13 of 14
```

`[DERIVED]`. `envelope.md` derived 4.7625 from one dimension — 19.05 / 4 — and
it is correct for that dimension and for the 0.75-inch family around it. It is
not a module for the surface, because **it does not divide Pattern A's 25.400 mm
width**, and Pattern A carries the ADC and every sensor.

**And nothing coarser than 1.27 mm can ever serve both hole quads**, which is
the part worth keeping:

```
Pattern B hole Y   16.510 / 1.27 = 13      13 and 17 are distinct primes,
Pattern B width    21.590 / 1.27 = 17      so gcd(13, 17) = 1 and the only
                                           common divisor is the unit itself.
```

`[DERIVED]`. That is a proof, not an observation. **The cross axis is laid out
in units; there is no alternative.** Which is fine, because the cross-axis hole
quads are *behind* the plate — they are screwed into, not looked at.

### 10.3 So: unit, module, tile

| | value | = | divides |
|---|---|---|---|
| **unit** | **1.27 mm** | 0.050 in | everything. The grain of the vendor's whole catalogue. |
| **module** | **6.35 mm** | 0.250 in, 5 units | 76.200 (12) · 25.400 (4) · **19.050 (3)** · 38.100 (6) — i.e. every long-axis dimension that the eye and the hand actually measure |
| **tile** | **25.40 mm** | 1 in, 4 modules, 20 units | the Pattern A board width; a Pattern B bay is exactly **3 tiles** |

**The rule is: the grid governs what you see; the unit governs what you screw
into.** Apertures, trough edges, plate edges, pot centres, rules and reveals sit
on the 6.35 mm module. Mounting-hole quads sit on the 1.27 mm unit, behind the
plate, where no one is looking for alignment.

Worked plate, for `layout.py` to check rather than to trust:

```
plate width   10 tiles = 40 modules = 200 units = 254.00 mm  = 10.000 in
plate height   2 tiles =  8 modules =  40 units =  50.80 mm  =  2.000 in
bay trough    12 x 4 modules                    =  76.20 x 25.40
sensor trough  4 x 4 modules                    =  25.40 x 25.40
reveal                                          =   0.40 per side  (sec 6.4)
recess opening                                  = 254.80 x 51.60
```

254.00 mm inside a deck of roughly **295 mm** (K9 Pro 287.0 + 2 × ~3.2 wall,
`keyboard-k9pro.md` §2, `keyboard.md` §5.1) leaves **≈20 mm of shell each
side** — enough for a recess shoulder, which an 11-tile (279.40 mm) plate would
not have. `[DERIVED]`

### 10.4 Pot pitch: 50.80 mm, and the reconciliation of 39 vs 19.05

MIL-STD-1472H Figure 10 asks for **25 mm minimum edge-to-edge separation**
between continuous rotary controls operated by one hand, 50 mm optimum
(`precedents.md` §6.2). On the module:

| pitch | modules | Ø13 knob | Ø20 knob | Ø26 knob |
|---|---|---|---|---|
| 31.75 | 5 | 18.75 ✗ | 11.75 ✗ | 5.75 ✗ |
| **38.10** | **6** | **25.10 ✓** | 18.10 ✗ | 12.10 ✗ |
| 44.45 | 7 | 31.45 ✓ | 24.45 ✗ *(misses by 0.55)* | 18.45 ✗ |
| **50.80** | **8** | 37.80 ✓ | **30.80 ✓** | 24.80 ✗ *(misses by 0.20)* |
| 57.15 | 9 | 44.15 ✓ | 37.15 ✓ | **31.15 ✓** |

`[DERIVED, separation = pitch − knob diameter]`

Two of those lines are worth staring at. **38.10 mm pitch with a 13 mm knob
gives 25.10 mm of separation — MIL's minimum, met with 0.10 mm to spare, at a
pitch that is also exactly the Pattern B mounting-hole pitch.** And **44.45 mm
misses by 0.55 mm with a Ø20 knob**, which is the kind of near-miss that gets
built by accident.

**Recommendation: pot pitch 50.80 mm = 8 modules = 2 tiles = 2.000 in**, with
the Ø20 × 12.5 set-screw knob (`parts.md` §5.5, Adafruit 5527/5528/5530) →
**30.80 mm separation**, comfortably over MIL's minimum and most of the way to
its 50 mm optimum. Three pots then occupy 2 × 50.80 + 20 = **121.60 mm** of
width, with their outer centres 101.60 mm apart = **4 tiles exactly**. If you
want MIL-legal *size coding* between pots and encoders —
"knob diameter differences not less than 13 mm" (MIL 5.1.1.4.3.2,
`precedents.md` §6.7) — the pair is Ø13 encoders and **Ø26 pots at 57.15 mm**
(9 modules), spanning 140.4 mm.

**Which standard wins where, stated so it is not re-litigated.** `envelope.md`
already settled this and the arithmetic holds:

- **The encoder row is 19.05 mm, and the knob is 13.00 mm.** The 5752 fixes four
  encoders at 19.05 mm `[STEP]`; MIL's 25 mm separation would need a 38.00 mm
  pitch and a 127.00 mm span on a 76.20 mm board, short by 50.80 mm. The
  standard's separation is **impossible on this part at any knob size**, and
  19.05 − 6.00 finger gap = 13.05 against MIL's 13.00 minimum knob leaves a
  0.05 mm window. `envelope.md` §"Settled by arithmetic".
- **The three pots are 50.80 mm**, because they are the controls MIL's 25 mm is
  actually written for: found blind, under stress, set and left
  (`precedents.md` §6.7). The encoder row is played *as a row*, where the hand
  finds the group and counts across it.
- **`precedents.md`'s 39 mm is not wrong, it is just not on the grid.** It came
  from 14 mm knob + 25 mm separation. The grid-legal versions of the same
  intent are **38.10 mm with a Ø13 knob** (25.10 mm separation — *better* than
  39 mm with a Ø14, which gives exactly 25.00) and **50.80 mm with a Ø20**. So
  the grid does not cost anything against the standard; it improves on it by
  0.10 mm and names a real inch.

Two different jobs, two different pitches, one standard read correctly — and
every number a whole multiple of 6.35 mm.

---

## 11. NOT SOURCED — get the calipers, or get a quote

Stated loudly, per the project's rule that an unsourced number is a bug.
Items 1–6 are closed by **one test coupon** ordered on the same sheet as the
plate (§8.3.7) plus one set of calipers.

**Measure on hardware:**

1. **PEC11 nut and washer dimensions** — across-flats and thickness. Carried
   over from `parts.md` §10.3 and still the input to the 4.10 mm panel budget
   in §1, to the 6.35 mm inter-nut clearance in §2.6, and to every bushing line
   in §2.2. The Bourns PTV09 figures (nut 11.0 A/F × 2.0, washer 0.4) are a
   **proxy**, not this part's data.
2. **Alpha RD901F anti-rotation lug** — whether the `B6`/`E6` bushing variants
   have a locating lug or stop pin, and if so its diameter and its distance
   from the shaft axis. §5.2 needs it to size a hole and to check the web
   between that hole and the Ø7.4. The Taiwan Alpha drawing available to this
   environment does not resolve the mounting-hole detail.
3. **What 1.5 mm of laser-cut acrylic survives under a pot nut** — no
   manufacturer publishes a bearing or torque limit for this joint. The RD901F's
   7 kgf·cm is the *pot's* limit, not the panel's. §5.2 and §9 route around the
   question rather than answer it; if the all-acrylic variant is built, **test
   it to destruction on a coupon** before trusting it.
4. **Kerf, machine tolerance and edge taper, on the machine that will cut this
   plate.** §4 spans 0.105–0.2 mm of kerf and ±0.13–0.23 mm of machine
   tolerance across three services, and the 1–2° taper figure is community
   practice whose source page could not be retrieved from this environment.
   **Also measure which way the taper leans** (§4.3).
5. **The MX cutout that actually works** in the delivered material —
   13.70 / 13.80 / 13.90 on the coupon. Cherry's 14.00 ± 0.05 cannot be held
   by any laser (§4.6).
6. **NeoSlider nubbin (5621/5741) bore depth** — carried over from `parts.md`
   §10.2. §4.5 cannot specify the layer-1 slider channel width or depth without
   it, and §2.3's 7.40 mm shank protrusion is only useful if the nubbin's bore
   reaches it.
7. **Ponoko's current acrylic catalogue** — thicknesses, matte colours,
   accepted file formats, maximum part size and whether the matte grades are
   cast or extruded. The thickness list in §3.2 is from an undated forum post
   and the catalogue page gave no formats or sheet size. One quote closes it.
8. **Whether SendCutSend will countersink acrylic.** Their countersinking page
   says "sheetmetal and plastic laser cut parts"; their acrylic material page
   lists laser cutting as the only add-on. If they will, a countersink in
   acrylic changes §2.5's fastener scheme. Ask them.
9. **Schaeffer AG tolerances, and whether Front Panel Designer will cut a
   counterbore** (a blind milled pocket concentric with a through-hole). §9's
   escape hatch depends on it. Their pages state "inner and outer contours" and
   "material thickness up to 10 mm" but publish no tolerances and never use the
   word counterbore.
10. **Real one-off prices** for (a) a 254 × 51 mm, 3 mm satin extruded acrylic
    plate and (b) a 254 × 51 mm, 1.6 mm FR4 panel, 5 off. §8.2 has anchors, not
    quotes. Both are instant-quote uploads.
11. **FDM recess accuracy on the printer that will make the shell.** §6 designs
    to Protolabs Network's published ±0.5 % / ±0.5 mm because that is what is
    published; a calibrated machine you own will be much better in Z and the
    governing error will be XY. **Print a 254 mm test frame with a 4.60 mm
    recess and measure it** — this is the single measurement that decides
    whether the step in §6.2 is 0.30 mm or 0.60 mm.
12. **Whether a satin acrylic window passes 940 nm** for the VL53L4CD (5396),
    and ST's cover-glass and crosstalk requirements. §7.5 assumes the ToF gets
    an open aperture. If it needs a window, that window is a different material
    from the rest of layer 1 and probably a different lamina.
13. **5752 NeoPixel visibility with an encoder fitted** — carried over from
    `parts.md` §10.8. §7.5 option 2 cannot place a light hole until it is known
    whether there is light to let out.
14. **The MX switch's clip height above the PCB face** — i.e. the distance from
    the 4980's front face to the underside of the plate the clips latch onto.
    `parts.md` §4 gives 11.6 mm to the stem tip, 3.6 mm of stem above the
    housing, a 3.30 mm locating post and a 1.5 mm plate, but **not that offset**,
    and the Cherry drawing was read for the cutout rather than for the stack.
    It sets the NeoKey bay's standoff, it is probably *not* the 6.50 mm that
    suits the encoder and slider bays (§2.6), and if it is not, **the shell needs
    two different boss heights under one plate.** Measure it on a real switch
    pressed into a real 4980 with a 1.6 mm offcut as the plate — five minutes,
    and it is the one open item in this document that can change the shell.
