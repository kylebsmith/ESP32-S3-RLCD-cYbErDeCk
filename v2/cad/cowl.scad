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
//  PRINT IT CLOSED SIDE DOWN, floor on the bed, open side up. Inverted it
//  would have to bridge the cavity's floor -- 1.500 over an 88.270 x 22.373
//  opening -- which is far too wide. This way the only upward-facing surfaces
//  are the cavity's own floor and the rim that lands on the panel, there is
//  no overhang anywhere, and the face on the bed is the one nobody sees.
// ===========================================================================

include <parameters.scad>
use <lib/util.scad>

part = "3d";

//  BOTH ARE MEASURED FROM THE WINDOW, which is what the carrier's floor and
//  the back panel actually cut, so the three parts cannot disagree about it.
//  Note what that makes the MATERIAL wall, because the name does not say it:
//  cowl_wall() is 2.000 from the window to the outer face and cowl_clear() is
//  0.500 of it, so the wall is 1.500 of PETG -- 3.54 extrusion lines at a 0.4
//  nozzle, the same figure cowl_floor() is justified on. [MEASURED off
//  export/stl/cowl.stl: the body's outer plan is window + 4.000 and the
//  cavity is window + 1.000.] Not a defect and not to be "fixed": widening it
//  moves the outer footprint and leaves the four screws off-centre in a
//  flange whose position check_cad.py already holds a baseline for.
cw_outer = [cowl_window()[0] + 2*(cowl_wall() + cowl_flange()),
            cowl_window()[1] + 2*(cowl_wall() + cowl_flange())];
cw_cav   = [cowl_window()[0] + 2*cowl_clear(),
            cowl_window()[1] + 2*cowl_clear()];

module cowl_3d() {
    difference() {
        //  THE BODY IS THE FULL OUTER PLAN FOR ITS FULL HEIGHT, and the
        //  first cut of this file was not: it had a narrow tray with a
        //  7.620 flange at z = 0, commented "flange at z = 0 against the
        //  back panel". MEASURED off the export, that flange was at the
        //  WRONG END. cowl_h() = disp_18650_proud + cowl_clear() +
        //  cowl_floor() fixes the orientation beyond argument: 1.820 of
        //  holder and 0.500 of air are measured FROM the panel and the 1.500
        //  floor is the far side, so the cavity opens against the panel at
        //  z = cowl_h() and the floor is the face nearest the desk. The
        //  flange was therefore 2.320 clear of the panel, the only thing
        //  touching it was a 1.500-wide wall rim of 346.929 mm2, and the four
        //  screws spanned 2.320 of air -- so "cowl, panel and carrier are one
        //  fastener stack" (carrier.scad) was not what the part did.
        //    One solid block of the outer plan fixes it for 4672.4 mm3 of
        //  PETG, which is 5.93 g: 2360.902 mm2 lands on the panel instead of
        //  346.929, every screw passes through 3.820 of material, and the
        //  joint closes with no gap to collect dust on a visible underside.
        //  [MEASURED off export/stl/cowl.stl before and after, by summing the
        //  area of the horizontal faces at each z.] It also stays printable floor-down,
        //  which a flange at the panel end would not: a 7.620 overhanging
        //  rim 2.320 up cannot be chamfered out in 2.320 of height.
        linear_extrude(cowl_h()) rrect(cw_outer[0], cw_outer[1], corner_r);
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
