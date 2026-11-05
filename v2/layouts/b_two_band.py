"""B -- two bands, and the pots move beside the display.

A's third row costs 39.37 mm of deck depth. This buys it back by noticing that
the keyboard already forces the deck to 294.75 mm wide while the board pocket
is only 93.90 -- so there is 97.2 mm of dead shell either side of the display,
created by the keyboard and used by nothing. Four pots go there: no extra depth,
and they sit beside the screen they read against.

They mount straight through the printed shell wall rather than through the
plate. A 2.40 mm wall is within the panel thickness a short Alpha bushing
allows, so the plate is not involved and the 10 mm RD901F bushing is not needed
for these.

The cost is the fourth pot. Three cannot be symmetric here either: an odd count
needs one control on the axis, and beside the display the axis IS the display.
Two mirrored pairs need no axis control, so four is what symmetry costs -- or
what it buys, depending which way the money is going.
"""
import _common as C
import parts as P

BAY_PITCH = P.BAY_W + P.BAY_GAP
BAY_ROW_GAP = 7.620
POT_X = 71.120                      # 112 x 0.635. Clears the 93.90 board pocket
POT_DY = 17.780                     # 28 x 0.635; 35.56 apart, a 6.35 rim gap


def build(Panel, ctl, display, plate, bay, keyboard):
    import layout as L
    band = 50.800                   # 80 x 0.635, two bay rows and one gap
    p = C.frame(L, band, "B  two bands \u00b7 pots beside the display")
    d = p.datums

    (y_far, y_near), span = C.rows(d, band, [
        (P.BAY_H / 2, BAY_ROW_GAP),
        (P.BAY_H / 2, 0.0),
    ])
    p.add(C.kbd(L, d))
    p.add(plate(*C.plate_rect(d, band)))

    p.add(bay(-BAY_PITCH, y_far, "FADER L"), bay(0.0, y_far, "ENCODERS"),
          bay(BAY_PITCH, y_far, "FADER R"))
    for i, k in enumerate((-1.5, -0.5, 0.5, 1.5)):
        p.add(ctl("encoder", k * P.ENC_PITCH, y_far, P.ENC_KNOB, P.ENC_KNOB,
                  f"E{i+1}"))
    for s in (-1, 1):
        p.add(ctl("slider", s * BAY_PITCH, y_far, *P.SLIDER_SLOT,
                  "FADE L" if s < 0 else "FADE R"))

    p.add(bay(-BAY_PITCH, y_near, "SPARE L"), bay(0.0, y_near, "KEYS"),
          bay(BAY_PITCH, y_near, "SPARE R"))
    for i, k in enumerate((-1.5, -0.5, 0.5, 1.5)):
        p.add(ctl("key", k * P.KEY_PITCH, y_near, P.KEY_CAP, P.KEY_CAP, f"K{i+1}"))

    # four pots in the shell, flanking the display, two per side
    for s in (-1, 1):
        for n, dy in (("U", POT_DY), ("D", -POT_DY)):
            p.add(ctl("knob", s * POT_X, p.disp_y + dy, P.POT_KNOB, P.POT_KNOB,
                      f"P{'L' if s < 0 else 'R'}{n}",
                      kw=P.POT_KEEP, kh=P.POT_KEEP, on_plate=False,
                      y_from=p.disp_y))

    for n in C.thumb(d, (y_far, y_near), ("encoder row", "key row"), L.THUMB):
        p.note(n)
    p.note(f"deck {p.bw:.2f} x {p.bh:.2f} mm \u00b7 band {band:.3f} holds {span:.3f}")
    p.note("pots mount through the 2.40 shell wall, not the plate")
    p.note("pot y is datumed to the display centre, declared, because the "
           "display lands on no inch grid")
    return p
