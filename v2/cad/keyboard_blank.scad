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
//   This blank is drawn flat-bottomed with a sloping top, which is the right
//   ENVELOPE - what the shell has to clear above a flat desk - and a flat
//   pocket floor is the correct seat. But the sentence that used to be here,
//   "the underside is the datum plane and the slope lives in the top shell",
//   is WRONG, and it is worth saying so because two documents repeated it.
//   Measured off the analytic surfaces: the aluminium frame's top is ONE flat
//   planar face, normal exactly (0,+1,0), and the wedge is the ABS tray's
//   outer face - a single plane at 3.37914 deg, shared by the steel shield,
//   the tray's inner floor and the frame's skirt rim. [CAD-EXACT]
//   v2/parts/keyboard-strip.md sections 1.4 and 2.1. The consequence for the
//   enclosure is in cad/parameters.scad section 8b: strip the tray and the
//   plate is flat, so the typing angle becomes ours, and it is zero.
//
// SOURCING
//   Every default below is tagged. Tags used:
//     [vendor]    published by Keychron on the K9 Pro product page
//     [derived]   computed here from [vendor] or [qmk] numbers, arithmetic shown
//     [qmk]       from the K9 Pro QMK keyboard definition (key positions)
//     [CAD-EXACT] read off an analytic surface or hole axis in Keychron's own
//                 STEP assembly - a plane's position, a cylinder's axis. No
//                 sampling, no tessellation, no tolerance.
//     [CAD]       measured from that assembly after explicit triangulation
//                 at 0.020 mm linear; carries +/-0.02.
//     [UNSOURCED] NOT a measurement. Guarded default. Get the calipers.
//   Provenance and the full argument: v2/research/keyboard-k9pro-model.md,
//   and for everything tagged [CAD*], v2/parts/keyboard-strip.md
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
// Kept as published, and now corroborated: the tray's outer plane gives
// 7.573 at the front edge and 13.274 at the rear [CAD-EXACT], agreement to
// 0.026 and 0.027. The published pair is retained because it is the rounder
// number and the difference is inside any print tolerance - but the CAD pair
// is the one to quote if 0.027 ever matters.
kb_h_front = 7.6;     // [vendor] 7.573 [CAD-EXACT]
kb_h_rear  = 13.3;    // [vendor] 13.274 [CAD-EXACT]

// How far the keycap tops stand above the resting plane, at each edge.
//
// THERE IS ONLY ONE NUMBER HERE, AND IT IS MEASURED. The keycap plane is FLAT
// and parallel to the frame's flat top: a 1 u cap's own shell tops out 9.842
// above that face, and the per-row maxima over all 61 caps spread only 0.145
// across the whole 285 x 95 field (the high value is the 6.25 u spacebar,
// whose bounding box catches more of its own dish). So the two edge heights
// are that one offset added to the two case heights, which also makes the
// keycap band parallel to the shell top BY CONSTRUCTION, as it is in the real
// object. Before, they were two independent numbers and the band came out at
// a different angle from the case it sits on.
//
// WHAT THIS REPLACES, and why it was not merely weak: kc_top_front was 19.8,
// Keychron's published front-with-keycaps, and the old comment called it the
// weakest number in this file. It is not a measurement at all -
// 19.8 - 7.6 = 12.2 is exactly Gateron's published switch height - so the
// published figure is front case height PLUS switch height, and it is 2.2 mm
// out. The rear figure survives only as a cross-check: 13.3 + 9.842 = 23.142
// against Keychron's 23.5, i.e. 0.358 low, and 0.214 on the tallest cap.
// v2/parts/keyboard-strip.md section 2.1 and cad/parameters.scad kbd_cap_h.
kc_above_frame = 9.842;                       // [CAD] cap top above the frame
kc_top_front = kb_h_front + kc_above_frame;   // [derived] = 17.442, at the
kc_top_rear  = kb_h_rear  + kc_above_frame;   // [derived] = 23.142, CASE edges

// Switch field (the key band that must come through an aperture).
// 15 u x 5 u, and THE UNIT IS NOT 19.05. All 61 of the K9 Pro's switch holes
// land on an exact 19.000 mm lattice in BOTH axes, read off the analytic hole
// axes in Keychron's own assembly, and every position agrees with Keychron's
// own QMK LAYOUT_61_ansi to 0.000 on the 1 u keys. This file carried
// 15 x 19.05 = 285.75 and was 0.750 mm out on the width, 0.250 on the depth.
// 19.050 remains correct for the CONTROL field - it is the MX key unit and
// Adafruit's grid - which is why parameters.scad carries both, as U and kbd_U.
// The wide keys are nudged off the nominal unit centre by up to 0.040, so a
// PER-KEY position must come from the measured column in section 1.6, not from
// this product. This is a field extent, not a key position.
kbd_u   = 19.000;     // [CAD-EXACT] v2/parts/keyboard-strip.md 1.6
field_w = 15 * kbd_u; // [derived] = 285.000  (was 285.75)
field_d =  5 * kbd_u; // [derived] =  95.000  (was  95.25)

// Corner radius of the outside footprint.
// IT IS NOW MEASURED: R4.400, four Y-axis cylinders at (-6.100, 12.400),
// (-6.100, 101.100), (272.100, 12.400), (272.100, 101.100) [CAD-EXACT]
// (v2/parts/keyboard-strip.md 1.4, and cad/parameters.scad kbd_frame_r).
// The default stays 0 because a sharp-cornered blank is the LARGEST, and so
// the safest, envelope - which was always the reason; "not published" was the
// other half of the old comment and is no longer true. Set corner_r = 4.400
// to draw the real footprint; leave it 0 to cut a pocket.
corner_r = 0;         // [DESIGN] conservative; the measured radius is 4.400

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
// the K9 Pro is very nearly a bare 60% plate with a lip - and that is not a
// figure of speech. The aluminium top frame IS the switch plate: one part
// whose upper 1.200 mm carries the 61 cutouts and whose skirt is the visible
// frame. You cannot throw the frame away and keep the plate.
// This is no longer on the calipers list: both terms are measured now, the
// footprint at 287.000 x 97.500 [CAD-EXACT] and the field at 285.000 x 95.000
// on the measured lattice, so the bezel is exact arithmetic on two exact
// numbers. It grew by 0.375 in x and 0.125 in y when the pitch was corrected.
bezel_x = (kb_w - field_w) / 2;   // = (287    - 285.000) / 2 = 1.000
bezel_y = (kb_d - field_d) / 2;   // = ( 97.5  -  95.000) / 2 = 1.250

// The blank's wedge angle. Comes out at 3.35 deg, which is what Keychron
// publishes as the no-feet typing angle (3.4 deg) - an independent check that
// these three numbers belong to the same object. The angle itself is now read
// off the plane's own normal: 3.37914 deg [CAD-EXACT]. It belongs to the TRAY,
// not the top shell, and it is retired as a design input - see the header and
// cad/parameters.scad section 8b.
wedge_deg = atan((kb_h_rear - kb_h_front) / kb_d);

// The same slope as a rise per mm of depth, which is what the keycap band
// needs. kc_top_front / kc_top_rear are the cap heights at the CASE's edges;
// the band is inset by bezel_y, so taking those two values at the BAND's
// shorter span tilted the caps 0.0015 rad more than the case they sit on and
// put the rear corner 0.073 mm high. The cap plane is parallel to the frame
// top, so the band interpolates the same plane instead.
top_slope = (kb_h_rear - kb_h_front) / kb_d;   // [derived] 0.0584615 / mm

// The cap plane sampled at the BAND's own two edges, which is where the caps
// actually are. Named rather than written twice: kc_top_rear is the plane
// extrapolated to the CASE's rear edge at y = 97.5, where there is no cap, so
// it is 0.0731 above anything the model contains and must not be echoed as the
// blank's height. kc_band_rear IS the blank's tallest point, and the exported
// envelope mesh measures 23.0689 to confirm it.
kc_band_front = kc_top_front + bezel_y * top_slope;              // [derived]
kc_band_rear  = kc_top_front + (bezel_y + field_d) * top_slope;  //   17.5151
                                                                 //   23.0689

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
// unioning it with the shell gives a closed envelope with no seam. Its two
// heights are the cap plane sampled at the BAND's own front and rear edges,
// which keeps it parallel to the frame top by construction - the one thing
// the measurement is unambiguous about.
module kb_keycaps() {
    translate([bezel_x, bezel_y, 0])
        wedge(field_w, field_d, kc_band_front, kc_band_rear, 0);
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
         " kbd_u=", kbd_u, " field=", field_w, "x", field_d,
         " wedge_deg=", wedge_deg,
         " bezel_x=", bezel_x, " bezel_y=", bezel_y,
         " kc_above_frame=", kc_above_frame,
         " kc_top_front=", kc_top_front,
         " kc_top_rear=", kc_top_rear, " tallest=", kc_band_rear));
