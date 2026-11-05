# Keychron K9 Pro — is there a 3D model?

*Task: find a CAD model of the chosen keyboard, or establish that none exists.
Deliverable is a volume the v2 shell can be designed around, plus an honest
account of which numbers are real.*

Confidence tags used throughout: **[vendor]** published by Keychron ·
**[third-party]** a keyboard database · **[qmk]** the K9 Pro's own firmware
definition · **[derived]** arithmetic from the above, shown · **[UNSOURCED]**
not a measurement, get the calipers.

---

## 0. Verdict up front

**Yes. Keychron publishes factory CAD for the K9 Pro, including a full ANSI
assembly and a dimensioned top-case drawing.** In April 2026 Keychron released
hardware design files for its whole catalogue, and the K9 Pro is one of the
covered models. Six files are published for it, named in §2.

**But none of them can be fetched from this session.** The files are not in the
GitHub repository — the repo is a catalogue of download links and nothing else
(§2.2, proven by file listing). The only host that serves them,
`www.keychron.co:34568`, refuses TCP connections from here. That is an upstream
failure, not a proxy restriction: the connection is refused at the IP, and the
same host answers on 443 with a 503.

So the outcome splits cleanly:

| | |
|---|---|
| **Does a real model exist?** | Yes, and it is free, official, and specific to the K9 Pro |
| **Can this session get it?** | No. Asset host down, twice, 16 minutes apart |
| **Can *you* get it?** | Probably, in one click, from a browser. §2.1 has the exact URLs |
| **Is v2 blocked meanwhile?** | No. §5 geometry is sufficient to cut the bay, and `v2/cad/keyboard_blank.scad` renders it |

**The one finding that changes the design:** the K9 Pro's switch field is
**exactly the standard GH60 60% field, 285.75 × 95.25 mm**, and its outside
footprint is **287 × 97.5 mm** — the same depth as the K7 Pro that
`keyboard.md` §6 costed, and **exactly one key unit (19.05 mm) narrower**. The
whole of `keyboard.md` §5.1's K7 Pro arithmetic transfers, with 19 mm of width
handed back. The user's hunch — *"it looks like the same footprint as the K7"* —
is correct in depth and off by precisely one key column in width.

**And one finding that would have corrupted the design if taken at face
value:** Keychron's own K9 Pro page carries a second, much larger footprint
(`316.6 × 126.91 mm`) labelled "Aluminum frame", and keycap-inclusive heights
of 32 mm and 42.07 mm. **All three numbers are rejected here, with evidence**
(§4.3). Designing a bay around 316.6 × 126.91 would have oversized the deck by
~30 mm in both axes.

---

## 1. What the K9 Pro is

From the QMK keyboard definition, which is the keyboard describing itself
**[qmk]** (`Keychron/qmk_firmware` @ `bluetooth_playground`,
`keyboards/keychron/k9_pro`, saved locally as
`v2/parts/models/keychron-k9pro/qmk-k9_pro-ansi-info.json`):

- layout `LAYOUT_61_ansi`, **61 keys**, matrix 5 rows × 14 cols, ROW2COL
- key field spans **15 u × 5 u**
- key widths used: 1, 1.25, 1.5, 1.75, 2, 2.25, 2.75, **6.25** (spacebar)
- MCU `STM32L432`, 5 dynamic keymap layers
- variants built: `ansi`, `iso`, `jis` × `rgb`, `white`

That width set is the canonical 60% ANSI: 2 u backspace, 1.5 u tab, 1.75 u
caps, 2.25 u left shift, 2.75 u right shift, 6.25 u space. **It is a textbook
60%, not a Keychron-specific oddity** — which is why §5's fallback is sound.

`ansi` is a built target, so the ANSI purchasing constraint from
`keyboard.md` §0 (the US-ANSI `keymap.c` has no entry for HID usage `0x64`) is
satisfiable. Bluetooth device name is **`Keychron K9 Pro`** [vendor] — useful
when the host's HOGP central is picking a peer.

---

## 2. The model that exists

### 2.1 The six published files, with their URLs

From `K-Pro-Series/K9 Pro/README.md` in
`Keychron/Keychron-Keyboards-Hardware-Design` (fetched via
raw.githubusercontent.com; saved locally as
`v2/parts/models/keychron-k9pro/keychron-K9Pro-hardware-README.md`):

| File | What it is | Worth, for this project |
|---|---|---|
| `K9-Pro-US-Full-Model-20241128.stp.zip` | complete assembled ANSI model | **the prize** — case + plate + keycaps |
| `K9-Pro-Top-And-Bottom-Case.stp` | top and bottom enclosure | **what a bay actually needs** |
| `K9-Pro-US-Top-Case.pdf` | dimensioned drawing of the top case | **closes every number in §7 at once** |
| `K9-Pro-US-Top-Case-20230427.dwg` | the same drawing as CAD | 2D outline, directly usable |
| `K9-Pro-Keycap.stp` | keycap profile solids | would settle the protrusion question |
| `k9-Pro-Stabilizer.stp` | stabiliser housing | not needed |

All six live under one directory on Keychron's asset server. The base is

```
http://www.keychron.co:34568/keychron-github-assets/Keychron-Keyboards-Hardware-Design/K-Pro-Series/K9%20Pro/
```

so e.g. the dimensioned drawing is that base + `K9-Pro-US-Top-Case.pdf`.
Keychron's own README warns: *"If a download link does not respond, copy the
link and paste it directly into your browser's address bar."* Note the scheme
is plain **`http`** and the port is **34568** — a browser will do this happily;
most tooling will not.

**If you get these, the whole of §7 becomes unnecessary.** Put them in
`v2/parts/models/keychron-k9pro/` (already created, already gitignored) and the
`K9-Pro-US-Top-Case.pdf` alone should close the footprint, the corner radius
and probably the feet.

### 2.2 Why this session could not fetch them — and why no mirror helps

Three facts, each checked:

1. **The files are not in the git repository.** A full file listing of
   `Keychron/Keychron-Keyboards-Hardware-Design@main` via the jsDelivr
   packages API returns **217 files: 196 `.md`, 11 images, 1 `.py`, 1 `.json`,
   4 `.yml`, 1 `.html`, 1 `.gitattributes`, 1 `.gitignore` — and zero `.stp`,
   `.dxf`, `.dwg` or `.pdf`.** Every one of the six filenames above returns
   404 from raw.githubusercontent.com. The repo is a link catalogue.
   (Full breakdown: 196 `.md`, 11 images — 4 `.jpg`, 4 `.jpeg`, 2 `.png`,
   1 `.webp` — 4 `.yml`, and one each of `.py`, `.json`, `.html`,
   `.gitattributes`, `.gitignore`, `LICENSE`. Sums to 217.)
2. **Therefore no git mirror can help.** Mirrors copy commits. These bytes were
   never committed, so Software Heritage, jsDelivr, gitcode and friends have
   nothing to serve. This is worth stating because it is the obvious next thing
   to try and it is a dead end by construction.
3. **The asset host is refusing connections.** `www.keychron.co:34568` →
   `ECONNREFUSED 183.62.235.170:34568`, retried 16 minutes apart, same result.
   The same hostname on standard 443 answers **HTTP 503**, i.e. a front end is
   alive and its backend is not. Nothing here is a sandbox restriction — the
   TCP connection reaches the host and is actively refused.

One internal inconsistency in Keychron's repo, for completeness: its own
generated `docs/repo-inventory.md` claims **734 data files** and lists the K9
Pro folder as holding `.dwg ×1, .pdf ×1, .stp ×3, .zip ×1`. That inventory
describes a filesystem that is not the public git tree. It is a useful
*manifest* of what exists upstream, and a misleading *index* of what is
downloadable.

### 2.3 Licence — reference only, never commit

Keychron's terms are **source-available, not open source**. Personal and
educational use is allowed; commercial use is allowed for *original compatible
accessories*; and *"you may not use these files to copy, manufacture, sell, or
distribute Keychron keyboards or mice, or substantially similar products, and
you may not use Keychron trademarks as your own branding."*

A v2 enclosure that holds a K9 Pro is an original compatible accessory, so
designing against the model is squarely permitted. **Redistributing the model
is not.** `v2/parts/models/` is gitignored and `git check-ignore` was run
against every file placed there.

### 2.4 Community CAD — nothing K9-specific

Searched GrabCAD, Printables, Thingiverse, Thangs, Sketchfab and MakerWorld for
K9 / K9 Pro and for the K7/K6/K2 siblings. **No K9 Pro model found anywhere.**

What does exist is a deep seam of generic 60% cases, nearly all descended from
one GH60 case, and they are useful for one reason: they independently confirm
the field size. A Thingiverse 60% case states its compatibility as *"plates of
standard size (i.e. 285.752 mm × 95.252 mm), such as the GH60 or DZ60"* — which
is the number §5 derives from the K9 Pro's own firmware, to three decimal
places. Two unrelated routes, same answer.

A **plate file** would have been nearly as good as a solid model, and in the
end was not needed: the layout is standard 60% ANSI in known key units (§1), so
the switch-field geometry is fully determined without downloading anything.

---

## 3. The numbers, as published

Extracted from the live keychron.com K9 Pro product page HTML on 2026-11-05.
The page renders its specs into `dl.mo-specs__list`, which is why a plain page
fetch returns nothing useful — the values are there, below the fold.

| Spec | Value | Tag |
|---|---|---|
| Dimension | **287 × 97.5 mm** (11.29 × 3.83") | [vendor] |
| Height without keycap (front) | **7.6 mm** (0.29") | [vendor] |
| Height without keycap (rear) | **13.3 mm** (0.52") | [vendor] |
| Angle | **3.4°, 5° and 8°** | [vendor] |
| Weight | **410 g** / 0.90 lbs | [vendor] |
| Build | **ABS + Aluminum frame, tray mount** | [vendor] |
| Keycap | Low-profile double-shot PBT | [vendor] |
| Switch | Gateron low-profile mechanical | [vendor] |
| Battery | 1550 mAh Li-polymer | [vendor] |
| Connectivity | Bluetooth / Type-C wired | [vendor] |
| Polling rate | 1000 Hz wired, **90 Hz Bluetooth** | [vendor] |
| Bluetooth device name | `Keychron K9 Pro` | [vendor] |
| Hot-swappable | YES | [vendor] |
| MCU | STM32L432 (128 KB Flash) | [vendor] |
| Bluetooth version | 5.1 | [third-party] |
| Switch pre-travel / travel | 1.7 ± 0.4 / 3.2 ± 0.2 mm | [third-party] |
| Switch force | red 50, brown 55, blue 65 gf | [third-party] |

Purchase options are only **Version** (RGB / White Backlight) × **Switch Type**
(Gateron low-profile, plain or hot-swappable) × **Switch Option** (colour).
**There is no frame choice** — every K9 Pro is ABS + aluminium frame. This
matters for §4.3.

The brown switch the user chose is the 55 gf tactile, and `keyboard.md` §7's
weight argument still holds: at **410 g** the K9 Pro is 26 g lighter than the
K7 Pro it recommended.

---

## 4. Reading those numbers honestly

### 4.1 The footprint is one key unit narrower than the K7 Pro

| | K7 Pro (65%) | K9 Pro (60%) | delta |
|---|---|---|---|
| width | 306 mm | **287 mm** | **−19 mm** |
| depth | 97.5 mm | **97.5 mm** | **0** |
| field | 16 u × 5 u | **15 u × 5 u** | −1 u = 19.05 mm |
| weight | 436 g | 410 g | −26 g |

One key column, 19.05 mm, reported as 19 mm. The depth is identical because
both are 5-row low-profile boards in the same case family. **This is the single
most useful fact in this document**: `keyboard.md` §5.1 already costed the K7
Pro into a v1-style bay, and that work transfers directly.

### 4.2 The wedge is in the top shell, not the sitting plane — a correction

`keyboard.md` §5.2 warns: *"these boards are wedged. A flat pocket floor will
not seat them — the floor needs a matching ramp or a stepped shelf."* **For the
K9 Pro that is the wrong way round.**

Both published heights are measured from the surface the keyboard rests on
(7.6 mm at the front edge, 13.3 mm at the rear). A single unambiguous "height"
at each edge only means something if the underside is one flat plane. It is a
tray-mount case. And the 3.4° no-feet typing angle is then exactly the slope of
the top shell:

```
atan((13.3 − 7.6) / 97.5) = atan(0.05846) = 3.3458°   [derived]
                                  published: 3.4°     [vendor]
```

Three independently published numbers closing to within 0.06° is a strong
indication they describe the same object, and that the slope lives in the top
shell.

**So the pocket floor should be flat**, and the wedge only shows up as a lip
height that varies front to back around the aperture. The 5° and 8° settings
are the flip-out feet and are irrelevant to a bay — unless the feet have to be
removed, which is a real question and is on the calipers list.

Caveat, stated rather than buried: this is **[derived]**. The height figures
almost certainly include the rubber feet, so the bare shell is slightly shorter
than 7.6/13.3 and there are feet to clear or remove. §7 items 4–6.

### 4.3 Three numbers on Keychron's own page that are rejected

The K9 Pro page carries, in the same spec block:

- `Dimension (Aluminum frame): 316.6 × 126.91 mm (12.46 × 4.99")`
- under **Front Height**: `Height incl. keycap: 32 mm (1.26")`
- under **Back Height**: `Height incl. keycap: 42.07 mm (1.66")`

All three are rejected. Four independent reasons:

1. **There is nothing for the "Aluminum frame" figure to apply to.** The K9 Pro
   is not sold in plastic and aluminium variants (§3). The build is stated once,
   as "ABS + Aluminum frame". A separate aluminium-frame footprint is a line
   from a product that *has* that choice.
2. **Keychron's real aluminium-frame delta is a few millimetres, not thirty.**
   The K6 Pro page, live now, lists both of its variants:
   `Aluminum frame version: 317 × 107 mm` and
   `Plastic frame version: 313 × 104 mm` — **+4 mm wide, +3 mm deep**. The K9
   Pro line would imply **+29.6 × +29.4 mm**, an order of magnitude out, and
   non-uniform (+10 % wide, +30 % deep).
3. **The keycap heights are geometrically impossible.** Over a bare case of
   7.6 / 13.3 mm they would mean **24.4 mm and 28.8 mm of keycap** standing
   proud. A Gateron low-profile switch plus a low-profile cap is about 10 mm —
   and Keychron publishes exactly that for the sibling K7 Pro: 18.2 − 8.8 =
   **9.4 mm** front, 23.2 − 13.2 = **10.0 mm** rear. They are also not a
   front/rear pair of one quantity; they differ by 10 mm where the case differs
   by 5.7.
4. **The page is demonstrably broken at that exact spot.** Two Shopify
   template errors — *"Liquid error (sections/pf-e8649df3 line 72):
   metafield_tag requires a 'field' parameter for metafields of type
   'metaobject_reference'"* — render as visible text immediately adjacent to
   the spec block. The spec metafields on this page are mis-bound.

The K7 Pro page, used as a control, is in the older spec format and labels its
figures properly: `Height incl. keycap (front): 18.2 mm` / `(rear): 23.2 mm`.
The K9 Pro's new-format block has lost the `(front)`/`(rear)` qualifiers, which
is consistent with values having been pulled from the wrong source.

**Consequence:** the bare-case numbers (7.6 / 13.3) are trustworthy — they
reproduce the published typing angle. The keycap-inclusive numbers on
keychron.com are not. §4.4 rebuilds them.

### 4.4 Keycap-top height, rebuilt by two routes

| Route | front | rear |
|---|---|---|
| **A** — third-party database (green-keys) [third-party] | 19.8 | **23.5** |
| **B** — K7 Pro's published protrusion (9.4 / 10.0) added to the K9 Pro case [derived] | 17.0 | **23.3** |
| keychron.com as printed — rejected, §4.3 | 32 | 42.07 |

**The rear agrees to 0.2 mm across two unrelated routes.** The rear is also the
number a bay cares about, because it is the tallest point:

> **Tallest point of the keyboard ≈ 23.5 mm above its own resting plane.**
> Reasonable confidence.

The **front is unresolved — the two routes are 2.8 mm apart** and it is the
weakest number in this work. `keyboard_blank.scad` defaults to the larger
(19.8), because for an aperture an over-estimate costs clearance and an
under-estimate costs a reprint. It is §7 item 1.

---

## 5. Geometry, derived and cross-checked

### 5.1 The switch field is the GH60 standard, exactly

```
field_w = 15 u × 19.05 mm = 285.75 mm      [derived from qmk]
field_d =  5 u × 19.05 mm =  95.25 mm      [derived from qmk]
```

Independently, the 60% case community specifies its plates as
**285.752 × 95.252 mm** (GH60 / DZ60) **[third-party]**. Agreement to three
decimal places, from the firmware on one side and a decade of community cases
on the other.

**Why this matters beyond a sanity check:** it means a generic 60% plate, a
GH60 plate DXF, or any plate generator fed a standard 60% ANSI layout produces
the K9 Pro's switch field correctly. The keyboard is a standard object.

### 5.2 The bezel comes out implausibly thin — flagged

```
bezel_x = (287   − 285.75) / 2 = 0.625 mm per side        [derived]
bezel_y = ( 97.5 −  95.25) / 2 = 1.125 mm front and back  [derived]
```

0.625 mm of aluminium frame beside the outermost switch is thin enough to
doubt. The likeliest explanation is that **287 is rounded** (285.75 + a 1–2 mm
frame would print as 287–288). The derivation is sound; the input has about
±0.5 mm of rounding in it. For a bay this is exactly the tolerance that
decides whether the keyboard drops in, so it is §7 item 2 — and it is the
reason not to design a zero-clearance pocket off a vendor web page.

### 5.3 What it does to the deck

Using the repo's own v1 bay rules (`cad/parameters.scad:748-749`), purely for
comparability with `keyboard.md` §5.1:

```
kbd_pocket_w = 287.0 + 1.35 = 288.35
kbd_pocket_h =  97.5 + 0.95 =  98.45
body_w       = max(288.35, 93.5) + 6.40 = 294.75
body_h       =  98.45 + 81.10           = 179.55
```

| | deck W × H | vs K7 Pro plan |
|---|---|---|
| **K9 Pro**, v1-style bay | **294.75 × 179.55** | **19.05 mm narrower, same depth** |
| K7 Pro, v1-style bay (`keyboard.md` §5.1) | 313.8 × 179.6 | — |

**Read `body_h` as comparison only, not as the v2 answer.** That formula
encodes v1's stack (keyboard bay in front, board bay behind, one spine).
Arrangement A puts the control band *between* display and keyboard, so v2's
depth is the keyboard, plus the control band, plus the display band — a
different sum, and the control-row-within-45 mm-of-the-keyboard constraint
from the brief drives it. The honest transferable result is the **width:
294.75 mm**, and that the keyboard contributes **98.45 mm** of pocket depth.

---

## 6. The stand-in: `v2/cad/keyboard_blank.scad`

Written, rendered, and verified. Three parts:

| `part=` | what it is | use |
|---|---|---|
| `envelope` *(default)* | everything the keyboard occupies | cut the bay |
| `shell` | case body only | the volume that sits down in the pocket |
| `keycaps` | the band inset to the switch field | what must stand proud through the aperture |

Construction: a flat-bottomed solid whose top slopes linearly front to rear,
built by extruding the side profile (`rotate([90,0,90]) linear_extrude`), so
the top face is exactly planar and the solid is manifold by construction. The
keycap band is modelled from z = 0 rather than from the shell top, so the union
is closed with no seam. Keycaps are deliberately not modelled.

Every parameter defaults to a tagged number, with `[UNSOURCED]` called out in
the file. `corner_r` defaults to **0**, which is both honest (not published)
and conservative (sharp corners are the largest envelope).

### Render verification

OpenSCAD 2021.01. CGAL reports `Simple: yes` for the unioned envelope.

| part | facets | X span | Y span | Z span | expected |
|---|---|---|---|---|---|
| `envelope` | 28 | 287.000 | 97.500 | 23.500 | 287 × 97.5 × 23.5 ✓ |
| `shell` | 12 | 287.000 | 97.500 | 13.300 | 287 × 97.5 × 13.3 ✓ |
| `keycaps` | 12 | 285.750 | 95.250 | 23.500 | field, offset 0.625 / 1.125 ✓ |
| `shell`, `corner_r=2` | 44 | 287.000 | 97.500 | 13.300 | rounded path also non-empty ✓ |

Beyond the bounding box, the wedge itself was checked in the mesh:

```
z values on FRONT edge (y=0)    : [0.0, 7.6]
z values on REAR  edge (y=97.5) : [0.0, 13.3]
flat bottom                     : True
echoed wedge_deg                : 3.34579
```

A bounding box alone would have passed a box; this confirms the slope and the
flat underside are really in the solid.

```
openscad -D 'part="envelope"' -o blank.stl v2/cad/keyboard_blank.scad
```

*Note: OpenSCAD was not installed in this container; it was installed from
Ubuntu noble/universe to run this check. If `tools/validate.py` is expected to
run here, it needs the same.*

---

## 7. NOT SOURCED — get the calipers

Ten minutes, a caliper, and a flat surface. **Datum everything to the flat
surface the keyboard rests on, with the flip-out feet folded away**, because
that is the plane the vendor's own heights are measured from and the plane a
pocket floor will present.

Work in this order — each step sets up the next.

1. **Keycap top at the front edge.** The weakest number in this work: 17.0 vs
   19.8 mm, 2.8 mm apart (§4.4). Board flat on the surface, caliper depth rod
   or a steel rule from the surface to the top of the **spacebar**. Also take
   the rear (top of a number-row cap) and expect ≈ 23.5; if the rear confirms,
   trust the front reading too. *Closes the aperture height.*
2. **Outside footprint, to 0.1 mm.** Caliper jaws across the aluminium frame at
   mid-length, then front-to-back at mid-width. Expect ≈ 287 × 97.5. Take all
   three positions on each axis — if the frame is not parallel, the largest
   reading is the one the pocket must clear. *Closes §5.2, where the derived
   0.625 mm bezel is suspect.*
3. **Case height at both edges, without keycaps in the way.** Surface to the
   top of the aluminium frame at the **front edge mid-width**, then at the
   **rear edge mid-width**. Expect 7.6 and 13.3. The difference over the
   measured depth gives the wedge angle; it should land near 3.35°.
   *Confirms §4.2.*
4. **Is the underside flat?** Put the board upside down on the flat surface and
   try to rock it. Then caliper the **rubber foot thickness** and note whether
   step 3's readings included them. *Decides whether the pocket floor is flat
   (§4.2 says it is, by inference only) and whether the floor needs foot
   recesses.*
5. **Rubber foot positions.** Distance from each foot's centre to the nearest
   two edges, and the foot diameter. Four of them. *Only needed if you recess
   rather than remove them.*
6. **Flip-out feet.** Folded thickness, footprint, and whether they unclip.
   *If they do not come off, the pocket floor has to swallow them.*
7. **Corner radius.** Roll a radius gauge, or trace a corner on paper and fit a
   circle. *`corner_r` defaults to 0, which is safe but loses a millimetre of
   snugness at each corner.*
8. **USB-C port and the mode switch.** Height of the port centre above the
   surface, and its distance from the left edge; same for the power/mode slide
   switch. *Decides whether the bay can enclose the rear edge at all, or has to
   open out — the board must stay chargeable and switchable in the deck.*
9. **Aluminium frame step.** Does the frame stand proud of the top plate, and
   by how much? *A proud frame is the surface the aperture lip actually
   contacts.*
10. **If and only if the "Aluminum frame: 316.6 × 126.91 mm" line ever turns
    out to describe something you received**, measure it and this whole section
    changes. §4.3 argues it does not, on four grounds, but it is Keychron's own
    page and the user holds the object.

Items 1, 2 and 8 are the ones that actually gate cutting a bay. 3 and 4 confirm
inferences. The rest are refinements.

**All ten collapse to nothing if the `K9-Pro-US-Top-Case.pdf` from §2.1
downloads.** Try that first.

---

## 8. Sources

**Keychron, official**
- [K9 Pro product page](https://www.keychron.com/products/keychron-k9-pro-qmk-via-wireless-custom-mechanical-keyboard) — §3 spec table, extracted from page HTML 2026-11-05
- [K7 Pro product page](https://www.keychron.com/products/keychron-k7-pro-qmk-via-wireless-custom-mechanical-keyboard) — §4.3 control, §4.4 route B
- [K6 Pro product page](https://www.keychron.com/products/keychron-k6-pro-qmk-via-wireless-custom-mechanical-keyboard) — §4.3 aluminium-frame delta evidence
- `Keychron/Keychron-Keyboards-Hardware-Design` — repo README, `docs/repo-inventory.md`, and `K-Pro-Series/K9 Pro/README.md`, all via raw.githubusercontent.com
- `Keychron/qmk_firmware` @ `bluetooth_playground`, `keyboards/keychron/k9_pro/` — §1, via raw.githubusercontent.com

**Third-party**
- [green-keys.info K9 Pro](https://green-keys.info/en/keyboard/keychron-k9-pro) — §4.4 route A, switch travel and force, Bluetooth version
- [TechSpot on the Keychron release](https://www.techspot.com/news/112022-keychron-shares-3d-keyboard-blueprints-github-opening-hardware.html) and [pyshine summary](https://pyshine.com/Keychron-Open-Source-Keyboard-Hardware-Design/) — located the repository
- jsDelivr packages API, `data.jsdelivr.com/v1/packages/gh/Keychron/Keychron-Keyboards-Hardware-Design@main` — §2.2 file listing
- [Thingiverse "The Morrel" 60% case](https://www.stlfinder.com/model/the-morrel-a-sculpted-60-keyboard-case-modified-tray-or-ring-gasket-mount-OkEICbXk/4037992) — GH60 plate standard 285.752 × 95.252 mm, §5.1 cross-check

**In this repo**
- `v2/research/keyboard.md` §4.1, §5.1, §5.2, §6, §7 — the K7 Pro baseline this corrects and extends
- `cad/parameters.scad:748-749` — the bay clearance rules used in §5.3
- `v2/parts/models/keychron-k9pro/` — gitignored; holds the Keychron download manifest, the QMK layout, and the rendered envelope STL

**Reachability, for the record:** `raw.githubusercontent.com` worked throughout.
`github.com` and `api.github.com` were not used. `www.keychron.co:34568`
refused TCP connections on both attempts; `gitcode.com` returned HTTP 418.
