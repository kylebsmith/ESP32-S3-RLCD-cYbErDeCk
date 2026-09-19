#!/usr/bin/env python3
"""
audit_reference.py - compare this enclosure's component-facing geometry against
the reference design it was measured from, feature by feature.

The question this answers is narrow and specific: **where this design touches the
hardware, does it agree with a design that is known to work?**

It is not a check that the two enclosures are alike - they are deliberately not.
It is a check that every surface which locates, retains or gives access to a
component sits where the reference puts it, or differs for a reason that is
stated here. A silent disagreement on a component-facing dimension is the one
class of error that reaches a printer and wastes a board.

Three verdicts:

    MATCH      within tolerance of the reference
    INTENDED   differs, and the difference is a recorded design decision
    REVIEW     differs with no recorded reason - investigate before printing

Exit status is non-zero if anything lands in REVIEW.

    python3 tools/audit_reference.py --reference /path/to/solar_term
"""

from __future__ import annotations

import argparse
import os
import sys

try:
    import numpy as np
    import trimesh
    from shapely.geometry import Polygon, LineString
    from shapely.ops import unary_union
except ImportError as exc:  # pragma: no cover
    sys.exit(f"missing dependency: {exc}\ninstall with: pip install -r tools/requirements.txt")

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import params  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

OK, INT, REV = "MATCH", "INTENDED", "REVIEW"
_C = {OK: "\033[32m", INT: "\033[36m", REV: "\033[31m"}
_R = "\033[0m"

ROWS = []


def row(group, feature, mine, ref, verdict, note=""):
    ROWS.append((group, feature, mine, ref, verdict, note))


# --- reference extraction --------------------------------------------------

_T_Y = np.array([[1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, 0], [0, 0, 0, 1]], float)


def sect_y(mesh, y):
    s = mesh.section(plane_origin=[0, y, 0], plane_normal=[0, 1, 0])
    return [] if s is None else list(s.to_2D(to_2D=_T_Y)[0].polygons_full)


def sect_z(mesh, z):
    s = mesh.section(plane_origin=[0, 0, z], plane_normal=[0, 0, 1])
    return [] if s is None else list(s.to_2D(to_2D=np.eye(4))[0].polygons_full)


def rings(polys, amin=0.0, amax=1e9):
    out = []
    for g in polys:
        for r in g.interiors:
            rp = Polygon(r)
            if amin <= rp.area <= amax:
                out.append(rp)
    return out


def intervals(polys, kind, at, lo, hi):
    if not polys:
        return []
    solid = unary_union(polys)
    ln = LineString([(lo, at), (hi, at)]) if kind == "h" else LineString([(at, lo), (at, hi)])
    hit = ln.intersection(solid)
    if hit.is_empty:
        return []
    parts = list(hit.geoms) if hit.geom_type.startswith("Multi") else [hit]
    i = 0 if kind == "h" else 1
    return sorted((round(float(np.asarray(p.coords)[:, i].min()), 3),
                   round(float(np.asarray(p.coords)[:, i].max()), 3))
                  for p in parts if p.geom_type == "LineString")


def measure_reference(ref):
    """Recover the reference's component-facing geometry.

    Reference frame: X is device height, Y is thickness, Z is device width.
    This file's frame: X is device width, Y is device height, Z is thickness.
    So reference (X, Z) maps to this design's (Y, X) - the audit compares
    like with like by naming the axes rather than by position.
    """
    cb = trimesh.load(os.path.join(ref, "stl/ata/Caseback.stl"), force="mesh")
    bz = trimesh.load(os.path.join(ref, "stl/ata/Bezel.stl"), force="mesh")
    r = {}

    # board pocket, at mid-depth
    big = max(rings(sect_y(cb, -9.0)), key=lambda q: q.area)
    b = big.bounds
    r["board_pocket_h"] = round(b[2] - b[0], 3)     # ref X -> our Y
    r["board_pocket_w"] = round(b[3] - b[1], 3)     # ref Z -> our X
    r["board_pocket_cx"] = round(big.centroid.x, 3)
    r["board_pocket_cy"] = round(big.centroid.y, 3)

    # M2.5 board mount pattern
    holes = [(round(h.centroid.x, 3), round(h.centroid.y, 3))
             for h in rings(sect_y(cb, -11.323))
             if 2.4 < h.bounds[2] - h.bounds[0] < 3.0 and 2.4 < h.bounds[3] - h.bounds[1] < 3.0]
    if len(holes) == 4:
        xs, ys = sorted({h[0] for h in holes}), sorted({h[1] for h in holes})
        r["mount_pitch_y"] = round(xs[-1] - xs[0], 3)
        r["mount_pitch_x"] = round(ys[-1] - ys[0], 3)

    # board pocket depth and back wall, by ray cast clear of every cutout
    for nm, (x, z) in {"back_wall": (25.0, -30.0)}.items():
        locs, _, _ = cb.ray.intersects_location(np.array([[x, -40.0, z]]), np.array([[0, 1, 0]]))
        v = sorted(float(q[1]) for q in locs)
        r[nm] = round(v[1] - v[0], 3) if len(v) >= 2 else None
    r["board_depth"] = 13.0   # pocket floor -10.0 to rim +3.0

    # display aperture, bezel inner face
    ap = max(rings(sect_y(bz, bz.bounds[0][1] + 0.05)), key=lambda q: q.area)
    a = ap.bounds
    r["display_aper_h"] = round(a[2] - a[0], 3)
    r["display_aper_w"] = round(a[3] - a[1], 3)

    # keyboard tray, by scan line - the tray is open to the exterior
    h = intervals(sect_y(cb, 0.0), "h", 0.0, -106.0, 49.0)
    v = intervals(sect_y(cb, 0.0), "v", -95.0, -58.0, 62.0)
    if len(h) >= 2 and len(v) >= 2:
        r["kbd_pocket_h"] = round(h[1][0] - h[0][1], 3)
        r["kbd_pocket_w"] = round(v[1][0] - v[0][1], 3)
        r["wall"] = round(h[0][1] - h[0][0], 3)
    locs, _, _ = cb.ray.intersects_location(np.array([[-70.0, -40.0, 0.0]]), np.array([[0, 1, 0]]))
    vv = sorted(float(q[1]) for q in locs)
    r["kbd_depth"] = round(3.0 - vv[1], 3) if len(vv) >= 2 else None

    # back-face features
    back = rings(sect_y(cb, -12.95))
    for rg in back:
        bb = rg.bounds
        w, hh = bb[2] - bb[0], bb[3] - bb[1]
        if w > 15 and hh > 50:
            r["batt_bay_h"], r["batt_bay_w"] = round(w, 3), round(hh, 3)
            r["batt_off"] = round(rg.centroid.x, 3)
        if 5.0 < w < 6.2 and 20 < hh < 23:
            r["header_win_h"], r["header_win_w"] = round(w, 3), round(hh, 3)
    grille = sorted([rg for rg in back
                     if 1.0 < rg.bounds[2] - rg.bounds[0] < 1.8 and rg.bounds[3] - rg.bounds[1] > 10],
                    key=lambda q: q.centroid.x)
    if len(grille) >= 2:
        r["grille_count"] = len(grille)
        r["grille_slot_h"] = round(grille[0].bounds[2] - grille[0].bounds[0], 3)
        r["grille_slot_w"] = round(grille[0].bounds[3] - grille[0].bounds[1], 3)
        r["grille_pitch"] = round(grille[1].centroid.x - grille[0].centroid.x, 3)
        r["grille_field"] = round(grille[-1].bounds[2] - grille[0].bounds[0], 3)
        r["grille_off"] = round((grille[0].centroid.x + grille[-1].centroid.x) / 2, 3)

    # top-edge apertures: three buttons, two microphones
    sx = cb.section(plane_origin=[36.0, 0, 0], plane_normal=[1, 0, 0])
    if sx is not None:
        T = np.array([[0, 1, 0, 0], [0, 0, 1, 0], [1, 0, 0, 0], [0, 0, 0, 1]], float)
        pr = list(sx.to_2D(to_2D=T)[0].polygons_full)
        tops = sorted([Polygon(q) for g in pr for q in g.interiors], key=lambda q: q.centroid.y)
        btn = [q for q in tops if 4.0 < q.bounds[3] - q.bounds[1] < 6.0
               and 3.5 < q.bounds[2] - q.bounds[0] < 5.0]
        mic = [q for q in tops if 4.0 < q.bounds[3] - q.bounds[1] < 6.0
               and 2.0 < q.bounds[2] - q.bounds[0] < 3.2]
        if len(btn) >= 2:
            c = sorted(q.centroid.y for q in btn)
            r["button_pitch"] = round(c[1] - c[0], 3)
            r["button_aper_w"] = round(btn[0].bounds[3] - btn[0].bounds[1], 3)
            r["button_aper_h"] = round(btn[0].bounds[2] - btn[0].bounds[0], 3)
        if len(mic) == 2:
            c = sorted(q.centroid.y for q in mic)
            r["mic_span"] = round(c[1] - c[0], 3)
            r["mic_aper_w"] = round(mic[0].bounds[3] - mic[0].bounds[1], 3)
            r["mic_aper_h"] = round(mic[0].bounds[2] - mic[0].bounds[0], 3)
    return r


# --- comparison ------------------------------------------------------------

def audit(p, r):
    def cmp(group, feature, mine, ref, tol=0.06, reason=None, fmt="{:.3f}"):
        if ref is None:
            row(group, feature, fmt.format(mine), "-", REV, "reference value not recovered")
            return
        d = mine - ref
        if abs(d) <= tol:
            row(group, feature, fmt.format(mine), fmt.format(ref), OK, f"{d:+.3f}")
        elif reason:
            row(group, feature, fmt.format(mine), fmt.format(ref), INT, f"{d:+.3f}  {reason}")
        else:
            row(group, feature, fmt.format(mine), fmt.format(ref), REV, f"{d:+.3f}")

    g = "BOARD — location"
    cmp(g, "mount pattern, long axis", p["board_mount_pitch_x"], r.get("mount_pitch_x"))
    cmp(g, "mount pattern, short axis", p["board_mount_pitch_y"], r.get("mount_pitch_y"))
    cmp(g, "pocket width", p["board_pocket_w"], r.get("board_pocket_w"),
        reason="pocket cut to the bare PCB (92.50) + 0.50/side; the reference "
               "carries 1.00/side")
    cmp(g, "pocket height", p["board_pocket_h"], r.get("board_pocket_h"),
        reason="same: 69.10 + 0.50/side")
    cmp(g, "pocket depth", p["board_depth"], r.get("board_depth"),
        reason="Waveshare stand base discarded, so the stack is 10.75 not 13.50")

    g = "BOARD — display"
    cmp(g, "aperture width", p["display_aper_w"], r.get("display_aper_w"))
    cmp(g, "aperture height", p["display_aper_h"], r.get("display_aper_h"))

    g = "BOARD — back-face features"
    cmp(g, "18650 bay width", p["batt_bay_w"], r.get("batt_bay_w"),
        reason="cut to the holder body 77.80 from vendor CAD; the reference "
               "cuts 79.00")
    cmp(g, "18650 bay height", p["batt_bay_h"], r.get("batt_bay_h"))
    cmp(g, "18650 offset from board centre", p["batt_off_y"],
        (r["batt_off"] - r["board_pocket_cx"]) if "batt_off" in r else None, tol=0.25)
    cmp(g, "grille slot width", p["grille_slot_h"], r.get("grille_slot_h"),
        reason="restyled to a finer 5-slot field inside the same 10.45 envelope")
    cmp(g, "grille pitch", p["grille_pitch"], r.get("grille_pitch"), reason="as above")
    cmp(g, "grille field height", (p["grille_count"] - 1) * p["grille_pitch"]
        + p["grille_slot_h"], r.get("grille_field"), tol=0.12)
    cmp(g, "grille offset from board centre", p["grille_off_y"],
        (r["grille_off"] - r["board_pocket_cx"]) if "grille_off" in r else None, tol=0.25)
    cmp(g, "expansion window height", p["expansion_win_h"], r.get("header_win_h"),
        reason="sized to the 6.603 header body; the reference copies Waveshare's "
               "narrower 5.60 pin window")
    cmp(g, "expansion window width", p["expansion_win_w"], r.get("header_win_w"),
        reason="as above")

    g = "BOARD — top edge"
    cmp(g, "button pitch", p["button_pitch"], r.get("button_pitch"))
    cmp(g, "button aperture width", p["button_aper_w"], r.get("button_aper_w"))
    cmp(g, "button aperture height", p["button_aper_h"], r.get("button_aper_h"))
    cmp(g, "microphone span", 2 * p["mic_offset_x"], r.get("mic_span"))
    cmp(g, "microphone aperture width", p["mic_aper_w"], r.get("mic_aper_w"))
    cmp(g, "microphone aperture height", p["mic_aper_h"], r.get("mic_aper_h"))

    g = "KEYBOARD"
    cmp(g, "pocket width", p["kbd_pocket_w"], r.get("kbd_pocket_w"),
        reason="reference is a -0.02 press fit at exactly 4.3 in; this sits "
               "between it and the looser proof-of-concept")
    cmp(g, "pocket height", p["kbd_pocket_h"], r.get("kbd_pocket_h"),
        reason="as above")
    cmp(g, "pocket depth", p["kbd_depth"], r.get("kbd_depth"),
        reason="reference leaves 1.24 of vertical float; this leaves 0.84")

    g = "SHELL"
    cmp(g, "wall thickness", p["wall"], r.get("wall"),
        reason="8 extrusions rather than 2.9, so every wall is solid perimeter")
    return ROWS


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--reference", default="/home/user/nilseuropa/solar_term")
    args = ap.parse_args()
    if not os.path.isdir(args.reference):
        sys.exit(f"reference tree not found: {args.reference}\n"
                 f"  git clone --depth 1 https://github.com/nilseuropa/solar_term {args.reference}")

    p = params.load_with_defaults()
    r = measure_reference(args.reference)
    audit(p, r)

    print("=" * 108)
    print("COMPONENT-FACING GEOMETRY  —  this design against the reference it was measured from")
    print("=" * 108)
    cur = None
    for group, feat, mine, ref, verdict, note in ROWS:
        if group != cur:
            print(f"\n{group}")
            cur = group
        col = _C[verdict]
        print(f"  {col}{verdict:<9}{_R} {feat:<34} {mine:>10}   ref {ref:>10}   {note}")

    n = {v: sum(1 for x in ROWS if x[4] == v) for v in (OK, INT, REV)}
    print("\n" + "=" * 108)
    print(f"{n[OK]} match   {n[INT]} intended difference   {n[REV]} to review")
    print("=" * 108)
    if n[REV]:
        print("\nAnything in REVIEW is a component-facing dimension that differs from a")
        print("design known to work, with no recorded reason. Resolve before printing.")
    return 1 if n[REV] else 0


if __name__ == "__main__":
    sys.exit(main())
