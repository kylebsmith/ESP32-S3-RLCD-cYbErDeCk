//  concrete.scad - a cast concrete jacket for the cYbErDeCk, and the two-part
//  mould that produces it.
//
//  COORDINATES. Everything here is built SHOW FACE DOWN, because that is how it
//  is cast: z = 0 is the outer front face, lying on the mould's face plate, and
//  +z runs back toward the open end you pour into. That is the opposite of
//  cyberdeck.scad, where +z runs toward the front. The skeleton is therefore
//  mirrored into this frame rather than the frame being bent to suit it.
//
//      z = 0                       outer show face (against the face plate)
//      z = conc_t                  skeleton's own front face
//      z = conc_t + body_t         open back - screed flat, back plate goes on
//
//  PARTS
//      jacket        the concrete itself. Not printed - rendered so its volume,
//                    mass and wall can be measured before anyone mixes a bag.
//      mold_face     face plate: forms the show face and carries the aperture
//                    blockouts. Print this face UP and smooth it; its finish is
//                    the object's finish.
//      mold_collar   the four sides, drafted. Bolts down onto the face plate.
//      assembly      everything in place, for looking at.

include <parameters.scad>
use <lib/util.scad>

part = "jacket";
$fn = 64;

//  Mould footprint. Defined early because the side-feature cores are sized
//  from it.
fw = conc_w + 2*conc_draft + 2*conc_mold_wall + 2*conc_flange;
fh = conc_h + 2*conc_draft + 2*conc_mold_wall + 2*conc_flange;

// ---------------------------------------------------------------------------
//  The outer form: a drafted loft, chamfered at the show face.
//  Three sections hulled. All convex and monotonically growing, so the hull is
//  exactly the loft and not an approximation of it.
// ---------------------------------------------------------------------------
module conc_outer(grow = 0) {
    c = conc_chamfer;
    hull() {
        rse_plate(conc_w - 2*c + 2*grow, conc_h - 2*c + 2*grow, 0.01,
                  max(conc_blend - c + grow, 1.0), form_n);
        translate([0, 0, c])
            rse_plate(conc_w + 2*grow, conc_h + 2*grow, 0.01,
                      conc_blend + grow, form_n);
        translate([0, 0, conc_stack - 0.01])
            rse_plate(conc_w + 2*(conc_draft + grow), conc_h + 2*(conc_draft + grow),
                      0.01, conc_blend + conc_draft + grow, form_n);
    }
}

//  The printed chassis's outer envelope, mirrored into cast coordinates. This
//  is the surface the concrete forms against on the inside, so it is taken from
//  the same rse_soft call the chassis itself uses - not re-described.
module skeleton_envelope(grow = 0) {
    translate([0, 0, conc_t + body_t]) mirror([0, 0, 1])
        rse_soft(body_w + 2*grow, body_h + 2*grow, body_t,
                 corner_blend + grow, form_n, edge_soft, edge_roll);
}

//  One aperture through the concrete face: widest at the show face so the
//  blockout pulls, and standing conc_aper_relief proud of the plastic opening
//  so the reveal reads as a deliberate plastic edge.
module conc_aperture(w, h, blend, ramp) {
    r    = conc_aper_relief;
    land = conc_t - ramp;          // straight wall left at the aperture edge
    union() {
        // the ramp, opening outward toward the show face
        hull() {
            translate([0, 0, -0.01])
                rse_plate(w + 2*(r+ramp), h + 2*(r+ramp), 0.01,
                          blend + r + ramp, form_n);
            translate([0, 0, ramp])
                rse_plate(w + 2*r, h + 2*r, 0.01, blend + r, form_n);
        }
        // the land, straight through to the plastic
        translate([0, 0, ramp - 0.01])
            rse_plate(w + 2*r, h + 2*r, land + 0.02, blend + r, form_n);
    }
}

//  mirror([0,0,1]) flips Z ONLY - X and Y are unchanged by it. An earlier
//  version negated Y here as well and put the display aperture over the
//  keyboard. Cast coordinates share the chassis's X and Y exactly.
module conc_apertures() {
    translate([0, board_bay_cy, 0])
        conc_aperture(display_aper_w, display_aper_h, aper_blend_display,
                      conc_aper_draft);
    // The keyboard gets the deep ramp; nothing reaches into the display.
    translate([0, kbd_bay_cy, 0])
        conc_aperture(kbd_aper_w, kbd_aper_h, aper_blend_kbd,
                      conc_kbd_relief);
}

// ---------------------------------------------------------------------------
//  SIDE FEATURES. The jacket would otherwise bury the USB-C and microSD
//  tunnels and the keyboard service window under 7 mm of concrete.
//
//  They cannot be moulded by a straight pull - a hole in a side wall is an
//  undercut relative to the +Z demould direction - so each is formed by a LOOSE
//  CORE: a printed plug pushed in through the collar wall until it butts
//  against the skeleton, withdrawn sideways before the collar is lifted.
//
//  The buttons and both microphones are NOT here. They live on the top edge,
//  and burying them would mean lengthening every button cap. The jacket simply
//  stops short of that edge instead, leaving a plastic control strip along the
//  top - see conc_top_open.
pcb_back_z = z_front_inner - board_w_display_front;

//  [x-sign, y, z_chassis, opening h (along Z), opening w (along Y)]
function core_sites() = [
    [ 1, board_cy + usbc_off_y, pcb_back_z + usbc_w_centre, usbc_open_h, usbc_open_w],
    [ 1, board_cy + tf_off_y,   pcb_back_z + tf_w_centre,   tf_open_h,   tf_open_w  ],
    [-1, kbd_bay_cy + kbd_pocket_h/2 - kbd_access_from_edge - kbd_access_w/2,
         z_back_inner + kbd_access_above_floor + kbd_access_h/2,
         kbd_access_h, kbd_access_w ] ];

//  A core, in cast coordinates. len runs outward from the skeleton flank.
module core_body(sx, y, zc, oh, ow, len, grow = 0) {
    translate([sx * (body_w/2 - 0.5), y, conc_t + body_t - zc])
        rotate([0, sx * 90, 0])
            rbox(oh + 2*grow, ow + 2*grow, len, 1.2);
}

module side_features(grow = 0) {
    for (c = core_sites())
        core_body(c[0], c[1], c[2], c[3], c[4], fw/2, grow);
}

// ---------------------------------------------------------------------------
module jacket() {
    difference() {
        intersection() {
            conc_outer();
            // Stop short of the top edge: buttons and microphones live there.
            if (conc_top_open)
                translate([-fw, -fh, -1]) cube([2*fw, fh + body_h/2, conc_stack + 2]);
            else
                translate([-fw, -fh, -1]) cube([2*fw, 2*fh, conc_stack + 2]);
        }
        skeleton_envelope();
        conc_apertures();
        side_features();
    }
}

//  The loose cores, printed as one plate of three.
module cores() {
    for (i = [0 : len(core_sites()) - 1]) {
        c = core_sites()[i];
        translate([i * 30 - 30, 0, 0])
            union() {
                // the moulding face, minus a slip fit
                translate([-c[3]/2, -c[4]/2, 0])
                    cube([c[3], c[4], fw/2 - body_w/2 - conc_mold_gap]);
                // a grip past the collar's outer face
                translate([-c[3]/2 - 4, -c[4]/2 - 4, fw/2 - body_w/2 - conc_mold_gap])
                    cube([c[3] + 8, c[4] + 8, 6]);
            }
    }
}

// ---------------------------------------------------------------------------
//  Mould. Two parts, parted at the show face perimeter - the one place a seam
//  is invisible, because it lands on the chamfer. A mould split down the middle
//  would run a flash line across the front of the object.
// ---------------------------------------------------------------------------
function bolt_sites() = [
    for (sx = [-1, 1], sy = [-1, 1]) [sx * (fw/2 - conc_flange/2), sy * (fh/2 - conc_flange/2)],
    for (sx = [-1, 1])               [sx * (fw/2 - conc_flange/2), 0],
    for (sy = [-1, 1])               [0, sy * (fh/2 - conc_flange/2)] ];

function pin_sites() = [ for (sx = [-1, 1], sy = [-1, 1])
    [sx * (fw/2 - conc_flange/2), sy * (fh/2 - conc_flange - 14)] ];

module mold_face() {
    difference() {
        union() {
            translate([-fw/2, -fh/2, 0]) cube([fw, fh, conc_mold_base]);
            // aperture blockouts: the negative of the holes in the casting
            translate([0, 0, conc_mold_base - 0.01]) conc_apertures();
            // alignment dowels
            for (s = pin_sites())
                translate([s[0], s[1], conc_mold_base - 0.01])
                    cylinder(h = conc_pin_h + 0.01, d = conc_pin_d);
        }
        for (s = bolt_sites())
            translate([s[0], s[1], -1]) cylinder(h = conc_mold_base + 2, d = conc_bolt_d);
    }
}

module mold_collar() {
    h = conc_stack + 4;             // 4 mm of freeboard to screed against
    difference() {
        translate([-fw/2, -fh/2, 0]) cube([fw, fh, h]);
        // the cavity
        translate([0, 0, -0.01]) conc_outer(grow = 0);
        // clear the blockouts and the dowels
        translate([0, 0, -conc_mold_base]) {
            conc_apertures();
            for (s = pin_sites())
                translate([s[0], s[1], conc_mold_base - 0.01])
                    cylinder(h = conc_pin_h + 0.02, d = conc_pin_d + 2*conc_mold_gap);
        }
        for (s = bolt_sites())
            translate([s[0], s[1], -1]) cylinder(h = h + 2, d = conc_bolt_d);
        // loose-core passages through the collar wall
        side_features(grow = conc_mold_gap);
        // thin the flange above the parting face so it is not a solid brick
        translate([0, 0, conc_stack + 4 - 0.01])
            translate([-fw/2 - 1, -fh/2 - 1, 0]) cube([fw + 2, fh + 2, 2]);
    }
}

// ---------------------------------------------------------------------------
//  POUR DAM. A frame the back plate drops into while its outer face is flood-
//  coated with two-part acrylic. Not part of the product; it comes off once the
//  resin has gelled. Coat the plate OFF the device, cowl-down.
module pour_dam() {
    pw = inner_w - 2*fit_slide + 2*pour_dam_clear;
    ph = body_h - bottom_wall - wall - 2*fit_slide + 2*pour_dam_clear;
    h  = pour_dam_floor + back_t + pour_dam_rise;
    difference() {
        rse_plate(pw + 2*pour_dam_wall, ph + 2*pour_dam_wall, h,
                  cavity_blend + pour_dam_wall, form_n);
        // the pocket the plate sits in
        translate([0, 0, pour_dam_floor])
            rse_plate(pw, ph, h, cavity_blend + pour_dam_clear, form_n);
        // relief for the battery cowl, which points DOWN into the jig
        translate([board_cx + batt_off_x, board_cy + batt_off_y - plate_cy, -0.01])
            rse_plate(batt_cowl_w + 4, batt_cowl_h + 4,
                      pour_dam_floor + 0.02, batt_cowl_base_r + 2, form_n);
    }
}

// ---------------------------------------------------------------------------
if (part == "jacket")           jacket();
else if (part == "mold_face")   mold_face();
else if (part == "mold_collar") mold_collar();
else if (part == "cores")       cores();
else if (part == "pour_dam")    pour_dam();
else if (part == "assembly") {
    color("gray")             jacket();
    color("orange", 0.35)     skeleton_envelope();
}
else assert(false, str("unknown part: ", part));
