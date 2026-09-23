//  carrycase.scad - a two-part sleeve the whole deck slides into.
//
//  Front half and back half, split on a plane parallel to the face, drawn
//  together by eight M5 socket screws into hex nuts trapped at the joint. Every
//  port is buried. Meant to be flocked inside and filled, sanded and polished
//  outside, so the only things that break the surface are the eight screw heads
//  and the two strap rails - and those are meant to be seen.
//
//  WHY TWO PARTS. Not for printing alone. One piece could be printed; it could
//  not be reached into. The magnets, the flock and the bed all want the inside
//  open, and a plane does that. See parameters.scad, IT IS TWO PARTS, BOLTED.
//
//  HOW IT HOLDS THE DECK. Three things, none of them a snap feature:
//    - the cowl channel, which keys the deck in X and in rotation for its whole
//      travel and bottoms it out on the floor
//    - the flock, a compliant interference fit once it is in
//    - gravity, because it is carried mouth-up
//  The four magnets are a seat at the end of that travel, not a latch. The
//  numbers are in parameters.scad under MAGNETS and they are not flattering.
//
//  PRINTING. Front half face-down, back half back-down. Both are then a flat
//  bed face and an open tray, no bridge, no support, and the two surfaces that
//  get looked at are the two that touch glass.
//
//  Coordinates are the deck's own, so alignment needs no arithmetic: the deck
//  occupies x +-58.125, y +-70.125, z -11.00 (cowl crown) to +16.85 (front).

include <parameters.scad>
use <lib/util.scad>
use <cyberdeck.scad>

part = "assembly";   // front | back | assembly | fitcheck
$fn = 64;

//  ---------------------------------------------------------------------------
//  One cross-section, taken at an inset. Every chamfer in the part is a hull
//  between two of these, so there is exactly one place the outline is defined.
//  The slab and the rails are hulled SEPARATELY - together they are not
//  convex, and hull() of the pair would quietly fill the step where a rail
//  meets the flank, which is the one line of this form that has to stay hard.
module slab_slice(ins, t) {
    translate([0, case_cy, 0])
        rse_plate(case_w - 2*ins, case_h - 2*ins, t, max(case_r - ins, 0.8), form_n);
}

module rail_slice(sx, ins, t) {
    xo = case_rail_x - ins;       // outer face, pulled in by the chamfer
    xi = case_w / 2 - 4;          // rooted inside the slab, never seen
    y0 = case_rail_y0 + ins;
    y1 = case_rail_y1 - ins;
    translate([sx * (xo + xi) / 2, (y0 + y1) / 2, 0])
        rbox(xo - xi, y1 - y0, t, max(case_rail_r - ins, 0.8));
}

//  A half of the outer solid. ch0 is the chamfer at z0, ch1 at z1; the caller
//  passes the seam chamfer at the joint and the edge chamfer at the free face.
module half_body(z0, z1, ch0, ch1) {
    e = 0.01;
    union() {
        hull() {
            translate([0,0,z0])       slab_slice(ch0, e);
            translate([0,0,z0+ch0])   slab_slice(0,   e);
            translate([0,0,z1-ch1])   slab_slice(0,   e);
            translate([0,0,z1-e])     slab_slice(ch1, e);
        }
        for (sx = [-1, 1]) hull() {
            translate([0,0,z0])       rail_slice(sx, ch0, e);
            translate([0,0,z0+ch0])   rail_slice(sx, 0,   e);
            translate([0,0,z1-ch1])   rail_slice(sx, 0,   e);
            translate([0,0,z1-e])     rail_slice(sx, ch1, e);
        }
    }
}

//  Everything the deck occupies, plus the flock allowance, running out the top.
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

//  The strap slot: straight through the rail, front to back, so a split ring or
//  webbing passes through the object rather than round a hook. It shares the
//  rail with the four fasteners and sits between two of them, so the joint is
//  clamped exactly where the strap pulls on it.
module case_lug_slots() {
    for (sx = [-1, 1])
        translate([sx * case_lug_x, case_lug_y, case_z0 - 1])
            rbox(case_lug_slot_w, case_lug_slot_h,
                 case_z1 - case_z0 + 2, case_lug_slot_r);
}

//  Screw clearance, blind: it stops short of the back face so the back stays an
//  unbroken surface and there is nowhere for grit to get in.
module bolt_holes() {
    for (sx = [-1, 1], y = case_bolt_ys)
        translate([sx * case_bolt_x, y, case_bolt_end])
            cylinder(d = case_bolt_clear, h = case_z1 + 1 - case_bolt_end);
}

//  Hex pockets open at the parting face. Drop eight nuts in, close the case,
//  drive from the front with one key.
module nut_pockets() {
    for (sx = [-1, 1], y = case_bolt_ys)
        translate([sx * case_bolt_x, y, case_split_z - case_nut_h])
            cylinder(d = case_nut_cd, h = case_nut_h + 0.01, $fn = 6);
}

module pins(d, h, z) {
    for (sx = [-1, 1], y = case_pin_ys)
        translate([sx * case_bolt_x, y, z]) cylinder(d = d, h = h);
}

//  Magnet pockets, opening at the cavity face of the front half - which is the
//  face pointing at the ceiling while it prints. The disc is glued in flush and
//  the flock goes over it.
module case_magnets() {
    for (m = magnet_sites())
        translate([m[0], m[1], case_z_fr - 0.01])
            cylinder(d = magnet_bore, h = magnet_pocket_h + 0.01);
}

module case_front() {
    difference() {
        union() {
            half_body(case_split_z, case_z1, case_seam_ch, case_edge_ch);
            pins(case_pin_d, case_pin_h + 0.01, case_split_z - case_pin_h);
        }
        case_cavity();
        case_lug_slots();
        bolt_holes();
        case_magnets();
    }
}

module case_back() {
    difference() {
        half_body(case_z0, case_split_z, case_edge_ch, case_seam_ch);
        case_cavity();
        case_lug_slots();
        bolt_holes();
        nut_pockets();
        pins(case_pin_d + case_pin_fit, case_pin_h + 0.2,
             case_split_z - case_pin_h - 0.2 + 0.01);
    }
}

if      (part == "front")    case_front();
else if (part == "back")     case_back();
else if (part == "assembly") { case_front(); case_back(); }
else if (part == "fitcheck") {
    color("DimGray")  case_back();
    color("Gainsboro") case_front();
    color("SteelBlue", 0.5) { chassis(); backplate(); }
}
else assert(false, str("unknown part: ", part));
