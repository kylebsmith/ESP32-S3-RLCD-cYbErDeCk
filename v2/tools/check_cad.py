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
WEB_BASELINE_TOL = 0.010    # mm. Baseline slack: the DXF's own resolution.

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
    "backpanel":    2.360,  # [MEASURED 2026-11-07, v2/export/dxf/backpanel.dxf]
                            # Was 4.320. The loss is deliberate and it is the
                            # cowl's four screws, which stand 2.360 off the
                            # 18650 window by design (cowl_flange = 7.620 was
                            # raised from 6.350 for exactly this). It is above
                            # the 2.000 hard floor. The first attempt put them
                            # on the window's CORNERS, which landed them 5.306
                            # from the display's own screw-head clearance holes
                            # -- a 1.104 web -- and this check is what caught
                            # it.
                            #   outline <-> the -x middle fastener SLOT
}

PART_LAYERS = ("CUT_OUTER", "CUT_INNER")    # dxf_post.py's own convention; the
                                            # FIDUCIAL is a token in the waste
                                            # and is not part geometry.


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
    """
    import ezdxf
    doc = ezdxf.readfile(path)
    loops = []
    for e in doc.modelspace():
        if e.dxf.layer not in PART_LAYERS:
            continue
        if e.dxftype() == "LWPOLYLINE":
            pts = [(p[0], p[1]) for p in e.get_points()]
            if e.closed and len(pts) > 2:
                loops.append(pts)
        elif e.dxftype() == "POLYLINE":
            pts = [(v.dxf.location.x, v.dxf.location.y) for v in e.vertices]
            if len(pts) > 2:
                loops.append(pts)
    return loops or dxf_loops(path)


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

    def _rrect(self):
        """An axis-aligned rounded rectangle, with its corner radius FITTED.

        The outline and the keyboard void. The radius is not read off a datum:
        it is recovered from each of the four corner arcs independently, and
        the four are required to agree -- which is how a void whose corners
        were drawn at the wrong radius gets caught.
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
    cl_x, cl_y = (W - cw)/2, (H - ch)/2
    lines.append(f"      frame {fw:.3f} x {fh:.3f}   "
                 f"keycap field {cw:.3f} x {ch:.3f}")
    lines.append(f"      slab-over-frame overlap   X {ov_x:+.4f}  "
                 f"Y {ov_y:+.4f}   per side")
    lines.append(f"      void-over-keycap clearance X {cl_x:+.4f}  "
                 f"Y {cl_y:+.4f}   per side")

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
    #  the answer: the inscribed 96-gon sits inside the nominal R3.175 and the
    #  corner is 0.0013 tighter than the nominal arithmetic says.
    #  parameters.scad 8a states the keycap's own corner radius is NOT SOURCED
    #  and asks this file to report the square-cap figure "so the assumption
    #  cannot hide". It is reported and it is NOT gated -- a perfectly square
    #  keycap is a fiction, and gating on a fiction fails a good part.
    best = None
    for sx in (1, -1):
        for sy in (1, -1):
            p = Point(void.bx + sx*cw/2, void.by + sy*ch/2)
            d = p.distance(void.lr)
            sd = d if void.poly.contains(p) else -d
            if best is None or sd < best[0]:
                best = (sd, sx, sy)
    rc = _min_cap_radius(void, cw, ch)
    lines.append(f"      CORNER clearance -- a perfectly SQUARE keycap whose "
                 f"outer corner sits on the keycap field's corner: "
                 f"{best[0]:+.4f} (corner {best[1]:+d},{best[2]:+d}), against "
                 f"the straight edges' {min(cl_x, cl_y):+.4f}")
    if rc is None:
        lines.append("        NO cap corner radius clears this void")
    else:
        lines.append(f"        a square cap is a fiction and the keycap's own "
                     f"corner radius is NOT SOURCED; the smallest cap corner "
                     f"radius that clears this void is {rc:.3f}. Reported, "
                     f"not gated -- parameters.scad 8a asks for exactly this "
                     f"figure \"so the assumption cannot hide\".")
    #  parameters.scad 8a quotes +0.139 here and solves the radius window as
    #  r <= 3.512. Both come from a Y gap of 1.1925, which is 0.9425 + 0.250:
    #  the keycap field offset by the switch field's 0.250 and taken on the
    #  LOOSE side. With the gap the datums actually give -- kbd_cap_clear's own
    #  0.9425, the cap field concentric with the void -- the window closes at
    #  r <= 3.102 and R3.175 is 0.030 outside it. Measured against the DXF's
    #  own 96-gon arc it is 0.0316, because the inscribed arc sits inside the
    #  nominal one; that extra 0.0013 is check 1's whole point, arriving here.
    #  Either way the sign is negative and the figure in 8a is positive.
    if abs(best[0] - 0.139) > 0.010:
        lines.append(
            f"        NOTE: parameters.scad 8a quotes +0.139 for this and "
            f"solves the radius window as r <= 3.512. Both follow from a Y gap "
            f"of 1.1925 = {cl_y:.4f} + 0.250, i.e. the keycap field displaced "
            f"by the switch field's 0.250 and then read on the LOOSE corner. "
            f"At the gaps the datums give here the window closes at "
            f"r <= {(cl_x + cl_y) + math.sqrt(2*cl_x*cl_y):.3f} and "
            f"kbd_void_r = {env['kbd_void_r']:.3f} is outside it.")

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


def _min_cap_radius(void, cw, ch):
    """The smallest keycap corner radius that clears the measured void.

    Bisected against the artefact's own ring: for a cap corner radius rc the
    critical feature is the cap's corner arc, whose centre sits rc inboard of
    the field corner, and it clears when that centre is at least rc from the
    void's boundary.
    """
    from shapely.geometry import Point

    def clear(rc):
        worst = None
        for sx in (1, -1):
            for sy in (1, -1):
                p = Point(void.bx + sx*(cw/2 - rc), void.by + sy*(ch/2 - rc))
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
    rings = [Ring(l) for l in dxf_part_loops(dxf)]
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
        clear_d, slot_l = env["screw_m3_clear_back"], env["screw_slot_l_back"]
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

    fast = check_datum_and_slots(rings, pattern, int(env["screw_datum_i"]),
                                 clear_d, slot_l, fails, lines) or []
    for r in fast:
        taken.add(id(r))
    loose = [r for r in rings if id(r) not in taken]
    if loose:
        for r in loose:
            fails.append(f"{part}: cut contour at {r.cx:+.3f},{r.cy:+.3f} "
                         f"({r.kind}, {r.w:.3f} x {r.h:.3f}) is in the DXF and "
                         f"matches no declared feature")
    nb = sum(1 for r in rings if r.label.startswith("bore"))
    inv = f"1 outline + {len(fast)} fasteners"
    if part == "slab":
        inv += f" + 1 void + {nb} bores + 1 fader slot"
    lines.append(f"    inventory: {inv} = {len(rings)} contours, "
                 + ("complete" if not loose else f"{len(loose)} UNACCOUNTED"))

    check_faceting(rings, nominals, fails, lines)
    t = env["acrylic_t"] if part == "slab" else env["back_t"]
    check_webs(part, rings, t, fails, lines)
    check_edge_distance(rings, fast, env, fails, lines)
    if part == "slab" and void is not None:
        check_keyboard_branch(env, void, fails, lines)
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
    cx, cy = cowl_xy(env)
    w, h = cowl_window(env)
    f = 7.620 / 2                       # cowl_flange() / 2
    return [(cx + w / 2 + f, cy), (cx - w / 2 - f, cy),
            (cx, cy + h / 2 + f), (cx, cy - h / 2 - f)]


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
                 f"{found_screws}/4 cowl screws")


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
        lines.append(
            f"      ROEHM SHORTFALL on {len(ungated)} hole(s), reported not "
            f"gated. Worst: {w[0].label} stands {w[3]:.3f} from the nearest "
            f"cut against 1.5 x O{w[1]:.3f} = {w[2]:.3f}, short by "
            f"{w[2]-w[3]:.3f}. fabrication.md 1.4's applied block checks the "
            f"O{env['bore_d']:.3f} bore against the {env['pitch']:.3f} knob "
            f"PITCH only and never against the sheet edge or the keyboard "
            f"void, which are both nearer. Closing it means moving the "
            f"control band or the void -- parameters.scad's call, not this "
            f"file's.")


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
    import trimesh
    for name in parts:
        stl = os.path.join(export, "stl", f"{name}.stl")
        if not os.path.exists(stl):
            fails.append(f"missing {stl}")
            continue
        m = trimesh.load(stl)
        if len(m.faces) == 0:
            fails.append(f"{name}.stl is empty")
        if not m.is_watertight:
            fails.append(f"{name}.stl is not watertight -- a slicer will "
                         f"guess at what you meant")
        if not m.is_volume:
            fails.append(f"{name}.stl is not a valid volume")
        ext = m.extents
        want_t = {"slab": env["acrylic_t"], "backpanel": env["back_t"],
                  "carrier": env["carrier_h"]}.get(name)
        if want_t is not None and abs(ext[2] - want_t) > TOL:
            fails.append(f"{name}.stl is {ext[2]:.3f} thick, "
                         f"want {want_t:.3f}")
        lines.append(f"  {name}.stl: {len(m.faces)} facets, "
                     f"{ext[0]:.2f} x {ext[1]:.2f} x {ext[2]:.2f}, "
                     f"watertight={m.is_watertight}")

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
    print()
    for f in fails:
        print(f"  FAIL  {f}")
    print(f"  {'PASS' if not fails else str(len(fails)) + ' PROBLEM(S)'}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
