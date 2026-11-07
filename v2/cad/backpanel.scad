// ===========================================================================
//  cYbErDeCk v2 - backpanel.scad
//  The back: laser-cut sheet closing the carrier from below.
//
//  Same one-source-two-outputs rule as slab.scad -- the profile is authored
//  once in 2D and the solid is that profile extruded, so the DXF and the
//  render cannot drift apart.
//
//      openscad -D 'part="2d"' -o export/dxf/backpanel.dxf v2/cad/backpanel.scad
// ===========================================================================

include <parameters.scad>
use <lib/util.scad>

part = "3d";

//  The panel is the full footprint. It is the floor the whole object stands
//  on, so it is also what keeps a 304 mm printed carrier flat: a bowed carrier
//  under a rigid 12.70 slab is this build's real failure mode, and a stiff
//  back panel fights it from the other side.
//  SAME ONE-DATUM-N-SLOTS RULE AS THE SLAB, and for the same reason: this
//  panel bolts into bosses printed in the same part over the same 289.560 mm
//  span, so it has to float the same way or one of the two plates binds.
//
//  BUT THE SCREW IS M2.5, NOT M3, AND THIS FILE USED TO DRILL IT FOR M3.
//  It drew all ten holes at screw_m3_clear_back -- O3.400, 6.400 slots -- and
//  reasoned out loud about the hole "an M3 passes", while carrier.scad's
//  back_insert_cut() presses M2.5 inserts at the SAME ten points and section 9b
//  explains why they have to be M2.5 (an M3 pocket is 6.700 deep against a
//  6.350 floor). The pattern is not in doubt: back_screw_pts() is one function
//  and both files call it. An M3 does not enter an M2.5 insert; read the other
//  way, an M2.5 in a O3.400 hole threw away half of what the round datum is
//  for. The giveaway was in this file all along -- the cowl's four screws below
//  go into the same inserts and are already drilled screw_m25_clear_d.
//  parameters.scad 6b has the arithmetic, the head's bearing area over a slot
//  narrower than itself, and the washer that follows from it.
//    The thickness argument that used to sit here still holds and is just
//  attached to the wrong screw: O2.900 through 3.175 finishes O2.648 at 2.0 deg
//  of taper, which still passes an M2.5, where the SLAB's 12.700 turns the same
//  class of hole into O2.483 and passes nothing -- which is the proof that the
//  slab's O4.445 is a thickness problem and not a hole problem. (The old line
//  said O3.400 "finishes O3.160" at 3.175; 3.160 is a 3.000 sheet's figure and
//  back_t has been 3.175 since section 3 was re-derived.)
//
//  TEN holes, not six: the six that mirror the slab's screw line, plus four
//  inside the keyboard's seat plinth. Six in the control band would leave the
//  keyboard half of a 304 mm plate unfastened, and fighting the carrier's bow
//  is this panel's whole structural job. SIX of the ten take a DIN 125 A 2.5
//  washer under the head on assembly and FOUR MUST NOT -- the four that carry
//  the feet, because a O6.000 washer cannot enter the foot's O5.100 pocket
//  mouth and would stand the pad 0.500 off the panel. parameters.scad 6b has
//  which is which and why. That split is deliberately invisible in this
//  drawing: a bought washer is not a cut feature, so nothing here changes for
//  it, the DXF must not grow a contour, and the ten holes stay identical.
module backpanel_2d() {
    difference() {
        rrect(body_w, body_h, corner_r);
        datum_and_slots(back_screw_pts(), screw_datum_i,
                        screw_m25_clear_d, screw_slot_l_back);

        //  THE DISPLAY'S FOUR SCREW HEADS LIVE IN THIS PANEL. The display
        //  hangs on four SMTSO-M2.5-7ET standoffs and the enclosure drives
        //  M2.5 UP into them, so the heads end up on the carrier floor's
        //  underside -- which is this panel's top face. There is only 3.175 of
        //  floor under that pocket, so a counterbore in the carrier would
        //  leave a 0.175 membrane. O5.500 clearance here instead: a cap head
        //  is 2.500 tall in a 3.175 panel, so it is swallowed with 0.675 to
        //  spare and the deck still sits flat. A laser cannot counterbore;
        //  this is the laser's version of one.
        //  TWO OF THESE FOUR MERGE INTO THE WINDOW BELOW, and that is a
        //  vendor fact rather than a drawing error: the 18650 holder's own
        //  edge (V 4.600) runs straight through the display's two lower
        //  mounting holes (V 3.500). The union is harmless -- a screw head in
        //  a bigger hole is still clear -- but it is drawn knowing it happens,
        //  because an unintended merge and an intended one look identical in
        //  a contour count.
        for (sx = [-1, 1], sy = [-1, 1])
            translate([disp_x + sx * 85.500/2, band_mid + sy * 62.100/2])
                circle(d = 5.500);

        //  And the window the 18650 holder goes through on its way out, with
        //  the cowl's four screws around it. Section 12.
        translate(cowl_xy()) square(cowl_window(), center = true);
        for (p = cowl_screw_pts())
            translate(p) circle(d = screw_m25_clear_d);
    }
}

module backpanel_3d() { linear_extrude(height = back_t) backpanel_2d(); }

if (part == "2d")      backpanel_2d();
else                   backpanel_3d();
