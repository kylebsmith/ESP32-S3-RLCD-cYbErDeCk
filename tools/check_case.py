#!/usr/bin/env python3
"""
check_case.py - the carry case, measured against the deck it has to swallow.

The case is an accessory, not part of the enclosure, so it is not in
validate.py's build gate. It still gets checked, and it gets checked the same
way everything else here does: by measuring the rendered mesh rather than by
trusting the parameters that produced it.

    python3 tools/check_case.py
"""
from __future__ import annotations
import os, subprocess, sys, tempfile
import numpy as np, trimesh

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from params import load_with_defaults                      # noqa: E402

FAILED = []


def check(name, ok, detail):
    print(f"  [{'PASS' if ok else 'FAIL'}] {name}  --  {detail}")
    if not ok:
        FAILED.append(name)
    return ok


def render(tmp, scad, part, extra=()):
    out = os.path.join(tmp, f"{part}.stl")
    run = ["xvfb-run", "-a"] if subprocess.run(["which", "xvfb-run"],
           capture_output=True).returncode == 0 else []
    cmd = run + ["openscad", "-D", f'part="{part}"', *extra, "-o", out,
                 os.path.join(ROOT, "cad", scad)]
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=1800)
    if not os.path.exists(out):
        raise RuntimeError(f"openscad produced no {part}:\n{r.stderr[-900:]}")
    return trimesh.load(out, force="mesh")


def main():
    p = load_with_defaults(os.path.join(ROOT, "cad", "parameters.scad"))
    tmp = tempfile.mkdtemp(prefix="cyberdeck-case-")
    print("-- CARRY CASE --  rendering")
    case = render(tmp, "carrycase.scad", "case")
    deck = trimesh.util.concatenate([
        render(tmp, "cyberdeck.scad", "chassis", ("-D", "variant=2")),
        render(tmp, "cyberdeck.scad", "backplate", ("-D", "variant=2"))])
    print()

    check("the case is one solid body",
          case.is_watertight and case.body_count == 1,
          f"watertight {case.is_watertight}, {case.body_count} body(ies)")

    # The whole point of the cowl channel: the deck must slide its full travel
    # without touching anything. A fit that only works when seated is not a
    # sleeve, it is a puzzle.
    worst, worst_at = 0.0, None
    for dy in (70, 55, 40, 25, 10, 0):
        d = deck.copy()
        d.apply_translation([0.0, float(dy), 0.0])
        it = case.intersection(d)
        v = float(it.volume) if it is not None and len(it.faces) else 0.0
        if v > worst:
            worst, worst_at = v, dy
    check("the deck slides its whole travel without touching",
          worst <= 1.0,
          f"worst interference {worst:.3f} mm^3"
          + (f" at {worst_at} mm above seated" if worst_at is not None else ""))

    # Bomb-proof means the openings are behind material, not merely covered by
    # something the size of a lid.
    zfi, bwd = p["z_front_inner"], p["board_w_display_front"]
    pcb = zfi - bwd
    sites = [("USB-C", 1, p["board_cy"] + p["usbc_off_y"], pcb + p["usbc_w_centre"]),
             ("microSD", 1, p["board_cy"] + p["tf_off_y"], pcb + p["tf_w_centre"]),
             ("keyboard window", -1,
              p["kbd_bay_cy"] + p["kbd_pocket_h"] / 2 - p["kbd_access_from_edge"]
              - p["kbd_access_w"] / 2,
              p["z_back_inner"] + p["kbd_access_above_floor"] + p["kbd_access_h"] / 2)]
    pts = np.array([[sx * (p["body_w"] / 2 + p["case_pad"] + p["case_side"] / 2), y, z]
                    for _, sx, y, z in sites])
    inside = case.contains(pts)
    check("every side opening is buried in wall",
          bool(np.all(inside)),
          ", ".join(f"{lbl} {'buried' if i else 'EXPOSED'}"
                    for (lbl, *_), i in zip(sites, inside)))

    # A lug that is not a hole is decoration.
    lug = np.array([[s * p["case_lug_x"], p["case_lug_y"], 0.0] for s in (-1, 1)])
    solid = case.contains(lug)
    flank = (p["case_side"] - p["case_lug_w"]) / 2.0
    depth = (p["case_z_fr"] + p["case_wall"]) - (p["case_z_bk"] - p["case_wall"])
    check("both strap lugs are through-holes",
          not bool(np.any(solid)),
          f"{int(np.sum(~solid))} of 2 open, {flank:.2f} mm of flank each side "
          f"carrying {flank * depth:.0f} mm^2 in shear")

    # Printed mouth-up nothing may bridge more than a slot roof.
    n, a, c = case.face_normals, case.area_faces, case.triangles_center
    bed = case.bounds[0][1]
    dn = n[:, 1] < -1e-6
    onbed = np.abs(c[:, 1] - bed) < 0.06
    ang = np.degrees(np.arccos(np.clip(-n[:, 1], 0, 1)))
    risk = dn & ~onbed & (ang < 44.0)
    area = float(a[risk].sum())
    check("nothing needs support printed mouth-up",
          area < 400.0,
          f"{area:.0f} mm^2 of shallow downward face, against the two "
          f"{p['case_lug_w']:.1f} mm slot roofs which bridge")

    vol = case.volume / 1000.0
    print(f"\n  case {case.extents[0]:.1f} x {case.extents[1]:.1f} x "
          f"{case.extents[2]:.1f} mm, {vol:.0f} cm^3 "
          f"(~{vol * 1.24 * 0.55:.0f} g printed)")
    print()
    if FAILED:
        print(f"  {len(FAILED)} check(s) failed.")
        return 1
    print("  all checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
