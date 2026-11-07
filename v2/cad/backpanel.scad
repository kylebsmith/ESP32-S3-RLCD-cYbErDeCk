// ===========================================================================
//  cYbErDeCk v2 - backpanel.scad
//  The back: laser-cut sheet closing the carrier from below.
//
//  Same one-source-two-outputs rule as slab.scad -- the profile is authored
//  once in 2D and the solid is that profile extruded, so the DXF and the
//  render cannot drift apart.
//
//      openscad -D 'part="2d"' -o export/dxf/backpanel.dxf v2/cad/backpanel.scad
// ===========================================================================

include <parameters.scad>
use <lib/util.scad>

part = "3d";

//  The panel is the full footprint. It is the floor the whole object stands
//  on, so it is also what keeps a 304 mm printed carrier flat: a bowed carrier
//  under a rigid 12.70 slab is this build's real failure mode, and a stiff
//  back panel fights it from the other side.
module backpanel_2d() {
    difference() {
        rrect(body_w, body_h, corner_r);
        for (sx = [-1, 1], y = back_screw_y)
            translate([sx * screw_x, y]) circle(d = screw_m3_clear_d);
    }
}

module backpanel_3d() { linear_extrude(height = back_t) backpanel_2d(); }

if (part == "2d")      backpanel_2d();
else                   backpanel_3d();
