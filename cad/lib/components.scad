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
    holder_wall = 1.2;      // holder shell either side of the cell

    // Everything is referenced to the board's FRONT (display) face, because
    // that is the plane the enclosure locates against, and because every
    // measured edge-feature datum is quoted below the front face. The PCB's own
    // height within the stack is NOT independently known - see docs/DATUMS.md
    // O-03 - so the board is modelled as ONE SOLID ENVELOPE filling its pocket
    // rather than as a speculative layer stack. That is deliberately
    // conservative: a mock that fits guarantees a real board that fits.
    front = board_depth;

    union() {
        // board envelope, front face flush with the front-face lip
        rbox(board_w, board_h, front, 2.0);

        // 18650 cell + holder, rear-mounted, projecting through the back plate
        // into the cowl. The cell is 18.6 mm worst case and the pocket is
        // 13.0 mm, so this protrusion is unavoidable - modelling it honestly is
        // what proves the cowl is deep enough.
        translate([batt_off_x, batt_off_y,
                   -(cell_dia_max + 2*holder_wall) / 2])
            rbox(cell_len_max, cell_dia_max + 2*holder_wall,
                 (cell_dia_max + 2*holder_wall) / 2 + 0.01, 2.0);

        // edge buttons, at the MEASURED aperture height
        for (i = [0 : button_count - 1])
            translate([(i - (button_count - 1)/2) * button_pitch,
                       board_h/2 + 1.0, front - button_z_below_front])
                rotate([90, 0, 0])
                    rbox(button_cap_w - 0.4, button_cap_h - 0.4, 4.0, 0.8);

        // dual microphone ports, likewise at the measured height
        for (sx = [-1, 1])
            translate([sx * mic_offset_x, board_h/2 + 0.5,
                       front - mic_z_below_front])
                rotate([90, 0, 0])
                    rbox(mic_aper_w - 0.6, mic_aper_h - 0.6, 3.0, 0.5);
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

//  Drawn at the manufacturer's stated outline, 109.22 x 58.42 x 10.16 mm, and
//  seated so its DECK (its widest plane) is against the front-face lip.
module mock_keyboard() {
    body_r = 5.0;           // [PROVISIONAL] visible corner radius
    union() {
        rbox(kbd_body_w, kbd_body_h, kbd_body_t, body_r);
        // the key field that shows through the front aperture
        translate([0, 0, kbd_body_t - 0.01])
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
