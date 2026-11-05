"""The carrier plate -- the interior, which is a visible surface here.

Under 12.70 mm of CLEAR acrylic every board, every standoff and every wire is
seen. So the wiring is laid out and checked rather than left to whoever builds
it: channels are orthogonal, their vertices are on the same 0.635 grid as the
panel above, and no channel is allowed to pass under a board, because a channel
you cannot see is the one thing this construction cannot afford.

The bus topology is the layout. From v2/parts/wiring.md:

  I2C  one chain, 3V3 / GND / SDA(GPIO13) / SCL(GPIO14), leaving the board's
       rear 2x8 header and running LEFT TO RIGHT through the three encoders
       and into the fader. The daisy chain and the signal flow are the same
       line, which is why it reads.

  POTS three wipers on GPIO1 / GPIO2 / GPIO3 -- ADC1_CH0/1/2, the unit that
       keeps working while Wi-Fi is up -- plus a shared 3V3 and GND pair. One
       trunk below the pot row with three stubs up to it. No ADS1115.

The header is the awkward part and it is drawn honestly: the Waveshare board's
2x8 is a FEMALE socket on its REAR face, so it points down into the carrier.
A male header is carried by the carrier and the deck's nine wires terminate on
its upper tails, under the display board, where the only buried run in the
whole plate is the short exit to the board's right edge.
"""
import _common as C
import parts as P

U = 19.050
BAND = 4 * U
GAP = 1.5 * U
KNOB, PITCH = 25.400, 2 * U
GRID_X = 36.195
GRID_W = 2 * PITCH + KNOB
DISP_BD = (93.900, 70.300)
DISP_X = GRID_X - GRID_W / 2 - GAP - DISP_BD[0] / 2
SLIDE_X = GRID_X + GRID_W / 2 + GAP + P.BAY_H / 2
COLS = (GRID_X - PITCH, GRID_X, GRID_X + PITCH)

HDR_X = -50.800                 # the 2x8, under the display board
EXIT_X = -41.910                # just clear of the board's right edge
POT_TRUNK_DY = -33.655          # 53 x 0.635, below the pot row, inside the band
POT_BODY = 16.000               # Alpha RV16 class body, what you see on the plate


def build(Panel, ctl, display, plate, bay, keyboard, route):
    import layout as L
    p = C.frame(L, BAND, "CARRIER  the wiring is the interior",
                board_band=False)
    d = p.datums
    mid = d["band_lo"] + BAND / 2
    enc_y, pot_y = mid + PITCH / 2, mid - PITCH / 2
    trunk_y = mid + POT_TRUNK_DY

    p.add(C.kbd(L, d))
    p.add(plate(0.0, mid, 15 * U, BAND))

    p.add(bay(DISP_X, mid, "_display board", size=DISP_BD, holes=(84.0, 60.0),
              mirror=DISP_X, metric=True))
    # layer 2: the header sits UNDER the display board, so it nests there
    # rather than colliding with it
    p.add(bay(HDR_X, mid, "2x8", size=(5.080, 20.320), holes=(2.54, 17.78),
              mirror=HDR_X, layer=2))

    for i, x in enumerate(COLS):
        p.add(bay(x, enc_y, f"_enc board {i+1}", pattern="A", mirror=GRID_X))
        p.add(ctl("encoder", x, enc_y, KNOB, KNOB, f"E{i+1}", mirror=GRID_X))
        p.add(ctl("knob", x, pot_y, POT_BODY, POT_BODY, f"P{i+1}",
                  mirror=GRID_X))

    p.add(bay(SLIDE_X, mid, "_fader board", pattern="Bv", mirror=SLIDE_X))
    p.add(ctl("slider", SLIDE_X, mid, P.SLIDER_SLOT[1], P.SLIDER_SLOT[0],
              "FADER", mirror=SLIDE_X))

    # --- the I2C chain, left to right, one line ---
    # Runs go EDGE to EDGE, not centre to centre: a STEMMA cable leaves a
    # breakout at its short end, so the channel lives in the gap between two
    # boards, which is also the only place it can be seen.
    eL = [x - P.BRK / 2 for x in COLS]      # each encoder board's left edge
    eR = [x + P.BRK / 2 for x in COLS]      # and its right edge
    fader_top = mid + P.BAY_W / 2           # the fader's upper short end
    fader_in = mid + 34.290                 # 54 x 0.635, clear of the board edge

    p.add(route("I2C", [(HDR_X, mid + 10.160), (EXIT_X, mid + 10.160),
                        (EXIT_X, enc_y), (eL[0], enc_y)]))
    p.add(route("E1-E2", [(eR[0], enc_y), (eL[1], enc_y)]))
    p.add(route("E2-E3", [(eR[1], enc_y), (eL[2], enc_y)]))
    p.add(route("E3-FADER", [(eR[2], enc_y), (eR[2], fader_in),
                             (SLIDE_X, fader_in)]))

    # --- the analogue trunk, and a stub up to each pot body ---
    p.add(route("ADC", [(HDR_X, mid - 10.160), (EXIT_X, mid - 10.160),
                        (EXIT_X, trunk_y), (COLS[2], trunk_y)], w=3.0))
    for i, x in enumerate(COLS):
        p.add(route(f"P{i+1} wiper", [(x, trunk_y), (x, pot_y - 7.620)], w=3.0))

    p.note(f"deck {p.bw:.2f} x {p.bh:.2f} mm")
    p.note("I2C: one chain, left to right -- the daisy chain and the signal "
           "flow are the same line")
    p.note("pots: GPIO1/2/3 = ADC1_CH0/1/2, the unit that survives Wi-Fi. "
           "No ADS1115, no multiplexer, no level shifter")
    p.note("the only buried run is the header's exit under the display board, "
           "which is unavoidable: the 2x8 is on the board's rear face")
    return p
