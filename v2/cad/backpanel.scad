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
//  span, so it has to float the same way or one of the two plates binds. Its
//  hole diameter is NOT the slab's, though -- O3.400 through 3.175 mm finishes
//  O3.160 worst case and an M3 passes, which is the proof that the slab's
//  O4.445 is a thickness problem and not a hole problem.
//
//  TEN holes, not six: the six that mirror the slab's screw line, plus four
//  inside the keyboard's seat plinth. Six in the control band would leave the
//  keyboard half of a 304 mm plate unfastened, and fighting the carrier's bow
//  is this panel's whole structural job.
module backpanel_2d() {
    difference() {
        rrect(body_w, body_h, corner_r);
        datum_and_slots(back_screw_pts(), screw_datum_i,
                        screw_m3_clear_back, screw_slot_l_back);

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
