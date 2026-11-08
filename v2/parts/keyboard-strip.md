# Stripping the Keychron K9 Pro — the internals, measured

The decision changed. `research/keyboard-k9pro.md` §8.1 and §14 both advised
mounting the keyboard whole; the user has since ruled the other way:

> *"We are sending this as a masterpiece — not mounting it whole. we will strip
> it back but use all info you have to get as accurate as possible."*

So this document does not re-argue it. It settles the geometry a stripped build
needs, from Keychron's own 37.7 MB assembly model, and says exactly what the
carrier plate has to take over from the ABS bottom case.

**The headline: the PCB's underside is at y = −3.700 and it is flat and
horizontal.** §14 of the research document refused to quote it because point
classification against this file is unreliable. That was the right call about
the wrong method — the number does not need classification. The PCB carries two
large planar faces normal to Y, at y = −2.500 and y = −3.700, and a plane's
position is an analytic property of the surface, not something you sample.
**The board is 1.200 mm thick, its top is 2.500 below the frame and its
underside is 3.700 below the frame**, everywhere, with no slope.

---

## 0. Verdict up front

| | |
|---|---|
| **the stripped assembly is** | the aluminium top frame (which **is** the switch plate), 61 switches, 61 hot-swap sockets, the PCB, the keycaps, the USB-C, both toggles, and the cell |
| **what the carrier takes over** | 18 cored screw bosses, 33 PCB seat pads totalling 261.350 mm², 16 snap-latch tongues, the battery bay, and the typing angle |
| **the seat plane** | y = −3.700 relative to the frame top; **flat and horizontal**, so the wedge stops being the keyboard's and becomes ours |
| **the screws** | 7, through the plate, into Ø4.000 bosses with Ø1.700 pilots. Positions in §3.3. Reproduce them and the assembly bolts in with its own hardware |
| **the ports are not lost** | all four rear apertures are cut in the **top** frame, at z = 8.000, and the top frame is a part we keep. §4 |
| **the one real cost** | the frame's skirt is 1.000 mm thick and hangs **8.595 mm below the PCB seat at the rear**. The carrier needs a groove that deep, or the skirt gets cut. §3.5 |
| **the grid is 19.000, not 19.050** | every switch sits on an exact 19.000 mm lattice. The switch field is **285.000 × 95.000**, not 285.75 × 95.25. §1.6 |

---

## 0.1 Source tags

| tag | means |
|---|---|
| `[CAD-EXACT]` | read off an analytic surface in the STEP file — a plane's position, a cylinder's radius and axis, a wire's extent with no tessellation. No tolerance, no sampling. |
| `[CAD]` | measured from the STEP file after triangulating at 0.020 mm linear / 0.2 rad angular. Bounding boxes are enlarged by the deflection, so these carry ±0.02 and are quoted as measured. |
| `[CAD-MESH]` | measured from an exported mesh at the stated deflection. |
| `[DERIVED]` | arithmetic from the above, shown. |
| `[VENDOR]` | published by Keychron. |
| `[REPO]` | from this repository. |
| `[HOUSE]` | from `/home/user/kylebsmith/cmtl-3d-assets`. |
| **NOT SOURCED** | not a measurement. §7 lists every one with the procedure that closes it. |

### A correction to how the last session measured

`research/keyboard-k9pro.md` §14 reported "plate assembly y −6.88 … −0.97".
That range is the **PCB part's** bounding box, not a plate, and it is a
bounding box of fused components rather than the board. It also reported the
whole model's extents as larger than they are. Both come from the same trap:

> `BRepBndLib::Add` with `useTriangulation = true` on a shape that has **not yet
> been meshed** silently falls back to the underlying surfaces' pole hull. On
> the top case that overshot by **8.4 mm in Y and 15.3 mm in Z** — it reported
> the part as 287.8 × 114.6 when it is 287.0 × 97.5. Triangulate first, then
> bound. Every `[CAD]` number here was taken after an explicit
> `BRepMesh_IncrementalMesh`.

`OCP 8.0.1.1` has a second trap worth recording: `Bnd_Box.Get()` resolves to an
unregistered `Bnd_Box::Limits` overload and raises `TypeError`. Use
`CornerMin()` / `CornerMax()`.

---

## 1. Getting the internals out, and what came out

### 1.1 The reader that worked

`STEPCAFControl_Reader` with `SetNameMode(True)` reads the file in **15.7 s** and
transfers cleanly. The earlier segfault came from a recursive child walk; the
walk that works asks the shape tool for components explicitly and composes the
locations itself:

```python
comps = Sequence_TDF_Label()                  # OCP.collections, not OCP.TDF
st.GetComponents_s(lab, comps, False)         # direct children only
for i in range(1, comps.Length() + 1):
    c = comps.Value(i)
    csh = XCAFDoc_ShapeTool.GetShape_s(c).Moved(parent_location)
    ref = TDF_Label()
    if st.GetReferredShape_s(c, ref):
        walk(ref, csh)                        # recurse on the prototype
```

`GetShape_s` on a *component* label returns the referred shape already carrying
that component's own location, relative to its parent. `Moved(parent_location)`
composes. That gives correct world geometry for all 1343 nodes.

`XCAFDoc_ShapeTool::GetShapes` reports **84 shapes**, exactly the 84 named
products, and `GetFreeShapes` reports **1** — the root. The STEP-text fallback
and the bounding-box-signature fallback were not needed.

### 1.2 The file holds three keyboards

`00-K9PRO-3D_20230426_ASM` contains **three** sub-assemblies:

```
00-K9PRO-US-3D_ASM      ANSI   <- the one we buy, and the only one measured here
00-K9PRO-UK-3D_ASM      ISO    K9PRO_TOP_UK, K9PRO_UK-PCB, ENTER-UK
00-K9PRO-JP-3D_ASM      JIS    K9PRO_TOP_JP, K9PRO_JP-PCB, SPACE-JP, PHG_76
```

Every number below is filtered on `-US-` in the instance path. A measurement
that mixes the three gives a keyboard with 183 keys.

### 1.3 The US assembly, as it is actually built

```
00-K9PRO-US-3D_ASM
├── 01-CK8876-TOP-84-WG_ASM            the top half
│   ├── CK8876_TOP_84-WG   x1          THE ALUMINIUM FRAME *AND* THE SWITCH PLATE
│   ├── K9PRO_PCB          x1
│   ├── SW-ASM             x61          switch + hot-swap socket + LED
│   ├── TYPE-C_16PIN       x1
│   ├── 2P_1_1             x1          2-position slide switch  (OS)
│   ├── 3P_1_1             x1          3-position slide switch  (BT / OFF / Cable)
│   ├── CK8892_BUTTON      x4          the two slider caps, each drawn twice
│   ├── PC_1_1             x1          indicator light pipe
│   ├── JGZHT / KEY-BKT    x6          stabilisers and the Caps bracket
├── 02-CK8876-BOT_ASM                  the bottom half — ALL OF THIS IS DISCARDED
│   ├── CK8876_K3_BOTTOM_43_20  x1     the ABS tray: 18 bosses, 33 pads, the chassis
│   ├── K9PRO-PLATE             x1     *not* a switch plate: a 0.500 SECC steel shield
│   ├── BATTERY-3030130         x1
│   ├── LEFT-PET / RIGHT-PET    x2     insulating films
│   ├── BIG_HOLDER / SMALL_HOLDER x4   the flip-out feet
│   └── BREP_122 / BREP_123     x2
├── keycaps                 x61 (as 8 named assemblies, 2 solids each)
├── FOOT_PAD_1  x4 / FOOT_PAD_2 x1     rubber pads
└── FOOT_STOOL_ASM x2                  one drawn deployed, one drawn folded
```

> **Two names in that tree are traps.**
>
> **`K9PRO-PLATE` is not the switch plate.** It lives in the *bottom* assembly,
> its component instance is named `SECC0_5_K7`, and it is a **0.500 mm
> electro-galvanised steel sheet** lying on the tray floor *under* the PCB
> (§1.5). The last session's §14 listed it as "the switch plate" on the
> strength of its name.
>
> **There is no separate switch plate at all.** The switch plate is the
> **top case part**, `CK8876_TOP_84-WG`: one solid whose upper 1.200 mm is the
> plate, with the 61 cutouts in it, and whose skirt is the visible frame. Plate
> and frame cannot be separated — they are one injection/stamped part. This is
> the single most consequential thing the model says about stripping: **you
> cannot throw the frame away and keep the plate.**

### 1.4 The frame of reference, re-confirmed

**X = width, Y = up, Z = depth, low Z at the REAR.** Confirmed three ways:

| check | value | tag |
|---|---|---|
| the bottom assembly's extents | 286.053 × 12.105 × 96.553 — a keyboard | `[CAD]` |
| the top frame's outer footprint | **287.000 × 97.500**, corner **R4.400** | `[CAD-EXACT]` |
| Keychron's published footprint | 287 × 97.5 | `[VENDOR]` |

The R4.400 corners are four Y-axis cylinders at (−6.100, 12.400), (−6.100,
101.100), (272.100, 12.400), (272.100, 101.100) `[CAD-EXACT]`, so the outer
footprint is X **−10.500 … 276.500** and Z **8.000 … 105.500**. Its centre is
X = 133.000, and that is the datum used to map into deck coordinates in §3.3.

**y = 0 is the frame's top face, and it is flat.** One planar face, normal
exactly (0, +1, 0), at y = 0.000, area 14298.438 mm², and it is the highest
point of the part `[CAD-EXACT]`. This confirms the earlier sectioning result
("top surface y = 0.00 at every depth FLAT") analytically rather than by
sampling.

### 1.5 The four parts the task asked for

#### K9PRO_PCB

| | value | tag |
|---|---|---|
| outline (X × Z) | **282.211 × 93.211**, X −9.206 … 273.006, Z 10.594 … 103.806 | `[CAD-EXACT]` |
| draft | none — the outer wire is identical on both faces | `[CAD-EXACT]` |
| **top face** | **y = −2.500**, area 22208.598 mm² | `[CAD-EXACT]` |
| **bottom face** | **y = −3.700**, area 22157.785 mm² | `[CAD-EXACT]` |
| **thickness** | **1.200** | `[DERIVED]` |
| solids | 2: the board, plus a duplicated hot-swap socket at the Esc position | `[CAD]` |
| board volume | 26916.696 mm³ | `[CAD]` |
| export | `models/k9pro-internals/k9pro-pcb.stl` | |

Its 253 distinct hole axes, clustered to full circles `[CAD-EXACT]`:

| n | size | what |
|---|---|---|
| 61 | Ø5.200 | the switch's central boss / LED window, one per key, **on the key centre** |
| 122 | Ø3.100 | the hot-swap socket barrels, two per key, at key-centre offsets (−4.428, +4.680) and (+2.580, +5.675) |
| 61 | 3.600 × 3.200 | the backlight LED window, at key-centre offset (−0.704, −4.540) |
| 10 | 5.400 × 8.160 | stabiliser clearance |
| 9 | 8.100 × 4.000 | stabiliser clearance |
| **9** | **Ø5.000** | **clearance for the top frame's screw bosses to pass through the board** |
| 1 | Ø5.500 | at (103.125, 95.000) |

**The PCB has no mounting holes of its own.** Nothing on this board is tapped,
bolted or bossed. It is held by the 61 switches above it and rests on the
tray's pads below it. That is the fact §3 has to design around.

The board's bounding box runs y −6.884 … −0.976 because the part has its
components fused into it: the USB-C shell reaches y = −6.868 and there is a
Ø3.500 × 0.500 pad on the top face at (103.375, 95.250). **The board itself is
only the 1.200 between the two planes.**

#### K9PRO-PLATE — the 0.500 SECC steel shield

| | value | tag |
|---|---|---|
| two parallel planes | normal (0, ±0.998261, ∓0.058943) — **inclined 3.37914°** | `[CAD-EXACT]` |
| **thickness** | **0.500** (plane-to-plane separation 0.500071) | `[DERIVED]` |
| top surface | y = **−10.568** at z = 11.904 → **−5.811** at z = 92.463 | `[CAD-EXACT]` |
| plan extent | X −7.240 … 272.624, Z 11.880 … 92.517 | `[CAD]` |
| sheet area / volume | 12775.496 mm² / 6387.636 mm³ | `[CAD]` |
| Y-normal planar faces | **zero** — the sheet is inclined, not horizontal | `[CAD-EXACT]` |
| holes | 13 × Ø2.800 over the tray's Ø2.500 posts; 5 × Ø4.600 around the PCB bosses | `[CAD-EXACT]` |
| export | `k9pro-shield-secc0p5.stl` | |

Two independent confirmations fall out of the hole geometry. A vertical bore of
radius *r* through a sheet of thickness *t* inclined at θ has a Y extent of
*t*/cos θ + 2*r* tan θ. Measured: 0.667 at r = 1.400 and 0.772 at r = 2.300
`[CAD-EXACT]`. Two equations, two unknowns:

```
2(2.300 - 1.400) tan θ = 0.772 - 0.667   ->  tan θ = 0.058333  ->  θ = 3.339 deg
t / cos θ = 0.667 - 2(1.400)(0.058333)   ->  t     = 0.5028
```

— against 3.37914° and 0.500071 from the plane normals. The sheet's *name*
says SECC **0.5**. Three routes, one answer.

**Its bottom plane and the tray's inner floor plane coincide to 0.0000**
`[DERIVED from CAD-EXACT]` — the steel lies flat on the ABS. It is a shield and
stiffener, it does not touch the PCB except incidentally (its highest point,
−5.787, is 0.004 above the lowest point of the hot-swap sockets), and **it is
discarded**: once the ABS tray goes, the sheet has nothing to lie on.

#### BATTERY-3030130

Six planar faces, volume **exactly 12480.000 mm³** `[CAD]`. That is not a
coincidence:

```
130.000 x 32.000 x 3.000 = 12480.000        [DERIVED]
```

and laying that prism in the 3.37914° floor plane gives

```
Y extent  3.000 cos 3.37914 + 32.000 sin 3.37914 = 4.88092   measured 4.881
Z extent 32.000 cos 3.37914 +  3.000 sin 3.37914 = 32.12119  measured 32.121
```

| | value | tag |
|---|---|---|
| **the cell** | **130.000 × 32.000 × 3.000** | `[DERIVED]`, exact |
| as placed | X 75.000 … 205.000 (centre **140.000**), Y −10.720 … −5.839, Z 17.632 … 49.753 | `[CAD]` |
| attitude | lying in the floor plane, tilted 3.37914° about X | `[DERIVED]` |
| export | `k9pro-battery-3030130.stl` | |

The part number 3030130 reads as 3.0 × 30 × 130; the model is 32 wide, so take
32.000. The cell sits in a window of the steel shield and bears directly on the
ABS floor: the shield's plan silhouette contains **0.00 mm² of steel** inside
the cell's 4175.73 mm² footprint `[CAD-MESH]`.

> **No connector, no leads.** The model contains no battery connector, no JST
> footprint and no wire. Every small cylinder on the PCB below Ø1.0 is inside
> the USB-C footprint at X 120 … 127. **NOT SOURCED** — §7.

#### GATERON-A_ASM — one switch

Six solids. Overall **15.052 × 15.052 in plan, y −5.676 … +6.026** `[CAD]`.

| solid | X × Z | Y | what |
|---|---|---|---|
| `MANIFOLD_SOLID_BREP_208` | 15.004 × 15.004 | −5.652 … +2.402 | lower housing and the three pins |
| `MANIFOLD_SOLID_BREP_209` | 14.617 × 13.147 | −2.202 … +3.303 | upper housing |
| `MANIFOLD_SOLID_BREP_204` | 10.602 × 7.026 | −2.351 … +6.001 | the stem — keycap mount |
| `MANIFOLD_SOLID_BREP_206/207` | — | −5.140 … +2.493 | the contact leaves |
| `PRT0001` | 2.818 × 2.827 | −4.931 … +1.791 | the backlight LED |

With its socket (`SW-ASM`) the whole switch station is **y −5.812 … +6.022**
`[CAD]`. Export: `k9pro-switch-gateron-one.stl`,
`k9pro-hotswap-socket-one.stl`.

The **hot-swap socket** `KS-2P02B02-02-20200922` (two solids, 61 off) spans
**y −5.791 … −2.763** `[CAD]` — body below the board, contacts up inside the
Ø3.100 barrels. Per key its envelope is X −8.514 … +6.587 and Z −6.041 …
+7.950 relative to the key centre, i.e. **15.101 × 13.991, offset (−0.964,
+0.955)**. **This is the deepest thing under the switch field and it is what
sets the carrier's pocket depth.**

Gateron's published 12.2 mm switch height `[VENDOR]` is not what the model
shows: housing bottom to stem top is 5.652 + 6.001 = **11.653** `[DERIVED]`,
and pin tip to stem top the same 11.653. The 12.2 is not reproduced here; §7.

### 1.6 The grid is 19.000 mm

All 61 Ø5.200 holes, from the board's top face `[CAD-EXACT]`:

| row | Z | n | X |
|---|---|---|---|
| 0 number | **19.000** | 14 | 0, 19, 38, 57, 76, 95, 114, 133, 152, 171, 190, 209, 228, **256.500** |
| 1 qwerty | **38.000** | 14 | 4.760, 28.540, 47.540, 66.540, 85.540, 104.540, 123.540, 142.540, 161.540, 180.540, 199.540, 218.540, 237.540, 261.300 |
| 2 asdf | **57.000** | 13 | 7.150, 33.250, 52.250, 71.250, 90.250, 109.250, 128.250, 147.250, 166.250, 185.250, 204.250, 223.250, 254.200 |
| 3 zxcv | **76.000** | 12 | 11.900, 42.800, 61.800, 80.800, 99.800, 118.800, 137.800, 156.800, 175.800, 194.800, 213.800, 249.395 |
| 4 bottom | **95.000** | 8 | 2.375, 26.125, 49.875, **121.125**, 192.370, 216.130, 239.890, 263.650 |

Every 1 u key lands on an exact **19.000** lattice, in both axes, and every
position agrees with Keychron's own QMK `LAYOUT_61_ansi` `[REPO]` under

```
model_x = (u_centre - 0.5) x 19.000        model_z = 19.000 x (row + 1)
```

to **0.000** on the 1 u keys. The wide keys are nudged off the nominal unit
centre by up to **0.040** (the 1.5 u Tab sits at 4.760 where the grid wants
4.750; the 1 u beside it at 28.540 where the grid wants 28.500). Use the
measured column above, not the arithmetic.

> **A correction to the layout specs.** `v2/layouts/_common.py` and
> `cad/keyboard_blank.scad` both take the field as 15 u × 5 u at **19.05**, i.e.
> 285.75 × 95.25. At 19.000 the switch-centre field is **285.000 × 95.000** and
> the plate's own outline is **285.000 × 95.500** at its underside. The repo's
> claim that "the control field is 15 u = 285.75 — the K9 Pro's own switch
> field" is 0.750 mm out. Nothing in this document changes those files; it is
> flagged for the enclosure task.

Spacebar: 6.25 u at X **121.125**, stabilisers at **121.125 ± 50.000**
`[CAD-EXACT]`.

---

## 2. The stack

All Y in the model frame: **y = 0 is the frame/plate top face, flat; Y increases
upward.** Z in brackets where the surface is inclined.

| | y top | y bottom | thickness | tag |
|---|---|---|---|---|
| **keycap top** (1 u, LSA) | **+9.5753** mesh / **+9.5759** analytic | | | `[CAD-EXACT]` / `[CAD]` |
| keycap outer shell | +9.5759 | **+5.2000** | — | `[CAD]`, and no longer unsourced |
| keycap stem / skirt | +8.5335 `?` | **+3.0000** | — | `[CAD]` / `[CAD-EXACT]` |
| switch stem top | +6.0000 | | | `[CAD-EXACT]` |
| switch upper housing | +3.303 `?` | −2.2000 | | `[CAD]` / `[CAD-EXACT]` |
| **plate top** = frame top | **0.000** | | | `[CAD-EXACT]` |
| **plate bottom** | | **−1.200** | **1.200** | `[CAD-EXACT]` |
| air gap, plate to PCB | −1.200 | −2.500 | **1.300** | `[DERIVED]` |
| **PCB top** | **−2.500** | | | `[CAD-EXACT]` |
| **PCB bottom — the seat** | | **−3.700** | **1.200** | `[CAD-EXACT]` |
| plate-seat boss tops | −1.209 | | | `[CAD-EXACT]` |
| PCB-seat pad tops | −3.709 | | | `[CAD-EXACT]` |
| switch pin tips | | −5.6500 | | `[CAD-EXACT]` |
| **hot-swap sockets ×61** | −2.763 | **−5.791** | | `[CAD]` |
| toggle bodies (2P / 3P) | −3.094 | −5.106 | | `[CAD]` |
| **USB-C receptacle** | −2.492 | **−6.868** | | `[CAD]` |
| PCB part's fused features | −0.976 | −6.884 | | `[CAD]` |
| **toggle slider caps** | −1.490 | **−8.260** | | `[CAD]` |
| PET films (discarded) | −5.716 | −7.044 | 1.325 / 1.004 | `[CAD]` |
| SECC 0.5 shield (discarded) | −5.811 (z 92.463) … −10.568 (z 11.904) | +0.500 below | 0.500 | `[CAD-EXACT]` |
| cell, as placed (retained) | −5.839 | −10.720 | 3.000 normal | `[CAD]` |
| tray inner floor (discarded) | −5.570 (front) … −11.271 (rear) | | | `[CAD-EXACT]` |
| **tray outer — the desk** (discarded) | | **−7.573** (front, z 105.027) … **−13.274** (rear, z 8.473) | floor 2.000 | `[CAD-EXACT]` |
| frame skirt rim (retained) | | **−6.538** (front) … **−12.295** (rear) | wall 1.000 | `[CAD-EXACT]` |
| lowest point of anything | | −22.100, a deployed foot | | `[CAD]` |

### 2.1 The slope, and the published heights

The tray's outer surface is one plane, normal (0, −0.998261, +0.058943)
`[CAD-EXACT]`:

```
tan theta = 0.058943 / 0.998261 = 0.0590457   ->  theta = 3.37914 deg
y(z) = -13.7358 + 0.0590457 (z - 0.6592)
```

| z | | y | Keychron | tag |
|---|---|---|---|---|
| 8.473 | case rear edge | **−13.274** | 13.3 | `[CAD-EXACT]` vs `[VENDOR]` |
| 105.027 | case front edge | **−7.573** | 7.6 | `[CAD-EXACT]` vs `[VENDOR]` |

Agreement to **0.026** and **0.027**. The repo's earlier derivation (3.374°,
rear 13.25, front 7.60) was right to within a rounding; this is the same
geometry read off the plane instead of sampled. The 3.37914° also appears,
identically, as the normal of the steel shield's two faces, the tray's inner
floor, and the frame skirt's rim — one plane family, four parts.

**With keycaps, Keychron's front figure does not reconcile and should not be
used.** The 1 u cap's own shell solid tops out at **+9.5753** `[CAD-EXACT]`.
**The keycap plane is NOT flat**, and the figures that said it was were padded.
All 61 cap solids isolated out of `k9pro-stripped-assembly.stl` give a maximum
that rises monotonically with cap **width**, because an LSA top is dished and
the bounding box catches a point on the cap's REAR RIM. *This read "the
cap's own left and right edges".* Measured on `k9pro-keycap-1u-one.stl`, the
single highest vertex is at x **+5.2563** and z **+25.7049** — **3.844 mm
inside** the left/right edge (57.8 % of the half-width) and **+6.705 rear** of
the cap's centre. The rise with width is a measured fact and is not in doubt;
the account of *where* on the cap it happens was wrong, and it was repeated in
three files. The location was checked on the 1 u cap only — isolating the other
eight out of the assembly to confirm it is not done here — so the mechanism
behind the monotonicity is stated as unresolved rather than re-explained:

| u | plan span | model y max |
|---|---|---|
| 1.00 | 18.2000 | **+9.5753** ×46 |
| 1.25 | 22.9500 | +9.6062 ×7 |
| 1.50 | 27.7000 | +9.6327 ×2 |
| 1.75 | 32.4500 | +9.6515 |
| 2.00 | 37.2000 | +9.6659 |
| 2.25 | 41.9500 | +9.6768 (left shift) |
| 2.25 | 42.0500 | +9.6770 (enter) |
| 2.75 | 51.4500 | +9.6871 |
| 6.25 | 118.0000 | **+9.7571** (spacebar) |

A spread of **0.1818** across the whole 285 × 95 field `[CAD-EXACT]`, the high
value being the 6.25 u spacebar. *This read "+9.842, and the per-row maxima run
+9.867 … +10.012, a spread of 0.145". Those were `manifest.json` bbox figures,
inflated by a per-shape pad of +0.2657 for this cap family; the spread was
understated as well as the heights.*

**THE COLUMN ABOVE IS THE MESH, AND IT IS NOT WHAT THE BODY IS DRAWN FROM.**
Every value in it is the bounding-box maximum of a *triangulation* of a dished
top, so each one sits just under the surface it samples. `cap_top_u()` carried
exactly this column, tagged `[CAD-EXACT]` — a tag that file defines as "read
off an analytic surface … No tessellation, no sampling, no tolerance" — and it
therefore under-stated **every** cap, by +0.0006 at 1 u rising to **+0.0087**
at 2.75 u. The envelope still did not enclose, which is the one defect a single
flat top had just been rejected for. The analytic column —
**9.5759 / 9.6130 / 9.6386 / 9.6579 / 9.6730 / 9.6844 / 9.6846 / 9.6958 /
9.7572**, spread **0.1813** — is `measured.json`'s nine named cap shapes
de-padded by each one's *own* solved pad, confirmed on three other faces of
each (x span, z span, y min, every one exact to ≤ 7e-6), and
`k9pro_internals.scad` 225-275 carries it with the procedure that would close
the remaining inference. Both columns are kept because they measure different
things and the difference is the point.

```
rear  13.274 +  9.5753 = 22.849   |  13.274 + 9.7571 = 23.031   Keychron 23.5
front  7.573 +  9.5753 = 17.148   |   7.573 + 9.7571 = 17.330   Keychron 19.8
19.8 - 7.6 = 12.2 = Gateron's published switch height
```

The rear figure lands within **0.469** of Keychron's on the tallest cap. The
front figure is **2.5 out** whichever cap you take. *Both gaps were quoted
0.255 smaller when the cap top carried the padded +9.842.*

The published "19.8 front with keycaps" looks like front height **plus switch
height** rather than a measurement — the same class of error
`research/keyboard-k9pro.md` §2.1 already found in two other rows of that
table. `cad/keyboard_blank.scad` currently carries 19.8 as `kc_top_front` with
a comment calling it the weakest number in the file. **It is wrong: the frame
top is flat, and the keycap tops stand +9.5753 … +9.7571 above it — a 0.1818
band, not the 2.2 mm the published figure needs.**

### 2.2 The stripped stack, in one block

Retaining frame + switches + sockets + PCB + keycaps + ports + cell:

```
                                              y        relative to the seat
  keycap top, 6.25u spacebar  (highest)    +9.7571           +13.4571
  keycap top, 1u                           +9.5753           +13.2753
  switch stem top                          +6.0000            +9.7000
  keycap underside  (all 61)               +3.0000            +6.7000
  PLATE TOP (the aluminium face)            0.000             +3.700
  plate underside                          -1.200             +2.500
  PCB top                                  -2.500             +1.200
  PCB UNDERSIDE  == THE SEAT               -3.700              0.000
  switch pin tips                          -5.6500            -1.9500
  hot-swap socket floor                    -5.791             -2.091
  USB-C shell floor                        -6.868             -3.168
  toggle slider cap floor                  -8.260             -4.560
  frame skirt rim, front                   -6.538             -2.838
  frame skirt rim, rear                   -12.2945            -8.5945
    (k9pro-topcase-switchplate.stl's mesh y min is -12.294510, so the rear
     rim is -8.594510; k9pro_internals.scad's rim_z_rear says -8.595, which
     is the same number to 3 dp. Both are right at their own precision and
     neither is a second measurement of the other.)
```

**THE TWO `?` ROWS WERE NOT UNSOURCED, AND THE `?` COMES OFF ONE OF THEM.** Both
figures are in `k9pro-internals/measured.json`, as the 1 u cap's two B-rep
solids — `[793]` `MANIFOLD_SOLID_BREP_421223_1_1`, vol **210.6842**, y
**2.9434 … 8.5901**, and `[794]` `MANIFOLD_SOLID_BREP_166287_1_1`, vol
**465.7881**, y **4.9343 … 9.8416**, whose volumes sum to the `1-KEY_ASM`
entry. `[794]` is the outer shell (de-padded span 18.2000, the full cap);
`[793]` is the narrower stem/skirt. So **+4.934 and +8.590 were the raw padded
bboxes all along**, and the table carried them with a `?` rather than naming
where they came from.

De-padded by the 1 u cap's own solved pad **+0.2657**, `[794]` gives y
**5.2000 … 9.5759** — and 5.2000 is independently `check_cad.py`'s
`CAP_WIDEST_Y`, while 9.5759 sits 0.0006 above the mesh's 9.5753, which is the
right side for an analytic surface over a tessellated dome. Two confirmations,
so that row's `?` is gone.

**`[793]` KEEPS ITS `?`, AND THE REASON IS A REAL INCONSISTENCY.** The same
+0.2657 would put the stem's bottom at **3.2091**, but the mesh bottoms the
whole cap at exactly **3.000000**, so the shell's pad is 0.209 too big for the
stem. Solving the stem's own pad from that 3.000000 instead gives **+0.0566**
and a top of **8.5335**, which is the figure now in the table — but it is one
equation in one unknown with nothing to check it against, where the shell's pad
had two. **The pad is per SOLID, not merely per shape:** two solids of one cap
want 0.2657 and 0.0566. That is a sharper statement of the same fact the
`cap_gap` note rests on, and it is why no pad in this file may be reused on
another shape.

**AND `+2.943` IS NOT UNRECORDED EITHER.** `k9pro_internals.scad` retired it as
a figure whose "provenance is unrecorded ... in neither the manifest (2.7343)
nor the mesh (3.000000)". It is `[793]`'s own padded y min, **2.9434**. Retiring
it was still right — it is a padded sub-solid bbox and the cap's bottom is
3.000000 — but it was never provenance-less. *The row also no longer claims the
manifest's two-solids count is wrong: the cap genuinely has two B-rep solids
that touch, which is exactly why mesh union-find welds them into one body, the
same B-rep-versus-mesh distinction that explains 617 shapes against 573 bodies.*

*The four keycap and switch rows read +9.842/+13.542, +6.022/+9.722,
+2.943/+6.643 and -5.652/-1.952. Every one was `manifest.json` bbox padding.
The cap row is now TWO rows because the cap plane is not flat.*

**Total retained height: 9.5753 + 12.2945 = 21.870 mm** at the rear on a 1 u
cap, **22.052 mm** under the spacebar, against the stock keyboard's 22.849.
Stripping the tray saves **0.979 mm** of overall height and nothing more —
because the frame's skirt, not the tray, is what reaches down. What stripping actually buys is listed in §3.5; height is not on
the list.

---

## 3. How the plate is held, and how we hold it

### 3.1 It is screwed *and* clipped, from above

The answer the research document could not get: **the K9 Pro's assembly screws
enter from the top, through the plate, hidden under the keycaps.** There are
seven of them, and sixteen snap latches.

The tray carries **18 cored bosses**, every one Ø4.000 outside with a Ø1.700
bore, the bore opening **upward** and blind `[CAD-EXACT]`. They split by the
height of their top face:

| family | boss top | n | lands on | role |
|---|---|---|---|---|
| **A** | **y = −1.209** | 11 | the plate's underside at −1.200 | 7 take screws, 4 are supports |
| **B** | **y = −3.709** | 7 | the PCB's underside at −3.700 | supports; nothing is screwed to them |

The 0.009 in both cases is the modeller's clearance, not a gap.

Seven of the eleven family-A bosses have a hole above them in the plate — a
**tapered square**, 4.500 square at y = 0 and 2.500 square at y = −1.200
`[CAD-EXACT]`. A square counterbore is unusual; it is what the plate has. The
other four family-A bosses have no plate hole and are pure props. The Ø1.700
bore in all 18 is a moulding core, not a thread.

Every family-A boss clears the PCB: nine through **Ø5.000** holes, and the two
at (85.500, 15.500) and (180.500, 15.449) through local openings in the board
(no board material at either point, confirmed against the 11407 triangles of
the y = −2.500 plane `[CAD-MESH]`).

**Sixteen latches** hold the rim between the screws. They show in the frame as
R5.000 relief arcs and in the tray as 4.000 × 0.500 tongues `[CAD-EXACT]`:

| run | z | y of the arc | x centres |
|---|---|---|---|
| rear | 13.500 | −5.700 … −4.000 | −0.300, 47.700, 98.700, 149.700, 200.700, 251.700 |
| front | 100.000 | −3.700 … −2.000 | −0.300, 47.700, 98.700, 149.700, 200.700, 251.700 |
| left | 31.300 / 87.200 | −5.700…−4.000 / −3.700…−2.000 | x = −5.000 |
| right | 26.300 / 87.200 | −5.700…−4.000 / −3.700…−2.000 | x = 271.000 |

The tray's six rear tongues sit at z = 8.549, y −5.680 … −5.180, x centres
−1.570, 47.550, 98.800, 149.700, 200.700, 251.700 `[CAD-EXACT]`.

### 3.2 So: the plate is held by its own frame, and the frame is screwed to the tray

Stated plainly, because it changes the problem:

- the **plate is not a separate part** and is not screwed to anything — it is
  the top 1.200 mm of the frame;
- the **frame** is screwed to the tray at 7 points and clipped at 16;
- the **PCB** is screwed to nothing. It is clamped by 61 switches whose
  shoulders bear on the 1.200 plate and whose pins go into sockets, and it rests
  on the tray's pads.

Our carrier therefore has exactly one job that is load-bearing: **be the tray.**

### 3.3 The screw bosses, which is the number this task exists to produce

Reproduce these and the stripped assembly bolts into the carrier with the
keyboard's own screws, through the keyboard's own holes.

**Deck coordinates** are `v2/layouts/slab.py`'s: the deck is 304.600 × 190.375,
origin at its centre, +y toward the control band (rearward). The mapping, from
`_common.py` with `KBD_FIT = (1.35, 0.95)` `[REPO]`:

```
deck_x = model_x - 133.000
deck_y =   7.1875 - (model_z - 8.000)
```

(133.000 is the frame's plan centre; 7.1875 is the deck y of the keyboard
pocket's rear face, the pocket's rear edge at 7.6625 less half the 0.95 depth
fit.)

#### The 7 screwed bosses — Ø4.000 OD, Ø1.700 pilot, top at y = −1.209

| # | model X | model Z | deck x | deck y |
|---|---|---|---|---|
| 1 | 16.540 | 34.500 | −116.460 | −19.3125 |
| 2 | 33.300 | 72.500 | −99.700 | −57.3125 |
| 3 | 85.500 | 15.500 | −47.500 | −0.3125 |
| 4 | 128.320 | 72.500 | −4.680 | −57.3125 |
| 5 | 180.500 | 15.500 | **+47.500** | −0.3125 |
| 6 | 223.300 | 72.500 | +90.300 | −57.3125 |
| 7 | 247.540 | 34.500 | +114.540 | −19.3125 |

They are not symmetric, because they live in the webs between switch cutouts
and the ANSI layout is not symmetric. Only #3 and #5 mirror (about x = 133.000
exactly). Each sits inside the 14.000 grid's web — #1, for instance, spans
X 14.290 … 18.790 in a web running 11.760 … 21.540.

#### The 4 unscrewed plate props — Ø4.000 OD, top at y = −1.209

| # | model X | model Z | deck x | deck y |
|---|---|---|---|---|
| 1 | 47.560 | 15.490 | −85.440 | −0.3025 |
| 2 | 99.750 | 53.500 | −33.250 | −38.3125 |
| 3 | 175.800 | 53.490 | +42.800 | −38.3025 |
| 4 | 209.000 | 34.490 | +76.000 | −19.3025 |

#### The 7 PCB bosses — Ø4.000 OD, top at y = −3.709

| # | model X | model Z | deck x | deck y |
|---|---|---|---|---|
| 1 | 38.115 | 37.949 | −94.885 | −22.7615 |
| 2 | 43.299 | 56.949 | −89.701 | −41.7615 |
| 3 | 71.300 | 75.949 | −61.700 | −60.7615 |
| 4 | 137.625 | 56.949 | +4.625 | −41.7615 |
| 5 | 146.736 | 75.949 | +13.736 | −60.7615 |
| 6 | 232.750 | 56.949 | +99.750 | −41.7615 |
| 7 | 253.395 | 87.949 | +120.395 | −72.7615 |

### 3.4 What supports the PCB across 282 mm — and it is almost nothing

**33 upward-facing planar faces at y = −3.709, totalling 261.350 mm²**
`[CAD-EXACT]`: the 7 cored boss annuli (10.297 mm² each, 72.079 total) and 26
ribs and ledges (189.271 total). The board's plan area is 22208.598 mm², so

```
261.350 / 22208.598 = 1.177 %        [DERIVED]
```

**Keychron supports a 282.211 × 93.211 PCB on 1.2 % of its area, at 33
points.** The largest single pad is 17.680 mm²; the smallest 2.803. They are
spread across the whole board, with the sparsest region the rear-centre where
the cell sits. The PCB is thin — 1.200, not 1.600 — so the support is doing
real work: a 1.2 mm FR-4 sheet spanning 282 mm unsupported would flex visibly.

The 26 non-boss pads, as X and Z ranges `[CAD-EXACT]`, every one at y = −3.709:

```
X  -8.601..  -5.301   Z  67.089.. 73.089      X 109.000.. 110.500  Z   9.050.. 12.050
X  -8.601..  -5.301   Z  36.089.. 42.089      X 109.175.. 110.175  Z  99.800..103.551
X  -8.301..  -3.501   Z  95.551.. 96.351      X 136.470.. 144.570  Z   9.050.. 16.849
X  -8.301..  -5.301   Z  13.550.. 20.149      X 167.150.. 167.950  Z  94.694.. 98.199
X  -4.100..   3.900   Z  75.543.. 77.047      X 174.350.. 175.150  Z  94.694.. 98.199
X  19.900..  27.900   Z  75.543.. 77.047      X 175.000.. 186.000  Z   9.850.. 12.850
X  63.601..  68.601   Z   9.850.. 14.850      X 233.395.. 241.395  Z  75.543.. 77.047
X  67.150..  67.950   Z  94.694.. 98.199      X 238.200.. 246.200  Z  56.543.. 58.047
X  74.350..  75.150   Z  94.694.. 98.199      X 240.500.. 248.500  Z  18.543.. 20.047
X  76.659..  82.659   Z  99.551..103.551      X 257.395.. 265.395  Z  75.543.. 77.047
                                              X 262.200.. 270.200  Z  56.543.. 58.047
                                              X 264.370.. 270.370  Z  10.149.. 12.950
                                              X 264.500.. 272.500  Z  18.543.. 20.047
                                              X 270.501.. 274.301  Z  96.551.. 97.351
                                              X 271.301.. 274.601  Z  66.289.. 72.289
                                              X 271.301.. 274.601  Z  32.289.. 38.289
```

**What our carrier must do to match.** Nothing here is hard; it is only easy to
get wrong.

1. **A continuous seat, not 33 pads.** A printed plate can offer a plane where
   an injection-moulded tray could only offer ribs. Put the whole seat at
   y = −3.700 wherever the PCB's underside is clear, and pocket only where it is
   not. That is strictly better support than the stock 1.2 %.
2. **The pocket under the switch field is the governing cut.** Every one of the
   61 hot-swap sockets reaches **2.091 mm below the seat**, in a 15.101 × 13.991
   footprint offset (−0.9635, +0.9545) from its key centre. On a 19.000 pitch
   the footprints do not touch — they leave **19.000 − 15.101 = 3.899** across
   and **19.000 − 13.991 = 5.009** along — so a per-key pocket is possible and
   leaves a lattice of webs. **Simpler and stronger: one pocket 2.100 deep
   covering the whole socket field, with the seat as a border and ribs on the
   19.000 lattice.** The field the pocket has to cover is
   X **−8.514 … 270.237**, Z **12.959 … 102.950** `[DERIVED from CAD]` — i.e.
   it runs 0.692 inside the PCB's left edge and 2.769 inside its right.
3. **Keep the seat at the board edge.** The stock design seats the board along
   its left and right edges (the pads at X −8.6 … −5.3 and 271.3 … 274.6) and
   along its front. Reproduce all three, continuously.
4. **Do not seat under the LED windows or the stabiliser clearances.** They are
   through-holes; a continuous seat behind them is harmless, but the stabiliser
   wires swing in the 5.400 × 8.160 and 8.100 × 4.000 openings and need
   clearance below the board — depth **NOT SOURCED** (§7).

### 3.5 The skirt, which is the real cost of stripping

The frame is one part. Keeping the plate means keeping the skirt:

| | value | tag |
|---|---|---|
| skirt wall | **1.000** (outer X = −10.500, inner X = −9.500; outer Z = 8.000, inner Z = 9.000) | `[CAD-EXACT]` |
| rebate at the rim | 0.400 deep × ~0.594 tall, where the tray's wall landed | `[CAD-EXACT]` |
| rim, rear | y = −12.295 → **8.595 below the PCB seat** | `[DERIVED]` |
| rim, front | y = −6.538 → **2.838 below the PCB seat** | `[DERIVED]` |
| rim plane | the same 3.37914° family | `[CAD-EXACT]` |

So the carrier needs a **perimeter groove on the 287.000 × 97.500 / R4.400
footprint, with an inclined floor, 8.595 deep at the rear and 2.838 at the
front**, or the skirt gets cut down. Two readings of that:

- **It is free registration.** A 1.000 mm wall dropped into a groove on the
  outer footprint locates the keyboard in X, Z and rotation with no added
  feature, to whatever the groove's fit is. The 7 screws then only have to pull
  it down, not position it. House fit for a sliding printed part is
  **0.2–0.3 per side** `[HOUSE]`; the repo's own bay fit is 1.35 total on width
  and 0.95 on depth `[REPO]`. For a groove taking a 1.000 mm metal rim, 0.250
  per side gives a 1.500 mm groove on the footprint — tight enough to register,
  loose enough to assemble.
- **It also means the typing angle is now ours.** The groove floor can be
  inclined 3.37914° to keep Keychron's angle, or flat for 0°. Keeping it costs
  **95.500 × 0.0590457 = 5.639 mm** of extra carrier depth at the rear
  `[DERIVED]`, which on a 304.600 × 190.375 desk slab is a real wedge. The deck
  as drawn is flat. **This is a design decision the enclosure task has to
  make, and it did not exist while the keyboard was mounted whole** — the stock
  case carried its own angle and sat flat (`layouts/README.md`, "no ramp is
  needed anywhere"). Strip it and that sentence stops being true.

### 3.6 Fastening the carrier, with the house standard

The stock boss is Ø4.000 outside, Ø1.700 bore, and the plate hole is 2.500
square. That constrains both ends.

| option | geometry | note |
|---|---|---|
| **reuse the stock screws** | print the boss Ø4.000 / Ø1.700 and self-tap | Ø1.700 bore in a Ø4.000 boss leaves a **1.150 wall**. House minimum for an M3 insert is 1.6 `[HOUSE]`, but that is for a 4.0 mm insert hole, not a 2.0 mm self-tapper. Printed PETG self-taps acceptably at this size. **Recommended for the first build** — zero new hardware and the screws are already the right length. |
| **M2 heat insert** | Ø3.200 × 4.000 deep `[HOUSE]` | will not fit a Ø4.000 boss: (4.000 − 3.200)/2 = **0.400 wall**. Needs the boss stepped: **Ø4.000 from y = −3.700 up to −1.209** (so it still passes the PCB's Ø5.000 holes), flaring to **Ø7.000 below y = −4.200**, which gives (7.000 − 3.200)/2 = 1.900 wall. Insert top at −4.200, so the screw is M2 × 10. |
| **M3 anything** | Ø4.000 insert hole `[HOUSE]` | impossible. The plate hole is 2.500 square; an M3 shank is 3.000. |

The stock screw's own size is **NOT SOURCED** (§7) — Ø1.700 is consistent with
a 2.0–2.3 mm self-tapper, and `research/keyboard-k9pro.md` §8.1 was right to
refuse to transfer the K10 Pro's KA2.3 figure.

**The 16 latches: reproduce the 12 along the rear and front runs as plain
bearing ribs, not snaps.** The frame's hooks are pockets; a printed tongue that
merely fills them stops the rim lifting between screws without asking a printed
snap to survive assembly. The 4 side latches are at x = −5.000 and x = 271.000,
inside the groove's own wall, and the groove does their job.

---

## 4. What is lost, and what must be re-provided

### 4.1 The good news first: the ports are not lost

All four rear features exit through **the top frame's own rear wall at
z = 8.000** — a part we keep. They are inner wires of that wall's single
planar face, measured without tessellation `[CAD-EXACT]`:

| aperture | size | model X | model Y | deck x | deck y |
|---|---|---|---|---|---|
| **USB-C** | **9.640 × 3.860** | 118.680 … 128.320, c **123.500** | −7.510 … −3.650 | −14.320 … −4.680 | +7.1875 |
| **mode toggle** (BT/OFF/Cable) | **9.100 × 3.500** | 23.950 … 33.050, c **28.500** | −7.250 … −3.750 | −109.050 … −99.950 | +7.1875 |
| **OS toggle** (Win ↔ Mac) | **7.300 × 3.500** | 5.850 … 13.150, c **9.500** | −7.250 … −3.750 | −127.150 … −119.850 | +7.1875 |
| **indicator** | **1.000 × 1.000** | 140.020 … 141.020, c **140.520** | −5.780 … −4.780 | +7.020 … +8.020 | +7.1875 |

The wall is **1.000** thick (outer face z = 8.000, inner face z = 9.000) and
the USB-C's mouth is at z = 9.492, so the receptacle sits **1.492** behind the
frame's outer face — 1.000 of aluminium and then 0.492 of air `[DERIVED]`. A
plug's overmould therefore has 1.492 of lead-in before it meets the shell, and
the carrier must not add any of its own in front of z = 8.000.

**This settles §8.3 and the first four rows of §13 of
`research/keyboard-k9pro.md` with measurements.** The manual's stylised drawing
put the OS toggle "above esc" and the mode toggle "above 1/2"; measured, the OS
toggle is centred at X 9.500 (esc is at 0, `1` at 19) and the mode toggle at
X 28.500 (between `1` and `2`). The drawing was right; it just had no scale.
The USB-C is centred at X 123.500, which is key-unit **7.000** exactly —
dead centre between `6` and `7`.

### 4.2 The parts behind those apertures, which we keep

| part | X (centre) | Y | Z | tag |
|---|---|---|---|---|
| `TYPE-C_16PIN` | 119.022 … 127.978 (**123.500**) | −6.868 … −2.492 | 9.492 … 17.088 | `[CAD]` |
| `2P_1_1` OS toggle | 5.544 … 13.456 (**9.500**) | −5.106 … −3.094 | 10.594 … 15.856 | `[CAD]` |
| `3P_1_1` mode toggle | 22.981 … 33.994 (**28.488**) | −5.106 … −3.194 | 10.594 … 15.806 | `[CAD]` |
| `PC_1_1` light pipe | 137.500 … 143.500 (**140.500**) | −6.300 … −1.200 | 9.000 … 10.000 | `[CAD]` |
| `CK8892_BUTTON` slider caps ×2 | 12.020 wide | **−8.260 … −1.490** | 6.890 … 11.710 | `[CAD]` |

The slider caps are drawn twice each, which gives the throw directly
`[DERIVED]`:

- **OS toggle, 2 positions**: cap centres X **8.725** and **10.300** — travel
  **1.575**.
- **Mode toggle, 3 positions**: cap centres X **26.988**, (28.488), **29.988**
  — **1.500** per step, the middle one being OFF and matching the switch body's
  own centre of 28.488.

Two things the carrier must notice about those caps:

1. **They reach y = −8.260**, which is **4.560 below the PCB seat** — deeper
   than the sockets, deeper than the USB-C. The carrier needs a local pocket
   that deep across X 2.715 … 35.998, Z 6.890 … 15.856.
2. **They stick out of the back of the keyboard.** Their rear face is at
   z = 6.890, which is **1.110 behind the frame's rear wall** `[DERIVED]`. In
   deck coordinates they reach **deck y = +8.2975**, while the keyboard
   pocket's rear edge is at **+7.6625** `[REPO]` — so they protrude **0.635 mm
   into the 3.200 mm spine** between the keyboard bay and the control band.
   The spine has 2.565 left. It will hold, but the relief has to be drawn, and
   a finger has to be able to reach OFF.

### 4.3 What is actually lost, with positions

Everything in `02-CK8876-BOT_ASM`, plus the feet:

| lost | what it did | what replaces it |
|---|---|---|
| `CK8876_K3_BOTTOM_43_20` the 2.000 mm ABS tray | the chassis: 18 bosses, 33 PCB pads, 13 plate pads, 16 latch tongues, the battery bay, the foot mounts | **the carrier.** §3.3, §3.4, §3.5 |
| `K9PRO-PLATE`, the 0.500 SECC shield | stiffened and shielded the board's underside; lay on the tray floor | nothing. It has nothing to lie on. Its 13 locating posts (Ø2.500/Ø1.000, positions below) go with it |
| `LEFT-PET` X −7.441…121.766, `RIGHT-PET` X 171.696…272.605, both Z 71.6…92.3, y −7.044…−5.716 | insulating films between the board's front underside and the tray | **a printed carrier is already an insulator** — but the seat under Z 71.6…92.3 must not be metal-filled filament, and no screw head may sit proud there |
| `BATTERY-3030130` 130.000 × 32.000 × 3.000 | — | retained; the bay is ours. §4.4 |
| the battery connector and leads | — | **NOT MODELLED.** §7 |
| `BIG_HOLDER` / `SMALL_HOLDER` ×2, the flip-out feet | the 5° and 8° typing angles | gone, and good: a deployed foot would foul any bay floor. One is drawn deployed in the model, reaching y = −22.100 |
| `FOOT_PAD_1` ×4, `FOOT_PAD_2` ×1, rubber | grip | gone |

The 16 SECC locating posts, recorded because they are the one set of tray
features a carrier should **not** reproduce — Ø2.500 OD with a Ø1.000 bore,
tops between y = −5.059 and −7.659 `[CAD-EXACT]`:

```
(  -0.000, 56.925) (   0.300, 85.949) (  40.300, 85.949) (  53.000, 26.949)
(  65.000, 56.925) (  75.300, 85.949) ( 107.800, 61.925) ( 113.300, 85.949)
( 167.800, 61.925) ( 185.300, 85.949) ( 197.800, 69.049) ( 212.000, 26.949)
( 216.300, 85.927) ( 225.800, 44.050) ( 263.300, 50.237) ( 266.300, 85.949)
```

### 4.4 The battery bay we have to build

The cell is **130.000 × 32.000 × 3.000** `[DERIVED]`, 1550 mAh `[VENDOR]`, and
in the stock keyboard it lies tilted in the floor plane, centred at X 140.000,
spanning Z 17.632 … 49.753, with its lowest corner 7.020 below the PCB seat
and its highest 2.139 below it `[CAD]`.

On a flat carrier the cell lies flat. Then it needs **3.000 plus a liner below
the PCB seat**, over a 130.000 × 32.000 footprint, and that fits inside the
depth the sockets already demand along most of the span — but **not** under the
rear two rows, where it would clash with the socket field. The stock bay sits at
Z 17.6 … 49.8, which is under switch rows 0 and 1 (Z 19.000 and 38.000), and the
stock design resolves it by dropping the cell 7.020 below the seat on the
sloped floor. On a flat carrier the cell must either go **below the socket
pocket's floor** (a bay 3.000 + liner deep beneath y = −5.791, i.e. its top at
or below −5.9, total depth from the seat ≈ 9.2) or **move**. It does not have to
stay where Keychron put it: nothing except the lead length constrains it, and
the lead length is **NOT SOURCED**.

Three things a cell bay owes a lithium pouch, and the house library has none of
them, so they are design notes rather than measurements:

- **no compression.** 0.300 clearance on thickness, 0.500 on plan.
- **no sharp edges, no screws through the bay**, and nothing that can be
  pinched when the carrier is closed.
- **swelling room.** A pouch cell gains thickness with age; 10 % of 3.000 is
  0.300. Budget 3.600 total.

---

## 5. The strip procedure, and the measurements to take

The keyboard is in hand. This is the short version: what to undo, in what
order, and the eleven calipered numbers that confirm or correct the CAD.
Nothing here asks for a measurement the CAD already settles.

### 5.1 Before anything

1. **Toggle to OFF** (mode switch to the middle position, X 28.500). Lithium
   cell, live board.
2. **Photograph the underside** with a ruler in frame, before any screw comes
   out. The foot-pad protrusion (§7) is settled by this photo plus one caliper.
3. Pull **all 61 keycaps** with a wire puller, straight up. They are LSA. A 1u
   cap is **18.200 square and 6.575 tall** (+3.000 … +9.575) `[CAD-EXACT]`,
   measured off `k9pro-keycap-1u-one.stl` and confirmed on all 46 1u bodies
   isolated out of `k9pro-stripped-assembly.stl`. The stem socket swallows
   9.700 − 6.700 = **3.000** of switch stem `[CAD-EXACT]` — they come off with
   less force than MX caps and the thin walls
   (1.4–1.6 `[VENDOR]`) split if levered.
   *The figures here were `18.731 square and 6.899 tall (+2.943 … +9.842)` and
   a 3.079 swallow. All four were `manifest.json` bbox padding, not cap: that
   manifest inflates every face of every shape by a per-shape amount, +0.2657
   for this cap. 18.731 is 0.531 too wide and 6.899 is 0.324 too tall. The
   caps are not all the same height either — the top is dished, so the bbox
   maximum rises with width, from 6.575 at 1u to 6.757 at the 6.25u spacebar.*

### 5.2 The disassembly

4. **Remove the 5 rubber foot pads** — two rear strips (X −3.5…2.5 and
   263.9…270.1, Z 13.2…34.2), two front strips (X −3.9…17.0 and 249.1…270.0,
   Z 92.5…98.6) and one front centre (X 128.0…138.0) `[CAD]`. There are
   **no screws under them** — the tray carries none on its outer face; the
   model has no bore reaching the outer surface anywhere. Keep them; they are
   the only thing that will ever stop the finished deck sliding.
5. **Back out the 7 plate screws**, from the top, through the square
   counterbores at the §3.3 positions. They are under keycaps, in the webs
   between switch cutouts — if a screw is not visible at one of those seven
   coordinates, stop and re-read §3.3 before putting a driver anywhere else.
   **Bag them.** The whole point of §3.3 is to reuse them.
6. **Release the 16 latches.** Six along the rear (x centres −0.3, 47.7, 98.7,
   149.7, 200.7, 251.7 at z = 13.5), six along the front at the same x
   (z = 100.0), and four on the sides (x = −5.0 at z = 31.3 and 87.2;
   x = 271.0 at z = 26.3 and 87.2) `[CAD-EXACT]`. Work a 0.5 mm shim in at the
   **rear corners first** — the rear latches sit 1.7 mm higher in the wall
   (y −5.700…−4.000) than the front ones (−3.700…−2.000), so the rear pair
   releases with less flex.
7. **Lift the tray straight off.** It takes the SECC shield, the two PET films
   and the feet with it. **The cell's lead is the only thing still connecting
   the halves.** Lift 10 mm, look, and find the lead before lifting further.
8. **Unplug the cell at its connector** — do not cut, do not pull the wires.
   Note its position and polarity now; this is the number the model does not
   have (§7).
9. **The PCB and frame stay together.** Do not separate them: the 61 switches
   are the clamp, and taking them out loses the only thing holding the board to
   the plate. If a switch has to come out (hot-swap, so it will), note that the
   board is **1.200 mm** thick and unsupported once the tray is off — support it
   under the socket you are pulling.

### 5.3 The eleven measurements that change the CAD

| # | measure | where | CAD says | why it matters |
|---|---|---|---|---|
| 1 | **the screw** — thread Ø, head Ø, head height, length, drive | the 7 bagged screws | pilot Ø1.700, head ≤ 4.500 square, 2.500 square shank hole | sets whether we self-tap at Ø1.700 or step the boss for an M2 insert (§3.6) |
| 2 | **PCB thickness** | any free edge, 3 places | **1.200** | if it is 1.6 the whole stack shifts 0.400 and the seat moves |
| 3 | **plate thickness** | at a switch cutout | **1.200** | same |
| 4 | **plate top to PCB top** | depth gauge in a switch cutout, cap and switch out | **2.500** | the one dimension that fixes the plate-PCB sandwich |
| 5 | **socket protrusion below the board** | depth gauge or caliper over a socket | **2.091** | the socket-field pocket depth, §3.4 item 2 |
| 6 | **the cell** — L × W × T, and the lead length and connector | the cell, unplugged | 130.000 × 32.000 × 3.000; lead **NOT SOURCED** | the bay, and whether the bay can move (§4.4) |
| 7 | **battery connector position and pitch on the PCB** | board underside | **NOT SOURCED** | the only unmodelled electrical interface |
| 8 | **skirt wall thickness and rim depth** | caliper on the frame's rim, rear and front | **1.000** wall; rim 8.595 / 2.838 below the seat | the groove, §3.5 — this is the number the whole carrier section depends on |
| 9 | **foot-pad protrusion** | pad bottom to tray floor, before removal | the model's pads sit 1.8–2.5 below the tray's outer plane, which does not reconcile with 7.6/13.3 | cross-check only, since the pads are discarded. If it reconciles, the model's pad placement is wrong; if not, the published heights are |
| 10 | **stabiliser swing depth below the board** | with a 2 u key fitted, depth below the board at a stabiliser hole | **NOT SOURCED** | §3.4 item 4 — a seat under a stabiliser jams a key |
| 11 | **keycap top above the frame** | height gauge on a 1 u cap in EACH of the five rows AND on the spacebar, frame top as datum | **+9.5753** at 1 u and **+9.7571** on the 6.25 u spacebar (mesh; analytic +9.5759 / +9.7572) `[CAD-EXACT]` / `[CAD]`; NOT flat — it rises with cap **width**, §2.1, and the vendor model has NO row variation at all (the 46 1 u bodies are one solid translated across four rows, which is what LSA means), so a gauge difference between rows would be a fact about the hardware that this repository does not contain | confirms the acrylic void's depth, and kills the 19.8 front figure for good. *This row said "+9.842, flat across all 5 rows"; both halves were wrong — the figure was padded and the plane is not flat. Gauge the spacebar too: it is the cap that governs the void.* |

Measurements 1, 5, 7, 8 and 10 are the ones that will change geometry. The rest
are confirmations, and if any of 2, 3, 4 or 11 disagrees with the CAD by more
than 0.1, stop and re-derive the stack — the CAD is Keychron's own and a
disagreement means we have the wrong revision.

---

## 6. The parametric stand-in

`v2/cad/parts/k9pro_internals.scad` — the stripped assembly as a reference
body, every number a named parameter carrying its source.

**Its frame is not the model's.** It uses the house OpenSCAD convention, with
the datum on the surface the carrier actually cuts to:

```
x = 0 at the keyboard's LEFT outer edge      x = model_x + 10.500      0 .. 287.000
y = 0 at the keyboard's FRONT outer edge     y = 105.500 - model_z     0 ..  97.500
z = 0 at the PCB UNDERSIDE, the seat         z = model_y + 3.700
```

so `z = 0` is the plane the carrier presents, `z = +3.700` is the aluminium
face, `z = +13.2753` is a 1 u keycap top, `z = +13.4571` is the spacebar's —
the highest cap in the assembly — and `z = −8.595` is the skirt's rear rim.

Parts: `all`, `plate`, `frame`, `pcb`, `sockets`, `switches`, `keycaps`,
`battery`, `ports`, `seat` (the 33 pads as the stock tray offered them),
`bosses` (the 18, colour-split by family), `envelope` (one solid to subtract).

Render and check:

```
openscad -o /tmp/k9.stl -D 'part="all"' v2/cad/parts/k9pro_internals.scad
```

### 6.1 Render verification

OpenSCAD **2021.01**. All twelve `part=` values were rendered; every one
produced a non-empty mesh, exit code 0, no warnings and no errors.

```
ECHO: "k9pro_internals: footprint 287 x 97.5  seat z=0  plate top z=3.7"
ECHO: "  keycap top z=13.4571  skirt rim rear z=-8.595 front z=-2.838"
ECHO: "  switch field 285 x 95 at 19 pitch ; 61 keys ; 7 screws"
ECHO: "  socket floor z=-2.091  USB-C floor z=-3.168  slider cap floor z=-4.56"
```

| `part=` | facets | bytes |
|---|---|---|
| `all` | **12 074** | 1 879 068 |
| `envelope` | 12 074 | 1 879 068 |
| `frame` | 8 272 | 1 304 239 |
| `plate` | 13 368 | 2 155 737 |
| `pcb` | 14 124 | 2 257 422 |
| `sockets` | 732 | 113 247 |
| `switches` | 2 684 | 417 615 |
| `keycaps` | 732 | 121 581 |
| `battery` | 12 | 1 935 |
| `ports` | 68 | 10 605 |
| `seat` | 3 000 | 538 545 |
| `bosses` | 6 072 | 1 060 421 |

`all` has fewer facets than `plate` alone because the union swallows the
cutout walls: the 61 switch bodies are 14.000 square and the cutouts are
14.000 square with R0.250 corners, so the switch fills the hole it passes
through and both sets of walls disappear. That is the right answer and it is
also a check — if `all` came out with *more* facets than its parts, something
would not be touching that should be.

The echoed extents are §2.2's, so the SCAD and this document cannot drift
apart without the echo saying so.

`part="envelope"` is the one to subtract from a carrier: the union of
everything retained, grown by `clearance` (default 0, so `envelope` and `all`
are byte-identical until a clearance is set), and nothing else.

Every coordinate table in the file — the five switch rows, the 10 stabiliser
slots, the 2 Caps-bracket slots, the 7 screws, the 4 props, the 7 PCB bosses,
the 26 seat pads, the 5 port bodies and the 4 apertures — was checked back
against the measured model values through the stated conversion. **0
mismatches**, and 61 keys.

---

## 7. NOT SOURCED

Every one of these is a real gap, with the procedure that closes it. None of
them blocks drawing the carrier; items 1, 2 and 3 block *finishing* it.

| # | unknown | why it matters | how to close it |
|---|---|---|---|
| 1 | **The battery connector** — type, pitch, position on the PCB, polarity | the only electrical interface between the two halves, and the thing that decides whether the cell can move off Keychron's Z 17.6…49.8 | §5.3 #7. The model has no connector and no lead. Nothing in the 253 PCB hole axes is a 2-pin footprint outside the USB-C cluster |
| 2 | **The battery lead length** | decides whether the bay can move at all | §5.3 #6 |
| 3 | **The stock screw's size and drive** | pick between self-tapping at Ø1.700 and an M2 insert in a stepped boss (§3.6) | §5.3 #1. Ø1.700 is consistent with a 2.0–2.3 self-tapper; the K10 Pro's KA2.3×8 is **not** transferable and is not quoted as if it were |
| 4 | **Stabiliser swing depth below the PCB** | a continuous seat under a stabiliser hole jams a 2 u key | §5.3 #10. The 10 × 5.400 × 8.160 and 9 × 8.100 × 4.000 PCB openings are measured; what moves through them is not |
| 5 | **Foot-pad protrusion below the tray floor** | cross-check on the published 7.6 / 13.3 only — the pads are discarded | §5.3 #9. The model's `FOOT_PAD_1` is one part used four times; its Y extent reads 3.235 at the front and 4.118 at the rear, which cannot both be right for one part, so one of the four placements is wrong in the model |
| 6 | **Whether the frame is aluminium all the way, or an aluminium face on a plastic carrier** | if the skirt is plastic it can be trimmed; if it is one aluminium part it cannot | one part, 23532.316 mm³, in the model `[CAD]` — the model does not carry materials. Scratch test and a magnet, or weigh the frame: 23532.316 mm³ is **63.5 g** in aluminium and **24.5 g** in ABS |
| 7 | **Whether the 2.500-square plate holes are moulded or machined, and their true corner radius** | only affects whether an M2 pan head can sit in the 4.500 square counterbore | caliper and a loupe. The 61 switch cutouts and 11 stabiliser slots are R0.250 (288 corner fillets, 72 × 4) `[CAD-EXACT]`; the screw holes' radius was not resolved |
| 8 | **Gateron's 12.2 mm switch height** | it is the figure `research/keyboard-k9pro.md` §5.2 built a comparison on | the model gives **11.653** housing-bottom to stem-top `[DERIVED]`. Either the 12.2 includes something this model does not, or it is wrong. Caliper a loose switch |
| 9 | **The two local PCB openings at (85.500, 15.500) and (180.500, 15.449)** | the board is confirmed open there, but the opening's shape was not extracted | they are not inner wires, so they merge with the board's outline — trace the outline, or look at the board |
| 10 | **Whether the 61 switches can be reseated after the plate has been off** | the PCB's only clamp | they are hot-swap `[VENDOR]`; try one before trusting sixty-one |

---

## 8. Reference geometry exported

`v2/parts/models/k9pro-internals/` — **gitignored** (`.gitignore:52`,
`v2/parts/models/`). Keychron Source-Available Licence: study and compatible
accessories are permitted, redistribution is not. **Reference only; never
committed.**

| file | what | solids |
|---|---|---|
| `k9pro-topcase-switchplate.stl` | `CK8876_TOP_84-WG` — **the frame and the plate, one part** | 1 |
| `k9pro-pcb.stl` | `K9PRO_PCB` | 2 |
| `k9pro-switch-gateron-one.stl` | one `GATERON-A_ASM` at the Esc position | 6 |
| `k9pro-hotswap-socket-one.stl` | one `KS-2P02B02-02-20200922` | 2 |
| `k9pro-keycap-1u-one.stl` | one 1 u LSA cap | 2 |
| `k9pro-battery-3030130.stl` | the cell, as placed | 1 |
| `k9pro-usbc-16pin.stl` | `TYPE-C_16PIN` | 1 |
| `k9pro-toggle-2pos-os.stl` | `2P_1_1` | 1 |
| `k9pro-toggle-3pos-mode.stl` | `3P_1_1` | 1 |
| `k9pro-indicator-lightpipe.stl` | `PC_1_1` | 1 |
| `k9pro-bottomcase.stl` | `CK8876_K3_BOTTOM_43_20` — **discarded, kept as the boss and pad reference** | 1 |
| `k9pro-shield-secc0p5.stl` | `K9PRO-PLATE` — discarded | 1 |
| `k9pro-pet-left.stl` / `-right.stl` | the insulating films — discarded | 1 each |
| `k9pro-stripped-assembly.stl` | **everything retained, in one mesh**: frame + 61 switches + 61 sockets + PCB + 61 keycaps + ports + cell. 66 MB, meshed at 0.40 / 1.2 rad | 129 pieces |
| `manifest.json` | each file's byte count, solid count and measured bounding box | |
| `tree.json`, `measured.json`, `detail.json`, `wires.json`, `cylinders.json`, `parts.json` | the extraction's own output: the 1343-node assembly walk, the post-mesh bounding boxes, the per-solid plane and cylinder census, and the exact wire extents every `[CAD-EXACT]` number came from | |

All meshes are in the **model** frame (y = 0 at the frame top), not the SCAD
frame. `k9pro_internals.scad` is the thing that is in the SCAD frame.

---

## 9. Documents used

**Keychron's own engineering artefacts**
- `/home/user/k9pro_full/K9-Pro-US-Full-Model_3d_20241128.stp` — 37.7 MB,
  84 named products, the source of every `[CAD]` and `[CAD-EXACT]` number here
- `/home/user/k9pro_cad/K9-Pro-Top-And-Bottom-Case.stp` — the two case halves
  alone; the full model supersedes it for this task and was used only to confirm
  that it carries no internals
- `v2/parts/models/keychron-k9pro/qmk-k9_pro-ansi-info.json` — `LAYOUT_61_ansi`,
  which the measured 19.000 grid reproduces exactly on every 1 u key
- `v2/parts/models/datasheets/keychron-K9-Pro-user-manual.pdf` — p10, the
  overview drawing whose port positions §4.1 confirms

**In this repo**
- `v2/research/keyboard-k9pro.md` §8.1, §8.3, §13, §14 — the case against
  stripping, the unknowns this document closes, and the bounding-box numbers
  this document corrects
- `v2/research/keyboard-k9pro-model.md` §4.2, §4.4, §5.1 — the wedge and
  keycap-height derivations, now measured
- `v2/layouts/_common.py`, `v2/layouts/slab.py`, `v2/tools/layout.py` — the deck
  identity, `KBD_FIT`, and the 285.75 × 95.25 field figure corrected in §1.6
- `v2/parts/stack.md` §5 — the carrier plate as the real design
- `v2/cad/keyboard_blank.scad` — the whole-keyboard stand-in this one
  complements, and whose `kc_top_front = 19.8` §2.1 retires

**House standard**
- `/home/user/kylebsmith/cmtl-3d-assets/README.md` — heat-insert holes
  (M2 3.2/4.0/1.3, M3 4.0/6.7/1.6), screw clearance dia + 0.3…0.4, sliding fits
  0.2…0.3 per side
- `.../08_cutouts/keyswitch_mx_plate_14mm.stl` — the house MX plate cutout,
  which is **exactly** the K9 Pro's 14.000 square. Use it; it already matches
- `.../08_cutouts/insert_M2_cnckitchen.stl`, `screw_clearance_M2.stl`,
  `usb_c_opening_10x4p2.stl` — the last of these is 10 × 4.2 against the
  frame's own 9.640 × 3.860 aperture, so it is **not** a drop-in here: the
  frame already carries the opening and we must not cut a second one
