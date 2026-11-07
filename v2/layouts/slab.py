"""The slab -- a desk instrument. Display, a 2 x 3 control grid, a fader.

Not handheld: it sits on a desk or a lap, two hands, so thumb reach is not the
constraint and the whole panel is in play.

Left to right:

    DISPLAY            4 : 3     84.80 x 63.60    the metric part
    2 x 3 GRID         8 : 5    101.60 x 63.50    4.000 x 2.500 inches
      encoders on top, pots beneath
    FADER                       21.59 x 76.20     one NeoSlider, stood on end

Two blocks of the SAME HEIGHT in two clean ratios, separated by two equal
28.575 mm gaps, with the end margins equal to 0.08 mm. The grid stands 63.50
tall and the display's active area is 63.60: they share a top line and a bottom
line to within a tenth of a millimetre.

The band is 76.20 = 4 u because the NeoSlider's board IS 76.200 mm. Stood on
end it is exactly the band, so the fader is the element that declares the
band's depth and nothing was chosen to make it fit.

The grid centre sits at +36.195, which is the position that makes the left and
right margins equal AND lands every knob column on the 0.635 grid. The display
floats: at 93.90 its board width is on no inch grid, and it is the one part
nothing here chose, so it absorbs the remainder instead of dragging the
controls off the grid with it.

CONSTRUCTION -- this layout is drawn for a three-layer build:
  front   12.70 mm (0.5 in) CLEAR laser-cut acrylic
  middle  a 3D-printed carrier plate holding every board and all the wiring,
          routed on its top face so it reads through the acrylic
  back    a laser-cut back panel

At 12.70 the acrylic is thicker than any standard pot bushing, so it carries
nothing: every control mounts to the carrier and its shaft passes up through a
clearance hole. Shaft length above the carrier becomes the binding dimension
and is being checked -- the NeoSlider's shank reaches only 15.50 above its PCB,
against 12.70 of panel, so the fader may need a local pocket.
"""
import _common as C
import parts as P

U = 19.050                  # [VENDOR] the MX key unit, and the module the
                            # CONTROL field is laid on. NOT the keyboard's
                            # pitch, which is 19.000 (_common.KBD_U).

#  There was a `FIELD = 15 * U` here, commented "285.75, the K9 Pro's switch
#  field exactly". It was wrong twice: the K9 Pro's pitch is 19.000, so its
#  switch field is 285.000 x 95.000, and nothing in this spec ever read the
#  value. cad/parameters.scad section 2 deleted the same variable, with the
#  same comment, for the same two reasons.

BAND = 4 * U                # 76.200, the NeoSlider board stood on end
GAP = 1.5 * U               # 28.575, both gaps

KNOB = 25.400               # 1 in, every control. Six identical circles.
PITCH = 2 * U               # 38.100 across and down
GRID_X = 36.195             # 57 x 0.635: balances the margins, keeps the grid on grid
SCREW_X = 144.780           # 228 x 0.635, outboard of every board
GRID_W = 2 * PITCH + KNOB   # 101.600 = 4.000 in
GRID_H = PITCH + KNOB       # 63.500 = 2.500 in

DISP_BD = (93.900, 70.300)
DISP_X = GRID_X - GRID_W / 2 - GAP - DISP_BD[0] / 2
SLIDE_X = GRID_X + GRID_W / 2 + GAP + P.BAY_H / 2


def build(k):
    import layout as L
    ctl, display, plate, bay = k.ctl, k.display, k.plate, k.bay
    route, slab, screw = k.route, k.slab, k.screw
    p = C.frame(L, BAND, "SLAB  display / 2x3 grid / fader", board_band=False)
    d = p.datums
    mid = d["band_lo"] + BAND / 2

    p.add(C.kbd(L, d))

    # The acrylic is the WHOLE footprint with the keyboard cut out of it, not a
    # panel over the control band. That makes the frame around the void thin --
    # 8.125 at the sides, 4.40 at the front -- so no fastener can live there and
    # the only solid acrylic is the 87.525 mm rear region. The front rail is not
    # a cantilever: it rests on the carrier's own front wall for its full length.
    kw, kh = d["kbd_pocket"]
    p.add(slab(p.bw, p.bh, void=(0.0, d["kbd_top"] - kh / 2, kw, kh)))

    # M3, not M4. At the only screw line that is both symmetric and on the
    # grid, M4's 4.5 clearance hole leaves 5.27 mm to the slab edge against the
    # 6.75 acrylic wants; M3's 3.4 leaves 5.82 against 5.10.
    for sx, side in ((-1, "L"), (1, "R")):
        for n, dy in enumerate((31.750, 0.0, -31.750)):
            p.add(screw(sx * SCREW_X, mid + dy, 3.40, label=f"M3 {side}{n+1}"))

    # the display, and its board outline -- visible through clear acrylic, so
    # the board is part of the composition, not hidden behind an aperture
    p.add(bay(DISP_X, mid, "_display board", size=DISP_BD, holes=(84.0, 60.0),
              mirror=DISP_X, metric=True))
    p.add(display(DISP_X, mid))

    # the 2 x 3 grid: encoders on top, pots beneath. Position is the legend.
    for i, cx in enumerate((-PITCH, 0.0, PITCH)):
        x = GRID_X + cx
        p.add(bay(x, mid + PITCH / 2, f"_enc board {i+1}", pattern="A",
                  mirror=GRID_X))
        p.add(ctl("encoder", x, mid + PITCH / 2, KNOB, KNOB, f"E{i+1}",
                  mirror=GRID_X))
        p.add(ctl("knob", x, mid - PITCH / 2, KNOB, KNOB, f"P{i+1}",
                  mirror=GRID_X))

    p.add(bay(SLIDE_X, mid, "_fader board", pattern="Bv", mirror=SLIDE_X))
    p.add(ctl("slider", SLIDE_X, mid, P.SLIDER_SLOT[1], P.SLIDER_SLOT[0],
              "FADER", mirror=SLIDE_X))

    p.note(f"deck {p.bw:.2f} x {p.bh:.2f} mm, ratio {p.bw/p.bh:.4f}")
    p.note(f"grid {GRID_W:.2f} x {GRID_H:.2f} = 4.000 x 2.500 in, 8:5 exactly; "
           f"display 4:3 exactly; same height to 0.10 mm")
    p.note(f"two gaps of {GAP:.3f}; end margins equal to 0.08 mm")
    p.note("pots have no PCB -- they mount to the carrier, so the lower row "
           "shows knobs and wiring where the upper row shows boards")
    return p
