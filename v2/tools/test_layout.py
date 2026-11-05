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
    p = L.Panel(200.0, 160.0, module=5.0, title="TEST")
    p.add(L.display(0.0, 40.0))
    for i, x in enumerate((-25.0, 0.0, 25.0)):
        p.add(L._ctl("knob", x, -20.0, 16.0, 16.0, f"POT{i+1}", kw=19.0, kh=19.0))
    p.add(L._ctl("slider", -35.0, -50.0, 50.0, 10.0, "SLIDE L"))
    p.add(L._ctl("slider", 35.0, -50.0, 50.0, 10.0, "SLIDE R"))
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

    # and the spec-level checks
    cases = [
        ("off the grid", lambda p: p.add(L._ctl("knob", -3.3, -70.0, 16, 16, "X")), "OFF"),
        ("two knobs too close", lambda p: p.add(
            L._ctl("knob", -60.0, -20.0, 16, 16, "L"),
            L._ctl("knob", 60.0, -20.0, 16, 16, "R"),
            L._ctl("knob", -45.0, -20.0, 16, 16, "L2"),
            L._ctl("knob", 45.0, -20.0, 16, 16, "R2")), "OVERLAP"),
        ("outside the body", lambda p: p.add(
            L._ctl("key", 95.0, 60.0, 18, 18, "W"),
            L._ctl("key", -95.0, 60.0, 18, 18, "W2")), "OFF"),
        ("no mirror partner", lambda p: p.add(
            L._ctl("knob", 70.0, -20.0, 16, 16, "LONE")), "ASYMMETRY"),
    ]
    for name, mutate, want in cases:
        q = clean()
        mutate(q)
        bad += not expect(name, q.audit(q.svg()), want)

    print(f"  {'ALL PASS' if not bad else str(bad) + ' CHECK(S) BLIND'}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
