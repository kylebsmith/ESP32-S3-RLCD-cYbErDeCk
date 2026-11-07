#!/usr/bin/env python3
"""Break the exported vector files on purpose and watch the validation refuse.

    /usr/bin/python3.11 v2/tools/export.py slab backpanel   # first
    /usr/bin/python3.11 v2/tools/test_dxf_post.py

THE RULE THIS FILE EXISTS FOR
-----------------------------
A check you have not seen fail is not a check. `dxf_post.validate_dxf()` and
`validate_pair()` pass on the files in `v2/export`, and a validation that only
ever passes is indistinguishable from `return []`. So every one of them is
exercised here against a DELIBERATELY BROKEN COPY of the real artefact, and
the test fails if the rejection does not appear.

Nothing in `v2/export` is modified. Each case copies the validated pair into a
scratch directory, edits the copy -- mostly by raw-text surgery, which is how a
real corruption would arrive -- and validates that. The originals are therefore
restored by construction, and `case_00_the_real_files_pass` re-checks them at
the end of every run to prove it.

Two of the cases go end-to-end through `dxf_post.post_process()`, the function
`export.py` calls, so the loud-failure path is proved and not only the
predicate.
"""
import logging
import os
import re
import shutil
import sys
import tempfile

# Case 12 writes an R12 file on purpose, and ezdxf logs "Drawing units
# ($INSUNITS) are not exported for DXF R12" when it does -- which is exactly
# the defect under test, but on stderr it reads as a failure of the suite.
logging.getLogger("ezdxf").setLevel(logging.ERROR)

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import dxf_post as dp                                         # noqa: E402

ROOT = os.path.dirname(HERE)
DXF_DIR = os.path.join(ROOT, "export", "dxf")
SVG_DIR = os.path.join(ROOT, "export", "svg")
PART = "slab"

CASES = []


def case(label, want):
    """Register a case.

    `want` is text that must appear in one of the rejections the case returns.
    `want = None` marks a case that must produce NO rejection -- a validator
    that refuses everything is not a check either, so the suite carries both
    directions.
    """
    def deco(fn):
        CASES.append((label, want, fn))
        return fn
    return deco


# ---------------------------------------------------------------------------
#  scratch: a private copy of the validated pair
# ---------------------------------------------------------------------------
class Copy:
    def __init__(self, tmp):
        self.dir = tmp
        os.makedirs(os.path.join(tmp, "dxf", "raw"), exist_ok=True)
        os.makedirs(os.path.join(tmp, "svg"), exist_ok=True)
        self.dxf = os.path.join(tmp, "dxf", f"{PART}.dxf")
        self.raw = os.path.join(tmp, "dxf", "raw", f"{PART}.dxf")
        self.svg = os.path.join(tmp, "svg", f"{PART}.svg")
        shutil.copy(os.path.join(DXF_DIR, f"{PART}.dxf"), self.dxf)
        shutil.copy(os.path.join(DXF_DIR, "raw", f"{PART}.dxf"), self.raw)
        shutil.copy(os.path.join(SVG_DIR, f"{PART}.svg"), self.svg)

    def validate(self):
        fails, _, part_rings, fid = dp.validate_dxf(self.dxf, self.raw)
        fails += dp.validate_pair(self.svg, part_rings, fid)[0]
        return fails

    def write_svg(self, rings):
        """A synthetic SVG for `rings`, in SVG's own Y-DOWN frame."""
        d = "\n".join(
            " ".join(f"{'M' if i == 0 else 'L'} {x:.6f},{-y:.6f}"
                     for i, (x, y) in enumerate(r)) + " z" for r in rings)
        b = dp.bbox_union([dp.ring_bbox([(x, -y) for x, y in r])
                           for r in rings])
        w = int(b[2] - b[0]) + 4
        h = int(b[3] - b[1]) + 4
        open(self.svg, "w", encoding="utf-8").write(
            f'<svg width="{w}mm" height="{h}mm" '
            f'viewBox="{int(b[0])-2} {int(b[1])-2} {w} {h}" '
            f'xmlns="http://www.w3.org/2000/svg" version="1.1">\n'
            f'<path d="\n{d}\n" stroke="black" fill="none" '
            f'stroke-width="0.1"/>\n</svg>\n')
        return self.svg

    def edit(self, path, *subs):
        txt = open(path, encoding="utf-8").read()
        for old, new in subs:
            assert old in txt, f"the break pattern is not in {path}: {old!r}"
            txt = txt.replace(old, new, 1)
        open(path, "w", encoding="utf-8").write(txt)


# ---------------------------------------------------------------------------
#  1-2. units -- the blocking fix
# ---------------------------------------------------------------------------
@case("$INSUNITS changed 4 -> 1 (inches): the 1/25.4 bug itself",
      "$INSUNITS is 1, want 4")
def _(c):
    c.edit(c.dxf, ("$INSUNITS\n 70\n4\n", "$INSUNITS\n 70\n1\n"))
    return c.validate()


@case("$MEASUREMENT changed 1 -> 0 (imperial)", "$MEASUREMENT is 0, want 1")
def _(c):
    c.edit(c.dxf, ("$MEASUREMENT\n 70\n1\n", "$MEASUREMENT\n 70\n0\n"))
    return c.validate()


@case("the whole HEADER section deleted -- the state OpenSCAD ships in",
      "no HEADER section in the file")
def _(c):
    txt = open(c.dxf, encoding="utf-8").read()
    i = txt.index("  0\nSECTION\n  2\nHEADER\n")
    j = txt.index("  0\nENDSEC\n", i) + len("  0\nENDSEC\n")
    open(c.dxf, "w", encoding="utf-8").write(txt[:i] + txt[j:])
    # And the trap, demonstrated on the artefact OpenSCAD actually ships --
    # the R12 raw export, which has no HEADER section either. ezdxf invents
    # one, so the PARSED value is never evidence that the file carries it.
    import ezdxf
    inv = ezdxf.readfile(c.raw).header
    print(f"          (on the raw R12 export, which has no HEADER section, "
          f"ezdxf reports $INSUNITS={inv.get('$INSUNITS')!r} and "
          f"$MEASUREMENT={inv.get('$MEASUREMENT')!r} -- those are ezdxf's "
          f"defaults, not the artefact's)")
    return c.validate()


@case("the TABLES section deleted", "no TABLES section in the file")
def _(c):
    txt = open(c.dxf, encoding="utf-8").read()
    i = txt.index("  0\nSECTION\n  2\nTABLES\n")
    j = txt.index("  0\nSECTION\n  2\nBLOCKS\n", i)
    open(c.dxf, "w", encoding="utf-8").write(txt[:i] + txt[j:])
    return c.validate()


# ---------------------------------------------------------------------------
#  3. the declared layer structure
# ---------------------------------------------------------------------------
@case("layer CUT_OUTER renamed out of the LAYER table",
      "layer CUT_OUTER is not in the LAYER table")
def _(c):
    c.edit(c.dxf, ("CUT_OUTER", "CUT_OUTRE"))     # the table entry is first
    return c.validate()


@case("CUT_INNER recoloured ACI 3 (green) -> 4 (cyan)",
      "layer CUT_INNER is ACI 4, fabrication.md 1.6 declares 3")
def _(c):
    # the LAYER table record, not an entity: group 62 after the table's name
    c.edit(c.dxf, ("CUT_INNER\n 70\n0\n 62\n3\n",
                   "CUT_INNER\n 70\n0\n 62\n4\n"))
    return c.validate()


@case("one contour moved onto layer \"0\" -- OpenSCAD's own layer",
      "entity(s) on undeclared layer '0'")
def _(c):
    txt = open(c.dxf, encoding="utf-8").read()
    k = txt.index("\nCUT_INNER\n", txt.index("ENTITIES"))
    open(c.dxf, "w", encoding="utf-8").write(
        txt[:k] + "\n0\n" + txt[k + len("\nCUT_INNER\n"):])
    return c.validate()


@case("the fiducial's layer entry stripped to CUT_INNER",
      f"0 entity(s) on {dp.FID_LAYER}, want exactly 1")
def _(c):
    txt = open(c.dxf, encoding="utf-8").read()
    k = txt.index("\nFIDUCIAL\n", txt.index("ENTITIES"))
    open(c.dxf, "w", encoding="utf-8").write(
        txt[:k] + "\nCUT_INNER\n" + txt[k + len("\nFIDUCIAL\n"):])
    return c.validate()


# ---------------------------------------------------------------------------
#  4. entity inventory, and the lossless-rewrite conservation law
# ---------------------------------------------------------------------------
@case("a contour deleted: 16 entities, 1460 raw segments unaccounted for",
      "the rewrite changed the geometry")
def _(c):
    txt = open(c.dxf, encoding="utf-8").read()
    i = txt.index("  0\nLWPOLYLINE\n", txt.index("ENTITIES"))
    j = txt.index("  0\nLWPOLYLINE\n", i + 10)
    open(c.dxf, "w", encoding="utf-8").write(txt[:i] + txt[j:])
    return c.validate()


@case("ONE VERTEX of the outer profile dragged 0.300 mm: the segment COUNT is "
      "untouched, and |area| and centroid are integral quantities that barely "
      "move, so only a vertex-by-vertex comparison against the RAW export can "
      "see it",
      "the shipped DXF's geometry is not the raw export's")
def _(c):
    # THE HOLE THIS CASE EXISTS FOR. Before the conservation check in
    # validate_dxf() section 5b, every other check here passed on this file:
    # by bisection, a vertex of the slab's outline could be dragged 6.782 mm
    # before the |area| comparison in handedness() noticed (0.300 mm moves the
    # area by 4.4e-08 relative against TOL_AREA_REL 1e-06, and the centroid by
    # 7.9e-06 mm against TOL_MAP 1e-03). A 6.782 mm notch in a 12.700 mm
    # acrylic perimeter shipped silently.   [MEASURED 2026-11-07]
    import ezdxf
    d = ezdxf.readfile(c.dxf)
    for e in d.modelspace():
        if e.dxftype() == "LWPOLYLINE" and e.dxf.layer == "CUT_OUTER":
            pts = [(p[0], p[1]) for p in e.get_points()]
            pts[0] = (pts[0][0] + 0.300, pts[0][1])
            e.set_points(pts, format="xy")
            e.close(True)
            break
    d.saveas(c.dxf)
    return c.validate()


@case("a contour's closed flag cleared (70 -> 0): an OPEN path",
      "is not flagged closed")
def _(c):
    txt = open(c.dxf, encoding="utf-8").read()
    i = txt.index("  0\nLWPOLYLINE\n", txt.index("ENTITIES"))
    j = txt.index(" 70\n", i)
    open(c.dxf, "w", encoding="utf-8").write(
        txt[:j] + " 70\n     0\n" + txt[txt.index("\n", j + 4) + 1:])
    return c.validate()


@case("a SPLINE added -- an entity type a receiver may silently drop",
      "not in the whitelist")
def _(c):
    import ezdxf
    d = ezdxf.readfile(c.dxf)
    d.modelspace().add_spline([(0, 0), (1, 1), (2, 0)],
                              dxfattribs={"layer": "CUT_INNER"})
    d.saveas(c.dxf)
    return c.validate()


@case("END-TO-END: one segment of an INNER contour cut out of the RAW "
      "export, so that loop cannot close; post_process leaves it as lines "
      "and the pipeline refuses the file",
      "would not stitch into a closed contour")
def _(c):
    import ezdxf
    segs = dp.dxf_segments(c.raw)
    rings, _, _ = dp.closed_rings(segs)
    # the SMALLEST contour, so the outside profile is untouched and the
    # containment test still knows which contour is the perimeter
    victim = min(rings, key=lambda r: abs(dp.ring_area(r)))
    drop = {frozenset((dp._key(victim[0]), dp._key(victim[1])))}
    kept = [s for s in segs
            if frozenset((dp._key(s[0]), dp._key(s[1]))) not in drop]
    assert len(kept) == len(segs) - 1, "expected to drop exactly one segment"
    d = ezdxf.new("R12", setup=False)
    for a, b in kept:
        d.modelspace().add_line(a, b)
    d.saveas(c.raw)
    fails, facts = dp.post_process(PART, os.path.join(c.dir, "dxf"), c.svg)
    print(f"          (rewrite kept {facts['rewrite']['rings']} closed rings "
          f"and {facts['rewrite']['loose']} loose segments -- it did not "
          f"force the loop shut)")
    return fails


# ---------------------------------------------------------------------------
#  5. extents
# ---------------------------------------------------------------------------
@case("$EXTMIN/$EXTMAX put back to AutoCAD's no-extents sentinels",
      "are sentinels")
def _(c):
    txt = open(c.dxf, encoding="utf-8").read()
    i = txt.index("$EXTMIN")
    j = txt.index("$EXTMAX")
    k = txt.index("$", j + 8)
    open(c.dxf, "w", encoding="utf-8").write(
        txt[:i] + "$EXTMIN\n 10\n1e+20\n 20\n1e+20\n 30\n1e+20\n"
                  "  9\n$EXTMAX\n 10\n-1e+20\n 20\n-1e+20\n 30\n-1e+20\n"
                  "  9\n" + txt[k:])
    return c.validate()


# ---------------------------------------------------------------------------
#  6. HANDEDNESS -- fabrication.md 4.2 item 4, checked on every export
# ---------------------------------------------------------------------------
@case("the SVG Y-MIRRORED: every y negated, so the two files disagree",
      "The SVG is a MIRROR of the DXF")
def _(c):
    txt = open(c.svg, encoding="utf-8").read()
    head, _, body = txt.partition('<path d="')
    body = re.sub(r'(-?)(\d+(?:\.\d+)?),(-?)(\d+(?:\.\d+)?)',
                  lambda m: f"{m.group(1)}{m.group(2)},"
                            f"{'' if m.group(3) else '-'}{m.group(4)}", body)
    open(c.svg, "w", encoding="utf-8").write(head + '<path d="' + body)
    return c.validate()


@case("the SVG X-MIRRORED: a true mirror that leaves the determinant alone",
      "OpenSCAD's writers give (x*+1, y*-1)")
def _(c):
    txt = open(c.svg, encoding="utf-8").read()
    head, _, body = txt.partition('<path d="')
    body = re.sub(r'(?<![.\d])(-?)(\d+(?:\.\d+)?),',
                  lambda m: f"{'' if m.group(1) else '-'}{m.group(2)},", body)
    open(c.svg, "w", encoding="utf-8").write(head + '<path d="' + body)
    return c.validate()


@case("the SVG's fiducial TRANSLATED 0.05 mm in x -- area preserved, so only "
      "the position check can catch it",
      "does not reconcile")
def _(c):
    txt = open(c.svg, encoding="utf-8").read()
    i = txt.index(f'<path id="{dp.FID_SVG_ID}"')
    j = txt.index(' d="', i) + 4          # the LEADING SPACE matters: without
    k = txt.index('"', j)                 #   it, index() finds id="'s own d="
    assert "," in txt[j:k], f"did not find the mark's path data: {txt[j:k]!r}"
    moved = re.sub(r'(-?\d+\.\d+),',
                   lambda m: f"{float(m.group(1)) + 0.05:.6f},", txt[j:k])
    assert moved != txt[j:k], "the translation did not apply"
    open(c.svg, "w", encoding="utf-8").write(txt[:j] + moved + txt[k:])
    return c.validate()


@case("the fiducial's <path> deleted from the SVG: contour counts disagree",
      "contour counts differ")
def _(c):
    txt = open(c.svg, encoding="utf-8").read()
    i = txt.index(f'<path id="{dp.FID_SVG_ID}"')
    j = txt.index("/>", i) + 2
    open(c.svg, "w", encoding="utf-8").write(txt[:i] + txt[j:])
    return c.validate()


@case("END-TO-END: post_process re-run on a Y-mirrored SVG -- the mark is "
      "placed by the solved map, and the validation still refuses the pair",
      "MIRROR of the DXF")
def _(c):
    # Start from OpenSCAD's own SVG (strip the mark this pipeline added), so
    # this is the state a fresh render would be in, then mirror it.
    txt = open(c.svg, encoding="utf-8").read()
    txt = re.sub(r'\n?<path id="' + dp.FID_SVG_ID + r'"[^>]*/>', "", txt)
    head, sep, body = txt.partition('<path d="')
    body = re.sub(r'(-?)(\d+(?:\.\d+)?),(-?)(\d+(?:\.\d+)?)',
                  lambda m: f"{m.group(1)}{m.group(2)},"
                            f"{'' if m.group(3) else '-'}{m.group(4)}", body)
    open(c.svg, "w", encoding="utf-8").write(head + sep + body)
    fails, _ = dp.post_process(PART, os.path.join(c.dir, "dxf"), c.svg)
    return fails


@case("MUST PASS: post_process run three times without re-rendering -- the "
      "SVG still carries exactly one mark", None)
def _(c):
    d = os.path.join(c.dir, "dxf")
    for _ in range(3):
        fails, _ = dp.post_process(PART, d, c.svg)
    n = open(c.svg, encoding="utf-8").read().count(f'id="{dp.FID_SVG_ID}"')
    print(f"          (three passes -> {n} fiducial path(s) in the SVG)")
    assert n == 1, f"not idempotent: {n} marks"
    return fails


# ---------------------------------------------------------------------------
#  7. THE DEGENERACY THE FIDUCIAL EXISTS TO REMOVE
#
#  Synthetic, and deliberately so: the slab is already asymmetric (one round
#  screw datum against five slots), so it cannot demonstrate the failure. The
#  point under test is the checker's logic, not this part's geometry -- a
#  doubly-symmetric part makes the model->SVG map unrecoverable, and then a
#  mirrored file cannot be told from a correct one at all.
# ---------------------------------------------------------------------------
def _sq(cx, cy, r):
    return [(cx - r, cy - r), (cx + r, cy - r), (cx + r, cy + r),
            (cx - r, cy + r)]


def _sym_rings(mode):
    """A part symmetric about BOTH axes, in three states."""
    rings = [_sq(0.0, 0.0, 50.0)]                       # the outside profile
    for sx in (1, -1):
        for sy in (1, -1):
            rings.append(_sq(sx * 20.0, sy * 10.0, 3.0))   # four equal holes
    if mode == "centre_hole":
        rings.append(_sq(0.0, 0.0, 7.0))     # a 2nd unique area, still on both
    if mode == "fiducial":                   #   axes of symmetry
        rings.append(dp.fiducial_ring((-50.0, -50.0, 50.0, 50.0)))
    return rings


@case("SYNTHETIC doubly-symmetric part, NO fiducial: only the outline has a "
      "unique area, so the map cannot be pinned at all",
      "the map cannot be pinned")
def _(c):
    rings = _sym_rings("bare")
    c.write_svg(rings)                       # a CORRECT SVG of that part
    return dp.validate_pair(c.svg, rings, None)[0]


@case("SYNTHETIC doubly-symmetric part with TWO unique areas but both on the "
      "axes: all four axis maps fit, so a mirror is undetectable",
      "handedness is DEGENERATE")
def _(c):
    rings = _sym_rings("centre_hole")
    c.write_svg(rings)
    fits = dp.handedness(rings, dp.svg_subpaths(c.svg))[0]
    print(f"          (axis maps that fit equally well: "
          f"{[f[:2] for f in fits]})")
    return dp.validate_pair(c.svg, rings, None)[0]


@case("SYNTHETIC: the SAME part with the fiducial added -- exactly one map "
      "fits, the correct pair passes, and the mirror is caught",
      "The SVG is a MIRROR of the DXF")
def _(c):
    rings = _sym_rings("fiducial")
    part, fid = rings[:-1], rings[-1]
    # 1. the correct pair must now PASS. Without this the next step proves
    #    nothing: a validator that refuses everything is not a check.
    c.write_svg(rings)
    ok = dp.validate_pair(c.svg, part, fid)[0]
    assert not ok, f"the correct synthetic pair was refused: {ok}"
    fits = dp.handedness(rings, dp.svg_subpaths(c.svg))[0]
    print(f"          (with the fiducial: {[f[:2] for f in fits]} -- one map, "
          f"and the correct pair validates)")
    # 2. the same pair with the Y-negation forgotten -- a true mirror.
    c.write_svg([[(x, -y) for x, y in r] for r in rings])
    return dp.validate_pair(c.svg, part, fid)[0]


# ---------------------------------------------------------------------------
#  8. the SVG's own guarantees -- fabrication.md 4.2 items 1 and 3
# ---------------------------------------------------------------------------
@case("one subpath's closepath (z) removed from the SVG: fabrication.md 4.2 "
      "item 5's second half, which was recorded as a fact and never asserted",
      "carry no closepath")
def _(c):
    txt = open(c.svg, encoding="utf-8").read()
    # the last ' z' that belongs to the PART's path, not the fiducial's
    i = txt.rfind(" z", 0, txt.find('id="' + dp.FID_SVG_ID + '"'))
    assert i > 0, "no part closepath found"
    open(c.svg, "w", encoding="utf-8").write(txt[:i] + txt[i + 2:])
    return c.validate()


@case("the SVG's fill put back to lightgray: an accidental raster engrave",
      "that is how you order a raster engrave by accident")
def _(c):
    c.edit(c.svg, ('fill="none"', 'fill="lightgray"'))
    return c.validate()


@case("the SVG's width grown 0.1 mm against an unchanged viewBox: no longer "
      "1.000 mm per user unit", "mm per user unit, want 1.0")
def _(c):
    txt = open(c.svg, encoding="utf-8").read()
    m = re.search(r'width="(\d+(?:\.\d+)?)mm"', txt)
    assert m, "no width in mm on the SVG root"
    open(c.svg, "w", encoding="utf-8").write(
        txt[:m.start(1)] + f"{float(m.group(1)) + 0.1}" + txt[m.end(1):])
    return c.validate()


# ---------------------------------------------------------------------------
#  9. the fiducial must be in the WASTE, not in the part
# ---------------------------------------------------------------------------
@case("the fiducial moved inside the part's bounding box",
      "must be in the sheet's waste")
def _(c):
    rings = dp.closed_rings(dp.dxf_segments(c.raw))[0]
    ob = dp.ring_bbox(max(rings, key=lambda r: abs(dp.ring_area(r))))
    inside = [(x + 160.0, y + 120.0) for x, y in
              dp.fiducial_ring(ob)]             # drag it back onto the part
    c.write_svg(rings + [inside])
    return dp.validate_pair(c.svg, rings, inside)[0]


# ---------------------------------------------------------------------------
#  0. and the real files, unbroken, at the end of every run
# ---------------------------------------------------------------------------
def real_files_pass():
    out = []
    for name in sorted(f[:-4] for f in os.listdir(os.path.join(DXF_DIR, "raw"))
                       if f.endswith(".dxf")):
        fails, _, pr, fid = dp.validate_dxf(
            os.path.join(DXF_DIR, f"{name}.dxf"),
            os.path.join(DXF_DIR, "raw", f"{name}.dxf"))
        fails += dp.validate_pair(os.path.join(SVG_DIR, f"{name}.svg"),
                                  pr, fid)[0]
        out.append((name, fails))
    return out


def main():
    # PRECONDITION. Every break starts from the validated pair, so if what is
    # on disk is not validated -- a half-finished export, a mid-edit
    # parameters.scad -- the breaks measure that instead, and the output is
    # noise. Refuse to run rather than report a misleading result.
    pre = real_files_pass()
    stale = [(n, f) for n, f in pre if f]
    if stale:
        print("  the exported files on disk are NOT in a validated state, so "
              "the breaks below would measure that, not the checks:")
        for n, f in stale:
            for one in f:
                print(f"      {n}: {one}")
        print("\n  run:  /usr/bin/python3.11 v2/tools/export.py slab backpanel")
        return 1

    bad = 0
    print(f"  breaking {PART} {len(CASES)} ways; every one must be refused\n")
    for n, (label, want, fn) in enumerate(CASES, 1):
        with tempfile.TemporaryDirectory() as tmp:
            c = Copy(tmp)
            try:
                fails = fn(c)
            except Exception as ex:
                print(f"  {n:2d}. ERROR  {label}\n          "
                      f"{type(ex).__name__}: {ex}")
                bad += 1
                continue
        if want is None:
            if fails:
                print(f"  {n:2d}. WRONGLY REFUSED  {label}")
                for f in fails:
                    print(f"          -> {f}")
                bad += 1
            else:
                print(f"  {n:2d}. PASSED {label}")
            continue
        hit = [f for f in fails if want in f]
        if hit:
            print(f"  {n:2d}. FIRED  {label}")
            print(f"          -> {hit[0]}")
            if len(fails) > 1:
                print(f"          (+{len(fails)-1} other rejection(s))")
        else:
            got = fails or ["NOTHING -- the check does not fire"]
            print(f"  {n:2d}. SILENT {label}")
            print(f"          wanted {want!r}, got {got}")
            bad += 1

    print("\n  and the real files, unmodified:")
    for name, fails in real_files_pass():
        if fails:
            bad += 1
            print(f"      {name}: {len(fails)} REJECTION(S)")
            for f in fails:
                print(f"        {f}")
        else:
            print(f"      {name}: accepted -- units, layers, closed loops, "
                  f"lossless rewrite, handedness")

    print(f"\n  {'every check fires' if not bad else str(bad) + ' PROBLEM(S)'}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
