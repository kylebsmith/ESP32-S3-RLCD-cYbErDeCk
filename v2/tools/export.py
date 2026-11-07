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

AND THE MESH GETS THE SAME TREATMENT, FOR THE SAME REASON
---------------------------------------------------------
A mesh can be loud or quiet in exactly the same way. Three failures this
project has already paid for are invisible in a picture and invisible in a
file size:

  * N SEPARATE BODIES. carrier.scad's keyboard pocket deleted the bottom of
    all eleven plate bosses and left them floating in mid-air. The only reason
    it was caught is that the mesh came back as twelve bodies. body_count is
    not available here -- trimesh 5.2.0 reaches for scipy and there is no
    scipy in this container -- so BODIES() is a union-find over vertex
    adjacency, which needs nothing.
  * A DROPPED -D. knob.scad is TWO parts behind one file, and a `variant`
    define that never arrives still renders, still exports, and silently ships
    the encoder knob twice. So the variant is not trusted: the bore's measured
    minimum radius has to be the D-flat's on one and the rib crest's on the
    other, read off the mesh.
  * A FIT THAT IS WRONG IN THE SOURCE'S OWN TERMS. The plug has to enter the
    slab's bore and the brim has to cover it. Both are measured on a
    cross-section of the exported solid, against the bore the slab is actually
    cut to.

MESH_RULES below is where that lives. The datums it compares against come from
ONE `openscad --export-format=echo` pass over parameters.scad, not from
literals retyped here, and the rules are written to measure the EXPORTED
SOLID: a check that recomputes the model's own arithmetic passes forever and
proves nothing. Where a rule unavoidably is a transcription of the model's own
expression -- the cowl's plan size is the only one -- it says so, because a
transcription catches a dropped feature and a changed datum and nothing else,
and pretending otherwise is how a check earns undeserved trust.
"""
import hashlib
import math
import os
import struct
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

ROOT = os.path.dirname(HERE)
CAD = os.path.join(ROOT, "cad")
EXPORT = os.path.join(ROOT, "export")

#  mm. Tessellation slop on a mesh datum, and 2000x binary STL's float32
#  round-trip error over this part's dimensions. Same figure and same basis as
#  check_cad.py's TOL.  [REPO v2/docs/fabrication.md 4.4]
TOL = 0.05

#  name          source            kind     defines                 notes
#
#  kind decides WHICH FILES GO OUT, and it is the one column that cannot be
#  got wrong quietly: a laser part exported as a mesh arrives as a solid
#  nobody can cut, and a printed part exported as a profile arrives as an
#  outline nobody can print.
#  defines are extra -D flags, string-valued, for a source file that draws
#  more than one part. knob.scad is the case that forced the column: the
#  encoder knob gets a D-bore and the pot knob gets a plain bore with six
#  internal ribs, because the pot's shaft stops 1.545 BELOW the deck's top
#  face and there is nowhere to put a grub screw. They are genuinely two
#  parts, not two settings, so they are two rows.
PARTS = [
    ("slab",         "slab.scad",       "laser", {},
     "12.70 clear acrylic, front. Laser: the slab is a 2D profile in sheet, "
     "and its STL is the same profile extruded, for the render only."),
    ("backpanel",    "backpanel.scad",  "laser", {},
     "3.175 black acrylic, laser-cut back"),
    ("carrier",      "carrier.scad",    "print", {},
     "the printed carrier plate"),

    #  THE SIX KNOBS. Two rows, one file: see `defines` above.
    ("knob_encoder", "knob.scad",       "print", {"variant": "encoder"},
     "3 off. Print: a flanged plug with 12 flutes and a blind D-bore -- no "
     "flat profile makes it. PETG, or cast acrylic from knob_mould."),
    ("knob_pot",     "knob.scad",       "print", {"variant": "pot"},
     "3 off. Print: as the encoder but with six internal ribs instead of the "
     "D-flat, because the pot's shaft stops under the deck's top face"),
    ("fader_cap",    "fader_cap.scad",  "print", {},
     "1 off. Print: a plug that fills the slab's slot with a wider cap over "
     "it, so it is fitted last and never passes through the slot"),

    #  THE COWL IS A PRINTED PART AND IT IS NOT CLOSE, which is worth stating
    #  because it is the one part somebody might reasonably file under the
    #  laser. It is a TRAY: a block 107.510 x 41.613 x 3.820 with a
    #  89.270 x 23.373 x 2.320 cavity in one face. A laser cuts a profile
    #  through a sheet, so it can make neither the cavity's floor nor a part
    #  whose thickness varies, and nothing here is a sheet thickness this
    #  build stocks: the floor is 1.500 (3.54 extrusion lines at a 0.4
    #  nozzle, which is where the figure comes from) against back_t's 3.175
    #  and acrylic_t's 12.700.
    ("cowl",         "cowl.scad",       "print", {},
     "1 off. Print: a tray over the display's 18650 holder, 3.820 deep with "
     "a 2.320 cavity and a 1.500 floor. NOT laser -- its thickness varies "
     "and 1.500 is no sheet in this build"),
    ("foot",         "foot.scad",       "print", {},
     "4 off, TPU 95A. Print: the only part not in PETG or PMMA, and the "
     "material is the design -- see foot.scad"),

    #  THE MOULD IS TOOLING, NOT A PART OF THE INSTRUMENT, and it is here
    #  because it has to be checked like one: a mould that cannot be opened is
    #  scrap, and that is a property of the exported solid.
    ("knob_mould_brim", "knob_mould.scad", "print", {"half": "brim"},
     "tooling, 1 off. Print: the half above the parting plane -- it forms the "
     "brim, the 12 flutes and the indicator"),
    ("knob_mould_plug", "knob_mould.scad", "print", {"half": "plug"},
     "tooling, 1 off. Print: the half below it -- it forms the plug, carries "
     "the counterbore's boss, the gate and the vents"),
]


def run(cmd, timeout=1800):
    t0 = time.time()
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    return r, time.time() - t0


# ---------------------------------------------------------------------------
#  THE EXPORT IS MADE BYTE-REPRODUCIBLE, AND THE CAUSE WAS MEASURED FIRST
# ---------------------------------------------------------------------------
#  v2/export/stl/carrier.stl was not reproducible. FOUR consecutive exports of
#  one frozen carrier.scad, nothing else touched:
#
#      0c2ba105a17bafd5  445a6a7e5f157dd3  15a47786aee2bdb9  ca2858172621f4bd
#
#  all 1793484 bytes, all 35868 facets.  [MEASURED 2026-11-07]
#
#  WHAT THE CAUSE IS, established rather than presumed. Each file was split
#  into its 35868 fifty-byte facet records and the 48 significant bytes of
#  each -- the normal and the three vertices, as written -- were compared as
#  MULTISETS:
#
#      run 2 facet multiset identical to run 1: True
#      run 3 facet multiset identical to run 1: True
#      run 4 facet multiset identical to run 1: True
#      14334 / 9840 / 11907 of 35868 POSITIONS hold a different facet
#
#  So every triangle is bit-for-bit the same triangle, with the same normal
#  and the same corner order, and only the ORDER THE FACETS ARE EMITTED IN
#  changes. Not tessellation geometry, not float noise, not the header -- the
#  80-byte header is a constant "OpenSCAD Model\n" with no timestamp. It is
#  the order of CGAL's traversal of the Nef polyhedron's halffacets, which is
#  built on pointer-ordered containers.
#
#  SO THE FIX IS A PERMUTATION AND NOTHING ELSE, and that matters: a
#  permutation cannot change the geometry, and this pass is written so that
#  the claim is structural rather than hopeful.
#
#    * NO ROUNDING. The canonical post-export pass this was briefed as would
#      round coordinates to a stated precision. The measurement above says
#      that would be paying geometry for nothing: the coordinates do not
#      differ between runs, so rounding them could only move the part.
#    * ROTATION IS CYCLIC ONLY. A facet's corners are rotated to put the
#      lexicographically smallest 12-byte vertex first. A cyclic rotation of
#      three corners preserves the winding exactly -- it is a swap that would
#      invert it -- and the stored normal is left untouched. Run to run the
#      corner order already agrees, so this changes nothing today; it is here
#      so that the sort still CONVERGES if a future CGAL emits a facet's
#      corners from a different starting corner, which a sort alone would not
#      survive.
#    * SORT ON THE 48 RAW BYTES. Any total order will do as long as it is the
#      same one every time; the byte order of IEEE floats is not numerically
#      monotone and does not need to be.
#    * THE 80-BYTE HEADER IS COPIED VERBATIM, not rebuilt, so the pass is
#      provably a reordering of the facet block and nothing else.
#
#  And then it is CHECKED rather than asserted: the mesh is measured off disk
#  before and after, and the export FAILS if face count, bounds, volume, area
#  or euler number moved at all. See canon_proof().
def canonicalise_stl(path):
    """Rewrite a binary STL in place in canonical facet order.

    Returns (n_facets, moved) where moved is how many facet slots changed.
    """
    raw = open(path, "rb").read()
    if len(raw) < 84:
        raise ValueError(f"{os.path.basename(path)}: {len(raw)} bytes is not "
                         f"a binary STL")
    head, n = raw[:80], struct.unpack("<I", raw[80:84])[0]
    if len(raw) != 84 + 50 * n:
        raise ValueError(f"{os.path.basename(path)}: header says {n} facets, "
                         f"which wants {84 + 50 * n} bytes, file has "
                         f"{len(raw)}")
    recs, before = [], []
    for i in range(n):
        o = 84 + 50 * i
        rec, attr = raw[o:o + 48], raw[o + 48:o + 50]
        #  The attribute byte count is zero in everything OpenSCAD writes. If
        #  it ever is not, it is carrying something -- colour, in the usual
        #  abuse of the field -- and silently dropping it would be the kind of
        #  quiet data loss this whole pipeline exists to prevent.
        if attr != b"\x00\x00":
            raise ValueError(f"{os.path.basename(path)}: facet {i} has a "
                             f"non-zero attribute byte count {attr.hex()}; "
                             f"this pass would discard it")
        before.append(rec)
        v = [rec[12:24], rec[24:36], rec[36:48]]
        k = min(range(3), key=lambda j: v[j:] + v[:j])
        recs.append(rec[:12] + b"".join(v[k:] + v[:k]))
    recs.sort()
    moved = sum(1 for a, b in zip(before, recs) if a != b)
    with open(path, "wb") as fh:
        fh.write(head + struct.pack("<I", n))
        for rec in recs:
            fh.write(rec + b"\x00\x00")
    return n, moved


def canon_proof(a, b):
    """What must be identical across the canonical pass, to six decimals.

    Six decimals is not a tolerance, it is the absence of one: a permutation
    of float32 triangles reproduces every one of these EXACTLY, so anything
    that moves at all is a defect in the pass and not slop in the arithmetic.
    """
    keep = []
    for k in ("faces", "euler"):
        keep.append((a[k] == b[k], f"{k} {a[k]} -> {b[k]}"))
    for k in ("volume", "area"):
        keep.append((f"{a[k]:.6f}" == f"{b[k]:.6f}",
                     f"{k} {a[k]:.6f} -> {b[k]:.6f}"))
    for k in ("lo", "hi"):
        keep.append((all(f"{x:.6f}" == f"{y:.6f}" for x, y in zip(a[k], b[k])),
                     f"{k} {['%.6f' % x for x in a[k]]} -> "
                     f"{['%.6f' % x for x in b[k]]}"))
    return keep


# ---------------------------------------------------------------------------
#  THE DATUMS, FROM OPENSCAD ITSELF
# ---------------------------------------------------------------------------
#  parameters.scad is evaluated by the thing that evaluates it for real, so
#  section 12's functions and the assignment ORDER both come out right. Parsing
#  it in Python would be a second implementation of OpenSCAD's semantics, and
#  the one trap this project keeps paying for is a forward reference that
#  evaluates to undef -- which a parser does not reproduce.
#
#  NOTE THE cube(1). OpenSCAD 2021.01 emits NO echo output at all from a file
#  that produces no geometry, so an echo-only dump returns an empty file and
#  every datum reads as missing.
#  EXACTLY THE DATUMS THE RULES READ, AND NO MORE. A name in this list that
#  no rule uses looks like coverage and is not; and because a missing echo is
#  a hard error, a trimmed list is also the cheapest check that the rules and
#  parameters.scad still agree about what exists.
_DATUM_NAMES = [
    "acrylic_t", "knob_h", "knob_proud", "knob_body_d", "knob_brim_d",
    "bore_d", "knob_bore_d", "knob_bore_flat", "knob_bore_shaft_d",
    "knob_rib_h", "knob_counterbore_h", "knob_shaft_bore_h",
    "slider_cap_w", "slider_slot_w", "foot_h_min",
]
_DATUM_FNS = ["cowl_h", "cowl_floor", "cowl_wall", "cowl_flange"]
_datum_cache = {}


def datums():
    if _datum_cache:
        return _datum_cache
    src = ["include <%s>" % os.path.join(CAD, "parameters.scad")]
    for n in _DATUM_NAMES:
        src.append(f'echo(str("DATUM {n} ", {n}));')
    for n in _DATUM_FNS:
        src.append(f'echo(str("DATUM {n} ", {n}()));')
    src.append('echo(str("DATUM cowl_window_x ", cowl_window()[0]));')
    src.append('echo(str("DATUM cowl_window_y ", cowl_window()[1]));')
    src.append("cube(1);")
    with tempfile.TemporaryDirectory() as d:
        scad = os.path.join(d, "dump.scad")
        out = os.path.join(d, "dump.echo")
        open(scad, "w", encoding="utf-8").write("\n".join(src) + "\n")
        r, _ = run(["openscad", "--export-format=echo", "-o", out, scad],
                   timeout=300)
        text = open(out, encoding="utf-8").read() if os.path.exists(out) else ""
    for line in text.splitlines():
        if "DATUM " not in line:
            continue
        body = line.split("DATUM ", 1)[1].rstrip('"')
        name, _, val = body.partition(" ")
        try:
            _datum_cache[name] = float(val)
        except ValueError:
            pass
    missing = [n for n in _DATUM_NAMES + _DATUM_FNS if n not in _datum_cache]
    if missing:
        raise RuntimeError(f"parameters.scad did not echo {missing}: "
                           f"{r.stderr.strip()[-300:]}")
    return _datum_cache


# ---------------------------------------------------------------------------
#  THE MESH, MEASURED OFF DISK
# ---------------------------------------------------------------------------
def bodies(m):
    """Connected components, by union-find over vertex adjacency.

    trimesh.body_count and trimesh.split both route through scipy's sparse
    connected_components and scipy is not installed here, so they raise. This
    needs nothing but the face list.
    """
    parent = list(range(len(m.vertices)))

    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a

    for f in m.faces:
        a, b, c = int(f[0]), int(f[1]), int(f[2])
        for x, y in ((a, b), (b, c)):
            rx, ry = find(x), find(y)
            if rx != ry:
                parent[rx] = ry
    used = {int(v) for f in m.faces for v in f}
    return len({find(v) for v in used})


def section(m, z):
    """Every edge/plane crossing at height z, as points.

    Not trimesh.section(): that returns a Path3D whose planar transform wants
    scipy. This is the whole of what the rules need.
    NOTE WHAT IT IS AND IS NOT. A crossing on a TRIANGULATION DIAGONAL is a
    chord of the real section, so it sits INSIDE the true boundary: measured
    maxima can read low and measured minima can read low. Both errors are in
    the safe direction for the rules below -- "the plug is no bigger than
    this" and "the brim reaches at least this far" -- which is why the rules
    are written as those inequalities and not as equalities on a minimum.
    """
    v = m.vertices
    pts = []
    for a, b in m.edges_unique:
        za, zb = v[a][2], v[b][2]
        if (za - z) * (zb - z) < 0:
            pts.append(v[a] + (z - za) / (zb - za) * (v[b] - v[a]))
    return pts


def radii(m, z):
    return [math.hypot(p[0], p[1]) for p in section(m, z)]


def x_extent(m, z):
    xs = [p[0] for p in section(m, z)]
    return (min(xs), max(xs)) if xs else (0.0, 0.0)


def mesh_facts(path):
    import trimesh
    m = trimesh.load(path, force="mesh")
    lo, hi = m.bounds
    return {
        "mesh": m, "faces": len(m.faces), "verts": len(m.vertices),
        "bodies": bodies(m), "watertight": bool(m.is_watertight),
        "winding": bool(m.is_winding_consistent), "volume": float(m.volume),
        "area": float(m.area), "euler": int(m.euler_number),
        "lo": [float(x) for x in lo], "hi": [float(x) for x in hi],
        "size": [float(hi[i] - lo[i]) for i in range(3)],
    }


# ---------------------------------------------------------------------------
#  THE RULES
# ---------------------------------------------------------------------------
#  Each returns a list of (ok, text). The text is printed whether the rule
#  passes or fails, because the measurement is the point: a number nobody ever
#  reads is a number nobody notices changing.
def _knob_rules(F, K, variant):
    m, out = F["mesh"], []
    #  The header of knob.scad calls `knob_h - knob_proud == acrylic_t` an
    #  identity rather than a coincidence. The model builds the height as
    #  acrylic_t + knob_proud and never mentions knob_h, so this is the only
    #  place the identity is tested, and it is tested on the solid.
    h = F["size"][2]
    out.append((abs(h - K["knob_h"]) < TOL,
                f"height {h:.4f} = knob_h {K['knob_h']:.3f} "
                f"(= acrylic_t + knob_proud, the file's own identity)"))
    #  THE PLUG HAS TO GO INTO THE BORE THE SLAB IS CUT TO.
    rp = max(radii(m, K["acrylic_t"] / 2))
    gap = K["bore_d"] / 2 - rp
    out.append((abs(rp - K["knob_body_d"] / 2) < TOL and gap > 1.000,
                f"plug radius {rp:.4f} in a O{K['bore_d']:.3f} bore: "
                f"{gap:.4f} annular gap"))
    #  AND THE BRIM HAS TO COVER IT. The flute roots are the brim's minimum
    #  radius; if knob_flute_cut ever exceeds (knob_brim_d - bore_d)/2 a flute
    #  opens straight into the bore and the knob stops being a cover.
    rb = min(radii(m, K["acrylic_t"] + K["knob_proud"] / 2))
    out.append((rb >= K["bore_d"] / 2,
                f"brim minimum radius {rb:.4f} (the flute roots) covers the "
                f"bore's {K['bore_d'] / 2:.4f} by {rb - K['bore_d'] / 2:.4f}"))
    #  THE BRIM IS THE WIDEST THING ON THE PART AND IT IS knob_brim_d WIDE.
    #  Two statements in one measurement, and the first draft of this rule was
    #  a bracket on the bounding box -- knob_brim_d - 2*knob_flute_cut ..
    #  knob_brim_d -- which CANNOT FAIL: the bbox is set by the intact rim
    #  between two flutes, so tripling the flute depth moved it by 0.05 mm and
    #  the bracket passed. What does carry the information is the rim's own
    #  maximum radius, which drops the moment the brim is drawn at the wrong
    #  diameter or the flutes grow until no full-radius rim survives; and the
    #  bbox compared against it, which catches anything standing outside the
    #  brim -- a plug drawn bigger than the brim it is supposed to hide under.
    rmax = max(radii(m, K["acrylic_t"] + K["knob_proud"] / 2))
    w = max(F["size"][0], F["size"][1])
    out.append((abs(rmax - K["knob_brim_d"] / 2) < TOL and w <= 2 * rmax + TOL,
                f"brim radius {rmax:.4f} = knob_brim_d/2 "
                f"{K['knob_brim_d'] / 2:.4f}, and it is the widest feature: "
                f"bbox {w:.4f} across"))
    #  THE VARIANT, READ OFF THE BORE AND NOT TRUSTED FROM THE COMMAND LINE.
    #  A -D that never arrives leaves `variant = "encoder"` as written in the
    #  file, and the pot knob ships as a second encoder knob: same bbox, same
    #  height, same everything but the one feature that carries the torque.
    zb = K["knob_counterbore_h"] + K["knob_shaft_bore_h"] / 2
    rmin = min(radii(m, zb))
    if variant == "encoder":
        want = K["knob_bore_d"] / 2 - (K["knob_bore_d"] - K["knob_bore_flat"])
        out.append((abs(rmin - want) < TOL,
                    f"bore minimum radius {rmin:.4f}, wanted the D-flat's "
                    f"{want:.4f} -- which is what makes this the ENCODER"))
    else:
        want = K["knob_bore_d"] / 2 - K["knob_rib_h"]
        out.append((abs(rmin - want) < TOL,
                    f"bore minimum radius {rmin:.4f}, wanted the rib crest's "
                    f"{want:.4f} -- which is what makes this the POT"))
    return out


def _fader_rules(F, K):
    m, out = F["mesh"], []
    #  The step from plug to cap is what "the cap's underside is flush with
    #  the acrylic's top face" means. Measured as the z at which the part
    #  suddenly gets wider, not as the sum the model added up.
    below = x_extent(m, K["acrylic_t"] - 2 * TOL)
    above = x_extent(m, K["acrylic_t"] + 2 * TOL)
    wp, wc = below[1] - below[0], above[1] - above[0]
    out.append((abs(wp - K["slider_cap_w"]) < TOL,
                f"plug {wp:.4f} wide at z = acrylic_t - {2 * TOL:.2f} "
                f"= slider_cap_w {K['slider_cap_w']:.3f}"))
    out.append((wp < K["slider_slot_w"],
                f"plug clears the {K['slider_slot_w']:.3f} slot by "
                f"{(K['slider_slot_w'] - wp) / 2:.4f} per side"))
    #  AND IT MUST NOT FIT THROUGH. stack.md 6.2 step 12 fits this part last
    #  precisely because the cap is wider than the slot it sits in; a cap that
    #  passes the slot is a cap that falls into the deck.
    out.append((wc > K["slider_slot_w"],
                f"cap {wc:.4f} wide above it, {wc - K['slider_slot_w']:+.4f} against "
                f"the slot: it must be WIDER or it falls through"))
    return out


def _cowl_rules(F, K):
    m, out = F["mesh"], []
    #  THE FLOOR, MEASURED FROM INSIDE THE CAVITY. Over the window's own
    #  footprint the only material is the tray's floor, so the horizontal
    #  surfaces there are the outside face and the cavity's own floor and
    #  nothing else -- the four screws are out in the flange. Their z spread
    #  IS the floor's thickness. This is the rule that would catch a cavity
    #  drawn deeper than the tray, which is a hole straight through the part
    #  and looks like nothing at all in a bounding box.
    #    FACE CENTROIDS, NOT VERTICES. A flat rectangle's triangulation puts
    #  every vertex on its boundary and none in its interior, so a vertex test
    #  selects nothing and reads as a pass-shaped failure.
    import numpy as np
    wx, wy = K["cowl_window_x"] / 2 - 1.0, K["cowl_window_y"] / 2 - 1.0
    c = m.triangles_center
    zs = c[(np.abs(c[:, 0]) < wx) & (np.abs(c[:, 1]) < wy)][:, 2]
    if not len(zs):
        out.append((False, "no face inside the cavity footprint: the "
                           "cavity is not where the window is"))
    else:
        lvl = sorted({round(float(v), 4) for v in zs})
        out.append((len(lvl) == 2 and abs(lvl[0]) < TOL
                    and abs(lvl[1] - K["cowl_floor"]) < TOL,
                    f"over the window, {len(zs)} faces on {len(lvl)} levels "
                    f"{lvl}: the floor is {lvl[-1] - lvl[0]:.4f} thick "
                    f"= cowl_floor {K['cowl_floor']:.3f}"))
    #  FOUR THROUGH HOLES, TOPOLOGICALLY. The cowl, the back panel and the
    #  carrier's inserts are ONE fastener stack; if the cowl loses its holes
    #  the stack silently becomes a bonded joint. euler = 2 - 2g, so g = 4.
    out.append((F["euler"] == -6,
                f"euler {F['euler']} = genus {(2 - F['euler']) // 2}, wanted genus 4: "
                f"the four clearance holes"))
    #  A TRANSCRIPTION, AND IT SAYS SO. cw_outer in cowl.scad is the same
    #  expression; this catches a dropped flange or a moved datum and nothing
    #  more.
    for i, k in ((0, "cowl_window_x"), (1, "cowl_window_y")):
        want = K[k] + 2 * (K["cowl_wall"] + K["cowl_flange"])
        out.append((abs(F["size"][i] - want) < TOL,
                    f"plan {'xy'[i]} {F['size'][i]:.4f} = window + "
                    f"2*(wall + flange) {want:.4f}  [transcription]"))
    out.append((abs(F["size"][2] - K["cowl_h"]) < TOL,
                f"height {F['size'][2]:.4f} = cowl_h {K['cowl_h']:.3f}"))
    return out


#  ISO 4762 M2.5 socket cap: dk 4.500 across the head, k 2.500 tall.
#  [STANDARD]  It is NOT in parameters.scad -- section 6 declares clearance
#  and insert diameters and no head -- and this file holds it rather than
#  adding it there, which is also the right place for it: the foot's pocket
#  has to fit a head whose size is a published fact, so the checker should
#  carry the published fact and the part should be measured against it. Both
#  values are reported upward for parameters.scad 6.
SCREW_M25_HEAD_D = 4.500
SCREW_M25_HEAD_H = 2.500


def _foot_rules(F, K):
    m, out = F["mesh"], []
    import numpy as np
    v = np.asarray(m.vertices)
    r = np.hypot(v[:, 0], v[:, 1])
    zmin, h = F["lo"][2], F["size"][2]
    #  THE ONE NUMBER THE FOOT EXISTS FOR. Below foot_h_min the deck does not
    #  stand on its feet at all -- it stands on the display's own 18650 cowl.
    out.append((h >= K["foot_h_min"],
                f"height {h:.4f} >= foot_h_min {K['foot_h_min']:.3f} by "
                f"{h - K['foot_h_min']:.4f}"))
    #  THE POCKET GRIPS THE HEAD, AND THE HEAD NEVER REACHES ITS FLOOR. These
    #  two are the whole design argument of the part, so they are the two that
    #  get measured. The foot is press-on precisely BECAUSE a clamped TPU pad
    #  puts 20.6 MPa on 17.15 mm2 of counterbore floor and creeps, so if the
    #  pocket ever got shallower than the head is tall the clamp would be back
    #  in the TPU and the part would be quietly wrong. ISO's head, not
    #  foot.scad's own numbers, is what both are measured against.
    inside = r < SCREW_M25_HEAD_D / 2 + 0.8
    if not inside.any():
        out.append((False, "no pocket on the axis at all"))
        return out
    r_wall = float(r[inside & (v[:, 2] > zmin + 1.0)].min())
    nip = SCREW_M25_HEAD_D - 2 * r_wall
    out.append((0 < nip <= 0.500,
                f"pocket wall r {r_wall:.4f} on a O{SCREW_M25_HEAD_D:.3f} "
                f"head: {nip:+.4f} diametral interference, which is the "
                f"retention"))
    floor_z = float(v[inside, 2].min())
    clear = (h - floor_z) - SCREW_M25_HEAD_H
    out.append((clear > 0,
                f"pocket floor {floor_z:.4f} above the sole, so it is "
                f"{h - floor_z:.4f} deep over a {SCREW_M25_HEAD_H:.3f} head: "
                f"{clear:+.4f} of air over the head, which must be positive"))
    #  AND NOTHING OPENS AT THE SOLE. euler says there is no hole through the
    #  part; this says the pocket is not the hole, i.e. the sole is one
    #  unbroken face -- which is what lets a bought elastomer bumper be stuck
    #  to it later without redrawing the foot.
    out.append((F["euler"] == 2,
                f"euler {F['euler']} = genus {(2 - F['euler']) // 2}, wanted genus 0: "
                f"nothing may pass through this part"))
    sole = np.abs(m.triangles_center[:, 2] - zmin) < 1e-6
    out.append((bool(sole.any()) and float(r[np.abs(v[:, 2] - zmin) < 1e-6]
                                           .min()) > SCREW_M25_HEAD_D / 2,
                f"sole {float(m.area_faces[sole].sum()):.4f} mm2 over "
                f"{int(sole.sum())} faces, and its innermost radius must "
                f"clear the head's {SCREW_M25_HEAD_D / 2:.3f}"))
    return out


def _mould_rules(F, K, half):
    """A mould that cannot be opened is scrap, so that is what gets measured.

    Not the draft ANGLE written in knob_mould.scad -- reading that back would
    be the source checking itself. What is measured is the cavity's own wall,
    off the exported solid: its radius at 33 heights along the pull, which
    must be monotone in the direction the part comes out, and whose end-to-end
    slope IS the draft. A mould half with a reversed sign or a lost factor
    stops being monotone and the part is locked in; a mould half with no draft
    at all reads 0.000 and fails the window.
    """
    m, out = F["mesh"], []
    #  The two halves split on the brim's underside, so the wall each one
    #  forms is a knob datum and not a tool decision.
    nominal = (K["knob_brim_d"] if half == "brim" else K["knob_body_d"]) / 2
    #  +1: the part leaves the plug half upward, so its cavity must widen
    #  with z. -1: it leaves the brim half downward, so that one must narrow.
    sgn = 1.0 if half == "plug" else -1.0
    lo, hi = F["lo"][2], F["hi"][2]
    rows = []
    for k in range(33):
        z = lo + (hi - lo) * (k + 0.5) / 33
        #  Below nominal*1.10 keeps the cavity wall and excludes the
        #  registration cones, which start at 17.720 and would otherwise read
        #  as a cavity 18 mm wide.
        rr = [x for x in radii(m, z) if x < nominal * 1.10]
        if rr and max(rr) > 0.9 * nominal:
            rows.append((z, max(rr)))
    if len(rows) < 4:
        out.append((False, f"found the cavity wall at only {len(rows)} of 33 "
                           f"heights: there is nothing to measure a draft on"))
        return out
    worst = min(sgn * (rows[i + 1][1] - rows[i][1])
                for i in range(len(rows) - 1))
    out.append((worst > -0.0005,
                f"cavity wall monotone along the pull over "
                f"{len(rows)} heights z {rows[0][0]:.3f}..{rows[-1][0]:.3f}: "
                f"worst step {worst:+.5f} mm (must not go backwards, or the "
                f"part is locked in)"))
    dr, dz = rows[-1][1] - rows[0][1], rows[-1][0] - rows[0][0]
    draft = math.degrees(math.atan2(abs(dr), abs(dz)))
    #  THE WINDOW IS RECORDED HERE AND ARGUED, not copied from the part:
    #  under about 0.5 degrees a printed cavity drags on its own layer lines,
    #  and over about 3 the taper shows in the knob.
    out.append((0.5 <= draft <= 3.0 and sgn * dr > 0,
                f"draft {draft:.4f} deg, wall {rows[0][1]:.4f} -> "
                f"{rows[-1][1]:.4f} over {dz:.4f} of pull, widening toward "
                f"the {'parting plane' if sgn * dr > 0 else 'WRONG END'}"))
    #  THE SHRINKAGE ALLOWANCE, read off the mouth. knob_mould.scad's own
    #  table says the knob has no dimension tight enough for an allowance
    #  under 1 % to matter; 2 % is where that table stops holding, so that is
    #  the gate. Under-size is never acceptable: the cast part would be small.
    r_mouth = rows[-1][1] if sgn > 0 else rows[0][1]
    allow = r_mouth / nominal - 1
    out.append((0 <= allow <= 0.02,
                f"cavity {r_mouth:.4f} at the parting plane against the "
                f"knob's {nominal:.4f}: {allow * 100:+.3f} % allowance"))
    if half == "plug":
        #  THE GATE HAS TO BE SMALLER THAN THE DRILL THAT REMOVES IT. The
        #  sprue lands on the counterbore's floor so the O6.000 bore drill
        #  takes the whole stub away; a gate wider than the drill leaves a
        #  scar inside the bore, on the one surface that carries the torque.
        z = K["knob_counterbore_h"] / 2
        inner = [x for x in radii(m, z) if x < nominal * 0.9]
        r_gate = min(inner) if inner else None
        out.append((r_gate is not None
                    and 0 < r_gate < K["knob_bore_shaft_d"] / 2,
                    f"gate radius {r_gate if r_gate is None else round(r_gate, 4)}"
                    f" at the counterbore's mid-height, inside the "
                    f"O{K['knob_bore_shaft_d']:.3f} drill that removes it"))
    else:
        #  AND THE FLUTES MUST NOT EAT THE BRIM'S COVER OVER THE BORE. Same
        #  requirement knob.scad is measured against, applied to the tool: the
        #  flute ridges are the cavity's minimum radius, and if they ever
        #  reach inside bore_d/2 the cast knob stops covering its own bore.
        z = (F["lo"][2] + rows[-1][0]) / 2
        inner = [x for x in radii(m, z) if x < nominal * 1.10]
        r_flute = min(inner) if inner else None
        out.append((r_flute is not None
                    and K["bore_d"] / 2 <= r_flute < nominal,
                    f"flute ridges at r {r_flute if r_flute is None else round(r_flute, 4)}"
                    f": between the bore's {K['bore_d'] / 2:.4f} and the "
                    f"brim's {nominal:.4f}"))
    out.append((True, f"block {F['size'][0]:.3f} x {F['size'][1]:.3f} x "
                      f"{F['size'][2]:.3f}, {F['faces']} faces"))
    return out


MESH_RULES = {
    "knob_encoder": lambda F, K: _knob_rules(F, K, "encoder"),
    "knob_pot":     lambda F, K: _knob_rules(F, K, "pot"),
    "fader_cap":    _fader_rules,
    "cowl":         _cowl_rules,
    "foot":         _foot_rules,
    "knob_mould_brim": lambda F, K: _mould_rules(F, K, "brim"),
    "knob_mould_plug": lambda F, K: _mould_rules(F, K, "plug"),
}


def finish_stl(name, dest):
    """Canonicalise the exported STL, prove the pass moved nothing, verify it.

    The order is deliberate. The canonical pass runs FIRST, so everything
    downstream -- the mesh rules, the render, the slicer -- sees the file that
    actually ships, and the file that ships is the one whose bytes are
    reproducible.
    """
    out = []
    try:
        before = mesh_facts(dest)
        n, moved = canonicalise_stl(dest)
        after = mesh_facts(dest)
    except Exception as ex:
        return [(name, "FAIL", f"canonical pass: {type(ex).__name__}: {ex}")]
    bad = [t for ok, t in canon_proof(before, after) if not ok]
    if bad:
        for t in bad:
            out.append((name, "FAIL", f"canonical pass MOVED THE GEOMETRY: "
                                      f"{t}"))
        return out
    #  IDEMPOTENCE IS THE DETERMINISM CHECK, and it is a property of the file
    #  on disk rather than of this code: canonical order is a fixed point, so
    #  if a second pass changes a single byte the file was not in canonical
    #  order and the reproducibility claim is void. It costs milliseconds and
    #  it is the only check available that does not need a second 120-second
    #  export of the carrier to run.
    h1 = hashlib.sha256(open(dest, "rb").read()).hexdigest()
    canonicalise_stl(dest)
    h2 = hashlib.sha256(open(dest, "rb").read()).hexdigest()
    out.append((name, "    " if h1 == h2 else "FAIL",
                f"canonical {n} facets, {moved} moved, sha256 {h1[:16]}"
                + ("" if h1 == h2 else f" -- NOT a fixed point: a second "
                                       f"pass gives {h2[:16]}")))
    out += verify_mesh(name, dest, after)
    return out


def verify_mesh(name, dest, F=None):
    """Read the STL back off disk and refuse it if it is not a solid.

    EVERY exported mesh gets the three universal checks, including the laser
    parts' render meshes, because "watertight, one body, positive volume" is
    true of every one of them and is not a statement about any dimension. The
    per-part rules in MESH_RULES are only for the parts this file's author
    owns; slab, backpanel and carrier are measured in full by check_cad.py.
    """
    out = []
    try:
        F = F or mesh_facts(dest)
    except Exception as ex:
        return [(name, "FAIL", f"mesh: cannot read {os.path.basename(dest)}: "
                               f"{type(ex).__name__}: {ex}")]
    out.append((name, "    ",
                f"mesh {F['faces']:6d} faces  {F['bodies']} body  "
                f"euler {F['euler']:+d}  "
                f"{F['size'][0]:.4f} x {F['size'][1]:.4f} x {F['size'][2]:.4f}"
                f"  vol {F['volume']:.4f} mm3"))
    if not F["watertight"]:
        out.append((name, "FAIL", "mesh: not watertight -- an open edge is a "
                                  "hole the slicer will fill with a guess"))
    if F["bodies"] != 1:
        out.append((name, "FAIL", f"mesh: {F['bodies']} separate bodies. "
                                  f"Something is floating in mid-air"))
    if not (F["volume"] > 0 and F["winding"]):
        out.append((name, "FAIL", f"mesh: volume {F['volume']:.4f}, winding "
                                  f"consistent {F['winding']} -- the shell is "
                                  f"inside out"))
    rules = MESH_RULES.get(name)
    if rules:
        #  A RULE THAT THROWS IS A FAILED EXPORT, NOT A TRACEBACK. The two
        #  ways it happens are both real: datums() raises when a name a rule
        #  reads has gone from parameters.scad, which is exactly the drift
        #  this file exists to catch; and a rule raises on a mesh degenerate
        #  enough that its own measurement has no answer. Either way the row
        #  has to say which part and why, and the remaining parts still get
        #  exported.
        try:
            rows = list(rules(F, datums()))
        except Exception as ex:
            rows = [(False, f"rule raised {type(ex).__name__}: {ex}")]
        for ok, text in rows:
            out.append((name, "    " if ok else "FAIL",
                        text if ok else f"mesh: {text}"))
    return out


def export_one(name, src, kind, defines):
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
        for k, v in defines.items():
            cmd += ["-D", f'{k}="{v}"']
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
            continue
        kb = os.path.getsize(dest) / 1024
        out.append((name, "ok", f"{ext:3s} {kb:8.1f} kB  {dt:5.1f}s"))
        #  ---- the canonical pass and the mesh refusal, on the file that was
        #  just written and on the bytes that will actually ship
        if ext == "stl":
            out += finish_stl(name, dest)

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
    for name, src, kind, defines, _ in PARTS:
        if name in want:
            rows += export_one(name, src, kind, defines)
    bad = 0
    for name, state, msg in rows:
        if state == "FAIL":
            bad += 1
        print(f"  {state:4s}  {name:18s} {msg}")
    if not rows:
        print(f"  nothing matched {want}")
        return 1
    print(f"  {'all exported' if not bad else str(bad) + ' FAILED'}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
