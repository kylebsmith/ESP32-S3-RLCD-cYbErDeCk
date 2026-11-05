#!/usr/bin/env python3
"""Prove layout.py's checks fire, including on a defect only in the DRAWING.

v1 taught this the hard way three times: a check that computes from the same
parameters as the thing it checks is worse than no check, because it passes
while the artefact is wrong. layout.py's audit() therefore parses the emitted
SVG. This file is the proof that it does -- it corrupts the SVG text, leaves
the spec untouched, and requires the audit to notice.

Usage:  python3 v2/tools/test_layout.py
"""
import re
import sys

sys.path.insert(0, __file__.rsplit("/", 1)[0])
import layout as L  # noqa: E402


def clean():
    p = L.Panel(200.66, 160.02, title="TEST")
    p.add(L.display(0.0, 40.005))
    for i, x in enumerate((-25.4, 0.0, 25.4)):
        p.add(L._ctl("knob", x, -20.32, 16.51, 16.51, f"POT{i+1}",
                     kw=19.05, kh=19.05))
    p.add(L._ctl("slider", -35.56, -50.8, 50.8, 10.16, "SLIDE L"))
    p.add(L._ctl("slider", 35.56, -50.8, 50.8, 10.16, "SLIDE R"))
    return p


def plated():
    """A plate carrying three Pattern B bays, with controls sitting on them."""
    p = L.Panel(240.0, 200.0, title="PLATED")
    p.add(L.display(0.0, 62.23))
    p.add(L.plate(0.0, -10.16, 190.5, 76.2))
    for i, y in enumerate((13.97, -10.16, -34.29)):
        p.add(L.bay(0.0, y, f"BAY{i+1}"))
    # Four encoders at the vendor's own 19.05 pitch. The knob is 13.00 and not
    # 14.00 because 19.05 - 6.00 finger gap = 13.05 is the whole budget, and
    # MIL-STD-1472H's minimum knob is 13 mm. The window is 0.05 mm wide.
    for i, x in enumerate((-28.575, -9.525, 9.525, 28.575)):
        p.add(L._ctl("encoder", x, 13.97, 13.0, 13.0, f"ENC{i+1}"))
    return p


def expect(name, fails, want):
    got = [f.split()[0] for f in fails]
    ok = want in got
    print(f"  {'ok  ' if ok else 'FAIL'}  {name:34s} -> {', '.join(got) or 'nothing'}")
    return ok


def main():
    bad = 0

    p = clean()
    text = p.svg()
    f = p.audit(text)
    if f:
        print(f"  FAIL  a clean layout must pass; got {f}")
        bad += 1
    else:
        print("  ok    a clean layout passes")

    # the one that matters: the drawing lies, the spec does not.
    moved = re.sub(r'(data-label="POT2" data-role="touch" cx=")([0-9.]+)',
                   lambda m: m.group(1) + f"{float(m.group(2)) + 4.0:.4f}",
                   text, count=1)
    if moved == text:
        print("  FAIL  the test's own substitution missed -- fix the test")
        bad += 1
    else:
        bad += not expect("a knob drawn 4 mm off", p.audit(moved), "MISDRAWN")

    # the frame is measured too: shrink the body rect only.
    shrunk = re.sub(r'(data-kind="body"[^/]*?width=")([0-9.]+)',
                    lambda m: m.group(1) + f"{float(m.group(2)) - 10.0:.4f}",
                    text, count=1)
    bad += not expect("the body rect drawn 10 mm narrow", p.audit(shrunk), "BODY")

    # a control dropped from the drawing
    dropped = re.sub(r'<circle data-kind="knob" data-label="POT1"[^/]*/>', "",
                     text, count=1)
    bad += not expect("a control not drawn at all", p.audit(dropped), "MISSING")

    # --- the deck identity must reproduce v1, or it is not the repo's rule ---
    w, h, _ = L.deck(108.50, 58.20)
    if abs(w - 116.25) > 1e-9 or abs(h - 140.25) > 1e-9:
        print(f"  FAIL  deck() gives {w:.2f} x {h:.2f}, v1 is 116.25 x 140.25")
        bad += 1
    else:
        print("  ok    deck() reproduces the v1 deck from the v1 keyboard")

    w, h, d = L.deck(287.00, 97.50, 0.0)
    if abs(w - 294.75) > 1e-9 or abs(h - 182.75) > 1e-9:
        print(f"  FAIL  deck() gives {w:.2f} x {h:.2f} for the K9 Pro floor")
        bad += 1
    else:
        print("  ok    deck() gives 294.75 x 182.75 as the K9 Pro floor")

    # the band datums have to bracket the band, in order, in model space
    w, h, d = L.deck(287.00, 97.50, 71.4375)
    if not (d["kbd_top"] < d["band_lo"] < d["band_hi"]
            and abs((d["band_hi"] - d["band_lo"]) - 71.4375) < 1e-9
            and abs((d["band_lo"] - d["kbd_top"]) - L.SPINE) < 1e-9):
        print(f"  FAIL  band datums out of order or wrong span: {d}")
        bad += 1
    else:
        print("  ok    band datums bracket the band, one spine off the keys")

    # --- the plate and the vendor grid ---
    q = plated()
    f = q.audit(q.svg())
    if f:
        print(f"  FAIL  a clean plated layout must pass; got {f}")
        bad += 1
    else:
        print("  ok    a plate carrying bays carrying controls passes")
        print("  ok    a control on a bay is not an overlap (different layers)")

    q = plated()
    q.add(L._ctl("knob", -90.17, -10.16, 16, 16, "HANGL"),
          L._ctl("knob", 90.17, -10.16, 16, 16, "HANGR"))
    bad += not expect("a control overhanging the plate", q.audit(q.svg()), "OFF")

    q = L.Panel(240.0, 200.0, title="SHIFTED")
    q.add(L.plate(9.525, -10.16, 190.5, 76.2))
    bad += not expect("a plate off the centreline", q.audit(q.svg()), "OFF")

    q = plated()
    q.add(L.bay(0.0, 20.32, "BAYCLASH"))
    bad += not expect("two bays overlapping", q.audit(q.svg()), "OVERLAP")

    q = plated()
    q.items = [it for it in q.items if it["kind"] != "display"]
    q.add(L.display(0.0, 28.575))        # dropped until it touches the plate
    bad += not expect("the display running into the plate", q.audit(q.svg()),
                      "OVERLAP")

    # and the spec-level checks
    cases = [
        ("off the grid", lambda p: p.add(
            L._ctl("knob", -3.3, -69.85, 16.51, 16.51, "X")), "OFF"),
        ("two knobs too close", lambda p: p.add(
            L._ctl("knob", -60.96, -20.32, 16.51, 16.51, "L"),
            L._ctl("knob", 60.96, -20.32, 16.51, 16.51, "R"),
            L._ctl("knob", -45.72, -20.32, 16.51, 16.51, "L2"),
            L._ctl("knob", 45.72, -20.32, 16.51, 16.51, "R2")), "OVERLAP"),
        ("outside the body", lambda p: p.add(
            L._ctl("key", 95.25, 60.96, 18.0, 18.0, "W"),
            L._ctl("key", -95.25, 60.96, 18.0, 18.0, "W2")), "OFF"),
        ("no mirror partner", lambda p: p.add(
            L._ctl("knob", 69.85, -20.32, 16.51, 16.51, "LONE")), "ASYMMETRY"),
    ]
    for name, mutate, want in cases:
        q = clean()
        mutate(q)
        bad += not expect(name, q.audit(q.svg()), want)

    print(f"  {'ALL PASS' if not bad else str(bad) + ' CHECK(S) BLIND'}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
