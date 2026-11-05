#!/usr/bin/env python3
"""Draw a v2 front-panel layout as SVG, on a Swiss grid, from a spec.

A layout is DATA, not a drawing. Every control is placed on a grid module and
every number in the output is computed, so a layout can be re-rendered the
moment a part dimension changes rather than redrawn by hand.

Usage:  python3 v2/tools/layout.py <spec.py> <out.svg>
"""
import importlib.util
import sys

# ---- the deck, inherited from v1 and not up for negotiation ---------------
BODY_W, BODY_H = 116.25, 140.25
DISP_W, DISP_H = 84.80, 63.60
DISP_CX, DISP_CY = -1.60, 31.775
WALL = 3.20

INK = "#111318"
RULE = "#c9ccd4"
GRID = "#e8eaee"
LIVE = "#d8552a"
GHOST = "#8d93a0"


def _ctl(kind, x, y, w, h, label, r=0.0):
    return dict(kind=kind, x=x, y=y, w=w, h=h, label=label, r=r)


class Panel:
    """Model space is millimetres, origin at the deck centre, +y up.

    SVG is y-down, so every emitted y is negated once, here, and nowhere else.
    """

    def __init__(self, body_w=BODY_W, body_h=BODY_H, module=4.75, title=""):
        self.bw, self.bh, self.mod, self.title = body_w, body_h, module, title
        self.items, self.notes = [], []

    def add(self, *items):
        self.items.extend(items)
        return self

    def note(self, text):
        self.notes.append(text)
        return self

    # -- grid helpers --------------------------------------------------
    def snapped(self):
        """Every control whose centre is off the grid module, with its error."""
        bad = []
        for it in self.items:
            for axis in ("x", "y"):
                off = abs(it[axis] % self.mod)
                off = min(off, self.mod - off)
                if off > 1e-6:
                    bad.append((it["label"], axis, it[axis], off))
        return bad

    def overlaps(self):
        """Pairs of controls whose footprints intersect, with the overlap."""
        hit = []
        for i, a in enumerate(self.items):
            for b in self.items[i + 1:]:
                dx = (a["w"] + b["w"]) / 2 - abs(a["x"] - b["x"])
                dy = (a["h"] + b["h"]) / 2 - abs(a["y"] - b["y"])
                if dx > 0 and dy > 0:
                    hit.append((a["label"], b["label"], round(min(dx, dy), 2)))
        return hit

    def off_body(self):
        """Controls that run outside the body, or into the wall."""
        out = []
        for it in self.items:
            if (abs(it["x"]) + it["w"] / 2 > self.bw / 2 - WALL
                    or abs(it["y"]) + it["h"] / 2 > self.bh / 2 - WALL):
                out.append(it["label"])
        return out

    # -- drawing -------------------------------------------------------
    def svg(self, pad=16):
        W, H = self.bw + 2 * pad, self.bh + 2 * pad + 20
        ox, oy = pad + self.bw / 2, pad + self.bh / 2

        def px(x):
            return round(ox + x, 3)

        def py(y):
            return round(oy - y, 3)

        s = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W*3.2:.0f}" '
             f'height="{H*3.2:.0f}" viewBox="0 0 {W} {H}">',
             '<rect width="100%" height="100%" fill="#fbfbfc"/>',
             '<g font-family="Helvetica,Arial,sans-serif">']

        # grid
        s.append(f'<g stroke="{GRID}" stroke-width="0.12">')
        n = int(self.bw / 2 / self.mod) + 1
        for i in range(-n, n + 1):
            x = i * self.mod
            if abs(x) <= self.bw / 2:
                s.append(f'<line x1="{px(x)}" y1="{py(self.bh/2)}" '
                         f'x2="{px(x)}" y2="{py(-self.bh/2)}"/>')
        n = int(self.bh / 2 / self.mod) + 1
        for i in range(-n, n + 1):
            y = i * self.mod
            if abs(y) <= self.bh / 2:
                s.append(f'<line x1="{px(-self.bw/2)}" y1="{py(y)}" '
                         f'x2="{px(self.bw/2)}" y2="{py(y)}"/>')
        s.append('</g>')

        # centreline, body, display
        s.append(f'<line x1="{px(0)}" y1="{py(self.bh/2)}" x2="{px(0)}" '
                 f'y2="{py(-self.bh/2)}" stroke="{RULE}" stroke-width="0.25" '
                 f'stroke-dasharray="2 1.5"/>')
        s.append(f'<rect x="{px(-self.bw/2)}" y="{py(self.bh/2)}" '
                 f'width="{self.bw}" height="{self.bh}" rx="3" fill="none" '
                 f'stroke="{INK}" stroke-width="0.6"/>')
        s.append(f'<rect x="{px(DISP_CX-DISP_W/2)}" y="{py(DISP_CY+DISP_H/2)}" '
                 f'width="{DISP_W}" height="{DISP_H}" fill="#eef0f3" '
                 f'stroke="{GHOST}" stroke-width="0.35"/>')
        s.append(f'<text x="{px(DISP_CX)}" y="{py(DISP_CY)+1.4}" fill="{GHOST}" '
                 f'font-size="3.4" text-anchor="middle">DISPLAY '
                 f'{DISP_W:.1f} × {DISP_H:.1f}</text>')

        # controls
        for it in self.items:
            k, x, y, w, h = it["kind"], it["x"], it["y"], it["w"], it["h"]
            if k in ("knob", "encoder"):
                s.append(f'<circle cx="{px(x)}" cy="{py(y)}" r="{w/2}" '
                         f'fill="none" stroke="{INK}" stroke-width="0.5"/>')
                s.append(f'<circle cx="{px(x)}" cy="{py(y)}" r="{w/2+1.6}" '
                         f'fill="none" stroke="{LIVE}" stroke-width="0.3" '
                         f'stroke-dasharray="1 1"/>')
                if k == "encoder":
                    s.append(f'<circle cx="{px(x)}" cy="{py(y)}" r="{w/6}" '
                             f'fill="{INK}"/>')
            elif k == "key":
                s.append(f'<rect x="{px(x-w/2)}" y="{py(y+h/2)}" width="{w}" '
                         f'height="{h}" rx="1" fill="none" stroke="{INK}" '
                         f'stroke-width="0.5"/>')
            else:  # slider / slot / sensor
                s.append(f'<rect x="{px(x-w/2)}" y="{py(y+h/2)}" width="{w}" '
                         f'height="{h}" rx="{it["r"] or min(w,h)/2}" '
                         f'fill="none" stroke="{INK}" stroke-width="0.5"/>')
            s.append(f'<text x="{px(x)}" y="{py(y-h/2)+3.4}" fill="{INK}" '
                     f'font-size="2.5" text-anchor="middle" '
                     f'letter-spacing="0.12">{it["label"]}</text>')

        s.append(f'<text x="{pad}" y="{H-6}" fill="{INK}" font-size="3.6" '
                 f'letter-spacing="0.3">{self.title}</text>')
        s.append(f'<text x="{W-pad}" y="{H-6}" fill="{GHOST}" font-size="2.6" '
                 f'text-anchor="end">{self.bw:.2f} × {self.bh:.2f} mm · '
                 f'{self.mod:.2f} mm module</text>')
        s.append('</g></svg>')
        return "\n".join(s)


def main():
    spec_path, out_path = sys.argv[1], sys.argv[2]
    spec = importlib.util.spec_from_file_location("spec", spec_path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    panel = mod.build(Panel, _ctl)

    bad, hits, out = panel.snapped(), panel.overlaps(), panel.off_body()
    for lbl, ax, v, off in bad:
        print(f"  OFF GRID  {lbl:16s} {ax} = {v:+.2f}, {off:.2f} off the module")
    for a, b, d in hits:
        print(f"  OVERLAP   {a} / {b} by {d} mm")
    for lbl in out:
        print(f"  OFF BODY  {lbl}")
    open(out_path, "w").write(panel.svg())
    print(f"  wrote {out_path}  ({len(panel.items)} controls, "
          f"{len(bad)} off grid, {len(hits)} overlaps, {len(out)} off body)")
    return 1 if (bad or hits or out) else 0


if __name__ == "__main__":
    sys.exit(main())
