#!/usr/bin/env python3
"""Photoreal assembly renders of the v2 deck, from the exported geometry.

    /usr/bin/python3.11 v2/tools/render/render.py                 hero, plan, exploded
    /usr/bin/python3.11 v2/tools/render/render.py --views hero
    /usr/bin/python3.11 v2/tools/render/render.py --list-bodies
    /usr/bin/python3.11 v2/tools/render/render.py --views orbit \
        --azimuth 120 --elevation 20

OpenSCAD is the right tool for the geometry and the wrong tool for the picture:
it has no materials, no textures, no transmission and no tone mapping, so it
cannot show clear acrylic as clear, a PCB as green or a silkscreen legend as a
silkscreen legend. This tool renders the SAME geometry in a real physically
based renderer (three.js r160.1, MeshPhysicalMaterial) driven headlessly
through Playwright/Chromium.

THE ONE RULE THIS TOOL IS BUILT AROUND
--------------------------------------
A render is a claim about a part. The only renders worth having are of the part
the fabricator actually receives, so:

  * every fabricated body in the scene is LOADED FROM THE EXPORTED STL in
    v2/export/stl/. Nothing is re-modelled in JavaScript. scene.js contains no
    dimension at all -- it is a material and camera script, and every number it
    uses arrives in manifest.json;
  * every number in manifest.json comes out of v2/cad/parameters.scad (and
    carrier.scad) by `openscad --export-format=echo`, so there is no second
    copy of the source of truth to drift;
  * every body declares its PROVENANCE and the manifest is written next to the
    PNG. Three classes, and the distinction is the whole point:

        EXPORTED   the fabricated part, from v2/export/stl/<name>.stl, as
                   produced by v2/tools/export.py. This is the part.
        REFERENCE  a v2/cad/parts/*.scad reference body: the project's own
                   measured model of a bought component. Not fabricated, and
                   deliberately rough away from the mounting interfaces.
        VENDOR     a mesh from the house asset library, re-materialised but
                   not re-modelled.

    and anything that exists in the design but has no model yet is listed as
    ABSENT rather than drawn, because a stand-in that later disagrees with the
    real part is exactly what makes people trust a picture of a part that does
    not exist.

WHAT IS CHECKED, AND WHERE
--------------------------
check_render.py measures the EMITTED PNGs and nothing else. It never reads
parameters.scad. See its docstring; run it with --prove to watch every check
fail on a deliberately broken input and then pass again.
"""
import argparse
import hashlib
import json
import math
import os
import re
import shutil
import subprocess
import sys
import time
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
ROOT = os.path.dirname(TOOLS)                       # .../v2
REPO = os.path.dirname(ROOT)
CAD = os.path.join(ROOT, "cad")
PARTS_SCAD = os.path.join(CAD, "parts")
EXPORT_STL = os.path.join(ROOT, "export", "stl")
OUT_DEFAULT = os.path.join(ROOT, "export", "render")
#  The reference bodies depend only on v2/cad/parts/*.scad and bodies.scad,
#  never on where the PNGs go, so they are cached in ONE place. Without
#  this, check_render.py --prove rebuilds all 18 of them into a fresh
#  scratch directory -- 105 s of pure waste before the first break.
BODIES_CACHE = os.path.join(ROOT, "export", "render", ".bodies")
SCENE = os.path.join(HERE, "scene")
VENDOR_JS = os.path.join(SCENE, "vendor")

#  The house asset library. Read-only, and its own README is the licence.
ASSETS = "/home/user/kylebsmith/cmtl-3d-assets"

THREE_VERSION = "0.160.1"
THREE_FILES = {
    "three.module.js": "build/three.module.js",
    "STLLoader.js": "examples/jsm/loaders/STLLoader.js",
    "RoomEnvironment.js": "examples/jsm/environments/RoomEnvironment.js",
}


def die(msg):
    print(f"  FATAL  {msg}", file=sys.stderr)
    sys.exit(1)


def rel(p):
    return os.path.relpath(p, REPO)


def run(cmd, timeout=1800, cwd=None):
    t0 = time.time()
    r = subprocess.run(cmd, capture_output=True, text=True,
                       timeout=timeout, cwd=cwd)
    return r, time.time() - t0


# ===========================================================================
#  1. THE PARAMETERS, out of OpenSCAD and not out of anybody's memory
# ===========================================================================
ECHO_K = re.compile(r'^ECHO: "K ([A-Za-z0-9_]+)=(.*)"$')
ECHO_P = re.compile(r'^ECHO: "P ([A-Za-z0-9_]+)=(.*)"$')


def _scalar_or_vec(s):
    s = s.strip()
    if s.startswith("["):
        return [float(x) for x in s[1:-1].split(",")]
    return float(s)


def dump_params(workdir, want_carrier=True):
    """Run the two echo-only .scad dumps and parse them.

    `undef` anywhere is fatal. OpenSCAD resolves an unknown variable to undef
    and carries on, which is how a render ends up quietly placing a board at
    the origin; export.py already treats that warning as fatal and so does
    this.
    """
    out, timings = {}, {}
    for name, src, required in (("deck", "deck_params.scad", True),
                                ("carrier", "carrier_params.scad",
                                 want_carrier)):
        dest = os.path.join(workdir, f"{name}.echo")
        r, dt = run(["openscad", "--export-format=echo", "-o", dest,
                     os.path.join(HERE, src)], timeout=300)
        timings[name] = dt
        if r.returncode != 0 or not os.path.exists(dest):
            if required:
                die(f"{src}: {r.stderr.strip()[-400:]}")
            print(f"  skip  {src}: {r.stderr.strip()[-120:]}")
            continue
        bad = [ln for ln in r.stderr.splitlines() if "unknown variable" in ln]
        if bad:
            die(f"{src}: {bad[0].strip()}  (+{len(bad)-1} more)")
        with open(dest) as fh:
            for ln in fh:
                ln = ln.rstrip("\n")
                m = ECHO_K.match(ln)
                if m:
                    if "undef" in m.group(2):
                        die(f"{src}: {m.group(1)} is undef")
                    out.setdefault("K", {})[m.group(1)] = \
                        _scalar_or_vec(m.group(2))
                    continue
                m = ECHO_P.match(ln)
                if m:
                    if "undef" in m.group(2):
                        die(f"{src}: placement {m.group(1)} is undef")
                    out.setdefault("P", {})[m.group(1)] = \
                        _scalar_or_vec(m.group(2))
    if "K" not in out:
        die("the parameter dump produced nothing")
    for n, t in timings.items():
        print(f"  ok    params {n:8s} {t:5.2f}s")
    return out.get("K", {}), out.get("P", {})


# ===========================================================================
#  2. THE FABRICATED PARTS -- exported, never modelled here
# ===========================================================================
#  Each row declares where in the deck's Z its own STL has to land. The STL is
#  then MEASURED and the offset solved, rather than assumed:
#
#      tz = declared_z0 - measured_stl_zmin
#
#  so a part whose extrusion datum moves is repositioned correctly instead of
#  silently sinking, AND the resulting z-range is written into the manifest for
#  check_render.py to measure against the exported mesh. slab.scad and
#  backpanel.scad both linear_extrude from 0; carrier.scad already translates
#  itself to -carrier_t, so its solved offset is 0 -- which is a result, not an
#  assumption.
DECK_PARTS = [
    # id          scad              z0 param        thickness param  material
    ("slab",      "slab.scad",      "acrylic_z0",   "acrylic_t",
     "acrylic_clear", "acrylic"),
    ("carrier",   "carrier.scad",   None,           "carrier_h_calc",
     "petg_carrier", "carrier"),
    ("backpanel", "backpanel.scad", "deck_bot_z",   "back_t",
     "acrylic_black", "back"),
]

# ===========================================================================
#  3. THE REFERENCE BODIES -- v2/cad/parts/*.scad, one mesh per material
# ===========================================================================
#  (body name in bodies.scad, defining .scad, placement key, material, layer)
REF_BODIES = [
    ("disp_pcb",     "waveshare_rlcd42.scad",   "display",  "pcb_navy",   "components"),
    ("disp_glass",   "waveshare_rlcd42.scad",   "display",  "glass_dark", "components"),
    ("disp_metal",   "waveshare_rlcd42.scad",   "display",  "metal_bright", "components"),
    ("disp_black",   "waveshare_rlcd42.scad",   "display",  "black_plastic", "components"),
    ("disp_buttons", "waveshare_rlcd42.scad",   "display",  "button_light", "components"),
    ("disp_silk",    "waveshare_rlcd42.scad",   "display",  "silk_white", "components"),

    ("fw_pcb",       "feather_rp2040_dvi.scad", "feather",  "pcb_green",  "components"),
    ("fw_metal",     "feather_rp2040_dvi.scad", "feather",  "metal_bright", "components"),
    ("fw_black",     "feather_rp2040_dvi.scad", "feather",  "black_plastic", "components"),
    ("fw_buttons",   "feather_rp2040_dvi.scad", "feather",  "button_light", "components"),
    ("fw_silk",      "feather_rp2040_dvi.scad", "feather",  "silk_white", "components"),

    ("kbd_frame",    "k9pro_internals.scad",    "keyboard", "aluminium",  "components"),
    ("kbd_pcb",      "k9pro_internals.scad",    "keyboard", "pcb_green",  "components"),
    ("kbd_switches", "k9pro_internals.scad",    "keyboard", "switch_body", "components"),
    ("kbd_keycaps",  "k9pro_internals.scad",    "keyboard", "keycap",     "components"),
    ("kbd_sockets",  "k9pro_internals.scad",    "keyboard", "black_plastic", "components"),
    ("kbd_ports",    "k9pro_internals.scad",    "keyboard", "metal_bright", "components"),
    ("kbd_battery",  "k9pro_internals.scad",    "keyboard", "pouch_cell", "components"),
]

# ===========================================================================
#  4. THE VENDOR MESHES -- house library, re-materialised, not re-modelled
# ===========================================================================
#  A single STL carries a single material, and these files are one solid each,
#  so each is split BY FACE into measured z-bands. The band edges are not
#  invented: each is a datum parameters.scad already declares, and the split is
#  a per-face test (every vertex of the face inside the band), which needs no
#  connected-component analysis -- scipy is absent on this interpreter and
#  trimesh's split()/section() both require it (v2/docs/fabrication.md 4.7).
#
#  z here is the LIBRARY file's own z. For the 5880 and the 5295 the library's
#  +z runs rearward from the PCB's front face, so the PCB occupies 0 .. pcb_t;
#  that is what `pcb_band` means. The placement's Rx(180) turns it the right
#  way up afterwards.
VENDOR_MESHES = [
    {"id": "encoder", "place": ["encoder0", "encoder1", "encoder2"],
     "file": "02_controls/adafruit_5880_rotary-encoder-i2c-stemma-qt/"
             "5880 Rotary Encoder I2C STEMMA QT.stl",
     "note": "Adafruit 5880 I2C rotary encoder breakout",
     "bands": [("pcb_band", "pcb_green")],
     "rest": "component_satin", "anchor": None},
    {"id": "neoslider", "place": ["neoslider"],
     "file": "02_controls/adafruit_5295_neoslider-stemma/"
             "5295 NeoSlider Stemma.stl",
     "note": "Adafruit 5295 NeoSlider",
     "bands": [("pcb_band", "pcb_green")],
     "rest": "component_satin", "anchor": None},
    {"id": "pot", "place": ["pot0", "pot1", "pot2"],
     "file": "02_controls/adafruit_562_10k-potentiometer/"
             "562 10K Potentiometer.stl",
     "note": "Adafruit 562 16 mm panel pot",
     #  The library's +z is the shaft direction here, i.e. the deck's +z, and
     #  the clamping face is at z = 2.000: parameters.scad's pot_bushing is
     #  "[MEASURED] 2.000 .. 8.600". Below it is the can, above it the bushing
     #  and the shaft.
     "bands": [("pot_can_band", "component_satin")],
     "rest": "brass", "anchor": "pot_z_bottom"},
]

# ===========================================================================
#  5. WHAT IS NOT IN THE PICTURE
# ===========================================================================
#  A labelled hole, not a stand-in. Every entry here is a body the design
#  calls for and no file in this repository models, so there is nothing to
#  load; drawing a box would make the render disagree with the real part the
#  moment somebody draws it properly. --annotate prints this list onto the
#  image, and it is written into every view's manifest whether annotated or
#  not.
def absent_bodies(K):
    """Every dimension quoted here comes from the parameter dump, so the
    labelled hole states the size of the thing that is missing."""
    return [
        {"id": "knob", "count": 2 * len(K["cols"]),
         "what": f"the recessed knob: O{K['knob_d']:.3f} brim x "
                 f"{K['knob_h']:.3f}, standing {K['knob_proud']:.3f} proud of "
                 f"the slab, in a O{K['bore_d']:.3f} bore",
         "why": "parameters.scad section 5 dimensions it; no .scad in this "
                "repository models it yet"},
        {"id": "fader_cap", "count": 1,
         "what": f"the printed NeoSlider cap, {K['slider_cap_w']:.3f} wide, "
                 f"in a {K['slider_slot_w']:.3f} x {K['slider_slot_l']:.3f} "
                 f"slot",
         "why": "parameters.scad section 10 dimensions the slot, not the cap"},
        {"id": "wiring", "count": 1,
         "what": "the harness itself -- STEMMA QT leads and pot flying leads",
         "why": "carrier.scad models the CHANNELS the wires lie in, and those "
                "ARE in these renders as recesses in the carrier's top face. "
                "Nothing models the wires. v2/parts/wiring.md is a route, not "
                "a solid"},
        {"id": "fastener", "count": 16,
         "what": "6 M3 into the slab, 10 M3 into the back panel, the M2/M2.5 "
                 "board screws, and the heat inserts they thread into",
         "why": "08_cutouts/ in the house library carries the clearance and "
                "insert cutouts, not the screws"},
        {"id": "foot", "count": 4,
         "what": f"a foot at least {K['foot_h_min']:.3f} tall -- below that "
                 f"the deck rocks on the display's own 18650 holder",
         "why": "parameters.scad derives foot_h_min and no part carries it"},
    ]

# ===========================================================================
#  6. MATERIALS
# ===========================================================================
#  PHYSICAL means the value is a measured or published property of the real
#  material and changing it makes the picture wrong. ARTISTIC means it is a
#  look, chosen here, and is honest only because it is labelled.
#  REPO means the colour is the one the project's own .scad already picked in
#  a color() call, carried over so the render and the CAD preview agree.
MATERIALS = {
    #  THE HERO. 12.700 mm cast PMMA.
    "acrylic_clear": {
        "kind": "physical", "color": "#ffffff",
        #  tau_D65 ~= 92 % and 4 % reflection at EACH surface, from Roehm's own
        #  property table (DIN 5036 Part 3 / ISO 489) via v2/parts/stack.md 4.1.
        #  4 + 4 = 8 accounts for essentially the whole shortfall from 100.
        "transmission": 0.92,                      # [VENDOR roehm-gsxt]
        "ior": 1.491,                              # [VENDOR roehm-gsxt] n_D 20
        #  thickness drives the refraction path length and is the difference
        #  between a 12.700 mm slab and a sheet of cling film. Set from
        #  acrylic_t at build time, not here.
        "thickness": "@acrylic_t",                 # [CAD-EXACT]
        #  Absorption in the visible is < 0.05 % [VENDOR roehm-gsxt], so there
        #  is NO green edge and no tint: stack.md 4.1 conclusion 3 says the
        #  thick edge "will read as a solid colourless bar". The artistic
        #  instinct to tint thick acrylic green is wrong for PMMA and this is
        #  where it would have crept in.
        "attenuationDistance": 1e6,                # [VENDOR] effectively none
        "attenuationColor": "#ffffff",             # [VENDOR] no tint
        "roughness": 0.02,                         # [ARTISTIC] as-cast faces
        "metalness": 0.0,                          # [PHYSICAL] a dielectric
        "clearcoat": 0.0,                          # [PHYSICAL] no coating
        "specularIntensity": 1.0,                  # [PHYSICAL] F0 from the IOR
        #  THE ONE PLACE THIS RENDER KNOWINGLY LEAVES PHYSICS. At 1.0 the
        #  studio IBL's broad soft source reflects off the whole top face and
        #  veils the interior -- which is what a real softbox over a real
        #  polished sheet does, and it is also the thing the brief most wants
        #  not to happen ("you should be able to SEE the wiring"). 0.45 keeps
        #  the Fresnel edge reflection, which is where the 12.700 mm actually
        #  reads, and takes the sheen off the middle. A photographer solves
        #  this with a polarising filter; this is the render's version of one.
        "envMapIntensity": 0.45,                   # [ARTISTIC]
        "side": "front",
    },
    #  The back panel. acrylic.md 2.5 recommends the opaque plate; this is a
    #  dark cast sheet, so it is the same substrate without the transmission.
    "acrylic_black": {
        "kind": "physical", "color": "#15171a",
        "transmission": 0.0, "ior": 1.491,         # [VENDOR roehm-gsxt]
        "roughness": 0.08, "metalness": 0.0,       # [ARTISTIC] gloss cast face
        "clearcoat": 1.0, "clearcoatRoughness": 0.06,
        "envMapIntensity": 1.0,
    },
    #  The printed carrier. v2/parts/stack.md 5.6's material table: PETG,
    #  "matte black or very dark grey", because "through clear acrylic a dark
    #  matte plane reads as a void, and the coloured wires and nickel nuts
    #  become the only things with brightness".
    "petg_carrier": {
        "kind": "physical", "color": "#23262a",    # [REPO stack.md]
        "roughness": 0.78, "metalness": 0.0,       # [ARTISTIC] matte, ironed
        "clearcoat": 0.12, "clearcoatRoughness": 0.6,
        "envMapIntensity": 0.9,
    },
    "pcb_green": {"kind": "artistic", "color": "#1b3a2a",   # [REPO fw_board()]
                  "roughness": 0.40, "metalness": 0.0,
                  "clearcoat": 0.55, "clearcoatRoughness": 0.25},
    "pcb_navy": {"kind": "artistic", "color": "#0d2d44",    # [REPO ws_board()]
                 "roughness": 0.40, "metalness": 0.0,
                 "clearcoat": 0.55, "clearcoatRoughness": 0.25},
    "silk_white": {"kind": "artistic", "color": "#f4f4ee",
                   "roughness": 0.80, "metalness": 0.0},
    "aluminium": {"kind": "artistic", "color": "#b7bcc0",
                  "roughness": 0.30, "metalness": 1.0, "envMapIntensity": 1.0},
    "metal_bright": {"kind": "artistic", "color": "#c8c8cc",  # [REPO]
                     "roughness": 0.24, "metalness": 0.95},
    "brass": {"kind": "artistic", "color": "#b08d57",
              "roughness": 0.32, "metalness": 1.0},
    "black_plastic": {"kind": "artistic", "color": "#101010",  # [REPO]
                      "roughness": 0.45, "metalness": 0.0},
    "button_light": {"kind": "artistic", "color": "#d8d8d8",   # [REPO]
                     "roughness": 0.50, "metalness": 0.0},
    "switch_body": {"kind": "artistic", "color": "#14161a",
                    "roughness": 0.40, "metalness": 0.0},
    #  An unpowered 4.2 in IPS panel is a dark glass rectangle. It is rendered
    #  as one because that is what it looks like, and because nothing in this
    #  repository holds a frame of its output.
    "glass_dark": {"kind": "artistic", "color": "#15181c",
                   "roughness": 0.05, "metalness": 0.0,
                   "clearcoat": 1.0, "clearcoatRoughness": 0.03},
    "keycap": {"kind": "artistic", "color": "#24272c",
               "roughness": 0.58, "metalness": 0.0},
    "pouch_cell": {"kind": "artistic", "color": "#9aa0a6",
                   "roughness": 0.35, "metalness": 0.55},
    #  One material for a whole vendor mesh, where the band split leaves
    #  nothing better to say. Named so it is obvious in the manifest.
    "component_satin": {"kind": "artistic", "color": "#3b4046",
                        "roughness": 0.42, "metalness": 0.35},
}

# ===========================================================================
#  7. VIEWS
# ===========================================================================
#  One orbit parameterisation, several named points on it.
#
#      azimuth    0 deg = from -Y, the user's own side (the keyboard is at -Y)
#                 and it turns toward +X
#      elevation  0 deg = in the deck's plane, +90 = straight down the Z axis
#      explode    mm the three layers are separated along Z
#
#  `mask` views are MEASUREMENT views, not pictures: flat white on black,
#  orthographic, device pixel ratio pinned to 1 and the frustum stated in mm,
#  so check_render.py can read a dimension off the PNG in millimetres. They
#  are the same scene graph, the same meshes and the same matrices as the
#  pictures -- only the camera and the materials change.
VIEWS = {
    "hero": dict(azimuth=38, elevation=34, projection="perspective", fov=26,
                 explode=0, annotate=False,
                 caption="cYbErDeCk v2 - hero"),
    "plan": dict(azimuth=0, elevation=90, projection="orthographic",
                 ortho_w=330.0, explode=0, annotate=True,
                 caption="cYbErDeCk v2 - plan"),
    "exploded": dict(azimuth=34, elevation=24, projection="perspective",
                     fov=26, explode=70, annotate=True,
                     caption="cYbErDeCk v2 - exploded stack"),
    #  the measurement views
    "mask_plan": dict(azimuth=0, elevation=90, projection="orthographic",
                      ortho_w=330.0, explode=0, materials="mask", dpr=1,
                      annotate=False, caption=""),
    "mask_elev": dict(azimuth=0, elevation=0, projection="orthographic",
                      ortho_w=330.0, explode=0, materials="mask", dpr=1,
                      annotate=False, caption=""),
    #  the transmission control: the same frame with the acrylic taken out
    "plan_noacrylic": dict(azimuth=0, elevation=90,
                           projection="orthographic", ortho_w=330.0,
                           explode=0, hide=["acrylic"], annotate=False,
                           caption=""),
    #  One mask per fabricated part, each showing that part ALONE. C1 needs
    #  the part's own silhouette, and C2's through-hole count is only that
    #  part's holes if nothing is behind it: the slab and the back panel have
    #  the same 304.600 x 190.375 outline, so a combined mask would hide
    #  exactly the mix-up C2 exists to catch.
    "mask_slab": dict(azimuth=0, elevation=90, projection="orthographic",
                      ortho_w=330.0, explode=0, materials="mask", dpr=1,
                      only=["slab"], annotate=False, caption=""),
    "mask_backpanel": dict(azimuth=0, elevation=90, projection="orthographic",
                           ortho_w=330.0, explode=0, materials="mask", dpr=1,
                           only=["backpanel"], annotate=False, caption=""),
    "mask_carrier": dict(azimuth=0, elevation=90, projection="orthographic",
                         ortho_w=330.0, explode=0, materials="mask", dpr=1,
                         only=["carrier"], annotate=False, caption=""),
    #  The three fabricated layers in elevation, and nothing else: the
    #  encoder's shaft tip stands 0.745 above the acrylic's top face, so a
    #  full-assembly elevation would measure the shaft and not the stack.
    "mask_elev_deck": dict(azimuth=0, elevation=0, projection="orthographic",
                           ortho_w=330.0, explode=0, materials="mask", dpr=1,
                           only=["acrylic", "carrier", "back"],
                           annotate=False, caption=""),
    #  the free one. --azimuth/--elevation/--explode/--annotate steer THIS
    #  view and only this view, so a flag on the command line can never move
    #  the camera of a measurement view out from under check_render.py.
    "orbit": dict(azimuth=38, elevation=34, projection="perspective", fov=26,
                  explode=0, annotate=False, caption="cYbErDeCk v2"),
}
DEFAULT_VIEWS = ["hero", "plan", "exploded"]
CHECK_VIEWS = ["mask_slab", "mask_backpanel", "mask_carrier",
               "mask_elev_deck", "plan", "plan_noacrylic"]


# ===========================================================================
#  7b. THE DELIBERATE BREAKS
# ===========================================================================
#  A check nobody has watched fail is not a check, so each check in
#  check_render.py has a switch here that breaks the thing it measures. These
#  produce a KNOWINGLY WRONG render; `broken` goes into the manifest and into
#  every sidecar so a broken PNG can never be mistaken for a good one, and
#  check_render.py --prove is the only thing that is supposed to use them.
BREAK_MODES = {
    "scale": "every body's matrix scaled by 1.02: the scene is 2 % larger "
             "than the meshes it was built from",
    "swap":  "the slab and the back panel staged into each other's "
             "filenames: the same outline, a different hole count",
    "z":     "the slab lifted 6.000 mm, opening a gap between the carrier's "
             "rim and the acrylic",
    "transmission": "the acrylic's transmission set to 0: the standard "
                    "clear-acrylic render lie",
    "blank": "every body hidden: what a lost WebGL context looks like",
    "stl":   "handled by check_render.py, which mutates a COPY of the "
             "exported mesh and points the checker at it",
}


def apply_break(mode, bodies, mats):
    """Break the render on purpose. Returns a note for the manifest."""
    if not mode or mode == "stl":
        return None
    if mode not in BREAK_MODES:
        die(f"unknown --break {mode!r}. known: {', '.join(BREAK_MODES)}")
    note = f"{mode}: {BREAK_MODES[mode]}"
    if mode == "scale":
        for b in bodies:
            M = list(b["matrix"])
            for i in (0, 1, 2, 4, 5, 6, 8, 9, 10):
                M[i] *= 1.02
            b["matrix"] = M
    elif mode == "z":
        for b in bodies:
            if b["id"] == "slab":
                M = list(b["matrix"])
                M[14] += 6.0
                b["matrix"] = M
    elif mode == "swap":
        a = next((b for b in bodies if b["id"] == "slab"), None)
        c = next((b for b in bodies if b["id"] == "backpanel"), None)
        if a and c:
            a["stl"], c["stl"] = c["stl"], a["stl"]
            a["sha256"], c["sha256"] = c["sha256"], a["sha256"]
            #  the geometry hash has to follow the mesh too, or the swap
            #  would be caught by C6 instead of by C2 and prove nothing
            #  about C2.
            a["geom_sha"], c["geom_sha"] = c["geom_sha"], a["geom_sha"]
    elif mode == "transmission":
        mats["acrylic_clear"]["transmission"] = 0.0
    elif mode == "blank":
        for b in bodies:
            b["layer"] = "__hidden__"
    return note


# ===========================================================================
#  8. BUILD
# ===========================================================================
def ensure_three():
    os.makedirs(VENDOR_JS, exist_ok=True)
    for name, path in THREE_FILES.items():
        dest = os.path.join(VENDOR_JS, name)
        if os.path.exists(dest) and os.path.getsize(dest) > 1000:
            continue
        url = f"https://cdn.jsdelivr.net/npm/three@{THREE_VERSION}/{path}"
        print(f"  fetch three@{THREE_VERSION} {name}")
        with urllib.request.urlopen(url, timeout=120) as r:
            data = r.read()
        if len(data) < 1000:
            die(f"{url} returned {len(data)} bytes")
        with open(dest, "wb") as fh:
            fh.write(data)


def newer(a, bs):
    """True if a is missing or older than any of bs."""
    if not os.path.exists(a):
        return True
    ta = os.path.getmtime(a)
    return any(os.path.exists(b) and os.path.getmtime(b) > ta for b in bs)


def export_deck_parts(rows, do_export, stl_dir=None):
    """Make sure each fabricated part's STL exists and is not stale.

    The STL is produced by v2/tools/export.py, which is the fabricator's
    pipeline; this tool does not have its own. If the STL is older than its
    source it is REGENERATED rather than rendered, because a render of a stale
    export is the single most expensive lie this tool could tell.
    """
    #  A FROZEN SNAPSHOT is explicitly not live, so it is not checked for
    #  staleness: --stl-dir says "render exactly these meshes". It exists
    #  because check_render.py --prove must be hermetic, and v2/export/stl is
    #  a live directory that another author's export.py rewrites underneath it.
    if stl_dir:
        found = {}
        for pid in (r[0] for r in rows):
            p = os.path.join(stl_dir, f"{pid}.stl")
            if os.path.exists(p):
                found[pid] = p
            else:
                print(f"  ABSENT {pid:10s} not in the frozen {rel(stl_dir)}")
        return found, [f"frozen snapshot: {rel(stl_dir)}"]

    deps = [os.path.join(CAD, "parameters.scad"),
            os.path.join(CAD, "lib", "util.scad")]
    srcof = {r[0]: os.path.join(CAD, r[1]) for r in rows}
    found, stale = {}, []
    for pid in srcof:
        if not os.path.exists(srcof[pid]):
            print(f"  ABSENT {pid:10s} {rel(srcof[pid])} does not exist yet")
            continue
        stl = os.path.join(EXPORT_STL, f"{pid}.stl")
        if newer(stl, [srcof[pid]] + deps):
            stale.append(pid)
        found[pid] = stl
    if stale and do_export:
        print(f"  build  v2/tools/export.py {' '.join(stale)}")
        r, dt = run([sys.executable, os.path.join(TOOLS, "export.py")] + stale,
                    timeout=3600)
        for ln in (r.stdout or "").splitlines():
            print("        |", ln)
        if r.returncode != 0:
            die("export.py failed; the render would be of a stale part")
        print(f"  ok    export.py {dt:5.1f}s")
    #  Re-check, because the answer matters: a render of a stale export is the
    #  one thing this tool must never produce quietly.
    #
    #  parameters.scad is authored concurrently with this tool, so an export
    #  that was fresh when it finished can be stale again seconds later. That
    #  is not the same failure as "nobody ran export.py" and it must not be
    #  treated as one: it gets ONE retry, and if a source is still moving the
    #  render goes ahead with the staleness WRITTEN INTO EVERY SIDECAR, so the
    #  admission travels with the picture instead of only with the terminal.
    stale_note = []
    for attempt in (1, 2):
        still = [p for p in found
                 if os.path.exists(found[p])
                 and newer(found[p], [srcof[p]] + deps)]
        if not still:
            break
        if not do_export:
            die(f"{', '.join(still)}: the STL is older than its source and "
                f"--no-export was given. Run v2/tools/export.py "
                f"{' '.join(still)} first.")
        if attempt == 1:
            print(f"  retry  export.py {' '.join(still)} (a source moved "
                  f"during the export)")
            r, dt = run([sys.executable, os.path.join(TOOLS, "export.py")]
                        + still, timeout=3600)
            if r.returncode != 0:
                die("export.py failed on the retry")
            continue
        for p in still:
            why = [rel(d) for d in [srcof[p]] + deps
                   if os.path.exists(d)
                   and os.path.getmtime(d) > os.path.getmtime(found[p])]
            age = max(os.path.getmtime(d) for d in why
                      if os.path.exists(d)) - os.path.getmtime(found[p])
            stale_note.append(
                f"{p}.stl is {age:.0f} s older than {', '.join(why)} -- it "
                f"was re-exported twice and the source kept moving")
            print(f"  WARN  {stale_note[-1]}")
    for pid in list(found):
        if not os.path.exists(found[pid]):
            print(f"  ABSENT {pid:10s} no STL at {rel(found[pid])}")
            del found[pid]
    return found, stale_note


def export_ref_bodies(workdir, fn, do_build):
    """One openscad run per material group. Cached on source mtime."""
    stldir = BODIES_CACHE
    os.makedirs(stldir, exist_ok=True)
    bodies_scad = os.path.join(HERE, "bodies.scad")
    out = {}
    for name, src, place, mat, layer in REF_BODIES:
        scad = os.path.join(PARTS_SCAD, src)
        dest = os.path.join(stldir, f"ref_{name}.stl")
        if not os.path.exists(scad):
            print(f"  ABSENT {name:13s} {rel(scad)} does not exist")
            continue
        if newer(dest, [scad, bodies_scad]) :
            if not do_build:
                print(f"  stale  {name:13s} (--no-bodies)")
                if not os.path.exists(dest):
                    continue
            else:
                r, dt = run(["openscad", "-D", f'body="{name}"',
                             "-D", f"fn={fn}",
                             "--export-format=binstl", "-o", dest,
                             bodies_scad], timeout=1800)
                bad = [ln for ln in r.stderr.splitlines()
                       if "unknown variable" in ln
                       or "undefined operation" in ln]
                if bad:
                    die(f"bodies.scad {name}: {bad[0].strip()}")
                if r.returncode != 0 or not os.path.exists(dest) \
                        or os.path.getsize(dest) < 100:
                    die(f"bodies.scad {name}: empty or failed export -- "
                        f"{r.stderr.strip()[-300:]}")
                print(f"  ok    body {name:13s} {dt:6.1f}s "
                      f"{os.path.getsize(dest)/1024:8.1f} kB")
        out[name] = (dest, src, place, mat, layer)
    return out


def prepare_vendor(workdir, params):
    """Split each library mesh into measured z-bands and write one STL each.

    This re-materialises vendor geometry. It does not re-model it: every
    triangle written out is a triangle read in, unchanged, and the only
    decision made is which output file each triangle goes to.
    """
    import numpy as np
    import trimesh
    stldir = os.path.join(workdir, "stl")
    os.makedirs(stldir, exist_ok=True)
    K = params
    #  EPS is the float32 slop a binary STL round-trip leaves on a coordinate.
    #  v2/docs/fabrication.md 4.7's last row measures the worst case over this
    #  part's dimensions at 2.44e-5, so 0.001 is 40x it and still 600x tighter
    #  than the thinnest band edge below.
    EPS = 0.001
    out = []
    for spec in VENDOR_MESHES:
        src = os.path.join(ASSETS, spec["file"])
        if not os.path.exists(src):
            print(f"  ABSENT {spec['id']:13s} {src}")
            continue
        m = trimesh.load(src, force="mesh")
        lo, hi = m.bounds
        #  Band edges. Each is a datum parameters.scad already declares,
        #  anchored where necessary on a MEASURED feature of this very mesh --
        #  not on a literal retyped out of a comment.
        edges = {
            #  the PCB occupies library z 0 .. pcb_t: the library's origin IS
            #  the PCB's front face and its +z runs rearward (confirmed by the
            #  mesh: the 5880's shaft tip is at -enc_above and its rear STEMMA
            #  QT at +pcb_t + enc_qt_below)
            "pcb_band": (-EPS, K["pcb_t"] + EPS),
            #  the 562's can is everything below its clamping face. In this
            #  library frame +z is the shaft direction, so the clamping face
            #  sits pot_shoulder_below above the can's bottom -- and the can's
            #  bottom is this mesh's own minimum.
            "pot_can_band": (-1e9,
                             lo[2] + K["pot_shoulder_below"] + EPS),
        }
        fz = m.vertices[m.faces][:, :, 2]            # (F,3) face vertex z
        fmin, fmax = fz.min(axis=1), fz.max(axis=1)
        taken = np.zeros(len(m.faces), dtype=bool)
        groups = []
        for band_name, mat in spec["bands"]:
            z0, z1 = edges[band_name]
            sel = (~taken) & (fmin >= z0) & (fmax <= z1)
            taken |= sel
            groups.append((band_name, mat, sel))
        groups.append(("rest", spec["rest"], ~taken))
        for band_name, mat, sel in groups:
            n = int(sel.sum())
            if n == 0:
                continue
            sub = m.submesh([np.where(sel)[0]], append=True)
            dest = os.path.join(stldir,
                                f"vend_{spec['id']}_{band_name}.stl")
            sub.export(dest, file_type="stl")
            out.append(dict(id=f"{spec['id']}_{band_name}", vendor=spec["id"],
                            stl=dest, material=mat, place=spec["place"],
                            anchor=spec["anchor"], note=spec["note"],
                            src=spec["file"], faces=n,
                            lib_bounds=[lo.tolist(), hi.tolist()]))
            print(f"  ok    vendor {spec['id']+'/'+band_name:26s} "
                  f"{n:6d} faces -> {mat}")
    return out


def measure(stl):
    import trimesh
    m = trimesh.load(stl, force="mesh")
    lo, hi = m.bounds
    return dict(faces=int(len(m.faces)),
                euler=int(m.euler_number),
                bounds=[lo.tolist(), hi.tolist()])


def geom_sha(stl):
    """A hash of the GEOMETRY, invariant to how the file happens to be laid
    out. Equal iff the two meshes are the same set of triangles.

    WHY THIS EXISTS, AND IT IS NOT AN OPTIMISATION.
    export.py's carrier is NOT byte-reproducible. Running it twice on an
    unchanged v2/cad/carrier.scad produces two files of identical length and
    different sha256, with identical geometry by every measure taken:
    35868 faces, euler 35868, bounds and volume equal to 6 decimal places
    [MEASURED 2026-11-07, three consecutive exports: deb5387abc6019a2,
    70aac9aa02a7e1a0, 38c9d406e30bddfa]. OpenSCAD's CGAL tessellation does not
    promise a triangle order and this build does not deliver one.

    So a sha256 over the FILE is the wrong invariant for C6: it goes red on
    every re-export whether or not the part changed, and a check that cries
    wolf is a check that gets switched off. C6 keeps the byte hash as its fast
    path -- equal bytes still prove equal geometry -- and falls back to this
    when the bytes differ, so that only a real change in the part fails.

    Canonicalisation: coordinates rounded to 6 dp (float32 STL round-trip slop
    over this part is 2.44e-5, v2/docs/fabrication.md 4.7, so 6 dp is well
    inside the noise floor); each triangle rotated to start at its
    lexicographically smallest corner, which preserves winding; then the
    triangles sorted. Two files describing the same solid hash the same.
    """
    import numpy as np
    import trimesh
    m = trimesh.load(stl, force="mesh", process=False)
    t = np.round(np.asarray(m.vertices[m.faces], dtype=np.float64), 6) + 0.0
    if len(t) == 0:
        return hashlib.sha256(b"").hexdigest()[:16]

    def less(p, q):
        return ((p[:, 0] < q[:, 0])
                | ((p[:, 0] == q[:, 0])
                   & ((p[:, 1] < q[:, 1])
                      | ((p[:, 1] == q[:, 1]) & (p[:, 2] < q[:, 2])))))

    a, b, c = t[:, 0, :], t[:, 1, :], t[:, 2, :]
    start = np.zeros(len(t), dtype=np.int64)
    start = np.where(less(b, a), 1, start)
    best = np.where((start == 1)[:, None], b, a)
    start = np.where(less(c, best), 2, start)
    rows = np.arange(len(t))
    rot = np.empty_like(t)
    for k in range(3):
        rot[:, k, :] = t[rows, (start + k) % 3, :]
    flat = np.ascontiguousarray(rot.reshape(len(t), 9))
    flat = flat[np.lexsort(flat.T[::-1])]
    return hashlib.sha256(
        np.ascontiguousarray(flat).tobytes()).hexdigest()[:16]


def mat4(t, rdeg):
    """T(t) . Rz . Ry . Rx, column-major, the order deck_params.scad declares."""
    rx, ry, rz = (math.radians(a) for a in rdeg)
    cx, sx = math.cos(rx), math.sin(rx)
    cy, sy = math.cos(ry), math.sin(ry)
    cz, sz = math.cos(rz), math.sin(rz)
    Rx = [[1, 0, 0], [0, cx, -sx], [0, sx, cx]]
    Ry = [[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]]
    Rz = [[cz, -sz, 0], [sz, cz, 0], [0, 0, 1]]

    def mul(A, B):
        return [[sum(A[i][k] * B[k][j] for k in range(3)) for j in range(3)]
                for i in range(3)]
    R = mul(Rz, mul(Ry, Rx))
    det = (R[0][0] * (R[1][1] * R[2][2] - R[1][2] * R[2][1])
           - R[0][1] * (R[1][0] * R[2][2] - R[1][2] * R[2][0])
           + R[0][2] * (R[1][0] * R[2][1] - R[1][1] * R[2][0]))
    if det < 0.999:
        die(f"placement rotation {rdeg} has determinant {det:.4f}: it mirrors "
            f"the mesh, which renders it inside out")
    #  three.js Matrix4.fromArray() reads column-major
    return [R[0][0], R[1][0], R[2][0], 0,
            R[0][1], R[1][1], R[2][1], 0,
            R[0][2], R[1][2], R[2][2], 0,
            t[0], t[1], t[2], 1]


def build_manifest(args, workdir):
    K, P = dump_params(workdir, want_carrier=os.path.exists(
        os.path.join(CAD, "carrier.scad")))
    K["carrier_h_calc"] = K["carrier_t"] + K["carrier_rim_h"]
    deck, stale_note = export_deck_parts(DECK_PARTS,
                                         not args.no_export, args.stl_dir)
    refs = export_ref_bodies(workdir, args.fn, not args.no_bodies)
    vend = prepare_vendor(workdir, K)

    bodies, absent = [], absent_bodies(K)

    # ---- the fabricated parts -------------------------------------------
    for pid, src, z0k, tk, mat, layer in DECK_PARTS:
        if pid not in deck:
            absent.append(dict(id=pid, count=1,
                               what=f"the exported {pid}",
                               why=f"{rel(os.path.join(CAD, src))} has not "
                                   f"been exported to "
                                   f"{rel(EXPORT_STL)}/{pid}.stl"))
            continue
        g = measure(deck[pid])
        zmin = g["bounds"][0][2]
        if z0k is None:
            #  carrier.scad places itself in the deck frame already; prove it.
            #  This USED to be the bare `tz = 0.0` with declared_z copied from
            #  the measurement, which is a tautology: it agreed with itself
            #  whatever the mesh did, and if carrier.scad's datum moved the
            #  render would have followed it in silence. So state the datum
            #  and measure it. carrier.scad's floor is
            #      translate([0, 0, -carrier_t]) linear_extrude(carrier_t)
            #  and its rim runs to carrier_rim_h, so the mesh must span
            #  -carrier_t .. carrier_rim_h in the deck frame. Both names come
            #  from the echo dump, so no literal is retyped here.
            #  TOL is float32 STL round-trip slop: fabrication.md 4.7 measures
            #  the worst case over this part at 2.44e-5 mm, and 0.001 is 40x
            #  it [DERIVED].
            TOL = 0.001
            want_lo, want_hi = -K["carrier_t"], K["carrier_rim_h"]
            zmax = g["bounds"][1][2]
            if abs(zmin - want_lo) > TOL or abs(zmax - want_hi) > TOL:
                die(f"carrier.stl does not sit in the deck frame: it spans "
                    f"{zmin:.6f} .. {zmax:.6f}, but carrier.scad's datum "
                    f"declares -carrier_t .. carrier_rim_h = "
                    f"{want_lo:.6f} .. {want_hi:.6f} "
                    f"(d {abs(zmin - want_lo):.6f} / {abs(zmax - want_hi):.6f}"
                    f" > {TOL}). Placing it at tz = 0 would put the picture's "
                    f"carrier somewhere the deck frame does not have one.")
            tz = 0.0
            declared = [want_lo, want_hi]
        else:
            tz = K[z0k] - zmin
            declared = [K[z0k], K[z0k] + K[tk]]
        bodies.append(dict(
            id=pid, provenance="EXPORTED", material=mat, layer=layer,
            stl=f"stl/{pid}.stl", copy_from=deck[pid],
            source=f"{rel(os.path.join(CAD, src))} -> {rel(deck[pid])}",
            matrix=mat4([0, 0, tz], [0, 0, 0]),
            geom=g, declared_z=declared,
            thickness_mm=K[tk]))

    # ---- the reference bodies -------------------------------------------
    for name, (dest, src, place, mat, layer) in refs.items():
        if place not in P:
            absent.append(dict(id=name, count=1,
                               what=f"reference body {name}",
                               why=f"no placement '{place}' in the parameter "
                                   f"dump (is {place}_params.scad present?)"))
            continue
        p = P[place]
        bodies.append(dict(
            id=f"ref_{name}", provenance="REFERENCE", material=mat,
            layer=layer, stl=f"stl/ref_{name}.stl", copy_from=dest,
            source=f"{rel(os.path.join(PARTS_SCAD, src))} "
                   f"[bodies.scad body=\"{name}\"]",
            matrix=mat4(p[0:3], p[3:6]), geom=measure(dest),
            placement=place))

    # ---- the vendor meshes ----------------------------------------------
    for v in vend:
        g = measure(v["stl"])
        for pkey in v["place"]:
            if pkey not in P:
                continue
            p = list(P[pkey])
            if v["anchor"]:
                p[2] = K[v["anchor"]] - g["bounds"][0][2]
            bodies.append(dict(
                id=f"{v['id']}@{pkey}", provenance="VENDOR",
                material=v["material"], layer="components",
                stl=f"stl/{os.path.basename(v['stl'])}", copy_from=v["stl"],
                source=f"{ASSETS}/{v['src']}  [{v['note']}]",
                matrix=mat4(p[0:3], p[3:6]), geom=g, placement=pkey))

    #  stage every mesh under one served root
    stldir = os.path.join(workdir, "stl")
    os.makedirs(stldir, exist_ok=True)
    for b in bodies:
        dst = os.path.join(workdir, b["stl"])
        if b["copy_from"] != dst:
            if newer(dst, [b["copy_from"]]):
                shutil.copy2(b["copy_from"], dst)
        b["sha256"] = hashlib.sha256(
            open(dst, "rb").read()).hexdigest()[:16]
        #  the byte hash proves the file; the geometry hash proves the PART,
        #  and only the second survives a non-reproducible exporter. See
        #  geom_sha(). Only the fabricated parts are compared by C6, so only
        #  they pay for the extra mesh load.
        if b["provenance"] == "EXPORTED":
            b["geom_sha"] = geom_sha(dst)
        b.pop("copy_from")

    #  scene bounds, in mm, from the placed meshes -- this is what the camera
    #  auto-fit uses, so the framing follows the geometry rather than a guess
    lo = [1e9] * 3
    hi = [-1e9] * 3
    for b in bodies:
        M = b["matrix"]
        (bx0, by0, bz0), (bx1, by1, bz1) = b["geom"]["bounds"]
        for x in (bx0, bx1):
            for y in (by0, by1):
                for z in (bz0, bz1):
                    w = [M[0] * x + M[4] * y + M[8] * z + M[12],
                         M[1] * x + M[5] * y + M[9] * z + M[13],
                         M[2] * x + M[6] * y + M[10] * z + M[14]]
                    for i in range(3):
                        lo[i] = min(lo[i], w[i])
                        hi[i] = max(hi[i], w[i])

    mats = {}
    for k, v in MATERIALS.items():
        v = dict(v)
        for kk, vv in list(v.items()):
            if isinstance(vv, str) and vv.startswith("@"):
                v[kk] = K[vv[1:]]
        mats[k] = v
    if args.acrylic_transmission is not None:
        mats["acrylic_clear"]["transmission"] = args.acrylic_transmission
        mats["acrylic_clear"]["_overridden"] = "transmission"

    broken = apply_break(getattr(args, "break_mode", None), bodies, mats)
    if broken:
        print(f"  BROKEN ON PURPOSE  {broken}")

    return dict(
        generated=time.strftime("%Y-%m-%dT%H:%M:%S"),
        three=THREE_VERSION, params=K, placements=P,
        bodies=bodies, absent=absent, materials=mats,
        scene_bounds=[lo, hi], broken=broken, stale_sources=stale_note,
        deck_z=[K["deck_bot_z"], K["deck_top_z"]],
        layers=["back", "carrier", "components", "acrylic", "__hidden__"],
    )


# ===========================================================================
#  9. SHOOT
# ===========================================================================
def shoot(workdir, outdir, views, args):
    jobs = []
    for name in views:
        if name not in VIEWS:
            die(f"unknown view {name!r}. known: {', '.join(sorted(VIEWS))}")
        v = dict(VIEWS[name])
        v["name"] = name
        v.setdefault("materials", "physical")
        v.setdefault("hide", [])
        v.setdefault("only", None)
        v.setdefault("dpr", args.dpr)
        v["width"] = args.width
        v["height"] = args.height
        if name == "orbit":
            for k, a in (("azimuth", args.azimuth),
                         ("elevation", args.elevation),
                         ("explode", args.explode),
                         ("annotate", args.annotate)):
                if a is not None:
                    v[k] = a
        if v["projection"] == "orthographic":
            #  mm per pixel is EXACT and recorded, which is what makes the
            #  measurement views measurable
            v["mm_per_px"] = v["ortho_w"] / (v["width"] * v["dpr"])
        jobs.append(v)
    with open(os.path.join(workdir, "views.json"), "w") as fh:
        json.dump(jobs, fh, indent=1)
    os.makedirs(outdir, exist_ok=True)
    r, dt = run(["node", os.path.join(HERE, "shoot.js"),
                 "--work", workdir, "--scene", SCENE, "--out", outdir],
                timeout=1800)
    for ln in (r.stdout or "").splitlines():
        print("        |", ln)
    if r.returncode != 0:
        die(f"shoot.js failed:\n{r.stderr[-3000:]}")
    return jobs, dt


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--views", default=",".join(DEFAULT_VIEWS),
                    help="comma list; 'all' or 'check' for the sets")
    ap.add_argument("--width", type=int, default=1600)
    ap.add_argument("--height", type=int, default=1000)
    ap.add_argument("--dpr", type=int, default=2,
                    help="device pixel ratio; 2 for anything a person looks at")
    ap.add_argument("--fn", type=int, default=96,
                    help="$fn for the REFERENCE bodies only; never touches the "
                         "fabricated parts")
    ap.add_argument("--out", default=OUT_DEFAULT)
    ap.add_argument("--azimuth", type=float, default=None,
                    help="steers the 'orbit' view only, so no flag can move "
                         "a measurement view's camera")
    ap.add_argument("--elevation", type=float, default=None,
                    help="'orbit' view only")
    ap.add_argument("--explode", type=float, default=None,
                    help="'orbit' view only; mm between the three layers")
    ap.add_argument("--annotate", dest="annotate", action="store_true",
                    default=None)
    ap.add_argument("--no-annotate", dest="annotate", action="store_false")
    ap.add_argument("--acrylic-transmission", type=float, default=None,
                    help="override the acrylic's transmission. This is the "
                         "break switch check_render.py --prove uses.")
    ap.add_argument("--break", dest="break_mode", default=None,
                    choices=sorted(BREAK_MODES),
                    help="BREAK THE RENDER ON PURPOSE. Writes a knowingly "
                         "wrong picture, with 'broken' set in the manifest "
                         "and in every sidecar. check_render.py --prove uses "
                         "this and nothing else should.")
    ap.add_argument("--stl-dir", default=None,
                    help="render a FROZEN snapshot of the fabricated meshes "
                         "instead of the live v2/export/stl. Not checked for "
                         "staleness, because a snapshot is not live. "
                         "check_render.py --prove uses this so that another "
                         "author re-running export.py cannot move the ground "
                         "under a proof.")
    ap.add_argument("--no-export", action="store_true",
                    help="never invoke v2/tools/export.py; fail on a stale STL")
    ap.add_argument("--no-bodies", action="store_true",
                    help="reuse cached reference-body STLs even if stale")
    ap.add_argument("--list-bodies", action="store_true")
    args = ap.parse_args()

    if args.views == "all":
        views = list(VIEWS)
    elif args.views == "check":
        views = list(CHECK_VIEWS)
    else:
        views = [v.strip() for v in args.views.split(",") if v.strip()]

    workdir = os.path.join(args.out, ".work")
    os.makedirs(workdir, exist_ok=True)
    ensure_three()
    man = build_manifest(args, workdir)
    with open(os.path.join(workdir, "manifest.json"), "w") as fh:
        json.dump(man, fh, indent=1)

    byprov = {}
    for b in man["bodies"]:
        byprov.setdefault(b["provenance"], []).append(b["id"])
    print()
    for k in ("EXPORTED", "REFERENCE", "VENDOR"):
        ids = byprov.get(k, [])
        print(f"  {k:9s} {len(ids):3d}  {', '.join(sorted(ids))}")
    print(f"  ABSENT    {len(man['absent']):3d}  "
          f"{', '.join(a['id'] for a in man['absent'])}")
    print(f"  scene     {man['scene_bounds'][0]} .. {man['scene_bounds'][1]}")
    if man["broken"]:
        print(f"  BROKEN    {man['broken']}")
    for n in man["stale_sources"]:
        print(f"  STALE     {n}")
    print()
    if args.list_bodies:
        return 0

    jobs, dt = shoot(workdir, args.out, views, args)
    print(f"  ok    shoot {dt:5.1f}s")
    for v in jobs:
        png = os.path.join(args.out, f"{v['name']}.png")
        side = os.path.join(args.out, f"{v['name']}.json")
        with open(side, "w") as fh:
            json.dump(dict(view=v, generated=man["generated"],
                           three=man["three"], broken=man["broken"],
                           stale_sources=man["stale_sources"],
                           bodies=[{k: b[k] for k in
                                    ("id", "provenance", "material", "layer",
                                     "source", "sha256", "geom_sha")
                                    if k in b}
                                   for b in man["bodies"]],
                           absent=man["absent"],
                           materials=man["materials"],
                           params=man["params"]), fh, indent=1)
        ok = os.path.exists(png)
        print(f"  {'ok  ' if ok else 'FAIL'}  {rel(png)}"
              f"{'' if not ok else f'  {os.path.getsize(png)/1024:8.1f} kB'}")
    print("\n  checks are NOT run by this tool. Run:")
    print("    /usr/bin/python3.11 v2/tools/render/check_render.py")
    return 0


if __name__ == "__main__":
    sys.exit(main())
