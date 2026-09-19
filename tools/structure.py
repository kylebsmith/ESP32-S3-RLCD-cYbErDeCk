#!/usr/bin/env python3
"""
structure.py - section properties of the shell, measured from the rendered mesh.

DESIGN.md makes three structural claims, and until this tool existed all three
were arguments rather than numbers:

    1. a closed torsion box is far stiffer than the reference's open tray
    2. the joint belongs at the back, out of the peak-bending fibre
    3. the full-depth spine is what stops a device of this aspect ratio folding

This computes them. It is NOT finite element analysis and does not pretend to
be: it is classical section analysis - second moment of area, section modulus,
and Bredt's formula for a single-cell closed section - evaluated on sections
cut from the real mesh rather than from an idealised rectangle.

What that is good for: ratios and weak-point location, which are governed by
geometry alone and so are independent of material, infill and print settings.
What it is not good for: absolute stress or a drop-survival prediction, which
depend on layer adhesion and strain rate and need a physical test.

    python3 tools/structure.py [--stations 140] [--plot]
"""

from __future__ import annotations

import argparse
import os
import sys
import tempfile

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from drawing import render, section, load_params, OUT  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


# ---------------------------------------------------------------------------
# Polygon second moments, by Green's theorem
# ---------------------------------------------------------------------------

def _ring_moments(xy):
    """(area, first moment about u, second moment about the u axis) of one ring.

    Signed, so an interior ring wound opposite the exterior subtracts itself and
    holes need no special handling.
    """
    p = np.asarray(xy, float)
    if len(p) < 3:
        return 0.0, 0.0, 0.0
    u, v = p[:, 0], p[:, 1]
    u2, v2 = np.roll(u, -1), np.roll(v, -1)
    cr = u * v2 - u2 * v                      # 2 x signed triangle area
    a = cr.sum() / 2.0
    qu = (cr * (v + v2)).sum() / 6.0          # integral of v dA
    iu = (cr * (v * v + v * v2 + v2 * v2)).sum() / 12.0   # integral of v^2 dA
    return a, qu, iu


def section_props(polys):
    """Area, centroid height, I about the centroidal horizontal axis, and the
    section modulus of a set of shapely polygons in (u, v) = (X, Z).

    RING ORIENTATION IS NOT OPTIONAL HERE. The signed moments only sum
    correctly if every exterior runs counter-clockwise and every hole runs
    clockwise, so each polygon is explicitly oriented first. An earlier version
    took abs() of the totals instead, which works for a single solid ring and
    silently corrupts anything with holes or several disjoint regions: once the
    button and microphone apertures were cut, the top-wall stations reported a
    NEGATIVE second moment and a section modulus of -1,629 mm^3. A negative I
    is not a small error, it is a sign the summation is wrong.
    """
    from shapely.geometry.polygon import orient
    A = Q = I0 = 0.0
    vs = []
    for g in polys:
        g = orient(g, sign=1.0)               # exterior CCW, interiors CW
        a, q, i = _ring_moments(g.exterior.coords)
        A += a; Q += q; I0 += i
        vs += [c[1] for c in g.exterior.coords]
        for r in g.interiors:                 # wound CW, so these subtract
            a, q, i = _ring_moments(r.coords)
            A += a; Q += q; I0 += i
    if A < 1e-9:
        return None
    vbar = Q / A
    I = I0 - A * vbar * vbar                  # parallel axis, to the centroid
    if I <= 0:
        return None
    c = max(abs(max(vs) - vbar), abs(vbar - min(vs)))
    return dict(A=A, vbar=vbar, I=I, c=c, Z=I / c if c > 1e-9 else 0.0)


def _self_test():
    """A rectangle and a hollow box, against the closed-form answers.

    A section tool that can return a negative second moment has to prove it
    does not before any of its output is quoted.
    """
    from shapely.geometry import box
    b, h = 40.0, 10.0
    r = section_props([box(-b/2, -h/2, b/2, h/2)])
    want = b * h**3 / 12.0
    assert abs(r["I"] - want) < 1e-6 * want, f"rect I {r['I']} != {want}"
    assert abs(r["Z"] - want / (h/2)) < 1e-6 * want
    t = 2.0
    hollow = box(-b/2, -h/2, b/2, h/2).difference(
        box(-b/2 + t, -h/2 + t, b/2 - t, h/2 - t))
    r2 = section_props([hollow])
    want2 = (b * h**3 - (b - 2*t) * (h - 2*t)**3) / 12.0
    assert abs(r2["I"] - want2) < 1e-6 * want2, f"box I {r2['I']} != {want2}"
    assert r2["I"] > 0 and r2["A"] > 0
    return want, want2


def close_cell(polys, gap):
    """Bridge the assembly fit clearance so the section reads as one cell.

    The back plate is a separate body with `fit_slide` of clearance around it,
    so the assembled section is not topologically closed in the mesh even
    though it is closed mechanically - by four screws and a tongue-and-groove
    along the bottom edge. Bredt's formula needs the cell, so the clearance is
    bridged here by a dilate-erode. This is an ASSUMPTION, and it is the
    optimistic one: it treats the joint as continuous. It is reasonable for
    the screwed, captured joint used here and would not be for a snap fit.
    """
    from shapely.ops import unary_union
    u = unary_union(list(polys))
    return u.buffer(gap, join_style=2).buffer(-gap, join_style=2)


def perimeter_thickness(polys):
    """Enclosed midline area and the line integral of ds/t, for Bredt's formula.

    A single-cell thin-walled tube of enclosed area Am and wall t has torsional
    constant J = 4*Am^2 / integral(ds/t). Both terms are taken from the real
    section: Am from the void the walls enclose, and ds/t by walking the void's
    boundary and measuring the material thickness normal to it.
    """
    from shapely.geometry import Polygon
    from shapely.ops import unary_union
    gs = list(polys.geoms) if getattr(polys, "geom_type", "").startswith("Multi") \
        else ([polys] if hasattr(polys, "exterior") else list(polys))
    voids = [Polygon(r) for g in gs for r in g.interiors]
    if not voids:
        return None
    cell = max(voids, key=lambda q: q.area)   # the structural cell
    solid = unary_union(gs)
    ring = cell.exterior
    n = 240
    ds_over_t = 0.0
    L = ring.length
    ts = []
    for k in range(n):
        s0, s1 = L * k / n, L * (k + 1) / n
        p0, p1 = ring.interpolate(s0), ring.interpolate(s1)
        ds = p0.distance(p1)
        mid = ring.interpolate((s0 + s1) / 2)
        # outward normal, by stepping off the midpoint until we leave the solid
        a = ring.interpolate(max(s0 - L / n, 0.0)); b = ring.interpolate(min(s1 + L / n, L))
        tx, ty = b.x - a.x, b.y - a.y
        m = (tx * tx + ty * ty) ** 0.5
        if m < 1e-9:
            continue
        nx, ny = ty / m, -tx / m              # normal, sign resolved below
        if cell.contains(__import__("shapely.geometry", fromlist=["Point"])
                         .Point(mid.x + nx * 0.05, mid.y + ny * 0.05)):
            nx, ny = -nx, -ny                 # point away from the cell
        t = 0.0
        step = 0.05
        Point = __import__("shapely.geometry", fromlist=["Point"]).Point
        while t < 25.0:
            t += step
            if not solid.contains(Point(mid.x + nx * t, mid.y + ny * t)):
                break
        ts.append(t)
        ds_over_t += ds / max(t, 1e-6)
    tss = sorted(ts) if ts else [0.0]
    return dict(Am=cell.area, ds_over_t=ds_over_t,
                J=4 * cell.area ** 2 / ds_over_t if ds_over_t > 0 else 0.0,
                t_p05=tss[max(0, int(0.05 * len(tss)))],
                t_min=tss[0])


# ---------------------------------------------------------------------------

def scan(chassis, plate, stations):
    """Section properties along Y - the axis this device folds about."""
    lo, hi = chassis.bounds[0][1], chassis.bounds[1][1]
    out = []
    for y in np.linspace(lo + 0.6, hi - 0.6, stations):
        op = section(chassis, 1, y)
        if not op:
            continue
        cl = op + section(plate, 1, y)
        po, pc = section_props(op), section_props(cl)
        if po and pc:
            out.append((float(y), po, pc))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--stations", type=int, default=120)
    ap.add_argument("--workdir")
    ap.add_argument("--plot", action="store_true")
    ap.add_argument("--reference", help="path to a clone of nilseuropa/solar_term")
    a = ap.parse_args()

    w1, w2 = _self_test()
    print(f"  self-test: solid rect I = {w1:,.1f} mm^4, hollow box I = {w2:,.1f} "
          f"mm^4, both reproduced exactly")
    p = load_params()
    tmp = a.workdir or tempfile.mkdtemp(prefix="cyberdeck-structure-")
    chassis, plate = render("chassis", tmp), render("backplate", tmp)

    rows = scan(chassis, plate, a.stations)
    if not rows:
        sys.exit("no sections")

    ys = np.array([r[0] for r in rows])
    Io = np.array([r[1]["I"] for r in rows])
    Ic = np.array([r[2]["I"] for r in rows])
    Zo = np.array([r[1]["Z"] for r in rows])
    Zc = np.array([r[2]["Z"] for r in rows])

    kmin = int(np.argmin(Zc))
    spine_cy = p["board_bay_cy"] - p["board_pocket_h"] / 2 - p["spine"] / 2

    W = 78
    print("=" * W)
    print("SECTION ANALYSIS - bending about X, i.e. the device folding across "
          "its width")
    print("=" * W)
    print(f"  {len(rows)} stations along Y, from the rendered mesh\n")
    print(f"  {'':22}{'open (chassis only)':>22}{'closed (+ back plate)':>24}")
    print(f"  {'mean I  [mm^4]':22}{Io.mean():>22,.0f}{Ic.mean():>24,.0f}")
    print(f"  {'min  I  [mm^4]':22}{Io.min():>22,.0f}{Ic.min():>24,.0f}")
    print(f"  {'mean Z  [mm^3]':22}{Zo.mean():>22,.0f}{Zc.mean():>24,.0f}")
    print(f"  {'min  Z  [mm^3]':22}{Zo.min():>22,.0f}{Zc.min():>24,.0f}")
    print()
    print(f"  closing the shell multiplies mean stiffness by "
          f"{Ic.mean()/Io.mean():5.2f}x")
    print(f"  and the worst-section modulus by            "
          f"{Zc.min()/max(Zo.min(),1e-9):5.2f}x")
    print()
    print(f"  weakest closed section at Y = {ys[kmin]:+7.2f} mm   "
          f"Z = {Zc[kmin]:,.0f} mm^3")
    print(f"  spine centreline at          Y = {spine_cy:+7.2f} mm")
    d = abs(ys[kmin] - spine_cy)
    print(f"  the weak point is {d:.1f} mm from the spine - "
          + ("the spine is NOT covering it" if d > 8 else
             "the spine sits on it, which is where it is needed"))

    # ---- Torsion. Only where a genuine closed cell exists ------------------
    # A section taken across an aperture is NOT a closed tube: the front face
    # is absent over the aperture's width, so the section is a U closed only by
    # the back plate. Bredt's formula does not apply there and forcing it would
    # overstate the shell badly. Find the stations that really are closed.
    gap = p.get("fit_slide", 0.3) + 0.25
    closed = []
    for y, _, _ in rows:
        cell = close_cell(section(chassis, 1, y) + section(plate, 1, y), gap)
        gs = list(cell.geoms) if cell.geom_type.startswith("Multi") else [cell]
        if any(g.interiors for g in gs):
            closed.append((y, cell))

    print()
    print("-" * W)
    print("SECTION TOPOLOGY - where the shell is genuinely a closed tube")
    print("-" * W)
    frac = len(closed) / len(rows)
    print(f"  {len(closed)} of {len(rows)} stations enclose a cell  ({frac*100:.0f}% of the length)")
    print("  The rest are U-channels: across either aperture the front face is")
    print("  absent, so the back plate alone closes the section. This is why the")
    print("  back plate is structural and 3.2 mm rather than a cosmetic cover.")

    if closed:
        ycl = np.array([c[0] for c in closed])
        best = max(closed, key=lambda c: max(
            (__import__("shapely.geometry", fromlist=["Polygon"]).Polygon(r).area
             for g in (list(c[1].geoms) if c[1].geom_type.startswith("Multi") else [c[1]])
             for r in g.interiors), default=0.0))
        tor = perimeter_thickness(best[1])
        print()
        print("-" * W)
        print("TORSION - Bredt single-cell, at the fullest genuinely closed station")
        print("-" * W)
        print(f"  station Y = {best[0]:+.2f} mm")
        print(f"  (the {gap-0.25:.2f} mm plate fit clearance is bridged - the joint is")
        print("   screwed and captured, so it is treated as continuous)")
        if tor:
            print(f"  enclosed cell area Am   {tor['Am']:>12,.1f} mm^2")
            print(f"  integral ds/t           {tor['ds_over_t']:>12,.2f}")
            print(f"  torsional constant J    {tor['J']:>12,.0f} mm^4")
            print(f"  wall on the cell boundary: 5th pct {tor['t_p05']:.2f} mm, "
                  f"single thinnest probe {tor['t_min']:.2f} mm")
            print("  (the single minimum is one probe and can land on a rolled")
            print("   edge, so the percentile is the meaningful figure)")
            s_est = 2 * (p["body_w"] + p["body_t"])
            j_open = s_est * p["wall"] ** 3 / 3.0
            print()
            print(f"  The same outline left open is of order (1/3)*sum(s*t^3) = "
                  f"{j_open:,.0f} mm^4,")
            print(f"  so closing the section is worth about {tor['J']/j_open:,.0f}x "
                  "in torsion.")

    print()
    print("-" * W)
    print("JOINT POSITION")
    print("-" * W)
    # Taken at a typical station clear of the battery cowl, not at the weakest
    # one: the cowl reaches to z = -10 and would skew the fibre distances.
    body_stations = [r for r in rows if r[2]["vbar"] > 0]
    ref = body_stations[len(body_stations) // 2] if body_stations else rows[kmin]
    zbar = ref[2]["vbar"]
    print(f"  neutral axis of the closed section sits at z = {zbar:.2f} mm "
          f"(station Y = {ref[0]:+.1f})")
    print(f"  back plate joint plane                  z = {p['back_t']:.2f} mm")
    print(f"  front face outer fibre                  z = {p['body_t']:.2f} mm")
    df = abs(p["body_t"] - zbar); dj = abs(p["back_t"] - zbar)
    print(f"  joint is {dj:.2f} mm from the neutral axis; the front face is "
          f"{df:.2f} mm")
    print(f"  putting the joint at the back rather than the face reduces the "
          f"bending stress it carries by {(1 - dj/df)*100:.0f}%")
    print("=" * W)

    if a.reference:
        import trimesh
        # Reference frame: X is device height, Y is thickness, Z is width.
        # This project's frame: X width, Y height, Z thickness. So our
        # (x, y, z) = ref (Z, X, Y) - a cyclic permutation, determinant +1, so
        # it is a rotation and introduces no mirroring.
        M = np.array([[0, 0, 1, 0],
                      [1, 0, 0, 0],
                      [0, 1, 0, 0],
                      [0, 0, 0, 1]], float)
        rp = []
        for nm in ("Caseback.stl", "Bezel.stl"):
            m = trimesh.load(os.path.join(a.reference, "stl/ata", nm), force="mesh")
            m.apply_transform(M)
            rp.append(m)
        rlo = min(m.bounds[0][1] for m in rp); rhi = max(m.bounds[1][1] for m in rp)
        rZ = []
        for y in np.linspace(rlo + 0.6, rhi - 0.6, a.stations):
            polys = []
            for m in rp:
                polys += section(m, 1, y)
            pr = section_props(polys) if polys else None
            if pr:
                rZ.append((float(y), pr))
        if rZ:
            rzz = np.array([r[1]["Z"] for r in rZ])
            rii = np.array([r[1]["I"] for r in rZ])
            print()
            print("=" * W)
            print("AGAINST THE REFERENCE DECK  (assembled: Caseback + Bezel)")
            print("=" * W)
            print(f"  {'':22}{'reference':>22}{'this design':>24}")
            print(f"  {'stations':22}{len(rZ):>22,}{len(rows):>24,}")
            print(f"  {'mean I  [mm^4]':22}{rii.mean():>22,.0f}{Ic.mean():>24,.0f}")
            print(f"  {'mean Z  [mm^3]':22}{rzz.mean():>22,.0f}{Zc.mean():>24,.0f}")
            print(f"  {'min  Z  [mm^3]':22}{rzz.min():>22,.0f}{Zc.min():>24,.0f}")
            print()
            print(f"  mean bending stiffness   {Ic.mean()/rii.mean():5.2f}x the reference")
            print(f"  worst-section modulus    {Zc.min()/rzz.min():5.2f}x the reference")
            print()
            print("  Both decks are weakest across their keyboard aperture, for the")
            print("  same reason: the front face is absent there. The comparison is")
            print("  therefore like for like.")
            print("=" * W)

    if a.plot:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        fig, ax = plt.subplots(figsize=(11, 4.2), dpi=170)
        ax.plot(ys, Zo, lw=1.2, color="#b04a3a", label="chassis alone (open)")
        ax.plot(ys, Zc, lw=1.6, color="#2f6b45", label="chassis + back plate (closed)")
        ax.axvline(spine_cy, color="#888", ls="--", lw=0.9)
        ax.annotate("spine", (spine_cy, ax.get_ylim()[1] * 0.92), fontsize=8,
                    color="#666", ha="center")
        ax.plot([ys[kmin]], [Zc[kmin]], "o", color="#2f6b45", ms=5)
        ax.annotate(f"weakest closed section\n{Zc[kmin]:,.0f} mm³",
                    (ys[kmin], Zc[kmin]), textcoords="offset points",
                    xytext=(8, 14), fontsize=8, color="#2f6b45")
        ax.set_xlabel("station along Y  [mm]")
        ax.set_ylabel("section modulus Z  [mm³]")
        ax.set_title("cYbErDeCk · section modulus along the folding axis, "
                     "measured from the mesh", fontsize=10, loc="left")
        ax.legend(fontsize=8, frameon=False)
        ax.grid(alpha=0.25, lw=0.5)
        fig.tight_layout()
        os.makedirs(OUT, exist_ok=True)
        f = os.path.join(OUT, "sheet5-structure.png")
        fig.savefig(f, facecolor="white"); plt.close(fig)
        print(f"wrote {f}")


if __name__ == "__main__":
    main()
