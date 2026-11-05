"""Part dimensions for the layout studies, all traceable to v2/parts/parts.md.

Nothing here is a round number chosen for looking tidy. Every value is either
measured from a vendor STEP file, read off a datasheet, or derived by shown
arithmetic from those. The grid is imperial because the parts are.
"""

# --- Pattern B: the interchangeable breakout bay -------------------------
# NeoSlider 5295, NeoKey 1x4 4980 and Quad Rotary 5752 share this outline.
BAY_W, BAY_H = 76.200, 21.590           # [STEP]
BAY_GAP = 9.525                         # >= 2 board-grid units, spec 9.4

# --- Pattern A: the 1 inch breakout -------------------------------------
BRK = 25.400                            # [STEP] 5880 encoder board is square

# --- actuators ----------------------------------------------------------
ENC_PITCH = 19.050                      # [STEP] 0.75 in, fixed by the 5752
ENC_KNOB = 13.000                       # the only diameter that satisfies both
                                        # the 19.05 pitch less a 6 mm finger gap
                                        # (13.05 max) and MIL's 13 mm minimum
POT_KNOB = 25.400                       # 1 in. Size-coded against the encoder:
                                        # 25.40 - 13.00 = 12.40, near MIL's
                                        # 13 mm delta-diameter for size coding
POT_KEEP = 29.210                       # 23 x 1.27, so HALF of it (14.605) is
                                        # still on the 0.635 grid. 28.575 is
                                        # not: its half is 22.5 units, and
                                        # stacking a row on a half-unit throws
                                        # every row below it off the grid.
POT_PITCH = 85.725                      # one bay pitch: each pot sits directly
                                        # over a bay centre. Separation is
                                        # 85.725 - 25.400 = 60.325 against
                                        # MIL's 25 mm for blind operation, which
                                        # is what a set-and-forget control wants.
SLIDER_SLOT = (50.800, 10.160)          # the window the shank sweeps  [NOT SOURCED
                                        # travel -- parts.md section 10]
KEY_CAP = 18.000                        # MX keycap plan  [DS cherry-mx]
KEY_PITCH = 19.050                      # the NeoKey's own 0.75 in spacing

# --- the display, a metric part in an imperial world --------------------
DISP_W, DISP_H = 84.800, 63.600         # active area  [REPO]
DISP_THIRD = DISP_W / 3                 # 28.2667: three encoders CAN align to
                                        # the display; four span 76.200, which
                                        # is 8.60 narrower than it

# --- the keyboard -------------------------------------------------------
KBD_W, KBD_D = 287.000, 97.500          # K9 Pro  [VENDOR, four regional stores]
