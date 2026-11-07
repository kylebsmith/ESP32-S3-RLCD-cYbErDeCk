// waveshare_rlcd42.scad - Waveshare ESP32-S3-RLCD-4.2, the deck's main board.
//
// WHAT THIS IS
//   A reference body for the v2 enclosure. Outline, the four mounting holes
//   and their standoffs, every port that has to reach a wall, the 2 x 8
//   expansion header, and the display - including WHERE THE ACTIVE AREA SITS,
//   because in v2 the acrylic runs over the display with no cutout and the two
//   have to line up. Everything else is a rough block on purpose.
//
// WHERE THE NUMBERS COME FROM
//   Waveshare publish a dimensioned package for this board. The local copy is
//     v2/parts/models/waveshare-rlcd42/ESP32-S3-RLCD-4.2-20261010.stp   (24.7 MB)
//     v2/parts/models/waveshare-rlcd42/ESP32-S3-RLCD-4.2-20261010.dxf
//     v2/parts/models/waveshare-rlcd42/ESP32-S3-RLCD-4.2_20261010.pdf
//   all gitignored. Every [step] figure was computed from the B-rep of that
//   assembly with OpenCASCADE (OCP): accumulated component transforms, exact
//   bounding boxes, planar-face areas and outlines, cylindrical-face axes.
//   Every [dxf] figure was read from a DIMENSION entity in the drawing file -
//   the measurement and both definition points - not off a rendered picture.
//
//   Tags used below:
//     [step]      measured from Waveshare's STEP by B-rep query. Re-derivable.
//     [dxf]       a DIMENSION entity in Waveshare's drawing file.
//     [vendor]    published by Waveshare in words.
//     [repo]      taken from this repository (wiring.md, cad/parameters.scad).
//     [derived]   arithmetic on a tagged value, shown inline.
//     [UNSOURCED] not established. Listed in v2/parts/board-models.md section 5.
//
//   Full provenance, including what this file corrects in
//   v2/parts/knobs.md section 1.1: v2/parts/board-models.md.
//
// FRAME
//   This is v1 cad/parameters.scad's U / V / W frame, kept so that v1 and v2
//   stay checkable against each other. It is RIGHT-HANDED.
//     x = U  0 .. 92.500   U = 92.500 is the USB-C / microSD / display-FPC edge
//     y = V  0 .. 69.100   V = 0 is the 18650 edge, V = 69.100 the button edge
//     z = W  0 at the PCB's REAR (component) face, +W toward the DISPLAY
//            PCB front face = +1.600, display front face = +4.350
//   Waveshare's own STEP runs its long axis the other way; the conversion is
//     U = 92.500 - x_step,   V = y_step,   W = 1.511 - z_step
//   and it is a double flip, so handedness is preserved. Taking the STEP frame
//   with +z toward the display instead would give a LEFT-handed frame and put
//   the USB-C on the wrong side of every back-view description in wiring.md.
//
// RENDER
//   openscad -o /tmp/rlcd.stl v2/cad/parts/waveshare_rlcd42.scad
//   openscad -D 'part="envelope"' -o /tmp/r_env.stl v2/cad/parts/waveshare_rlcd42.scad
//   An empty or tiny mesh is a failure symptom, not a quiet pass.

// --------------------------------------------------------------- build config

$fn = $preview ? 24 : 96;

// "board"     the board as the deck uses it: no stand base, no kickstand
// "pcb"       the bare PCB only
// "shipped"   board + stand base, i.e. what 14.100 mm of vendor thickness is
// "envelope"  one worst-case block, for interference checks
// "display"   the display stack alone - spacer, glass, active area
// "silk"      the P1 legends alone
// "ports"     only the things that must reach an enclosure wall
part = "board";

// The moulded stand base and its fold-out kickstand are removable and v1
// already discards them (cad/parameters.scad section 1). They are modelled so
// "shipped" can be rendered and so the 14.100 mm chain can be seen to close.
show_base = false;

// The 18650 holder. It is on the board, it is 15.200 mm deep, and it is the
// single deepest feature - so it is ON by default. Nothing good comes of
// forgetting it.
show_battery = true;

show_silk = true;

// ----------------------------------------------------------------- parameters
// ---- PCB -------------------------------------------------------------------

pcb_u = 92.500;  // [dxf] "92.5 PCB OD"; [step] board solid 92.530 incl. mask
pcb_v = 69.100;  // [dxf] "69.1 PCB OD"; [step] board solid 69.131 incl. mask
pcb_t =  1.600;  // [step] board core, exactly 1.600 between the two faces.
                 //   Waveshare do not publish it; this is measured, not the
                 //   1.57 that v2/parts/knobs.md section 1.1 assumed.
//  0 IS MEASURED, not conservative. The drawing says R0.5; the B-rep outline
//  carries NO corner arcs at all - the only cylindrical faces in the board
//  solid are four O4.200, one O2.400 and the vias - and v1 cad/parameters.scad
//  records a real board measured SQUARE at all four corners with a
//  straightedge (docs/DATUMS.md C-34). Two independent sources against the
//  drawing. A square-cornered PCB is also the larger body, so a pocket cut to
//  it fits either part. See board-models.md section 2.2.
pcb_corner_r = 0.000;  // [step][repo] square, all four corners

// ---- mounting --------------------------------------------------------------
//  Four O4.200 through-holes, each with a surface-mount SMTSO-M2.5-7ET
//  standoff on the REAR face: O5.500 body, female M2.5, standing 7.030 off
//  that face. The enclosure drives M2.5 UP into them; it supplies no bosses.
mount_pitch_u = 85.500;  // [dxf] "85.50"; [step] 89.000 - 3.500
mount_pitch_v = 62.100;  // [dxf] "62.10"; [step] 65.600 - 3.500
mount_inset   =  3.500;  // [dxf] "3.50"; [step] same from all four PCB edges
mount_hole_d  =  4.200;  // [step] four cylindrical faces, full board thickness
standoff_d    =  5.500;  // [step] SMTSO body OD
standoff_h    =  7.030;  // [step] rear face (W 0) to the standoff's end face.
                         //   The part is a nominal 7.00; this is the modelled
                         //   assembly, and it is the SAME 7.00 that appears as
                         //   the middle term of the drawing's thickness chain.

// One more hole, which nothing in this repository had recorded:
aux_hole_u = 10.410;  // [step] a single O2.400 through-hole. Purpose unknown
aux_hole_v =  7.050;  // [step]   - see board-models.md section 5.
aux_hole_d =  2.400;  // [step]

// ---- the display stack, and the chain that closes it -----------------------
//  Waveshare's drawing carries three numbers in a chain down the thickness:
//  4.35, 7.00, 2.75, totalling 14.10 [dxf, four DIMENSION entities]. Measured
//  against the solids, every term lands:
//
//     W +4.350  display front face           [step]  <- the 4.35 ends here
//     W +3.600  acrylic spacer front face    [step]
//     W +1.600  PCB front face = spacer back [step]
//     W  0.000  PCB REAR face                        <- 4.35 measured to here
//     W -7.000  stand-base front face        [step]  <- 7.00, and it is also
//                                                       exactly the standoff
//     W -9.750  stand-base rear face         [step]  <- 2.75, the base
//     ---------------------------------------------
//     4.350 - (-9.750) = 14.100                      [derived] = the drawing
//
//  So the display assembly stands 4.350 - 1.600 = 2.750 mm above the PCB's
//  FRONT face, and it is built as 2.000 of acrylic spacer plus 0.750 of glass.
//  That also explains Waveshare's own V1 -> V2 note: 3.75 -> 4.35 is 0.600,
//  which is the spacer going from 1.400 to 2.000 and nothing else moving.
//
//  v2/parts/knobs.md section 1.1 assigns the chain differently and concludes
//  "the display is 5.53 mm tall above its own PCB". It is 2.750. The error is
//  in the deck's favour - more clearance under the acrylic, not less - but it
//  is 2.78 mm and it should not stay in the repository. board-models.md s.3.3.

spacer_t      = 2.000;  // [step] acrylic spacer, W 1.600 .. 3.600
glass_t       = 0.750;  // [step] LCD glass layer, W 3.600 .. 4.350
display_front = 4.350;  // [step][dxf] front face of the whole display assembly
display_above_pcb = 2.750;  // [derived] display_front - pcb_t

// Spacer outline (it covers nearly the whole board face).
spacer_u0 =  0.048; spacer_u1 = 92.350;  // [step]
spacer_v0 =  0.050; spacer_v1 = 69.053;  // [step]

// TFT module glass outline. Centred on the PCB to within 0.003 on both axes.
tft_u0 =  0.748; tft_u1 = 91.750;  // [step]; [dxf] "91.00+/-0.10 TFT", margins 0.75
tft_v0 =  0.750; tft_v1 = 68.353;  // [step]; [dxf] "67.60+/-0.10 TFT", margins 0.75

// LCD ACTIVE AREA. This is the one that has to line up with the acrylic.
//  Measured as a single planar face of 5393.280 mm2 in the STEP - 84.802 x
//  63.603 - at W = +4.325, i.e. 0.025 below the glass's outer border.
//  It is NOT centred on the PCB along U: the margins are 2.248 at U = 0 and
//  5.450 at the USB-C edge, so the AA centre sits 1.601 toward U = 0 of the
//  PCB centre. Getting this wrong puts the slab 1.6 mm off the panel.
aa_u0 =  2.248; aa_u1 = 87.050;  // [step]; [dxf] "84.80+/-0.10 LCD AA"
aa_v0 =  2.750; aa_v1 = 66.353;  // [step]; [dxf] "63.60+/-0.10 LCD AA"
aa_w  =  4.325;  // [step] the active-area plane

// The FPC leaves the module at the USB-C edge and overhangs the PCB there.
fpc_overhang = 0.080;  // [step] module reaches U 92.580 against a 92.500 PCB
fpc_len      = 0.831;  // [step] tab length past the glass outline
//  The tab is 12.676 wide [step], but the measurement does not separate the
//  two possible senses of V across the module's own symmetry - the band is
//  either V 23.99..36.66 or V 32.44..45.11. It is a 0.2 mm film that reaches
//  0.080 past the PCB and touches nothing, so it is drawn across BOTH
//  candidates, which is this project's usual rule: the conservative envelope.
fpc_v0 = 23.990;  // [step, conservative union of the two candidate bands]
fpc_v1 = 45.110;  // [step, conservative union of the two candidate bands]

// ---- 2 x 8 expansion header, P1 ---------------------------------------------
//  The number this file was written to close. v2/parts/wiring.md carries the
//  full pin map but not a position; now there is one, measured three ways -
//  the moulded housing's bounding box, its nine internal partition walls, and
//  the sixteen 1.600 x 1.600 holes in its top face.
//
//  It is a SOCKET on the board's REAR face, so it sets the standoff height the
//  carrier has to give this board: 8.702 mm of header below the PCB's rear
//  face, before a single mating pin.
hdr_u0  = 35.649;  // [step] moulded housing
hdr_u1  = 56.649;  // [step] ... 21.000 long
hdr_v0  = 31.351;  // [step] housing
hdr_v1  = 36.352;  // [step] ... 5.001 wide
hdr_lead_v0 = 30.550;  // [step] SMD gull-wing leads, 6.603 over the pair
hdr_lead_v1 = 37.153;  // [step]
hdr_w_body  = -0.600;  // [step] housing's near face, below the PCB rear face
hdr_w_far   = -8.700;  // [step] housing's far face. v1 cad/parameters.scad
                       //   carries exactly 8.700 for this from the other
                       //   Waveshare package, by a different route.
hdr_w_env   = -8.702;  // [step] gross envelope incl. the leads' plating
hdr_w_lead  = -0.099;  // [step] nearest lead metal to the PCB rear face
hdr_pitch   =  2.540;  // [step] both axes
hdr_hole    =  1.600;  // [step] square, measured from the top-face wires
// Pin 1 is at the USB-C end, on the LOWER row in a back view with the 18650 at
// the bottom - i.e. the lower V. [repo wiring.md 1.2, schematic + silkscreen]
hdr_pin1_u  = 55.039;  // [step] column 1 of 8; columns run DOWN in U
hdr_row_odd_v  = 32.582;  // [step] pins 1,3,5,...,15
hdr_row_even_v = 35.122;  // [step] pins 2,4,6,...,16
hdr_cols = 8;

// ---- ports on the U = 92.500 edge -------------------------------------------
usbc_u0 = 84.988; usbc_u1 = 92.521;  // [step] shell face 0.021 PROUD of the PCB
usbc_v0 = 29.754; usbc_v1 = 39.336;  // [step] 9.582 wide, V centre 34.545
usbc_w0 = -3.251; usbc_w1 =  0.911;  // [step] mid-mount, 4.162 tall

sd_u0 = 76.461; sd_u1 = 91.467;  // [step] mouth stops 1.033 INSIDE the PCB edge
sd_v0 = 45.649; sd_v1 = 61.751;  // [step] 16.102 wide, V centre 53.700
sd_w0 = -1.852; sd_w1 =  0.601;  // [step] mid-mount, 2.453 tall

// ---- things on the V = 69.100 edge ------------------------------------------
//  Three side-actuated tact switches (SWITCH-TS24CA). Each body stops 0.185
//  INSIDE the PCB edge, so an enclosure has to supply a plunger - an open hole
//  cannot reach the actuator. v1 learned this the hard way; see v1
//  cad/parameters.scad "THE BUTTON STACK".
btn_u      = [56.250, 46.250, 36.250];  // [step] BOOT, PWR, KEY in that order
btn_name   = ["BOOT", "PWR", "KEY"];    // [repo wiring.md 1.1]
btn_body_u =  4.553;  // [step] along U
btn_body_v =  3.658;  // [step] into the board from the edge
btn_v1     = 68.915;  // [step] body's outer face: 0.185 inside the PCB edge
btn_w0     = -1.802; btn_w1 = 0.401;  // [step] 2.203 tall, centred W -0.700

mic_u      = [78.750, 13.750];  // [step] two MIC-4X3X1MM
mic_body_u =  4.000; mic_body_v = 3.000;  // [step]
mic_v1     = 68.600;  // [step] 0.500 inside the PCB edge
mic_w0     = -1.000;  // [step] 1.000 tall, flush with the PCB rear face

// ---- the deep things on the rear face ---------------------------------------
batt_u0 =  2.748; batt_u1 = 89.748;  // [step] 18650 holder, 87.000 along U.
batt_v0 =  4.600; batt_v1 = 25.703;  // [step] 21.103 along V, V centre 15.152
batt_w  = -15.200;  // [step] THE DEEPEST FEATURE ON THE BOARD, and it is
                    //   5.450 deeper than the stand base it is supposed to
                    //   live behind. v1 cad/parameters.scad agrees to 0.000.
//  v1 carries batt_bay_w = 77.80 as the holder body along U, caliper-confirmed,
//  against 87.000 here. The two are probably measuring different things - body
//  versus footprint including the end contacts. 87.000 is the conservative
//  value in every role it plays, so it is what this model draws.
batt_u_v1_measured = 77.800;  // [repo] kept for the audit; nothing reads it

spk_u0 = 26.249; spk_u1 = 66.250;  // [step] SPK4020-H3_5MM
spk_v0 = 30.552; spk_v1 = 60.552;  // [step]
spk_w0 =  -7.000; spk_w1 = -3.500;  // [step] 3.500 tall, sits on the base

wroom_u0 =  7.345; wroom_u1 = 24.945;  // [step] WROOM-1 shielding can
wroom_v0 = 39.150; wroom_v1 = 54.950;  // [step]
wroom_w0 = -3.100; wroom_w1 = -0.800;  // [step] 2.300 tall

// ---- the stand base, discarded by this build --------------------------------
base_u0 = -0.002; base_u1 = 92.500;  // [step]
base_v0 = -1.000; base_v1 = 69.103;  // [step] 70.103 wide: it overhangs the
                                     //   PCB by 1.000 on the 18650 edge only.
                                     //   [dxf] "70.10" is THE BASE, not the PCB.
base_w0 = -9.750; base_w1 = -7.000;  // [step] 2.750 thick

// ---- derived ----------------------------------------------------------------

pcb_front   = pcb_t;                        // +1.600
deepest     = show_battery ? batt_w : hdr_w_far;   // -15.200 or -8.702
stack_naked = display_front + standoff_h;   // 11.380, display front face down
                                            //   to the standoffs' end face
stack_total = display_front - base_w0;      // 14.100 [derived] = the drawing
stack_batt  = display_front - batt_w;       // 19.550 [derived] with the holder

// ------------------------------------------------------------------- helpers

module _rbox(w, d, h, r) {
    if (r <= 0) cube([w, d, h]);
    else linear_extrude(height = h) offset(r = r) offset(r = -r) square([w, d]);
}
module _block(u0, v0, w0, u1, v1, w1, r = 0) {
    translate([u0, v0, w0]) _rbox(u1 - u0, v1 - v0, w1 - w0, r);
}

// --------------------------------------------------------------------- bodies

module ws_pcb(u = pcb_u, v = pcb_v, t = pcb_t, r = pcb_corner_r, drill = true) {
    difference() {
        _rbox(u, v, t, r);
        if (drill) {
            ws_mount_holes(through = t);
            translate([aux_hole_u, aux_hole_v, -1])
                cylinder(d = aux_hole_d, h = t + 2);
        }
    }
}

module ws_mount_holes(through = 4.0, d = mount_hole_d) {
    for (p = ws_mount_points())
        translate([p[0], p[1], -1]) cylinder(d = d, h = through + 2);
}

// SMTSO-M2.5-7ET x 4, on the REAR face. Drawn with their M2.5 bore open, so a
// back panel's screw can be checked straight through them.
module ws_standoffs(d = standoff_d, h = standoff_h, bore = 2.5) {
    for (p = ws_mount_points())
        translate([p[0], p[1], -h]) difference() {
            cylinder(d = d, h = h);
            translate([0, 0, -1]) cylinder(d = bore, h = h + 2);
        }
}

// Acrylic spacer + glass + the active area as a visible inlay. The AA is drawn
// 0.025 proud-recessed exactly as measured, so an assembly view shows the
// border the slab will look through.
module ws_display(show_aa = true) {
    color("#d0dce4", 0.45) _block(spacer_u0, spacer_v0, pcb_front,
                                  spacer_u1, spacer_v1, pcb_front + spacer_t);
    color("#20242a") _block(tft_u0, tft_v0, pcb_front + spacer_t,
                            tft_u1, tft_v1, display_front);
    if (show_aa)
        color("#8fa49c") _block(aa_u0, aa_v0, aa_w, aa_u1, aa_v1,
                                display_front + 0.001);
    // the module's FPC, where it overhangs the USB-C edge
    color("#a07828") _block(tft_u1, fpc_v0, pcb_front + spacer_t,
                            pcb_u + fpc_overhang, fpc_v1,
                            pcb_front + spacer_t + 0.2);
}

// The 2 x 8 socket: housing, SMD leads, and the sixteen bores so a mating
// header can be checked against it.
module ws_header_2x8(bores = true) {
    difference() {
        union() {
            _block(hdr_u0, hdr_v0, hdr_w_far, hdr_u1, hdr_v1, hdr_w_body, 0.3);
            _block(hdr_u0, hdr_lead_v0, hdr_w_env,
                   hdr_u1, hdr_lead_v1, hdr_w_lead);
        }
        if (bores)
            for (n = [1 : 2 * hdr_cols]) {
                p = ws_p1_pin(n);
                translate([p[0], p[1], hdr_w_far - 0.1])
                    cube([hdr_hole, hdr_hole, 4.0], center = false);
            }
    }
}

// What a mating MALE header costs below the board. Not fitted by default: this
// is the number a carrier bay has to clear, and it is NOT in any vendor model.
module ws_header_mate(stack = 8.500, card_t = 1.600) {
    translate([hdr_u0, hdr_v0, hdr_w_far - stack])
        cube([hdr_u1 - hdr_u0, hdr_v1 - hdr_v0, stack]);
    translate([hdr_u0 - 4, hdr_v0 - 4, hdr_w_far - stack - card_t])
        cube([(hdr_u1 - hdr_u0) + 8, (hdr_v1 - hdr_v0) + 8, card_t]);
}

module ws_usbc()    { _block(usbc_u0, usbc_v0, usbc_w0, usbc_u1, usbc_v1, usbc_w1, 0.4); }
module ws_microsd() { _block(sd_u0,   sd_v0,   sd_w0,   sd_u1,   sd_v1,   sd_w1,   0.3); }

module ws_buttons() {
    for (u = btn_u)
        _block(u - btn_body_u / 2, btn_v1 - btn_body_v, btn_w0,
               u + btn_body_u / 2, btn_v1,              btn_w1, 0.3);
}

module ws_mics() {
    for (u = mic_u)
        _block(u - mic_body_u / 2, mic_v1 - mic_body_v, mic_w0,
               u + mic_body_u / 2, mic_v1,              0.0);
}

module ws_battery_holder() {
    _block(batt_u0, batt_v0, batt_w, batt_u1, batt_v1, 0.0, 1.0);
}

module ws_speaker() { _block(spk_u0, spk_v0, spk_w0, spk_u1, spk_v1, spk_w1, 1.0); }
module ws_wroom()   { _block(wroom_u0, wroom_v0, wroom_w0, wroom_u1, wroom_v1, wroom_w1, 0.3); }
module ws_base()    { _block(base_u0, base_v0, base_w0, base_u1, base_v1, base_w1, 2.0); }

// --------------------------------------------------------------------- silk
//  The P1 legends, exactly as wiring.md section 1.2 reads them off the vendor
//  photograph and the schematic. They are on the REAR face, so in the v2 deck -
//  display up - they face the carrier and are NOT what the acrylic shows. They
//  are modelled because the person routing thirteen wires into this socket is
//  the one who needs them, and a render of the carrier's underside is where
//  that is checked.
silk_h    = 0.020;
silk_size = 1.10;

silk_odd  = ["3V3", "GND", "GP0",  "GP1",  "GP2",  "GP3",  "GP17", "GP18"];
silk_even = ["VBUS","GND", "GP19", "GP20", "TXD",  "RXD",  "SDA",  "SCL" ];

module ws_silk(h = silk_h, size = silk_size) {
    color("white") translate([0, 0, -h]) mirror([1, 0, 0])
        translate([-pcb_u, 0, 0]) linear_extrude(height = h) {
            for (c = [0 : hdr_cols - 1]) {
                u = hdr_pin1_u - c * hdr_pitch;
                translate([u, hdr_row_odd_v - 2.6])
                    rotate([0, 0, 90]) text(silk_odd[c], size = size,
                                            halign = "right", valign = "center");
                translate([u, hdr_row_even_v + 2.6])
                    rotate([0, 0, 90]) text(silk_even[c], size = size,
                                            halign = "left", valign = "center");
            }
            translate([hdr_pin1_u + 2.2, hdr_row_odd_v])
                text("1", size = size, halign = "center", valign = "center");
            for (i = [0 : len(btn_u) - 1])
                translate([btn_u[i], btn_v1 - btn_body_v - 1.6])
                    text(btn_name[i], size = size, halign = "center", valign = "top");
            translate([pcb_u / 2, 28.0])
                text("ESP32-S3-RLCD-4.2", size = size * 1.4,
                     halign = "center", valign = "center");
        }
}

// ------------------------------------------------------------------ assembly

module ws_ports() { ws_usbc(); ws_microsd(); ws_header_2x8(); ws_buttons(); }

module ws_board(base = show_base, battery = show_battery, silk = show_silk) {
    color("#0d2d44") ws_pcb();
    ws_display();
    color("#b8b8bc") ws_standoffs();
    color("#101010") ws_header_2x8();
    color("#c8c8cc") ws_usbc();
    color("#c8c8cc") ws_microsd();
    color("#d8d8d8") ws_buttons();
    color("#202020") ws_mics();
    color("#303030") ws_wroom();
    color("#1a1a1a") ws_speaker();
    if (battery) color("#2a2a2a") ws_battery_holder();
    if (base)    color("#303338") ws_base();
    if (silk)    ws_silk();
}

// One block at the worst case on every axis. With the battery holder shown it
// is 19.550 deep, which is the honest number for a bay; without it, 11.380.
module ws_envelope(margin = 0, battery = show_battery, base = show_base) {
    lo = base ? min(base_w0, battery ? batt_w : 0) : (battery ? batt_w : -standoff_h);
    translate([-margin, -margin, lo - margin])
        cube([pcb_u + 2 * margin, pcb_v + 2 * margin,
              (display_front - lo) + 2 * margin]);
}

// ----------------------------------------------------------------- interfaces

// Mounting-hole centres, as [u, v]. Same four whichever way U runs, because
// the pattern is symmetric - unlike the Feather's.
function ws_mount_points() = [
    [mount_inset,          mount_inset         ],
    [mount_inset,          pcb_v - mount_inset ],
    [pcb_u - mount_inset,  mount_inset         ],
    [pcb_u - mount_inset,  pcb_v - mount_inset ]
];

// P1 pin n (1..16) as [u, v] of the hole's lower-left corner, matching
// wiring.md's numbering: odd pins on the lower row, pin 1 at the USB-C end.
function ws_p1_pin(n) = [
    hdr_pin1_u - floor((n - 1) / 2) * hdr_pitch - hdr_hole / 2,
    ((n % 2) == 1 ? hdr_row_odd_v : hdr_row_even_v) - hdr_hole / 2
];

// P1 pin n as a centre, which is what a wire route wants.
function ws_p1_centre(n) = [
    hdr_pin1_u - floor((n - 1) / 2) * hdr_pitch,
    (n % 2) == 1 ? hdr_row_odd_v : hdr_row_even_v
];

// The active area as [u0, v0, u1, v1, w]. The slab lines up with THIS, not
// with the PCB centre and not with the TFT module.
function ws_active_area() = [aa_u0, aa_v0, aa_u1, aa_v1, aa_w];

// ---------------------------------------------------------------------- main

if      (part == "board")    ws_board();
else if (part == "pcb")      ws_pcb();
else if (part == "shipped")  ws_board(base = true, battery = true);
else if (part == "envelope") ws_envelope();
else if (part == "display")  ws_display();
else if (part == "silk")     ws_silk();
else if (part == "ports")    ws_ports();
else assert(false, "part must be board, pcb, shipped, envelope, display, silk or ports");

echo(str("RLCD-4.2  pcb=", pcb_u, " x ", pcb_v, " x ", pcb_t,
         "  display_front_W=", display_front,
         " (", display_above_pcb, " above the PCB front face)",
         "  AA=", aa_u1 - aa_u0, " x ", aa_v1 - aa_v0,
         " at u ", aa_u0, "..", aa_u1, ", v ", aa_v0, "..", aa_v1,
         "  AA_centre_offset_u=", (aa_u0 + aa_u1) / 2 - pcb_u / 2,
         "  P1_far_face_W=", hdr_w_far,
         "  P1_pin1=", ws_p1_centre(1), " pin16=", ws_p1_centre(16),
         "  stack: naked=", stack_naked, " shipped=", stack_total,
         " with_cell=", stack_batt));
