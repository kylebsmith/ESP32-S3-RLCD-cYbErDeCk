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
//  Plus two non-structural prints: the button sprue and (optionally) a
//  printed screen protector. An optional 2 mm laser-cut acrylic window drops
//  in behind the display aperture.
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
//    part = "chassis" | "backplate" | "buttons" | "window" | "assembly"
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

inner_w = body_w - 2 * wall;                 // clear interior width
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
boss_flank = (inner_w - board_pocket_w) / 2;          // available strip width
boss_cx    = board_pocket_w/2 + boss_flank/2 + 0.4;   // nudged outboard so the
                                                      // boss merges into the
                                                      // side wall and clears
                                                      // the board pocket

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

plate_half_h = (body_h - 2*wall - 2*fit_slide) / 2;
plate_edge_margin = 0.8;        // [DESIGN] material left outboard of a countersink

boss_rows = [ board_bay_cy - board_pocket_h/2 + m3_boss_d/2,
              plate_half_h - m3_cs_head_d/2 - plate_edge_margin ];

function boss_positions() = [ for (sx = [-1, 1], cy = boss_rows) [sx * boss_cx, cy] ];

// Published so an accessory bracket can be drilled to match. These four screws
// are the deck's rigging points - see parameters.scad section 5.
function accessory_pattern() = boss_positions();

assert(boss_rows[0] + m3_boss_d/2 <= port_cut_lo - 0.8,
       "lower fastener row fouls the USB-C tunnel");
assert(boss_rows[1] - m3_boss_d/2 >= port_cut_hi + 0.8,
       "upper fastener row fouls the microSD tunnel: shrink tf_open_w or m3_boss_wall");
assert(boss_rows[1] + m3_cs_head_d/2 <= plate_half_h - 0.5,
       "upper fastener countersink breaks out of the back plate edge");
assert(abs(boss_rows[0]) + m3_cs_head_d/2 <= plate_half_h - 0.5,
       "lower fastener countersink breaks out of the back plate edge");

// Bottom-edge tongue and groove. The back plate's bottom lip slides into a
// groove in the chassis bottom wall, so the plate is mechanically captured on
// its bottom edge and only needs screws along the top.
tongue_t     = 1.6;                  // [DESIGN] 4 extrusions
tongue_depth = 3.0;                  // [DESIGN] engagement length
tongue_z     = back_t / 2 - tongue_t / 2;

// The keyboard bay is shallower than the board bay. The back plate carries a
// raised pad over the keyboard so the keyboard is held forward against the
// front face's retaining lip. Derived, not chosen.
kbd_keeper_t = board_depth - kbd_depth;      // = 1.6 mm

assert(boss_flank >= m3_boss_d - 1.0,
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

            // --- fastener bosses, rising from the front face rearward --------
            for (p = boss_positions())
                translate([p[0], p[1], z_back_inner])
                    cylinder(h = z_front_inner - z_back_inner, d = m3_boss_d);

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
            rbox(board_pocket_w, board_pocket_h, board_depth + 1, board_pocket_r);

        // --- keyboard bay ---------------------------------------------------
        // Cut clear through to the back-plate seating plane, NOT merely to
        // kbd_depth. Stopping at kbd_depth would leave a 1.6 mm web across the
        // bay in exactly the volume the back plate's keeper pad occupies: the
        // two would collide and the keyboard could not be loaded at all. The
        // keeper pad, not the chassis, is what sets the keyboard's depth.
        translate([0, kbd_bay_cy, z_back_inner])
            rbox(kbd_pocket_w, kbd_pocket_h,
                 z_front_inner - z_back_inner + 1, kbd_pocket_corner_r);

        // --- display aperture, with the reference's draft angle --------------
        translate([board_cx + display_off_x, board_bay_cy + display_off_y,
                   z_front_inner - 0.01])
            rse_aperture(display_aper_w, display_aper_h, front_t + 0.02,
                         aper_blend_display, form_n, display_aper_draft);

        // --- keyboard aperture ----------------------------------------------
        translate([0, kbd_bay_cy, z_front_inner - 0.01])
            rse_aperture(kbd_aper_w, kbd_aper_h, front_t + 0.02,
                         aper_blend_kbd, form_n, 0.6);

        // --- control cluster recess ------------------------------------------
        // The three buttons sit in ONE shallow dish rather than in three bare
        // holes, so the cluster reads as a single considered element instead of
        // as perforation. Cut from the outer face inward, flaring outward, so
        // the recess has no hard rim.
        top_wall_y = body_h/2;
        if (dish_enable)
            translate([board_cx, top_wall_y + 0.01,
                       pcb_back_z + button_w_centre])
                rotate([90, 0, 0])
                    mirror([0, 0, 1])
                        rse_aperture((button_count - 1) * button_pitch
                                     + button_aper_w + 2 * dish_margin,
                                     button_aper_h + 2 * dish_margin,
                                     dish_depth + 0.01, dish_blend, form_n,
                                     dish_flare);

        // --- top-edge apertures: three buttons, two microphones --------------
        for (i = [0 : button_count - 1]) {
            bx = board_cx + (i - (button_count - 1)/2) * button_pitch;
            translate([bx, top_wall_y - wall/2, pcb_back_z + button_w_centre])
                rotate([90, 0, 0])
                    rbox(button_aper_w, button_aper_h, wall + 2, 1.0);
        }
        for (sx = [-1, 1])
            translate([board_cx + sx * mic_offset_x, top_wall_y - wall/2,
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
        for (prt = [[usbc_off_y, usbc_open_w, usbc_open_h, usbc_w_centre],
                    [tf_off_y,   tf_open_w,   tf_open_h,   tf_w_centre]])
            translate([board_pocket_w/2 - 1,
                       board_cy + prt[0],
                       pcb_back_z + prt[3]])
                rotate([0, 90, 0])
                    rbox(prt[2], prt[1], body_w/2 - board_pocket_w/2 + 2, 1.2);

        // --- keyboard service access windows ---------------------------------
        // Reaches the keyboard's power slide switch and its charging port,
        // which share one short edge. Cut on both sides so the keyboard can go
        // in either way round.
        kbd_floor   = z_front_inner - kbd_depth;
        kbd_top_edge = kbd_bay_cy + kbd_pocket_h/2;
        for (sx = (kbd_access_both_sides ? [-1, 1] : [1]))
            translate([sx * (kbd_pocket_w/2 - 1),
                       kbd_top_edge - kbd_access_from_edge - kbd_access_w/2,
                       kbd_floor + kbd_access_above_floor + kbd_access_h/2])
                rotate([0, sx * 90, 0])
                    rbox(kbd_access_h, kbd_access_w, wall + 3, 1.5);

        // --- bottom-edge tongue groove ---------------------------------------
        translate([0, -body_h/2 + wall - tongue_depth/2 + 0.01, tongue_z])
            cube([inner_w - 2*corner_gusset, tongue_depth + 0.02, tongue_t],
                 center = true);

        // --- back face is open ------------------------------------------------
        // Everything rearward of the back plate's seating plane is removed,
        // except the bosses and gussets added above.
        translate([0, 0, -0.01])
            rse_plate(inner_w, body_h - 2*wall, z_back_inner + 0.01,
                      cavity_blend, form_n);

        // --- heat-set insert bores, drilled from the back ---------------------
        for (p = boss_positions())
            translate([p[0], p[1], z_back_inner - 0.01])
                cylinder(h = m3_bore_depth, d = m3_insert_bore);
    }
}


// ===========================================================================
//  BACK PLATE
// ===========================================================================

module backplate() {
    plate_w = inner_w - 2 * fit_slide;
    plate_h = body_h - 2*wall - 2 * fit_slide;

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
            rse_soft(plate_w, plate_h, back_t, cavity_blend, form_n,
                     plate_edge_soft, plate_edge_roll);

            // --- keyboard keeper pad ----------------------------------------
            // Holds the keyboard forward against the front face's lip.
            translate([0, kbd_bay_cy, back_t - 0.01])
                rbox(kbd_pocket_w - 2*fit_slide, kbd_pocket_h - 2*fit_slide,
                     kbd_keeper_t + 0.01, 6.0);

            // --- bottom tongue ----------------------------------------------
            // Engages the groove in the chassis bottom wall. Length is set so
            // the tip stops short of the groove's blind end, and so the tongue
            // never reaches the chassis outer face.
            // Root the tongue 1.0 mm INSIDE the plate outline. The plate's
            // perimeter is rolled, so at the tongue's height the edge has
            // already drawn back ~0.25 mm; a tongue that starts at the nominal
            // outline floats free of it and the part renders as two bodies.
            tongue_len = tongue_depth - 0.4 + 1.0;
            translate([0, -plate_h/2 - tongue_len/2 + 1.0, tongue_z])
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

            // --- stiffening ribs across the keyboard bay ---------------------
            // They run in the plate's weakest direction: the long span between
            // the bottom tongue and the spine.
            for (i = [-1, 0, 1])
                translate([i * 34, kbd_bay_cy, back_t + kbd_keeper_t/2 - 0.01])
                    cube([3.0, kbd_pocket_h - 8, kbd_keeper_t], center = true);

        }

        // --- back-plate fastener holes, countersunk -------------------------
        for (p = boss_positions())
            translate([p[0], p[1], 0])
                countersunk_hole(m3_clear, m3_cs_head_d, m3_cs_head_h, back_t);

        // --- board mounting holes, M2.5, counterbored from outside ----------
        for (sx = [-1, 1], sy = [-1, 1])
            translate([board_cx + sx * board_mount_pitch_x/2,
                       board_cy + sy * board_mount_pitch_y/2, 0])
                counterbored_hole(board_screw_clear, 4.6, 1.6, back_t);

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
            cylinder(h = back_t + kbd_keeper_t + 0.02, d1 = kbd_eject_d,
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
    span = (button_count - 1) * button_pitch;
    web_t = 0.8;
    for (i = [0 : button_count - 1]) {
        x = (i - (button_count - 1)/2) * button_pitch;
        translate([x, 0, 0]) {
            // outer cap, passes through the aperture
            rbox(button_cap_w, button_cap_h, wall + 0.6, 0.8);
            // retaining flange, sits behind the wall
            translate([0, 0, wall + 0.6 - 0.01])
                rbox(button_cap_w + 2*button_flange,
                     button_cap_h + 2*button_flange, 1.2, 1.0);
            // actuator post, reaches the switch
            translate([0, 0, wall + 1.8 - 0.01])
                cylinder(h = 1.6, d = 3.0);
        }
    }
    // connecting webs
    for (i = [0 : button_count - 2]) {
        x = (i - (button_count - 1)/2 + 0.5) * button_pitch;
        translate([x, 0, wall + 0.6 + 0.6])
            cube([button_pitch, 2.0, web_t], center = true);
    }
}


// ===========================================================================
//  OPTIONAL ACRYLIC WINDOW  (reference outline, for laser cutting)
// ===========================================================================

module window() {
    rbox(window_w, window_h, window_t, window_corner_r);
}


// ===========================================================================
//  ASSEMBLY AND PLATE VIEWS
// ===========================================================================

//  Component colours are deliberately flat and unsaturated: these views exist
//  to show fit and arrangement, not to sell anything.
module assembly(explode = 0) {
    e = explode;
    color("#cfcabf")                      chassis();
    color("#b6b0a4") translate([0, 0, -e * 1.6])  backplate();
    color("#2f6b45", 0.95)
        translate([board_cx, board_cy, z_front_inner - board_depth - e * 0.9])
            mock_board();
    color("#2b2e33", 0.95)
        translate([0, kbd_bay_cy, z_front_inner - kbd_depth - e * 0.55])
            mock_keyboard();
    color("#a8342b")
        for (i = [0 : button_count - 1])
            translate([board_cx + (i - (button_count - 1)/2) * button_pitch,
                       body_h/2 + 0.3 + e * 0.5,
                       pcb_back_z + button_w_centre])
                rotate([90, 0, 0]) buttons_single();
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
else if (part == "window")    window();
else if (part == "plate")     plate();
else if (part == "exploded")  assembly(explode = 14);
else                          assembly();
