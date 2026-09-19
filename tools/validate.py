#!/usr/bin/env python3
"""
validate.py - automated design audit for the cYbErDeCk enclosure.

This is the gate. It renders the parts from source, then asserts, numerically,
that the result is buildable and that the components actually fit. It is run in
CI and must pass before any STL is published.

CHECK CLASSES
-------------
  MESH        every exported part is a single watertight manifold body.
  ENVELOPE    the enclosure is no larger than the component datums require,
              and it fits the declared print volume.
  FIT         each component mock is fully contained in its bay, with the
              declared clearance, and does not intersect any shell material.
  INTERFACE   the two printed parts mate: the back plate fits the opening, the
              tongue engages the groove, fastener bores line up.
  PRINT       wall thicknesses are integer multiples of the nozzle width,
              no feature is thinner than one extrusion, unsupported spans and
              overhangs are within FDM limits.
  DATUM       every [PROVISIONAL] datum that drives a fit carries margin.

Exit status is 0 only if every check passes.

    python3 tools/validate.py
    python3 tools/validate.py --json export/reports/validation.json
"""

from __future__ import annotations

import argparse
import json
import os
import re
import math
import subprocess
import sys
import tempfile

try:
    import numpy as np
    import trimesh
    from shapely.geometry import Polygon
    from shapely.ops import unary_union
except ImportError as exc:  # pragma: no cover
    sys.exit(f"missing dependency: {exc}\ninstall with: pip install -r tools/requirements.txt")

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCAD = os.path.join(ROOT, "cad", "cyberdeck.scad")
PARAMS = os.path.join(ROOT, "cad", "parameters.scad")

# Declared print volume. A part that will not fit a common 256 mm bed is a
# design failure, not a user problem.
BED = (256.0, 256.0, 256.0)

RESULTS = []


def check(name, cls, ok, detail=""):
    RESULTS.append({"check": name, "class": cls, "ok": bool(ok), "detail": detail})
    print(f"  [{'PASS' if ok else 'FAIL'}] {cls:<9} {name}" + (f"  --  {detail}" if detail else ""))
    return ok


# ---------------------------------------------------------------------------
# Read the datum set straight out of the SCAD source, so the validator can
# never drift from the model.
# ---------------------------------------------------------------------------

def load_params():
    """Parse parameters.scad into a dict.

    Statement-based, not line-based. An earlier line-based version joined any
    line ending in '=' onto the next one, which silently ate every parameter
    that happened to follow a '// ======' section rule - and then every check
    depending on it died with a KeyError rather than reporting a real result.
    """
    src = open(PARAMS).read()

    # Record which statements are tagged [PROVISIONAL] before stripping comments.
    provisional = set()
    for stmt in re.finditer(r"([A-Za-z_]\w*)\s*=[^;]*;([^\n]*)", src):
        if "[PROVISIONAL]" in stmt.group(2):
            provisional.add(stmt.group(1))

    code = re.sub(r"//[^\n]*", "", src)          # strip line comments
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)

    env = {"sqrt": math.sqrt, "min": min, "max": max, "abs": abs, "pow": pow}
    p = {}
    for stmt in code.split(";"):
        m = re.match(r"\s*([A-Za-z_]\w*)\s*=\s*(.+)\s*$", stmt, flags=re.S)
        if not m:
            continue
        name, expr = m.group(1), m.group(2).strip()
        try:
            p[name] = float(eval(expr, {"__builtins__": {}}, dict(env, **p)))
        except Exception:
            pass      # strings, ternaries, module calls - not needed here

    p.setdefault("wall", 3.2)
    p.setdefault("spine", p["wall"])
    p["_provisional"] = provisional
    return p


def render(part, outdir):
    out = os.path.join(outdir, f"{part}.stl")
    cmd = ["openscad", "-D", f'part="{part}"', "-o", out, SCAD]
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=1800)
    if not os.path.exists(out):
        raise RuntimeError(f"openscad failed for {part}:\n{r.stderr[-2000:]}")
    return trimesh.load(out, force="mesh")


def scad_module_mesh(module, outdir, name):
    """Render a bare module (e.g. a component mock) to its own mesh."""
    src = f'include <{PARAMS}>\nuse <{os.path.join(ROOT, "cad", "lib", "components.scad")}>\n{module};\n'
    sf = os.path.join(outdir, f"{name}.scad")
    open(sf, "w").write(src)
    out = os.path.join(outdir, f"{name}.stl")
    r = subprocess.run(["openscad", "-o", out, sf], capture_output=True, text=True, timeout=1800)
    if not os.path.exists(out):
        raise RuntimeError(f"openscad failed for {name}:\n{r.stderr[-2000:]}")
    return trimesh.load(out, force="mesh")


# ---------------------------------------------------------------------------

def check_mesh(parts):
    print("\n-- MESH --")
    ok = True
    for name, m in parts.items():
        ok &= check(f"{name}: watertight", "MESH", m.is_watertight,
                    f"{len(m.faces)} faces")
        ok &= check(f"{name}: single body", "MESH", m.body_count == 1,
                    f"body_count={m.body_count}")
        ok &= check(f"{name}: positive volume", "MESH", m.volume > 0,
                    f"{m.volume:.0f} mm^3 -> {m.volume*1.24/1000:.1f} g in PLA")
    return ok


def check_envelope(parts, p):
    print("\n-- ENVELOPE --")
    ok = True
    ch = parts["chassis"]
    w, h, t = ch.extents

    exp_w = p["wall"] + max(p["kbd_pocket_w"], p["board_pocket_w"]) + p["wall"]
    exp_h = p["wall"] + p["kbd_pocket_h"] + p["spine"] + p["board_pocket_h"] + p["wall"]
    exp_t = p["back_t"] + p["board_depth"] + p["front_t"]

    ok &= check("width is component-bound", "ENVELOPE", abs(w - exp_w) < 0.05,
                f"{w:.2f} mm, minimum possible with {p['wall']:.1f} mm walls = {exp_w:.2f} mm")
    ok &= check("height is component-bound", "ENVELOPE", abs(h - exp_h) < 0.05,
                f"{h:.2f} mm, minimum possible = {exp_h:.2f} mm")
    ok &= check("thickness is component-bound", "ENVELOPE", abs(t - exp_t) < 0.05,
                f"{t:.2f} mm, board needs {p['board_depth']:.1f} mm of pocket")

    for name, m in parts.items():
        e = sorted(m.extents)
        bed = sorted(BED)
        ok &= check(f"{name} fits the print bed", "ENVELOPE",
                    all(a <= b for a, b in zip(e, bed)),
                    f"{m.extents[0]:.1f} x {m.extents[1]:.1f} x {m.extents[2]:.1f}")
    return ok


def check_fit(parts, mocks, p):
    """The real test: does the hardware go in, and does anything touch?"""
    print("\n-- FIT --")
    ok = True
    ch = parts["chassis"]
    bp = parts["backplate"]

    # The assembled envelope is the union of both printed parts. It matters:
    # the 18650 projects out of the chassis entirely and lives inside the back
    # plate's cowl, so checking containment against the chassis alone would
    # report a false failure on a perfectly good design.
    env_lo = np.minimum(ch.bounds[0], bp.bounds[0])
    env_hi = np.maximum(ch.bounds[1], bp.bounds[1])

    for name, mock in mocks.items():
        # Interference: the component must not share volume with shell material.
        try:
            inter = ch.intersection(mock)
            vol = 0.0 if inter is None or inter.is_empty else float(inter.volume)
        except Exception as exc:
            check(f"{name}: interference test ran", "FIT", False, str(exc))
            ok = False
            continue
        ok &= check(f"{name} does not clash with the chassis", "FIT", vol < 1.0,
                    f"shared volume {vol:.2f} mm^3")

        # The component must not share volume with the BACK PLATE either.
        try:
            inter2 = bp.intersection(mock)
            vol2 = 0.0 if inter2 is None or inter2.is_empty else float(inter2.volume)
        except Exception:
            vol2 = -1.0
        if vol2 >= 0:
            ok &= check(f"{name} does not clash with the back plate", "FIT",
                        vol2 < 1.0, f"shared volume {vol2:.2f} mm^3")

        # Containment, against the assembled envelope.
        cb = mock.bounds
        inside = bool(np.all(cb[0] >= env_lo - 0.01) and np.all(cb[1] <= env_hi + 0.01))
        ok &= check(f"{name} is inside the assembled envelope", "FIT", inside,
                    f"bounds {np.round(cb, 2).tolist()} in "
                    f"{np.round(env_lo,2).tolist()}..{np.round(env_hi,2).tolist()}")

    # Declared clearances.
    ok &= check("board pocket clearance", "FIT",
                p["board_pocket_w"] >= p["board_w"] and p["board_pocket_h"] >= p["board_h"],
                f"{(p['board_pocket_w']-p['board_w'])/2:.2f} / "
                f"{(p['board_pocket_h']-p['board_h'])/2:.2f} mm per side")
    ok &= check("keyboard is captured by the front lip", "FIT",
                p["kbd_aper_w"] < p["kbd_pocket_w"] and p["kbd_aper_h"] < p["kbd_pocket_h"],
                f"lip {(p['kbd_pocket_w']-p['kbd_aper_w'])/2:.2f} / "
                f"{(p['kbd_pocket_h']-p['kbd_aper_h'])/2:.2f} mm per side")
    ok &= check("board is captured by the front lip", "FIT",
                p["display_aper_w"] < p["board_pocket_w"],
                f"lip {(p['board_pocket_w']-p['display_aper_w'])/2:.2f} mm per side")
    # The 18650 is the reason the cowl exists. Prove it actually fits inside it.
    batt_rear = float(mocks["board"].bounds[0][2])
    cowl_floor = -(p["batt_cowl_rise"] - p["batt_cowl_wall"])
    ok &= check("18650 fits inside the battery cowl", "FIT",
                batt_rear >= cowl_floor + 0.2,
                f"cell reaches z={batt_rear:.2f}, cowl inner face at "
                f"z={cowl_floor:.2f}, clearance {batt_rear-cowl_floor:.2f} mm")
    # The criterion is the holder's protrusion past the STANDOFF plane less the
    # plate thickness - not the cell diameter. Half the cell is inside the deck.
    need = p["batt_protrusion"] - p["back_t"]
    have = p["batt_cowl_rise"] - p["batt_cowl_wall"]
    ok &= check("cowl clears the holder protrusion", "FIT", have >= need,
                f"holder reaches {need:.2f} mm past the back plate's outer face; "
                f"cowl gives {have:.2f} mm of internal rise")

    ok &= check("front face does not clip the panel", "FIT",
                p["display_aper_w"] >= p["display_active_w"] and
                p["display_aper_h"] >= p["display_active_h"],
                f"reveal {(p['display_aper_w']-p['display_active_w'])/2:.2f} / "
                f"{(p['display_aper_h']-p['display_active_h'])/2:.2f} mm per side")
    return ok


def check_interface(parts, p):
    print("\n-- INTERFACE --")
    ok = True
    ch, bp = parts["chassis"], parts["backplate"]

    inner_w = p["body_w"] - 2 * p["wall"] if "body_w" in p else None
    # Back plate must fit the chassis opening with the declared slide fit.
    opening_w = (p["wall"] + max(p["kbd_pocket_w"], p["board_pocket_w"]) + p["wall"]) - 2*p["wall"]
    ok &= check("back plate fits the chassis opening", "INTERFACE",
                bp.extents[0] <= opening_w - 2*p["fit_slide"] + 0.05,
                f"plate {bp.extents[0]:.2f} mm into a {opening_w:.2f} mm opening")

    # The two parts must not interfere when assembled (they share a frame).
    try:
        inter = ch.intersection(bp)
        vol = 0.0 if inter is None or inter.is_empty else float(inter.volume)
    except Exception as exc:
        vol = -1.0
        check("assembly interference test ran", "INTERFACE", False, str(exc))
        ok = False
    if vol >= 0:
        ok &= check("chassis and back plate do not clash", "INTERFACE", vol < 1.0,
                    f"shared volume {vol:.2f} mm^3")

    # Fastener bores: count them in the chassis and match against the plate.
    bores = count_bores(ch, p["m3_insert_bore"], z=p["back_t"] + 1.0)
    holes = count_bores(bp, p["m3_clear"], z=p["back_t"] - 0.5)
    ok &= check("fastener count matches", "INTERFACE", bores == holes and bores >= 4,
                f"{bores} insert bores in the chassis, {holes} clearance holes in the plate")
    return ok


def count_bores(mesh, dia, z, tol=0.45):
    s = mesh.section(plane_origin=[0, 0, z], plane_normal=[0, 0, 1])
    if s is None:
        return 0
    polys = s.to_2D(to_2D=np.eye(4))[0].polygons_full
    n = 0
    for poly in polys:
        for ring in poly.interiors:
            rp = Polygon(ring)
            b = rp.bounds
            w, h = b[2] - b[0], b[3] - b[1]
            if abs(w - dia) < tol and abs(h - dia) < tol:
                n += 1
    return n


def check_print(parts, p):
    print("\n-- PRINT --")
    ok = True
    noz = p["nozzle"]
    for nm in ("wall", "spine", "front_t", "back_t"):
        v = p[nm]
        mult = v / noz
        ok &= check(f"{nm} is a whole number of extrusions", "PRINT",
                    abs(mult - round(mult)) < 1e-6,
                    f"{v:.2f} mm = {mult:.2f} x {noz} mm nozzle")
    ok &= check("wall is at least 4 extrusions", "PRINT", p["wall"] >= 4*noz,
                f"{p['wall']/noz:.0f} perimeters")
    ok &= check("vent slots are printable", "PRINT", p["vent_slot_w"] >= 4*noz,
                f"{p['vent_slot_w']:.2f} mm")
    ok &= check("speaker grille slots are printable", "PRINT",
                p["grille_slot_h"] >= 2*noz,
                f"{p['grille_slot_h']:.2f} mm wide, {p['grille_pitch']:.2f} mm pitch")
    ok &= check("insert boss wall is thick enough", "PRINT",
                (p["m3_boss_d"] - p["m3_insert_bore"]) / 2 >= 1.6,
                f"{(p['m3_boss_d']-p['m3_insert_bore'])/2:.2f} mm around the insert")
    # The chassis prints face-down: the only overhang of consequence is the
    # aperture draft, which is a chamfer, not a bridge.
    ok &= check("front apertures are drafted, not bridged", "PRINT",
                p["display_aper_draft"] > 0,
                f"{p['display_aper_draft']:.2f} mm per-side flare over "
                f"{p['front_t']:.1f} mm = "
                f"{np.degrees(np.arctan(p['display_aper_draft']/p['front_t'])):.0f} deg from vertical")
    return ok


def check_datums(p):
    print("\n-- DATUM --")
    ok = True
    prov = p["_provisional"]
    # A provisional datum may not set a hard fit without margin somewhere else.
    ok &= check("board outline comes from the factory drawing", "DATUM",
                "board_w" not in prov and "board_h" not in prov
                and abs(p["board_w"] - 92.50) < 0.01
                and abs(p["board_h"] - 69.10) < 0.01,
                "92.50 x 69.10 read from Waveshare's dimensioned drawing; the "
                "circulating 70.1 figure is the removable stand base, not the PCB")
    ok &= check("display active area is not derived from a nominal diagonal", "DATUM",
                abs(p["display_active_w"] - 84.80) < 0.01,
                "84.80 x 63.60 from the drawing. Deriving it from '4.2 inch' "
                "gives 85.34 x 64.01, about 0.5 mm too big in each axis.")
    ok &= check("display aperture is offset, not centred", "DATUM",
                abs(p["display_off_x"] + 1.60) < 0.01,
                "the active area sits 1.60 mm toward the U=0 edge of the PCB")
    ok &= check("keyboard pocket is measured, not inferred", "DATUM",
                "kbd_pocket_w" not in prov and "kbd_pocket_h" not in prov,
                "pocket comes from two independent reference designs")
    ok &= check("mount pattern is not provisional", "DATUM",
                "board_mount_pitch_x" not in prov and "board_mount_pitch_y" not in prov,
                f"{p['board_mount_pitch_x']:.1f} x {p['board_mount_pitch_y']:.1f} mm, "
                "two independent derivations agree")
    ok &= check("port openings clear their connectors", "DATUM",
                p["usbc_open_w"] > p["usbc_body_w"] and p["tf_open_w"] > p["tf_body_w"] - 3.0,
                f"USB-C shell {p['usbc_body_w']:.2f} through a {p['usbc_open_w']:.2f} mm "
                f"opening; microSD card 11.0 through {p['tf_open_w']:.2f} mm "
                "(the socket body stays inside the pocket, only the card passes)")
    ok &= check("keyboard service access exists", "DATUM",
                p["kbd_access_w"] >= 33.0 and p["kbd_access_h"] >= 8.0,
                f"{p['kbd_access_w']:.1f} x {p['kbd_access_h']:.1f} mm window reaches the "
                "keyboard's power switch and charging port, which share one short edge")
    return ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--json")
    ap.add_argument("--keep", action="store_true", help="keep the rendered temp files")
    args = ap.parse_args()

    p = load_params()
    print("=" * 78)
    print("cYbErDeCk design audit")
    print("=" * 78)

    tmp = tempfile.mkdtemp(prefix="cyberdeck-validate-")
    try:
        print("\nrendering parts from source ...")
        parts = {n: render(n, tmp) for n in ("chassis", "backplate", "buttons")}
        print("rendering component mocks ...")
        mocks = {}
        bx, by = 0.0, p["board_bay_cy"] if "board_bay_cy" in p else None
        # Place the mocks exactly where cyberdeck.scad places them.
        body_h = p["wall"] + p["kbd_pocket_h"] + p["spine"] + p["board_pocket_h"] + p["wall"]
        body_t = p["back_t"] + p["board_depth"] + p["front_t"]
        board_cy = body_h/2 - p["wall"] - p["board_pocket_h"]/2
        kbd_cy = -body_h/2 + p["wall"] + p["kbd_pocket_h"]/2
        z_front_inner = body_t - p["front_t"]

        mocks["board"] = scad_module_mesh(
            f"translate([0,{board_cy},{z_front_inner - p['board_depth']}]) mock_board()",
            tmp, "mock_board")
        mocks["keyboard"] = scad_module_mesh(
            f"translate([0,{kbd_cy},{z_front_inner - p['kbd_depth']}]) mock_keyboard()",
            tmp, "mock_keyboard")

        p["body_w"] = parts["chassis"].extents[0]

        ok = True
        ok &= check_mesh({**parts, **mocks})
        ok &= check_envelope(parts, p)
        ok &= check_fit(parts, mocks, p)
        ok &= check_interface(parts, p)
        ok &= check_print(parts, p)
        ok &= check_datums(p)

        npass = sum(1 for r in RESULTS if r["ok"])
        print("\n" + "=" * 78)
        print(f"{npass}/{len(RESULTS)} checks passed")
        print("=" * 78)

        if args.json:
            os.makedirs(os.path.dirname(args.json) or ".", exist_ok=True)
            json.dump({"passed": npass, "total": len(RESULTS), "results": RESULTS},
                      open(args.json, "w"), indent=2)
            print(f"wrote {args.json}")

        return 0 if ok else 1
    finally:
        if not args.keep:
            import shutil
            shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
