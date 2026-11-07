// ===========================================================================
//  cYbErDeCk v2 - fader_cap.scad
//  The NeoSlider's cap. Same logic as the knob: a plug that fills the slab's
//  slot, with only its top proud.
//
//      openscad -D 'part="3d"' -o export/stl/fader_cap.stl v2/cad/fader_cap.scad
//
//  It is the LAST part fitted (v2/parts/stack.md 6.2 step 12), which is what
//  lets it be wider than the shank's own travel window: it never has to pass
//  through the slot, it drops onto the shank from above once the slab is on.
//
//  PRINT IT CAP DOWN, for the knob's reasons: the top face is the only one
//  anyone looks at, and inverted there is no overhang anywhere.
// ===========================================================================

include <parameters.scad>
use <lib/util.scad>

part = "3d";

fc_plug   = acrylic_t;                  // 12.700, inside the slot
fc_proud  = 3.175;                      // [DESIGN] 0.125 in. HALF the knob's
                                        //   6.350, because a fader is pushed
                                        //   along and a knob is turned: the
                                        //   cap wants a low, wide purchase and
                                        //   the knob wants a tall rim.
fc_cap_w  = slider_slot_w + 2*2.000;    // [DERIVED] 16.000, overhanging the
                                        //   slot by 2.000 each side the way
                                        //   the knob's brim overhangs its bore
fc_cap_l  = 25.400;                     // [DESIGN] 1 in along the travel

//  The shank, measured: 5.000 wide, 1.786 thick, tip 15.500 above the board's
//  front face. With the board's rear on the wiring plane that puts the tip at
//  17.070, which is 7.545 above the acrylic's underside.
fc_grip_w = slider_shank + 0.200;       // [DESIGN] 5.200
fc_grip_t = 1.700;                      // [DESIGN] 0.086 UNDER the measured
                                        //   1.786, so the blade is an
                                        //   interference fit. There is no room
                                        //   for a set screw -- the shank tip
                                        //   stops 5.155 below the deck's top
                                        //   face -- so the grip is the fit.
fc_grip_h = 8.000;                      // [DESIGN] over the shank's 7.545

module fader_cap_3d() {
    difference() {
        union() {
            //  The plug is SQUARE on the cap's width, not the slot's
            //  length. Drawn the length of the slot it fills the slot, and a
            //  fader that fills its slot does not move -- the first cut of
            //  this file was 50.000 long against a 40.000 travel.
            //  slider_slot_l is travel + cap + 2.000, so the cap is
            //  slider_cap_w in BOTH axes and the travel is what is left.
            rbox(slider_cap_w, slider_cap_w, fc_plug, corner_r);
            translate([0, 0, fc_plug])
                rbox(fc_cap_w, fc_cap_l, fc_proud, corner_r);
        }
        //  the shank slot, up from the underside
        translate([-fc_grip_t/2, -fc_grip_w/2, -1])
            cube([fc_grip_t, fc_grip_w, fc_grip_h + 1]);
        //  one groove across the top, the same legend the knobs carry
        translate([-fc_cap_w/2, -knob_mark[0]/2,
                   fc_plug + fc_proud - knob_mark[1]])
            cube([fc_cap_w, knob_mark[0], knob_mark[1] + 1]);
    }
}

fader_cap_3d();
