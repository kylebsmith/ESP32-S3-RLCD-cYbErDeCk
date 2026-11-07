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
//  KERF. These outlines are drawn at NOMINAL. The cutter compensates. Every
//  bore is therefore drawn at the size the finished hole must be, and a
//  service that does not compensate will cut every hole one kerf oversize --
//  state it on the order. v2/docs/fabrication.md carries the number.
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

module fastener_holes() {
    for (sx = [-1, 1], dy = [screw_dy, 0, -screw_dy])
        translate([sx * screw_x, band_mid + dy]) circle(d = screw_m3_clear_d);
}

module keyboard_void() {
    translate([0, kbd_top_y - kbd_pocket[1]/2])
        rrect(kbd_pocket[0], kbd_pocket[1], corner_r);
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
        fastener_holes();
        fader_slot();
    }
}

module slab_3d() { linear_extrude(height = acrylic_t) slab_2d(); }

if (part == "2d")       slab_2d();
else if (part == "3d")  slab_3d();
else                    slab_3d();
