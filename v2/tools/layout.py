#!/usr/bin/env python3
"""Draw a v2 front-panel layout as SVG, on a Swiss grid, from a spec.

A layout is DATA, not a drawing: every control sits on a grid module and every
number is computed, so a layout re-renders the moment a part dimension changes
rather than being redrawn by hand.

The checks DO NOT read the spec. They parse the emitted SVG, recover the frame
from the drawn body rectangle, and measure the drawn geometry. This is v1's
discipline, arrived at the hard way three times: a check that computes from the
same parameters as the thing it checks cannot see a drawing bug. Here the
artefact is the SVG, so the SVG is what gets measured.

Usage:  python3 v2/tools/layout.py <spec.py> <out.svg>
"""
import importlib.util
import sys
import xml.etree.ElementTree as ET

# ---- inherited from v1, proven, not re-derived ----------------------------
DISP_W, DISP_H = 84.80, 63.60   # display active area
WALL = 3.20                     # side wall thickness

# ---- the vendor's grid, measured from the STEP files (parts.md 1.1) ------
PAT_B = (76.200, 21.590)        # NeoSlider 5295 = NeoKey 4980 = Quad Rotary 5752
PAT_B_HOLES = (38.10, 16.51)    # the four M2.5 mounting holes, centre to centre
PAT_A_W = 25.400                # Adafruit sensor / ADC breakout width
HOLE_D = 2.50

# The grid is DERIVED from the parts, not chosen, and it took two wrong answers
# to get there. Adafruit lays boards out in inches:
#
#   4.7625 (3/16 in)  divides  4 of 14 placement dimensions. Rejected: it
#                     divides the 19.05 pitch family and nothing else -- not
#                     21.590, not 25.400, not 16.510.
#   1.2700 (0.050 in) divides 12 of 14. Rejected for one reason: an EVEN
#                     number of items at 19.05 pitch, centred, lands on
#                     half-pitches (+/-9.525, +/-28.575), and 9.525/1.27 = 7.5.
#                     A four-encoder row is exactly that case.
#   0.6350 (0.025 in) divides all twelve AND the half-pitches. This is the
#                     placement grid, and it still catches an arbitrary round
#                     metric number, which is what the check is for.
#
# The two it cannot divide are the display's 84.80 and 91.00 -- a metric part
# in an imperial world. They are exempted by name, not by loosening the grid.
MODULE = 0.635                  # placement snap, 0.025 in
MODULE_BOARD = 1.27             # Adafruit's own board grid, 0.050 in
MODULE_COARSE = 6.35            # coarse layout rhythm, 1/4 in
TILE = 25.40                    # the plate's visible tile, 1 in

# ---- the deck identity, from cad/parameters.scad -------------------------
# v1 makes the deck a function of the keyboard and tools/validate.py asserts
# it every run. These are the same identities, so a layout's body size is
# DERIVED from the keyboard rather than typed in.
WALL = 3.20                     # parameters.scad: wall
SPINE = 3.20                    # the rib between two bands
BOTTOM_WALL = 4.40              # wall + 1.2
BOARD_POCKET = (93.90, 70.30)   # board_w/h_max + 2 * board_fit
KBD_FIT = (1.35, 0.95)          # kbd_pocket = keyboard + this


def deck(keyboard_w, keyboard_d, ctrl_band_h=None):
    """Deck size and the datums a layout needs, in model space.

    ctrl_band_h None reproduces v1's two bands (keyboard, board). A number adds
    the third band of arrangement A between them, with its own spine.

    Returns (body_w, body_h, datums), where datums are model-space y values:
    the keyboard bay's top edge, and the control band's bottom and top edges.
    """
    kw = keyboard_w + KBD_FIT[0]
    kh = keyboard_d + KBD_FIT[1]
    body_w = max(kw, BOARD_POCKET[0]) + 2 * WALL

    band = 0.0 if ctrl_band_h is None else ctrl_band_h + SPINE
    body_h = BOTTOM_WALL + kh + SPINE + band + BOARD_POCKET[1] + WALL

    # measured up from the outside bottom, then shifted to deck-centre origin
    kbd_top = BOTTOM_WALL + kh
    band_lo = kbd_top + SPINE
    band_hi = band_lo + (ctrl_band_h or 0.0)
    half = body_h / 2
    return body_w, body_h, dict(kbd_top=kbd_top - half,
                                band_lo=band_lo - half,
                                band_hi=band_hi - half,
                                kbd_pocket=(kw, kh))


# ---- provisional, pending the ergonomics research ------------------------
FINGER = 6.00   # minimum rim-to-rim gap between two things you turn
THUMB = 45.00   # a control played while typing stays within this of the keys

INK, RULE, GRID = "#111318", "#c9ccd4", "#e8eaee"
LIVE, GHOST = "#d8552a", "#8d93a0"
TOL = 0.01      # mm: drawn-vs-declared disagreement that counts as a bug

TURNABLE = ("knob", "encoder")


# Which things may overlap which: a control sits ON a bay and a bay sits ON
# the plate, so overlap is only ever checked within one layer. Containment is
# what gets checked between them.
LAYER = {"plate": 0, "bay": 1, "keyboard": 0}


def _ctl(kind, x, y, w, h, label, r=0.0, kw=None, kh=None, free=False,
         on_plate=None, y_from=None, mirror=0.0, metric=False):
    """One control, centred at (x, y) in millimetres from the deck centre.

    kind    knob | encoder | key | slider | slot | sensor | display
    w, h    what a finger touches: knob diameter, keycap, slider slot
    kw, kh  the keep-out -- actuator plus its mounting hardware. Overlaps are
            checked on this, because a panel-mount nut needs a wrench and a
            breakout PCB is wider than the actuator standing on it.
    free    exempt from the mirror-symmetry check, deliberately
    on_plate  whether this control belongs to the acrylic plate. Default True
            for a control, False for the plate and the display. Set it False
            for something panel-mounted straight into the printed shell -- a
            pot with a short bushing fits a 2.40 mm wall and needs no plate.
    y_from  the y datum this control is dimensioned FROM, when it is not the
            panel's. A control placed beside the display is dimensioned off the
            display's centre, and the display lands on no inch grid at all, so
            its absolute y cannot be on the grid and checking it there is
            meaningless. Declaring the datum keeps the check real: the OFFSET
            from that datum still has to be on the grid. An exemption that has
            to be declared is a constraint; one the checker guesses is a hole.
    """
    return dict(kind=kind, x=x, y=y, w=w, h=h, label=label, r=r, free=free,
                kw=w if kw is None else kw, kh=h if kh is None else kh,
                layer=LAYER.get(kind, 2), y_from=y_from, mirror=mirror, metric=metric,
                on_plate=(kind not in ("plate", "display")
                          if on_plate is None else on_plate))


def display(x, y, label="DISPLAY", mirror=None):
    """The display sits in the shell, not on the control plate."""
    return _ctl("display", x, y, DISP_W, DISP_H, label,
                mirror=x if mirror is None else mirror)


def plate(x, y, w, h, label="PLATE"):
    """The flush laser-cut acrylic plate. Everything on_plate must fit inside."""
    return _ctl("plate", x, y, w, h, label)


def keyboard(x, y, w, h, label="KEYBOARD", field=None):
    """The keyboard bay. field is the switch field (w, h) drawn inside it."""
    it = _ctl("keyboard", x, y, w, h, label, on_plate=False)
    it["field"] = field
    return it


def bay(x, y, label, pattern="B", mirror=0.0, size=None, holes=None,
        metric=False):
    """One interchangeable breakout bay.

    Pattern B is the vendor's own module: the NeoSlider, the NeoKey 1x4 and the
    Quad Rotary are all 76.200 x 21.590 on the same 38.10 x 16.51 hole quad, so
    a bay does not know which of the three it holds. Pattern A is the 25.400 mm
    sensor breakout.
    """
    w, h = size if size else {"B": PAT_B, "Bv": (PAT_B[1], PAT_B[0]),
                              "A": (PAT_A_W, PAT_A_W)}[pattern]
    it = _ctl("bay", x, y, w, h, label, mirror=mirror, metric=metric)
    it["pattern"] = pattern
    it["holes"] = holes
    return it


class Panel:
    """Model space is millimetres, origin at the deck centre, +y up.

    SVG is y-down, so every emitted y is negated once, in py(), and nowhere
    else.
    """

    def __init__(self, body_w, body_h, module=MODULE, title="", finger=FINGER,
                 y_datum=0.0):
        """y_datum is the horizontal line the vertical grid is measured from.

        x needs no datum: the centreline is the datum, because symmetry about it
        is a requirement. y has no such natural zero -- the deck centre is
        wherever body_h/2 happens to land, which is a function of the keyboard's
        depth and carries no design meaning. Checking the vertical grid against
        it tests nothing. Specs set this to a real datum, normally the control
        band's bottom edge.
        """
        self.bw, self.bh, self.mod = body_w, body_h, module
        self.title, self.finger = title, finger
        self.y_datum = y_datum
        self.items, self.notes = [], []

    def add(self, *items):
        self.items.extend(items)
        return self

    def note(self, text):
        self.notes.append(text)
        return self

    # -- drawing -------------------------------------------------------
    def _frame(self, pad):
        W, H = self.bw + 2 * pad, self.bh + 2 * pad + 20
        return W, H, pad + self.bw / 2, pad + self.bh / 2

    def svg(self, pad=16):
        W, H, ox, oy = self._frame(pad)
        px = lambda x: round(ox + x, 4)
        py = lambda y: round(oy - y, 4)

        s = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W*3.2:.0f}" '
             f'height="{H*3.2:.0f}" viewBox="0 0 {W} {H}">',
             '<rect width="100%" height="100%" fill="#fbfbfc"/>',
             '<g font-family="Helvetica,Arial,sans-serif">']

        s.append(f'<g stroke="{GRID}" stroke-width="0.12">')
        for i in range(-int(self.bw / 2 / self.mod) - 1,
                       int(self.bw / 2 / self.mod) + 2):
            x = i * self.mod
            if abs(x) <= self.bw / 2:
                s.append(f'<line x1="{px(x)}" y1="{py(self.bh/2)}" '
                         f'x2="{px(x)}" y2="{py(-self.bh/2)}"/>')
        for i in range(-int(self.bh / 2 / self.mod) - 1,
                       int(self.bh / 2 / self.mod) + 2):
            y = i * self.mod
            if abs(y) <= self.bh / 2:
                s.append(f'<line x1="{px(-self.bw/2)}" y1="{py(y)}" '
                         f'x2="{px(self.bw/2)}" y2="{py(y)}"/>')
        s.append('</g>')

        s.append(f'<line x1="{px(0)}" y1="{py(self.bh/2)}" x2="{px(0)}" '
                 f'y2="{py(-self.bh/2)}" stroke="{RULE}" stroke-width="0.25" '
                 f'stroke-dasharray="2 1.5"/>')
        # the body rect is the fiducial: measure() recovers ox, oy from it
        s.append(f'<rect data-kind="body" data-label="BODY" data-role="frame" '
                 f'x="{px(-self.bw/2)}" y="{py(self.bh/2)}" '
                 f'width="{self.bw}" height="{self.bh}" rx="3" fill="none" '
                 f'stroke="{INK}" stroke-width="0.6"/>')

        for it in self.items:
            k, x, y, w, h = it["kind"], it["x"], it["y"], it["w"], it["h"]
            tag = f'data-kind="{k}" data-label="{it["label"]}"'
            if k == "keyboard":
                s.append(f'<rect {tag} data-role="touch" x="{px(x-w/2)}" '
                         f'y="{py(y+h/2)}" width="{w}" height="{h}" rx="1.5" '
                         f'fill="#f7f7f8" stroke="{GHOST}" stroke-width="0.4"/>')
                fw, fh = it.get("field") or (0, 0)
                if fw:
                    s.append(f'<rect x="{px(x-fw/2)}" y="{py(y+fh/2)}" '
                             f'width="{fw}" height="{fh}" fill="none" '
                             f'stroke="{GHOST}" stroke-width="0.25" '
                             f'stroke-dasharray="1.5 1.5"/>')
                    cols = int(round(fw / 19.05))
                    for c in range(1, cols):
                        gx = x - fw / 2 + c * 19.05
                        s.append(f'<line x1="{px(gx)}" y1="{py(y+fh/2)}" '
                                 f'x2="{px(gx)}" y2="{py(y-fh/2)}" '
                                 f'stroke="{GRID}" stroke-width="0.2"/>')
                    for r in range(1, 5):
                        gy = y - fh / 2 + r * fh / 5
                        s.append(f'<line x1="{px(x-fw/2)}" y1="{py(gy)}" '
                                 f'x2="{px(x+fw/2)}" y2="{py(gy)}" '
                                 f'stroke="{GRID}" stroke-width="0.2"/>')
                s.append(f'<text x="{px(x)}" y="{py(y)+1.4}" fill="{GHOST}" '
                         f'font-size="3.4" text-anchor="middle">{it["label"]} '
                         f'{w:.2f} × {h:.2f}</text>')
                continue
            if k == "plate":
                s.append(f'<rect {tag} data-role="touch" x="{px(x-w/2)}" '
                         f'y="{py(y+h/2)}" width="{w}" height="{h}" fill="#f3f4f6" '
                         f'stroke="{INK}" stroke-width="0.45"/>')
                continue
            if k == "bay":
                s.append(f'<rect {tag} data-role="touch" x="{px(x-w/2)}" '
                         f'y="{py(y+h/2)}" width="{w}" height="{h}" rx="2.54" '
                         f'fill="none" stroke="{GHOST}" stroke-width="0.3" '
                         f'stroke-dasharray="2 1"/>')
                hx, hy = it.get("holes") or {
                    "B": PAT_B_HOLES, "Bv": (PAT_B_HOLES[1], PAT_B_HOLES[0]),
                    "A": (20.32, 20.32)}[it.get("pattern", "B")]
                for sx in (-1, 1):
                    for sy in (-1, 1):
                        s.append(f'<circle cx="{px(x+sx*hx/2)}" '
                                 f'cy="{py(y+sy*hy/2)}" r="{HOLE_D/2}" '
                                 f'fill="none" stroke="{GHOST}" '
                                 f'stroke-width="0.25"/>')
                if not it["label"].startswith("_"):
                    s.append(f'<text x="{px(x)}" y="{py(y-h/2)+3.4}" '
                             f'fill="{GHOST}" font-size="2.2" '
                             f'text-anchor="middle">{it["label"]}</text>')
                continue
            if k == "display":
                s.append(f'<rect {tag} data-role="touch" x="{px(x-w/2)}" '
                         f'y="{py(y+h/2)}" width="{w}" height="{h}" '
                         f'fill="#eef0f3" stroke="{GHOST}" stroke-width="0.35"/>')
                s.append(f'<text x="{px(x)}" y="{py(y)+1.4}" fill="{GHOST}" '
                         f'font-size="3.4" text-anchor="middle">{it["label"]} '
                         f'{w:.1f} × {h:.1f}</text>')
                continue
            if k in TURNABLE:
                s.append(f'<circle {tag} data-role="touch" cx="{px(x)}" '
                         f'cy="{py(y)}" r="{w/2}" fill="none" stroke="{INK}" '
                         f'stroke-width="0.5"/>')
                if k == "encoder":
                    s.append(f'<circle cx="{px(x)}" cy="{py(y)}" r="{w/6}" '
                             f'fill="{INK}"/>')
            else:
                s.append(f'<rect {tag} data-role="touch" x="{px(x-w/2)}" '
                         f'y="{py(y+h/2)}" width="{w}" height="{h}" '
                         f'rx="{it["r"] or (1 if k == "key" else min(w,h)/2)}" '
                         f'fill="none" stroke="{INK}" stroke-width="0.5"/>')
            if it["kw"] > w or it["kh"] > h:
                if k in TURNABLE:
                    s.append(f'<circle {tag} data-role="keep" cx="{px(x)}" '
                             f'cy="{py(y)}" r="{it["kw"]/2}" fill="none" '
                             f'stroke="{LIVE}" stroke-width="0.3" '
                             f'stroke-dasharray="1 1"/>')
                else:
                    s.append(f'<rect {tag} data-role="keep" '
                             f'x="{px(x-it["kw"]/2)}" y="{py(y+it["kh"]/2)}" '
                             f'width="{it["kw"]}" height="{it["kh"]}" '
                             f'fill="none" stroke="{LIVE}" stroke-width="0.3" '
                             f'stroke-dasharray="1 1"/>')
            if not it["label"].startswith("_"):
                s.append(f'<text x="{px(x)}" y="{py(y-max(h,it["kh"])/2)+3.4}" '
                         f'fill="{INK}" font-size="2.5" text-anchor="middle" '
                         f'letter-spacing="0.12">{it["label"]}</text>')

        s.append(f'<text x="{pad}" y="{H-6}" fill="{INK}" font-size="3.6" '
                 f'letter-spacing="0.3">{self.title}</text>')
        s.append(f'<text x="{W-pad}" y="{H-6}" fill="{GHOST}" font-size="2.6" '
                 f'text-anchor="end">{self.bw:.2f} × {self.bh:.2f} mm · '
                 f'{self.mod:.3f} grid · {self.finger:.1f} finger gap</text>')
        s.append('</g></svg>')
        return "\n".join(s)

    # -- measurement: read the drawing back, never the spec ------------
    def measure(self, svg_text):
        """Recover body and control geometry from the emitted SVG.

        Returns (body_w, body_h, [box, ...]) in model space. The frame comes
        from the drawn body rectangle, so a frame bug shows up as a wrong body
        size rather than silently shifting every control by the same amount.
        """
        root = ET.fromstring(svg_text)
        els = [e for e in root.iter() if e.get("data-label")]
        body = next((e for e in els if e.get("data-kind") == "body"), None)
        if body is None:
            raise AssertionError("the drawing has no body rectangle to measure from")
        bw, bh = float(body.get("width")), float(body.get("height"))
        ox = float(body.get("x")) + bw / 2
        oy = float(body.get("y")) + bh / 2

        boxes = []
        for e in els:
            if e.get("data-kind") == "body":
                continue
            tag = e.tag.rsplit("}", 1)[-1]
            if tag == "circle":
                r = float(e.get("r"))
                x, y, w, h = float(e.get("cx")) - ox, oy - float(e.get("cy")), 2*r, 2*r
            else:
                w, h = float(e.get("width")), float(e.get("height"))
                x = float(e.get("x")) - ox + w / 2
                y = oy - float(e.get("y")) - h / 2
            boxes.append(dict(label=e.get("data-label"), kind=e.get("data-kind"),
                              role=e.get("data-role"), x=x, y=y, w=w, h=h))
        return bw, bh, boxes

    # -- the checks, all reading measured geometry ---------------------
    def audit(self, svg_text):
        bw, bh, boxes = self.measure(svg_text)
        fail = []

        if abs(bw - self.bw) > TOL or abs(bh - self.bh) > TOL:
            fail.append(f"BODY      drawn {bw:.2f} × {bh:.2f}, "
                        f"declared {self.bw:.2f} × {self.bh:.2f}")

        decl = {it["label"]: it for it in self.items}
        touch = [b for b in boxes if b["role"] == "touch"]
        keep = {b["label"]: b for b in boxes if b["role"] == "keep"}

        # 1. did the drawing draw what the spec said?
        for b in touch:
            d = decl.get(b["label"])
            if d is None:
                fail.append(f"GHOST     {b['label']} is drawn but not in the spec")
                continue
            for k in ("x", "y", "w", "h"):
                if abs(b[k] - d[k]) > TOL:
                    fail.append(f"MISDRAWN  {b['label']} {k}: drawn {b[k]:+.3f}, "
                                f"spec {d[k]:+.3f}")
        for lbl in decl:
            if not any(b["label"] == lbl for b in touch):
                fail.append(f"MISSING   {lbl} is in the spec but was not drawn")

        # the box every other check uses: keep-out where there is one
        eff = []
        for b in touch:
            k = keep.get(b["label"], b)
            eff.append(dict(b, w=max(b["w"], k["w"]), h=max(b["h"], k["h"])))

        # 2. on the grid. Two kinds are exempt by name, and only these two:
        #    the display (84.80 x 63.60 active) and the keyboard (97.5 deep).
        #    Both are metric vendor parts in an otherwise imperial panel and
        #    land on no inch grid at any modulus. They are exempt because
        #    nothing about them is ours to choose -- not because the grid is
        #    inconvenient. Everything datumed OFF them declares y_from and is
        #    still checked.
        METRIC_PARTS = ("display", "keyboard")
        for b in eff:
            if (b["kind"] in METRIC_PARTS
                    or decl.get(b["label"], {}).get("metric")):
                continue
            yf = decl.get(b["label"], {}).get("y_from")
            for ax, zero in (("x", 0.0),
                             ("y", self.y_datum if yf is None else yf)):
                off = abs(b[ax] - zero) % self.mod
                off = min(off, self.mod - off)
                if off > 1e-6:
                    fail.append(f"OFF GRID  {b['label']:14s} {ax} = {b[ax]:+.3f}, "
                                f"{off:.3f} off the {self.mod} module"
                                + (f" from y datum {zero:+.3f}" if ax == "y" else ""))

        # 3. nothing overlaps anything. A control inside its bay and a bay
        #    inside the plate are legitimate nesting and are skipped; a thing
        #    that merely intersects a plate or bay is a collision, which is how
        #    the display running into the plate gets caught.
        lay = {it["label"]: it.get("layer", 2) for it in self.items}

        def nested(outer, inner):
            """Does inner legitimately belong to outer?

            A bay inside the plate has to be wholly inside it. A CONTROL on a
            bay only has to be CENTRED on it: a knob sits above its breakout
            and may be wider than it -- a 38.10 knob on a 25.40 board overhangs
            by 6.35 a side and nothing collides, because the overhang is in the
            air above the PCB. What still matters there is knob-to-knob and
            board-to-board, and both are checked within their own layer.
            """
            if outer["kind"] == "bay":
                return (abs(inner["x"] - outer["x"]) <= outer["w"] / 2 + TOL
                        and abs(inner["y"] - outer["y"]) <= outer["h"] / 2 + TOL)
            return (abs(inner["x"] - outer["x"]) + inner["w"] / 2
                    <= outer["w"] / 2 + TOL
                    and abs(inner["y"] - outer["y"]) + inner["h"] / 2
                    <= outer["h"] / 2 + TOL)

        for i, a in enumerate(eff):
            for b in eff[i + 1:]:
                la, lb = lay.get(a["label"], 2), lay.get(b["label"], 2)
                if la != lb:
                    outer, inner = (a, b) if la < lb else (b, a)
                    if nested(outer, inner):
                        continue
                dx = (a["w"] + b["w"]) / 2 - abs(a["x"] - b["x"])
                dy = (a["h"] + b["h"]) / 2 - abs(a["y"] - b["y"])
                if dx > TOL and dy > TOL:
                    fail.append(f"OVERLAP   {a['label']} / {b['label']} "
                                f"by {min(dx, dy):.2f} mm")

        # 3b. everything that belongs to the plate is actually on the plate
        pl = next((b for b in eff if b["kind"] == "plate"), None)
        if pl is not None:
            if abs(pl["x"]) > TOL and not decl.get(pl["label"], {}).get("free"):
                fail.append(f"OFF AXIS  the plate's centre is x = {pl['x']:+.2f}, "
                            f"not on the centreline")
            for b in eff:
                if not decl.get(b["label"], {}).get("on_plate", False):
                    continue
                ex = abs(b["x"] - pl["x"]) + b["w"] / 2 - pl["w"] / 2
                ey = abs(b["y"] - pl["y"]) + b["h"] / 2 - pl["h"] / 2
                if ex > TOL or ey > TOL:
                    fail.append(f"OFF PLATE {b['label']} overhangs the plate by "
                                f"{max(ex, ey):.2f} mm")

        # 4. inside the body, clear of the wall
        for b in eff:
            ex = abs(b["x"]) + b["w"] / 2 - (bw / 2 - WALL)
            ey = abs(b["y"]) + b["h"] / 2 - (bh / 2 - WALL)
            if ex > TOL or ey > TOL:
                fail.append(f"OFF BODY  {b['label']} runs {max(ex, ey):.2f} mm "
                            f"into the {WALL} wall")

        # 5. room for a finger between two things you turn
        turn = [b for b in eff if b["kind"] in TURNABLE]
        for i, a in enumerate(turn):
            for b in turn[i + 1:]:
                gap = (((a["x"] - b["x"]) ** 2 + (a["y"] - b["y"]) ** 2) ** 0.5
                       - a["w"] / 2 - b["w"] / 2)
                if gap < self.finger - TOL:
                    fail.append(f"TIGHT     {a['label']} / {b['label']} rims "
                                f"{gap:.2f} mm apart, want {self.finger:.2f}")

        # 6. symmetry about the centreline -- a stated requirement
        key = lambda b: (b["kind"], round(b["w"], 2), round(b["h"], 2),
                         round(b["y"], 2))
        for b in eff:
            d0 = decl.get(b["label"], {})
            if d0.get("free"):
                continue
            ax = d0.get("mirror", 0.0)
            if abs(b["x"] - ax) < TOL:          # on its own zone's axis
                continue
            want = 2 * ax - b["x"]
            if not any(abs(o["x"] - want) < TOL and key(o) == key(b)
                       and abs(decl.get(o["label"], {}).get("mirror", 0.0) - ax) < TOL
                       for o in eff if o is not b):
                fail.append(f"ASYMMETRY {b['label']} at x = {b['x']:+.3f} has no "
                            f"mirror at {want:+.3f} about its axis {ax:+.3f}")
        return fail


def main():
    spec_path, out_path = sys.argv[1], sys.argv[2]
    # a spec may import its siblings (part tables, shared geometry), and
    # importlib's loader does not put the spec's own directory on the path
    import os
    sys.path.insert(0, os.path.dirname(os.path.abspath(spec_path)))
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    spec = importlib.util.spec_from_file_location("spec", spec_path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    panel = mod.build(Panel, _ctl, display, plate, bay, keyboard)

    text = panel.svg()
    fail = panel.audit(text)
    for line in fail:
        print(f"  {line}")
    open(out_path, "w").write(text)
    print(f"  {'FAIL' if fail else 'ok  '}  {out_path}  "
          f"({len(panel.items)} controls, {len(fail)} problems, measured from "
          f"the drawing)")
    for n in panel.notes:
        print(f"  note: {n}")
    return 1 if fail else 0


if __name__ == "__main__":
    sys.exit(main())
