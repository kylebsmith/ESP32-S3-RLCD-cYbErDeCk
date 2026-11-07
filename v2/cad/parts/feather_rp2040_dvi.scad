// feather_rp2040_dvi.scad - Adafruit 5710, Feather RP2040 with DVI Output Port.
//
// WHAT THIS IS
//   A reference body for the v2 enclosure, not a replica of the board. The
//   outline, the four mounting holes, every connector that has to reach a wall
//   or a cable, and the tallest thing above and below the PCB are modelled to
//   the measured value. Everything else - the QFN, the flash, the resistor
//   packs - is a rough block, because a pocket does not care what shape a
//   passive is. That is the asset library's own rule (cmtl-3d-assets README)
//   and it is the right one.
//
// WHERE THE NUMBERS COME FROM
//   Adafruit publish a CAD model of this board in Adafruit_CAD_Parts. The
//   local copy is
//     v2/parts/models/5710 Feather RP2040 DVI.step     (gitignored)
//     v2/parts/models/5710 Feather RP2040 DVI.stl      (gitignored)
//   and every [step] figure below was computed from the B-rep of that STEP
//   with OpenCASCADE (OCP) - exact bounding boxes, cylindrical-face axes and
//   radii, and plane sections to find which way each socket opens. It is NOT
//   read off a drawing and it is NOT the faceted STL: where the two differ,
//   this file carries the B-rep value and v2/parts/board-models.md says so.
//
//   Tags used below:
//     [step]      measured from Adafruit's STEP by B-rep query. Re-derivable.
//     [vendor]    published by Adafruit in words (product page / Learn guide).
//     [silk]      read off Adafruit's own render of the board.
//     [derived]   arithmetic on a tagged value, shown inline.
//     [spec]      the published Feather mechanical standard.
//     [UNSOURCED] not established. Listed in v2/parts/board-models.md section 5.
//
//   Full provenance, including the three places this file corrects
//   v2/parts/hdmi-feather.md: v2/parts/board-models.md.
//
// FRAME
//   This is Adafruit's own model frame, unrotated and unmirrored, so every
//   coordinate here can be checked against the STEP directly.
//     x  0 .. 50.800   along the long axis. x = 0 is the USB-C end,
//                      x = 50.800 is the HDMI end.
//     y  0 .. 22.860   along the short axis. y = 1.270 carries the 16-pin
//                      row, y = 21.654 the 12-pin row.
//     z  0 at the PCB's BOTTOM face, +z up through the components.
//                      PCB top face = 1.570.
//   Right-handed. The HDMI shell is the only thing below z = 0.
//
// RENDER
//   openscad -o /tmp/feather.stl v2/cad/parts/feather_rp2040_dvi.scad
//   openscad -D 'part="envelope"' -o /tmp/f_env.stl v2/cad/parts/feather_rp2040_dvi.scad
//   An empty or tiny mesh is a failure symptom, not a quiet pass.

// --------------------------------------------------------------- build config

// 24 is enough to see a Ø2.5 hole is round; 96 is enough to cut one.
$fn = $preview ? 24 : 96;

// "board"     every modelled body, the way the real board is populated
// "pcb"       the bare PCB only
// "envelope"  one worst-case block, for interference checks
// "silk"      the silkscreen legends alone, 0.02 proud of the PCB
// "ports"     only the things that must reach an enclosure wall
part = "board";

// The board ships with its headers LOOSE and this build solders wires to the
// pads instead (v2/parts/hdmi-feather.md section 2.3: male pin tails would add
// ~3 mm below the PCB and break the -3.00 channel floor). Set true to model
// them anyway and see what they cost.
fit_headers = false;

// Silkscreen is drawn by the "board" view too, because the whole point of the
// v2 deck is that these boards are seen through clear acrylic. It is cheap -
// a 0.02 mm extrusion - but it is text(), so turn it off while iterating.
show_silk = true;

// ----------------------------------------------------------------- parameters
// ---- PCB -------------------------------------------------------------------

pcb_l      = 50.800;  // [step] board solid, x 0.000 .. 50.800 (= 2.000 in)
pcb_w      = 22.860;  // [step] board solid, y 0.000 .. 22.860 (= 0.900 in)
pcb_t      =  1.570;  // [step] board solid, z 0.000 .. 1.570
//  2.540, not the ~1.90 that a scan of the faceted STL gave. The B-rep carries
//  four R2.540 cylindrical faces centred 2.540 in from both edges at every
//  corner - i.e. 0.100 in, which is the Feather standard's own corner. The two
//  USB-end mounting holes are CONCENTRIC with their corner arcs; the two HDMI-
//  end holes are not (see below), which is the whole trap of this footprint.
pcb_corner_r = 2.540; // [step] four corner arcs, R2.540 at (2.540|48.260, 2.540|20.320)

// ---- mounting: the trapezoid, not a rectangle ------------------------------
//  Adafruit moved the HDMI-end pair 0.635 outboard to clear the connector
//  shell, and they drilled it 0.040 larger while they were there. Four
//  numbers, four different values - do not pattern this from one corner.
//      USB end   y 2.540 / 20.320   pitch 17.780 (0.700 in)   O2.500
//      HDMI end  y 1.905 / 20.955   pitch 19.050 (0.750 in)   O2.540
//      columns   x 2.540 / 48.260   pitch 45.720 (1.800 in)
//  v2/parts/stack.md section 5.4's ruling stands: a O2.50 hole takes M2, not
//  M2.5 (M2.5 clearance is O2.7-2.9 and will not pass). The HDMI pair's O2.540
//  does not change that - it is still under M2.5 clearance.
mount_x_usb   =  2.540;  // [step] hole axis, USB end
mount_x_hdmi  = 48.260;  // [step] hole axis, HDMI end
mount_y_usb   = [ 2.540, 20.320];  // [step] both USB-end holes
mount_y_hdmi  = [ 1.905, 20.955];  // [step] both HDMI-end holes
mount_d_usb   =  2.500;  // [step] cylinder diameter, USB end
mount_d_hdmi  =  2.540;  // [step] cylinder diameter, HDMI end - 0.040 LARGER

// ---- HDMI receptacle, Molex 47151-0001 --------------------------------------
//  The tallest thing on the board, the only thing below it, and the one part
//  whose position decides the whole placement. Two envelopes are given because
//  they do different jobs: the SHELL is what a wall aperture has to pass, the
//  GROSS box is what a pocket has to clear.
hdmi_x0       = 39.524;  // [step] gross box, rear-most point
hdmi_x1       = 51.686;  // [step] gross box, front face (0.886 past the PCB)
hdmi_y0       =  3.905;  // [step] gross box
hdmi_y1       = 18.918;  // [step] gross box
hdmi_z0       = -0.636;  // [step] gross box, through-hole shield tabs
hdmi_z1       =  7.789;  // [step] gross box, top
hdmi_shell_w  = 15.000;  // [step] clean shell, y 3.912 .. 18.912
hdmi_shell_y  = 11.412;  // [step] shell centre; board centre is 11.430,
                         //   so the shell sits 0.018 toward y = 0  [derived]
hdmi_mouth_z0 =  2.205;  // [step] shell section at the mouth, x >= 49.5
hdmi_mouth_z1 =  7.755;  // [step] ... 5.550 tall, 15.000 wide
hdmi_wall_t   =  0.450;  // [step] shell wall, from the mouth section's area
                         //   (18.027 mm2 round a 15.000 x 5.550 ring) [derived]

// ---- USB-C, mid-mount -------------------------------------------------------
usbc_x0 = -1.182;  // [step] protrudes 1.182 past the PCB edge at x = 0
usbc_x1 =  6.730;  // [step]
usbc_y0 =  6.954;  // [step] y centre 11.430 = the PCB centre exactly [derived]
usbc_y1 = 15.906;  // [step]
usbc_z0 =  0.564;  // [step] mid-mount: it sits INSIDE the board, but its
usbc_z1 =  4.776;  // [step] lowest point is still 0.564 ABOVE the PCB bottom,
                   //   so nothing of it protrudes below. Only the HDMI does.

// ---- battery socket, JST PH 2-pin ------------------------------------------
//  SIDE ENTRY, facing +y - out the long edge that carries the 12-pin row.
//  v2/parts/hdmi-feather.md section 2.5 calls this top-entry, from a photo.
//  The B-rep says otherwise and says it three ways: the housing's y = 22.453
//  face is an open ring (section area 10.100 against a 15.040 mid-body), both
//  y-end walls are solid, and each contact runs 8.200 mm along y with its
//  blade at z 4.170 .. 4.920 and its solder tail bent down to the pads at
//  z 1.570 .. 2.320. A top-entry PH has neither of those. Adafruit's own
//  render agrees. See v2/parts/board-models.md section 4.1.
jst_ph_x0 =  6.972;  // [step] housing
jst_ph_x1 = 14.872;  // [step]
jst_ph_y0 = 14.853;  // [step] housing
jst_ph_y1 = 22.453;  // [step] MOUTH - 0.407 inside the PCB edge at 22.860
jst_ph_z1 =  6.370;  // [step] top; 4.800 above the PCB top face [derived]
jst_ph_tail_y0 = 13.853;  // [step] solder tails reach back to here

// ---- STEMMA QT, JST SH 4-pin -----------------------------------------------
//  TOP ENTRY. Same three tests, opposite answer: the z = 5.870 face is a ring
//  of 6.418 mm2 (a 0.4 mm wall round a 2.900 x 6.000 outline), both x-end
//  walls are solid at 21.590 and 22.984 mm2, and the four contacts lie flat on
//  the board. Adafruit's EAGLE package is called JST_SH4_RA, which is what put
//  "right angle" into the earlier reading; the geometry is not right-angled.
qt_x0 = 30.478;  // [step] housing
qt_x1 = 33.378;  // [step] housing; SMT tails run on to 34.078
qt_y0 = 13.095;  // [step]
qt_y1 = 19.095;  // [step]
qt_z0 =  1.620;  // [step]
qt_z1 =  5.870;  // [step] 4.300 above the PCB top face [derived]
qt_tail_x1 = 34.078;  // [step] solder tails
qt_pitch   =  1.000;  // [step] contacts at y 14.595, 15.595, 16.595, 17.595

// ---- the two 0.1 in header rows ---------------------------------------------
//  Pitch, count and the 16-pin row are exactly the Feather standard. The
//  12-pin row is NOT: the B-rep puts it at y = 21.654, which makes the row
//  separation 20.384 and not the published 0.800 in = 20.320. The faceted STL
//  reads 21.590 and that is what v2/parts/hdmi-feather.md section 2.7 carries.
//  0.064 mm does not matter to a pocket; it matters to anyone laying out a
//  mating board, so the measured value is the one kept here.
hdr_pitch   =  2.540;  // [step] = 0.100 in, both rows
hdr_drill   =  1.000;  // [step] plated hole diameter, both rows
hdr16_y     =  1.270;  // [step] 16-pin row (JP1), 0.050 in from the long edge
hdr16_x0    =  6.350;  // [step] pin 1 of JP1 is at the HDMI end, x = 44.450
hdr16_n     = 16;      // [step]
hdr12_y     = 21.654;  // [step] 12-pin row (JP3) - NOT 21.590, see above
hdr12_x0    = 16.510;  // [step] pin 1 of JP3 is at the USB end
hdr12_n     = 12;      // [step]
hdr_body_h  =  8.500;  // [UNSOURCED] a 0.1 in female header's insulator, if
                       //   one is ever fitted. The board ships headers loose
                       //   and none is in the CAD model. Not used by default.
hdr_tail_h  =  3.000;  // [UNSOURCED] male pin tails below the PCB, if fitted.
                       //   This is the number that breaks the -3.00 channel
                       //   floor; measure it before you solder anything in.

// The two extra plated pads on the HDMI side: CEC and UTILITY.
pad_cec_u_x =  36.830;  // [step] both at x = 36.830, O1.000
pad_cec_y   =   5.080;  // [step]
pad_util_y  =   7.620;  // [step]

// ---- the rest, deliberately rough ------------------------------------------
rp2040_x0 = 18.788; rp2040_y0 =  6.660;  // [step] QFN-56, 7.000 x 7.000
rp2040_s  =  7.000; rp2040_h  =  1.000;  // [step] z 1.570 .. 2.570
flash_x0  = 25.621; flash_y0  = 15.907;  // [step] USON8 4x4, 8 MB QSPI
flash_w   =  4.002; flash_d   =  4.000; flash_h = 0.800;  // [step]
npx_x0    = 26.250; npx_y0    =  4.330;  // [step] SK6805-1515 status NeoPixel
npx_s     =  1.500; npx_h     =  0.480;  // [step]
btn_y0    =  3.680; btn_y1    =  6.480;  // [step] both tactile buttons
btn_z1    =  3.470; btn_x     = [7.225, 13.575];  // [step] 4.600 wide each
btn_w     =  4.600;                      // [step] 11.825-7.225 = 18.175-13.575
//  Which is which. Adafruit's own render prints the legend to the lower-left
//  of each button and the button body covers the end of the word, but the
//  fragments are unambiguous: "Rese..." on the one nearer the USB-C, "Boo..."
//  on the other. Low-confidence but checkable in two seconds on a real board -
//  see board-models.md section 5.
btn_name  = ["Rst", "Boot"];             // [silk, partly occluded]
respack_x0 = 36.094; respack_w = 1.600;  // [step] R14/R15, the "220 Pack"
respack_y  = [9.079, 13.143];            // [step] two packs, 3.050 tall each
respack_d  =  3.050; respack_h = 0.500;  // [step]

// ---- derived ----------------------------------------------------------------

pcb_top     = pcb_t;                              // 1.570
tallest_up  = hdmi_z1 - pcb_top;                  // 6.219 [derived]
deepest_dn  = -hdmi_z0;                           // 0.636 [derived]
overall_l   = hdmi_x1 - usbc_x0;                  // 52.868; Adafruit say 53.0
overall_h   = hdmi_z1 - hdmi_z0;                  // 8.425; Adafruit say 7.7,
                                                  //   which ignores the tabs

// ------------------------------------------------------------------- helpers

// A rounded-corner plate. Local to this file on purpose: these part models are
// meant to be droppable into any assembly without dragging a library behind.
module _rbox(w, d, h, r) {
    if (r <= 0) cube([w, d, h]);
    else linear_extrude(height = h)
        offset(r = r) offset(r = -r) square([w, d]);
}

module _block(x0, y0, z0, x1, y1, z1, r = 0) {
    translate([x0, y0, z0]) _rbox(x1 - x0, y1 - y0, z1 - z0, r);
}

// --------------------------------------------------------------------- bodies

// The bare PCB: outline, corner arcs, four mounting holes, both header rows
// and the two breakout pads. Drilled, so a mating plate can be checked against
// it by intersection rather than by trust.
module fw_pcb(l = pcb_l, w = pcb_w, t = pcb_t, r = pcb_corner_r,
              drill = true) {
    difference() {
        _rbox(l, w, t, r);
        if (drill) {
            fw_mount_holes(through = t);
            fw_header_holes(through = t);
            for (p = [[pad_cec_u_x, pad_cec_y], [pad_cec_u_x, pad_util_y]])
                translate([p[0], p[1], -1]) cylinder(d = hdr_drill, h = t + 2);
        }
    }
}

// The four mounting holes as cut geometry. Takes its own diameters so a
// carrier can call it for a clearance pattern without editing this file.
module fw_mount_holes(through = 4.0, d_usb = mount_d_usb, d_hdmi = mount_d_hdmi) {
    for (y = mount_y_usb)
        translate([mount_x_usb, y, -1]) cylinder(d = d_usb, h = through + 2);
    for (y = mount_y_hdmi)
        translate([mount_x_hdmi, y, -1]) cylinder(d = d_hdmi, h = through + 2);
}

module fw_header_holes(through = 4.0, d = hdr_drill) {
    for (i = [0 : hdr16_n - 1])
        translate([hdr16_x0 + i * hdr_pitch, hdr16_y, -1])
            cylinder(d = d, h = through + 2);
    for (i = [0 : hdr12_n - 1])
        translate([hdr12_x0 + i * hdr_pitch, hdr12_y, -1])
            cylinder(d = d, h = through + 2);
}

// Molex 47151-0001, right-angle HDMI Type A. Drawn as the gross box with the
// mouth's bore taken out of the front, so the model shows which 15.000 x 5.550
// of it a wall aperture actually has to pass.
module fw_hdmi(x0 = hdmi_x0, x1 = hdmi_x1, y0 = hdmi_y0, y1 = hdmi_y1,
               z0 = hdmi_z0, z1 = hdmi_z1, wall = hdmi_wall_t,
               mz0 = hdmi_mouth_z0, mz1 = hdmi_mouth_z1) {
    difference() {
        _block(x0, y0, z0, x1, y1, z1, 0.3);
        // the Type A opening, cut back 6.0 mm from the front face
        translate([x1 - 6.0, y0 + wall, mz0 + wall])
            cube([6.1, (y1 - y0) - 2 * wall, (mz1 - mz0) - 2 * wall]);
    }
}

// The aperture a wall needs for the shell, as a solid, at the shell's own
// y centre. Clearance is the caller's: pass what the build allows.
module fw_hdmi_aperture(clear = 0.5, depth = 10.0) {
    translate([hdmi_x1 - depth, hdmi_shell_y - (hdmi_shell_w / 2 + clear),
               hdmi_mouth_z0 - clear])
        cube([depth + 1, hdmi_shell_w + 2 * clear,
              (hdmi_mouth_z1 - hdmi_mouth_z0) + 2 * clear]);
}

module fw_usbc() { _block(usbc_x0, usbc_y0, usbc_z0, usbc_x1, usbc_y1, usbc_z1, 0.5); }

// Side-entry, mouth at +y. The notch is cosmetic but it is the fastest way to
// see, in an assembly view, that the cable leaves across the board's long edge
// and not straight up into the acrylic.
module fw_jst_ph() {
    difference() {
        union() {
            _block(jst_ph_x0, jst_ph_y0, pcb_top, jst_ph_x1, jst_ph_y1, jst_ph_z1, 0.3);
            _block(jst_ph_x0 + 2.4, jst_ph_tail_y0, pcb_top,
                   jst_ph_x1 - 2.4, jst_ph_y0 + 0.1, pcb_top + 0.6);
        }
        translate([jst_ph_x0 + 0.8, jst_ph_y1 - 3.0, pcb_top + 0.8])
            cube([(jst_ph_x1 - jst_ph_x0) - 1.6, 3.1, (jst_ph_z1 - pcb_top) - 1.6]);
    }
}

// Top entry: the bore opens upward, which is why nothing can be plugged into
// it once 12.70 mm of acrylic is 6.35 mm above the wiring plane.
module fw_stemma_qt() {
    difference() {
        union() {
            _block(qt_x0, qt_y0, qt_z0, qt_x1, qt_y1, qt_z1, 0.3);
            _block(qt_x1, qt_y0 + 1.4, pcb_top, qt_tail_x1, qt_y1 - 1.4, pcb_top + 0.25);
        }
        translate([qt_x0 + 0.45, qt_y0 + 0.45, qt_z1 - 2.8])
            cube([(qt_x1 - qt_x0) - 0.9, (qt_y1 - qt_y0) - 0.9, 2.9]);
    }
}

module fw_buttons() {
    for (x = btn_x) _block(x, btn_y0, pcb_top, x + btn_w, btn_y1, btn_z1, 0.4);
}

// Everything that is only here so the board reads as a board through acrylic.
module fw_passives() {
    _block(rp2040_x0, rp2040_y0, pcb_top,
           rp2040_x0 + rp2040_s, rp2040_y0 + rp2040_s, pcb_top + rp2040_h, 0.3);
    _block(flash_x0, flash_y0, pcb_top,
           flash_x0 + flash_w, flash_y0 + flash_d, pcb_top + flash_h, 0.2);
    _block(npx_x0, npx_y0, pcb_top,
           npx_x0 + npx_s, npx_y0 + npx_s, pcb_top + npx_h, 0.1);
    for (y = respack_y)
        _block(respack_x0, y, pcb_top,
               respack_x0 + respack_w, y + respack_d, pcb_top + respack_h, 0.1);
}

// Optional, and off by default. See fit_headers above.
module fw_headers(body_h = hdr_body_h, tail_h = hdr_tail_h) {
    for (row = [[hdr16_x0, hdr16_y, hdr16_n], [hdr12_x0, hdr12_y, hdr12_n]]) {
        span = (row[2] - 1) * hdr_pitch + hdr_pitch;
        translate([row[0] - hdr_pitch / 2, row[1] - hdr_pitch / 2, pcb_top])
            cube([span, hdr_pitch, body_h]);
        translate([row[0] - hdr_pitch / 2, row[1] - hdr_pitch / 2, -tail_h])
            cube([span, hdr_pitch, tail_h]);
    }
}

// --------------------------------------------------------------------- silk
//  The legends, as Adafruit print them. Read off Adafruit's own render of the
//  board (5710 Feather RP2040 DVI.jpg, shipped beside the STEP) and identical
//  to the list in v2/parts/hdmi-feather.md section 2.7.
//
//  NOTE, because it is a trap: the 16-pin row has FIFTEEN labels over SIXTEEN
//  holes. Pins 14 and 15 are both +3V3 and only one of them is printed. Do not
//  count silk to find a pin; count holes.

silk_h    = 0.020;  // [derived] thin enough to be a surface, thick enough to mesh
silk_size = 1.30;   // [derived] legible at 1:1 under 12.70 mm of acrylic

// 16-pin row (JP1), pin 1 at the HDMI end, printed along the row.
silk_jp1 = ["Rst", "3.3V", "GND", "A0", "A1", "A2", "A3", "24",
            "25", "SCK", "MO", "MI", "RX", "TX", "HPD"];
// 12-pin row (JP3), pin 1 at the USB end.
silk_jp3 = ["Bat", "EN", "USB", "13", "12", "11", "10", "9", "6", "5",
            "SCL", "SDA"];

module fw_silk(h = silk_h, size = silk_size) {
    color("white") translate([0, 0, pcb_top]) linear_extrude(height = h) {
        // JP1: labels run inboard of the row, reading along the board.
        for (i = [0 : len(silk_jp1) - 1])
            translate([hdr16_x0 + i * hdr_pitch, hdr16_y + 1.9])
                rotate([0, 0, 90]) text(silk_jp1[i], size = size,
                                        halign = "left", valign = "center");
        for (i = [0 : len(silk_jp3) - 1])
            translate([hdr12_x0 + i * hdr_pitch, hdr12_y - 1.9])
                rotate([0, 0, 90]) text(silk_jp3[i], size = size,
                                        halign = "right", valign = "center");
        // the two tactile buttons
        for (i = [0 : len(btn_x) - 1])
            translate([btn_x[i] - 0.6, btn_y0]) rotate([0, 0, 90])
                text(btn_name[i], size = size, halign = "left", valign = "center");
        // the two breakout pads
        translate([pad_cec_u_x + 1.5, pad_cec_y])
            text("C", size = size, valign = "center");
        translate([pad_cec_u_x + 1.5, pad_util_y])
            text("U", size = size, valign = "center");
        // board name, three lines, as printed beside the RP2040
        translate([27.5, 16.5]) rotate([0, 0, 90])
            text("RP2040", size = size, halign = "center", valign = "center");
        translate([29.1, 16.5]) rotate([0, 0, 90])
            text("Feather", size = size, halign = "center", valign = "center");
        translate([30.7, 16.5]) rotate([0, 0, 90])
            text("DVI", size = size, halign = "center", valign = "center");
    }
}

// ------------------------------------------------------------------ assembly

// Only the bodies that have to reach an enclosure wall or a cable. This is the
// view to intersect a carrier against.
module fw_ports() {
    fw_hdmi();
    fw_usbc();
    fw_jst_ph();
    fw_stemma_qt();
}

module fw_board(headers = fit_headers, silk = show_silk) {
    color("#1b3a2a") fw_pcb();
    color("#c8c8cc") fw_hdmi();
    color("#c8c8cc") fw_usbc();
    color("#101010") fw_jst_ph();
    color("#101010") fw_stemma_qt();
    color("#d8d8d8") fw_buttons();
    color("#202020") fw_passives();
    if (headers) color("#101010") fw_headers();
    if (silk) fw_silk();
}

// One block at the worst case on every axis, for the cheap interference test.
// A mock that fits guarantees a board that fits, never the other way round.
module fw_envelope(margin = 0) {
    translate([usbc_x0 - margin, -margin, hdmi_z0 - margin])
        cube([overall_l + 2 * margin,
              pcb_w + 2 * margin,
              overall_h + 2 * margin]);
}

// ----------------------------------------------------------------- interfaces

// Mounting-hole centres, as [x, y, diameter]. A carrier should pattern its
// bosses from THIS, not from a rectangle.
function fw_mount_points() = [
    [mount_x_usb,  mount_y_usb[0],  mount_d_usb ],
    [mount_x_usb,  mount_y_usb[1],  mount_d_usb ],
    [mount_x_hdmi, mount_y_hdmi[0], mount_d_hdmi],
    [mount_x_hdmi, mount_y_hdmi[1], mount_d_hdmi]
];

// The HDMI shell, as [y_centre, width, z_lo, z_hi, x_face]. Everything a wall
// aperture needs, with no height taken from the gross box by accident.
function fw_hdmi_shell() =
    [hdmi_shell_y, hdmi_shell_w, hdmi_mouth_z0, hdmi_mouth_z1, hdmi_x1];

// ---------------------------------------------------------------------- main

if      (part == "board")    fw_board();
else if (part == "pcb")      fw_pcb();
else if (part == "envelope") fw_envelope();
else if (part == "silk")     fw_silk();
else if (part == "ports")    fw_ports();
else assert(false, "part must be board, pcb, envelope, silk or ports");

echo(str("5710  pcb=", pcb_l, " x ", pcb_w, " x ", pcb_t,
         "  overall=", overall_l, " x ", pcb_w, " x ", overall_h,
         "  above_pcb=", tallest_up, "  below_pcb=", deepest_dn,
         "  hdmi_face_x=", hdmi_x1, " (", hdmi_x1 - pcb_l, " past the edge)",
         "  mounts=", fw_mount_points()));
