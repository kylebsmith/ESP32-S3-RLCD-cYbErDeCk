// ===========================================================================
//  cYbErDeCk v2 - slab.scad
//  The front panel: 12.70 mm CLEAR acrylic, the whole footprint, laser cut.
//
//  ONE SOURCE, TWO OUTPUTS. The cut profile is authored once as 2D geometry
//  in slab_2d(); the 3D body is that same profile extruded. The DXF the
//  cutter receives and the solid the render shows therefore cannot disagree,
//  which is the only way to be sure the picture is of the part.
//
//      openscad -D 'part="2d"' -o export/dxf/slab.dxf  v2/cad/slab.scad
//      openscad -D 'part="3d"' -o export/stl/slab.stl  v2/cad/slab.scad
//
//  WHICH WAY UP, AND WHY IT MATTERS AT THIS THICKNESS
//  --------------------------------------------------
//  Cut VISIBLE FACE UP -- the face at z = acrylic_t in the extrusion. Two
//  independent reasons, both already in the repo: the micro-chamfer lands on
//  the edge you look at, and the bore's taper cone opens TOWARDS the viewer
//  (cut it the other way up and the bore wall becomes a conical mirror
//  showing a smeared ring of the carrier plate). Same part, same bore,
//  opposite result, decided by which way up it goes on the bed.
//
//  Trotec: above 6 mm, defocus 1/3 into the material -- so the waist is about
//  4.3 mm down and the kerf opens toward the entry face. Cut visible-face-up,
//  every hole is WIDE AT THE TOP AND NARROW AT THE BOTTOM. Therefore:
//
//      the knob bores are nominal at the WIDE (visible) face -- the visible
//      diameter IS the design, and the bore can afford the taper: O22.000
//      finishes O21.083 worst case against an O18.000 knob body, 1.542 of
//      radial clearance against a 1.193 need;
//
//      everything that has to PASS something is nominal at the NARROW face,
//      which is why the M3 holes are drawn O4.445 and not O3.400. At 12.700
//      mm O3.400 finishes O2.483 and an M3 does not go through.
//
//  KERF. Drawn at NOMINAL, and deliberately KERF-AGNOSTIC: the one primary
//  source we have says a service may not compensate at all ("kerf is centered
//  on cutting lines"), so every feature here is drawn so that an
//  uncompensated 0.200 mm kerf moves it in a don't-care direction. That
//  property is only available because every load and every fit was taken out
//  of the acrylic first.
//
//  NOTHING IS COUNTERBORED. A laser cannot counterbore 12.70 mm, so the M3
//  button heads are proud and owned. The only edge treatment is by hand --
//  break the bores' top arris 0.300-0.500 with a deburring blade -- and it
//  must NOT be modelled, because a chamfer in the STL would break the
//  prismatic check in check_cad.py.
//
//  v2/docs/fabrication.md is the document all of the above comes from.
// ===========================================================================

include <parameters.scad>
use <lib/util.scad>

part = "3d";

// ---------------------------------------------------------------------------
//  The six control bores. Encoders on the upper row, pots on the lower, which
//  is the only thing distinguishing them: position is the legend, because the
//  panel carries no engraving at all.
// ---------------------------------------------------------------------------
module control_bores() {
    for (cx = [-pitch, 0, pitch], sy = [1, -1])
        translate([grid_x + cx, band_mid + sy * pitch/2]) circle(d = bore_d);
}

// ---------------------------------------------------------------------------
//  ONE ROUND DATUM AND FIVE SLOTS, and this is not belt-and-braces.
//
//  Two acrylic plates bolt into bosses printed in ONE part, over a 289.560 mm
//  screw span, and the two materials do not agree about length:
//      PMMA heat and moisture, 5 mm/m  ->  1.448 mm across that span
//      FDM boss pitch at +/-0.5 %      -> +/-1.448 mm across that span
//  Two independent terms, both about +/-1.5 mm, against zero float. Six round
//  holes is a part that either will not assemble or will bind and craze.
//
//  Each slot points back at the datum, so thermal growth runs ALONG the slot
//  and never across it. The datum is the mid hole of the +x column.
// ---------------------------------------------------------------------------
module fastener_holes(d, slot_l) {
    datum_and_slots(slab_screw_pts(), screw_datum_i, d, slot_l);
}

// ---------------------------------------------------------------------------
//  THE KEYBOARD VOID IS SMALLER THAN THE KEYBOARD. That is the whole of the
//  recessed branch (parameters.scad 8a): the slab overlaps the retained
//  aluminium frame's rim by 0.625 in X and 0.808 in Y and closes the 0.635 mm
//  gap under it, while the keycaps come up through with 0.875 / 0.943 of
//  clearance and sit 3.493 below this face.
//
//  It is NOT centred on the switch field. The field's centre is 0.250
//  rearward of the frame's, and the frame is what this edge overlaps.
// ---------------------------------------------------------------------------
module keyboard_void() {
    translate([0, kbd_void_y]) rrect(kbd_void[0], kbd_void[1], kbd_void_r);
}

module fader_slot() {
    translate([slide_x, band_mid]) rotate(90) slot2d(slider_slot_l, slider_slot_w);
}

// ---------------------------------------------------------------------------
//  The cut profile. Everything the laser does, and nothing else. The display
//  gets NO aperture: the slab runs over it to protect it, which is also what
//  makes the 6.350 underside height the governing dimension of the build.
// ---------------------------------------------------------------------------
module slab_2d() {
    difference() {
        rrect(body_w, body_h, corner_r);
        keyboard_void();
        control_bores();
        fastener_holes(screw_m3_clear_slab, screw_slot_l_slab);
        fader_slot();
    }
}

module slab_3d() { linear_extrude(height = acrylic_t) slab_2d(); }

if (part == "2d")       slab_2d();
else if (part == "3d")  slab_3d();
else                    slab_3d();
