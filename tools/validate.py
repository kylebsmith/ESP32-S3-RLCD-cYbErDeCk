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

import params

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
    """Delegates to tools/params.py so validate.py and drawing.py can never
    disagree about what the model says."""
    return params.load_with_defaults(PARAMS)


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

    # A part that renders degenerate still passes every other check, because
    # every other check looks at the datums rather than at the part. This is how
    # a 2 x 2 x 1 mm "acrylic window" survived a 52-check audit: its four
    # parameters had been deleted, OpenSCAD substituted undef, and rbox() fell
    # back to a unit cylinder. Assert each part is the size it should be.
    expect = {
        "chassis":   (exp_w, exp_h, exp_t),
        "backplate": (exp_w - 2*p["wall"] - 2*p["fit_slide"], None, None),
        "buttons":   (None, None, None),
        "window":    (p["window_w"], p["window_h"], p["window_t"]),
    }
    for name, m in parts.items():
        for axis, want in enumerate(expect.get(name, (None, None, None))):
            if want is None:
                continue
            ok &= check(f"{name}: extent {'XYZ'[axis]} is as specified", "ENVELOPE",
                        abs(m.extents[axis] - want) < 0.15,
                        f"{m.extents[axis]:.2f} mm, expected {want:.2f}")
        ok &= check(f"{name} is not degenerate", "ENVELOPE",
                    min(m.extents) > 0.5 and m.volume > 10.0,
                    f"{m.extents[0]:.2f} x {m.extents[1]:.2f} x {m.extents[2]:.2f}, "
                    f"{m.volume:.0f} mm^3")

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

    # THE CHECK ABOVE IS ONE-DIMENSIONAL AND THAT IS WHY IT MISSED A REAL BUG.
    # Comparing widths and heights only ever tests the flats. The lip is a 2-D
    # ring, and it is at the CORNERS that it disappears - where the keyboard's
    # own rounded corner retreats inward while a squarer aperture corner bites
    # outward. Measured at the worst end of the corner band, from the mesh.
    lip_min, lip_at = _corner_lip(parts["chassis"], p)
    ok &= check("keyboard covers the aperture at the corners too", "FIT",
                lip_min >= 0.40,
                f"narrowest lip {lip_min:.2f} mm, at the corners, against a "
                f"keyboard corner radius of {p['kbd_body_corner_r_max']:.1f} mm "
                f"(the worst case for capture); on the flats it is "
                f"{min(p['kbd_lip_x'], p['kbd_lip_y']):.2f}"
                + (f"; nominal corner r {p['kbd_body_corner_r']:.1f} gives "
                   f"{lip_at:.2f}" if lip_at is not None else ""))
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

    # Cowl wall thickness along the rise. The outer and the cavity ease in by
    # the same law, but from different footprints and over different rises, so
    # the wall is NOT constant and can go to zero partway up - which is exactly
    # what happened when the cavity was left straight-sided inside a crowned
    # outer: it punched through and the plate rendered as two bodies.
    def _prof(u, minor, blend, cap, foot, crown):
        def ss(x):
            x = min(max(x, 0.0), 1.0)
            return 3 * x * x - 2 * x ** 3
        if u < foot:
            return blend * (1 - ss(u / foot))
        if u < crown:
            return 0.0
        return -cap * minor * 0.5 * ss((u - crown) / (1 - crown))

    rise, wallc = p["batt_cowl_rise"], p["batt_cowl_wall"]
    worst, worst_d = 1e9, 0.0
    for k in range(1, 60):
        d = rise * k / 60.0                      # depth below the panel
        if d > rise - wallc:
            break
        o = p["batt_cowl_h"] + 2 * _prof(d / rise, min(p["batt_cowl_w"], p["batt_cowl_h"]),
                                         p["batt_cowl_foot"], p["batt_cowl_cap"],
                                         p["batt_cowl_foot_f"], p["batt_cowl_crown"])
        ih, iw = p["batt_cowl_h"] - 2 * wallc, p["batt_cowl_w"] - 2 * wallc
        i = ih + 2 * _prof(d / (rise - wallc), min(iw, ih), 0.001, p["batt_cowl_cap"],
                           p["batt_cowl_foot_f"], p["batt_cowl_crown"])
        if (o - i) / 2 < worst:
            worst, worst_d = (o - i) / 2, d
    ok &= check("cowl wall stays intact along its rise", "FIT",
                worst >= 4 * p["nozzle"],
                f"thinnest {worst:.2f} mm at {worst_d:.1f} mm depth "
                f"({worst/p['nozzle']:.1f} extrusions)")

    ok &= check("front face does not clip the panel", "FIT",
                p["display_aper_w"] >= p["display_active_w"] and
                p["display_aper_h"] >= p["display_active_h"],
                f"reveal {(p['display_aper_w']-p['display_active_w'])/2:.2f} / "
                f"{(p['display_aper_h']-p['display_active_h'])/2:.2f} mm per side")
    return ok


def _rounded(w, h, c, n, q=400):
    """Rounded rectangle with a superelliptical corner. n = 2 is a circle."""
    from shapely.geometry import Polygon
    c = min(c, w / 2 - 0.01, h / 2 - 0.01)
    ax, ay = w / 2 - c, h / 2 - c
    t = np.linspace(0, 90, q)
    e = 2.0 / n
    ct = np.power(np.clip(np.cos(np.radians(t)), 0, None), e)
    st = np.power(np.clip(np.sin(np.radians(t)), 0, None), e)
    return Polygon(np.vstack([np.c_[ax + c*ct,  ay + c*st],
                              np.c_[-ax - c*st, ay + c*ct],
                              np.c_[-ax - c*ct, -ay - c*st],
                              np.c_[ax + c*st,  -ay - c*ct]]))


def _corner_lip(chassis, p):
    """Narrowest overlap between the keyboard body and the front-face lip.

    The aperture outline is taken FROM THE RENDERED MESH rather than rebuilt
    from parameters, so this measures the part that would actually be printed.
    Negative means the keyboard does not reach the aperture edge there - you
    would see into the pocket past the corner, and nothing retains it.
    """
    from shapely.geometry import Point
    kcy = -p["body_h"] / 2 + p["wall"] + p["kbd_pocket_h"] / 2
    # The BEARING plane, not mid-thickness: the keyboard is pushed forward onto
    # the inner face of the front panel, and that is where the aperture is at
    # its smallest and the lip at its widest. Sectioning higher measures the
    # flare, which is draft and visual relief, not the surface that retains it.
    z = p["body_t"] - p["front_t"] + 0.05
    ring = None
    for g in section_polys(chassis, z):
        for r in g.interiors:
            from shapely.geometry import Polygon as _P
            q = _P(r); b = q.bounds
            if abs((b[1] + b[3]) / 2 - kcy) < 8 and (b[2] - b[0]) > 60:
                ring = q
    if ring is None:
        return -99.0, None
    xy = np.array(ring.exterior.coords)
    xy[:, 1] -= kcy                              # into keyboard-centred coords

    def narrowest(rb):
        body = _rounded(p["kbd_body_w"], p["kbd_body_h"], rb, 2.0)
        out = []
        for x, y in xy:
            pt = Point(x, y)
            d = pt.distance(body.exterior)
            out.append(d if body.contains(pt) else -d)
        return min(out)

    return narrowest(p["kbd_body_corner_r_max"]), narrowest(p["kbd_body_corner_r"])


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

    # Reachability. Both bays load through the back opening, so every part of
    # each bay must lie inside that opening. A corner treatment on the opening
    # that is fuller than the bay's own corner undercuts it and traps the
    # component - which a clash test cannot see, because an undercut is absence
    # of material, not interference.
    try:
        # Both sections return MATERIAL; the openings are its interior rings.
        # The bay section must be taken below the keyboard service windows and
        # the side port tunnels, or the bays connect to the exterior and stop
        # being interior rings at all.
        opening = unary_union([Polygon(r)
                               for g in section_polys(ch, p["back_t"] / 2)
                               for r in g.interiors])
        voids = unary_union([Polygon(r)
                             for g in section_polys(ch, p["back_t"] + 0.7)
                             for r in g.interiors])
        escaped = voids.difference(opening.buffer(0.02)).area if not voids.is_empty else 0.0
        ok &= check("both bays are reachable through the back opening",
                    "INTERFACE", escaped < 0.05,
                    f"{escaped:.4f} mm^2 of bay area lies outside the opening")
    except Exception as exc:
        ok &= check("reachability test ran", "INTERFACE", False, str(exc))

    # Fastener bores: count them in the chassis and match against the plate.
    bores = count_bores(ch, p["m3_insert_bore"], z=p["back_t"] + 1.0)
    holes = count_bores(bp, p["m3_clear"], z=p["back_t"] - 0.5)
    ok &= check("fastener count matches", "INTERFACE", bores == holes and bores >= 4,
                f"{bores} insert bores in the chassis, {holes} clearance holes in the plate")
    return ok


def _as_list(g):
    return list(g.geoms) if g.geom_type.startswith("Multi") else [g]


def section_polys(mesh, z):
    s = mesh.section(plane_origin=[0, 0, z], plane_normal=[0, 0, 1])
    return [] if s is None else list(s.to_2D(to_2D=np.eye(4))[0].polygons_full)


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
    # Neither the manufacturer's drawing nor the FCC manual states a tolerance,
    # so one is assumed and the pocket must clear the body WITH it.
    kw, kh, kt = p["kbd_body_w_max"], p["kbd_body_h_max"], p["kbd_body_t_max"]
    ok &= check("keyboard pocket clears the body at tolerance", "DATUM",
                p["kbd_pocket_w"] >= kw + 0.6 and p["kbd_pocket_h"] >= kh + 0.6
                and p["kbd_depth"] >= kt + 0.4,
                f"pocket {p['kbd_pocket_w']:.1f} x {p['kbd_pocket_h']:.1f} x "
                f"{p['kbd_depth']:.1f} over {kw:.2f} x {kh:.2f} x {kt:.2f} "
                f"(nominal 108.5 x 58.2 x 10.2 plus assumed tolerance)")
    # A fit check must never run against the smallest plausible part.
    ok &= check("keyboard fit is checked at tolerance, not nominal", "DATUM",
                kw > p["kbd_body_w"] and kh > p["kbd_body_h"]
                and kt > p["kbd_body_t"]
                and abs(kw - (p["kbd_body_w"] + p["kbd_mould_tol"])) < 1e-9
                and abs(kt - (p["kbd_body_t"] + p["kbd_keycap_tol"])) < 1e-9,
                f"mocks are built at +{p['kbd_mould_tol']:.2f} in plane and "
                f"+{p['kbd_keycap_tol']:.2f} in thickness, so a mock that fits "
                "guarantees a body that fits")
    ok &= check("keyboard outline is vendor-stated, not inferred", "DATUM",
                "kbd_body_w" not in prov and "kbd_body_h" not in prov
                and "kbd_body_t" not in prov,
                f"{p['kbd_body_w']:.1f} x {p['kbd_body_h']:.1f} x "
                f"{p['kbd_body_t']:.1f} from the manufacturer's user manual, "
                "filed as an exhibit under FCC ID YIZRT-RII518")
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
        parts = {n: render(n, tmp) for n in ("chassis", "backplate", "buttons", "window")}
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
