// ===========================================================================
//  cYbErDeCk v2 - cowl.scad
//  The cover over the display board's 18650 holder, on the deck's underside.
//
//      openscad -D 'part="3d"' -o export/stl/cowl.stl v2/cad/cowl.scad
//
//  IT EXISTS BECAUSE ONE COMPONENT WILL NOT FIT AND CANNOT BE MOVED. The
//  Waveshare ESP32-S3-RLCD-4.2's 18650 holder reaches 15.200 mm behind the
//  PCB's rear face -- 6.500 deeper than anything else on that board, and
//  5.450 deeper than Waveshare's own stand base, which is why the vendor's
//  base does not contain it either. In this deck that puts it 1.820 below the
//  deck's own underside. v1 built a cowl around exactly this fact and so does
//  this one; it is the only part of the whole object that is not flat.
//
//  It is held by the SAME four screws that pass up through the back panel into
//  inserts in the carrier's floor, so cowl, panel and carrier are one fastener
//  stack and nothing is bonded. Take four screws out and the holder is
//  reachable without disturbing anything else.
//
//  PRINT IT OPEN SIDE UP: the flange is then the bed surface, the walls rise
//  from it, and the only bridge is the cavity's floor at the top, which is
//  1.500 over an 88.270 x 22.373 opening -- too wide to bridge. So print it
//  CLOSED SIDE DOWN instead, with the floor on the bed and the open side up.
//  The flange's underside is then the last layer; it is also the face nobody
//  sees.
// ===========================================================================

include <parameters.scad>
use <lib/util.scad>

part = "3d";

cw_outer = [cowl_window()[0] + 2*(cowl_wall() + cowl_flange()),
            cowl_window()[1] + 2*(cowl_wall() + cowl_flange())];
cw_cav   = [cowl_window()[0] + 2*cowl_clear(),
            cowl_window()[1] + 2*cowl_clear()];

module cowl_3d() {
    difference() {
        //  the body: a flanged tray, flange at z = 0 against the back panel
        union() {
            linear_extrude(cowl_h()) rrect(cw_outer[0] - 2*cowl_flange(),
                                           cw_outer[1] - 2*cowl_flange(),
                                           corner_r);
            linear_extrude(cowl_floor())
                rrect(cw_outer[0], cw_outer[1], corner_r);
        }
        //  the cavity the holder sits in, open at the top (against the panel)
        translate([0, 0, cowl_floor()])
            linear_extrude(cowl_h())
                rrect(cw_cav[0], cw_cav[1], corner_r);
        //  four clearance holes, on the same centres as the carrier's inserts
        for (p = cowl_screw_pts())
            translate([p[0] - cowl_xy()[0], p[1] - cowl_xy()[1], -1])
                cylinder(d = screw_m25_clear_d, h = cowl_h() + 2);
    }
}

cowl_3d();
