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

THE DXF OPENSCAD WRITES IS NOT SAFE TO SEND, SO IT IS NOT WHAT WE SHIP
----------------------------------------------------------------------
OpenSCAD 2021.01 writes an R12 DXF with no HEADER section, no TABLES section,
no units and no layers -- nothing but bare LINE entities on layer "0". A DXF
with no $INSUNITS is interpreted as drawing units by the receiver and the
classic outcome is a part cut at 1/25.4 scale, which on a 304.600 x 190.375 mm
slab in 12.700 mm acrylic is an expensive mistake
(v2/docs/fabrication.md 0.5 and 1.6).

So the laser pipeline has three stages, not one:

    openscad  ->  export/dxf/raw/<name>.dxf      exactly what OpenSCAD wrote
    dxf_post  ->  export/dxf/<name>.dxf          units, layers, closed loops,
                  export/svg/<name>.svg          and the orientation fiducial
    validate  ->  reads BOTH back off disk and REFUSES the pair unless
                  $INSUNITS is 4, the declared layers exist, the entity count
                  is sane and lossless against the raw export, and the DXF and
                  SVG agree about handedness.

The raw file is kept because that is what makes the validation a measurement:
the rewrite is proved lossless against the artefact it came from, not against
the code that produced it. A validation failure FAILS THE EXPORT -- the whole
point is that a bad vector file is loud rather than quiet.
"""
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

ROOT = os.path.dirname(HERE)
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
        # The DXF OpenSCAD writes goes to raw/; dxf_post builds the shippable
        # file from it and the validation compares the two.
        if ext == "dxf":
            d = os.path.join(d, "raw")
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

    # ---- the vector post-process, and then the refusal -------------------
    # Only for laser parts, and only once both 2D files are on disk, because
    # the handedness check needs the pair.
    if kind == "laser" and not any(s == "FAIL" for _, s, _ in out):
        import dxf_post
        svg = os.path.join(EXPORT, "svg", f"{name}.svg")
        try:
            fails, facts = dxf_post.post_process(
                name, os.path.join(EXPORT, "dxf"), svg)
        except Exception as ex:
            out.append((name, "FAIL", f"dxf post-process: "
                                      f"{type(ex).__name__}: {ex}"))
            return out
        for line in dxf_post.report(name, facts).splitlines():
            out.append((name, "    ", line.strip()))
        if fails:
            for f in fails:
                out.append((name, "FAIL", f"vector: {f}"))
        else:
            kb = os.path.getsize(os.path.join(EXPORT, "dxf",
                                              f"{name}.dxf")) / 1024
            out.append((name, "ok", f"dxf {kb:8.1f} kB  validated: units, "
                                    f"layers, closed loops, handedness"))
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
