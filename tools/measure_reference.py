#!/usr/bin/env python3
"""
measure_reference.py - reproducible metrology harness for the cYbErDeCk datum set.

PURPOSE
-------
Every dimension in `cad/parameters.scad` that is marked `computed-from-mesh` or
`computed-from-dxf` is produced by this script. Running it regenerates the
measurement report from the raw reference artefacts, so any claim in
`docs/DATUMS.md` can be re-derived from first principles by a third party.

WHAT IT MEASURES
----------------
The reference artefacts are the published enclosure models for the *same two
hardware components* this project targets:

  * Waveshare ESP32-S3-RLCD-4.2 development board
  * Rii 518BT mini Bluetooth keyboard

Those models were authored by people who had the physical parts in hand. Their
pockets, bores and apertures are therefore an *indirect measurement of the
hardware itself*. This script recovers those measurements numerically. It does
not copy, re-export or derive any of the reference geometry - see
`docs/PROVENANCE.md` for the licensing rationale.

METHOD
------
1.  Bounding-box extraction (`trimesh`) for gross envelopes.
2.  Planar cross-sectioning at a swept series of levels. Sections are converted
    to 2D with an EXPLICIT transform so that the resulting coordinates remain in
    the world frame. (trimesh's default `to_2D()` picks an arbitrary in-plane
    basis that drifts from slice to slice; using it without an explicit
    transform silently corrupts absolute positions while leaving sizes intact.)
3.  Interior-loop analysis: each section polygon's interior rings are pockets,
    bores and apertures. Ring bounds + centroid give position and size.
4.  Scan-line probing: material intervals along a ray in-plane, used to recover
    wall thicknesses and tray boundaries where a pocket is open to the exterior
    and therefore is not a closed interior loop.
5.  Ray casting along the thickness axis to recover floor thicknesses, pocket
    depths and bore depths.
6.  DXF group-code parsing for the 2D acrylic window template, which is an
    exact, unambiguous statement of the display aperture.

USAGE
-----
    python3 tools/measure_reference.py --reference /path/to/reference/repo
    python3 tools/measure_reference.py --reference ... --json export/reports/measurements.json

Dependencies: trimesh, numpy, scipy, shapely, ezdxf  (see tools/requirements.txt)
"""

from __future__ import annotations

import argparse
import json
import math
import os
import sys
from dataclasses import dataclass, field, asdict

try:
    import numpy as np
    import trimesh
    from shapely.geometry import Polygon, LineString
    from shapely.ops import unary_union
except ImportError as exc:  # pragma: no cover
    sys.exit(f"missing dependency: {exc}\ninstall with: pip install -r tools/requirements.txt")


# --------------------------------------------------------------------------
# Section helpers
# --------------------------------------------------------------------------

# Explicit world-preserving transforms for each slicing axis. The 4x4 maps world
# XYZ into the section's 2D frame; we choose it so (u, v) are the two world axes
# that remain in-plane, in a right-handed order.
_TO_2D = {
    # slice along Z -> (u, v) = (X, Y)
    2: np.eye(4),
    # slice along Y -> (u, v) = (X, Z)
    1: np.array([[1, 0, 0, 0],
                 [0, 0, 1, 0],
                 [0, -1, 0, 0],
                 [0, 0, 0, 1]], dtype=float),
    # slice along X -> (u, v) = (Y, Z)
    0: np.array([[0, 1, 0, 0],
                 [0, 0, 1, 0],
                 [1, 0, 0, 0],
                 [0, 0, 0, 1]], dtype=float),
}

_AXIS_LABEL = {0: ("Y", "Z"), 1: ("X", "Z"), 2: ("X", "Y")}


def section_polygons(mesh: "trimesh.Trimesh", axis: int, level: float):
    """Return the shapely polygons of a planar section, in WORLD in-plane coords."""
    normal = [0.0, 0.0, 0.0]
    normal[axis] = 1.0
    origin = [0.0, 0.0, 0.0]
    origin[axis] = float(level)
    sec = mesh.section(plane_origin=origin, plane_normal=normal)
    if sec is None:
        return []
    planar, _ = sec.to_2D(to_2D=_TO_2D[axis])
    return list(planar.polygons_full)


def material_intervals(mesh, axis: int, level: float, kind: str, at: float,
                       lo: float, hi: float):
    """Scan-line probe. Returns [(start, end), ...] of solid material along the line."""
    polys = section_polygons(mesh, axis, level)
    if not polys:
        return []
    solid = unary_union(polys)
    line = LineString([(lo, at), (hi, at)]) if kind == "h" else LineString([(at, lo), (at, hi)])
    hit = line.intersection(solid)
    if hit.is_empty:
        return []
    parts = list(hit.geoms) if hit.geom_type.startswith("Multi") else [hit]
    idx = 0 if kind == "h" else 1
    out = []
    for part in parts:
        if part.geom_type != "LineString":
            continue
        coords = np.asarray(part.coords)
        out.append((round(float(coords[:, idx].min()), 3),
                    round(float(coords[:, idx].max()), 3)))
    return sorted(out)


def thickness_profile(mesh, axis: int, a: float, b: float, start: float = -500.0):
    """Ray-cast along `axis` at in-plane point (a, b). Returns solid intervals."""
    origin = [0.0, 0.0, 0.0]
    other = [i for i in range(3) if i != axis]
    origin[other[0]] = float(a)
    origin[other[1]] = float(b)
    origin[axis] = float(start)
    direction = [0.0, 0.0, 0.0]
    direction[axis] = 1.0
    locs, _, _ = mesh.ray.intersects_location(np.array([origin]), np.array([direction]))
    if len(locs) == 0:
        return []
    vals = sorted(float(p[axis]) for p in locs)
    return [(round(vals[i], 3), round(vals[i + 1], 3)) for i in range(0, len(vals) - 1, 2)]


# --------------------------------------------------------------------------
# Result container
# --------------------------------------------------------------------------

@dataclass
class Datum:
    ident: str
    name: str
    value: str
    unit: str = "mm"
    method: str = "computed-from-mesh"
    source: str = ""
    note: str = ""


@dataclass
class Report:
    datums: list = field(default_factory=list)
    raw: dict = field(default_factory=dict)

    def add(self, *a, **kw):
        self.datums.append(Datum(*a, **kw))

    def emit(self, ident, name, value, **kw):
        self.add(ident, name, value, **kw)
        print(f"  {ident:<34} {name:<44} = {value}")


# --------------------------------------------------------------------------
# Measurement routines
# --------------------------------------------------------------------------

def measure_envelopes(ref: str, rep: Report):
    print("\n=== 1. GROSS ENVELOPES (bounding boxes) ===")
    files = [
        ("stl/ata/Caseback.stl", "ref.ata.caseback"),
        ("stl/ata/Caseback_left.stl", "ref.ata.caseback_left"),
        ("stl/ata/Bezel.stl", "ref.ata.bezel"),
        ("stl/ata/Buttons.stl", "ref.ata.buttons"),
        ("stl/ata/Battery_cover.stl", "ref.ata.battery_cover"),
        ("stl/ata/screensaver.stl", "ref.ata.screensaver"),
        ("stl/poc_bottom.stl", "ref.poc.bottom"),
        ("stl/poc_top.stl", "ref.poc.top"),
        ("stl/poc_button.stl", "ref.poc.button"),
    ]
    for rel, ident in files:
        path = os.path.join(ref, rel)
        if not os.path.exists(path):
            print(f"  !! missing {rel}")
            continue
        m = trimesh.load(path, force="mesh")
        e = m.extents
        rep.emit(f"{ident}.envelope", f"{os.path.basename(rel)} bounding box",
                 f"{e[0]:.3f} x {e[1]:.3f} x {e[2]:.3f}", source=rel,
                 note=f"watertight={m.is_watertight} faces={len(m.faces)} "
                      f"bounds={np.round(m.bounds, 3).tolist()}")


def measure_board_pattern(ref: str, rep: Report):
    """The board mounting-hole pattern, measured independently from BOTH reference
    designs. This is the single most load-bearing datum in the project."""
    print("\n=== 2. WAVESHARE BOARD MOUNTING-HOLE PATTERN (two independent derivations) ===")

    # --- derivation A: poc_bottom.stl, sliced along Z, M2.5 clearance holes -----
    m = trimesh.load(os.path.join(ref, "stl/poc_bottom.stl"), force="mesh")
    polys = section_polygons(m, 2, m.bounds[0][2] + 0.05)
    holes = []
    for p in polys:
        for ring in p.interiors:
            rp = Polygon(ring)
            b = rp.bounds
            w, h = b[2] - b[0], b[3] - b[1]
            if 2.4 < w < 3.0 and 2.4 < h < 3.0:          # M2.5 clearance bore
                c = rp.centroid
                holes.append((round(c.x, 3), round(c.y, 3), round((w + h) / 2, 3)))
    holes.sort()
    if len(holes) == 4:
        xs = sorted({h[0] for h in holes})
        ys = sorted({h[1] for h in holes})
        pitch_a = (round(xs[-1] - xs[0], 3), round(ys[-1] - ys[0], 3))
        dia = round(sum(h[2] for h in holes) / 4, 3)
        rep.emit("board.mount.pattern.A", "mount pattern (derivation A: poc_bottom)",
                 f"{pitch_a[0]:.3f} x {pitch_a[1]:.3f}", source="stl/poc_bottom.stl",
                 note=f"4 x bore dia {dia} at {holes}")
    else:
        pitch_a = None
        print(f"  !! derivation A found {len(holes)} candidate holes, expected 4")

    # --- derivation B: ata/Caseback.stl, sliced along Y, M2.5 clearance holes ----
    m2 = trimesh.load(os.path.join(ref, "stl/ata/Caseback.stl"), force="mesh")
    polys = section_polygons(m2, 1, -11.323)
    holes2 = []
    for p in polys:
        for ring in p.interiors:
            rp = Polygon(ring)
            b = rp.bounds
            w, h = b[2] - b[0], b[3] - b[1]
            if 2.4 < w < 3.0 and 2.4 < h < 3.0:
                c = rp.centroid
                holes2.append((round(c.x, 3), round(c.y, 3), round((w + h) / 2, 3)))
    holes2.sort()
    if len(holes2) == 4:
        us = sorted({h[0] for h in holes2})
        vs = sorted({h[1] for h in holes2})
        pitch_b = (round(us[-1] - us[0], 3), round(vs[-1] - vs[0], 3))
        dia2 = round(sum(h[2] for h in holes2) / 4, 3)
        rep.emit("board.mount.pattern.B", "mount pattern (derivation B: ATA Caseback)",
                 f"{pitch_b[0]:.3f} x {pitch_b[1]:.3f}", source="stl/ata/Caseback.stl",
                 note=f"4 x bore dia {dia2} at {holes2}")
    else:
        pitch_b = None
        print(f"  !! derivation B found {len(holes2)} candidate holes, expected 4")

    if pitch_a and pitch_b:
        # The two designs use opposite axis conventions; compare as unordered pairs.
        agree = sorted(pitch_a) == sorted(pitch_b)
        rep.emit("board.mount.pattern", "AGREED mounting-hole pattern",
                 f"{max(sorted(pitch_a)):.3f} x {min(sorted(pitch_a)):.3f}",
                 method="computed-from-mesh (2 independent sources)",
                 source="poc_bottom.stl + ata/Caseback.stl",
                 note=f"cross-check {'PASS' if agree else 'FAIL'}: A={pitch_a} B={pitch_b}")
        if not agree:
            print("  !! PATTERN MISMATCH - do not trust this datum until resolved")


def measure_board_pocket(ref: str, rep: Report):
    print("\n=== 3. BOARD POCKET / BACK WALL / DEPTH (ATA Caseback) ===")
    m = trimesh.load(os.path.join(ref, "stl/ata/Caseback.stl"), force="mesh")

    # Largest interior loop at mid-pocket = the board pocket outline.
    best = None
    for p in section_polygons(m, 1, -9.0):
        for ring in p.interiors:
            rp = Polygon(ring)
            if best is None or rp.area > best.area:
                best = rp
    if best is not None:
        b = best.bounds
        c = best.centroid
        rep.emit("board.pocket", "board pocket (ATA) U x V",
                 f"{b[2]-b[0]:.3f} x {b[3]-b[1]:.3f}", source="stl/ata/Caseback.stl",
                 note=f"centroid=({c.x:.3f},{c.y:.3f}) bounds={[round(v,3) for v in b]}")

    # Depth stack via ray casting at points clear of every cutout.
    for label, (x, z) in {
        "board back wall (A)": (25.0, -30.0),
        "board back wall (B)": (-32.0, -30.0),
        "board back wall (C)": (30.0, 30.0),
        "keyboard tray floor": (-70.0, 0.0),
        "keyboard tray floor (2)": (-70.0, 45.0),
        "divider wall": (-38.5, 0.0),
    }.items():
        iv = thickness_profile(m, 1, x, z)
        total = sum(b - a for a, b in iv)
        ident = "ref.ata." + label.split(" (")[0].replace(" ", "_")
        rep.emit(ident, label, f"{total:.3f}", source="stl/ata/Caseback.stl",
                 note=f"solid intervals along Y at (x={x}, z={z}): {iv}")


def measure_keyboard_pocket(ref: str, rep: Report):
    print("\n=== 4. Rii 518BT KEYBOARD POCKET (two independent derivations) ===")

    # --- ATA: the tray is open to the exterior, so use scan lines, not loops ----
    m = trimesh.load(os.path.join(ref, "stl/ata/Caseback.stl"), force="mesh")
    h = material_intervals(m, 1, 0.0, "h", 0.0, -106.0, 49.0)
    v_edge = material_intervals(m, 1, 0.0, "v", -95.0, -58.0, 62.0)
    if len(h) >= 2 and len(v_edge) >= 2:
        tray_u = round(h[1][0] - h[0][1], 3)      # divider inner face - left wall inner face
        tray_v = round(v_edge[1][0] - v_edge[0][1], 3)
        wall_l = round(h[0][1] - h[0][0], 3)
        rep.emit("kbd.pocket.A", "keyboard tray (derivation A: ATA Caseback)",
                 f"{tray_u:.3f} x {tray_v:.3f}", source="stl/ata/Caseback.stl",
                 note=f"h-scan v=0 -> {h}; v-scan u=-95 -> {v_edge}; wall={wall_l}")
        rep.emit("ref.ata.wall", "ATA outer wall thickness", f"{wall_l:.3f}",
                 source="stl/ata/Caseback.stl")

    # --- PoC: closed interior loops at the shell's inner face -------------------
    m2 = trimesh.load(os.path.join(ref, "stl/poc_top.stl"), force="mesh")
    rings = []
    for p in section_polygons(m2, 2, m2.bounds[0][2] + 0.05):
        for ring in p.interiors:
            rp = Polygon(ring)
            if rp.area > 1000:
                rings.append(rp)
    rings.sort(key=lambda r: r.bounds[0])
    if len(rings) == 2:
        board_c, kbd_c = rings           # board cavity is at lower X in this model
        bb, kb = board_c.bounds, kbd_c.bounds
        rep.emit("kbd.pocket.B", "keyboard cavity (derivation B: poc_top)",
                 f"{kb[2]-kb[0]:.3f} x {kb[3]-kb[1]:.3f}", source="stl/poc_top.stl",
                 note=f"bounds={[round(v,3) for v in kb]}")
        rep.emit("ref.poc.board_cavity", "board cavity (poc_top)",
                 f"{bb[2]-bb[0]:.3f} x {bb[3]-bb[1]:.3f}", source="stl/poc_top.stl")
        rep.emit("ref.poc.divider", "PoC divider wall between the two bays",
                 f"{kb[0]-bb[2]:.3f}", source="stl/poc_top.stl")

    # --- PoC top face: retention apertures ------------------------------------
    rings = []
    for p in section_polygons(m2, 2, m2.bounds[1][2] - 0.05):
        for ring in p.interiors:
            rp = Polygon(ring)
            if rp.area > 1000:
                rings.append(rp)
    rings.sort(key=lambda r: r.bounds[0])
    if len(rings) == 2:
        disp, kbd = rings
        db, kb = disp.bounds, kbd.bounds
        dc, kc = disp.centroid, kbd.centroid
        rep.emit("display.aperture.poc", "display aperture (poc_top outer face)",
                 f"{db[2]-db[0]:.3f} x {db[3]-db[1]:.3f}", source="stl/poc_top.stl",
                 note=f"centre=({dc.x:.3f},{dc.y:.3f})")
        rep.emit("kbd.aperture.poc", "keyboard retention aperture (poc_top outer face)",
                 f"{kb[2]-kb[0]:.3f} x {kb[3]-kb[1]:.3f}", source="stl/poc_top.stl",
                 note=f"centre=({kc.x:.3f},{kc.y:.3f})")
        rep.emit("layout.bay_gap.poc", "clear gap between display and keyboard apertures",
                 f"{kb[0]-db[2]:.3f}", source="stl/poc_top.stl",
                 note="measured along the device's long axis")


def measure_bezel(ref: str, rep: Report):
    print("\n=== 5. BEZEL / DISPLAY APERTURE / FASTENER PATTERN (ATA) ===")
    m = trimesh.load(os.path.join(ref, "stl/ata/Bezel.stl"), force="mesh")
    y0, y1 = m.bounds[0][1], m.bounds[1][1]
    rep.emit("ref.ata.bezel.thickness", "bezel thickness", f"{y1-y0:.3f}",
             source="stl/ata/Bezel.stl")

    for tag, y in (("inner", y0 + 0.05), ("outer", y1 - 0.05)):
        aperture, bores = None, []
        for p in section_polygons(m, 1, y):
            for ring in p.interiors:
                rp = Polygon(ring)
                b = rp.bounds
                w, h = b[2] - b[0], b[3] - b[1]
                if w > 40 and h > 40:
                    aperture = rp
                elif 2.8 < w < 3.6 and 2.8 < h < 3.6:
                    c = rp.centroid
                    bores.append((round(c.x, 3), round(c.y, 3), round((w + h) / 2, 3)))
        if aperture is not None:
            b = aperture.bounds
            c = aperture.centroid
            rep.emit(f"display.aperture.{tag}", f"display aperture ({tag} face)",
                     f"{b[2]-b[0]:.3f} x {b[3]-b[1]:.3f}", source="stl/ata/Bezel.stl",
                     note=f"centre=({c.x:.3f},{c.y:.3f})")
        if bores and tag == "inner":
            bores.sort()
            rep.emit("ref.ata.fastener.pattern", "M3 fastener positions (bezel clearance bores)",
                     f"4 x dia {bores[0][2]:.3f}", source="stl/ata/Bezel.stl",
                     note=f"positions (u,v) = {[(b[0], b[1]) for b in bores]}")

    # Heat-set insert bores in the caseback, at the same positions.
    m2 = trimesh.load(os.path.join(ref, "stl/ata/Caseback.stl"), force="mesh")
    ins = []
    for p in section_polygons(m2, 1, 0.068):
        for ring in p.interiors:
            rp = Polygon(ring)
            b = rp.bounds
            w, h = b[2] - b[0], b[3] - b[1]
            if 3.6 < w < 4.4 and 3.6 < h < 4.4:
                c = rp.centroid
                ins.append((round(c.x, 3), round(c.y, 3), round((w + h) / 2, 3)))
    if ins:
        ins.sort()
        rep.emit("ref.ata.insert.bore", "M3 heat-set insert bore diameter",
                 f"{ins[0][2]:.3f}", source="stl/ata/Caseback.stl",
                 note=f"{len(ins)} bores at {[(i[0], i[1]) for i in ins]}")
        depth = thickness_profile(m2, 1, ins[0][0], ins[0][1])
        rep.emit("ref.ata.insert.boss", "insert boss solid interval along thickness",
                 f"{depth}", unit="", source="stl/ata/Caseback.stl")


def measure_features(ref: str, rep: Report):
    print("\n=== 6. BACK-FACE FEATURES (battery bay, speaker grille, ports) ===")
    m = trimesh.load(os.path.join(ref, "stl/ata/Caseback.stl"), force="mesh")
    slots = []
    for p in section_polygons(m, 1, -12.95):
        for ring in p.interiors:
            rp = Polygon(ring)
            b = rp.bounds
            c = rp.centroid
            slots.append((round(b[2] - b[0], 3), round(b[3] - b[1], 3),
                          round(c.x, 3), round(c.y, 3)))
    grille = sorted([s for s in slots if 1.0 < s[0] < 1.8 and s[1] > 10])
    batt = [s for s in slots if s[0] > 15 and s[1] > 50]
    if batt:
        w, h, cx, cy = batt[0]
        rep.emit("batt.bay", "18650 holder cutout in back wall", f"{w:.3f} x {h:.3f}",
                 source="stl/ata/Caseback.stl", note=f"centre=({cx},{cy})")
    if grille:
        pitch = round(grille[1][2] - grille[0][2], 3) if len(grille) > 1 else 0.0
        rep.emit("audio.grille", "speaker grille slots",
                 f"{len(grille)} x {grille[0][0]:.3f} x {grille[0][1]:.3f}",
                 source="stl/ata/Caseback.stl", note=f"pitch={pitch}")

    # Cross-check the battery bay against the PoC, which must agree.
    m2 = trimesh.load(os.path.join(ref, "stl/poc_bottom.stl"), force="mesh")
    for p in section_polygons(m2, 2, m2.bounds[0][2] + 0.05):
        for ring in p.interiors:
            rp = Polygon(ring)
            b = rp.bounds
            w, h = b[2] - b[0], b[3] - b[1]
            if w > 15 and h > 50:
                rep.emit("batt.bay.crosscheck", "18650 holder cutout (PoC cross-check)",
                         f"{w:.3f} x {h:.3f}", source="stl/poc_bottom.stl")


def measure_dxf(ref: str, rep: Report):
    print("\n=== 7. ACRYLIC WINDOW TEMPLATE (exact 2D source) ===")
    path = os.path.join(ref, "stl/ata/plexiglass.dxf")
    if not os.path.exists(path):
        print("  !! plexiglass.dxf not present")
        return
    try:
        import ezdxf
    except ImportError:
        print("  !! ezdxf not installed, skipping")
        return
    doc = ezdxf.readfile(path)
    insunits = doc.header.get("$INSUNITS")
    unit_name = {1: "inches", 4: "millimetres", 0: "unitless"}.get(insunits, f"code {insunits}")
    xs, ys, nverts, narcs = [], [], 0, 0
    for e in doc.modelspace():
        if e.dxftype() == "LWPOLYLINE":
            pts = list(e.get_points("xyseb"))
            nverts += len(pts)
            narcs += sum(1 for p in pts if abs(p[4]) > 1e-12)
            xs += [p[0] for p in pts]
            ys += [p[1] for p in pts]
    if xs:
        rep.emit("window.outline", "acrylic display window outline",
                 f"{max(xs)-min(xs):.4f} x {max(ys)-min(ys):.4f}",
                 method="computed-from-dxf", source="stl/ata/plexiglass.dxf",
                 note=f"DXF {doc.dxfversion}, $INSUNITS={insunits} ({unit_name}), "
                      f"{nverts} vertices, {narcs} arc segments, "
                      f"bbox X[{min(xs):.4f},{max(xs):.4f}] Y[{min(ys):.4f},{max(ys):.4f}]")


def derive_display_active_area(rep: Report):
    """Demonstrate, numerically, why the active area must NOT be derived from
    the panel's nominal diagonal.

    This started life as a derivation. It is kept as a CHECK, because the
    derivation was wrong and the way it was wrong is instructive: "4.2 inch" is
    a marketing size, and using it here produced an active area about 0.5 mm too
    large in each axis - enough for a front-face aperture cut to it to be
    visibly oversized, and centred when the real one is not.
    """
    print("\n=== 8. DISPLAY ACTIVE AREA (derivation vs. the factory drawing) ===")
    official_w, official_h = 84.80, 63.60      # drawing: "84.80+/-0.10 LCD AA"
    px_w, px_h = 400, 300

    naive_pitch = (4.2 * 25.4) / math.hypot(px_w, px_h)
    naive_w, naive_h = px_w * naive_pitch, px_h * naive_pitch
    true_pitch = official_w / px_w
    true_diag = math.hypot(official_w, official_h)

    rep.emit("display.active", "display active area",
             f"{official_w:.2f} x {official_h:.2f}", method="official-drawing",
             source="Waveshare ESP32-S3-RLCD-4.2 drawing: '84.80+/-0.10 LCD AA'",
             note=f"pixel pitch {true_pitch:.4f} mm square; true diagonal "
                  f"{true_diag:.2f} mm = {true_diag/25.4:.3f} in, NOT 4.2")
    rep.emit("display.active.naive", "same, derived from a nominal 4.2 in",
             f"{naive_w:.3f} x {naive_h:.3f}", method="derived-from-spec",
             source="arithmetic on the marketing diagonal",
             note=f"REJECTED: overstates the panel by {naive_w-official_w:.3f} x "
                  f"{naive_h-official_h:.3f} mm. Recorded to document the trap.")

    if abs(naive_w - official_w) > 0.2:
        print(f"  -> nominal-diagonal derivation is off by "
              f"{naive_w-official_w:.3f} x {naive_h-official_h:.3f} mm; "
              f"the drawing is authoritative")



# --------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--reference", required=True,
                    help="path to a checkout of the reference enclosure repository")
    ap.add_argument("--json", help="write the machine-readable report here")
    args = ap.parse_args()

    if not os.path.isdir(args.reference):
        sys.exit(f"reference path not found: {args.reference}")

    rep = Report()
    print("=" * 96)
    print("cYbErDeCk reference metrology")
    print(f"reference tree: {args.reference}")
    print("=" * 96)

    measure_envelopes(args.reference, rep)
    measure_board_pattern(args.reference, rep)
    measure_board_pocket(args.reference, rep)
    measure_keyboard_pocket(args.reference, rep)
    measure_bezel(args.reference, rep)
    measure_features(args.reference, rep)
    measure_dxf(args.reference, rep)
    derive_display_active_area(rep)

    print("\n" + "=" * 96)
    print(f"{len(rep.datums)} datums measured")
    print("=" * 96)

    if args.json:
        os.makedirs(os.path.dirname(args.json) or ".", exist_ok=True)
        with open(args.json, "w") as fh:
            json.dump({"datums": [asdict(d) for d in rep.datums]}, fh, indent=2)
        print(f"wrote {args.json}")


if __name__ == "__main__":
    main()
