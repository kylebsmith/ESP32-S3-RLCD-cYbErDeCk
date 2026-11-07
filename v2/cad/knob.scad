// ===========================================================================
//  cYbErDeCk v2 - knob.scad
//  The oversized knob. Six of them: three encoders, three pots.
//
//      openscad -D 'variant="encoder"' -o export/stl/knob_encoder.stl ...
//      openscad -D 'variant="pot"'     -o export/stl/knob_pot.stl ...
//
//  IT IS A FLANGED PLUG, NOT A CYLINDER, and that follows from the brief:
//  nothing panel-mounts, everything is recessed, and the knob fills the slab's
//  bore so only its top is proud. So the knob is 19.050 tall, of which 12.700
//  -- exactly the acrylic's thickness -- lives INSIDE the O22.000 bore at
//  O18.000, and 6.350 stands above the deck at O25.400.
//
//      knob_h - knob_proud == acrylic_t
//
//  is an identity, not a coincidence: it is what "the knob's underside is
//  flush with the acrylic's underside" means, and it holds at any acrylic_z0.
//
//  THE GRIP IS THE BRIM AND NOTHING ELSE. A finger cannot enter the bore --
//  the annular gap is 2.000 and MIL-STD-1472H fig 57 wants 32 mm for finger
//  access to the first joint -- so the 6.350 of proud rim does all the work.
//  Hence the flutes, and hence 25.400 rather than the 13.000 the Quad Rotary's
//  19.050 pitch would have forced. Individual encoders on a 38.100 pitch are
//  what bought the diameter.
//
//  PRINT IT BRIM DOWN. As drawn the plug is at the bottom and the brim
//  overhangs it by 3.700 radially, which is a 90-degree overhang and wants
//  support. Flipped, the brim's top face -- the one with the indicator on it,
//  and the only face anyone ever looks at -- is the bed surface, the
//  brim-to-plug transition is a step inward, the flutes stay vertical, and the
//  bore opens upward. Nothing needs support.
//
//  CAST OR PRINTED. The brief allows either: printed in PETG, or cast in
//  acrylic from a printed mould. Everything here is drawn with that second
//  route in mind -- no undercuts on the outside, the flutes are cylindrical
//  cuts with a clear draw direction, and the bore features are on the inside.
//  The mould is v2/cad/knob_mould.scad, and drawing it settled two things
//  this comment used to leave open:
//    the parting plane is at acrylic_t, the brim's underside, which is the
//      widest section and also a face that bears on NOTHING -- the knob hangs
//      on its shaft -- so the flash line there is cosmetic only;
//    "a separate core" for the bore turned out to be only half right. The
//      counterbore is cast, because 1 degree of draft over 6.350 costs 0.222
//      of a 2.000 clearance. The D-BORE IS NOT: the same draft over
//      knob_shaft_bore_h is 0.279 diametral against the 0.100 that
//      knob_bore_d - knob_bore_shaft_d allows, so it is 2.8x the whole fit.
//      It is drilled and filed after casting, and the sprue is put on the
//      axis so the drill removes the gate.
//    AND THE POT VARIANT CANNOT BE CAST AT ALL. Its six 0.300 ribs would need
//      draft on an interference, and machining them is a broach. Cast knobs
//      are the three encoders; the three pot knobs are printed.
// ===========================================================================

include <parameters.scad>
use <lib/util.scad>

variant = "encoder";            // "encoder" | "pot"

//  NOTE THE PREFIX, AND DO NOT DROP IT. These were first written without one,
//  and the deck's own depth is already called by that name in parameters.scad.
//  OpenSCAD takes the LAST assignment of a name in a scope and applies it to
//  the WHOLE scope, so the local silently redefined the deck -- and then
//  evaluated as undef, because the deck's first use comes before acrylic_t is
//  assigned. One shadowed name took out six downstream datums and the part
//  still exported. Every local in a part file gets a prefix.
kb_plug = acrylic_t;            // 12.700, the part inside the bore
kb_brim = knob_proud;           //  6.350, the part proud of the deck

// ---------------------------------------------------------------------------
//  The solid: a plug and a brim, with the flutes cut out of the brim's rim.
// ---------------------------------------------------------------------------
module knob_solid() {
    difference() {
        union() {
            cylinder(d = knob_body_d, h = kb_plug);
            translate([0, 0, kb_plug]) cylinder(d = knob_brim_d, h = kb_brim);
        }
        //  Flutes. Cut with a cylinder of knob_flute_d whose axis stands off
        //  the rim by knob_flute_cut, so the scallop is a true arc and the
        //  draw direction is straight up -- which is what makes the printed
        //  mould route possible.
        for (i = [0 : knob_flutes - 1])
            rotate(i * 360 / knob_flutes)
                translate([knob_brim_d/2 + knob_flute_d/2 - knob_flute_cut,
                           0, kb_plug - 1])
                    cylinder(d = knob_flute_d, h = kb_brim + 2);
        //  The indicator, and it is the only legend anywhere on this deck.
        translate([0, -knob_mark[0]/2, kb_plug + kb_brim - knob_mark[1]])
            cube([knob_brim_d/2, knob_mark[0], knob_mark[1] + 1]);
    }
}

// ---------------------------------------------------------------------------
//  The bore. Counterbore first -- it has to swallow the encoder's 7.000 mm
//  bushing, whose top lands 0.805 under the counterbore's floor, and the pot's
//  M7 nut, which O8.000 could never have taken.
// ---------------------------------------------------------------------------
module knob_bore() {
    translate([0, 0, -1])
        cylinder(d = knob_counterbore_d, h = knob_counterbore_h + 1);

    if (variant == "encoder") {
        //  D-bore: round, with one flat. The PEC11's flat window is 7.00 mm
        //  long and the engagement sits inside it, so the flat carries the
        //  whole torque and no grub screw is needed.
        intersection() {
            translate([0, 0, knob_counterbore_h])
                cylinder(d = knob_bore_d, h = knob_shaft_bore_h);
            translate([-knob_bore_d, -knob_bore_d/2 + (knob_bore_d - knob_bore_flat),
                       knob_counterbore_h])
                cube([2*knob_bore_d, knob_bore_d, knob_shaft_bore_h]);
        }
    } else {
        //  Round bore with six internal ribs. The ribs are MATERIAL, so they
        //  are added back in knob_3d() rather than cut here.
        translate([0, 0, knob_counterbore_h])
            cylinder(d = knob_bore_d, h = knob_shaft_bore_h);
    }
}

module knob_ribs() {
    for (i = [0 : knob_rib_n - 1])
        rotate(i * 360 / knob_rib_n)
            //  EMBEDDED 0.500 into the bore's wall, not laid against it.
            //  Flush, a rib touches the cylinder at a single point and CGAL
            //  returns six loose bodies plus the knob: the first cut of this
            //  file came back as 8 volumes.
            translate([knob_bore_d/2 - knob_rib_h, -knob_rib_w/2,
                       knob_counterbore_h])
                cube([knob_rib_h + 0.500, knob_rib_w, knob_shaft_bore_h]);
}

module knob_3d() {
    union() {
        difference() { knob_solid(); knob_bore(); }
        if (variant != "encoder") knob_ribs();
    }
}

knob_3d();
