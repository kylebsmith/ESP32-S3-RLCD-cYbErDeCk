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
port_cut_hi = board_cy + max(side_port_a_y + side_port_a_w/2,
                             side_port_b_y + side_port_b_w/2) + fit_free;
port_cut_lo = board_cy + min(side_port_a_y - side_port_a_w/2,
                             side_port_b_y - side_port_b_w/2) - fit_free;

plate_half_h = (body_h - 2*wall - 2*fit_slide) / 2;
plate_edge_margin = 0.9;        // [DESIGN] material left outboard of a countersink

boss_rows = [ board_bay_cy - board_pocket_h/2 + m3_boss_d/2,
              plate_half_h - m3_cs_head_d/2 - plate_edge_margin ];

function boss_positions() = [ for (sx = [-1, 1], cy = boss_rows) [sx * boss_cx, cy] ];

// Published so an accessory bracket can be drilled to match. These four screws
// are the deck's rigging points - see parameters.scad section 5.
function accessory_pattern() = boss_positions();

assert(boss_rows[0] + m3_boss_d/2 <= port_cut_lo,
       "lower fastener row fouls the side-port tunnel band");
assert(boss_rows[1] - m3_boss_d/2 >= port_cut_hi,
       "upper fastener row fouls the side-port tunnel band");
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
            rbox_chamfered(body_w, body_h, body_t, corner_r,
                           ch_bot = edge_chamfer, ch_top = edge_chamfer);

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
            rbox(board_pocket_w, board_pocket_h, board_depth + 1, 3.0);

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
            flared_aperture(display_aper_w, display_aper_h,
                            front_t + 0.02, display_aper_draft, r = 2.0);

        // --- keyboard aperture ----------------------------------------------
        translate([0, kbd_bay_cy, z_front_inner - 0.01])
            flared_aperture(kbd_aper_w, kbd_aper_h, front_t + 0.02, 0.6, r = 5.0);

        // --- top-edge apertures: three buttons, two microphones --------------
        top_wall_y = body_h/2;
        for (i = [0 : button_count - 1]) {
            bx = board_cx + (i - (button_count - 1)/2) * button_pitch;
            translate([bx, top_wall_y - wall/2,
                       z_front_inner - button_z_below_front])
                rotate([90, 0, 0])
                    rbox(button_aper_w, button_aper_h, wall + 2, 1.0);
        }
        for (sx = [-1, 1])
            translate([board_cx + sx * mic_offset_x, top_wall_y - wall/2,
                       z_front_inner - mic_z_below_front])
                rotate([90, 0, 0])
                    rbox(mic_aper_w, mic_aper_h, wall + 2, mic_aper_h/2 * 0.9);

        // --- side port tunnels ------------------------------------------------
        // Because the keyboard sets the device width, the board bay leaves a
        // solid flank either side. A side port therefore has to be a TUNNEL
        // across that flank, not a hole in the outer wall - cutting only the
        // wall would leave the connector buried behind ~7 mm of plastic.
        // Cut oversize: the openings' geometry is measured, but which connector
        // is which is not yet confirmed. See docs/DATUMS.md O-02.
        for (p = [[side_port_a_y, side_port_a_w], [side_port_b_y, side_port_b_w]])
            translate([side_port_side * (board_pocket_w/2 - 1),
                       board_bay_cy + p[0],
                       z_front_inner - side_port_z_below_front])
                rotate([0, side_port_side * 90, 0])
                    rbox(side_port_h + 2*fit_free, p[1] + 2*fit_free,
                         body_w/2 - board_pocket_w/2 + 2, 1.5);

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
            rbox(inner_w, body_h - 2*wall, z_back_inner + 0.01, corner_r - wall);

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
            translate([0, 0, 0])
                rbox(plate_w, plate_h, back_t, corner_r - wall);

            // --- keyboard keeper pad ----------------------------------------
            // Holds the keyboard forward against the front face's lip.
            translate([0, kbd_bay_cy, back_t - 0.01])
                rbox(kbd_pocket_w - 2*fit_slide, kbd_pocket_h - 2*fit_slide,
                     kbd_keeper_t + 0.01, 6.0);

            // --- bottom tongue ----------------------------------------------
            // Engages the groove in the chassis bottom wall. Length is set so
            // the tip stops short of the groove's blind end, and so the tongue
            // never reaches the chassis outer face.
            tongue_len = tongue_depth - 0.4;
            translate([0, -plate_h/2 - tongue_len/2 + 0.01, tongue_z])
                cube([inner_w - 2*corner_gusset - 2*fit_slide,
                      tongue_len, tongue_t - 2*0.15], center = true);

            // --- 18650 cowl --------------------------------------------------
            // Projects from the OUTER face, so it is subtracted from Z rather
            // than added to it: the plate body occupies 0..back_t, and the cowl
            // occupies -batt_cowl_rise..0.
            if (batt_cowl_enable)
                translate([board_cx + batt_off_x, board_cy + batt_off_y, 0])
                    mirror([0, 0, 1])
                        ridge(batt_cowl_w, batt_cowl_h, batt_cowl_rise,
                              batt_cowl_cap_r);

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
                    ridge(batt_cowl_w - 2*batt_cowl_wall,
                          batt_cowl_h - 2*batt_cowl_wall,
                          batt_cowl_rise - batt_cowl_wall,
                          batt_cowl_cap_r - 0.5);

        // --- speaker grille --------------------------------------------------
        translate([board_cx + grille_off_x, board_cy + grille_off_y, -0.01])
            slot_row(grille_slot_w, grille_slot_h, grille_pitch, grille_count,
                     back_t + 0.02);

        // --- expansion-header access window ----------------------------------
        translate([board_cx, board_cy - 6.0, -0.01])
            rbox(expansion_win_h, expansion_win_w, back_t + 0.02, 2.0);

        // --- keyboard eject finger hole ---------------------------------------
        translate([kbd_eject_off_x, kbd_bay_cy + kbd_eject_off_y, -0.01])
            cylinder(h = back_t + kbd_keeper_t + 0.02, d1 = kbd_eject_d,
                     d2 = kbd_eject_d_min);

        // --- ventilation over the ESP32-S3 module -----------------------------
        if (vent_enable)
            translate([board_cx - 30, board_cy + 20, -0.01])
                slot_col(vent_slot_w, vent_slot_h, vent_pitch, vent_count,
                         back_t + 0.02);

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

module assembly() {
    color("DimGray", 0.85) chassis();
    color("SlateGray", 0.9) translate([0, 0, 0]) backplate();
    color("Crimson") for (i = [0 : button_count - 1]) {
        bx = board_cx + (i - (button_count - 1)/2) * button_pitch;
        translate([bx, body_h/2 + 0.3, z_front_inner - button_z_below_front])
            rotate([90, 0, 0]) buttons_single();
    }
    // component mock-ups, for visual fit checking only
    color("ForestGreen", 0.55)
        translate([board_cx, board_cy, z_front_inner - board_depth])
            mock_board();
    color("Black", 0.7)
        translate([0, kbd_bay_cy, z_front_inner - kbd_depth])
            mock_keyboard();
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
else                          assembly();
