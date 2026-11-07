// ===========================================================================
//  cYbErDeCk v2 - knob_mould.scad
//  The two-part printed mould the ENCODER knob can be cast in, instead of
//  printed. The brief allows either: "3D printed or cast in acrylic from a
//  printed mould". knob.scad is the printed route; this is the other one.
//
//      openscad -D 'half="brim"' -o export/stl/knob_mould_brim.stl ...
//      openscad -D 'half="plug"' -o export/stl/knob_mould_plug.stl ...
//
//  IT IS TOOLING, NOT PART OF THE INSTRUMENT. Nothing in the deck references
//  it and nothing in it may change the deck. Every number below is either a
//  datum of the knob -- taken from parameters.scad 5, never retyped -- or a
//  km_-prefixed decision about the tool.
//
//  WHAT WAS CHECKED BEFORE THE PARTING LINE WAS CHOSEN
//  --------------------------------------------------
//  The brief for this part says the flutes and the indicator groove are
//  undercut-free in ONE DIRECTION ONLY and to check that before choosing the
//  split. Checked, feature by feature, against knob.scad as drawn:
//
//    the 12 flutes      cut by cylinders whose axes are PARALLEL TO Z, at a
//                       fixed radius of knob_brim_d/2 + knob_flute_d/2 -
//                       knob_flute_cut = 13.4875. A prism swept along z has
//                       no undercut along z in either direction. Their inner
//                       extent is 13.4875 - 1.5875 = 11.900, which is 2.900
//                       clear of the plug's 9.000 radius, so they never touch
//                       the plug and never make a step in it.
//    the indicator      a cube in the top face, 2.000 x 1.000 deep, running
//                       from the axis to the rim. Walls parallel to z, open
//                       at the top face and open at the rim: no undercut
//                       along z, and nothing to trap along the radius either.
//    the brim/plug step the ONE undercut in the part. The brim overhangs the
//                       plug by (knob_brim_d - knob_body_d)/2 = 3.700 and
//                       faces -z. It cannot be drawn out of a plug-shaped
//                       cavity in either direction, which is exactly why
//                       there are two halves and why the parting plane has to
//                       be at it.
//    the bore           counterbore O12.000 x 6.350 and D-bore O6.100 x
//                       8.000, both opening at the plug's end face and blind
//                       at the top: no undercut along -z. Dealt with below;
//                       only the counterbore is cast.
//
//  So the part is monotone in z and the parting plane must sit where the
//  silhouette is widest, which is the brim: anywhere in z = acrylic_t ..
//  acrylic_t + knob_proud works, and the two ends of that window are the only
//  candidates worth having.
//
//      z = acrylic_t + knob_proud (the top face)  the brim half becomes a
//          flat lid and the whole part comes out of the other half, which
//          also opens. But the flash lands as a ring around the TOP FACE --
//          "the only face anyone ever looks at" (knob.scad) and the one that
//          carries the only legend on the deck. Rejected on that alone.
//      z = acrylic_t (the brim's underside)       the flash lands on the
//          brim's underside outer edge. And the knob is located by its SHAFT,
//          not by that face -- knob_h - knob_proud == acrylic_t means the
//          plug's end finishes flush with the acrylic's underside, so the
//          brim's underside hangs over the O22.000 bore with 1.700 of
//          overhang and touches NOTHING. A flash line there is cosmetic only,
//          on a face you see at a grazing angle, and it dresses off against a
//          flat stone in seconds.
//
//  Anywhere strictly between the two puts the flash as a ring around the
//  middle of the grip, which is the worst of both. So: acrylic_t.
//
//  WHAT IS CAST AND WHAT IS NOT, AND IT IS A COMPUTED DECISION
//  ----------------------------------------------------------
//  THE D-BORE IS NOT CAST. A core has to draw, so it has to be drafted, and
//  drafting this one destroys it:
//
//      knob_shaft_bore_h = 8.000 of core at 1 degree
//      taper = 2 * 8.000 * tan(1 deg) = 0.279 diametral
//      knob_bore_d - knob_bore_shaft_d = 0.100, the whole fit allowance
//
//  so the draft is 2.8x the fit it has to live inside, and the bore would
//  accept a straight O6.000 shaft over about a third of its length. A loose
//  printed core pin avoids the taper and loses on tolerance instead: +/-0.1
//  on a 6 mm printed feature against a 0.100 fit. The bore is the one
//  precision feature on this part -- the flat carries the entire torque --
//  so it is DRILLED AND FILED after casting: O6.0 drill 8.000 deep, piloted
//  by the cast counterbore's own floor, then the 4.600 flat.
//  THE COUNTERBORE IS CAST, because it is the opposite case: it exists to
//  swallow a O6.8 bushing and an M7 nut, so its tolerance is 2.000 of
//  clearance and 1 degree of draft over 6.350 costs 0.222 of it.
//    AND THE GATE LIVES INSIDE IT. The sprue is O4.000 on the axis, landing
//  on the counterbore's floor, so the O6.0 drill that makes the bore removes
//  the entire gate stub with 1.000 of margin all round. There is no gate
//  scar to dress anywhere on the part.
//
//  THE POT KNOB CANNOT TAKE THIS ROUTE AT ALL, which is worth stating because
//  the obvious assumption is that one mould serves all six. knob_pot's grip
//  is six internal ribs 0.300 deep in a O6.100 bore: a cast core for them
//  would need draft on a 0.300 interference, which is not a feature, and
//  machining them is six internal keyways in a 6 mm bore, which is a broach.
//  So the cast route is for the THREE ENCODER KNOBS; the three pot knobs are
//  printed, or all six are.
//
//  SHRINKAGE, AND IT IS NOT SOURCED
//  --------------------------------
//  km_shrink is the linear cure shrinkage of the casting resin and this
//  repository cannot source it for a specific product. What was found:
//  0.2-0.5 % linear for denture-base PMMA (adjacent chemistry, heat-cured),
//  about 8 % VOLUMETRIC for the same, 0.5-0.8 % for injection-moulded PMMA
//  (a different process), and a patent claiming 8-21 % for casting acrylic
//  without saying whether it is linear or volumetric. None of that is a
//  number to cut a tool to, so it is a PARAMETER with a stated default and
//  the mould is correct for a resin whose linear shrinkage is km_shrink.
//
//  WHAT SAVES IT: this part has no dimension tight enough for the allowance
//  to matter, and that is computed rather than hoped:
//
//      feature                         clearance it has   1 % of it
//      plug O18.000 in a O22.000 bore   2.000 per side      0.090
//      brim covering the same bore      1.700 of overhang   0.127
//      counterbore over an M7 nut       1.000 per side      0.060
//      height, 19.050                   nothing bears on    0.191
//                                       either end face
//      the D-bore                       drilled afterwards  n/a
//
//  So anywhere in 0 .. 1 % the knob still fits, still covers the bore, and
//  still stands proud by 6.350 to within 0.2. The allowance is applied as an
//  isotropic scale of the cavity -- one scale() in one place -- because a
//  cast monomer shrinks isotropically and a per-axis allowance would be
//  inventing anisotropy nobody measured.
//  TO CLOSE IT: cast one knob, measure the brim's diameter with calipers, and
//  set km_shrink = 1 - measured/25.400.
//
//  PRINT EACH HALF PARTING FACE UP. Both are exported in their ASSEMBLED
//  positions -- the brim half above the parting plane, the plug half below --
//  so the two STLs can be loaded together and seen to close, and so the draft
//  can be measured along the real pull axis. For printing, the plug half is
//  already parting-face-up; the brim half gets flipped. Parting face up puts
//  every cavity opening upward, which is the only orientation with no bridge:
//  the alternative bridges a O25.400 cavity mouth.
// ===========================================================================

include <parameters.scad>
use <lib/util.scad>

half  = "brim";                 // "brim" | "plug"
part  = "3d";                   // export.py passes it; this file has no 2D

//  EVERY LOCAL IS km_-PREFIXED. parameters.scad owns knob_*, and a bare
//  `draft` or `block` here is one keystroke from the scope accident that
//  silently redefined the deck's depth: the LAST assignment of a name in a
//  scope wins for the WHOLE scope.
km_draft   = 1.0;               // [DESIGN] degrees, on every drawn surface.
                                //   Bracketed: under about 0.5 a printed
                                //   cavity drags on its own layer lines, and
                                //   over about 3 the taper starts showing in
                                //   the part. 1.0 is the usual figure for a
                                //   cast part with no texture.
km_shrink  = 0.005;             // [PROVISIONAL] 0.5 % linear. NOT SOURCED for
                                //   any specific resin -- see the header. The
                                //   mould is correct for a resin at this
                                //   figure and the table above shows it is
                                //   correct anyway over 0 .. 1 %.
km_t       = tan(km_draft);
km_s       = 1 + km_shrink;     // [DERIVED] the cavity is this much over size
                                //   so the cured part lands on nominal

km_part    = acrylic_t * km_s;  // [DERIVED] 12.7635, the parting plane
km_block   = 50.800;            // [DESIGN] 2 in square. Sized from the bolts
                                //   outward, not chosen: a O3.400 hole at
                                //   km_bolt needs its own edge distance.
km_floor   = 3.175;             // [DESIGN] 0.125 in behind each cavity. THIS
                                //   IS A POURED MOULD, not an injection tool
                                //   -- the only pressure in it is the head of
                                //   resin in the sprue, about 1 kPa -- so the
                                //   floor's job is stiffness at the parting
                                //   line, where a bowed half becomes flash.
km_bolt    = 17.145;            // [DERIVED] 27 x grid, on both axes, so the
                                //   four bolts sit at r = 24.24: that is
                                //   9.78 clear of the brim cavity's 12.764
                                //   edge and leaves 6.56 of material to the
                                //   block's own edge.
km_reg_r   = 20.320;            // [DERIVED] 32 x grid = 0.8 in. Between the
                                //   cavity and the bolts, 5.06 of land to the
                                //   cavity's edge.
km_reg_d   = [5.000, 3.000];    // [DESIGN] a frustum, base and tip
km_reg_h   = 2.540;             // [DESIGN] 0.1 in
km_reg_fit = 0.100;             // [DESIGN] the socket's radial clearance. A
                                //   cone in a cone SELF-CENTRES: the pair
                                //   contacts on its whole conical surface and
                                //   the clearance only makes the cone sit
                                //   0.100/tan(21.5 deg) = 0.254 deeper, which
                                //   is why the socket is also 0.300 deeper
                                //   than the cone is tall. The cone must
                                //   never bottom, or it holds the halves
                                //   apart and every closure is flash.
km_sprue_d = 4.000;             // [DERIVED] under the O6.000 drill that makes
                                //   the bore, with 1.000 of margin per side,
                                //   so the gate stub is drilled away whole
km_pour_d  = 10.000;            // [DESIGN] the funnel's mouth on the outside
km_vent    = [0.600, 0.200];    // [DESIGN] width and depth of the vent lands.
                                //   0.200 passes air and stops a monomer
                                //   syrup running far. They are cut in the
                                //   plug cavity's FLOOR, which is the one
                                //   surface air can be trapped against when
                                //   the mould stands plug-half-up, and they
                                //   start inside the trapped annulus itself
                                //   so the air has nowhere to sit.

//  THREE REGISTRATION CONES AND NOT FOUR, for the same reason the deck has a
//  rocking problem with four feet and would not with three: three points fix
//  a plane and cannot fight each other. A fourth cone on a printed parting
//  face is a fourth height to get wrong.
km_reg_a   = [90, 210, 330];    // [DESIGN]

// ---------------------------------------------------------------------------
//  THE CAST PART, which is NOT the printed part
// ---------------------------------------------------------------------------
//  knob.scad's solid is drawn with no draft, because a printed part needs
//  none. The cast part is a different shape by the draft and the allowance,
//  so it is drawn here rather than imported, and the difference is the whole
//  content of this file. Every dimension still comes from parameters.scad 5.
//
//  Drafted so that each half's cavity is WIDEST AT ITS MOUTH, which is the
//  parting plane for both: the plug tapers down going away from the parting
//  plane, the brim tapers down going up from it.
module km_cast_solid() {
    difference() {
        union() {
            //  the plug: full at the parting plane, smaller at its end face
            cylinder(d1 = knob_body_d - 2*acrylic_t*km_t,
                     d2 = knob_body_d, h = acrylic_t);
            //  the brim: full at the parting plane, smaller at the top face
            translate([0, 0, acrylic_t])
                cylinder(d1 = knob_brim_d,
                         d2 = knob_brim_d - 2*knob_proud*km_t,
                         h = knob_proud);
        }
        //  The flutes, as CONES on the same axes knob.scad uses. The axis
        //  radius does not move; the cutter grows toward the parting plane,
        //  so the scallop is knob_flute_cut deep at the top face and
        //  knob_flute_cut + knob_proud*tan(draft) = 0.911 deep at the parting
        //  plane, and the ridge that forms it in the brim half is drafted.
        //  Extended 1.000 past both ends: the cutter must CROSS the brim's
        //  wall rather than stop on it, which is the tangency that makes CGAL
        //  answer "the given mesh is not closed" and drop the part.
        for (i = [0 : knob_flutes - 1])
            rotate(i * 360 / knob_flutes)
                translate([knob_brim_d/2 + knob_flute_d/2 - knob_flute_cut,
                           0, acrylic_t - 1])
                    cylinder(d1 = knob_flute_d + 2*(knob_proud + 1)*km_t,
                             d2 = knob_flute_d - 2*km_t,
                             h = knob_proud + 2);
        //  The indicator, drafted the only way it can be: wider at the top
        //  face than at its floor, so the ridge in the brim half draws.
        km_mark_w0 = knob_mark[0] - 2*knob_mark[1]*km_t;
        km_mark_w1 = knob_mark[0] + 2*km_t;
        hull() {
            translate([0, -km_mark_w0/2,
                       acrylic_t + knob_proud - knob_mark[1]])
                cube([knob_brim_d/2, km_mark_w0, eps]);
            translate([0, -km_mark_w1/2, acrylic_t + knob_proud + 1])
                cube([knob_brim_d/2, km_mark_w1, eps]);
        }
        //  The counterbore, which IS cast: full at the plug's end face,
        //  smaller going in, so the boss that forms it draws out of the
        //  casting. Overshoots the end face by 1.000 for the same reason.
        translate([0, 0, -1])
            cylinder(d1 = knob_counterbore_d + 2*km_t,
                     d2 = knob_counterbore_d - 2*knob_counterbore_h*km_t,
                     h = knob_counterbore_h + 1);
    }
}

//  The cavity: the cast part at cavity size. ONE scale(), in ONE place.
module km_cavity() { scale(km_s) km_cast_solid(); }

// ---------------------------------------------------------------------------
//  FEATURES SHARED BY BOTH HALVES
// ---------------------------------------------------------------------------
module km_bolts(z0, h) {
    for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * km_bolt, sy * km_bolt, z0 - 1])
            cylinder(d = screw_m3_clear_back, h = h + 2);
}

module km_reg_cones() {
    for (a = km_reg_a) rotate(a) translate([km_reg_r, 0, km_part])
        cylinder(d1 = km_reg_d[0], d2 = km_reg_d[1], h = km_reg_h);
}

module km_reg_sockets() {
    for (a = km_reg_a) rotate(a) translate([km_reg_r, 0, km_part - eps])
        cylinder(d1 = km_reg_d[0] + 2*km_reg_fit,
                 d2 = km_reg_d[1] + 2*km_reg_fit, h = km_reg_h + 0.300);
}

// ---------------------------------------------------------------------------
//  THE BRIM HALF: everything above the parting plane
// ---------------------------------------------------------------------------
module km_brim_half() {
    km_h = knob_proud * km_s + km_floor;
    difference() {
        translate([0, 0, km_part])
            rbox(km_block, km_block, km_h, corner_r);
        km_cavity();
        km_reg_sockets();
        km_bolts(km_part, km_h);
    }
}

// ---------------------------------------------------------------------------
//  THE PLUG HALF: everything below it, plus the counterbore's boss, the gate
//  and the vents
// ---------------------------------------------------------------------------
module km_plug_half() {
    difference() {
        union() {
            translate([0, 0, -km_floor])
                rbox(km_block, km_block, km_part + km_floor, corner_r);
            km_reg_cones();
        }
        km_cavity();
        //  The gate, up the axis and through the counterbore's boss. It
        //  overshoots the boss's top by 1.000 into the cavity rather than
        //  stopping on it: the first cut of this file ended the sprue exactly
        //  ON that face, and a cut that ends on a face is the coplanar half
        //  of the family of CGAL failures this repo has paid for.
        translate([0, 0, -km_floor - 1])
            cylinder(d = km_sprue_d,
                     h = km_floor + knob_counterbore_h*km_s + 2);
        //  and its funnel, on the outside face
        translate([0, 0, -km_floor - eps])
            cylinder(d1 = km_pour_d, d2 = km_sprue_d, h = km_floor);
        //  THE VENTS. Lands in the cavity's floor running out through the
        //  block's wall: 4 of them on the axes, which clears the bolts at 45
        //  degrees and the cones at 90/210/330 by construction. They start at
        //  r = 6.800, inside the annulus the air is trapped in, so the air
        //  does not have to travel to reach one.
        for (a = [0, 90, 180, 270]) rotate(a)
            translate([6.800, -km_vent[0]/2, 0])
                cube([km_block, km_vent[0], km_vent[1]]);
    }
}

if (half == "brim") km_brim_half();
else                km_plug_half();
