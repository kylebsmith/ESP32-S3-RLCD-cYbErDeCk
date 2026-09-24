//  carrycase.scad - a two-part sleeve the whole deck slides into.
//
//  Front half and back half, split on a plane parallel to the face, drawn
//  together by thirteen M5 socket screws into hex nuts trapped at the joint.
//  Every port is buried. Meant to be flocked inside and filled, sanded and
//  polished outside. Nothing stands off it: the strap slots go straight through
//  the wall, and the only things breaking the surface are six bolt heads on the
//  front and six nuts on the back, which are meant to be seen.
//
//  WHY TWO PARTS. Not for printing alone. One piece could be printed; it could
//  not be reached into. The magnets, the flock and the bed all want the inside
//  open, and a plane does that. See parameters.scad, IT IS TWO PARTS, BOLTED.
//
//  WHY THERE ARE NO RAILS. A rail is a straight bar laid against an outline
//  that is straight in the middle and curved at the ends, so its ends always
//  land where the body is turning. Here the fasteners sit ON the outline - the
//  wall's own centreline, sampled at even arc length - so they follow the
//  superellipse round the bottom corners and there is no straight bar to clash
//  with the curve. See THE FASTENER RING in parameters.scad.
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

//  ===========================================================================
//  THE FASTENER RING
//  ===========================================================================
//  The right half of the path, as a polyline: bottom dead centre, along the
//  floor, round the bottom-right corner, up the flank, stopping short of where
//  the flank begins to turn. The corner comes out of rse_poly itself rather
//  than being approximated, so the fasteners sit on the outline the part is
//  actually cut from and cannot drift off it.
function ring_path() =
    let (W  = case_w - 2 * case_bolt_ins,
         H  = case_h - 2 * case_bolt_ins,
         C  = max(case_r - case_bolt_ins, 0.8),
         q  = 32,
         o  = rse_poly(W, H, C, form_n, q),
         ay = H / 2 - C,
         // rse_poly's fourth quadrant is the bottom-right corner, running from
         // (ax, -H/2) to (W/2, -ay).
         q4 = [ for (i = [3 * (q + 1) : 4 * (q + 1) - 1]) o[i] ])
    [ for (p = concat([[0, -H / 2]], q4,
                      [[W / 2, ay - case_bolt_top_back]]))
        [p[0], p[1] + case_cy] ];

//  Cumulative arc length, and a point at a given distance along it.
function _cum(p, i = 1, a = [0]) =
    i >= len(p) ? a : _cum(p, i + 1, concat(a, [a[i-1] + norm(p[i] - p[i-1])]));

function _at(p, a, s, i = 1) =
    i >= len(p)  ? p[len(p) - 1]
  : a[i] >= s    ? p[i-1] + (p[i] - p[i-1])
                   * ((s - a[i-1]) / max(a[i] - a[i-1], 1e-9))
                 : _at(p, a, s, i + 1);

//  f = 0 at bottom dead centre, 1 at the top of the run. Fasteners land on
//  whole pitches, locating pins on half pitches.
function ring_at(f) =
    let (p = ring_path(), a = _cum(p)) _at(p, a, a[len(a) - 1] * f);

function _mirrored(r) =
    concat(r, [ for (q = r) if (q[0] > 0.01) [-q[0], q[1]] ]);

function bolt_sites() =
    _mirrored([ for (k = [0 : case_bolt_m]) ring_at(k / case_bolt_m) ]);

function pin_sites() =
    _mirrored([ for (k = case_pin_ks) ring_at(k / case_bolt_m) ]);

//  ===========================================================================
//  FORM
//  ===========================================================================
//  THE BODY IS ONE OBJECT. THE SPLIT IS A CUT THROUGH IT, NOT A JOIN BETWEEN
//  TWO OF THEM.
//
//  The previous version rolled each half separately from the seam outward. Both
//  halves were therefore at their WIDEST at the parting plane, and the surface
//  curved away from it in both directions - so the object's widest line ran all
//  the way round at mid-height and read as a ridge. Two pillows stacked, not one
//  case. That is the crease.
//
//  So the outer form is built once, over the whole 38.25 mm, and the halves are
//  cut out of it. The roll now belongs to the OBJECT and lives at its two outer
//  faces, where an edge actually is; through the middle the sides are straight,
//  which is where the seam happens to fall.
//
//  Built as the intersection of two one-ended rolls facing opposite ways: each
//  is full width where the other is rolled, so the intersection takes the roll
//  at both faces and full width between. They cross only where both are at full
//  width AND both have zero slope, so they meet tangentially and add no line of
//  their own.
module full_body() {
    t = case_z1 - case_z0;
    intersection() {
        translate([0, case_cy, case_z0])
            rse_soft(case_w, case_h, t, case_r, form_n, case_soft, case_roll);
        translate([0, case_cy, case_z1]) mirror([0, 0, 1])
            rse_soft(case_w, case_h, t, case_r, form_n, case_soft, case_roll);
    }
}

//  One half, cut from that. case_seam_ch is a hairline, not a shadow gap: the
//  object is meant to read as one piece, so the joint gets just enough relief
//  to stop a few tenths of print mismatch showing as a step.
module half_body(flip) {
    ch = case_seam_ch;
    d  = flip ? -1 : 1;
    t  = case_z1 - case_z0;
    big = 24;
    intersection() {
        full_body();
        hull() {
            // rse_plate extrudes UPWARD from its origin, so the mirrored
            // half's pinch plate would sit 0.01 mm past the parting plane and
            // the two halves would interfere over the whole 8,800 mm2 face.
            // 88 mm3 of overlap, which is what that number was.
            translate([0, case_cy, case_split_z - (d < 0 ? 0.01 : 0)])
                rse_plate(case_w - 2*ch, case_h - 2*ch, 0.01,
                          max(case_r - ch, 0.8), form_n);
            translate([0, case_cy, case_split_z + d * ch])
                rse_plate(case_w + big, case_h + big, 0.01, case_r + big/2, form_n);
            translate([0, case_cy, case_split_z + d * (t + 1)])
                rse_plate(case_w + big, case_h + big, 0.01, case_r + big/2, form_n);
        }
    }
}

//  ===========================================================================
//  SUBTRACTIONS
//  ===========================================================================
//  Everything the deck occupies, plus the flock allowance, running out the top.
module case_cavity() {
    over = 40;                                   // run the mouth well clear
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

//  The strap slot: straight through the boss, front to back, so a split ring or
//  webbing passes through the object rather than round a hook.
module case_lug_slots() {
    for (sx = [-1, 1])
        translate([sx * case_lug_x, case_lug_y, case_z0 - 1])
            rbox(case_lug_slot_w, case_lug_slot_h,
                 case_z1 - case_z0 + 2, case_lug_slot_r);
}

//  One straight bore, right through the object. Head proud on the front face,
//  plain nut proud on the back. Nothing recessed, nothing hidden, and nowhere
//  for the clamp to short-circuit inside one half - C-43 records what that
//  looks like when it does.
module bolt_holes() {
    for (b = bolt_sites())
        translate([b[0], b[1], case_z0 - 1])
            cylinder(d = case_bolt_clear, h = case_z1 - case_z0 + 2);
}

module pins(d, h, z) {
    for (b = pin_sites()) translate([b[0], b[1], z]) cylinder(d = d, h = h);
}

//  Magnet pockets, opening at the cavity face of the front half - which is the
//  face pointing at the ceiling while it prints. The disc is glued in flush and
//  the flock goes over it.
module case_magnets() {
    for (m = magnet_sites())
        translate([m[0], m[1], case_z_fr - 0.01])
            cylinder(d = magnet_bore, h = magnet_pocket_h + 0.01);
}

//  ===========================================================================
//  PARTS
//  ===========================================================================
module case_front() {
    difference() {
        union() {
            half_body(false);
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
        half_body(true);
        case_cavity();
        case_lug_slots();
        bolt_holes();
        pins(case_pin_d + case_pin_fit, case_pin_h + 0.2,
             case_split_z - case_pin_h - 0.2 + 0.01);
    }
}

if      (part == "front")    case_front();
else if (part == "back")     case_back();
else if (part == "assembly") { case_front(); case_back(); }
else if (part == "fitcheck") {
    color("DimGray")   case_back();
    color("Gainsboro") case_front();
    color("SteelBlue", 0.5) { chassis(); backplate(); }
}
else assert(false, str("unknown part: ", part));
