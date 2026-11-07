"""Shared geometry for the layout studies.

A spec states intent -- which bays, in which rows -- and this computes the deck
from the keyboard by the repo's own identity, then places the display in the
board band. No layout types a body size in.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import parts as P  # noqa: E402

SPINE = 3.20
BOARD_POCKET_H = 70.30
PLATE_W = 266.700           # 14 CONTROL key units, 420 x 0.635. Leaves
                            # 10.825 mm of shell each side of a 288.35
                            # interior -- enough for the seat and the drawn
                            # reveal, which a plate sized to the bay row
                            # (285.000) does not leave.

# The keyboard's grid, and it is NOT the control unit. All 61 of the K9 Pro's
# switch holes land on an exact 19.000 mm lattice in BOTH axes, read off the
# analytic hole axes in Keychron's own 37.7 MB assembly, and every position
# agrees with Keychron's own QMK LAYOUT_61_ansi to 0.000 on the 1 u keys. This
# repo took the keyboard's pitch to be the 19.050 MX unit in five files and was
# 0.750 mm out on the field. cad/parameters.scad carries both, as U and kbd_U.
KBD_U = 19.000              # [CAD-EXACT] v2/parts/keyboard-strip.md 1.6
KBD_FIELD = (15 * KBD_U, 5 * KBD_U)
                            # [DERIVED] 285.000 x 95.000 -- switch CENTRES.
                            # The wide keys are nudged off the nominal unit
                            # centre by up to 0.040; nothing here places a
                            # single key, so the field is the lattice's extent
                            # and the measured per-key column in 1.6 is what a
                            # per-key position must come from.


def frame(L, ctrl_band_h, title, board_band=True):
    """A Panel sized from the K9 Pro, datumed on the control band's bottom edge.

    board_band False means the display lives INSIDE the control band rather
    than in a band of its own -- the slab arrangement, where the screen sits
    beside the controls instead of above them. That removes a whole band and
    its spine from the depth.
    """
    if board_band:
        bw, bh, d = L.deck(P.KBD_W, P.KBD_D, ctrl_band_h)
    else:
        kw = P.KBD_W + L.KBD_FIT[0]
        kh = P.KBD_D + L.KBD_FIT[1]
        bw = max(kw, L.BOARD_POCKET[0]) + 2 * L.WALL
        bh = L.BOTTOM_WALL + kh + SPINE + ctrl_band_h + L.WALL
        kbd_top = L.BOTTOM_WALL + kh
        half = bh / 2
        d = dict(wall=L.WALL, bottom_wall=L.BOTTOM_WALL,
                 kbd_top=kbd_top - half, band_lo=kbd_top + SPINE - half,
                 band_hi=kbd_top + SPINE + ctrl_band_h - half,
                 kbd_pocket=(kw, kh))
    panel = L.Panel(bw, bh, title=title, y_datum=d["band_lo"],
                    wall=d["wall"], bottom_wall=d["bottom_wall"])
    if board_band:
        disp_y = d["band_hi"] + SPINE + BOARD_POCKET_H / 2
        panel.add(L.display(0.0, disp_y))
    else:
        disp_y = d["band_lo"] + ctrl_band_h / 2
    panel.datums, panel.disp_y, panel.band = d, disp_y, ctrl_band_h
    return panel


def kbd(L, d):
    """The keyboard bay, centred, sitting on the bottom wall."""
    kw, kh = d["kbd_pocket"]
    return L.keyboard(0.0, d["kbd_top"] - kh / 2, kw, kh,
                      "K9 PRO BAY", field=KBD_FIELD, pitch=KBD_U)


def rows(d, ctrl_band_h, stack):
    """Centre y for each row, returned top-first. stack is [(half, gap_below)].

    Rows hang from the band's BOTTOM edge, not centred in it. Centring looked
    tidier and was wrong: it distributes the spare band height as half of it per
    side, and half of an odd number of grid units is a half unit, which throws
    every row off the grid. Hanging from the datum keeps every row an exact
    multiple of the module from it, and any spare collects at the top where
    nothing is datumed to it.
    """
    n = len(stack)
    half = [h for h, _ in stack]
    gap = [g for _, g in stack]
    span = sum(2 * h for h in half) + sum(gap[:-1])
    y = [0.0] * n
    y[n - 1] = d["band_lo"] + half[n - 1]
    for i in range(n - 2, -1, -1):
        y[i] = y[i + 1] + half[i + 1] + gap[i] + half[i]
    return y, span


def plate_rect(d, ctrl_band_h):
    """The plate fills the band: centred on it, the full band height."""
    return 0.0, d["band_lo"] + ctrl_band_h / 2, PLATE_W, ctrl_band_h


def thumb(d, ys, labels, limit):
    out = []
    for lab, y in zip(labels, ys):
        mm = round(y - d["kbd_top"], 2)
        out.append(f"{lab} centreline {mm} mm from the keyboard edge"
                   f" ({'within' if mm <= limit else 'BEYOND'} the {limit:.0f} limit)")
    return out
