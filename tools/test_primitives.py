#!/usr/bin/env python3
"""
test_primitives.py - unit tests for the geometry helpers in cad/lib/util.scad.

WHY THIS EXISTS

Every other tool here checks the finished parts. Nothing checked the primitives
those parts are built from, and a primitive that quietly does the wrong thing
produces parts that pass every dimensional check while being wrong in a way
only the eye catches.

That is exactly what happened. rse_aperture() called

    rse_plate(w + 2*flare, h + 2*flare, cr + flare, n, q)

into a module declared (w, h, t, cr, n, q = 16). The thickness argument was
missing, so cr+flare landed in `t`, n landed in `cr`, q landed in `n`, and q
fell back to its default. The aperture still opened to the correct WIDTH and
HEIGHT, so the envelope checks, the clearance checks, the lip check and the
reference audit all passed. What it got wrong was the corner: 10.2 mm collapsed
to about 1.3 mm by the visible face, and the cutter was extruded 13.2 mm
instead of 2.4.

An argument-arity checker was written first and DOES NOT CATCH THIS: the call
passes five positional arguments into five required parameters, so it is
arity-legal. The error is a wrong value in the right slot. Only measuring what
the primitive actually produces finds it.

Each test states the inputs and the geometry they must produce, renders the
primitive on its own, and measures it.

    python3 tools/test_primitives.py
"""

from __future__ import annotations

import math
import os
import subprocess
import sys
import tempfile

try:
    import numpy as np
    import trimesh
    from scipy.optimize import least_squares
except ImportError as exc:  # pragma: no cover
    sys.exit(f"missing dependency: {exc}\ninstall with: pip install -r tools/requirements.txt")

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UTIL = os.path.join(ROOT, "cad", "lib", "util.scad")

FAILED = []


# The model sets $fn = 64 in parameters.scad. A test that renders a primitive
# without it measures OpenSCAD's DEFAULT resolution instead of the model's, and
# reports a faceting error the real parts do not have. The first version of this
# file did exactly that and produced a spurious 0.116 mm rbox failure - the test
# was wrong, not the model.
FN = 64


def render(body, tmp, name):
    src = f'use <{UTIL}>\n$fn = {FN};\n{body}\n'
    sf = os.path.join(tmp, f"{name}.scad")
    open(sf, "w").write(src)
    out = os.path.join(tmp, f"{name}.stl")
    cmd = ["openscad", "-o", out, sf]
    if subprocess.run(["which", "xvfb-run"], capture_output=True).returncode == 0:
        cmd = ["xvfb-run", "-a"] + cmd
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=900)
    if not os.path.exists(out):
        raise RuntimeError(f"openscad failed for {name}:\n{r.stderr[-1500:]}")
    return trimesh.load(out, force="mesh")


def ok(name, cond, detail):
    print(f"  [{'PASS' if cond else 'FAIL'}] {name}  --  {detail}")
    if not cond:
        FAILED.append(name)
    return cond


def corner_radius(mesh, z, frac=12.0):
    """Least-squares circle through one corner of a horizontal section."""
    s = mesh.section(plane_origin=[0, 0, z], plane_normal=[0, 0, 1])
    if s is None:
        return None, None, None
    polys = list(s.to_2D(to_2D=np.eye(4))[0].polygons_full)
    if not polys:
        return None, None, None
    g = max(polys, key=lambda q: q.area)
    b = g.bounds
    xy = np.asarray(g.exterior.coords)
    X, Y = b[2], b[3]
    sel = (np.abs(xy[:, 0] - X) < frac) & (np.abs(xy[:, 1] - Y) < frac)
    pts = xy[sel]
    if len(pts) < 8:
        return (b[2] - b[0]), (b[3] - b[1]), None
    sol = least_squares(
        lambda c: np.hypot(pts[:, 0] - c[0], pts[:, 1] - c[1]) - c[2],
        [X - frac / 2, Y - frac / 2, frac / 2])
    return (b[2] - b[0]), (b[3] - b[1]), float(sol.x[2])


def main():
    tmp = tempfile.mkdtemp(prefix="cyberdeck-prims-")
    print("-- PRIMITIVES --")

    # ---- rbox: the plain rounded box, exact extents -----------------------
    m = render("rbox(40, 25, 6, 4);", tmp, "rbox")
    e = m.extents
    # A hull of polygonal cylinders INSCRIBES the true circle, so the extents
    # come up short by the sagitta, r*(1 - cos(pi/$fn)), on each rounded side.
    # At $fn = 64 and r = 4 that is 0.005 mm per side. It is real and it is
    # systematic - every rounded pocket in the design is this much tight - so
    # this states the budget explicitly rather than hiding it in a loose
    # tolerance. If $fn is ever lowered, this is the check that notices.
    sag = 4.0 * (1 - math.cos(math.pi / FN))
    ok("rbox: extents exact to within the faceting budget",
       abs(e[0] - 40) < 2 * sag + 0.005 and abs(e[1] - 25) < 2 * sag + 0.005
       and abs(e[2] - 6) < 0.02,
       f"{e[0]:.3f} x {e[1]:.3f} x {e[2]:.3f}, asked 40 x 25 x 6; "
       f"faceting budget at $fn={FN} is {2*sag:.4f} mm across")

    # ---- rse_plate: a thin plate, thickness and corner ---------------------
    m = render("rse_plate(60, 40, 3, 8, 3.2);", tmp, "rse_plate")
    e = m.extents
    ok("rse_plate: thickness honoured",
       abs(e[2] - 3.0) < 0.02, f"{e[2]:.3f} mm, asked 3.0")
    ok("rse_plate: plan size honoured",
       abs(e[0] - 60) < 0.05 and abs(e[1] - 40) < 0.05,
       f"{e[0]:.3f} x {e[1]:.3f}, asked 60 x 40")

    # ---- rse_aperture: THE REGRESSION TEST --------------------------------
    # A draft cut through a 2.4 mm panel: 40 x 30 opening, 6 mm corner,
    # circular (n=2), flaring 0.6 mm per side.
    W, H, T, CR, N, FL = 40.0, 30.0, 2.4, 6.0, 2.0, 0.6
    m = render(f"rse_aperture({W}, {H}, {T}, {CR}, {N}, {FL});", tmp, "rse_aperture")
    e = m.extents
    ok("rse_aperture: height equals the thickness asked for",
       abs(e[2] - T) < 0.02,
       f"{e[2]:.3f} mm, asked {T}. (The arity bug made this 13.219 for a 2.42 cut.)")
    lo, hi = m.bounds[0][2], m.bounds[1][2]
    w0, h0, r0 = corner_radius(m, lo + 0.02 * (hi - lo), frac=CR * 2)
    w1, h1, r1 = corner_radius(m, hi - 0.02 * (hi - lo), frac=(CR + FL) * 2)
    ok("rse_aperture: narrow face is the stated opening",
       abs(w0 - W) < 0.15 and abs(h0 - H) < 0.15,
       f"{w0:.3f} x {h0:.3f}, asked {W} x {H}")
    ok("rse_aperture: wide face is the opening plus the flare",
       abs(w1 - (W + 2*FL)) < 0.15 and abs(h1 - (H + 2*FL)) < 0.15,
       f"{w1:.3f} x {h1:.3f}, asked {W+2*FL} x {H+2*FL}")
    ok("rse_aperture: corner radius is the stated one at the narrow face",
       r0 is not None and abs(r0 - CR) < 0.25,
       f"{r0:.3f} mm, asked {CR}")
    ok("rse_aperture: corner GROWS with the flare, never collapses",
       r1 is not None and abs(r1 - (CR + FL)) < 0.25,
       f"{r1:.3f} mm, asked {CR+FL}. (The arity bug collapsed this to ~1.3.)")

    # ---- rse_soft: the rolled shell ---------------------------------------
    m = render("rse_soft(80, 50, 10, 12, 3.2, 1.2, 0.28);", tmp, "rse_soft")
    e = m.extents
    ok("rse_soft: overall envelope is exactly as asked",
       abs(e[0] - 80) < 0.05 and abs(e[1] - 50) < 0.05 and abs(e[2] - 10) < 0.05,
       f"{e[0]:.3f} x {e[1]:.3f} x {e[2]:.3f}, asked 80 x 50 x 10")
    wmid, hmid, _ = corner_radius(m, 5.0)
    wtop, htop, _ = corner_radius(m, 9.9)
    ok("rse_soft: widest at mid-thickness, drawn in at the face",
       wmid > wtop and hmid > htop,
       f"mid {wmid:.2f} x {hmid:.2f} vs face {wtop:.2f} x {htop:.2f}")

    # ---- countersunk_hole: depth through the plate -------------------------
    m = render("difference(){ rbox(20,20,3.2,2); countersunk_hole(3.2, 6.0, 1.86, 3.2); }",
               tmp, "cs")
    ok("countersunk_hole: cuts right through the plate",
       m.is_watertight and m.body_count == 1,
       f"watertight={m.is_watertight}, bodies={m.body_count}")

    print()
    if FAILED:
        print(f"  {len(FAILED)} primitive test(s) FAILED: {', '.join(FAILED)}")
        return 1
    print("  all primitive tests passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
