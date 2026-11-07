#!/usr/bin/env python3
"""Set check_render.py's thresholds from MEASUREMENTS instead of from one look.

    /usr/bin/python3.11 v2/tools/render/calibrate.py
    /usr/bin/python3.11 v2/tools/render/calibrate.py --keep

WHY THIS FILE EXISTS
--------------------
check_render.py's C4, C5 and C7 thresholds were each a line drawn once, from a
single known-good render, and tagged [DESIGN] because that is what they were:
nine numbers nobody had measured anything against. The README listed them under
NOT SOURCED with the procedure that would close the entry --

    "rendering a set of known-good and known-bad variants and setting each
     threshold from the separation between the two distributions, rather than
     from one sample"

-- and this file is that procedure, so the entry closes by being carried out
rather than by being argued with.

WHAT A "GOOD" VARIANT IS
------------------------
Not "the shipped render again". A threshold has to tolerate every render this
tool legitimately produces, so the good set varies the things a person varies:
the frame size, the device pixel ratio, the tone-mapping exposure and the
reference bodies' faceting. All of them are correct pictures of the same deck,
and a threshold that any of them fails is a threshold that will fire on
somebody doing nothing wrong.

WHAT A "BAD" VARIANT IS
-----------------------
The failure each check exists to catch, produced two ways. Through render.py's
own --break switches, which make a knowingly wrong RENDER; and by mutating the
emitted PNG, which makes a knowingly wrong IMAGE -- a frosted slab and a flat
frame are degradations no break switch produces and both are things a broken
renderer really does.

THE RULE FOR PLACING A THRESHOLD, stated once and applied everywhere
--------------------------------------------------------------------
For a FLOOR (the metric must be at least t): if the worst good value and the
best bad value are both positive, t is their GEOMETRIC mean -- these are
ratios and correlations, so the scale is multiplicative and the midpoint of a
ratio is its geometric midpoint. If the bad side is exactly zero the two
distributions are infinitely separated and any positive floor divides them; t
is then a quarter of the worst good value, and the choice is headroom for a
view nobody has rendered yet rather than discrimination.

For a CEILING, the arithmetic midpoint of the best good and the worst bad.

Every threshold is printed with both distributions and the margin either side,
so a number that has drifted toward its gate is visible before it fires.
"""
import argparse
import json
import math
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import check_render as C          # noqa: E402  (its measurement functions)

EXPORT_STL = os.path.join(os.path.dirname(HERE), "..", "export", "stl")
EXPORT_STL = os.path.normpath(os.path.join(HERE, "..", "..", "export", "stl"))

VIEWS = ("mask_slab,mask_backpanel,mask_carrier,mask_elev_deck,"
         "mask_plan,mask_elev,plan,plan_noacrylic,hero")

#  The good set. Every one of these is a correct render of the same deck.
GOOD = [
    ("shipped",  []),
    ("small",    ["--width", "1000", "--height", "700", "--dpr", "1"]),
    ("wide",     ["--width", "1280", "--height", "800"]),
    ("dim",      ["--exposure", "0.62"]),
    ("bright",   ["--exposure", "0.95"]),
    ("coarse",   ["--fn", "48"]),
]

#  The bad set, through render.py's own break switches.
BAD = [
    ("opaque",   ["--break", "transmission"],
     "the acrylic's transmission set to 0"),
    ("blank",    ["--break", "blank"], "nothing drawn at all"),
    ("screenoff", ["--break", "screen"], "the panel blanked to one tone"),
    ("thick",    ["--break", "thick"],
     "the acrylic's refraction thickness tripled"),
]


def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=3600)
    if r.returncode != 0:
        print(r.stdout[-2000:] + r.stderr[-2000:])
        raise SystemExit(f"render failed: {' '.join(cmd[-6:])}")


def render(out, extra, frozen):
    run([sys.executable, os.path.join(HERE, "render.py"),
         "--views", VIEWS, "--out", out, "--stl-dir", frozen,
         "--no-export"] + extra)


def mutate(src, dst, how):
    """A knowingly wrong IMAGE, by ImageMagick, from a good render."""
    shutil.copytree(src, dst)
    if how == "frosted3":
        #  a FROSTED slab, not an opaque one: the detail survives as energy
        #  and stops being registered to anything.
        run(["convert", os.path.join(dst, "plan.png"), "-blur", "0x3",
             os.path.join(dst, "plan.png")])
    elif how == "frosted6":
        run(["convert", os.path.join(dst, "plan.png"), "-blur", "0x6",
             os.path.join(dst, "plan.png")])
    elif how == "flat":
        for v in ("hero", "plan", "plan_noacrylic"):
            run(["convert", os.path.join(dst, f"{v}.png"), "-fill", "#20242a",
                 "-colorize", "100", os.path.join(dst, f"{v}.png")])
    elif how == "maskfull":
        #  a mask that lost its background: every pixel lit.
        for v in ("mask_slab", "mask_plan", "mask_elev_deck"):
            run(["convert", os.path.join(dst, f"{v}.png"), "-fill", "white",
                 "-colorize", "100", os.path.join(dst, f"{v}.png")])


MUTATIONS = [
    ("frosted3", "plan.png blurred 0x3 -- a frosted slab, not an opaque one"),
    ("frosted6", "plan.png blurred 0x6"),
    ("flat", "the pictures flattened to one colour"),
    ("maskfull", "the masks painted fully lit -- a mask with no background"),
]


def measure(outdir, frozen):
    """Every metric check_render.py gates on, off one render directory."""
    m = {}
    sc = C.sidecar(outdir, "plan")
    mm = sc["view"]["mm_per_px"]
    got = C.detail_match(os.path.join(outdir, "plan.png"),
                         os.path.join(outdir, "plan_noacrylic.png"),
                         os.path.join(outdir, "mask_slab.png"), mm)
    if got:
        ratio, rho, shift, n, hp, step, pinned = got
        ceil, _ = C.refraction_ceiling(sc, os.path.join(outdir,
                                                        "mask_slab.png"),
                                       frozen, C.sidecar(outdir, "mask_slab"))
        m["c4_ratio"] = ratio
        m["c4_r"] = rho
        m["c4_tiles"] = float(n)
        #  the displacement as a MULTIPLE of its own derived ceiling, which is
        #  what makes it comparable between variants whose cameras differ
        m["c4_shift_over_ceiling"] = (shift / ceil) if ceil else float("nan")
    pics, masks = [], []
    for f in sorted(os.listdir(outdir)):
        if not f.endswith(".png"):
            continue
        s2 = C.sidecar(outdir, f[:-4])
        p = os.path.join(outdir, f)
        if s2 and s2["view"].get("materials") == "mask":
            ex = C.mask_extents(p)
            masks.append(0.0 if ex is None else ex[6] / C._pixels(p))
        else:
            frac, lv = C.modal_fraction_and_levels(p)
            pics.append((frac, lv))
    if pics:
        m["c5_modal"] = max(p[0] for p in pics)
        m["c5_levels"] = float(min(p[1] for p in pics))
    if masks:
        m["c5_lit_min"] = min(masks)
        m["c5_lit_max"] = max(masks)
    screen = (sc or {}).get("screen") or {}
    if screen.get("deck_box"):
        g = C.panel_content(os.path.join(outdir, "plan.png"),
                            screen["deck_box"], mm)
        if g:
            m["c7_ink"], m["c7_runs"], lv, _sz = g
            m["c7_levels"] = float(lv)
    return m


#  WHICH FAILURES EACH THRESHOLD IS RESPONSIBLE FOR, and this is the part that
#  has to be argued rather than assumed. C4 is a CONJUNCTION: it fails if any
#  of its terms fails, so each term only has to divide the failures it is the
#  right term for, and what guarantees nothing slips through is
#  check_render.py --prove, which watches every break fire. Asking one term to
#  divide a failure another term catches by a factor of four makes it tighter
#  than it has any reason to be -- and here it would make it WRONG: a 0x3
#  blur correlates at 0.715, which is inside the good set's 0.659..0.874, so
#  no correlation floor can separate it. The detail ratio separates it at
#  0.322 against 0.794. Every bad variant is printed against every threshold
#  below, with the ones in its dividing set marked, so an exclusion is
#  visible rather than quiet.
#
#  (metric, sense, the bad variants this threshold must divide, constant, what)
#  sense "floor": good is HIGH, bad is LOW.  "ceiling": good is LOW.
RULES = [
    #  the FROSTED slab is this term's job: an opaque one still has bright
    #  edges round its own holes and keeps most of its high-frequency energy,
    #  which is why it is caught by the tile count instead.
    ("c4_ratio", "floor", ("frosted3", "frosted6"),
     "C4_MIN_DETAIL_RATIO", "high-frequency energy through the slab"),
    #  the OPAQUE slab and a heavy blur. A light blur is not separable here
    #  and is not asked to be.
    ("c4_r", "floor", ("opaque", "frosted6"),
     "C4_MIN_CORRELATION", "median per-tile match"),
    ("c4_tiles", "floor", ("opaque",),
     "C4_MIN_TILES", "detail-bearing tiles"),
    ("c5_modal", "ceiling", ("flat",),
     "C5_MAX_MODAL_FRACTION", "modal colour share of a picture"),
    ("c5_levels", "floor", ("flat",),
     "C5_MIN_LEVELS", "distinct luma levels in a picture"),
    ("c5_lit_min", "floor", ("blank",),
     "C5_MASK_LIT_MIN", "lit share of a mask"),
    ("c5_lit_max", "ceiling", ("maskfull",),
     "C5_MASK_LIT_MAX", "lit share of a mask"),
    ("c7_ink", "floor", ("screenoff",),
     "C7_INK_MIN", "ink share of the active area"),
    ("c7_ink", "ceiling", ("screenoff_inv",),
     "C7_INK_MAX", "ink share of the active area"),
    ("c7_runs", "floor", ("screenoff",),
     "C7_RUNS_MIN", "dark/light transitions per row of the panel"),
    ("c7_levels", "floor", ("screenoff",),
     "C7_LEVELS_MIN", "luma levels in the panel"),
]


def place(sense, good, bad):
    if sense == "floor":
        wg, bb = min(good), max(bad)
        if bb <= 0:
            return wg / 4.0, "bad is exactly 0: any positive floor divides " \
                             "them, so this is a quarter of the worst good"
        return math.sqrt(wg * bb), "geometric mean of the worst good and " \
                                   "the best bad"
    bg, wb = max(good), min(bad)
    return (bg + wb) / 2.0, "arithmetic midpoint of the best good and the " \
                            "worst bad"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--width", default="1000")
    ap.add_argument("--height", default="700")
    args = ap.parse_args()
    scratch = tempfile.mkdtemp(prefix="deckcal-")
    print(f"  calibrating in {scratch}\n")
    try:
        #  FREEZE THE MESHES, for --prove's reason: v2/export/stl is live and
        #  a study whose inputs move underneath it measures two things.
        frozen = os.path.join(scratch, "stl")
        os.makedirs(frozen)
        for f in sorted(os.listdir(EXPORT_STL)):
            if f.endswith(".stl"):
                shutil.copy2(os.path.join(EXPORT_STL, f),
                             os.path.join(frozen, f))
        base = ["--width", args.width, "--height", args.height]

        runs = {}
        for name, extra in GOOD:
            out = os.path.join(scratch, "good_" + name)
            render(out, base + extra, frozen)
            runs[name] = measure(out, frozen)
            print(f"  good {name:10s} {json.dumps(runs[name])}")
        for name, extra, _why in BAD:
            out = os.path.join(scratch, "bad_" + name)
            render(out, base + extra, frozen)
            runs[name] = measure(out, frozen)
            print(f"  BAD  {name:10s} {json.dumps(runs[name])}")
        for name, _why in MUTATIONS:
            out = os.path.join(scratch, "bad_" + name)
            mutate(os.path.join(scratch, "good_shipped"), out, name)
            runs[name] = measure(out, frozen)
            print(f"  BAD  {name:10s} {json.dumps(runs[name])}")
        #  A panel blanked to INK rather than to paper: the other end of
        #  C7_INK_MAX. Produced from the blanked render by inverting the
        #  active area, which is the same failure wearing the other colour.
        src = os.path.join(scratch, "bad_screenoff")
        dst = os.path.join(scratch, "bad_screenoff_inv")
        shutil.copytree(src, dst)
        run(["convert", os.path.join(dst, "plan.png"), "-negate",
             os.path.join(dst, "plan.png")])
        runs["screenoff_inv"] = measure(dst, frozen)
        print(f"  BAD  {'screenoff_inv':10s} "
              f"{json.dumps(runs['screenoff_inv'])}")

        goods = [n for n, _ in GOOD]
        print("\n" + "=" * 78)
        print("  THRESHOLD                 good (worst..best)   "
              "bad (worst..best)    set to")
        print("=" * 78)
        out_lines = []
        for metric, sense, bads, const, what in RULES:
            g = [runs[n][metric] for n in goods if metric in runs[n]]
            b = [runs[n][metric] for n in bads
                 if n in runs and metric in runs[n]]
            if not g or not b:
                print(f"  {const:25s} NO DATA")
                continue
            t, why = place(sense, g, b)
            print(f"  {const:25s} {min(g):8.4f}..{max(g):8.4f}  "
                  f"{min(b):8.4f}..{max(b):8.4f}  {t:9.4f}")
            #  every bad variant, in or out of the dividing set, so that an
            #  exclusion is on the page
            marks = []
            for n, *_ in BAD + [(m, None) for m, _ in MUTATIONS] \
                    + [("screenoff_inv", None)]:
                if n not in runs or metric not in runs[n]:
                    continue
                v = runs[n][metric]
                sep = (v <= t) if sense == "floor" else (v >= t)
                marks.append(f"{'*' if n in bads else ' '}{n} {v:.4f}"
                             f"{'' if sep else ' (NOT separated)'}")
            print(f"      {' | '.join(marks)}")
            out_lines.append((const, t, sense, g, b, why, what, bads))
        print("=" * 78)
        print("\n  the block to paste into check_render.py:\n")
        for const, t, sense, g, b, why, what, bads in out_lines:
            v = (f"{t:.4f}" if abs(t) < 100 else f"{t:.1f}")
            if const in ("C4_MIN_TILES", "C5_MIN_LEVELS", "C7_LEVELS_MIN"):
                v = str(int(math.floor(t)))
            print(f"{const} = {v}   # [MEASURED] {what}: good "
                  f"{min(g):.4f}..{max(g):.4f} over {len(g)} renders, bad "
                  f"{min(b):.4f}..{max(b):.4f} over {len(b)} "
                  f"({', '.join(bads)}); {why}")
    finally:
        if args.keep:
            print(f"\n  kept {scratch}")
        else:
            shutil.rmtree(scratch, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
