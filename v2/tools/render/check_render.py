#!/usr/bin/env python3
"""Measure the EMITTED RENDERS. Never the source that produced them.

    /usr/bin/python3.11 v2/tools/render/check_render.py
    /usr/bin/python3.11 v2/tools/render/check_render.py --prove

A check that computes from the same parameters as the thing it checks is worse
than no check at all: it reports agreement between a number and itself. So
this file does not read v2/cad/parameters.scad and contains no deck dimension.
It takes exactly two things from the renderer's own side, both named here so
neither can grow quietly:

    the camera calibration  mm_per_px, below -- a PNG has no dimensions
                            without a stated scale, and every check is a test
                            of that stated scale against geometry the
                            renderer did not produce;
    render.geom_sha()       C6 only, and only to hash a mesh FILE. Comparing
                            two hashes requires one hash function, exactly as
                            it does for sha256; it carries no dimension, and
                            the two sides it is applied to are the staged copy
                            the picture was made from and the file on disk
                            now. Sharing it is what stops two copies drifting.

It reads two kinds of artefact off disk:

    the PNGs in v2/export/render/      what the renderer actually drew
    the STLs in v2/export/stl/        what the fabricator actually receives

and asks whether they are pictures of the same object. The only thing it takes
from the renderer's own side is the CAMERA CALIBRATION -- `mm_per_px`, in each
view's sidecar JSON -- because without a stated scale a PNG has no dimensions
at all. That is the renderer's claim about its own camera, and every check
below is a test of that claim against geometry the renderer did not produce.

THE CHECKS
----------
C1  SILHOUETTE SCALE AND POSITION. For each fabricated part, an orthographic
    flat-white mask of that part alone at a stated mm/px. Measure its
    silhouette's width, height and centroid in millimetres off the PNG;
    compare against the part's own exported STL bounding box. Catches a
    mis-scaled scene, a mis-centred camera, a dropped part and a wrong
    projection.

C2  THROUGH-HOLE COUNT. On the same mask, count the enclosed background
    regions -- the holes you can see daylight through -- and compare against
    the STL's genus, which for a prismatic plate is exactly its through-hole
    count and comes free from the mesh's Euler characteristic. Catches the
    failure C1 cannot see: the renderer loading the WRONG MESH. The slab and
    the back panel have identical 304.600 x 190.375 outlines and 14 holes
    against 17, so swapping them is invisible to a bounding box and loud here.
    (14 and 17 are [CAD-EXACT]: they are the genus this file reads off
    v2/export/stl/{slab,backpanel}.stl, and they are printed on every run.)

C3  STACK HEIGHT. An orthographic elevation mask of the three fabricated
    layers only. Measure the assembled height in millimetres off the PNG;
    compare against the sum of the three parts' own STL thicknesses. Catches a
    layer placed at the wrong Z -- a gap or an overlap in the stack -- which no
    plan view can show.

C4  TRANSMISSION. The hero claim of the whole brief is that you can see the
    wiring and the boards through 12.700 mm of clear acrylic. Two renders of
    the same frame, one with the acrylic and one without, and inside the
    slab's own footprint: the contrast through the acrylic must survive, and
    the two images must still be of the same thing. A flat sheet of grey where
    there should be boards fails on contrast; a plausible-looking but
    unrelated image fails on correlation.

C5  NOT DEGENERATE. Every emitted PNG: not one flat colour, and enough
    distinct levels to be an image. This is the check that catches a lost
    WebGL context, which otherwise produces a clean run and a blank file.

C6  THE RENDER USED THE FABRICATOR'S PART. Each view's sidecar names, for
    every EXPORTED body, the SHA-256 of the mesh file the renderer loaded and
    a hash of that mesh's GEOMETRY. One row per PART, not per body: six knobs
    are two parts and eight placements, four feet are one part and four, and
    every instance is staged from the same file. Equal bytes pass outright;
    differing bytes are judged on the geometry.

    THIS FILE USED TO SAY that the fallback existed because export.py's
    carrier was not byte-reproducible. It is reproducible now -- export.py
    canonicalises the facet order and two consecutive full exports give ten
    identical STLs -- so what a byte difference MEANS has changed: it means
    THE PICTURE PREDATES A RE-EXPORT. v2/export/stl is a live directory that
    another author's export.py rewrites, and failing a picture because the
    part was re-exported without changing is crying wolf. What still fails is
    the thing C6 is for: a picture of a part the fabricator never receives,
    which is the quietest way for a picture to be of last week's part.

C7  THE PANEL IS SHOWING SOMETHING. The display is a reflective LCD and a
    reflective LCD with nothing on it is a grey rectangle -- which is what
    this render drew for a long time with every other material correct, the
    kind of failure that survives review because nothing about it looks
    broken. Go to the rectangle the sidecar says the active area occupies and
    ask the PNG whether there is an image in it: ink, transitions per row, and
    level count.

--prove RUNS EVERY ONE OF THEM AGAINST A DELIBERATELY BROKEN RENDER
-------------------------------------------------------------------
A check nobody has watched fail is not a check. `--prove` re-renders into a
scratch directory with render.py's --break switch, confirms the check that
should fire DOES fire, then renders clean and confirms it passes. The break
modes are named for what they break; render.py writes `broken` into every
sidecar so a broken PNG can never be mistaken for a good one.
"""
import argparse
import hashlib
import json
import math
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
ROOT = os.path.dirname(TOOLS)
REPO = os.path.dirname(ROOT)
EXPORT_STL = os.path.join(ROOT, "export", "stl")
OUT_DEFAULT = os.path.join(ROOT, "export", "render")

#  The fabricated parts, by the name their mask view and their STL share.
#  Only these three are measured: a REFERENCE or VENDOR body is a model of
#  something bought, so there is no artefact to measure it against.
LASER_PARTS = ["slab", "backpanel"]      # prismatic, so genus == hole count
PRINTED_PARTS = ["carrier"]              # blind pockets, so C2 does not apply

#  C1. Two device pixels of edge, plus the project's own STL tolerance.
#  v2/docs/fabrication.md 4.7: binary STL is float32 and the worst round-trip
#  error over this part's dimensions is 2.44e-5, so the floor is not zero and
#  check_cad.py's 0.050 is the house figure. Anti-aliasing on a mask edge is
#  worth about a pixel either side, hence the 2.
C1_EDGE_PX = 2.0               # [DESIGN] one AA pixel either side of an edge
C1_FLOOR_MM = 0.050            # [REPO] check_cad.py's house STL tolerance

#  C3. The same budget: the stack is three edges, so three pixels.
C3_EDGE_PX = 3.0               # [DESIGN] three stacked parts, one edge each

#  C4. Measured on a HIGH-PASSED image, and that is not a detail.
#
#  The first version correlated raw luminance and it was fragile for a reason
#  worth writing down: the slab's own top face carries a broad, smooth
#  shading gradient from the key light, and that gradient is a large-scale
#  signal with nothing to do with what is underneath. It does not belong in
#  the measurement either way -- it dilutes the correlation of a GOOD render
#  (observed: r fell from 0.695 to 0.487 purely because the exposure changed)
#  and it would prop up a bad one.
#
#  The claim being tested is "you can see the boards through it", which is a
#  claim about DETAIL. So both frames are high-passed first -- the image minus
#  its own local mean over a 1.500 mm box -- and the ratio and the correlation
#  are taken on what is left. An opaque slab has no detail at all, so both
#  numbers collapse, which is what makes the check bite.
#  Every one of these is a [DESIGN] threshold -- a line this project draws,
#  not a quantity anyone measured off a part. They are stated here, once, so
#  that a render that
#  drifts is argued with rather than quietly re-tuned. The observed values on
#  a good render are in the README and printed on every run, which is what
#  says how much headroom each one has.
#  ===================================================================
#  CALIBRATION. The nine numbers below were nine lines drawn once, from a
#  single known-good render, every one tagged [DESIGN] because that is what
#  they were. They are now MEASURED, by v2/tools/render/calibrate.py, which
#  renders six known-good variants -- frame size, device pixel ratio,
#  tone-mapping exposure, reference-body faceting, all of them correct
#  pictures of the same deck -- against seven known-bad ones: four of
#  render.py's own --break modes and three mutations of the emitted PNGs.
#  Each threshold is then placed in the gap between the two distributions by
#  one stated rule: the GEOMETRIC mean of the worst good and the best bad for
#  a floor, because these are ratios and the midpoint of a ratio is geometric;
#  the arithmetic midpoint for a ceiling; and a quarter of the worst good
#  where the bad side is exactly zero and any positive floor would divide them.
#
#  EACH TERM DIVIDES THE FAILURES IT IS THE RIGHT TERM FOR, and that is not a
#  convenience. C4 is a conjunction, so a term only has to catch its own
#  failures; what guarantees nothing slips through is --prove, which watches
#  every break fire. Asking one term to divide a failure another term catches
#  by a factor of four makes it tighter than it has any reason to be -- and
#  twice here it would have made it WRONG, which is the thing a single sample
#  could never have shown:
#
#    * a 0x3 Gaussian blur -- a FROSTED slab -- correlates at 0.7145, which is
#      INSIDE the good set's 0.6585..0.8743. The worst good there is a
#      1000 x 700 render, whose coarser search quantum costs it match quality.
#      No correlation floor separates a light blur. The detail ratio does, at
#      0.3224 against a worst good of 0.7935.
#    * an OPAQUE slab keeps 0.7163 of its high-frequency energy, because a
#      mirror-finish opaque plate still has bright edges round its own holes.
#      No detail-ratio floor separates it either. The tile count does, at 12
#      against a worst good of 52.
#
#  Every bad variant is printed against every threshold by calibrate.py, with
#  the ones in its dividing set marked, so an exclusion is on the page rather
#  than quiet. [MEASURED 2026-11-07, 13 renders.]
#  ===================================================================
C4_HIGHPASS_MM = 1.500         # [DESIGN] local-mean box in mm, so scale-free
#  good 0.7935..0.8767 over 6 renders; bad 0.1411..0.3224 (frosted3, frosted6)
C4_MIN_DETAIL_RATIO = 0.5058   # [MEASURED] geometric mean of the two
#  good 0.6585..0.8743 over 6; bad 0.3173..0.4795 (opaque, frosted6)
C4_MIN_CORRELATION = 0.5619    # [MEASURED] geometric mean of the two
#  THE REFRACTION CEILING IS NOW DERIVED AND IS NO LONGER A NUMBER.
#  It used to be C4_MAX_SHIFT_MM = 3.000, tagged [DESIGN] and listed NOT
#  SOURCED, with the honest admission that an ORTHOGRAPHIC camera's rays are
#  parallel to -Z, that flat faces at normal incidence refract by ~0, and that
#  the 0.922 mm observed could therefore not be a Snell prediction. Both
#  halves of that were right and the conclusion drawn from them was wrong,
#  because it asked what the CAMERA does and the displacement is set by what
#  the SHADER does. Those are not the same thing under an orthographic camera.
#
#  three.js r160's getIBLVolumeRefraction() builds the refracted ray from
#  `cameraPosition - vWorldPosition` -- the camera as a POINT -- whatever the
#  projection is. It then walks `thickness` along that refracted direction and
#  projects the EXIT point to screen space to sample the backdrop. So:
#
#      i    = atan(rho / D)      rho = the point's radius from the camera
#                                axis in the slab's plane, D = the camera's
#                                distance to its target
#      sin r = sin i / n         Snell
#      displacement = t * sin r  the LATERAL part of a walk of length t
#                              = t * sin(i) / n
#
#  and that is a bound this file can compute from things it measures: t is the
#  z-extent of the SLAB'S OWN EXPORTED STL, rho_max comes off the slab's mask
#  in millimetres, D comes from the camera the sidecar records, and n is the
#  one optical constant taken from the renderer's claim ([VENDOR roehm-gsxt],
#  1.491, and the ceiling moves only +-10 % between n = 1.4 and n = 1.6).
#
#  NOTE WHICH FORMULA IS NOT USED. The parallel-slab OFFSET,
#  t*sin i*(1 - cos i/sqrt(n^2 - sin^2 i)), is what a real sheet of PMMA does
#  to a ray that goes in one face and out the other; at i = 9.407 deg it is
#  0.6939 mm, which is LESS than the 0.922 this render measures. That
#  disagreement is how the right model was found: three.js does not refract
#  out of the second surface at all, it samples at the end of a single walk,
#  so the lateral term is the whole of it -- 1.3920 mm at the same angle.
#  An honest ceiling has to be a ceiling on what the renderer does.
#
#  The tolerance added to it is ONE SEARCH STEP, which is not a fudge: the
#  per-tile offset is found by an integer search on a 4x downsampled image, so
#  the reported magnitude can overstate the true one by up to half a step per
#  axis, i.e. 0.707 of a step, and one step is the next value up that is also
#  a quantity this file already has.
#  good 52..59 over 6; bad 12 (opaque). The term that catches an opaque slab.
C4_MIN_TILES = 24              # [MEASURED] geometric mean of the two
C4_BLOCK_MM = 13.0             # [DESIGN] the correlation tile
#  [DESIGN], and calibrate.py does NOT set it -- it is a measurement
#  parameter, not a gate, and tagging it [MEASURED] because it sat next to
#  measured things would be a tag nobody earned. What bounds it is stated
#  instead: it must exceed the DERIVED refraction ceiling (1.391 mm on the
#  shipped plan view) with room over, or the search clips the displacement it
#  is meant to report. The evidence that it does is printed on every run as
#  the percentage of tiles whose best offset sits on the edge of the window:
#  4 % on the shipped render, 7 % with the thickness tripled.
C4_SEARCH_MM = 4.0

#  AND IT HAS TO BE SHIFT-TOLERANT, which is the thing a naive version of this
#  check gets wrong. 12.700 mm of PMMA at n = 1.491 REFRACTS: the image of a
#  board seen through the slab is not where the board is. Measured on this
#  render, over the Feather, the displacement is about (+0.825, -0.413) mm,
#  and it varies across the frame. A fixed-registration correlation therefore
#  reads 0.031 on a perfectly good render while the same pair correlates 0.849
#  once the offset is allowed for -- i.e. the naive check would have failed the
#  render for doing the one thing the material is supposed to do.
#
#  So the measurement is per tile, with a search: for each tile of the slab's
#  footprint, find the offset that best matches the un-occluded frame, and
#  report the MEDIAN of those correlations and the median displacement. The
#  displacement is then a result in its own right.#  C5, on a PICTURE.
#  good 0.3377..0.3836 over 6; bad 1.0000 (every picture flattened to one
#  colour). Arithmetic midpoint, because this one is a ceiling.
C5_MAX_MODAL_FRACTION = 0.6918  # [MEASURED]
#  good 73..178 over 6; bad 1 (flat). Note what is NOT here: --break blank
#  leaves 26 levels, because the backdrop and the annotation layer are still
#  drawn. C5's PICTURE branch does not catch a blank render and never did;
#  its MASK branch does, at 0.00 % lit.
C5_MIN_LEVELS = 8              # [MEASURED] geometric mean of the two
#  C5, on a MEASUREMENT MASK, which is a different question. A mask is
#  SUPPOSED to be nearly binary -- flat white on black, tone mapping off -- so
#  counting distinct levels on one asks it to fail for doing its job
#  correctly, which it duly did on the first run. What a mask must not be is
#  all one thing: the right test is that it has both a part and a background.
#  good 0.1259..0.1411 over 6; bad 0.0000 (--break blank). The two
#  distributions are infinitely separated, so this is not a discrimination
#  threshold at all -- any positive floor divides them -- and the number is a
#  quarter of the worst good, chosen as headroom for a mask view nobody has
#  rendered yet. calibrate.py says so in those words rather than dressing it
#  up as a measurement.
C5_MASK_LIT_MIN = 0.0315       # [MEASURED] a mask must have a part ...
#  good 0.7629..0.8518 over 6; bad 1.0000 (the masks painted fully lit)
C5_MASK_LIT_MAX = 0.9259       # [MEASURED] ... and a background

#  C7, on the DISPLAY, and calibrated the same way against a panel
#  deliberately blanked -- to PAPER for the floor and to INK for the ceiling,
#  because a screen stuck on is as dead as a screen stuck off.
#  good 0.1545..0.1890 over 6; bad 0.0238 (--break screen)
C7_INK_MIN = 0.0606            # [MEASURED] geometric mean of the two
#  good as above; bad 0.9762 (the same break, inverted)
C7_INK_MAX = 0.5826            # [MEASURED] arithmetic midpoint
#  good 4.9948..17.2753 over 6; bad 1.0000 (--break screen). This one also
#  separates both blurs, 1.92 and 1.39, which it was not asked to.
C7_RUNS_MIN = 2.2349           # [MEASURED] geometric mean of the two
#  good 123..127 over 6; bad 27 (--break screen)
C7_LEVELS_MIN = 57             # [MEASURED] geometric mean of the two


class Result:
    def __init__(self):
        self.rows = []

    def add(self, cid, name, ok, detail, skipped=False):
        self.rows.append((cid, name, "skip" if skipped else
                          ("PASS" if ok else "FAIL"), detail))

    def report(self):
        w = max((len(r[1]) for r in self.rows), default=10)
        for cid, name, st, detail in self.rows:
            print(f"  {st:4s}  {cid:3s} {name:{w}s}  {detail}")
        nf = sum(1 for r in self.rows if r[2] == "FAIL")
        ns = sum(1 for r in self.rows if r[2] == "skip")
        np_ = sum(1 for r in self.rows if r[2] == "PASS")
        print(f"\n  {np_} passed, {nf} FAILED, {ns} skipped")
        return nf

    def failed(self):
        return {r[0] for r in self.rows if r[2] == "FAIL"}


# ===========================================================================
#  PNG measurement
# ===========================================================================
#  PIL IS NOT USABLE ON THIS INTERPRETER, and v2/docs/fabrication.md 4.7 says
#  it is -- it lists "PIL 10.2.0" among what /usr/bin/python3.11 has. The
#  package is installed but its C extension is not:
#
#      /usr/lib/python3/dist-packages/PIL/_imaging.cpython-312-...so
#
#  is built for 3.12, so `from PIL import Image` raises ImportError on 3.11.
#  Reported in this tool's README; the row in fabrication.md needs amending.
#
#  So the PNG is decoded by ImageMagick, which is on PATH (6.9.12-98), into a
#  binary PPM, and measured with numpy. That keeps the thing being measured
#  the PNG a person actually looks at -- not a side-channel dump of the frame
#  buffer, which would stop being a check of the emitted file.
PPM_TOOLS = ["magick", "convert"]


def _ppm_tool():
    for t in PPM_TOOLS:
        if shutil.which(t):
            return t
    raise RuntimeError(
        "no PNG decoder: neither 'magick' nor 'convert' is on PATH, and "
        "PIL's C extension on this interpreter is built for another Python. "
        "Install ImageMagick or run this file on an interpreter with a "
        "working Pillow.")


_RGB_CACHE = {}


def load_rgb(path):
    """The PNG as an (H, W, 3) uint8 array."""
    import numpy as np
    key = (path, os.path.getmtime(path))
    if key in _RGB_CACHE:
        return _RGB_CACHE[key]
    tool = _ppm_tool()
    cmd = ([tool, path, "-depth", "8", "ppm:-"] if tool == "convert"
           else [tool, "convert", path, "-depth", "8", "ppm:-"])
    out = subprocess.run(cmd, capture_output=True, timeout=300)
    if out.returncode != 0 or not out.stdout.startswith(b"P6"):
        raise RuntimeError(f"{tool} could not decode {path}: "
                           f"{out.stderr[-300:]!r}")
    #  P6 header: three whitespace-separated integers after the magic, then a
    #  single whitespace byte, then the raster. Comments may appear anywhere.
    buf = out.stdout
    i, fields = 2, []
    while len(fields) < 3:
        while buf[i:i + 1].isspace():
            i += 1
        if buf[i:i + 1] == b"#":
            while buf[i:i + 1] not in (b"\n", b""):
                i += 1
            continue
        j = i
        while not buf[j:j + 1].isspace():
            j += 1
        fields.append(int(buf[i:j]))
        i = j
    i += 1
    w, h, maxv = fields
    if maxv != 255:
        raise RuntimeError(f"{path}: PPM maxval {maxv}, expected 255")
    a = np.frombuffer(buf, dtype=np.uint8, count=w * h * 3,
                      offset=i).reshape(h, w, 3)
    _RGB_CACHE.clear()
    _RGB_CACHE[key] = a
    return a


def load_gray(path):
    """Rec.709 luma of the PNG, (H, W) float64, and its (W, H) size.

    Luma on the sRGB-ENCODED values, not linear luminance. Every metric here
    is a contrast ratio or a correlation, both of which only need the mapping
    to be the same for every image, and this one is stated rather than
    inherited from whatever a library's default colour management does.
    """
    import numpy as np
    a = load_rgb(path).astype(np.float64)
    g = 0.2126 * a[:, :, 0] + 0.7152 * a[:, :, 1] + 0.0722 * a[:, :, 2]
    return g, (g.shape[1], g.shape[0])


def mask_extents(path, thresh=96):
    """Bounding box and centroid of the lit region of a mask PNG, in PIXELS.

    Returns (x0, y0, x1, y1, cx, cy, n) with inclusive integer pixel bounds,
    or None if nothing is lit.
    """
    import numpy as np
    a, (W, H) = load_gray(path)
    m = a >= thresh
    if not m.any():
        return None
    cols = np.flatnonzero(m.any(axis=0))
    rows = np.flatnonzero(m.any(axis=1))
    ys, xs = np.nonzero(m)
    wgt = a[ys, xs] / 255.0
    sw = wgt.sum()
    return (int(cols[0]), int(rows[0]), int(cols[-1]), int(rows[-1]),
            float((wgt * xs).sum() / sw), float((wgt * ys).sum() / sw),
            int(m.sum()))


def count_enclosed_holes(path, thresh=96, min_px=12):
    """How many enclosed dark regions a mask PNG has.

    A through-hole in a plate, seen square-on in a flat-white orthographic
    mask, is a dark island with lit pixels all the way round it. Count those:
    label the dark pixels into 4-connected components and keep the ones that
    never touch the image border.

    Done on RUNS, not on pixels. scipy's label() and trimesh's split() both
    need scipy and it is absent (v2/docs/fabrication.md 4.7), and a per-pixel
    union-find over 3.2 megapixels in Python is minutes. A 1600 x 1000 mask of
    this part has a few thousand horizontal runs, and union-find over those is
    milliseconds and exactly equivalent.

    min_px discards anti-aliasing specks. The smallest real hole in this
    design is the M2.5 clearance at O2.900, which is 60+ pixels at the
    coarsest calibration this tool uses, so the floor is nowhere near it.
    """
    import numpy as np
    a, (W, H) = load_gray(path)
    dark = a < thresh

    parent = {}

    def find(x):
        r = x
        while parent[r] != r:
            r = parent[r]
        while parent[x] != r:
            parent[x], x = r, parent[x]
        return r

    def union(x, y):
        rx, ry = find(x), find(y)
        if rx != ry:
            parent[ry] = rx

    prev = []                      # [(x0, x1, id)] for the row above
    runs = {}                      # id -> [npix, touches_border]
    nxt = 0
    for y in range(H):
        row = dark[y]
        if not row.any():
            prev = []
            continue
        #  run boundaries, from the transitions in the row
        d = np.flatnonzero(np.diff(row.astype(np.int8)))
        starts = ([0] if row[0] else []) + list(d[row[d] == False] + 1)
        ends = list(d[row[d]] ) + ([W - 1] if row[-1] else [])
        cur = []
        for x0, x1 in zip(starts, ends):
            rid = nxt
            nxt += 1
            parent[rid] = rid
            runs[rid] = [x1 - x0 + 1,
                         x0 == 0 or x1 == W - 1 or y == 0 or y == H - 1]
            for p0, p1, pid in prev:
                if p0 <= x1 and x0 <= p1:      # 4-connected overlap
                    union(pid, rid)
            cur.append((x0, x1, rid))
        prev = cur

    agg = {}
    for rid, (n, border) in runs.items():
        r = find(rid)
        e = agg.setdefault(r, [0, False])
        e[0] += n
        e[1] = e[1] or border
    return sum(1 for n, border in agg.values()
               if not border and n >= min_px)


def _boxmean(a, r):
    """Mean of a over a (2r+1) square, by integral image. No scipy."""
    import numpy as np
    H, W = a.shape
    ii = np.zeros((H + 1, W + 1), dtype=np.float64)
    ii[1:, 1:] = a.cumsum(axis=0).cumsum(axis=1)
    y0 = np.clip(np.arange(H) - r, 0, H)
    y1 = np.clip(np.arange(H) + r + 1, 0, H)
    x0 = np.clip(np.arange(W) - r, 0, W)
    x1 = np.clip(np.arange(W) + r + 1, 0, W)
    tot = (ii[y1][:, x1] - ii[y0][:, x1] - ii[y1][:, x0] + ii[y0][:, x0])
    cnt = ((y1 - y0)[:, None] * (x1 - x0)[None, :]).astype(np.float64)
    return tot / cnt


def _erode(mask, r):
    """Shrink a boolean mask by r, so a high-pass window never straddles its
    edge and manufactures detail that is really just the mask's own border."""
    import numpy as np
    full = _boxmean(mask.astype(np.float64), r)
    return full > 0.999


def _downsample(a, k):
    """Block mean by k. Reduces the search space without losing structure."""
    import numpy as np
    H, W = (a.shape[0] // k) * k, (a.shape[1] // k) * k
    return a[:H, :W].reshape(H // k, k, W // k, k).mean(axis=(1, 3))


def detail_match(a_path, b_path, mask_path, mm_per_px, thresh=96):
    """How much of b survives in a, and how far refraction moves it.

    Returns (detail_ratio, median_r, median_shift_mm, n_tiles, hp_px,
    search_step_mm, pinned_fraction).
    `a` is the frame WITH the acrylic, `b` the same frame without it.
    """
    import numpy as np
    a, _ = load_gray(a_path)
    b, _ = load_gray(b_path)
    mk, _ = load_gray(mask_path)
    if b.shape != a.shape:
        return None
    if mk.shape != a.shape:
        yi = np.arange(a.shape[0]) * mk.shape[0] // a.shape[0]
        xi = np.arange(a.shape[1]) * mk.shape[1] // a.shape[1]
        mk = mk[yi][:, xi]

    #  (1) DETAIL RATIO, at full resolution. High-pass = the image minus its
    #  own local mean, so the slab's broad shading gradient -- which is its
    #  own surface, not what is under it -- does not count either way.
    r = max(2, int(round(C4_HIGHPASS_MM / mm_per_px)))
    sel = _erode(mk >= thresh, r)
    if not sel.any():
        return None
    ha, hb = a - _boxmean(a, r), b - _boxmean(b, r)
    sa, sb = float(ha[sel].std()), float(hb[sel].std())
    ratio = sa / sb if sb > 0 else 0.0

    #  (2) SHIFT-TOLERANT CORRELATION, on a 4x block mean. The search is the
    #  expensive part and structure survives downsampling; the reported shift
    #  is converted back to mm, so the number does not depend on the scale it
    #  was measured at.
    k = 4
    da, db, dm = _downsample(ha, k), _downsample(hb, k), _downsample(
        sel.astype(np.float64), k)
    mmpp = mm_per_px * k
    blk = max(8, int(round(C4_BLOCK_MM / mmpp)))
    srch = max(2, int(round(C4_SEARCH_MM / mmpp)))
    H, W = da.shape
    rs, shifts, pinned = [], [], 0
    for y in range(srch, H - blk - srch, blk):
        for x in range(srch, W - blk - srch, blk):
            if dm[y:y + blk, x:x + blk].min() < 0.999:
                continue
            t = da[y:y + blk, x:x + blk]
            t = t - t.mean()
            tn = float((t * t).sum())
            #  a tile with no detail in it cannot say anything either way
            if tn < blk * blk * 1.0:
                continue
            best, bdx, bdy = -2.0, 0, 0
            for dy in range(-srch, srch + 1):
                for dx in range(-srch, srch + 1):
                    u = db[y + dy:y + dy + blk, x + dx:x + dx + blk]
                    u = u - u.mean()
                    un = float((u * u).sum())
                    if un <= 0:
                        continue
                    c = float((t * u).sum()) / math.sqrt(tn * un)
                    if c > best:
                        best, bdx, bdy = c, dx, dy
            if best > -2.0:
                rs.append(best)
                shifts.append(math.hypot(bdx, bdy) * mmpp)
                #  A tile whose best offset sits ON the edge of the search
                #  window has not been measured, it has been clipped: the true
                #  offset is somewhere beyond. Counting them is what tells a
                #  displacement that has been measured from one that has only
                #  been bounded by the window.
                if max(abs(bdx), abs(bdy)) >= srch:
                    pinned += 1
    if not rs:
        return (ratio, 0.0, 0.0, 0, r, mmpp, 0.0)
    #  THE DISPLACEMENT IS READ OFF THE TILES THAT MATCHED, and only those.
    #  A tile whose best correlation is 0.2 has not found the same piece of
    #  board in the other frame; the offset it reports is wherever the noise
    #  happened to peak, which is uniformly distributed over the search window
    #  and therefore BIG. Including those pulls the median displacement up in
    #  exactly the renders where it means least -- measured: blanking the
    #  display's own content, which is not a transmission failure at all, put
    #  the median at 1.50x its derived ceiling and tripped C4 for a reason
    #  that had nothing to do with the acrylic.
    good = [s for s, c in zip(shifts, rs) if c >= C4_MIN_CORRELATION]
    if not good:
        good = shifts
    rs.sort()
    good.sort()
    return (ratio, rs[len(rs) // 2], good[len(good) // 2], len(rs), r,
            mmpp, pinned / len(rs))


def masked_values(path, mask_path, thresh=96, step=2):
    """The luma of a render inside a mask taken from a DIFFERENT render.

    Returned in a fixed raster order, so two calls on two renders of the same
    frame are element-for-element comparable -- which is what makes C4's
    correlation mean anything.
    """
    import numpy as np
    a, _ = load_gray(path)
    mk, _ = load_gray(mask_path)
    if mk.shape != a.shape:
        #  nearest-neighbour, by indexing: the mask is binary and must not be
        #  interpolated into grey at its edges
        yi = (np.arange(a.shape[0]) * mk.shape[0] // a.shape[0])
        xi = (np.arange(a.shape[1]) * mk.shape[1] // a.shape[1])
        mk = mk[yi][:, xi]
    sel = mk[::step, ::step] >= thresh
    v = a[::step, ::step][sel]
    return None if v.size == 0 else v


def stats(v):
    return dict(mean=float(v.mean()), std=float(v.std()), n=int(v.size))


def correlate(a, b):
    import numpy as np
    n = min(a.size, b.size)
    if n < 100:
        return 0.0
    a, b = a[:n] - a[:n].mean(), b[:n] - b[:n].mean()
    da, db = float((a * a).sum()), float((b * b).sum())
    if da <= 0 or db <= 0:
        return 0.0
    return float((a * b).sum() / math.sqrt(da * db))


def _pixels(path):
    g, _ = load_gray(path)
    return int(g.size)


def modal_fraction_and_levels(path):
    """How flat the image is: the share taken by its single commonest
    luma level, and how many levels carry a non-trivial share."""
    import numpy as np
    g, _ = load_gray(path)
    h = np.bincount(np.clip(g, 0, 255).astype(np.uint8).ravel(),
                    minlength=256)
    tot = int(h.sum())
    return float(h.max()) / tot, int((h > tot * 1e-5).sum())


# ===========================================================================
#  STL measurement
# ===========================================================================
def stl_facts(path):
    import trimesh
    m = trimesh.load(path, force="mesh")
    lo, hi = m.bounds
    return dict(w=float(hi[0] - lo[0]), d=float(hi[1] - lo[1]),
                t=float(hi[2] - lo[2]),
                cx=float((lo[0] + hi[0]) / 2), cy=float((lo[1] + hi[1]) / 2),
                euler=int(m.euler_number),
                genus=(2 - int(m.euler_number)) // 2,
                faces=int(len(m.faces)))


def sha(path):
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def panel_content(path, deck_box, mmpp):
    """Is the panel in this picture showing anything?

    `deck_box` is where the renderer says the active area is, in millimetres
    of the deck frame; the mapping from there to pixels is the same one C1
    uses for a silhouette, and the only renderer claim involved is mm_per_px.
    Inside that rectangle, three numbers off the PNG:

        ink     the share of pixels below the midpoint of the region's own
                range. A panel with nothing on it is one tone and has none.
        runs    dark/light transitions per row, which is what distinguishes
                TEXT from a rectangle of flat grey or a smooth gradient: a
                blank panel has 0, a panel with glyphs on it has one pair of
                transitions per stroke.
        levels  distinct luma levels. A bi-level panel seen through 12.700 mm
                of acrylic is not literally two levels -- the slab shades it
                -- but it is not a photograph either.

    Returns (ink_fraction, runs_per_row, levels, (w_mm, h_mm)) or None.
    """
    import numpy as np
    g, (W, H) = load_gray(path)
    x0, y0, x1, y1 = deck_box
    #  +X right, and the image's +y runs DOWN while the plan view's up is the
    #  deck's +Y, so y flips -- exactly as in C1.
    px0 = int(round((W - 1) / 2.0 + x0 / mmpp))
    px1 = int(round((W - 1) / 2.0 + x1 / mmpp))
    py0 = int(round((H - 1) / 2.0 - y1 / mmpp))
    py1 = int(round((H - 1) / 2.0 - y0 / mmpp))
    px0, px1 = max(0, px0), min(W - 1, px1)
    py0, py1 = max(0, py0), min(H - 1, py1)
    if px1 - px0 < 8 or py1 - py0 < 8:
        return None
    a = g[py0:py1 + 1, px0:px1 + 1]
    lo, hi = float(a.min()), float(a.max())
    if hi - lo < 1e-9:
        return (0.0, 0.0, 1, ((px1 - px0 + 1) * mmpp, (py1 - py0 + 1) * mmpp))
    mid = (lo + hi) / 2.0
    dark = a < mid
    ink = float(dark.mean())
    #  transitions per row, counted on the row's own binarisation
    runs = float(np.abs(np.diff(dark.astype(np.int8), axis=1)).sum()
                 / dark.shape[0])
    h = np.bincount(np.clip(a, 0, 255).astype(np.uint8).ravel(), minlength=256)
    levels = int((h > a.size * 1e-4).sum())
    return (ink, runs, levels,
            ((px1 - px0 + 1) * mmpp, (py1 - py0 + 1) * mmpp))


def refraction_ceiling(sc, footprint_png, stl_dir,
                       footprint_sidecar=None):
    """How far three.js's transmission pass CAN move a feature in this frame.

    Every term is measured off an artefact or read from the one camera claim
    this file accepts, and the formula is the renderer's own, not a textbook's
    (see the note above C4_HIGHPASS_MM):

        t        the SLAB'S OWN exported STL, z extent. Not the material's
                 declared `thickness` -- that is the number a break moves, and
                 a ceiling computed from it would rise to meet any lie told
                 about it.
        rho_max  the slab's silhouette in THIS frame, measured off mask_slab
                 in millimetres at the stated mm_per_px: the furthest any
                 transmitting pixel is from the camera's axis.
        D        |camera position - camera target|, from the sidecar.
        n        the renderer's claimed ior. The only term taken on trust, and
                 the ceiling moves by about +-10 % over n = 1.4 .. 1.6.

    Returns (ceiling_mm, explanation) or (None, why not).
    """
    cam = sc.get("camera")
    if not cam or not cam.get("position") or not cam.get("target"):
        return None, ("the sidecar records no camera position, so the "
                      "incidence angles cannot be derived -- re-run render.py")
    mat = (sc.get("materials") or {}).get("acrylic_clear") or {}
    n = mat.get("ior")
    if not n:
        return None, "the sidecar states no ior for acrylic_clear"
    stl = os.path.join(stl_dir, "slab.stl")
    if not os.path.exists(stl):
        return None, f"{os.path.relpath(stl, REPO)} does not exist"
    t = stl_facts(stl)["t"]
    #  THE MASK'S OWN SCALE, NOT THE PLAN'S. mask_slab is rendered at device
    #  pixel ratio 1 and plan at 2, so they are the same frame in millimetres
    #  and a factor of two apart in pixels. Measuring the mask's silhouette at
    #  the plan's mm_per_px halved rho and halved the ceiling with it, which
    #  is the kind of error that makes a gate quietly tighter than it should
    #  be [MEASURED: rho read 89.88 where the slab's half-diagonal is 179.76].
    msc = footprint_sidecar or sc
    mmpp = msc["view"].get("mm_per_px") or sc["view"]["mm_per_px"]
    ex = mask_extents(footprint_png)
    if ex is None:
        return None, "the slab's mask is empty"
    x0, y0, x1, y1, _cx, _cy, _n = ex
    rho = 0.5 * math.hypot((x1 - x0 + 1) * mmpp, (y1 - y0 + 1) * mmpp)
    D = math.dist(cam["position"], cam["target"])
    i = math.atan2(rho, D)
    ceil = t * math.sin(i) / n
    return ceil, (f"t {t:.3f} off slab.stl, rho {rho:.2f} off the mask, "
                  f"D {D:.1f} from the sidecar's camera, so i_max "
                  f"{math.degrees(i):.3f} deg and t*sin(i)/n = {ceil:.3f}")


# ===========================================================================
#  THE CHECKS
# ===========================================================================
def sidecar(outdir, view):
    p = os.path.join(outdir, f"{view}.json")
    if not os.path.exists(p):
        return None
    with open(p) as fh:
        return json.load(fh)


def run_checks(outdir, stl_dir=EXPORT_STL, verbose=True):
    R = Result()
    if verbose:
        print(f"  reading renders from {os.path.relpath(outdir, REPO)}")
        print(f"  reading meshes  from {os.path.relpath(stl_dir, REPO)}\n")

    # ---- C1 / C2 ---------------------------------------------------------
    for part in LASER_PARTS + PRINTED_PARTS:
        view = f"mask_{part}"
        png = os.path.join(outdir, f"{view}.png")
        stl = os.path.join(stl_dir, f"{part}.stl")
        sc = sidecar(outdir, view)
        if not (os.path.exists(png) and os.path.exists(stl) and sc):
            R.add("C1", f"{part} silhouette", False,
                  "no render / no STL / no sidecar", skipped=True)
            continue
        mmpp = sc["view"].get("mm_per_px")
        if not mmpp:
            R.add("C1", f"{part} silhouette", False,
                  "the sidecar states no mm_per_px: the view is not "
                  "orthographic and cannot be measured")
            continue
        ex = mask_extents(png)
        if ex is None:
            R.add("C1", f"{part} silhouette", False,
                  "the mask is empty: the part is not in the render")
            continue
        x0, y0, x1, y1, _wcx, _wcy, _n = ex
        _g, (W, H) = load_gray(png)
        g = stl_facts(stl)
        #  pixel span -> mm. (x1 - x0 + 1) counts both end pixels.
        w_png = (x1 - x0 + 1) * mmpp
        h_png = (y1 - y0 + 1) * mmpp
        #  BOUNDING-BOX centre, not the area centroid, because the STL side of
        #  the comparison is a bounding box too. The slab's keyboard void is
        #  41.563 mm off centre, so its area centroid is nowhere near its
        #  bbox centre and comparing one against the other would fail a
        #  correct render.
        #  +X is right; the image's +y runs DOWN and the plan view's up is the
        #  deck's +Y, so y flips.
        cx_png = ((x0 + x1) / 2.0 - (W - 1) / 2.0) * mmpp
        cy_png = -((y0 + y1) / 2.0 - (H - 1) / 2.0) * mmpp
        tol = C1_EDGE_PX * mmpp + C1_FLOOR_MM
        dw, dh = abs(w_png - g["w"]), abs(h_png - g["d"])
        dcx, dcy = abs(cx_png - g["cx"]), abs(cy_png - g["cy"])
        ok = dw <= tol and dh <= tol and dcx <= tol and dcy <= tol
        R.add("C1", f"{part} silhouette", ok,
              f"PNG {w_png:8.3f} x {h_png:7.3f} @ "
              f"({cx_png:+8.3f},{cy_png:+7.3f})  "
              f"STL {g['w']:8.3f} x {g['d']:7.3f} @ "
              f"({g['cx']:+8.3f},{g['cy']:+7.3f})  "
              f"max d {max(dw, dh, dcx, dcy):.4f} / tol {tol:.4f} mm "
              f"[{mmpp:.6f} mm/px]")

        if part in LASER_PARTS:
            holes = count_enclosed_holes(png)
            okh = holes == g["genus"]
            R.add("C2", f"{part} through-holes", okh,
                  f"PNG {holes} enclosed regions  STL genus {g['genus']} "
                  f"(euler {g['euler']}, {g['faces']} faces)")

    # ---- C3 --------------------------------------------------------------
    view = "mask_elev_deck"
    png = os.path.join(outdir, f"{view}.png")
    sc = sidecar(outdir, view)
    if not (os.path.exists(png) and sc and sc["view"].get("mm_per_px")):
        R.add("C3", "stack height", False, "no elevation mask", skipped=True)
    else:
        mmpp = sc["view"]["mm_per_px"]
        ex = mask_extents(png)
        stls = [os.path.join(stl_dir, f"{p}.stl")
                for p in LASER_PARTS + PRINTED_PARTS]
        have = [p for p in stls if os.path.exists(p)]
        if ex is None or len(have) != len(stls):
            R.add("C3", "stack height", False,
                  f"empty mask, or {len(stls) - len(have)} of the three "
                  f"fabricated STLs are missing")
        else:
            x0, y0, x1, y1, _cx, _cy, _n = ex
            h_png = (y1 - y0 + 1) * mmpp
            h_stl = sum(stl_facts(p)["t"] for p in have)
            tol = C3_EDGE_PX * mmpp + C1_FLOOR_MM
            ok = abs(h_png - h_stl) <= tol
            R.add("C3", "stack height", ok,
                  f"PNG {h_png:7.3f} mm  sum of the three STL thicknesses "
                  f"{h_stl:7.3f} mm  d {abs(h_png - h_stl):.4f} / tol "
                  f"{tol:.4f} mm -- a gap or an overlap in the stack shows "
                  f"here and nowhere else")

    # ---- C4 --------------------------------------------------------------
    with_p = os.path.join(outdir, "plan.png")
    without_p = os.path.join(outdir, "plan_noacrylic.png")
    footprint = os.path.join(outdir, "mask_slab.png")
    sc = sidecar(outdir, "plan")
    if not all(os.path.exists(p) for p in (with_p, without_p, footprint)) \
            or not sc or not sc["view"].get("mm_per_px"):
        R.add("C4", "acrylic transmission", False,
              "need plan.png, plan_noacrylic.png, mask_slab.png and a "
              "calibrated plan sidecar", skipped=True)
    else:
        got = detail_match(with_p, without_p, footprint,
                           sc["view"]["mm_per_px"])
        if got is None:
            R.add("C4", "acrylic transmission", False,
                  "empty probe region after eroding the slab's footprint")
        else:
            ratio, rho, shift, n, hp, step, pinned = got
            ceil, why = refraction_ceiling(
                sc, footprint, stl_dir, sidecar(outdir, "mask_slab"))
            lim = None if ceil is None else ceil + step
            ok = (ratio >= C4_MIN_DETAIL_RATIO
                  and rho >= C4_MIN_CORRELATION
                  and (lim is None or shift <= lim)
                  and n >= C4_MIN_TILES)
            #  An opaque slab has NO detail-bearing tiles, and that -- not the
            #  energy ratio -- is the term that catches it. A mirror-finish
            #  opaque slab still has bright edges around its own holes, so the
            #  high-frequency energy alone stays high and is not sufficient on
            #  its own; it is reported because the pair of numbers is what
            #  shows which way a failure went.
            note = ("  <- AN OPAQUE SLAB HAS NONE, WHICH IS THE POINT"
                    if n < C4_MIN_TILES else "")
            R.add("C4", "acrylic transmission", ok,
                  f"inside the slab's footprint, high-passed at "
                  f"{C4_HIGHPASS_MM:.3f} mm ({hp} px): {n} detail-bearing "
                  f"tiles (need >= {C4_MIN_TILES}){note}, median match "
                  f"r = {rho:5.3f} (need >= {C4_MIN_CORRELATION}), "
                  f"high-frequency energy {ratio:5.3f} x the un-occluded "
                  f"frame (need >= {C4_MIN_DETAIL_RATIO}); refraction "
                  f"displacement median {shift:5.3f} mm, "
                  f"over the tiles that matched; "
                  f"{pinned * 100:.0f} % of all tiles clipped by the "
                  f"{C4_SEARCH_MM:.1f} mm search window, "
                  + (f"against a DERIVED ceiling of {ceil:5.3f} + one "
                     f"{step:.4f} mm search step = {lim:5.3f} -- {why}"
                     if ceil is not None else
                     "and no ceiling could be derived: " + why))

    # ---- C5 --------------------------------------------------------------
    pngs = sorted(f for f in os.listdir(outdir) if f.endswith(".png"))
    if not pngs:
        R.add("C5", "not degenerate", False, "no PNGs at all")
    for f in pngs:
        path = os.path.join(outdir, f)
        sc = sidecar(outdir, f[:-4])
        is_mask = bool(sc) and sc["view"].get("materials") == "mask"
        if is_mask:
            ex = mask_extents(path)
            lit = 0.0 if ex is None else ex[6] / _pixels(path)
            ok = C5_MASK_LIT_MIN <= lit <= C5_MASK_LIT_MAX
            R.add("C5", f"{f} not degenerate", ok,
                  f"mask: {lit * 100:5.2f} % lit (want "
                  f"{C5_MASK_LIT_MIN * 100:.0f}..{C5_MASK_LIT_MAX * 100:.0f});"
                  f" a mask is meant to be binary, so it is judged on having "
                  f"both a part and a background, not on level count")
        else:
            frac, levels = modal_fraction_and_levels(path)
            ok = frac <= C5_MAX_MODAL_FRACTION and levels >= C5_MIN_LEVELS
            R.add("C5", f"{f} not degenerate", ok,
                  f"picture: modal colour {frac * 100:5.1f} % (max "
                  f"{C5_MAX_MODAL_FRACTION * 100:.0f}), {levels:3d} distinct "
                  f"levels (min {C5_MIN_LEVELS})")

    # ---- C6 --------------------------------------------------------------
    #  The per-body comparison below reads ONE sidecar, on the stated grounds
    #  that every sidecar carries the same body list. That was an assumption
    #  in a comment and nothing tested it, so a sha that was honest in the
    #  first sidecar and a lie in the tenth passed silently [MEASURED
    #  2026-11-07, by editing hero.json alone: 21 passed, 0 FAILED]. Assert it
    #  instead: the EXPORTED (id, sha) list of every sidecar must be identical,
    #  and only then is one of them the proof.
    lists = {}
    for f in pngs:
        sc = sidecar(outdir, f[:-4])
        if not sc:
            continue
        key = tuple((b["id"], b["sha256"]) for b in sc.get("bodies", [])
                    if b["provenance"] == "EXPORTED")
        lists.setdefault(key, []).append(f[:-4])
    if len(lists) > 1:
        groups = sorted(lists.values(), key=len, reverse=True)
        R.add("C6", "sidecars agree", False,
              f"the {len(pngs)} sidecars do not name the same exported "
              f"meshes: {len(lists)} different body lists -- "
              + " vs ".join("{" + ", ".join(g) + "}" for g in groups)
              + ". One sidecar is only the proof if they all agree.")
    elif lists:
        R.add("C6", "sidecars agree", True,
              f"all {sum(len(v) for v in lists.values())} sidecars name the "
              f"same exported meshes, so one of them is the proof")

    checked = 0
    for f in pngs:
        sc = sidecar(outdir, f[:-4])
        if not sc:
            continue
        #  ONE ROW PER PART, NOT PER BODY. Six knobs are two parts and eight
        #  placements; four feet are one part and four. Every instance of a
        #  part is staged from the same file and so carries the same hash, so
        #  fifteen rows would be eight answers and seven echoes. The instance
        #  count is printed instead, because "knob_encoder, 3 placements" is
        #  the thing a reader can check against the drawing.
        seen = {}
        for b in sc.get("bodies", []):
            if b["provenance"] != "EXPORTED":
                continue
            part = b.get("part", b["id"])
            seen.setdefault(part, []).append(b)
        for part, insts in seen.items():
            b = insts[0]
            many = (f" ({len(insts)} placements)" if len(insts) > 1 else "")
            shas = {i["sha256"] for i in insts}
            if len(shas) > 1:
                R.add("C6", f"{f[:-4]}/{part} provenance", False,
                      f"the {len(insts)} placements of {part} name "
                      f"{len(shas)} different meshes: {sorted(shas)}. They "
                      f"are staged from one file and must be one mesh.")
                continue
            stl = os.path.join(stl_dir, f"{part}.stl")
            if not os.path.exists(stl):
                R.add("C6", f"{f[:-4]}/{part} provenance", False,
                      f"the sidecar claims an exported body but "
                      f"{os.path.relpath(stl, REPO)} does not exist")
                continue
            want = sha(stl)[:16]
            checked += 1
            if want == b["sha256"]:
                #  identical bytes: identical geometry, nothing more to ask
                R.add("C6", f"{f[:-4]}/{part} provenance", True,
                      f"rendered mesh sha256 {b['sha256']} vs "
                      f"{os.path.relpath(stl, REPO)} {want}{many}")
                continue
            #  The bytes differ. That is NOT yet a failure, because
            #  export.py's carrier is not byte-reproducible: three consecutive
            #  exports of an unchanged carrier.scad gave three sha256 and one
            #  geometry [MEASURED 2026-11-07 -- see render.py geom_sha()]. So
            #  ask the question C6 actually means: is the mesh in the picture
            #  the same PART as the mesh on disk? A render.py old enough not
            #  to have recorded a geom_sha cannot answer, and fails as before.
            if not b.get("geom_sha"):
                R.add("C6", f"{f[:-4]}/{part} provenance", False,
                      f"rendered mesh sha256 {b['sha256']} vs "
                      f"{os.path.relpath(stl, REPO)} {want}, and the sidecar "
                      f"records no geom_sha, so whether the geometry changed "
                      f"cannot be told from it -- re-run render.py")
                continue
            sys.path.insert(0, HERE)
            import render as _r
            got_geom = _r.geom_sha(stl)
            same = got_geom == b["geom_sha"]
            R.add("C6", f"{f[:-4]}/{part} provenance", same,
                  (f"bytes differ (rendered {b['sha256']}, on disk {want}) "
                   f"but the GEOMETRY is identical ({got_geom}): "
                   f"{os.path.relpath(stl, REPO)} was re-exported, not "
                   f"re-designed, and the picture is still of this part"
                   if same else
                   f"the picture is of a different part: rendered geometry "
                   f"{b['geom_sha']} vs {os.path.relpath(stl, REPO)} "
                   f"{got_geom} (bytes {b['sha256']} vs {want})"))
        break   # every sidecar carries the same body list; one is the proof
    if checked == 0:
        R.add("C6", "provenance", False,
              "no sidecar named an EXPORTED body", skipped=True)

    # ---- C7 --------------------------------------------------------------
    #  THE PANEL IS SHOWING SOMETHING. A reflective LCD with nothing on it is
    #  a grey rectangle, and a grey rectangle is what this render drew for a
    #  long time while every other material was correct -- which is exactly
    #  the kind of failure that survives review, because nothing about it
    #  looks broken. So: go to the rectangle the renderer says the active area
    #  occupies and ask the PNG whether there is an image in it.
    #
    #  What this check does NOT do is verify that the panel is the right size
    #  or in the right place. It cannot: the display is a REFERENCE body,
    #  there is no exported artefact of it to measure against, and this file
    #  will not test a CAD number against itself. Its size is printed so a
    #  reader can see it, and its provenance is in the sidecar.
    sc = sidecar(outdir, "plan")
    png = os.path.join(outdir, "plan.png")
    screen = (sc or {}).get("screen") or {}
    if not (sc and os.path.exists(png) and screen.get("deck_box")
            and sc["view"].get("mm_per_px")):
        R.add("C7", "the panel is lit", False,
              "need plan.png and a sidecar whose screen names a deck_box",
              skipped=True)
    else:
        got = panel_content(png, screen["deck_box"], sc["view"]["mm_per_px"])
        if got is None:
            R.add("C7", "the panel is lit", False,
                  "the active area falls outside the frame")
        else:
            ink, runs, levels, (wmm, hmm) = got
            ok = (C7_INK_MIN <= ink <= C7_INK_MAX
                  and runs >= C7_RUNS_MIN
                  and levels >= C7_LEVELS_MIN)
            R.add("C7", "the panel is lit", ok,
                  f"the {wmm:.2f} x {hmm:.2f} mm active area carries "
                  f"{ink * 100:5.2f} % ink (want "
                  f"{C7_INK_MIN * 100:.0f}..{C7_INK_MAX * 100:.0f}), "
                  f"{runs:6.2f} dark/light transitions per row (need >= "
                  f"{C7_RUNS_MIN}) and {levels} levels (need >= "
                  f"{C7_LEVELS_MIN}) -- a panel with nothing on it has one "
                  f"tone, no transitions and no ink")
    return R


# ===========================================================================
#  --prove
# ===========================================================================
#  break mode -> the check that MUST fail, and why that break is the right
#  test of that check.
BREAKS = [
    ("scale", "C1",
     "every body's matrix scaled by 1.02, so the scene is 2 % larger than the "
     "meshes it was built from. The PNG still looks entirely plausible."),
    ("swap", "C2",
     "the slab and the back panel staged into each other's filenames. Both "
     "are 304.600 x 190.375 so C1 cannot see it; the hole counts are 14 and "
     "17 so C2 can."),
    ("z", "C3",
     "the slab lifted 6 mm, opening a gap between the carrier's rim and the "
     "acrylic. Invisible in plan, invisible in the hole count."),
    ("transmission", "C4",
     "the acrylic's transmission set to 0, i.e. the standard clear-acrylic "
     "render lie: a slab you cannot see through, rendered with every other "
     "material correct."),
    ("blank", "C5",
     "nothing drawn. This is what a lost WebGL context looks like, and it "
     "otherwise produces a clean run and a valid PNG file."),
    ("stl", "C6",
     "the staged mesh mutated after export, so the picture is of a part the "
     "fabricator never receives."),
    ("screen", "C7",
     "the panel's framebuffer blanked to one tone. Every material is still "
     "correct, the display is still in the picture, and the instrument is "
     "dead -- which is what this render drew before anything was put on the "
     "screen, and what nothing noticed."),
    ("thick", "C4",
     "the acrylic's refraction thickness tripled. It still transmits, the "
     "picture still looks plausible, and everything seen through it has "
     "moved further than a 12.700 mm slab can move it. This is the break "
     "the old C4_MAX_SHIFT_MM = 3.000 could not catch, because 3.000 was a "
     "round number and the thing it was bounding is 1.392."),
]


def prove(args):
    scratch = tempfile.mkdtemp(prefix="deckrender-prove-")
    print(f"  proving the checks fire. scratch: {scratch}\n")
    rc = 0
    try:
        #  FREEZE THE MESHES FIRST. v2/export/stl is a live directory --
        #  another author's export.py rewrites it, and it did so twice during
        #  the session this was written in, which is also how C6 earned its
        #  keep. A proof whose inputs move underneath it proves nothing, so
        #  every break below renders the SAME frozen snapshot and the checker
        #  measures against that same snapshot.
        frozen = os.path.join(scratch, "stl-frozen")
        os.makedirs(frozen, exist_ok=True)
        n = 0
        for f in sorted(os.listdir(EXPORT_STL)):
            if f.endswith(".stl"):
                shutil.copy2(os.path.join(EXPORT_STL, f),
                             os.path.join(frozen, f))
                n += 1
        if n == 0:
            print(f"  FATAL  nothing to freeze: {EXPORT_STL} has no STLs. "
                  f"Run v2/tools/export.py first.")
            return 1
        print(f"  froze {n} meshes from {os.path.relpath(EXPORT_STL, REPO)} "
              f"into the scratch tree\n")
        base = [sys.executable, os.path.join(HERE, "render.py"),
                "--views", "check", "--out", scratch,
                "--width", str(args.prove_width),
                "--height", str(args.prove_height),
                "--stl-dir", frozen, "--no-export"]
        for mode, cid, why in BREAKS:
            print(f"  --- break '{mode}' -> expects {cid} to FAIL")
            print(f"      {why}")
            stl_dir = frozen
            cmd = base + ["--break", mode]
            if mode == "stl":
                #  C6's break must not touch v2/export/stl, so the mutated
                #  copy lives in the scratch tree and the CHECKER is pointed
                #  at it while the RENDER still uses the clean frozen copy.
                #  That is exactly the condition C6 is for: the picture is of
                #  a mesh that is no longer the one on disk. A hand-edited
                #  copy, which is what the discipline asks for.
                stl_dir = os.path.join(scratch, "stl-mutated")
                os.makedirs(stl_dir, exist_ok=True)
                for f in os.listdir(frozen):
                    if f.endswith(".stl"):
                        shutil.copy2(os.path.join(frozen, f),
                                     os.path.join(stl_dir, f))
                #  MUTATE THE GEOMETRY, not the header. This break used to
                #  overwrite the 80-byte STL header, which changed the file's
                #  sha256 while leaving every triangle alone -- so it proved
                #  only that C6 could notice a byte, which is the one thing
                #  C6 must NOT fail on now that export.py is known not to be
                #  byte-reproducible. A part the fabricator never receives is
                #  a part whose GEOMETRY differs, so move a vertex: 1.000 mm,
                #  far outside the 2.44e-5 float32 slop.
                import numpy as np
                import trimesh
                victim = os.path.join(stl_dir, "slab.stl")
                m = trimesh.load(victim, force="mesh", process=False)
                v = np.asarray(m.vertices).copy()
                v[0][0] += 1.000
                m.vertices = v
                m.export(victim, file_type="stl")
                cmd = base
            r = subprocess.run(cmd, capture_output=True, text=True,
                               timeout=3600)
            if r.returncode != 0:
                print("      render.py failed:\n" + r.stdout[-2000:]
                      + r.stderr[-2000:])
                rc = 1
                continue
            R = run_checks(scratch, stl_dir=stl_dir, verbose=False)
            fails = R.failed()
            for row in R.rows:
                if row[0] == cid:
                    print(f"      {row[2]:4s} {row[0]} {row[1]}  {row[3]}")
            if cid in fails:
                print(f"      OK: {cid} fired.\n")
            else:
                print(f"      *** {cid} DID NOT FIRE. The check is not a "
                      f"check.\n")
                rc = 1

        print("  --- restored: no break, every check must pass")
        r = subprocess.run(base, capture_output=True, text=True, timeout=3600)
        if r.returncode != 0:
            print(r.stdout[-3000:] + r.stderr[-2000:])
            return 1
        R = run_checks(scratch, stl_dir=frozen, verbose=False)
        nf = R.report()
        if nf:
            print("  *** the clean render does not pass its own checks")
            rc = 1
        else:
            print("  OK: clean render passes all of them.")
    finally:
        if not args.keep:
            shutil.rmtree(scratch, ignore_errors=True)
        else:
            print(f"\n  kept {scratch}")
    return rc


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", default=OUT_DEFAULT)
    ap.add_argument("--stl", default=EXPORT_STL)
    ap.add_argument("--prove", action="store_true",
                    help="break each input, watch the check fail, restore")
    ap.add_argument("--prove-width", type=int, default=1000)
    ap.add_argument("--prove-height", type=int, default=700)
    ap.add_argument("--keep", action="store_true",
                    help="keep the --prove scratch directory")
    args = ap.parse_args()
    if args.prove:
        return prove(args)
    if not os.path.isdir(args.out):
        print(f"  FATAL  no renders at {args.out}; run render.py first")
        return 1
    R = run_checks(args.out, stl_dir=args.stl)
    nf = R.report()
    print("\n  WHAT THESE CHECKS CANNOT SEE, and must not be taken to cover:")
    print("    * whether the render is PRETTY. Nothing here measures that.")
    print("    * whether the REFERENCE and VENDOR bodies match the real")
    print("      components -- there is no artefact to measure them against.")
    print("      Their provenance is in each view's sidecar JSON; the three")
    print("      classes are not interchangeable and the render says so.")
    print("    * kerf, taper, warp, flatness, the slab's as-bought thickness:")
    print("      v2/docs/fabrication.md 4.6 lists them and they are not in")
    print("      any file, let alone in a picture.")
    return 1 if nf else 0


if __name__ == "__main__":
    sys.exit(main())
