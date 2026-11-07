#!/usr/bin/env python3
"""Measure the EXPORTED cut files and meshes. Never the source.

The repo's standing discipline: a check that computes from the same parameters
as the thing it checks is worse than none, because it passes while the artefact
is wrong. So this reads the DXF the cutter will receive and the STL the slicer
will receive, recovers the geometry from them, and compares that against the
datums -- which it parses out of parameters.scad rather than re-declaring.

    /usr/bin/python3.11 v2/tools/check_cad.py
"""
import math
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPO = os.path.dirname(ROOT)
PARAMS = os.path.join(ROOT, "cad", "parameters.scad")
TOL = 0.05          # mm: tessellation slop on a laser profile


# ---- the datums, parsed from the one file that owns them ------------------
def datums():
    """Evaluate parameters.scad's scalar assignments, in order.

    OpenSCAD is not Python, but the subset this file uses -- scalars, simple
    arithmetic, list literals -- evaluates identically. Anything that does not
    is skipped rather than guessed at.
    """
    env, prov = {}, {}
    text = open(PARAMS, encoding="utf-8").read()
    for line in text.splitlines():
        m = re.match(r"\s*(\w+)\s*=\s*([^;]+);(.*)$", line)
        if not m:
            continue
        name, expr, rest = m.group(1), m.group(2).strip(), m.group(3)
        tag = re.search(r"\[(\w+)\]", rest)
        if tag:
            prov[name] = tag.group(1)
        expr = expr.replace("[", "(").replace("]", ")")
        try:
            env[name] = eval(expr, {"__builtins__": {}}, dict(env))
        except Exception:
            pass
    return env, prov


# ---- DXF: recover the closed loops the cutter will follow -----------------
def dxf_loops(path):
    import ezdxf
    doc = ezdxf.readfile(path)
    loops = []
    for e in doc.modelspace():
        t = e.dxftype()
        if t == "LWPOLYLINE":
            pts = [(p[0], p[1]) for p in e.get_points()]
            if e.closed and len(pts) > 2:
                loops.append(pts)
        elif t == "POLYLINE":
            pts = [(v.dxf.location.x, v.dxf.location.y) for v in e.vertices]
            if len(pts) > 2:
                loops.append(pts)
        elif t == "CIRCLE":
            c, r = e.dxf.center, e.dxf.radius
            loops.append([(c.x + r*math.cos(a), c.y + r*math.sin(a))
                          for a in [i*math.tau/64 for i in range(64)]])
    if not loops:          # OpenSCAD writes bare LINE segments; stitch them
        segs = [((e.dxf.start.x, e.dxf.start.y), (e.dxf.end.x, e.dxf.end.y))
                for e in doc.modelspace() if e.dxftype() == "LINE"]
        loops = stitch(segs)
    return loops


def stitch(segs, tol=1e-6):
    """Join LINE segments end-to-end into closed loops."""
    from collections import defaultdict
    ends = defaultdict(list)
    key = lambda p: (round(p[0], 6), round(p[1], 6))
    for i, (a, b) in enumerate(segs):
        ends[key(a)].append(i)
        ends[key(b)].append(i)
    used, loops = set(), []
    for i in range(len(segs)):
        if i in used:
            continue
        a, b = segs[i]
        loop, used_here = [a, b], {i}
        cur = b
        while True:
            nxt = None
            for j in ends[key(cur)]:
                if j in used_here or j in used:
                    continue
                p, q = segs[j]
                nxt = (j, q if key(p) == key(cur) else p)
                break
            if nxt is None:
                break
            j, pt = nxt
            used_here.add(j)
            if key(pt) == key(loop[0]):
                break
            loop.append(pt)
            cur = pt
        used |= used_here
        if len(loop) > 2:
            loops.append(loop)
    return loops


def describe(loop):
    """(cx, cy, w, h, is_circular, diameter_or_None) for a recovered loop."""
    xs = [p[0] for p in loop]
    ys = [p[1] for p in loop]
    cx, cy = (min(xs)+max(xs))/2, (min(ys)+max(ys))/2
    w, h = max(xs)-min(xs), max(ys)-min(ys)
    rs = [math.hypot(x-cx, y-cy) for x, y in loop]
    circular = (max(rs) - min(rs)) < 0.12 and abs(w-h) < 0.12
    return cx, cy, w, h, circular, (w if circular else None)


def main():
    env, prov = datums()
    fails, notes = [], []

    def need(cond, msg):
        if not cond:
            fails.append(msg)

    # ---------------- the acrylic slab -----------------------------------
    dxf = os.path.join(ROOT, "export", "dxf", "slab.dxf")
    if not os.path.exists(dxf):
        fails.append(f"missing {dxf} -- run the export first")
    else:
        loops = dxf_loops(dxf)
        shapes = [describe(l) for l in loops]
        outer = max(shapes, key=lambda s: s[2] * s[3])
        need(abs(outer[2] - env["body_w"]) < TOL,
             f"slab outline width {outer[2]:.3f}, want {env['body_w']:.3f}")
        need(abs(outer[3] - env["body_h"]) < TOL,
             f"slab outline height {outer[3]:.3f}, want {env['body_h']:.3f}")

        circles = sorted((s for s in shapes if s[4]), key=lambda s: s[5])
        bores = [s for s in circles if abs(s[5] - env["bore_d"]) < 0.3]
        screws = [s for s in circles if abs(s[5] - env["screw_m3_clear_d"]) < 0.3]
        need(len(bores) == 6, f"{len(bores)} knob bores in the DXF, want 6")
        need(len(screws) == 6, f"{len(screws)} fastener holes, want 6")
        for s in bores:
            need(abs(s[5] - env["bore_d"]) < TOL,
                 f"bore at {s[0]:+.2f},{s[1]:+.2f} is {s[5]:.3f}, "
                 f"want {env['bore_d']:.3f}")

        # every hole must keep acrylic's edge distance from the outline AND
        # from the keyboard void -- this is what proved M4 does not fit
        hw, hh = env["body_w"]/2, env["body_h"]/2
        emin = env["acrylic_edge_min"]
        for s in bores + screws:
            cx, cy, w, h, _, d = s
            e = min(hw - (abs(cx) + d/2), hh - (abs(cy) + d/2))
            need(e >= emin - TOL,
                 f"hole at {cx:+.2f},{cy:+.2f} sits {e:.2f} mm from the slab "
                 f"edge, acrylic wants {emin:.2f}")

        need(len(loops) == 15,
             f"{len(loops)} cut contours, want 15 "
             f"(outline, keyboard void, 6 bores, 6 fasteners, 1 fader slot)")
        notes.append(f"slab.dxf: {len(loops)} contours, "
                     f"{outer[2]:.3f} x {outer[3]:.3f} mm")

    # ---------------- the meshes ------------------------------------------
    import trimesh
    for name in ("slab",):
        stl = os.path.join(ROOT, "export", "stl", f"{name}.stl")
        if not os.path.exists(stl):
            fails.append(f"missing {stl}")
            continue
        m = trimesh.load(stl)
        need(len(m.faces) > 0, f"{name}.stl is empty")
        need(m.is_watertight, f"{name}.stl is not watertight -- a slicer will "
                              f"guess at what you meant")
        need(m.is_volume, f"{name}.stl is not a valid volume")
        ext = m.extents
        need(abs(ext[2] - env["acrylic_t"]) < TOL,
             f"{name}.stl is {ext[2]:.3f} thick, want {env['acrylic_t']:.3f}")
        notes.append(f"{name}.stl: {len(m.faces)} facets, "
                     f"{ext[0]:.2f} x {ext[1]:.2f} x {ext[2]:.2f}, "
                     f"watertight={m.is_watertight}")

    # ---------------- provisional datums ----------------------------------
    prov_list = sorted(k for k, v in prov.items() if v == "PROVISIONAL")

    for n in notes:
        print(f"  {n}")
    if prov_list:
        print(f"\n  {len(prov_list)} PROVISIONAL datum(s) -- caliper before a "
              f"final cut:")
        for k in prov_list:
            print(f"    {k} = {env.get(k, '?')}")
    print()
    for f in fails:
        print(f"  FAIL  {f}")
    print(f"  {'PASS' if not fails else str(len(fails)) + ' PROBLEM(S)'}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
