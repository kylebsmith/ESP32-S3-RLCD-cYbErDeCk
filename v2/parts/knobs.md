# The recessed knob-in-bore

*An enclosure input. The user has changed the mounting concept: nothing is
panel-mounted, every control is carried by the **3D-printed carrier plate**, and
each rotary control gets a **bore straight through the 12.70 mm clear acrylic**
which its knob sits down inside, with only the top proud. The acrylic also runs
over the display with no cutout.*

*Every number carries a source tag. Numbers that could not be traced to a
primary source are marked **NOT SOURCED** and listed in §6. Nothing here is
estimated.*

---

## 0. Verdict up front

1. **The stock Adafruit 5880 now works, unmodified. Do not buy 4991 boards, do
   not buy 25 mm PEC11s, do not hand-solder three encoders.** `stack.md` §2.3
   reached that conclusion because a knob sitting *on top of* 12.70 mm of
   acrylic had only 2.30 mm of shaft to grip. Once the knob comes **down inside
   the bore**, the shaft it can reach is measured from the acrylic's *underside*,
   not its top face, and that is **15.00 mm** — 5.00 mm of M7 bushing plus the
   **full 10.00 mm of Ø6 D-shaft**, of which **7.00 mm carries the flat**
   `[DS pec11 p3]`. That is more usable shaft than the rebuilt 25 mm encoder was
   going to deliver through a flat panel. §1.3.

2. **The pots also get *shorter*, not longer, and wobble less.** `stack.md` §2.5
   needed a 30 mm shaft on a 1.270 mm boss and paid **1.20 mm P-P** of shaft play
   for it `[DS ptv09 §2.3]`. Recessed, a **20 mm** shaft on the same 1.270 mm boss
   puts the tip 0.08 mm under the encoders and the play drops to **0.80 mm P-P**.
   §1.4.

3. **The acrylic's underside stays at Z = 6.350 (¼ in) — the display fits under
   it with 1.270 mm to spare, and this is now sourced rather than assumed.**
   Waveshare publish the ESP32-S3-RLCD-4.2 V2 as **14.1 mm total thickness** and
   **4.35 mm LCD-to-acrylic** `[VENDOR W1]`, and their V2 dimension drawing
   carries a third, unlabelled **7.00** `[VENDOR W5]`. The three close exactly —
   in both revisions — on a middle term of **2.75 mm = LCD glass + PCB**, which
   makes the **display assembly 5.53 mm tall above its PCB front face**. §1.1.

4. **A finger cannot get into the bore, and that settles the whole grip
   question.** MIL-STD-1472H figure 57 puts *minimal finger access to the first
   joint* at **Ø32 mm bare-handed** `[MIL fig 57]`. The annular gap around a
   recessed knob is 2.00 mm. **Nothing of the hand enters the bore**, so the
   entire grip is carried by the proud part and the rim. That is the real risk
   the brief names, and it is not a matter of opinion. §2.2.

5. **The knob is therefore a flanged plug, not a cylinder, and the flange is the
   whole design.** Ø**18.00** body filling a Ø**22.00** bore, under a Ø**25.40**
   brim standing **6.350 mm** proud and overhanging the bore by **1.70 mm** all
   round. The grip is a positive lip you curl a fingertip under, not friction on
   a short cylinder. Total knob height **19.050 mm = one key unit**, of which
   12.70 is inside the slab. §2.3.

6. **Torque is a non-issue; purchase is everything.** The worst control torque on
   this deck is **24.5 mN·m** (Alpha RD901F, 250 gf·cm `[DS RD901F via parts.md]`)
   against MIL's 32 mN·m ceiling for ≤25 mm knobs `[MIL fig 10]`. At a Ø25.40 rim
   that is **1.93 N** of tangential force `[DERIVED]`. Two fingertips deliver that
   trivially. What they cannot do is hold on to a 2 mm step. §2.2.

7. **The knob's internal shoulder becomes the push-switch's overtravel stop.**
   The PEC11's switch needs **610 ± 306 gf — up to 8.98 N** `[DS pec11 p1]` and
   travels **0.5 ± 0.2 mm**. Size the knob's Ø8.00 counterbore **5.00 mm** deep
   and its floor sits **0.80 mm** above the bushing's end face: past the switch's
   maximum travel, so the shoulder stops overtravel and the 9 N goes
   knob → bushing → encoder body → PCB, never into four solder joints. §1.3.

8. **The bore wall becomes the encoder's indicator, for free.** The 5880's
   reverse-mounted NeoPixel sits **10.41 mm off the knob axis** with a Ø2.40
   aperture `[STEP 4991]`, i.e. spanning R9.21–R11.61. The knob body ends at
   R9.00 and the bore wall is at R11.00: **the NeoPixel straddles the bore wall**
   and lights both the annular gap and the acrylic itself. Do not move the bore.
   §2.4.

9. **NeoSlider travel is 60 mm. It is sourced, and the project's single most
   important missing number is now closed.** It is not in the STEP — I confirmed
   today that the cover is drawn solid with a hole exactly the shank's own
   5.141 × 1.800 section, so the model genuinely cannot give it. It **is** on the
   manufacturer's outline drawing that Adafruit publish as the datasheet for the
   bare pot, **PID 4219**, `4219_C11375.pdf`: the lever is drawn at both extremes
   with **60** between them. Verified against the drawing's own scale, which
   measures **8.000 px/mm** across five independent dimensions. §4.2.

10. **But that same drawing says the lever is 15 mm proud of the body, and
    Adafruit's STEP draws 9.50.** A 5.5 mm disagreement, on the one part that was
    already marginal. §4.2 gives the caliper procedure. It is good news if the
    drawing wins.

11. **The 2 × 8 header is centred on the Waveshare board.** Measured off the
    vendor's V2 back view by pixel scaling against its own 92.50 × 70.10 outline:
    body **22.6 × 6.3 mm**, centre within **0.31 mm in X and 0.68 mm in Y** of the
    board's geometric centre. Its body *height* is still not published but is now
    **bounded at ≤ 7.00 mm** by the drawing. §4.3.

12. **One thing the recess quietly deletes: the acrylic stops carrying load.**
    `stack.md` §2.1 engineered a path where finger push went shaft → encoder
    shoulder → the acrylic's underside. With a Ø22.00 bore the acrylic no longer
    touches the encoder at all. Every mechanical load now lands on the printed
    carrier. On a material whose failure mode is crazing under stress plus
    solvent `[RÖHM mach §8]`, that is a straight win.

---

## 0.1 Source tags

| Tag | Meaning |
|---|---|
| `[STEP]` / `[STL]` | **Measured by me**, today, from the vendor model — mesh slicing and connected-component analysis, not the model's face list. Method and numbers in §4.2. |
| `[DS 4219]` | The slide-potentiometer manufacturer's outline drawing, `cdn-shop.adafruit.com/product-files/4219/4219_C11375.pdf`, linked from the Adafruit 4219 product page as its datasheet. Read directly; pages 1 and 3 rendered at 200 dpi and measured. |
| `[DS pec11]`, `[DS ptv09]` | Bourns datasheets, local in `v2/parts/models/datasheets/`. |
| `[MIL …]` | **MIL-STD-1472H**, read directly from the PDF (the file is named …1472G; its running header is H). Clause numbers and figure numbers given. |
| `[VENDOR W1]` | `docs.waveshare.com/ESP32-S3-RLCD-4.2`, product page text, fetched and quoted. |
| `[VENDOR W5]` | Waveshare's **V2 Product Dimensions** drawing, `ESP32-S3-RLCD-4.2-ProductSize-V2-*.webp`, fetched and measured. |
| `[SO cc200]` | Smooth-On **Crystal Clear 200** technical data, read from the product page. |
| `[FL]` | Formlabs **Silicone mold making and silicone part production** white paper. |
| `[SVC …]` | A fabrication service's published specification. |
| `[3P …]` | Third party — vendor blog or community practice. Weakest; always flagged. |
| `[DERIVED]` | Arithmetic from the above, shown in full. |

URLs in §7.

---

# 1. The section, solved

Datum, unchanged from `stack.md` §2: **Z = 0 at the carrier plate's top face** —
the visible wiring plane. The acrylic's underside sits at Z = **A**, its top face
at **A + T**.

## 1.1 First: where the acrylic's underside has to be

The acrylic now covers the display with **no cutout**, so the tallest thing under
it is almost certainly the display module. That number was unknown. It is now
sourced.

**What Waveshare publish, verbatim** `[VENDOR W1]`:

> "The thickness between the LCD and the acrylic is adjusted from 3.75 mm to 4.35 mm"
> "The total thickness is adjusted from 13.50 mm to 14.1 mm"
> "To reduce the risk of damage from stress on the LCD screen, the V2 version uses a
> different acrylic spacer, so the spacer thickness is increased compared with the
> V1 version; all other parts remain unchanged."

**And what their V2 dimension drawing carries** `[VENDOR W5]`: three numbers on
the edge view — **4.35**, **14.1**, and an unlabelled **7.00**. The V1 drawing
carries the same three as **3.75**, **13.50**, **7.00**.

The chain closes, in both revisions, on the same middle term:

```
V2:   14.10  -  4.35  -  7.00  =  2.75
V1:   13.50  -  3.75  -  7.00  =  2.75          [DERIVED]
```

The vendor says the revisions differ **only** in the spacer. The arithmetic says
exactly that, twice, to the hundredth. So the three dimensions are a chain
through the stack:

| | mm | what it is |
|---|---|---|
| 4.35 | front | acrylic spacer, outer face → LCD surface `[VENDOR W1]` |
| **2.75** | middle | **LCD glass + PCB** `[DERIVED — the term the chain forces]` |
| 7.00 | back | PCB back face → back-shell outer face `[VENDOR W5, unlabelled]` |
| **14.10** | total | `[VENDOR W1 + W5]` |

With the industry-standard **1.57 mm** PCB (which is what every Adafruit breakout
in `parts.md` §1 measures, and which Waveshare do not publish — §6):

```
display assembly above the PCB FRONT face  =  14.10 - 7.00 - 1.57  =  5.53 mm
everything behind the PCB BACK face        =                          7.00 mm
implied LCD glass                          =  5.53 - 4.35         =  1.18 mm
                                                                    [DERIVED]
```

**So the display is only 5.53 mm tall above its own PCB.** That is the number the
whole section hangs off, and it is smaller than anyone expected — because the
4.2 in reflective panel is thin glass and most of the 14.1 mm is the back shell
and the battery compartment.

### The consequence

`carrier.py` already drops the Waveshare board into a bay in the carrier's top
face. Put its **PCB front face at Z = −0.450** — essentially flush, like every
other board — and:

```
Waveshare PCB front face           Z = -0.450
display outer (acrylic) face       Z =  5.080      = 4 units = 0.200 in
ACRYLIC UNDERSIDE                  Z =  6.350      = 1/4 in
clearance over the display         =     1.270 mm  = 1 unit            [DERIVED]
acrylic TOP face                   Z = 19.050      = 3/4 in = 1 key unit
```

**`stack.md` §2.1's A = 6.350 survives untouched.** The 6.35 : 12.70 : 19.05
ratio, the whole reason the project chose ¼ inch, is not disturbed by covering
the display.

**Why 1.270 mm of clearance and not zero.** Waveshare say, in their own words
`[VENDOR W1]`: *"reserve enough space for the screen and spacer to avoid damage
to the LCD screen caused by assembly pressure."* The gap has to absorb the FDM
pocket-depth tolerance — **±0.5 mm** prototyping, ±0.25 mm industrial
`[SVC protolabs via stack.md §5.1]` — plus the cast slab's flatness. 1.270 covers
±0.5 with 0.77 mm to spare and is one module.

**And the cost of glazing it, stated once.** Four surface crossings at 4 % each
`[RÖHM gsxt]`: `0.96⁴ = 0.849` → **15.1 % of the display's light is gone**, plus a
full-strength specular reflection off the slab's front face, on a panel with no
backlight. `stack.md` §4.5 recommended an aperture instead. The user has
overruled that deliberately, for protection, and §5 is the reason that decision
can still work out.

## 1.2 The governing equation, restated for a bore

`stack.md` §1.2 had:

```
protrusion above the panel = (shaft above the part's shoulder) - (panel thickness T)
```

That equation is what killed the stock encoder. **The bore deletes the T term.**
A knob that comes down inside the bore meets the shaft wherever it likes, so the
question becomes:

```
engagement available = (shaft tip Z) - (knob bottom face Z)             [DERIVED]
```

and the knob's bottom face can descend to the acrylic's **underside** — 12.70 mm
lower than before. That single change is worth 12.70 mm of shaft, and it is why
every conclusion in `stack.md` §2 about buying longer shafts is now void.

## 1.3 The rotary encoder — stock Adafruit 5880, no modification

Mounted exactly as `stack.md` §5.4 already draws it: sunk into its carrier
pocket, **PCB front face at Z = −0.150**, nothing standing proud of the wiring
plane. **Standoff required: zero. The carrier does not change.**

```
PCB front face                      Z = -0.150                    [stack.md 2.1]
encoder body (16.70 sq keep-out)   -0.150 ->  6.350   6.50 tall   [STEP 5880]
M7x0.75 bushing, LB = 5.00          6.350 -> 11.350               [DS pec11 p3]
O6.0 shaft, ROUND                  11.350 -> 14.350   3.00        [DERIVED]
O6.0 shaft, FLATTED to 4.5         14.350 -> 21.350   7.00 = l    [DS pec11 p3]
shaft tip                           Z = 21.350
---------------------------------------------------------------------------
ACRYLIC                             6.350 -> 19.050  12.70 thick
bore through it                     O22.00 drawn                      [sec 2.1]
---------------------------------------------------------------------------
knob body bottom face               Z =  7.150
knob counterbore O8.00, 5.00 deep   7.150 -> 12.150
   -> floor sits 0.800 ABOVE the bushing end at 11.350   = overtravel stop
knob D-bore O6.10 (flat 4.60)      12.150 -> 22.150  10.00 deep
   -> floor sits 0.800 ABOVE the shaft tip at 21.350     = never bottoms
grub screw axis                     Z = 17.500   (inside the 14.35-21.35 flat)
brim underside                      Z = 19.850   (0.800 clear of the slab)
brim top                            Z = 26.200
---------------------------------------------------------------------------
shaft engagement in the D-bore  =  21.350 - 12.150  =  9.20 mm of O6 shaft
usable shaft above the acrylic's underside  =  21.350 - 6.350  = 15.00 mm
                                                                    [DERIVED]
```

### Say it loudly: the encoder rebuild is cancelled

`stack.md` §2.3 concluded that the 5880 "is out", that you must buy **4991**
boards and **PEC11-4225F-S0024** encoders from a distributor, and hand-solder
three of them — against a datasheet that says *"Hand Soldering… **Not
recommended**"* `[DS pec11 p1]`. That conclusion was correct **for a flat panel**
and is **wrong for a bore**. Side by side:

| | `stack.md` §2.3, 25 mm PEC11, flat panel | **this document, stock 5880, bore** |
|---|---|---|
| part | 4991 + PEC11-4225F-S0024, distributor order | **Adafruit 5880, off the shelf** |
| soldering | three encoders, by hand, against the datasheet | **none** |
| Ø6 shaft available to the knob | 12.30 mm (all above the panel) | **10.00 mm, all of it** |
| of which flatted | 12.00 mm `[DS pec11 p3]` | **7.00 mm** `[DS pec11 p3]` |
| engagement actually used | ~11 mm | **9.20 mm** |
| serviceable? | **no** — soldered to the 4991 | **yes** — it is a bought board |
| spares needed | a 4991 and a PEC11 | a 5880 |

**9.20 mm of engagement on a Ø6 shaft is a normal knob fit.** The only two bore
depths anyone publishes bracket it: Adafruit's 2056 has an **11 mm keyway**
`[AF via parts.md §5.5]` and an RS PRO Ø20 × 12.5 knob lists **"Depth 9 mm"**
`[DIST rs-pro via stack.md §2.3.1]`. And unlike either, **the grub screw lands on
the flat over its whole engagement**: the flat window is Z = 14.350 → 21.350 and
the screw sits at Z = 17.500, 3.15 mm from each end of it.

**The one thing to check on arrival:** the PEC11's flat length `l` is **7.00 mm**
for the 15 mm shaft and 12.00 for the 25 `[DS pec11 p3]`. 7.00 mm is enough for
one grub screw but **not for two at different heights** — so do not design a
two-screw hub.

## 1.4 The potentiometers — and they get *shorter*

The pot is held by its own bushing nut to the carrier, with the body below and
the washer and nut on the carrier's top face, in view through the acrylic
(`stack.md` §2.5's aesthetic, unchanged).

Design rule, also unchanged: **every shaft tip lands in one plane**, set by the
encoder at Z = 21.350. The pot's mounting surface therefore sits at
Z = 21.350 − *L*:

| shaft code *L* | mounting surface Z | carrier boss | shaft play at the tip `[DS ptv09 §2.3]` | verdict |
|---|---|---|---|---|
| 15 | 6.350 | 6.350 mm pillar, inside the bore | 0.60 mm P-P | works, but a 6.35 pillar on the visible plane |
| **20** | **1.350** | **1.270 mm (1 unit)**, tip lands at 21.270 | **0.80 mm P-P** | **this one** |
| 25 | −3.650 | carrier locally pocketed 3.65 deep | 1.00 mm P-P | awkward |
| 30 | −8.650 | below the carrier's own bottom face | 1.20 mm P-P | no |

`[DERIVED]`

**Recommendation: *L* = 20 on a 1.270 mm boss.** Compare `stack.md` §2.5, which
needed *L* = 30 and paid **1.20 mm P-P** of wobble for it. Bourns print play as
**0.8 × L/20 mm peak-to-peak** under 250 gf·cm of end thrust `[DS ptv09 §2.3]`, so
the recess buys a **33 % reduction in shaft wobble** as a side effect of needing
less shaft. At the knob's top face (24.85 mm above the pot's mounting surface)
that scales to **0.99 mm P-P**, i.e. the knob's rim moves ±0.50 mm — visible if
you look for it, and the dominant term in the bore-clearance budget of §2.1.

Checks:

```
carrier thickness at the pot   = 1.270
bushing needed                 = 1.270 + 0.400 washer + 2.000 nut = 3.670
PTV09A-6 bushing               = 5.000  [DS ptv09 p3, and sec 6]   -> 1.33 spare
nut + washer occupy            Z = 1.270 -> 3.670, on the wiring plane, visible
knob D-bore engagement         = 21.270 - 12.150 = 9.12 mm                [DERIVED]
```

The pot's body hangs below the boss: a PTV09A body is ~7.5 mm deep `[DS ptv09]`,
so it reaches Z ≈ −7.8, about 0.2 mm past the carrier's −7.620 bottom face.
**The carrier needs a local through-relief under each pot.** One line, but it is
the kind of thing that is discovered during assembly if it is not drawn.

**Order code: `PTV09A-6020F-B103`** — rear mount with metal threaded bushing, no
detent, **20 mm**, **F = flat type shaft**, linear 10 kΩ `[DS ptv09 p1]`.
The **F** matters and §3.1 explains why: it makes the pot's bore identical to the
encoder's, so **one knob part fits every control on the deck**.

## 1.5 The whole section, in one block

```
                                                   Z (mm)   source
  knob brim TOP (all six, one plane)                26.200  [DERIVED]
  knob brim underside (0.800 float)                 19.850  [DERIVED]
  --------------------------------------------------------
  acrylic TOP face                                  19.050  = 3/4 in
  encoder shaft tip                                 21.350  [DS pec11 + STEP]
  pot shaft tip (L=20 on a 1.270 boss)              21.270  [DERIVED]
  knob D-bore floor                                 22.150  (0.800 over the tip)
  grub screw axis                                   17.500  (in the 7.00 flat)
  knob counterbore floor                            12.150  (0.800 over bushing)
  encoder bushing top (LB = 5.00)                   11.350  [DS pec11 p3]
  knob body bottom face                              7.150
  ACRYLIC UNDERSIDE                                  6.350  = 1/4 in
  encoder body top / its own shoulder                6.350  [STEP 5880]
  display outer acrylic face                         5.080  [DERIVED, sec 1.1]
  pot nut + washer                              1.270-3.670 [DS ptv09 p3]
  pot mounting boss top                              1.270
  CARRIER TOP FACE = wiring plane                    0.000  <- datum
  breakout PCB front faces                          -0.150  [stack.md 2.1]
  Waveshare PCB front face                          -0.450  [DERIVED, sec 1.1]
  carrier bottom face                               -7.620  = 6 units
  --------------------------------------------------------
  knob height H = 26.200 - 7.150 = 19.050 = ONE KEY UNIT
  of which inside the slab: 12.700;  proud: 6.350              [DERIVED]
```

Three numbers worth noticing, because none of them was aimed at:
**H = 19.050 = one key unit**, **body = 12.700 = the slab thickness exactly**,
**proud = 6.350 = the wiring gap**. The knob is built out of the same three
numbers the rest of the instrument is.

---

# 2. The bore, and the finger

## 2.1 Bore against knob: the clearance budget

The knob is located by its **shaft**, not by the bore. The bore only has to not
touch it, under the accumulated error of a laser-cut slab, a printed carrier and
a potentiometer's own shaft play.

| term | radial, mm | source |
|---|---|---|
| laser hole size and position | ±0.23 | `[SVC sendcutsend]` — a blanket acrylic figure, **not** thickness-qualified (§6) |
| FDM carrier, board pocket position | ±0.50 | `[SVC protolabs]`, prototyping |
| pot shaft play at the knob's rim | ±0.50 | `[DS ptv09 §2.3]`, 0.8 × 24.85/20 = 0.99 P-P |
| knob D-bore to shaft | ±0.10 | §3.1 |
| **RSS** | **0.75** | `[DERIVED]` |
| **laser taper, 12.70 mm, 1.0–2.0°** | **0.22 – 0.44 one-sided** | `[DERIVED, stack.md §3.2]` |
| **worst case at the narrow face** | **1.19** | `[DERIVED]` |

**Which face is narrow matters.** `stack.md` §3.2 establishes that at 12.70 mm you
are cutting deliberately out of focus — Trotec: *"Above 0.25″ (6 mm) defocus 1/3
into the material"* `[TROTEC]` — so the kerf opens at the entry face and the hole
is **wide at the top, narrow at the bottom**, and `stack.md` §4.3's procedural rule
is to **put the visible face up**. The knob therefore drops in through the wide
end and must clear the narrow end.

```
bore drawn (= finished wide face)   O22.00
taper across the diameter           0.44 (1.0 deg) .. 0.89 (2.0 deg)
finished NARROW face                O21.56 .. O21.11
knob body                           O18.00
radial clearance at the narrow face  1.78 .. 1.56   > 1.19 worst case   OK
annular gap at the visible top face  2.00                            [DERIVED]
```

**Draw the bore Ø22.00.** It also satisfies the material rules at the layout's
existing 38.100 mm pitch, with margin on all three:

```
Rohm, hole centre >= 1.5 x hole diameter   1.5 x 22.00 = 33.00 <= 38.10   OK
minimum web >= 1 x sheet thickness         38.10 - 22.00 = 16.10 >= 12.70 OK
minimum hole 0.5-1.5 x t = 6.35-19.05      22.00 is ABOVE the band        OK
                                   [ROHM kb] [3P fab-guides] [DERIVED]
```

Being *above* the minimum-hole band is the safe side, and a Ø22.00 well is also
the large, gently curved, low-stress-concentration aperture that `stack.md` §4.4
argues for on crazing grounds — far better than the Ø8.20 bushing hole the flat
panel would have needed.

## 2.2 Can a finger actually turn it?

This is the TX-6 question. `precedents.md` §5 records the reviews verbatim: the
knobs are *"way too small for anything else than 'set and forget'"*, *"closely
placed, tiny knobs can be challenging to operate"*. The failure there was
diameter and pitch. Here the failure mode is different and worse: **the knob is
in a hole.**

### The decisive number: no part of the hand enters the bore

MIL-STD-1472H figure 57, *Minimal Finger Access to First Joint*, verbatim
`[MIL fig 57]`:

> Push Button Access:
> Bare hand — **32 mm (1.25 in) dia**
> Gloved hand — 38 mm (1.5 in) dia

Our annular gap is **2.00 mm**. It is not a question of degree. **Nothing reaches
down the bore.** Every claim that "you can get a fingertip in beside the knob" is
wrong by more than an order of magnitude, and the whole grip is therefore carried
by the **6.350 mm of proud brim and its rim**. Design accordingly, and stop
thinking of the annulus as finger room — it is a light gap (§2.4) and a
tolerance, nothing else.

### What the standard asks of that 6.350 mm, and where we fail it

MIL-STD-1472H figure 10, verbatim `[MIL fig 10]`:

| | H, height | D, diameter |
|---|---|---|
| **A, fingertip grasp** min | **13 mm** | 10 mm |
| A max | 25 mm | 100 mm |
| **B, thumb and finger encircled** min | **13 mm** | **25 mm** |
| B max | 25 mm | 75 mm |
| **C, palm grasp** min | — | 38 mm |

**H min = 13 mm, and we are offering 6.350. We do not meet figure 10, and it
should be said in those words.** What we have instead:

- **The standard explicitly asks for this knob to be recessed.** 5.7.9.3.5, verbatim:
  *"**Recessed mounting.** If small projecting parts, such as toggle switches or
  small knobs, must be mounted on a front panel, they should be recessed."*
  `[MIL 5.7.9.3.5]`
- **Figure 10's H assumes friction on a bare cylinder.** Our grip is a **positive
  lip**: the brim overhangs the bore by 1.70 mm all round, so a fingertip curls
  *under* the rim and the hand cannot slide axially off the top. The standard has
  no clause for that case; it is the geometry of a bottle cap and a camera lens
  ring, and it works at heights a plain cylinder cannot.
- **The torque is near the bottom of the standard's range**, which is the explicit
  licence figure 10 gives: *"Within these ranges, knob size is relatively
  unimportant, **provided the resistance is low** and the knob can be easily
  grasped and manipulated"* `[MIL 5.1.4.1.2.1.7]`.

```
PEC11 rotational torque      50 - 200 gf-cm  =  4.9 - 19.6 mN-m   [DS pec11 p1]
Bourns PTV09 torque          10 - 100 gf-cm  =  1.0 -  9.8 mN-m   [DS ptv09]
Alpha RD901F torque          20 - 250 gf-cm  =  2.0 - 24.5 mN-m   [DS RD901F]
MIL ceiling, knobs <= 25 mm                      32   mN-m        [MIL fig 10]

worst case, at a O25.40 rim:  F = 0.0245 / 0.01270 = 1.93 N = 197 gf  [DERIVED]
```

**1.93 N.** A pinch of a few newtons at µ ≈ 0.4–0.6 on a serrated rim delivers
that with an order of magnitude to spare. **The knob is not torque-limited. It is
purchase-limited**, which is a geometry problem, not a force problem.

### So: how far does it poke up?

```
  2 mm  MIL fig 14, minimum push-button displacement        [MIL fig 14]
  6 mm  MIL fig 14, maximum push-button displacement        [MIL fig 14]
 13 mm  MIL fig 10, minimum knob height H                   [MIL fig 10]
```

The proud height falls between two of the standard's own scales — bigger than any
button travel, smaller than any knob height — and there is no clause that covers
it, so I take **the top of the lower scale and round to the project module**:

> ### **Proud height p = 6.350 mm (¼ in, 5 modules). Minimum 5.00. Below 4.00 the push switch stops working.**

Defence, term by term:

- **6.350 is the top of MIL's own button-displacement band plus a margin**, so a
  fingertip meets a step it is dimensioned to find.
- **It equals the wiring gap A**, so the knob stands as far above the slab as the
  wiring sits below it — the object has one proud dimension, used twice.
- **Below 4.00 mm the push switch fails first, not the rotation.** The PEC11's
  switch takes **610 ± 306 gf — up to 8.98 N** `[DS pec11 p1]`. Pushing 9 N down a
  2–3 mm-proud disc with a 2.00 mm moat round it, you skid off and your fingertip
  lands on the acrylic. The rotation would still work at 3 mm; the press would not.
- **Above ~10 mm it is no longer recessed**, and the brief's whole thesis —
  "it allows things to sink back into the enclosure and therefore is more robust"
  — is gone. At 13 mm the knob is as tall as the slab is thick.

### Three things the rim must have, and all three are requirements, not taste

MIL-STD-1472H, verbatim:

- *"All rotary adjustment knobs shall have rims with surfaces capable of obtaining
  a secure grasp."* — 5.1.4.1.2.1.4 `[MIL]`
- *"**Serrated rims.** Intermediate and larger size knobs shall have serrated rims
  for **low torque applications**."* — 5.1.4.1.2.1.4.2 `[MIL]`. Ours is a low
  torque application by the arithmetic above. **Serrate the rim.**
- *"The push button surface **shall be concave (indented)** to fit the finger."* —
  5.1.4.2.1.1.2 `[MIL]`. **The encoder knob's top *is* a push button** — the PEC11
  carries an SPST momentary switch `[DS pec11 p1]` — so this clause applies
  directly. **Dish the top**, and the dish doubles as rotational purchase.
- *"All edges shall be rounded in order to minimize the potential injuries
  associated with small, sharp knob designs."* — 5.1.4.1.2.1.6 `[MIL]`.

A 2 mm-deep spherical dish in a Ø25.40 top, a 24- or 36-flute serrated rim
6.350 mm tall, a 1.70 mm overhanging lip, and a 0.5 mm radius on every edge. That
is the knob.

## 2.3 Knob diameter, and what pitch "chunky" really costs

The layout is three encoders over three pots on a **38.100 mm** pitch, with
Ø25.400 knobs — and `layouts/README.md` is explicit that 38.10 + 25.40 = 63.50 is
what makes the encoder grid align with the display's 63.60 active height.

**At that pitch, Ø25.40 is already the largest knob the material allows**, and
recessing does not change it:

```
brim    O25.40  (the layout's existing knob -- unchanged)
bore    O22.00  (brim overhangs it by 1.70 radial)
body    O18.00  (1.78-2.00 radial clearance in the bore)

Rohm 1.5x rule caps the bore at 38.10 / 1.5 = O25.40
minimum-web rule caps it at      38.10 - 12.70 = O25.40     [identical]
                                                       [ROHM kb][3P fab-guides]
```

**So the layout does not have to move.** The plan is unchanged; only the section
is new. That is the right answer for this iteration.

### But the separation rule is broken, and it was broken before this document

MIL-STD-1472H figure 10, separation, edge to edge, one hand individually:
**minimum 25 mm, optimum 50 mm** `[MIL fig 10]`. Table III agrees for continuous
rotary against continuous rotary `[MIL table III]`.

```
at P = 38.10 with a O25.40 brim:  separation = 12.70 mm = 50.8% of the minimum
```

**Half.** And `precedents.md` §6.1 already noticed the same arithmetic from the
other end ("three key pitches, 39 mm, is exactly the minimum for a 14 mm knob").
This is a **pre-existing layout condition**, not something recessing causes — but
it is the TX-6 trap written as a number, and it has to be on the record.

What it would take to fix it:

| pitch | = | max brim for MIL's 25 mm | what that brim is, per MIL fig 10 | three-knob span |
|---|---|---|---|---|
| 38.100 | 2 key units | **13.10** | below B's Ø25 min — a fingertip knob | 89.3 |
| 44.450 | 35 modules | 19.45 | still below B | 108.4 |
| **50.800** | **2 in = 40 modules** | **25.80** | **meets B, thumb-and-finger encircled** | **127.4** |
| **57.150** | **3 key units** | **32.15** | comfortably B; "oversized chunky" | **146.5** |
| 63.500 | 2.5 in | 38.50 | reaches C, palm grasp | 165.5 |

`[DERIVED from MIL fig 10]`

**The smallest grid-honest pitch that gives a MIL-legal knob you can encircle
with thumb and finger is 50.800 mm = 2 inches = 40 modules, with a Ø25.40 brim —
both inches, both module multiples, separation 25.40, clearing the minimum by
0.40 mm.** For the Ø32 "chunky" the user is describing, **57.150 mm = 3 key
units**, separation 25.15.

**The cost is 25.4 mm of band width at 50.80 and 44.7 mm at 57.15,** against a
control field that is already 95.25 (display) + 101.60 (grid) + 76.20 (fader)
inside 285.75. **It does not fit without rearranging**, and the layout thread owns
that trade. This document owns only the fact that **38.100 is the pitch at which
the knob stops being a MIL-compliant knob**, and the exact pitch that fixes it.

## 2.4 The bore wall is the indicator — do not move it

Measured, not inferred. The 5880/4991's NeoPixel is **reverse-mounted**, emitting
through the board to the knob side, its centre **10.41 mm off the knob axis**,
aperture **Ø2.40** `[STEP 4991, via parts.md §2.2]`. So it spans **R9.21 → R11.61**.

```
knob body outer radius     R 9.00
bore wall radius           R11.00
NeoPixel spans             R 9.21 .. R11.61        [STEP]

-> the LED STRADDLES the bore wall:  1.79 mm of it under the annular gap,
   0.61 mm of it under solid acrylic outside the bore              [DERIVED]
```

Two things happen, both wanted. The inboard part shines **up the 2.00 mm annulus**
and rims the knob. The outboard part couples **into the 12.70 mm slab at the bore
wall**, at grazing incidence, which is the one geometry that lights a thick clear
edge. `stack.md` §3.3 predicted a per-encoder indicator "with no light hole to
cut and no light pipe"; the measured numbers say it lands almost exactly on the
wall.

**Consequence for §3: a translucent knob is worth more than an opaque one.**
A clear cast knob with its bottom face 6.80 mm above the LED becomes the
indicator itself. That is the single strongest argument for casting rather than
printing, and it falls out of a measured dimension rather than a preference.

**Consequence for the carrier:** do not let the knob body grow past Ø20.80 (R10.40)
or it eclipses the LED entirely. Ø18.00 leaves 1.41 mm of clear annulus inboard
of the LED centre.

## 2.5 The top edge of the bore

On a 12.70 mm clear slab the bore wall is **a 12.70 mm tall polished cylinder you
look straight down**, and its top edge is a visible optical line. Three options:

| | effect | verdict |
|---|---|---|
| **sharp, as-lasered** | one crisp bright ring at the arris (total internal reflection along the edge); the cleanest line | **the look you want, but it is an arris on a part the hand lives on** — MIL 5.7.9.3.1 and 5.7.9.3.3 require no sharp edges and smooth exposed surfaces `[MIL]` |
| **small hand chamfer, 0.3–0.5 mm** | still reads as a single line at arm's length; removes the arris; relieves the notch | **this one** |
| **radius** | a 12.70 mm clear slab with a radiused bore edge is **a lens ring** — it will throw a distorted, magnified image of the knob and the carrier below | **no** |

**Recommendation: cut it sharp, then break the top arris by hand with a 0.3–0.5 mm
chamfer** — a deburring blade, two passes, no sanding (sanding frosts it and the
frost shows). Leave the bottom arris alone; it is inside the instrument. Röhm's
own guidance for holes is in the same direction: *"the drill holes should be
slightly chamfered or countersunk"* to avoid notch effects
`[RÖHM mach §2.4, §3.1, via stack.md §4.4]`.

**And the thing nobody expects: the bore is a cone, and the cone is a mirror.**
1–2° of taper over 12.70 mm `[3P taper]` makes the bore a shallow truncated cone.
Cut **visible face up** (which `stack.md` §4.3 already mandates for a different
reason) and the cone **opens toward the viewer** — you look down a flared well at
the knob. Cut it the other way up and the cone closes toward the knob, and the
wall becomes a conical mirror showing a smeared ring of the carrier plate. Same
part, same bore, opposite result, decided by which way up it goes on the bed.

---

# 3. Making the knobs

## 3.1 Specify the shaft first — it decides everything downstream

The single most useful decision in this document:

> ### **Every rotary shaft on this deck is a Ø6.0 flatted (D) shaft. No splines anywhere.**

- The PEC11 already is one: **Ø6.0 flatted to 4.5**, F type, flat length 7.00 mm
  on the 15 mm shaft `[DS pec11 p2, p3]`.
- The Bourns PTV09A offers the same: *"**F = Flat Type** Insulated Shaft"*,
  Ø6.0 +0.0/−0.1, flatted to **4.5**, alongside *"S = Insulated Knurled (18
  Teeth)"* and *"U = Insulated Knurled (40 Teeth)"* `[DS ptv09 p1]`.
- Therefore **one bore geometry — Ø6.0 round with a 4.5 flat — serves every
  control**, and there is one knob part number for the whole instrument.

**Why not the 18-tooth spline**, which is what `parts.md` §5.5 was steering toward:

```
T18 on a O6 shaft: circumferential tooth pitch = pi x 6 / 18 = 1.047 mm   [DERIVED]
FDM bead width, 0.4 mm nozzle                  = 0.42 - 0.45 mm
```

A **1.047 mm pitch internal spline**, with teeth a couple of tenths deep, formed
on a *concave* wall, by a 0.42 mm bead. It cannot be done, and no slicer setting
rescues it. SLA at 25–50 µm layers can form it `[FL]`, but the tooth roots in a
rigid photopolymer are the obvious crack initiators on a part you pull off a
shaft. **A flat is one feature, 4.5 mm across, and a 0.4 mm nozzle resolves it
perfectly.** It also gives the grub screw a proper seat and positive
anti-rotation.

Note the knock-on: `parts.md` §11 lists the **Adafruit 5527/5528/5530** Ø20 × 12.5
aluminium set-screw knobs, chosen because they fit *both* T18 and D. With a D
shaft everywhere that constraint relaxes — and §3.4 gives those knobs a better job.

## 3.2 Printed

**Orientation: top face DOWN on the build plate.** Three reasons, in order:

1. The **top face is the only surface anyone looks at**, and a face printed
   against the plate is the best finish FDM produces — better than ironing, which
   `stack.md` §5.5 only resorts to because the carrier's channels are *in* its top
   face. Here nothing is.
2. The **bore then faces up** and prints as a clean vertical cavity with no
   support — the one point the vendor guidance gets right `[3P qidi]`.
3. The serrated rim is a vertical wall either way, so the flutes cost nothing.

**The one thing that orientation breaks, and its fix:** the brim's underside
overhang becomes a 90° ceiling. **Do not draw it as a square step.** Draw the lip's
underside as a **45° chamfer** from Ø25.40 down to Ø18.00 over 3.70 mm — it prints
unsupported, and a 45° undercut is *better* to grip than a square one because the
fingertip wedges rather than hooks.

| decision | value | why |
|---|---|---|
| layer height | **0.10 mm**, 0.4 mm nozzle | the rim flutes and the 45° lip both resolve; 0.10 keeps the lip's chamfer smooth |
| flute count | **24 or 36** on a Ø25.40 rim | circumferential pitch **3.32** or **2.22 mm** `[DERIVED]`, both ≥ 5 bead widths. 60 flutes = 1.33 mm pitch, at the limit, and will look mushy |
| material | **PETG**, or ASA | PLA prints the nicest face but creeps under a grub screw and is brittle at the lip. A knob is a part that gets pulled |
| walls / infill | 4 perimeters, 30 % gyroid | the knob's own volume is ~6.5 cm³ `[DERIVED]`; solid, it is 40 min of printing and it warps as it cools |
| colour | **natural / clear PETG** if you want §2.4's light; matte black otherwise | the NeoPixel argument |

**The D-bore fit, honestly sourced.** The only figures I could find are a vendor
blog, and that vendor contradicts itself across two of its own articles
`[3P qidi]`: one says **0.10–0.15 mm per side** on a 6.0 mm D-shaft (bore 6.20–6.30)
for a snug friction fit, 0.05 for a press fit; another says **0.15–0.25 mm**. Both
are starting points, neither is a specification, and the true number depends on
your printer's horizontal expansion more than on anything in this document.

> **Cut a coupon, not a knob.** Print one 12 mm cylinder carrying four bores —
> **Ø6.10 / 6.20 / 6.30 / 6.40, each with a 4.60 flat**, 10 mm deep — and try all
> four on a real PEC11 shaft and a real pot shaft. Take the tightest that goes on
> by hand without the flat shaving. Then build the knob. This is one 15-minute
> print and it closes the only unsourced number in §3.

**It will not look right next to the slab, and that is the honest verdict.**
At 0.10 mm layers next to a cast, polished, 12.70 mm specular acrylic edge, the
knob is the one part of the instrument that reads as 3D-printed. Three ways out,
in increasing cost: print it on **SLA at 25–50 µm** `[FL]` and wet-sand it;
**prime and paint** an FDM knob (the flutes hide the sanding); or **cast** it
(§3.3). If the knob stays FDM, make it **deliberately matte and dark** so it reads
as a different material on purpose rather than as a failed attempt at the same one.

## 3.3 Cast — and first, what "cast in acrylic" actually means

**You cannot cast PMMA at home, and the reason is arithmetic.** Methyl
methacrylate polymerising to PMMA shrinks about **21 % by volume**, straight from
the monomer-to-polymer density change `[3P mma-shrinkage]`. Industry controls it
by pre-polymerising part of the charge into a syrup, casting into a cell in a
water bath, and running a cycle of order **16 hours**; and above ~100 °C the
exotherm makes voids, which gets worse *the thicker the casting*
`[3P mma-shrinkage]`. A Ø25 × 19 knob is a thick casting. **This route is closed**,
and it should be said plainly because "cast in acrylic" sounds like it should be
available and it is not.

**What is available, and what to call it:** a clear **polyurethane** casting
resin, which is what everyone who says "cast acrylic knob" is actually using.
The reference product, with real published data `[SO cc200]`:

| Smooth-On **Crystal Clear 200** | |
|---|---|
| mix ratio | **100A : 90B** by weight |
| pot life | **20 minutes** |
| cure | **16 hours** |
| **shrinkage** | **0.001 in/in = 0.1 %** — two orders of magnitude better than MMA |
| hardness | **Shore D 80** |
| clarity | *"water white clear and made specifically for applications that require clarity"* |
| UV | *"Castings will resist yellowing when exposed to UV, **but may darken over time**"* |

**Bubbles are the whole job**, and the vendor is unambiguous `[SO cc200]`:

> *"Bubbles in the finished casting will be greatly reduced by **vacuum degassing**
> prior to pouring… Your vacuum pump must pull a minimum of **29 inches of
> mercury**… **Bubble elimination is best achieved by pressure casting** …
> subjected to **60 PSI (4.2 kg/cm²)** air pressure for full cure time."

So the real bill of materials for a clear cast knob is a **vacuum pump and a
pressure pot**, not a mould. In a clear part every bubble is two refracting
surfaces in the middle of the thing the user is looking at — the same argument
`stack.md` §3.4 makes against laminating the slab.

**The mould, and whether layer lines transfer.** They transfer completely —
**that is what silicone is for.** Which is exactly why you do not print the
*mould*:

> **Print a MASTER, not a mould.** Print the knob at 25–50 µm on SLA, wet-sand and
> polish it, then take a **tin-cured** silicone mould from the polished master, and
> cast into that.

Formlabs' own cautions on that workflow, verbatim `[FL]`:

- *"Thoroughly wash excess resin off of the mold stock with clean IPA as **uncured
  resin can inhibit curing of silicone**."*
- Wait *"24–48 hours after post-processing before casting silicone"*.
- *"Use an **epoxy coating** for an effective barrier between the silicone and the
  3D printed mold."*
- *"Parts printed in Clear Resin **develop a yellow tint when fully cured**"* — so
  the master is for shape, not for optics. *"Polish the outer surface for a highly
  transparent finish."*
- Moulds *"typically last **10–20 uses** before signs of wear appear"* — ample for
  six knobs plus spares.

And the silicone must be the right chemistry: Smooth-On specify **tin-cured**
(Mold Max 20/25/30) or urethane rubber with a release agent, and explicitly
*"**Do not use** Mold Max 'T' or Mold Star 'T' Series"* — the platinum-cure
silicones — because the urethane inhibits against them `[SO cc200]`.

### Verdict: print for this iteration, cast only for the light

| | printed (FDM) | printed (SLA) | **cast clear PU** |
|---|---|---|---|
| looks right beside the slab | no | with polishing | **yes** |
| lights from the NeoPixel (§2.4) | no | partly | **yes — this is the reason** |
| holds a grub screw | with a captive nut | marginal, brittle | **no — needs a metal hub** |
| equipment you must own | a printer | a printer | printer **+ vacuum pump + pressure pot** |
| time per knob | ~40 min | ~40 min + wash + cure | **16 h cure, 6 knobs a day** |
| iterate the shape | minutes | minutes | **re-mould** |

**Print the v2 knobs.** The shape is not settled, §2.2's proud height wants
trying in the hand, and you cannot iterate a shape through a 16-hour cure.
**Cast them once the shape is right**, and when you do, cast them **clear**, so
§2.4's NeoPixel argument pays off. The cast version is a finish, not a prototype.

## 3.4 Retention — a knob that comes off mid-set is a failure

Three routes. The first is wrong, the second is right for printing, the third is
the only one that works for casting.

| | how | verdict |
|---|---|---|
| **press fit on the D** | bore 0.05 mm under the shaft | **No.** The PEC11's switch pushes **up to 8.98 N** `[DS pec11 p1]` down the axis every time it is used; a press fit walks down the shaft, and a printed bore wallows after a few dozen removals. |
| **M3 grub screw into a captive nut** | a hexagonal pocket in the printed knob at Z = 17.500, bridged over, nut dropped in during the print | **Recommended for printed knobs.** A printed M3 thread strips; a captive nut does not. Torque margin is enormous: an Elma M2.5 knob set screw is rated **0.2 N·m** `[DS elma-k1 via stack.md]` against **24.5 mN·m** of worst-case control torque — **8×**. |
| **a bought metal knob as the hub** | an **Adafruit 5527/5528/5530** (Ø20.0 × 12.5, Ø6 bore, 2 mm hex set screw, *"fits T18, round AND D-shaft"* `[AF]`) sunk into the knob's body, printed or cast around it | **The only option for a cast knob**, and the safest for a printed one. A cast urethane cannot hold a thread at all. It also means the bore tolerance problem goes away — the metal knob is the bore. |

**And an axial stop, which matters more than the screw.** §1.3 sizes the knob's
Ø8.00 counterbore **5.00 mm deep** so its floor sits **0.800 mm above the
bushing's end face**. The switch travels **0.5 ± 0.2 mm**, i.e. 0.70 mm worst
case `[DS pec11 p1]`. So the shoulder is **past full switch travel but short of
infinity**: press the knob and the switch actuates; press harder and the knob
**bottoms on the bushing**, and the 9 N goes into the encoder's own threaded
bushing, not into the set screw and not into the four PCB solder tails.

**Do not make the counterbore shallower than 4.20 mm** (the knob would bottom on
the bushing before the switch actuates) **or deeper than 5.60** — the shoulder
then lands 1.40 mm up, twice the switch's maximum travel, so the switch takes the
overload instead of the bushing. The window is 1.40 mm wide. Draw it at 5.00.

---

# 4. The three open dimensions

## 4.1 Waveshare board total thickness — **FOUND: 14.1 mm**

| | mm | source |
|---|---|---|
| total thickness, **V2** (shipping from 2026-10-10) | **14.1** | `[VENDOR W1]` text, `[VENDOR W5]` drawing |
| total thickness, V1 | 13.50 | `[VENDOR W1]` |
| LCD-to-acrylic spacer, **V2** | **4.35** | `[VENDOR W1]` |
| LCD-to-acrylic spacer, V1 | 3.75 | `[VENDOR W1]` |
| rear sub-dimension, both revisions, **unlabelled by Waveshare** | **7.00** | `[VENDOR W5]` |
| → LCD glass + PCB | **2.75** | `[DERIVED]` — the term the chain forces, identically in V1 and V2 |
| → **display assembly above the PCB front face** | **5.53** | `[DERIVED]`, assuming a 1.57 PCB |
| → implied LCD glass | 1.18 | `[DERIVED]` |

**Also now sourced from the same drawing** `[VENDOR W5]`, which nothing in the
repository had recorded:

| | mm |
|---|---|
| board outline | 92.50 × 70.10 |
| display **glass** outline | 84.80 × 69.10 |
| display **active** height | 63.60 (agrees with `layouts/README.md`) |
| glass inset from the left edge / from the bottom edge | 1.50 / 2.00 |
| mounting, 4 × M2.50 | 85.50 × 62.10; 3.50 in from the right edge, 4.50 up from the bottom |
| top-edge buttons (BOOT / PWR / KEY) | first at **36.25** from the edge, then **10.00**, **10.00** |

**Still open:** the **PCB's own thickness**. 1.57 mm is what every Adafruit board
in `parts.md` §1 measures and is the industry default, but **Waveshare do not
publish it** and the 5.53 depends on it 1:1. §6.

**And Waveshare's own warning, which should be read before cutting the carrier's
display pocket** `[VENDOR W1]`: *"If you need to design or make your own back
shell, please use the actual measured dimensions after receiving the product as
the standard, and reserve enough space for the screen and spacer to avoid damage
to the LCD screen caused by assembly pressure."* The 1.270 mm gap in §1.1 is that
space. **Caliper the board on arrival and confirm 5.53 ± 0.3 before the carrier is
printed.**

## 4.2 NeoSlider travel — **FOUND: 60 mm**

### First, confirming the user's finding: the STEP cannot give it

Measured today from `5295 NeoSlider Stemma.stl` (the mesh beside the STEP in
Adafruit's library), by mesh slicing and connected-component analysis:

```
whole model bbox      76.200 x 21.590 x 20.030    (= parts.md section 3)
PCB front face at Z = 0; shank tip at Z = -15.500; QT sockets to Z = +4.530
the entire slide pot, SHANK INCLUDED, is ONE connected component:
     75.000 x 9.500 x 19.824,  Z = -15.500 .. +4.324
```

**The shank is welded to the cover in the mesh.** The cover's front face at
Z = −6.000 carries a single opening, and the opening is the shank's own section:

```
cover face, z = -6.000, inner boundary on the centreline:
     x = -2.500 .. +2.641         = 5.141 mm                       [STL]
     with the local widening to y = -1.141 .. +0.659 = 1.800 mm
shank section, z = -6.60 .. -10.30:   5.141 x 1.800                [STL]
```

**They are the same number.** There is no slot — only a hole the exact size and
shape of the shank at its centred position. Scanning every z level between the
cover and the tip returns one run and no gap anywhere along the 75 mm body.
**The user's reading of the model is exactly right, and travel is not in it.**

While in there, the shank is now fully characterised `[STL]`, which the
nubbin's bore has to match:

| above the PCB front face | section |
|---|---|
| 6.50 → 10.30 | **5.141 × 1.800** — the section the nubbin grips |
| 10.30 → 10.65 | step |
| 10.65 → 13.10 | 4.141 × 1.800 |
| 13.10 → 13.50 | step |
| 13.50 → 14.75 | 4.141 × 1.200 |
| 14.75 → 15.50 | tapered tip |

**And it reconciles `parts.md`'s two body heights, which were both right.** The
cover's recessed face is at **6.00**; its raised outer rim is at **6.50**
(`z = −6.500` carries a separate front-facing flat, present at the ends and down
the sides, absent along the centre) `[STL]`. So "6.00 and 9.50 proud" measures to
the cover and "6.50 and 9.00 proud" measures to the rim. **A panel resting on the
part rests on the rim at 6.50.**

### Then, where travel actually lives

Adafruit's product page for the **bare** slide pot, **PID 4219** — the same part
fitted to the 5295 — links a manufacturer datasheet, `4219_C11375.pdf`. Page 1 is
the outline drawing, and it carries the lever **drawn at both extremes with a
dimension between them**:

> ## **Slide travel = 60 mm**

**Verified against the drawing's own scale.** Rendered at 200 dpi and measured by
pixel run-length, the drawing is **8.000 px/mm** across five independent
dimensions — the agreement is what makes the reading safe rather than a guess:

| dimension on the drawing | px | px/mm |
|---|---|---|
| 75 (body) | 600 | 8.000 |
| 71 (slot) | 567 | 7.986 |
| 62.8 | 502 | 7.994 |
| 65.2 | 524 | 8.037 |
| 73.5 | 588 | 8.000 |
| **the travel dimension** | **482** | → **60.25 mm**, labelled **60** ✔ |

Everything else on the same drawing `[DS 4219]`:

| | |
|---|---|
| body | **75 × 9**, slot **71** long |
| mounting | **2 × M2 × 0.4** threaded, at the ends |
| lever height above the body top | **15** (but see below) |
| lever section | 3 / 4 / 5 wide, 1.2 and 1.8 thick — lever type **C**, insulated, L options 10 / 12.5 / 15 / 18 / 20 / 25 |
| body height / pins below | **6.5 / 3.4** |
| terminals | 3 × Ø1.2 and 4 × Ø1.7; spacings **62.8 / 65.2 / 73.5**, offsets 3.5 / 8.5 |
| circuit | 3-terminal, wiper = 2 |
| electrical | 1 KΩ–1 MΩ, taper A/B/C/W, linear 0.1 W, **10,000 cycle** life, operating force **10–200 gf·cm**, detent option "0 or 1 centre click" |

**Cross-check against the STEP:** the drawing's outer terminal spacing is 73.5;
the model's outer lugs sit at X = −36.83 and +36.97 → **73.80** `[STL]`. Same part.

**One caveat, stated rather than buried:** the datasheet's §2.1 *Slide travel*
line reads **"T = (15, 20, 30) ± 1.0 mm"** — that is the **series** specification
sheet, covering the maker's whole slide-pot family, and it is inconsistent with
its own outline drawing for this part. **The outline drawing governs**: it is the
drawing for the part Adafruit link it against, its scale verifies, and 60 mm
travel in a 75 mm body with a 71 mm slot is the standard "60 mm fader" geometry.

### The layout consequence, immediately

```
swept opening needed = travel + shank width = 60.000 + 5.141 = 65.141 mm
draw the acrylic slot 66.00 long                                     [DERIVED]
```

`layouts/README.md` carries a **50.80 mm placeholder**. The drawn slot has to
grow from 50.80 to 66.00 — **15.20 mm longer** — and the fader band grows with it. That is the layout thread's to
absorb, and it should be told today.

### A new open item, and it is on the one part that was already marginal

**The drawing says the lever stands 15 mm proud of the body. Adafruit's STEP
draws 9.50.** The 15 is measured: the drawing's lever-tip extension line and its
body-top line are 119 px apart = **14.9 mm** at the verified 8.000 px/mm. And
Adafruit's own 4219 page lists *"Height: 6.5mm"* and *"Shank length: 15mm"* as
two separate specs, which agrees with the drawing, not with the model.

The maker offers lever **L = 10 / 12.5 / 15 / 18 / 20 / 25** `[DS 4219 p3]`, so
the most likely explanation is that the **5295 is fitted with a shorter lever
(L ≈ 10) than the bare 4219**. It is also possible Adafruit's CAD is simply wrong.

**Design to 9.50 and be pleased if it is 15.** `stack.md` §2.4's whole
fader-in-a-window argument is built on 9.00 of shank; if the real part has 15, the
fader problem evaporates in the same way the encoder's just did. **Caliper it the
moment the board arrives** — §6 has the procedure.

## 4.3 The 2 × 8 header — position **MEASURED**, height **BOUNDED**

`wiring.md` §8 items 10 and 11 list the header's XY position and body height as
unsourced. The position can be recovered from the vendor's own V2 back view,
because that view carries the board's full outline as a dimension and the header
is visible through the back-shell cutout.

**Method:** the board silhouette is 471 px wide against its dimensioned
**92.50 mm**, and 357 px tall against **70.10 mm** — **5.092 and 5.090 px/mm**,
two independent scales agreeing to 0.04 %. The header's own body measures 115 ×
32 px.

```
header body        115 px / 5.0919 = 22.59 mm long
                    32 px / 5.0919 =  6.30 mm wide                 [MEASURED]
```

**That is the self-check that makes the measurement trustworthy:** a standard
2 × 8 2.54 mm socket is 8 × 2.54 = 20.32 mm of pin span plus two end walls —
**~22.6 mm** — and two rows at 2.54 plus walls is ~6.3 mm. The scale reproduces a
part whose true size is known from its pitch.

Position, in the frame of the vendor's V2 back view (battery compartment at the
bottom, the 3.50 mm screw offset at the right):

| | mm |
|---|---|
| board left edge → header left end | **35.28** |
| header body length | **22.59** |
| header right end → board right edge | **34.65** |
| board top edge → header top | **32.58** |
| header body width | **6.30** |
| header bottom → board bottom edge | **31.22** |

Both chains close on the dimensioned outline: 35.28 + 22.59 + 34.65 = **92.52**
against 92.50, and 32.58 + 6.30 + 31.22 = **70.10** exactly. `[MEASURED]`

> ### **The header is centred on the board: 0.31 mm off centre in X, 0.68 mm in Y.**

For the carrier plate that is a usable answer today — put the mating male header
on the board's centreline and the error is under a millimetre, which the 2.54 mm
pitch and the socket's own lead-in swallow. It also matches `wiring.md` §1.1's
"centre of the board, immediately above the battery holder".

**Body height: still not published, but now bounded.** §4.1 establishes that
everything behind the PCB's back face occupies **7.00 mm** `[VENDOR W5]`, and the
header appears flush in the shell cutout. So **the socket body is ≤ 7.00 mm tall**
and, less the shell wall, realistically 5.5–7.0. That is enough to say the
carrier's male header and its standoff must clear about 7 mm, and not enough to
draw it. §6.

**Read this as a drawing measurement, not a vendor dimension.** It is a photograph
with a little perspective, dimensioned by the vendor on the board outline only.
Treat it as **±0.5 mm**, and caliper it before the carrier's header pocket is
final.

---

# 5. Edge-lighting the slab — one paragraph, as asked

Light injected into the cut edge of a 12.70 mm slab travels by total internal
reflection and **only leaves where the surface is disturbed**, so an undisturbed
polished slab over a reflective LCD will light its own edges beautifully and put
essentially nothing on the display — which is the failure mode to expect, not a
subtle one. Making it work means laser-etching an extraction pattern into the
slab's **underside** over the display, which is exactly how a front-lit e-reader
works (a light guide with engineered extraction features over a reflective panel),
and the slab is already being laser-cut so the pattern costs one more raster pass
on the same job. **The one thing that decides it: whether the extraction pattern
can be made fine enough to disappear in daylight.** Etch too coarse and you have
permanently frosted a window over the screen and lost the daylight contrast that
is the entire reason for choosing a reflective panel; etch too fine and the LEDs
cannot push enough light out of it to be useful at night. That is a
pattern-density experiment on a scrap coupon from the same sheet — cheap, and
answerable before anything else about the idea is worth deciding. Two secondary
facts for whenever it is picked up: a real front light is a ~0.5 mm guide a
fraction of a millimetre off the panel, not a 12.70 mm slab standing 13.7 mm
above it, so the light will arrive at shallow, uneven angles; and a bidirectional
edge feed (LEDs on two opposite edges) is what keeps a guide of this length even.
Not designing it here.

---

# 6. NOT SOURCED

Stated loudly, per the project's rule that an unsourced number is a bug.

**Blocking — these change the design if they come back wrong:**

1. **Waveshare's PCB thickness.** §4.1's **5.53 mm** display height is
   `14.10 − 7.00 − PCB`, so it moves 1:1 with this number. 1.57 mm is the
   industry default and what every Adafruit breakout in `parts.md` §1 measures,
   but **Waveshare publish neither it nor the 7.00's endpoints.**
   *Measure:* calipers on a bare corner of the PCB, away from any component.
   Then, directly: **lay a straightedge across the board's front and measure from
   the PCB's front face to the display's outer acrylic face.** That one
   measurement replaces the whole derivation and takes thirty seconds.

2. **What the drawing's 7.00 actually spans.** Waveshare label the 4.35 and the
   14.1 in words and leave the 7.00 unlabelled `[VENDOR W5]`. The reading here —
   PCB back face to shell outer face — is forced by the fact that **both V1 and V2
   close on the same 2.75 middle term**, which is as strong as circumstantial
   evidence gets, but it is not the vendor saying it.
   *Measure:* depth gauge from the shell's back face to the PCB's back face.

3. **NeoSlider lever height: 15 mm (drawing) vs 9.50 mm (Adafruit STEP).** §4.2.
   *Measure:* calipers from the pot's cover **rim** (not its recessed face — they
   are 0.50 mm apart, §4.2) to the shank tip, with the nubbin off. Then from the
   PCB's front face to the shank tip, which should read 15.50 if the model is
   right and ~21.5 if the drawing is.

4. **NeoSlider travel, confirmed on hardware.** The 60 mm is from the
   manufacturer's outline drawing `[DS 4219]` and the drawing's own scale
   verifies, but the same datasheet's series page says "T = (15, 20, 30)".
   *Measure:* push the shank to one stop, mark the cover, push to the other,
   measure between the marks. Thirty seconds, and it closes the layout's
   longest-running open item for good.

5. **The printed D-bore clearance on a Ø6 shaft.** The only figures found are one
   vendor's blog contradicting itself at 0.10–0.15 and 0.15–0.25 mm per side
   `[3P qidi]`. **Not a specification.** §3.2's four-bore coupon is the answer and
   costs one 15-minute print.

6. **Knob bore depths of every bought knob.** Carried forward unchanged from
   `stack.md` §7.2: Adafruit 5527/5528/5530, Elma K1 and Adafruit 2056 all publish
   height and bore *diameter* and **none publishes bore depth**. It matters here
   only if §3.4's metal-hub route is taken.

7. **The 2 × 8 socket's body height.** Bounded at **≤ 7.00 mm** by §4.1, not
   measured. *Measure:* calipers from the PCB's back face to the socket's top
   face, with the shell off.

**Secondary — flagged, not blocking:**

8. **Index-finger breadth**, for sizing the annulus as finger room. DIN 33402-2 is
   paywalled; the figures that surfaced are a student handout with an unnamed
   source (≈16 / 18 / 20 mm at the 5th / 50th / 95th percentile). **Not used
   anywhere in this document** — §2.2 rests on MIL-STD-1472H figure 57's Ø32 mm,
   which is primary and decides the question on its own.

9. **The Bourns PTV09A F-type shaft's *flat length*.** The datasheet dimensions the
   flat **across** (4.5 on a Ø6.0 shaft) but I could not read a flat *length* from
   it. §1.4's grub screw sits at Z = 17.500, which is **16.15 mm above the pot's
   mounting surface** on a 20 mm shaft — so the flat must run at least that far
   down from the tip. On most 9 mm pots it runs the full exposed length.
   **Confirm before ordering.**

10. **PTV09A-6 bushing length.** Carried forward from `stack.md` §7.5: the drawing
    carries both a 5.0 and a 5.9 in the bushing region. 5.0 is used; §1.4's
    1.270 mm boss works at either value.

11. **Laser hole tolerance and kerf taper at 12.70 mm, measured.** Carried forward
    from `stack.md` §7.15 and §7.16. §2.1's bore budget uses SendCutSend's blanket
    ±0.23 mm and a 1–2° community taper figure. **Add a Ø22.00 bore to the test
    coupon in `stack.md` §6.3** and measure both faces — it is one more contour on
    a job you are already paying the minimum for.

12. **Whether the knob body eclipses the 5880's NeoPixel in practice.** §2.4 is
    geometry from a measured LED position `[STEP 4991]` and an assumed Lambertian
    emission. *Check:* power one 5880 on the bench with the printed knob fitted,
    in a dark room, before committing six bores.

13. **Smooth-On Crystal Clear 200's clarity *through 19 mm*.** The *"water white"*
    claim and the 0.1 % shrinkage are the vendor's `[SO cc200]`; no transmittance
    figure or path-length data is published, and the knob is a 19 mm path. If the
    cast knob is meant to light from below (§2.4) that matters.

---

# 7. Documents used

- **MIL-STD-1472H**, *Department of Defense Design Criteria Standard: Human
  Engineering* — `[MIL]` — figure 10 (knobs), figure 11 (ganged knobs),
  figure 14 (push buttons), figure 57 (arm and hand access), table I, table III,
  and clauses 5.1.1.7.4, 5.1.4.1.2.1.4–.7, 5.1.4.2.1.1.2, 5.7.9.3.1–.5 —
  https://cvgstrategy.com/wp-content/uploads/2023/04/MIL-STD-1472G.pdf
  *(the file is named …1472G; its running header reads MIL-STD-1472H, as
  `precedents.md` §6 already records)*
- Slide potentiometer series specification and outline drawing — `[DS 4219]` —
  https://cdn-shop.adafruit.com/product-files/4219/4219_C11375.pdf
  *(linked from https://www.adafruit.com/product/4219 as its datasheet)*
- Adafruit **4219** product page (Overall length 75 mm · Width 9.5 · Height 6.5 ·
  Shank length 15 · Shank width 4 · Nub 11.2 × 7 × 9.3 · 10 KΩ) — `[AF]` —
  https://www.adafruit.com/product/4219
- Adafruit **5295** NeoSlider product page — `[AF]` —
  https://www.adafruit.com/product/5295
- Waveshare **ESP32-S3-RLCD-4.2** wiki, V1/V2 revision notes and Dimensions —
  `[VENDOR W1]`, `[VENDOR W5]` — https://docs.waveshare.com/ESP32-S3-RLCD-4.2
  and `assets/images/ESP32-S3-RLCD-4.2-ProductSize-V2-*.webp`
- Bourns **PEC11** 12 mm incremental encoder — `[DS pec11]` — local:
  `v2/parts/models/datasheets/bourns-PEC11-12mm-encoder.pdf`
- Bourns **PTV09** 9 mm potentiometer — `[DS ptv09]` — local:
  `v2/parts/models/datasheets/bourns-PTV09-9mm-pot.pdf`
- Smooth-On **Crystal Clear 200** technical data — `[SO cc200]` —
  https://www.smooth-on.com/products/crystal-clear-200/
- Formlabs, **Silicone mold making and silicone part production** — `[FL]` —
  https://formlabs.com/support/Silicone-mold-making-and-silicone-part-production/
- MMA → PMMA polymerisation shrinkage and casting exotherm — `[3P mma-shrinkage]`
  — US 10,968,296 (air-void control in carbonyl-monomer polymerisation, exotherm
  >100 °C → voids, worse in thick sections) and US 5,530,041 (20–22 % for pure
  MMA; prepolymer syrup practice) via https://image-ppubs.uspto.gov/
- Printed knob bore clearances — `[3P qidi]` — *vendor blog, self-contradictory,
  flagged in §6* —
  https://qidi3d.com/blogs/print-lab/3d-printed-knob-fit-problems-solutions and
  https://qidi3d.com/blogs/print-lab/design-print-replacement-knobs
- Adafruit **5527 / 5528 / 5530** knobs, **5621** nubbin — `[AF]` —
  adafruit.com/product/5527, /5621
- `[RÖHM gsxt]`, `[RÖHM kb]`, `[RÖHM mach]`, `[TROTEC]`, `[SVC sendcutsend]`,
  `[SVC protolabs]`, `[3P taper]`, `[3P fab-guides]`, `[DS elma-k1]`,
  `[DIST rs-pro]`, `[DS RD901F]` — all as cited, quoted and URL-listed in
  `v2/parts/stack.md` §6.4 and `v2/parts/acrylic.md` §8.4.

### Measurement method, for reproducibility

The STL work in §4.2 is binary-STL parsing in NumPy: vertex de-duplication at
1e-4 mm, union-find for connected components, plane-triangle intersection for
cross-sections, and barycentric rasterisation at 0.1 mm for the cover-face
coverage. The drawing work in §4.1–§4.3 is `pdftoppm` / `convert` to greyscale
raster, then run-length measurement of dimension lines and silhouettes, with the
scale established from two or more of the drawing's own dimensioned features and
cross-checked against a third. Scripts were throwaway and are not committed; the
numbers and the method are both written out above so that any of it can be redone
from the same public files.

---

## 8. One-line summary

**The knob goes down the bore, so the stock 5880 works and nothing gets
hand-soldered · acrylic underside stays at 6.350 because the display is only
5.53 tall · Ø18 body in a Ø22 bore under a Ø25.40 brim, 6.350 proud, 19.050 tall ·
no finger enters the bore, so the brim is the whole grip · 38.10 pitch breaks
MIL's 25 mm separation and 50.80 fixes it · D-shafts everywhere, never splines ·
print now, cast clear later so the NeoPixel lights the knob · NeoSlider travel is
60 mm and the fader slot is 15 mm short.**
