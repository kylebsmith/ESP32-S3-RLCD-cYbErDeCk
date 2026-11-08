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
#  Two files, not three. RoomEnvironment.js used to be the third and the
#  scene's whole IBL; it is a neutral grey room, which is a fine default and
#  is exactly why every render of this deck came out grey. The environment is
#  now generated in scene.js -- an HDR equirectangular built texel by texel
#  and prefiltered by PMREMGenerator, with sources bright enough for a
#  roughness-0.02 face to have something to reflect. It is a build input in
#  the same sense three.js is, except that it has no download and no vendor
#  file: it is six numbers and a loop, so it is GENERATED and not FETCHED.
THREE_FILES = {
    "three.module.js": "build/three.module.js",
    "STLLoader.js": "examples/jsm/loaders/STLLoader.js",
}


def _commit_stamp():
    """The commit this render was made from, with -dirty when the tree is not
    clean. Replaces a wall clock: see the note at its call site. Falls back to
    "unknown" rather than to a timestamp, because a timestamp here is exactly
    the thing being removed."""
    import subprocess
    try:
        h = subprocess.run(["git", "rev-parse", "--short", "HEAD"],
                           cwd=os.path.dirname(os.path.abspath(__file__)),
                           capture_output=True, text=True, timeout=10)
        if h.returncode != 0:
            return "unknown"
        rev = h.stdout.strip()
        st = subprocess.run(["git", "status", "--porcelain"],
                            cwd=os.path.dirname(os.path.abspath(__file__)),
                            capture_output=True, text=True, timeout=10)
        return rev + ("-dirty" if st.returncode == 0 and st.stdout.strip()
                      else "")
    except Exception:
        return "unknown"


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
#  2b. THE PARTS FITTED LAST -- also exported, also never modelled here
# ===========================================================================
#  Six knobs, a fader cap, a cowl and four feet. v2/tools/export.py cuts every
#  one of them from its own .scad, so they are EXPORTED bodies in exactly the
#  sense the three plates are; what is different is only that each is placed
#  in PLAN as well as in Z, from a placement deck_params.scad echoes.
#
#  EVERY ONE OF THEM IS A PURE TRANSLATION -- no rotation, no flip -- and that
#  is the entire danger. A part put in upside down renders perfectly, looks
#  fine, and is wrong; nothing in the picture says so. So the translation is
#  not taken on trust. Each row declares:
#
#    span(K)   the Z range, in the deck frame, that the placed mesh must end
#              up occupying, written out of the echoed datums. Both ends are
#              checked. This catches the mistake that actually happens --
#              hanging a part off the wrong datum, e.g. deck_bot_z instead of
#              deck_bot_z - foot_h -- before anything is drawn.
#    big       which extreme face of the part carries the larger area, and
#              why. This is the test that catches a mesh that is upside down
#              rather than merely in the wrong place, because the two ends of
#              every one of these parts are different features. All five
#              figures are [MEASURED] off the exported STLs (facets coplanar
#              with the bound to 1e-6 and facing it):
#                   knob_encoder / knob_pot   top  464.4993   sole  141.2707
#                   fader_cap                 top  370.9572   sole   87.7173
#                   cowl                      top 2360.9022   sole 4443.9657
#                   foot                      top  416.6374   sole  384.6606
#              The cowl is the one that goes the other way, for the reason
#              that is the part's whole purpose: its cavity opens UPWARD to
#              swallow the display's 18650 holder, so the face against the
#              back panel is a flange ring and the face toward the desk is
#              solid. The foot's margin is the narrowest at 1.083x, and it is
#              not a tolerance -- a mesh either is this way up or it is not.
#    bias      a sign the plan centroid must have about the placement origin,
#              where the part is not centred on it. Only the foot needs one:
#              its screw axis is on the origin and its toe runs to -Y, so a
#              foot turned 180 deg in plan -- which no Z test can see -- puts
#              the toe under the deck instead of out from under it.
FITTED_PARTS = [
    ("knob_encoder", ["knob_encoder0", "knob_encoder1", "knob_encoder2"],
     lambda K: (K["acrylic_z0"], K["acrylic_z0"] + K["knob_h"]),
     "top", None, "petg_knob", "fitted",
     "knob.scad: axis on the origin, plug 0 .. acrylic_t, brim to knob_h, so "
     "z = 0 is the acrylic's UNDERSIDE -- which is what "
     "knob_h - knob_proud == acrylic_t means"),
    ("knob_pot", ["knob_pot0", "knob_pot1", "knob_pot2"],
     lambda K: (K["acrylic_z0"], K["acrylic_z0"] + K["knob_h"]),
     "top", None, "petg_knob", "fitted",
     "the same envelope as the encoder knob with six internal ribs instead "
     "of the D-flat: a different PART, not a second copy"),
    ("fader_cap", ["fader_cap"],
     #  No height datum. The cap's proud height is fc_proud, a file-local in
     #  fader_cap.scad, and parameters.scad does not carry it -- so the top
     #  of the span is None and only the base is checked. Declaring a number
     #  here that no file in v2/cad holds would be the second copy this whole
     #  tool exists to avoid.
     lambda K: (K["acrylic_z0"], None),
     "top", None, "petg_knob", "fitted",
     "fader_cap.scad: centred in XY, plug acrylic_t deep from z = 0 with the "
     "cap over it; drawn at mid-travel"),
    ("cowl", ["cowl"],
     lambda K: (K["deck_bot_z"] - K["cowl_h"], K["deck_bot_z"]),
     "bottom", None, "petg_carrier", "under",
     "cowl.scad: a tray whose cavity opens toward the back panel, so the "
     "face at z = cowl_h() is the one that closes against it"),
    ("foot", ["foot0", "foot1", "foot2", "foot3"],
     lambda K: (K["deck_bot_z"] - K["foot_h"], K["deck_bot_z"]),
     "top", (None, -1), "tpu_foot", "under",
     "foot.scad: sole on z = 0, screw axis on the origin, toe toward -Y"),
]

#  Which .scad each fitted part is cut from, for the staleness test: the STL
#  is re-exported when its own source moves, exactly as for the three plates.
#  knob.scad makes two parts off one file, which is why this is a map and not
#  a rule on the name.
FITTED_SRC = {
    "knob_encoder": "knob.scad", "knob_pot": "knob.scad",
    "fader_cap": "fader_cap.scad", "cowl": "cowl.scad", "foot": "foot.scad",
}

#  v2/tools/export.py also cuts knob_mould_brim and knob_mould_plug. They are
#  TOOLING -- the two halves of the casting mould, exported in their assembled
#  positions so the pair can be seen to close -- and they are not part of the
#  instrument. Neither table above names them, and nothing else in this file
#  loads a mesh by name, so they cannot reach a deck view by accident.

# ===========================================================================
#  3. THE REFERENCE BODIES -- v2/cad/parts/*.scad, one mesh per material
# ===========================================================================
#  (body name in bodies.scad, defining .scad, placement key, material, layer)
REF_BODIES = [
    ("disp_pcb",     "waveshare_rlcd42.scad",   "display",  "pcb_navy",   "components"),
    #  ws_display() is the spacer, the TFT border, the active area and the
    #  FPC in one mesh. The active area is where the screen image goes; the
    #  UV box that puts it there is ws_active_area(), echoed into the manifest
    #  as disp_aa, so the panel's 400 x 300 pixels land on the rectangle the
    #  STEP measured and nowhere else.
    ("disp_glass",   "waveshare_rlcd42.scad",   "display",  "lcd_reflective", "components"),
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
    labelled hole states the size of the thing that is missing.

    THIS LIST HAS JUST GOT SHORTER, AND THAT IS THE POINT OF IT. The knob, the
    fader cap and the foot used to be here; v2/tools/export.py now cuts all
    three, so they are EXPORTED bodies in FITTED_PARTS and they are in the
    picture. The discipline is not that the list stays long -- it is that
    nothing leaves it by being drawn as a stand-in. Each entry carries a
    `size` as well as a `what`, because the annotated views have room for one
    line and a hole labelled only `1x wiring` states nothing a reader can
    check.
    """
    return [
        {"id": "wiring", "count": 1, "size": "routed, not solid",
         "what": "the harness itself -- STEMMA QT leads and pot flying leads",
         "why": "carrier.scad models the CHANNELS the wires lie in, and those "
                "ARE in these renders as recesses in the carrier's top face. "
                "Nothing models the wires. v2/parts/wiring.md is a route, not "
                "a solid"},
        #  The counts are len() of the two point lists in parameters.scad,
        #  echoed, not asserted: a retyped count is a count that goes stale,
        #  and this one did -- the back panel's ten were still described as M3
        #  after commit ae4cc9f drilled them screw_m25_clear_d.
        {"id": "fastener",
         "count": int(K["n_slab_screw"] + K["n_back_screw"]),
         "size": f"{int(K['n_slab_screw'])} M3 + "
                 f"{int(K['n_back_screw'])} M2.5",
         "what": f"{int(K['n_slab_screw'])}x M3 through the slab and "
                 f"{int(K['n_back_screw'])}x M2.5 through the back panel, the "
                 f"brass heat inserts they thread into, and the DIN 125 A2.5 "
                 f"washers at six of those ten (parameters.scad 6b: the four "
                 f"that are also foot_pts() must not have one)",
         "why": "08_cutouts/ in the house library carries the clearance and "
                "insert cutouts, not the screws"},
        #  NEW, and it is the honest half of "the keycaps are grey rectangles".
        #  k9pro_internals.scad says what it models in its own words -- "the
        #  keycaps as a correct envelope: each cap's own plan size from its
        #  width in key units ... one flat top plane PER WIDTH" -- so the cap
        #  is a prism, the dish's curvature is not in it, and no file in this
        #  repository records what is printed on the tops. Drawing a legend
        #  would be inventing the one thing about the keyboard a photograph
        #  would show.
        #    NOT "the sculpted row profile", which this list used to call
        #  absent. The caps are LSA -- a UNIFORM profile -- and that is
        #  recorded, at keyboard-strip.md 345/396/828 and
        #  k9pro_internals.scad 192. The vendor model's 46 1u caps being one
        #  solid translated is therefore faithful and not a gap, and the rise
        #  with width is the DISH plus a bounding box catching the cap's own
        #  left and right edges. Whether the real hardware varies by row is
        #  NOT SOURCED; keyboard-strip.md measurement 11 is where that gets
        #  settled with a height gauge.
        {"id": "keycap legend", "count": 61,
         "size": "envelopes 18.200 square and 6.5753 tall at 1u, one flat "
                 "top plane per cap WIDTH",
         "what": "the printed legends and the dish's own curvature on the "
                 "K9 Pro's caps. The caps in this render are "
                 "k9pro_internals.scad's stated ENVELOPE -- one flat top "
                 "plane per cap width, from the measured table -- and they "
                 "are drawn as what they are",
         "why": "nothing in this repository records the K9 Pro's legends or "
                "the dish's curvature; both are properties of the bought "
                "part. The PROFILE is not on this list: it is LSA, uniform "
                "by designation, so there is no row sculpting to be missing"},
    ]

# ===========================================================================
#  5b. WHAT THE SCREEN IS SHOWING
# ===========================================================================
#  THE PANEL. Every number here is a property of the bought part, recorded in
#  docs/HARDWARE.md with its own source, and quoted with the line it is on.
#  None of them is a fabricated dimension and none of them reaches scene.js
#  except through the manifest.
#
#      400 x 300 landscape, 1 bit per pixel, bi-level   [VENDOR] H1, H3
#          docs/HARDWARE.md "Display -- the part that decides the OS"
#      Sitronix ST7305, reflective, NO BACKLIGHT        [VENDOR] H1, H2
#      the firmware defaults to 12 x 24 cells giving 33 x 12, having tried
#          6 x 12 at 66 x 25 and found it unreadable on a panel with no
#          backlight -- docs/HARDWARE.md, and the owner's words there are
#          "with such a naturally low contrast screen we gotta have sexy
#          chunky letters"                              [REPO]
#
#  A REFLECTIVE LCD DOES NOT EMIT. It is a mirror with a switchable mask in
#  front of it, so its brightness is the room's brightness and nothing else.
#  The material below is therefore an ordinary dielectric with a map, and it
#  has no emissive term at all -- a screen that glows in this render would be
#  a picture of a part that does not exist.
PANEL_PX = [400, 300]          # [VENDOR] docs/HARDWARE.md, H1 + [CODE] H6
PANEL_CELL = [12, 24]          # [REPO]   docs/HARDWARE.md, the firmware default
PANEL_GRID = [33, 12]          # [REPO]   33 x 12, same line
PANEL_BITS = 1                 # [VENDOR] H3 Features: 1 bit, bi-level

#  The character grid is 396 x 288 in a 400 x 300 frame, so 4 px and 12 px are
#  left over. Centring them is [DESIGN]: 2 px at the left, 6 px at the top.
PANEL_ORIGIN = [(PANEL_PX[0] - PANEL_GRID[0] * PANEL_CELL[0]) // 2,
                (PANEL_PX[1] - PANEL_GRID[1] * PANEL_CELL[1]) // 2]

#  THE BEZEL, AND WHY THE TEXTURE IS BIGGER THAN THE PANEL. The screen image
#  is projected onto the display body by a planar UV, and that body is the
#  whole glass stack -- spacer, TFT border, FPC -- not just the active area.
#  Everything outside the active area lands outside [0,1] and takes the
#  texture's CLAMPED EDGE TEXEL, so with a bare 400 x 300 image the module's
#  dark border would take the colour of the panel's own edge row and the
#  display would read as a white slab the size of the PCB. So the image
#  carries PANEL_BEZEL_PX of module border all round, at the panel's own
#  pitch, and the UV box is grown to match: the 400 x 300 pixels still land
#  exactly on ws_active_area() and nothing else does.
#  [DESIGN] 4: one texel would be enough for the clamp, four survive the
#  mipmap chain at the grazing angles the hero view has.
PANEL_BEZEL_PX = 4

WIRING_MD = os.path.join(ROOT, "parts", "wiring.md")


def _md_rows(path, heading, want):
    """The first markdown table under `heading`, as lists of stripped cells.

    `want` is called on each row and decides whether it is one of the rows
    being looked for. Nothing is matched by position, because a column order
    is exactly the kind of thing that changes under a tool that is not looking.
    """
    if not os.path.exists(path):
        die(f"{rel(path)} does not exist, and the screen's content is read "
            f"out of it")
    out, seen = [], False
    with open(path) as fh:
        for ln in fh:
            if ln.startswith("#"):
                if seen and out:
                    break
                seen = heading in ln
                continue
            if not seen or not ln.lstrip().startswith("|"):
                continue
            cells = [c.strip().strip("*").strip()
                     for c in ln.strip().strip("|").split("|")]
            if want(cells):
                out.append(cells)
    return out


def screen_content(K):
    """The 33 x 12 character grid the panel is showing, READ OUT OF THE REPO.

    This is the one thing in the picture that is neither geometry nor a
    material: an image on a screen. So the rule that applies to a dimension
    applies to it too -- it is not composed here out of plausible-looking
    text, it is assembled from tables v2/parts/wiring.md already carries, and
    if those tables move the screen follows them. A parse that finds nothing
    is fatal, for the same reason an `undef` in the parameter dump is: a
    silently empty screen is a render of a dead instrument dressed up as a
    live one.

    What is NOT claimed: this is not a frame captured from the firmware.
    firmware/main/main.c draws the real UI and nothing in this tool runs it.
    It is the render's own composition of the repository's device tables, laid
    out on the panel's own character grid, and every sidecar says so in those
    words.
    """
    #  wiring.md 2.3: "| 0x30 | NeoSlider 5295 | none -- as shipped |"
    i2c = _md_rows(WIRING_MD, "Addresses for the four boards",
                   lambda c: len(c) >= 2 and re.fullmatch(r"0x[0-9a-fA-F]{2}",
                                                          c[0]))
    #  wiring.md 3.1: "| **1** | **ADC1_CH0** | **7** | ... |"
    adc = _md_rows(WIRING_MD, "Which pins can read an analogue voltage",
                   lambda c: len(c) >= 3 and re.fullmatch(r"\d+", c[0])
                   and re.fullmatch(r"ADC\d_CH\d", c[1]))
    if not i2c:
        die(f"{rel(WIRING_MD)} 2.3: no I2C address rows found. The screen's "
            f"content comes from that table and nothing invents it here.")
    if not adc:
        die(f"{rel(WIRING_MD)} 3.1: no ADC pin rows found. Same rule.")

    enc = [c[0] for c in i2c if c[1].lower().startswith("encoder")]
    sld = [c[0] for c in i2c if c[1].lower().startswith("neoslider")]
    gp = [f"GP{c[0]}" for c in adc]
    ch = [c[1].split("_")[-1] for c in adc]

    def cols(label, items):
        return (" " + label.ljust(6)
                + "".join(x.ljust(8) for x in items)).rstrip()

    W = PANEL_GRID[0]
    #  The two rows are in the order the deck has them: the encoders are at
    #  enc_y and the pots at pot_y, and enc_y is the rearward of the two, so
    #  ENC sits above POT on the screen exactly as it does on the panel.
    lines = [
        " cYbErDeCk v2       ST7305 RLCD",
        "",
        cols("ENC", enc),
        cols("POT", gp),
        cols("", ch),
        cols("SLD", sld),
        "",
        f" {len(i2c)} devices on one I2C chain",
        f" {len(adc)} pots on {adc[0][1].split('_')[0]}",
        "",
        f" {PANEL_PX[0]} x {PANEL_PX[1]} px   {PANEL_BITS} bit   no light",
        f" {PANEL_GRID[0]} x {PANEL_GRID[1]} cells of "
        f"{PANEL_CELL[0]} x {PANEL_CELL[1]} px",
    ]
    over = [i for i, t in enumerate(lines) if len(t) > W]
    if over:
        die(f"screen line {over[0]} is {len(lines[over[0]])} characters and "
            f"the panel's grid is {W}: {lines[over[0]]!r}")
    lines = (lines + [""] * PANEL_GRID[1])[:PANEL_GRID[1]]
    return dict(
        px=PANEL_PX, cell=PANEL_CELL, grid=PANEL_GRID, origin=PANEL_ORIGIN,
        bezel=PANEL_BEZEL_PX, bits=PANEL_BITS, lines=lines,
        invert=[0, PANEL_GRID[1] - 1],
        source=f"composed by render.py from {rel(WIRING_MD)} 2.3 (I2C "
               f"addresses) and 3.1 (ADC pins), on the panel's own "
               f"{PANEL_GRID[0]} x {PANEL_GRID[1]} character grid "
               f"[REPO docs/HARDWARE.md]. NOT a frame captured from "
               f"firmware/ -- nothing here runs the firmware.")


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
        #  not to happen ("you should be able to SEE the wiring"). This keeps
        #  the Fresnel edge reflection, which is where the 12.700 mm actually
        #  reads, and takes the sheen off the middle. A photographer solves
        #  this with a polarising filter; this is the render's version of one.
        #
        #  IT WAS 0.45, AND 0.45 WAS CALIBRATED AGAINST A DIFFERENT ROOM.
        #  The environment used to be three.js's RoomEnvironment, whose
        #  brightest source is about 1.0; it is now a generated HDR studio
        #  whose key reaches 9.0, so the same 0.45 veiled the slab exactly as
        #  1.0 veiled it before -- measured on the exploded view, where the
        #  top face reads as a frosted sheet at a grazing angle. A filter
        #  that stops a stop and a half of a brighter light has to be a
        #  denser filter. Still [ARTISTIC], still NOT SOURCED, and now at
        #  least re-derived from the thing it is a filter against.
        "envMapIntensity": 0.22,                   # [ARTISTIC]
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
    #  THE DISPLAY, AND IT IS NOT AN IPS PANEL. An earlier version of this row
    #  called it one and rendered it as a dark glass rectangle because "nothing
    #  in this repository holds a frame of its output". The first half was
    #  wrong about the part and the second half was wrong about the repository.
    #
    #  It is a 4.2 in REFLECTIVE LCD on a Sitronix ST7305, 400 x 300 at one
    #  bit, with NO BACKLIGHT [VENDOR, docs/HARDWARE.md H1/H2/H3]. A reflective
    #  LCD is a mirror with a switchable mask over it: every photon it sends
    #  you came out of the room. So this is a dielectric with a map and NO
    #  EMISSIVE TERM -- the one thing that would make this render a lie about
    #  the part is a screen that glows.
    #
    #  roughness 0.32 [ARTISTIC]: the front polariser of an RLCD is a diffuser,
    #  which is the whole reason the thing reads like paper and the whole
    #  reason it survives a desk lamp. 0.32 is a look, not a measurement, and
    #  nothing in this repository measures it.
    "lcd_reflective": {
        "kind": "physical", "color": "#ffffff",
        "roughness": 0.32, "metalness": 0.0,
        "clearcoat": 0.35, "clearcoatRoughness": 0.10,
        "envMapIntensity": 1.0,
        "map": "screen",                           # the panel's own framebuffer
        #  The two tones. That there are EXACTLY TWO is sourced -- 1 bit,
        #  bi-level [VENDOR H3 Features] -- and the two values are not:
        #  no reflectance for this panel is recorded anywhere in this
        #  repository, so both are [ARTISTIC] and listed as NOT SOURCED.
        "map_paper": "#8d9188",                    # [ARTISTIC] NOT SOURCED
        "map_ink": "#16181a",                      # [ARTISTIC] NOT SOURCED
        #  The dark surround the clamped texture gives everything outside the
        #  active area -- the spacer, the TFT border, the FPC. It is the
        #  module's own border and it is what stops the whole glass stack
        #  taking the colour of the panel's edge row.
        "map_bezel": "#15181c",                    # [ARTISTIC]
    },
    #  The glass stack around the active area: spacer, TFT border, FPC. Still
    #  dark glass, because that is what the border of a reflective panel is.
    "glass_dark": {"kind": "artistic", "color": "#15181c",
                   "roughness": 0.05, "metalness": 0.0,
                   "clearcoat": 1.0, "clearcoatRoughness": 0.03},
    #  PBT double-shot, which is what the caps on a K9 Pro are. Matte, with
    #  almost no specular lobe of its own -- the sheen on a keycap in a photo
    #  is the room, not the plastic, which is why this one is nearly all
    #  roughness and why it needed a room worth reflecting before it read as
    #  anything but grey. [ARTISTIC], every number.
    #
    #  WHAT THE GEOMETRY IS, because the material must not be asked to cover
    #  for it: k9pro_internals.scad draws each cap as "a correct envelope ...
    #  one flat top plane PER WIDTH". The dish's curvature is not in it and
    #  there are no legends, and both are in the ABSENT list rather than
    #  painted on here. There is no ROW profile to miss -- the caps are LSA,
    #  which is uniform.
    "keycap": {"kind": "artistic", "color": "#2a2d33",
               "roughness": 0.72, "metalness": 0.0,
               "clearcoat": 0.08, "clearcoatRoughness": 0.55,
               "envMapIntensity": 1.0},
    "pouch_cell": {"kind": "artistic", "color": "#9aa0a6",
                   "roughness": 0.35, "metalness": 0.55},
    #  One material for a whole vendor mesh, where the band split leaves
    #  nothing better to say. Named so it is obvious in the manifest.
    "component_satin": {"kind": "artistic", "color": "#3b4046",
                        "roughness": 0.42, "metalness": 0.35},
    #  THE KNOBS AND THE FADER CAP. v2/parts/knobs.md 717 specifies PETG (or
    #  ASA) and offers two colours: "natural / clear PETG if you want 2.4's
    #  light; matte black otherwise -- the NeoPixel argument". 2.4's light is
    #  the 5880's own NeoPixel shining up through a translucent knob, and
    #  NOTHING IN THIS RENDER MODELS THAT LIGHT, so the clear option would be
    #  a knob lit by an emitter that is not in the scene. The matte black
    #  branch is the one that can be drawn honestly, and it is drawn.
    #
    #  Smoother than the carrier on purpose and for a stated reason: both
    #  knob.scad and fader_cap.scad say to print it cap down, because "the top
    #  face is the only one anyone looks at" -- and a face printed against the
    #  build plate comes off with the plate's finish, not with layer lines.
    #  [REPO] for the material and the colour branch, [ARTISTIC] for the
    #  finish.
    "petg_knob": {"kind": "artistic", "color": "#1a1c20",
                  "roughness": 0.34, "metalness": 0.0,
                  "clearcoat": 0.55, "clearcoatRoughness": 0.22,
                  "envMapIntensity": 1.0},
    #  The feet, and they are the only part in the build that is neither PETG
    #  nor PMMA: TPU 95A, which foot.scad argues for at length -- a rigid pad
    #  takes lateral impulses from 61 keys and two hands and walks. TPU 95A
    #  reads as a soft, slightly waxy black with a wide specular lobe.
    #  [REPO] the material, [ARTISTIC] the finish.
    "tpu_foot": {"kind": "artistic", "color": "#121314",
                 "roughness": 0.85, "metalness": 0.0,
                 "clearcoat": 0.20, "clearcoatRoughness": 0.80,
                 "envMapIntensity": 0.9},
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
    #
    #  ITS DEFAULT USED TO BE hero's, EXACTLY. That made orbit.png a byte-for-
    #  byte copy of hero.png and the tool's own claim of "11 views" a claim of
    #  10 pictures and one duplicate. A free camera whose default is another
    #  view's is not a free camera, so the default is now a camera no other
    #  view has: low and from the player's right, which is the angle that
    #  rakes the slab's 12.700 mm edge and stands the knobs up against the
    #  backdrop. 11 views, 11 distinct images.
    "orbit": dict(azimuth=-62, elevation=14, projection="perspective", fov=30,
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
    "screen": "the panel's framebuffer blanked to one tone: a reflective LCD "
              "drawn with nothing on it, which is what a dead instrument "
              "looks like and what every other material being correct hides",
    "thick":  "the acrylic's refraction thickness tripled: the slab still "
              "transmits, the picture still looks plausible, and everything "
              "under it has moved further than 12.700 mm of PMMA can move it",
}

#  [DESIGN] 3. Big enough that the displacement clears the derived ceiling by
#  a wide margin, small enough that the shifted tile is still inside C4's own
#  search window -- a break that saturates the search would fail the match
#  term instead of the displacement term and would prove the wrong thing.
BREAK_THICK_FACTOR = 3.0


def apply_break(mode, bodies, mats, screen=None):
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
    elif mode == "thick":
        mats["acrylic_clear"]["thickness"] *= BREAK_THICK_FACTOR
    elif mode == "screen":
        #  Not "remove the map" -- that would change the material and C7 would
        #  be catching a missing texture rather than a blank panel. Blank the
        #  CONTENT and leave everything else alone.
        if screen is not None:
            screen["lines"] = [""] * len(screen["lines"])
            screen["invert"] = []
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


def face_extremes(stl):
    """The area of the planar face at each end of the mesh's own Z, and its
    plan centroid. This is what tells a part that is the right way up from one
    that is upside down, and it needs no tolerance to tune: a facet counts
    only if all three of its vertices sit on the bound and it faces outward,
    so the answer is a property of the solid and not of a threshold."""
    import numpy as np
    import trimesh
    m = trimesh.load(stl, force="mesh")
    lo, hi = m.bounds
    z = m.vertices[m.faces][:, :, 2]
    nz = m.face_normals[:, 2]
    a = m.area_faces
    EPS = 1e-6
    top = float(a[(np.abs(z - hi[2]) < EPS).all(axis=1) & (nz > 0.9)].sum())
    bot = float(a[(np.abs(z - lo[2]) < EPS).all(axis=1) & (nz < -0.9)].sum())
    c = m.centroid
    return top, bot, [float(c[0]), float(c[1])]


def geom_sha(stl):
    """A hash of the GEOMETRY, invariant to how the file happens to be laid
    out. Equal iff the two meshes are the same set of triangles.

    WHY THIS EXISTS, AND IT IS NOT AN OPTIMISATION.
    IT USED TO SAY the export was not byte-reproducible, and quoted three
    hashes of one unchanged carrier.scad to prove it. THAT IS NO LONGER TRUE
    AND THE PROSE WAS LEFT BEHIND. export.py now runs canonicalise_stl() over
    every mesh it writes -- a cyclic rotation of each facet's corners followed
    by a sort of the facet records, which is provably a reordering of the
    facet block and nothing else -- and two consecutive full exports give ten
    byte-identical STLs. The cause of the old nondeterminism was established
    before it was fixed: facet EMISSION ORDER only, proved by comparing four
    runs' 48-byte facet records as multisets (export.py section 1b).

    So why keep the fallback at all? Because what a byte difference MEANS has
    changed, not whether it can happen. It now means THE PICTURE PREDATES A
    RE-EXPORT: v2/export/stl is a live directory and another author's
    export.py rewrites it, which it did twice while this tool was being
    written. If that re-export did not change the part, the picture is still a
    picture of the part and failing it would be crying wolf -- and a check
    that cries wolf is a check that gets switched off. C6 keeps the byte hash
    as its fast path, because equal bytes still prove equal geometry, and asks
    this question when they differ. It is also the only thing that would
    survive export.py losing its canonical pass.

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
    #  The fitted parts go through exactly the same staleness machinery as the
    #  three plates: same exporter, same "regenerate rather than render a
    #  stale part" rule, same --no-export refusal. The only reason they are a
    #  second table is that they are placed in plan as well as in Z.
    fitted_rows = [(fid, FITTED_SRC[fid], None, None, mat, layer)
                   for fid, _, _, _, _, mat, layer, _ in FITTED_PARTS]
    deck, stale_note = export_deck_parts(DECK_PARTS + fitted_rows,
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

    # ---- the parts fitted last ------------------------------------------
    #  TOL is the float32 STL round-trip slop, as everywhere else in this file:
    #  v2/docs/fabrication.md 4.7 measures the worst case over this part at
    #  2.44e-5 mm and 0.001 is 40x it [DERIVED].
    FIT_TOL = 0.001
    for fid, keys, span, big, bias, mat, layer, note in FITTED_PARTS:
        if fid not in deck:
            absent.append(dict(
                id=fid, count=len(keys), size="not exported",
                what=f"the exported {fid}",
                why=f"{rel(os.path.join(CAD, FITTED_SRC[fid]))} has not been "
                    f"exported to {rel(EXPORT_STL)}/{fid}.stl"))
            continue
        g = measure(deck[fid])
        zlo, zhi = g["bounds"][0][2], g["bounds"][1][2]
        want_lo, want_hi = span(K)
        top_a, bot_a, cen = face_extremes(deck[fid])

        #  WHICH WAY UP. Both ends of every one of these parts are different
        #  features, so the bigger planar face says which end is which, and a
        #  mesh that comes back the other way up is caught here rather than in
        #  a picture nobody measures.
        if (big == "top") != (top_a > bot_a):
            die(f"{fid}.stl is upside down: its face at z = {zhi:.4f} is "
                f"{top_a:.4f} mm2 and its face at z = {zlo:.4f} is "
                f"{bot_a:.4f}, and {note.split(':')[0]} declares the larger "
                f"of the two to be the {big}. Placing it would stand the part "
                f"on the wrong face, which renders perfectly and is wrong.")
        #  WHICH WAY ROUND. Only where the part is not centred on its own
        #  placement origin, and the foot is the only one: a foot turned
        #  180 deg in plan puts its toe under the deck.
        if bias:
            for ax, sgn in enumerate(bias):
                if sgn and cen[ax] * sgn <= 0:
                    die(f"{fid}.stl faces the wrong way in plan: its "
                        f"centroid is at ({cen[0]:+.4f}, {cen[1]:+.4f}) and "
                        f"{note} requires axis {ax} to be "
                        f"{'negative' if sgn < 0 else 'positive'}.")

        for key in keys:
            if key not in P:
                absent.append(dict(
                    id=key, count=1, size="no placement",
                    what=f"fitted part {fid}",
                    why=f"deck_params.scad echoes no placement '{key}'"))
                continue
            t, r = list(P[key][0:3]), list(P[key][3:6])
            #  Every one of these is a PURE TRANSLATION by its author's own
            #  statement of its frame. A rotation here would mean the
            #  placement and the part disagree about which way the part is
            #  drawn, and that is exactly the failure that is silent.
            if any(abs(a) > 1e-9 for a in r):
                die(f"placement {key} carries a rotation {r}; every fitted "
                    f"part is drawn at its own origin and placed by a "
                    f"translation, so a rotation means the mesh and the "
                    f"placement disagree.")
            got_lo, got_hi = t[2] + zlo, t[2] + zhi
            if abs(got_lo - want_lo) > FIT_TOL:
                die(f"{key}: the placed mesh starts at z = {got_lo:.6f} and "
                    f"the datums declare {want_lo:.6f} "
                    f"(d {abs(got_lo - want_lo):.6f} > {FIT_TOL}). {note}")
            if want_hi is not None and abs(got_hi - want_hi) > FIT_TOL:
                die(f"{key}: the placed mesh ends at z = {got_hi:.6f} and "
                    f"the datums declare {want_hi:.6f} "
                    f"(d {abs(got_hi - want_hi):.6f} > {FIT_TOL}). {note}")
            bodies.append(dict(
                id=key, part=fid, provenance="EXPORTED", material=mat,
                layer=layer, stl=f"stl/{fid}.stl", copy_from=deck[fid],
                source=f"{rel(os.path.join(CAD, FITTED_SRC[fid]))} -> "
                       f"{rel(deck[fid])}  [{note}]",
                matrix=mat4(t, r), geom=g, placement=key,
                declared_z=[want_lo, want_hi if want_hi is not None else got_hi],
                height_mm=zhi - zlo))

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

    screen = screen_content(K)
    #  The UV rectangle the screen image is projected onto, in the display
    #  body's OWN frame: ws_active_area() as echoed, [u0, v0, u1, v1]. The
    #  pixel pitch that falls out of it is checked here rather than asserted,
    #  because docs/DATUMS.md D-07 settles the panel's orientation on exactly
    #  this property -- "only 400 x 300 gives square 0.2120 mm pixels against
    #  the enclosure's active area" -- and a render that silently stretched
    #  the panel would make that argument a lie.
    if "disp_aa" in K:
        u0, v0, u1, v1 = K["disp_aa"][:4]
        screen["body"] = "ref_disp_glass"
        screen["box"] = [u0, v0, u1, v1]
        px = [(u1 - u0) / screen["px"][0], (v1 - v0) / screen["px"][1]]
        screen["pitch_mm"] = px
        #  the UV box, grown by the bezel at the panel's own pitch, so the
        #  active area still maps 1:1 onto ws_active_area()
        b = screen["bezel"]
        screen["box"] = [u0 - b * px[0], v0 - b * px[1],
                         u1 + b * px[0], v1 + b * px[1]]
        screen["aa_box"] = [u0, v0, u1, v1]
        #  AND WHERE THAT RECTANGLE LANDS IN THE DECK, so a checker can find
        #  the panel in the picture without reading any CAD source. The
        #  display's placement is a pure translation, which deck_params.scad
        #  states and nothing here assumes: if it ever carries a rotation this
        #  box stops being a box and the claim is dropped rather than bent.
        dp = P.get("display")
        if dp and not any(abs(a) > 1e-9 for a in dp[3:6]):
            screen["deck_box"] = [dp[0] + u0, dp[1] + v0,
                                  dp[0] + u1, dp[1] + v1]
        if abs(px[0] - px[1]) > 1e-4:
            die(f"the panel's pixels are not square on this active area: "
                f"{px[0]:.6f} x {px[1]:.6f} mm. docs/DATUMS.md D-07 settles "
                f"the orientation on them being square.")
    else:
        screen["body"] = None

    broken = apply_break(getattr(args, "break_mode", None), bodies, mats,
                         screen)
    if broken:
        print(f"  BROKEN ON PURPOSE  {broken}")

    return dict(
        #  THE COMMIT, NOT THE CLOCK. This was time.strftime(), which made
        #  every sidecar and the manifest differ between two runs of identical
        #  inputs -- the one source of nondeterminism in the whole render, and
        #  the same defect dxf_post.py's DXF_EPOCH exists to pin. A wall clock
        #  records when the file was written, which nothing needs; the commit
        #  records what it was written FROM, which is the only honest answer
        #  to "where did this picture come from", and it is what a reader
        #  would have to recover by hand otherwise.
        generated=_commit_stamp(),
        three=THREE_VERSION, params=K, placements=P,
        #  ACES filmic, and the exposure it is given. [ARTISTIC], and it lives
        #  here rather than in scene.js so that it is (a) in every sidecar
        #  next to the picture it made and (b) variable from the command line,
        #  which is what lets calibrate.py build a set of legitimately
        #  different-looking good renders to set thresholds against.
        exposure=args.exposure,
        bodies=bodies, absent=absent, materials=mats,
        scene_bounds=[lo, hi], broken=broken, stale_sources=stale_note,
        deck_z=[K["deck_bot_z"], K["deck_top_z"]],
        screen=screen,
        #  The exploded view moves whole layers, so a layer is "what comes
        #  away together". The two new ones:
        #    under   the cowl and the four feet. They are screwed to the back
        #            panel, so they travel with it and stay in contact.
        #    fitted  the six knobs and the fader cap. They travel with the
        #            acrylic, and not with the shafts they are pressed onto,
        #            because the brim is O25.400 over a O22.000 bore: the
        #            slab cannot come off until the knob does, and an
        #            explosion that passes the slab through the knob is a
        #            picture of an assembly nobody can build.
        #  Both are separate layers rather than aliases of `back` and
        #  `acrylic` because mask_elev_deck isolates the three fabricated
        #  LAYERS by name, and a knob standing 6.350 proud inside that mask
        #  would add itself to the stack height C3 measures.
        layers=["back", "under", "carrier", "components", "acrylic",
                "fitted", "__hidden__"],
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
    ap.add_argument("--exposure", type=float, default=0.78,
                    help="ACES filmic tone-mapping exposure. [ARTISTIC] "
                         "0.78 is the shipped value; calibrate.py varies it "
                         "to build a set of good renders.")
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
    #  the camera scene.js actually built, per view
    campath = os.path.join(workdir, "cameras.json")
    cams = json.load(open(campath)) if os.path.exists(campath) else {}
    for v in jobs:
        png = os.path.join(args.out, f"{v['name']}.png")
        side = os.path.join(args.out, f"{v['name']}.json")
        with open(side, "w") as fh:
            json.dump(dict(view=v, camera=cams.get(v["name"]),
                           generated=man["generated"],
                           three=man["three"], broken=man["broken"],
                           stale_sources=man["stale_sources"],
                           bodies=[{k: b[k] for k in
                                    ("id", "part", "provenance", "material",
                                     "layer", "source", "sha256", "geom_sha",
                                     "thickness_mm", "height_mm")
                                    if k in b}
                                   for b in man["bodies"]],
                           absent=man["absent"],
                           #  The sidecar carries the screen's content and its
                           #  provenance, so a reader of the PNG can see where
                           #  every glyph on the panel came from -- and so
                           #  check_render.py can find the active area in the
                           #  frame without reading any CAD source.
                           screen=man["screen"],
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
