// ===========================================================================
//  cYbErDeCk v2 - lib/util.scad
//  Small, dependency-free helpers. No dimensions live here.
//  Carried over from v1's cad/lib/util.scad, which earned its notes the hard
//  way; the comments are kept because the traps are the same.
// ===========================================================================

// Rounded rectangle, 2D, centred. A hull of four cylinders is far cheaper than
// minkowski() and gives exact corner radii.
//  r <= 0 means a SQUARE rectangle, not a degenerate one. A hull of four
//  zero-radius circles is not a region, and in v1 that silently produced nine
//  bodies instead of one the moment a radius was corrected to zero.
module rrect(w, h, r) {
    rr = min(r, w/2 - 0.001, h/2 - 0.001);
    if (rr <= 0.005)
        square([w, h], center = true);
    else
        hull()
            for (sx = [-1, 1], sy = [-1, 1])
                translate([sx * (w/2 - rr), sy * (h/2 - rr)]) circle(r = rr);
}

// Rounded rectangular prism, centred on XY, sitting on z = 0.
module rbox(w, h, t, r) { linear_extrude(height = t) rrect(w, h, r); }

// A through hole that is guaranteed to cut: it overshoots both faces.
module thru(d, t, over = 1.0) {
    translate([0, 0, -over]) cylinder(d = d, h = t + 2*over);
}

// A slot: a 2D stadium of overall length L and width W, centred.
module slot2d(L, W) {
    hull() for (s = [-1, 1]) translate([s * (L - W)/2, 0]) circle(d = W);
}

// A heat-set insert pocket, drilled from the TOP face of a part of height t.
// CNC Kitchen pockets are blind: the hole stops short so the insert lands
// flush and the screw does not bottom out on the far wall.
module insert_pocket(d, depth, t) {
    translate([0, 0, t - depth]) cylinder(d = d, h = depth + 0.01);
}

// ONE ROUND DATUM AND N SLOTS, for a fastener pattern that bolts dissimilar
// materials together. pts is a list of [x, y] and i is the index of the datum.
// Every other hole becomes a slot of overall length L pointing BACK AT THE
// DATUM, so differential expansion runs along the slot and never across it.
//  This lives here, with no dimensions in it, so that the slab and the back
//  panel cannot drift apart: they bolt into the SAME printed bosses, so they
//  have to float the same way or one of them binds.
module datum_and_slots(pts, i, d, L) {
    for (k = [0 : len(pts) - 1])
        translate(pts[k])
            if (k == i)
                circle(d = d);
            else
                rotate(atan2(pts[i][1] - pts[k][1], pts[i][0] - pts[k][0]))
                    slot2d(L, d);
}
