#!/usr/bin/env python3
"""
check_golden.py - prove that v1 still builds exactly as v1 built.

WHY THIS EXISTS

v2 adds a magnetic front cover. It shares one codebase with v1, because forking
5,561 lines guarantees the two drift and every future fix has to be made twice.
The cost of sharing is that a v2 edit can silently move v1's geometry, and v1 is
a design somebody has already printed.

A git tag cannot detect that. A tag is a pointer; it says where v1 was, not
whether v1 still reproduces. This does: it holds every scalar and vector
parameter, and the three parts' extents and volumes, as measured at the v1
release, and it fails loudly and by name when one of them moves.

    python3 tools/check_golden.py              # parameters only, fast
    python3 tools/check_golden.py --stl        # also rebuild and measure meshes
    python3 tools/check_golden.py --update     # re-baseline (deliberate only)

A new parameter that v1 never had is NOT a failure - v2 is allowed to add
things. A parameter that v1 had and that has CHANGED VALUE is a failure, and so
is one that has disappeared.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
GOLDEN = os.path.join(ROOT, "tests", "golden", "v1.json")

sys.path.insert(0, HERE)

# Parameters are compared exactly, to 1e-6, because they are authored numbers
# rather than measured ones: a derived value that moves by 1e-6 has had one of
# its inputs changed, which is exactly what this is here to catch.
PARAM_TOL = 1e-6
# Mesh figures come out of a CSG kernel whose output is not bit-reproducible
# between runs, so they get a real tolerance. It is still far tighter than any
# change a design edit would produce.
EXTENT_TOL = 1e-3      # mm
VOLUME_TOL = 1e-2      # mm^3


def build_params(variant):
    """Parse parameters.scad as the given variant sees it."""
    from params import load_with_defaults
    return load_with_defaults(os.path.join(ROOT, "cad", "parameters.scad"))


def measure_parts(variant, names):
    """Render each part from source and measure it."""
    import trimesh
    out = {}
    tmp = tempfile.mkdtemp(prefix="cyberdeck-golden-")
    runner = []
    if subprocess.run(["which", "xvfb-run"], capture_output=True).returncode == 0:
        runner = ["xvfb-run", "-a"]
    for name in names:
        stl = os.path.join(tmp, f"{name}.stl")
        cmd = runner + ["openscad", "-D", f'part="{name}"',
                        "-D", f"variant={variant}", "-o", stl,
                        os.path.join(ROOT, "cad", "cyberdeck.scad")]
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=1800)
        if not os.path.exists(stl):
            # A model that predates the variant switch will reject -D variant.
            cmd = runner + ["openscad", "-D", f'part="{name}"', "-o", stl,
                            os.path.join(ROOT, "cad", "cyberdeck.scad")]
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=1800)
        if not os.path.exists(stl):
            raise RuntimeError(f"openscad produced no {name}:\n{r.stderr[-1200:]}")
        m = trimesh.load(stl, force="mesh")
        out[name] = {
            "extents": [round(float(q), 4) for q in m.extents],
            "volume": round(float(m.volume), 3),
            "watertight": bool(m.is_watertight),
            "bodies": int(m.body_count),
        }
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--stl", action="store_true",
                    help="also rebuild the parts and compare meshes (slow)")
    ap.add_argument("--update", action="store_true",
                    help="re-baseline the golden file; use only on purpose")
    ap.add_argument("--golden", default=GOLDEN)
    args = ap.parse_args()

    if not os.path.exists(args.golden) and not args.update:
        sys.exit(f"no golden file at {args.golden}")

    variant = 1
    p = build_params(variant)
    scalars = {k: round(float(v), 6) for k, v in p.items()
               if isinstance(v, (int, float))}
    vectors = {k: [round(float(q), 6) for q in v] for k, v in p.items()
               if isinstance(v, list)}

    if args.update:
        g = {"version": "v1.0",
             "commit": subprocess.run(["git", "-C", ROOT, "rev-parse", "HEAD"],
                                      capture_output=True, text=True).stdout.strip(),
             "scalars": dict(sorted(scalars.items())),
             "vectors": dict(sorted(vectors.items())),
             "parts": measure_parts(variant, ["chassis", "backplate", "buttons"])}
        os.makedirs(os.path.dirname(args.golden), exist_ok=True)
        json.dump(g, open(args.golden, "w"), indent=1, sort_keys=True)
        print(f"  re-baselined {args.golden} at {g['commit'][:8]}")
        return 0

    g = json.load(open(args.golden))
    print(f"-- GOLDEN v1 --  baseline {g['commit'][:8]}, "
          f"{len(g['scalars'])} scalars, {len(g['vectors'])} vectors")

    drift, missing = [], []
    for name, want in g["scalars"].items():
        if name not in scalars:
            missing.append(name)
        elif abs(scalars[name] - want) > PARAM_TOL:
            drift.append((name, want, scalars[name]))
    for name, want in g["vectors"].items():
        got = vectors.get(name)
        if got is None:
            missing.append(name)
        elif len(got) != len(want) or any(abs(a - b) > PARAM_TOL
                                          for a, b in zip(got, want)):
            drift.append((name, want, got))

    added = sorted(set(scalars) - set(g["scalars"])) + \
        sorted(set(vectors) - set(g["vectors"]))

    for name, want, got in drift:
        print(f"  [FAIL] {name} moved: v1 had {want}, now {got}")
    for name in missing:
        print(f"  [FAIL] {name} no longer exists; v1 depended on it")
    if added:
        print(f"  [ok]   {len(added)} parameter(s) added since v1 "
              f"(allowed): {', '.join(added[:8])}"
              + (" ..." if len(added) > 8 else ""))
    if not drift and not missing:
        print(f"  [PASS] all {len(g['scalars']) + len(g['vectors'])} v1 "
              "parameters reproduce exactly")

    mesh_bad = []
    if args.stl:
        got = measure_parts(variant, list(g["parts"]))
        for name, want in g["parts"].items():
            g2 = got[name]
            de = max(abs(a - b) for a, b in zip(g2["extents"], want["extents"]))
            dv = abs(g2["volume"] - want["volume"])
            ok = (de <= EXTENT_TOL and dv <= VOLUME_TOL
                  and g2["watertight"] == want["watertight"]
                  and g2["bodies"] == want["bodies"])
            print(f"  [{'PASS' if ok else 'FAIL'}] {name}: "
                  f"d(extent) {de:.5f} mm, d(volume) {dv:.4f} mm^3, "
                  f"watertight {g2['watertight']}, bodies {g2['bodies']}")
            if not ok:
                mesh_bad.append(name)

    bad = len(drift) + len(missing) + len(mesh_bad)
    print()
    if bad:
        print(f"  v1 NO LONGER REPRODUCES - {bad} discrepancy(ies). "
              "A v2 edit has moved v1 geometry.")
        return 1
    print("  v1 reproduces exactly.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
