//  carrycase.scad - a sleeve the whole deck slides into.
//
//  Not a cover. Front, back, both flanks and the bottom; open only at the top.
//  Every port is buried. Meant to be flocked inside and filled, sanded and
//  polished outside until it reads as one object, so nothing here protrudes,
//  clips or hinges.
//
//  HOW IT HOLDS. Three things, none of them a snap feature:
//    - the cowl channel, which keys the deck in X and in rotation for its
//      whole travel and bottoms out on the floor
//    - the flock, which is a compliant interference fit once it is in
//    - gravity, because it is carried mouth-up
//  The four magnets add a positive seat at the end of the travel. They are not
//  the retention and are not asked to be.
//
//  PRINT MOUTH-UP, standing on the closed floor. Built that way nothing in the
//  part bridges: the floor is the first layer, the walls rise around the
//  cavity, the mouth is the last layer. Printed on its back instead, the front
//  wall would have to bridge 118 mm of open cavity.
//
//  Coordinates are the deck's own, so alignment needs no arithmetic: the deck
//  occupies x +-58.125, y +-70.125, z -11.00 (cowl crown) to +16.85 (front).

include <parameters.scad>
use <lib/util.scad>
use <cyberdeck.scad>

part = "case";
$fn = 64;

case_spine_w = case_slot_w + 2 * case_wall;   // 94.33
case_z0 = case_z_cowl - case_wall;            // deepest point of the shell
case_z1 = case_z_fr  + case_wall;             // front face
case_cav_r  = corner_blend + case_pad;
case_y_floor = case_y_bot - case_floor;

//  THE BASE IS FLAT, AND THAT IS A PRINT DECISION BEFORE IT IS A STYLE ONE.
//  The case stands on its floor to print, so it builds along +Y. A superellipse
//  outline curves OUTWARD as it leaves the bed, which is a 0 degree overhang at
//  the very bottom - 1,216 mm2 of it, measured. Squaring the base removes all
//  of it, gives the object a plinth to stand on, and costs nothing: the top
//  keeps the full soft corner, where curving inward as it rises is free.
module case_profile(w, t, r) {
    hull() {
        translate([0, case_y_floor + 2, 0]) rbox(w, 4, t, 3);
        translate([0, case_y_top - r, 0])
            rse_plate(w, 2 * r, t, r, form_n);
    }
}

//  The shell. A slab, plus a full-height spine on the back carrying the cowl
//  channel. The spine is not applied decoration - it is the only place the
//  channel can go, and it runs the full height because the deck has to slide
//  past it. Flared at its base so it grows out of the slab rather than sitting
//  on it; the flare is what a filler-and-sand finish needs to read as one form.
module case_shell() {
    union() {
        translate([0, 0, case_z_bk - case_wall])
            case_profile(case_w, case_z1 - (case_z_bk - case_wall), case_r);
        hull() {
            translate([0, 0, case_z_bk - case_wall + 0.01])
                case_profile(case_spine_w + 16, 0.01, case_r);
            translate([0, 0, case_z0])
                case_profile(case_spine_w, 0.01, max(case_r - 8, 3));
        }
    }
}

//  Everything the deck occupies, plus the flock allowance, open at the top.
module case_cavity() {
    over = 30;                                   // run the mouth well clear
    h    = (case_y_top + over) - case_y_bot;
    cy   = (case_y_top + over + case_y_bot) / 2;
    union() {
        translate([0, cy, case_z_bk])
            rse_plate(case_cav_w, h, case_z_fr - case_z_bk, case_cav_r, form_n);
        // the cowl channel, full height, which is what makes it a guide
        translate([0, cy, case_z_cowl])
            rse_plate(case_slot_w, h, case_z_bk - case_z_cowl + 0.01,
                      max(case_cav_r - 4, 2), form_n);
    }
}

//  A strap lug is a slot through the flank, nothing more. No ear, no boss, no
//  hardware. 10 mm of wall and 27 mm of depth either side of a 4 mm slot is
//  82 mm2 in shear per side - past anything a strap applies - and from outside
//  there is only a hole in a continuous surface.
module case_lugs() {
    for (sx = [-1, 1])
        translate([sx * case_lug_x, case_lug_y, case_z0 - 1])
            rbox(case_lug_w, case_lug_h, case_z1 - case_z0 + 2, case_lug_r);
}

module carrycase() {
    difference() {
        case_shell();
        case_cavity();
        case_lugs();
        // magnet pockets in the front wall, opening into the cavity. Internal
        // and flocked over, so the bridge across each one is never seen.
        for (m = magnet_sites())
            translate([m[0], m[1], case_z_fr - 0.01])
                magnet_pocket(magnet_pocket_h + 0.01);
    }
}

if (part == "case") carrycase();
else if (part == "fitcheck") {
    color("DimGray") carrycase();
    color("SteelBlue", 0.5) { chassis(); backplate(); }
}
else assert(false, str("unknown part: ", part));
