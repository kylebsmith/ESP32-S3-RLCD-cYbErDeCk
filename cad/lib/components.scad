// ===========================================================================
//  cYbErDeCk - lib/components.scad
//
//  Mock-ups of the two target components, built strictly from the datums in
//  parameters.scad. They exist for two reasons:
//
//    1. visual fit-checking in the "assembly" view;
//    2. automated interference checking - tools/validate.py renders the mock
//       and the enclosure and asserts that the intersection is empty.
//
//  These are ENVELOPES, not replicas. They are deliberately drawn at the
//  worst-case size implied by the datum set, so a mock that fits guarantees a
//  real part that fits, and never the other way round.
// ===========================================================================

include <../parameters.scad>
use <util.scad>

// ---------------------------------------------------------------------------
// Waveshare ESP32-S3-RLCD-4.2
// Origin: centre of the mounting-hole pattern, on the board's REAR face.
// ---------------------------------------------------------------------------

//  Z DATUM: z = 0 is the REAR-MOST plane of the board assembly excluding the
//  battery, i.e. the plane that seats on the pocket floor. The 18650 and its
//  holder deliberately extend to NEGATIVE z, because the cell is 18.4 mm in
//  diameter and physically cannot be contained inside the deck - it projects
//  through the back plate and into the cowl. Modelling that honestly is the
//  whole point of the mock: it is what proves the cowl is deep enough.

module mock_board() {
    //  Z DATUM: z = 0 is the STANDOFF SEATING PLANE (Waveshare W = -7.00), the
    //  plane the back plate's inner face bears on. +z runs toward the display.
    //  Every height below is Waveshare's own W coordinate shifted by +7.00.
    //
    //  The board is drawn as ONE SOLID ENVELOPE rather than as a layer stack.
    //  That is deliberately conservative: a mock that fits guarantees a real
    //  board that fits, never the other way round.
    front    = board_stack;                              // display glass front
    pcb_back = board_w_pcb_back - board_w_standoff_end;   // = 7.00
    batt_end = board_w_battery_end - board_w_standoff_end;  // = -8.20

    union() {
        // board envelope: standoff ends up to the display glass front
        rbox(board_w, board_h, front, board_corner_r);

        // 18650 holder. It reaches batt_protrusion BELOW z = 0, i.e. straight
        // through the back plate and into the cowl. That is not a modelling
        // error - the cell is 18.6 mm across and the deck is 16.6 mm thick, so
        // it cannot be contained. Drawing it honestly is what proves the cowl.
        translate([batt_off_x, batt_off_y, batt_end])
            rbox(batt_bay_w, batt_bay_h, pcb_back - batt_end, 2.0);

        // 2 x 8 expansion header, on the PCB back face
        translate([expansion_win_x, expansion_win_y, pcb_back - expansion_body_h])
            rbox(expansion_cols * expansion_pitch, expansion_rows * expansion_pitch + 1.5,
                 expansion_body_h, 0.5);

        // three side-actuated tact switches, on the board's top edge
        for (i = [0 : button_count - 1])
            translate([(i - (button_count - 1)/2) * button_pitch,
                       board_h/2 - 1.0, pcb_back + button_w_centre])
                rotate([90, 0, 0])
                    rbox(button_body_w, button_body_h, 3.0, 0.4);

        // dual microphone ports
        for (sx = [-1, 1])
            translate([sx * mic_offset_x, board_h/2 - 0.5, pcb_back + mic_w_centre])
                rotate([90, 0, 0])
                    rbox(4.0, 1.0, 2.0, 0.3);

        // USB-C and microSD, on the board's right edge. Both are seated where
        // Waveshare puts them: the USB-C shell face is flush with the PCB edge,
        // and the microSD socket mouth stops 1.03 mm INSIDE it - so neither
        // body protrudes, and the microSD opening only has to pass the card.
        translate([board_w/2 - 8.0, usbc_off_y, pcb_back + usbc_w_centre])
            rotate([0, 90, 0]) rbox(usbc_body_h, usbc_body_w, 8.0, 0.5);
        translate([board_w/2 - 1.03 - 15.0, tf_off_y, pcb_back + tf_w_centre])
            rotate([0, 90, 0]) rbox(tf_body_h, tf_body_w, 15.0, 0.5);
    }
}

// Mounting-hole positions, as a list, for validation.
function board_mount_points() =
    [ for (sx = [-1, 1], sy = [-1, 1])
        [sx * board_mount_pitch_x/2, sy * board_mount_pitch_y/2] ];


// ---------------------------------------------------------------------------
// Rii 518BT mini Bluetooth keyboard
// Origin: centre of the keyboard, on its REAR face.
// ---------------------------------------------------------------------------

//  Drawn at the body's UPPER TOLERANCE on every axis - 108.80 x 58.50 x 10.60
//  against a nominal 108.5 x 58.2 x 10.2 - so a mock that fits guarantees a
//  body that fits. Seated so its DECK (its widest plane) is against the
//  front-face lip.
module mock_keyboard() {
    body_r = 5.0;           // [PROVISIONAL] visible corner radius
    union() {
        rbox(kbd_body_w_max, kbd_body_h_max, kbd_body_t_max, body_r);
        // the key field that shows through the front aperture
        translate([0, 0, kbd_body_t_max - 0.01])
            rbox(kbd_aper_w - 3.0, kbd_aper_h - 3.0, 0.8, body_r - 1);
    }
}


// ---------------------------------------------------------------------------
// SolarLink-class expansion card envelope, for clearance checking.
// ---------------------------------------------------------------------------

module mock_expansion_card() {
    card_t = 1.6;
    stack_h = 8.5;          // mated 2 x 8 header height
    translate([0, 0, -stack_h - card_t])
        rbox(expansion_card_w, expansion_card_h, card_t, 2.0);
}
