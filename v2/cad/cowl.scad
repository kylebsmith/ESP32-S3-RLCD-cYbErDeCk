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
//  It is held by four M2.5 x 6 passing up through the back panel into four
//  captive nuts sunk in the carrier's floor, so cowl, panel and carrier are one
//  fastener stack and nothing is bonded. Take four screws out and the holder is
//  reachable without disturbing anything else. THEY USED TO GO INTO INSERTS AND
//  THREE OF THE FOUR INSERTS DID NOT EXIST; parameters.scad 12's
//  cowl_screw_pts() carries the four measurements and the depth map that says
//  why no four-insert pattern can exist under this cowl at all.
//
//  PRINT IT CLOSED SIDE DOWN, floor on the bed, open side up. Inverted it
//  would have to bridge the cavity's floor -- 1.500 over an 88.270 x 22.373
//  opening -- which is far too wide. This way the only upward-facing surfaces
//  are the cavity's own floor, the rim that lands on the panel, and the four
//  head counterbores' ceilings, and the face on the bed is the one nobody sees.
//  THAT THIRD ITEM IS NEW AND THIS HEADER USED TO SAY "no overhang anywhere":
//  each counterbore is a O4.900 void opening at the bed face, so 1.280 of PETG
//  bridges 4.900 above it. 4.900 is a routine bridge and the alternative was
//  the deck standing on four screw heads -- see cw_head_cb.
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

//  THE HEAD HAS TO BE BURIED, AND THE DECK USED TO STAND ON IT. This file cut
//  four plain O2.900 holes with no counterbore, so an ISO 4762 M2.5 cap bore on
//  the cowl's OUTER face -- the lowest surface on the whole deck. MEASURED and
//  derived from the z stack: deck_bot_z -9.525 less cowl_h() 3.820 puts that
//  face at -13.345, screw_m25_head_h 2.500 puts the head's underside at
//  -15.845, and the desk is deck_bot_z - foot_h = -15.240. The heads stood
//  0.605 BELOW the plane the four feet stand on, in the middle of a 304.600 mm
//  span. foot_h_min = cowl_h() + 1.270 is the rule that is supposed to prevent
//  exactly this and it reasons about the cowl's BODY only; check_cad.py reports
//  it as a tagged datum it cannot evaluate, because the right-hand side calls a
//  function, so nothing gated it either.
//    4 x grid sinks the head 0.040 PROUD OF NOTHING -- it is 0.040 recessed --
//  so the cowl's own face governs the desk clearance again at the full
//  foot_h - cowl_h() = 1.895, and foot_h_min's derivation becomes true of the
//  part instead of nearly true. It leaves 1.280 of PETG over the head: a
//  12.2435 mm2 annulus in pure COMPRESSION between the head and the panel, not
//  a wall and not a floor spanning a cavity, which is what cowl_floor()'s 1.500
//  is justified as. [MEASURED off export/stl/cowl.stl: the four horizontal
//  facet groups at z = 2.540 are 4 x 12.2435 = 48.9738 mm2. This line used to
//  read 12.2522, which is pi/4 (4.900^2 - 2.900^2) on ideal circles; the part
//  is drawn at $fn = 96 and the 96-gon annulus is 0.0087 mm2 smaller.]
//    The shallowest counterbore that still satisfies the 1.270 rule is 1.875;
//  2.540 was taken instead because it costs 0.595 of a membrane that carries
//  nothing and buys the whole 0.625 of air back.
cw_head_cb = 2.540;     // [DESIGN] 4 x grid, 0.040 over screw_m25_head_h

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
        //  four clearance holes, on the same centres as the carrier's nut
        //  pockets, each with its head SUNK -- see cw_head_cb
        //    THE COUNTERBORE'S +0.400 IS NOT THE +0.200 CONVENTION AND MUST
        //  NOT BE "CORRECTED" TO IT. nut_m25_pocket_af and pot_bore_d both add
        //  0.200 to bought metal, but each of those bores LOCATES the metal in
        //  it: the nut is captive in its pocket and the bushing is a press in
        //  its bore, so 0.200 is assembly slop and nothing else. This
        //  counterbore locates nothing. The screw is located by the O2.900
        //  clearance hole above it, which is screw_m25_clear_d = 2.500 + 0.400,
        //  so the shank -- and with it the concentric O4.500 head -- may sit
        //  0.200 off this bore's axis in any direction. The head's edge then
        //  reaches 2.250 + 0.200 = 2.450, which is exactly this bore's radius:
        //  +0.400 here is the SAME 0.400 the clearance hole carries, and it is
        //  the condition for the head to seat on the counterbore's ceiling
        //  wherever the nut puts the screw. At +0.200 the bore is r 2.350 and
        //  the head can bind on its wall 0.100 before bearing, which is a
        //  screw that never develops preload. It would also cut the ceiling
        //  annulus from 12.2435 to 10.7366 mm2 -- 12.3 % less bearing, both
        //  figures as the 96-gons the part is actually drawn with -- and buy
        //  only 0.100 of web. At the cowl_screw_dx() of 48.895 this paragraph
        //  was first written against, that would have left 0.9542 against the
        //  1.500 this part is drawn to and so would not even have solved the
        //  problem it was proposed for. THAT dx IS GONE: at 47.625 the web is
        //  1.9654 as drawn and +0.200 measures 2.0653, so the door buys
        //  0.0999 of a web that no longer needs it.
        //    AND IT IS SHUT FOR A REASON THAT HAS NOTHING TO DO WITH THE WEB,
        //  which is why it stays shut if dx ever moves back: the shank is
        //  O2.500 in a screw_m25_clear_d of 2.900, so it can sit 0.200 off
        //  the bore's axis, and the O4.500 head's edge then reaches 2.450.
        //  head_d + 0.400 gives a bore radius of 2.450 and the head passes at
        //  exactly nil; head_d + 0.200 gives 2.350 and the head BINDS on the
        //  counterbore by 0.100. A counterbore a cap head does not enter is
        //  not a counterbore.
        //    [0.9542 DERIVED at the abandoned dx; 1.9654 and 2.0653 MEASURED
        //  off exports of this file, the second with +0.200 substituted]
        for (p = cowl_screw_pts())
            translate([p[0] - cowl_xy()[0], p[1] - cowl_xy()[1], -1]) {
                cylinder(d = screw_m25_clear_d, h = cowl_h() + 2);
                cylinder(d = screw_m25_head_d + 0.400, h = cw_head_cb + 1);
            }
        //  AND THE TWO WASHER SCALLOPS. Not a choice and not decoration:
        //  back_screw_pts()[4] and [5] carry a O6.000 DIN 125 washer under a
        //  O4.500 cap head, and the head's bearing face and this part's rim
        //  land on the SAME face -- the panel's underside at z -9.525.
        //  MEASURED off this file's own earlier export, the washers overlapped
        //  that rim by 6.1013 and 8.4883 mm2 and 2.0420 mm in x, the bare heads
        //  by 2.7415 and 3.7751, and even the bare O2.900 holes by 0.4908 and
        //  0.7424: the deck as drawn could not be closed.
        //    WHY A SCALLOP AND NOT A SMALLER FLANGE. parameters.scad 12's
        //  cowl_flange() has the brackets. Retreating clear of the washers caps
        //  the flange at 5.078 and then no hole on it reaches sound floor;
        //  growing past them wants 11.578 and the foot at back_screw_pts()[3]
        //  caps it at 10.657. Both doors are shut, so the flange stays 7.620
        //  and gives way only where the washer actually is.
        //    IT CROSSES THE EDGE AND DOES NOT KISS IT, which is this project's
        //  CGAL lesson: the centre stands 0.958 OUTSIDE the rim and the radius
        //  is 3.500, so the circle cuts 2.542 in. The bounding box does not
        //  move -- the rim still reaches x -53.755 either side of the bite --
        //  which is why export.py's plan transcription still measures
        //  window + 2*(wall + flange).
        //    WHICH POINTS, DERIVED AND NOT TYPED: any back_screw_pts() whose
        //  relief envelope reaches this plan at all. The test is the plan's
        //  rectangle grown by the relief's radius, and it selects exactly two
        //  -- the next nearest misses in y by 26.842.
        for (p = back_screw_pts())
            if (abs(p[0] - cowl_xy()[0]) < cw_outer[0]/2 + cowl_washer_relief_d()/2
             && abs(p[1] - cowl_xy()[1]) < cw_outer[1]/2 + cowl_washer_relief_d()/2)
                translate([p[0] - cowl_xy()[0], p[1] - cowl_xy()[1], -1])
                    cylinder(d = cowl_washer_relief_d(), h = cowl_h() + 2);
    }
}

cowl_3d();
