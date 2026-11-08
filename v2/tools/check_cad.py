#!/usr/bin/env python3
"""Measure the EXPORTED cut files and meshes. Never the source.

The repo's standing discipline: a check that computes from the same parameters
as the thing it checks is worse than none, because it passes while the artefact
is wrong. So this reads the DXF the cutter will receive and the STL the slicer
will receive, recovers the geometry from them, and compares that against the
datums -- which it parses out of parameters.scad rather than re-declaring.

    /usr/bin/python3.11 v2/tools/check_cad.py

WHAT WAS ADDED, AND WHY EACH ONE IS HERE
----------------------------------------
v2/docs/fabrication.md found four classes of defect BY HAND that this file
walked straight past. Each is now measured:

 1. INSCRIBED diameter, not vertex diameter (fabrication.md 0.10, 4.1 items
    9-10). Every circle exports as an N-gon whose vertices sit ON the nominal
    circle, so a check that measures vertex radii reads 22.000 and PASSES
    while the material is proud at every chord. At $fn = 64 the O22.000 bore's
    inscribed diameter is 21.9735. Both diameters are now measured, on every
    arc in the file -- bores, fastener holes, slot end caps and every rounded
    corner -- and the faceting loss is printed so it can never hide again.

 2. ONE ROUND DATUM AND N SLOTS (fabrication.md 0.2, 3 row 7). PMMA's
    heat-and-moisture movement over the 289.560 mm screw span and the printed
    carrier's boss-pitch error are two independent +/-1.5 mm terms against zero
    float. Six round holes is a part that binds. The DXF is now classified
    SHAPE BY SHAPE: exactly one round hole, the rest stadiums, each stadium's
    overall length checked, and -- the part that actually matters -- each
    stadium's major axis compared with the bearing from its own centre to the
    datum's. A slot at the wrong angle floats in the useless direction and is
    invisible in a hole count.

 3. THE MINIMUM WEB INVENTORY (fabrication.md 0.8, 1.5, 4.1 item 11). The slab
    breaks the >= 1 x t web rule in thirteen places. That is an irreducible
    property of a frame with a big void in a sheet, not a bug -- so it is
    MEASURED and REPORTED, the narrowest pair is named, and it fails only if it
    gets WORSE than a recorded baseline or crosses a hard floor.

 4. THE KEYBOARD BRANCH (fabrication.md 5.1, parameters.scad 8a). "recessed"
    and "flush" are different parts. The void is measured from the DXF and
    checked against the branch the datums declare, and the Z chain is checked
    for consistency.

ARTEFACT PATHS CAN BE OVERRIDDEN, AND THAT IS NOT A CONVENIENCE
---------------------------------------------------------------
A check you have not watched fail is not a check. `--params` and `--export`
exist so that a deliberately broken copy -- a hand-edited parameters.scad, a
DXF exported under `-D '$fn=16'` -- can be put in front of this file in a
scratch directory and the failure observed, without touching the repo:

    openscad -D 'part="2d"' -D '$fn=16' -o /tmp/b/export/dxf/raw/slab.dxf \\
             v2/cad/slab.scad
    /usr/bin/python3.11 v2/tools/dxf_post.py ...            # layers + units
    /usr/bin/python3.11 v2/tools/check_cad.py --export /tmp/b/export
"""
import argparse
import itertools
import math
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPO = os.path.dirname(ROOT)

# ---------------------------------------------------------------------------
#  TOLERANCES. Every one of them has a reason; none is a round number picked
#  because it looked safe.
# ---------------------------------------------------------------------------
TOL = 0.05              # mm. Tessellation slop on a laser profile, and 2000x
                        #   binary STL's float32 round-trip error over this
                        #   part's dimensions.  [REPO fabrication.md 4.4]
TOL_DXF = 0.010         # mm. The DXF is ASCII decimal at 6 significant
                        #   figures, so at |x| ~ 152 it quantises to 0.001.
                        #   10x the artefact's own resolution.  [DERIVED]
CIRC_TOL = 0.003        # mm. Vertex-radius spread that still reads as a
                        #   circle: 3x the DXF's quantisation.  [DERIVED]
CORNER_TOL = 0.003      # mm. Two things, on the same basis as CIRC_TOL -- 3x
                        #   the DXF's 0.001 quantisation: the SPREAD between a
                        #   rounded rectangle's four independently fitted
                        #   corner radii, and each arc's own fit residual.
                        #   MEASURED on the shipped files: the outline's spread
                        #   is 4.7e-08 and the void's 6.6e-06, residuals
                        #   2.6e-04 and 4.7e-04 (the Kasa fit on 25 points
                        #   quantised to 0.001). So this is 450x the worst
                        #   spread and 6.4x the worst residual, and still 6.7x
                        #   inside the 0.020 single-corner error it exists to
                        #   name.  [DERIVED]
SHAPE_TOL = 0.006       # mm. How far a vertex may sit off the boundary of the
                        #   shape we claim it is. Two quantised coordinates
                        #   plus a quantised fitted axis.  [DERIVED]
BEARING_TOL = 0.25      # deg. A slot's axis against the bearing to the datum.
                        #   The apex vertices are exact, so the real error is
                        #   ~0.015 deg; this is 16x that and still 360x short
                        #   of the 90 deg mistake it exists to catch. [DESIGN]
FACET_SAGITTA_MAX = 0.010   # mm. Faceting sagitta budget for every circular
                            #   feature.  [REPO fabrication.md 4.1 item 10]
WEB_ADVISORY_T = 1.0        # x material thickness. The web/gap guidance that
                            #   this part knowingly breaks.
                            #   [3P fab-guides, via fabrication.md 1.4]
WEB_HARD_FLOOR = 2.000      # mm = 10 x the 0.200 mm kerf. Below this a web is
                            #   not a web.  [REPO fabrication.md 4.1 item 11]
WEB_BASELINE_TOL = 0.010    # mm. Baseline slack: 10x the DXF's own 0.001
                            #   quantisation, same basis as TOL_DXF.
                            #   [DERIVED]

#  THE RECORDED WEB BASELINE, and it is a MEASUREMENT, not a target.
#
#  fabrication.md 1.5 tabulated the slab's webs from an export that still had
#  SIX ROUND HOLES, and named the front rail (4.400) as the narrowest. That is
#  no longer true and the slots are why: the -x column's slots point at a datum
#  in the +x column, so the middle one lies ALONG x and its outboard end
#  reaches 1.500 mm further toward the sheet edge than a round hole did. The
#  land there goes 5.298 -> 3.798 and becomes the governing web. The front rail
#  meanwhile WIDENED, 4.400 -> 5.6825, because the recessed branch's void is
#  smaller. Both facts are properties of the part and both are accepted; what
#  is not accepted is either of them getting worse without anyone noticing.
#
#  Re-record ONLY with a deliberate geometry change, and only from the artefact:
#      /usr/bin/python3.11 v2/tools/export.py
#      /usr/bin/python3.11 v2/tools/check_cad.py     # prints the new block
WEB_BASELINE = {
    # part          min web   what it is between
    "slab":         3.798,  # [MEASURED 2026-11-07, v2/export/dxf/slab.dxf]
                            #   outline <-> the -x middle fastener SLOT
    "backpanel":    2.248,  # [MEASURED 2026-11-07, v2/export/dxf/backpanel.dxf]
                            # Was 4.320, then 2.360, then 2.6748, now 2.2484 --
                            # and the binding PAIR has moved three times. At
                            # 4.320 it was the outline. At 2.360 it was the
                            # 18650 window against one of the cowl's four
                            # screws, which stood at the window's edge
                            # midpoints. At 2.6748 those four had moved to the
                            # flange's corners (parameters.scad 12 has why:
                            # three of the four had no thread in the carrier at
                            # all) and the nearest thing to them was a
                            # back-panel fastener SLOT.
                            #   IT MOVED AGAIN AND THE LOSS IS DELIBERATE.
                            # cowl_screw_dx() went 48.895 -> 47.625 because at
                            # 48.895 the cowl's own O4.900 counterbore stood
                            # 0.8542 from a washer scallop -- the thinnest wall
                            # in that part, under the 1.500 it is drawn to.
                            # Moving the four screws 1.270 inboard takes them
                            # AWAY from the fastener slot (2.6748 -> 3.7067) and
                            # TOWARD the 18650 window, and parameters.scad 12
                            # shows the two brackets leave no dx that holds both
                            # 1.500 in the cowl and 2.6748 here -- the window is
                            # empty by 0.0265 mm. So this baseline is
                            # re-recorded, measured, rather than held.
                            #   18650 window <-> cowl screw
                            # ABOVE THE 2.000 HARD FLOOR BY 0.2484, which is
                            # asserted separately from this baseline, where the
                            # 2.6748 pair cleared it by 0.6748 and the 2.360
                            # pair by 0.3600.
                            #   AND THE PAIR IS NOT TWO CONTOURS BUT ONE. The
                            # thing the cowl screw approaches is the display's
                            # O5.500 head clearance hole at (-47.315, +17.9125),
                            # which has MERGED into the window -- backpanel.scad
                            # draws it knowing it does, because the 18650
                            # holder's edge runs through the display's two lower
                            # mounting holes. The merged contour measures
                            # 91.000 x 25.588 against cowl_window()'s 88.270 x
                            # 22.373, so no check may treat the window's
                            # nominal rectangle as its boundary.
                            #   AND THE REFUTATION THAT USED TO CLOSE THIS NOTE
                            # WAS STALE. It said the corner layout "landed them
                            # 5.306 from the display's own screw-head clearance
                            # holes -- a 1.104 web -- and this check is what
                            # caught it". Both figures were computed at
                            # cowl_flange = 6.350, the value the same sentence
                            # says was refused. At the live 7.620 the window's
                            # corner is (-42.122, +14.568), the nearest head
                            # hole is (-47.315, +17.9125), the centres are
                            # 6.1768 apart and the web is 1.9768: refused by
                            # 0.0232, not by 0.896. The 2 x 2 recorded now is
                            # NOT that layout, and the "2.0496" that used to
                            # close this sentence is NOT REPRODUCIBLE off any
                            # artefact here: the figure it was reaching for is
                            # the measured 2.2484 above, against the merged
                            # contour the head hole is part of.
}

def part_layers():
    """dxf_post.py's own layer names, IMPORTED and not copied.

    This was a hardcoded ("CUT_OUTER", "CUT_INNER"), and the stale-copy failure
    mode is a WRONG ANSWER rather than an error. Rename both part layers in
    dxf_post and nothing in this file matches them, dxf_part_loops() falls
    through to dxf_loops(), and the FIDUCIAL -- an orientation token in the
    sheet's waste, not part geometry -- joins the part inventory. Measured on
    the slab with the two layers renamed PROFILE / INTERNAL in a scratch copy
    of dxf_post:

        16 cut contours where there are 15
        120 web pairs where there are 105
        the "F" at -170.080,-112.968 in the edge-distance table
        "? corner 0: faceting sagitta 5.8301" against the F's square corner

    LAZY, and that is forced rather than stylistic. dxf_post.py does
    `from check_cad import stitch` at its own module level, so a module-level
    `from dxf_post import PART_LAYERS` here breaks whichever of the two is
    loaded second: PART_LAYERS is declared below that import in dxf_post, so
    running dxf_post.py first would hit a half-built module. Inside a function
    the cycle resolves either way round.
    """
    global _PART_LAYERS
    if _PART_LAYERS is None:
        here = os.path.dirname(os.path.abspath(__file__))
        if here not in sys.path:
            sys.path.insert(0, here)
        import dxf_post
        got = tuple(dxf_post.PART_LAYERS)
        if dxf_post.FID_LAYER in got:
            raise RuntimeError(
                f"dxf_post.PART_LAYERS contains its own FID_LAYER "
                f"{dxf_post.FID_LAYER!r}: the fiducial would be counted as "
                f"part geometry and every inventory in this file would be off "
                f"by one.")
        _PART_LAYERS = got
    return _PART_LAYERS


_PART_LAYERS = None


# ---- the datums, parsed from the one file that owns them ------------------
#  A STATEMENT-AT-A-TIME parser, not a line-at-a-time one. The old one was
#  line-based and silently dropped every multi-line assignment in the file --
#  back_screw_kbd, kbd_overlap, kbd_cap_clear, disp_board -- and every
#  assignment that indexes a list, because it rewrote `[` as `(` and turned
#  `kbd_frame[0]` into a call. Both are now parsed: OpenSCAD's list literal and
#  its indexing are spelled exactly as Python's, so the honest transform is no
#  transform at all.
_SCAD_FNS = {
    "min": min, "max": max, "abs": abs, "len": len, "pow": pow,
    "round": round, "floor": math.floor, "ceil": math.ceil,
    "sqrt": math.sqrt,
    "concat": lambda *a: [x for s in a for x in s],
}


def _strip_comment(line):
    """Split a line into (code, comment), respecting string literals."""
    quoted = False
    for i, c in enumerate(line):
        if c == '"':
            quoted = not quoted
        elif not quoted and c == "/" and line[i + 1:i + 2] == "/":
            return line[:i], line[i + 2:]
    return line, ""


def datums(path=None):
    """Evaluate parameters.scad's scalar and list assignments, in order.

    OpenSCAD is not Python, but the subset this file uses -- scalars, simple
    arithmetic, list literals, list indexing -- evaluates identically. Anything
    that does not (`function`, `module`, list comprehensions, ranges) fails to
    match or fails to eval and is skipped rather than guessed at.

    Returns (env, prov): the values, and each name's provenance tag.
    """
    path = path or os.path.join(ROOT, "cad", "parameters.scad")
    env, prov, buf = {}, {}, ""
    for line in open(path, encoding="utf-8").read().splitlines():
        code, comment = _strip_comment(line)
        buf += " " + code
        while ";" in buf:
            stmt, buf = buf.split(";", 1)
            m = re.match(r"\s*(\$?\w+)\s*=\s*(.+)$", stmt, re.S)
            if not m:
                continue
            name, expr = m.group(1), m.group(2).strip()
            tag = re.search(r"\[(\w+)\]", comment)
            if tag:
                prov[name] = tag.group(1)
            try:
                env[name] = eval(expr, {"__builtins__": {}},
                                 dict(_SCAD_FNS, **env))
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


def dxf_part_loops(path):
    """The loops that are PART geometry, with the fiducial excluded.

    dxf_post.py puts the orientation mark on its own FIDUCIAL layer precisely
    so it can be told apart from the part; counting it as a cut contour would
    make every inventory off by one. A raw OpenSCAD export has no layers at
    all, so that case falls back to everything.

    THE FALLBACK IS NARROWED TO THE CASE IT WAS WRITTEN FOR. A raw export puts
    every entity on layer "0"; a post-processed file puts them on named layers.
    If the file has named layers and NONE of them is a part layer, the fallback
    would hand back the fiducial as part geometry and every count downstream
    would be quietly wrong -- so that case raises instead. It is the only
    reading under which this function cannot return a wrong answer.
    """
    import ezdxf
    doc = ezdxf.readfile(path)
    want = part_layers()
    loops, seen = [], set()
    for e in doc.modelspace():
        seen.add(e.dxf.layer)
        if e.dxf.layer not in want:
            continue
        if e.dxftype() == "LWPOLYLINE":
            pts = [(p[0], p[1]) for p in e.get_points()]
            if e.closed and len(pts) > 2:
                loops.append(pts)
        elif e.dxftype() == "POLYLINE":
            pts = [(v.dxf.location.x, v.dxf.location.y) for v in e.vertices]
            if len(pts) > 2:
                loops.append(pts)
    if loops:
        return loops
    if seen - {"0", "Defpoints"}:
        raise RuntimeError(
            f"{os.path.basename(path)} carries named layers "
            f"{sorted(seen)} and not one of them is a PART layer "
            f"{list(want)} as dxf_post.py declares them. Falling back to every "
            f"contour in the file would count the fiducial as part geometry: "
            f"on the slab that reads 16 cut contours where there are 15. "
            f"Either dxf_post.LAYERS was renamed without re-posting the export "
            f"or the file was written by something else.")
    return dxf_loops(path)


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


# ===========================================================================
#  SHAPE RECOVERY -- what kind of contour is this, measured from its vertices
# ===========================================================================
def _solve3(A, b):
    """3x3 solve with partial pivoting. No numpy, so this module imports
    cheaply for dxf_post.py, which only wants stitch()."""
    M = [row[:] + [bi] for row, bi in zip(A, b)]
    for c in range(3):
        p = max(range(c, 3), key=lambda r: abs(M[r][c]))
        if abs(M[p][c]) < 1e-14:
            return None
        M[c], M[p] = M[p], M[c]
        for r in range(3):
            if r == c:
                continue
            f = M[r][c] / M[c][c]
            for k in range(c, 4):
                M[r][k] -= f * M[c][k]
    return [M[i][3] / M[i][i] for i in range(3)]


def circle_fit(pts):
    """Algebraic (Kasa) circle fit. Returns (cx, cy, r, max_residual)."""
    if len(pts) < 3:
        return None
    sx = sy = sxx = syy = sxy = sz = szx = szy = 0.0
    for x, y in pts:
        z = x*x + y*y
        sx += x; sy += y; sxx += x*x; syy += y*y; sxy += x*y
        sz += z; szx += z*x; szy += z*y
    n = float(len(pts))
    sol = _solve3([[2*sxx, 2*sxy, sx], [2*sxy, 2*syy, sy], [2*sx, 2*sy, n]],
                  [szx, szy, sz])
    if sol is None:
        return None
    a, b, c = sol
    disc = c + a*a + b*b
    if disc <= 0:
        return None
    r = math.sqrt(disc)
    res = max(abs(math.hypot(x - a, y - b) - r) for x, y in pts)
    return a, b, r, res


class Ring:
    """One recovered closed contour, and everything measurable about it."""

    def __init__(self, pts):
        from shapely.geometry import LinearRing, Polygon
        self.pts = pts
        self.poly = Polygon(pts)
        self.lr = LinearRing(pts)
        self.area = self.poly.area
        c = self.poly.centroid
        self.cx, self.cy = c.x, c.y
        xs = [p[0] for p in pts]
        ys = [p[1] for p in pts]
        self.bx, self.by = (min(xs)+max(xs))/2, (min(ys)+max(ys))/2
        self.w, self.h = max(xs)-min(xs), max(ys)-min(ys)
        self.n = len(pts)
        self.kind, self.shape = self._classify()
        self.label = "?"

    # -- shape tests, in order of specificity ------------------------------
    def _classify(self):
        rs = [math.hypot(x - self.cx, y - self.cy) for x, y in self.pts]
        if max(rs) - min(rs) <= CIRC_TOL and abs(self.w - self.h) <= CIRC_TOL:
            return "circle", self._circle(max(rs))
        s = self._stadium()
        if s:
            return "slot", s
        r = self._rrect()
        if r:
            return "rrect", r
        return "other", {}

    def _circle(self, r_vertex):
        """Vertex radius, INSCRIBED radius, and the faceting loss between.

        fabrication.md 0.10 is the whole reason this exists: OpenSCAD puts the
        polygon's vertices ON the nominal circle, so the vertex radius reads
        nominal no matter how coarse $fn is, while the material stands proud by
        the sagitta at every chord. The inscribed radius is the one the part
        has, and shapely's point-to-ring distance is the apothem.
        """
        from shapely.geometry import Point
        r_insc = Point(self.cx, self.cy).distance(self.lr)
        return _facet(r_vertex, r_insc, self.area)

    def _stadium(self):
        """A slot: the convex hull of two equal circles.

        Proved on the vertices, not assumed from the bounding box, because a
        bounding box cannot tell a 7.445 slot lying at -6.26 deg from a 7.426
        one lying flat -- which is exactly the pair this check has to separate.
        """
        from shapely.geometry import Point
        p, q, L = _farthest_pair(self.pts)
        if L <= 0:
            return None
        ux, uy = (q[0]-p[0])/L, (q[1]-p[1])/L
        mx, my = (p[0]+q[0])/2, (p[1]+q[1])/2
        if math.hypot(mx - self.cx, my - self.cy) > SHAPE_TOL:
            return None                 # a stadium's centroid is its midpoint
        st = [((x-mx)*ux + (y-my)*uy, -(x-mx)*uy + (y-my)*ux)
              for x, y in self.pts]
        W = 2 * max(abs(t) for _, t in st)
        if W > L + SHAPE_TOL:
            return None
        half = (L - W) / 2
        for s, t in st:                 # every vertex on the stadium boundary
            if abs(s) <= half + SHAPE_TOL:
                if abs(abs(t) - W/2) > SHAPE_TOL:
                    return None
            elif abs(math.hypot(abs(s) - half, t) - W/2) > SHAPE_TOL:
                return None
        caps = [(mx + ux*half, my + uy*half), (mx - ux*half, my - uy*half)]
        # the end cap's own vertex radius and apothem, about the cap's centre
        r_v = W / 2
        for v, (s, _) in zip(self.pts, st):
            if s > half + SHAPE_TOL:
                r_v = max(r_v, math.hypot(v[0] - caps[0][0],
                                          v[1] - caps[0][1]))
        r_i = min(Point(*c).distance(self.lr) for c in caps)
        f = _facet(r_v, r_i, math.pi * r_v * r_v)
        return dict(l_vertex=2*half + 2*r_v, l_inscribed=2*half + 2*r_i,
                    width=W, straight=2*half,
                    bearing=math.degrees(math.atan2(uy, ux)) % 180.0,
                    caps=caps, facet=f)

    def corner_arcs(self):
        """Fit the four corner arcs of this contour INDEPENDENTLY.

        Separated from _rrect() because it is useful on a contour _rrect() has
        just REFUSED: that is the case where something is wrong with a corner
        and the only thing a reader needs is which corner and by how much.
        Returns None only if a corner has too few vertices to fit at all.
        """
        from shapely.geometry import Point
        if self.w <= 0 or self.h <= 0:
            return None
        arcs = []
        for sx, sy in ((1, 1), (1, -1), (-1, 1), (-1, -1)):
            d = Point(self.bx + sx*self.w/2,
                      self.by + sy*self.h/2).distance(self.lr)
            r0 = d / (math.sqrt(2) - 1)
            if r0 <= SHAPE_TOL:                     # a genuinely square corner
                arcs.append(dict(sx=sx, sy=sy, r=0.0, resid=0.0, n=0,
                                 cx=self.bx + sx*self.w/2,
                                 cy=self.by + sy*self.h/2))
                continue
            sel = [v for v in self.pts
                   if sx*(v[0]-self.bx) > self.w/2 - r0*1.1
                   and sy*(v[1]-self.by) > self.h/2 - r0*1.1]
            fit = circle_fit(sel)
            if fit is None:
                return None
            cx, cy, r, res = fit
            arcs.append(dict(sx=sx, sy=sy, r=r, resid=res, n=len(sel),
                             cx=cx, cy=cy))
        return arcs

    def _rrect(self):
        """An axis-aligned rounded rectangle, with its corner radius FITTED.

        The outline and the keyboard void. The radius is not read off a datum:
        it is recovered from each of the four corner arcs independently.

        WHAT THIS FUNCTION DOES AND DOES NOT DECIDE. It returns the four fits
        and the spread between them; it does NOT gate the spread. That gate is
        check_corner_radii(), which has the datums and can name the corner. The
        docstring used to claim the four "are required to agree -- which is how
        a void whose corners were drawn at the wrong radius gets caught", and
        the agreement was computed, printed and never tested: a single corner at
        R3.195 instead of R3.175 does get caught, but only indirectly, by the
        per-vertex boundary test below refusing the whole contour, so the
        operator reads "no rounded-rectangular keyboard void in the DXF" and an
        unaccounted contour and is told nothing about a corner. Both paths now
        exist: the indirect refusal stays, and the named gate is explicit.
        """
        from shapely.geometry import Point
        arcs = self.corner_arcs()
        if arcs is None:
            return None
        rs = [a["r"] for a in arcs]
        r = sum(rs) / 4.0
        # every vertex must lie on the boundary of the rounded rect we fitted
        for x, y in self.pts:
            dx = abs(x - self.bx) - (self.w/2 - r)
            dy = abs(y - self.by) - (self.h/2 - r)
            if dx > 0 and dy > 0:
                off = abs(math.hypot(dx, dy) - r)
            else:
                off = min(abs(abs(x-self.bx) - self.w/2),
                          abs(abs(y-self.by) - self.h/2))
            if off > max(SHAPE_TOL, 2 * max(a["resid"] for a in arcs)):
                return None
        facets = []
        for a in arcs:
            if a["r"] <= 0:
                continue
            r_v = max(math.hypot(v[0]-a["cx"], v[1]-a["cy"]) for v in self.pts
                      if a["sx"]*(v[0]-self.bx) > self.w/2 - a["r"]*1.1
                      and a["sy"]*(v[1]-self.by) > self.h/2 - a["r"]*1.1)
            r_i = Point(a["cx"], a["cy"]).distance(self.lr)
            facets.append(_facet(r_v, r_i, math.pi*r_v*r_v))
        return dict(w=self.w, h=self.h, r=r,
                    r_spread=max(rs) - min(rs),
                    arc_resid=max(a["resid"] for a in arcs),
                    arcs=arcs, facets=facets)


def _facet(r_vertex, r_inscribed, area):
    """The faceting record for one arc, all of it measured.

    `n_eff` inverts the sagitta: s = r(1 - cos(pi/n)). `n_min` is the coarsest
    tessellation that would still meet the budget at this radius. Reporting
    both is what makes a $fn regression legible instead of mysterious.
    """
    sag = r_vertex - r_inscribed
    n_eff = n_min = None
    if sag > 1e-12 and r_vertex > 0:
        c = 1.0 - sag / r_vertex
        if -1.0 < c < 1.0:
            n_eff = math.pi / math.acos(c)
    if r_vertex > 0:
        c = 1.0 - FACET_SAGITTA_MAX / r_vertex
        n_min = math.ceil(math.pi / math.acos(c)) if -1.0 < c < 1.0 else 3
    return dict(r_vertex=r_vertex, r_inscribed=r_inscribed,
                d_vertex=2*r_vertex, d_inscribed=2*r_inscribed,
                d_area=2*math.sqrt(max(area, 0.0)/math.pi),
                sagitta=sag, n_eff=n_eff, n_min=n_min)


def _farthest_pair(pts):
    best = (None, None, -1.0)
    for i in range(len(pts)):
        xi, yi = pts[i]
        for j in range(i + 1, len(pts)):
            d = math.hypot(xi - pts[j][0], yi - pts[j][1])
            if d > best[2]:
                best = (pts[i], pts[j], d)
    return best


def _pt_dist(x, y, ring):
    from shapely.geometry import Point
    return Point(x, y).distance(ring.lr)


def _bearing(ax, ay, bx, by):
    return math.degrees(math.atan2(by - ay, bx - ax)) % 180.0


def _bearing_err(a, b):
    """Smallest angle between two undirected bearings, in degrees."""
    d = abs(a - b) % 180.0
    return min(d, 180.0 - d)


def _show180(a):
    """A bearing for printing. A slot's axis is undirected, so 179.999 and
    0.001 are the same line; print the one a reader can compare at a glance
    instead of the one the modulus happened to land on."""
    return a - 180.0 if a > 179.5 else a


# ===========================================================================
#  CHECK 1 -- INSCRIBED DIAMETER, AND THE FACETING LOSS, ON EVERY ARC
# ===========================================================================
def check_faceting(rings, nominals, fails, lines):
    """fabrication.md 0.10 and 4.1 items 9-10.

    Gates the INSCRIBED diameter and the sagitta. The vertex diameter is
    checked too, but it is checked in the knowledge that it is the measurement
    that lied: it reads nominal at any $fn.
    """
    lines.append("    faceting -- vertex d, INSCRIBED d, and the loss between")
    lines.append(f"      {'feature':26s} {'nominal':>9s} {'vertex':>9s} "
                 f"{'inscribed':>10s} {'area-eq':>9s} {'sagitta':>8s} "
                 f"{'n~':>5s} {'n_min':>6s}")
    arcs = []
    for r in rings:
        if r.kind == "circle":
            arcs.append((r.label, r.shape, _nearest(r.shape["d_vertex"],
                                                    nominals)))
        elif r.kind == "slot":
            arcs.append((r.label + " end cap", r.shape["facet"],
                         _nearest(r.shape["width"], nominals)))
        elif r.kind == "rrect":
            for i, f in enumerate(r.shape["facets"]):
                arcs.append((f"{r.label} corner {i}", f,
                             _nearest(f["d_vertex"], nominals)))
    for label, f, nom in arcs:
        nstr = f"{f['n_eff']:.1f}" if f["n_eff"] else "exact"
        lines.append(
            f"      {label:26s} {('%.3f' % nom) if nom else '-':>9s} "
            f"{f['d_vertex']:9.4f} {f['d_inscribed']:10.4f} "
            f"{f['d_area']:9.4f} {f['sagitta']:8.4f} {nstr:>5s} "
            f"{f['n_min'] or '-':>6}")
        if f["sagitta"] > FACET_SAGITTA_MAX + 1e-9:
            fails.append(
                f"{label}: faceting sagitta {f['sagitta']:.4f} exceeds the "
                f"{FACET_SAGITTA_MAX:.3f} budget -- the material stands that "
                f"proud of the nominal arc at every chord. At r = "
                f"{f['r_vertex']:.3f} the drawing needs $fn >= {f['n_min']}; "
                f"the artefact was tessellated at about "
                f"{(f['n_eff'] or 0):.0f}. fabrication.md 4.1 item 10")
        if nom is None:
            continue
        if abs(f["d_vertex"] - nom) > TOL_DXF:
            fails.append(f"{label}: vertex diameter {f['d_vertex']:.4f}, "
                         f"want {nom:.3f}")
        if f["d_inscribed"] < nom - 2*FACET_SAGITTA_MAX - 1e-9:
            fails.append(
                f"{label}: INSCRIBED diameter {f['d_inscribed']:.4f} against a "
                f"nominal {nom:.3f} -- short by {nom - f['d_inscribed']:.4f}, "
                f"more than the {2*FACET_SAGITTA_MAX:.3f} diametral faceting "
                f"budget. A vertex-radius check would have read "
                f"{f['d_vertex']:.4f} and passed. fabrication.md 4.1 item 9")
    lines.append(f"      n~ is the facet count the MEASURED sagitta implies, "
                 f"s = r(1 - cos(pi/n)); the DXF's own 0.001 mm quantisation "
                 f"(6 significant figures) inflates the sagitta on small arcs, "
                 f"so n~ reads low below r ~ 3. n_min is the coarsest $fn that "
                 f"still meets the {FACET_SAGITTA_MAX:.3f} mm budget at that "
                 f"radius.")


def _nearest(d, nominals):
    if not nominals:
        return None
    n = min(nominals, key=lambda v: abs(v - d))
    return n if abs(n - d) < 0.30 else None


# ===========================================================================
#  CHECK 2 -- ONE ROUND DATUM, N SLOTS, AND EVERY SLOT POINTING AT THE DATUM
# ===========================================================================
def check_datum_and_slots(rings, pattern, datum_i, clear_d, slot_l,
                          fails, lines):
    """fabrication.md 0.2 and 3 row 7; parameters.scad 6; lib/util.scad.

    `pattern` is the expected hole centres, transcribed from parameters.scad's
    own `slab_screw_pts()` / `back_screw_pts()`. THOSE ARE FUNCTIONS, so the
    datum parser cannot evaluate them, and this is the one place in this file
    where a shape of geometry is restated rather than read. It is restated
    loudly and it is checked three ways -- count, position and leftovers -- so
    a change to those functions fails here instead of passing quietly.

    Everything that actually matters is measured from the DXF: which hole is
    round, how long each slot is, and WHICH WAY EACH SLOT POINTS. A slot at the
    wrong angle floats in the useless direction and is identical in a count.
    """
    found = []
    for r in rings:
        if r.kind == "circle" and abs(r.shape["d_vertex"] - clear_d) < 0.30:
            found.append(r)
        elif r.kind == "slot" and abs(r.shape["width"] - clear_d) < 0.30:
            found.append(r)
    rounds = [r for r in found if r.kind == "circle"]
    slots = [r for r in found if r.kind == "slot"]

    lines.append(f"    fasteners -- {len(rounds)} round datum + {len(slots)} "
                 f"slot(s), from the DXF's own shapes")
    if len(rounds) != 1:
        fails.append(
            f"{len(rounds)} ROUND fastener hole(s) in the DXF and "
            f"{len(slots)} slot(s); want exactly 1 round datum and "
            f"{len(pattern)-1} slots. PMMA moves 1.448 mm over the 289.560 mm "
            f"screw span and the printed carrier's boss pitch moves "
            f"+/-1.448 mm on the same span -- two independent +/-1.5 mm terms "
            f"against zero float. fabrication.md 0.2, 3 row 7")
    if len(found) != len(pattern):
        fails.append(f"{len(found)} fastener features in the DXF, want "
                     f"{len(pattern)} (parameters.scad's screw pattern)")

    # -- position: every expected centre is in the artefact, and vice versa --
    unmatched = list(found)
    for k, (px, py) in enumerate(pattern):
        hit = [r for r in unmatched
               if math.hypot(r.cx - px, r.cy - py) <= TOL_DXF]
        if not hit:
            fails.append(f"no fastener feature at {px:+.3f},{py:+.3f} "
                         f"(pattern index {k})")
            continue
        r = hit[0]
        unmatched.remove(r)
        r.label = f"fastener {k}"
        want_round = (k == datum_i)
        if want_round and r.kind != "circle":
            fails.append(f"fastener {k} at {px:+.3f},{py:+.3f} is the DATUM "
                         f"(screw_datum_i = {datum_i}) and must be round; the "
                         f"DXF has a {r.kind}")
        if not want_round and r.kind != "slot":
            fails.append(f"fastener {k} at {px:+.3f},{py:+.3f} must be a slot; "
                         f"the DXF has a {r.kind}")
    for r in unmatched:
        fails.append(f"fastener-sized feature at {r.cx:+.3f},{r.cy:+.3f} "
                     f"matches no point in parameters.scad's screw pattern")

    if not rounds:
        return
    datum = rounds[0]
    if abs(datum.shape["d_vertex"] - clear_d) > TOL_DXF:
        fails.append(f"datum hole is {datum.shape['d_vertex']:.4f}, "
                     f"want {clear_d:.3f}")
    lines.append(f"      datum  {datum.label:14s} ({datum.cx:+9.3f},"
                 f"{datum.cy:+9.3f})  O{datum.shape['d_vertex']:.4f} round")
    lines.append(f"      {'slot':14s} {'centre':>21s} {'overall L':>10s} "
                 f"{'inscr L':>9s} {'W':>7s} {'axis':>8s} {'->datum':>8s} "
                 f"{'err':>6s}")
    for r in sorted(slots, key=lambda s: s.label):
        s = r.shape
        want = _bearing(r.cx, r.cy, datum.cx, datum.cy)
        err = _bearing_err(s["bearing"], want)
        lines.append(
            f"      {r.label:14s} ({r.cx:+9.3f},{r.cy:+9.3f}) "
            f"{s['l_vertex']:10.4f} {s['l_inscribed']:9.4f} "
            f"{s['width']:7.4f} {_show180(s['bearing']):8.3f} "
            f"{_show180(want):8.3f} {err:6.3f}")
        if abs(s["l_vertex"] - slot_l) > TOL_DXF:
            fails.append(f"{r.label}: overall slot length "
                         f"{s['l_vertex']:.4f}, want {slot_l:.3f}")
        if abs(s["width"] - clear_d) > TOL_DXF:
            fails.append(f"{r.label}: slot width {s['width']:.4f}, "
                         f"want {clear_d:.3f}")
        if err > BEARING_TOL:
            fails.append(
                f"{r.label} at {r.cx:+.3f},{r.cy:+.3f} points at "
                f"{s['bearing']:.3f} deg but the datum at {datum.cx:+.3f},"
                f"{datum.cy:+.3f} bears {want:.3f} deg -- off by {err:.3f}. "
                f"A slot at the wrong angle floats in the useless direction "
                f"and is invisible in a hole count. lib/util.scad's "
                f"datum_and_slots, fabrication.md 0.2")
    return found


# ===========================================================================
#  CHECK 2b -- EVERY ROUNDED CORNER, GATED AND NAMED
# ===========================================================================
def _corner_name(a):
    return f"{'+x' if a['sx'] > 0 else '-x'}/{'+y' if a['sy'] > 0 else '-y'}"


def check_corner_radii(rings, env, fails, lines):
    """Gate the four corner radii of every rounded rectangle, by NAME.

    The gate _rrect's docstring used to claim and never applied. Three things
    are tested and they catch different mistakes:

      SPREAD     the four fitted radii against each other. This is the one that
                 names a single corner drawn at the wrong radius. It does not
                 need a datum, so it works on any rounded rectangle in the file.
      RESIDUAL   each arc's own circle-fit residual. A corner that is a chamfer,
                 an ellipse or a different radius part-way round fits a circle
                 badly, and the mean radius alone cannot see it.
      NOMINAL    the mean against the datum, where this file knows which datum
                 governs. That one already existed for the void; it is here too
                 so the outline's corner_r is no longer unchecked.

    THE MEAN IS NOT ENOUGH AND THAT IS THE WHOLE POINT. One corner at R3.195
    with three at R3.175 has a mean of R3.180, which is inside TOL_DXF, so the
    nominal gate passes it. The spread is 0.020 and this fires.
    """
    want = {"outline": "corner_r", "keyboard void": "kbd_void_r"}
    rr = [r for r in rings if r.kind == "rrect"]
    if not rr:
        return
    lines.append("    corners -- the four arcs of every rounded rectangle, "
                 "fitted one at a time")
    lines.append(f"      {'contour':20s} {'datum':>8s} {'mean r':>9s} "
                 f"{'spread':>10s} {'resid':>10s}   per-corner r")
    for r in rr:
        s = r.shape
        nm = want.get(r.label)
        nom = env.get(nm) if nm else None
        per = "  ".join(f"{_corner_name(a)} {a['r']:.4f}" for a in s["arcs"])
        lines.append(f"      {r.label:20s} "
                     f"{(('%.3f' % nom) if nom is not None else '-'):>8s} "
                     f"{s['r']:9.4f} {s['r_spread']:10.2e} "
                     f"{s['arc_resid']:10.2e}   {per}")
        if s["r_spread"] > CORNER_TOL:
            lo = min(s["arcs"], key=lambda a: a["r"])
            hi = max(s["arcs"], key=lambda a: a["r"])
            fails.append(
                f"{r.label}: the four corner radii DISAGREE by "
                f"{s['r_spread']:.4f} -- {_corner_name(hi)} fits "
                f"R{hi['r']:.4f} and {_corner_name(lo)} fits R{lo['r']:.4f}"
                + (f", against {nm} = {nom:.3f}" if nom is not None else "")
                + f". The mean is R{s['r']:.4f}, which is why a check on the "
                f"mean alone passes this. Tolerance {CORNER_TOL:.3f} = 3x the "
                f"DXF's quantisation; the shipped files read 7e-06.")
        for a in s["arcs"]:
            if a["resid"] > CORNER_TOL:
                fails.append(
                    f"{r.label}: the {_corner_name(a)} corner's {a['n']} "
                    f"vertices do not lie on one circle -- best fit R"
                    f"{a['r']:.4f} centred {a['cx']:+.3f},{a['cy']:+.3f} with "
                    f"a {a['resid']:.4f} residual, against the "
                    f"{CORNER_TOL:.3f} budget. A chamfer, an ellipse or a "
                    f"radius that changes part-way round the arc all read like "
                    f"this and all pass a mean-radius check.")
        if nom is not None and abs(s["r"] - nom) > TOL_DXF:
            fails.append(f"{r.label}: mean corner radius R{s['r']:.4f}, "
                         f"want {nm} = {nom:.3f}")


def diagnose_rrect(ring, nom_r, what):
    """Say WHICH corner is wrong on a contour that _rrect() has refused.

    _rrect()'s per-vertex boundary test assumes one radius, so a contour with
    one corner drawn at the wrong radius is classified "other" and surfaces as
    "no rounded-rectangular keyboard void in the DXF" plus an unaccounted
    contour. True, loud, and silent about the cause. This turns it into the
    corner's name and its measured radius.
    """
    arcs = ring.corner_arcs()
    if arcs is None:
        return []
    rs = [a["r"] for a in arcs]
    out = [f"{what}: the contour at {ring.bx:+.3f},{ring.by:+.3f} is "
           f"{ring.w:.4f} x {ring.h:.4f} and its four corners fit "
           + ", ".join(f"{_corner_name(a)} R{a['r']:.4f}" for a in arcs)
           + f" (spread {max(rs)-min(rs):.4f}"
           + (f", want R{nom_r:.3f}" if nom_r is not None else "") + ")."]
    if nom_r is not None:
        bad = [a for a in arcs if abs(a["r"] - nom_r) > CORNER_TOL]
        if bad:
            out.append(f"{what}: " + "; ".join(
                f"the {_corner_name(a)} corner is R{a['r']:.4f} and should be "
                f"R{nom_r:.3f}, out by {a['r']-nom_r:+.4f}" for a in bad))
    return out


# ===========================================================================
#  CHECK 3 -- THE MINIMUM WEB INVENTORY
# ===========================================================================
def check_webs(part, rings, t, fails, lines):
    """fabrication.md 0.8, 1.5 and 4.1 item 11.

    Boundary-to-boundary distance between every distinct pair of stitched
    rings, which covers every hole-to-outline land as a special case. The
    >= 1 x t guidance is BROKEN BY DESIGN here and is reported, not gated;
    what is gated is the hard floor and the recorded baseline, so an accepted
    property cannot silently degrade into a different one.
    """
    pairs = sorted((rings[i].lr.distance(rings[j].lr), i, j)
                   for i, j in itertools.combinations(range(len(rings)), 2))
    under = [p for p in pairs if p[0] < WEB_ADVISORY_T * t]
    worst, wi, wj = pairs[0]
    lines.append(f"    webs -- {len(pairs)} contour pairs, {len(under)} below "
                 f"{WEB_ADVISORY_T:.0f} x t = {t:.3f} (advisory, broken by "
                 f"design: fabrication.md 1.5)")
    lines.append(f"      narrowest {worst:.4f} = {worst/t:.3f} x t  "
                 f"{rings[wi].label} <-> {rings[wj].label}")
    for d, i, j in under:
        lines.append(f"        {d:9.4f}  {d/t:5.3f} x t  "
                     f"{rings[i].label} <-> {rings[j].label}")

    # the front rail is named because fabrication.md 1.5 named it, and because
    # the recessed branch was argued partly on widening it
    out = next((r for r in rings if r.label == "outline"), None)
    void = next((r for r in rings if r.label == "keyboard void"), None)
    if out is not None and void is not None:
        rail = out.lr.distance(void.lr)
        lines.append(f"      front rail (outline <-> keyboard void) "
                     f"{rail:.4f} = {rail/t:.3f} x t")

    base = WEB_BASELINE.get(part)
    if worst < WEB_HARD_FLOOR:
        fails.append(
            f"{part}: narrowest web {worst:.4f} is below the "
            f"{WEB_HARD_FLOOR:.3f} hard floor (10 x the 0.200 mm kerf): "
            f"{rings[wi].label} <-> {rings[wj].label}. "
            f"fabrication.md 4.1 item 11")
    if base is None:
        lines.append(f"      NO WEB BASELINE RECORDED for {part} -- add "
                     f"\"{part}\": {worst:.3f} to WEB_BASELINE")
    elif worst < base - WEB_BASELINE_TOL:
        fails.append(
            f"{part}: narrowest web got WORSE -- {worst:.4f} against the "
            f"recorded baseline {base:.3f} ({rings[wi].label} <-> "
            f"{rings[wj].label}). This part already breaks three SendCutSend "
            f"DFM rules and goes to a fabricator who reads a drawing "
            f"(fabrication.md 1.5); it is accepted at the baseline and not "
            f"below it. If the loss is deliberate, re-export and update "
            f"WEB_BASELINE[\"{part}\"] to {worst:.3f} in this file.")
    elif worst > base + WEB_BASELINE_TOL:
        lines.append(f"      baseline is STALE and in the good direction: "
                     f"{worst:.4f} vs {base:.3f} -- update "
                     f"WEB_BASELINE[\"{part}\"] to {worst:.3f}")
    return worst


# ===========================================================================
#  CHECK 4 -- THE KEYBOARD BRANCH
# ===========================================================================
def check_keyboard_branch(env, void, fails, lines):
    """parameters.scad 8a; fabrication.md 5.1 item 1 and 3 row 12.

    "recessed" and "flush" are different parts, and the one blocking unknown
    in fabrication.md 5.1 was which. The branch is now fixed, so the drawn
    void must be self-consistent with it -- measured from the DXF.
    """
    from shapely.geometry import Point
    mount = env.get("kbd_mount")
    fw, fh = env["kbd_frame"]
    cw, ch = env["kbd_cap_field"]
    s = void.shape
    W, H = s["w"], s["h"]
    lines.append(f"    keyboard -- kbd_mount = \"{mount}\"; void measured "
                 f"{W:.4f} x {H:.4f} at ({void.bx:+.4f},{void.by:+.4f}), "
                 f"corner R{s['r']:.4f} (fitted, 4 arcs agree to "
                 f"{s['r_spread']:.5f})")
    if abs(s["r"] - env["kbd_void_r"]) > TOL_DXF:
        fails.append(f"void corner radius measures {s['r']:.4f}, "
                     f"want kbd_void_r = {env['kbd_void_r']:.3f}")
    if abs(void.by - env["kbd_void_y"]) > TOL_DXF:
        fails.append(f"void centre y {void.by:+.4f}, want kbd_void_y = "
                     f"{env['kbd_void_y']:+.4f}")

    ov_x, ov_y = (fw - W)/2, (fh - H)/2
    #  FOUR CLEARANCES, NOT TWO, AND THE CAP FIELD IS NOT CONCENTRIC.
    #  This read (W - cw)/2 and (H - ch)/2, which is the clearance you get if
    #  the keycap field sits on the void's centre. It does not:
    #  kbd_cap_field_d is the field's centre in DECK axes relative to the
    #  frame's, and the void is concentric with the frame, so the gap is
    #  (W - cw)/2 -/+ d[0] across x and (H - ch)/2 +/- d[1] through the depth.
    #  Concentric arithmetic gave 0.7125 and 0.8425 per side and hid both the
    #  tightest gap (+x 0.650) and the loosest (rear 1.0925). Measured here off
    #  the DXF's own void and gated against parameters.scad's kbd_cap_clear,
    #  which is the same four numbers computed from the datums -- so the void
    #  as DRAWN and the clearance as DECLARED cannot drift apart.
    dxc, dyc = env["kbd_cap_field_d"]
    cl = ((W - cw)/2 - dxc, (W - cw)/2 + dxc,
          (H - ch)/2 + dyc, (H - ch)/2 - dyc)
    cl_x, cl_y = min(cl[0], cl[1]), min(cl[2], cl[3])
    lines.append(f"      frame {fw:.3f} x {fh:.3f}   "
                 f"keycap field {cw:.3f} x {ch:.3f}, centre "
                 f"{dxc:+.4f},{dyc:+.4f} off the frame's (kbd_cap_field_d)")
    lines.append(f"      slab-over-frame overlap   X {ov_x:+.4f}  "
                 f"Y {ov_y:+.4f}   per side")
    lines.append(f"      void-over-keycap clearance, measured per SIDE and not "
                 f"per axis: +x {cl[0]:+.4f}  -x {cl[1]:+.4f}  "
                 f"front {cl[2]:+.4f}  rear {cl[3]:+.4f}")
    decl = env.get("kbd_cap_clear")
    if decl and len(decl) == 4:
        for nm, got, nom in zip(("+x", "-x", "front", "rear"), cl, decl):
            if abs(got - nom) > TOL_DXF:
                fails.append(
                    f"void-over-keycap clearance at {nm} measures {got:.4f} "
                    f"off the DXF and parameters.scad's kbd_cap_clear declares "
                    f"{nom:.4f}. The void as drawn and the clearance as "
                    f"declared have drifted apart by {got-nom:+.4f}.")
    else:
        lines.append("        kbd_cap_clear did not parse as four numbers, so "
                     "the declared clearance is unchecked")

    if mount == "recessed":
        for nm, v, pair in (("X", ov_x, (W, fw)), ("Y", ov_y, (H, fh))):
            if v <= 0:
                fails.append(
                    f"recessed branch: the void is {pair[0]:.3f} in {nm} "
                    f"against a {pair[1]:.3f} frame, so it is NOT smaller and "
                    f"the slab does not overlap the frame's rim (overlap "
                    f"{v:+.4f}). parameters.scad 8a says "
                    f"kbd_mount = \"recessed\"; that branch requires the void "
                    f"SMALLER than the frame on both axes.")
        for nm, v, pair in (("X", cl_x, (W, cw)), ("Y", cl_y, (H, ch))):
            if v <= 0:
                fails.append(
                    f"recessed branch: the void is {pair[0]:.3f} in {nm} "
                    f"against a {pair[1]:.3f} keycap field, so the caps do "
                    f"not come through (clearance {v:+.4f}).")
    elif mount == "flush":
        for nm, v, pair in (("X", ov_x, (W, fw)), ("Y", ov_y, (H, fh))):
            if v >= 0:
                fails.append(
                    f"flush branch: the void is {pair[0]:.3f} in {nm} against "
                    f"a {pair[1]:.3f} frame, so the frame cannot pass up into "
                    f"it (overlap {v:+.4f}). The flush branch requires the "
                    f"void BIGGER than the frame on both axes; the DXF has "
                    f"the recessed void drawn. parameters.scad 8a")
    else:
        fails.append(f"kbd_mount = {mount!r} -- parameters.scad 8a allows "
                     f"\"recessed\" or \"flush\" and the void's sign depends "
                     f"on which")

    # -- the corner, which is the tight one -------------------------------
    #  Measured against the ARTEFACT's own corner arc, so the faceting is in
    #  the answer: the inscribed 96-gon sits inside the nominal R3.175.
    #
    #  WHAT THIS FIGURE IS AND WHAT IT IS NOT, because the file used to claim
    #  more than it could. It is the clearance for a cap with a PERFECTLY
    #  SQUARE plan corner, and it is reported, not gated, because that cap does
    #  not exist. It used to say parameters.scad "asks this file to report the
    #  square-cap figure so the assumption cannot hide", and also that "the
    #  keycap's own corner radius is NOT SOURCED". parameters.scad says neither
    #  any more, and the reason is a finding rather than an omission: the 1u
    #  cap's B-rep has 77 faces of which six are cylindrical and NONE has a
    #  vertical axis, so the plan corner is a swept blend on a tapered shell.
    #  There is no feature radius to source, so there is no number waiting to
    #  be looked up and no assumption hiding -- and _min_cap_radius below
    #  answers the only well-posed version of the question, which is how much
    #  corner the void could tolerate.
    #
    #  THE CORNER IS ALSO NOT CONCENTRIC, for the same reason the sides are
    #  not: the cap field's corner sits at the void's centre displaced by
    #  kbd_cap_field_d. Taken concentrically this read -0.2185; the governing
    #  corner is the front +x one and it reads -0.4376.
    best = None
    for sx in (1, -1):
        for sy in (1, -1):
            p = Point(void.bx + dxc + sx*cw/2, void.by + dyc + sy*ch/2)
            d = p.distance(void.lr)
            sd = d if void.poly.contains(p) else -d
            if best is None or sd < best[0]:
                best = (sd, sx, sy)
    rc = _min_cap_radius(void, cw, ch, dxc, dyc)
    lines.append(f"      CORNER clearance -- a perfectly SQUARE keycap whose "
                 f"outer corner sits on the keycap field's corner, the field "
                 f"displaced by kbd_cap_field_d: {best[0]:+.4f} (corner "
                 f"{best[1]:+d},{best[2]:+d}), against the tightest straight "
                 f"edge's {min(cl):+.4f}")
    if rc is None:
        lines.append("        NO cap corner radius clears this void")
    else:
        lines.append(f"        a square cap is a fiction: the 1u cap's B-rep "
                     f"has 77 faces, six cylindrical, NONE with a vertical "
                     f"axis, so its plan corner is a swept blend on a tapered "
                     f"shell and there is no feature radius to source. The "
                     f"well-posed figure is the other way round -- the "
                     f"smallest cap corner radius that WOULD clear this void "
                     f"is {rc:.3f}. Reported, not gated.")
    #  parameters.scad 8a's old +0.139 and its r <= 3.512 window both rested on
    #  a Y gap of 1.1925 taken on the LOOSE side of the depth offset. The
    #  arithmetic this file printed for it -- "1.1925 = 0.8425 + 0.250" -- does
    #  not close: those sum to 1.0925. 1.0925 is the REAR gap and it is
    #  kbd_cap_clear[3], which the datums now carry directly, so the figure is
    #  read off the datum instead of being rebuilt from a concentric gap plus a
    #  remembered 0.250. The tight corner is the FRONT one, kbd_cap_clear[2] =
    #  0.5925, and that is where the sign comes from.
    if abs(best[0] - 0.139) > 0.010:
        wl = _rwindow(max(cl[0], cl[1]), max(cl[2], cl[3]))
        wt = _rwindow(min(cl[0], cl[1]), min(cl[2], cl[3]))
        lines.append(
            f"        NOTE: parameters.scad 8a quotes +0.139 for this and "
            f"solves the radius window as r <= 3.512. Both are the LOOSE "
            f"corner: kbd_cap_clear's own rear {cl[3]:.4f} with the wide side "
            f"{max(cl[0], cl[1]):.4f}, which closes at "
            + (f"r <= {wl:.3f}" if wl else "no window at all") + ". "
            f"The corner that governs is the front one, {cl[2]:.4f} against "
            f"{min(cl[0], cl[1]):.4f}, and kbd_void_r = "
            f"{env['kbd_void_r']:.3f} is outside its window of "
            + (f"r <= {wt:.3f}" if wt else
               "-- there is none: a side clearance is negative, so no corner "
               "radius clears these caps and the branch gate above says so")
            + ". check_real_cap_clearance below settles whether that matters, "
            f"against the caps themselves rather than a rectangle.")

    # -- the Z chain -------------------------------------------------------
    #  There is no artefact to measure this against: no export in this repo
    #  carries the keyboard's Z. So it is arithmetic on the parsed datums, and
    #  it is labelled as such rather than dressed up as a measurement.
    z0, at = env["acrylic_z0"], env["acrylic_t"]
    gap, caph_ = env["kbd_plate_gap"], env["kbd_cap_h"]
    plate = env["kbd_plate_top_z"]
    captop = env["kbd_cap_top_z"]
    deck = z0 + at
    recess_chain = deck - captop
    recess_formula = at + gap - caph_
    lines.append(f"      Z chain (parsed datums, no artefact carries it): "
                 f"plate top {plate:.3f}, cap top {captop:.3f}, deck top "
                 f"{deck:.3f}, recess {recess_chain:+.3f}")
    if abs(plate - (z0 - gap)) > 1e-6:
        fails.append(f"kbd_plate_top_z {plate:.4f} != acrylic_z0 - "
                     f"kbd_plate_gap = {z0-gap:.4f}")
    if abs(captop - (plate + caph_)) > 1e-6:
        fails.append(f"kbd_cap_top_z {captop:.4f} != kbd_plate_top_z + "
                     f"kbd_cap_h = {plate+caph_:.4f}")
    if mount == "recessed" and captop >= deck:
        fails.append(
            f"recessed branch: kbd_plate_top_z + kbd_cap_h = {captop:.4f} is "
            f"NOT below acrylic_z0 + acrylic_t = {deck:.4f} -- the keycaps "
            f"stand out of the deck's top face by {captop-deck:.4f}")
    if abs(env["kbd_recess"] - recess_chain) > 1e-6:
        fails.append(f"kbd_recess {env['kbd_recess']:.4f} disagrees with the "
                     f"Z chain's {recess_chain:.4f}")
    if abs(recess_chain - recess_formula) > 1e-6:
        fails.append(f"recess {recess_chain:.4f} != acrylic_t + "
                     f"kbd_plate_gap - kbd_cap_h = {recess_formula:.4f}")
    #  THE SIGN OF kbd_plate_gap IN THE DOCUMENTED FORMULA IS WRONG, and it is
    #  reported rather than enforced. parameters.scad 8a's prose and the task
    #  that produced this check both write the recess as
    #  `acrylic_t - kbd_cap_h - kbd_plate_gap`. The plate top is BELOW the
    #  acrylic's underside, so the gap ADDS to the recess:
    #      recess = (acrylic_z0 + acrylic_t) - (acrylic_z0 - gap + cap_h)
    #             =  acrylic_t + gap - cap_h
    #  parameters.scad's own derived kbd_recess agrees with the chain, so only
    #  the prose is wrong -- by 2 x gap = 1.270.
    wrong = at - caph_ - gap
    lines.append(f"        NOTE: parameters.scad 8a's prose writes the recess "
                 f"as acrylic_t - kbd_cap_h - kbd_plate_gap = {wrong:.3f}. "
                 f"The Z chain gives {recess_chain:.3f}; the gap puts the "
                 f"plate BELOW the acrylic, so it adds. The file's own "
                 f"kbd_recess = {env['kbd_recess']:.3f} is right; the prose "
                 f"is out by 2 x kbd_plate_gap = {2*gap:.3f}.")


def _rwindow(a, b):
    """The largest void corner radius a square cap with side gaps (a, b) clears.

    r <= (a + b) + sqrt(2ab), and it only means anything while both gaps are
    positive -- with a negative gap the cap is outside the void on that side
    and no radius helps. The old code called math.sqrt on the product
    unguarded, so a void shrunk 0.400 per side crashed this check with a
    ValueError instead of diagnosing it. Returns None where there is no window.
    """
    if a <= 0 or b <= 0:
        return None
    return (a + b) + math.sqrt(2 * a * b)


def _min_cap_radius(void, cw, ch, dx=0.0, dy=0.0):
    """The smallest keycap corner radius that clears the measured void.

    Bisected against the artefact's own ring: for a cap corner radius rc the
    critical feature is the cap's corner arc, whose centre sits rc inboard of
    the field corner, and it clears when that centre is at least rc from the
    void's boundary. (dx, dy) is kbd_cap_field_d -- the field is not concentric
    with the void and taking it so moves this answer.
    """
    from shapely.geometry import Point

    def clear(rc):
        worst = None
        for sx in (1, -1):
            for sy in (1, -1):
                p = Point(void.bx + dx + sx*(cw/2 - rc),
                          void.by + dy + sy*(ch/2 - rc))
                d = p.distance(void.lr)
                sd = (d if void.poly.contains(p) else -d) - rc
                worst = sd if worst is None else min(worst, sd)
        return worst

    if clear(0.0) >= 0:
        return 0.0
    lo, hi = 0.0, min(cw, ch) / 4.0
    if clear(hi) < 0:
        return None
    for _ in range(60):
        mid = (lo + hi) / 2
        if clear(mid) >= 0:
            hi = mid
        else:
            lo = mid
    return hi


# ===========================================================================
#  the parts
# ===========================================================================
def label_slab(rings, env, fails):
    """Name every contour by its role, and refuse to leave one unaccounted.

    The old check asserted `len(loops) == 15` with the breakdown in a comment.
    A count cannot tell you WHICH fifteen, so this builds the inventory from
    the shapes and then requires it to be complete: every ring claimed by
    exactly one role, nothing left over.
    """
    rings = sorted(rings, key=lambda r: -r.area)
    rings[0].label = "outline"
    taken = {id(rings[0])}
    bores, void, fader = [], None, None
    for r in rings[1:]:
        if r.kind == "circle" and abs(r.shape["d_vertex"] - env["bore_d"]) < 0.3:
            r.label = f"bore ({r.cx:+.3f},{r.cy:+.3f})"
            bores.append(r); taken.add(id(r))
        elif (r.kind == "slot"
              and abs(r.shape["width"] - env["slider_slot_w"]) < 0.3
              and abs(r.shape["l_vertex"] - env["slider_slot_l"]) < 0.3):
            r.label = "fader slot"
            fader = r; taken.add(id(r))
        elif r.kind == "rrect" and void is None:
            r.label = "keyboard void"
            void = r; taken.add(id(r))
    if len(bores) != 6:
        fails.append(f"{len(bores)} knob bores in the DXF, want 6")
    if void is None:
        fails.append("no rounded-rectangular keyboard void in the DXF")
        #  ...and WHY. The void is still the largest contour after the outline
        #  whatever is wrong with it, so the candidate is never in doubt; what
        #  is in doubt is which corner, and a reader who is only told the void
        #  is absent has to go and fit the arcs by hand. This is the indirect
        #  catch check_corner_radii's docstring keeps, with its diagnosis
        #  attached.
        cand = next((r for r in rings[1:]
                     if id(r) not in taken and r.kind not in ("circle",
                                                              "slot")), None)
        if cand is not None:
            fails.extend(diagnose_rrect(cand, env.get("kbd_void_r"),
                                        "keyboard void"))
    if fader is None:
        fails.append(f"no fader slot {env['slider_slot_l']:.3f} x "
                     f"{env['slider_slot_w']:.3f} in the DXF")
    else:
        f = fader.shape
        if abs(f["l_vertex"] - env["slider_slot_l"]) > TOL_DXF:
            fails.append(f"fader slot length {f['l_vertex']:.4f}, want "
                         f"{env['slider_slot_l']:.3f}")
    return rings, void, taken


def check_part(part, env, export, fails, lines):
    dxf = os.path.join(export, "dxf", f"{part}.dxf")
    if not os.path.exists(dxf):
        if part == "slab":
            fails.append(f"missing {dxf} -- run the export first")
        return
    try:
        rings = [Ring(l) for l in dxf_part_loops(dxf)]
    except RuntimeError as e:
        fails.append(f"{part}: {e}")
        return
    if not rings:
        fails.append(f"{dxf}: no closed contour recovered")
        return
    nominals = [env["bore_d"], env["slider_slot_w"], env["corner_r"]*2]
    void = None
    if part == "slab":
        clear_d, slot_l = env["screw_m3_clear_slab"], env["screw_slot_l_slab"]
        pattern = slab_screw_pts(env)
        rings, void, taken = label_slab(rings, env, fails)
        nominals.append(env["kbd_void_r"] * 2)
    else:
        #  NOT screw_m3_clear_back. The back panel's screw is M2.5 -- it
        #  threads into carrier.scad's M2.5 inserts -- and this line read the
        #  M3 datum while backpanel.scad drilled the M3 hole.
        #  parameters.scad 6b.
        clear_d, slot_l = env["screw_m25_clear_d"], env["screw_slot_l_back"]
        pattern = back_screw_pts(env)
        rings = sorted(rings, key=lambda r: -r.area)
        rings[0].label = "outline"
        taken = {id(rings[0])}
        label_backpanel(rings, env, taken, fails, lines)
    nominals.append(clear_d)

    lines.append(f"  {part}.dxf: {len(rings)} cut contours, "
                 f"{rings[0].w:.3f} x {rings[0].h:.3f} mm")
    if abs(rings[0].w - env["body_w"]) > TOL:
        fails.append(f"{part} outline width {rings[0].w:.3f}, "
                     f"want {env['body_w']:.3f}")
    if abs(rings[0].h - env["body_h"]) > TOL:
        fails.append(f"{part} outline height {rings[0].h:.3f}, "
                     f"want {env['body_h']:.3f}")

    #  ONLY THE RINGS NOTHING ELSE HAS CLAIMED, and the line that set clear_d
    #  is what forces it. The back panel's fasteners are now O2.900, which is
    #  also the cowl's four screw holes, so "fastener-sized" no longer
    #  distinguishes them and this call used to scan every ring: unfiltered it
    #  reports 5 round datums where there is 1, picks a cowl hole as the datum,
    #  and then fails all nine slot bearings off it -- 15 FAILs for a panel
    #  that is right. It cannot hide a missing fastener either, because a ring
    #  reaches `taken` only by being matched to some other declared feature by
    #  POSITION. No change for the slab, whose claimed rings are the outline,
    #  the keyboard void, the six bores and the fader slot.
    fast = check_datum_and_slots([r for r in rings if id(r) not in taken],
                                 pattern, int(env["screw_datum_i"]),
                                 clear_d, slot_l, fails, lines) or []
    for r in fast:
        taken.add(id(r))
    loose = [r for r in rings if id(r) not in taken]
    if loose:
        for r in loose:
            fails.append(f"{part}: cut contour at {r.cx:+.3f},{r.cy:+.3f} "
                         f"({r.kind}, {r.w:.3f} x {r.h:.3f}) is in the DXF and "
                         f"matches no declared feature")
    #  THE INVENTORY COUNTS WHAT IS THERE, NOT WHAT WAS ORDERED. Writing
    #  "1 void ... 1 fader slot" as literals made this line report a part it
    #  had not measured: with fader_slot() commented out the DXF loses a
    #  contour, nothing is left over, and the line still read
    #  "+ 1 fader slot = 14 contours, complete" beside a FAIL saying the fader
    #  slot is absent. A vanished feature leaves no unaccounted contour, so
    #  `loose` alone cannot see it -- and a spec that silently stops rendering
    #  while the artefact still looks right is this repo's own named failure
    #  (fabrication.md 4.1's preamble). Both halves are counted now.
    nb = sum(1 for r in rings if r.label.startswith("bore"))
    nv = sum(1 for r in rings if r.label == "keyboard void")
    nf = sum(1 for r in rings if r.label == "fader slot")
    inv = f"1 outline + {len(fast)} fasteners"
    short = []
    if part == "slab":
        inv += f" + {nv} void + {nb} bores + {nf} fader slot"
        for got, want, what in ((nv, 1, "void"), (nb, 6, "bores"),
                                (nf, 1, "fader slot")):
            if got != want:
                short.append(f"{what} {got}/{want}")
    verdict = []
    if loose:
        verdict.append(f"{len(loose)} UNACCOUNTED")
    if short:
        verdict.append("MISSING " + ", ".join(short))
    lines.append(f"    inventory: {inv} = {len(rings)} contours, "
                 + ("complete" if not verdict else "; ".join(verdict)))

    check_faceting(rings, nominals, fails, lines)
    check_corner_radii(rings, env, fails, lines)
    t = env["acrylic_t"] if part == "slab" else env["back_t"]
    check_webs(part, rings, t, fails, lines)
    check_edge_distance(rings, fast, env, fails, lines)
    if part == "slab" and void is not None:
        check_keyboard_branch(env, void, fails, lines)
        check_real_cap_clearance(env, void, fails, lines)
    if part == "backpanel":
        check_insert_clash(env, fast, fails, lines)


def slab_screw_pts(env):
    """A transcription of parameters.scad's `slab_screw_pts()`. See
    check_datum_and_slots' docstring for why this exists at all."""
    return [(sx * env["screw_x"], env["band_mid"] + dy)
            for sx in (1, -1)
            for dy in (env["screw_dy"], 0.0, -env["screw_dy"])]


def back_screw_pts(env):
    """A transcription of parameters.scad's `back_screw_pts()`."""
    return slab_screw_pts(env) + [tuple(p) for p in env["back_screw_kbd"]]


# --- the back panel's three non-fastener features --------------------------
# Transcriptions of parameters.scad section 12, for the same reason
# slab_screw_pts() is one: these are OpenSCAD *functions*, and datums() parses
# scalar assignments. Transcribing is the lesser evil, and the check that
# follows measures the ARTEFACT against the transcription -- so a drift between
# the two shows up as an unaccounted contour, which is the loudest failure this
# file has.

def disp_pcb_sz():
    return (92.500, 69.100)             # [MEASURED] board-models.md 2.2


def cowl_window(env):
    return (87.000 + 2 * env["pocket_fit"], 21.103 + 2 * env["pocket_fit"])


def disp_x(env):
    """A transcription of parameters.scad's `disp_x`.

    It is transcribed rather than parsed because disp_board is now a vector
    built from a FUNCTION call -- disp_pcb_sz() -- and datums() only evaluates
    scalar assignments. Parsing stops at the function; the transcription does
    not. If the two ever disagree the window lands in the wrong place and the
    unaccounted-contour check says so.
    """
    return (env["grid_x"] - env["grid_w"] / 2 - env["gap"]
            - (disp_pcb_sz()[0] + 2 * env["pocket_fit"]) / 2)


def cowl_xy(env):
    return (disp_x(env) + 46.248 - disp_pcb_sz()[0] / 2,
            env["band_mid"] + 15.152 - disp_pcb_sz()[1] / 2)


def cowl_screw_pts(env):
    #  A 2 x 2 ON THE FLANGE'S CORNERS, and it USED TO BE FOUR EDGE MIDPOINTS
    #  at cowl_xy() +/- (window/2 + flange/2). Three of those four had no
    #  material to live in; parameters.scad 12 carries the four measurements.
    #  dx and dy are a TRANSCRIPTION of cowl_screw_dx()/cowl_screw_dy(), for
    #  the same reason the flange was: datums() cannot evaluate a function.
    cx, cy = cowl_xy(env)
    dx, dy = 47.625, 15.875             # cowl_screw_dx(), cowl_screw_dy()
    return [(cx + sx * dx, cy + sy * dy)
            for sx in (-1, 1) for sy in (-1, 1)]


def disp_head_pts(env):
    return [(disp_x(env) + sx * 85.500 / 2, env["band_mid"] + sy * 62.100 / 2)
            for sx in (1, -1) for sy in (1, -1)]


def label_backpanel(rings, env, taken, fails, lines):
    """Claim the window, the display's screw heads and the cowl's screws.

    TWO OF THE FOUR HEAD HOLES ARE EXPECTED TO BE MISSING, and that is a vendor
    collision rather than a drawing error: the 18650 holder's edge (V 4.600)
    runs straight through the display's two lower mounting holes (V 3.500), so
    those two O5.500 circles union into the window and the window's bbox grows
    by about 2.7 x 3.2. The check therefore demands the window be AT LEAST
    nominal and allows it to be larger, and accepts 2..4 head circles -- but it
    still refuses a head hole that is neither present nor inside the window,
    which is the case that would mean the board had moved.
    """
    free = [r for r in rings if id(r) not in taken]
    wx, wy = cowl_xy(env)
    ww, wh = cowl_window(env)

    win = None
    for r in free:
        if abs(r.cx - wx) < 3.0 and abs(r.cy - wy) < 3.0 \
                and r.w >= ww - TOL and r.h >= wh - TOL \
                and r.w < ww + 8.0 and r.h < wh + 8.0:
            win = r
            break
    if win is None:
        fails.append("backpanel: the 18650 window is not in the DXF at "
                     f"{wx:+.3f},{wy:+.3f} ({ww:.3f} x {wh:.3f} or larger)")
    else:
        win.label = "18650 window"
        taken.add(id(win))

    found_heads = 0
    for (hx, hy) in disp_head_pts(env):
        hit = next((r for r in free if id(r) not in taken
                    and abs(r.cx - hx) < TOL and abs(r.cy - hy) < TOL
                    and abs(r.w - 5.500) < 0.05), None)
        if hit is not None:
            hit.label = "display head"
            taken.add(id(hit))
            found_heads += 1
        elif win is None or not (abs(hx - wx) <= win.w / 2 + 2.75
                                 and abs(hy - wy) <= win.h / 2 + 2.75):
            fails.append(f"backpanel: display screw-head clearance at "
                         f"{hx:+.3f},{hy:+.3f} is neither cut nor inside the "
                         f"18650 window")

    found_screws = 0
    for (sx_, sy_) in cowl_screw_pts(env):
        hit = next((r for r in free if id(r) not in taken
                    and abs(r.cx - sx_) < TOL and abs(r.cy - sy_) < TOL
                    and abs(r.w - env["screw_m25_clear_d"]) < 0.05), None)
        if hit is None:
            fails.append(f"backpanel: cowl screw at {sx_:+.3f},{sy_:+.3f} "
                         f"(O{env['screw_m25_clear_d']:.3f}) is not in the DXF")
        else:
            hit.label = "cowl screw"
            taken.add(id(hit))
            found_screws += 1

    lines.append(f"    18650: window {'present' if win else 'MISSING'}, "
                 f"{found_heads}/4 display heads cut "
                 f"({4 - found_heads} merged into the window, expected 2), "
                 f"{found_screws}/{len(cowl_screw_pts(env))} cowl screws")


def check_edge_distance(rings, fast, env, fails, lines):
    """Roehm's rule, measured from the artefact, and the land REPORTED.

    fabrication.md 1.4 settles which rule and how to apply it: a hole's CENTRE
    stands off a free edge by 1.5 x the hole diameter `[ROEHM acrylite-kb]`,
    and the conservative reading this file used to gate on -- the LAND, wall to
    edge -- "fails at O4.445: land 5.298 against 6.668. Keep reporting the land
    as a margin; do not gate on it."

    WHAT IS GATED AND WHAT IS ONLY REPORTED, because the two differ and the
    difference is a finding:

      GATED   the fastener holes, against `acrylic_edge_min`, which is the
              datum parameters.scad 6 declares for exactly this and which
              fabrication.md 1.4's "rule of record, applied" block gates.
              Measured centre-to-OUTLINE-CONTOUR, not body_w/2 - screw_x.

      REPORTED, NOT GATED
              the land, per fabrication.md 1.4's instruction; and the O22.000
              bores' own 1.5 x O = 33.000, which they MISS against the sheet
              edge. 1.4's applied block checks the bore against the 38.100
              knob PITCH and never against the edge: the upper bore row stands
              27.175 from the rear outline. Moving it means moving the control
              band, which is parameters.scad's to decide, not this file's.
    """
    from shapely.geometry import Point
    out = next((r for r in rings if r.label == "outline"), None)
    if out is None:
        return
    emin = env["acrylic_edge_min"]
    holes = fast + [x for x in rings if x.label.startswith("bore")]
    if not holes:
        return
    lines.append("    edge distance -- Roehm: a hole's CENTRE stands off any "
                 "edge or cut by 1.5 x its own diameter")
    lines.append(f"      {'hole':26s} {'O':>7s} {'need':>7s} "
                 f"{'c->edge':>8s} {'c->any':>8s} {'land':>7s} {'margin':>8s}")
    short = []
    for r in sorted(holes, key=lambda x: x.label):
        d = (r.shape["width"] if r.kind == "slot" else r.shape["d_vertex"])
        need = 1.5 * d
        c_out = Point(r.cx, r.cy).distance(out.lr)
        c_any = min(Point(r.cx, r.cy).distance(o.lr)
                    for o in rings if o is not r)
        land = min(r.lr.distance(o.lr) for o in rings if o is not r)
        lines.append(f"      {r.label:26s} {d:7.3f} {need:7.3f} {c_out:8.3f} "
                     f"{c_any:8.3f} {land:7.3f} {c_any-need:+8.3f}")
        if c_any < need - TOL_DXF:
            short.append((r, d, need, c_any))
    for r in fast:
        c_out = Point(r.cx, r.cy).distance(out.lr)
        if c_out < emin - TOL_DXF:
            fails.append(
                f"{r.label}: centre stands {c_out:.3f} from the outline; "
                f"acrylic_edge_min (1.5 x O{env['screw_m3_clear_slab']:.3f}) "
                f"is {emin:.3f}. fabrication.md 1.4")
    ungated = [s for s in short if s[0] not in fast]
    if ungated:
        w = min(ungated, key=lambda s: s[3] - s[2])
        #  WHAT EACH ROW IS ACTUALLY NEAREST TO, measured, because the two rows
        #  are not short against the same thing and the message used to say
        #  "the sheet edge or the keyboard void" for both. Named off the ring
        #  the distance was taken from rather than asserted.
        by_row = {}
        for (r, d, need, got) in ungated:
            #  EVERY ring at the minimum, not whichever min() reached first.
            #  The knob pitch is 38.100 in BOTH axes, so an upper-row bore is
            #  exactly 27.100 from its side neighbour AND from the bore below
            #  it, and naming one of them would be the same sort artefact this
            #  block exists to point out.
            ds = sorted(((_pt_dist(r.cx, r.cy, o), o) for o in rings
                         if o is not r), key=lambda t: t[0])
            near = [o.label for dd, o in ds if dd <= ds[0][0] + TOL_DXF]
            by_row.setdefault(round(got, 3), []).append((r, near, need, got))
        lines.append(
            f"      ROEHM SHORTFALL on {len(ungated)} hole(s), reported not "
            f"gated. Worst: {w[0].label} stands {w[3]:.3f} from the nearest "
            f"cut against 1.5 x O{w[1]:.3f} = {w[2]:.3f}, short by "
            f"{w[2]-w[3]:.3f}. fabrication.md 1.4's applied block checks the "
            f"O{env['bore_d']:.3f} bore against the {env['pitch']:.3f} knob "
            f"PITCH only, and never against what each row is really nearest "
            f"to:")
        for got in sorted(by_row, reverse=True):
            grp = by_row[got]
            tied = sorted({n for g in grp for n in g[1]})
            what = ("an adjacent bore's wall"
                    if all(n.startswith("bore") for n in tied)
                    else "/".join(tied) if len(tied) < 4
                    else f"{len(tied)} contours, {tied[0]} among them")
            #  `out`, not rings[0]. They are the same ring today because both
            #  label paths sort by area and name the largest "outline", but
            #  this line is about the SHEET EDGE and the only thing that says
            #  which ring that is, is the label.
            sheet = min(_pt_dist(g[0].cx, g[0].cy, out) for g in grp)
            lines.append(
                f"        {len(grp)} hole(s) at {got:.3f} -- NOT the sheet "
                f"edge, which is {sheet:.3f} away -- against {what}. All "
                f"{len(grp)} read the same distance, so the \"worst\" above is "
                f"whichever the sort reached first: "
                + ", ".join(sorted(g[0].label for g in grp)))
        #  parameters.scad section 5 has now MADE this call in writing: the
        #  measurement, the primary-source quote, the 88.8075-against-104.100
        #  corridor, five costed moves that all refuse, and an acceptance
        #  resting on the load path. The rule's own BASIS is recorded there as
        #  NOT SOURCED, because the source states it and gives no rationale.
        #  So this is no longer an open question handed back to that file.
        lines.append(
            f"        parameters.scad section 5 has made this call in writing "
            f"and ACCEPTED it, with five costed alternatives that all refuse "
            f"and an acceptance resting on the load path; the rule's own basis "
            f"is recorded there as NOT SOURCED. Reported here so the "
            f"acceptance stays visible in the artefact's own numbers.")


def check_insert_clash(env, fast, fails, lines):
    """Two heat-set inserts pressed into one printed part from opposite faces.

    The old check forbade a back screw from sharing a y with a front screw.
    parameters.scad 9 now puts six of them on the SAME x AND y line on purpose
    -- "the slab's inserts occupy 2.825..9.525 and the back panel's
    -6.350..-1.350, with 4.175 of clear air between" -- so the check moves to
    where the constraint actually lives, which is Z. Gated at the worst case
    (an M3 insert from both faces) so it does not depend on guessing which
    insert the back panel takes.
    """
    front = slab_screw_pts(env)
    coax = [r for r in fast
            if any(math.hypot(r.cx - x, r.cy - y) <= TOL_DXF
                   for x, y in front)]
    need = 2 * env["insert_m3_depth"]
    have = env["carrier_h"]
    lines.append(f"    inserts -- {len(coax)} back hole(s) coaxial with a "
                 f"slab hole; two M3 pockets nose to nose need {need:.3f} in "
                 f"{have:.3f} of carrier, air {have-need:+.3f}")
    if coax and need > have:
        fails.append(
            f"{len(coax)} back fastener(s) are coaxial with a slab fastener "
            f"and two M3 inserts nose to nose need {need:.3f} mm through a "
            f"carrier that is only {have:.3f} mm thick -- the pockets meet "
            f"inside the rim. parameters.scad 6, 9")


# ===========================================================================
#  CHECK 4b -- THE KEYCAPS THEMSELVES AGAINST THE VOID AS EXPORTED
#
#  Every other keyboard-void check in this file compares one rectangle with
#  another. The caps are not a rectangle and the void is not a rectangle: it is
#  a 96-gon whose corners sit inside their nominal arc. So this is the direct
#  measurement, with no idealised shape anywhere in the chain -- every vertex
#  of all 61 caps in the vendor assembly, above the cap's widest section,
#  mapped into deck axes by parameters.scad's OWN kbd_dx/kbd_dy, against the
#  DXF's own polygon.
#
#  It is what settled the question the rectangle arithmetic could only make
#  worrying: the square-cap corner reads -0.4376 and the real worst clearance
#  is +0.3691, so there is no clash and kbd_void_r stays at 3.175.
#
#  THE VENDOR MESH IS NOT IN THE REPOSITORY and cannot be: v2/parts/models/ is
#  gitignored because Keychron's licence covers study and compatible
#  accessories, not redistribution. So its absence is reported as a SKIP with
#  the path, and it is not a pass -- every rectangle gate above still runs.
# ===========================================================================
CAP_WIDEST_Y = 5.200        # mm, in the MODEL's own y (height above the
                            #   plate). The cap's widest section: below it the
                            #   skirt flares and those vertices are inside the
                            #   plate, not in the void.  [MEASURED, the same
                            #   section parameters.scad's kbd_cap_field was
                            #   taken above]
CAP_STAGE2_N = 4000         # how many of the worst stage-1 candidates get
                            #   re-measured against the DXF polygon. The
                            #   analytic screen is exact for a true rounded
                            #   box, so it can only misrank by the faceting,
                            #   which is bounded by one sagitta (0.0025 on the
                            #   void's corner); 4000 vertices spans 0.03 mm of
                            #   clearance here, 12x that.  [DERIVED]


def check_real_cap_clearance(env, void, fails, lines):
    import struct
    import numpy as np
    from shapely.geometry import Point
    stl = os.path.join(ROOT, "parts", "models", "k9pro-internals",
                       "k9pro-stripped-assembly.stl")
    if not os.path.exists(stl):
        lines.append(f"      REAL cap clearance SKIPPED, not passed: "
                     f"{os.path.relpath(stl, REPO)} is not present. It is "
                     f"vendor CAD and v2/parts/models/ is gitignored, so a "
                     f"clone does not carry it. Every rectangle gate above "
                     f"still ran.")
        return
    b = open(stl, "rb").read()
    n = struct.unpack("<I", b[80:84])[0]
    rec = np.frombuffer(b[84:84 + 50*n],
                        dtype=np.dtype([("nr", "<3f4"), ("v", "<3,3f4"),
                                        ("a", "<u2")]))
    V = rec["v"].reshape(-1, 3)
    P = V[V[:, 1] > CAP_WIDEST_Y - 1e-4].astype(np.float64)
    if len(P) == 0:
        fails.append(f"no vertex of {os.path.basename(stl)} is above model "
                     f"y = {CAP_WIDEST_Y:.3f}; the mesh is not the stripped "
                     f"assembly this check was written against")
        return
    #  parameters.scad's own map, transcribed because kbd_dx/kbd_dy are
    #  FUNCTIONS and datums() evaluates assignments. kbd_dy's literal 8.000 is
    #  the frame's model-Z front edge, so it is derived here instead of copied.
    mz0 = env["kbd_frame_cz"] - env["kbd_frame"][1]/2
    dx = P[:, 0] - env["kbd_frame_cx"]
    dy = env["kbd_frame_rear_y"] - (P[:, 2] - mz0)
    #  stage 1: the analytic rounded-box SDF at the void's MEASURED datums,
    #  which screens 3.9 M vertices in one vectorised pass
    A, B, R = void.w/2, void.h/2, void.shape["r"]
    u, v = np.abs(dx - void.bx), np.abs(dy - void.by)
    qx, qy = u - (A - R), v - (B - R)
    sdf = (np.hypot(np.maximum(qx, 0), np.maximum(qy, 0))
           + np.minimum(np.maximum(qx, qy), 0) - R)
    order = np.argsort(-sdf)[:CAP_STAGE2_N]
    #  stage 2: the worst of them against the DXF polygon itself, so the
    #  faceting is in the answer rather than idealised away
    worst, wi = None, None
    for i in order:
        p = Point(dx[i], dy[i])
        d = p.distance(void.lr)
        sd = d if void.poly.contains(p) else -d
        if worst is None or sd < worst:
            worst, wi = sd, i
    lines.append(
        f"      REAL cap clearance -- {len(P)} cap vertices above model "
        f"y = {CAP_WIDEST_Y:.3f}, mapped by kbd_dx/kbd_dy, against the DXF's "
        f"own {void.n}-vertex ring: WORST {worst:+.4f} at deck "
        f"({dx[wi]:+.4f},{dy[wi]:+.4f}) = model X {P[wi][0]:.3f} Z "
        f"{P[wi][2]:.3f}. No rectangle anywhere in this one.")
    if worst <= 0:
        fails.append(
            f"THE KEYCAPS FOUL THE SLAB. The worst of {len(P)} measured cap "
            f"vertices is {worst:.4f} INSIDE the void's boundary, at deck "
            f"({dx[wi]:+.4f},{dy[wi]:+.4f}) = model X {P[wi][0]:.3f} "
            f"Z {P[wi][2]:.3f}. This is the caps as the vendor models them "
            f"against the void as exported, with no idealised shape in "
            f"between, so it is not an arithmetic disagreement: the part would "
            f"not assemble.")
    elif worst < 0.100:
        lines.append(
            f"        the margin is under 0.100 and the laser's kerf "
            f"allowance is +0.230 diametral, so this is the number to watch "
            f"if the void or the frame position moves")


# ===========================================================================
#  CHECK 5 -- THE MESHES: ONE BODY, AND THE CARRIER'S PLANE AUDIT
#
#  Nothing measured the carrier at all. It is the keystone part -- every other
#  part bolts to it -- and the one defect class it has already suffered is
#  invisible in a picture and invisible in a bounding box: a pocket that eats
#  the bottom out of a boss and leaves the boss hanging in mid-air. The first
#  render of kbd_socket_cut() did exactly that to all eleven plate bosses and
#  the ONLY thing that caught it was the mesh coming back as twelve separate
#  bodies instead of one (carrier.scad's own comment above kbd_island_d).
#
#  So: body count, and then the plane audit. Every designed feature of a part
#  built by extruding and differencing 2D profiles terminates in a HORIZONTAL
#  PLANE at a known height with a known area -- a pocket floor, a boss top, an
#  insert pocket's end. Extract them from the STL, group by z, and the result
#  is a complete census of the part's Z structure in 25 rows. A feature that
#  vanishes loses its plane. A feature that moves changes its z. A feature that
#  grows changes its area. A feature nobody declared adds a row.
# ===========================================================================
PLANE_Z_TOL = 0.0005        # mm. How close two facets must be in z to be the
                            #   same plane, and how close a measured plane must
                            #   be to its declared height. Binary STL is
                            #   float32, which quantises to 1.1e-06 at
                            #   |z| ~ 9.5; the narrowest real gap between two
                            #   distinct planes in this part is 0.080
                            #   (-1.270 feather seat to -1.350 insert ceiling).
                            #   This is 450x the quantisation and 160x inside
                            #   that gap.  [DERIVED]
PLANE_AREA_TOL = 0.050      # mm2. float32's quantum at |x| = 150.395 is
                            #   8.96e-06 mm, and the largest plane here is the
                            #   floor underside, whose boundary MEASURES
                            #   1444.4 mm, so its area carries 0.0129 mm2.
                            #   This is 3.9x that. For scale: the smallest
                            #   thing it has to see is one M2 insert floor,
                            #   8.0367. The observed variation is in fact
                            #   ZERO -- the export is byte-reproducible, so
                            #   this budget is for a float32 round-trip on
                            #   another machine and nothing else.  [DERIVED]

#  THE CARRIER'S PLANE INVENTORY.
#
#  MEASURED off v2/export/stl/carrier.stl on 2026-11-07, every row of it, with
#  the heights written as the expression over parameters.scad's datums that
#  produces them -- so a datum change moves the expectation and the check
#  follows it, instead of going red and being "fixed" by retyping a number.
#  Where carrier.scad states a height as a LOCAL literal rather than a datum
#  (feather_seat_d, brk_seat_d, the HDMI window, and the `+ 1` overshoot that
#  chan() and the keyboard cuts both use) the literal is named with its line,
#  because there is no datum to read and pretending otherwise would be worse.
#
#  Re-record ONLY from the artefact, and only after a deliberate change:
#      /usr/bin/python3.11 v2/tools/export.py
#      /usr/bin/python3.11 v2/tools/check_cad.py    # prints every difference
CARRIER_PLANES = (
    # z, from the datums                              dir     area   regions
    (lambda e: e["carrier_rim_h"],
     "UP",   5396.1272,  1, "rim top, the face the acrylic sits on"),
    (lambda e: e["pot_web_z"] + e["pot_web_t"],
     "UP",   2165.9942,  3, "the three pot towers' tops"),
    (lambda e: 8.289,
     "DOWN",  105.8206,  1, "HDMI window ceiling [REPO carrier.scad:344-347, "
                            "a literal: the shell's 7.789 + 0.500]"),
    (lambda e: e["kbd_boss_top_z"],
     "UP",    122.2541, 11, "the 11 plate bosses' tops, 7 screwed + 4 props. "
                            "THIS IS THE ROW THAT CATCHES THE FLOATING-BOSS "
                            "BUG -- and it could not catch the one it was "
                            "sitting on. It recorded 138.1314, which is 11 x "
                            "the O4.000 96-gon's 12.5574 EXACTLY: eleven solid "
                            "discs, because kbd_pilot_cut() translated to "
                            "kbd_boss_top_z + 1 and extruded UPWARD and so "
                            "removed nothing at all. An area cannot tell a "
                            "solid disc from a disc whose pilot was never "
                            "drawn; only the ring count can, and this table "
                            "records areas. Now 4 x 12.5574 + 7 x (12.5574 - "
                            "the O1.700 96-gon's 2.2682) = 122.2541, with 7 "
                            "interior rings. See also the +3.871 row, which is "
                            "the other end of the same seven holes"),
    (lambda e: e["kbd_seat_plane_z"] + 1.000,
     "DOWN",   15.2628,  3, "cut-overshoot ceilings: kbd_cell_cut and "
                            "kbd_toggle_cut both extrude seat - z + 1, so "
                            "they undercut the spine and two bosses"),
    (lambda e: e["kbd_seat_plane_z"],
     "UP",   2062.8909,  9, "the keyboard's seat plane"),
    (lambda e: e["pot_web_z"],
     "DOWN", 1391.8028,  3, "the pot pockets' ceilings -- the face the pot's "
                            "shoulder bears on"),
    (lambda e: e["kbd_boss_top_z"] - e["kbd_boss_pilot_depth"],
     "UP",     15.8773,  7, "the 7 screwed bosses' pilot-hole floors. 7 x the "
                            "O1.700 96-gon's 2.2682. ADDED when kbd_pilot_cut() "
                            "was made to cut: this plane did not exist before, "
                            "and the UNDECLARED-plane gate is what reported it"),
    (lambda e: e["kbd_socket_z"],
     "UP",  20227.4603,  2, "socket-field pocket floor"),
    (lambda e: e["carrier_rim_h"] - e["insert_m3_depth"],
     "UP",     75.3444,  6, "the 6 M3 slab-insert pocket floors"),
    (lambda e: e["kbd_cell_z"],
     "UP",   4322.9998,  1, "cell bay floor"),
    (lambda e: 1.000,
     "DOWN",  193.5615,  4, "channel ceilings [REPO carrier.scad:382-387: "
                            "chan() extrudes chan_qt_d + 1, so a channel "
                            "running under a solid tops out 1.000 above the "
                            "wiring plane]"),
    (lambda e: e["kbd_seat_z"] - e["kbd_toggle_below"],
     "UP",    282.7404,  2, "toggle-slider relief floor"),
    (lambda e: 0.0,
     "UP",   7659.2572,  6, "THE WIRING PLANE"),
    (lambda e: 0.0,
     "DOWN",   33.5507,  4, "where the breakout and Feather seats undercut "
                            "the rim and the spine by exactly pocket_fit"),
    (lambda e: -1.136,
     "UP",     95.0176,  1, "HDMI window sill [REPO carrier.scad:344-347, a "
                            "literal: the shell's -0.636 - 0.500]"),
    (lambda e: -1.270,
     "UP",   1139.1707,  3, "Feather seat [REPO carrier.scad:332 "
                            "feather_seat_d, a local in the part file]"),
    (lambda e: -e["carrier_t"] + e["insert_m25_depth"],
     "DOWN",  125.5741, 10, "M2.5 insert ceilings, THE BACK PANEL'S TEN AND "
                            "NOTHING ELSE: 10 x the O4.000 96-gon's 12.5574, "
                            "all whole. It recorded 139.4939 over 13 regions -- "
                            "the ten, plus three FRAGMENTS of the cowl's four "
                            "bores (10.3160 + 3.2022 + 0.4016, the fourth "
                            "absent) that other cavities had eaten. The cowl's "
                            "four are captive nuts now and live on the -4.150 "
                            "row; CARRIER_INSERT_OPEN above has the whole "
                            "history"),
    (lambda e: -e["chan_qt_d"],
     "UP",   1410.9436,  5, "wiring channel floors. Was 1401.0427 over SIX "
                            "regions: the cowl's old +x insert bore crossed the "
                            "analogue trunk at exit_x and cut this floor in "
                            "two, 9.9009 mm2 of it. One hole fewer is one "
                            "region fewer, which is the honest direction for a "
                            "count to move"),
    (lambda e: e["disp_mount_z"],
     "UP",   4427.0834,  1, "display pocket floor, where the four "
                            "SMTSO-M2.5-7ET standoffs land. Was 4407.3789: two "
                            "of the cowl's old O4.000 insert bores punched "
                            "straight THROUGH this 3.175 floor and a third "
                            "clipped its edge, and the 19.7045 mm2 they took is "
                            "back. The region count was 1 either way, which is "
                            "why the area is the gate here"),
    (lambda e: -3.460,
     "UP",   3261.3483, 16, "the 4 breakout seats + 12 island scallops "
                            "[REPO carrier.scad:300 brk_seat_d = 2.960 + "
                            "0.500, a local in the part file]"),
    (lambda e: e["kbd_annulus_z"],
     "UP",   1352.4278,  1, "the annulus the frame's skirt hangs in"),
    (lambda e: -e["insert_m2_depth"],
     "UP",    128.5878, 16, "the 16 M2 breakout-insert pocket floors"),
    (lambda e: -e["carrier_t"] + e["nut_m25_pocket_d"],
     "DOWN",   93.6692,  4, "the 4 cowl captive-nut pocket ceilings. 4 x a "
                            "regular hexagon of across-flats nut_m25_pocket_af "
                            "5.200, which is 23.4173. ADDED with the nut "
                            "family; the UNDECLARED-plane gate reported it. "
                            "ONE REGION FEWER HERE IS A LOST THREAD, so the "
                            "count matters as much as the area"),
    (lambda e: e["pot_z_bottom"],
     "UP",   1513.8607,  3, "the three pot body pockets' floors"),
    (lambda e: -e["insert_m25_depth"],
     "UP",     25.1148,  2, "the 2 Feather M2.5 insert pocket floors"),
    (lambda e: -e["carrier_t"],
     "DOWN",53725.2587,  1, "floor underside, the print's bed face. Was "
                            "53775.4026: the cowl's four hexagonal nut pockets "
                            "open on this face and the four insert bores they "
                            "replaced were smaller in plan, three of them "
                            "merged into cavities that were already here"),
)

#  THE DEFECT THIS DICT WAS CARRYING IS CLOSED, AND IT STAYS A DICT AND GOES
#  EMPTY. Empty means the strict branch of check_carrier_inserts applies to
#  every family: a pocket that goes missing or gets eaten into is a FAIL and
#  not a recorded baseline. An entry here is a floor, never a target, and the
#  only correct end state for one is deletion.
#
#  WHAT IT USED TO RECORD, kept because the reason is the useful part. Three of
#  the four cowl M2.5 insert pockets did not exist in the part. They were drawn
#  -- cowl_insert_cut() looped over cowl_screw_pts() exactly as
#  back_insert_cut() loops over back_screw_pts() -- and other cavities swallowed
#  them. MEASURED on carrier.stl by probing every horizontal facet containing
#  each declared axis, and the four end faces came to 10.3160 + 3.2022 + 0.4016
#  + 0.0000 = 13.9198 mm2 of a nominal 4 x 12.5574:
#
#    (-138.012, +29.5645)   -1.350 ceiling, but ERODED to 10.3160 of 12.5574 by
#                           the display pocket's left wall
#    ( -42.122, +29.5645)   no horizontal facet at all: the analogue trunk at
#                           exit_x (floor -chan_qt_d, ceiling +1.000) crosses
#                           the pocket, so the two merge and the bore opens
#                           straight into the visible wiring plane
#    ( -90.067, +14.568)    no horizontal facet at all: inside the display
#                           pocket (x -136.950..-43.180, y +13.778..+84.148,
#                           floor disp_mount_z = -3.175), where the floor is
#                           3.175 and the pocket wants 5.000, so it punched a
#                           O4.000 through-hole in the display's own seating
#                           floor
#    ( -90.067, +44.561)    no horizontal facet, AND THE DIAGNOSIS ABOVE WAS
#                           WRONG FOR THIS ONE. It is 0.1065 inside
#                           disp_hdr_cut()'s THROUGH slot (y +44.4545..
#                           +52.0745), so there is no floor there at any depth
#                           and no pocket of any depth could have existed.
#                           Ray-cast at x -90.067: solid [-6.350,-3.175] at
#                           y +42.000, NOTHING from +43.000 to +52.000, solid
#                           again at +53.000.
#
#  back_insert_cut()'s comment was the diagnosis one size up -- "an M3 pocket is
#  6.700 deep and the floor is 6.350, so pressing M3 from below would break the
#  visible wiring plane" -- and the same arithmetic applies to M2.5 wherever the
#  floor is not 6.350. THE FIX WAS NOT TO MOVE THE POINTS. A depth map of the
#  underside taken off the exported mesh is bimodal: under this cowl the floor
#  is 0.000, 2.645, 3.175, 4.350, 6.350, 6.980, 9.449, 11.531, 14.031 or 15.875
#  thick and nothing in between, so a 5.000 pocket is in the full 6.350 or it is
#  nowhere, and the two x bands that qualify are further out than the cowl's own
#  flange can reach without landing on back_screw_pts()[4] and [5]. The thread
#  got shallower instead: a 2.200 captive-nut pocket, ceiling -4.150, which is
#  under the display pocket's floor and under every channel's. The cowl's four
#  are now their own family in insert_families(), on their own plane, with their
#  own hexagonal end-face area, and all four are whole.
CARRIER_INSERT_OPEN = {
    # family                               present as a whole pocket
}


def mesh_bodies(m):
    """How many separate connected solids are in this mesh.

    NOT trimesh.body_count: this trimesh has no scipy and body_count raises.
    Union-find over faces that share a vertex INDEX, which is exact on a mesh
    trimesh has already merged the coincident vertices of -- and it is the only
    thing that has ever caught a floating boss in this project.
    """
    F = m.faces
    par = list(range(len(F)))

    def find(x):
        while par[x] != x:
            par[x] = par[par[x]]
            x = par[x]
        return x

    byv = {}
    for i, tri in enumerate(F):
        for v in tri:
            byv.setdefault(int(v), []).append(i)
    for fl in byv.values():
        a = find(fl[0])
        for j in fl[1:]:
            b = find(j)
            if a != b:
                par[b] = a
                a = find(a)
    return len({find(i) for i in range(len(F))})


def _plane_groups(m):
    """Every horizontal planar facet, grouped by (z, up/down)."""
    n = m.face_normals
    z = m.vertices[m.faces[:, 0]][:, 2]
    out = {}
    for i in range(len(m.faces)):
        if abs(n[i, 2]) > 1.0 - 1e-6:
            key = (round(float(z[i]) / PLANE_Z_TOL) * PLANE_Z_TOL,
                   "UP" if n[i, 2] > 0 else "DOWN")
            out.setdefault(key, []).append(i)
    return out


def _components(m, faces):
    """The connected regions of one plane, as lists of face indices."""
    par = {i: i for i in faces}

    def find(x):
        while par[x] != x:
            par[x] = par[par[x]]
            x = par[x]
        return x

    byv = {}
    for i in faces:
        for v in m.faces[i]:
            byv.setdefault(int(v), []).append(i)
    for fl in byv.values():
        a = find(fl[0])
        for j in fl[1:]:
            b = find(j)
            if a != b:
                par[b] = a
                a = find(a)
    groups = {}
    for i in faces:
        groups.setdefault(find(i), []).append(i)
    return list(groups.values())


def _tri_has(m, i, x, y):
    """Is (x, y) inside facet i's triangle, in plan? Barycentric signs."""
    (ax, ay), (bx, by), (cx, cy) = m.vertices[m.faces[i]][:, :2]
    d = (by - cy)*(ax - cx) + (cx - bx)*(ay - cy)
    if abs(d) < 1e-12:
        return False
    u = ((by - cy)*(x - cx) + (cx - bx)*(y - cy)) / d
    v = ((cy - ay)*(x - cx) + (ax - cx)*(y - cy)) / d
    return u >= -1e-9 and v >= -1e-9 and u + v <= 1 + 1e-9


def ngon_area(d, fn):
    """The area a drawn circle of diameter d actually has at $fn facets.

    Not pi*r^2. The vertices sit ON the nominal circle and the material is the
    inscribed polygon, which is check 1's whole subject arriving in the mesh.
    """
    r = d / 2.0
    return 0.5 * fn * r * r * math.sin(math.tau / fn)


def insert_families(env):
    """Every heat-set insert pocket in the carrier: plane, count, and source.

    The counts come from the same transcribed point functions the DXF checks
    use, so a change to parameters.scad's screw pattern moves this too. The
    breakout count is 4 per bay over len(cols) encoder bays plus the fader's,
    which is carrier_3d()'s own loop.
    """
    return (
        ("M3, slab -> rim", env["insert_m3_hole_d"], env["insert_m3_depth"],
         env["carrier_rim_h"] - env["insert_m3_depth"], "UP",
         slab_screw_pts(env), "slab_screw_pts()"),
        ("M2.5, back panel", env["insert_m25_hole_d"],
         env["insert_m25_depth"],
         -env["carrier_t"] + env["insert_m25_depth"], "DOWN",
         back_screw_pts(env), "back_screw_pts()"),
        #  THE COWL IS NO LONGER IN THAT ROW, and it is not an oversight. Its
        #  four points took an insert_m25 pocket whose ceiling is -1.350, and
        #  three of them had no material: under disp_pocket_cut() the floor is
        #  3.175 and a 5.000 pocket punches through it. They are captive HEX
        #  NUTS now -- a 2.200 pocket whose ceiling is -4.150 -- so they are a
        #  family of their own, on their own plane, with their own end-face
        #  area. SIX FACETS AND NOT 96: ngon_area is exact for any regular
        #  polygon, so the hexagon drawn to nut_m25_pocket_af across the flats
        #  is ngon_area(af / cos 30, 6) = 23.4173 and nothing here has to carry
        #  a second area formula.
        ("M2.5 nut, cowl",
         env["nut_m25_pocket_af"] / math.cos(math.radians(30)),
         env["nut_m25_pocket_d"],
         -env["carrier_t"] + env["nut_m25_pocket_d"], "DOWN",
         cowl_screw_pts(env), "cowl_screw_pts()", 6),
        ("M2.5, Feather", env["insert_m25_hole_d"], env["insert_m25_depth"],
         -env["insert_m25_depth"], "UP",
         [(env["feather_x"] + sx * env["feather_holes_usb"] / 2,
           env["interior_y1"] - env["feather"][1] / 2
           - env["feather_holes_span"] / 2) for sx in (1, -1)],
         "feather_inserts(), two at the USB end"),
        ("M2, breakouts", env["insert_m2_hole_d"], env["insert_m2_depth"],
         -env["insert_m2_depth"], "UP",
         [(cx + sx * env["pat_a_holes"] / 2, env["enc_y"] + sy
           * env["pat_a_holes"] / 2)
          for cx in env["cols"] for sx in (1, -1) for sy in (1, -1)]
         + [(env["slide_x"] + sx * env["pat_b_holes"][1] / 2,
             env["band_mid"] + sy * env["pat_b_holes"][0] / 2)
            for sx in (1, -1) for sy in (1, -1)],
         "four per bay: three encoder bays and the fader's"),
    )


def check_carrier_inserts(m, env, groups, fails, lines):
    """Every declared insert pocket, found AT ITS OWN AXIS or named missing.

    A count would say "10 of 14" and leave the reader to work out which four.
    This probes each declared (x, y) against the facets of the family's own
    plane, so the answer is a list of positions -- and the area of the region
    it landed in says whether the pocket is whole or has been eaten into.
    """
    fn = env.get("$fn", 96)
    lines.append("    inserts -- every heat-set pocket, probed at its own axis")
    lines.append(f"      {'family':26s} {'O':>6s} {'deep':>6s} {'floor z':>8s} "
                 f"{'whole':>7s} {'part':>5s} {'gone':>5s} {'want':>5s}")
    for fam in insert_families(env):
        (nm, d, depth, z, dr, pts, src), ffn = fam[:7], (fam[7] if len(fam) > 7
                                                         else fn)
        want = ngon_area(d, ffn)
        faces = next((f for (zz, dd), f in groups.items()
                      if dd == dr and abs(zz - z) <= PLANE_Z_TOL), [])
        comps = _components(m, faces)
        areas = [sum(float(m.area_faces[i]) for i in c) for c in comps]
        whole, part, gone = [], [], []
        for (px, py) in pts:
            hit = next((k for k, c in enumerate(comps)
                        if any(_tri_has(m, i, px, py) for i in c)), None)
            if hit is None:
                gone.append((px, py))
            elif abs(areas[hit] - want) <= PLANE_AREA_TOL:
                whole.append((px, py))
            else:
                part.append((px, py, areas[hit]))
        lines.append(f"      {nm:26s} {d:6.3f} {depth:6.3f} {z:+8.3f} "
                     f"{len(whole):7d} {len(part):5d} {len(gone):5d} "
                     f"{len(pts):5d}")
        base = CARRIER_INSERT_OPEN.get(nm)
        if base is None:
            for (px, py) in gone:
                fails.append(
                    f"carrier: the {nm} insert pocket at {px:+.3f},{py:+.3f} "
                    f"({src}) IS NOT IN THE MESH -- no facet of the {z:+.3f} "
                    f"{dr} plane covers its axis, so the O{d:.3f} x "
                    f"{depth:.3f} bore has been swallowed by another cavity "
                    f"and there is nothing to press an insert into.")
            for (px, py, a) in part:
                fails.append(
                    f"carrier: the {nm} insert pocket at {px:+.3f},{py:+.3f} "
                    f"has only {a:.4f} mm2 of its {want:.4f} mm2 end face "
                    f"left -- another cavity has cut into the bore.")
        elif len(whole) < base:
            fails.append(
                f"carrier: {len(whole)} whole {nm} insert pocket(s) against "
                f"the recorded {base} of {len(pts)} declared. One more has "
                f"gone: " + ", ".join(f"{x:+.3f},{y:+.3f}" for x, y in gone)
                + f". {src}; CARRIER_INSERT_OPEN in this file records which "
                f"were already missing and why.")
        elif len(whole) > base:
            lines.append(
                f"        CARRIER_INSERT_OPEN is STALE in the good direction: "
                f"{len(whole)} whole pockets against the recorded {base}. If "
                f"carrier.scad has been fixed, set "
                f"CARRIER_INSERT_OPEN[\"{nm}\"] = {len(whole)} -- and delete "
                f"the entry once it reaches {len(pts)}.")
        if len(whole) < len(pts) and base is not None:
            lines.append(
                f"        OPEN DEFECT IN carrier.scad, not in this file: of "
                f"{len(pts)} declared {nm} pockets, {len(gone)} have no "
                f"material on their axis at all and {len(part)} is eroded. "
                + "; ".join(f"{x:+.3f},{y:+.3f} ABSENT" for x, y in gone)
                + "".join(f"; {x:+.3f},{y:+.3f} has {a:.4f} of its "
                          f"{want:.4f} mm2 end face left, so its bore is open "
                          f"over the rest" for x, y, a in part)
                + ". The cowl's fastener stack clamps cowl, back panel and "
                "carrier together through these four, so it has one sound "
                "thread. CARRIER_INSERT_OPEN above carries the diagnosis.")


#  ---------------------------------------------------------------------------
#  THE CARRIER'S NUT-POCKET WEB, AND WHY THIS CHECK HAD TO BE WRITTEN
#
#  Of the three parts that carry a web, the carrier was the only one with no
#  web gate of any kind -- and the change that moved cowl_screw_dx() to 47.625
#  put the carrier's governing web there. check_carrier_inserts measures each
#  pocket's end-face AREA, which says whether a pocket is whole, not how far
#  it stands from anything. check_insert_clash is about z. check_webs reads the
#  laser DXFs and the carrier is not one. So the number that governed was
#  hand-recorded in a comment in parameters.scad, quoted second after a more
#  comfortable one, with nothing measuring it -- which is the same shape of
#  defect as the three insert pockets that did not exist, and it cost a phase.
#
#  WHAT IS MEASURED, AND WHAT IS NOT. This sections the carrier through the
#  nut-pocket band ONLY -- its two ends and its midpoint, by the same rule as
#  the cowl -- and gates the least wall in that section. The rest of the
#  carrier is still ungated: 37k facets over the whole z range is a different
#  job and a slow one. TO EXTEND IT: drop the band restriction below and give
#  it the same cuts-from-the-mesh's-own-vertices treatment the cowl check
#  uses; expect it to want a list of features that are legitimately thinner
#  than WEB_HARD_FLOOR before it is green.
#
#  WHICH FIGURE IS THE WEB. Polygon to polygon, off the artefact, the same
#  convention check_webs uses on the back panel -- the part's real geometry.
#  The ACROSS-CORNERS proxy is reported beside it but is NOT gated, because it
#  is not a distance: it replaces the hexagon with its circumscribed circle,
#  which the hexagon does not occupy. On this part the two readings differ by
#  0.4022 and they disagree about the floor, so the choice is load-bearing and
#  is made here, in code, instead of in a comment. The honest conservatism is
#  not the circumscribed circle, it is the PRINT tolerance, and that is
#  sourced: FDM bore +/-0.500 proto / +/-0.250 industrial, DIAMETRAL
#  [SVC protolabs, v2/docs/fabrication.md rows 3, 8, 9 -- row 8's own
#  arithmetic, 0.600 of interference becoming 0.100 at "+0.500 bore", is what
#  fixes it as diametral and not radial]. Both are reported against the floor.
def check_carrier_nut_web(env, export, fails, lines):
    """The least wall in the carrier's nut-pocket band, measured, not typed."""
    import numpy as np
    import trimesh
    from shapely.geometry import Point
    from shapely.ops import nearest_points

    stl = os.path.join(export, "stl", "carrier.stl")
    if not os.path.exists(stl):
        return
    m = trimesh.load(stl, process=False)
    m.merge_vertices()
    t = env.get("carrier_t")
    pd = env.get("nut_m25_pocket_d")
    af = env.get("nut_m25_pocket_af")
    if t is None or pd is None or af is None:
        fails.append(
            "check_carrier_nut_web could not read carrier_t, "
            "nut_m25_pocket_d and nut_m25_pocket_af out of parameters.scad, "
            "so the carrier's web was NOT measured.")
        return
    lo, hi = -t, -t + pd
    rc = af / (2.0 * math.cos(math.radians(30.0)))      # across-corners radius
    hexa = 3.0 * rc * rc * math.sin(math.radians(60.0))  # a regular hexagon's
    lines.append(f"    carrier nut-pocket web -- sectioned at z {lo:+.3f}, "
                 f"{0.5 * (lo + hi):+.3f} and {hi:+.3f} (the band's ends and "
                 f"its midpoint), polygon to polygon off the mesh")
    worst = None
    nhex = 0
    for z in (lo + 1e-6, 0.5 * (lo + hi), hi - 1e-6):
        rings, degs = _plane_rings(m, z)
        mat, polys, maxdepth = _section_polys(rings)
        if mat is None:
            continue
        if degs and set(degs) != {2}:
            fails.append(
                f"THE CARRIER'S SECTION AT z {z:+.4f} IS NOT A SET OF CLOSED "
                f"CURVES: vertex degrees {degs}, so the carrier's web was NOT "
                f"measured at this height.")
            continue
        hexes = [i for i, p in enumerate(polys) if abs(p.area - hexa) < 0.01]
        nhex = max(nhex, len(hexes))
        for i, j in itertools.combinations(range(len(polys)), 2):
            d = polys[i].exterior.distance(polys[j].exterior)
            if worst is not None and d >= worst[0]:
                continue
            p, q = nearest_points(polys[i].exterior, polys[j].exterior)
            mid = Point(0.5 * (p.x + q.x), 0.5 * (p.y + q.y))
            if not mat.contains(mid):
                continue                     # across a void, not a wall
            worst = (d, z, polys[i], polys[j], i in hexes or j in hexes)
        for i, poly in enumerate(polys):
            nk = _ring_neck(poly)
            if nk is None or (worst is not None and nk[0] >= worst[0]):
                continue
            mid = Point(0.5 * (nk[1][0] + nk[2][0]), 0.5 * (nk[1][1] + nk[2][1]))
            if not mat.contains(mid):
                continue
            worst = (nk[0], z, poly, poly, i in hexes)
    if nhex != 4:
        fails.append(
            f"THE CARRIER'S NUT-POCKET BAND HOLDS {nhex} HEXAGONAL POCKET(S) "
            f"OF AREA {hexa:.4f} mm2, want 4. The four cowl screws thread "
            f"into captive nuts and a pocket that is not there is a screw "
            f"with nothing to pull against. Measured off carrier.stl, so a "
            f"pocket that parameters.scad declares and the mesh does not have "
            f"is caught here and not at assembly.")
    if worst is None:
        fails.append(
            "THE CARRIER'S NUT-POCKET BAND HAS NO MEASURABLE WALL: no pair of "
            "rings and no ring self-approach put its shortest segment in "
            "material, so nothing here was measured.")
        return
    d, z, pa, pb, touches_hex = worst

    def lab(p):
        c, b = p.centroid, p.bounds
        return (f"eq-O{2 * math.sqrt(p.area / math.pi):8.4f} "
                f"[{b[2] - b[0]:7.3f} x{b[3] - b[1]:7.3f}] at "
                f"({c.x:+9.3f},{c.y:+9.3f})")

    same = "  -- a NECK INSIDE ONE RING" if pa is pb else ""
    lines.append(f"      LEAST WALL IN THE BAND {d:.4f} at z {z:+.4f}, "
                 f"{lab(pa)} <-> {lab(pb)}{same}")
    #  the ungated proxy and the sourced one, both reported against the floor
    for nm, give in (("industrial +/-0.250 diametral", 0.125),
                     ("prototype  +/-0.500 diametral", 0.250)):
        w = d - give
        lines.append(f"        at FDM bore {nm}: {w:.4f} "
                     f"({'OVER' if w >= WEB_HARD_FLOOR else 'UNDER'} the "
                     f"{WEB_HARD_FLOOR:.3f} floor)")
    if touches_hex:
        cen = pa.centroid if pa.area < pb.area else pb.centroid
        other = pb if pa.area < pb.area else pa
        cc = other.exterior.distance(Point(cen.x, cen.y))
        lines.append(f"        across-corners proxy {cc - rc:.4f} (centre to "
                     f"contour {cc:.4f} less the {rc:.4f} circumscribed "
                     f"radius) -- NOT gated: it is not a distance, the "
                     f"hexagon does not occupy that circle, and the two "
                     f"readings differ by {rc - af / 2.0:.4f}")
    if d < WEB_HARD_FLOOR:
        fails.append(
            f"THE CARRIER'S LEAST WALL IN THE NUT-POCKET BAND IS {d:.4f}, "
            f"BELOW THE {WEB_HARD_FLOOR:.3f} HARD FLOOR: at z {z:+.4f}, "
            f"between {lab(pa)} and {lab(pb)}. Measured polygon to polygon "
            f"off carrier.stl, so it is the artefact and not the datums. The "
            f"carrier had no web gate at all until this one, and the change "
            f"that moved cowl_screw_dx() put the part's governing web here, "
            f"so a hand-recorded figure in a comment was all that stood "
            f"between this and a print.")
    elif d < CARRIER_NUT_WEB_BASELINE - WEB_BASELINE_TOL:
        fails.append(
            f"the carrier's nut-pocket web got WORSE -- {d:.4f} at z {z:+.4f} "
            f"against the recorded baseline {CARRIER_NUT_WEB_BASELINE:.3f}. "
            f"It is accepted at the baseline and not below it. If the loss is "
            f"deliberate, re-export and update CARRIER_NUT_WEB_BASELINE to "
            f"{d:.3f} in this file.")
    elif d > CARRIER_NUT_WEB_BASELINE + WEB_BASELINE_TOL:
        lines.append(f"      baseline is STALE and in the good direction: "
                     f"{d:.4f} vs {CARRIER_NUT_WEB_BASELINE:.3f} -- update "
                     f"CARRIER_NUT_WEB_BASELINE to {d:.3f}")


def check_carrier_planes(m, env, groups, fails, lines):
    """The whole Z structure of the carrier, measured, against the declaration.

    Two gates and they catch opposite mistakes. A DECLARED plane that is absent
    means a feature stopped being drawn -- and a vanished feature leaves no
    unaccounted anything, so nothing else in this file can see it. An
    UNDECLARED plane means a feature arrived that nobody wrote down.
    """
    want = [(fn(env), dr, a, n, what) for fn, dr, a, n, what in CARRIER_PLANES]
    lines.append(f"    planes -- {len(groups)} horizontal planes in the mesh, "
                 f"{len(want)} declared")
    lines.append(f"      {'z':>9s} {'dir':5s} {'area':>12s} {'d-area':>8s} "
                 f"{'reg':>4s} {'want':>4s}  what it is")
    claimed = set()
    for (z, dr, area, nreg, what) in want:
        key = next((k for k in groups
                    if k[1] == dr and abs(k[0] - z) <= PLANE_Z_TOL), None)
        if key is None:
            fails.append(
                f"carrier: NO {dr}-facing plane at z = {z:+.4f} -- "
                f"{what.splitlines()[0]}. A declared plane that is not in the "
                f"mesh means the feature that terminates there stopped being "
                f"drawn, and a vanished feature leaves nothing unaccounted for "
                f"anything else here to notice.")
            continue
        claimed.add(key)
        faces = groups[key]
        got = sum(float(m.area_faces[i]) for i in faces)
        reg = len(_components(m, faces))
        lines.append(f"      {key[0]:+9.4f} {dr:5s} {got:12.4f} "
                     f"{got-area:+8.4f} {reg:4d} {nreg:4d}  {what[:52]}")
        if abs(got - area) > PLANE_AREA_TOL:
            fails.append(
                f"carrier: the {dr} plane at z = {z:+.4f} ({what[:60]}) "
                f"measures {got:.4f} mm2 against the recorded {area:.4f} -- "
                f"out by {got-area:+.4f}, more than the {PLANE_AREA_TOL:.3f} "
                f"float32 budget. Re-record from the artefact only if the "
                f"change was deliberate.")
        if reg != nreg:
            fails.append(
                f"carrier: the {dr} plane at z = {z:+.4f} ({what[:60]}) is "
                f"{reg} connected region(s) against the recorded {nreg}. The "
                f"area can be right while the count is wrong: two features "
                f"merging into one, or one splitting in two, is a change of "
                f"shape at constant area.")
    for key in sorted(set(groups) - claimed, key=lambda k: -k[0]):
        faces = groups[key]
        got = sum(float(m.area_faces[i]) for i in faces)
        fails.append(
            f"carrier: an UNDECLARED {key[1]}-facing plane at z = "
            f"{key[0]:+.4f}, {got:.4f} mm2 over {len(faces)} facet(s) in "
            f"{len(_components(m, faces))} region(s). Every designed feature "
            f"of this part terminates in a plane CARRIER_PLANES names; this "
            f"one is not named, so either a feature arrived undocumented or a "
            f"cut stopped at the wrong height.")


#  THE EXPORTED MESHES, and what is known about each independently of the mesh.
#  `t` names the datum that gives the part's Z extent where one exists; the
#  plan extents and the volume are MEASURED baselines, because nothing in
#  parameters.scad declares the cowl's or the fader cap's outline.
#
#  THE Z FACTS ARE THE ONES THAT MATTER TO A READER, because four of these
#  parts are drawn at their own origin and are placed by a pure translate: get
#  one upside down and the picture is silently wrong. v2/tools/render owns the
#  placement; this records which face is which so the claim can be checked.
MESHES = (
    # name              z datum     plan w      plan d    volume      euler
    ("slab",            "acrylic_t", 304.600, 190.375, 350042.7559, -26),
    ("backpanel",       "back_t",    304.600, 190.375, 177036.5845, -32),
    #  carrier: WAS 444256.5134 / -20. The volume lost 91.3346 -- seven
    #  O1.700 x 3.810 pilots that kbd_pilot_cut() had been failing to cut
    #  (60.4866) and four hexagonal nut pockets (206.0722), against 175.2242
    #  of material RETURNED by dropping the four cowl insert bores. The euler
    #  moved +6, which is exactly THREE HANDLES CLOSED: three of those four
    #  bores were through-holes. The seven pilots and the four nut pockets are
    #  blind, so neither adds a handle, which is the independent confirmation
    #  that the +6 is the three bores and nothing else.
    ("carrier",         "carrier_h", 300.790, 186.565, 444165.1788, -14),
    #  cowl: WAS 12143.2409. The plan and the euler do NOT move -- four
    #  clearance holes before and after, and the bounding box is still
    #  window + 2*(wall + flange), which is what export.py transcribes. The
    #  124.3940 is four O4.900 x 2.540 head counterbores and the 82.1133 is two
    #  washer scallops at back_screw_pts()[4] and [5], whose O6.000 washers
    #  overlapped this part's rim by 6.1013 and 8.4883 mm2.
    ("cowl",            None,        107.510,  41.613,  11936.7336,  -6),
    ("foot",            "foot_h",     13.970,  34.290,   2433.7571,   2),
    ("knob_encoder",    "knob_h",     25.247,  25.247,   5400.8707,   2),
    ("knob_pot",        "knob_h",     25.247,  25.247,   5372.3497,   2),
    ("fader_cap",       None,         16.000,  25.400,   2402.9466,   2),
    ("knob_mould_brim", None,         50.800,  50.800,  21069.3280,  -6),
    ("knob_mould_plug", None,         50.800,  50.800,  38474.3493,  -8),
)
#  z0 .. z1 as the part is DRAWN, and the face a reader has to get the right way
#  up. Measured; the expression beside each is where the number comes from.
MESH_Z = {
    "slab":            (0.0, "acrylic_t", None),
    "backpanel":       (0.0, "back_t", None),
    "carrier":         (None, None, None),      # -carrier_t .. carrier_rim_h
    "cowl":            (0.0, None,
                        "the face at z1 is the one against the BACK PANEL"),
    "foot":            (0.0, "foot_h",
                        "the sole is z = 0; the screw axis is the origin and "
                        "the toe runs to -y"),
    "knob_encoder":    (0.0, "knob_h",
                        "z = acrylic_t is the brim's shoulder, which lands on "
                        "the acrylic's TOP face"),
    "knob_pot":        (0.0, "knob_h",
                        "z = acrylic_t is the brim's shoulder, which lands on "
                        "the acrylic's TOP face"),
    "fader_cap":       (0.0, None, None),
    "knob_mould_brim": (None, None, "TOOLING, not part of the instrument"),
    "knob_mould_plug": (None, None, "TOOLING, not part of the instrument"),
}


#  WHICH OF THE TEN BACK-PANEL SCREWS CARRY A FOOT.
#  parameters.scad 6b is the source and it is an ASSEMBLY fact: no file's
#  geometry carries it, because the panel drills ten identical apertures and
#  the foot is a separate printed part. foot_pts() is a vector-returning
#  OpenSCAD function, so datums() cannot evaluate it -- but it is exactly these
#  four of back_screw_pts(), which this file already transcribes, so there is
#  nothing new to restate. 6b also records that these four are the holes that
#  must NOT take a washer: a O6.000 DIN 125 A2.5 cannot enter the foot's
#  O4.300 pocket mouth and would stand all four pads 0.500 off the panel.
FOOT_AT = (0, 3, 8, 9)


def check_foot_cowl_plan(env, export, fails, lines):
    """Do the feet clear the cowl in PLAN? Nothing in this repo measured it.

    The two rear feet sit at the panel's +/-x rear screws and the cowl's tray
    hangs off the 18650 window, which is well toward -x. The -x rear foot
    OVERLAPS THE COWL IN X and clears it only in Y. A foot standing on the
    cowl's flange lifts the whole deck on three points plus a rock, and both
    parts are drawn in different files from different datums, so nothing makes
    the two agree. Measured here from the two exported meshes' real plan
    outlines at their declared positions -- not from bounding boxes, which
    would both overstate the overlap and understate the gap.
    """
    import trimesh
    from shapely.geometry import MultiPolygon, Polygon
    from shapely.ops import unary_union

    def plan(name):
        p = os.path.join(export, "stl", f"{name}.stl")
        if not os.path.exists(p):
            return None
        m = trimesh.load(p)
        tri = [Polygon(m.vertices[f][:, :2]) for f in m.faces]
        u = unary_union([t for t in tri if t.area > 1e-12]).buffer(0)
        return u if isinstance(u, (Polygon, MultiPolygon)) else None

    foot, cowl = plan("foot"), plan("cowl")
    if foot is None or cowl is None:
        return
    from shapely.affinity import translate as _tr
    cx, cy = cowl_xy(env)
    cowl = _tr(cowl, cx, cy)
    pts = back_screw_pts(env)
    lines.append(f"    plan clearance -- the four feet against the cowl, both "
                 f"from their exported meshes at their declared positions "
                 f"(cowl_xy {cx:+.3f},{cy:+.3f})")
    cb = cowl.bounds
    lines.append(f"      cowl plan x {cb[0]:+.3f}..{cb[2]:+.3f}  "
                 f"y {cb[1]:+.3f}..{cb[3]:+.3f}")
    worst = None
    for k in FOOT_AT:
        fx, fy = pts[k]
        f = _tr(foot, fx, fy)
        fb = f.bounds
        d = f.distance(cowl)
        ox = min(fb[2], cb[2]) - max(fb[0], cb[0])
        oy = min(fb[3], cb[3]) - max(fb[1], cb[1])
        lines.append(f"      foot at back screw {k} ({fx:+8.3f},{fy:+8.3f})  "
                     f"x {fb[0]:+9.3f}..{fb[2]:+8.3f}  "
                     f"y {fb[1]:+8.3f}..{fb[3]:+8.3f}  gap {d:8.4f}  "
                     f"x-overlap {ox:+8.3f}  y-overlap {oy:+8.3f}")
        if worst is None or d < worst[0]:
            worst = (d, k, fx, fy, ox, oy)
        if d <= 0:
            fails.append(
                f"THE FOOT AT BACK SCREW {k} ({fx:+.3f},{fy:+.3f}) STANDS ON "
                f"THE COWL. Their plan outlines overlap by {ox:+.3f} in x and "
                f"{oy:+.3f} in y. The foot is {env['foot_h']:.3f} tall and the "
                f"cowl's flange is between it and the back panel, so the deck "
                f"rests on three feet and a rock. foot_pts() and cowl_xy() are "
                f"derived from different datums in different files and nothing "
                f"else makes them agree.")
    if worst and worst[0] > 0:
        lines.append(
            f"      tightest {worst[0]:.4f} at back screw {worst[1]}, which "
            f"overlaps the cowl by {worst[4]:+.3f} in x and clears it by "
            f"{worst[0]:.4f} in y. This is the first number to go negative if "
            f"the cowl's window, disp_x or band_mid move.")


# ===========================================================================
#  CHECK -- THE COWL'S PLANE SECTIONS: ITS OWN THINNEST WALL, AND THE BOUGHT
#  METAL ITS RIM LANDS ON
# ===========================================================================
#  WHY NOT A SILHOUETTE, which is what check_foot_cowl_plan builds and the
#  obvious thing to copy. That function unions every triangle's 2D projection,
#  so it sees the part's OUTLINE and nothing inside it. The cowl's four head
#  counterbores are BLIND: each is a O4.900 void opening at the bed face with
#  1.280 of PETG over it, so in projection the ceiling covers the void
#  completely and the silhouette is byte-identical whether the counterbore is
#  there or not. A silhouette gate would have passed the defect this check
#  exists to catch -- a counterbore standing 0.854 from a washer scallop -- and
#  would equally pass a counterbore that broke clean out through the cavity
#  wall. So this check SECTIONS the mesh, at a z inside every band the part
#  actually has.
#
#  WHERE THE SECTIONS GO, AND NOTHING HERE IS TYPED. The breakpoints are the
#  distinct z of the MESH'S OWN vertices -- so every change of cross-section is
#  a band boundary -- union the depths at which the panel's bought metal starts
#  and stops obstructing. The part is sectioned at the MIDPOINT of every
#  resulting band, so each band is visited exactly once and a band that a later
#  change introduces is visited without editing this file. The cowl today has
#  three bands of its own (floor, cavity, over-the-counterbore) and the washer
#  adds a fourth boundary 0.500 under the rim; sectioning only at the mesh's
#  own levels would never put a plane inside the washer's 0.500 and would miss
#  half of (B).
#
#  mesh.section() and mesh.split() RAISE here -- this trimesh has no scipy --
#  so the plane cut is done by hand: every triangle is intersected with the
#  plane, the crossing points are snapped onto a 1e-7 mm lattice (the two
#  triangles sharing an edge compute the same point from the same two vertices,
#  so the snap is exact in intent and not a tolerance), and the rings are
#  walked off the degree-2 adjacency. A section whose vertex degrees are not
#  all 2 is reported and not trusted rather than silently measured.
#
#  (A) THE PART AGAINST ITSELF. Boundary-to-boundary distance between every
#  distinct pair of rings in each section -- the same measure check_webs takes
#  on the flat parts, which covers every hole-to-outline land as a special
#  case. A pair whose shortest segment does not lie in MATERIAL is reported and
#  NOT gated: that gap is across a void and is not a wall. It cannot hide a
#  real web, because if a third ring intervenes then that ring is nearer still
#  and is caught on a pair of its own.
#
#  (B) THE PART AGAINST THE PANEL'S BOUGHT METAL. parameters.scad 6b ends
#  "NOTHING DRAWS THESE TWO AND THAT IS CORRECT: a bought washer is not a cut
#  feature and not a printed one, so no part file reads them and no checker
#  gates them." THIS is that checker. Six of the ten back_screw_pts() carry a
#  DIN 125 A 2.5 washer under an ISO 4762 cap head and four carry a bare head,
#  all on the panel's UNDERSIDE -- the same face the cowl's rim lands on.
#  Measured DOWN from that face a washered hole obstructs O6.000 for
#  screw_m25_washer_t and then O4.500 for screw_m25_head_h; a bare hole
#  obstructs O4.500 from the face. Cowl material that shares a z with one of
#  those discs and overlaps it in plan is a joint that cannot close, and the
#  flange overlapped two of them by 6.1013 and 8.4883 mm2 before the scallops
#  existed. These two diameters are the ONE thing here that is transcribed
#  rather than measured, for the reason 6b gives: there is no artefact to
#  measure, because nothing draws a bought washer.
#
#  THE FLOOR IS THE PART'S OWN DECLARED MINIMUM, NOT A STRENGTH. This
#  repository carries no PETG strength figure of any kind, in any file -- the
#  only mechanical properties in it are PMMA's E = 3300 MPa and TPU 95A's creep
#  and modulus -- and parameters.scad 6b's "I will not invent a number" is on
#  the record for exactly this situation. So the floor is not an allowable:
#  cowl.scad justifies cowl_wall()'s 1.500 of material and cowl_floor()'s 1.500
#  on one figure, "3.54 extrusion lines at a 0.4 nozzle", and 1.500 is
#  therefore what this part has already declared its own thinnest feature may
#  be. A web below it is not weak-by-calculation; it is thinner than the part
#  says it is drawn to.  [REPO cowl.scad, parameters.scad 12]
COWL_WEB_FLOOR = 1.500

#  THE RECORDED SECTION BASELINE, and like WEB_BASELINE it is a MEASUREMENT.
#  Re-record ONLY with a deliberate geometry change, and only from the artefact:
#      /usr/bin/python3.11 v2/tools/export.py
#      /usr/bin/python3.11 v2/tools/check_cad.py    # prints the new value
#  THE RIM'S TIGHTEST APPROACH TO THE PANEL'S BOUGHT METAL. This number was
#  printed and never gated, and the check's own text called it "the first
#  number to go negative if cowl_flange(), cowl_xy() or cowl_washer_relief_d()
#  moves" -- but nothing fired until it HAD gone negative AND produced an
#  overlap area, so the whole 0.498 could be spent in silence. That band is
#  exactly where this repository's sourced tolerances live: FDM bore +/-0.500
#  proto / +/-0.250 industrial, diametral [SVC protolabs, v2/docs/
#  fabrication.md rows 3, 8, 9], and DIN 125 d1 up to 2.84 [parameters.scad
#  825], which together can consume it without any geometry moving at all.
COWL_METAL_CLEAR_BASELINE = 0.498  # [MEASURED 2026-11-08,
                                   # v2/export/stl/cowl.stl] the rim's
                                   # tightest approach to the panel's bought
                                   # metal, at the O6.000 washer on back screw
                                   # 5. Re-record ONLY from the artefact.

CARRIER_NUT_WEB_BASELINE = 2.281  # [MEASURED 2026-11-08,
                                  # v2/export/stl/carrier.stl] The least wall
                                  # in the carrier's nut-pocket band, polygon
                                  # to polygon: the +x nut pocket against the
                                  # merged 18650 window, 2.2816. Re-record
                                  # ONLY from the artefact.

COWL_SECTION_BASELINE = 1.965   # [MEASURED 2026-11-07, v2/export/stl/cowl.stl]
                                # Was 0.8542 at cowl_screw_dx() 48.895, between
                                # a O4.900 counterbore and a washer scallop, and
                                # that is the defect this check was written for:
                                # under the 1.500 the part declares and under
                                # insert_m25_wall's 1.600, about 2.1 extrusion
                                # lines at a 0.4 nozzle. 47.625 moves the two
                                # left counterbores 1.270 off the scallops and
                                # the governing pair is still the same one, at
                                # 1.9654 measured here and 1.9653 derived --
                                # over the 1.500 by 0.4654 and over 1.600 by
                                # 0.3654. Reproduced off the same mesh a second,
                                # independent way at 1.9657 by casting 7200
                                # radial rays per bore and taking the first two
                                # surface crossings, which is why the baseline
                                # is carried to three decimals and not four.
                                # See parameters.scad 12 cowl_screw_dx() for why
                                # 75 x grid is the ONLY grid value that satisfies
                                # both this and the back panel's 2.000 floor.


def _ring_neck(poly, k=4.0, halo=4):
    """The narrowest NECK inside one ring: the least distance between two
    stretches of the SAME boundary that are far apart ALONG it.

    WHY THIS IS NOT "the least distance between non-adjacent edges". That is
    the obvious rule and it is wrong, because a ring's own curvature beats
    any index-based skip: on this repository's bores -- 96-gons, and the
    section splits their edges further -- a skip of two steps reported 0.0000
    across a O2.900 bore, which is not a neck at all but the hole it is meant
    to ignore. The criterion has to be scale-free, so it is ARC LENGTH:

        report a distance d only if the two points are more than k*d apart
        ALONG the ring.

    On a circle of radius R two points subtending theta have d = 2R sin(theta/2)
    and arc R*theta, and theta <= 8 sin(theta/2) for every theta in (0, pi].
    So NO pair on a convex ring EVER passes at k = 4, whatever its radius or
    its tessellation -- convex bores are excluded by geometry and not by a
    special case. A real neck is the opposite shape: to get from one of its
    faces to the other you walk around the feature that made it, so a 0.4642
    rib between two O7.000 scallops has an arc separation near 11 mm. Ratio
    24, not 4.

    Vertex pairs first, which is O(n^2) in numpy and lands within one vertex
    spacing; then exact segment-to-segment over a halo of edges around the
    best pair, so the number returned is a true distance and not a sample.
    Returns (distance, point_a, point_b), or None when the ring has no neck.
    """
    import numpy as np
    from shapely.geometry import LineString
    from shapely.ops import nearest_points
    co = np.asarray(poly.exterior.coords[:-1], dtype=float)
    n = len(co)
    if n < 8:
        return None
    seg = np.linalg.norm(np.roll(co, -1, axis=0) - co, axis=1)
    per = float(seg.sum())
    arcpos = np.concatenate([[0.0], np.cumsum(seg)[:-1]])
    D = np.linalg.norm(co[:, None, :] - co[None, :, :], axis=2)
    A = np.abs(arcpos[:, None] - arcpos[None, :])
    A = np.minimum(A, per - A)
    ok = A > k * D
    if not ok.any():
        return None
    D = np.where(ok, D, np.inf)
    ia, ib = np.unravel_index(int(np.argmin(D)), D.shape)
    best = None
    for u in range(int(ia) - halo, int(ia) + halo + 1):
        su = LineString([co[u % n], co[(u + 1) % n]])
        for v in range(int(ib) - halo, int(ib) + halo + 1):
            sv = LineString([co[v % n], co[(v + 1) % n]])
            av = abs(arcpos[u % n] - arcpos[v % n])
            av = min(av, per - av)
            d = su.distance(sv)
            if av <= k * d:
                continue
            if best is None or d < best[0]:
                p0, p1 = nearest_points(su, sv)
                best = (d, (p0.x, p0.y), (p1.x, p1.y))
    if best is None:
        return (float(D[ia, ib]), tuple(co[ia]), tuple(co[ib]))
    return best


def _plane_rings(m, z, snap=1e7):
    """Closed rings of the section z = const, walked off the raw triangles.

    Generic: the cowl section check and the carrier nut-web check both use it.
    """
    import numpy as np
    from collections import defaultdict
    V = np.asarray(m.vertices, float)

    def key(p):
        return (int(round(p[0] * snap)), int(round(p[1] * snap)))

    adj, coord = defaultdict(set), {}
    for tri in np.asarray(m.faces, int):
        p = V[tri]
        d = p[:, 2] - z
        if (d > 0).all() or (d < 0).all():
            continue
        pts = []
        for i in range(3):
            a, b = i, (i + 1) % 3
            da, db = d[a], d[b]
            if da == 0.0:
                pts.append(p[a][:2])
            elif (da < 0 < db) or (db < 0 < da):
                t = da / (da - db)
                pts.append((p[a] + t * (p[b] - p[a]))[:2])
        seen, uniq = set(), []
        for q in pts:
            k = key(q)
            if k not in seen:
                seen.add(k)
                uniq.append((k, q))
        if len(uniq) != 2:
            continue                    # a vertex-grazing triangle: no segment
        (ka, qa), (kb, qb) = uniq
        coord[ka], coord[kb] = qa, qb
        adj[ka].add(kb)
        adj[kb].add(ka)
    degs = {}
    for k, nb in adj.items():
        degs[len(nb)] = degs.get(len(nb), 0) + 1
    rings, done = [], set()
    for start in list(adj):
        if start in done:
            continue
        walk, prev, cur = [start], None, start
        done.add(start)
        while True:
            nxt = [n for n in adj[cur] if n != prev and n not in done]
            if not nxt:
                if start in adj[cur] and len(walk) > 2:
                    rings.append([coord[k] for k in walk])
                break
            prev, cur = cur, nxt[0]
            walk.append(cur)
            done.add(cur)
    return rings, degs


def _section_polys(rings):
    """(material polygon, rings as polygons, max nesting depth) for a section."""
    from shapely.geometry import Point, Polygon
    from shapely.ops import unary_union
    polys = [Polygon(r) for r in rings if len(r) > 2]
    polys = [p for p in polys if p.area > 1e-9]
    #  NESTING DEPTH MUST BE TESTED WITH A POINT ON THE RING. A centroid does
    #  not work and fails silently in the worst direction: the OUTER ring's
    #  centroid lies inside the cavity ring, which classifies the outer
    #  boundary as a hole and leaves the section with no shell at all.
    depth = []
    for i, p in enumerate(polys):
        probe = Point(p.exterior.coords[0])
        depth.append(sum(1 for j, q in enumerate(polys)
                         if j != i and q.contains(probe)))
    shells = [p for p, d in zip(polys, depth) if d % 2 == 0]
    holes = [p for p, d in zip(polys, depth) if d % 2 == 1]
    mat = unary_union([
        Polygon(s.exterior.coords,
                [h.exterior.coords for h in holes
                 if s.contains(Point(h.exterior.coords[0]))])
        for s in shells]) if shells else None
    return mat, polys, (max(depth) if depth else 0)


def check_section_polys_clearance(env, export, fails, lines):
    """The cowl's own thinnest wall, and the panel's bought metal under its rim.

    Measured by SECTIONING the exported mesh -- not from a silhouette, which
    cannot see a blind counterbore at all, and not from the parameters the part
    was drawn from, which would pass while the artefact was wrong. The block
    above this function has the whole argument.
    """
    import numpy as np
    import trimesh
    from shapely.geometry import Point, LineString
    from shapely.affinity import translate as _tr
    from shapely.ops import nearest_points

    stl = os.path.join(export, "stl", "cowl.stl")
    if not os.path.exists(stl):
        return
    m = trimesh.load(stl, process=False)
    #  a binary STL loaded with process=False reports is_watertight False on
    #  EVERY part in this repo -- unmerged vertices in the file format, not a
    #  defect -- so merge before anything reads the topology
    m.merge_vertices()
    V = np.asarray(m.vertices, float)
    zlo, zhi = float(V[:, 2].min()), float(V[:, 2].max())
    cx, cy = cowl_xy(env)

    #  THE BOUGHT METAL, as discs measured DOWN from the rim plane. The rim
    #  plane is the MESH's own zmax, not a datum: MESH_Z records that the face
    #  at z1 is the one against the back panel.
    wo, wt = env["screw_m25_washer_od"], env["screw_m25_washer_t"]
    hd, hh = env["screw_m25_head_d"], env["screw_m25_head_h"]
    obstr = []
    for k, (px, py) in enumerate(back_screw_pts(env)):
        top = zhi
        stack = ([] if k in FOOT_AT else [("DIN 125 washer", wo, wt)])
        stack.append(("ISO 4762 head", hd, hh))
        for nm, dia, th in stack:
            obstr.append((k, px, py, nm, dia, top - th, top))
            top -= th

    cuts = sorted({round(float(v), 6) for v in V[:, 2]}
                  | {round(o[5], 6) for o in obstr if zlo < o[5] < zhi}
                  | {round(o[6], 6) for o in obstr if zlo < o[6] < zhi})
    bands = [(a, b) for a, b in zip(cuts, cuts[1:]) if b - a > 1e-6]

    lines.append(f"    cowl section clearance -- {len(bands)} band(s) from the "
                 f"mesh's own z levels {cuts} plus the bought metal's steps; "
                 f"each sectioned at BOTH ENDS and its midpoint and the "
                 f"worst of the three kept (a silhouette cannot see "
                 f"a blind counterbore -- see this check's header)")
    worst_web = None
    worst_clear = None
    #  COLLISIONS ARE AGGREGATED PER FASTENER, not per band. One washer sitting
    #  on this rim is ONE defect; it shows up in every band its stack spans, and
    #  reporting it once per band buried the two real collisions under ten rows.
    hits = {}
    #  EVERY BAND IS SECTIONED AT THREE HEIGHTS, not one: its two ends and
    #  its midpoint, and the band's figure is the WORST of the three. A
    #  midpoint alone is exact only for a PRISM. The cowl is a prism today --
    #  all three heights give the same wall to 4 dp, which is how this change
    #  was shown to move no number -- but a lead-in chamfer of t per side
    #  (foot.scad already has one, so this is not hypothetical) has vertices
    #  only at its ends, so its taper is ONE band and a midpoint hides t/2 of
    #  its own worst section. Measured: a 0.520/side lead-in puts the true
    #  wall at 1.4455, under the floor, and the midpoint read 1.5294 and the
    #  floor rule stayed silent.
    for a, b in bands:
        samples = []
        for z in (a + 1e-6, 0.5 * (a + b), b - 1e-6):
            rings, degs = _plane_rings(m, z)
            mat, polys, maxdepth = _section_polys(rings)
            if degs and set(degs) != {2}:
                fails.append(
                    f"THE COWL'S SECTION AT z {z:.4f} IS NOT A SET OF CLOSED "
                    f"CURVES: vertex degrees {degs}, so this check could not "
                    f"measure it. A manifold solid cuts a plane in closed "
                    f"rings; this one did not.")
                continue
            if mat is None:
                continue
            if maxdepth > 1:
                #  THIS WAS A NOTE AND A NOTE WAS NOT ENOUGH. The material
                #  polygon is what (A) decides a wall from -- mat.contains()
                #  is what sorts a wall from a void -- and what (B) measures
                #  the bought metal against. If it is not trusted then this
                #  height was not measured, and a wall wrongly sorted into
                #  the ungated ACROSS A VOID bucket goes silent. Fail, the
                #  same way the degree check above already does.
                fails.append(
                    f"THE COWL'S SECTION AT z {z:+.4f} HAS RINGS NESTING "
                    f"{maxdepth} DEEP, which this check does not model: the "
                    f"material polygon it would gate (A) with and measure "
                    f"(B) against is not trustworthy, so THIS HEIGHT WAS NOT "
                    f"MEASURED. Nothing below is a reading of it.")
                continue
            samples.append((z, mat, polys))
        if not samples:
            lines.append(f"      z {0.5 * (a + b):+7.4f}  no material")
            continue

        def lab(polys, i):
            #  eq-O, not O: a ring is only a circle if it is one, and the
            #  outer boundary is a 107.510 x 41.613 rounded rectangle. The
            #  span is printed beside it so a ring is identifiable either way.
            p = polys[i]
            c = p.centroid
            bb = p.bounds
            return (f"eq-O{2 * math.sqrt(p.area / math.pi):8.4f} "
                    f"[{bb[2] - bb[0]:7.3f} x{bb[3] - bb[1]:7.3f}] at "
                    f"({c.x + cx:+9.3f},{c.y + cy:+9.3f})")

        # ---- (A) the part against itself ------------------------------------
        #  TWO measurements, and the second one is the whole reason this block
        #  was rewritten:
        #
        #  DISTINCT RINGS -- a bore against a bore, a bore against the
        #  boundary. This is all the first version of this check did.
        #
        #  EACH RING AGAINST ITSELF -- a neck inside ONE ring. Two washer
        #  scallops bitten into the same outer boundary leave a rib between
        #  them that belongs to no PAIR of rings, so a pairwise loop cannot
        #  see it AT ANY WIDTH. Demonstrated on this very part: one extra
        #  back_screw_pts() entry, which cowl.scad's derived-not-typed scallop
        #  loop picks up by itself, leaves a 0.4642 mm rib between two O7.000
        #  bites -- thinner than the 0.8542 this check exists to catch -- and
        #  the pairwise-only version printed "THINNEST WALL IN THE PART
        #  1.9654" and the run came back PASS. Edges within two steps along a
        #  ring are skipped so a facet is not measured against its own
        #  neighbours; at $fn 96 that is 7.5 degrees of arc and cannot hide a
        #  real neck.
        best = None
        for z, mat, polys in samples:
            pairs = []
            for i, j in itertools.combinations(range(len(polys)), 2):
                d = polys[i].exterior.distance(polys[j].exterior)
                p, q = nearest_points(polys[i].exterior, polys[j].exterior)
                mid = Point(0.5 * (p.x + q.x), 0.5 * (p.y + q.y))
                pairs.append((d, i, j, mat.contains(mid)))
            for i, poly in enumerate(polys):
                sb = _ring_neck(poly)
                if sb is None:
                    continue
                d, pa, pb = sb
                mid = Point(0.5 * (pa[0] + pb[0]), 0.5 * (pa[1] + pb[1]))
                pairs.append((d, i, i, mat.contains(mid)))
            pairs.sort(key=lambda r: r[0])
            sol = [r for r in pairs if r[3]]
            voi = [r for r in pairs if not r[3]]
            key = sol[0][0] if sol else 1e9
            if best is None or key < best[0]:
                best = (key, z, polys, sol, voi)

        _, z, polys, solid, voidp = best
        if solid:
            d, i, j, _x = solid[0]
            same = " (a neck inside ONE ring)" if i == j else ""
            lines.append(f"      z {z:+7.4f}  {len(polys)} ring(s), thinnest "
                         f"wall {d:.4f}{same}  {lab(polys, i)} <-> "
                         f"{lab(polys, j)}")
            if worst_web is None or d < worst_web[0]:
                worst_web = (d, z, lab(polys, i), lab(polys, j), i == j)
        else:
            lines.append(f"      z {z:+7.4f}  {len(polys)} ring(s), no pair "
                         f"whose shortest segment lies in material")
        if voidp and voidp[0][0] < (solid[0][0] if solid else 1e9):
            d, i, j, _x = voidp[0]
            lines.append(f"                 ({d:.4f} {lab(polys, i)} <-> "
                         f"{lab(polys, j)} is ACROSS A VOID, not a wall -- "
                         f"not gated)")

        # ---- (B) the part against the panel's bought metal ------------------
        #  Run at every sampled height, but AGGREGATED PER FASTENER and
        #  counted per BAND. One washer sitting on this rim is ONE defect; it
        #  shows up in every band its stack spans, and reporting it once per
        #  band buried the two real collisions under ten rows. Counting
        #  samples instead of bands would now treble that count.
        for z, mat, polys in samples:
            gmat = _tr(mat, cx, cy)
            for (k, px, py, nm, dia, olo, ohi) in obstr:
                if not (olo < z < ohi):
                    continue
                disc = Point(px, py).buffer(dia / 2.0, quad_segs=512)
                inter = gmat.intersection(disc)
                if not inter.is_empty and inter.area > 1e-9:
                    rec = hits.setdefault(k, {"px": px, "py": py,
                                              "bands": set(),
                                              "worst": None, "parts": set()})
                    rec["bands"].add((a, b))
                    rec["parts"].add(nm)
                    if rec["worst"] is None or inter.area > rec["worst"][0]:
                        rec["worst"] = (inter.area, nm, dia, z, inter.bounds)
                else:
                    d = gmat.distance(disc)
                    if worst_clear is None or d < worst_clear[0]:
                        worst_clear = (d, z, k, px, py, nm, dia)

    if worst_web:
        d, z, li, lj, same = worst_web
        lines.append(f"      THINNEST WALL IN THE PART {d:.4f} at z {z:+.4f}, "
                     f"{li} <-> {lj}"
                     f"{'  -- a NECK INSIDE ONE RING' if same else ''}")
        if d < COWL_WEB_FLOOR:
            fails.append(
                f"THE COWL'S THINNEST WALL IS {d:.4f}, BELOW THE "
                f"{COWL_WEB_FLOOR:.3f} THIS PART DECLARES: at z {z:+.4f}, "
                f"between {li} and {lj}. cowl.scad justifies cowl_wall()'s "
                f"1.500 of material and cowl_floor()'s 1.500 on \"3.54 "
                f"extrusion lines at a 0.4 nozzle\"; {d:.4f} is "
                f"{d / (1.500 / 3.54):.2f} lines on the SAME line width that "
                f"sentence implies, 1.500/3.54 = 0.4237 (a literal 0.400 "
                f"would make its own 1.500 come to 3.75 lines, not 3.54). "
                f"This is NOT a strength claim and cannot "
                f"be -- there is no PETG strength figure anywhere in this "
                f"repository -- it is the part measuring thinner than it is "
                f"drawn to. Measured off the exported mesh by plane section, "
                f"so it is the artefact and not the parameters.")
        elif d < COWL_SECTION_BASELINE - WEB_BASELINE_TOL:
            fails.append(
                f"the cowl's thinnest wall got WORSE -- {d:.4f} at z {z:+.4f} "
                f"against the recorded baseline {COWL_SECTION_BASELINE:.3f} "
                f"({li} <-> {lj}). It is accepted at the baseline and not "
                f"below it. If the loss is deliberate, re-export and "
                f"update COWL_SECTION_BASELINE to {d:.3f} in this file -- but "
                f"NOT below "
                f"{COWL_WEB_FLOOR + 2 * (COWL_SECTION_BASELINE - COWL_WEB_FLOOR):.3f} "
                f"without first checking that the governing feature is a "
                f"PRISM: {d:.3f} is the worst of three samples per band, and "
                f"a taper steeper than twice the baseline's own headroom over "
                f"the floor can still be walked under the floor through this "
                f"door, one accepted re-record at a time.")
        elif d > COWL_SECTION_BASELINE + WEB_BASELINE_TOL:
            lines.append(f"      baseline is STALE and in the good direction: "
                         f"{d:.4f} vs {COWL_SECTION_BASELINE:.3f} -- update "
                         f"COWL_SECTION_BASELINE to {d:.3f}")
    for k in sorted(hits):
        rec = hits[k]
        area, nm, dia, z, bb = rec["worst"]
        parts = ", ".join(sorted(rec["parts"]))
        fails.append(
            f"THE COWL'S RIM LANDS ON THE BOUGHT METAL AT BACK SCREW {k} "
            f"({rec['px']:+.3f},{rec['py']:+.3f}). Worst overlap "
            f"{area:.4f} mm2 in plan, against the O{dia:.3f} {nm} at "
            f"z {z:.4f}, over x {bb[0]:+.3f}..{bb[2]:+.3f} "
            f"y {bb[1]:+.3f}..{bb[3]:+.3f}; it fouls {parts} across "
            f"{len(rec['bands'])} of the sectioned bands. That metal is on the panel's "
            f"UNDERSIDE, which is the same face this rim lands on, so the "
            f"joint cannot close: the cowl would stand off the panel on a "
            f"washer and the four cowl screws would pull against it. "
            f"parameters.scad 6b records that nothing draws a bought washer "
            f"and that no part file reads it, which is why only a check can "
            f"see this -- a boundary notch moves neither the plan, the genus "
            f"nor the height, so ALL FIVE of export.py's cowl rules pass on "
            f"the collision unchanged.")
    if worst_clear:
        d, z, k, px, py, nm, dia = worst_clear
        lines.append(f"      tightest approach to the panel's bought metal "
                     f"{d:.4f} at z {z:+.4f} -- the O{dia:.3f} {nm} at back "
                     f"screw {k} ({px:+.3f},{py:+.3f})"
                     f"{', among those that CLEAR it' if hits else ''}. This "
                     f"is the first number to go negative if cowl_flange(), "
                     f"cowl_xy() or cowl_washer_relief_d() moves, and it is "
                     f"GATED against {COWL_METAL_CLEAR_BASELINE:.3f} -- it "
                     f"used to be printed and nothing more, so the whole of "
                     f"it could be spent before the overlap rule above fired.")
        if d < COWL_METAL_CLEAR_BASELINE - WEB_BASELINE_TOL:
            fails.append(
                f"THE COWL'S RIM CAME CLOSER TO THE PANEL'S BOUGHT METAL -- "
                f"{d:.4f} at z {z:+.4f} against the recorded "
                f"{COWL_METAL_CLEAR_BASELINE:.3f}, at the O{dia:.3f} {nm} on "
                f"back screw {k} ({px:+.3f},{py:+.3f}). This is not yet a "
                f"collision and that is the point: the rule above fires only "
                f"once the rim and the metal OVERLAP, so without this gate "
                f"the clearance can be spent to nil in silence and the first "
                f"report of it is a deck that will not close. The sourced "
                f"tolerances that eat this band -- FDM bore +/-0.250 "
                f"industrial, DIN 125 d1 up to 2.84 -- are bigger than what "
                f"is left. If the loss is deliberate, re-export and update "
                f"COWL_METAL_CLEAR_BASELINE to {d:.3f} in this file.")
        elif d > COWL_METAL_CLEAR_BASELINE + WEB_BASELINE_TOL:
            lines.append(f"      metal-clearance baseline is STALE and in the "
                         f"good direction: {d:.4f} vs "
                         f"{COWL_METAL_CLEAR_BASELINE:.3f} -- update "
                         f"COWL_METAL_CLEAR_BASELINE to {d:.3f}")


def check_meshes(env, export, names, fails, lines):
    import trimesh
    tab = {r[0]: r for r in MESHES}
    for name in names:
        if name not in tab:
            continue
        _, zname, pw, pd, vol, euler = tab[name]
        stl = os.path.join(export, "stl", f"{name}.stl")
        if not os.path.exists(stl):
            fails.append(f"missing {stl} -- run the export first")
            continue
        m = trimesh.load(stl)
        if len(m.faces) == 0:
            fails.append(f"{name}.stl is empty")
            continue
        nb = mesh_bodies(m)
        ext, b = m.extents, m.bounds
        lines.append(f"  {name}.stl: {len(m.faces)} facets, {nb} body, "
                     f"{ext[0]:.3f} x {ext[1]:.3f} x {ext[2]:.3f}, "
                     f"z {b[0][2]:+.3f}..{b[1][2]:+.3f}, "
                     f"vol {m.volume:.4f}, euler {m.euler_number}, "
                     f"watertight={m.is_watertight}")
        if nb != 1:
            fails.append(
                f"{name}.stl is {nb} SEPARATE BODIES. Something in it is "
                f"floating in mid-air: a boss whose bottom a pocket removed, "
                f"or an island a difference() cut free. This is how "
                f"kbd_socket_cut() was caught deleting the bottom of all "
                f"eleven plate bosses -- the picture was identical. "
                f"carrier.scad, above kbd_island_d")
        if not m.is_watertight:
            fails.append(f"{name}.stl is not watertight -- a slicer will "
                         f"guess at what you meant")
        if not m.is_volume:
            fails.append(f"{name}.stl is not a valid volume")
        if m.euler_number != euler:
            fails.append(
                f"{name}.stl has Euler characteristic {m.euler_number}, "
                f"recorded {euler}. V - E + F counts the part's HOLES and "
                f"nothing else: a through-hole that closed, or a new one, "
                f"moves it by 2 and moves nothing in the bounding box.")
        if abs(m.volume - vol) > 0.05:
            fails.append(f"{name}.stl displaces {m.volume:.4f} mm3 against the "
                         f"recorded {vol:.4f}, out by {m.volume-vol:+.4f}")
        for i, (nom, what) in enumerate(((pw, "width"), (pd, "depth"))):
            if abs(ext[i] - nom) > TOL:
                fails.append(f"{name}.stl is {ext[i]:.3f} in {what}, "
                             f"recorded {nom:.3f}")
        if zname:
            if abs(ext[2] - env[zname]) > TOL:
                fails.append(f"{name}.stl is {ext[2]:.3f} thick, want "
                             f"{zname} = {env[zname]:.3f}")
        z0, z1name, note = MESH_Z.get(name, (None, None, None))
        if z0 is not None and abs(b[0][2] - z0) > TOL:
            fails.append(
                f"{name}.stl starts at z = {b[0][2]:+.4f} and is drawn to "
                f"start at {z0:+.3f}. It is placed by a pure translate with no "
                f"rotation, so a Z origin that moves puts the part through the "
                f"face it is supposed to sit on and nothing says so.")
        if z1name and abs(b[1][2] - (z0 or 0.0) - env[z1name]) > TOL:
            fails.append(f"{name}.stl ends at z = {b[1][2]:+.4f}, want "
                         f"{z1name} = {env[z1name]:.3f} above {z0:+.3f}")
        if note:
            lines.append(f"      {note}")
        if name == "knob_encoder" or name == "knob_pot":
            groups = _plane_groups(m)
            key = next((k for k in groups if k[1] == "DOWN"
                        and abs(k[0] - env["acrylic_t"]) <= PLANE_Z_TOL), None)
            if key is None:
                fails.append(
                    f"{name}.stl has no DOWN-facing plane at z = acrylic_t = "
                    f"{env['acrylic_t']:.3f}. That plane IS the brim's "
                    f"shoulder and it is what makes knob_h - knob_proud == "
                    f"acrylic_t true of the part rather than of the datums; "
                    f"without it the knob has no seat and the only thing "
                    f"locating it in Z is the shaft.")
            else:
                lines.append(
                    f"      brim shoulder at z = {key[0]:+.4f} = acrylic_t, "
                    f"{sum(float(m.area_faces[i]) for i in groups[key]):.4f} "
                    f"mm2 bearing on the acrylic's top face")
        if name == "carrier":
            groups = _plane_groups(m)
            check_carrier_planes(m, env, groups, fails, lines)
            check_carrier_inserts(m, env, groups, fails, lines)
            check_carrier_nut_web(env, export, fails, lines)


# ===========================================================================
#  CHECK 6 -- keyboard_blank.scad, WHICH SHIPS NO ARTEFACT
#
#  It has had zero check coverage and its switch-field placement has now been
#  corrected twice by hand. The obstacle is real: it is NOT in export.py's
#  PARTS table, so there is no file on disk to measure. So this exports two of
#  its three parts to a scratch directory and measures those.
#
#  WHAT MAKES THIS WORTH THE OPENSCAD CALL. keyboard_blank.scad has no
#  `include` at all -- every number in it is its own literal. So it and
#  parameters.scad describe the same keyboard with two independent sets of
#  numbers and nothing has ever compared them. Every gate below is
#  parameters.scad's datum against keyboard_blank's exported mesh.
#
#  THE CONTRADICTION THIS WAS WARNED ABOUT IS RESOLVED, and the resolution is
#  what is gated. The question was whether the 1.000/1.250 concentric bezel
#  could stand beside parameters.scad's statement that the field's centre is
#  0.250 off the frame's. It cannot, and parameters.scad has settled it:
#  kbd_cap_field_d = [0.0625, -0.250] in DECK axes, and kbd_dy()'s d/dmz = -1
#  puts the 0.250 FRONTWARD. keyboard_blank.scad now carries bezel_front =
#  1.000 and bezel_rear = 1.500. In X there is no offset to apply: the +0.0625
#  is the CAP field's, set by the wide caps' outer edges, and the switch
#  LATTICE is concentric. Both halves are gated separately below so neither can
#  be "fixed" into the other.
# ===========================================================================
MESH_XY_TOL = 0.001        # mm. float32 carries 1.7e-05 mm of error at
                           #   |x| ~ 287, and these parts are 12-facet wedges
                           #   whose coordinates OpenSCAD computes exactly, so
                           #   this is 58x the only error there is.  [DERIVED]
KB_NORMAL_TOL = 1e-5       # the cap plane against the frame's top plane,
                           #   componentwise on the unit normal. The two are
                           #   parallel BY CONSTRUCTION (the band's two heights
                           #   are the same plane sampled at its own edges), so
                           #   the measurement reads 5.2e-08 and this is 190x
                           #   that -- tight enough that a band built from two
                           #   independently typed heights would fail. [DESIGN]
KB_SHELL_H = (7.600, 13.300)   # [REPO keyboard_blank.scad:55-56] the frame's
                               #   front and rear heights. parameters.scad
                               #   declares no frame height at all, so there is
                               #   no datum to read and this is the part file's
                               #   own [vendor] literal, recorded as a baseline.


def _up_plane(m):
    """The plane of a mesh's largest up-facing face: (normal, point)."""
    i = max((i for i in range(len(m.faces)) if m.face_normals[i][2] > 1e-9),
            key=lambda i: m.area_faces[i])
    return m.face_normals[i], m.vertices[m.faces[i][0]]


def _plane_z(plane, x, y):
    n, p = plane
    return float(p[2] - (n[0]*(x - p[0]) + n[1]*(y - p[1])) / n[2])


#  ---------------------------------------------------------------------------
#  THE KEYCAP ENVELOPE, AND WHY NOTHING COULD FIRE ON IT
#
#  cad/parts/k9pro_internals.scad is a REFERENCE file: no part includes it and
#  until this check nothing in v2/cad or v2/tools read it. So its cap table
#  could be broken to a flat 13.2753, and then to 113.2753 -- every cap 106.575
#  tall, straight through the acrylic, the cowl and the lid -- and OpenSCAD
#  exported it at exit 0 with "Simple: yes", export.py's FATAL list saw
#  nothing, check_render returned 28 passed / 0 FAILED because it sha-checks
#  only provenance == EXPORTED bodies and the keycaps are REFERENCE, and
#  check_cad never opened the file. Three green checkers on a 106 mm keycap.
#
#  WHAT THIS MEASURES. The render's own exported keycap body, against this
#  file's datums -- which is check_cad's normal direction and the one thing
#  check_render cannot do, because it is written never to read parameters.scad
#  (its own header says so). The frame offset is not typed either: it comes off
#  ref_kbd_frame.stl's top face, so the comparison is mesh-to-datum and
#  mesh-to-mesh and nothing in it is a datum against itself.
#
#  IT SKIPS WHEN THE RENDER HAS NOT RUN, and says so rather than passing
#  quietly: v2/export/render/.bodies is a render output, not a build product of
#  export.py, so a bare checkout does not have it.
def check_keycap_envelope(env, export, fails, lines):
    """The exported keycap body against kbd_cap_field and kbd_cap_h."""
    import trimesh

    bodies = os.path.join(export, "render", ".bodies")
    caps = os.path.join(bodies, "ref_kbd_keycaps.stl")
    frame = os.path.join(bodies, "ref_kbd_frame.stl")
    if not (os.path.exists(caps) and os.path.exists(frame)):
        lines.append(f"    keycap envelope -- SKIPPED, no render bodies at "
                     f"{bodies} (run v2/tools/render/render.py); nothing "
                     f"here was measured")
        return
    mc = trimesh.load(caps, force="mesh")
    mf = trimesh.load(frame, force="mesh")
    clo, chi = mc.bounds
    plate = float(mf.bounds[1][2])
    lines.append(f"    keycap envelope -- ref_kbd_keycaps.stl, "
                 f"{len(mc.faces)} facets, against ref_kbd_frame.stl's top "
                 f"face at z {plate:+.4f}")

    #  -- the plan, against the MEASURED cap field -------------------------
    want = env.get("kbd_cap_field")
    if want:
        for i, ax in enumerate("xy"):
            got = float(chi[i] - clo[i])
            lines.append(f"      {ax} span {got:.4f}, kbd_cap_field "
                         f"{want[i]:.3f}")
            if abs(got - want[i]) > MESH_XY_TOL:
                fails.append(
                    f"THE EXPORTED KEYCAP FIELD IS {got:.4f} IN {ax.upper()} "
                    f"AND kbd_cap_field SAYS {want[i]:.3f}, out by "
                    f"{got - want[i]:+.4f}. kbd_cap_field is [CAD-MESH] -- a "
                    f"measurement of all 61 caps in the vendor assembly -- so "
                    f"the two are independent and this is the envelope "
                    f"disagreeing with the thing it is an envelope of. The "
                    f"width formula reached exactly this way before: it gave "
                    f"every cap u*pitch - cap_gap, which is right for 59 of "
                    f"the 61 and under-states enter by 0.100 and the spacebar "
                    f"by 0.050, because those two take a different inset -- "
                    f"and enter is the rightmost key of its row, so half its "
                    f"error landed on the field's +x edge.")

    #  -- the top, against kbd_cap_h, through the frame's own face ---------
    caph = env.get("kbd_cap_h")
    if caph is not None:
        got = float(chi[2]) - plate
        lines.append(f"      tallest cap stands {got:.4f} above that face, "
                     f"kbd_cap_h {caph:.4f}")
        if abs(got - caph) > MESH_XY_TOL:
            fails.append(
                f"THE EXPORTED KEYCAPS STAND {got:.4f} ABOVE THE FRAME'S TOP "
                f"FACE AND kbd_cap_h SAYS {caph:.4f}, out by {got - caph:+.4f}"
                f". kbd_cap_h sets kbd_cap_top_z and therefore kbd_recess, so "
                f"this is how deep the keyboard sits in the carrier. The "
                f"offset is NOT typed here -- it is ref_kbd_frame.stl's own "
                f"top face at z {plate:+.4f} -- so a disagreement is between "
                f"two files and not between a number and itself. Until this "
                f"check existed, k9pro_internals.scad's cap table could be "
                f"set to any value at all and every checker stayed green.")

    #  -- and the envelope must ENCLOSE the vendor cap it stands for -------
    #  Artefact against artefact, no datum in it at all: the vendor's own
    #  single 1u cap. An envelope shorter than the thing it encloses is the
    #  defect a flat 13.2753 was rejected for, and the table that replaced it
    #  was still 0.0006..0.0087 short at every width because it took
    #  tessellation maxima off a dished top.
    one = os.path.join(ROOT, "parts", "models", "k9pro-internals",
                       "k9pro-keycap-1u-one.stl")
    if os.path.exists(one):
        m1 = trimesh.load(one, force="mesh")
        l1, h1 = m1.bounds
        #  the vendor cap's own axes: y is up, the plan is x by z
        vh = float(h1[1] - l1[1])
        eh = float(chi[2] - clo[2])
        lines.append(f"      vendor 1u cap is {vh:.4f} tall "
                     f"(k9pro-keycap-1u-one.stl, its own y span); the "
                     f"envelope's z span is {eh:.4f}")
        if eh < vh - TOL:
            fails.append(
                f"THE KEYCAP ENVELOPE IS {eh:.4f} TALL AND THE VENDOR'S OWN "
                f"1u CAP IS {vh:.4f}: it does not enclose. Both figures are "
                f"meshes and neither is a datum.")
        base = float(clo[2]) - plate
        vbase = float(l1[1])
        if abs(base - vbase) > TOL:
            fails.append(
                f"THE KEYCAP ENVELOPE'S UNDERSIDE SITS {base:.4f} ABOVE THE "
                f"FRAME'S TOP FACE AND THE VENDOR 1u CAP'S OWN BOTTOM IS AT "
                f"{vbase:.4f} IN ITS FRAME, out by {base - vbase:+.4f}. Both "
                f"are meshes. All 61 cap bodies in the vendor assembly share "
                f"this bottom, so it is one plane and not an average.")
    else:
        lines.append("      vendor 1u cap STL absent, so the "
                     "artefact-to-artefact enclosure test was NOT run")


def check_keyboard_blank(env, fails, lines):
    import shutil
    import subprocess
    import tempfile
    import trimesh
    scad = os.path.join(ROOT, "cad", "keyboard_blank.scad")
    if not os.path.exists(scad):
        fails.append(f"missing {scad}")
        return
    if shutil.which("openscad") is None:
        fails.append("keyboard_blank.scad is not in export.py's PARTS table, "
                     "so it has to be exported here to be measured, and "
                     "openscad is not on PATH. The check is BLIND, not "
                     "passing.")
        return
    tmp = tempfile.mkdtemp(prefix="kbblank.")
    try:
        mesh = {}
        for p in ("shell", "keycaps"):
            dest = os.path.join(tmp, f"{p}.stl")
            #  The include trap does not bite here and that is worth saying:
            #  OpenSCAD resolves `include` against the MAIN FILE's directory,
            #  and keyboard_blank.scad has no include at all. The main file
            #  stays where it lives; only the output goes to the scratch dir.
            r = subprocess.run(
                ["openscad", "-D", f'part="{p}"', "--export-format=binstl",
                 "-o", dest, scad],
                capture_output=True, text=True)
            bad = [ln for ln in r.stderr.splitlines()
                   if "unknown variable" in ln or "undefined operation" in ln
                   or "ERROR" in ln or "Assertion" in ln]
            if bad or not os.path.exists(dest) or os.path.getsize(dest) == 0:
                fails.append(f"keyboard_blank.scad part=\"{p}\" did not "
                             f"export: {(bad or [r.stderr.strip()[-200:]])[0]}")
                return
            mesh[p] = trimesh.load(dest)
        sh, kc = mesh["shell"], mesh["keycaps"]
        sb, kb = sh.bounds, kc.bounds
        fw, fd = env["kbd_frame"]
        sw, sd = env["kbd_switch_field"]
        lines.append(f"  keyboard_blank.scad (exported to measure it; it is "
                     f"not in export.py's PARTS table)")
        lines.append(f"    shell   {len(sh.faces)} facets, "
                     f"x {sb[0][0]:.4f}..{sb[1][0]:.4f}  "
                     f"y {sb[0][1]:.4f}..{sb[1][1]:.4f}  "
                     f"z {sb[0][2]:.4f}..{sb[1][2]:.4f}, "
                     f"watertight={sh.is_watertight}")
        lines.append(f"    keycaps {len(kc.faces)} facets, "
                     f"x {kb[0][0]:.4f}..{kb[1][0]:.4f}  "
                     f"y {kb[0][1]:.4f}..{kb[1][1]:.4f}  "
                     f"z {kb[0][2]:.4f}..{kb[1][2]:.4f}, "
                     f"watertight={kc.is_watertight}")
        for nm, m in (("shell", sh), ("keycaps", kc)):
            if not m.is_watertight or mesh_bodies(m) != 1:
                fails.append(f"keyboard_blank {nm}: watertight="
                             f"{m.is_watertight}, {mesh_bodies(m)} bodies")

        # -- the shell IS the frame, in parameters.scad's own numbers --------
        for i, (lo, hi, nom, ax) in enumerate(
                ((sb[0][0], sb[1][0], fw, "width"),
                 (sb[0][1], sb[1][1], fd, "depth"))):
            if abs(lo) > MESH_XY_TOL or abs(hi - nom) > MESH_XY_TOL:
                fails.append(
                    f"keyboard_blank shell: {ax} runs {lo:.4f}..{hi:.4f}, want "
                    f"0..{nom:.3f} -- parameters.scad's kbd_frame. The two "
                    f"files share no numbers, so this is the only thing that "
                    f"makes them the same keyboard.")
        for lo, hi, nom in ((sb[0][2], sb[1][2], KB_SHELL_H[1]),):
            if abs(lo) > MESH_XY_TOL or abs(hi - nom) > MESH_XY_TOL:
                fails.append(f"keyboard_blank shell: z runs {lo:.4f}..{hi:.4f},"
                             f" recorded 0..{nom:.3f} "
                             f"[REPO keyboard_blank.scad:55-56]")

        # -- X IS CONCENTRIC. kbd_cap_field_d[0] DOES NOT APPLY HERE ---------
        bx0, bx1 = kb[0][0] - sb[0][0], sb[1][0] - kb[1][0]
        want_bx = (fw - sw) / 2
        lines.append(f"    bezels  x {bx0:.4f} / {bx1:.4f} (concentric, want "
                     f"{want_bx:.4f} both)   "
                     f"depth front {kb[0][1]-sb[0][1]:.4f} rear "
                     f"{sb[1][1]-kb[1][1]:.4f} (UNEQUAL by design)")
        if abs(bx0 - bx1) > MESH_XY_TOL:
            fails.append(
                f"keyboard_blank: the switch field is NOT concentric in X -- "
                f"{bx0:.4f} at -x against {bx1:.4f} at +x. The cap field's "
                f"kbd_cap_field_d[0] = {env['kbd_cap_field_d'][0]:+.4f} is the "
                f"offset of the CAPS, set by the wide keys' outer edges, and "
                f"it does not apply to the switch LATTICE. parameters.scad's "
                f"kbd_cap_field block.")
        for nm, v in (("-x", bx0), ("+x", bx1)):
            if abs(v - want_bx) > MESH_XY_TOL:
                fails.append(f"keyboard_blank: the {nm} bezel is {v:.4f}, want "
                             f"(kbd_frame[0] - kbd_switch_field[0])/2 = "
                             f"{want_bx:.4f}")

        # -- DEPTH IS NOT CONCENTRIC, and the asymmetry is the whole point ---
        #  The switch lattice's model Z centre is 57.000, which is
        #  kbd_frame_cz - kbd_cap_field_d[1]: the sign flip is kbd_dy()'s
        #  d/dmz = -1, so -0.250 in deck y is +0.250 in model Z.
        #  keyboard-strip.md 1.6's row axes 19/38/57/76/95 give the same 57.000
        #  independently, which is parameters.scad's own corroboration.
        mzc = env["kbd_frame_cz"] - env["kbd_cap_field_d"][1]
        oy = env["kbd_frame_cz"] + fd / 2
        want_front = oy - (mzc + sd / 2)
        want_rear = fd - sd - want_front
        gf, gr = kb[0][1] - sb[0][1], sb[1][1] - kb[1][1]
        if abs(gf - gr) <= MESH_XY_TOL:
            fails.append(
                f"keyboard_blank: the depth bezels are EQUAL at {gf:.4f} -- a "
                f"concentric switch field. parameters.scad's kbd_cap_field_d "
                f"is {env['kbd_cap_field_d'][1]:+.4f} in deck y and kbd_dy() "
                f"has d/dmz = -1, so the field sits "
                f"{abs(want_front-want_rear)/2:.4f} FRONTWARD of the frame's "
                f"centre and the two bezels are {want_front:.4f} front and "
                f"{want_rear:.4f} rear.")
        for nm, got, nom in (("front", gf, want_front),
                             ("rear", gr, want_rear)):
            if abs(got - nom) > MESH_XY_TOL:
                fails.append(
                    f"keyboard_blank: the {nm} depth bezel is {got:.4f}, want "
                    f"{nom:.4f} from parameters.scad (switch lattice centred "
                    f"on model Z {mzc:.3f} = kbd_frame_cz - "
                    f"kbd_cap_field_d[1], in a {fd:.3f} frame)")
        for i, (nom, ax) in enumerate(((sw, "width"), (sd, "depth"))):
            got = kb[1][i] - kb[0][i]
            if abs(got - nom) > MESH_XY_TOL:
                fails.append(f"keyboard_blank: the switch band's {ax} is "
                             f"{got:.4f}, want kbd_switch_field = {nom:.3f}")

        # -- THE BAND MUST ENCLOSE THE MEASURED CAP FIELD --------------------
        #  The strongest gate here, and it is a pure cross-file one: the box is
        #  built from kbd_frame, kbd_frame_cx/cz, kbd_cap_field and
        #  kbd_cap_field_d, mapped into the blank's own axes, and compared with
        #  the band this file exported. A band SMALLER than the caps it stands
        #  for is a worst-case envelope that is not one.
        ox = env["kbd_frame_cx"] - fw / 2
        cw, cd = env["kbd_cap_field"]
        mxc = env["kbd_frame_cx"] + env["kbd_cap_field_d"][0]
        cap = ((mxc - cw/2) - ox, (mxc + cw/2) - ox,
               oy - (mzc + cd/2), oy - (mzc - cd/2))
        marg = (cap[0] - kb[0][0], kb[1][0] - cap[1],
                cap[2] - kb[0][1], kb[1][1] - cap[3])
        lines.append(f"    encloses the MEASURED cap field: blank x "
                     f"{cap[0]:.4f}..{cap[1]:.4f}  y {cap[2]:.4f}..{cap[3]:.4f}"
                     f"  margins -x {marg[0]:+.4f} +x {marg[1]:+.4f} front "
                     f"{marg[2]:+.4f} rear {marg[3]:+.4f}")
        for nm, v in zip(("-x", "+x", "front", "rear"), marg):
            if v < -MESH_XY_TOL:
                fails.append(
                    f"keyboard_blank: the {sw:.3f} x {sd:.3f} switch band does "
                    f"NOT enclose the measured keycap field at {nm} -- it is "
                    f"inside it by {-v:.4f}. kbd_cap_field is 61 real caps "
                    f"measured off the vendor mesh (parameters.scad), mapped "
                    f"here by blank_x = mx {-ox:+.3f}, blank_y = {oy:.3f} - mz."
                    f" An envelope smaller than the thing it stands for is not "
                    f"an envelope.")
        if min(marg) < 0.100:
            tight = min(zip(marg, ("-x", "+x", "front", "rear")))
            lines.append(f"      tightest margin {tight[0]:.4f} at "
                         f"{tight[1]} -- parameters.scad declares the cap "
                         f"field's X envelope NOT SOURCED in its last 0.100")

        # -- THE CAP PLANE IS PARALLEL TO THE FRAME'S TOP --------------------
        ps, pk = _up_plane(sh), _up_plane(kc)
        dn = max(abs(float(a) - float(b)) for a, b in zip(ps[0], pk[0]))
        xm = (kb[0][0] + kb[1][0]) / 2
        rise = [_plane_z(pk, xm, y) - _plane_z(ps, xm, y)
                for y in (kb[0][1], kb[1][1])]
        lines.append(f"    cap plane || frame top to {dn:.2e} on the unit "
                     f"normal; cap top stands {rise[0]:.4f} / {rise[1]:.4f} "
                     f"above the frame's at the band's own two edges "
                     f"(kbd_cap_h = {env['kbd_cap_h']:.3f})")
        if dn > KB_NORMAL_TOL:
            fails.append(
                f"keyboard_blank: the cap band's top plane is NOT parallel to "
                f"the frame's -- the unit normals differ by {dn:.3e} against a "
                f"{KB_NORMAL_TOL:.0e} budget. The band's two heights are meant "
                f"to be the SAME plane sampled at the band's own front and "
                f"rear edges; two independently typed heights give exactly "
                f"this failure. keyboard_blank.scad:191-192")
        for y, v in zip((kb[0][1], kb[1][1]), rise):
            if abs(v - env["kbd_cap_h"]) > MESH_XY_TOL:
                fails.append(
                    f"keyboard_blank: at the band's y = {y:.3f} the cap top "
                    f"stands {v:.4f} above the frame's top face, and "
                    f"parameters.scad's kbd_cap_h is "
                    f"{env['kbd_cap_h']:.3f}. These are two files' independent "
                    f"literals for one measurement (keyboard_blank's "
                    f"kc_above_frame against kbd_cap_h) and nothing else "
                    f"compares them.")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--params", default=None,
                    help="parameters.scad to read the datums from. Point it "
                         "at a deliberately broken copy to prove a check "
                         "fires.")
    ap.add_argument("--export", default=None,
                    help="export/ directory to measure. Point it at a scratch "
                         "tree to prove a check fires.")
    ap.add_argument("--only", action="append", default=None,
                    help="limit to one part (slab, backpanel). Repeatable.")
    args = ap.parse_args(argv)
    export = args.export or os.path.join(ROOT, "export")

    env, prov = datums(args.params)
    fails, lines = [], []
    parts = args.only or ["slab", "backpanel"]

    for part in parts:
        check_part(part, env, export, fails, lines)

    # ---------------- the meshes ------------------------------------------
    check_meshes(env, export, args.only or [r[0] for r in MESHES],
                 fails, lines)
    if args.only is None or {"foot", "cowl"} & set(args.only):
        check_foot_cowl_plan(env, export, fails, lines)
    if args.only is None or "cowl" in (args.only or []):
        check_section_polys_clearance(env, export, fails, lines)
    if args.only is None or "keyboard_blank" in args.only:
        check_keyboard_blank(env, fails, lines)
        check_keycap_envelope(env, export, fails, lines)

    # ---------------- provisional datums ----------------------------------
    prov_list = sorted(k for k, v in prov.items() if v == "PROVISIONAL")

    src = args.params or os.path.join(ROOT, "cad", "parameters.scad")
    for n in lines:
        print(n if n.startswith(" ") else f"  {n}")
    print(f"\n  $fn in the drawing = {env.get('$fn', '?')}; {len(env)} datums "
          f"parsed from {os.path.relpath(src, REPO)}")
    if prov_list:
        print(f"\n  {len(prov_list)} PROVISIONAL datum(s) -- caliper before a "
              f"final cut:")
        for k in prov_list:
            print(f"    {k} = {env.get(k, '?')}")
    #  DATUMS THIS FILE CANNOT SEE, named rather than silently absent.
    #  datums() evaluates scalars, list literals and indexing; an assignment
    #  whose right-hand side calls an OpenSCAD function does not evaluate, so
    #  the name never enters `env` -- and env.get() then hands back None, which
    #  reads exactly like a datum that is there and happens to be zero. The
    #  provenance tag is still picked up, so the two sets differ and the
    #  difference is printable. foot_h_min is the live example: its RHS is
    #  cowl_h() + 1.270.
    unseen = sorted(k for k in prov if k not in env)
    if unseen:
        print(f"\n  {len(unseen)} tagged datum(s) that datums() could NOT "
              f"evaluate, so nothing here checks them -- the right-hand side "
              f"calls a function:")
        for k in unseen:
            print(f"    {k}  [{prov[k]}]")
    print()
    for f in fails:
        print(f"  FAIL  {f}")
    print(f"  {'PASS' if not fails else str(len(fails)) + ' PROBLEM(S)'}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
