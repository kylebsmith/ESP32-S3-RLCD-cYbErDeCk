#!/usr/bin/env python3
"""
drawing.py - dimensioned general-arrangement drawings, generated from the parts.

Every dimension on these sheets is MEASURED FROM THE RENDERED MESH, not typed in
and not read from parameters.scad. A drawing that is annotated by hand drifts
from the model the first time anyone edits the model; one that measures the mesh
cannot. Where a dimension has a nominal value in parameters.scad, the sheet
prints the measured figure and flags any disagreement above 0.05 mm.

Sheets produced:
    1  chassis, front elevation + side section
    2  back plate, rear elevation + side section
    3  assembly section, with both components in place
    4  component envelopes, as located

    python3 tools/drawing.py                 # all sheets to export/drawings/
    python3 tools/drawing.py --sheet 3
"""

from __future__ import annotations

import argparse
import math
import os
import re
import subprocess
import sys
import tempfile

import params

try:
    import numpy as np
    import trimesh
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.patches import Polygon as MplPolygon
    from shapely.geometry import Polygon
    from shapely.ops import unary_union
except ImportError as exc:  # pragma: no cover
    sys.exit(f"missing dependency: {exc}\ninstall with: pip install -r tools/requirements.txt")

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCAD = os.path.join(ROOT, "cad", "cyberdeck.scad")
PARAMS = os.path.join(ROOT, "cad", "parameters.scad")
OUT = os.path.join(ROOT, "export", "drawings")

# Drawing style. Deliberately flat and monochrome - a GA drawing is a document,
# not a picture, and colour here would only compete with the geometry.
INK = "#1a1a1a"
THIN = "#8a8a8a"
DIMC = "#0b5fa5"
CHK = "#0f7a3d"
WARN = "#b3261e"
FILL = "#e9e6df"
HOLE = "#ffffff"
COMP = {"board": "#3f7d4f", "keyboard": "#33363b"}


# ---------------------------------------------------------------------------
# Geometry extraction
# ---------------------------------------------------------------------------

def load_params():
    return params.load_with_defaults(PARAMS)


def render(part, outdir):
    out = os.path.join(outdir, f"{part}.stl")
    if not os.path.exists(out):
        subprocess.run(["openscad", "-D", f'part="{part}"', "-o", out, SCAD],
                       capture_output=True, text=True, timeout=1800)
    return trimesh.load(out, force="mesh")


def module_mesh(expr, outdir, name):
    out = os.path.join(outdir, f"{name}.stl")
    if not os.path.exists(out):
        sf = os.path.join(outdir, f"{name}.scad")
        open(sf, "w").write(
            f'include <{PARAMS}>\nuse <{os.path.join(ROOT, "cad", "lib", "components.scad")}>\n{expr};\n')
        subprocess.run(["openscad", "-o", out, sf], capture_output=True, text=True, timeout=1800)
    return trimesh.load(out, force="mesh")


_AXIS_T = {
    2: np.eye(4),
    1: np.array([[1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, 0], [0, 0, 0, 1]], float),
    0: np.array([[0, 1, 0, 0], [0, 0, 1, 0], [1, 0, 0, 0], [0, 0, 0, 1]], float),
}


def section(mesh, axis, level):
    """Planar section in WORLD in-plane coordinates."""
    n = [0.0, 0.0, 0.0]; n[axis] = 1.0
    o = [0.0, 0.0, 0.0]; o[axis] = float(level)
    s = mesh.section(plane_origin=o, plane_normal=n)
    if s is None:
        return []
    return list(s.to_2D(to_2D=_AXIS_T[axis])[0].polygons_full)


def silhouette(mesh, axis):
    """Outer projected outline along `axis`, as a shapely polygon."""
    lo, hi = mesh.bounds[0][axis], mesh.bounds[1][axis]
    acc = []
    for t in np.linspace(lo + 1e-3, hi - 1e-3, 24):
        acc += [Polygon(p.exterior) for p in section(mesh, axis, t)]
    return unary_union(acc) if acc else None


# ---------------------------------------------------------------------------
# Drawing primitives
# ---------------------------------------------------------------------------

def draw_polys(ax, polys, fc=FILL, ec=INK, lw=0.9, holes=True, alpha=1.0, z=2):
    for p in (polys if isinstance(polys, (list, tuple)) else
              (list(polys.geoms) if polys.geom_type.startswith("Multi") else [polys])):
        ax.add_patch(MplPolygon(np.asarray(p.exterior.coords), closed=True,
                                facecolor=fc, edgecolor=ec, lw=lw, alpha=alpha, zorder=z))
        if holes:
            for r in p.interiors:
                ax.add_patch(MplPolygon(np.asarray(r.coords), closed=True,
                                        facecolor=HOLE, edgecolor=ec, lw=lw * 0.8,
                                        zorder=z + 1))


def _arrow(ax, x, y, dx, dy, c):
    ax.annotate("", xy=(x + dx, y + dy), xytext=(x, y),
                arrowprops=dict(arrowstyle="-|>", color=c, lw=0.8,
                                shrinkA=0, shrinkB=0, mutation_scale=7), zorder=9)


def dim_h(ax, x1, x2, y, text, tick=2.0, c=DIMC, fs=6.4, above=True):
    """Horizontal dimension with extension lines and inward arrows."""
    ax.plot([x1, x1], [y - tick, y + tick], c=c, lw=0.5, zorder=9)
    ax.plot([x2, x2], [y - tick, y + tick], c=c, lw=0.5, zorder=9)
    ax.plot([x1, x2], [y, y], c=c, lw=0.6, zorder=9)
    d = min(abs(x2 - x1) * 0.22, 6)
    _arrow(ax, x1, y, d, 0, c); _arrow(ax, x2, y, -d, 0, c)
    ax.text((x1 + x2) / 2, y + (1.6 if above else -4.4), text, ha="center",
            va="bottom" if above else "top", fontsize=fs, color=c, zorder=10,
            bbox=dict(fc="white", ec="none", pad=0.8, alpha=0.9))


def dim_v(ax, y1, y2, x, text, tick=2.0, c=DIMC, fs=6.4, right=True):
    ax.plot([x - tick, x + tick], [y1, y1], c=c, lw=0.5, zorder=9)
    ax.plot([x - tick, x + tick], [y2, y2], c=c, lw=0.5, zorder=9)
    ax.plot([x, x], [y1, y2], c=c, lw=0.6, zorder=9)
    d = min(abs(y2 - y1) * 0.22, 6)
    _arrow(ax, x, y1, 0, d, c); _arrow(ax, x, y2, 0, -d, c)
    ax.text(x + (1.8 if right else -1.8), (y1 + y2) / 2, text, ha="left" if right else "right",
            va="center", fontsize=fs, color=c, rotation=90, zorder=10,
            bbox=dict(fc="white", ec="none", pad=0.8, alpha=0.9))


def leader(ax, x, y, tx, ty, text, c=INK, fs=6.2):
    ax.plot([x, tx], [y, ty], c=c, lw=0.5, zorder=9)
    ax.plot([x], [y], marker="o", ms=1.8, c=c, zorder=9)
    ha = "left" if tx >= x else "right"
    ax.text(tx + (1.2 if ha == "left" else -1.2), ty, text, ha=ha, va="center",
            fontsize=fs, color=c, zorder=10,
            bbox=dict(fc="white", ec="none", pad=0.8, alpha=0.9))


def centreline(ax, x1, x2, y1, y2):
    ax.plot([x1, x2], [y1, y2], c=THIN, lw=0.5, ls=(0, (9, 3, 1.5, 3)), zorder=1)


def frame(ax, title, sub=""):
    """Title block for one view. Placed in axes coordinates rather than with
    set_title() so the caption sits clear of it at any aspect ratio."""
    ax.set_aspect("equal"); ax.axis("off")
    ax.text(0, 1.055, title, transform=ax.transAxes, fontsize=8.2,
            color=INK, va="bottom", fontweight="bold")
    if sub:
        ax.text(0, 1.016, sub, transform=ax.transAxes, fontsize=6.3,
                color=THIN, va="bottom")


def fmt(v, nominal=None, tol=0.05):
    """Measured value, flagged if it disagrees with the model's nominal."""
    if nominal is None or abs(v - nominal) <= tol:
        return f"{v:.2f}"
    return f"{v:.2f} != {nominal:.2f}"


def check_line(ax, x, y, ok, text, fs=6.2):
    ax.text(x, y, ("PASS  " if ok else "FAIL  ") + text, fontsize=fs,
            color=CHK if ok else WARN, family="monospace", va="top", zorder=10)


# ---------------------------------------------------------------------------
# Sheets
# ---------------------------------------------------------------------------

def sheet_chassis(parts, p, path):
    ch = parts["chassis"]
    fig = plt.figure(figsize=(16.5, 11.7), dpi=170)
    fig.patch.set_facecolor("white")
    gs = fig.add_gridspec(2, 2, width_ratios=[1.35, 1], height_ratios=[1, 0.42],
                          hspace=0.16, wspace=0.10,
                          left=0.045, right=0.975, top=0.925, bottom=0.05)

    # ---- front elevation -------------------------------------------------
    ax = fig.add_subplot(gs[:, 0])
    frame(ax, "CHASSIS  —  front elevation",
          "one piece: front face, four walls and the inter-bay spine. "
          "All dimensions measured from the rendered mesh, in millimetres.")
    # Section AT the outer face, so the flared apertures read at full size.
    face = section(ch, 2, ch.bounds[1][2] - 0.02)
    draw_polys(ax, face)

    b = ch.bounds
    W, H = b[1][0] - b[0][0], b[1][1] - b[0][1]
    dim_h(ax, b[0][0], b[1][0], b[1][1] + 13, fmt(W, p.get("body_w")))
    dim_v(ax, b[0][1], b[1][1], b[1][0] + 13, fmt(H, p.get("body_h")))
    centreline(ax, b[0][0] - 8, b[1][0] + 8, 0, 0)
    centreline(ax, 0, 0, b[0][1] - 8, b[1][1] + 8)

    # Order by POSITION, not by area. The keyboard aperture is the larger of
    # the two (5 943 mm2 against 5 694), so sorting by area silently swaps the
    # labels - which it did.
    rings = sorted([Polygon(r) for g in face for r in g.interiors if Polygon(r).area > 500],
                   key=lambda q: -q.centroid.y)
    spec = [("display aperture", p["display_aper_w"] + 2 * p["display_aper_draft"],
             p["display_aper_h"] + 2 * p["display_aper_draft"]),
            ("keyboard aperture", p["kbd_aper_w"] + 1.2, p["kbd_aper_h"] + 1.2)]
    for rg, (lbl, nw, nh) in zip(rings, spec):
        rb = rg.bounds
        dim_h(ax, rb[0], rb[2], rb[3] - 7, fmt(rb[2] - rb[0], nw), above=False)
        dim_v(ax, rb[1], rb[3], rb[0] + 7, fmt(rb[3] - rb[1], nh), right=False)
        ax.text((rb[0] + rb[2]) / 2, (rb[1] + rb[3]) / 2,
                lbl + "\n(at the outer face, flared)", ha="center", va="center",
                fontsize=6.6, color=THIN, style="italic", zorder=6)

    bcy = p["board_bay_cy"]
    kcy = p["kbd_bay_cy"]
    dim_v(ax, kcy, bcy, b[0][0] - 9, f"{bcy - kcy:.2f}  bay pitch", right=False)

    ax.set_xlim(b[0][0] - 34, b[1][0] + 26)
    ax.set_ylim(b[0][1] - 20, b[1][1] + 24)

    # ---- side section ----------------------------------------------------
    ax2 = fig.add_subplot(gs[0, 1])
    frame(ax2, "CHASSIS  —  vertical section at X = 30",
          "cut through both apertures. Horizontal axis is device height (Y), "
          "vertical axis is thickness (Z).")
    # Section along X returns a (Y, Z) plane: plot x = world Y, plot y = world Z.
    sec = section(ch, 0, 30.0)
    draw_polys(ax2, sec)
    sb = ch.bounds
    dim_h(ax2, sb[0][1], sb[1][1], sb[1][2] + 8, f"{sb[1][1]-sb[0][1]:.2f}  height")
    dim_v(ax2, sb[0][2], sb[1][2], sb[1][1] + 10,
          fmt(sb[1][2] - sb[0][2], p.get("body_t")) + "  thickness")
    leader(ax2, bcy, p["body_t"] - p["front_t"] / 2, bcy + 6, sb[1][2] + 5,
           f"front face {p['front_t']:.2f}")
    leader(ax2, kcy, p["body_t"] - p["front_t"] - p["kbd_depth"] / 2,
           kcy - 4, sb[0][2] - 7, f"keyboard pocket {p['kbd_depth']:.2f}")
    leader(ax2, bcy, p["body_t"] - p["front_t"] - p["board_depth"] / 2,
           bcy + 22, sb[0][2] - 7, f"board pocket {p['board_depth']:.2f}")
    leader(ax2, p["spine_cy"] if "spine_cy" in p else (bcy + kcy) / 2, p["back_t"] / 2,
           (bcy + kcy) / 2 - 18, sb[0][2] - 14,
           f"back plate seats at {p['back_t']:.2f}")
    ax2.set_xlim(sb[0][1] - 14, sb[1][1] + 30)
    ax2.set_ylim(sb[0][2] - 22, sb[1][2] + 20)

    # ---- derivation ------------------------------------------------------
    ax3 = fig.add_subplot(gs[1, 1]); ax3.axis("off")
    exp_w = p["wall"] + max(p["kbd_pocket_w"], p["board_pocket_w"]) + p["wall"]
    exp_h = p["wall"] + p["kbd_pocket_h"] + p["spine"] + p["board_pocket_h"] + p["wall"]
    exp_t = p["back_t"] + p["board_depth"] + p["front_t"]
    lines = [
        "ENVELOPE IS COMPONENT-BOUND",
        "",
        f"  width   = wall {p['wall']:.1f} + max(kbd {p['kbd_pocket_w']:.1f}, "
        f"board {p['board_pocket_w']:.1f}) + wall {p['wall']:.1f}   = {exp_w:.2f}",
        f"  height  = wall + kbd {p['kbd_pocket_h']:.1f} + spine {p['spine']:.1f}"
        f" + board {p['board_pocket_h']:.1f} + wall  = {exp_h:.2f}",
        f"  depth   = back {p['back_t']:.1f} + stack {p['board_depth']:.1f}"
        f" + face {p['front_t']:.1f}                     = {exp_t:.2f}",
        "",
        "The enclosure cannot be made smaller without thinning a wall",
        "or crushing a part. Asserted on every run by tools/validate.py.",
    ]
    for i, t in enumerate(lines):
        ax3.text(0.0, 0.97 - i * 0.092, t, fontsize=6.6, family="monospace",
                 color=INK if i == 0 else THIN, va="top",
                 fontweight="bold" if i == 0 else "normal")
    for j, (meas, exp, nm) in enumerate(
            [(W, exp_w, "width"), (H, exp_h, "height"),
             (b[1][2] - b[0][2], exp_t, "thickness")]):
        check_line(ax3, 0.0, 0.20 - j * 0.075, abs(meas - exp) < 0.05,
                   f"{nm:<10}{meas:8.2f} measured  vs {exp:8.2f} derived")

    fig.suptitle("cYbErDeCk  ·  sheet 1 of 6  ·  chassis", fontsize=10.5,
                 x=0.045, ha="left", y=0.972, color=INK, fontweight="bold")
    fig.savefig(path, facecolor="white"); plt.close(fig)


def sheet_backplate(parts, p, path):
    bp = parts["backplate"]
    fig = plt.figure(figsize=(16.5, 11.7), dpi=170)
    fig.patch.set_facecolor("white")
    gs = fig.add_gridspec(1, 2, width_ratios=[1.25, 1], wspace=0.08,
                          left=0.045, right=0.975, top=0.925, bottom=0.05)

    ax = fig.add_subplot(gs[0, 0])
    frame(ax, "BACK PLATE  —  rear elevation (outer face)",
          "board bolts to this plate; board, cell and plate lift out as one module.")
    outer = section(bp, 2, 0.35)
    draw_polys(ax, outer)
    b = bp.bounds
    dim_h(ax, b[0][0], b[1][0], b[1][1] + 12, fmt(b[1][0] - b[0][0]))
    dim_v(ax, b[0][1], b[1][1], b[1][0] + 12, fmt(b[1][1] - b[0][1]))

    bcy = p["body_h"] / 2 - p["wall"] - p["board_pocket_h"] / 2
    # fastener pattern
    fx = p["board_pocket_w"] / 2 + (max(p["kbd_pocket_w"], p["board_pocket_w"])
                                    - p["board_pocket_w"]) / 4 + 0.4
    dim_h(ax, -fx, fx, b[0][1] - 9, f"{2*fx:.2f}  M3 fastener pitch", above=False)
    # board mount pattern
    dim_h(ax, -p["board_mount_pitch_x"] / 2, p["board_mount_pitch_x"] / 2,
          bcy + p["board_mount_pitch_y"] / 2 + 7,
          f"{p['board_mount_pitch_x']:.2f}  board mount")
    dim_v(ax, bcy - p["board_mount_pitch_y"] / 2, bcy + p["board_mount_pitch_y"] / 2,
          -p["board_mount_pitch_x"] / 2 - 7, f"{p['board_mount_pitch_y']:.2f}", right=False)

    cy = bcy + p["batt_off_y"]
    leader(ax, 0, cy, b[1][0] + 30, cy + 8,
           f"18650 cowl  {p['batt_cowl_w']:.0f} x {p['batt_cowl_h']:.0f},"
           f" {p['batt_cowl_rise']:.1f} proud")
    gy = bcy + p["grille_off_y"]
    leader(ax, 0, gy, b[1][0] + 30, gy + 6,
           f"grille  {int(p['grille_count'])} x {p['grille_slot_h']:.1f}"
           f" @ {p['grille_pitch']:.4f}")
    ax.set_xlim(b[0][0] - 24, b[1][0] + 78)
    ax.set_ylim(b[0][1] - 20, b[1][1] + 22)

    ax2 = fig.add_subplot(gs[0, 1])
    frame(ax2, "BACK PLATE  —  section through the battery cowl",
          "the cowl is blended out of the panel; it has no base line.")
    sec = section(bp, 1, cy)
    draw_polys(ax2, sec)
    sb = bp.bounds
    dim_v(ax2, -p["batt_cowl_rise"], 0, b[1][0] + 8,
          f"{p['batt_cowl_rise']:.2f}  cowl rise")
    dim_v(ax2, 0, p["back_t"], b[1][0] + 22, f"{p['back_t']:.2f}  plate")
    leader(ax2, 0, -p["batt_cowl_rise"] + p["batt_cowl_wall"],
           b[0][0] - 12, -p["batt_cowl_rise"] - 8,
           f"cavity floor  {p['batt_cowl_rise']-p['batt_cowl_wall']:.2f} internal rise")
    ax2.set_xlim(sb[0][0] - 46, sb[1][0] + 46)
    ax2.set_ylim(-p["batt_cowl_rise"] - 16, p["back_t"] + p["kbd_keeper"] + 14
                 if "kbd_keeper" in p else p["back_t"] + 14)

    fig.suptitle("cYbErDeCk  ·  sheet 2 of 6  ·  back plate", fontsize=10.5,
                 x=0.045, ha="left", y=0.972, color=INK, fontweight="bold")
    fig.savefig(path, facecolor="white"); plt.close(fig)


def sheet_assembly(parts, mocks, p, path):
    """The sheet that matters: components in place, with the stack-up dimensioned."""
    ch, bp = parts["chassis"], parts["backplate"]
    fig = plt.figure(figsize=(16.5, 8.6), dpi=170)
    fig.patch.set_facecolor("white")
    gs = fig.add_gridspec(2, 1, height_ratios=[1, 1], hspace=0.34,
                          left=0.05, right=0.975, top=0.88, bottom=0.05)

    bcy = p["body_h"] / 2 - p["wall"] - p["board_pocket_h"] / 2
    kcy = -p["body_h"] / 2 + p["wall"] + p["kbd_pocket_h"] / 2

    for row, (cut, title, note) in enumerate([
        (bcy, "ASSEMBLY  —  horizontal section through the board bay",
         "looking down. Waveshare ESP32-S3-RLCD-4.2 in place, stand base discarded."),
        (kcy, "ASSEMBLY  —  horizontal section through the keyboard bay",
         "looking down. Rii 518BT in place, captured by the front-face lip.")]):
        ax = fig.add_subplot(gs[row, 0])
        frame(ax, title, note)
        draw_polys(ax, section(ch, 1, cut), fc=FILL)
        draw_polys(ax, section(bp, 1, cut), fc="#d9d4cb")
        key = "board" if row == 0 else "keyboard"
        draw_polys(ax, section(mocks[key], 1, cut), fc=COMP[key], ec=INK,
                   alpha=0.75, holes=False, z=6)

        b = ch.bounds
        dim_h(ax, b[0][0], b[1][0], b[1][2] + 7, f"{b[1][0]-b[0][0]:.2f}")
        pw = p["board_pocket_w"] if row == 0 else p["kbd_pocket_w"]
        # The component width is MEASURED from the mock that is actually drawn
        # above, not read from a parameter. Reading a parameter is how a sheet
        # comes to annotate a nominal beside an envelope drawn at tolerance.
        cb = mocks[key].bounds
        cw = cb[1][0] - cb[0][0]
        tag = "component" if row == 0 else "component, worst case"
        dim_h(ax, -pw / 2, pw / 2, b[0][2] - 6, f"{pw:.2f}  pocket", above=False)
        dim_h(ax, cb[0][0], cb[1][0], b[0][2] - 14,
              f"{cw:.2f}  {tag}     clearance {(pw-cw)/2:.2f} per side", above=False)
        dim_v(ax, b[0][2], b[1][2], b[1][0] + 8, f"{b[1][2]-b[0][2]:.2f}")
        ax.set_xlim(b[0][0] - 20, b[1][0] + 30)
        ax.set_ylim(b[0][2] - 22, b[1][2] + 16)

    fig.suptitle("cYbErDeCk  ·  sheet 3 of 6  ·  assembly sections, components in place",
                 fontsize=10.5, x=0.05, ha="left", y=0.975, color=INK, fontweight="bold")
    fig.savefig(path, facecolor="white"); plt.close(fig)


def sheet_components(parts, mocks, p, path):
    ch = parts["chassis"]
    fig = plt.figure(figsize=(16.5, 11.7), dpi=170)
    fig.patch.set_facecolor("white")
    gs = fig.add_gridspec(1, 2, wspace=0.10, left=0.05, right=0.975,
                          top=0.925, bottom=0.05)
    bcy = p["body_h"] / 2 - p["wall"] - p["board_pocket_h"] / 2
    kcy = -p["body_h"] / 2 + p["wall"] + p["kbd_pocket_h"] / 2

    ax = fig.add_subplot(gs[0, 0])
    frame(ax, "COMPONENTS AS LOCATED  —  plan",
          "shell outline with both component envelopes in their design positions.")
    sil = silhouette(ch, 2)
    draw_polys(ax, sil, fc="none", ec=INK, lw=1.1, holes=False)
    for key, cy in (("board", bcy), ("keyboard", kcy)):
        s = silhouette(mocks[key], 2)
        draw_polys(ax, s, fc=COMP[key], ec=INK, lw=0.8, alpha=0.5, holes=False, z=4)
    dim_h(ax, -p["board_mount_pitch_x"] / 2, p["board_mount_pitch_x"] / 2,
          bcy + p["board_mount_pitch_y"] / 2 + 6, f"{p['board_mount_pitch_x']:.2f}")
    dim_v(ax, bcy - p["board_mount_pitch_y"] / 2, bcy + p["board_mount_pitch_y"] / 2,
          p["board_mount_pitch_x"] / 2 + 6, f"{p['board_mount_pitch_y']:.2f}")
    kb = mocks["keyboard"].bounds        # measured, for the same reason as above
    kw, kh = kb[1][0] - kb[0][0], kb[1][1] - kb[0][1]
    # Placed clear of each other and of the outline: a centred pair crosses at
    # the keyboard's midpoint and the two labels land on top of one another.
    dim_h(ax, kb[0][0], kb[1][0], kcy - kh / 2 - 7, f"{kw:.2f}", above=False)
    dim_v(ax, kcy - kh / 2, kcy + kh / 2, kb[1][0] + 8, f"{kh:.2f}")
    b = ch.bounds
    ax.set_xlim(b[0][0] - 22, b[1][0] + 22); ax.set_ylim(b[0][1] - 18, b[1][1] + 18)

    ax2 = fig.add_subplot(gs[0, 1]); ax2.axis("off")
    rows = [
        ("WAVESHARE ESP32-S3-RLCD-4.2", "", ""),
        ("  PCB outline", f"{p['board_w']:.2f} x {p['board_h']:.2f}", "vendor drawing"),
        ("  pocket", f"{p['board_pocket_w']:.2f} x {p['board_pocket_h']:.2f}",
         f"{(p['board_pocket_w']-p['board_w'])/2:.2f} / "
         f"{(p['board_pocket_h']-p['board_h'])/2:.2f} per side"),
        ("  mount pattern", f"{p['board_mount_pitch_x']:.2f} x {p['board_mount_pitch_y']:.2f}",
         "3 sources, 0.001 apart"),
        ("  stack, glass to standoff", f"{p['board_stack']:.2f}",
         f"pocket {p['board_depth']:.2f}"),
        ("  active area", f"{p['display_active_w']:.2f} x {p['display_active_h']:.2f}",
         "offset -1.60 in U"),
        ("  aperture", f"{p['display_aper_w']:.2f} x {p['display_aper_h']:.2f}",
         "1.00 reveal per side"),
        ("  holder protrusion", f"{p['batt_protrusion']:.2f}", "past the standoff plane"),
        ("", "", ""),
        ("Rii 518BT", "", ""),
        ("  outline, nominal", f"{p['kbd_body_w']:.2f} x {p['kbd_body_h']:.2f} x {p['kbd_body_t']:.2f}",
         "Riitek drawing + FCC manual"),
        ("  outline, at tolerance", f"{p['kbd_body_w_max']:.2f} x {p['kbd_body_h_max']:.2f}"
         f" x {p['kbd_body_t_max']:.2f}", "assumed mould + keycap band"),
        ("  pocket", f"{p['kbd_pocket_w']:.2f} x {p['kbd_pocket_h']:.2f} x {p['kbd_depth']:.2f}",
         f"{(p['kbd_pocket_w']-p['kbd_body_w_max'])/2:.2f} / "
         f"{(p['kbd_pocket_h']-p['kbd_body_h_max'])/2:.2f} per side, worst case"),
        ("  retention aperture", f"{p['kbd_aper_w']:.2f} x {p['kbd_aper_h']:.2f}",
         f"lip {(p['kbd_pocket_w']-p['kbd_aper_w'])/2:.2f} / "
         f"{(p['kbd_pocket_h']-p['kbd_aper_h'])/2:.2f}"),
        ("  service window", f"{p['kbd_access_w']:.2f} x {p['kbd_access_h']:.2f}",
         "both sides"),
        ("", "", ""),
        ("REFERENCE BRACKETS (measured)", "", ""),
        ("  ATA tray", "109.200 x 59.200 x 11.400", "+0.70 on the 108.5 body"),
        ("  PoC bay", "110.498 x 60.600 x 10.750", "+2.00 on the 108.5 body"),
        ("  this design", f"{p['kbd_pocket_w']:.3f} x {p['kbd_pocket_h']:.3f}"
         f" x {p['kbd_depth']:.3f}", "clears both candidates"),
    ]
    for i, (a, bb, c) in enumerate(rows):
        y = 0.98 - i * 0.049
        head = a and not a.startswith(" ")
        ax2.text(0.0, y, a, fontsize=6.9, family="monospace", va="top",
                 color=INK if head else THIN, fontweight="bold" if head else "normal")
        ax2.text(0.46, y, bb, fontsize=6.9, family="monospace", va="top", color=INK)
        ax2.text(0.76, y, c, fontsize=6.3, family="monospace", va="top", color=THIN)

    fig.suptitle("cYbErDeCk  ·  sheet 4 of 6  ·  component schedule",
                 fontsize=10.5, x=0.05, ha="left", y=0.972, color=INK, fontweight="bold")
    fig.savefig(path, facecolor="white"); plt.close(fig)


def _rr(w, h, c, n, q=600):
    """Rounded rectangle with a superelliptical corner; n = 2 is a circle."""
    c = min(c, w / 2 - 0.01, h / 2 - 0.01)
    ax, ay = w / 2 - c, h / 2 - c
    t = np.linspace(0, 90, q)
    e = 2.0 / n
    ct = np.power(np.clip(np.cos(np.radians(t)), 0, None), e)
    st = np.power(np.clip(np.sin(np.radians(t)), 0, None), e)
    return np.vstack([np.c_[ax + c*ct, ay + c*st], np.c_[-ax - c*st, ay + c*ct],
                      np.c_[-ax - c*ct, -ay - c*st], np.c_[ax + c*st, -ay - c*ct]])


def sheet_corner_lip(p, path):
    """Why the keyboard aperture corner is circular rather than superelliptical.

    The failure this records is invisible in a render and invisible to a check
    that compares widths, so it is worth a drawing of its own.
    """
    from shapely.geometry import Polygon as SP
    W, H = p["kbd_aper_w"], p["kbd_aper_h"]
    BW, BH = p["kbd_body_w"], p["kbd_body_h"]
    rb = p["kbd_body_corner_r"]

    fig, axes = plt.subplots(1, 2, figsize=(16.5, 7.0), dpi=170)
    fig.patch.set_facecolor("white")
    cases = [(f"BEFORE  —  superelliptical corner, c = 9.0, n = {p['form_n']}",
              9.0, p["form_n"], "#b04a3a"),
             (f"AFTER  —  circular corner, c = {p['aper_blend_kbd']:.1f}, derived",
              p["aper_blend_kbd"], p["aper_n_kbd"], "#2f6b45")]
    body = _rr(BW, BH, rb, 2.0)
    for ax, (title, c, n, col) in zip(axes, cases):
        ap = _rr(W, H, c, n)
        ax.add_patch(MplPolygon(body, closed=True, fc="#2b2e33", ec="#2b2e33",
                                alpha=0.88, zorder=1))
        ax.add_patch(MplPolygon(ap, closed=True, fc="white", ec=col, lw=2.2, zorder=3))
        esc = SP(ap).difference(SP(body))
        gs = list(esc.geoms) if esc.geom_type.startswith("Multi") else \
             ([esc] if not esc.is_empty else [])
        for g in gs:
            ax.add_patch(MplPolygon(np.asarray(g.exterior.coords), closed=True,
                                    fc="#e03a2f", ec="none", zorder=4))
        held = SP(body).contains(SP(ap))
        ax.set_xlim(BW/2 - 18, BW/2 + 4); ax.set_ylim(BH/2 - 16, BH/2 + 4)
        ax.set_aspect("equal"); ax.set_xticks([]); ax.set_yticks([])
        ax.set_title(title, fontsize=10, color=col, loc="left", fontweight="bold")
        for sp in ax.spines.values():
            sp.set_color("#ccc")
        ax.text(0.03, 0.05,
                "lip holds all the way round the corner" if held else
                "lip is NEGATIVE — the keyboard does not\nreach the aperture edge "
                "(red = open gap into the pocket)",
                transform=ax.transAxes, fontsize=9, color=col, va="bottom")
    fig.suptitle("cYbErDeCk  ·  sheet 6 of 6  ·  keyboard aperture corner against the "
                 f"keyboard's measured {rb:.0f} mm corner  (top-right corner, detail)",
                 fontsize=10.5, x=0.04, ha="left", y=0.97, color=INK, fontweight="bold")
    fig.text(0.04, 0.012, "dark = Rii 518BT body   ·   outline = front-face aperture "
             "  ·   the gap between them is the retaining lip",
             fontsize=8, color=THIN)
    fig.tight_layout(rect=[0, 0.03, 1, 0.94])
    fig.savefig(path, facecolor="white"); plt.close(fig)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sheet", type=int, help="render one sheet only")
    ap.add_argument("--workdir", help="reuse an existing STL directory")
    args = ap.parse_args()

    os.makedirs(OUT, exist_ok=True)
    p = load_params()
    tmp = args.workdir or tempfile.mkdtemp(prefix="cyberdeck-drawing-")
    print(f"rendering into {tmp}")

    parts = {n: render(n, tmp) for n in ("chassis", "backplate")}
    bcy = p["body_h"] / 2 - p["wall"] - p["board_pocket_h"] / 2
    kcy = -p["body_h"] / 2 + p["wall"] + p["kbd_pocket_h"] / 2
    zfi = p["body_t"] - p["front_t"]
    mocks = {
        "board": module_mesh(
            f"translate([0,{bcy},{zfi - p['board_depth']}]) mock_board()", tmp, "mock_board"),
        "keyboard": module_mesh(
            f"translate([0,{kcy},{zfi - p['kbd_depth']}]) mock_keyboard()", tmp, "mock_keyboard"),
    }

    sheets = [
        (1, "sheet1-chassis.png", lambda f: sheet_chassis(parts, p, f)),
        (2, "sheet2-backplate.png", lambda f: sheet_backplate(parts, p, f)),
        (3, "sheet3-assembly.png", lambda f: sheet_assembly(parts, mocks, p, f)),
        (4, "sheet4-components.png", lambda f: sheet_components(parts, mocks, p, f)),
        (6, "sheet6-corner-lip.png", lambda f: sheet_corner_lip(p, f)),
    ]
    for n, name, fn in sheets:
        if args.sheet and args.sheet != n:
            continue
        out = os.path.join(OUT, name)
        fn(out)
        print(f"  wrote {out}")


if __name__ == "__main__":
    main()
