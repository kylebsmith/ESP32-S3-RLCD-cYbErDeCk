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


// ===========================================================================
//  ORGANIC FORM PRIMITIVES
// ===========================================================================
//  The form language is a rounded rectangle whose corners are SUPERELLIPTICAL
//  quadrants rather than circular arcs, lofted through a rolled edge profile.
//  Both choices are about curvature continuity.
//
//  A conventional rounded rectangle is an arc tangent to a line. Position and
//  tangent match at the join; curvature does not - it jumps from 1/r to zero.
//  The eye reads that discontinuity as a hard corner however large the radius
//  is, which is why a fillet can look applied rather than grown. Replacing the
//  arc with a superelliptical quadrant of a LARGER corner size gives the same
//  visual roundness while ramping curvature in from zero at the tangent point.
//
//  A pure Lame curve over the whole outline was tried first and is wrong here:
//  at this scale it cuts about 10 mm off each corner, which eats the wall and
//  loses the straight edges the design is disciplined by. Straight edges with a
//  continuous corner is the Rams reading; a global superellipse is a lozenge.
//
//    n = 2.0   circular arc, the conventional fillet
//    n = 3.2   used here; with cr = 13 it tracks an R8 arc to within 0.2 mm
//    n = 5.0   nearly square corner with a long, very soft approach

function _se(v, n) = pow(max(v, 0), 2 / n);

function rse_poly(w, h, cr, n, q) =
    let (c  = min(cr, w / 2 - 0.01, h / 2 - 0.01),
         ax = w / 2 - c,
         ay = h / 2 - c)
    concat(
        [ for (i = [0 : q]) let (t = i * 90 / q)
            [ ax + c * _se(cos(t), n),  ay + c * _se(sin(t), n) ] ],
        [ for (i = [0 : q]) let (t = i * 90 / q)
            [ -ax - c * _se(sin(t), n), ay + c * _se(cos(t), n) ] ],
        [ for (i = [0 : q]) let (t = i * 90 / q)
            [ -ax - c * _se(cos(t), n), -ay - c * _se(sin(t), n) ] ],
        [ for (i = [0 : q]) let (t = i * 90 / q)
            [ ax + c * _se(sin(t), n),  -ay - c * _se(cos(t), n) ] ]);

// A flat plate with continuous-curvature corners.
module rse_plate(w, h, t, cr, n, q = 16) {
    linear_extrude(height = max(t, 0.001))
        polygon(rse_poly(w, h, cr, n, q));
}

// Smoothstep: value and first derivative both vanish at each end, so a profile
// built from it leaves a face with zero slope and no visible arris.
function sstep(x) = x <= 0 ? 0 : (x >= 1 ? 1 : (3 * x * x - 2 * x * x * x));

// Inset fraction at normalised depth u: 1 at both faces, 0 through the middle.
function roll_f(u, roll) =
      u < roll     ? 1 - sstep(u / roll)
    : u > 1 - roll ? 1 - sstep((1 - u) / roll)
    : 0;

//  A prism whose edges roll off into both faces instead of meeting them at an
//  arris or a chamfer.
//
//  The widest section is at mid-thickness and is EXACTLY (w, h), so the part's
//  envelope is unchanged by the treatment: the softening is taken out of the
//  faces inward, never added outward. The middle (1 - 2*roll) of the thickness
//  stays at full section, so the wall is not thinned where it carries load.
//
//  Built as one polyhedron rather than a stack of hull()s - 40 hulls of 64-gons
//  is minutes of CGAL, a polyhedron is instant.
module rse_soft(w, h, t, cr, n, soft, roll, nz = 40, q = 16) {
    na = 4 * (q + 1);
    pts = [ for (i = [0 : nz], j = [0 : na - 1])
              let (u = i / nz,
                   d = 2 * soft * roll_f(u, roll),
                   p = rse_poly(w - d, h - d, max(cr - d / 2, 0.4), n, q)[j])
              [p[0], p[1], u * t] ];
    // OpenSCAD wants each face wound CLOCKWISE seen from OUTSIDE, i.e. the
    // right-hand-rule normal of the listed order points INTO the solid.
    sides = [ for (i = [0 : nz - 1], j = [0 : na - 1])
                [ i * na + j,
                  (i + 1) * na + j,
                  (i + 1) * na + (j + 1) % na,
                  i * na + (j + 1) % na ] ];
    bottom = [ for (j = [0 : na - 1]) j ];
    top    = [ for (j = [na - 1 : -1 : 0]) nz * na + j ];
    polyhedron(points = pts, faces = concat(sides, [bottom], [top]), convexity = 10);
}

// An aperture that flares outward along +Z, so the frame does not visually clip
// the panel when the deck is viewed off-axis. Corner language matches the shell.
module rse_aperture(w, h, t, cr, n, flare, q = 16) {
    hull() {
        rse_plate(w, h, 0.001, cr, n, q);
        translate([0, 0, t - 0.001])
            rse_plate(w + 2 * flare, h + 2 * flare, cr + flare, n, q);
    }
}

//  Section offset for rse_blob() at normalised rise u: positive near the base
//  (the foot fillet), easing negative toward the cap.
//  Section offset for rse_blob() at normalised rise u.
//    u < foot            the base fillet, easing out to the panel
//    foot <= u < crown   straight sides
//    u >= crown          the crown, easing in
//  The straight band in the middle is load-bearing, not styling: a blob that
//  starts crowning immediately narrows faster than whatever it has to contain,
//  and the cavity punches straight out through its own wall.
function blob_prof(u, minor, blend, cap, foot, crown) =
      u < foot  ? blend * (1 - sstep(u / foot))
    : u < crown ? 0
                : -cap * minor * 0.5 * sstep((u - crown) / (1 - crown));

//  A swelling that grows OUT of a flat surface instead of sitting on it.
//
//  The foot section is larger than the nominal footprint and is reached within
//  the first `foot` fraction of the rise, which puts a tangent fillet where the
//  form meets the panel: there is no base line to catch the eye or the light.
//  The cap eases in by the same smoothstep as the shell edges, so the whole
//  object shares one curvature language.
module rse_blob(w, h, rise, cr, n, blend = 3.0, cap = 0.34, foot = 0.20,
                crown = 0.50, nz = 32, q = 16) {
    na = 4 * (q + 1);
    pts = [ for (i = [0 : nz], j = [0 : na - 1])
              let (u = i / nz,
                   d = blob_prof(u, min(w, h), blend, cap, foot, crown),
                   p = rse_poly(w + 2 * d, h + 2 * d, max(cr + d, 0.4), n, q)[j])
              [p[0], p[1], u * rise] ];
    // OpenSCAD wants each face wound CLOCKWISE seen from OUTSIDE, i.e. the
    // right-hand-rule normal of the listed order points INTO the solid.
    sides = [ for (i = [0 : nz - 1], j = [0 : na - 1])
                [ i * na + j,
                  (i + 1) * na + j,
                  (i + 1) * na + (j + 1) % na,
                  i * na + (j + 1) % na ] ];
    bottom = [ for (j = [0 : na - 1]) j ];
    top    = [ for (j = [na - 1 : -1 : 0]) nz * na + j ];
    polyhedron(points = pts, faces = concat(sides, [bottom], [top]), convexity = 10);
}
