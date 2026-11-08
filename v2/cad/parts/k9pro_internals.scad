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
//  15.052 WAS 0.0521 PROUD OF EVERY ARTEFACT ON THIS DISK and its provenance
//  is not reproducible from any of the three extractions. tree.json carries
//  GATERON-A_ASM at x -7.5017668..+7.5017668, span 15.0035336, and that entry
//  is inflated by a UNIFORM gap of +0.0018354 per face -- solved below against
//  the switch mesh's own exact -5.650000 / +6.000000, and holding on all six
//  faces to 3e-7. De-gapped the span is 14.9998628, which is the switch
//  mesh's own 14.999863 to seven decimals. Two artefacts agreeing, so 15.052
//  is not a plan extent; it is 0.052 of width this file was drawing that the
//  switch does not have.
sw_flange   = 14.9999; // [cad] GATERON-A_ASM plan extent, tree.json de-gapped
                       //   and confirmed by k9pro-switch-gateron-one.stl
// THE TWO Z EXTREMES ARE NOW EXACT, not +/-0.020. They read "model y -5.652"
// and "model y 6.022", which are a 0.020-deflection triangulation and carry
// the [cad] tolerance this file declares. k9pro-switch-gateron-one.stl's own
// mesh is EXACTLY y -5.650000..+6.000000, and the de-pad is confirmed on all
// six faces against manifest.json, which records that shape as
// [-7.6598, -5.8099, 11.3402, 7.6598, 6.1599, 26.6598] where the mesh is
// x -7.4999..+7.4999, y -5.650000..+6.000000, z +11.5001..+26.4999 -- a
// uniform +0.1599 per face, a THIRD per-shape pad (the 1u cap's is +0.2657,
// the frame's +0.1502). y max de-pads to exactly 6.000000, so the stem top is
// a plane at a round number and the old 6.022 was the deflection, not a datum.
sw_pin_z    = -1.950;  // [cad-exact] model y -5.650000 + 3.700, pin tips
sw_housing_z=  7.0015; // [cad] model y 3.3015367 + 3.700, top housing top.
                       //   THE FLAG IS CLOSED, AND WITHOUT THE STEP FILE.
                       //   This read 7.003 and was FLAGGED "left alone on
                       //   purpose": the upper housing is its own connected
                       //   solid in the mesh (3078 facets, plan
                       //   x -7.4761..+7.1375) with y max +3.250251, 0.053
                       //   BELOW the old line and more than the +/-0.020
                       //   deflection explains -- but 3.250251 is not a round
                       //   number, so it is the triangulation of a crowned or
                       //   filleted top, and an analytic B-rep plane can
                       //   legitimately sit above a tessellation of a dome.
                       //   That reasoning was right and the note said it
                       //   closes by reading the plane off the STEP file.
                       //   IT DOES NOT NEED THE STEP FILE. tree.json carries
                       //   that solid -- MANIFOLD_SOLID_BREP_209_1, y
                       //   -2.2018203..+3.3033721 -- and the same uniform gap
                       //   that de-gaps GATERON-A_ASM, +0.0018354, de-gaps
                       //   this solid's BOTTOM to -2.2000, which is the
                       //   [cad-exact] figure keyboard-strip.md's stack table
                       //   already carries for the upper housing. One gap,
                       //   two faces, independently confirmed; solving it off
                       //   that -2.2000 instead gives +0.0018203 and moves
                       //   the answer by 1.5e-5. So the analytic top is
                       //   3.30154 either way and this line is 7.0015, not
                       //   7.003. The old 7.003 is the same figure rounded
                       //   from a 3-dp quotation of the padded tree value.
sw_stem     = [10.602, 7.026];  // [cad] MANIFOLD_SOLID_BREP_204
sw_stem_z   =  9.700;  // [cad-exact] model y  6.000000 + 3.700, stem top

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

// CAP Z, AND BOTH FIGURES WERE THE SAME MANIFEST INFLATION cap_gap WAS. These
// lines read "model y +2.943 + 3.700" and "model y +9.842 + 3.700", giving
// 6.643 and 13.542 for a cap 6.899 tall. The mesh's own y span is
// 3.000000..9.575284, so the bottom was 0.057 LOW, the top 0.267 HIGH and the
// 1u cap 0.3237 too TALL. Measured directly, this run:
//     k9pro-keycap-1u-one.stl        y 3.000000 .. 9.575284   span 6.575284
//     k9pro-topcase-switchplate.stl  y          .. 0.000000   plate top, so
//                                    the + 3.700 frame offset is confirmed
// 9.842 is manifest.json's padded 9.841 rounded up. 2.943 is in neither the
// manifest (2.7343) nor the mesh (3.000000) and its provenance is unrecorded;
// it is retired rather than explained, because the mesh settles the question.
cap_bot_z =  6.700;   // [cad-exact] model y 3.000000 + 3.700. EXACT and
                      //   UNIFORM: all 61 cap bodies in
                      //   k9pro-stripped-assembly.stl have y min 3.000000.

// THE TOP IS NOT ONE PLANE AND THE MESH SAYS SO. Isolating all 61 cap bodies
// out of k9pro-stripped-assembly.stl (union-find over faces sharing a vertex)
// gives a y max that rises monotonically with cap WIDTH. THE MECHANISM THIS
// NOTE GAVE FOR THAT WAS WRONG: it said the bbox maximum is "at the cap's own
// left and right edges -- the wider the cap, the further that edge rides up
// the dish". It is not at the edges. k9pro-keycap-1u-one.stl has exactly ONE
// vertex at its y max, at x +5.2563 and z +25.7049 -- 3.844 INSIDE the
// left/right edge, 57.8 % of the half-width, and 6.705 REAR of the cap's
// centre. So the high point is on the rear rim. The monotonic rise with width
// is measured, on all 61 bodies, and it stands; why it follows width is left
// OPEN rather than re-explained, because the location was checked on the 1u
// cap only and isolating the other eight to settle it is not done here:
//     u     plan span   MESH y max   ANALYTIC y max   this frame
//     1.00    18.2000      9.5753         9.5759         13.2759   x46
//     1.25    22.9500      9.6062         9.6130         13.3130   x 7
//     1.50    27.7000      9.6327         9.6386         13.3386   x 2
//     1.75    32.4500      9.6515         9.6579         13.3579   x 1
//     2.00    37.2000      9.6659         9.6730         13.3730   x 1
//     2.25    41.9500      9.6768         9.6844         13.3844   x 1  l shift
//     2.25    42.0500      9.6770         9.6846         13.3846   x 1  enter
//     2.75    51.4500      9.6871         9.6958         13.3958   x 1
//     6.25   118.0000      9.7571         9.7572         13.4572   x 1  space
// Spread 0.1813 over the field. So ONE flat top cannot be both an envelope and
// exact: 13.2759 is the 1u cap's own top and under-states the spacebar by
// 0.1813, which would be an envelope that does not enclose. The body takes
// the table per width and no cap is stated wrong.
//
// WHY THE ANALYTIC COLUMN IS THE ONE THE BODY TAKES, AND WHY THE TAG CAME OFF.
// The MESH column is the bbox maximum of a TRIANGULATION of a dished top, and
// this file's own standard refuses that: it is the identical case in which
// sw_housing_z keeps its CAD value because "an analytic B-rep plane can
// legitimately sit above a tessellation of a dome". A table of mesh maxima
// tagged [cad-exact] -- defined in this file as "read off an analytic surface
// ... No tessellation, no sampling, no tolerance" -- applied two incompatible
// standards to two identically-shaped features in one change, and it
// under-stated every cap, worst +0.0087 at 2.75u. So the envelope STILL did
// not enclose, which is the defect a flat 13.2753 was rejected for.
//   WHERE THE ANALYTIC COLUMN COMES FROM, and it is an inference, stated as
// one. k9pro-internals/measured.json carries all nine caps BY NAME --
// 1-KEY_ASM, 1_25KEY_ASM, TAB_ASM, CAPS_ASM, BACKSPACE_ASM, SHIFT_L_ASM,
// ENTER_ASM, R_SHIFT_ASM, KT21005 -- as B-rep bboxes that the extraction
// inflated by a uniform per-shape pad, the same pad cap_gap's note above is
// about. Solving each shape's pad from its y MIN alone (every cap bottoms at
// model y 3.000000 exactly, which the mesh settles independently) gives nine
// pads: 0.2657, 0.2687, 0.2719, 0.1937, 0.1939, 0.1941, 0.1941, 0.2524,
// 0.2553 -- non-monotone, so no two transfer, which is the whole reason a
// single pad convention cannot be used on this file. De-padded by its OWN pad,
// each cap's x span comes out at exactly the plan span in the column above and
// its z span at exactly 18.200000, to <= 7e-6, on all nine. That is two
// independent confirmations per cap of a quantity solved from a third face,
// nine times over. It is still an inference about the extraction's bbox gap
// rather than a plane queried off a surface, so it is [CAD] and not
// [CAD-EXACT].
//   THE PROCEDURE THAT WOULD CLOSE IT: read each dish's analytic maximum
// directly off K9-Pro-US-Full-Model_3d_20241128.stp with OCP 8.0.1.1 in
// /usr/bin/python3.11 (BRepAdaptor_Surface, no tessellation); measured.json
// gives every shape's full path and name. THAT STEP FILE IS NOT ON THIS DISK
// -- v2/parts/models/ holds the STLs and the three JSON extractions of it and
// no .stp for the keyboard -- so the procedure needs the vendor download
// again and cannot be run from the repository as it stands.
function cap_top_u(u) =          // [CAD] see the block above
      u <= 1.00 ? 13.2759
    : u <= 1.25 ? 13.3130
    : u <= 1.50 ? 13.3386
    : u <= 1.75 ? 13.3579
    : u <= 2.00 ? 13.3730
    : u <= 2.25 ? 13.3846   // the two 2.25u caps differ by 0.0002; the taller
                            //   one is ALSO the wider one (enter), so the
                            //   envelope encloses both
    : u <= 2.75 ? 13.3958
    :             13.4572;  // any width above 2.75u, which here is only 6.25u
// NOT A LITERAL, AND IT USED TO BE ONE. This line read "cap_top_z = 13.4571"
// with a comment calling it "the MAXIMUM of the table above" -- but nothing
// made that true, and breaking cap_top_u() to a flat 13.2753, and then to
// 113.2753, left this echoing 13.4571 unchanged in both. It now IS the table's
// maximum because it is the table's own widest entry.
cap_top_z = cap_top_u(6.25); // [CAD] 13.4572, the tallest cap in the
                     //   assembly, which is the spacebar. This is the single
                     //   number for anything that needs the field's top
                     //   plane; the bodies are drawn from cap_top_u() per cap.

// THE SPACEBAR LINE, SETTLED BY MEASUREMENT. It read "Measured spacebar plan
// is 118.511; the formula gives 118.481. 0.030 apart." The flag asked for the
// 6.25u cap to be isolated out of k9pro-stripped-assembly.stl and its plan
// taken, because a mesh bbox carries no pad. Done, this run:
//     the assembly's 1317088 facets carry 653556 distinct vertices; union-find
//     over faces sharing one gives 573 connected bodies. Exactly 61 of them
//     have z span 18.2000 and y min 3.000000 -- the 61 caps, matching the 61
//     keys row_u declares. ONE of those 61 has an x span over 52:
//         body 244   x 62.1250..180.1250   y 3.0000..9.7571   z 85.9000..104.1000
//     and four independent facts make it the 6.25u cap and nothing else:
//       * its z centre 95.000 is row_y's own "row 4 bottom row model z 95.000";
//       * its x centre 121.1250 + this file's documented x = model_x + 10.500
//         is 131.6250, which is row_x[0][3] EXACTLY;
//       * row_u[0][3] is 6.25;
//       * stab_xy's "6.25u space, +/-50.000" pair centres on 131.625 too.
//     PLAN = 118.0000 x 18.2000.
//   SO THE PLAN IS 118.0000 -- AND 118.511 IS THAT SAME NUMBER, PADDED. This
// block first concluded "118.511 IS REFUTED, by 0.511 ... its provenance stays
// unrecorded AND it is now contradicted by the artefact", on the grounds that
// the 1u cap's +0.2657 pad would give 118.5314 and 118.511 misses it by
// 0.0204. That test was the wrong one, and it was defeated by this file's own
// load-bearing fact: THE PAD IS PER SHAPE, so the 1u cap's pad was never the
// spacebar's to use. measured.json[969], name KT21005_1_1, bbox
// [61.8697, 2.7447, 85.6447, 180.3803, 10.0125, 104.3553]: x span 118.5106.
// Its OWN pad, solved from y min alone -- 3.000000 - 2.7447 = 0.2553 -- takes
// that to exactly 118.0000000, and its z span to exactly 18.2000000. So
// 118.511 was RECORDED all along, it is the padded form of precisely the
// 118.0000 the mesh gives, and the artefact confirms it rather than
// contradicting it. Nothing about the plan changes; the claim about it does.
//   The 0.030 does not come back: against the formula 6.25 x 19.000 - cap_gap
// = 117.950 the residual is +0.0500.
//   WHERE THE 0.050 LIVES, measured: every cap is inset from its own nominal
// cell by a per-cap amount, not by cap_gap/2. 59 of the 61 are inset 0.400 a
// side, so cap_gap = 0.800 is right for them. The spacebar is inset 0.375 a
// side (gap 0.750) and enter 0.350 (gap 0.700). The width formula therefore
// under-states exactly two caps -- the spacebar by 0.050 and enter by 0.100 --
// and closing that needs a per-KEY inset table, not a per-u one, because the
// two 2.25u caps take different insets. CLOSED, below, by cap_gap_key(): the
// gap is 0.800 for 59 of the 61 and the two exceptions are named by POSITION,
// because width cannot name them -- left shift and enter are both 2.25u and
// take different insets, which is exactly why a per-u table could not do it.
//   WHAT IT WAS COSTING. The field's x span came out 284.274988 against
// parameters.scad 1008's measured kbd_cap_field = [284.325, 94.200]
// [CAD-MESH], short by 0.050018 at +x -- enter is the rightmost key of its
// row, so its missing 0.100 showed up as half of itself on the envelope. A
// 0.100 error on enter is the same order as the 0.1818 of cap-top spread this
// file calls fatal two blocks up, and it was live in the same change.
function cap_gap_key(r, i) =
      (r == 0 && i == 3)  ? 0.750     // the 6.25u spacebar, inset 0.375/side
    : (r == 2 && i == 12) ? 0.700     // enter, inset 0.350/side
    :                       cap_gap;  // 0.800, the other 59
function cap_w(r, i) = key_u(r, i) * key_pitch - cap_gap_key(r, i);

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
// key units, 0.800 of air between neighbours, one flat top plane PER WIDTH --
// not one across the field, because the measured tops spread 0.1818 and a
// single plane would have to either under-state the spacebar or over-state the
// 46 1u caps. cap_top_u() carries the measured table.
module k9_keycaps() {
    for (r = [0 : len(row_y)-1])
        for (i = [0 : len(row_x[r])-1]) {
            w = cap_w(r, i);
            translate([row_x[r][i] - w/2, row_y[r] - (key_pitch - cap_gap)/2,
                       cap_bot_z])
                cube([w, key_pitch - cap_gap,
                      cap_top_u(key_u(r, i)) - cap_bot_z]);
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
