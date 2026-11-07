// k9pro_internals.scad - the STRIPPED Keychron K9 Pro as a reference body.
//
// WHAT THIS IS
//   The assembly that survives taking the ABS bottom case off: the aluminium
//   top frame (which IS the switch plate), the PCB, the 61 switch stations with
//   their hot-swap sockets, the keycap envelope, the two toggles, the USB-C and
//   the cell. It is a body to cut a carrier plate to, not a model of a
//   keyboard. Switches are envelopes, not mechanisms - section 6 of
//   v2/parts/keyboard-strip.md says why that is the right level.
//
//   Its companion is cad/keyboard_blank.scad, which is the WHOLE keyboard as a
//   single wedge. Use that one if the keyboard is mounted intact. Use this one
//   now, because it is not.
//
// FRAME - read this before using any number
//   x = 0 at the keyboard's LEFT outer edge      x spans 0 .. 287.000
//   y = 0 at the keyboard's FRONT outer edge     y spans 0 ..  97.500, +y REARWARD
//   z = 0 at the PCB UNDERSIDE - the seat        the aluminium face is z = +3.700
//
//   The source STEP file uses a different frame (X width, Y up with 0 at the
//   frame's top face, Z depth with low z at the REAR). Every number below was
//   converted once, here:
//       x = model_x + 10.500
//       y = 105.500 - model_z
//       z = model_y +  3.700
//   The exported reference meshes in v2/parts/models/k9pro-internals/ are in
//   the MODEL frame, not this one. Do not overlay them without converting.
//
// SOURCING - every default is tagged
//   [cad-exact] read off an analytic surface in Keychron's own assembly model,
//               K9-Pro-US-Full-Model_3d_20241128.stp: a plane's position, a
//               cylinder's radius and axis, a wire's extent. No tessellation,
//               no sampling, no tolerance.
//   [cad]       measured from the same file after triangulating at 0.020 mm;
//               carries about +/-0.02 from the mesh deflection.
//   [derived]   arithmetic from the above, shown.
//   [vendor]    published by Keychron.
//   [UNSOURCED] NOT a measurement. Guarded default. Get the calipers.
//   Full provenance, and the 11 measurements that confirm these:
//   v2/parts/keyboard-strip.md
//
// RENDER
//   openscad -o /tmp/k9.stl -D 'part="all"' v2/cad/parts/k9pro_internals.scad
//   An empty or tiny mesh is a failure symptom, not a quiet pass.

// ====================================================================== shell

// Outer footprint of the aluminium top frame. This is the whole keyboard's
// plan size: the frame wraps outside the bottom case, so nothing we discard
// makes the footprint smaller.
frame_w  = 287.000;   // [cad-exact] from the four corner cylinders
frame_d  =  97.500;   // [cad-exact]
frame_r  =   4.400;   // [cad-exact] corner cylinders at (4.400, 4.400) etc

// The frame's skirt: a 1.000 wall whose bottom rim lies in the same 3.37914 deg
// plane as everything else in this keyboard. This is the deepest retained
// feature and it is what a carrier groove has to swallow.
skirt_t       = 1.000;   // [cad-exact] outer x -10.500, inner x -9.500
skirt_r_inner = 3.500;   // [cad-exact] inner corner cylinders
rim_z_front   = -2.838;  // [derived] model y -6.538  + 3.700
rim_z_rear    = -8.595;  // [derived] model y -12.295 + 3.700
skirt_rebate  =  0.400;  // [cad-exact] where the discarded tray's wall landed

// ====================================================================== plate

// The switch plate is the TOP 1.200 mm OF THE FRAME. It is not a separate part
// and it cannot be separated - one solid, CK8876_TOP_84-WG. Two exact planes.
plate_t      = 1.200;    // [cad-exact] y = 0.000 and y = -1.200
plate_top_z  = 3.700;    // [derived] model y 0.000 + 3.700
plate_w      = 285.000;  // [cad-exact] outer wire of the y=-1.200 face
plate_d      =  95.500;  // [cad-exact]
plate_x0     =   1.000;  // [cad-exact] model x -9.500 + 10.500
plate_y0     =   1.000;  // [cad-exact] 105.500 - model z 104.500
plate_chamf  =   0.200;  // [cad-exact] the y=0 wire is 0.200/side larger

// The cutouts. 14.000 square with R0.250 corners - 288 corner fillets over
// 61 switch cutouts plus 11 stabiliser slots, 72 x 4, which is the file
// checking its own arithmetic. The house cutout 08_cutouts/
// keyswitch_mx_plate_14mm.stl is exactly this size.
sw_cut    = 14.000;   // [cad-exact]
sw_cut_r  =  0.250;   // [cad-exact]
stab_cut  = [6.000, 13.500];   // [cad-exact] x by y, 10 of them
caps_cut  = [4.000, 15.250];   // [cad-exact] the 1.75u Caps bracket, 2 of them
space_cut = [15.500, 6.000];   // [cad-exact] one, under the spacebar

// =================================================================== the grid

// 19.000 mm, NOT 19.050. Every 1u switch lands on an exact 19.000 lattice in
// both axes and reproduces Keychron's own QMK LAYOUT_61_ansi to 0.000. The wide
// keys are nudged off the nominal unit centre by up to 0.040, so the measured
// columns below are used verbatim rather than recomputed.
//   v2/layouts/_common.py and cad/keyboard_blank.scad both assume 19.050 and a
//   285.75 x 95.25 field. The real field is 285.000 x 95.000.
key_pitch = 19.000;   // [cad-exact]
field_w   = 285.000;  // [derived] 15 x 19.000
field_d   =  95.000;  // [derived]  5 x 19.000

// Row y, front to back. [cad-exact] from the 61 central Phi 5.200 PCB holes.
row_y = [10.500,    // row 4  bottom row   model z 95.000
         29.500,    // row 3  zxcv         model z 76.000
         48.500,    // row 2  asdf         model z 57.000
         67.500,    // row 1  qwerty       model z 38.000
         86.500];   // row 0  number row   model z 19.000

// Switch x per row, same order as row_y. [cad-exact], converted from model x.
row_x = [
  // row 4 - bottom, 8 keys
  [ 12.875,  36.625,  60.375, 131.625, 202.870, 226.630, 250.390, 274.150],
  // row 3 - zxcv, 12 keys
  [ 22.400,  53.300,  72.300,  91.300, 110.300, 129.300,
   148.300, 167.300, 186.300, 205.300, 224.300, 259.895],
  // row 2 - asdf, 13 keys
  [ 17.650,  43.750,  62.750,  81.750, 100.750, 119.750, 138.750,
   157.750, 176.750, 195.750, 214.750, 233.750, 264.700],
  // row 1 - qwerty, 14 keys
  [ 15.260,  39.040,  58.040,  77.040,  96.040, 115.040, 134.040,
   153.040, 172.040, 191.040, 210.040, 229.040, 248.040, 271.800],
  // row 0 - number row, 14 keys
  [ 10.500,  29.500,  48.500,  67.500,  86.500, 105.500, 124.500,
   143.500, 162.500, 181.500, 200.500, 219.500, 238.500, 267.000]];

// Keycap width in key units, per key, same order. [vendor] via QMK.
row_u = [
  [1.25, 1.25, 1.25, 6.25, 1.25, 1.25, 1.25, 1.25],
  [2.25, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2.75],
  [1.75, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2.25],
  [1.5,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1.5],
  [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2]];

// Stabiliser slot centres in the plate. [cad-exact]
stab_xy = [[255.000, 85.250], [279.000, 85.250],   // 2u  backspace
           [252.700, 47.250], [276.700, 47.250],   // 2.25u enter
           [ 10.400, 28.250], [ 34.400, 28.250],   // 2.25u left shift
           [247.895, 28.250], [271.895, 28.250],   // 2.75u right shift
           [ 81.625, 11.750], [181.625, 11.750]];  // 6.25u space, +/-50.000
caps_xy  = [[  6.150, 49.125], [ 29.150, 49.125]]; // [cad-exact] Caps bracket
space_xy = [[108.875, 10.500]];                    // [cad-exact]

// ======================================================================== PCB

pcb_t     = 1.200;     // [cad-exact] y = -2.500 and y = -3.700. NOT 1.6.
pcb_x0    = 1.294;     // [cad-exact] model x -9.206 + 10.500
pcb_y0    = 1.694;     // [cad-exact] 105.500 - model z 103.806
pcb_w     = 282.211;   // [cad-exact]
pcb_d     =  93.211;   // [cad-exact]
pcb_bot_z = 0.000;     // THE SEAT, by definition of this frame
pcb_hole  = 5.200;     // [cad-exact] the central switch hole, one per key
pcb_boss_clear = 5.000;// [cad-exact] clearance for the frame's bosses, 9 off

// ============================================== switches, sockets, keycaps

// The switch station, as an envelope. Below the plate it must pass a 14.000
// cutout; above it the housing flange is wider but it is always inside the
// keycap's own footprint, so it never reaches the enclosure.
sw_body     = 14.000;  // [derived] it passes the 14.000 cutout
sw_flange   = 15.052;  // [cad] GATERON-A_ASM plan extent
sw_pin_z    = -1.952;  // [derived] model y -5.652 + 3.700, pin tips
sw_housing_z=  7.003;  // [derived] model y  3.303 + 3.700, top housing top
sw_stem     = [10.602, 7.026];  // [cad] MANIFOLD_SOLID_BREP_204
sw_stem_z   =  9.722;  // [derived] model y  6.022 + 3.700

// The hot-swap socket. THIS IS THE DEEPEST THING UNDER THE SWITCH FIELD and it
// is what sets the carrier's pocket depth. Envelope and offset are relative to
// the key centre, converted from the model's X -8.514..+6.587, Z -6.041..+7.950.
sock_x    = [-8.514,  6.587];  // [cad] relative to the key centre, this frame
sock_y    = [-7.950,  6.041];  // [cad] note the sign flip from model Z
sock_z    = -2.091;            // [derived] model y -5.791 + 3.700

// Keycap, LSA. 18.2000 square for 1u on a 19.000 pitch, so 0.800 between caps.
// THIS FILE CARRIED 18.731 AND IT IS A BOUNDING-BOX ARTEFACT, not a cap. It is
// manifest.json's figure, and that manifest pads every face of this very shape
// by a uniform +0.2657: it records the 1u cap at [-9.3657, 2.7343, 9.6343,
// 9.3657, 9.841, 28.3657] where k9pro-keycap-1u-one.stl's own mesh is EXACTLY
// x -9.1000..+9.1000, y 3.0000..9.5753, z 9.9000..28.1000 -- 18.2000 square.
// The control is the frame: the same manifest records it as 287.3004 x 97.8004
// where k9pro-topcase-switchplate.stl measures EXACTLY 287.000 x 97.500, both
// [CAD-EXACT] and [VENDOR]. So the pad is real and it is PER SHAPE (0.5314 on
// the cap's span, 0.3004 on the frame's), which is why parameters.scad 965-986
// settles the cap at 18.2000 [CAD-EXACT] off the B-rep with AddOptimal and puts
// the inter-cap gap at 0.800. At 0.269 every 1u cap in the render spanned
// 18.7310 -- 0.531 too wide, with 0.269 of air between neighbours instead of
// 0.800, 61 times over.
cap_gap   =  0.800;   // [cad-exact] 19.000 - 18.200, parameters.scad 965-986
cap_bot_z =  6.643;   // [cad] model y +2.943 + 3.700
cap_top_z = 13.542;   // [cad] model y +9.842 + 3.700
// THE SPACEBAR LINE, AND IT IS FLAGGED RATHER THAN RECONCILED. It read
// "Measured spacebar plan is 118.511; the formula gives 118.481. 0.030 apart."
// The formula is 6.25 x 19.000 - cap_gap, which is identically
// 5.25 x 19.000 + cap_width, so at 18.200 it gives 117.950 and the residual
// against 118.511 is 0.561 and not 0.030.
//   WHAT WOULD RESCUE THE 0.030, exactly: the residual is measured - 99.750 -
// cap_width, so a bbox pad present in BOTH terms cancels and the residual is
// invariant to it. If 118.511 is itself a manifest-convention figure carrying
// this cap family's own +0.5314 on the span, its true plan is 117.9796 and the
// residual is 0.0296 -- the line's own 0.030, arriving back.
//   IT CANNOT BE DECIDED HERE AND IT IS NOT GUESSED. 118.511 appears in NONE of
// the extraction's seven files -- manifest.json, detail.json, measured.json,
// parts.json, tree.json, cylinders.json, wires.json (the only near match is
// 118.525, a PCB wire's bbox) -- and the extraction carries no 6.25u cap solid
// to measure, so its provenance is unrecorded and the pad is per shape and so
// not transferable from the 1u cap. CLOSES BY isolating the 6.25u cap out of
// k9pro-stripped-assembly.stl (617 solids; union-find over faces sharing a
// vertex, which check_cad.mesh_bodies already does) and taking its plan: a mesh
// bbox carries no pad, so that one number decides between 117.950 and 118.511.

// ====================================================== bosses and the screws

boss_od    = 4.000;   // [cad-exact] all 18 bosses, both families
boss_pilot = 1.700;   // [cad-exact] a moulding core; only 7 take a screw
plate_seat_z = 2.491; // [derived] model y -1.209 + 3.700, boss top
pcb_seat_z   = -0.009;// [derived] model y -3.709 + 3.700, pad top

// The 7 SCREWED positions. Reproduce these and the stripped assembly bolts
// into the carrier with the keyboard's own screws through its own holes.
screw_xy = [[ 27.040, 71.000], [ 43.800, 33.000], [ 96.000, 90.000],
            [138.820, 33.000], [191.000, 90.000], [233.800, 33.000],
            [258.040, 71.000]];                       // [cad-exact]
// The plate's hole over each: a tapered SQUARE, 4.500 at the top face and
// 2.500 at the underside. Unusual, and it is what the part has.
screw_sq_top = 4.500; // [cad-exact]
screw_sq_bot = 2.500; // [cad-exact]

// The 4 plate props - same boss, no screw, no hole above them.
prop_xy = [[ 58.060, 90.010], [110.250, 52.000],
           [186.300, 52.010], [219.500, 71.010]];     // [cad-exact]

// The 7 PCB bosses, top at the PCB underside.
pcb_boss_xy = [[ 48.615, 67.551], [ 53.799, 48.551], [ 81.800, 29.551],
               [148.125, 48.551], [157.236, 29.551], [243.250, 48.551],
               [263.895, 17.551]];                    // [cad-exact]

// The 26 further pads the discarded tray offered the PCB, as [x0,x1,y0,y1] in
// this frame. With the 7 boss annuli they total 261.350 mm2 - 1.177 % of the
// board's area. A printed carrier can and should offer a plane instead; these
// are drawn so a design can be checked against what the stock case did.
seat_pads = [
  [ 1.899,  5.199, 32.411, 38.411], [ 1.899,  5.199, 63.411, 69.411],
  [ 2.199,  6.999,  9.149,  9.949], [ 2.199,  5.199, 85.351, 91.950],
  [ 6.400, 14.400, 28.453, 29.957], [30.400, 38.400, 28.453, 29.957],
  [74.101, 79.101, 90.650, 95.650], [77.650, 78.450,  7.301, 10.806],
  [84.850, 85.650,  7.301, 10.806], [87.159, 93.159,  1.949,  5.949],
  [119.500,121.000, 93.450, 96.450], [119.675,120.675,  1.949,  5.700],
  [146.970,155.070, 88.651, 96.450], [177.650,178.450,  7.301, 10.806],
  [184.850,185.650,  7.301, 10.806], [185.500,196.500, 92.650, 95.650],
  [243.895,251.895, 28.453, 29.957], [248.700,256.700, 47.453, 48.957],
  [251.000,259.000, 85.453, 86.957], [267.895,275.895, 28.453, 29.957],
  [272.700,280.700, 47.453, 48.957], [274.870,280.870, 92.550, 95.351],
  [275.000,283.000, 85.453, 86.957], [281.001,284.801,  8.149,  8.949],
  [281.801,285.101, 33.211, 39.211], [281.801,285.101, 67.211, 73.211]];
                                                      // [cad-exact]

// ====================================================================== ports

// Every one of these exits through the frame's OWN rear wall at y = 97.500, a
// part we keep. Stripping loses no port presentation. Apertures are the inner
// wires of that wall's single planar face, measured without tessellation.
// Each entry is [x0, x1, z0, z1] on the plane y = frame_d.
aperture_usbc  = [129.180, 138.820, -3.810,  0.050];  // [cad-exact] 9.640 x 3.860
aperture_mode  = [ 34.450,  43.550, -3.550, -0.050];  // [cad-exact] 9.100 x 3.500
aperture_os    = [ 16.350,  23.650, -3.550, -0.050];  // [cad-exact] 7.300 x 3.500
aperture_led   = [150.520, 151.520, -2.080, -1.080];  // [cad-exact] 1.000 x 1.000
rear_wall_t    = 1.000;   // [cad-exact] a plug passes 1.492 before the socket

// The parts behind them, as [x0,x1, y0,y1, z0,z1].   [cad]
usbc_body  = [129.522, 138.478, 88.412, 96.008, -3.168,  1.208];
os_body    = [ 16.044,  23.956, 89.644, 94.906, -1.406,  0.606];
mode_body  = [ 33.481,  44.494, 89.694, 94.906, -1.406,  0.506];
led_pipe   = [148.000, 154.000, 95.500, 96.500, -2.600,  2.500];
// The slider caps reach z = -4.560, DEEPER THAN THE SOCKETS, and they stand
// 1.110 proud of the rear wall - 0.635 into the deck's 3.200 spine.
cap_slider = [ 13.215,  46.498, 93.790, 98.610, -4.560,  2.210];
// Throw, from the two positions each cap is drawn in.   [derived]
os_throw   = 1.575;   // cap centres x 19.225 and 20.800
mode_step  = 1.500;   // cap centres x 37.488, 38.988, 40.488

// ===================================================================== battery

// 130.000 x 32.000 x 3.000 exactly - the modelled prism's volume is 12480.000
// mm3 and 130 x 32 x 3 = 12480.000. 1550 mAh [vendor].
bat        = [130.000, 32.000, 3.000];   // [derived], exact
bat_swell  = 0.600;    // [UNSOURCED] a pouch cell gains thickness with age;
                       // 10 % of 3.000 is 0.300, doubled for margin
bat_clear  = [0.500, 0.500];             // [UNSOURCED] plan clearance per side
// In the stock keyboard the cell lies TILTED in the floor plane: x centre
// 150.500, y 55.747..87.868, z -7.020..-2.139. On a flat carrier it lies flat,
// and the only placement that clears the socket pocket is below its floor.
bat_mode   = "flat";                     // "flat" | "stock"
bat_xy     = [150.500, 71.808];          // [cad] stock plan centre
bat_top_z  = -2.391;                     // [derived] sock_z - 0.300 liner
// NOT SOURCED: the connector, its position on the PCB, and the lead length.
// Until #1, #2 and #7 of keyboard-strip.md section 5.3 are measured, the bay
// cannot be said to be allowed to move.

// =================================================================== controls

clearance = 0;        // grow every retained solid by this, for cutting a pocket
$fn       = 48;

// part = all | frame | plate | pcb | sockets | switches | keycaps
//        battery | ports | seat | bosses | envelope
//   envelope - the union of everything retained, grown by `clearance`.
//              This is the one to subtract from a carrier.
//   seat     - the 33 pads the discarded tray offered the PCB, as a check.
//   bosses   - the 18 bosses, so a carrier can be overlaid on them.
part = "all";

// ===================================================================== helpers

module rrect(w, d, r) { offset(r = r) offset(r = -r) square([w, d]); }

// A slab on a rounded-rectangle footprint, placed at (x0, y0, z0).
module slab(x0, y0, w, d, r, z0, h) {
    translate([x0, y0, z0]) linear_extrude(height = h) rrect(w, d, r);
}

module box6(b) {   // [x0,x1,y0,y1,z0,z1]
    translate([b[0], b[2], b[4]])
        cube([b[1]-b[0], b[3]-b[2], b[5]-b[4]]);
}

// Walk every key: f(x, y, u) for all 61.
module each_key() {
    for (r = [0 : len(row_y)-1])
        for (i = [0 : len(row_x[r])-1])
            translate([row_x[r][i], row_y[r], 0]) children();
}

function key_u(r, i) = row_u[r][i];

// ====================================================================== bodies

// The 1.200 plate, with all 72 openings and the 7 screw holes. Drawn as the
// underside outline plus a 0.200 chamfered top, which is what the two measured
// wires describe.
module k9_plate() {
    difference() {
        union() {
            slab(plate_x0, plate_y0, plate_w, plate_d, skirt_r_inner,
                 plate_top_z - plate_t, plate_t - plate_chamf);
            slab(plate_x0 - plate_chamf, plate_y0 - plate_chamf,
                 plate_w + 2*plate_chamf, plate_d + 2*plate_chamf,
                 skirt_r_inner + plate_chamf,
                 plate_top_z - plate_chamf, plate_chamf);
        }
        // 61 switch cutouts
        each_key() translate([0, 0, plate_top_z - plate_t - 0.5])
            linear_extrude(height = plate_t + 1)
                rrect(sw_cut, sw_cut, sw_cut_r);
        // stabiliser and bracket slots
        for (p = stab_xy) translate([p[0], p[1], plate_top_z - plate_t - 0.5])
            linear_extrude(height = plate_t + 1)
                translate([-stab_cut[0]/2, -stab_cut[1]/2])
                    rrect(stab_cut[0], stab_cut[1], sw_cut_r);
        for (p = caps_xy) translate([p[0], p[1], plate_top_z - plate_t - 0.5])
            linear_extrude(height = plate_t + 1)
                translate([-caps_cut[0]/2, -caps_cut[1]/2])
                    rrect(caps_cut[0], caps_cut[1], sw_cut_r);
        for (p = space_xy) translate([p[0], p[1], plate_top_z - plate_t - 0.5])
            linear_extrude(height = plate_t + 1)
                translate([-space_cut[0]/2, -space_cut[1]/2])
                    rrect(space_cut[0], space_cut[1], sw_cut_r);
        // the 7 tapered square screw holes
        for (p = screw_xy) translate([p[0], p[1], plate_top_z])
            rotate([180, 0, 0])
                linear_extrude(height = plate_t + 0.002,
                               scale = screw_sq_bot / screw_sq_top)
                    translate([-screw_sq_top/2, -screw_sq_top/2])
                        square(screw_sq_top);
    }
}

// The skirt: a 1.000 wall on the outer footprint, its rim in the 3.37914 deg
// plane, with the 0.400 rebate the discarded tray's wall sat in.
module k9_skirt() {
    difference() {
        // full-depth outer prism from the deepest rim up to the plate
        slab(0, 0, frame_w, frame_d, frame_r, rim_z_rear, plate_top_z - rim_z_rear);
        // hollow it
        slab(skirt_t, skirt_t, frame_w - 2*skirt_t, frame_d - 2*skirt_t,
             skirt_r_inner, rim_z_rear - 1,
             plate_top_z - plate_t - rim_z_rear + 1);
        // cut the rim back to the inclined plane: a wedge below it
        translate([-1, 0, 0]) rotate([0, 90, 0])
            linear_extrude(height = frame_w + 2)
                polygon([[rim_z_front, 0], [rim_z_rear, frame_d],
                         [rim_z_rear - 20, frame_d], [rim_z_front - 20, 0]]);
        // the four rear-wall apertures, and the rebate
        for (a = [aperture_usbc, aperture_mode, aperture_os, aperture_led])
            box6([a[0], a[1], frame_d - skirt_t - 0.5, frame_d + 0.5,
                  a[2], a[3]]);
    }
}

module k9_frame() { union() { k9_plate(); k9_skirt(); } }

module k9_pcb() {
    difference() {
        translate([pcb_x0, pcb_y0, pcb_bot_z]) cube([pcb_w, pcb_d, pcb_t]);
        each_key() translate([0, 0, -0.5]) cylinder(d = pcb_hole, h = pcb_t + 1);
        for (p = concat(screw_xy, prop_xy))
            translate([p[0], p[1], -0.5])
                cylinder(d = pcb_boss_clear, h = pcb_t + 1);
    }
}

// One envelope per key for the hot-swap sockets. At 19.000 pitch the 15.101 x
// 13.991 footprints leave a 3.899 web across and a 5.009 web along, so a
// per-key pocket is possible - but one pocket over the whole field is stronger.
module k9_sockets() {
    each_key() box6([sock_x[0], sock_x[1], sock_y[0], sock_y[1],
                     sock_z, pcb_bot_z]);
}

module k9_switches() {
    each_key() {
        box6([-sw_body/2, sw_body/2, -sw_body/2, sw_body/2,
              sw_pin_z, plate_top_z]);
        box6([-sw_flange/2, sw_flange/2, -sw_flange/2, sw_flange/2,
              plate_top_z, sw_housing_z]);
        box6([-sw_stem[0]/2, sw_stem[0]/2, -sw_stem[1]/2, sw_stem[1]/2,
              sw_housing_z, sw_stem_z]);
    }
}

// The keycaps as a correct envelope: each cap's own plan size from its width in
// key units, 0.269 of air between neighbours, one flat top plane.
module k9_keycaps() {
    for (r = [0 : len(row_y)-1])
        for (i = [0 : len(row_x[r])-1]) {
            w = key_u(r, i) * key_pitch - cap_gap;
            translate([row_x[r][i] - w/2, row_y[r] - (key_pitch - cap_gap)/2,
                       cap_bot_z])
                cube([w, key_pitch - cap_gap, cap_top_z - cap_bot_z]);
        }
}

module k9_ports() {
    box6(usbc_body); box6(os_body); box6(mode_body); box6(led_pipe);
    box6(cap_slider);
}

module k9_battery() {
    if (bat_mode == "stock") {
        // as Keychron places it: lying in the 3.37914 deg floor plane
        translate([bat_xy[0], bat_xy[1], (-7.020 + -2.139)/2])
            rotate([-atan(0.0590457), 0, 0])
                cube([bat[0], bat[1], bat[2]], center = true);
    } else {
        translate([bat_xy[0] - bat[0]/2, bat_xy[1] - bat[1]/2,
                   bat_top_z - bat[2] - bat_swell])
            cube([bat[0], bat[1], bat[2] + bat_swell]);
    }
}

// What the discarded tray gave the PCB: 7 boss annuli plus 26 pads. Drawn
// 0.400 thick so it is visible; it is a surface, not a solid.
module k9_seat() {
    for (p = pcb_boss_xy) translate([p[0], p[1], pcb_seat_z - 0.400])
        difference() {
            cylinder(d = boss_od, h = 0.400);
            translate([0, 0, -0.5]) cylinder(d = boss_pilot, h = 1.4);
        }
    for (b = seat_pads)
        box6([b[0], b[1], b[2], b[3], pcb_seat_z - 0.400, pcb_seat_z]);
}

// The 18 bosses as the tray presented them, so a carrier can be overlaid.
module k9_bosses() {
    for (p = screw_xy) translate([p[0], p[1], rim_z_rear])
        difference() {
            cylinder(d = boss_od, h = plate_seat_z - rim_z_rear);
            translate([0, 0, plate_seat_z - rim_z_rear - 6.000])
                cylinder(d = boss_pilot, h = 6.001);
        }
    for (p = prop_xy) translate([p[0], p[1], rim_z_rear])
        cylinder(d = boss_od, h = plate_seat_z - rim_z_rear);
    for (p = pcb_boss_xy) translate([p[0], p[1], rim_z_rear])
        difference() {
            cylinder(d = boss_od, h = pcb_seat_z - rim_z_rear);
            translate([0, 0, pcb_seat_z - rim_z_rear - 5.000])
                cylinder(d = boss_pilot, h = 5.001);
        }
}

module k9_all() {
    k9_frame(); k9_pcb(); k9_sockets(); k9_switches(); k9_keycaps();
    k9_ports(); k9_battery();
}

// Everything retained, as one solid, grown by `clearance`. Subtract this.
module k9_envelope() {
    if (clearance > 0) minkowski() { k9_all(); sphere(r = clearance); }
    else k9_all();
}

// ======================================================================== main

if      (part == "all")      k9_all();
else if (part == "frame")    k9_frame();
else if (part == "plate")    k9_plate();
else if (part == "pcb")      k9_pcb();
else if (part == "sockets")  k9_sockets();
else if (part == "switches") k9_switches();
else if (part == "keycaps")  k9_keycaps();
else if (part == "battery")  k9_battery();
else if (part == "ports")    k9_ports();
else if (part == "seat")     k9_seat();
else if (part == "bosses")   k9_bosses();
else if (part == "envelope") k9_envelope();
else assert(false, "part must be one of all frame plate pcb sockets switches keycaps battery ports seat bosses envelope");

echo(str("k9pro_internals: footprint ", frame_w, " x ", frame_d,
         "  seat z=0  plate top z=", plate_top_z));
echo(str("  keycap top z=", cap_top_z, "  skirt rim rear z=", rim_z_rear,
         " front z=", rim_z_front));
echo(str("  switch field ", field_w, " x ", field_d, " at ", key_pitch,
         " pitch ; ", len(row_x[0]) + len(row_x[1]) + len(row_x[2])
         + len(row_x[3]) + len(row_x[4]), " keys ; ", len(screw_xy), " screws"));
echo(str("  socket floor z=", sock_z, "  USB-C floor z=", usbc_body[4],
         "  slider cap floor z=", cap_slider[4]));
