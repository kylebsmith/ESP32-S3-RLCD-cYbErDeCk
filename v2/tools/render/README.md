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
body in the scene is the exported STL from `v2/export/stl/` — all **fifteen**
of them, which is eight parts and their placements: the three plates, six
knobs, a fader cap, a cowl and four feet. Nothing is re-modelled in JavaScript
— `scene/scene.js` contains no dimension at all and is a material, light and
camera script. Every number the scene uses arrives in `manifest.json`, which
`render.py` obtains by asking OpenSCAD to echo the values in
`v2/cad/parameters.scad`. Anything the design calls for and no file models is
**not drawn**; it is listed as ABSENT, with its size, in every view's sidecar
JSON **and on the annotated views** — the images print the size too, which
they did not when that sentence was first written.

---

## 1. Run it

```sh
# the three pictures, into v2/export/render/
/usr/bin/python3.11 v2/tools/render/render.py

# measure what came out
/usr/bin/python3.11 v2/tools/render/check_render.py

# watch every check fail on a deliberately broken render, then pass again
/usr/bin/python3.11 v2/tools/render/check_render.py --prove

# re-derive the C4/C5/C7 thresholds from good and bad renders. ~4 minutes.
/usr/bin/python3.11 v2/tools/render/calibrate.py
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
| network, once | `render.py` fetches **two** three.js r0.160.1 files into `scene/vendor/` on first run and never again. That directory is gitignored: vendor JavaScript is a build input, not source. Delete it and it refetches. **The environment map is not among them** — see §4 — it is generated in `scene.js`, so there is no HDR to fetch, no vendor image and no licence to carry. |

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
| `orbit` | az −62°, el 14°, perspective 30° — or yours | The free camera. `--azimuth/--elevation/--explode/--annotate` steer this view and no other, so a flag on the command line can never move a measurement view's camera out from under the checker. **Its default used to be `hero`'s, exactly**, which made `orbit.png` a byte-for-byte copy of `hero.png` and "11 views" a claim of ten pictures and one duplicate. It is now a camera no other view has: low and from the player's right, raking the slab's edge. **11 views, 11 distinct images.** |

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

| body | material | ×  | from |
|---|---|---|---|
| `slab` | `acrylic_clear`, 12.700 mm | 1 | `v2/cad/slab.scad` |
| `carrier` | `petg_carrier`, 15.875 mm | 1 | `v2/cad/carrier.scad` |
| `backpanel` | `acrylic_black`, 3.175 mm | 1 | `v2/cad/backpanel.scad` |
| `knob_encoder` | `petg_knob`, 19.050 tall | 3 | `v2/cad/knob.scad` |
| `knob_pot` | `petg_knob`, 19.050 tall | 3 | `v2/cad/knob.scad` |
| `fader_cap` | `petg_knob`, 15.875 tall | 1 | `v2/cad/fader_cap.scad` |
| `cowl` | `petg_carrier`, 3.820 tall | 1 | `v2/cad/cowl.scad` |
| `foot` | `tpu_foot`, 5.715 tall | 4 | `v2/cad/foot.scad` |

`render.py` will run `v2/tools/export.py` itself if an STL is older than its
source, and refuses outright under `--no-export`. For the three plates the Z
placement is **solved, not assumed**: `tz = declared_z0 − measured_stl_zmin`,
where the declared z0 comes from `parameters.scad` and the zmin is measured
off the mesh. The carrier's solved offset comes out as 0 because
`carrier.scad` already translates itself into the deck frame — that is a
result, not an assumption.

### The five parts fitted last, and the three ways a placement can be wrong

The knobs, the cap, the cowl and the feet are each drawn at their own origin
and placed by a **pure translation** — no rotation, no flip. That is the whole
danger: a part put in the wrong way renders perfectly, looks fine and is
wrong, and nothing in the picture says so. So `render.py`'s `FITTED_PARTS`
declares, and checks, three things per part before anything is drawn.

| | what it catches | how |
|---|---|---|
| **the Z span** | the part hung off the wrong datum | the placed mesh's own z-range must equal the one the echoed datums declare, to 0.001 (40× the float32 STL round-trip slop). Proved: hanging the cowl off `deck_bot_z` instead of `deck_bot_z − cowl_h()` gives *"the placed mesh starts at z = −9.525000 and the datums declare −13.345000 (d 3.820000 > 0.001)"* |
| **which way up** | a mesh exported or edited upside down, which no Z range can see | both ends of every one of these parts are different features, so the **larger planar end face** says which end is which `[MEASURED]`: knob top 464.4993 / sole 141.2707; fader cap 370.9572 / 87.7173; cowl 2360.9022 / 4443.9657 — the one that goes the other way, because its cavity opens *upward* to swallow the 18650 holder; foot 416.6374 / 384.6606, the narrowest margin at 1.083×. Proved: mirroring `foot.stl` in z gives *"its face at z = −0.0000 is 384.6606 mm² and its face at z = −5.7150 is 416.6374, and foot.scad declares the larger of the two to be the top"* |
| **which way round** | a part turned 180° in plan, where it is not symmetric about its own origin | only the foot needs one — its screw axis is on the origin and its toe runs to −Y, so the plan centroid's sign is the test. Proved: rotating `foot.stl` 180° about Z gives *"its centroid is at (−0.0000, +9.8628) … requires axis 1 to be negative"* |

The fader cap is the one part whose **top** is not declared, and that is
deliberate: its proud height is `fc_proud`, a file-local in `fader_cap.scad`,
and `parameters.scad` does not carry it. Writing a number here that no file in
`v2/cad` holds would be the second copy of the source of truth this whole tool
exists to avoid, so only its base is checked and this paragraph is the reason.

`export.py` also cuts `knob_mould_brim` and `knob_mould_plug`. They are
**tooling** — the two halves of the casting mould, exported in their assembled
positions so the pair can be seen to close — and no table in `render.py` names
them, so they cannot reach a deck view by accident.

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

**This list has just got shorter, and that is the point of it.** The knob, the
fader cap and the foot were on it; `export.py` now cuts all three, so they are
EXPORTED bodies and they are in the picture. The discipline was never that the
list stays long — it is that nothing leaves it by being drawn as a stand-in.

| | |
|---|---|
| **1× wiring harness** | `carrier.scad` models the **channels** the wires lie in, and those *are* in these renders as recesses in the carrier's top face. Nothing models the wires. `v2/parts/wiring.md` is a route, not a solid. |
| **16× fastener** | 6× M3 through the slab and 10× M2.5 through the back panel — counted as `len()` of `parameters.scad`'s own two point lists, not asserted, because a retyped count goes stale and this one did: the back panel's ten were still described as M3 after commit `ae4cc9f` drilled them `screw_m25_clear_d`. Plus the brass heat inserts they thread into and the DIN 125 A2.5 washers at **six** of those ten — §6b of `parameters.scad` is why the four that are also `foot_pts()` must not have one. `08_cutouts/` has the clearance and insert *cutouts*, not the screws. |
| **61× keycap legend** | and the sculpted row profile. `k9pro_internals.scad` says what it models in its own words — *"the keycaps as a correct envelope: each cap's own plan size from its width in key units … one flat top plane"* — so what is in the picture is a prism, 18.731 square at 1u `[MEASURED off ref_kbd_keycaps.stl]`, and it is drawn as what it is. Nothing in this repository records what is printed on the tops or what shape they are, and both are properties of the bought part. **This is the honest half of "the keycaps are untextured grey rectangles":** the material got better, the geometry did not change, and the gap is listed rather than painted over. |

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
| `envMapIntensity` | **0.22** | `[ARTISTIC]` | **The one place this render knowingly leaves physics.** At 1.0 the studio IBL's broad soft source reflects off the whole top face and veils the interior — which is exactly what a real softbox over a real polished sheet does, and exactly what the brief most wants not to happen. This keeps the Fresnel edge reflection, where the 12.700 mm actually reads, and takes the sheen off the middle. A photographer solves it with a polarising filter; this is the render's version of one. **It was 0.45, and 0.45 was calibrated against a different room:** the IBL was three.js's `RoomEnvironment`, whose brightest source is about 1.0, and it is now a generated studio whose key reaches 9.0 — so the same 0.45 veiled the slab exactly as 1.0 veiled it before, visibly, on the exploded view. A denser light wants a denser filter. Still `[ARTISTIC]`, still NOT SOURCED, and now at least re-derived against the thing it filters. |

### Everything else

| material | class | note |
|---|---|---|
| `petg_carrier` `#23262a`, roughness 0.78 | colour `[REPO]`, finish `[ARTISTIC]` | `v2/parts/stack.md` §5.6 specifies PETG, *"matte black or very dark grey"*, because *"through clear acrylic a dark matte plane reads as a void, and the coloured wires and nickel nuts become the only things with brightness"*. |
| `acrylic_black` | `[PHYSICAL]` ior, `[ARTISTIC]` finish | Same substrate, same 1.491, transmission 0. |
| `pcb_green` `#1b3a2a`, `pcb_navy` `#0d2d44` | `[REPO]` | The colours `fw_board()` and `ws_board()` already chose, carried over so the render and the OpenSCAD preview agree. Likewise `metal_bright` `#c8c8cc`, `black_plastic` `#101010`, `button_light` `#d8d8d8`. |
| `lcd_reflective` | `[PHYSICAL]` no emissive, `[ARTISTIC]` finish | **The display, and the old row called it the wrong part.** It said "an unpowered 4.2 in IPS panel is a dark glass rectangle … nothing in this repository holds a frame of its output". The first half is wrong about the part and the second about the repository — see §4b. |
| `glass_dark` | `[ARTISTIC]` | What is left of that mesh: the spacer, the TFT border and the FPC, which really are dark glass. |
| `petg_knob` `#1a1c20` | colour `[REPO knobs.md]`, finish `[ARTISTIC]` | The six knobs and the fader cap. `v2/parts/knobs.md` §717 specifies PETG and offers two colours — *"natural / clear PETG if you want §2.4's light; matte black otherwise — the NeoPixel argument"*. §2.4's light is the 5880's own NeoPixel shining up through a translucent knob, and **nothing in this render models that light**, so the clear branch would be a knob lit by an emitter that is not in the scene. Smoother than the carrier because both `knob.scad` and `fader_cap.scad` say to print it cap down — *"the top face is the only one anyone looks at"* — and a face printed against the plate comes off with the plate's finish. |
| `tpu_foot` `#121314` | material `[REPO foot.scad]`, finish `[ARTISTIC]` | The only part in the build that is neither PETG nor PMMA. Soft, waxy, wide specular lobe. |
| `keycap` `#2a2d33` | `[ARTISTIC]` | PBT double-shot, nearly all roughness: the sheen on a keycap in a photograph is the room, not the plastic, which is why this material needed a room worth reflecting before it read as anything but grey. The **geometry** is unchanged and is an envelope — see ABSENT. |
| `aluminium`, `keycap`, `brass`, `switch_body`, `pouch_cell`, `silk_white`, `component_satin` | `[ARTISTIC]` | Plausible finishes. `component_satin` is explicitly the "one material for a whole vendor mesh" case and is named so it is obvious in the manifest. |

## 4b. The display, which is the other thing a photograph of this would show

`glass_dark` used to carry the whole display and the material table said, in
these words: *"an **unpowered** 4.2 in IPS panel is a dark glass rectangle,
and it is rendered as one. Nothing in this repository holds a frame of its
output, so none is invented."* **The first half is wrong about the part and
the second is wrong about the repository,** and the result was a render of a
live-coding instrument whose screen is off, which is a render of a dead
instrument.

**What the part is.** `docs/HARDWARE.md` has a table of it: a 4.2 in
**reflective** LCD (RLCD) on a Sitronix ST7305, **400 × 300 landscape**,
**1 bit per pixel, bi-level**, and **no backlight** `[VENDOR]`. A reflective
LCD is a mirror with a switchable mask in front of it. Every photon it sends
you came out of the room, so `lcd_reflective` is an ordinary dielectric with a
`map` and **no emissive term at all**. A screen that glows here would be a
picture of a part that does not exist.

**Where the glyphs come from, and this is the bit that had to be sourced.**
The image is not composed of plausible-looking text. `render.py`'s
`screen_content()` parses two tables that `v2/parts/wiring.md` already
carries — §2.3's I²C addresses (`0x30` NeoSlider, `0x36`/`0x37`/`0x38`
encoders) and §3.1's three free ADC pins (GPIO 1/2/3 on ADC1\_CH0/1/2) — and
lays them out on the panel's own character grid. **A parse that finds nothing
is fatal**, for the same reason an `undef` in the parameter dump is: a
silently empty screen dressed up as a live one is the failure this whole tool
is about. The ENC row sits above the POT row because `enc_y` is rearward of
`pot_y`, so the screen's two rows are in the order the deck's two rows are.

**The grid is the firmware's.** `docs/HARDWARE.md` works out that the
controller's own addressing quantises the landscape height to 12 px, so a cell
must be a multiple of 12 tall and even wide; it recommends 6 × 12 giving
66 × 25, and then records that the recommendation **failed on the real panel**
— *"at 0.212 mm pixel pitch and with no backlight, a 5 × 7 glyph body is too
small and too thin to read comfortably"*, and the owner's verdict, *"the text
is too small … with such a naturally low contrast screen we gotta have sexy
chunky letters"*. The firmware therefore defaults to **12 × 24 giving
33 × 12**, and that is the grid this texture is drawn on: 396 × 288 inside a
400 × 300 frame, centred, which is where the 2 px and 6 px origin comes from.

**It is drawn at one bit, because the panel is one bit.** Canvas text is
anti-aliased; the glyphs are quantised to the two tones before upload, and the
texture is magnified `NearestFilter`, so what you are looking at in the plan
view is 400 × 300 actual pixels. The projection is a planar UV from
`ws_active_area()` — `[2.248, 2.750, 87.050, 66.353]` in the display body's
own frame, which `waveshare_rlcd42.scad` measured off the STEP and which is
**not** centred on the PCB. `render.py` checks the pitch that falls out of it:
84.802/400 = 0.212005 and 63.603/300 = 0.212010 mm, square to 5 × 10⁻⁶, which
is the property `docs/DATUMS.md` D-07 settles the panel's orientation on. A
render that silently stretched the panel would make that argument a lie.

The texture carries four panel pixels of **bezel** all round and the UV box is
grown to match, because everything outside the active area — the spacer, the
TFT border, the FPC — falls outside `[0,1]` and takes the clamped edge texel.
With a bare 400 × 300 image the module's dark border would take the colour of
the panel's own edge row and the display would read as a white slab the size
of the PCB.

**What is NOT claimed:** this is not a frame captured from the firmware.
`firmware/main/main.c` draws the real UI and nothing in this tool runs it.
Every sidecar says so in those words, and the annotated views print it next to
the picture.

The two tones (`#8d9188` paper, `#16181a` ink) are `[ARTISTIC]` and **NOT
SOURCED**: no reflectance for this panel is recorded anywhere in this
repository. What *is* sourced is that there are exactly two of them.

### Lighting — all `[ARTISTIC]`, and the environment is generated

A studio, not a CAD viewport, and not a grey room. The dominant term is an
IBL and **it is not decoration** — it is the only thing a `transmission`
material has to refract at the edges of the frame, and point lights alone
would give clear acrylic nothing to show.

**It used to be three.js's `RoomEnvironment`, and that is why every render of
this deck came out grey.** `RoomEnvironment` is a neutral grey room whose
brightest source is about 1.0. A dielectric at roughness 0.02 reflects what is
in front of it; if the brightest thing in the world is 1.0, polished acrylic
reflects 1.0 and reads as paint. So `scene.js` now **generates** an HDR
equirectangular environment, texel by texel, and prefilters it through
`PMREMGenerator` exactly as a loaded `.hdr` would be: a floor-to-sky gradient
with a horizon band, and four rectangular softboxes with smoothstep penumbrae
— a warm key at **L 9.0**, a cool fill, a long narrow **rim** whose only job
is the slab's 12.700 mm edge, and a weak kicker under the horizon so the deck
does not float on a black nothing. It is a build input in the same sense
three.js is, **except that it is generated and not fetched**: no download, no
vendor image, no licence, six numbers and a loop.

**The one convention that matters.** three.js's equirect mapping uses +Y as
its polar axis and this deck is +Z up, so the image is *not* authored in image
space. Each texel's direction is reconstructed first — `equirectUv()`
inverted — and the lighting function is evaluated on that vector in the
**deck's** frame. Authoring it the other way round puts the key light out of
the back of the keyboard.

Three conventional lights sit on top, for the hard highlight a prefiltered
environment cannot give: PMREM blurs a source into its roughness lobe and a
0.02-roughness face wants a point of light, not a lobe. The key's intensity
was set on the **plan** view and not on the hero, because the plan looks
straight down and every surface it shows is lit by the key's own +Z component
and by nothing else that matters — at 2.30 the hero was still fine (mean luma
43.2 against the old 43.9) while the plan's median went from 64.9 to 91.2 with
its 95th percentile clipping at 226.7 `[MEASURED, two renders]`. It is 1.55,
and the keycap field's median luma then lands at 66.94 against the old
render's 64.86 on the same crop.

ACES filmic tone mapping at exposure **0.78**, which now lives in the manifest
rather than as a literal in `scene.js` — partly so it is in every sidecar next
to the picture it made, and partly because `calibrate.py` has to build a set
of legitimately different-looking good renders and a differently exposed one
is the obvious example. A studio render with specular highlights on aluminium
will clip in sRGB, and a clipped highlight is what makes a render look like a
screenshot. The backdrop is a vertical gradient on a canvas — a `Texture` on
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

The only thing it takes from the renderer's own side is the **camera
calibration** — `mm_per_px` in each view's sidecar, and, since C4's ceiling
had to be derived, the camera's **position and target** as `scene.js` actually
computed them. Without a stated scale a PNG has no dimensions at all, and
without a camera distance the incidence angles the transmission shader sees
cannot be known. Both are the renderer's claim about its own camera, and every
check below is a test of that claim against geometry the renderer did not
produce.

| | check | measured | against | observed |
|---|---|---|---|---|
| **C1** | silhouette scale and position | the part's own flat-white mask: width, height and bounding-box centre, in mm | that part's exported STL bounding box | slab PNG 304.837 × 190.575 vs STL 304.600 × 190.375 — **max Δ 0.238 mm**, tolerance 0.463 |
| **C2** | through-hole count | enclosed dark regions in the same mask | the STL's **genus**, free from its Euler characteristic, which for a prismatic plate *is* its through-hole count | slab **14 = 14**; back panel **17 = 17** |
| **C3** | stack height | the elevation mask of the three fabricated layers | the **sum of those three STLs' own thicknesses** | PNG 31.969 vs STL 31.750 mm, Δ 0.219, tolerance 0.669 |
| **C4** | transmission | high-passed detail inside the slab's footprint, `plan` vs `plan_noacrylic`, tile-matched with a search | each other, and the displacement against a **derived** ceiling | detail survives at **0.805** of the un-occluded frame (floor 0.5058); over **52** tiles (floor 24) the median match is **r = 0.831** (floor 0.5619) at a median displacement of **0.922 mm** against a ceiling of **1.391 + 0.4125** |
| **C5** | not degenerate | every PNG | itself | pictures on level count and modal share; masks on **lit fraction**, because a mask is *meant* to be binary |
| **C6** | the fabricator's **part** was used | the SHA-256 and the geometry hash the sidecar records for every EXPORTED body, one row per **part**, plus the agreement of all 11 sidecars | `v2/export/stl/<name>.stl` **now** | identical, 8 parts over 15 placements |
| **C7** | the panel is lit | the active-area rectangle the sidecar names, in `plan.png` | itself | **19.04 %** ink (want 6…58), **17.96** dark/light transitions per row (floor 2.23), **126** levels (floor 57) |

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
report the median correlation **and the median displacement**.

**The displacement is read off the tiles that matched, and only those.** A tile
whose best correlation is 0.2 has not found the same piece of board in the
other frame; the offset it reports is wherever the noise happened to peak,
which is uniformly distributed over the search window and therefore big.
Including them pulls the median up in exactly the renders where it means
least: measured, blanking the *display's own content* — not a transmission
failure at all — put the median at **1.50×** its ceiling and tripped C4 for a
reason that had nothing to do with the acrylic. Filtered to matched tiles the
same render reads **1.06×** `[MEASURED]`.

### And the ceiling on that displacement is derived, not chosen

It used to be `C4_MAX_SHIFT_MM = 3.000`, a round number listed under NOT
SOURCED. The entry's own reasoning was that C4's camera is **orthographic**,
that its rays are parallel to −Z, that flat faces at normal incidence refract
by ≈ 0, and that the 0.922 mm observed could therefore not be physical. Every
premise is true and the conclusion is wrong, because it asked what the
**camera** does and the displacement is set by what the **shader** does.

three.js r160's `getIBLVolumeRefraction()` builds the refracted ray from
`cameraPosition − vWorldPosition` — the camera as a **point** — whatever the
projection is. It then walks `thickness` along that direction and projects the
**exit point** to screen space to sample the backdrop. So under an orthographic
camera the screen displacement is the lateral part of a single walk:

```
i  = atan(rho / D)          rho : the point's radius from the camera axis
sin r = sin i / n           D   : the camera's distance to its target
displacement = t * sin r  =  t * sin(i) / n
```

and every term on the right is something `check_render.py` can measure or
already accepts: **t** is the z-extent of the *slab's own exported STL* — not
the material's declared `thickness`, because that is the number a break moves
and a ceiling computed from it would rise to meet any lie told about it;
**rho** is half the diagonal of the slab's silhouette *in this frame*, off
`mask_slab` at its own mm/px; **D** comes from the camera the sidecar records;
**n** is the renderer's claimed IOR, the one term taken on trust, and the
ceiling moves about ±10 % over n = 1.4 … 1.6.

For the shipped plan view: t 12.700, rho 179.75, D 1085.6, so i<sub>max</sub> =
**9.402°** and the ceiling is **1.391 mm**. One search step (0.4125 mm) is
added, and that is not a fudge: the per-tile offset is an integer search on a
4× downsampled image, so the reported magnitude can overstate the true one by
up to 0.707 of a step. Observed 0.922 against a limit of 1.804.

**Note which formula is not used.** The parallel-slab *offset*,
`t·sin i·(1 − cos i/√(n² − sin² i))`, is what a real sheet of PMMA does to a
ray that goes in one face and out the other; at 9.402° it is **0.6939 mm**,
which is *less* than the 0.922 this render measures. That disagreement is how
the right model was found — three.js does not refract out of the second
surface at all, so the lateral term is the whole of it, and an honest ceiling
has to be a ceiling on what the renderer does.

**Why C7 is there at all, and what it does not claim.** A reflective LCD with
nothing on it is a grey rectangle, and a grey rectangle is what this render
drew for a long time while every other material was correct — which is the
kind of failure that survives review, because nothing about it looks broken.
C7 goes to the rectangle the sidecar says the active area occupies, at the
stated mm/px, and asks the PNG whether there is an image in it: ink, dark/light
transitions per row (which is what distinguishes *text* from a flat tone or a
smooth gradient) and level count. What it does **not** do is check that the
panel is the right size or in the right place. It cannot: the display is a
REFERENCE body, there is no exported artefact of it to measure against, and
this file will not test a CAD number against itself. The size is printed so a
reader can see it.

**Why C2 is there at all.** C1 cannot see the renderer loading the wrong mesh:
the slab and the back panel have identical 304.600 × 190.375 outlines. Their
hole counts are 14 and 17. Swapping them is invisible to a bounding box and
loud in a hole count.

**C6 is not theoretical, and its first version cried wolf.** On the first full
run it fired because the carrier had been re-exported three minutes after the
render staged its mesh. The reading at the time — "the picture was of a
carrier that no longer existed" — was **wrong**, and so was comparing file
bytes at all.

**This section used to say `export.py`'s carrier was not byte-reproducible,
and quoted three hashes of one unchanged `carrier.scad` to prove it. It is
reproducible now, and the prose was left behind.** `export.py` runs
`canonicalise_stl()` over every mesh it writes — a cyclic rotation of each
facet's corners followed by a sort of the facet records, which is provably a
reordering of the facet block and nothing else. The cause of the old
nondeterminism was established before it was fixed: facet **emission order**
only, proved by comparing four runs' 48-byte facet records as multisets.
Verified from this directory rather than taken on report, twice and two ways
`[MEASURED 2026-11-07]`: two re-exports of `knob_encoder` give
`f97402e779134600…` both times; and a **full** `export.py` run, made after the
renders were already on disk, left **every one of the eight exported parts**
matching the `sha256` its sidecar had recorded before the run — which is C6
taking the fast path on all eight, and is a stronger statement than two runs
of one part.

**So why keep the fallback?** Because what a byte difference *means* has
changed, not whether it can happen. It now means **the picture predates a
re-export**: `v2/export/stl/` is a live directory that another author's
`export.py` rewrites, and it did so twice while this tool was being written.
If that re-export did not change the part, the picture is still a picture of
the part, and failing it would be crying wolf — which is how a check gets
switched off. It is also the only thing that would survive `export.py` losing
its canonical pass.

C6 keeps the byte hash as its **fast path** — equal bytes still prove equal
geometry — and when the bytes differ it compares a canonical hash of the
**triangle set** (`render.geom_sha()`: coordinates rounded to 6 dp, each
triangle rotated to its lexicographically smallest corner so winding survives,
then the triangles sorted). Measured sensitivity: a vertex moved **1e-3 mm**
fails; **1e-7 mm**, which is inside the float32 STL round-trip slop of
2.44e-5, does not.

**One row per part, not per body.** Six knobs are two parts and eight
placements; four feet are one part and four, and every instance is staged from
the same file, so fifteen rows would be eight answers and seven echoes. The
instance count is printed instead — `knob_encoder … (3 placements)` — because
that is the thing a reader can check against the drawing. If two placements of
one part ever named different meshes, that is its own failure and it is
reported as one.

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
| `stl` | **C6** | a hand-edited copy of the exported mesh in the scratch tree — **one vertex moved 1.000 mm**; `v2/export/stl/` is never touched. | `slab` rendered geometry vs on-disk geometry differ; the other seven parts still match, so the failure is localised to the mesh that moved |
| `screen` | **C7** | the panel's framebuffer blanked to one tone. Every material still correct, the display still in the picture, and the instrument dead. | **2.38 %** ink against a floor of 6.06, **1.00** transition per row against 2.23, **27** levels against 57 |
| `thick` | **C4** | the acrylic's refraction thickness tripled. It still transmits, the picture still looks plausible, and everything under it has moved further than a 12.700 mm slab can move it. | median displacement **1.95×** its derived ceiling, where six good renders sit at 0.67…0.95× `[MEASURED]`. This is the break the old `C4_MAX_SHIFT_MM = 3.000` could not catch: 3.000 was a round number and the quantity it was bounding is 1.391. |

This break used to rewrite 8 bytes of the STL's 80-byte **header**, which
changed the file's `sha256` while leaving every triangle alone. That proved
only that C6 could notice a byte — the one thing it must *not* fail on, now
that `export.py` is known not to be byte-reproducible. A part the fabricator
never receives is a part whose **geometry** differs, so the break moves a
vertex.

Restored, with no break: **23 passed, 0 failed** over the six
`--views check` frames. The full set is 28.

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

## 5a. Where the thresholds come from

`calibrate.py` renders the study and prints the table; this is the result.
Six **known-good** variants — the shipped settings, 1000 × 700 at dpr 1,
1280 × 800, exposure 0.62, exposure 0.95, and `$fn` 48 — against seven
**known-bad** ones: `--break transmission`, `blank`, `screen`, `thick`, plus
`plan.png` blurred 0×3 and 0×6 and the pictures flattened to one colour, and
one extra made by inverting the blanked panel.

| threshold | good | bad | set to | rule |
|---|---|---|---|---|
| `C4_MIN_DETAIL_RATIO` | 0.7935…0.8767 | 0.1411…0.3224 *(frosted)* | **0.5058** | geometric mean |
| `C4_MIN_CORRELATION` | 0.6585…0.8743 | 0.3173…0.4795 *(opaque, 0×6)* | **0.5619** | geometric mean |
| `C4_MIN_TILES` | 52…59 | 12 *(opaque)* | **24** | geometric mean |
| `C5_MAX_MODAL_FRACTION` | 0.3377…0.3836 | 1.0000 *(flat)* | **0.6918** | midpoint |
| `C5_MIN_LEVELS` | 73…178 | 1 *(flat)* | **8** | geometric mean |
| `C5_MASK_LIT_MIN` | 0.1259…0.1411 | 0.0000 *(blank)* | **0.0315** | ¼ of the worst good |
| `C5_MASK_LIT_MAX` | 0.7629…0.8518 | 1.0000 *(fully lit)* | **0.9259** | midpoint |
| `C7_INK_MIN` | 0.1545…0.1890 | 0.0238 *(screen off)* | **0.0606** | geometric mean |
| `C7_INK_MAX` | 0.1545…0.1890 | 0.9762 *(screen inverted)* | **0.5826** | midpoint |
| `C7_RUNS_MIN` | 4.995…17.275 | 1.000 *(screen off)* | **2.2349** | geometric mean |
| `C7_LEVELS_MIN` | 123…127 | 27 *(screen off)* | **57** | geometric mean |

**Each term divides the failures it is the right term for**, and that is not a
convenience: C4 is a conjunction, so a term only has to catch its own
failures, and what guarantees nothing slips through is `--prove`. Asking one
term to divide a failure another catches by a factor of four makes it tighter
than it has any reason to be — and twice here it would have made it **wrong**,
which a single sample could never have shown:

* a 0×3 blur correlates at **0.7145**, *inside* the good set's 0.6585…0.8743.
  The worst good there is the 1000 × 700 render, whose coarser search quantum
  costs it match quality. No correlation floor separates a light blur; the
  detail ratio does, 0.3224 against 0.7935.
* an **opaque** slab keeps **0.7163** of its high-frequency energy, because a
  mirror-finish opaque plate still has bright edges round its own holes. No
  detail-ratio floor separates it; the tile count does, 12 against 52.

`calibrate.py` prints every bad variant against every threshold, marking the
ones in its dividing set, so an exclusion is on the page rather than quiet.

Two of these are worth reading as results rather than as settings.
`C5_MASK_LIT_MIN`'s bad side is **exactly zero**, so the distributions are
infinitely separated and no value in between discriminates better than any
other; the number is a quarter of the worst good, chosen as headroom for a
mask view nobody has rendered yet, and the tool says so in those words rather
than dressing it up. And `C5_MIN_LEVELS` cannot catch `--break blank`, which
still leaves **26** levels because the backdrop and the annotation layer are
drawn either way — C5's *mask* branch is what catches that, at 0.00 % lit, and
it always was.

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
3. **No screws.** §3 ABSENT. The knobs, the fader cap, the cowl and the feet
   have come off that list — they are exported parts now and they are in the
   picture, placed and checked three ways (§3).
4. **The screen is not the firmware's output.** §4b. It is the repository's
   own device tables laid out on the panel's real character grid. Nothing here
   runs `firmware/`, and the sidecar says so rather than leaving a reader to
   assume either way.
5. **The keycaps have no legends and no profile.** §3 ABSENT. The material got
   better; the geometry is still `k9pro_internals.scad`'s stated envelope, one
   flat top plane per cap.
6. **Edge finish is uniform.** The real slab has as-cast faces and laser-cut
   edges; `fabrication.md` §1.3 cares a great deal about which way up it is
   cut, and §1.2 about the taper. One `roughness` for the whole body cannot
   express that, and **`slab.scad` deliberately does not model the hand
   chamfer** (it would break `check_cad.py`'s prismatic test), so it is not in
   the picture either.
7. **The reference and vendor bodies are not checked against anything.** There
   is no artefact to measure them against — they are models of parts nobody
   here fabricates. C1–C3 run on the three EXPORTED bodies only. The class of
   every body is in the sidecar, and the three classes are not
   interchangeable.
8. **`$fn = 96`** on the reference bodies only. It cannot reach the fabricated
   parts, which come from `export.py`.
9. **Kerf, taper, warp, flatness, the slab's as-bought thickness** (cast
   "half-inch" is in spec anywhere from 10.790 to 13.590) are not in any file,
   let alone in a picture. `v2/docs/fabrication.md` §4.6 is the list.
10. **Nothing here measures whether the render is any good to look at.** C5
    establishes that it is not blank and C7 that the panel is not dark.
    Neither is a judgement.

---

## 6b. NOT SOURCED

Numbers this tool needed and could not source. None of them is load-bearing on
a fabricated dimension — every one is a **threshold in a check** or a
**property of the toolchain** — but an unsourced number is a bug whether or
not it reaches a part, so they are listed with the procedure that closes each.

**Three of the four entries that were here are now closed, and they closed by
being carried out rather than by being argued with.** They are kept, struck
through, with what closed them: a NOT SOURCED ledger that only ever grows
teaches a reader nothing about which entries were real.

1. ~~**`C4_MAX_SHIFT_MM = 3.000`**~~ — **CLOSED, by deriving it.** The old
   entry reasoned that an *orthographic* camera's rays are parallel to −Z,
   that flat faces at normal incidence refract by ≈ 0, and that the 0.922 mm
   observed therefore could not be a Snell prediction. Both premises are right
   and the conclusion drawn from them was wrong, because it asked what the
   **camera** does and the displacement is set by what the **shader** does.
   three.js builds the refracted ray from `cameraPosition − worldPos` — the
   camera as a *point* — whatever the projection is, so the incidence angle is
   `atan(ρ/D)` and is not zero. The ceiling is now computed per render from
   the slab STL's own thickness, the slab mask's own radius, the sidecar's
   camera distance and the stated IOR; §5 has the derivation and the numbers.
   *(An earlier draft asserted "2.74 mm at 60°"; that was arithmetic nobody
   had run and it was wrong by 2.4×. It is recorded here rather than deleted,
   because inventing a derivation to retire a NOT SOURCED entry is the exact
   failure this ledger exists to catch — and the correct 6.468 that replaced
   it turned out to be the right arithmetic on the wrong model.)*

2. ~~**The C4 and C5 thresholds**~~ — **CLOSED, by `calibrate.py`.** They were
   nine lines drawn once from a single render. They are now measured, from six
   known-good variants (frame size, device pixel ratio, exposure, faceting)
   against seven known-bad ones (four `--break` modes and three PNG
   mutations), each placed in the gap by a stated rule. The study is a file in
   this directory, so the numbers can be re-derived rather than believed; §5a
   has the distributions. Two of them turned out to be wrong in a way one
   sample could never have shown: a 0×3 blur correlates *better* than a
   legitimate 1000 × 700 render, and an opaque slab keeps most of its
   high-frequency energy. Each is caught by a different term, which is what a
   conjunction is for.

3. ~~**Why `export.py`'s carrier is not byte-reproducible.**~~ — **CLOSED by
   somebody else, and this file was still asserting the opposite.** The export
   is canonical now: `export.py` sorts its facets, and two consecutive full
   exports give ten byte-identical STLs. C6's byte-then-geometry fallback is
   **kept** — it is what makes a mid-flight re-export survivable — but what a
   byte difference now means is that **the picture predates a re-export**, not
   that the exporter is nondeterministic.

4. **`envMapIntensity = 0.22` on the acrylic.** `[ARTISTIC]`, and §4 calls it
   the one place this render knowingly leaves physics. There is no measurement
   behind it. It was 0.45 against a dimmer environment and was re-set against
   the new one on the exploded view, which is the re-derivation of a choice
   and not the measurement of a quantity. **Closes by:** nothing — it is a
   stated art-direction choice, listed so it is never mistaken for a derived
   one.

5. **The reflective panel's two tones**, `#8d9188` paper and `#16181a` ink.
   `[ARTISTIC]`. Nothing in this repository records a reflectance, a contrast
   ratio or a white-state luminance for this panel; what *is* recorded is that
   it has exactly two states (1 bit, bi-level `[VENDOR]`). They were chosen so
   the panel is the brightest surface in the deck without clipping.
   **Closes by:** a measured white-state and black-state reflectance for the
   ST7305 panel, from the vendor or from a photometer.

6. **`PANEL_BEZEL_PX = 4`** — the module border carried in the screen texture
   so the clamped surround is not the panel's own edge row. One texel would do
   for the clamp; four is `[DESIGN]`, for the mipmap chain at grazing angles.
   It moves no dimension: the 400 × 300 still lands exactly on
   `ws_active_area()`.

7. **`BREAK_THICK_FACTOR = 3.0`** — how much `--break thick` multiplies the
   acrylic's refraction thickness by. `[DESIGN]`, bounded at both ends rather
   than chosen: large enough that the displacement clears the derived ceiling,
   small enough that the shifted tile is still inside C4's own search window,
   because a break that saturates the search would fail the *match* term and
   prove the wrong thing.

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
                     Also the materials table, the screen's content and the
                     --break switches.
  check_render.py    the checks. Reads PNGs and STLs; never the CAD source.
  calibrate.py       renders good and bad variants and sets check_render.py's
                     thresholds from the separation between them. 5a.
  deck_params.scad   echo-only. Dumps parameters.scad. No dimension written.
  carrier_params.scad  echo-only. The one placement that lives in carrier.scad.
  bodies.scad        `use`s v2/cad/parts/*.scad; one export per material group.
  shoot.js           Playwright. Serves two roots on loopback, screenshots.
  scene/index.html   the page. Import map + the annotation layer (DOM, so it
                     can never be mistaken for part of the model).
  scene/scene.js     the renderer. Materials, lights, orbit. No dimensions.
  scene/.gitignore   re-includes index.html against the root *.html rule. §7.
  scene/vendor/      three.js r0.160.1, two files, fetched on first run.
                     Gitignored. The environment map is NOT here: it is
                     generated in scene.js. 4.

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
