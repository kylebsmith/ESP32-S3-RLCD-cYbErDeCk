"""A -- three bands. The recommendation.

Why three and not two: the control complement needs four Pattern B bays (the
Quad Rotary, two NeoSliders, the NeoKey) and only three fit side by side in a
288.35 mm interior, so two bay rows are forced. Stacking a pot row with a bay
row instead puts the bay row's centreline 52.7 mm from the keyboard -- beyond
the 45 mm thumb limit -- and the bay row holds the encoders, the most-played
control on the panel. So the pots get their own row and it sits deliberately
outside thumb reach, which is correct for them: a pot is set and left, reached
with the whole hand, and MIL's 25 mm blind-operation separation applies to it
rather than to a played row.

Each pot sits directly above a bay centre. Three pots, three columns: one on
the axis, two mirrored, so the odd count is symmetric for free.
"""
import _common as C
import parts as P

BAY_PITCH = P.BAY_W + P.BAY_GAP     # 85.725
BAY_ROW_GAP = 7.620                 # 12 x 1.27. Below the 9.525 the interface
                                    # study prefers, and the reason is the thumb:
                                    # 9.525 pushes the far bay row to 45.11 mm,
                                    # 0.11 over the limit. Declared, not silent.
POT_ROW_GAP = 9.525


def build(Panel, ctl, display, plate, bay, keyboard):
    import layout as L
    # 142 x 0.635, so HALF the band is still on the grid and the plate -- which
    # fills the band -- has a grid-legal centre. 141 units does not: its half is
    # 70.5. The 0.635 of spare collects above the pot row.
    band = 90.170
    p = C.frame(L, band, "A  three bands \u00b7 pots / encoders+faders / keys")
    d = p.datums

    (y_pot, y_mid, y_near), span = C.rows(d, band, [
        (P.POT_KEEP / 2, POT_ROW_GAP),
        (P.BAY_H / 2, BAY_ROW_GAP),
        (P.BAY_H / 2, 0.0),
    ])
    p.add(C.kbd(L, d))
    p.add(plate(*C.plate_rect(d, band)))

    # pot row -- one per column, each over a bay centre
    for s, tag in ((-1, "L"), (0, "C"), (1, "R")):
        p.add(ctl("knob", s * P.POT_PITCH, y_pot, P.POT_KNOB, P.POT_KNOB,
                  f"POT {tag}", kw=P.POT_KEEP, kh=P.POT_KEEP))

    # middle row -- the encoder bay centred, a fader either side
    p.add(bay(-BAY_PITCH, y_mid, "FADER L"), bay(0.0, y_mid, "ENCODERS"),
          bay(BAY_PITCH, y_mid, "FADER R"))
    for i, k in enumerate((-1.5, -0.5, 0.5, 1.5)):
        p.add(ctl("encoder", k * P.ENC_PITCH, y_mid, P.ENC_KNOB, P.ENC_KNOB,
                  f"E{i+1}"))
    for s in (-1, 1):
        p.add(ctl("slider", s * BAY_PITCH, y_mid, *P.SLIDER_SLOT,
                  "FADE L" if s < 0 else "FADE R"))

    # near row -- the key strip centred, two spare bays flanking it
    p.add(bay(-BAY_PITCH, y_near, "SPARE L"), bay(0.0, y_near, "KEYS"),
          bay(BAY_PITCH, y_near, "SPARE R"))
    for i, k in enumerate((-1.5, -0.5, 0.5, 1.5)):
        p.add(ctl("key", k * P.KEY_PITCH, y_near, P.KEY_CAP, P.KEY_CAP, f"K{i+1}"))

    for n in C.thumb(d, (y_pot, y_mid, y_near),
                     ("pot row", "encoder row", "key row"), L.THUMB):
        p.note(n)
    p.note(f"deck {p.bw:.2f} x {p.bh:.2f} mm \u00b7 band {band:.3f} holds {span:.3f}")
    p.note("two bays are left spare, and that is the point: a bay does not know "
           "what it holds")
    return p
