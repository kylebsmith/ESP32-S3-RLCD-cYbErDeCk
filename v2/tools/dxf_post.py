#!/usr/bin/env python3
"""Make the exported vector files safe to hand a fabricator -- then refuse them.

WHAT IS WRONG WITH WHAT OPENSCAD WRITES
---------------------------------------
OpenSCAD 2021.01's DXF writer emits an R12 file whose only sections are
BLOCKS (empty), ENTITIES and OBJECTS (one empty DICTIONARY). There is **no
HEADER section and no TABLES section**: no $INSUNITS, no $MEASUREMENT, no
LAYER table. Every entity is a bare LINE on layer "0" at colour 256
(BYLAYER -- of a layer that is not declared anywhere).

A DXF with no $INSUNITS is interpreted as *drawing units* by the receiver and
the classic outcome is a part cut at 1/25.4 scale. On a 304.600 x 190.375 mm
slab in 12.700 mm acrylic that is an expensive mistake.
`v2/docs/fabrication.md` 0.5 and 1.6 are the documents this module implements.

THE TRAP, AND IT IS WHY THE VALIDATION READS RAW TEXT
-----------------------------------------------------
`ezdxf.readfile()` on that file reports `$INSUNITS == 6`, `$MEASUREMENT == 1`
and `doc.layers == ['0', 'Defpoints']`. **None of that is in the file.** ezdxf
synthesises a default HEADER and TABLES in memory for an R12 document that has
none. So a check written as `doc.header["$INSUNITS"]` is measuring ezdxf's
defaults, not the artefact -- and `$MEASUREMENT == 1` would have *passed* on a
file with no header at all. fabrication.md 4.1 item 2 anticipates exactly this
("fail if the HEADER section is absent, which is not the same as the value
being wrong. Read the raw text ... to tell them apart") and `validate_dxf()`
below does both: raw-text section presence first, parsed values second.

WHAT THIS MODULE DOES
---------------------
1. OpenSCAD's untouched output is kept at `export/dxf/raw/<name>.dxf`. The
   fabricator file at `export/dxf/<name>.dxf` is built from it. Keeping both
   is what lets `validate_dxf()` prove, artefact against artefact, that the
   rewrite lost nothing and invented nothing -- rather than trusting the
   rewrite because it is the same code that wrote the file.
2. $INSUNITS = 4 (millimetres), $MEASUREMENT = 1 (metric), $LUNITS = 2
   (decimal), and real $EXTMIN/$EXTMAX measured off the geometry. Saved as
   R2000 (AC1015), because $INSUNITS is an R13+ header variable.
3. A LAYER table carrying fabrication.md 1.6's declared convention, exactly.
4. The bare LINE soup stitched into closed LWPOLYLINEs -- using
   `check_cad.stitch()`, which already does this correctly, not a second
   implementation of it. A chain that does not close is left as LINE
   entities and reported; nothing is forced shut.
5. An asymmetric fiducial in the sheet's waste area, in BOTH files, so that a
   mirrored file is obvious at a glance and so that the handedness check
   below is well-posed.

HANDEDNESS -- AND A CORRECTION TO fabrication.md 1.3 ITEM 2
-----------------------------------------------------------
fabrication.md 1.3 item 2 says "the SVG is Y-mirrored and the DXF is not" and
concludes that "the moment one asymmetric feature appears ... the SVG becomes a
true mirror of the part". The first half is true of the *numbers* and the
conclusion does not follow, which was verified by rasterising both files:

    DXF  is a Y-UP   coordinate system (CAD convention).
    SVG  is a Y-DOWN coordinate system (SVG 1.1: +y goes down the page).

So `svg_y = -model_y` is the *correct* conversion between them, not a mirror,
and OpenSCAD's SVG writer is doing the right thing: rasterised under each
format's own axis convention the two files produce a pixel-identical page.
Rasterised under the same convention they do not. Therefore:

  * a check that asserts the two files' y SIGNS AGREE would fail forever on a
    correct file, which is worse than no check;
  * the check that is worth having asserts the model->SVG map solved from the
    geometry in the two files is **orientation-reversing** (determinant -1),
    because that is what makes the two render alike. Determinant +1 means the
    SVG really is a mirror of the DXF.

`handedness()` solves that map from OpenSCAD's own emitted part contours -- not
from any constant in this file -- and requires that **exactly one** of the four
axis hypotheses fits. On a part that is symmetric about both axes more than one
fits, the map is not recoverable, and the file pair cannot be checked at all.
That degeneracy is what the fiducial removes, permanently: it is the one
contour whose area is unique and whose centroid is off both axes.

    /usr/bin/python3.11 v2/tools/dxf_post.py            # re-post-process and
                                                        # validate what is on disk
"""
import math
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

# The stitcher is NOT reimplemented here. check_cad.py's is correct and is the
# one the measurements in fabrication.md 1.5 were taken with; two stitchers
# that disagree would make the DXF and the checker disagree about the part.
from check_cad import stitch                                  # noqa: E402


# ---------------------------------------------------------------------------
#  fabrication.md 1.6's DECLARED LAYER CONVENTION
#
#  Read it there; it is reproduced here only so this file is self-contained.
#
#  HONEST NOTE, which fabrication.md 1.6 makes itself and 5.4 leaves open:
#  red/green/blue for cut/engrave/score is a widely used convention but it is
#  OUR declared convention and NOT any service's published requirement. No
#  service we have read publishes one -- Ponoko's catalogue page does not state
#  accepted formats, SendCutSend's acrylic page does not either, and Schaeffer
#  says only "all standard file formats". Its value is that it is *declared*,
#  so the order can say "red is the outer profile, cut it last" and this module
#  can verify the file matches what the order says. It is NOT SOURCED and the
#  thing that closes it is a quote, not a guess.
# ---------------------------------------------------------------------------
LAYERS = [
    # name          ACI   meaning                                 [REPO 1.6]
    ("CUT_OUTER",     1, "the outside profile. Cut last."),
    ("CUT_INNER",     3, "every internal through-contour. Cut first."),
    ("ENGRAVE",       5, "raster, if any ever exists. None today."),
    ("SCORE",         6, "vector score, if any ever exists. None today."),
    # A FIFTH LAYER, WHICH fabrication.md 1.6's TABLE DOES NOT DECLARE.
    # The fiducial is neither the part's outside profile nor an internal
    # contour of it -- it is a separate token in the sheet's waste -- so
    # neither "cut last" nor "cut first" applies to it, and putting it on
    # CUT_OUTER would hand the operator two red perimeters when the order says
    # red is "the outer profile". It gets ACI 2 (yellow), a colour 1.6's table
    # does not use. 1.6's table needs amending to say so; this module cannot
    # edit that document.
    ("FIDUCIAL",      2, "the asymmetric orientation mark, in the waste."),
]
PART_LAYERS = ("CUT_OUTER", "CUT_INNER")
FID_LAYER = "FIDUCIAL"

# fabrication.md 4.1 item 4: entity types a receiver may not silently drop.
ENTITY_WHITELIST = {"LINE", "LWPOLYLINE", "POLYLINE", "CIRCLE", "ARC"}

INSUNITS_MM = 4          # [STANDARD] DXF $INSUNITS: 4 == millimetres
MEASUREMENT_METRIC = 1   # [STANDARD] DXF $MEASUREMENT: 1 == metric
LUNITS_DECIMAL = 2       # [STANDARD] DXF $LUNITS: 2 == decimal

# ---------------------------------------------------------------------------
#  THE FIDUCIAL
#
#  An "F", because an "F" is chiral in both axes: mirror it about x and it
#  reads backwards, mirror it about y and it reads upside down. Either is
#  obvious at a glance across a workshop, which a dot or a notched corner is
#  not. fabrication.md 1.7 already specifies the same glyph for the coupon --
#  "a 3.000 mm 'F' cut as a through-slot ... reads correctly only if the sheet
#  was cut face-up as drawn" -- so this is that convention at sheet scale, not
#  a new one.
#
#  WHY NOT 3.000 mm. At 3.000 mm tall the strokes come out near 0.600 mm, which
#  is 3 x the 0.200 mm kerf [SVC ponoko-kerf, via fabrication.md 1.1] and a
#  21:1 depth-to-width ratio in 12.700 mm acrylic. It will not cut. The glyph
#  below is the same shape on the project's own 0.635 mm grid at a size that
#  will.                                                            [DERIVED]
#
#  WHICH CORNER, AND WHICH WAY ROUND -- the thing the build notes must say:
#
#      The "F" is in the waste BEYOND THE NEAR-LEFT CORNER of the part, in
#      MODEL coordinates: left of x = -body_w/2 and nearer than
#      y = -body_h/2. "Near" is the keyboard end (-Y); "left" is the user's
#      left as they sit at the deck (-X). It is 12.700 mm clear of the part on
#      both sides.
#
#      It is UPRIGHT in model coordinates: the spine on the left, both arms
#      pointing +X (to the user's right), the long arm at the TOP (the +Y,
#      display end).
#
#      Cut visible-face-up as drawn (fabrication.md 1.3) and the "F" reads
#      correctly when you look at the visible face with the keyboard end
#      toward you. If it reads backwards, the sheet was flipped. If it reads
#      upside down, the file was Y-mirrored. If it has moved to the far-right
#      corner, both.
# ---------------------------------------------------------------------------
FID_GAP = 12.700     # [DESIGN] 20 x 0.635 grid. Also >= 1 x t for the 12.700
                     #   slab, the minimum-web advisory of fabrication.md 1.4
                     #   [3P fab-guides], so the mark can never encroach on
                     #   the part's own web budget.
FID_H = 12.700       # [DESIGN] 20 x 0.635. Glyph height.
FID_S = 2.540        # [DESIGN]  4 x 0.635. Stroke width: 12.7 x the 0.200 mm
                     #   kerf, and above the 2.000 mm hard floor fabrication.md
                     #   4.1 item 11 gates a FAIL on.
FID_ARM_TOP = 7.620  # [DESIGN] 12 x 0.635. Total width at the top arm.
FID_ARM_MID = 5.080  # [DESIGN]  8 x 0.635. Total width at the middle arm.
FID_MID_Y = 5.080    # [DESIGN]  8 x 0.635. Bottom of the middle arm.

# An upright "F" as ONE closed loop, counter-clockwise, in a local frame whose
# origin is the bottom-left of the glyph. 10 vertices.
FID_RING_LOCAL = [
    (0.0,          0.0),
    (FID_S,        0.0),
    (FID_S,        FID_MID_Y),
    (FID_ARM_MID,  FID_MID_Y),
    (FID_ARM_MID,  FID_MID_Y + FID_S),
    (FID_S,        FID_MID_Y + FID_S),
    (FID_S,        FID_H - FID_S),
    (FID_ARM_TOP,  FID_H - FID_S),
    (FID_ARM_TOP,  FID_H),
    (0.0,          FID_H),
]

FID_SVG_ID = "orientation-fiducial"   # the id of the mark's <path> in the SVG
SVG_HAIRLINE = 0.100  # [DESIGN] half the 0.200 mm kerf, so the stroke can
                      #   never be read as the cut's width. fabrication.md 4.2
                      #   item 3 asks for "a hairline" and names no number.
SVG_MARGIN = 1.270    # [DESIGN] 2 x 0.635. Sheet padding around the union of
                      #   the part and the fiducial.

TOL_JOIN = 1e-6       # mm. [CAD-EXACT] Not a choice: it is check_cad.stitch()'s
                      #   own default (`def stitch(segs, tol=1e-6)`) and the
                      #   rounding of its key() (`round(p[0], 6)`), and
                      #   closed_rings() keys its conservation argument on the
                      #   same _key(). A different value here would make the
                      #   stitcher and the proof of closure disagree.
TOL_MAP = 1e-3        # mm. OpenSCAD writes both files at 6 significant
                      #   figures from the same geometry, so matching
                      #   landmarks are bit-identical decimal strings and the
                      #   true residual is 0. 1e-3 is the printing resolution
                      #   at these magnitudes -- generous, and 230x below the
                      #   +/-0.230 mm the machine itself holds [SVC sendcutsend].
TOL_AREA_REL = 1e-6   # relative. Loose enough that a rounding difference
                      #   between the two writers cannot break a landmark
                      #   correspondence, tight enough that the six 379.86 mm2
                      #   bores and the five 28.84 mm2 slots still read as
                      #   distinct areas (they differ by 2.4e-6 and 1.0e-4
                      #   relative) [MEASURED].


# ---------------------------------------------------------------------------
#  geometry helpers -- no shapely dependency for the ones on the hot path
# ---------------------------------------------------------------------------
def _key(p):
    return (round(p[0], 6), round(p[1], 6))


def ring_area(ring):
    """Signed area by the shoelace formula. Sign carries the winding."""
    a = 0.0
    for i in range(len(ring)):
        x0, y0 = ring[i]
        x1, y1 = ring[(i + 1) % len(ring)]
        a += x0 * y1 - x1 * y0
    return a / 2.0


def ring_centroid(ring):
    """Area centroid of a simple closed ring."""
    a = ring_area(ring)
    if abs(a) < 1e-12:                      # degenerate: fall back to vertices
        n = len(ring)
        return (sum(p[0] for p in ring) / n, sum(p[1] for p in ring) / n)
    cx = cy = 0.0
    for i in range(len(ring)):
        x0, y0 = ring[i]
        x1, y1 = ring[(i + 1) % len(ring)]
        cr = x0 * y1 - x1 * y0
        cx += (x0 + x1) * cr
        cy += (y0 + y1) * cr
    return (cx / (6.0 * a), cy / (6.0 * a))


def ring_bbox(ring):
    xs = [p[0] for p in ring]
    ys = [p[1] for p in ring]
    return min(xs), min(ys), max(xs), max(ys)


def bbox_union(boxes):
    return (min(b[0] for b in boxes), min(b[1] for b in boxes),
            max(b[2] for b in boxes), max(b[3] for b in boxes))


# ---------------------------------------------------------------------------
#  read the artefact
# ---------------------------------------------------------------------------
def dxf_segments(path):
    """Every LINE in the file, as ((x0,y0),(x1,y1))."""
    import ezdxf
    doc = ezdxf.readfile(path)
    return [((e.dxf.start.x, e.dxf.start.y), (e.dxf.end.x, e.dxf.end.y))
            for e in doc.modelspace() if e.dxftype() == "LINE"]


def closed_rings(segs):
    """Stitch segments into rings and PROVE which ones close, on the data.

    `check_cad.stitch()` walks real segments but appends an open chain and a
    closed loop indistinguishably, so closure is established here by a
    conservation argument on the segment multiset rather than by trusting the
    walk: a ring of n vertices must consume exactly n input segments,
    including the one from its last vertex back to its first.

    Returns (rings, loose, note) -- `loose` is the segments that belong to no
    closed ring and must stay as LINE entities.
    """
    from collections import Counter
    pool = Counter(frozenset((_key(a), _key(b))) for a, b in segs)
    rings, open_chains = [], []
    for loop in stitch(segs, tol=TOL_JOIN):
        want = Counter(
            frozenset((_key(loop[i]), _key(loop[(i + 1) % len(loop)])))
            for i in range(len(loop)))
        if all(pool[k] >= v for k, v in want.items()):
            pool.subtract(want)
            rings.append(loop)
        else:
            open_chains.append(loop)
    leftover = {k for k, v in pool.items() if v > 0}
    loose = [s for s in segs
             if frozenset((_key(s[0]), _key(s[1]))) in leftover]
    note = ""
    if open_chains or loose:
        note = (f"{len(open_chains)} chain(s) would not close and "
                f"{len(loose)} segment(s) are left over")
    return rings, loose, note


#  One coordinate pair of an SVG path's `d`. Shared by the two readers below
#  so that they can never disagree about what counts as a subpath.
_COORD_PAIR = (r'(-?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?),'
               r'(-?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?)')


def _svg_chunks(path):
    """Every subpath of every <path> in the SVG, as (points, chunk_text)."""
    txt = open(path, encoding="utf-8").read()
    out = []
    for d in re.findall(r'<path[^>]*\bd="(.*?)"', txt, re.S):
        for chunk in d.split("M"):
            if not chunk.strip():
                continue
            pts = [(float(a), float(b))
                   for a, b in re.findall(_COORD_PAIR, chunk)]
            if len(pts) > 2:
                out.append((pts, chunk))
    return out


def svg_subpaths(path):
    """Every subpath of every <path> in the SVG, as a ring of (x, y).

    Coordinates are returned exactly as the file states them -- i.e. in SVG's
    own Y-DOWN frame. Converting them is handedness()'s job, not this one's.
    """
    return [pts for pts, _ in _svg_chunks(path)]


def svg_closures(path):
    """(subpaths, how many carry an explicit closepath) -- fabrication.md 4.2
    item 5, whose second half is "every subpath ends in z".

    Counting "z" over the whole file would also count a closepath belonging to
    a subpath svg_subpaths() discards, and would miss an uppercase "Z", so the
    split here is the same one svg_subpaths() uses.
    """
    chunks = _svg_chunks(path)
    return len(chunks), sum(1 for _, c in chunks if "z" in c or "Z" in c)


# ---------------------------------------------------------------------------
#  the fiducial, placed from the part's OWN measured bounding box
# ---------------------------------------------------------------------------
def fiducial_ring(part_bbox):
    """The "F" in model coordinates, in the waste beyond the near-left corner.

    `part_bbox` is measured off the stitched contours of the exported file --
    never off parameters.scad -- so the mark follows the part if the part
    moves.
    """
    minx, miny, _, _ = part_bbox
    x0 = minx - FID_GAP - FID_ARM_TOP      # right edge of the glyph sits
    y0 = miny - FID_GAP - FID_H            #   FID_GAP clear of the part
    ring = [(x + x0, y + y0) for x, y in FID_RING_LOCAL]
    if ring_area(ring) < 0:                # keep it counter-clockwise
        ring.reverse()
    return ring


# ---------------------------------------------------------------------------
#  handedness: solve the model -> SVG map FROM THE TWO ARTEFACTS
# ---------------------------------------------------------------------------
def handedness(dxf_rings, svg_rings):
    """Which axis map takes the DXF's coordinates to the SVG's.

    Returns (fits, detail) where `fits` is the list of (sx, sy, residual)
    hypotheses that explain the data to TOL_MAP. OpenSCAD's writers are
    axis-aligned and unit-scale, so the four sign combinations are the whole
    hypothesis space; the offset is recovered from the landmark bounding
    boxes, which makes a translation harmless.

    Landmarks are the area centroids of the contours OpenSCAD emitted. Only
    contours whose |area| is UNIQUE within their file are used to fix the
    offset, so near-equal contours (six bores at 379.86, five slots at 28.84)
    can never be mismatched to each other.
    """
    dxf_m = [(abs(ring_area(r)),) + ring_centroid(r) for r in dxf_rings]
    svg_m = [(abs(ring_area(r)),) + ring_centroid(r) for r in svg_rings]
    detail = {"n_dxf": len(dxf_m), "n_svg": len(svg_m)}
    if len(dxf_m) != len(svg_m) or not dxf_m:
        detail["error"] = (f"contour counts differ: DXF {len(dxf_m)}, "
                           f"SVG {len(svg_m)}")
        return [], detail

    # |area| is invariant under reflection, so the two files' area multisets
    # must agree before any map can be solved. This also proves unit scale.
    da = sorted(a for a, _, _ in dxf_m)
    sa = sorted(a for a, _, _ in svg_m)
    worst = max(abs(x - y) / max(1.0, x) for x, y in zip(da, sa))
    detail["area_mismatch_rel"] = worst
    if worst > TOL_AREA_REL:
        detail["error"] = (f"contour areas disagree by {worst:.3e} relative "
                           f"-- the two files are not the same geometry")
        return [], detail

    def unique(marks):
        out = []
        for i, (a, cx, cy) in enumerate(marks):
            if all(abs(a - b) > TOL_AREA_REL * max(1.0, a)
                   for j, (b, _, _) in enumerate(marks) if j != i):
                out.append((a, cx, cy))
        return out

    du, su = unique(dxf_m), unique(svg_m)
    detail["n_unique"] = len(du)
    if len(du) < 2 or len(du) != len(su):
        detail["error"] = (f"only {len(du)} contour(s) have a unique area; "
                           f"the map cannot be pinned. The fiducial is the "
                           f"contour that is meant to guarantee this.")
        return [], detail
    du.sort(key=lambda m: m[0])
    su.sort(key=lambda m: m[0])

    fits, scores = [], {}
    for sx in (1, -1):
        for sy in (1, -1):
            ox, oy = solve_offset(du, su, sx, sy)
            # score every contour, not only the unique ones: for each mapped
            # DXF centroid, the distance to the nearest SVG centroid of the
            # same area. The WORST one is the score, so a single displaced
            # contour is enough to reject the map.
            res, worst = 0.0, None
            for a, cx, cy in dxf_m:
                tx, ty = sx * cx + ox, sy * cy + oy
                best = min((math.hypot(tx - ux, ty - uy)
                            for b, ux, uy in svg_m
                            if abs(a - b) <= TOL_AREA_REL * max(1.0, a)),
                           default=float("inf"))
                if best > res:
                    res, worst = best, (a, cx, cy)
            scores[(sx, sy)] = (res, worst)
            if res <= TOL_MAP:
                fits.append((sx, sy, res))
    detail["fits"] = fits
    detail["scores"] = scores
    return fits, detail


def solve_offset(dxf_marks, svg_marks, sx, sy):
    """The translation between the two landmark sets under the sign map.

    Recovered from the landmarks' bounding boxes, which costs nothing and
    makes a translated file harmless to the SIGN determination -- the thing
    the handedness check is actually about. `dxf_marks`/`svg_marks` are
    (area, cx, cy) tuples or plain (x, y) points.
    """
    def xs(m):
        return [p[-2] for p in m]

    def ys(m):
        return [p[-1] for p in m]

    return (min(xs(svg_marks)) - min(sx * v for v in xs(dxf_marks)),
            min(ys(svg_marks)) - min(sy * v for v in ys(dxf_marks)))


# ---------------------------------------------------------------------------
#  write the DXF
# ---------------------------------------------------------------------------
def rewrite_dxf(raw, dest):
    """Build the fabricator DXF from OpenSCAD's raw output.

    Returns a dict describing what was done, for the report and for the
    validator's expectations.
    """
    import ezdxf

    segs = dxf_segments(raw)
    rings, loose, note = closed_rings(segs)
    if not rings:
        raise RuntimeError(f"{raw}: no closed contour could be recovered from "
                           f"{len(segs)} LINE entities")

    # The outside profile is the contour that contains every other one. Decided
    # by area and then VERIFIED by containment, so a part whose largest
    # contour is not its perimeter fails here instead of shipping mislabelled.
    outer_i = max(range(len(rings)), key=lambda i: abs(ring_area(rings[i])))
    ob = ring_bbox(rings[outer_i])
    for i, r in enumerate(rings):
        if i == outer_i:
            continue
        b = ring_bbox(r)
        if not (ob[0] <= b[0] and ob[1] <= b[1]
                and b[2] <= ob[2] and b[3] <= ob[3]):
            raise RuntimeError(
                f"{raw}: contour {i} at {ring_centroid(r)} is not inside the "
                f"largest contour -- which of them is the outside profile?")

    fid = fiducial_ring(ob)

    doc = ezdxf.new("AC1015", setup=False)      # R2000: $INSUNITS is R13+
    doc.header["$INSUNITS"] = INSUNITS_MM
    doc.header["$MEASUREMENT"] = MEASUREMENT_METRIC
    doc.header["$LUNITS"] = LUNITS_DECIMAL
    for name, aci, descr in LAYERS:
        doc.layers.add(name, color=aci)
        # The meaning goes in the file too, so the fabricator's reader can
        # show what the order says. fabrication.md 1.6.
        doc.layers.get(name).description = descr

    msp = doc.modelspace()
    aci = {n: c for n, c, _ in LAYERS}

    def add_ring(ring, layer):
        # The ACI is set on the ENTITY as well as on the layer. BYLAYER alone
        # is correct CAD but it is invisible to a CAM front end that reads only
        # entities, which is the kind of reader this file has to survive.
        msp.add_lwpolyline([(x, y) for x, y in ring], close=True,
                           dxfattribs={"layer": layer, "color": aci[layer]})

    add_ring(rings[outer_i], "CUT_OUTER")
    for i, r in enumerate(rings):
        if i != outer_i:
            add_ring(r, "CUT_INNER")
    add_ring(fid, FID_LAYER)

    # Anything that would not close stays EXACTLY as OpenSCAD drew it. Forcing
    # a loop shut moves material; leaving it open is visible to the validator,
    # which refuses the file. CUT_INNER, because "cut first" is the safe order
    # for a contour whose role is unknown.
    for a, b in loose:
        msp.add_line(a, b, dxfattribs={"layer": "CUT_INNER",
                                       "color": aci["CUT_INNER"]})

    # Real drawing extents, measured off the geometry. OpenSCAD's file carries
    # AutoCAD's "no extents" sentinels (+/-1e20) because it has no header at
    # all. ezdxf's Drawing.update_all() copies the MODELSPACE layout's extents
    # into $EXTMIN/$EXTMAX on write, so setting the header alone is silently
    # discarded -- set the layout's.
    ext = bbox_union([ob, ring_bbox(fid)])
    msp.dxf.extmin = (ext[0], ext[1], 0.0)
    msp.dxf.extmax = (ext[2], ext[3], 0.0)
    doc.header["$EXTMIN"] = (ext[0], ext[1], 0.0)
    doc.header["$EXTMAX"] = (ext[2], ext[3], 0.0)

    os.makedirs(os.path.dirname(os.path.abspath(dest)), exist_ok=True)
    doc.saveas(dest)
    return {"raw_lines": len(segs), "rings": len(rings), "loose": len(loose),
            "note": note, "fiducial": fid, "part_bbox": ob,
            "fid_area": abs(ring_area(fid))}


# ---------------------------------------------------------------------------
#  write the SVG
# ---------------------------------------------------------------------------
def inject_svg_fiducial(path, fid_ring, dxf_rings):
    """Add the same fiducial to the SVG, and fix its fill and stroke.

    The model -> SVG map is SOLVED from the part contours present in these two
    files (handedness()), never hardcoded. If OpenSCAD's SVG writer ever stops
    flipping Y, the mark still lands in the right place here and
    `validate_pair()` is the thing that reports the change -- rather than this
    function silently writing a mark that agrees with itself by construction.
    """
    txt = open(path, encoding="utf-8").read()
    # IDEMPOTENT: strip a fiducial this function injected on an earlier run,
    # or running the post-process twice without re-rendering would leave the
    # SVG with two marks and one more subpath than the DXF has contours.
    txt = re.sub(r'\n?<path id="' + FID_SVG_ID + r'"[^>]*/>', "", txt)
    open(path, "w", encoding="utf-8").write(txt)

    fits, detail = handedness(dxf_rings, svg_subpaths(path))
    if len(fits) != 1:
        return None, (detail.get("error")
                      or f"{len(fits)} axis maps fit the two files; "
                         f"the SVG's handedness is not recoverable")
    sx, sy, _ = fits[0]
    ox, oy = solve_offset([ring_centroid(r) for r in dxf_rings],
                          [ring_centroid(r) for r in svg_subpaths(path)],
                          sx, sy)
    fid_svg = [(sx * x + ox, sy * y + oy) for x, y in fid_ring]

    # Six DECIMAL places, not six significant figures: OpenSCAD writes %.6g,
    # which rounds a coordinate like -120.5875 to -120.588 and put a 0.0005 mm
    # error into the handedness residual the first time this ran.
    d = " ".join(f"{'M' if i == 0 else 'L'} {x:.6f},{y:.6f}"
                 for i, (x, y) in enumerate(fid_svg)) + " z"

    # Its OWN <path>, with an id, so the next run can find it and replace it
    # rather than add a second one. svg_subpaths() reads every <path> in the
    # file, so it still counts as one contour of the sheet.
    if "</svg>" not in txt:
        return None, "the SVG has no </svg> -- it is not a complete document"
    mark = (f'<path id="{FID_SVG_ID}" d="{d}" stroke="black" fill="none" '
            f'stroke-width="{SVG_HAIRLINE}"/>')
    txt = txt.replace("</svg>", mark + "\n</svg>", 1)

    # fabrication.md 1.6 item 3 / 4.2 item 3: a filled path is how you order a
    # raster engrave by accident, and a 0.5 mm stroke puts a 0.250 mm
    # ambiguity on where the cut line is.
    txt = re.sub(r'fill="[^"]*"', 'fill="none"', txt)
    txt = re.sub(r'stroke-width="[^"]*"', f'stroke-width="{SVG_HAIRLINE}"', txt)

    # The sheet has to contain the mark, so grow the viewBox to the union and
    # keep width_mm / viewBox_width == 1.000 exactly (fabrication.md 4.2 item 1).
    boxes = [ring_bbox(r) for r in svg_subpaths(path)] + [ring_bbox(fid_svg)]
    u = bbox_union(boxes)
    x0 = math.floor(u[0] - SVG_MARGIN)
    y0 = math.floor(u[1] - SVG_MARGIN)
    x1 = math.ceil(u[2] + SVG_MARGIN)
    y1 = math.ceil(u[3] + SVG_MARGIN)
    w, h = x1 - x0, y1 - y0
    txt = re.sub(r'\bwidth="[^"]*"', f'width="{w}mm"', txt, count=1)
    txt = re.sub(r'\bheight="[^"]*"', f'height="{h}mm"', txt, count=1)
    txt = re.sub(r'\bviewBox="[^"]*"', f'viewBox="{x0} {y0} {w} {h}"', txt,
                 count=1)

    open(path, "w", encoding="utf-8").write(txt)
    return {"map": (sx, sy), "viewBox": (x0, y0, w, h),
            "fid_svg": fid_svg}, None


# ---------------------------------------------------------------------------
#  VALIDATE -- read the files back off disk and refuse them
# ---------------------------------------------------------------------------
def validate_dxf(dest, raw):
    """Every reason to refuse the DXF that is on disk. Returns a list of them.

    Reads the file back rather than trusting the writer, and reads the RAW
    export alongside it so the rewrite can be proved lossless against the
    artefact it came from instead of against the code that produced it.
    """
    import ezdxf
    from collections import Counter

    fails, facts = [], {}
    txt = open(dest, encoding="utf-8", errors="replace").read()

    # 1. The sections must be PHYSICALLY PRESENT. ezdxf invents a HEADER and
    #    TABLES in memory for a file that has none, so the parsed values are
    #    not evidence that the file carries them. fabrication.md 4.1 item 2.
    secs = []
    lines = [ln.strip() for ln in txt.splitlines()]
    for i, ln in enumerate(lines):
        if ln == "SECTION" and i + 2 < len(lines) and lines[i + 1] == "2":
            secs.append(lines[i + 2])
    facts["sections"] = secs
    for want in ("HEADER", "TABLES", "ENTITIES"):
        if want not in secs:
            fails.append(f"no {want} section in the file "
                         f"(sections present: {secs or 'none'})")
    if "$INSUNITS" not in txt:
        fails.append("the raw text does not contain $INSUNITS")
    if "$MEASUREMENT" not in txt:
        fails.append("the raw text does not contain $MEASUREMENT")

    # A file that will not parse must be REFUSED, not raise through the
    # pipeline. ezdxf raises DXFTypeError on an R2000 document whose HEADER
    # section has been lost, which is a corruption a receiver would also hit.
    try:
        doc = ezdxf.readfile(dest)
        msp = list(doc.modelspace())
    except Exception as ex:
        fails.append(f"ezdxf cannot read the file back: "
                     f"{type(ex).__name__}: {ex}")
        return fails, facts, [], None
    facts["dxfversion"] = doc.dxfversion
    facts["entities"] = dict(Counter(e.dxftype() for e in msp))

    # 2. Units. THE BLOCKING FIX.
    for var, want in (("$INSUNITS", INSUNITS_MM),
                      ("$MEASUREMENT", MEASUREMENT_METRIC)):
        got = doc.header.get(var, None)
        facts[var] = got
        if got != want:
            fails.append(f"{var} is {got!r}, want {want} "
                         f"({'millimetres' if var == '$INSUNITS' else 'metric'})"
                         f" -- a receiver will guess the scale")

    # 3. Layers: the table must declare them, with 1.6's colours, and every
    #    entity must sit on one of them.
    declared = {lay.dxf.name: lay.dxf.color for lay in doc.layers}
    facts["layers_declared"] = declared
    for name, aci, _ in LAYERS:
        if name not in declared:
            fails.append(f"layer {name} is not in the LAYER table "
                         f"(declared: {sorted(declared)})")
        elif declared[name] != aci:
            fails.append(f"layer {name} is ACI {declared[name]}, "
                         f"fabrication.md 1.6 declares {aci}")
    used = Counter(e.dxf.layer for e in msp)
    facts["layers_used"] = dict(used)
    allowed = {n for n, _, _ in LAYERS}
    for name in used:
        if name not in allowed:
            fails.append(f"{used[name]} entity(s) on undeclared layer "
                         f"{name!r}")
    if used.get(FID_LAYER, 0) != 1:
        fails.append(f"{used.get(FID_LAYER, 0)} entity(s) on {FID_LAYER}, "
                     f"want exactly 1 -- the orientation mark")
    if not used.get("CUT_OUTER"):
        fails.append("nothing on CUT_OUTER -- no outside profile")

    # 4. Entity types a receiver may silently drop. fabrication.md 4.1 item 4.
    for t in facts["entities"]:
        if t not in ENTITY_WHITELIST:
            fails.append(f"{facts['entities'][t]} x {t} -- not in the "
                         f"whitelist {sorted(ENTITY_WHITELIST)}")

    # 5. Entity count sane, and the rewrite lossless against the RAW export.
    raw_segs = dxf_segments(raw)
    n_raw = len(raw_segs)
    facts["raw_lines"] = n_raw
    if len(msp) < 2:
        fails.append(f"{len(msp)} entities -- a part and its mark is at least 2")
    if len(msp) > n_raw + 1:
        fails.append(f"{len(msp)} entities from {n_raw} raw LINEs; stitching "
                     f"can only reduce the count, so something was invented")

    drawn = 0
    fid_ring = None
    part_rings = []
    loose_segs = []
    for e in msp:
        if e.dxftype() == "LWPOLYLINE":
            pts = [(p[0], p[1]) for p in e.get_points()]
            if not e.closed:
                fails.append(f"an LWPOLYLINE on {e.dxf.layer} is not flagged "
                             f"closed -- a laser's worst input")
            if len(pts) < 3:
                fails.append(f"an LWPOLYLINE on {e.dxf.layer} has {len(pts)} "
                             f"vertices")
            if e.dxf.layer == FID_LAYER:
                fid_ring = pts
            else:
                part_rings.append(pts)
                drawn += len(pts)
        elif e.dxftype() == "LINE":
            drawn += 1
            loose_segs.append(((e.dxf.start.x, e.dxf.start.y),
                               (e.dxf.end.x, e.dxf.end.y)))
    facts["segments_drawn"] = drawn
    if drawn != n_raw:
        fails.append(f"the part's contours account for {drawn} segments but "
                     f"OpenSCAD drew {n_raw} -- the rewrite changed the "
                     f"geometry")

    # 5b. THE GEOMETRY ITSELF, VERTEX BY VERTEX, AGAINST THE RAW EXPORT.
    #
    #     The count above is only a count, and the only other comparison
    #     between the two files is per-contour |area| and centroid -- both
    #     INTEGRAL quantities that a single displaced vertex barely moves. On
    #     the real slab a vertex of the outer profile could be dragged
    #     6.782 mm before any other check here noticed: at that displacement
    #     the contour's area moves 1.000e-06 relative (TOL_AREA_REL 1e-06)
    #     and its centroid 1.799e-04 mm (TOL_MAP 1e-03), so the file was
    #     ACCEPTED with a 6.782 mm notch in a 12.700 mm acrylic perimeter
    #     [MEASURED 2026-11-07, by bisection on v2/export/dxf/slab.dxf].
    #     So "lossless" is asserted here as what it claims to be: the shipped
    #     file's segment multiset must EQUAL the raw export's, exactly.
    #
    #     This is the conservation argument closed_rings() already uses, but
    #     asserted on what is ON DISK rather than inside the writer that
    #     produced it -- the raw OpenSCAD export is the artefact being
    #     measured against, so no parameter is consulted either side.
    #     The fiducial is excluded: the rewrite invents it, so it is the one
    #     contour that is deliberately NOT in the raw file.
    want_segs = Counter(frozenset((_key(a), _key(b))) for a, b in raw_segs)
    got_segs = Counter()
    for ring in part_rings:
        got_segs.update(
            frozenset((_key(ring[i]), _key(ring[(i + 1) % len(ring)])))
            for i in range(len(ring)))
    got_segs.update(frozenset((_key(a), _key(b))) for a, b in loose_segs)
    if got_segs != want_segs:
        missing = want_segs - got_segs
        extra = got_segs - want_segs
        n_missing, n_extra = sum(missing.values()), sum(extra.values())
        sample = ""
        for src, label in ((extra, "invented"), (missing, "lost")):
            if src:
                pts = sorted(next(iter(src)))
                sample = (f" e.g. {label} "
                          + " -> ".join(f"{p[0]:.4f},{p[1]:.4f}" for p in pts))
                break
        facts["segment_conservation"] = (n_missing, n_extra)
        fails.append(
            f"the shipped DXF's geometry is not the raw export's: "
            f"{n_extra} segment(s) appear that OpenSCAD did not draw and "
            f"{n_missing} that it did are gone -- the rewrite moved "
            f"material.{sample}")
    n_loose = facts["entities"].get("LINE", 0)
    facts["loose_lines"] = n_loose
    if n_loose:
        # Left as lines deliberately rather than forced shut -- and still
        # refused, because an open contour must not reach a fabricator.
        fails.append(f"{n_loose} segment(s) would not stitch into a closed "
                     f"contour and were left as LINE entities; the file is "
                     f"not closed-loop and must not be sent")
    if fid_ring is None:
        fails.append("no fiducial contour on the DXF")

    # 6. $EXTMIN/$EXTMAX must be real, not OpenSCAD's 1e20 sentinels.
    try:
        lo, hi = doc.header["$EXTMIN"], doc.header["$EXTMAX"]
        facts["extents"] = (tuple(lo)[:2], tuple(hi)[:2])
        if not all(abs(v) < 1e6 for v in tuple(lo)[:2] + tuple(hi)[:2]):
            fails.append(f"$EXTMIN/$EXTMAX are sentinels {lo} {hi}")
    except Exception as ex:
        fails.append(f"$EXTMIN/$EXTMAX unreadable: {ex}")

    return fails, facts, part_rings, fid_ring


def _match_by_area(rings, target):
    """The rings whose |area| matches `target`'s. Area survives reflection AND
    translation, so this identifies the mark's counterpart even when it is in
    the wrong place -- which is the case the position check exists for."""
    ta = abs(ring_area(target))
    return [r for r in rings
            if abs(abs(ring_area(r)) - ta) <= TOL_AREA_REL * max(1.0, ta)]


def _locate_fiducial_disagreement(part_rings, fid_ring, svg_rings, facts):
    """Measure the fiducial against the map solved from the PART alone."""
    if not fid_ring:
        return []
    cand = _match_by_area(svg_rings, fid_ring)
    if len(cand) != 1:
        return [f"{len(cand)} SVG subpath(s) match the fiducial's area, "
                f"want exactly 1 -- the mark cannot be located in the SVG"]
    svg_fid = cand[0]
    svg_part = [r for r in svg_rings if r is not svg_fid]
    pf, pdetail = handedness(part_rings, svg_part)
    if len(pf) != 1:
        return [f"and the part's own contours fit {len(pf)} axis map(s), so "
                f"the disagreement cannot be localised"]
    sx, sy, _ = pf[0]
    ox, oy = solve_offset([ring_centroid(r) for r in part_rings],
                          [ring_centroid(r) for r in svg_part], sx, sy)
    dcx, dcy = ring_centroid(fid_ring)
    scx, scy = ring_centroid(svg_fid)
    dx, dy = abs((sx * dcx + ox) - scx), abs((sy * dcy + oy) - scy)
    facts["fiducial_vs_part_map"] = (dx, dy)
    if max(dx, dy) > TOL_MAP:
        return [f"the fiducial is at {dcx:+.3f},{dcy:+.3f} in the DXF and "
                f"{scx:+.3f},{scy:+.3f} in the SVG, which the map solved from "
                f"the part (x*{sx:+d}, y*{sy:+d}) does not reconcile "
                f"({dx:.4f},{dy:.4f} mm out)"]
    return [f"the part's contours disagree between the two files, but the "
            f"fiducial does not -- the mark is not the thing that moved"]


def validate_pair(svg, part_rings, fid_ring):
    """The DXF and the SVG must agree about handedness. Every export.

    fabrication.md 4.2 item 4 requires this to be CHECKED, not assumed.

    `part_rings` and `fid_ring` are the contours `validate_dxf()` recovered
    from the DXF on disk; the SVG is read here. Taking the DXF's side as
    arguments is what lets `test_dxf_post.py` feed this a synthetic pair and
    see the degeneracy rejection for real.
    """
    fails, facts = [], {}
    svg_rings = svg_subpaths(svg)
    all_dxf = part_rings + ([fid_ring] if fid_ring else [])
    fits, detail = handedness(all_dxf, svg_rings)
    facts["handedness"] = detail
    if detail.get("error"):
        fails.append(f"handedness: {detail['error']}")
        return fails, facts
    if len(fits) > 1:
        fails.append(
            f"handedness is DEGENERATE: {len(fits)} of the four axis maps "
            f"{[f[:2] for f in fits]} explain the two files equally well, so "
            f"a mirrored file could not be told from a correct one. The "
            f"fiducial exists to prevent exactly this.")
        return fails, facts
    if not fits:
        best = min(detail["scores"].items(), key=lambda kv: kv[1][0])
        fails.append(
            f"NO axis map explains the two files -- the DXF and the SVG are "
            f"not the same drawing. The closest is (x*{best[0][0]:+d}, "
            f"y*{best[0][1]:+d}) with a worst contour {best[1][0]:.4f} mm out "
            f"of place.")
        # Localise it. Solve the map from the PART alone -- which is
        # independent of where the mark sits -- and then measure the mark
        # against it. fabrication.md 4.2 item 4 asks for the fiducial's
        # position to be compared in the two files, and this is where that
        # comparison earns its keep rather than echoing the fit above.
        fails += _locate_fiducial_disagreement(part_rings, fid_ring,
                                               svg_rings, facts)
        return fails, facts
    sx, sy, res = fits[0]
    facts["map"] = (sx, sy)
    facts["residual"] = res
    facts["det"] = sx * sy
    # DXF is Y-up, SVG is Y-down: the map between them MUST be
    # orientation-reversing, or the SVG renders as a mirror of the DXF.
    if sx * sy != -1:
        fails.append(
            f"the model->SVG map is (x*{sx:+d}, y*{sy:+d}), determinant "
            f"{sx*sy:+d}. DXF is Y-up and SVG is Y-down, so the map must be "
            f"orientation-REVERSING (determinant -1) for the two files to "
            f"render alike. The SVG is a MIRROR of the DXF.")
    if (sx, sy) != (1, -1):
        fails.append(f"the model->SVG map is (x*{sx:+d}, y*{sy:+d}); "
                     f"OpenSCAD's writers give (x*+1, y*-1)")

    # Where the mark is in each file -- reported on every export, so the
    # build notes can be checked against the artefact rather than the source.
    if fid_ring:
        cand = _match_by_area(svg_rings, fid_ring)
        if len(cand) != 1:
            fails.append(f"{len(cand)} SVG subpath(s) match the fiducial's "
                         f"area {abs(ring_area(fid_ring)):.4f} mm2, want "
                         f"exactly 1")
        else:
            ox, oy = solve_offset([ring_centroid(r) for r in all_dxf],
                                  [ring_centroid(r) for r in svg_rings],
                                  sx, sy)
            dcx, dcy = ring_centroid(fid_ring)
            scx, scy = ring_centroid(cand[0])
            facts["fiducial_dxf"] = (dcx, dcy)
            facts["fiducial_svg"] = (scx, scy)
            facts["fiducial_delta"] = (abs((sx * dcx + ox) - scx),
                                       abs((sy * dcy + oy) - scy))
        # and it must be OUTSIDE the part, in the waste
        pb = bbox_union([ring_bbox(r) for r in part_rings])
        fb = ring_bbox(fid_ring)
        if not (fb[2] < pb[0] or fb[0] > pb[2]
                or fb[3] < pb[1] or fb[1] > pb[3]):
            fails.append("the fiducial overlaps the part's bounding box "
                         "-- it must be in the sheet's waste")

    # fabrication.md 4.2 items 1, 3 and 5.
    txt = open(svg, encoding="utf-8").read()
    mw = re.search(r'\bwidth="([0-9.]+)mm"', txt)
    mv = re.search(r'\bviewBox="(-?[0-9.]+) (-?[0-9.]+) '
                   r'([0-9.]+) ([0-9.]+)"', txt)
    if not mw or not mv:
        fails.append("the SVG's width is not in mm, or it has no viewBox")
    else:
        ratio = float(mw.group(1)) / float(mv.group(3))
        facts["svg_mm_per_unit"] = ratio
        if abs(ratio - 1.0) > 1e-6:
            fails.append(f"the SVG is {ratio:.9f} mm per user unit, want 1.0")
    if re.search(r'fill="(?!none)[^"]*"', txt):
        fails.append("the SVG path has a fill -- that is how you order a "
                     "raster engrave by accident")
    n_sub = len(svg_rings)
    facts["svg_subpaths"] = n_sub
    if n_sub != len(all_dxf):
        fails.append(f"{n_sub} SVG subpaths against {len(all_dxf)} DXF "
                     f"contours")
    # fabrication.md 4.2 item 5's SECOND half: every subpath ends in z. An
    # unclosed subpath is the SVG's version of the open contour item 5 of 4.1
    # refuses in the DXF, and it is invisible on screen because a renderer
    # closes the fill for you. This was recorded as a fact and never asserted,
    # so a subpath with no closepath shipped silently [MEASURED 2026-11-07].
    n_sub_z, n_closed = svg_closures(svg)
    facts["svg_z"] = n_closed
    if n_closed != n_sub_z:
        fails.append(f"{n_sub_z - n_closed} of the SVG's {n_sub_z} subpaths "
                     f"carry no closepath (z) -- an open contour in the file "
                     f"the fabricator reads")
    return fails, facts


# ---------------------------------------------------------------------------
#  the whole pass, for one part
# ---------------------------------------------------------------------------
def post_process(name, dxf_dir, svg_path):
    """Rewrite <dxf_dir>/<name>.dxf from <dxf_dir>/raw/<name>.dxf, mark the
    SVG, then read both back and refuse them if anything is wrong.

    Returns (fails, facts). An empty `fails` means the pair is safe to send.
    """
    raw = os.path.join(dxf_dir, "raw", f"{name}.dxf")
    dest = os.path.join(dxf_dir, f"{name}.dxf")
    facts = {}
    info = rewrite_dxf(raw, dest)
    facts["rewrite"] = {k: v for k, v in info.items() if k != "fiducial"}

    fails, dfacts, part_rings, fid_ring = validate_dxf(dest, raw)
    facts["dxf"] = dfacts

    if os.path.exists(svg_path):
        svginfo, err = inject_svg_fiducial(svg_path, info["fiducial"],
                                           part_rings)
        if err:
            fails.append(f"SVG: {err}")
        else:
            facts["svg"] = svginfo
            pf, pfacts = validate_pair(svg_path, part_rings, fid_ring)
            fails += pf
            facts["pair"] = pfacts
    else:
        fails.append(f"missing {svg_path}")
    return fails, facts


def report(name, facts):
    d = facts.get("dxf", {})
    rw = facts.get("rewrite", {})
    out = [f"    {name}.dxf  {d.get('dxfversion', '?')}  "
           f"sections {'+'.join(d.get('sections', []))}  "
           f"$INSUNITS={d.get('$INSUNITS')}  "
           f"$MEASUREMENT={d.get('$MEASUREMENT')}",
           f"    {' ':{len(name)}}       entities {d.get('entities')}  "
           f"from {rw.get('raw_lines')} raw LINEs  "
           f"loose {rw.get('loose')}",
           f"    {' ':{len(name)}}       layers {d.get('layers_used')}"]
    p = facts.get("pair", {})
    if "map" in p:
        out.append(f"    {' ':{len(name)}}       model->SVG map "
                   f"(x*{p['map'][0]:+d}, y*{p['map'][1]:+d}) det "
                   f"{p['det']:+d}, residual {p['residual']:.2e} mm, "
                   f"{p.get('handedness', {}).get('n_unique')} unique-area "
                   f"landmarks")
    if "fiducial_dxf" in p:
        fd, fs = p["fiducial_dxf"], p["fiducial_svg"]
        out.append(f"    {' ':{len(name)}}       fiducial DXF "
                   f"{fd[0]:+.3f},{fd[1]:+.3f}  SVG {fs[0]:+.3f},{fs[1]:+.3f}"
                   f"  reconciled to {max(p['fiducial_delta']):.1e} mm")
    return "\n".join(out)


def main():
    root = os.path.dirname(HERE)
    dxf_dir = os.path.join(root, "export", "dxf")
    names = sys.argv[1:] or [f[:-4] for f in sorted(os.listdir(
        os.path.join(dxf_dir, "raw"))) if f.endswith(".dxf")]
    bad = 0
    for n in names:
        svg = os.path.join(root, "export", "svg", f"{n}.svg")
        fails, facts = post_process(n, dxf_dir, svg)
        print(report(n, facts))
        for f in fails:
            print(f"    REJECT  {n}: {f}")
        bad += len(fails)
    print(f"  {'vector files accepted' if not bad else str(bad) + ' REJECTION(S)'}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
