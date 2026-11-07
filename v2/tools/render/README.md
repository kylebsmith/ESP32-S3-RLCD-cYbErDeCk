# `v2/tools/render` — the photoreal assembly render

OpenSCAD is the right tool for the geometry and the wrong tool for the picture.
It has no materials, no textures, no transmission and no tone mapping, so it
cannot show clear acrylic as clear, a PCB as green, or a silkscreen legend as a
silkscreen legend. The brief asks for both:

> "I want everything rendered flawlessly … a extremely realistic full render
> that is also perfect for 3D printing and laser cutting of acrylic layers"

So the geometry stays in OpenSCAD and the picture moves to a real physically
based renderer: **three.js r0.160.1**, `MeshPhysicalMaterial`, driven headlessly
through **Playwright/Chromium**.

---

## 0. The contract, in one paragraph

**The render is of the same geometry the fabricator gets.** Every fabricated
body in the scene is the exported STL from `v2/export/stl/`. Nothing is
re-modelled in JavaScript — `scene/scene.js` contains no dimension at all and
is a material and camera script. Every number the scene uses arrives in
`manifest.json`, which `render.py` obtains by asking OpenSCAD to echo the
values in `v2/cad/parameters.scad`. Anything the design calls for and no file
models is **not drawn**; it is listed as ABSENT, with its size, in every
view's sidecar JSON and on the annotated views.

---

## 1. Run it

```sh
# the three pictures, into v2/export/render/
/usr/bin/python3.11 v2/tools/render/render.py

# measure what came out
/usr/bin/python3.11 v2/tools/render/check_render.py

# watch every check fail on a deliberately broken render, then pass again
/usr/bin/python3.11 v2/tools/render/check_render.py --prove
```

Other things you will want:

```sh
# one view
/usr/bin/python3.11 v2/tools/render/render.py --views hero

# every view, including the measurement masks
/usr/bin/python3.11 v2/tools/render/render.py --views all

# an arbitrary camera; these four flags steer the 'orbit' view and nothing else
/usr/bin/python3.11 v2/tools/render/render.py --views orbit \
    --azimuth 120 --elevation 18 --explode 40 --annotate

# what is in the scene and where it came from, without rendering
/usr/bin/python3.11 v2/tools/render/render.py --list-bodies

# render a frozen snapshot instead of the live export directory
/usr/bin/python3.11 v2/tools/render/render.py --stl-dir /some/snapshot
```

`--no-export` refuses rather than re-cutting a stale part. `--stl-dir` renders
a snapshot and does not check it for staleness at all, because a snapshot is
not live; that is what makes `--prove` hermetic.

A cold run takes about **3½ minutes**: ~105 s exporting the 18 reference
bodies, ~125 s if `v2/tools/export.py` has to re-cut the carrier, ~35 s of
rendering. A warm run is **~35 s** — the reference bodies are cached on source
mtime in `v2/export/render/.bodies/`, which is deliberately *not* under the
output directory, because they depend only on `v2/cad/parts/*.scad` and
`bodies.scad` and never on where the PNGs go.

### What it needs, and what it must not be given

| | |
|---|---|
| `/usr/bin/python3.11` | trimesh (meshes), numpy. **Not** PIL — see §7. |
| `openscad` 2021.01 | `--export-format=echo` for the parameters, `binstl` for the bodies |
| `node` + `playwright` | **pre-installed and version-matched.** Do not `npm install playwright` and do not run `playwright install`: another version will not find this Chromium. |
| `convert` (ImageMagick) | PNG → PPM for `check_render.py`. §7. |
| network, once | `render.py` fetches three.js r0.160.1 into `scene/vendor/` on first run and never again. That directory is gitignored: 1.3 MB of vendor JavaScript is a build input, not source. Delete it and it refetches. |

---

## 2. The views

`render.py` has one orbit parameterisation and several named points on it.

```
azimuth    0°  = from −Y, the user's own side (the keyboard is at −Y),
               turning toward +X
elevation  0°  = in the deck's plane;  +90° = straight down the Z axis
explode        mm the three layers are separated along Z
```

`up` is derived rather than fixed, because `[0,0,1]` is degenerate at
elevation 90° and the plan view would come out silently rolled.

### Pictures

| view | camera | what it is for |
|---|---|---|
| `hero` | az 38°, el 34°, perspective 26° | The product shot. The acrylic is the subject: you should be able to read the green of the encoder breakouts and the brass of the pot bushings *through* 12.700 mm of it, and the rim light exists to catch the slab's edge, which is the only place the thickness is directly visible. |
| `plan` | az 0°, el 90°, **orthographic**, 330 mm across | The instrument as the player sees it, and the view where the silkscreen is legible. Annotated. |
| `exploded` | az 34°, el 24°, perspective, explode 70 mm | The three layers apart: back panel below, carrier with everything mounted to it in the middle, acrylic above. The components do **not** separate from the carrier, because they are screwed to its bosses and an explosion that floats a board away from the bosses it is fastened to is a picture of an assembly nobody will build. Annotated. |
| `orbit` | yours | The free camera. `--azimuth/--elevation/--explode/--annotate` steer this view and no other, so a flag on the command line can never move a measurement view's camera out from under the checker. |

### Measurement views

These are **not pictures**. Flat white on black, orthographic, device pixel
ratio pinned to 1, tone mapping off, and the frustum stated in millimetres so
the sidecar can record an exact `mm_per_px`. They are the *same scene graph,
the same meshes and the same matrices* as the pictures — only the camera and
the materials change.

| view | what it isolates |
|---|---|
| `mask_slab`, `mask_backpanel`, `mask_carrier` | one fabricated part alone, in plan |
| `mask_elev_deck` | the three fabricated layers in elevation, and nothing else |
| `mask_plan`, `mask_elev` | the whole assembly, for eyeballing |
| `plan_noacrylic` | the transmission control: the `plan` frame with the acrylic taken out |

---

## 3. What is in the picture, and where each body came from

Three classes, and the distinction is the whole point. Every view's sidecar
JSON (`v2/export/render/<view>.json`) names the class, the source and the
SHA-256 of every body; the annotated views print the summary into the image.

### `EXPORTED` — this *is* the part

Loaded from `v2/export/stl/<name>.stl`, produced by `v2/tools/export.py`.

| body | material | from |
|---|---|---|
| `slab` | `acrylic_clear`, 12.700 mm | `v2/cad/slab.scad` |
| `carrier` | `petg_carrier`, 15.875 mm | `v2/cad/carrier.scad` |
| `backpanel` | `acrylic_black`, 3.175 mm | `v2/cad/backpanel.scad` |

`render.py` will run `v2/tools/export.py` itself if an STL is older than its
source, and refuses outright under `--no-export`. The Z placement is **solved,
not assumed**: `tz = declared_z0 − measured_stl_zmin`, where the declared z0
comes from `parameters.scad` and the zmin is measured off the mesh. The
carrier's solved offset comes out as 0 because `carrier.scad` already
translates itself into the deck frame — that is a result, not an assumption.

### `REFERENCE` — the project's own models of bought components

Exported by `bodies.scad`, which `use`s `v2/cad/parts/*.scad` and calls the
modules those files already expose, once per material group. `use` rather than
`include`, so nothing in `v2/cad/parts/` can be shadowed from here — the
read-only relationship is enforced by the language and not by good intentions.
**No geometry is authored in `bodies.scad`;** the only decision it makes is
which bodies share a material, and that grouping follows the `color()` calls
those files already wrote.

| source | bodies |
|---|---|
| `waveshare_rlcd42.scad` | `disp_pcb`, `disp_glass`, `disp_metal`, `disp_black`, `disp_buttons`, `disp_silk` |
| `feather_rp2040_dvi.scad` | `fw_pcb`, `fw_metal`, `fw_black`, `fw_buttons`, `fw_silk` |
| `k9pro_internals.scad` | `kbd_frame`, `kbd_pcb`, `kbd_switches`, `kbd_keycaps`, `kbd_sockets`, `kbd_ports`, `kbd_battery` |

These are reference bodies, deliberately rough away from the mounting
interfaces — that is their own files' stated rule and the house library's. The
silkscreen you can read in the `plan` view is real `text()` geometry from those
files, given its own white material; it is `[silk]`-tagged there, read off the
vendors' own renders, and is the one thing in this render that is a picture of
a picture.

### `VENDOR` — house asset library meshes, re-materialised, not re-modelled

From `/home/user/kylebsmith/cmtl-3d-assets`. A single STL carries a single
material, so each is split **by face** into measured z-bands: every triangle
written out is a triangle read in, unchanged, and the only decision is which
output file it goes to. The band edges are datums `parameters.scad` already
declares, anchored where necessary on a measured feature of the mesh itself.

| mesh | bands |
|---|---|
| Adafruit 5880 rotary encoder ×3 | library z `0 … pcb_t` → PCB green; the rest → satin |
| Adafruit 5295 NeoSlider ×1 | same |
| Adafruit 562 16 mm pot ×3 | below the clamping face (`zmin + pot_shoulder_below`) → satin can; above → brass |

Split by face and not by connected component because **scipy is absent** on
this interpreter, so `trimesh.split()` raises (`v2/docs/fabrication.md` §4.7).

### `ABSENT` — a labelled hole, not a stand-in

Nothing in this repository models these, so nothing is drawn. Each is listed
with its size, taken from the parameter dump, in every sidecar.

| | |
|---|---|
| **6× knob** | Ø25.400 brim × 19.050, standing 6.350 proud, in a Ø22.000 bore. `parameters.scad` §5 dimensions it; no `.scad` models it. |
| **1× fader cap** | 10.000 wide, in a 12.000 × 52.000 slot. §10 dimensions the slot, not the cap. |
| **1× wiring harness** | `carrier.scad` models the **channels** the wires lie in, and those *are* in these renders as recesses in the carrier's top face. Nothing models the wires. `v2/parts/wiring.md` is a route, not a solid. |
| **16× fastener** | and the heat inserts they thread into. `08_cutouts/` has the clearance and insert *cutouts*, not the screws. |
| **4× foot** | at least 3.090 tall, or the deck rocks on the display's own 18650 holder. |

The brief asks to see the wiring through the acrylic. **This render shows the
wire channels, not the wires.** That is the honest state of the design, and a
drawn-in harness would be the exact kind of stand-in that later disagrees with
the real thing.

---

## 4. Materials: which numbers are physical and which are artistic

The table lives in `render.py` (`MATERIALS`) so it can be shipped in the
manifest; `scene.js` turns a row of it into a `MeshPhysicalMaterial`.

### The acrylic — the hero of the image

Every optical constant below is **sourced**, from Röhm's own property table
(DIN 5036 Part 3 / ISO 489) by way of `v2/parts/stack.md` §4.1.

| parameter | value | class | why |
|---|---|---|---|
| `transmission` | **0.92** | `[VENDOR roehm-gsxt]` | τ<sub>D65</sub> ≈ 92 % for both cast and extruded PMMA. The remaining 8 % is 4 % of surface reflection at *each* face, which is what the other 0.08 goes to. |
| `ior` | **1.491** | `[VENDOR roehm-gsxt]` | n<sub>D</sub><sup>20</sup> for PMMA. Drives the Fresnel term *and* the refraction. |
| `thickness` | **12.700** | `[CAD-EXACT]` | `acrylic_t`, from the parameter dump. **This is the number that separates a half-inch slab from a sheet of cling film:** `thickness` is in world units, the scene is in millimetres, and with it left at 0 the identical material reads as a glass film laid over the parts. That is the standard way a clear-acrylic render lies. |
| `attenuationDistance` | **effectively infinite**, no tint | `[VENDOR roehm-gsxt]` | PMMA's absorption in the visible is **< 0.05 %**. `stack.md` §4.1 conclusion 3: *"There is no green edge … the thick edge really will read as a solid colourless bar."* The artistic instinct is to tint thick acrylic green; for PMMA that is simply wrong, and this is the line where it would have crept in. |
| `metalness` 0, `clearcoat` 0, `specularIntensity` 1 | | `[PHYSICAL]` | A bare dielectric with no coating; F0 comes from the IOR. |
| `roughness` | 0.02 | `[ARTISTIC]` | As-cast faces are optically smooth. The laser-cut *edges* are not, and this render does not distinguish them — see §6. |
| `envMapIntensity` | **0.45** | `[ARTISTIC]` | **The one place this render knowingly leaves physics.** At 1.0 the studio IBL's broad soft source reflects off the whole top face and veils the interior — which is exactly what a real softbox over a real polished sheet does, and exactly what the brief most wants not to happen. 0.45 keeps the Fresnel edge reflection, where the 12.700 mm actually reads, and takes the sheen off the middle. A photographer solves this with a polarising filter; this is the render's version of one. |

### Everything else

| material | class | note |
|---|---|---|
| `petg_carrier` `#23262a`, roughness 0.78 | colour `[REPO]`, finish `[ARTISTIC]` | `v2/parts/stack.md` §5.6 specifies PETG, *"matte black or very dark grey"*, because *"through clear acrylic a dark matte plane reads as a void, and the coloured wires and nickel nuts become the only things with brightness"*. |
| `acrylic_black` | `[PHYSICAL]` ior, `[ARTISTIC]` finish | Same substrate, same 1.491, transmission 0. |
| `pcb_green` `#1b3a2a`, `pcb_navy` `#0d2d44` | `[REPO]` | The colours `fw_board()` and `ws_board()` already chose, carried over so the render and the OpenSCAD preview agree. Likewise `metal_bright` `#c8c8cc`, `black_plastic` `#101010`, `button_light` `#d8d8d8`. |
| `glass_dark` | `[ARTISTIC]` | An **unpowered** 4.2 in IPS panel is a dark glass rectangle, and it is rendered as one. Nothing in this repository holds a frame of its output, so none is invented. |
| `aluminium`, `keycap`, `brass`, `switch_body`, `pouch_cell`, `silk_white`, `component_satin` | `[ARTISTIC]` | Plausible finishes. `component_satin` is explicitly the "one material for a whole vendor mesh" case and is named so it is obvious in the manifest. |

### Lighting — all `[ARTISTIC]`

A neutral studio, not a CAD viewport. The dominant term is an IBL: three.js's
own `RoomEnvironment` through `PMREMGenerator`, a neutral grey room with broad
area sources. **It is not decoration** — it is the only thing a `transmission`
material has to refract at the edges of the frame, and point lights alone
would give clear acrylic nothing to show. Then a key (warm, high, camera
left), a cool fill at a fifth of it, and a rim that exists for exactly one
feature: the slab's 12.700 mm edge. ACES filmic tone mapping at exposure 0.78,
because a studio render with specular highlights on aluminium will clip in
sRGB, and a clipped highlight is what makes a render look like a screenshot.
The backdrop is a vertical gradient on a canvas — a `Texture` on
`scene.background` and deliberately **not a mesh**, because a floor plane
would be a body in the scene and this tool's contract is that every body in
the scene is a part.

---

## 5. The checks — and they measure the PNG, not the source

A check that computes from the same parameters as the thing it checks is worse
than no check at all. `check_render.py` does not import `render.py`, does not
read `parameters.scad`, and contains no deck dimension. It reads two kinds of
artefact off disk — **the PNGs** in `v2/export/render/` and **the STLs** in
`v2/export/stl/` — and asks whether they are pictures of the same object.

The only thing it takes from the renderer's own side is the camera
calibration, `mm_per_px`, in each view's sidecar. Without a stated scale a PNG
has no dimensions at all. That is the renderer's claim about its own camera,
and every check is a test of that claim against geometry the renderer did not
produce.

| | check | measured | against | observed |
|---|---|---|---|---|
| **C1** | silhouette scale and position | the part's own flat-white mask: width, height and bounding-box centre, in mm | that part's exported STL bounding box | slab PNG 304.837 × 190.575 vs STL 304.600 × 190.375 — **max Δ 0.238 mm**, tolerance 0.463 |
| **C2** | through-hole count | enclosed dark regions in the same mask | the STL's **genus**, free from its Euler characteristic, which for a prismatic plate *is* its through-hole count | slab **14 = 14**; back panel **17 = 17** |
| **C3** | stack height | the elevation mask of the three fabricated layers | the **sum of those three STLs' own thicknesses** | PNG 31.969 vs STL 31.750 mm, Δ 0.219, tolerance 0.669 |
| **C4** | transmission | high-passed detail inside the slab's footprint, `plan` vs `plan_noacrylic`, tile-matched with a search | each other | detail survives at **1.215** of the un-occluded frame (floor 0.35); over 24 tiles the median match is **r = 0.887** (floor 0.60) at a median refraction displacement of **0.922 mm** |
| **C5** | not degenerate | every PNG | itself | pictures on level count and modal share; masks on **lit fraction**, because a mask is *meant* to be binary |
| **C6** | the fabricator's **part** was used | the SHA-256 and the geometry hash the sidecar records for every EXPORTED body, plus the agreement of all 11 sidecars | `v2/export/stl/<name>.stl` **now** | identical |

**Why C4 is high-passed and shift-tolerant, and this is the interesting one.**
The naive version — correlate the two frames' raw luminance, pixel for pixel —
is wrong twice.

* The slab's own top face carries a broad smooth shading gradient from the key
  light. That is the slab's surface, not what is under it, and it does not
  belong in the measurement either way. So both frames are high-passed first:
  the image minus its own local mean over a 1.500 mm box. Observed: changing
  nothing but the exposure moved the raw correlation from 0.695 to 0.487, which
  is a metric measuring the lighting rather than the claim.
* **12.700 mm of PMMA at n = 1.491 refracts.** The image of a board seen
  through the slab is *not where the board is*. Measured on this render, over
  the Feather, the displacement is about (+0.825, −0.413) mm and it varies
  across the frame. Fixed-registration correlation reads **0.031** on a
  perfectly good render; the same pair correlates **0.849** once the offset is
  allowed for. The naive check would have failed the render for doing the one
  thing the material is supposed to do.

So the measurement is per tile with a search: for each ~13 mm tile of the
slab's footprint, find the offset that best matches the un-occluded frame, and
report the median correlation **and the median displacement**. The displacement
is then a result in its own right, and it is bounded — a large one would mean
something was wrong.

**Why C2 is there at all.** C1 cannot see the renderer loading the wrong mesh:
the slab and the back panel have identical 304.600 × 190.375 outlines. Their
hole counts are 14 and 17. Swapping them is invisible to a bounding box and
loud in a hole count.

**C6 is not theoretical, and its first version cried wolf.** On the first full
run it fired because the carrier had been re-exported three minutes after the
render staged its mesh. The reading at the time — "the picture was of a
carrier that no longer existed" — was **wrong**, and so was comparing file
bytes at all.

`export.py`'s carrier is **not byte-reproducible**. Three consecutive exports
of an unchanged `carrier.scad` gave three different `sha256` —
`deb5387abc6019a2`, `70aac9aa02a7e1a0`, `38c9d406e30bddfa` — and **one
geometry**: 35868 faces, euler 35868, bounds and volume equal to six decimal
places every time `[MEASURED 2026-11-07]`. OpenSCAD's CGAL tessellation does
not promise a triangle order and this build does not deliver one.

So the original C6 was guaranteed to go red on every re-export whether or not
the part changed, and a check that cries wolf is a check that gets switched
off. C6 now keeps the byte hash as its **fast path** — equal bytes still prove
equal geometry — and when the bytes differ it compares a canonical hash of the
**triangle set** (`render.geom_sha()`: coordinates rounded to 6 dp, each
triangle rotated to its lexicographically smallest corner so winding survives,
then the triangles sorted). Measured sensitivity: a vertex moved **1e-3 mm**
fails; **1e-7 mm**, which is inside the float32 STL round-trip slop of
2.44e-5, does not.

C6 also used to read **one** sidecar, on the stated grounds that they all
carry the same body list — an assumption in a comment that nothing tested. A
`sha256` that was honest in `exploded.json` and falsified in `hero.json` alone
passed silently, 21/0 `[MEASURED 2026-11-07]`. The list is now asserted equal
across all 11 sidecars before any one of them is treated as the proof.

### Proving they fire

```
/usr/bin/python3.11 v2/tools/render/check_render.py --prove
```

`render.py --break <mode>` writes a knowingly wrong render — with `broken` set
in the manifest and in every sidecar, so a broken PNG can never be mistaken
for a good one. `--prove` renders each break into a scratch directory,
confirms the check that should fire *does* fire, then renders clean and
confirms everything passes.

**It freezes the meshes first.** `v2/export/stl/` is a live directory that
another author's `export.py` rewrites — it did so twice while this was being
written — so `--prove` copies the three meshes into its scratch tree and
renders *and* checks that snapshot throughout, via `render.py --stl-dir`. A
proof whose inputs move underneath it proves nothing.

Observed, on the run that this README records:

| break | fires | what it is | what the check said |
|---|---|---|---|
| `scale` | **C1** | every matrix × 1.02. The PNG still looks entirely plausible. | PNG 310.860 × 194.040 vs STL 304.600 × 190.375, **Δ 6.260** against a 0.710 tolerance |
| `swap` | **C2** | slab and back panel staged into each other's filenames. | slab mask shows **17** holes against genus **14**; back panel mask shows **14** against genus **17**. C1 passed throughout — the outlines are identical. |
| `z` | **C3** | the slab lifted 6 mm, opening a gap between the rim and the acrylic. Invisible in plan, invisible in the hole count. | PNG **37.950** mm against 31.750 of STL thickness |
| `transmission` | **C4** | the acrylic's transmission set to 0: the standard clear-acrylic render lie, with every other material still correct. | **0 detail-bearing tiles** (need ≥ 8), median r 0.000. Note the high-frequency *energy* stayed at 1.543 — a mirror-finish opaque slab still has bright edges round its own holes — which is exactly why that term alone is not sufficient and the tile match is what catches it. |
| `blank` | **C5** | nothing drawn. What a lost WebGL context looks like — otherwise a clean run and a valid PNG. | every mask **0.00 % lit** |
| `stl` | **C6** | a hand-edited copy of the exported mesh in the scratch tree — **one vertex moved 1.000 mm**; `v2/export/stl/` is never touched. | `slab` rendered geometry `8055862a…`, on disk `ba85f88f…`; `carrier` and `backpanel` still matched, so the failure is localised to the mesh that moved |

This break used to rewrite 8 bytes of the STL's 80-byte **header**, which
changed the file's `sha256` while leaving every triangle alone. That proved
only that C6 could notice a byte — the one thing it must *not* fail on, now
that `export.py` is known not to be byte-reproducible. A part the fabricator
never receives is a part whose **geometry** differs, so the break moves a
vertex.

Restored, with no break: **17 passed, 0 failed.**

Four further breaks, applied to the emitted artefacts rather than through
`--break`, to exercise terms the table above never reaches
`[MEASURED 2026-11-07]`:

| break | fires | what the check said |
|---|---|---|
| `mask_slab.png` translated 4 px (0.825 mm), **same size** | **C1** | centroid `+0.825` vs STL `+0.000`, Δ 0.825 against 0.463. The `scale` break only ever exercised C1's *size* term; this is its *position* term. |
| a black disc painted inside the slab's silhouette | **C2** | **15** enclosed regions against genus 14 — a feature drawn that is not in the part |
| `plan.png` Gaussian-blurred (a *frosted* slab, not an opaque one) | **C4** | median r **0.394** (floor 0.60) and detail ratio **0.171** (floor 0.35). The `transmission` break collapses the tile count to 0; this one leaves 15 tiles and fails on the match, so both terms are now known to bite. |
| `hero.png` flattened to one colour | **C5** | modal colour **100 %**, **1** level. `--break blank` only ever blanked the *masks* — `plan.png` kept passing — so C5's picture branch had never been seen to fire. |

---

## 6. The honest limits

Things this render cannot show, in roughly decreasing order of how likely they
are to mislead someone.

1. **three.js transmission is a screen-space single-bounce approximation.** It
   refracts the already-rendered opaque backdrop through the front surface
   using the normal, the IOR and the thickness. It is not a path tracer: there
   are **no caustics, no correct second-surface refraction, and a transmissive
   body does not refract another transmissive body.** For a flat slab viewed
   from the front — which is every view this tool emits — those terms are
   small, and that is exactly why the views are parameterised the way they
   are. A shot raking across the slab's edge would be outside what this
   renderer can honestly claim.
2. **There is no harness.** §3 ABSENT. The channels are real exported
   geometry; the wires are not drawn.
3. **No knobs, no fader cap, no screws, no feet.** §3 ABSENT. Six empty Ø22
   bores are what the slab actually has.
4. **The display is off.** `glass_dark` is an unpowered panel. No UI is
   invented.
5. **Edge finish is uniform.** The real slab has as-cast faces and laser-cut
   edges; `fabrication.md` §1.3 cares a great deal about which way up it is
   cut, and §1.2 about the taper. One `roughness` for the whole body cannot
   express that, and **`slab.scad` deliberately does not model the hand
   chamfer** (it would break `check_cad.py`'s prismatic test), so it is not in
   the picture either.
6. **The reference and vendor bodies are not checked against anything.** There
   is no artefact to measure them against — they are models of parts nobody
   here fabricates. C1–C3 run on the three EXPORTED bodies only. The class of
   every body is in the sidecar, and the three classes are not
   interchangeable.
7. **`$fn = 96`** on the reference bodies only. It cannot reach the fabricated
   parts, which come from `export.py`.
8. **Kerf, taper, warp, flatness, the slab's as-bought thickness** (cast
   "half-inch" is in spec anywhere from 10.790 to 13.590) are not in any file,
   let alone in a picture. `v2/docs/fabrication.md` §4.6 is the list.
9. **Nothing here measures whether the render is any good to look at.** C5
   only establishes that it is not blank.

---

## 6b. NOT SOURCED

Numbers this tool needed and could not source. None of them is load-bearing on
a fabricated dimension — every one is a **threshold in a check** or a
**property of the toolchain** — but an unsourced number is a bug whether or
not it reaches a part, so they are listed with the procedure that closes each.

1. **`C4_MAX_SHIFT_MM = 3.000`** — the ceiling on refraction displacement.
   Tagged `[DESIGN]`, and it must not be read as physical. Snell through a
   parallel slab gives `t·sin i·(1 − cos i/√(n² − sin² i))`, which for
   `t = acrylic_t = 12.700` and `n = 1.491` `[RÖHM gsxt]` is 0.739 mm at
   *i* = 10° but **6.468 mm at 60°** — so the physical ceiling depends
   entirely on the incidence angles the camera produces, and C4's view is the
   *orthographic* plan, whose rays are parallel to −Z and whose flat faces
   therefore refract by ≈ 0. The 0.922 mm actually observed is a property of
   three.js's screen-space transmission approximation, not a Snell prediction.
   **Closes by:** deriving the bound from the ray-angle distribution this
   camera produces, or by dropping the term and reporting displacement as a
   result only. *(An earlier draft of this section asserted "2.74 mm at 60°";
   that was arithmetic nobody had run, and it is wrong by 2.4×. It is recorded
   here rather than deleted, because inventing a derivation to retire a
   NOT SOURCED entry is the exact failure this ledger exists to catch.)*

2. **The C4 and C5 thresholds** — `0.35`, `0.60`, `8`, `13.0`, `4.0`, `0.97`,
   `24`, `0.02`, `0.98`. All `[DESIGN]`: lines drawn once, from a single
   known-good render, not quantities measured off anything. The observed
   values and their floors are printed on every run, which is what shows the
   headroom, and §5's break table shows which term bites in which failure.
   **Closes by:** rendering a set of known-good and known-bad variants and
   setting each threshold from the separation between the two distributions,
   rather than from one sample.

3. **Why `export.py`'s carrier is not byte-reproducible.** The *symptom* is
   measured and reproduced (§5, three exports, three hashes, one geometry).
   The *cause* — presumably ordering inside OpenSCAD 2021.01's CGAL
   tessellation — is not established here, and this tool has no business
   establishing it. **Closes by:** an owner of `v2/tools/export.py` either
   making the export deterministic or recording that it cannot be. Until then
   C6 compares geometry, not bytes, and says so.

4. **`envMapIntensity = 0.45` on the acrylic.** Tagged `[ARTISTIC]` at its
   definition and called out in §4: it is the one place this render knowingly
   leaves physics, to keep the IBL from veiling the interior the brief exists
   to show. There is no measurement behind `0.45`. **Closes by:** nothing —
   it is a stated art-direction choice, listed here so it is never mistaken
   for a derived one.

---

## 7. Two toolchain facts this tool had to be written around

Both belong in `v2/docs/fabrication.md` §4.7; one of them contradicts it.

* **PIL is NOT usable on `/usr/bin/python3.11`.** §4.7 lists "PIL 10.2.0"
  among what that interpreter has. The package is installed, but its C
  extension is `_imaging.cpython-**312**-x86_64-linux-gnu.so` — built for
  3.12 — so `from PIL import Image` raises `ImportError` on 3.11. Verified by
  running it. `check_render.py` therefore decodes PNGs by shelling out to
  **ImageMagick 6.9.12-98** (`convert … ppm:-`) and measuring with numpy,
  which keeps the thing being measured the PNG a person actually looks at
  rather than a side-channel dump of the frame buffer.
* **The root `.gitignore` silently excludes this tool's own page.** It carries
  a blanket `*.html` / `!docs/**/*.html`, added to stop scraped research pages
  being swept into a commit — and it also excludes
  `v2/tools/render/scene/index.html`, which is the file Playwright loads and
  without which nothing renders. This is not theoretical: commit **b9608f2**
  committed `v2/tools/render/` without it. `scene/.gitignore` re-includes it
  (a deeper rule wins, and `*.html` excludes files rather than a directory, so
  the re-include is allowed). The root rule would be better written as
  `/*.html` or scoped to the research directories, but that file is not this
  tool's to edit.
* **No scipy, so no connected-component labelling.** Already in §4.7 for
  meshes; it bites images too. C2 labels the mask's dark regions with
  **union-find over horizontal runs**, not pixels: a 1600 × 1000 mask of this
  part has a few thousand runs, which is milliseconds and exactly equivalent
  to a per-pixel 4-connected labelling.

---

## 8. Files

```
v2/tools/render/
  render.py          the pipeline. Parameters -> meshes -> manifest -> shoot.
                     Also the materials table and the --break switches.
  check_render.py    the checks. Reads PNGs and STLs; never the CAD source.
  deck_params.scad   echo-only. Dumps parameters.scad. No dimension written.
  carrier_params.scad  echo-only. The one placement that lives in carrier.scad.
  bodies.scad        `use`s v2/cad/parts/*.scad; one export per material group.
  shoot.js           Playwright. Serves two roots on loopback, screenshots.
  scene/index.html   the page. Import map + the annotation layer (DOM, so it
                     can never be mistaken for part of the model).
  scene/scene.js     the renderer. Materials, lights, orbit. No dimensions.
  scene/.gitignore   re-includes index.html against the root *.html rule. §7.
  scene/vendor/      three.js r0.160.1, fetched on first run. Gitignored.

v2/export/render/            (gitignored, like all of v2/export/)
  .bodies/           the reference-body STL cache, keyed on source mtime
  <view>.png         the render
  <view>.json        its sidecar: camera, every body's class + source + sha256,
                     the ABSENT list, the material table, the parameter dump
  .work/             staged meshes and manifest.json
```

### Where a number can enter the scene, and nowhere else

```
v2/cad/parameters.scad          the single source of truth
        |  openscad --export-format=echo
        v
deck_params.scad / carrier_params.scad     names and arithmetic only
        |
        v
render.py  ---->  manifest.json  ---->  scene.js
                        ^
v2/export/stl/*.stl ----+   measured with trimesh (z anchoring, scene bounds)
```

`scene.js` has no other input. If you find yourself typing a millimetre into
it, the number belongs in `parameters.scad` and the plumbing belongs in
`render.py`.
