// keyboard_blank.scad - a parametric stand-in for the Keychron K9 Pro.
//
// WHAT THIS IS
//   A volume to design the v2 enclosure around, not a model of a keyboard.
//   No keycaps, no switches, no ports, no feet - just the envelope the shell
//   has to clear. Keycaps are deliberately not modelled: a bay needs to know
//   how far the key band stands proud of the case, which is one number, not
//   61 solids.
//
// GEOMETRY, IN ONE PARAGRAPH
//   The K9 Pro has a FLAT BOTTOM and a WEDGED TOP. Keychron publishes the
//   case height at the front edge (7.6) and at the rear edge (13.3); both are
//   measured from the surface the keyboard rests on, so the underside is the
//   datum plane and the slope lives in the top shell. A flat pocket floor is
//   therefore correct - see the note on v2/research/keyboard.md's bay caveat
//   in v2/research/keyboard-k9pro-model.md section 6.
//
// SOURCING
//   Every default below is tagged. Tags used:
//     [vendor]    published by Keychron on the K9 Pro product page
//     [derived]   computed here from [vendor] or [qmk] numbers, arithmetic shown
//     [qmk]       from the K9 Pro QMK keyboard definition (key positions)
//     [UNSOURCED] NOT a measurement. Guarded default. Get the calipers.
//   Provenance and the full argument: v2/research/keyboard-k9pro-model.md
//
// RENDER
//   openscad -D 'part="envelope"' -o blank.stl v2/cad/keyboard_blank.scad
//   An empty or tiny mesh is a failure symptom, not a quiet pass.

// ---------------------------------------------------------------- parameters

// Outside footprint of the keyboard as it ships (ABS + aluminium frame; there
// is no plastic-frame variant of the K9 Pro).
kb_w       = 287.0;   // [vendor] width,  across the keyboard
kb_d       = 97.5;    // [vendor] depth,  front-to-back, drives deck height

// Case height at the two edges, from the resting plane, WITHOUT keycaps.
kb_h_front = 7.6;     // [vendor]
kb_h_rear  = 13.3;    // [vendor]

// How far the keycap tops stand above the resting plane, at each edge.
// The rear figure is corroborated twice (see research doc section 4); the
// FRONT figure is the weakest number in this file - two defensible routes
// disagree by 2.8 mm. The larger value is used, because for an aperture an
// over-estimate costs clearance and an under-estimate costs a reprint.
kc_top_front = 19.8;  // [vendor-page, third-party] CONTESTED: 17.0 by the other route
kc_top_rear  = 23.5;  // [vendor-page, third-party] corroborated to within 0.2

// Switch field (the key band that must come through an aperture).
// 15 u x 5 u at 19.05 mm pitch, from the 61-key ANSI QMK layout.
field_w = 15 * 19.05; // [derived from qmk] = 285.75
field_d =  5 * 19.05; // [derived from qmk] =  95.25

// Corner radius of the outside footprint.
// DEFAULT 0 IS DELIBERATE: a sharp-cornered blank is the largest, and so the
// safest, envelope, and the real radius is not published. Set it once measured.
corner_r = 0;         // [UNSOURCED] real board has some radius; 0 = conservative

// Clearance to add all round when this blank is used to cut a pocket. Left at
// 0 here so the blank is the keyboard, not the pocket; v1's rule was +1.35 on
// width and +0.95 on depth (cad/parameters.scad:748-749).
clearance = 0;

// part = "envelope" | "shell" | "keycaps"
//   envelope - everything the keyboard occupies. Use this to cut a bay.
//   shell    - the case body only. This is what sits down in the pocket.
//   keycaps  - the band that must stand proud through the aperture.
part = "envelope";

// ------------------------------------------------------------------- derived

// The frame around the switch field, per side. Note how small this comes out:
// the K9 Pro is very nearly a bare 60% plate with a lip. That thinness is
// real but it is derived, not measured - it is on the calipers list.
bezel_x = (kb_w - field_w) / 2;   // = (287    - 285.75) / 2 = 0.625
bezel_y = (kb_d - field_d) / 2;   // = ( 97.5  -  95.25) / 2 = 1.125

// Top-shell wedge angle. Comes out at 3.35 deg, which is what Keychron
// publishes as the no-feet typing angle (3.4 deg) - an independent check that
// these three numbers belong to the same object.
wedge_deg = atan((kb_h_rear - kb_h_front) / kb_d);

// ------------------------------------------------------------------- modules

// A flat-bottomed solid whose top slopes linearly from h_front at y=0 to
// h_rear at y=d. Built by extruding the side profile, so the top face is
// exactly planar and the solid is manifold by construction.
module wedge(w, d, h_front, h_rear, r = 0) {
    if (r > 0) {
        // Rounded footprint: intersect the wedge with a rounded-corner prism.
        intersection() {
            wedge(w, d, h_front, h_rear, 0);
            linear_extrude(height = max(h_front, h_rear) + 1)
                offset(r = r) offset(r = -r) square([w, d]);
        }
    } else {
        // rotate([90,0,90]) maps local (a,b,c) -> (c,a,b): the profile's x
        // becomes depth, its y becomes height, the extrusion becomes width.
        rotate([90, 0, 90])
            linear_extrude(height = w)
                polygon([[0, 0], [d, 0], [d, h_rear], [0, h_front]]);
    }
}

// The case body: full footprint, no keycaps.
module kb_shell() {
    wedge(kb_w + 2*clearance, kb_d + 2*clearance,
          kb_h_front, kb_h_rear, corner_r);
}

// The keycap band: inset to the switch field, standing from the resting plane
// up to the keycap tops. Modelled from z=0 (not from the shell top) so that
// unioning it with the shell gives a closed envelope with no seam.
module kb_keycaps() {
    translate([bezel_x, bezel_y, 0])
        wedge(field_w, field_d, kc_top_front, kc_top_rear, 0);
}

// Everything the keyboard occupies.
module kb_envelope() {
    union() {
        kb_shell();
        kb_keycaps();
    }
}

// ---------------------------------------------------------------------- main

if (part == "envelope")     kb_envelope();
else if (part == "shell")   kb_shell();
else if (part == "keycaps") kb_keycaps();
else assert(false, "part must be envelope, shell or keycaps");

echo(str("kb_w=", kb_w, " kb_d=", kb_d,
         " wedge_deg=", wedge_deg,
         " bezel_x=", bezel_x, " bezel_y=", bezel_y,
         " tallest=", kc_top_rear));
