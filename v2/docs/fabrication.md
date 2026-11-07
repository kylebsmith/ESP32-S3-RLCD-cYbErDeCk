# Fabrication rules — what the exported files must contain

*The document you take to a laser cutter and a printer, and the document
`v2/tools/export.py` and `v2/tools/check_cad.py` are written against.*

`v2/parts/stack.md` §3 established that 12.70 mm acrylic is cuttable and what it
costs. `v2/parts/knobs.md` §2 established the recessed bore. **This document does
not repeat either.** It takes their figures and turns them into rules with a
sign, a number and a face — the things a CAD exporter can obey and a checker can
measure.

Every number carries a source tag. Everything that could not be traced goes in
§5 with the procedure that closes it. Nothing here is estimated.

**Three things were measured today, from the artefacts and from the asset
library, and they are the reason several sections below disagree with
`v2/cad/parameters.scad`:** the exported DXF's actual contents, the exported
slab's actual webs, and the CNC Kitchen insert-cutout geometry.

---

## 0. Verdict up front

**Blocking — the files as exported today will not produce a working part.**

1. **The six M3 clearance holes close below Ø3.000 at the narrow face and an M3
   screw will not pass.** Ø3.400 drawn through 12.700 mm finishes **Ø2.927 at
   1.0° of taper and Ø2.483 at 2.0°**, worst case `[DERIVED §1.2]`. This is
   exactly `stack.md` §3.2's "hole that closes", which that document solved for
   M4 by drawing Ø5.60 — and the fix was lost when the design moved to M3.
   **Draw Ø4.445 (7 × 0.635).** §1.2, §3.

2. **The six holes are also round, and `stack.md` §6.1 requires one round datum
   and five slots.** PMMA's heat-and-moisture allowance is 5 mm/m `[RÖHM gsxt]`
   = **1.448 mm across the 289.560 mm screw span**, and the printed carrier's
   own boss-pitch error at Protolabs' ±0.5 % is **±1.448 mm** on the same span
   `[SVC protolabs]` `[DERIVED]`. Two independent terms, both ±1.5 mm, against
   zero float. **One Ø4.445 round hole, five 4.445 × 7.445 slots.** §3.

3. **`parameters.scad`'s M2.5 insert bore is the M2 row and is wrong.** It reads
   `insert_m25_hole_d = 3.200`, `insert_m25_depth = 4.000`. The house library has
   **no M2.5 row** — but it ships the insert as a solid, and it measures
   **Ø4.600 × 4.000**, the same knurl diameter as the M3 insert `[MEASURED
   07_hardware/heat-insert_M2p5_4.6x4.stl]`. A Ø4.600 knurl into a Ø3.200 bore is
   1.400 mm of diametral interference against the M3 case's 0.600. **M2.5 takes
   Ø4.000 × 5.000, min wall 1.600** — and two independent vendors already say
   4.000 `[3P ruthex-3djake]` `[3P insertsdirect]` (`stack.md` §5.4). §2.3.

4. **The keyboard void's Y clearance does not survive the taper.** The retained
   top frame is **287.000 × 97.500, corner R4.400** `[CAD-EXACT, keyboard-strip.md
   §1.4]`. A void drawn 98.450 finishes **97.533 at the narrow face** at 2.0° —
   **0.017 mm per side** `[DERIVED §1.2]`. If the frame passes through the slab,
   the void must be drawn **288.925 × 99.695** (455 × 0.635 and 157 × 0.635). If
   only the keycaps pass — 284.000 × 94.000 at the 19.000 grid `[DERIVED from
   keyboard-strip.md §1.6]` — the drawn 288.350 × 98.450 clears by 1.717 per side
   and is fine. **Which of the two is true is not in `parameters.scad`: the
   keyboard's Z seat has never been written down.** §1.2, §5.1.

5. **The exported DXF has no HEADER section, no TABLES section, no units and no
   layers.** OpenSCAD 2021.01 writes an R12 file containing nothing but **970
   bare `LINE` entities on layer "0"** `[MEASURED v2/export/dxf/slab.dxf]`. A
   DXF with no `$INSUNITS` is interpreted as drawing units by the receiver; the
   classic outcome is a part cut at 1/25.4 scale. **The exporter must
   post-process with ezdxf to write `$INSUNITS = 4` and `$MEASUREMENT = 1`, or
   the checker must fail the file.** §1.6, §4.1.

**Not blocking, but decided here so the CAD stops having to decide.**

6. **The drawing is nominal and it is deliberately kerf-agnostic.** `slab.scad`'s
   header already states "drawn at NOMINAL; the cutter compensates". The one
   primary source we have says the opposite happens by default — Ponoko:
   "**kerf is centered on cutting lines**" `[SVC ponoko-kerf]`. So the rule is
   not "trust the service". The rule is: **every feature in the acrylic is drawn
   so that an uncompensated 0.200 mm kerf moves it in a don't-care direction**,
   and §1.1 shows, feature by feature, that it does. That property is what makes
   the file safe to send to a service whose compensation behaviour you do not
   know, and it is only available because `knobs.md` §12 took every load and
   every fit out of the acrylic.

7. **Cut visible face up, and the bore is nominal at that face. Every other
   through-feature is nominal at the narrow face.** The bore can afford it — a
   Ø22.000 bore finishes Ø21.083 worst case against a Ø18.000 knob body,
   **1.542 mm of radial clearance** against the 1.193 mm worst-case stack-up
   `[knobs.md §2.1]` `[DERIVED]`. Nothing else can afford it, because everything
   else is something passing through a hole. §1.2, §1.3.

8. **The slab breaks the minimum-web rule in thirteen places and the narrowest
   is 4.400 mm.** Measured from the exported DXF, not from the source: against
   `[3P fab-guides]`'s ≥ 1 × *t* = 12.700, **thirteen pairs of cut contours are
   closer than that**, the front rail at **4.400** being 0.346 × *t* `[MEASURED
   §1.5]`. This is a real property of a frame with a 288.350 × 98.450 void in a
   304.600 × 190.375 sheet and it cannot be designed away. It is also three
   separate SendCutSend DFM rules, so **the automated-quote route is closed and
   this part goes to a fabricator who reads a drawing.** §1.4, §1.5.

9. **At 12.70 mm, SendCutSend's hole-to-edge rule and Röhm's invert.** At 3 mm
   `acrylic.md` §5.3 called SendCutSend's "1.5 × material thickness" the *looser*
   rule and chose Röhm's. At 12.700 mm SendCutSend's becomes **19.050 mm from
   any edge**, which no fastener in a deck with an 8.125 mm wall can ever meet,
   while Röhm's 1.5 × hole Ø gives **6.668 mm for a Ø4.445 hole** against the
   7.520 mm we have. **Röhm's is the rule of record.** §1.4.

10. **$fn = 64 is not quite enough, and the checker will miss why.** Every circle
    exports as a 64-gon whose **vertices sit on the nominal circle**: the Ø22.000
    bore's vertex diameter is 22.0001 but its **inscribed diameter is 21.9735**
    `[MEASURED]`. A check that measures vertex radii reads 22.000 and passes. The
    material is 0.013 mm proud at every chord. **Measure the inscribed diameter,
    and set $fn = 96** (sagitta 0.006 at r = 11.000). §4.1.

11. **The acrylic, the carrier and the back panel are all drawn 304.600 × 190.375
    — flush on all four sides.** The acrylic holds ±0.230 `[SVC sendcutsend]`; the
    printed carrier holds ±0.5 % = **±1.523 mm on 304.600** `[SVC protolabs]`.
    A flush edge will mismatch by up to 1.753 mm, visibly, and sometimes the wrong
    way. **Draw the carrier 1.905 mm (3 × 0.635) inboard all round** so the
    acrylic always overhangs and the tolerance becomes the reveal `[acrylic.md
    §6.3's rule, applied]`. §3.

12. **The bow the design can absorb is 0.770 mm, and it is measured at the
    display.** The acrylic's underside clears the display module by **1.270 mm**
    `[knobs.md §1.1]`, which already has to absorb ±0.500 mm of FDM pocket depth
    `[SVC protolabs]`. **0.770 mm is left for the slab's flatness and the
    carrier's bow combined**, and the 12.70 mm slab is far the stiffer of the
    two, so it is the datum and the carrier owns the 0.770. §2.5.

13. **The carrier is pulled flat at six points and they are all in one place.**
    The six slab screws are two columns 289.560 mm apart, three per column over a
    63.500 mm span — **nothing fastens the middle 289.560 mm of either long
    edge**, front or back `[DERIVED from parameters.scad]`. That span, not the
    material, is the argument for ribs. §2.5, §2.6.

14. **The back panel does not have the taper problem, and that is the proof this
    is a thickness problem.** The same Ø3.400 hole through 3.000 mm finishes
    **Ø3.160 worst case** and an M3 passes `[DERIVED]`. Nothing about the hole is
    wrong; 12.700 mm is.

---

## 0.1 Source tags

| tag | means |
|---|---|
| `[MEASURED]` | measured **today**, by me, from the artefact named — an exported DXF/STL in `v2/export`, or an STL in the asset library. Reproducible with the commands in §6.2. |
| `[CAD-EXACT]` | read off an analytic surface in a vendor STEP, as `keyboard-strip.md` §0.1 defines it. |
| `[STEP]` | measured from a vendor model; the numbers live in `parts.md` §1. |
| `[HOUSE]` | from `/home/user/kylebsmith/cmtl-3d-assets` — its README's own tables, or its own geometry. |
| `[DS …]` | a component datasheet, with page. |
| `[RÖHM …]`, `[TROTEC]` | material maker's or laser maker's own document, as cited and quoted in `acrylic.md` §8.4 and `stack.md` §6.4. |
| `[SVC …]` | a fabrication service's published specification. |
| `[3P …]` | third party — vendor blog, distributor, community practice. Weakest; always flagged. |
| `[DERIVED]` | arithmetic on the above. The arithmetic is shown. |
| `[REPO]` | another document in this repository, named with its section. |

---

# 1. The laser: what the exported vector files must contain

## 1.1 Kerf — nominal, uncompensated, and kerf-agnostic by design

**The decision: outlines are drawn at nominal. We do not pre-compensate. And we
do not depend on the service compensating either.**

The temptation is to pick one and assume it, which is how you double the error.
What we actually know:

| | |
|---|---|
| kerf, 3 mm clear acrylic | **0.200 mm**, and "**kerf is centered on cutting lines**" `[SVC ponoko-kerf]` |
| kerf, general | "starts at about 0.2 mm", widens with thickness `[SVC ponoko-kerf]` |
| kerf, 3 mm / 5 mm acrylic | 0.105 / 0.170 mm `[3P sculpteo]` |
| machine tolerance | ±0.130 mm `[SVC ponoko-kerf]`, ±0.230 mm acrylic `[SVC sendcutsend]` |

Centred kerf, uncompensated, means a **hole finishes one kerf larger** and an
**outside profile one kerf smaller** `[SVC ponoko-kerf]`. So each feature class
has a known sign, and the design question is whether that sign is harmful:

| feature class | drawn | if uncompensated | sign | does it matter? |
|---|---|---|---|---|
| **outside profile** | 304.600 × 190.375 | 304.400 × 190.175 | **−0.200** per overall dimension | **No.** The slab bears on the carrier's rim; it is not a fit. With the 1.905 reveal of §3 it is invisible. |
| **keyboard void** | 288.350 × 98.450 | 288.550 × 98.650 | **+0.200** | **No — and it is the helpful direction**, which is the only thing saving §0.4 at 1.0° of taper. |
| **Ø22.000 knob bore** | 22.000 | 22.200 | **+0.200** | **No.** The annular reveal goes 2.000 → 2.100 radial. All six move together, so they still match; the bore wall moves R11.000 → R11.100 and the NeoPixel at R9.21–11.61 still straddles it `[knobs.md §2.4]`. |
| **M3 fastener hole** | 4.445 | 4.645 | **+0.200** | **No, in this direction** — the screw only has to pass. The hole is sized by taper (§1.2), not by kerf, and the head needs a washer either way (§3). |
| **fader slot** | 12.000 × 52.000 | 12.200 × 52.200 | **+0.200** | **No.** The 10.000 cap's gap goes 1.000 → 1.100 per side. |

**Every class is in a don't-care direction.** That is the rule, and it is a
design property, not luck: `knobs.md` §12 removed every load path and every fit
from the acrylic, so there is no feature left whose size has to be right.

**What the order must say, verbatim:** *"Drawing is nominal finished size. Kerf
is not compensated in the file. If your machine does not offset the beam, cut on
the line."* Both outcomes are acceptable and §3's table carries both columns.

**What the checker must not do:** it must not try to verify kerf. Kerf is not in
the file. The checker verifies that the file is nominal — §4.1 item 9.

## 1.2 Taper at 12.70 mm — which face is nominal, feature by feature

`stack.md` §3.2 gives the magnitudes. Restated at the three angles:

| taper | per side | across a diameter |
|---|---|---|
| 1.0° | 0.222 | **0.443** |
| 1.5° | 0.333 | **0.665** |
| 2.0° | 0.443 | **0.887** |

`[DERIVED, t · tan θ, t = 12.700]`

The magnitude is community practice `[3P taper]`, supported in direction but not
magnitude by `[3P ump]`, and it is **still not measured** — `stack.md` §7.16 and
`acrylic.md` §11.4 both carry it, and it is carried forward again in §5.3.
**At 12.70 mm it decides whether a hole exists, so it is a blocking unknown and
the coupon of §1.7 is the only thing that closes it.**

**The direction.** Trotec: "Above 0.25″ (6 mm) **defocus 1/3 into the
material**" `[TROTEC]`, so the beam waist is 4.3 mm below the top face and the
kerf opens toward the entry face. With the slab cut visible-face-up (§1.3) the
hole is **wide at the top, narrow at the bottom**. `acrylic.md` §4.3 is right
that the direction "can go either way" and that the only safe instruction is
procedural; the geometry above is the prediction, not the measurement.

**So the governing face is the NARROW one for everything that has to pass
through, and the WIDE one only where the visible dimension is the design.**

Worst case throughout: uncompensated kerf **+0.200**, machine **−0.230**, taper
**−0.887** (2.0°). That is `narrow = drawn + 0.200 − 0.230 − 0.887 = drawn −
0.917`, and at 1.0° `drawn − 0.473`.

| feature | nominal at | drawn | narrow face, 1.0° / 2.0° | must clear | per side at 2.0° |
|---|---|---|---|---|---|
| **Ø22.000 knob bore** | **wide (visible) face** | 22.000 | 21.527 / **21.083** | Ø18.000 knob body `[knobs.md §2.3]` | **+1.542** ✔ |
| **M3 fastener hole, as drawn today** | — | **3.400** | 2.927 / **2.483** | Ø3.000 M3 shank | **−0.258 ✘ FAILS** |
| **M3 fastener hole, required** | **narrow face** | **4.445** | 3.972 / **3.528** | Ø3.000 + 0.300 `[HOUSE]` | **+0.264** ✔ |
| **keyboard void X, as drawn** | **narrow face** | 288.350 | 287.877 / **287.433** | 287.000 frame `[CAD-EXACT]` | **+0.217** marginal |
| **keyboard void Y, as drawn** | **narrow face** | 98.450 | 97.977 / **97.533** | 97.500 frame `[CAD-EXACT]` | **+0.017 ✘ zero** |
| **keyboard void, required** | **narrow face** | **288.925 × 99.695** | — | 287.000 × 97.500 + 0.800 fit | ✔ |
| **fader slot, 12.000 wide** | wide face | 12.000 | 11.527 / **11.083** | 10.000 printed cap | **+0.542** ✔ |

`[DERIVED]`. The 0.800 mm total fit for the keyboard void is `acrylic.md`'s own
figure for a laser-cut aperture over a hard part — *"A 0.25 mm per-side gap is
inside the laser's error alone and will sometimes not go in. 0.40 mm per side
survives the laser"* `[acrylic.md §4.5 commentary]`.

**And the Ø22.000 bore has a second, better reason to be nominal at the wide
face**, which `knobs.md` §2.5 already found: cut visible-face-up and the bore's
cone **opens toward the viewer** — you look down a flared well. Cut it the other
way up and the wall becomes a conical mirror showing a smeared ring of the
carrier plate. Same part, same bore, opposite result, decided by which way up it
goes on the bed.

**If the void has to grow, grow it rearward only.** The front rail is 4.400 mm
and it is the governing web of the whole part (§1.5). Growing the void
symmetrically takes it to 4.025. Keeping the void's front edge at y = −90.788
and taking all 1.245 mm of Y growth off the rear leaves the front rail at 4.400
and the rear land at 87.150 `[DERIVED]`. The layout thread owns the move; this
document owns the constraint.

## 1.3 Which way up, and how the file says so

**Visible face up.** Two independent reasons, both already in the repo:
the micro-chamfer lands on the edge you look at `[acrylic.md §4.3]`, and the
bore's cone opens toward the viewer `[knobs.md §2.5]`.

For the slab, "visible face up" is the face at **z = acrylic_t** in
`slab.scad`'s extrusion, i.e. the +Z face.

**What the file has to carry, and does not.** A flat vector file has no front and
no back; handedness is a convention the receiver guesses. Three things close it:

1. **The DXF is authored in model coordinates and must stay there.** Verified:
   the keyboard void's centre is at **y = −41.563** in `slab.dxf`, matching
   `kbd_top_y − kbd_pocket[1]/2 = −41.5625` `[MEASURED]`. Not mirrored.
2. **The SVG is Y-mirrored and the DXF is not.** OpenSCAD's SVG writer flips Y:
   the same void centre appears at **y = +41.563**, and the viewBox runs
   `-153 -96 306 192` `[MEASURED]`. For *this* part the mirror is harmless
   **only because the slab is symmetric about x = 0**, which makes a Y-mirror
   equal to a 180° rotation. **The moment one asymmetric feature appears — an
   engraved mark, a port relief, the Feather's HDMI notch — the SVG becomes a
   true mirror of the part.** The checker must compare the two files' handedness
   on every export (§4.2 item 4), not assume it.
3. **An asymmetric fiducial, cut on the job.** The coupon (§1.7) carries a
   3.000 mm "F" cut as a through-slot. If it comes back readable, the sheet was
   cut face-up as drawn.

**Nothing is counterbored.** A laser cannot counterbore 12.70 mm `[stack.md
§6.1]`, so the M3 button heads are proud and owned, and the only acrylic edge
treatment is by hand: **break the bores' top arris 0.300–0.500 mm with a
deburring blade, two passes, no sanding** `[knobs.md §2.5]`. That is a bench
operation, not a file feature, and it must not be modelled — a chamfer in the
STL would break the prismatic check of §4.3 item 6.

## 1.4 Minimum feature, minimum web, hole-to-edge — the three rules reconciled

Three sources, three rules, and at 12.70 mm they do not agree. Resolved in order
of authority:

| rule | source | at *t* = 12.700 | status |
|---|---|---|---|
| minimum hole Ø = 0.5 – 1.5 × *t* | `[3P fab-guides]` | **6.350 – 19.050** | **Advisory.** Ø22.000 is *above* the band, which `knobs.md` §2.1 correctly calls the safe side. The Ø4.445 fastener hole is **below** it and we are cutting it anyway — see below. |
| minimum web / gap ≥ 1 × *t* | `[3P fab-guides]` | **≥ 12.700** | **Advisory, and broken thirteen times.** §1.5. |
| minimum feature width ≥ 1 × *t* | `[3P fab-guides]` | **≥ 12.700** | **Broken.** The front rail is 4.400 and the side rails 8.125. §1.5. |
| hole **centre** to sheet edge ≥ 1.5 × hole Ø | **`[RÖHM acrylite-kb]`**, quoted verbatim in `acrylic.md` §5.3 | **6.668 for Ø4.445** | **THE RULE OF RECORD.** Material maker, primary, quoted. |
| hole to edge ≥ 1.5 × *t* | `[SVC sendcutsend]` | **19.050** | **Rejected, explicitly.** |
| internal corner fillet 0.500 – 1.300 | `[SVC sendcutsend]` | **≥ 0.500** | **Adopted as a floor.** `corner_r = 2.000` clears it. |
| predrill / chamfer recess corners | `[RÖHM mach §2.4, §3.1]` | — | **Adopted:** fillet every internal corner. A sharp internal corner in acrylic is a designed-in crack `[acrylic.md §4.4]`. |

**Why Röhm's and not SendCutSend's, in one line:** `acrylic.md` §5.3 chose
Röhm's at 3 mm on the grounds that SendCutSend's was *looser* and was "a
service's DFM minimum, not a material limit". **At 12.700 mm the two invert** —
SendCutSend's becomes 19.050 and Röhm's stays proportional to the hole — but the
reasoning does not: one is a property of PMMA around a hole, the other is a
quoting-engine threshold. The conclusion is unchanged and the arithmetic that
produced it is now the opposite way round, which is worth saying out loud.

**The rule of record, applied:**

```
hole centre to any edge, any window, any other cut   >=  1.5 x hole diameter
                                                         [ROHM acrylite-kb]
   O 4.445 fastener  ->  6.668     have 7.520 at x = +/-144.780   OK
   O22.000 bore      -> 33.000     have 38.100 pitch              OK
```

`[DERIVED]`. And the conservative reading the repo's own check currently
uses — *land* (hole wall to edge) ≥ 1.5 × Ø, which is Röhm's rule plus half a
diameter — **fails at Ø4.445**: land 5.298 against 6.668. Keep reporting the
land as a margin; do not gate on it. If anyone prefers to gate on it, the fix is
`screw_x = 142.875` (= field/2, 225 × 0.635), which gives a land of 7.203 and
still clears the fader board by 1.905 mm at the boss `[DERIVED]`.

**Where we knowingly miss a rule, we miss it deliberately** — the same discipline
`acrylic.md` §4.6 applied to the MX cutout. A Ø4.445 hole is 0.350 × *t*,
well under the 6.350 minimum-hole guidance, and it will cut: it is 1.7 × the
0.200 mm kerf and the coupon's Ø7.80/8.20/8.60 bores already prove the machine
at this thickness. What it will not do is pass an automated DFM check.

## 1.5 The webs this part actually has — measured from the export

Measured on `v2/export/dxf/slab.dxf` by stitching its 970 `LINE` entities into
closed loops and taking the **boundary-to-boundary distance between every pair of
distinct loops** `[MEASURED]`:

| distance | between | vs 12.700 |
|---|---|---|
| **4.400** | outline ↔ keyboard void (**the front rail**) | 0.346 × *t* |
| **5.820** | outline ↔ each of the six M3 holes | 0.458 × *t* |
| **8.125** | outline ↔ keyboard void (**the side rails** — the same contour pair as the 4.400, measured at a different place) | 0.640 × *t* |
| **8.142** | keyboard void ↔ the two lower M3 holes | 0.641 × *t* |
| **10.715** | fader slot ↔ the middle right M3 hole | 0.844 × *t* |
| **11.250** | keyboard void ↔ the three lower-row bores | 0.886 × *t* |
| 14.147 | fader slot ↔ upper/lower right M3 holes | ✔ |
| 15.300 | keyboard void ↔ fader slot | ✔ |
| **16.100** | bore ↔ bore (= 38.100 − 22.000) | ✔ |
| 16.175 | outline ↔ the three upper-row bores | ✔ |

**Thirteen pairs below 1 × *t*.** `slab.py`'s own comment already names the
condition — *"that makes the frame around the void thin — 8.125 at the sides,
4.40 at the front — so no fastener can live there"* — and the front rail's
answer is already written: *"The front rail is not a cantilever: it rests on the
carrier's own front wall for its full length."* That is the right answer, and it
is a **structural** answer, not a fabrication one. The fabrication consequences
are separate and are:

1. **The part is fragile between cutting and assembly, not after.** The 4.400 mm
   rail spans 288.350 mm unsupported on the cutting bed. Specify a **lamella
   cutting table** — Trotec already require one above 6 mm `[TROTEC]` — and
   specify **film masking left on both faces until the slab is on the carrier** — and if the sheet arrives paper-backed, the top paper must come off before cutting, because "the paper contains **paraffin, which causes flaming and reduces the cutting edge quality**" `[TROTEC]`
   (`stack.md` §6.2 step 11 already sequences it), because the masking is what
   stops the rail flexing when the part is lifted off the bed.
2. **Cut the perimeter last.** A conventional requirement, stated here because
   nothing in the exported file communicates it: all internal contours first,
   the outline last, so the part is held by the sheet while the void is cut.
   This is an **order-of-operations note on the order**, not a file property, and
   §4.1 item 3's layer convention is how you make it machine-readable.
3. **The automated-quote route is closed.** Three distinct SendCutSend DFM rules
   are broken (hole-to-edge 1.5 × *t*; minimum web 1 × *t* at thirteen contour pairs;
   minimum feature 1 × *t* at both rails). **Go to Visiontek — "1 mm to 15 mm"
   and "a polished edge without flame or diamond polishing" `[3P visiontek]` —
   or to a local fabricator who reads a drawing** `[stack.md §3.5]`.
4. **The void's internal corners carry the load.** `corner_r = 2.000` clears the
   0.500 mm floor `[SVC sendcutsend]` but it is the one internal corner adjacent
   to a 4.400 mm web, and Röhm's notch-effect warning is aimed exactly here
   `[RÖHM mach §2.4]`. **Take the void's two front corners to the largest radius
   the keyboard's own R4.400 frame corner allows** — a part corner rounder than
   its aperture always fits, so the aperture may go to R4.400 and no further
   without clipping the frame `[CAD-EXACT, keyboard-strip.md §1.4]` `[DERIVED]`.

## 1.6 File format — what real services want, and what our files actually are

**What is sourced, and it is less than you would like.** `acrylic.md` §8.1
records that **Ponoko's catalogue page does not state accepted formats** (§11.7,
still open) and **SendCutSend's acrylic material page does not either** (§11.8).
What is published:

| service | format | units | layers / colours |
|---|---|---|---|
| **Schaeffer / Front Panel Express** | "all standard file formats"; free Front Panel Designer; **DXF import** `[SVC schaeffer]` | not published | not published |
| **a local acrylic fabricator** | **DXF / AI** `[acrylic.md §8.1]` | theirs | theirs |
| **Ponoko** | "vector"; formats not stated `[3P ponoko-cat]` | — | — |
| **SendCutSend** | not stated on the material page `[SVC sendcutsend]` | — | — |
| **JLCPCB** (the FR4 escape hatch) | **Gerber**; outline ±0.200 `[SVC jlcpcb]` | — | — |

**So: send DXF as the cut file and SVG as the human-readable one.** DXF is the
only format with a published acceptance anywhere in the chain (Schaeffer, local
fabricators), and it is the format a cutter's CAM reads natively. SVG rides
along for the quote email and for the repo's own review, not for the machine.
**Any requirement beyond that is NOT SOURCED (§5.4) and the answer is a quote,
not a guess.**

**What our two files actually are, measured today:**

| | `v2/export/dxf/slab.dxf` | `v2/export/svg/slab.svg` |
|---|---|---|
| version / form | **R12 (AC1009)** | SVG 1.1 |
| sections present | **BLOCKS (empty), ENTITIES. That is all.** | — |
| **HEADER** | **absent — no `$INSUNITS`, no `$MEASUREMENT`, no `$LUNITS`** | — |
| **TABLES / LAYER table** | **absent** | — |
| entities | **970 × `LINE`**, nothing else | **one `<path>`, 15 subpaths, 15 `z`** |
| layer / colour | all on layer `"0"`, colour `256` (BYLAYER) | one colour |
| units | **none declared** | `width="306mm"`, `viewBox="-153 -96 306 192"` → **1 user unit = 1.000 mm** ✔ |
| arcs / circles | **none — every circle is a 64-segment `LINE` chain** | polyline chords, same |
| coordinate precision | 6 significant figures | 6 significant figures |
| handedness | **model coordinates** ✔ | **Y-mirrored** (§1.3) |
| stroke / fill | n/a | **`fill="lightgray"`, `stroke-width="0.5"`** |

`[MEASURED, both files, with ezdxf 1.4.5 and by reading the raw text]`

**Four things the exporter must fix, and they are all post-processing:**

1. **Write a HEADER with `$INSUNITS = 4` (millimetres) and `$MEASUREMENT = 1`
   (metric).** This is the single highest-consequence item in the document: a
   unitless DXF is the standard way to receive a part at 1/25.4 scale. ezdxf can
   open the R12 file, set the header variables and rewrite it as R2000.
2. **Write a LAYER table and put every entity on a named layer.** Our convention,
   declared here so the checker can enforce it and the order can reference it:

   | layer | colour (ACI) | means |
   |---|---|---|
   | `CUT_OUTER` | **1** (red) | the outside profile. **Cut last.** |
   | `CUT_INNER` | **3** (green) | every internal through-contour: void, bores, slots, fastener holes. **Cut first.** |
   | `ENGRAVE` | **5** (blue) | raster, if any ever exists. There is none today — the panel carries no engraving at all `[slab.scad]`. |
   | `SCORE` | **6** (magenta) | vector score, if any ever exists. None today. |

   Red/green/blue for cut/engrave/score is a widely used convention, but **it is a
   convention, not a published requirement of any service we have read — NOT
   SOURCED (§5.4).** Its value here is that it is
   *declared*, so the order can say "red is the outer profile, cut it last" and
   the checker can verify the file matches what the order says.
3. **In the SVG: `fill="none"` and a hairline stroke.** A filled path is how you
   ask for a raster engrave by accident. `stroke-width="0.5"` additionally puts a
   0.250 mm ambiguity on where the cut line is for any front end that cuts the
   stroke's edge rather than its centre.
4. **Either un-mirror the SVG or label it.** §1.3 item 2.

**One thing that is fine as it is:** the DXF having no arcs. A 64-gon at $fn = 96
is inside 0.006 mm of the true circle (§4.1 item 10) and no laser's motion
control cares. Arcs would be nicer to read; they are not worth a custom writer.

## 1.7 The coupon — `stack.md` §6.3, amended

`stack.md` §6.3 specifies the coupon and it closes five open items for ~500 mm of
cut length. **Three amendments, from what this document found:**

```
   as specified in stack.md 6.3:
   - a 25.00 mm square               -> kerf and taper, both faces
   - bores at O7.80 / O8.20 / O8.60  -> which passes an M7x0.75 bushing
   - one O25.40 well                 -> finished diameter and edge quality
   - a 12.70 mm web between two wells
   - a 2.50 x 80 mm slot             -> can the machine hold it at this thickness
   - 50 mm of outline                -> the edge finish you are buying

   ADD:
   - measure the 25 mm square at the TOP, at MID-DEPTH and at the BOTTOM, not
     just the two faces. Trotec's "defocus 1/3 into the material" [TROTEC] puts
     the beam waist 4.3 mm down, which predicts an HOURGLASS, not a cone. Two
     face measurements cannot tell a cone from an hourglass, and the difference
     decides whether a bore's narrowest point is its exit or its middle.
   - bores at O4.00 / O4.445 / O5.08  -> which passes an M3 at this thickness.
     This is section 0.1's whole question and nothing else answers it.
   - a 4.400 x 60 mm rail with a filleted internal corner at each end
     -> whether the governing web of section 1.5 survives being cut and lifted.
   - a 3.000 mm "F" cut as a through-slot -> reads correctly only if the sheet
     was cut face-up as drawn. Section 1.3 item 3.
                                                                    [DERIVED]
```

Cut the coupon **from the same sheet on the same job** as the slab, and keep the
offcut: §5 needs it.

---

# 2. The print: the carrier plate

## 2.1 Orientation, and what it does to the channels

**Visible face up, flat on the bed.** `stack.md` §5.5 settles it and gives the
reason — ironing cannot rescue an angled surface, the fix is orientation, and
printing face-*down* would turn the channels into unsupported voids off the bed.

**What that buys, stated as geometry rather than as advice:** with the channels
cut *into* the up-facing surface, **every channel is an open groove printed from
the bed upward and there is no overhang anywhere in the wiring plane.** The
channel floors at Z = −3.000 are flat tops; the walls are vertical; nothing
hangs. That is unusual and it is worth protecting, because it means:

1. **The only unsupported geometry in the entire part is the crossing bridges.**
   `stack.md` §5.1: a **1.200 mm bridge roof over a 4.000 mm channel = 3.3 : 1**,
   with its roof 0.200 mm below the wiring plane. Nothing else bridges.
2. **The bridge's first layer is the whole risk.** At 0.120 mm the roof is 10
   layers and the first one spans 4.000 mm of air. Set bridge flow and bridge
   speed from the slicer's bridge detection and **print one bridge as a test
   patch before the 304.600 mm part**, because a failed bridge is visible through
   clear acrylic forever and is not repairable.
3. **Two ironed planes, and both are single-Z.** The wiring plane at Z = 0.000
   and every channel floor at Z = −3.000 `[stack.md §5.5]`. The single-Z property
   is the *condition* for ironing each in one pass, and it is a geometric
   constraint the CAD must hold — §4.5 item 5 makes it checkable.
4. **Do not add a chamfer or a radius to the channel mouths.** They are at the
   ironed plane; a chamfer there puts a non-horizontal facet in the middle of the
   surface the whole design exists to show. Deburr by hand instead `[stack.md
   §6.2 step 1]`.

**The one thing orientation does not fix: the vertical channel walls are
visible.** A 3.000 mm channel wall seen through clear acrylic from 150 mm is
25 layers at 0.120 mm and 15 at 0.200. That is the surface layer height actually
shows on — not the top face, which is set by ironing.

## 2.2 Layer height and nozzle — and a contradiction to resolve

**The repo currently says both 0.120 and 0.200 and one of them has to go.**

| source | says |
|---|---|
| `stack.md` §5.5 | **0.120 mm**, 0.4 mm nozzle — "three layers to a 0.36 mm feature; the 1.20 mm bridge roof is 10 layers" |
| `v2/cad/parameters.scad` §0 | **`layer_h = 0.200`** — "0.2 for a readable top face" |

`parameters.scad`'s justification is the weaker of the two, because **the top
face's finish does not come from the layer height.** The wiring plane is a flat
top at a single Z; its finish is set by ironing flow and line spacing
(0.150–0.250 mm, slower = glossier `[3P ironing-guides]`), and the layer height
is irrelevant to it. Where layer height *does* show is §2.1 item 4 — the
vertical channel walls — and there 0.120 is better.

**Recommendation: `layer_h = 0.120` for the part that ships, `0.200` for
fit-check prints.** Both documents are each half right and the resolution is
that there are two prints, not two opinions. **This document does not edit
`parameters.scad`; the contradiction is logged here and in §5.5.**

**Nozzle: 0.400, and it is sourced.** The asset library's own instruction for its
insert coupon is *"a 0.4 nozzle, for accurate holes"* `[HOUSE README]`, and the
insert bores at Ø4.000 are the accuracy-critical feature of the whole part. A
0.600 nozzle would make the 1.270 mm rib two extrusions instead of three and
would widen the insert bores.

**And the rib arithmetic, precisely:** `stack.md` §5.1 draws the inter-channel
rib at **1.270 mm** and calls it "three extrusions wide". 1.270 / 3 = **0.4233 mm
of extrusion width**, which is inside the usual 0.38–0.56 mm range a slicer will
accept on a 0.400 nozzle, so it lays three solid lines with **no infill and no
gap-fill** `[DERIVED]`. That is the actual mechanism, and it is why 1.270 and not
1.200.

## 2.3 Heat inserts — the house standard, and the M2.5 row that does not exist

The asset library's table, verbatim `[HOUSE README]`:

| thread | hole | depth to model | min wall |
|---|---|---|---|
| M2 | **3.200** | **4.000** | **1.300** |
| M3 | **4.000** | **6.700** | **1.600** |
| M4 | 5.600 | 9.100 | 2.100 |
| M5 | 6.400 | 10.500 | 2.600 |

**There is no M2.5 row.** The task that asked for "the asset library's CNC
Kitchen standard" for M2.5 was asking for something that is not in it.

**What is in it, measured.** `08_cutouts/insert_M3_cnckitchen.stl` is not a
cylinder. Sliced today `[MEASURED]`:

```
   z 0.000 .. 6.300    O 4.000 straight bore
   z 6.300 .. 6.700    45 deg flare, O4.000 -> O4.820   (0.410 radial over 0.400)
   z 6.700 .. 7.700    O 4.820, 1.000 mm of overcut
```

and `insert_M2_cnckitchen.stl` is the same shape at Ø3.200 → Ø4.029 with the
flare at z 3.600–4.000. **So the cutout bodies encode the table exactly: align
the cutout's z = (depth) plane with the part's face, let the top 1.000 mm
protrude for a clean boolean, and you get the table's bore and depth with a
0.400 mm × 45° lead-in at the mouth.** Subtract the library's STL and you cannot
get it wrong; draw it by hand and you will forget the lead-in.

**The inserts themselves, measured from `07_hardware` `[MEASURED]`:**

| file | knurl Ø × L | the table's bore | interference |
|---|---|---|---|
| `heat-insert_M2_3.6x3.stl` | **3.600 × 3.000** | 3.200 | 0.400 |
| `heat-insert_M2p5_4.6x4.stl` | **4.600 × 4.000** | *(no row)* | — |
| `heat-insert_M3_4.6x5.7.stl` | **4.600 × 5.700** | 4.000 | 0.600 |
| `heat-insert_M4_6.3x8.1.stl` | 6.300 × 8.100 | 5.600 | 0.700 |

**The depth rule is exact on every verifiable row: depth = insert length +
1.000.** M2 3.000 → 4.000; M3 5.700 → 6.700; M4 8.100 → 9.100 `[DERIVED]`.

**Therefore M2.5, derived from the library's own geometry and its own rule:**

```
   M2.5 insert measured    O 4.600 x 4.000            [MEASURED, 07_hardware]
   knurl O is IDENTICAL to M3's 4.600  ->  bore = M3's bore = O 4.000
   depth = L + 1.000                  ->  5.000
   min wall = M3's                    ->  1.600
                                                                    [DERIVED]
   corroborated twice, independently:
     ruthex RX-M2,5x5.7  D3 (recommended bore) = 4.00   [3P ruthex-3djake]
     Insertsdirect M2.5  "Recommended Hole Size: 4.00"  [3P insertsdirect]
     -- both already quoted in stack.md 5.4
```

**And `parameters.scad` is wrong:** `insert_m25_hole_d = 3.200`,
`insert_m25_depth = 4.000` is the M2 row. **A Ø4.600 knurl into a Ø3.200 bore is
1.400 mm of diametral interference**, against 0.600 for the M3 that the same
knurl is designed for. It will crack a 1.600 mm boss wall or leave the insert
proud. §0.3.

**The bosses to draw:**

| | bore | depth | min wall | boss OD ≥ | **draw** | actual wall |
|---|---|---|---|---|---|---|
| **M2** — every breakout, every comb | 3.200 | 4.000 | 1.300 | 5.800 | **6.350** (10 × 0.635 = 5 × 1.270) | **1.575** |
| **M2.5** — the Feather's two USB-end screws | **4.000** | **5.000** | **1.600** | 7.200 | **7.620** (12 × 0.635 = 6 × 1.270) | **1.810** |
| **M3** — the six slab screws, the six back-panel screws | 4.000 | 6.700 | 1.600 | 7.200 | **7.620** | **1.810** |

`[DERIVED from HOUSE]`. The 6.350 and 7.620 draw sizes are `stack.md` §5.4's own
convention (a boss OD that is a module multiple), carried forward.

**This closes `stack.md` §7.12.** That item reads "ruthex M2 insert dimensions,
and the definition of ruthex's D1/D2/D3/W symbols — NOT SOURCED". The house
library answers the question that mattered — **M2: bore Ø3.200 × 4.000, min wall
1.300** — from a table plus its own geometry, without needing ruthex's symbols
decoded. The ruthex symbol definitions remain unknown and no longer matter.

**Insert the inserts first, always** `[stack.md §6.2 step 2]`: you cannot press
one in next to a populated board. **Iron at filament temperature + 10 to 20 °C,
use an insert tip, let gravity do the work, stop at flush** `[HOUSE README]`.

## 2.4 The M3 bosses on the 8.125 mm wall — the check, and it does not hold as a wall

The question is whether `wall = 8.125` can carry a Ø4.000 M3 insert bore at
`screw_x = 144.780`. **The wall is thick enough and the position is wrong.**

```
   deck outer face        x = 152.300        [parameters.scad, body_w/2]
   interior face          x = 144.175        [parameters.scad, interior_w/2]
   wall                       8.125  >=  4.000 + 2 x 1.600 = 7.200   OK in the abstract
   screw line             x = 144.780  --  only 0.605 OUTBOARD of the interior face

   O4.000 insert bore spans   142.780 .. 146.780
   outboard material          152.300 - 146.780 = 5.520   >= 1.600   OK
   inboard material           the bore crosses the interior face by 1.395
                              -> ZERO wall on the inboard side
                                                                    [DERIVED]
```

**So the answer is: as a plain wall, no. As a boss, yes, and the boss is
mandatory, not cosmetic.** Drawn at OD 7.620:

```
   boss O7.620 at x = 144.780   spans 140.970 .. 148.590
   inside the outer face        148.590 < 152.300           OK
   intrudes past the interior   144.175 - 140.970 = 3.205 into the interior
   nearest board edge           the fader board's right edge at 137.160  [STEP]
   clearance to it              140.970 - 137.160 = 3.810               OK
   mirrored, left side          display board left edge -137.080 -> 3.890 OK
                                                                    [DERIVED]
```

**The boss projects 3.205 mm into the interior at six places and that is fine** —
it is in the air between boards, in the control band, outboard of the 285.750 mm
switch field. **Draw it as a boss that is part of the wall's inner face, not as a
free-standing pillar**, because a pillar on the wiring plane is exactly what
`stack.md` §5.4 forbids: *"the moment a standoff pillar appears on the visible
plane, the plane stops being a plane."* It reads as a local thickening of the
wall, which is what it is.

**And if `screw_x` moves to 142.875** to satisfy the conservative hole-to-edge
reading (§1.4), the boss intrudes **5.110 mm** and clears the fader board by
**1.905 mm** — still legal, tighter, and worth knowing before anyone moves it
`[DERIVED]`.

## 2.5 Warp, flatness, and exactly how much bow the design can absorb

**The failure mode, named:** a bowed carrier under a rigid 12.700 mm slab.
`backpanel.scad`'s own header already says so. The slab is 12.700 mm of cast
PMMA and the carrier is 6.000 mm of PETG floor; **the slab is the stiffer part by
a wide margin, so the slab is the datum and the carrier does the complying.**

**The budget, and it is a real number:**

```
   clearance, acrylic underside to the display's outer face    1.270   [knobs.md 1.1]
   - FDM pocket-depth tolerance, Protolabs prototyping         0.500   [SVC protolabs]
   = left for the slab's flatness AND the carrier's bow        0.770   [DERIVED]

   at Protolabs' industrial +/-0.250 instead                   1.020   [DERIVED]
```

**0.770 mm, measured at the display pocket, relative to the plane through the six
M3 bosses.** That is the whole allowance, it is shared with the slab's own
flatness, and Waveshare's reason for wanting it is explicit: *"reserve enough
space for the screen and spacer to avoid damage to the LCD screen caused by
assembly pressure"* `[VENDOR W1, via knobs.md §1.1]`. **Exceed it and you are
pressing on the LCD.**

**Why the span, not the material, is the problem:**

```
   slab screw columns      x = +/-144.780  ->  289.560 mm apart
   screws within a column  3 at 31.750 pitch  ->  a 63.500 mm span
   back-panel screws       x = +/-144.780, y = -20.000 / -55.000 / -85.000
                           -- all in the keyboard region

   -> the middle 289.560 mm of the control band's front edge and rear edge
      is fastened at NEITHER face.                              [DERIVED]
```

**What keeps it flat, in order of leverage:**

1. **The back panel, bolted up into the same rim.** This is already designed and
   `backpanel.scad` states the reason: "a stiff back panel fights it from the
   other side". 3.000 mm of acrylic over the full 304.600 × 190.375 footprint is
   a second rigid plate, and it is the single biggest term. **Its thickness is
   `[PROVISIONAL]` in `parameters.scad` and it should not be reduced.**
2. **Ribs on the underside.** `stack.md` §5.5: a **1.270 mm cross-rib grid on the
   25.400 mm module**, the same module as the combs. **Run the ribs that resolve
   the 289.560 mm span continuously from side wall to side wall** — a grid does
   both directions, but the X ribs are the ones doing the work, and an X rib
   interrupted by a board pocket is not a rib.
3. **Material: PETG** `[stack.md §5.5]`, which creeps less than PLA under screw
   preload and does not go brittle. PETG also warps less than ABS. The cost is
   lower ironing flow and stringing.
4. **Bed and brim.** **NOT SOURCED (§5.6).** No bed temperature, adhesion or brim
   figure in this repo or the house library is traceable to anything, and a
   304.600 × 190.375 part is beyond what the library's own "print a test slice"
   advice covers. The procedure that closes it is in §5.6; what the *design* can
   do is listed above, and the design should not depend on the answer.

**The flatness check is physical and the STL cannot do it** — §4.6. `stack.md`
§6.2 step 1 already specifies it: *"Check flatness on glass."* Make it a number:
**feeler-gauge the carrier against a float-glass reference at the four corners,
the four edge midpoints and the display pocket, and record nine readings. The
display pocket reading is the one that has to be ≤ 0.770 mm, and if a corner is
high you can pull it down with the screw; if the display pocket is high you
cannot.**

## 2.6 Wall, infill, ribs — "rigid, structured, overbuilt"

The brief's word is overbuilt and `parameters.scad` already took it: `wall =
8.125` is tagged *"sides and rear; overbuilt by design"* and `front_wall = 4.400`
is tagged *"the one wall that must NOT be overbuilt"* — the near edge, where the
heels of the hands land. Nothing here re-argues that. What it needs is the
printing consequence:

| | rule | why |
|---|---|---|
| **every rib is a whole number of 0.4233 lines** | **1.270 = 3.00 lines, 2.540 = 6.00** `[DERIVED]` | A rib that is a whole number of extrusion widths prints solid with no infill and no gap-fill `[stack.md §5.1]`. The walls are not and do not need to be — 8.125 is 19.19 lines and 4.400 is 10.39, both thick enough to be perimeters plus infill. **The 1.270 rib is exact and must stay 1.270.** |
| **minimum wall anywhere** | **0.800** = 2 × nozzle `[stack.md §5.1]` | And the insert bosses have their own, higher minimum: 1.600 for M3/M2.5, 1.300 for M2 (§2.3). |
| **perimeters** | enough to make every rib solid | A 1.270 rib at 3 lines means **≥ 2 perimeters**, i.e. the rib is all perimeter. |
| **infill** | whatever is left, and it is not load-bearing | With 8.125 walls, 1.270 ribs and a 6.000 floor, the infill's job is to support the top surface, not to carry load. The ironed wiring plane is what needs it: **a dense top solid-layer count matters more than the infill percentage**, because an ironed surface over sparse infill pillows. |
| **ribs** | **1.270 on the 25.400 module, underside** `[stack.md §5.5]` | §2.5 item 2. |
| **no chamfers or radii on the wiring plane** | — | §2.1 item 4, and it is also what keeps the prismatic/single-Z checks of §4.5 meaningful. |

---

# 3. The tolerance budget — one table the CAD obeys

Every mating pair in the build. **"Designed" is the nominal clearance as drawn;
"worst case" stacks every tolerance in the harmful direction.** Laser features
carry two columns because §1.1 does not assume the service compensates.

| # | mating pair | nominal | side A tolerance | side B tolerance | designed clearance | worst case | verdict |
|---|---|---|---|---|---|---|---|
| **1** | **acrylic bore → knob body**, radial | bore **Ø22.000** `[DESIGN]` / body **Ø18.000** `[DESIGN]` | laser size+position **±0.230** `[SVC sendcutsend]`; faceting **−0.013/side** at $fn 64 `[MEASURED]`; kerf **+0.100/side** or 0 `[SVC ponoko-kerf]`; taper **−0.222 … −0.443/side** `[DERIVED]` | FDM **±0.500** proto / **±0.250** industrial `[SVC protolabs]`; shaft play at the rim **±0.500** `[DS ptv09 §2.3]`; knob D-bore to shaft **±0.100** `[knobs.md §3.1]` | **2.000 radial** at the visible face | **1.542 radial** at the narrow face, 2.0° | **PASS.** Need is RSS 0.750 + taper 0.443 = **1.193** `[knobs.md §2.1]`. Margin **0.349**. |
| **2** | **acrylic perimeter → carrier perimeter** | both **304.600 × 190.375** `[parameters.scad]` | laser **±0.230** `[SVC sendcutsend]` | FDM **±0.5 % = ±1.523** on 304.600 `[SVC protolabs]` | **0.000 — flush** | **±1.753 of visible step, either direction** | **FAIL as drawn.** Draw the carrier **1.905 (3 × 0.635) inboard all round**; the acrylic then always overhangs, by **1.028 … 2.782** `[DERIVED]`. §0.11. |
| **3** | **carrier pocket → breakout PCB** (patterns A and B) | A **25.400 sq**, B **76.200 × 21.590** `[STEP]` | FDM **±0.500** proto / **±0.250** industrial `[SVC protolabs]` | PCB outline: JLCPCB **±0.200** `[SVC jlcpcb]`; Adafruit's own fab **NOT SOURCED (§5.7)** | **0.500/side** `[stack.md §5.4]` | **−0.200/side at Protolabs prototyping → interference** | **MARGINAL.** 0.500/side needs an XY-calibrated printer at ±0.250. **Draw 0.635/side (1 unit)** for a safe 0.185 at ±0.250, or **0.953 (1.5 units)** if it is outsourced. |
| **4** | **carrier pocket → Waveshare display board**, Y | pocket **70.300** `[parameters.scad]` vs the **acrylic stand base 70.103** `[step, board-models.md]` — *not* the PCB's 69.100 `[dxf]` | FDM **±0.500 / ±0.250** | stand-base outline **NOT SOURCED** | **0.0985/side** `[MEASURED from the repo's own numbers]` | **−0.4/side → it will not drop in** | **FAIL as drawn.** The pocket is sized to the PCB and the stand base is 1.003 wider. **Draw 93.770 × 71.373** (= stand base + 0.635/side). |
| **5** | **M3 screw → acrylic clearance hole** | screw **Ø3.000**; hole **Ø3.400** as drawn `[parameters.scad]` | laser **±0.230**; kerf **+0.200** or 0; **taper −0.443 … −0.887** | screw shank Ø3.000 nominal; ISO shank tolerance **NOT SOURCED (§5.8)** | 0.400 diametral *at the top face* | **Ø2.483 at the narrow face → the screw does not pass** | **FAIL. Draw Ø4.445 (7 × 0.635)**: narrow face **Ø3.528** worst case, **0.528 diametral**. §0.1. |
| **6** | **M3 screw → back-panel clearance hole** | screw **Ø3.000**; hole **Ø3.400**; *t* = **3.000** | laser **±0.230**; kerf **+0.200** or 0; **taper −0.105 … −0.210** | as above | 0.400 diametral | **Ø3.160 at the narrow face** | **PASS, unchanged.** The same hole is fine at 3.000 mm. §0.14. |
| **7** | **M3 screw → carrier float** | hole **4.445 round** at the datum, **4.445 × 7.445 slots** ×5 | — | PMMA heat+moisture **5 mm/m = 1.448 on the 289.560 span** `[RÖHM gsxt]`; FDM boss pitch **±0.5 % = ±1.448** on the same span `[SVC protolabs]` | **±1.500 of float** from the slots | each outer hole misaligns by **0.724 + 0.724 = 1.448** | **PASS with the slots — by 0.052 — and FAIL without.** `parameters.scad` has six round holes. §0.2. |
| **8** | **heat insert → boss, M3** | insert **Ø4.600 × 5.700** `[MEASURED]`; bore **Ø4.000 × 6.700** `[HOUSE]` | insert, as bought | FDM bore **±0.500 / ±0.250** | **0.600 diametral interference** — this *is* the fit | at +0.500 bore: 0.100 interference, loose; at −0.500: 1.100, cracks a 1.600 wall | **The insert fit needs an XY-calibrated printer.** Print the library's `00_design-rules/insert_coupon_M2-M5.stl` first `[HOUSE]`. Boss OD **7.620**, wall **1.810**. |
| **9** | **heat insert → boss, M2.5** | insert **Ø4.600 × 4.000** `[MEASURED]`; bore **Ø4.000 × 5.000** `[DERIVED §2.3]` | as above | as above | **0.600 diametral** | as above | **PASS at the corrected bore. `parameters.scad`'s Ø3.200 is 1.400 of interference and fails.** §0.3. |
| **10** | **heat insert → boss, M2** | insert **Ø3.600 × 3.000** `[MEASURED]`; bore **Ø3.200 × 4.000** `[HOUSE]` | as above | as above | **0.400 diametral** | — | **PASS.** Boss OD **6.350**, wall **1.575**. Closes `stack.md` §7.12. |
| **11** | **M2 screw → breakout mounting hole** | hole **Ø2.500** `[STEP, every board]`; M2 clearance **Ø2.400** `[DERIVED, stack.md §5.4]` | PCB hole, as fabricated | — | **0.100 diametral** | — | **PASS, and it is the reason the boards are M2 and not M2.5** `[stack.md §5.4]`. |
| **12** | **keyboard void → retained top frame** | void **288.350 × 98.450** `[parameters.scad]`; frame **287.000 × 97.500, R4.400** `[CAD-EXACT]` | laser **±0.230**; kerf **+0.200** or 0; **taper −0.443 … −0.887** | frame, as manufactured | **0.675 / 0.475 per side** | X **+0.217**, **Y +0.017** per side at 2.0° | **FAIL in Y if the frame passes the slab.** Draw **288.925 × 99.695**, growing Y rearward only. **Whether the frame passes is unresolved — §5.1.** |
| **13** | **fader slot → printed cap** | slot **12.000 × 52.000** `[parameters.scad]`; cap **10.000** wide `[DESIGN]` | laser, kerf, taper as above | printed cap, FDM **±0.500 / ±0.250** | **1.000/side** | **0.542/side** at the narrow face | **PASS**, and the cap is fitted last `[stack.md §6.2 step 12]` so it never has to pass through. |
| **14** | **acrylic underside → display outer face** (axial) | gap **1.270** `[knobs.md §1.1]` | slab flatness **NOT SOURCED (§5.2)** | FDM pocket depth **±0.500 / ±0.250** `[SVC protolabs]` | **1.270** | **0.770 for flatness + bow combined** | **PASS only if the carrier is flat to ≤0.770 at the display pocket.** This is §2.5's whole budget. |
| **15** | **acrylic underside → carrier rim** (bearing, not a fit) | rim top **Z = 6.350** `[parameters.scad]` | slab flatness **NOT SOURCED** | FDM Z: layer height 0.120 → **±0.060** plus first-layer squish **NOT SOURCED (§5.6)** | bearing contact | a high rim lifts the slab off its other bearing points | **Print the rim, then scrape it flat with the carrier on glass.** It is 8.125 mm wide (4.400 at the front) and 989.950 mm long; it is a lapping operation, not a tolerance. |
| **16** | **knob brim → slab top face** (axial) | brim **Ø25.400** standing **6.350** proud, overhanging the bore **1.700** radial `[knobs.md §2.3]` | slab thickness: **cast "half-inch" is in spec 10.79 … 13.59** `[3P eplastics]` | set-screw position on the shaft, set with a shim `[stack.md §6.2 step 12]` | **set at assembly, not drawn** | — | **Not a designed fit, and it must not become one.** The cast slab's thickness tolerance is 2.800 mm wide; the only thing that makes six knobs match is the shim. Draw the knob to nominal and say "set with a shim" in the build notes. |

---

# 4. What the export pipeline must guarantee

A checklist for `v2/tools/check_cad.py`. **Every item is measurable on an
exported artefact with trimesh, ezdxf, shapely or numpy** — the repo's standing
rule is that a check which computes from the source parameters instead of
measuring the artefact is worse than none, because it passes while the part is
wrong.

**Read first: §4.7 is the toolchain's actual capability, measured today, and two
of the obvious ways to write these checks do not work.**

## 4.1 On the DXF

| # | check | measure it with | state today |
|---|---|---|---|
| 1 | **The file opens and has entities.** | `ezdxf.readfile`; `len(list(doc.modelspace()))` > 0 | ✔ 970 `[MEASURED]` |
| 2 | **`$INSUNITS == 4` and `$MEASUREMENT == 1`.** | `doc.header["$INSUNITS"]` — and fail if the HEADER section is **absent**, which is not the same as the value being wrong. Read the raw text for `"HEADER"` to tell them apart. | **✘ no HEADER section at all** `[MEASURED]` |
| 3 | **Every entity is on a declared layer** from {`CUT_OUTER`, `CUT_INNER`, `ENGRAVE`, `SCORE`}, the LAYER table exists, and the colour matches §1.6's table. | `doc.layers`; `e.dxf.layer`; `e.dxf.color` | **✘ layer `"0"` only, no TABLES section** `[MEASURED]` |
| 4 | **No entity type the receiver may silently drop.** Whitelist {`LINE`, `LWPOLYLINE`, `POLYLINE`, `CIRCLE`, `ARC`}. Fail on `SPLINE`, `ELLIPSE`, `3DFACE`, `HATCH`, `TEXT`, `MTEXT`, `INSERT`. | `collections.Counter(e.dxftype() …)` | ✔ 100 % `LINE` `[MEASURED]` |
| 5 | **Every path is closed and nothing dangles.** Build the endpoint graph; assert **every vertex has degree exactly 2** and no segment is used twice. An open path is a laser's worst input and it is invisible on screen. | the `stitch()` already in `check_cad.py`, plus a degree histogram | ✔ 15 loops close `[MEASURED]` |
| 6 | **Loop count equals the declared feature count.** | `len(loops)` | ✔ slab 15 = 1 + 1 void + 6 bores + 6 fasteners + 1 slot; backpanel 7 `[MEASURED]` |
| 7 | **Nesting is exactly one level deep.** Exactly one loop contains all others; no loop inside a hole. An island in a hole is cut as a separate part and lost on the floor. | `shapely` `contains`, build the tree, assert max depth 1 | ✔ `[MEASURED]` |
| 8 | **Bounding box equals `body_w × body_h`** to 0.010. | min/max over all loop vertices | ✔ **304.600 × 190.375** exactly `[MEASURED]` |
| 9 | **Every bore's INSCRIBED diameter is within the faceting budget of nominal.** Inscribed = `2 × Point(cx,cy).distance(ring)`. **Do not measure the vertex radius:** the vertices sit on the nominal circle, so a vertex-radius check reads 22.0001 and passes while the material is 0.013 mm proud at every chord. | `shapely` | **Ø22.000 bore: vertex 22.0001, inscribed 21.9735, area-equivalent 21.9823** `[MEASURED]` |
| 10 | **Faceting sagitta ≤ 0.010 for every circular feature:** `r(1 − cos(π/n)) ≤ 0.010`, i.e. `n ≥ π / arccos(1 − 0.010/r)`. | arithmetic on the recovered vertex count | **✘ at $fn = 64, r = 11.000 gives 0.0132. Needs n ≥ 74. Set $fn = 96 → 0.0059** `[DERIVED]` |
| 11 | **Minimum web: the pairwise boundary distance between every pair of distinct loops**, printed as a sorted list, gated at 1 × *t*. | `ring_a.distance(ring_b)` | **13 pairs below 12.700, minimum 4.400** `[MEASURED §1.5]` — so gate this as a **WARN with the list**, not a FAIL, and FAIL only below a hard floor (**2.000**, 10 × kerf). |
| 12 | **Self-approach within a single loop** — the narrow neck a slot or a filleted corner can create. For each loop, minimum distance between non-adjacent segments. | pairwise over segments, skipping neighbours | ✔ `[MEASURED]` — **and do NOT use an erosion/connectivity test for this.** `solid.buffer(-w/2)` reports 8.125 on the slab and misses the 4.400 front rail entirely, because severing a frame's rail does not disconnect the frame. Verified today. |
| 13 | **Hole centre to the nearest other boundary ≥ 1.5 × hole Ø** (Röhm). Report the *land* as a margin; do not gate on it. | `Point(c).distance(other_ring)` | ✔ Ø4.445 at x = ±144.780: centre 7.520 ≥ 6.668 `[DERIVED]` |
| 14 | **No internal corner sharper than R0.500.** Fit a circle to every consecutive vertex triple on every internal loop; take the minimum radius. | numpy | `corner_r = 2.000` ✔ |
| 15 | **No duplicate or co-located loops.** Two loops whose centroids are within 0.010 **and** whose areas agree to 0.1 % are a double-cut path: it burns the edge twice and can drop a part. | centroid + area comparison | ✔ `[MEASURED]` |
| 16 | **Handedness: the DXF is in model coordinates.** Assert an asymmetric landmark's sign — the keyboard void's centroid y must be **negative** and within 0.010 of `kbd_top_y − kbd_pocket[1]/2`. | centroid | ✔ **−41.563** vs −41.5625 `[MEASURED]` |

## 4.2 On the SVG

| # | check | measure it with | state today |
|---|---|---|---|
| 1 | **`width`/`height` carry an explicit `mm`, and `width_mm / viewBox_width == 1.000`** to 1e-6. This is the only units guarantee in the whole pipeline that currently works. | regex on the root element | ✔ `306mm / 306 = 1.000` `[MEASURED]` |
| 2 | **The viewBox contains the part's bounding box**, and the padding is reported. | arithmetic | ✔ 306 × 192 vs 304.600 × 190.375 → 0.700 / 0.813 per side `[MEASURED]` |
| 3 | **`fill="none"` and a hairline stroke.** A fill is how you order a raster engrave by accident; a 0.5 mm stroke puts a 0.250 mm ambiguity on the cut line. | regex | **✘ `fill="lightgray"`, `stroke-width="0.5"`** `[MEASURED]` |
| 4 | **Handedness matches the DXF.** Parse the same asymmetric landmark out of the SVG and compare its sign with the DXF's. | path parsing | **✘ SVG y = −(DXF y)** — harmless today only because the slab is x-symmetric `[MEASURED §1.3]` |
| 5 | **Subpath count equals the DXF loop count**, and every subpath ends in `z`. | count `M` and `z` | ✔ slab **15 / 15**, backpanel **7 / 7** `[MEASURED]` |

## 4.3 On the laser parts' STL

The STL of a laser part is not sent to anyone — it exists so the render is
provably of the same geometry as the cut file. Check it as that.

| # | check | measure it with | state today |
|---|---|---|---|
| 1 | **Non-empty.** | `len(m.faces) > 0` | ✔ slab 3932, backpanel 1828 `[MEASURED]` |
| 2 | **`is_watertight`, `is_volume`, `is_winding_consistent` all True.** | trimesh | ✔ all three, both parts `[MEASURED]` |
| 3 | **Exactly one body.** **Use union-find over `m.face_adjacency`, not `m.body_count`** — §4.7. | pure python | ✔ 1 each `[MEASURED]` |
| 4 | **Euler characteristic equals `2 − 2 × (expected through-holes)`.** This is a free, exact count of through-features and it needs nothing but the mesh. | `m.euler_number` | ✔ **slab −26 → genus 14 → 14 through-holes; backpanel −10 → genus 6 → 6.** Both match the declared feature count `[MEASURED]` |
| 5 | **Thickness equals nominal** to 0.010, and **Z runs 0.000 … *t*** so the slicer and the assembly agree on the datum. | `m.bounds` | ✔ slab 0.000 … 12.700, backpanel 0.000 … 3.000 `[MEASURED]` |
| 6 | **Prismatic: every facet normal is either ±Z or has `n_z == 0`** to 1e-6. A laser cuts a prism; any drafted or chamfered facet means somebody modelled something a laser cannot make. | `m.face_normals` | ✔ `[MEASURED, implied by a linear_extrude]` — make it explicit, it is the cheapest guard in the list |
| 7 | **No degenerate facets** (area < 1e-9) and **no duplicate facets** (sorted vertex triples). | `m.area_faces`, numpy | ✔ `[MEASURED]` |
| 8 | **Bounding box equals the DXF's** to 1e-4. | `m.bounds` | ✔ `[MEASURED]` |

## 4.4 Cross-artefact — the checks that catch a stale or mismatched export

These are the ones that actually earn their keep, because each compares two
artefacts rather than an artefact against a parameter.

| # | check | measure it with | state today |
|---|---|---|---|
| 1 | **STL volume ÷ thickness == the DXF's total polygon area.** The single strongest proof that the mesh and the cut file are the same part. | trimesh + shapely | ✔ **slab 26675.7339 vs 26675.738, Δ = 0.0041 mm²** (1.5e-7 relative); **backpanel 57930.3788 vs 57930.388, Δ = 0.009** `[MEASURED]`. **Tolerance 1.000 mm² is 240× the observed residual.** |
| 2 | **Every DXF loop centroid appears in the STL's mid-height section** within 0.010. | `trimesh.intersections.mesh_plane` + shapely | ✔ `[MEASURED]` |
| 3 | **The section is identical at `z = 0.050`, `t/2` and `t − 0.050`** — ring count, centroids, inscribed diameters. Proves the prism and gives item 4.3.6 a second witness. | as above | ✔ inscribed Ø21.9735 at all three z `[MEASURED]` |
| 4 | **Every expected file exists, is non-zero, and is NEWER than the `.scad` it came from and newer than `parameters.scad`.** **A stale export that passes every geometric check is the worst failure in this document**, because it is the one that looks like success. | `os.path.getmtime` | not currently checked |
| 5 | **No `[PROVISIONAL]` datum is load-bearing for a dimension on a file about to be sent.** `check_cad.py` already parses and reports them; make it **FAIL** rather than warn when the part is being exported for fabrication, and pass when it is being exported for review. | the existing `datums()` parser | **3 PROVISIONAL today: `back_t`, `carrier_t`, `slider_travel`** `[MEASURED]` |
| 6 | **OpenSCAD's stderr is clean.** `export.py` already fatals on "unknown variable", "undefined operation" and "may not be a valid 2-manifold". **Add `"CGAL error"`, `"Ignoring unknown module"` and `"Ignoring unknown function"`** — all three produce a part that renders, exports, and is silently wrong. | the existing filter | partially |

## 4.5 On the carrier STL, when it exists

`v2/cad/carrier.scad` does not exist yet; `export.py` correctly reports `skip`.
These are the checks it will need, all scipy-free.

| # | check | measure it with |
|---|---|---|
| 1 | **Watertight, a volume, winding-consistent, one body, no degenerate facets.** | as §4.3 |
| 2 | **Bounding box = 304.600 − 2 × 1.905 by 190.375 − 2 × 1.905 by 12.350** (§3 row 2 and `parameters.scad`'s `carrier_h`). | `m.bounds` |
| 3 | **Every insert bore is the right diameter and deep enough.** Section at a set of z; for each bore ring assert inscribed Ø = **4.000 ± 0.050** (M3, M2.5) or **3.200 ± 0.050** (M2); then measure the bore's **z-extent** — the span over which the ring exists — against **6.700 / 5.000 / 4.000** respectively. | `mesh_plane` + shapely |
| 4 | **Boss wall at every insert bore ≥ 1.600 (M3, M2.5) or 1.300 (M2).** In the bore's own slice, the distance from the bore ring to the nearest *other* ring. | shapely `distance` |
| 5 | **Every +Z facet's z falls in the declared set {0.000, −3.000, −4.400}** and nothing else. This is what makes `stack.md` §5.5's two-ironed-planes claim true rather than aspirational: bucket the z of every facet whose normal is +Z and assert the bucket set. | `m.face_normals`, `m.triangles` |
| 6 | **Minimum wall ≥ 0.800** (2 × nozzle). Slice at a set of z and, per slice, assert `polygon.buffer(-0.400)` loses no declared feature; report the thinnest surviving member. | `mesh_plane` + shapely |
| 7 | **Bridge spans.** For each channel crossing, measure the ring width at the bridge's z and assert span / roof thickness ≤ **4 : 1** (`stack.md` §5.1 designs 3.3 : 1). | shapely bounds |
| 8 | **Overhang audit.** Report the total facet area whose normal has `n·(−Z) > cos(45°)` and which is not on the bed plane. **For this part the answer should be the bridges and nothing else** (§2.1) — a non-zero surprise means a chamfer or a radius got onto the wiring plane. | `m.face_normals`, `m.area_faces` |
| 9 | **No channel passes under a board.** `carrier.py`'s own rule. Intersect every route's swept rectangle with every board pocket in plan; assert empty except the one declared exception, the 2×8 header's exit under the display board. | shapely |

## 4.6 What cannot be checked on an artefact, and must not be faked

State these in the checker's own output, so nobody mistakes a green run for a
finished part:

1. **Kerf.** Not in the file (§1.1). The coupon measures it.
2. **Taper.** Not in the file — the STL is a prism and §4.3.6 asserts that it is.
   The coupon measures it, at three depths (§1.7).
3. **Carrier flatness and warp.** A print outcome, not a geometry property.
   Nine feeler-gauge readings against glass (§2.5).
4. **Ironing finish.** Every figure in `stack.md` §5.5 is a starting point from
   slicer documentation, spanning 7 % to 43 % for the same materials
   `[3P ironing-guides]`. A test patch decides it.
5. **Heat-insert seating.** Print `00_design-rules/insert_coupon_M2-M5.stl`
   `[HOUSE]` and press real inserts into it.
6. **The slab's actual thickness.** Cast "half-inch" is in spec anywhere from
   **10.790 to 13.590** `[3P eplastics]` and the file says 12.700. Caliper the
   sheet before anything depends on it. `knobs.md`'s whole section is built on
   the *bore*, not the thickness, specifically so that this does not matter —
   §3 row 16 is where it would bite.

## 4.7 Toolchain facts the checker must be written around — measured today

| fact | consequence |
|---|---|
| **scipy is not installed** on `/usr/bin/python3.11`. Available: trimesh **5.2.0**, shapely **2.2.0**, ezdxf **1.4.5**, numpy **2.4.6**, OCP **8.0.1.1**, PIL 10.2.0. Missing: **scipy, networkx, manifold3d, rtree, lxml**. | `m.body_count`, `m.split()`, `m.convex_hull` and **`m.section()`** all raise `ModuleNotFoundError: No module named 'scipy'`. **Verified by running them.** |
| **`trimesh.intersections.mesh_plane(mesh, plane_normal, plane_origin)` works without scipy** and returns raw segment pairs. | This is how the checker sections a mesh. `m.section()` is not available. Stitch the segments the same way `check_cad.py.stitch()` already stitches DXF `LINE`s. |
| **Body count without scipy**: union-find over `m.face_adjacency`. | ~15 lines, and it is exact. |
| **OpenSCAD 2021.01** writes R12 DXF with no HEADER, no TABLES, bare `LINE`s on layer `"0"`. | §1.6's four post-processing fixes. ezdxf can read the R12 and rewrite it as R2000 with a header and layers. |
| **OpenSCAD's SVG writer flips Y** and emits one filled path. | §4.2 items 3 and 4. |
| **`check_cad.py`'s `dxf_loops()` already falls through to `stitch()`** because there are no polylines or circles to find — so the loops it returns *are* the real 64-gons, and `describe()`'s vertex-radius measurement is therefore the blind spot of §4.1 item 9. | Add an inscribed-diameter measurement beside it; do not replace the existing one, because the pair of numbers is what shows the faceting. |
| **`check_cad.py`'s `acrylic_edge_min = 1.5 × screw_m3_clear_d`** is compared against the hole-wall-to-sheet-edge land. That is Röhm's rule **plus half a diameter** — the conservative reading. | §1.4. It passes at Ø3.400 and fails at the Ø4.445 of §0.1. Re-gate on centre-to-edge and keep the land as a reported margin. |
| **Binary STL stores float32.** The slab's bounding box reads **304.59998 × 190.37500 × 12.69999998** `[MEASURED]`, and the largest round-trip error over this part's dimensions is **2.44 × 10⁻⁵**. | **No STL check may use exact equality.** `check_cad.py`'s `TOL = 0.05` is 2000× the float32 error and is right; the point is to know *why* it cannot be zero, so nobody tightens it to 1e-9 and gets a mystery. The DXF is ASCII decimal at 6 significant figures and does read exactly. |

---

# 5. NOT SOURCED

Stated loudly, per the project's rule that an unsourced number is a bug. Each
carries the procedure that closes it.

**Blocking — these change the exported geometry if they come back wrong.**

1. **Whether the keyboard's retained top frame passes through the slab's void,
   or only the keycaps do.** The frame is 287.000 × 97.500 `[CAD-EXACT]` and the
   keycap field is 284.000 × 94.000 `[DERIVED from keyboard-strip.md §1.6]`;
   the void drawn 288.350 × 98.450 clears the second comfortably and the first by
   0.017 mm per side at 2.0° of taper (§1.2). **The deciding number is the
   keyboard's Z seat, and it is in no file in this repo.** `keyboard-strip.md`
   §2.2 gives the stack relative to the seat (frame top +3.700, keycap top
   +13.542) and `parameters.scad` §8 gives the pocket in plan only. **Procedure:
   fix `kbd_seat_z` in `parameters.scad`, compare `kbd_seat_z + 3.700` against
   `acrylic_z0 = 6.350`, and the branch picks itself.** Until then the void's size
   is undecided and §3 row 12 carries both answers.

2. **The cast slab's flatness, at 12.700 mm, over 304.600 × 190.375.** Röhm
   publish thickness tolerance `[3P eplastics]` and nothing on flatness at this
   size. It shares the 0.770 mm budget of §2.5 with the carrier's bow, so an
   unknown here is an unknown in the one axial clearance that protects the LCD.
   **Procedure: when the slab arrives, lay it on float glass and feeler-gauge the
   four corners, the four edge midpoints and the centre. Nine readings, before
   the carrier is printed, so the carrier's budget is what is left.**

3. **Acrylic kerf taper, measured — magnitude AND profile.** Carried forward from
   `acrylic.md` §11.4 and `stack.md` §7.16, unchanged, and now with a sharper
   question: §1.7 argues the profile may be an **hourglass** rather than a cone,
   because Trotec put the waist 4.3 mm down `[TROTEC]`. **Procedure: the coupon's
   25.00 mm square, measured at the top face, at mid-depth and at the bottom
   face.** Two face measurements cannot distinguish the two profiles and the
   difference decides whether a bore's narrowest point is its exit or its middle.

4. **Accepted file formats, units convention and layer/colour convention, from
   the services that would actually cut this.** `acrylic.md` §11.7 (Ponoko) and
   §11.8 (SendCutSend) are both still open, and §1.5 has now ruled both out
   anyway on DFM grounds. The layer/colour scheme in §1.6 is **our declared
   convention, not any service's published requirement.** **Procedure: one quote
   each from Visiontek `[3P visiontek]` and a local fabricator, with the question
   asked explicitly: "DXF or SVG; what units do you assume; do you compensate
   kerf; which layer or colour do you read as the cut."**

**Not blocking, but they are gaps in a table above.**

5. **Which layer height ships.** `stack.md` §5.5 says 0.120 and
   `parameters.scad` says 0.200 — a direct contradiction between two current
   repo documents (§2.2). This document recommends 0.120 for the shipping part
   and 0.200 for fit checks, and **does not edit either file**. One of them has
   to be changed by whoever owns it.

6. **Bed temperature, bed adhesion and brim strategy for a 304.600 × 190.375
   PETG print; and the first-layer squish that sets the rim's actual Z.** Nothing
   in this repo or the house library sources any of it, and the library's own
   advice — *"Print a test slice before you print the whole box"* `[HOUSE
   README]` — is the honest answer at this size. **Procedure: print a 60 × 60 mm
   corner of the real part with the real rim profile and the real ribs, measure
   its rim height with a depth gauge, and lift a corner to see whether the brim
   was needed. Then scale.** §2.5 is written so the design does not depend on the
   answer.

7. **Adafruit's PCB outline tolerance.** §3 row 3 uses JLCPCB's ±0.200
   `[SVC jlcpcb]` as a class proxy. Adafruit publish none. **Procedure: caliper
   the long edge of a Pattern-B board (nominal 76.200) and a Pattern-A board
   (nominal 25.400) and record the deviation.**

8. **The M3 screw's shank tolerance, and the M3 button head and washer
   dimensions.** `stack.md` §6.1 cites ISO 7380 M4 as Ø7.600 × 2.200
   `[3P fastener-tables]`, a tag that appears nowhere in either document's source
   list. The design needs the **M3 button-head OD** (to know it does not cover
   the Ø4.445 hole) and the **M3 flat washer OD** (to know it does: a Ø7.000
   washer over a Ø4.445 hole gives 1.278 mm of bearing per side). **Procedure:
   read ISO 7380-1 and ISO 7089 / DIN 125 from a primary table, or caliper the
   actual hardware.** Until then: **specify an M3 button head plus a flat
   washer**, because the washer makes the question moot.

9. **Whether `screw_x` stays at 144.780.** §1.4 keeps it by adopting Röhm's
   literal rule; §2.4 shows 142.875 also works and gives more land. This is a
   decision, not a missing number, and it is recorded here so the next session
   does not re-derive it.

---

# 6. Documents and files used

## 6.1 Read

- `v2/parts/stack.md` — §3 (laser at 12.70), §5 (the carrier), §6 (assembly,
  fastening, the coupon), §7 (NOT SOURCED). The primary input.
- `v2/parts/knobs.md` — §1.1 (the display clearance), §2.1 (the bore's clearance
  budget), §2.3 (knob diameter), §2.4 (the bore wall as indicator), §2.5 (the
  bore's top edge and the cone).
- `v2/parts/acrylic.md` — §4 (kerf, machine tolerance, taper, minimum features,
  the CAD table), §5.3 (hole-to-edge, quoted verbatim), §8.1 (services and
  formats), §11 (its own NOT SOURCED list).
- `v2/parts/keyboard-strip.md` — §1.4 (the top frame's 287.000 × 97.500 R4.400),
  §1.6 (the 19.000 grid), §2.2 (the stripped stack), §3.6 (inserts against the
  house standard).
- `v2/parts/board-models.md` — §(rlcd) the Waveshare PCB at 92.500 × 69.100 and
  the stand base at 70.103.
- `v2/parts/parts.md`, `v2/parts/wiring.md` — board and header geometry.
- `v2/cad/parameters.scad`, `v2/cad/slab.scad`, `v2/cad/backpanel.scad`,
  `v2/layouts/slab.py`, `v2/layouts/carrier.py`, `v2/layouts/parts.py`.
- `v2/tools/export.py`, `v2/tools/check_cad.py` — the pipeline §4 is written for.
- `/home/user/kylebsmith/cmtl-3d-assets/README.md` — the house heat-insert table,
  the fits, the nozzle instruction, the workshop coupons.

## 6.2 Measured — reproduce it

Every `[MEASURED]` tag in this document comes from one of these:

```
# the exported DXF: entity inventory, sections, header, units, layers
/usr/bin/python3.11 -c "import ezdxf,collections; d=ezdxf.readfile('v2/export/dxf/slab.dxf'); \
print(d.dxfversion, collections.Counter(e.dxftype() for e in d.modelspace()))"
grep -c HEADER v2/export/dxf/slab.dxf      # 0 -- there is no header section
grep -c TABLES v2/export/dxf/slab.dxf      # 0 -- there is no layer table

# webs, loop inventory, inscribed diameters: stitch the LINEs, then shapely
#   -- check_cad.py.dxf_loops() already does the stitching
#   -- inscribed diameter is 2 * Point(cx,cy).distance(ring)
#   -- min web is ring_a.distance(ring_b) over all distinct pairs

# the exported STL
/usr/bin/python3.11 -c "import trimesh; m=trimesh.load('v2/export/stl/slab.stl',force='mesh'); \
print(m.extents, m.is_watertight, m.euler_number, m.volume, m.volume/m.extents[2])"
#   -> [304.59997559 190.375 12.69999981] True -26 338781.8153 26675.733889
#      (the bbox is float32 -- section 4.7, last row)

# the asset library's insert cutouts and inserts
/usr/bin/python3.11 -c "import trimesh; \
[print(f, trimesh.load(f,force='mesh').extents) for f in \
 ['08_cutouts/insert_M3_cnckitchen.stl','07_hardware/heat-insert_M2p5_4.6x4.stl']]"
#   run from /home/user/kylebsmith/cmtl-3d-assets
#   -> insert_M3 cutout 4.820 x 4.820 x 7.700 ; M2.5 insert 4.600 x 4.600 x 4.000
#   the cutout's profile (bore 0..6.300, 45 deg flare to 6.700, 1.000 overcut)
#   comes from grouping its vertices by z and taking the max radius at each level
```

## 6.3 Sources cited, that are not already in `stack.md` §6.4 or `acrylic.md` §8.4

- **CMTL 3D Assets** README and geometry — `[HOUSE]` —
  `/home/user/kylebsmith/cmtl-3d-assets`, CC BY 4.0 for `00_design-rules`,
  `07_hardware`, `08_cutouts`.
- Everything else — `[RÖHM …]`, `[TROTEC]`, `[SVC ponoko-kerf]`,
  `[SVC sendcutsend]`, `[SVC protolabs]`, `[SVC jlcpcb]`, `[SVC schaeffer]`,
  `[3P taper]`, `[3P ump]`, `[3P fab-guides]`, `[3P eplastics]`,
  `[3P ironing-guides]`, `[3P ruthex-3djake]`, `[3P insertsdirect]`,
  `[3P visiontek]`, `[3P sculpteo]`, `[3P ponoko-cat]`, `[DS ptv09]`,
  `[VENDOR W1]` — as cited, quoted and URL-listed in `v2/parts/stack.md` §6.4,
  `v2/parts/acrylic.md` §8.4 and `v2/parts/knobs.md` §7.

---

# 7. One-line summary

The acrylic is safe to send because nothing in it is a fit — but **the six
fastener holes are sized as if the sheet were 3 mm thick and they close before
an M3 gets through them**, the M2.5 insert bore is the M2 row, the void's Y clearance is zero
at the narrow face, and the DXF has no units: fix those four, cut the amended
coupon, and the rest of this document is a checklist.
