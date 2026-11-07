# The two boards the asset library does not have

*A CAD input. Every other part in the v2 bill of materials exists as a .step in
`/home/user/kylebsmith/cmtl-3d-assets`. Two do not, and both have to be mounted
in this enclosure: **Adafruit 5710**, the Feather RP2040 with DVI output, and
the **Waveshare ESP32-S3-RLCD-4.2**, the deck's main board.*

*This document is the provenance for `v2/cad/parts/feather_rp2040_dvi.scad` and
`v2/cad/parts/waveshare_rlcd42.scad`. Both vendors publish a CAD model; both
models were downloaded and **measured** with OpenCASCADE rather than read off a
drawing, because measured and quoted are different things and this repository
tags them differently. Where a measurement disagrees with something already
written down in `v2/`, section 3 says so by name.*

*Anything that could not be traced to a primary source is in section 5, with
the procedure for closing it. Nothing is estimated.*

---

## 0. Verdict up front

1. **Both boards' vendor CAD is a B-rep assembly with named parts, and both
   were measured from the B-rep, not from a mesh and not from a drawing.**
   Adafruit's 5710 STEP carries 60 named leaf shapes; Waveshare's carries 859.
   Bounding boxes, cylindrical-face axes and radii, planar-face areas and
   outlines, and plane sections were all computed directly. Section 4 gives the
   method so it can be re-run.

2. **The 5710's four mounting holes are a trapezoid AND two different
   diameters.** `v2/parts/hdmi-feather.md` section 2.2 already found the
   trapezoid from the faceted STL. The B-rep adds the second half of the trap:
   the USB-end pair is **Ø2.500** and the HDMI-end pair is **Ø2.540**. One
   pattern, four holes, two drills. Section 1.3.

3. **The 5710's corner radius is 2.540, not 1.90.** The STL scan in
   `hdmi-feather.md` section 2.1 gave "≈1.90 (0.075 in)". The B-rep carries
   four R2.540 cylindrical faces centred 2.540 in from both edges — 0.100 in,
   the Feather standard's own corner — and the two USB-end mounting holes are
   **concentric with their corner arcs**. Section 1.2.

4. **The 5710's battery socket is SIDE ENTRY, not top entry.**
   `hdmi-feather.md` section 2.5 reads it as top-entry from a photograph and
   builds part of section 5.2's argument on that. The geometry says otherwise,
   three independent ways, and Adafruit's own render agrees. It opens at
   **y = 22.453, facing the long edge that carries the 12-pin row** — the cable
   leaves flat, across the board, not up into the acrylic. Section 1.5.

5. **The STEMMA QT on the 5710 is top entry**, despite Adafruit's EAGLE package
   being named `JST_SH4_RA`. Same three tests, opposite answer. Section 1.5.

6. **The Waveshare 2×8 header's position is now sourced, to the hole.** The
   task that produced this document listed it as the board's one missing
   dimension. It is measured three ways from the same solid — the moulded
   housing's box, its nine internal partition walls, and the sixteen
   1.600 × 1.600 holes in its top face — and all three agree. **Housing centre
   (U 46.149, V 33.852); pin 1 at (U 55.039, V 32.582); columns every 2.540
   running down in U; rows 2.540 apart.** Section 2.6.

7. **The Waveshare header's far face is 8.700 mm behind the PCB's rear face**
   (8.702 counting the leads' plating), which is what sets the carrier's
   standoff under that board — before a single mating pin.
   `cad/parameters.scad` carries exactly 8.700 for this from the *other*
   Waveshare package, by a different route. Section 2.6.

8. **The Waveshare's display stands 2.750 mm above its PCB's front face, not
   5.53.** `v2/parts/knobs.md` section 1.1 derives 5.53 by assigning the
   vendor's 4.35 / 7.00 / 2.75 thickness chain in the wrong order. Measured
   against the solids, the chain is
   **4.350 (display front → PCB rear) · 7.000 (PCB rear → base front) · 2.750
   (the base)**, totalling exactly 14.100, and the middle 7.000 is *literally
   the 7 mm standoff*. The error is 2.780 mm and it is in the deck's favour —
   **4.050 mm of clearance under the acrylic instead of 1.270** — but it should
   not stay in the repository. Section 2.4 and section 3.2.

9. **The Waveshare's display assembly is a 2.000 mm acrylic spacer plus a
   0.750 mm glass**, and that is why Waveshare's V1 → V2 note reads the way it
   does: 3.750 → 4.350 is 0.600, which is the spacer going 1.400 → 2.000 and
   nothing else moving. Section 2.4.

10. **The LCD active area was measured as a single planar face, not inferred
    from margins.** 84.802 × 63.603 at **U 2.248 … 87.050, V 2.750 … 66.353**,
    sitting at W = +4.325. It is centred in V and **offset 1.601 mm toward
    U = 0 in U**. The acrylic goes over this with no cutout, so this is the
    rectangle the slab has to line up with — not the PCB centre, not the TFT
    module. `cad/parameters.scad`'s `display_off_x = -1.60` is confirmed to
    0.001 mm. Section 2.5.

11. **The Waveshare PCB is 1.600 mm thick and square-cornered.** Neither is
    published in words. The B-rep's board core runs exactly 1.600 between its
    two faces, and the outline carries **no corner arcs at all** — which
    independently corroborates the caliper finding in `docs/DATUMS.md` C-34
    that the real board is square, against the drawing's R0.5. Section 2.2.

12. **There is a fifth hole in the Waveshare PCB that nothing in this
    repository had recorded**: a single **Ø2.400** through-hole at
    **(U 10.410, V 7.050)**. Purpose unknown — section 5. It is modelled,
    because a carrier boss landing on it would be a silent fit failure.

13. **Both models render.** Every `part=` view of both files produces a
    non-empty mesh under OpenSCAD 2021.01. Section 4.3 gives the facet counts.

---

## 0.1 Sources

| ID | Source | How read |
|----|--------|----------|
| A1 | **Adafruit_CAD_Parts**, `5710 Feather RP2040 DVI/5710 Feather RP2040 DVI.step` — 4,316,166 bytes, STEP assembly, **60 named leaf shapes** | already cloned on this machine at `/home/user/adafruit/adafruit_cad_parts`; copied to `v2/parts/models/` here and measured with OCP 8.0.1.1 |
| A2 | the same folder's `5710 Feather RP2040 DVI.jpg` — Adafruit's own render of the populated board | read as an image, and again at 4× on the two button legends |
| A3 | the same folder's `5710 Feather RP2040 DVI.stl` — the faceted model `hdmi-feather.md` was measured from | copied to `v2/parts/models/` so the two can be compared |
| W6 | **Waveshare** `ESP32-S3-RLCD-4.2-20261010.stp` — 24,669,342 bytes, STEP assembly, **859 leaf shapes**, vendor revision date 2026-10-10 in the filename | obtained from `files.waveshare.com` in an earlier pass of this project and held in the session scratchpad; copied to `v2/parts/models/waveshare-rlcd42/` here and measured with OCP |
| W7 | **Waveshare** `ESP32-S3-RLCD-4.2-20261010.dxf` — AC1024, **24 DIMENSION entities** | read with `ezdxf`: each dimension's measurement *and* both definition points, so the margins can be assigned to the right edge instead of guessed off a picture. Note `$INSUNITS = 1` in the header, which nominally means inches and is plainly wrong — every measurement in the file is a millimetre value matching the PDF |
| W8 | **Waveshare** `ESP32-S3-RLCD-4.2_20261010.pdf` — the same drawing | rendered at 200 and 600 dpi and read as an image, to see which view each DIMENSION belongs to |
| REPO | this repository: `v2/parts/{hdmi-feather,wiring,knobs,stack}.md`, `cad/parameters.scad`, `cad/lib/components.scad`, `.gitignore` | read in place |

Tags used in this document and in both `.scad` files:
`[step]` measured from a vendor STEP by B-rep query ·
`[dxf]` read from a DIMENSION entity in a vendor drawing file ·
`[vendor]` published by the vendor in words ·
`[silk]` read off a vendor render of the board ·
`[derived]` arithmetic on a tagged value, shown ·
`[spec]` the published Feather mechanical standard ·
`[repo]` read from this repository ·
`[UNSOURCED]` not established — section 5.

**Nothing vendor is redistributed.** `.gitignore` line 52 covers
`v2/parts/models/`, so both STEPs, the DXF and the PDF sit beside the repo and
outside it, exactly as `docs/PROVENANCE.md` requires for the Waveshare files.

---

## 1. Adafruit 5710 — Feather RP2040 with DVI Output Port

### 1.1 The frame

`v2/cad/parts/feather_rp2040_dvi.scad` uses **Adafruit's own model frame,
unrotated and unmirrored**, so every number below can be checked against the
STEP directly:

```
x   0.000 ... 50.800    long axis. x = 0 is the USB-C end,
                        x = 50.800 is the HDMI end.
y   0.000 ... 22.860    short axis. y = 1.270 is the 16-pin row,
                        y = 21.654 is the 12-pin row.
z   0.000               the PCB's BOTTOM face. +z up through the components.
                        PCB top face = 1.570.
```

Right-handed. **The HDMI receptacle is the only thing below z = 0.**

### 1.2 The PCB

| | mm | source |
|---|---|---|
| Outline | **50.800 × 22.860** | `[step]` board solid, exactly; = 2.000 × 0.900 in |
| Thickness | **1.570** | `[step]` board solid, z 0.000 … 1.570 |
| Corner radius | **2.540** | `[step]` four R2.540 cylindrical faces, centres at (2.540\|48.260, 2.540\|20.320) |
| Vendor's quoted size | 53.0 × 23.3 × 7.7 | `[vendor, via REPO hdmi-feather.md §1.1]` |

**The corner radius is 0.100 in, not 0.075 in.** This corrects
`hdmi-feather.md` section 2.1's "≈1.90", which came from scanning the faceted
STL's edge. The consequence is small but real: at R2.540 the **USB-end mounting
holes are concentric with their own corner arcs**, which is the Feather
standard's arrangement, and the HDMI-end holes are *not*, which is the whole
point of section 1.3.

**The vendor's 53.0 is not the PCB.** 50.800 of board, plus 1.182 of USB-C
overhang at x = 0 and 0.886 of HDMI overhang at x = 50.800, gives
**52.868 mm overall** `[derived]`. Nothing overhangs in y: the board is 22.860
and the tallest thing near the edge, the battery socket, stops at y = 22.453.

### 1.3 Mounting — the trapezoid, and the second drill

All four holes, measured as cylindrical faces through the full board thickness
`[step]`:

| Hole | x | y | Ø |
|---|---|---|---|
| USB end, lower | **2.540** | **2.540** | **2.500** |
| USB end, upper | **2.540** | **20.320** | **2.500** |
| HDMI end, lower | **48.260** | **1.905** | **2.540** |
| HDMI end, upper | **48.260** | **20.955** | **2.540** |

```
column pitch   48.260 - 2.540  =  45.720  =  1.800 in    [derived]  spec
row pitch, USB end                17.780  =  0.700 in    [derived]  spec
row pitch, HDMI end               19.050  =  0.750 in    [derived]  NOT spec
diameter difference               0.040                  [derived]  NOT spec
```

`hdmi-feather.md` section 2.2 found the trapezoid and quoted all four at
Ø2.50 — which is what a 0.05 mm grid scan of a mesh will give you. The B-rep
carries two different radii. **It does not change the fastener**:
`v2/parts/stack.md` section 5.4's ruling stands, because an M2.5 clearance hole
is Ø2.7–2.9 and will not pass Ø2.540 either. It does mean a carrier that
patterns four identical bosses from one corner is wrong twice over — once in
position, once in bore.

`feather_rp2040_dvi.scad` exposes this as `fw_mount_points()`, which returns
`[x, y, diameter]` per hole, so a carrier cannot pattern it by mistake.

### 1.4 The video connector

Molex **47151-0001**, right-angle HDMI Type A, designator `X2` `[step]` name,
`[REPO hdmi-feather.md §1.2]` for the part number. Two envelopes, because they
do different jobs:

```
GROSS BOX - what a pocket must clear                           [step]
  x  39.524 ... 51.686     12.162, of which 0.886 is PAST the PCB edge
  y   3.905 ... 18.918     15.013
  z  -0.636 ...  7.789      8.425      6.219 above the PCB top face
                                       0.636 below the PCB bottom face

CLEAN SHELL - what a wall aperture must pass                   [step]
  y   3.912 ... 18.912     15.000      centre 11.412
  z   2.205 ...  7.755      5.550      at the mouth, x >= 49.5
  wall thickness            0.450      [derived] from the mouth section's
                                       18.027 mm2 round a 15.000 x 5.550 ring
```

- The shell's y centre is **11.412** against a board centre of 11.430, so it
  sits **0.018 toward y = 0** `[derived]`. `hdmi-feather.md` section 2.4 says
  the same to the same precision.
- **The shell at the mouth is 5.550 tall, not 8.385.** The 8.385 includes the
  through-hole shield tabs that reach below the PCB; they never pass a wall.
  A slot cut to the gross box is 2.8 mm taller than it needs to be.
  `fw_hdmi_shell()` returns the five numbers a wall needs and nothing else.
- The B-rep is **0.034 taller and 0.006 longer** than the STL
  `hdmi-feather.md` measured (7.789 against 7.755; 51.686 against 51.680).
  That matters in one place, section 3.1 item 5.

### 1.5 The two plug-in sockets, and which way they open

This is where this document disagrees with `hdmi-feather.md` section 2.5, so
here is the whole argument.

**Battery, JST PH 2-pin (`X1`): SIDE ENTRY, mouth at y = 22.453, facing +y.**

```
housing    x  6.972 ... 14.872      7.900                      [step]
           y 14.853 ... 22.453      7.600   mouth at y = 22.453,
                                            0.407 inside the PCB edge
           z  1.570 ...  6.370      4.800 above the PCB top face
solder tails reach back to y = 13.853, giving an 8.600 footprint in y
```

Three independent tests, all pointing the same way `[step]`:

| test | result |
|---|---|
| section at y = 22.146, just inside the +y face | area **10.100 mm²** — a thin ring, i.e. an open mouth |
| sections at y = 16.617 and across the mid-body | **24.751** and 13–15 mm² — a solid wall at the −y end, hollow between |
| the contacts themselves | each runs **8.200 mm along y**, with its blade at z 4.170 … 4.920 and its solder tail bent down to the pad at z 1.570 … 2.320, over a 0.750 radius at (9.922, 16.703) |

A top-entry PH has contacts that go straight down into the board and a housing
closed on all four sides. This one has neither. **Adafruit's own render agrees**
`[A2]`: the socket's two blades are visible in a mouth facing the long edge.

**STEMMA QT, JST SH 4-pin (`CONN1`): TOP ENTRY, mouth at z = 5.870.**

```
housing    x 30.478 ... 33.378      2.900                      [step]
           y 13.095 ... 19.095      6.000
           z  1.620 ...  5.870      4.250   4.300 above the PCB top face
SMT tails run on to x = 34.078
contacts at y 14.595, 15.595, 16.595, 17.595  -  1.000 pitch
```

Same three tests `[step]`: the z = 5.655 section is a **6.418 mm² ring** (a
0.4 mm wall round a 2.900 × 6.000 outline); both x-end sections are
**21.590** and **22.984 mm²**, i.e. solid walls; and the four contacts lie flat
on the board. **Adafruit's EAGLE package is called `JST_SH4_RA`** and that name
is almost certainly what put "right angle" into circulation — but the geometry
in Adafruit's own CAD is not right-angled.

**What this changes.** `hdmi-feather.md` section 2.5 concludes *"Nothing can be
plugged into the top of this board once the acrylic is on."* That is still true
of the STEMMA QT. It is **not** true of the battery: a JST PH plug on this
board leaves sideways, across the long edge, inside the 6.350 mm of air between
the wiring plane and the acrylic underside — if the plug's own body fits, which
is section 5 item 3. That makes section 4.4's option 1 (give the Feather its
own cell) cheaper than that document thinks.

### 1.6 Heights, and the two numbers a pocket needs

All referenced to the PCB's bottom face at z = 0 `[step]`:

| Item | z range | above PCB top (1.570) | below PCB bottom |
|---|---|---|---|
| **HDMI receptacle** | **−0.636 … 7.789** | **+6.219** | **0.636** |
| Battery JST PH | 1.570 … 6.370 | +4.800 | 0 |
| STEMMA QT | 1.620 … 5.870 | +4.300 | 0 |
| USB-C, mid-mount | 0.564 … 4.776 | +3.206 | 0 |
| Both tactile buttons | 1.570 … 3.470 | +1.900 | 0 |
| RP2040, QFN-56 7.000 × 7.000 | 1.570 … 2.570 | +1.000 | 0 |
| 8 MB QSPI flash, USON8 | 1.570 … 2.370 | +0.800 | 0 |
| **Whole board** | **−0.636 … 7.789** | **+6.219** | **0.636** |

**The USB-C is mid-mount and still does not go below the board.** Its lowest
point is z = 0.564, which is inside the PCB, 0.564 above the bottom face. Only
the HDMI's shield tabs break the plane.

Port positions, for the walls `[step]`:

| | x | y | notes |
|---|---|---|---|
| **USB-C** `X6` | −1.182 … 6.730 | 6.954 … 15.906 | protrudes **1.182** past x = 0; y centre **11.430 = the PCB centre exactly** `[derived]`; shell at the mouth 8.940 × 3.210 |
| **HDMI** `X2` | 39.524 … 51.686 | 3.905 … 18.918 | section 1.4 |

### 1.7 The header rows, the two extra pads, and one departure from the spec

```
16-pin row (JP1)   y = 1.270     x 6.350 ... 44.450, 2.540 pitch     [step]
12-pin row (JP3)   y = 21.654    x 16.510 ... 44.450, 2.540 pitch    [step]
drill, both rows   O1.000                                            [step]
row separation     21.654 - 1.270  =  20.384                         [derived]
the Feather spec   0.800 in       =  20.320                          [spec]
```

**The 12-pin row is 0.064 mm off the published 0.800 in spacing.** The faceted
STL reads 21.590 — the spec value — and that is what `hdmi-feather.md`
section 2.7 carries. The B-rep reads 21.654 at every one of the twelve holes.
0.064 mm is nothing to a pocket; it is not nothing to anyone laying out a
mating board against the spec's *"Don't change the GPIO spacing or location"*,
so the measured value is what the model carries and this is why.

Two further Ø1.000 plated holes, which are the `C` (CEC) and `U` (UTILITY)
breakout pads `[step]`, `[REPO hdmi-feather.md §1.2]`:
**(36.830, 5.080)** and **(36.830, 7.620)**.

**No castellations.** `hdmi-feather.md` section 2.6 scanned the mesh outline
and found none; the B-rep's outline is four straight edges and four R2.540
arcs, which says the same thing more directly.

**The headers are not in the model and the board ships them loose.** The model
has `fit_headers = false` for that reason, and the two numbers it would need —
the insulator height and the male tail length below the PCB — are `[UNSOURCED]`
(section 5 item 2). That tail length is the number that decides whether the
board clears `stack.md`'s −3.000 channel floor.

### 1.8 Silkscreen

Read off Adafruit's own render `[A2]`, identical to the list in
`hdmi-feather.md` section 2.7:

```
JP1, 16 holes, pin 1 at the HDMI end:
    Rst  3.3V  GND  A0  A1  A2  A3  24  25  SCK  MO  MI  RX  TX  HPD
JP3, 12 holes, pin 1 at the USB end:
    Bat  EN  USB  13  12  11  10  9  6  5  SCL  SDA
pads:  C   U
board: RP2040 / Feather / DVI, three lines beside the QFN
```

**Fifteen labels over sixteen holes.** Pins 14 and 15 are both +3V3 and only
one is printed. The model draws the legends as they are printed, including that
asymmetry, so a render of the board under acrylic matches the real thing. **Do
not count silk to find a pin; count holes.**

The two buttons are legended `Rst` and `Boot`, with the word running up-left
from each button and partly hidden behind the button body in the render. The
fragments are unambiguous — `Rese…` on the one nearer the USB-C (x 7.225 …
11.825), `Boo…` on the other (x 13.575 … 18.175) — but it is a partly occluded
read, so it is also in section 5.

---

## 2. Waveshare ESP32-S3-RLCD-4.2

### 2.1 The frame, and why it is v1's and not the STEP's

`v2/cad/parts/waveshare_rlcd42.scad` uses the **U / V / W frame of v1
`cad/parameters.scad`**, so the two files stay checkable against each other:

```
x = U   0.000 ... 92.500    U = 92.500 is the USB-C / microSD / display-FPC edge
y = V   0.000 ... 69.100    V = 0 is the 18650 edge, V = 69.100 the button edge
z = W   0.000               the PCB's REAR (component) face. +W toward the
                            DISPLAY.  PCB front face = +1.600.
```

Waveshare's STEP runs its long axis the other way. The conversion is

```
U = 92.500 - x_step        V = y_step        W = 1.511 - z_step
```

**This is a double flip, so handedness is preserved.** That is not a detail.
Taking the STEP's own x with +z toward the display gives a **left-handed**
frame, and in a left-handed frame every back-view sentence in
`v2/parts/wiring.md` — *"with the board back-up and the battery holder at the
bottom … pin 1 at the USB-C end"*, the USB-C on the left — comes out mirrored.
The USB-C edge is `U = 92.500` precisely so that it reads correctly in a back
view, which is how the header is actually wired.

The 1.511 is the z of the PCB's **rear** face in the frame the measurement is
re-rooted on — the `PCB/BOARD` solid's own definition frame, which every other
leaf is brought into by its accumulated transform (section 4.2). In that frame
the board solid's bounding box runs −0.094 … 1.517, because it carries ~0.005
of mask on each face; its **core** runs exactly 1.600 between −0.089 and 1.511
`[step]`. The offset is worth no trust on its own; what justifies it is that
with it every derived figure in section 2 lands on a clean vendor value —
4.350, 7.000, 2.750, 14.100, 8.700, 15.200, 7.030, 85.500, 62.100, 84.800,
63.600 — and none of them was fitted to.

### 2.2 The PCB

| | mm | source |
|---|---|---|
| Outline | **92.500 × 69.100** | `[dxf]` `92.5 PCB OD`, `69.1 PCB OD`; `[step]` solid 92.530 × 69.131 including mask |
| Thickness | **1.600** | `[step]` board core, exactly, between its two faces |
| Corner radius | **0.000** | `[step]` the outline carries **no corner arcs**; `[repo]` `docs/DATUMS.md` C-34, a real board measured square |

**Waveshare do not publish the PCB thickness in words.** `knobs.md`
section 1.1 assumes the industry-standard 1.57 and says so. It is 1.600, and
the 0.030 matters once because section 2.4's chain is arithmetic on it.

**The corner radius is the second time this board's drawing has been wrong
about its own corners.** v1 recorded the drawing saying R0.5 and a physical
board measuring square. The B-rep outline has four straight edges and nothing
else. Two independent sources now agree against the drawing, so
`pcb_corner_r = 0.000` is a measurement, not a conservative guess.

**One more hole.** Besides the four mounting holes there is a single
**Ø2.400** through-hole at **(U 10.410, V 7.050)** `[step]`. It is not on the
drawing and it is in nothing this repository had written down. Purpose unknown
— section 5 item 5. It is modelled.

### 2.3 Mounting

```
four O4.200 through-holes                                        [step]
  (3.500, 3.500)  (3.500, 65.600)  (89.000, 3.500)  (89.000, 65.600)
  pitch          85.500 x 62.100           [dxf] "85.50", "62.10"
  inset          3.500 from every PCB edge [dxf] "3.50"
  fastener       4-M2.50                   [dxf] note on the back view
```

On each hole, on the **rear** face, a surface-mount **SMTSO-M2.5-7ET**:
Ø**5.500** body, female M2.5, standing **7.030** off the rear face `[step]`
against a 7.00 nominal part. The enclosure drives M2.5 screws up into these; it
supplies no bosses of its own. v1 `cad/parameters.scad` carries 85.50 / 62.10 /
5.50 / 7.00 from the other package; every one is confirmed here.

**That 7.030 is the same 7.00 that appears as the middle term of the thickness
chain**, which is the check that makes section 2.4 safe.

### 2.4 The thickness chain, and the 2.780 mm this corrects

Waveshare's drawing carries three dimensions down the edge view:
**4.35**, **7.00**, **2.75**, with **14.10** across all three. In the DXF they
are four DIMENSION entities on one line, and their definition points put them
in that order, front to back `[dxf]`:

```
93.214  --4.35--  97.564  --7.00--  104.564  --2.75--  107.314
        \_________________________ 14.10 __________________/
```

Measured against the solids, in the U/V/W frame, every term lands `[step]`:

| W | what is there | which term |
|---|---|---|
| **+4.350** | display assembly's front face | ← the 4.35 ends here |
| +3.600 | acrylic spacer's front face | |
| +1.600 | PCB front face = the spacer's back face | |
| **0.000** | **PCB REAR face** | ← the 4.35 is measured to here |
| **−7.000** | stand base's front face | the 7.00 — **and the standoff's end is −7.030** |
| **−9.750** | stand base's rear face | the 2.75 — the base is 2.750 thick |

```
4.350 - (-9.750)  =  14.100      [derived]  =  the drawing, exactly
```

**`v2/parts/knobs.md` section 1.1 assigns the same three numbers as
4.35 / 2.75 / 7.00** — "LCD glass + PCB" in the middle and the back shell at
the rear — and concludes that the **display stands 5.53 mm above the PCB's
front face**. It stands **2.750**:

```
display_front - pcb_t  =  4.350 - 1.600  =  2.750      [derived]
measured directly, front face to front face          2.745      [step]
```

The 0.005 is mask. The error in `knobs.md` is **2.780 mm**.

**It is in the deck's favour, which is exactly why it should be fixed.**
`knobs.md` puts the Waveshare PCB's front face at Z = −0.450 in the deck frame
and the acrylic underside at Z = +6.350, and reports 1.270 mm of clearance over
the display. The real figure is

```
display front   -0.450 + 2.750  =  +2.300
acrylic under                      +6.350
clearance                           4.050 mm          [derived]
```

A conclusion that survives a 2.78 mm correction is a conclusion that was not
being tested by its own arithmetic. `stack.md`'s A = 6.350 survives either way;
what changes is that there is now room to lower the acrylic or raise the board,
and that is a decision for whoever owns `stack.md`, not for this file.

### 2.5 The display, and where its active area actually is

The display assembly is **two layers**, both measured as planar faces `[step]`:

| layer | W | thickness | outline |
|---|---|---|---|
| acrylic spacer | 1.600 … 3.600 | **2.000** | U 0.048 … 92.350, V 0.050 … 69.053 |
| LCD glass | 3.600 … 4.350 | **0.750** | U 0.748 … 91.750, V 0.750 … 68.353 |

The glass outline is **91.002 × 67.603**, against the drawing's
`91.00±0.10 TFT` and `67.60±0.10 TFT` `[dxf]`, with margins 0.748 / 0.750 in U
and 0.750 / 0.747 in V — **centred on the PCB to within 0.003 on both axes**.

**And this is why Waveshare's own release note reads the way it does.** They
say *"the V2 version uses a different acrylic spacer, so the spacer thickness is
increased … all other parts remain unchanged"*, and their V1 drawing carries
3.75 where the V2 carries 4.35 `[REPO knobs.md §1.1]`. The difference is
**0.600**, which is this spacer going from 1.400 to 2.000 — and nothing else in
the stack moving, exactly as they say.

**The active area.** Not inferred from margins: it is a **single planar face of
5393.280 mm²** in the STEP `[step]`:

```
LCD AA     U  2.248 ... 87.050      84.802      [dxf] "84.80+/-0.10 LCD AA"
           V  2.750 ... 66.353      63.603      [dxf] "63.60+/-0.10 LCD AA"
           W                         4.325      0.025 below the glass's
                                                outer border at 4.350

margins    U   2.248 at U = 0,  5.450 at the USB-C edge
           V   2.750 and 2.747  -  centred
centre     U  44.649  against a PCB centre of 46.250
           -> offset 1.601 toward U = 0                      [derived]
```

v1 `cad/parameters.scad` has `display_off_x = -1.60` and `display_off_y = 0.00`,
derived from the drawing's margins. **Both are confirmed to 0.001 mm**, and
this time from a face rather than from reading a leader off a picture. The DXF
corroborates independently: its `84.80` dimension's definition points put the
AA at 2.250 from one edge and 5.460 from the other, and its `63.60` puts it at
2.753 / 2.757 — centred.

**Which edge gets the 2.248 is the part that is easy to get backwards**, and it
is settled by the face's own coordinates rather than by argument. For what it
is worth, the physics agrees: the display's FPC leaves the module at the
**U = 92.500 edge**, overhanging the PCB by 0.080 `[step]`, and a chip-on-glass
bond eats margin on the edge it bonds to. The larger margin is on the FPC side.

**The acrylic slab lines up with this rectangle**, not with the PCB and not
with the TFT module. `waveshare_rlcd42.scad` exposes it as
`ws_active_area()` for exactly that reason.

### 2.6 The 2 × 8 expansion header, P1

The one number the board model was written to close. `v2/parts/wiring.md`
section 1.2 has the complete pin map; it did not have a position.

```
moulded housing      U 35.649 ... 56.649     21.000              [step]
                     V 31.351 ... 36.352      5.001
                     W  -0.600 ... -8.700     8.100
with SMD leads       U 35.647 ... 56.650     21.003              [step]
                     V 30.550 ... 37.153      6.603
                     W  -0.099 ... -8.702     8.603
housing centre       U 46.149    V 33.852                        [derived]
far face             W = -8.702                                  [step]
```

Measured three independent ways, all agreeing `[step]`:

1. the moulded housing's bounding box;
2. its **nine internal partition walls**, at 2.540 pitch from x 36.191 to
   56.511 in the STEP frame — nine walls make eight cavities;
3. the **sixteen 1.600 × 1.600 holes** in its top face, extracted as the inner
   wires of that face. The top face's area is 64.040 mm² against a
   21.000 × 5.001 = 105.021 outline, so 40.981 mm² is missing — against
   16 × 1.600² = 40.960. The 0.021 is the housing's own corner reliefs.

**The pin grid:**

```
columns      U = 55.039, 52.499, 49.959, 47.419,
                 44.879, 42.339, 39.799, 37.259      2.540 pitch   [step]
rows         V = 32.582  (odd pins 1, 3, 5 ... 15)                 [step]
             V = 35.122  (even pins 2, 4, 6 ... 16)
hole         1.600 square                                          [step]
pin 1        U 55.039, V 32.582
```

Pin 1 is at the **USB-C end** on the **lower** row, which is what
`wiring.md` section 1.2 says — *"with the board back-up and the battery holder
at the bottom, the odd row is the lower one … pin 1 at the USB-C end"* — read
there off the schematic and confirmed against the silkscreen. In the U/V/W
frame the USB-C end is the high-U end and the battery edge is V = 0, so the
lower row is the lower V. The two agree, and `ws_p1_centre(n)` returns the
centre of pin *n* on that numbering.

**Two things this fixes for the carrier:**

- **The header's far face at W = −8.700 is what sets the standoff under this
  board**, before any mating pin. v1 `cad/parameters.scad` carries 8.700 for
  this from the *other* Waveshare package, by a different route (insulator
  8.500 + 0.100 lead + 0.100 seating). **The two agree exactly**, which is
  about as good as two independent reads of a vendor assembly get. The gross
  envelope including the leads' plating is 8.702.
- **What plugs in is not in anybody's model.** A mating male header plus a card
  is another ~10 mm below −8.702, and that is `ws_header_mate()`, which is
  drawn from a default and tagged as such — section 5 item 2.

### 2.7 The ports, and the two edges they are on

```
USB-C  (698B116-264-211, Type-C 16-pin)                          [step]
   U 84.988 ... 92.521   shell face 0.021 PROUD of the PCB edge
   V 29.754 ... 39.336   9.582 wide, V centre 34.545 vs a PCB centre of 34.550
   W  -3.251 ...  0.911  4.162 tall, mid-mount, centre W -1.170

microSD                                                          [step]
   U 76.461 ... 91.467   mouth stops 1.033 INSIDE the PCB edge
   V 45.649 ... 61.751   16.102 wide, V centre 53.700
   W  -1.852 ...  0.601  2.453 tall, mid-mount, centre W -0.625
```

Every one of these matches v1 `cad/parameters.scad` to the third decimal —
`usbc_body_w = 9.582`, `usbc_body_h = 4.163`, `usbc_w_centre = -1.170`,
`tf_body_w = 16.103`, `tf_body_h = 2.452`, `tf_w_centre = -0.625`,
`tf_off_y = 19.15` (V centre 53.70). That is a different vendor package,
measured by a different person, in a different year, agreeing to 0.001 mm. It
is the strongest cross-check in this document and it is worth saying out loud.

**The microSD mouth stops 1.033 inside the edge**, so a card has to be pushed
*past* the wall. Do not make that opening a tight slot.

On the **V = 69.100** edge `[step]`:

```
three SWITCH-TS24CA tact switches, body 4.553 (U) x 3.658 (V) x 2.203 (W)
   U 56.250  BOOT       U 46.250  PWR        U 36.250  KEY      [repo wiring.md 1.1]
   body's outer face at V 68.915  -  0.185 INSIDE the PCB edge
   W -1.802 ... 0.401   centre W -0.700

two MIC-4X3X1MM, 4.000 x 3.000 x 1.000
   U 78.750 and U 13.750,  V 65.600 ... 68.600,  W -1.000 ... 0.000
```

**The switch body stops 0.185 inside the PCB edge**, so an enclosure has to
supply a plunger; an open hole cannot reach the actuator. v1 spent a long
section of `cad/parameters.scad` learning that; it is true of this package too.

### 2.8 The deep things on the rear face

```
18650 holder     U  2.748 ... 89.748    87.000                   [step]
                 V  4.600 ... 25.703    21.103   V centre 15.152
                 W  0.000 ... -15.200   THE DEEPEST FEATURE

speaker SPK4020  U 26.249 ... 66.250,  V 30.552 ... 60.552
                 W -7.000 ... -3.500   3.500 tall, sits on the base

WROOM-1 can      U  7.345 ... 24.945,  V 39.150 ... 54.950
                 W -3.100 ... -0.800   2.300 tall

stand base       U -0.002 ... 92.500,  V -1.000 ... 69.103
                 W -9.750 ... -7.000   2.750 thick
```

**The 18650 holder reaches 15.200 behind the PCB's rear face, which is 5.450
deeper than the stand base it is supposed to sit behind.** It cannot be
contained; v1 built a whole cowl around that fact and `batt_off_y = -19.40`
(V centre 15.15) and `board_w_battery_end = -15.20` are both confirmed here
to 0.002.

**The base is 70.103 wide and the PCB is 69.100.** The drawing's `70.10` is the
base, overhanging the PCB by 1.000 **on the 18650 edge only** `[step]` `[dxf]`.
v1 flagged exactly this trap; it is confirmed, including which edge.

**One figure is not reconciled.** v1 carries `batt_bay_w = 77.800` for the
holder's length along U, caliper-confirmed on hardware, against **87.000** here.
The two are probably measuring different things — body against footprint
including the end contacts — and 87.000 is the conservative value in every role
it plays, so it is what the model draws. Section 5 item 6.

### 2.9 Silkscreen

`v2/parts/wiring.md` section 1.2, read off the vendor photo and confirmed
against the schematic:

```
odd  row (lower, V 32.582):  3V3  GND  GP0   GP1   GP2  GP3  GP17  GP18
even row (upper, V 35.122):  VBUS GND  GP19  GP20  TXD  RXD  SDA   SCL
                             ^ pin 1 / pin 2 at the USB-C end
```

**These legends are on the REAR face.** In the v2 deck the board lies display
up, so they face the carrier and are *not* what the acrylic shows — which is
the opposite of the 5710, whose legends are the thing you will be looking at.
They are modelled anyway, because the person routing thirteen wires into this
socket is the one who needs them, and the carrier's underside is where that
gets checked.

---

## 3. What this corrects, by name

### 3.1 `v2/parts/hdmi-feather.md`

| § | says | measured | matters? |
|---|---|---|---|
| 2.1 | corner radius ≈ 1.90 | **2.540** | yes — it is why the USB-end holes are concentric with their corners |
| 2.2 | all four mounting holes Ø2.50 | **Ø2.500 / Ø2.540** | mildly — same fastener, different bore |
| 2.3, 2.4 | HDMI 6.185 above the PCB, 0.630 below, 15.000 × 8.385 | **6.219 / 0.636 / 15.013 × 8.425** gross; the clean shell is still 15.000 and the **mouth is 5.550 tall** | yes, see below |
| 2.5 | the battery JST PH is top-entry | **side entry, mouth at y = 22.453 facing +y** | yes — it changes what can be plugged in under the acrylic |
| 2.5 | the STEMMA QT is top-entry | **confirmed**, despite the `_RA` package name | no |
| 2.7 | 12-pin row at y = 21.590 | **21.654** | only to a mating board |

**The one arithmetic consequence.** `hdmi-feather.md` section 5.2 computes that
a 5710 pocketed with its PCB front face at Z = −0.150 puts the HDMI shell top
at +6.035, clearing the acrylic underside at +6.350 by 0.315. With the measured
6.219 it is

```
-0.150 + 6.219  =  +6.069        clearance  6.350 - 6.069  =  0.281 mm
```

Still positive, still not a design, and the document's conclusion — that the
board goes *below* the wiring plane because the **plug** cannot get in — is
untouched. But the number in that section is 0.034 optimistic.

### 3.2 `v2/parts/knobs.md` and `v2/parts/stack.md`

- **`knobs.md` section 1.1's "display assembly 5.53 mm above its PCB front
  face" is wrong by 2.780 mm.** It is **2.750**. The chain is
  4.35 / 7.00 / 2.75 front-to-back, not 4.35 / 2.75 / 7.00, and the 7.00 is the
  standoff, measured. Section 2.4.
- **`knobs.md`'s "clearance over the display = 1.270 mm" becomes 4.050 mm**
  at the same board height. Section 2.4.
- **`knobs.md`'s "implied LCD glass 1.18 mm" is a 2.000 mm acrylic spacer plus
  a 0.750 mm glass**, and the V1→V2 delta of 0.600 is the spacer alone.
  Section 2.5.
- The PCB thickness it assumes, 1.57, is **1.600** `[step]`.

None of these moves `stack.md`'s A = 6.350, and none of them is fixed here —
this task does not edit those files. They are listed so that whoever owns them
can.

### 3.3 Where v1 `cad/parameters.scad` is confirmed

Worth recording, because it is a large number of independent agreements between
two different vendor packages:

`board_mount_pitch_x = 85.50` · `board_mount_pitch_y = 62.10` ·
`board_standoff_d = 5.50` · `board_standoff_h = 7.00` ·
`display_module_w = 91.00` · `display_module_h = 67.60` ·
`display_active_w = 84.80` · `display_active_h = 63.60` ·
`display_off_x = -1.60` · `display_off_y = 0.00` ·
`expansion_win_x = -0.10` (U 46.150 against 46.149) ·
`expansion_win_y = -0.70` (V 33.850 against 33.852) ·
`expansion_body_h = 8.700` (exactly) ·
`expansion_pitch = 2.54` · body 21.003 × 6.603 ·
`usbc_body_w = 9.582` · `usbc_body_h = 4.163` · `usbc_w_centre = -1.170` ·
`tf_body_w = 16.103` · `tf_body_h = 2.452` · `tf_w_centre = -0.625` ·
`tf_off_y = 19.15` · the microSD mouth 1.03 inside the edge ·
`button_pitch = 10.00` at U 36.25 / 46.25 / 56.25 · `button_body_w = 4.553` ·
`button_body_h = 2.203` · `button_w_centre = -0.70` · the switch body
0.19 inside the edge · `mic_offset_x = 32.50` · `mic_w_centre = -0.50` ·
`batt_off_y = -19.40` · `batt_bay_h_step = 21.10` ·
`board_w_battery_end = -15.20` · the base overhanging the PCB by 1.00 on one
long edge · `board_corner_r = 0.00`.

**Disagreement with v1: one.** `batt_bay_w = 77.80` against 87.000
(section 2.8). Everything else in that list lands. Note also that v1's
`board_pcb_t = 1.60` is **right** and it is `knobs.md` section 1.1's assumed
1.57 that is not — the two v2 documents disagree with each other, not with v1.

---

## 4. How the measurements were made

### 4.1 Toolchain

`/usr/bin/python3.11` with **OCP 8.0.1.1** (OpenCASCADE), **ezdxf**, and
`pdftoppm` for the drawing. OpenSCAD 2021.01 for the renders. The STEP files
are read with `STEPCAFControl_Reader` in name mode, which gives the assembly
tree with the names the vendor authored.

### 4.2 The four queries, and the one trap

1. **Accumulated transforms.** `XCAFDoc_ShapeTool::GetShape_s` on a component
   label returns the shape located relative to its *parent*, not to the
   assembly root. Walking the tree and composing `GetLocation_s` at every
   reference is what puts all 859 Waveshare leaves in one frame. Without it
   the PCB and the display come out in different coordinate systems and
   everything downstream is quietly wrong. The whole tree is then re-rooted on
   the `PCB/BOARD` leaf, which is what makes U/V/W possible.
2. **Exact bounding boxes.** `BRepBndLib::Add_s(shape, box, useTriangulation)`
   with `useTriangulation = False`. With `True` it pads by the mesh deflection,
   which showed up here as ±0.015 on the PCB outline — small, and large enough
   to turn a 1.600 PCB into a 1.611 one.
3. **Holes and radii** from cylindrical faces: `BRepAdaptor_Surface`, axis
   direction, location and radius, filtered by diameter. This is how the
   Ø2.500 / Ø2.540 split on the 5710 and the Ø2.400 aux hole on the Waveshare
   were found, neither of which a mesh scan will separate.
4. **Which way a socket opens**, which is not a bounding-box question. Two
   complementary queries: a 0.05 mm-thick boolean slab swept along each axis,
   reporting section area — a wall is a full section, a mouth is a thin ring —
   and planar-face areas and outlines, which is also how the LCD active area
   fell out as a single 5393.280 mm² face and the header's sixteen holes as
   the inner wires of its top face.

The trap worth naming: **the vendor's assembly is not watertight and its solids
interpenetrate.** The Waveshare PMMA spacer's bounding box overlaps the display
module's by 1.051 mm, which looks like an error until the planar faces show
that the module's body sits *in the spacer's window*. Point classification on
these files is not to be trusted; bounding boxes, faces and boolean sections
are.

### 4.3 The renders, and the check that matters

Both files render under OpenSCAD 2021.01, **every `part=` view, non-empty and
watertight**, and every rendered bounding box lands on the measured number:

| file, part | triangles | bounding box, mm |
|---|---|---|
| `feather` board | 50,960 | x −1.182 … 51.686 · y 0.000 … 22.860 · z −0.636 … 7.789 |
| `feather` pcb | 13,588 | x 0.000 … 50.800 · y 0.000 … 22.860 · z 0.000 … 1.570 |
| `feather` ports | 1,660 | x −1.182 … 51.686 · y 3.905 … 22.453 · z −0.636 … 7.789 |
| `feather` silk | 47,480 | x 5.705 … 45.075 · y 3.174 … 19.779 · z 1.570 … 1.590 |
| `feather` envelope | 12 | the same box as `board` |
| `rlcd` board | 53,728 | U 0.000 … 92.580 · V 0.000 … 69.100 · W −15.200 … 4.351 |
| `rlcd` pcb | 1,952 | U 0.000 … 92.500 · V 0.000 … 69.100 · W 0.000 … 1.600 |
| `rlcd` shipped | 54,124 | U −0.002 … 92.580 · V −1.000 … 69.103 · W −15.200 … 4.351 |
| `rlcd` display | 64 | U 0.048 … 92.580 · V 0.050 … 69.053 · W 1.600 … 4.351 |
| `rlcd` ports | 2,248 | U 33.974 … 92.521 · V 29.754 … 68.915 · W −8.702 … 0.911 |
| `rlcd` silk | 45,172 | U 34.150 … 57.617 · V 26.244 … 63.682 · W −0.020 … 0.000 |
| `rlcd` envelope | 12 | U 0.000 … 92.500 · V 0.000 … 69.100 · W −15.200 … 4.350 |

Read with `trimesh`; the counts are triangles, not OpenSCAD's own polygon
"Facets" line. **Every one is watertight, and the bounding boxes are the real
check**: the `rlcd` board's 92.580 is the PCB's 92.500 plus the display FPC's
0.080 overhang, its −15.200 is the 18650 holder, its 4.350 is the display
front face, the `shipped` view's −1.000 in V is the stand base overhanging the
18650 edge, and `ports` bottoms out at exactly the header's −8.702. Every one
of those is a measured number from sections 1 and 2, arriving back out of the
geometry.

The 4.351 is the one deliberate exception: the active-area inlay is drawn
0.001 mm proud of the display's front face so that it is visible in a render
rather than coplanar with it. The datum is 4.350 and `ws_active_area()`
returns the AA's own plane at 4.325.

`$fn = $preview ? 24 : 96`. The silk views are the expensive ones — they are
`text()` — which is why `show_silk` exists.

### 4.4 Re-running it

```
openscad -o /tmp/feather.stl v2/cad/parts/feather_rp2040_dvi.scad
openscad -D 'part="envelope"' -o /tmp/f_env.stl v2/cad/parts/feather_rp2040_dvi.scad
openscad -o /tmp/rlcd.stl    v2/cad/parts/waveshare_rlcd42.scad
openscad -D 'part="shipped"' -o /tmp/r_ship.stl v2/cad/parts/waveshare_rlcd42.scad
```

Both files `echo()` their own key dimensions at the end, so a render is also a
check: if the echoed active area, pin-1 position or thickness chain moves, the
geometry moved.

---

## 5. NOT SOURCED

Stated loudly, per the project's rule that an unsourced number is a bug.

1. **Which Waveshare revision the board in hand is — V1 or V2.** It is worth
   0.600 mm of acrylic clearance and it is the only thing in section 2.4 that
   is not settled. The measured package is the **V2** (its filename carries the
   vendor's 2026-10-10 revision). V1 would put the display front at W = +3.750
   and the glass-front-to-standoff-back stack at 10.780; V2 gives 4.350 and
   **11.380**. v1 `cad/parameters.scad` records a caliper reading of **11.00**
   on the owner's board, which sits between the two and leans V1.
   *Measure:* calipers from the display's outer face to the back of a corner
   standoff, on the real board. 10.78 is a V1, 11.38 is a V2.

2. **What a mating male header costs below the Waveshare's P1, and what the
   5710's headers cost below it if they are ever fitted.** Neither is in either
   vendor model. `ws_header_mate()` defaults to an 8.500 stack plus a 1.600
   card and `feather_rp2040_dvi.scad` carries `hdr_body_h = 8.500` and
   `hdr_tail_h = 3.000`; **all four are defaults, not measurements.** The 5710
   tail length is the number that decides whether that board clears
   `stack.md`'s −3.000 channel floor.
   *Measure:* calipers on the actual parts, soldered, before cutting a carrier.

3. **The HDMI Type A plug's overmould envelope**, still open from
   `hdmi-feather.md` section 8.7 and not closed here — the Molex document gives
   the receptacle, not the mating plug. Section 1.5's finding that the 5710's
   **battery** plug now leaves sideways has the same gap: a JST PH plug's body
   height was not measured either, and whether it clears 6.350 mm of air under
   the acrylic depends on it.
   *Measure:* calipers on the cable and the battery lead you will actually use,
   and buy the right-angle HDMI.

4. **Which of the 5710's two buttons is Reset and which is Boot.** Read off
   Adafruit's render as `Rese…` on the button nearer the USB-C and `Boo…` on
   the other, but in that render the button bodies cover the end of each word.
   It affects nothing mechanical — both bodies are identical and both are
   modelled — only the legend the model prints.
   *Check:* look at the silk on the real board, or at the fab print.

5. **The purpose of the Ø2.400 hole at (U 10.410, V 7.050) on the Waveshare.**
   It is in the B-rep, it is not on the drawing, and it is in no document here.
   It sits inside the 18650 holder's footprint in V and well outside it in U.
   *Check:* the schematic's board outline layer, or the real board.

6. **The 18650 holder's length along U: 87.000 `[step]` against 77.800
   `[repo, calipers]`.** Not reconciled; the two are probably body against
   footprint-including-contacts. The model draws 87.000 because it is the
   conservative value in both roles it plays.
   *Measure:* calipers across the holder including its end contacts.

7. **Neither vendor publishes a PCB thickness in words.** 1.570 for the 5710
   and 1.600 for the Waveshare are both `[step]`, from the board solids, and
   both are plausible standard stack-ups. They are not vendor statements.

8. **Waveshare's speaker is modelled 40.001 × 30.000** for a part named
   SPK4020, where 4020 would normally mean 40 × 20. The extra 10 mm in V is
   probably a gasket or a mounting flange. Nothing in this enclosure touches
   it, so it is recorded rather than chased.

9. **The internal layering of the Waveshare display module above W = 2.549.**
   The module solid interpenetrates the acrylic spacer's bounding box by
   1.051 mm (section 4.2). The front face at **4.350** and the active-area
   plane at **4.325** are both measured faces and are what the enclosure needs;
   what is between 2.549 and 3.600 is read from areas, not from a part drawing.

10. **`hdmi-feather.md` section 8.12's standing item is unchanged**: all the
    5710 positions here come from Adafruit's CAD model, not from a caliper on
    a real board. They are internally consistent and land on clean imperial
    values, which is why they are stated as findings. **Caliper the board when
    it arrives, before cutting the carrier.**

---

## 6. One-line summary

**Adafruit 5710: 50.800 × 22.860 × 1.570, R2.540 corners, mounting holes on a
trapezoid with TWO drills (Ø2.500 at the USB end, Ø2.540 at the HDMI end,
45.720 between columns), HDMI shell 15.000 wide and 6.219 above the PCB with a
5.550-tall mouth and 0.636 of tab below, battery JST PH SIDE-entry out the long
edge and STEMMA QT top-entry · Waveshare ESP32-S3-RLCD-4.2: 92.500 × 69.100 ×
1.600, square corners, four Ø4.200 on 85.500 × 62.100 with 7.030 standoffs, one
unexplained Ø2.400 at (10.410, 7.050), display front 4.350 above the PCB rear
face and therefore 2.750 above its front face — not 5.53 — active area
84.802 × 63.603 at U 2.248…87.050 and V 2.750…66.353 offset 1.601 toward U = 0,
and the 2×8 header at centre (46.149, 33.852) with pin 1 at (55.039, 32.582)
and its far face 8.700 behind the PCB.**
