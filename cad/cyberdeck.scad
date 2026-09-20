// ===========================================================================
//  cYbErDeCk - cyberdeck.scad
//
//  A single-form, overbuilt handheld terminal enclosure for the
//  Waveshare ESP32-S3-RLCD-4.2 and a Rii 518BT mini Bluetooth keyboard.
//
//  ARCHITECTURE
//  ------------
//  Two printed structural parts, not four:
//
//    chassis    - a monocoque. The entire front face, all four side walls and
//                 the inter-bay spine are one continuous body. There is no
//                 separate bezel, so the face the user touches has no joint
//                 across it and the shell is a closed torsion box once the
//                 back plate is fitted.
//    backplate  - the structural closure and the service access. The board
//                 bolts to it, so board + battery + back plate lift out as one
//                 module.
//
//  Plus one non-structural print: the button sprue. There is no separate
//  window or screen protector - the display sits 2.65 mm below the outer face
//  behind a 2.4 mm front panel, which is the protection.
//
//  WHY THIS IS STIFFER THAN A BEZEL-ON-TRAY DESIGN
//  -----------------------------------------------
//  A separate bezel puts a joint line across the largest face of the part, in
//  exactly the plane where bending stress from a drop is highest, and relies
//  on four screws in shear to transfer it. Making the front face continuous
//  and moving the joint to the back moves that joint into the neutral-ish
//  plane and turns the shell into a closed section. The full-depth spine
//  between the two bays then acts as a shear web across the middle, which is
//  where a device of this aspect ratio wants to fold.
//
//  USAGE
//  -----
//    part = "chassis" | "backplate" | "buttons" | "assembly"
//                     | "plate"   (everything, arranged for one print bed)
//
//    openscad -D 'part="chassis"' -o chassis.stl cad/cyberdeck.scad
//
// ===========================================================================

include <parameters.scad>
use <lib/util.scad>
use <lib/components.scad>

part = "assembly";      // override on the command line

// ---------------------------------------------------------------------------
// Derived layout - nothing here is a free choice
// ---------------------------------------------------------------------------

inner_hw = inner_w / 2;

// Spine centreline, between the two bays.
spine_cy = board_bay_cy - board_pocket_h/2 - spine/2;

// The PCB's back (component) face. Every edge-feature position Waveshare
// publishes is quoted in W, measured from this plane, so locating it once here
// lets the ports, buttons and microphones be placed from vendor data directly
// instead of from measurements of somebody else's enclosure.
pcb_back_z = z_front_inner - board_w_display_front;

// Back-plate fastener bosses. They live in the dead strips either side of the
// board pocket: the keyboard is wider than the board, so the shell interior is
// keyboard-width and the board bay leaves (inner_w - board_pocket_w)/2 of
// unused material on each flank. That is where the screws go, at zero cost to
// the footprint.

// Boss rows are NOT free. They are squeezed between two hard constraints and
// there is only about a millimetre of slack, so both are derived and asserted
// rather than chosen by eye:
//
//   lower bound  the side-port tunnels cross the SAME flank strip. A boss may
//                not overlap the band those tunnels occupy - and the band is
//                the band as actually CUT, which is the measured opening plus
//                fit_free on each side, not the measured opening alone.
//   upper bound  the back plate's countersink must stay inside the plate. The
//                plate edge sits inboard of the chassis wall by fit_slide, so a
//                row placed to suit the chassis can still break out of the
//                plate - which is exactly what an earlier revision did, and
//                what the plate_edge assert below now prevents.
port_cut_hi = board_cy + max(usbc_off_y + usbc_open_w/2, tf_off_y + tf_open_w/2);
port_cut_lo = board_cy + min(usbc_off_y - usbc_open_w/2, tf_off_y - tf_open_w/2);

function boss_positions() = [ for (sx = [-1, 1], cy = boss_rows) [sx * boss_cx, cy] ];

// Published so an accessory bracket can be drilled to match. These four screws
// are the deck's rigging points - see parameters.scad section 5.
function accessory_pattern() = boss_positions();

assert(boss_rows[0] + shell_screw_boss_d/2 <= port_cut_lo - 0.8,
       "lower fastener row fouls the USB-C tunnel");
assert(boss_rows[1] - shell_screw_boss_d/2 >= port_cut_hi + 0.8,
       "upper fastener row fouls the microSD tunnel: shrink tf_open_w or shell_screw_boss_wall");
assert(boss_rows[1] + shell_screw_cs_head_d/2 <= plate_half_h - 0.5,
       "upper fastener countersink breaks out of the back plate edge");
assert(abs(boss_rows[0]) + shell_screw_cs_head_d/2 <= plate_half_h - 0.5,
       "lower fastener countersink breaks out of the back plate edge");

// Bottom-edge tongue and groove. The back plate's bottom lip slides into a
// groove in the chassis bottom wall, so the plate is mechanically captured on
// its bottom edge and only needs screws along the top.
tongue_t     = 1.6;                  // [DESIGN] 4 extrusions
//  tongue_depth is how far the groove eats into the bottom wall, so the wall
//  left outboard of it is back_t - tongue_depth. At 3.0 that was 0.20 mm -
//  half an extrusion - and the groove effectively broke out of the bottom
//  face. 1.6 leaves 1.6 mm, four extrusions.
//
//  ENGAGEMENT IS 0.90 mm, NOT THE 1.2 THIS COMMENT USED TO CLAIM. 1.2 is the
//  groove's own depth measured from its blind end; what actually holds the
//  plate is how far the tongue reaches past the plane where the chassis has
//  material both above and below it. Measured on the rendered parts: the
//  tongue tip is at y = -67.25 and that plane is at -66.35, so 0.90 mm is
//  captured, with 0.15 mm of clearance above and below the tongue in a 1.6 mm
//  groove. It holds - but the figure is the one to check against, not 1.2.
//  See docs/DATUMS.md C-14.
//  AND back_t - tongue_depth IS NOT THE WALL EITHER. That arithmetic assumes
//  the outer face sits at the nominal envelope, and it does not: rse_soft's
//  edge roll withdraws the face by up to 1.2 mm over the back 4.7 mm of the
//  thickness, and the groove sits inside that band. Measured on the rendered
//  chassis the wall outboard of the groove was 0.480 mm - 1.2 extrusions -
//  while this line's comment, and the check that guarded it, both said 1.60.
//  Both computed from parameters and neither touched the mesh.
//  See docs/DATUMS.md C-30.
tongue_depth = 1.3;                  // [DESIGN] groove depth into the bottom wall
//  tongue_z is the groove's CENTRE, not its base: both the groove and the
//  tongue are cube(..., center = true), which centres in Z as well as X and Y.
//  Reading it as a base put the groove at z 0.000..1.600 - open to the
//  chassis's outer face, with no lip under it at all, so the tongue was not
//  captured in Z and the bottom edge of the plate could simply lift away. At
//  back_t/2 the groove sits 0.800..2.400 with 0.8 mm of chassis above and
//  below it, which is the joint the comment above describes.
//  2.2, not back_t/2. Raising the groove's centre lifts its floor from 0.80 to
//  1.40, out of the deepest part of the roll, and doubles the chassis lip under
//  it. It is a ceiling, not a round number: the capture check samples z in 0.1
//  steps up to back_t, so a groove ceiling above 3.0 leaves it nothing to
//  sample above the tongue. 2.2 puts the ceiling at exactly 3.0.
tongue_z     = 2.2;

// The keyboard bay is shallower than the board bay. The back plate carries a
// raised pad over the keyboard so the keyboard is held forward against the
// front face's retaining lip. Derived, not chosen.
kbd_keeper_t = board_depth - kbd_depth;      // = 0.25 mm

assert(boss_flank >= shell_screw_boss_d - 1.0,
       "no room beside the board pocket for the back-plate bosses");
assert(kbd_keeper_t >= 0, "keyboard bay deeper than the board bay");


// ===========================================================================
//  CHASSIS
// ===========================================================================

module chassis() {
    difference() {
        union() {
            // --- outer shell -------------------------------------------------
            rse_soft(body_w, body_h, body_t, corner_blend, form_n,
                     edge_soft, edge_roll);

            // --- magnet bosses, variant 2 only -------------------------------
            //  front_t is 2.4 and a 2.0 disc under a 0.8 skin needs 2.95, so
            //  the material is added INWARD as a local boss rather than by
            //  thinning the show face. Filleted into the face so it cannot
            //  read as a bulge or a gloss patch from outside.
            if (variant >= 2)
                for (m = magnet_sites())
                    translate([m[0], m[1], z_front_inner - magnet_boss_rise])
                        // Runs a full millimetre INTO the front panel rather
                        // than kissing it: a 0.01 mm overlap leaves CGAL a
                        // hair-thin weld and the boss comes out as a separate
                        // body in the mesh.
                        cylinder(h = magnet_boss_rise + 1.0, d = magnet_boss_d);

            // --- fastener bosses, rising from the front face rearward --------
            for (p = boss_positions())
                translate([p[0], p[1], z_back_inner])
                    cylinder(h = z_front_inner - z_back_inner, d = shell_screw_boss_d);

            // --- interior corner gussets -------------------------------------
            // Solid fillets tying the side walls to the front face. These are
            // what stops a corner impact peeling the wall off the face.
            for (sx = [-1, 1], sy = [-1, 1])
                translate([sx * inner_hw, sy * (body_h/2 - wall), z_back_inner])
                    corner_fillet(corner_gusset, z_front_inner - z_back_inner,
                                  -sx, -sy);
        }

        // --- board bay ------------------------------------------------------
        translate([board_cx, board_bay_cy, z_front_inner - board_depth])
            // +0.01, not +1. Both pockets used to overshoot a whole millimetre
            // into the front panel, which is not an epsilon - it is 1 mm off the
            // thickness of the retaining lip the component bears on, leaving
            // 1.4 mm of a 2.4 mm panel. See docs/DATUMS.md C-08.
            rbox(board_pocket_w, board_pocket_h, board_depth + 0.01, board_pocket_r);


        // --- keyboard bay ---------------------------------------------------
        // Stops kbd_keeper_t short of the panel, leaving a 0.25 mm band across
        // the bay that the keyboard bears on. That band used to be a raised pad
        // on the BACK PLATE, and printing the plate cowl-up made it the only
        // thing touching the bed: 5776.9 mm2 - 48% of the plate's underside -
        // then printed as a flat face hanging 0.25 mm over bare air. Moving the
        // step here turns it into an upward-facing ledge on solid material and
        // leaves the plate's inner face one flat plane. Nothing about the
        // keyboard's clear depth, bearing area or lip changes.
        // See docs/DATUMS.md C-32.
        //  Starts 0.2 mm BELOW the seating plane. The keyboard bay's -Y wall and
        //  the back opening's bottom edge both land on y = -body_h/2 +
        //  bottom_wall, and both cutters stopped at z = z_back_inner exactly -
        //  so the two shared an edge and CGAL left three degenerate slivers
        //  there. The bay's clear depth is unchanged; only where the cutter
        //  starts moved, and it starts inside material the opening already
        //  removes.
        translate([0, kbd_bay_cy, z_back_inner - 0.2])
            rbox(kbd_pocket_w, kbd_pocket_h,
                 z_front_inner - kbd_keeper_t - z_back_inner + 0.2,
                 kbd_pocket_corner_r);

        // --- display aperture, with the reference's draft angle --------------
        translate([board_cx + display_off_x, board_bay_cy + display_off_y,
                   z_front_inner - 0.01])
            rse_aperture(display_aper_w, display_aper_h, front_t + 0.02,
                         aper_blend_display, form_n, display_aper_draft);

        // --- keyboard aperture ----------------------------------------------
        translate([0, kbd_bay_cy, z_front_inner - 0.01])
            rse_aperture(kbd_aper_w, kbd_aper_h, front_t + 0.02,
                         aper_blend_kbd, aper_n_kbd, kbd_aper_draft);

        // Carry the keyboard aperture down through the 0.25 mm band, at the
        // same section as the flared aperture's narrow end so the two meet
        // tangentially with no step and no sliver.
        translate([0, kbd_bay_cy, z_front_inner - kbd_keeper_t - 0.01])
            rse_plate(kbd_aper_w, kbd_aper_h, kbd_keeper_t + 0.02,
                      aper_blend_kbd, aper_n_kbd);

        // --- control cluster recess ------------------------------------------
        // The three buttons sit in ONE shallow dish rather than in three bare
        // holes, so the cluster reads as a single considered element instead of
        // as perforation. Cut from the outer face inward, flaring outward, so
        // the recess has no hard rim.
        top_wall_y = body_h/2;
        //  ORIENTATION, and it was wrong. rotate([90,0,0]) maps local +Z to
        //  global -Y, so an rse_aperture built narrow-at-z=0 widens as it goes
        //  INWARD - backwards for a dish cut from the outside. The previous
        //  version tried to correct that with mirror([0,0,1]), which instead
        //  placed the whole cutter at y > top_wall_y: entirely outside the
        //  wall, removing nothing at all. See docs/DATUMS.md C-10.
        //
        //  rotate([-90,0,0]) maps local +Z to +Y, so the narrow end sits at the
        //  dish floor and it flares out to the face, which is the dish.
        if (dish_enable)
            translate([board_cx, top_wall_y - dish_depth,
                       pcb_back_z + button_w_centre])
                rotate([-90, 0, 0])
                    rse_aperture((button_count - 1) * button_pitch
                                 + button_aper_w + 2 * dish_margin,
                                 button_aper_h + 2 * dish_margin,
                                 dish_depth + 0.01, dish_blend, form_n,
                                 dish_flare);

        // --- top-edge apertures: three buttons, two microphones --------------
        //  rbox() extrudes from z = 0 to +t, NOT centred on z = 0. After
        //  rotate([90,0,0]) that runs inward from the translate point, so
        //  starting at top_wall_y - wall/2 cut a BLIND POCKET: it reached the
        //  inner face but stopped 1.60 mm short of the outer one, leaving the
        //  top wall solid. Every button and both microphones were sealed in,
        //  and 64 checks passed. Start outboard of the face instead, so the
        //  cut is unambiguously through. See docs/DATUMS.md C-10.
        for (i = [0 : button_count - 1]) {
            bx = board_cx + (i - (button_count - 1)/2) * button_pitch;
            translate([bx, top_wall_y + 1.0, pcb_back_z + button_w_centre])
                rotate([90, 0, 0])
                    rbox(button_aper_w, button_aper_h, wall + 2, 1.0);
        }
        //  Flange counterbore: ONE continuous slot, not three pockets, because
        //  the sprue is a single part and its connecting webs have to recess
        //  with the flanges. Cut from the inner face outward, so it never
        //  breaks the outer skin; wall - dish_depth - btn_cb_depth = 1.50 mm of
        //  material is left outboard of it, and that shoulder is what the
        //  flanges bear on.
        translate([board_cx, top_wall_y - wall + btn_cb_depth,
                   pcb_back_z + button_w_centre])
            rotate([90, 0, 0])
                rbox(2 * button_pitch + button_cap_w + 2 * button_flange + 0.4,
                     button_cap_h + 2 * button_flange + 0.4,
                     btn_cb_depth + 0.01, 1.2);

        for (sx = [-1, 1])
            translate([board_cx + sx * mic_offset_x, top_wall_y + 1.0,
                       pcb_back_z + mic_w_centre])
                rotate([90, 0, 0])
                    rbox(mic_aper_w, mic_aper_h, wall + 2, mic_aper_h/2 * 0.9);

        // --- side port tunnels: USB-C and microSD ---------------------------
        // Because the keyboard sets the device width, the board bay leaves a
        // solid flank either side. A side port is therefore a TUNNEL across
        // that flank, not a hole in the outer wall - cutting only the wall
        // would leave the connector buried behind ~8 mm of plastic.
        //
        // Both connectors are mid-mount: the body straddles a cut-out in the
        // PCB and sits partly behind the PCB back face, which is why each
        // opening is positioned in W rather than centred on the board.
        //  Each port is a straight tunnel with a FLARED MOUTH at the outer
        //  face. Without the flare the opening is a square-edged slot and only
        //  a slim cable will seat: a normal USB-C plug's overmould lands on the
        //  outside of the shell and holds the plug proud, so the contacts never
        //  fully engage. The flare gives the overmould somewhere to sit.
        for (prt = [[usbc_off_y, usbc_open_w, usbc_open_h, usbc_w_centre],
                    [tf_off_y,   tf_open_w,   tf_open_h,   tf_w_centre]]) {
            translate([board_pocket_w/2 - 1,
                       board_cy + prt[0],
                       pcb_back_z + prt[3]])
                rotate([0, 90, 0])
                    rbox(prt[2], prt[1], body_w/2 - board_pocket_w/2 + 2, 1.2);
            translate([body_w/2 - port_mouth_d,
                       board_cy + prt[0],
                       pcb_back_z + prt[3]])
                rotate([0, 90, 0])
                    flared_aperture(prt[2], prt[1], port_mouth_d + 0.5,
                                    port_mouth, 1.2);
        }

        // --- keyboard service access windows ---------------------------------
        // Reaches the keyboard's power slide switch and its charging port,
        // which share one short edge. Cut on both sides so the keyboard can go
        // in either way round.
        kbd_floor   = z_back_inner;   // the plate is flat now; see C-32
        kbd_top_edge = kbd_bay_cy + kbd_pocket_h/2;
        //  ONE window, on the LEFT as the device is used, positioned from the
        //  TOP of the keyboard. Measured on the real unit: the slide switch
        //  starts 10.4 mm from the keyboard's top edge and the USB-C receptacle
        //  starts 29.6 mm, so both features live in a band roughly 10.4 to 38.6
        //  down one short edge. There is nothing on the other edge.
        //
        //  Two corrections at once, see docs/DATUMS.md C-22. A previous version
        //  cut BOTH flanks "so the keyboard can go in either way round", which
        //  it cannot - the keys only read one way up. Worse, mirroring the
        //  second window about the bay centreline put the LEFT one, the only
        //  one that matters, at 15.4..49.4 from the keyboard's top: 5 mm clear
        //  of the slide switch it exists to reach.
        //
        //  In this frame +X is right and +Y is up when looking at the front
        //  face, so left-as-used is sx = -1.
        kbd_access_cy = kbd_top_edge - kbd_access_from_edge - kbd_access_w/2;
        for (sx = (kbd_access_both_sides ? [-1, 1] : [-1]))
            translate([sx * (kbd_pocket_w/2 - 1),
                       kbd_access_cy,
                       kbd_floor + kbd_access_above_floor + kbd_access_h/2])
                rotate([0, sx * 90, 0])
                    rbox(kbd_access_h, kbd_access_w, wall + 3, 1.5);

        // --- magnet pockets, variant 2 only ----------------------------------
        //  Blind, opening INWARD. The chassis prints front-face-down, so this
        //  is a hole opening upward: the show face stays continuous, normally
        //  printed solid material with nothing bridged over a void.
        if (variant >= 2)
            for (m = magnet_sites()) {
                pocket_z = body_t - magnet_skin - magnet_pocket_h;
                // access shaft, from the back-plate seating plane
                translate([m[0], m[1], z_back_inner - 0.01])
                    cylinder(d = magnet_shaft_d,
                             h = pocket_z - z_back_inner + 0.02);
                // the ribbed pocket the disc actually grips in
                translate([m[0], m[1], pocket_z])
                    magnet_pocket(magnet_pocket_h + 0.01);
            }

        // --- bottom-edge tongue groove ---------------------------------------
        //  The groove runs 0.4 mm PAST the back opening's edge, not 0.02. At a
        //  0.02 overlap the cutter's inboard face landed on the cavity boundary
        //  within CGAL's tolerance and left four zero-width slivers at the
        //  groove's ends - the chassis rendered as one body and was not
        //  watertight. The groove's OUTER edge, which is what sets the wall, is
        //  unchanged.
        translate([0, -body_h/2 + bottom_wall - tongue_depth/2 + 0.2, tongue_z])
            cube([inner_w - 2*corner_gusset, tongue_depth + 0.4, tongue_t],
                 center = true);

        // --- back face is open ------------------------------------------------
        // Everything rearward of the back plate's seating plane is removed,
        // except the bosses and gussets added above.
        translate([0, cavity_cy, -0.01])
            rse_plate(inner_w, cavity_h, z_back_inner + 0.01,
                      cavity_blend, form_n);

        // --- heat-set insert bores, drilled from the back ---------------------
        for (p = boss_positions())
            translate([p[0], p[1], z_back_inner - 0.01])
                cylinder(h = shell_screw_bore_depth, d = shell_screw_insert_bore);
    }

    //  --- keyboard locating ribs -------------------------------------------
    //  UNIONED, so they come after the difference above closes. Nothing bonds
    //  or clamps the keyboard in X and Y: it is a drop-in part held only by the
    //  front lip and the keeper pad behind it. With a bare pocket it sits
    //  wherever it lands, and at one extreme the front lip went negative at a
    //  corner. These four pairs take the play out and make the lip a
    //  guarantee. See docs/DATUMS.md C-24.
    kbd_locating_ribs();
}


// ===========================================================================
//  BACK PLATE
// ===========================================================================

//  Half-round fins on the pocket walls, tapered at the entry end because the
//  keyboard loads from BEHIND (-Z). Each cylinder is centred ON the wall plane,
//  so half of it is buried in wall material already there and only the inboard
//  half is new: the rib fuses to the wall with no seam to delaminate.
module kbd_rib(r) {
    union() {
        cylinder(r1 = 0.05, r2 = r, h = kbd_rib_lead);
        translate([0, 0, kbd_rib_lead - 0.001])
            cylinder(r = r, h = kbd_depth - kbd_rib_lead + 0.001);
    }
}

module kbd_locating_ribs() {
    //  The ribs start exactly at the keeper pad's top face, which is where the
    //  keyboard's own back face begins, and run the full pocket depth.
    kfloor = z_back_inner;   // the plate is flat now; see C-32
    for (sx = [-1, 1], dy = kbd_rib_dy)        // long axis, +-X walls
        translate([sx * kbd_pocket_w / 2, kbd_bay_cy + dy, kfloor])
            kbd_rib(kbd_rib_r_x);
    for (sy = [-1, 1], dx = kbd_rib_dx)        // short axis, +-Y walls
        translate([dx, kbd_bay_cy + sy * kbd_pocket_h / 2, kfloor])
            kbd_rib(kbd_rib_r_y);
}

//  A magnet pocket is a generous bore with crush ribs, NOT a toleranced hole.
//  The discs arrive at +-0.10 mm on diameter - a 0.20 mm band - and PLA's
//  usable press-fit window is 0.05-0.10 mm diametral, so no single bore grips a
//  whole bag. Eight ribs absorb the band by deforming. Tuning the fit is one
//  parameter, magnet_rib_h, and nothing else moves.
module magnet_pocket(h) {
    difference() {
        cylinder(d = magnet_bore, h = h);
        // Subtracted from the CUTTER, so what is left behind is material: eight
        // bumps standing magnet_rib_h proud of the bore wall.
        for (i = [0 : magnet_rib_n - 1])
            rotate([0, 0, i * 360 / magnet_rib_n])
                translate([magnet_bore / 2, 0, -0.01])
                    cylinder(r = magnet_rib_h, h = h + 0.02);
    }
}

//  THE COVER. A flat plate, because nothing on the front face stands proud to
//  work around: the glass is 2.65 mm down and the keycaps 2.80 at worst case.
//
//  It is located by two drafted platforms that drop into the apertures the
//  design already has, and held by four buried magnets. That split is the whole
//  idea - shear is only ~20% of a magnet's pull and comes from friction, so
//  magnets can never be the shear path. The platforms take every bit of it and
//  self-centre the cover as it closes; the magnets only resist lift-off.
//  One register platform: a drafted rim that drops into an aperture. Outer face
//  tapers with the aperture so it self-centres; inner face is parallel to it,
//  so the rim is a constant cover_reg_rim thick all the way round.
module register_rim(w, h, cr, n, draft) {
    difference() {
        rse_aperture(w - 2 * draft, h - 2 * draft,
                     cover_reg_depth + 0.01, cr, n, draft);
        translate([0, 0, -0.01])
            rse_aperture(w - 2 * draft - 2 * cover_reg_rim,
                         h - 2 * draft - 2 * cover_reg_rim,
                         cover_reg_depth + 0.03,
                         max(cr - cover_reg_rim, 0.4), n, draft);
    }
}

module cover() {
    difference() {
        union() {
            rse_soft(cover_w, cover_h, cover_t,
                     corner_blend - cover_gap, form_n, edge_soft, edge_roll);
            // register platforms, one per aperture, as rims
            translate([board_cx + display_off_x, board_bay_cy + display_off_y,
                       -cover_reg_depth])
                register_rim(cover_reg_w_display, cover_reg_h_display,
                             cover_reg_blend_display, form_n,
                             cover_reg_draft_display);
            translate([0, kbd_bay_cy, -cover_reg_depth])
                register_rim(cover_reg_w_kbd, cover_reg_h_kbd,
                             cover_reg_blend_kbd, aper_n_kbd,
                             cover_reg_draft_kbd);
        }
        // magnet pockets, opening on the INNER face. No skin on this side: it
        // is never seen, and halving the gap is worth more than another magnet.
        for (m = magnet_sites())
            translate([m[0], m[1], -0.01])
                magnet_pocket(magnet_pocket_h + 0.01);
        // thumb scallop - ONE affordance, on the flank opposite the keyboard
        // service window, so the intended peel starts furthest from the end
        // that is keyed deepest into its aperture.
        translate([cover_notch_x,
                   cover_h / 2 + cover_notch_r - cover_notch_depth, -1])
            cylinder(r = cover_notch_r, h = cover_t + 2);
    }
}

module backplate() {
    plate_w = inner_w - 2 * fit_slide;
    //  THE OPENING IS NO LONGER CENTRED. bottom_wall is 1.2 mm thicker than the
    //  rest of the shell, so the back opening runs from -body_h/2 + bottom_wall
    //  to +body_h/2 - wall, and its centre sits (bottom_wall - wall)/2 above
    //  the part's. A plate built symmetrically about y = 0 overlaps the bottom
    //  wall by exactly that much - 101 mm3 of interference, which is what the
    //  clash check caught. Height and centre both follow the two walls.
    plate_h  = body_h - bottom_wall - wall - 2 * fit_slide;
    plate_cy = (bottom_wall - wall) / 2;

    difference() {
        union() {
            // --- plate ------------------------------------------------------
            // A small roll on the perimeter turns the panel seam into a
            // deliberate shadow gap rather than a tolerance gap.
            // Same corner size as the cavity, with the plate 2*fit_slide
            // smaller in each axis. Keeping the blend equal rather than
            // shrinking it means the gap opens slightly at the corners
            // (0.42 mm against 0.30 mm on the straight edges) instead of
            // closing, so the plate can never bind on a corner.
            translate([0, plate_cy, 0])
                rse_soft(plate_w, plate_h, back_t, cavity_blend, form_n,
                         plate_edge_soft, plate_edge_roll);

            // --- bottom tongue ----------------------------------------------
            // Engages the groove in the chassis bottom wall. Length is set so
            // the tip stops short of the groove's blind end, and so the tongue
            // never reaches the chassis outer face.
            // Root the tongue 1.0 mm INSIDE the plate outline. The plate's
            // perimeter is rolled, so at the tongue's height the edge has
            // already drawn back ~0.25 mm; a tongue that starts at the nominal
            // outline floats free of it and the part renders as two bodies.
            //  + fit_slide so that shortening the GROOVE does not shorten the
            //  TONGUE: the tongue's length is set by how far it must reach past
            //  the capture plane, not by the groove's depth. The value stays
            //  2.20 and the tip does not move in y.
            tongue_len = tongue_depth - 0.4 + 1.0 + fit_slide;
            translate([0, plate_cy - plate_h/2 - tongue_len/2 + 1.0, tongue_z])
                cube([inner_w - 2*corner_gusset - 2*fit_slide,
                      tongue_len, tongue_t - 2*0.15], center = true);

            // --- 18650 cowl --------------------------------------------------
            // Projects from the OUTER face, so it is mirrored below z = 0: the
            // plate body occupies 0..back_t and the cowl -batt_cowl_rise..0.
            // Its rise is derived from the holder's measured protrusion past
            // the standoff plane, not copied from the reference's cover.
            if (batt_cowl_enable)
                translate([board_cx + batt_off_x, board_cy + batt_off_y, 0])
                    mirror([0, 0, 1])
                        rse_blob(batt_cowl_w, batt_cowl_h, batt_cowl_rise,
                                 batt_cowl_base_r, form_n,
                                 blend = batt_cowl_foot, cap = batt_cowl_cap,
                                 foot = batt_cowl_foot_f,
                                 crown = batt_cowl_crown);

            // --- stiffening ribs across the keyboard bay: DELETED -------------
            // Three 3.0 mm ribs used to be added here, described as running
            // "in the plate's weakest direction". They added exactly nothing.
            // cube(..., center = true) centres in Z, so at a z-centre of
            // back_t + kbd_keeper_t/2 - 0.01 and a height of kbd_keeper_t they
            // occupied z 3.19..3.44 - entirely INSIDE the keeper pad, which is
            // 3.19..3.45. Sectioning the rendered plate at z = 3.30 and 3.44
            // gives 6275.2 and 6280.2 mm^2 against a bare pad of the same area:
            // zero added material, and deleting them changes the part's volume
            // by 0.0 mm^3.
            //
            // They could not have worked as written either: kbd_keeper_t is
            // board_depth - kbd_depth = 0.25 mm, so a rib standing proud of the
            // pad would be a fraction of a millimetre tall and would come
            // straight out of the keyboard's 0.40 mm of float. The plate is
            // stiffened by being 3.2 mm of solid PETG, not by ribs that are one
            // layer high. See docs/DATUMS.md C-27.

        }

        // --- back-plate fastener holes, countersunk -------------------------
        for (p = boss_positions())
            translate([p[0], p[1], 0])
                countersunk_hole(shell_screw_clear, shell_screw_cs_head_d, shell_screw_cs_head_h, back_t);

        // --- board mounting holes, M2.5, COUNTERSUNK from outside ----------
        // The screws supplied with the board have a sloped head, and this was
        // cutting a flat-bottomed counterbore. A countersunk head in a
        // counterbore lands on the shoulder edge instead of on a cone: it does
        // not seat, it sits proud, and it wedges the bore open. ISO 10642.
        // See docs/DATUMS.md C-12.
        for (sx = [-1, 1], sy = [-1, 1])
            translate([board_cx + sx * board_mount_pitch_x/2,
                       board_cy + sy * board_mount_pitch_y/2, 0])
                countersunk_hole(board_screw_clear, board_cs_head_d,
                                 board_cs_head_h, back_t);

        // --- relief bores for the two lower board screws --------------------
        //  The cowl's flank still passes over these two countersinks, so a
        //  driver reaches them through the cowl rather than around it. Entirely
        //  at z < 0, so nothing in the plate itself is touched.
        if (batt_cowl_enable)
            for (sx = [-1, 1])
                translate([board_cx + sx * board_mount_pitch_x/2,
                           board_cy - board_mount_pitch_y/2,
                           -batt_cowl_rise - 1])
                    cylinder(h = batt_cowl_rise + 1.01, d = cowl_screw_relief_d);

        // --- 18650 bay: through the plate, and hollowed inside the cowl -----
        translate([board_cx + batt_off_x, board_cy + batt_off_y, -0.01])
            rbox(batt_bay_w + 2*fit_slide, batt_bay_h + 2*fit_slide,
                 back_t + 0.02, 2.0);
        if (batt_cowl_enable)
            translate([board_cx + batt_off_x, board_cy + batt_off_y, 0.01])
                mirror([0, 0, 1])
                    // The cavity follows the SAME easing law as the outer, one
                    // wall thickness in. A straight-sided cavity inside a
                    // crowned outer punches through the wall partway up.
                    rse_blob(batt_cowl_w - 2*batt_cowl_wall,
                             batt_cowl_h - 2*batt_cowl_wall,
                             batt_cowl_rise - batt_cowl_wall,
                             batt_cowl_base_ri, form_n,
                             blend = 0.001, cap = batt_cowl_cap,
                             foot = batt_cowl_foot_f,
                             crown = batt_cowl_crown);

        // --- speaker grille --------------------------------------------------
        translate([board_cx + grille_off_x, board_cy + grille_off_y, -0.01])
            slot_row(grille_slot_w, grille_slot_h, grille_pitch, grille_count,
                     back_t + 0.02);

        // --- expansion-header access window ----------------------------------
        // Position and size are Waveshare's own: the window they cut in their
        // stand base, 21.60 x 5.60 at header centre (U 46.150, V 33.850). The
        // reference proof-of-concept enclosure cuts the identical 21.600 x
        // 5.600 rectangle, which is an independent confirmation.
        if (expansion_win_enable)
            translate([board_cx + expansion_win_x, board_cy + expansion_win_y, -0.01])
                rbox(expansion_win_w + 2*fit_slide, expansion_win_h + 2*fit_slide,
                     back_t + 0.02, 1.5);

        // --- keyboard eject finger hole ---------------------------------------
        translate([kbd_eject_off_x, kbd_bay_cy + kbd_eject_off_y, -0.01])
            cylinder(h = back_t + 0.02, d1 = kbd_eject_d,
                     d2 = kbd_eject_d_min);


    }
}


// ===========================================================================
//  BUTTON SPRUE
// ===========================================================================
//  Printed as one connected piece, exactly as the reference does, so three
//  2 mm parts do not have to be handled individually. The connecting webs are
//  thin enough to flex and are clipped after fitting, or left in place - they
//  sit behind the wall and do not bind.

module buttons() {
    //  THE STACK, in part coordinates with z = 0 at the cap's outer tip:
    //      0.00                cap tip, 0.6 proud of the dish floor
    //      0.60                dish floor
    //      2.90                the top wall's INNER face
    //      3.44                the plunger's face, btn_reach behind it
    //      3.49 .. 3.69        where the actuator actually is, once the
    //                          board's +-0.10 mm mounting float is allowed for
    //  and the flange does NOT sit at 2.90. It sits in a counterbore cut back
    //  to 2.10, so the 0.50 mm the board leaves behind the wall stays free for
    //  the plunger and for travel. See cad/parameters.scad and DATUMS.md C-23.
    cap_len   = 0.6 + (wall - dish_depth) - btn_cb_depth;
    face_z    = 0.6 + (wall - dish_depth);            // the wall's inner face
    post_len  = face_z + btn_reach - (cap_len + btn_flange_t);

    for (i = [0 : button_count - 1]) {
        x = (i - (button_count - 1)/2) * button_pitch;
        translate([x, 0, 0]) {
            // cap: through the aperture, standing 0.6 proud of the dish floor
            rbox(button_cap_w, button_cap_h, cap_len, 0.8);
            // flange: bears on the counterbore shoulder, retains the cap
            translate([0, 0, cap_len - 0.01])
                rbox(button_cap_w + 2 * button_flange,
                     button_cap_h + 2 * button_flange, btn_flange_t, 1.0);
            // plunger: the only thing that goes behind the wall. Offset in Y so
            // it lands on the switch body and misses the PCB's edge entirely.
            translate([0, btn_post_dy, cap_len + btn_flange_t - 0.01])
                rbox(btn_post_w, btn_post_h, post_len + 0.01, 0.6);
        }
    }
    //  Connecting webs, COPLANAR with the flanges so they recess into the same
    //  counterbore. They are also the springs: at 0.4 mm thick over a 10 mm
    //  span they bend far enough for one cap to travel while its neighbours
    //  stay put, which is what lets three caps share one sprue.
    for (i = [0 : button_count - 2]) {
        x = (i - (button_count - 1)/2 + 0.5) * button_pitch;
        translate([x, 0, cap_len - 0.01 + btn_flange_t/2])
            cube([button_pitch, 2.0, btn_flange_t], center = true);
    }
}

module buttons_single() {
    rbox(button_cap_w, button_cap_h, wall + 0.6, 0.8);
}

module plate() {
    // Arranged for a 256 x 256 bed.
    translate([-body_w/2 - 4, 0, 0]) chassis();
    translate([ body_w/2 + 4, 0, 0]) backplate();
    translate([0, -body_h/2 - 12, 0]) buttons();
}


// ===========================================================================
//  DISPATCH
// ===========================================================================

if      (part == "chassis")   chassis();
else if (part == "backplate") backplate();
else if (part == "buttons")   buttons();
else if (part == "cover")     cover();
else if (part == "plate")     plate();
else if (part == "exploded")  assembly(explode = 22);
else                          assembly();
