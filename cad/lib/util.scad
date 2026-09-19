// ===========================================================================
//  cYbErDeCk - lib/util.scad
//  Small, dependency-free geometry helpers. No dimensions live here.
// ===========================================================================

// Rounded rectangular prism, centred on X/Y, sitting on Z=0.
// Built from a hull of four cylinders: far cheaper to render than minkowski()
// and it produces exact vertical corner radii.
module rbox(w, h, t, r) {
    rr = min(r, w/2 - 0.01, h/2 - 0.01);
    hull()
        for (sx = [-1, 1], sy = [-1, 1])
            translate([sx * (w/2 - rr), sy * (h/2 - rr), 0])
                cylinder(h = t, r = rr);
}

// Rounded rectangular prism with a chamfer on the top and/or bottom arris.
// Implemented as a stack of hulled slices so the chamfer follows the radius.
module rbox_chamfered(w, h, t, r, ch_bot = 0, ch_top = 0) {
    eps = 0.001;
    union() {
        if (ch_bot > 0)
            hull() {
                translate([0, 0, 0])      rbox_plate(w - 2*ch_bot, h - 2*ch_bot, eps, max(r - ch_bot, 0.01));
                translate([0, 0, ch_bot]) rbox_plate(w, h, eps, r);
            }
        translate([0, 0, ch_bot])
            rbox(w, h, max(t - ch_bot - ch_top, eps), r);
        if (ch_top > 0)
            hull() {
                translate([0, 0, t - ch_top]) rbox_plate(w, h, eps, r);
                translate([0, 0, t - eps])    rbox_plate(w - 2*ch_top, h - 2*ch_top, eps, max(r - ch_top, 0.01));
            }
    }
}

// A vanishingly thin rounded plate, used as a hull cross-section.
module rbox_plate(w, h, t, r) {
    rr = min(r, w/2 - 0.001, h/2 - 0.001);
    hull()
        for (sx = [-1, 1], sy = [-1, 1])
            translate([sx * (w/2 - rr), sy * (h/2 - rr), 0])
                cylinder(h = t, r = rr);
}

// A rectangular aperture that flares outward along +Z, reproducing the draft
// angle on a display bezel. `flare` is the per-side increase at the far face.
module flared_aperture(w, h, t, flare, r = 1.0) {
    hull() {
        rbox_plate(w, h, 0.001, r);
        translate([0, 0, t - 0.001])
            rbox_plate(w + 2*flare, h + 2*flare, 0.001, r + flare);
    }
}

// A screw boss: outer cylinder with a blind bore for a heat-set insert.
// Rendered as a positive; call bore() separately if you need to subtract.
module insert_boss(boss_d, bore_d, height, bore_depth, from_top = true) {
    difference() {
        cylinder(h = height, d = boss_d);
        if (from_top)
            translate([0, 0, height - bore_depth + 0.01])
                cylinder(h = bore_depth, d = bore_d);
        else
            translate([0, 0, -0.01])
                cylinder(h = bore_depth, d = bore_d);
    }
}

// A counterbored through-hole for a socket-cap screw, drilled along -Z from
// z = 0 downward through `plate_t`, with the head recess at the bottom face.
module counterbored_hole(clear_d, head_d, head_h, plate_t) {
    eps = 0.01;
    translate([0, 0, -eps])
        cylinder(h = plate_t + 2*eps, d = clear_d);
    translate([0, 0, -eps])
        cylinder(h = head_h + eps, d = head_d);
}

// A row of slots, centred on the origin, running along X, stacked along Y.
module slot_row(slot_w, slot_h, pitch, count, depth, r = undef) {
    rr = is_undef(r) ? slot_h/2 * 0.98 : r;
    for (i = [0 : count - 1])
        translate([0, (i - (count - 1)/2) * pitch, 0])
            rbox(slot_w, slot_h, depth, rr);
}

// A row of slots running along Y, stacked along X.
module slot_col(slot_w, slot_h, pitch, count, depth, r = undef) {
    rr = is_undef(r) ? slot_w/2 * 0.98 : r;
    for (i = [0 : count - 1])
        translate([(i - (count - 1)/2) * pitch, 0, 0])
            rbox(slot_w, slot_h, depth, rr);
}

// An interior fillet along a vertical edge at the origin, filling the corner
// between two walls that meet at 90 degrees. `leg` is the fillet leg length.
// `qx`/`qy` are +/-1 and select which quadrant the material sits in.
module corner_fillet(leg, height, qx, qy) {
    translate([0, 0, 0])
        linear_extrude(height = height)
            difference() {
                polygon([[0, 0], [qx * leg, 0], [0, qy * leg]]);
                translate([qx * leg, qy * leg])
                    circle(r = leg, $fn = 32);
            }
}

// Simple open-ended slot through a plate, used for strap anchors.
module strap_slot(w, h, depth) {
    rbox(w, h, depth, h/2 * 0.95);
}


// A countersunk through-hole for an ISO 10642 screw, drilled along +Z through
// `plate_t`, with the 90-degree cone opening at the z = 0 face.
module countersunk_hole(clear_d, head_d, head_h, plate_t) {
    eps = 0.01;
    translate([0, 0, -eps])
        cylinder(h = plate_t + 2*eps, d = clear_d);
    translate([0, 0, -eps])
        cylinder(h = head_h + eps, d1 = head_d, d2 = clear_d);
}


// A raised ridge with VERTICAL sides and a rounded cap only at the tip.
//
// The obvious implementation - a single hull() between a wide plate at the base
// and a narrow one at the top - is wrong for a battery cowl: it tapers over the
// whole rise, so the cavity is already too narrow at the depth where the widest
// part of a cylindrical cell sits. Keeping the sides vertical until the last
// `cap_r` of rise is what actually clears an 18650.
module ridge(w, h, rise, cap_r, base_r = 8) {
    rr  = min(cap_r, h/2 - 0.02, rise - 0.02);
    br  = min(base_r, h/2 - 0.01);
    union() {
        rbox(w, h, rise - rr, br);
        translate([0, 0, rise - rr])
            hull() {
                rbox_plate(w, h, 0.001, br);
                translate([0, 0, rr])
                    rbox_plate(w - 2*rr, h - 2*rr, 0.001, max(br - rr, 0.01));
            }
    }
}
