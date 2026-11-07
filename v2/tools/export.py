#!/usr/bin/env python3
"""Render every v2 part to the files a fabricator actually receives.

    /usr/bin/python3.11 v2/tools/export.py            everything
    /usr/bin/python3.11 v2/tools/export.py slab       one part

Laser parts export as DXF (the cut profile) and SVG (for reading); printed
parts export as binary STL. Each part declares which it is, because exporting
a laser part as a mesh and a printed part as a profile are both silent ways to
send the wrong file.

Every export is a full CGAL render, not a preview. OpenSCAD's preview
depth-peels a limited number of CSG layers and stripes anything deeper, which
looks like a modelling fault and is not one; v1 learned that and kept its
presentation views separate for the same reason.
"""
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CAD = os.path.join(ROOT, "cad")
EXPORT = os.path.join(ROOT, "export")

#  name          source            kind     notes
PARTS = [
    ("slab",      "slab.scad",      "laser", "12.70 clear acrylic, front"),
    ("backpanel", "backpanel.scad", "laser", "laser-cut back"),
    ("carrier",   "carrier.scad",   "print", "the printed carrier plate"),
]


def run(cmd, timeout=1800):
    t0 = time.time()
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    return r, time.time() - t0


def export_one(name, src, kind):
    path = os.path.join(CAD, src)
    if not os.path.exists(path):
        return [(name, "skip", f"{src} does not exist yet")]
    out = []
    jobs = ([("dxf", "dxf", "2d"), ("svg", "svg", "2d"),
             ("stl", "stl", "3d")] if kind == "laser"
            else [("stl", "stl", "3d")])
    for sub, ext, part in jobs:
        d = os.path.join(EXPORT, sub)
        os.makedirs(d, exist_ok=True)
        dest = os.path.join(d, f"{name}.{ext}")
        cmd = ["openscad", "-D", f'part="{part}"']
        if ext == "stl":
            cmd += ["--export-format=binstl"]
        cmd += ["-o", dest, path]
        r, dt = run(cmd)
        # OpenSCAD treats an unknown variable as undef and carries on, which
        # produces a part that renders, exports, and is silently wrong -- the
        # exact class of bug this project keeps paying for. Treat it as fatal.
        fatal = [ln for ln in r.stderr.splitlines()
                 if "unknown variable" in ln or "undefined operation" in ln
                 or "WARNING: Object may not be a valid 2-manifold" in ln]
        if fatal:
            out.append((name, "FAIL", f"{ext}: {fatal[0].strip()}"
                                      f"  (+{len(fatal)-1} more)"))
            continue
        if r.returncode != 0 or not os.path.exists(dest) \
                or os.path.getsize(dest) == 0:
            out.append((name, "FAIL", f"{ext}: {r.stderr.strip()[-200:]}"))
        else:
            kb = os.path.getsize(dest) / 1024
            out.append((name, "ok", f"{ext:3s} {kb:8.1f} kB  {dt:5.1f}s"))
    return out


def main():
    want = sys.argv[1:] or [p[0] for p in PARTS]
    rows = []
    for name, src, kind, _ in PARTS:
        if name in want:
            rows += export_one(name, src, kind)
    bad = 0
    for name, state, msg in rows:
        if state == "FAIL":
            bad += 1
        print(f"  {state:4s}  {name:10s} {msg}")
    if not rows:
        print(f"  nothing matched {want}")
        return 1
    print(f"  {'all exported' if not bad else str(bad) + ' FAILED'}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
