"""A -- two bands, four pots, the Quad Rotary. The shallowest deck.

Everything mirrors about the centreline with no exemptions, and it gets there by
buying a FOURTH pot. Three pots cannot be strictly symmetric in two bands: an
odd count needs one control on the centreline, and both centreline slots are
already taken by the two bays that must be centred -- the encoder bay, which is
the one the screen labels, and the key strip.
"""
import _common as C
import parts as P

BAY_PITCH = P.BAY_W + P.BAY_GAP     # 85.725 = 135 x 0.635
ROW_GAP = P.BAY_GAP                 # 9.525
CTRL_BAND_H = 63.500                # 100 x 0.635 = 2.5 in
POT_IN, POT_OUT = 53.340, 81.915    # both on the 0.635 grid


def build(Panel, ctl, display, plate, bay):
    import layout as L
    p = C.frame(L, CTRL_BAND_H, "A  two bands, four pots, quad rotary")
    d = p.datums

    (y_far, y_near), span = C.rows(d, CTRL_BAND_H, [
        (P.BAY_H / 2, ROW_GAP),
        (P.POT_KEEP / 2, 0.0),
    ])
    p.add(plate(*C.plate_rect(d, CTRL_BAND_H)))

    # far row -- the encoder bay centred, a slider either side
    p.add(bay(-BAY_PITCH, y_far, "SLIDER L"), bay(0.0, y_far, "ENCODERS"),
          bay(BAY_PITCH, y_far, "SLIDER R"))
    for i, k in enumerate((-1.5, -0.5, 0.5, 1.5)):
        p.add(ctl("encoder", k * P.ENC_PITCH, y_far, P.ENC_KNOB, P.ENC_KNOB,
                  f"E{i+1}"))
    for s in (-1, 1):
        p.add(ctl("slider", s * BAY_PITCH, y_far, *P.SLIDER_SLOT,
                  "FADE L" if s < 0 else "FADE R"))

    # near row -- the key strip centred, two mirrored pairs of pots outboard
    p.add(bay(0.0, y_near, "KEYS"))
    for i, k in enumerate((-1.5, -0.5, 0.5, 1.5)):
        p.add(ctl("key", k * P.KEY_PITCH, y_near, P.KEY_CAP, P.KEY_CAP, f"K{i+1}"))
    for s in (-1, 1):
        for tag, x in (("I", POT_IN), ("O", POT_OUT)):
            p.add(ctl("knob", s * x, y_near, P.POT_KNOB, P.POT_KNOB,
                      f"P{'L' if s < 0 else 'R'}{tag}",
                      kw=P.POT_KEEP, kh=P.POT_KEEP))

    for n in C.thumb(d, (y_far, y_near), ("far row", "near row"), L.THUMB):
        p.note(n)
    p.note(f"deck {p.bw:.2f} x {p.bh:.2f} mm | band {CTRL_BAND_H} holds {span:.3f}")
    return p
