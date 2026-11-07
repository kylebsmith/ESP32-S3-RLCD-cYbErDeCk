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
import xml.etree.ElementTree as ET

sys.path.insert(0, __file__.rsplit("/", 1)[0])
import layout as L  # noqa: E402

# ---------------------------------------------------------------------------
# The keyboard's pitch, and the control pitch it is NOT.
#
# These two literals are the whole point of switch_field() below, and they are
# deliberately NOT imported from layouts/_common.py, from the spec under test,
# or from layout.py. A check that asks the drawing what pitch it used cannot
# see a wrong pitch -- which is exactly how this bug lived in five files: the
# field was 15 x 19.05 and the lattice was drawn on a hard-coded 19.05, so the
# drawing agreed with itself and the SVG looked right.
#
# 19.000 is measured: all 61 of the K9 Pro's switch holes land on an exact
# 19.000 mm lattice in BOTH axes, read off the analytic hole axes in Keychron's
# own assembly, agreeing with Keychron's own QMK LAYOUT_61_ansi to 0.000 on the
# 1 u keys. 19.050 is the MX key unit and is still right for the CONTROL field,
# which is why the two have to be told apart rather than reconciled.
KBD_PITCH = 19.000              # [CAD-EXACT] v2/parts/keyboard-strip.md 1.6
CTRL_PITCH = 19.050             # [VENDOR] 0.75 in, the MX unit, Adafruit's grid
# The field's EXTENT in pitches, and it is checked as well as the pitch,
# because the pitch alone does not pin the field down: a field drawn
# 285.000 x 76.000 is on a perfect 19.000 lattice in both axes and spans a
# whole number of pitches in both, and it has lost the bottom key row -- an
# aperture cut to it would clip the spacebar row entirely. 1.6 gives five row
# axes at Z 19 / 38 / 57 / 76 / 95 and an X column reaching 263.650, which is
# inside 15 u and outside 14.
KBD_COLS, KBD_ROWS = 15, 5      # [CAD-EXACT] v2/parts/keyboard-strip.md 1.6
# px()/py() round to 4 decimals, and the two pitches are 0.050 apart, so a
# tolerance three orders of magnitude inside that gap still separates them.
PITCH_TOL = 0.002


def switch_field(name, svg_text, require_bay=False):
    """Measure the DRAWN switch lattice off the SVG and check its pitch.

    This reads the artefact and nothing else: the frame comes from the body
    rect, the field from the rect the drawing tagged data-kind="switch-field",
    and the lattice from the lines it tagged switch-col / switch-row. Those
    carry no data-label, so they are invisible to measure() and cannot become
    controls the audit expects -- they exist so the pitch can be measured
    rather than trusted.

    A keyboard bay in a v2 layout must draw its field: the field, not the bay,
    is what an aperture is cut to, and an undrawn field is an unchecked one.

    require_bay closes the hole that sentence otherwise only asserted. Called
    on a panel built by the test itself, a missing bay means nothing -- most of
    those panels have no keyboard. Called on a v2 DECK SPEC it means the spec
    stopped drawing the keyboard, and returning [] for that is the project's
    own failure mode in miniature: the pitch check goes silent at exactly the
    moment there is no pitch left to check, and the suite reads ALL PASS. So
    the spec loop passes require_bay=True and a bayless deck is red.
    """
    root = ET.fromstring(svg_text)
    els = list(root.iter())
    body = next((e for e in els if e.get("data-kind") == "body"), None)
    if body is None:
        return [f"PITCH     {name}: no body rect, nothing to measure from"]
    bw, bh = float(body.get("width")), float(body.get("height"))
    ox = float(body.get("x")) + bw / 2
    oy = float(body.get("y")) + bh / 2

    bays = [e for e in els if e.get("data-kind") == "keyboard"]
    fields = [e for e in els if e.get("data-kind") == "switch-field"]
    if not bays:
        if require_bay:
            return [f"FIELD     {name}: no keyboard bay is drawn at all, so "
                    f"there is no pitch to measure -- a v2 deck layout is "
                    f"sized from the K9 Pro and must draw its bay"]
        return []                      # no keyboard drawn, nothing to measure
    if not fields:
        return [f"PITCH     {name}: a keyboard bay is drawn with no switch "
                f"field, so its pitch cannot be measured"]

    cols = [float(e.get("x1")) - ox for e in els
            if e.get("data-kind") == "switch-col"]
    rows = [oy - float(e.get("y1")) for e in els
            if e.get("data-kind") == "switch-row"]

    fail = []
    for f in fields:
        x0 = float(f.get("x")) - ox
        x1 = x0 + float(f.get("width"))
        y1 = oy - float(f.get("y"))
        y0 = y1 - float(f.get("height"))

        # the field has to be inside the bay it belongs to, or it is measuring
        # a lattice that is not over the keys
        bay = min(bays, key=lambda b: abs((float(b.get("x"))
                                           + float(b.get("width")) / 2) - ox
                                          - (x0 + x1) / 2))
        bx0 = float(bay.get("x")) - ox
        bx1 = bx0 + float(bay.get("width"))
        by1 = oy - float(bay.get("y"))
        by0 = by1 - float(bay.get("height"))
        if x0 < bx0 - PITCH_TOL or x1 > bx1 + PITCH_TOL \
                or y0 < by0 - PITCH_TOL or y1 > by1 + PITCH_TOL:
            fail.append(f"FIELD     {name}: the drawn switch field "
                        f"{x0:+.3f}..{x1:+.3f} / {y0:+.3f}..{y1:+.3f} is not "
                        f"inside the bay {bx0:+.3f}..{bx1:+.3f} / "
                        f"{by0:+.3f}..{by1:+.3f}")

        for axis, lo, hi, cut in (("x", x0, x1, cols), ("y", y0, y1, rows)):
            inner = sorted(v for v in cut if lo - PITCH_TOL < v < hi + PITCH_TOL)
            edges = [lo] + inner + [hi]
            steps = [b - a for a, b in zip(edges, edges[1:])]
            for i, step in enumerate(steps):
                if abs(step - KBD_PITCH) <= PITCH_TOL:
                    continue
                why = (" -- that is the CONTROL pitch, not the keyboard's"
                       if abs(step - CTRL_PITCH) <= PITCH_TOL else "")
                fail.append(f"PITCH     {name}: drawn lattice step {i} in "
                            f"{axis} measures {step:.4f}, want "
                            f"{KBD_PITCH:.3f}{why}")
            span = hi - lo
            n = len(steps)
            if abs(span - n * KBD_PITCH) > PITCH_TOL:
                fail.append(f"FIELD     {name}: drawn switch field {axis} span "
                            f"{span:.4f} is not {n} x {KBD_PITCH:.3f} = "
                            f"{n * KBD_PITCH:.3f}")
            want_n = KBD_COLS if axis == "x" else KBD_ROWS
            if n != want_n:
                fail.append(f"FIELD     {name}: drawn switch field spans {n} "
                            f"pitch(es) in {axis}, want {want_n} -- the K9 "
                            f"Pro's field is {KBD_COLS} u x {KBD_ROWS} u "
                            f"({KBD_COLS * KBD_PITCH:.3f} x "
                            f"{KBD_ROWS * KBD_PITCH:.3f})")
    return fail


def keyed(pitch=KBD_PITCH, field=None):
    """A panel that is nothing but a keyboard bay and its drawn switch field.

    The body, the walls and the pocket are the real deck's (parameters.scad
    section 2 and 8c: 304.600 x 190.375, walls 8.125 / 4.400, pocket
    288.350 x 98.450 centred at y = -41.5625) so the field sits in the bay it
    really sits in, rather than in a frame invented for the test. -41.5625 is
    the pocket seated on the front wall: -190.375/2 + 4.400 + 98.450/2.
    """
    p = L.Panel(304.600, 190.375, title="KEYED", wall=8.125, bottom_wall=4.40)
    p.add(L.keyboard(0.0, -41.5625, 288.350, 98.450, "K9 PRO BAY",
                     field=field or (15 * pitch, 5 * pitch), pitch=pitch))
    return p


def clean():
    p = L.Panel(200.66, 160.02, title="TEST", wall=3.20, bottom_wall=3.20)
    p.add(L.display(0.0, 40.005))
    for i, x in enumerate((-25.4, 0.0, 25.4)):
        p.add(L._ctl("knob", x, -20.32, 16.51, 16.51, f"POT{i+1}",
                     kw=19.05, kh=19.05))
    p.add(L._ctl("slider", -35.56, -50.8, 50.8, 10.16, "SLIDE L"))
    p.add(L._ctl("slider", 35.56, -50.8, 50.8, 10.16, "SLIDE R"))
    return p


def plated():
    """A plate carrying three Pattern B bays, with controls sitting on them."""
    p = L.Panel(240.0, 200.0, title="PLATED", wall=3.20, bottom_wall=3.20)
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
    w, h, _ = L.deck(108.50, 58.20, wall=3.20)
    if abs(w - 116.25) > 1e-9 or abs(h - 140.25) > 1e-9:
        print(f"  FAIL  deck() gives {w:.2f} x {h:.2f}, v1 is 116.25 x 140.25")
        bad += 1
    else:
        print("  ok    deck() reproduces the v1 deck from the v1 keyboard")

    w, h, d = L.deck(287.00, 97.50, 0.0, wall=3.20)
    if abs(w - 294.75) > 1e-9 or abs(h - 182.75) > 1e-9:
        print(f"  FAIL  deck() gives {w:.2f} x {h:.2f} for the K9 Pro floor")
        bad += 1
    else:
        print("  ok    deck() gives 294.75 x 182.75 as the K9 Pro floor")

    # the band datums have to bracket the band, in order, in model space
    w, h, d = L.deck(287.00, 97.50, 71.4375, wall=3.20)
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

    q = L.Panel(240.0, 200.0, title="SHIFTED", wall=3.20, bottom_wall=3.20)
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

    # --- the wiring, which is a visible surface and so is checked ---
    def wired(route_pts, w=4.0, extra=None):
        q = L.Panel(240.0, 200.0, title="WIRED", y_datum=-10.16,
                    wall=3.20, bottom_wall=3.20)
        q.add(L.plate(0.0, -10.16, 190.5, 76.2))
        q.add(L.bay(0.0, -10.16, "_board", pattern="A"))
        q.add(L.route("RUN", route_pts, w=w))
        if extra:
            extra(q)
        return q

    q = wired([(-50.80, -40.64), (-50.80, -27.94), (-25.40, -27.94)])
    f = q.audit(q.svg())
    if f:
        print(f"  FAIL  a clean orthogonal run must pass; got {f}")
        bad += 1
    else:
        print("  ok    an orthogonal, on-grid run inside the plate passes")

    bad += not expect("a diagonal channel", wired(
        [(-50.80, -40.64), (-25.40, -27.94)]).audit(
        wired([(-50.80, -40.64), (-25.40, -27.94)]).svg()), "DIAGONAL")

    q = wired([(-50.80, -40.64), (-50.80, -27.94), (-25.41, -27.94)])
    bad += not expect("an off-grid vertex", q.audit(q.svg()), "OFF")

    q = wired([(-50.80, -46.99), (-25.40, -46.99)])
    bad += not expect("a channel overhanging the plate", q.audit(q.svg()), "OFF")

    # a run that crosses under a board without ending there is invisible
    q = wired([(-25.40, -10.16), (25.40, -10.16)])
    bad += not expect("a channel buried under a board", q.audit(q.svg()), "BURIED")

    # ...but one that ENDS at the board is how a cable legitimately arrives
    q = wired([(-25.40, -10.16), (-12.70, -10.16)])
    f = [x for x in q.audit(q.svg()) if x.startswith("BURIED")]
    if f:
        print(f"  FAIL  a run ending at a board must not read as buried; {f}")
        bad += 1
    else:
        print("  ok    a run ending at a board is not buried")

    # --- the keyboard's pitch, measured off the drawing -------------------
    # The bug this guards: the field was 15 x 19.05 and layout.py drew the
    # lattice on a hard-coded 19.05, so the two agreed and the SVG looked
    # right. The measured pitch is 19.000. The check carries that number
    # itself; the drawing is not consulted about it.
    k = keyed()
    text = k.svg()
    f = k.audit(text) + switch_field("keyed", text)
    if f:
        print(f"  FAIL  a keyboard on its measured pitch must pass; got {f}")
        bad += 1
    else:
        print("  ok    the drawn 285.000 x 95.000 field measures 19.000 in "
              "both axes")

    # a field with no pitch at all is an error, not a 19.05 default
    try:
        L.keyboard(0.0, 0.0, 288.350, 98.450, "NO PITCH",
                   field=(285.0, 95.0))
        print("  FAIL  a switch field with no pitch must raise, not default")
        bad += 1
    except ValueError:
        print("  ok    a switch field with no stated pitch raises")

    # the bug itself, as the artefact: a field and a lattice both on 19.050.
    # Nothing in the drawing is inconsistent -- which is why only an
    # independently sourced pitch can catch it.
    q = keyed(pitch=CTRL_PITCH)
    bad += not expect("the field drawn on the control pitch",
                      switch_field("ctrl-pitch", q.svg()), "PITCH")

    # ...and the field right but the lattice wrong, which is the same bug the
    # other way round: a 285.000 field divided 19.050 at a time. Done on the
    # SVG TEXT, with the field's own left edge read back out of it, so the
    # drawing code gets no say in where the lines land.
    q = keyed()
    text = q.svg()
    left = float(re.search(r'<rect data-kind="switch-field" x="([-0-9.]+)"',
                           text).group(1))
    seen = []

    def relattice(m):
        seen.append(m)
        return m.group(1) + f"{left + len(seen) * CTRL_PITCH:.4f}"

    wrong = re.sub(r'(data-kind="switch-col" x1=")([-0-9.]+)', relattice, text)
    if len(seen) != 14:
        print(f"  FAIL  expected 14 lattice lines to rewrite, found {len(seen)}")
        bad += 1
    else:
        bad += not expect("a 19.050 lattice inside a 285.000 field",
                          switch_field("relattice", wrong), "PITCH")

    # one line nudged in the SVG text alone: proof the check reads the drawing
    q = keyed()
    nudged = re.sub(r'(data-kind="switch-col" x1=")([0-9.]+)',
                    lambda m: m.group(1) + f"{float(m.group(2)) + 0.5:.4f}",
                    q.svg(), count=1)
    bad += not expect("one lattice line nudged 0.5 mm in the SVG",
                      switch_field("nudged", nudged), "PITCH")

    # the field dropped from the drawing while the bay stays
    q = keyed()
    gone = re.sub(r'<rect data-kind="switch-field"[^/]*/>', "", q.svg(), count=1)
    bad += not expect("the switch field not drawn at all",
                      switch_field("nofield", gone), "PITCH")

    # a field that is drawn but not over the keys
    q = keyed()
    moved = re.sub(r'(<rect data-kind="switch-field" x=")([-0-9.]+)',
                   lambda m: m.group(1) + f"{float(m.group(2)) - 12.0:.4f}",
                   q.svg(), count=1)
    if moved == q.svg():
        print("  FAIL  the test's own field substitution missed -- fix it")
        bad += 1
    else:
        bad += not expect("the switch field slid out of its bay",
                          switch_field("outside", moved), "FIELD")

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

    # Every spec in v2/layouts must actually render. The checks above build
    # panels directly, so they cannot see a change to the tool that breaks the
    # spec interface -- and one did: adding route() to build()'s argument list
    # left slab.py unrenderable and nothing noticed until a reader tried it.
    import glob
    import importlib.util
    import os
    here = os.path.dirname(os.path.abspath(__file__))
    specs = sorted(glob.glob(os.path.join(here, "..", "layouts", "*.py")))
    specs = [s for s in specs if not os.path.basename(s).startswith("_")
             and os.path.basename(s) != "parts.py"]
    import sys as _sys
    _sys.path.insert(0, os.path.join(here, "..", "layouts"))
    for path in specs:
        name = os.path.basename(path)
        try:
            sp = importlib.util.spec_from_file_location("spec_" + name, path)
            mod = importlib.util.module_from_spec(sp)
            sp.loader.exec_module(mod)
            panel = mod.build(L.KIT)
            text = panel.svg()
            fails = panel.audit(text) + switch_field(name, text,
                                                     require_bay=True)
            if fails:
                print(f"  FAIL  {name} renders but does not pass: {fails[:2]}")
                bad += 1
            else:
                print(f"  ok    {name} renders and passes")
        except Exception as e:
            print(f"  FAIL  {name} does not render: {type(e).__name__}: {e}")
            bad += 1

    print(f"  {'ALL PASS' if not bad else str(bad) + ' CHECK(S) BLIND'}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
