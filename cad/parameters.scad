// ===========================================================================
//  cYbErDeCk - parameters.scad
//  SINGLE SOURCE OF TRUTH for every dimension in this project.
// ===========================================================================
//
//  RULES OF THIS FILE
//  ------------------
//  1. No number appears anywhere else in the CAD. If a model file contains a
//     bare dimension, that is a bug.
//  2. Every datum carries a provenance tag in its comment:
//
//       [MEASURED]  numerically recovered from a reference artefact by
//                   tools/measure_reference.py. Re-derivable; see
//                   export/reports/measurements.json.
//       [VENDOR]    published by the component manufacturer.
//       [DERIVED]   arithmetic on a [VENDOR] or [MEASURED] value.
//       [STANDARD]  an ISO/IEC standard or a manufacturer's fitting spec.
//       [DESIGN]    a choice made by this project. Rationale given inline.
//       [PROVISIONAL] not yet corroborated by a second independent source.
//                   Listed in docs/DATUMS.md under "Open items".
//
//  3. Anything tagged [PROVISIONAL] must be caliper-checked against the
//     physical parts before committing to a final print. tools/validate.py
//     fails the build if a [PROVISIONAL] datum is used in a load-bearing fit
//     without a declared safety margin.
//
//  COORDINATE SYSTEM
//  -----------------
//      origin  = centre of the device footprint, on the BACK OUTER face
//      +X      = device width,  left -> right as the user faces the display
//      +Y      = device height, bottom -> top  (keyboard at -Y, display at +Y)
//      +Z      = thickness, back face (Z=0) -> front face (Z=body_t)
//
//  The device is used in portrait: display on top, keyboard below.
//
// ===========================================================================

// ---------------------------------------------------------------------------
// 0. BUILD CONFIGURATION
// ---------------------------------------------------------------------------

// Render quality. 64 for exports, 32 while iterating.
$fn = 64;

// Nozzle diameter. Wall thicknesses below are integer multiples of this so
// that every wall prints as solid perimeters with no weak sparse infill.
//  0.8 mm NOZZLE. This deck is printed on a 0.8 nozzle, not a 0.4, and that is
//  not a slicer setting - it changes what "thin" means in every wall on the
//  object. A wall that was three comfortable extrusions at 0.4 is 1.2 at 0.8,
//  which the slicer resolves as a single bead with a gap beside it. The whole
//  PRINT class is written against this number, so raising it is what turns
//  "chunky and overbuilt" from an intention into a gate.
nozzle = 0.8;                       // [DESIGN]
layer_h = 0.3;                      // [DESIGN] typical for a 0.8 nozzle
//  The floor every structural wall has to clear: two full beads. One bead is
//  printable but single-walled, and single-walled is not what this object is.
min_wall = 2 * nozzle;              // [DERIVED] = 1.60

// Structural preset:
//   "overbuilt" - 3.2 mm walls (8 perimeters). Default. Impact-tolerant.
//   "compact"   - 2.4 mm walls (6 perimeters). ~1.6 mm narrower, ~2.4 mm
//                 shorter, noticeably less rigid. For pocket carry.
preset = "overbuilt";               // [DESIGN]

wall     = (preset == "compact") ? 2.4 : 3.2;   // [DESIGN] n x nozzle
spine    = wall;                                 // [DESIGN] inter-bay rib
front_t  = 2.4;                                  // [DESIGN] 12 layers @0.2
back_t   = 3.2;                                  // [DESIGN] 16 layers @0.2, and
                                                 //   8 extrusions, matching the wall


// ===========================================================================
// 1. WAVESHARE ESP32-S3-RLCD-4.2  -  TARGET COMPONENT A
// ===========================================================================
//
//  PRIMARY SOURCE. Waveshare publishes a 3D package for this board:
//    https://files.waveshare.com/wiki/ESP32-S3-RLCD-4.2/ESP32-S3-RLCD-4.2-3dFile.rar
//  containing a Creo STEP assembly, a dimensioned DXF and a dimensioned PDF
//  drawing. Every [VENDOR] figure below is read from that package, either from
//  a drawing DIMENSION entity or from a B-rep bounding box of a named solid.
//  This supersedes the "92.5 x 70.1 x 13.5 mm" figure that circulates on
//  distributor pages - see the correction note below.
//
//  BOARD DATUM FRAME (Waveshare's, retained so coordinates stay checkable):
//    U  0 -> 92.50   along the long PCB axis.  U = 92.50 is the USB-C / TF edge.
//    V  0 -> 69.10   along the short PCB axis. V = 69.10 is the button edge.
//    W  0 at the PCB BACK (component) face, +W toward the display.
//  Mapping into this file's device frame:  U -> +X,  V -> +Y,  W -> +Z.
//  That mapping is not assumed: it is fixed by three asymmetric features that
//  must land where the reference enclosure puts them - the buttons on V=69.10
//  (device top edge), USB-C/TF on U=92.50 (device right edge), and the battery
//  offset toward V=0 (device bottom).
//
//  THE STAND BASE IS DISCARDED. The board ships with a moulded back plate and a
//  fold-out 60-degree kickstand, both removable. This design leaves them off:
//    - the 70.10 mm figure disappears (that is the base, which overhangs the
//      PCB by exactly 1.00 mm on one long edge; the PCB itself is 69.10)
//    - 2.75 mm of stack thickness disappears with it
//    - the four SMTSO standoffs are on the PCB, not the base, so nothing is
//      lost mechanically
//  See docs/ASSEMBLY.md step 1.

// --- outline ---------------------------------------------------------------
board_w        = 92.50;   // [VENDOR] drawing text "92.5 PCB OD"
board_h        = 69.10;   // [VENDOR] drawing text "69.1 PCB OD"
board_pcb_t    = 1.60;    // [VENDOR] STEP BOARD solid
//  CORRECTED FROM A PRINTED PART. The drawing says R0.5; the board in hand is
//  a PERFECT RECTANGLE, square to the eye and to a straightedge at all four
//  corners. This is the same class of error as the 18650 holder (C-21): a
//  vendor radius that the real part does not have. It matters because the
//  pocket was sized on the assumption that its own corner arc could tuck
//  inside the board's - it cannot, because there is nothing to tuck into.
//  See docs/DATUMS.md C-34.
board_corner_r = 0.00;    // [MEASURED] square, all four corners

//  CORRECTION, recorded deliberately. Earlier revisions of this file carried
//  board_h = 70.1 from a distributor spec line, tagged [PROVISIONAL]. The
//  factory drawing shows that "92.5 x 70.1 x 13.5" conflates three different
//  objects: 92.50 is the PCB length, 70.10 is the STAND BASE outline, and 13.50
//  is the display-front-to-base-rear stack, which EXCLUDES the battery holder.
//  A pocket cut to 70.1 leaves a 1 mm gap on one long edge.

// --- depth stack, all in the W frame ---------------------------------------
board_w_display_front = 3.75;    // [VENDOR] display glass front face
board_w_pcb_front     = 1.60;    // [VENDOR]
board_w_pcb_back      = 0.00;    // [VENDOR] datum
board_w_standoff_end  = -7.00;   // [VENDOR] SMTSO-M2.5-7ET, 7.00 mm tall
board_w_battery_end   = -15.20;  // [VENDOR] 18650 holder, the deepest feature

// Clear depth the enclosure must provide between the front-face lip and the
// back plate's inner face: display front down to the standoff seating plane.
//  MEASURED ON THE HARDWARE and it is NOT what the vendor CAD implies. Calipers
//  on a real board, glass front to the back of the metal standoffs at the
//  corners, give 11.00 mm against the 10.75 this derives from Waveshare's own
//  W coordinates. The 0.25 difference is the whole gasket squeeze, so taking
//  the CAD value would have seated the display hard against the front panel
//  with no compliance at all. The measurement wins; see docs/DATUMS.md D-03.
board_stack_cad = board_w_display_front - board_w_standoff_end;   // = 10.75
board_stack     = 11.00;  // [MEASURED] calipers, corners, glass front to standoff back
board_depth     = 11.25;  // [DESIGN] board_stack + 0.25, taken up by a 0.5 mm
                          //   adhesive foam gasket round the display aperture
// How far the battery holder reaches past the standoff plane - this, not the
// cell diameter, is what the cowl actually has to swallow.
batt_protrusion = board_w_standoff_end - board_w_battery_end;     // = 8.20

//  A real board measures 92.7 x 69.1 against the drawing's 92.50 x 69.10. The
//  height is exact; the width is +0.20, which is ordinary PCB routing tolerance
//  (open item O-05) rather than a wrong datum, so the VENDOR nominal is kept
//  and the pocket is asserted against the measured figure instead.
board_w_measured = 92.70;  // [MEASURED] calipers on a real board
board_h_measured = 69.10;  // [MEASURED] exact agreement with the drawing
//  SIZED ON THE BOARD THAT EXISTS, not the drawing. board_w is Waveshare's
//  92.50; the owner's board measures 92.70, so a pocket built on the drawing
//  gave 0.400 mm per side in X, not the 0.500 it claimed. Both the datum and
//  the tolerance now come off the larger of the two, and the clearance is
//  raised to 0.60 because this is a part that has to SLIDE in past 11 mm of
//  depth, not merely fit.
board_w_max    = max(board_w, board_w_measured);   // [DERIVED] = 92.70
board_h_max    = max(board_h, board_h_measured);   // [DERIVED] = 69.10
board_fit      = 0.60;            // [DESIGN] per side, a sliding fit
board_pocket_w = board_w_max + 2 * board_fit;
board_pocket_h = board_h_max + 2 * board_fit;
//  The pocket's corner radius is NOT free. The PCB's corners are R0.50, so a
//  generously rounded pocket corner leaves material exactly where the board's
//  much sharper corner wants to be. With 0.5 mm per-side clearance the pocket
//  radius must not exceed about 2.2 mm; an earlier revision used 3.0 and the
//  board fouled all four corners by 0.33 mm. Asserted in section 6.
//  A ROUNDED POCKET CANNOT ACCEPT A SQUARE PART unless its radius is small
//  enough. For a square corner with c mm of clearance per side the pocket's
//  corner arc clears it only while
//        r <= c * sqrt(2) / (sqrt(2) - 1)
//  which at the old c = 0.40 (the drawing's 92.50 against a real 92.70) allowed
//  1.37 against a modelled 2.00 - so the arc left material 0.193 mm inside
//  where the board's corner had to be, and the board would not slide in.
//
//  Corner reliefs were tried first and are worse here: a circle at each
//  theoretical corner reaches x = 48.55, and the lower fastener bosses start at
//  47.94, so the relief chewed into them and left CGAL a sliver. Shrinking the
//  radius needs no extra geometry at all, and an FDM inside corner is rounded
//  to about nozzle/2 regardless of what the model says.
board_pocket_r = 1.20;            // [DESIGN] well inside the square-corner limit

// --- mounting --------------------------------------------------------------
//  Not four plain holes. The PCB carries four Ø4.20 through-holes, each with a
//  surface-mount SMTSO-M2.5-7ET standoff on the back face: Ø5.50 body, 7.00 mm
//  tall, female M2.5 thread. The enclosure drives M2.5 screws UP into those
//  standoffs from outside the back plate; it does not provide its own bosses.
board_mount_pitch_x = 85.50;  // [VENDOR] drawing DIMENSION; 3.50 inset from
board_mount_pitch_y = 62.10;  //   every PCB edge. Independently re-derived from
                              //   two unrelated reference enclosures, both
                              //   agreeing to 0.001 mm - see docs/DATUMS.md D-01.
board_screw         = 2.5;    // [VENDOR] M2.5 into the standoff's female thread
board_screw_clear   = 2.7;    // [STANDARD] ISO 273 close fit for M2.5
//  The screws supplied with the board are COUNTERSUNK - a sloped head, not a
//  flat one - and the back plate was cutting a flat-bottomed counterbore for
//  them. A countersunk head in a counterbore does not seat: it lands on the
//  shoulder edge instead of on a cone. ISO 10642 M2.5. See DATUMS.md C-12.
board_cs_head_d     = 5.0;    // [STANDARD] ISO 10642 M2.5 head diameter
board_cs_head_h     = 1.50;   // [STANDARD] ISO 10642 M2.5 head depth, 90 deg
//  The stock screw is 4.90 mm overall. Countersunk heads sink flush, so through
//  a 3.20 mm plate that leaves only 1.70 mm biting the standoff - 0.68 x
//  diameter, where 1 x diameter is the usual minimum. It will hold a light
//  part; it is not what should carry the board plus an 18650 through a drop.
board_screw_len_stock = 4.90;  // [MEASURED] the screws supplied with the board
board_screw_len_spec  = 8.00;  // [DESIGN] M2.5 x 8 countersunk, the BOM part
board_standoff_d    = 5.50;   // [VENDOR] SMTSO body OD
board_standoff_h    = 7.00;   // [VENDOR] SMTSO height

// --- display ---------------------------------------------------------------
display_module_w  = 91.00;   // [VENDOR] "91.00+/-0.10 TFT"
display_module_h  = 67.60;   // [VENDOR] "67.60+/-0.10 TFT"
display_active_w  = 84.80;   // [VENDOR] "84.80+/-0.10 LCD AA"
display_active_h  = 63.60;   // [VENDOR] "63.60+/-0.10 LCD AA"
//  NOTE: 84.80 x 63.60 gives a 106.00 mm diagonal = 4.173 in, and a square
//  0.2120 mm pixel pitch at 400 x 300. Deriving the active area from a nominal
//  "4.2 inch" instead yields 85.344 x 64.008 - about 0.5 mm too big in each
//  axis. An earlier revision of this file made exactly that mistake.

display_aper_w    = 86.8;    // [MEASURED] reference bezel aperture: a 1.00 mm
display_aper_h    = 65.6;    // [MEASURED] reveal per side round the active area
display_aper_draft = 1.50;   // [MEASURED] the reference bezel's per-side 45 deg
                             //   flare, 1.500 over 1.500 of depth. Read as 1.45
                             //   until measure_reference.py stopped sectioning
                             //   0.05 below the outer face - on a 45 deg draft
                             //   that inset shrinks the opening by exactly the
                             //   same 0.05 per side. See docs/DATUMS.md C-16.

//  The active area is NOT centred on the PCB along U. Its margins are 2.25 mm
//  from the U=0 edge and 5.45 mm from the U=92.50 (USB-C) edge, so its centre
//  sits 1.60 mm toward U=0 from the PCB centre. It IS centred along V.
//  Getting this wrong puts the frame 1.6 mm off the panel on one side.
display_off_x = -1.60;   // [VENDOR] computed from the drawing's AA margins
display_off_y =  0.00;   // [VENDOR] centred in V

// --- the reference's acrylic window: PROVENANCE ONLY, NOT A PART HERE --------
//  Outline taken from the reference's own 2D acrylic template, which is an
//  exact, unambiguous source: a single closed 26-vertex LWPOLYLINE, DXF AC1015,
//  $INSUNITS = 4 (millimetres). It is authored in the board's portrait frame,
//  so its X/Y are transposed relative to this file's device frame.
//
//  THIS DESIGN HAS NO ACRYLIC WINDOW - see docs/DATUMS.md C-09. The numbers are
//  kept because they are a good measurement of somebody else's part and they
//  explain part of why this deck is thinner: the reference's board pocket is
//  13.0 mm (board 10.75 + acrylic 2.0 + 0.25 clearance) and this one is 11.0
//  (board 10.75 + a 0.25 squeeze on a foam gasket). Dropping the acrylic is
//  where 2 mm of that thickness went. Nothing in the model reads them.
window_w = 94.2193;   // [MEASURED] plexiglass.dxf, transposed
window_h = 70.8163;   // [MEASURED] plexiglass.dxf, transposed
window_t = 2.0;       // [VENDOR] nominal cast-acrylic sheet
window_corner_r = 1.294;  // [MEASURED] the DXF corners are polygonal chamfers;
                          //   fitting a circle to each of the four gives
                          //   1.2941, 1.2941, 1.2942 and 1.2942 mm

// --- edge features, all relative to the mounting-pattern centre -------------
//  The mounting-pattern centre coincides with the PCB centre, (U,V) =
//  (46.25, 34.55), so these are simply the feature centres minus that.

// Three side-actuated tact switches (SWITCH-TS24CA) on the V = 69.10 edge.
// The switch body stops 0.19 mm INSIDE the PCB edge, so the enclosure has to
// supply a plunger - it cannot just be an open hole.
button_pitch   = 10.00;  // [VENDOR] centres at U 36.25, 46.25, 56.25
button_count   = 3;      // [VENDOR] PWR, BOOT, KEY
button_body_w  = 4.553;  // [VENDOR] switch body along U
button_body_h  = 2.203;  // [VENDOR] switch body along W
button_aper_w  = 5.4;    // [MEASURED] reference aperture, along X
button_aper_h  = 4.4;    // [MEASURED] reference aperture, along Z
//  THESE WERE THE SPRUE'S BOUNDING BOX, NOT THE CAP. 5.2 is the flange skirt at
//  the rear face and 4.0 is the sprue's axial depth; the cap's own prismatic
//  section is a constant 4.800 x 3.800 swept along the cap axis, concentric
//  with the 5.400 x 4.400 aperture. The old numbers left 0.10 mm per side in X
//  against this project's own sliding fit of fit_slide = 0.30, so the caps
//  would have bound in their apertures. See docs/DATUMS.md C-29.
button_cap_w   = 4.8;    // [MEASURED] reference cap prismatic section
button_cap_h   = 3.8;    // [MEASURED]
//  Past the CAP, not past the aperture. The reference flange is 6.201 x 5.201 =
//  cap + 0.700 per side = aperture + 0.400 per side; 0.4 was the past-APERTURE
//  figure used as a past-cap one, which left 0.200 of overhang against 0.200 of
//  float - a cap that could walk out of its own aperture.
button_flange  = 0.7;    // [MEASURED] per-side retaining flange, past the cap
button_w_centre = -0.70; // [VENDOR] switch centre in W: (-1.802 + 0.402)/2

//  THE BUTTON STACK. Everything here follows from one sentence above: the
//  switch body stops 0.19 mm INSIDE the PCB edge. So a cap alone cannot work -
//  it needs a plunger - and the plunger has to get past the board's edge plane
//  without touching the board.
//
//  Depths behind the top wall's INNER face:
//      0.000   inner face
//      0.500   the PCB's own edge          (board_pocket_h - board_h) / 2
//      0.690   the switch's actuator face  = 0.500 + 0.19
//
//  Half a millimetre is not enough for a retaining flange AND the switch's
//  travel, which is what made the first two attempts fail in opposite
//  directions: a 1.2 mm flange plus a 1.6 mm post was 0.7 mm into the PCB, and
//  cutting back to a bare 0.4 mm flange left the caps 0.34 mm short of ever
//  touching a switch. Neither could be made to work behind the wall, because
//  the space behind the wall belongs to the board.
//
//  So the flange does not live behind the wall. It lives IN it, in a
//  counterbore cut from the inner face, and only the plunger goes past. That
//  buys the travel from the wall's own thickness instead of from the board's
//  clearance. See docs/DATUMS.md C-23.
btn_cb_depth = 1.00;   // [DESIGN] flange counterbore, from the wall's inner face
btn_flange_t = 0.40;   // [DESIGN] flange thickness; travel is the difference
btn_travel   = btn_cb_depth - btn_flange_t;   // [DERIVED] = 0.60 mm of free travel

//  HOW DEEP THE PLUNGER ACTUALLY GOES - and why it is NOT 0.690.
//
//  The board is located by its four M2.5 screws, not by the pocket, and an M2.5
//  screw in an ISO 273 close-fit 2.7 mm hole can sit anywhere in +-0.10 mm. So
//  the actuator is not at a depth, it is in a BAND: 0.590 to 0.790 behind the
//  wall's inner face. A plunger cut to the nominal 0.690 would hold a switch
//  permanently pressed on any build that floated toward the wall - a button
//  that never releases, which is worse than one that never presses.
//
//  So the plunger is cut to the SHALLOW end of that band, less a margin, and
//  the counterbore supplies enough free travel to cross the resulting gap and
//  still work the switch at the far end of the band.
board_screw_d     = 2.50;   // [STANDARD] M2.5 nominal
board_mount_float = (board_screw_clear - board_screw_d) / 2;   // [DERIVED] = 0.10
btn_switch_throw  = 0.25;   // [VENDOR] SWITCH-TS24CA actuation travel
btn_rest_clear    = 0.05;   // [DESIGN] guaranteed gap at rest, worst case
btn_reach = (board_pocket_h - board_h) / 2 + 0.19
            - board_mount_float - btn_rest_clear;   // [DERIVED] = 0.540
//    gap at rest      0.05 .. 0.25    (never zero, so nothing is preloaded)
//    press to actuate 0.30 .. 0.50    (against 0.60 mm of free travel)

//  The plunger's cross-section has to land on the SWITCH and miss the BOARD.
//  Measured from the button axis, the switch body runs +-button_body_h/2 =
//  +-1.1015, and the PCB's back face is at +0.70 (it is -button_w_centre above
//  the axis, since the axis is set button_w_centre below the board's back).
//  The usable band is therefore -1.1015 .. +0.700: 1.8015 mm tall, centred
//  0.2008 below the axis.
btn_band_lo  = -button_body_h / 2;        // [DERIVED] = -1.1015, the switch's edge
btn_band_hi  = -button_w_centre;          // [DERIVED] = +0.7000, the PCB's back face
btn_post_h   = 1.60;   // [DESIGN] inside a 1.8015 band, 0.10 clear top and bottom
btn_post_w   = 3.60;   // [DESIGN] inside the switch body's 4.553 along U
btn_post_dy  = (btn_band_lo + btn_band_hi) / 2;   // [DERIVED] = -0.2008

// Dual microphone array, also on the V = 69.10 edge, at U 13.75 and 78.75.
mic_offset_x = 32.50;  // [VENDOR] +/-32.500 from the PCB centre. Independently
                       //   measured from the reference caseback as exactly
                       //   +/-32.500 - see docs/DATUMS.md D-07.
//  ENLARGED, DELIBERATELY. The reference's 5.4 x 2.5 slot is a 0.4-nozzle
//  feature: at 0.8 the roof of a 2.5 mm hole is a single bridged bead and it
//  printed rough enough to need support, which then had to be dug out of a
//  3.2 mm tunnel. Taller and longer fixes it twice over - the stadium roof
//  becomes a proper self-supporting arch, and there is more open area for the
//  microphone. Still one clean slot per mic, not a perforation.
mic_aper_w   = 8.0;    // [DESIGN] was 5.4 (reference, 0.4-nozzle)
mic_aper_h   = 3.2;    // [DESIGN] was 2.5; 4 beads, arch self-supports
mic_w_centre = -0.50;  // [VENDOR] mic body W 0 to -1.00

// 18650 holder, on the PCB back face, running ACROSS the device.
batt_bay_w = 77.80;    // [VENDOR] holder body along U
//  DISPUTED, AND THE CALIPERS WIN. A re-derivation from Waveshare's own STEP
//  says the holder BODY is 21.10 along V (20.65 through the mid-body), and that
//  22.10 is not a holder feature. The owner then measured a real board and
//  confirmed the holder as drawn here. Both readings are kept because they may
//  be measuring different things - body versus footprint including the skirt -
//  and because 22.10 is the conservative value in BOTH roles it plays: it makes
//  the back plate's clearance cut larger, and it makes the component mock
//  larger. A pocket that is 1 mm too generous costs nothing; one that is 1 mm
//  too tight does not close. See docs/DATUMS.md C-18.
batt_bay_h      = 22.10;  // [MEASURED] confirmed on hardware with calipers
batt_bay_h_step = 21.10;  // [VENDOR] holder body along V, from the Creo STEP
batt_off_x =   0.00;   // [VENDOR] centre U 46.25 = PCB centre
batt_off_y = -19.40;   // [VENDOR] centre V 15.15, i.e. 19.40 toward V=0.
                       //   Independently measured from BOTH reference designs
                       //   as exactly -19.40 - see docs/DATUMS.md D-06.

// --- 18650 battery cowl -----------------------------------------------------
//  An 18650 cell is 18.4 mm in diameter (18.6 for protected cells) and this
//  deck is 16.6 mm thick overall, so the cell CANNOT be contained: the holder
//  reaches 8.20 mm past the standoff plane and must project through the back.
//
//  The reference handles this with a separate clip-on cover. Here the cowl is
//  integral to the back plate, which removes a part and two clips that can
//  break, and turns the bulge into a grip ridge that falls under the fingers
//  when the deck is held in two hands. The trade-off is deliberate and is
//  stated in docs/DESIGN.md: swapping the cell means removing four screws.
cell_dia_max     = 18.6;   // [STANDARD] 18650 worst case (protected cells);
                           //   bare flat-tops are 18.4
cell_len_max     = 69.0;   // [STANDARD] protected / button-top worst case
batt_cowl_enable = true;   // [DESIGN]
batt_cowl_wall   = 2.0;    // [MEASURED] reference cover wall thickness
batt_cowl_clear  = 0.5;    // [DESIGN] clearance over the holder
//  Declared here, out of narrative order, because batt_cowl_crown below is
//  derived from the cavity's rise. The reasoning behind both numbers is given
//  at the end of this block, where the rise used to be defined.
batt_cowl_head   = 2.5;    // [DESIGN] clear rise above the holder for the crown
//  ERGONOMIC EXTRA. The cowl is not only a cover: resting on it the deck
//  leans toward the user, which is the posture it is actually used in on a
//  desk, and it is where the hands sit when it is held. One more millimetre
//  deepens that lean and gives the fingers a fuller form to wrap.
batt_cowl_extra  = 1.0;    // [DESIGN] ergonomic rise past what the cell needs
batt_cowl_rise   = (batt_protrusion - back_t) + batt_cowl_clear + batt_cowl_wall
                   + batt_cowl_head + batt_cowl_extra;
//  THE COWL'S PLAN FORM IS NOT FREE, AND IT USED TO BE TREATED AS IF IT WERE.
//  83.0 x 30.0 was taken from the reference's separate clip-on cover, which
//  sits on a blank panel. This cowl does not: it is integral to the back
//  plate, and the back plate already carries the two LOWER M2.5 board screws
//  at (+/-42.75, +0.25) and the expansion-header window whose lower edge is at
//  y = +26.50. At 83.0 x 30.0 with a 3.2 mm foot flare the cowl's footprint AT
//  THE PANEL is 89.4 x 36.4, and it covered BOTH of them:
//
//    lower board screws   1.11 mm of cowl foot lying over the mouth of each
//                         countersink - the screws could not be inserted at
//                         all, and the board was left hanging on the two
//                         upper screws at the end away from the cell
//    expansion window     3.60 mm of its 8.20 mm height buried, 5.84 mm deep
//                         at the worst point; the clear mouth was 4.70 mm
//                         against a 6.60 mm header body
//
//  Both passed every check in the suite, because the suite counted holes and
//  never asked whether anything was lying on top of one. See DATUMS.md C-25.
//
//  The footprint is therefore DERIVED from the holder, not copied, and the
//  foot flare and the base corner are sized so the footprint stays clear of
//  both. The slack that is spent doing it was accidental, not designed: the
//  cavity had 1.95 mm per side over the holder in V where the design's own
//  declared clearance is batt_cowl_clear = 0.50.
batt_cowl_w      = batt_bay_w + 2 * (batt_cowl_clear + batt_cowl_wall);  // = 82.80
batt_cowl_h      = batt_bay_h + 2 * (batt_cowl_clear + batt_cowl_wall);  // = 27.10
batt_cowl_w_ref  = 83.0;   // [MEASURED] reference Battery_cover.stl footprint,
batt_cowl_h_ref  = 27.0;   //   kept for the audit; neither is a part here
//  The cowl is a swelling BLENDED OUT of the back panel, not a box with a cap
//  sitting on it. batt_cowl_foot is the tangent fillet where it meets the
//  panel, which is what removes the base line; batt_cowl_cap is how much the
//  section eases in toward the crown, as a fraction of the minor axis.
//  0.60, not 3.2. `blend` is an outward offset of the WHOLE section at the
//  panel, so it grows the footprint by 2*blend in both axes - 3.2 added 6.4 mm
//  of plan form that nothing accounted for, and it was never a tangent fillet
//  anyway: it decayed over batt_cowl_foot_f = 0.20 of a 10 mm rise, i.e. a
//  3.2 mm flare spread over 2.0 mm of height. The bound is the expansion
//  window: the cowl's footprint top is (batt_cowl_h/2 + blend) above the
//  holder centre and must stay below the window's lower edge.
batt_cowl_foot   = 0.6;    // [DESIGN] foot fillet, blended into the panel
batt_cowl_foot_f = 0.20;   // [DESIGN] fraction of rise the foot fillet takes
batt_cowl_cap    = 0.34;   // [DESIGN] crown easing, fraction of the minor axis
//  DERIVED, and it used to be 0.50, which was wrong by a whole millimetre of
//  depth. The comment already said the sides must stay parallel until they are
//  clear of the holder - but the holder reaches (batt_protrusion - back_t) =
//  5.00 mm below the panel and the cavity's rise is only 8.00, so 0.50 started
//  the crown at 4.00 mm, a millimetre TOO EARLY. Measured on the rendered
//  plate, the cavity closed to 77.592 mm at the holder's deepest plane against
//  a 77.80 mm holder: 0.104 mm of interference per side. A one-dimensional
//  depth check ("the cell reaches z = -5.00, the cowl floor is at -8.00") saw
//  nothing, because the section is not one-dimensional.
batt_cowl_crown  = (batt_protrusion - back_t + batt_cowl_clear + 0.1)
                   / (batt_cowl_rise - batt_cowl_wall);   // = 0.700
batt_cowl_cap_r  = 3.0;    // [DESIGN] retained for the cavity's ridge()
//  12.6, not 6.0, and the reason is the two lower board screws. With the
//  footprint derived above the cowl still reaches x = 41.61 at y = +0.25 on a
//  R6 corner, which is 1.36 mm inside the Oe5.0 countersink there. A fuller
//  corner is what pulls the cowl's lower flank back off them: at 12.6 the
//  footprint reaches x = 39.36, clearing the countersink rim by 0.89 mm.
//  Asserted from the rendered plate, not from this arithmetic.
//  AND THAT FIX WAS ITSELF BLOCKING. Opening the corner to 12.6 pulled the
//  cowl's flank off the screws and, at the same time, pulled the OUTER surface
//  in at 45 degrees while the cavity's near-square R2.2 corner stayed put. The
//  wall between them collapsed from 2.07 mm on the flats to 0.0385 mm at the
//  corner - an open slit into the battery cavity, about 0.6 mm of arc by
//  3.8 mm tall, at all four corners. Every assert passed: all four were
//  one-dimensional or measured the cavity against the HOLDER, and none of them
//  measured the outer surface against the cavity. See docs/DATUMS.md C-28.
//
//  There is no value that fixes both. The holder's square corner is at
//  (38.90, 11.05) and the screw axis at (42.75, 11.65) - 3.896 mm apart, less
//  the Ø5.0 head radius leaves 1.396 mm for clearance AND wall, against the
//  2.50 the design asks for. So the two constraints are decoupled instead: the
//  corner goes back to 6.0 and the screws get their own relief bores.
batt_cowl_base_r  = 6.0;   // [DESIGN] outer plan-view corner radius
cowl_screw_relief_d = board_cs_head_d + 0.2;   // [DESIGN] Ø5.2 driver access
//  CORRECTED RATIONALE. This used to say "the holder's corners are R2.0".
//  They are not: in Waveshare's STEP the holder's plan form is exactly square
//  at every height through the body - 0.0000 mm deviation from its bounding
//  rectangle, 504 of 576 edges are straight lines, and every circle in the
//  part lies on the cell axis. The value is right for a different reason: a
//  cavity corner must stay small enough not to bite into a square-cornered
//  body sitting inside it. The number stands; the reason it was given for did
//  not. See docs/DATUMS.md C-21.
//  2.2, down from 3.0. The cavity is now 78.80 x 23.10 rather than
//  79.00 x 26.00, so its corner sits much closer to the holder's SQUARE
//  corner at (38.90, 11.05). At n = 3.2 a corner size of 3.0 puts that point
//  0.98 outside the cavity outline; 2.2 leaves it 0.88 inside. This is the
//  board_pocket_r trap, and shrinking the cavity re-opened it.
batt_cowl_base_ri = 2.2;   // [DESIGN] INNER plan-view corner radius. Same trap
                           //   as board_pocket_r: the holder's corners are
                           //   R2.0, so a generously rounded cavity corner
                           //   leaves material exactly where they want to be.
                           //   At R8 the holder fouled all four corners by
                           //   1.39 mm.
batt_cowl_cap_ri = 1.5;    // [DESIGN] INNER cap radius. Kept small on purpose:
                           //   the cavity has to stay full width all the way
                           //   down to the holder's deepest point, and every
                           //   millimetre of inner cap radius eats into that.
                           //   A cowl that tapers over its whole rise pinches
                           //   the cell - see lib/util.scad ridge().

//  Rise is DERIVED, not copied. The reference's cover stands 12.0 mm proud, but
//  it sits on an enclosure whose pocket still contains Waveshare's 2.75 mm
//  stand base. With the base discarded the holder reaches batt_protrusion past
//  the standoff plane, of which back_t is already inside the plate, so only the
//  remainder needs covering - about 7.5 mm rather than 12.
//  rise = how far the holder sticks out past the plate, plus clearance, plus
//  the wall, plus the inner cap radius - because the cavity must still be at
//  full width when it reaches the holder, and the cap is where it stops being.
//  ... plus headroom for the crown itself, which the earlier straight-sided
//  ridge did not need. Without it the crown starts inside the cell's envelope.
//  batt_cowl_head and batt_cowl_rise are declared ABOVE, with batt_cowl_clear,
//  because batt_cowl_crown is derived from the cavity's rise and OpenSCAD does
//  not forward-reference: a use before the definition is silently `undef` and
//  the whole expression collapses. The rationale for the two values is here.

// Onboard speaker grille.
//  Waveshare's own grille field is 14.70 (U) x 10.45 (V), and the reference
//  enclosure reproduces the 10.45 with four 1.3 mm slots on a 3.05 pitch. The
//  acoustic opening is kept inside that same field, but restyled as a finer,
//  more numerous set: five slots at 1.2 mm on a 2.3125 pitch span exactly the
//  same 10.45 mm. Finer perforation on a tighter pitch is the Braun grille
//  idiom, and it is the one perforated element on the object.
grille_slot_w  = 14.0;    // [DESIGN] within Waveshare's 14.70 field
//  RESTRUCK FOR THE 0.8 NOZZLE. Five slots at 1.2 on a 2.3125 pitch left
//  1.11 mm webs - 1.4 beads - so the grille printed as a row of ragged bridges.
//  Three slots at two beads each, with 2.825 mm webs, span the same field
//  exactly and are the same Braun idiom with fewer, cleaner teeth.
grille_slot_h  = 1.6;     // [DESIGN] 2 beads at 0.8
grille_pitch   = 4.425;   // [DESIGN] 2 gaps x 4.425 + 1.6 = 10.45 exactly
grille_count   = 3;       // [DESIGN]
grille_field_h = 10.45;   // [VENDOR] Waveshare's grille height, matched exactly
//  PORT MOUTHS. A square-edged slot only accepts a slim cable: a normal USB-C
//  plug's overmould lands on the outside of the shell and holds the plug proud
//  so the contacts never fully seat. Flaring the mouth gives the overmould
//  somewhere to sit. 1.5 per side over 2.0 of depth opens the USB-C mouth to
//  15.5 x 9.5 at the face, which takes an ordinary moulded cable end.
port_mouth   = 1.5;   // [DESIGN] per-side flare at the outer face
port_mouth_d = 2.0;   // [DESIGN] depth of the flare into the flank

grille_off_x   =  0.00;  // [VENDOR] Waveshare's own grille centre is U 46.25
grille_off_y   = 16.00;  // [VENDOR] ... and V 50.55, i.e. +16.00 from centre.
                         //   The four slots above span 3*3.05 + 1.3 = 10.45 mm,
                         //   which is Waveshare's grille height to 0.00 mm.

// 2 x 8, 2.54 mm expansion header, and the access window Waveshare cuts for it.
expansion_win_enable = true;   // [DESIGN] position is now a VENDOR datum
//  Sized from the header BODY, not from Waveshare's own window. Their base
//  plate cuts 21.60 x 5.60, which is NARROWER than the 21.003 x 6.603 insulator
//  - their window exposes the pin field, and the body sits in a wider recess
//  behind it. This plate is only 3.2 mm thick and the header stands 8.603 mm
//  off the PCB back face, i.e. 1.70 mm PROUD of the standoff plane, so the body
//  itself has to pass through.
expansion_win_w = 22.0;   // [DESIGN] body 21.003 + 0.5 per side
expansion_win_h =  7.6;   // [DESIGN] body  6.603 + 0.5 per side
expansion_win_w_waveshare = 21.60;  // [VENDOR] kept for audit
expansion_win_h_waveshare =  5.60;  // [VENDOR] kept for audit
expansion_win_x = -0.10;  // [VENDOR] header centre U 46.150
expansion_win_y = -0.70;  // [VENDOR] header centre V 33.850
expansion_pitch = 2.54;   // [VENDOR]
expansion_rows  = 2;      // [VENDOR]
expansion_cols  = 8;      // [VENDOR]
//  8.700, not the part's own 8.603 bounding-box height: Waveshare's assembly
//  seats it insulator 8.500 + 0.100 lead + 0.100 seating below the PCB back
//  plane. The bbox of the component in isolation is not where the assembly
//  puts it. See docs/DATUMS.md C-21.
expansion_body_h = 8.700; // [VENDOR] far face of the 2x8 header as ASSEMBLED,
                          //   below the PCB back plane
expansion_card_w = 76.2;    // [VENDOR] SolarLink reference card
expansion_card_h = 48.26;   // [VENDOR] SolarLink reference card

// --- ports on the U = 92.50 edge (device right wall) -----------------------
//  Both are mid-mount: the connector body straddles a cut-out in the PCB, so
//  each sits partly behind the PCB back face. Positions are VENDOR; the opening
//  sizes are opened up from the receptacle to clear a cable overmould / finger.
usbc_off_y   =  0.00;   // [VENDOR] shell centred on V 34.535 = PCB centre
usbc_body_w  =  9.582;  // [VENDOR] shell incl. mounting tabs
usbc_body_h  =  4.163;  // [VENDOR] W -3.251 to +0.912
usbc_w_centre = -1.170; // [VENDOR] centre in W
usbc_open_w  = 12.5;    // [DESIGN] clears a moulded USB-C cable boot
usbc_open_h  =  6.5;    // [DESIGN]

tf_off_y     = 19.15;   // [VENDOR] socket centre V 53.70
tf_body_w    = 16.103;  // [VENDOR]
tf_body_h    =  2.452;  // [VENDOR] W -1.851 to +0.601
tf_w_centre  = -0.625;  // [VENDOR] centre in W
tf_open_w    = 14.0;    // [DESIGN] a microSD card is 11.0 mm wide, so this is
                        //   the card plus 1.5 mm each side for a fingernail.
                        //   It is NOT free to enlarge: the tunnel crosses the
                        //   same flank strip the upper fastener boss needs, and
                        //   cyberdeck.scad asserts the two do not meet.
tf_open_h    =  4.5;    // [DESIGN]
//  The socket mouth stops 1.03 mm inside the U=92.50 edge, so the card must be
//  pushed in past the wall - do not make this opening a tight slot.

// ===========================================================================
// 2. Rii 518BT MINI BLUETOOTH KEYBOARD  -  TARGET COMPONENT B
// ===========================================================================
//
//  IMPORTANT MODELLING NOTE
//  ------------------------
//  An enclosure needs the POCKET, not the keyboard's nominal outline. The
//  pocket is measured directly from two independent reference designs, so it
//  is a stronger datum than any retailer's stated product size (retail
//  listings for this keyboard frequently quote the BOX, not the keyboard).
//  The nominal outline below is therefore derived FROM the pocket, not the
//  other way round, and is used only for the fit-check mock-up.

//  PRIMARY SOURCE: the manufacturer's own user manual, filed as the User Manual
//  exhibit (attachment 1470433) of FCC ID YIZRT-RII518, page 13, section
//  "6. Technical Specifications":
//
//      Dimension: 108.5mm x 58.2mm x 10.2mm
//      Weight: 75g
//
//  That closes the +/-2.54 mm rounding band this project carried for a long
//  time. 108.5 x 58.2 x 10.2 is 4.272 x 2.291 x 0.402 in, every axis of which
//  rounds correctly to riitek.com's published "4.3*2.3*0.4in" - so the vendor's
//  inch figure is a rounded restatement of this same value, not a second
//  independent source.
//
//  Why it took so long to find: the manual's text is converted to vector
//  outlines (the PDF title contains "转曲", Chinese for "convert to curves").
//  pdftotext returns 2 139 characters from the 14-page file - only the FCC
//  boilerplate pages have a text layer. The specification table is invisible to
//  any full-text search and appears only when the page is rasterised. Grepping
//  these exhibits gives the confident, wrong answer that they carry no
//  dimensions.
kbd_body_w = 108.5;   // [VENDOR] FCC ID YIZRT-RII518, User Manual exhibit p13
kbd_body_h =  58.2;   // [VENDOR] as above
kbd_body_t =  10.2;   // [VENDOR] as above, overall, including keycaps

//  CORNER RADIUS. Measured - and then re-measured, because the first
//  measurement was wrong in a way worth recording. See docs/DATUMS.md C-11.
//
//  A plain least-squares circle fitted over a window that also contains the
//  straight edges INFLATES with the window size, because it is no longer
//  fitting an arc. On the replica STL the same corner reads
//
//      window +/- 4 mm   r = 7.00   rms 0.003     <- the arc
//      window +/- 8 mm   r = 6.99   rms 0.004
//      window +/-15 mm   r = 8.78   rms 0.227     <- eating the flats
//
//  and on the manufacturer's drawing 5.87 (rms 0.034) at +/-5 mm against 12.46
//  (rms 0.812) at +/-20. The earlier "10.0 nominal, 9.5-11.2" came from a
//  +/-14 to +/-16 mm window and was an artefact of the method, not a property
//  of the keyboard. The rms column is the tell and it was not being read.
//
//  Measured arc-only, four sources agree on roughly 6 to 7 mm:
//
//    manufacturer drawing, rear view, arc only          5.87 - 5.96
//    third-party CAD replica STL, arc only              6.99  (tangency 6.81/6.88)
//    a Shapr3D STEP carrying authored CIRCLE entities   6.5 exactly
//    reference ATA tray R6.100 requires a body of      >= 4.66
//
//  Neither silhouette is a dimensioned callout, so this is [MEASURED] with a
//  real band rather than [VENDOR]. The band matters in BOTH directions and they are not
//  the same direction for every check:
//
//    pocket clearance  a SQUARER body (small r) is the worst case - its corner
//                      reaches furthest into the pocket's own rounded corner
//    lip capture       a ROUNDER body (large r) is the worst case - its corner
//                      retreats furthest from the aperture it has to cover
//
//  So there is no single conservative value, and anything that checks a corner
//  must say which end of the band it is using.
//  AND IT IS NOT A CONSTANT RADIUS. A sub-pixel trace of the drawing's rear
//  view gives a circle-fit radius that depends on how much of the corner you
//  include - 5.87 over a +/-5 mm span, 7.30 over +/-11 - with an rms of
//  0.16 mm either way. A true arc does not behave like that. The four corners
//  agree with each other to 0.08 mm, so the moulding is uniform corner to
//  corner; what it is not is circular.
//
//  That drawing is a marketing RENDER, not an orthographic projection, so its
//  silhouette includes the edge roll and its own shading and cannot separate
//  "genuinely non-circular" from "rendering artefact". The dimension callouts
//  on it are authoritative; the outline is not. The two AUTHORED CAD sources
//  are what set the band: a Shapr3D STEP carrying CIRCLE entities of exactly
//  6.5, and the replica's exact tangency at 7.000.
//
//  The design does not depend on resolving it. The aperture corner derives
//  from the top of the band, and the retaining lip holds a full 1.00 mm for
//  any real corner from 4.0 to 7.0, 0.98 at 7.3, and only degrades past 8.0.
//  See docs/DATUMS.md C-20.
kbd_body_corner_r     = 6.5;   // [MEASURED] authored CIRCLE value, mid-band
kbd_body_corner_r_min = 5.9;   // [MEASURED] worst case for pocket clearance
kbd_body_corner_r_max = 7.0;   // [MEASURED] worst case for lip capture
kbd_mass_g =  75.0;   // [VENDOR] as-certified 2011 sample

//  CORRECTION - THERE IS NO SECOND CANDIDATE BODY. See docs/DATUMS.md C-07.
//
//  A previous revision of this file carried 109.22 x 58.42 x 10.16 as a rival
//  "retail" body, reasoning that riitek.com's 4.3 x 2.3 x 0.4 in might describe
//  a later, larger revision, and sized the fit mocks to the larger of the two.
//  That body does not exist. The manufacturer's own product drawing prints BOTH
//  units on every axis, in one string per dimension:
//
//      108.5mm/4.3inch     58.2mm/2.3inch     10.2mm/0.4inch
//
//  4.3 in is 109.22 mm, not 108.5, so the millimetre figure cannot be a
//  conversion of the inch figure - the inch figure must be the conversion of
//  the millimetre one. 109.22 was this project's own round trip through 0.1 in
//  granularity, and nothing else.
//
//  A previous version of this paragraph went further and claimed 109.22 was
//  "physically excluded" because the reference ATA tray's pocket is 109.200.
//  That does not follow: 0.02 mm is inside injection tolerance, and a pocket
//  is not a hard gauge. The unit-conversion argument above stands on its own
//  and does not need it. Withdrawn - see docs/DATUMS.md C-17.
//
//  The same drawing settles two other things this file had wrong. It is the
//  CURRENT product page, and it shows a MINI-USB charging port, so the
//  "revised to USB-C" claim was false. And the 2011 FCC manual and the 2023
//  drawing give identical dimensions twelve years apart, so whatever changed
//  between the 75 g and 64.8 g samples, the outline did not.
//
//  What remains is ordinary moulding tolerance on a ~110 mm shell. Neither
//  document states one, so it is assumed here and tagged as an assumption.
kbd_mould_tol  = 0.30;   // [DESIGN] assumed injection tolerance, L and W
//  NOTE on the 0.40: an earlier version justified it with "a third-party CAD
//  replica measures 10.600 overall". That replica is a featureless envelope -
//  its entire key face is one flat plateau - so it is a print-clearance model,
//  not a measurement of the keyboard, and it cannot corroborate a keycap
//  height. The value is kept as what it always really was: an assumption
//  covering an ambiguity the drawing does not resolve. See DATUMS.md C-19.
kbd_keycap_tol = 0.40;   // [DESIGN] thickness only, and ONE-SIDED: the drawing
                         //   does not say whether 10.2 is to the moulding's top
                         //   face or to the keycap crowns. A third-party CAD
                         //   replica of this keyboard measures 10.600 overall,
                         //   which is the upper end of that ambiguity.

//  The pocket is stated, not derived from the body plus a clearance, so that
//  the envelope does not move every time a tolerance assumption is revisited.
//  What it must satisfy is asserted below instead.
//
//    against the drawing body   108.5 x 58.2 x 10.2     0.85 / 0.60 per side
//    against body + tolerance   108.8 x 58.5 x 10.6     0.70 / 0.45 per side
//
//  It also clears all three measured third-party pockets, the widest of which
//  (the reference ATA tray, 109.200 x 59.200) belongs to a built, working
//  device and is therefore a hard physical ceiling on any real unit.
//  TIGHTENED to the floor both asserts allow, and then RIBBED. At 110.2 x 59.4
//  the pocket gave a min-tolerance body 1.00 mm of travel in X, and the front
//  lip is only 0.85 mm per side on that body: slid hard over, the aperture
//  edge cleared the keyboard by 0.05 mm on the long side and went 0.36 mm
//  NEGATIVE at a corner - a visible sliver into the pocket. Nothing bonds the
//  keyboard, so that is where it actually sits, not a worst case on paper.
//  See docs/DATUMS.md C-24.
kbd_pocket_w = 109.85;  // [DESIGN] floor is kbd_pocket_w_ata + 0.6 = 109.80
kbd_pocket_h =  59.15;  // [DESIGN] floor is kbd_body_h_max  + 0.6 =  59.10
kbd_depth    =  11.0;   // [DESIGN]

//  WORST-CASE BODY, for fit checking. The mock-ups in lib/components.scad are
//  built from these rather than from the nominal, so a clearance check is never
//  run against the smallest plausible part.
kbd_body_w_max = kbd_body_w + kbd_mould_tol;    // [DERIVED] = 108.80
kbd_body_h_max = kbd_body_h + kbd_mould_tol;    // [DERIVED] =  58.50
kbd_body_t_max = kbd_body_t + kbd_keycap_tol;   // [DERIVED] =  10.60

//  FCC ID YIZRT-RII518 (Shenzhen Riitek), granted 2011-05-23, covers this body
//  under ten retail model names including "Rii518" and "Rii mini 518BT". The
//  FCC external and internal photos show a 68-key field with NO pointing
//  device of any kind.
//
//  TRAPS, all confirmed from primary sources and all avoided here:
//    * 150 x 100 x 20 mm / 120 g is the RETAIL BOX. It is 38% longer and 72%
//      wider than the certified body and cannot be the product.
//    * The Rii K18 is NOT this keyboard, and neither is the RT518 / RT518S:
//      the RT518 manual gives 317.2 x 123.6 x 18.3 mm, 342 g - three times the
//      length, with a touchpad.
//    * "4.09 x 2.28 x 0.43 in" circulates on review sites. Traced to a single
//      article that cites no source, contradicts its own specification table
//      two paragraphs later, and gives a length of 103.9 mm - physically
//      impossible, since both measured reference pockets are larger than that.
//    * riimall.com's own storefront copy claims an integrated touchpad. The
//      FCC photographs show there is none. Marketing copy for this model is
//      unreliable even from the brand's own shop.

kbd_pocket_corner_r = 6.0;   // [DESIGN] bay corner radius

//  LOCATING RIBS. Half-round fins standing off the pocket walls, sized so the
//  LARGEST credible body still enters without interference and the SMALLEST is
//  still held central. They are what makes the front lip a guarantee rather
//  than an average, and they stop a free-floating keyboard rattling in a
//  handheld. Each is only 2*r wide, so it takes almost no wall length and
//  clears the service window and the pocket corners easily.
//
//    per-side gap, X:  body_max 0.525   nominal 0.675   body_min 0.825
//    per-side gap, Y:  body_max 0.325   nominal 0.475   body_min 0.625
//
//  Rib heights are set just under the body_max gap, so worst case they kiss;
//  a half-round in PLA deflects or shaves that last few hundredths.
kbd_rib_r_x   = 0.45;   // [DESIGN] < 0.525, the body_max gap in X
kbd_rib_r_y   = 0.25;   // [DESIGN] < 0.325, the body_max gap in Y
kbd_rib_lead  = 1.6;    // [DESIGN] taper at the entry end, keyboard loads from behind
//  Rib stations, relative to the keyboard-bay centre. The X pair straddles the
//  service window, which spans about -12.9 .. +21.1 on the left wall; the Y
//  pair sits well inboard of the corner blends.
kbd_rib_dy    = [-18.0, 22.5];   // [DESIGN] on the long (+-X) walls
kbd_rib_dx    = [-35.0, 35.0];   // [DESIGN] on the short (+-Y) walls

// Cross-checks, kept for audit. Both measured; neither is used. Clearances are
// restated against the real 108.5 x 58.2 body.
kbd_pocket_w_ata = 109.200;  // [MEASURED] +0.70 mm clearance
kbd_pocket_h_ata =  59.200;  // [MEASURED] +1.00 mm
kbd_pocket_w_poc = 110.498;  // [MEASURED] +2.00 mm
kbd_pocket_h_poc =  60.600;  // [MEASURED] +2.40 mm

// Front-face retention aperture. Captures the keyboard's outer lip so it
// cannot fall forward; it is pushed out from behind instead.
kbd_aper_w = 106.5;   // [MEASURED] ref. 106.502
kbd_aper_h = 55.8;    // [MEASURED] ref.  55.802
kbd_aper_draft = 0.6; // [MEASURED] per-side flare through the front panel. Was
                      //   a bare literal in cyberdeck.scad; the cover's
                      //   register platform has to taper at the same rate, and
                      //   a number that lives in one file cannot cascade.

// --- keyboard service access ------------------------------------------------
//  The power slide switch and the charging port are BOTH on one short edge of
//  the keyboard. Without a window in the side wall the deck cannot be switched
//  on or charged. Both reference designs cut one; this design cuts one on each
//  side, so the keyboard can be installed either way round.
//
//  The window below is the INTERSECTION of the two reference notches
//  (ATA 39.75 x 11.40 full-depth, PoC 32.25 x 7.55 starting 1.45 above the
//  floor), which is the envelope that both proven designs agree covers the
//  switch and the port.
//  FALSE. The keyboard's charge port and slide switch share ONE short edge -
//  the left as the device is used - and the keys only read one way up, so it
//  cannot be fitted the other way round. A second window on the right flank
//  reached nothing and was a hole for the sake of symmetry.
kbd_access_both_sides = false;  // [DESIGN] left flank only
//  RETAGGED [MEASURED] -> [DESIGN]. Neither of these is a measured value: the
//  window is an outward-rounded ENVELOPE that CONTAINS the reference notch
//  (32.074 x 7.596), not a reading of it. Calling a chosen envelope a
//  measurement is exactly the conflation this file keeps correcting.
kbd_access_w          = 34.0;   // [DESIGN] along the keyboard's short axis
kbd_access_h          = 8.0;    // [DESIGN] along the deck's thickness
//  8.5 from the pocket edge = 8.0 from the keyboard's own top edge, since the
//  keyboard sits 0.5 down in a 59.4 pocket. The window then spans 8.0..42.0
//  from the keyboard top, against features measured on the real unit at 10.4
//  (slide switch) and 29.6..38.6 (USB-C): 2.4 mm of margin above, 3.4 below.
kbd_access_from_edge  = 8.5;    // [MEASURED] near edge of the window, measured
                                //   from the keyboard's display-side long edge
kbd_access_above_floor = 1.4;   // [MEASURED] above the keyboard's bottom face

// Service access: a finger cut-out in the back plate to push the keyboard out.
kbd_eject_d      = 19.0;   // [MEASURED] ref. dia 18.9, tapering to 13.0
kbd_eject_d_min  = 13.0;   // [MEASURED]
kbd_eject_off_x  = 33.5;   // [MEASURED] from the keyboard-bay centre
kbd_eject_off_y  = 0.0;    // [DESIGN] centred on the bay's short axis


// ===========================================================================
// 3. FASTENERS AND FITS
// ===========================================================================

//  CHASSIS <-> BACK PLATE FASTENERS ARE M2, not M3. Changed on the owner's
//  instruction after handling the parts. It is also the better fit for the
//  space: the flank strip either side of the board pocket is 8.35 mm and an M3
//  boss at 7.4 mm very nearly filled it, where an M2 boss at 6.6 leaves room.
//  The trade is real and is recorded rather than buried - four M2 screws in
//  brass inserts are ample to close a 110 g shell, but they are a weaker
//  accessory anchor than four M3 were, so DESIGN.md no longer advertises them
//  as general-purpose rigging points. See docs/DATUMS.md C-13.
shell_screw_clear        = 2.4;   // [STANDARD] ISO 273 close fit for M2
shell_screw_head_d       = 3.8;   // [STANDARD] ISO 4762 M2 socket-cap head diameter
shell_screw_head_h       = 2.0;   // [STANDARD] ISO 4762 M2 head height
//  The back plate uses COUNTERSUNK screws, not socket caps. An ISO 4762 cap
//  head is 3.0 mm tall, which is the entire back-plate thickness: counterboring
//  for it leaves no material under the head to take the preload. An ISO 10642
//  countersunk head is 1.86 mm deep and bears on a cone, leaving 1.34 mm of
//  plate and spreading the load instead of concentrating it on a thin annulus.
shell_screw_cs_head_d    = 4.0;   // [STANDARD] ISO 10642 M2 head diameter
shell_screw_cs_head_h    = 1.20;  // [STANDARD] ISO 10642 M2 head depth, 90 deg
shell_screw_insert_bore  = 3.2;   // [STANDARD] bore for a standard M2 brass heat-set insert
                         //   recommendation for a 4.0 mm OD brass heat-set
                         //   insert in PLA/PETG
shell_screw_insert_len   = 4.0;   // [STANDARD] common M2 short insert
shell_screw_boss_wall    = 1.7;   // [DESIGN] material around an insert. 1.6 mm is the
                         //   usual minimum to stop the boss splitting as the
                         //   insert is driven; 1.7 is used because the flank
                         //   strip the bosses live in is shared with the port
                         //   tunnels and every 0.1 mm of boss diameter comes
                         //   straight off that clearance. Each boss also merges
                         //   into the side wall, so its outboard side is much
                         //   thicker than this figure suggests.
shell_screw_boss_d       = shell_screw_insert_bore + 2 * shell_screw_boss_wall;   // = 8.0
shell_screw_bore_depth   = 6.5;   // [DESIGN] insert length plus clearance for swarf
                         //   ~3 mm of screw-tip relief

// Print fits.
fit_slide  = 0.30;   // [DESIGN] per side, part that must slide in and out
fit_press  = 0.10;   // [DESIGN] per side, interference fit
fit_free   = 0.50;   // [DESIGN] per side, generous, for cables and ports


// ===========================================================================
// 4. DERIVED ENCLOSURE ENVELOPE
// ===========================================================================
//  Nothing below is a free choice - it all falls out of the component datums
//  and the structural preset. This is the whole point of the file: the
//  enclosure is as small as the two components allow, and no smaller.

body_w = wall + max(kbd_pocket_w, board_pocket_w) + wall;
//  THE BOTTOM WALL IS THICKER THAN THE REST, AND HAS TO BE. It carries the
//  back plate's tongue groove, and the shell's edge roll withdraws the outer
//  face by up to 0.94 mm exactly where that groove sits. At wall = 3.2 the
//  material left outboard of a 1.3 mm groove measured 0.96 mm on the printed
//  part - 1.2 beads at 0.8. No groove depth fixes that and no roll setting
//  fixes it without flattening the edge everywhere: the wall itself is the
//  variable. See docs/DATUMS.md C-33.
bottom_wall = wall + 1.2;   // [DESIGN] = 4.40, the groove's wall
body_h = bottom_wall + kbd_pocket_h + spine + board_pocket_h + wall;
body_t = back_t + board_depth + front_t;

// Bay centres in the device frame.
board_bay_cy =  body_h/2 - wall - board_pocket_h/2;
kbd_bay_cy   = -body_h/2 + bottom_wall + kbd_pocket_h/2;

// The board's mount-pattern centre coincides with the board bay centre.
board_cx = 0;
board_cy = board_bay_cy;

// Z planes.
z_back_outer  = 0;
z_back_inner  = back_t;
z_front_inner = body_t - front_t;
z_front_outer = body_t;

// ---------------------------------------------------------------------------
// FORM LANGUAGE
// ---------------------------------------------------------------------------
//  The brief is Rams, read organically: straight edges and a single radius
//  system, but with curvature continuity everywhere a surface turns.
//
//  A conventional fillet is an arc tangent to a line - curvature jumps from
//  1/r to zero at the join, and the eye reads that as a hard corner however
//  large r is. Every visible corner here is instead a SUPERELLIPTICAL quadrant
//  of a larger corner size, which tracks the same visual line while ramping
//  curvature in from zero. See lib/util.scad.
form_n        = 3.2;    // [DESIGN] superelliptical exponent. 2.0 would be a
                        //   plain arc; 3.2 with corner_blend 11.2 tracks an R8
                        //   arc to within 0.2 mm while being curvature-
                        //   continuous. Above ~4 it reads as a square corner
                        //   with a long approach, which is Braun but not
                        //   biophilic.
corner_blend  = 11.2;   // [DESIGN] corner SIZE, not a radius: how far along
                        //   each edge the corner curve runs.

//  Edges roll into the faces instead of meeting them at a chamfer. The widest
//  section stays exactly body_w x body_h, so this costs nothing in envelope -
//  the softening is taken out of the faces inward, never added outward - and
//  the middle (1 - 2*edge_roll) of the thickness stays at full wall.
edge_soft     = 1.2;    // [DESIGN] face inset from the widest section. Bounded
                        //   by the wall: the shell is thinnest at the arris,
                        //   where it measures wall - edge_soft. At 1.2 that
                        //   leaves 2.0 mm (5 extrusions) of rim around the back
                        //   opening, which is the real constraint - the front
                        //   arris is carried by the 2.4 mm face, not the wall.
edge_roll     = 0.28;   // [DESIGN] fraction of thickness the roll occupies

//  The internal cavity is deliberately NOT given the organic treatment. A
//  continuous corner is fuller than an arc near the tangent points and tighter
//  at the corner itself; applied to the back opening it undercuts the keyboard
//  bay's own corners by ~0.15 mm and traps the keyboard. The cavity is hidden,
//  so it stays a plain arc, and the wall simply runs thicker at the corners
//  (3.7 mm against 3.2 mm nominal) - which is where a shell wants material.
//  The cavity DOES get the continuous corner, but the size is not free: it is
//  bounded above by the keyboard bay, which is full interior width and runs to
//  the bottom of the shell, so its own corners sit in the cavity's corners.
//  Measured by intersecting the two outlines:
//
//    plain arc  R6.0   0.000 mm^2 of the bay unreachable
//    continuous 6.0    0.000     <- used here, the largest that works
//    continuous 8.0    0.223
//    continuous 9.8    4.768
//    continuous 11.2  10.958     (= the shell's own corner blend)
//
//  So the cavity cannot simply inherit corner_blend. tools/validate.py asserts
//  reachability against the rendered mesh, not against this comment.
cavity_blend  = 6.0;    // [DESIGN] back-opening corner, continuous
corner_gusset = 6.0;    // [DESIGN] solid corner fillet leg length, inside

//  Aperture corner sizes. Not free: see the asserts in section 6.
aper_blend_display = 4.2;  // [DESIGN] PANEL-BOUND, not chosen. The aperture
                           //   height is trapped between the active area it
                           //   must not clip (63.60) and the module edge it
                           //   must still bear on (67.60), which leaves 1.0 mm
                           //   per side. A superelliptical corner eats
                           //   blend * 0.1947 of that at 45 deg, so anything
                           //   above ~4.6 clips the panel corner. This is the
                           //   one place on the object where the corner is
                           //   smaller than the form language wants, and the
                           //   reason is the component, not taste. The outward
                           //   flare opens it to an effective 5.65 at the face,
                           //   which is what the eye actually sees.
//  THE KEYBOARD APERTURE CORNER IS CIRCULAR, NOT SUPERELLIPTICAL, AND IT IS
//  DERIVED RATHER THAN CHOSEN. This is the second place the form language is
//  deliberately broken, and like the first (the display aperture) the component
//  sets it, not taste.
//
//  A lip of constant width around an aperture is the component's outline eroded
//  by the lip width. Eroding a circular corner of radius r by d gives a circular
//  corner of radius r - d. A superellipse is not the erosion of a circle: at
//  n = 3.2 it sits up to 1.55 mm PROUD of the circular arc of the same corner
//  size, biting toward the box corner exactly where the keyboard's own corner is
//  retreating away from it. Those two effects add, and at the measured corner
//  radius they cancelled the lip entirely - it went NEGATIVE, meaning the
//  keyboard no longer covered its own aperture and you could see into the
//  pocket past all four corners.
//
//  Derived from the LARGEST plausible keyboard corner, because that is the
//  worst case for capture, minus the narrower of the two lip widths.
kbd_lip_x = (kbd_body_w - kbd_aper_w) / 2;   // [DERIVED] = 1.00
kbd_lip_y = (kbd_body_h - kbd_aper_h) / 2;   // [DERIVED] = 1.20
aper_blend_kbd  = kbd_body_corner_r_max - min(kbd_lip_x, kbd_lip_y);  // = 6.00
aper_n_kbd      = 2.0;    // [DERIVED] circular - see above

//  Back-plate outer perimeter. A small roll turns the panel seam into a
//  deliberate shadow gap rather than a tolerance gap.
plate_edge_soft = 0.6;  // [DESIGN]
plate_edge_roll = 0.45; // [DESIGN]

//  Control cluster. The three buttons sit in ONE shallow recess rather than in
//  three separate holes, so they read as a single considered element.
dish_enable = true;    // [DESIGN]
dish_margin = 3.2;     // [DESIGN] recess margin around the aperture group
dish_depth  = 0.9;     // [DESIGN] leaves 2.3 mm of the 3.2 mm top wall
dish_blend  = 5.0;     // [DESIGN] recess corner size
dish_flare  = 0.5;     // [DESIGN] per-side flare at the outer face


// ===========================================================================
// 5. PERFORMANCE-RIG FEATURES
// ===========================================================================
//  This deck is intended for live use: stand-mounted or worn, not desk-bound.

//  ACCESSORY MOUNTING
//  ------------------
//  There is deliberately no 1/4"-20 socket and no moulded strap lug. Both were
//  designed, modelled, and then removed, because neither can be fitted without
//  either growing the envelope or cutting into a component bay:
//
//    * A 1/4"-20 insert needs ~8 mm of bore depth. The only material at the
//      deck's centroid is the inter-bay spine, which is 3.2 mm wide - a 10.5 mm
//      bore does not fit in it. Putting the socket anywhere else means a boss
//      protruding from the back face, which stops the deck lying flat.
//    * Strap lugs want the corner voids, but the keyboard is full-width, so the
//      only corner material is the gusset, and a 12 mm webbing slot does not
//      fit inside a 6 mm fillet without breaking into the keyboard bay.
//
//  Instead the four M2 back-plate screws ARE the accessory mounting points.
//  They thread into brass inserts in the chassis, not into plastic, so they
//  are the strongest anchors on the device. Fit M2 x 12 in place of the
//  standard M2 x 6 and sandwich a bracket, strap yoke or stand clamp under the
//  heads. The pattern is given by accessory_pattern() in cyberdeck.scad.
//  THE LENGTHS WERE STALE. They still read 8 and 12 from the M3 revision while
//  ASSEMBLY.md's own arithmetic had settled on M2 x 6 - the plate is 3.20 and
//  the insert 4.00, so 6 engages 2.80 mm and 8 spins its last 0.80 in the
//  relief below the insert. The rig length is bounded from the other side:
//  shell_screw_bore_depth is 6.5, so a 12 mm screw needs at least
//  12 - back_t - shell_screw_bore_depth = 2.30 mm of bracket under its head or
//  it bottoms in the bore before it is tight.
accessory_screw_len_std = 6;    // [DESIGN] normal build
accessory_screw_len_rig = 12;   // [DESIGN] with a bracket under the heads
accessory_bracket_min_t = accessory_screw_len_rig - back_t - shell_screw_bore_depth;  // = 2.30

// Ventilation over the ESP32-S3 module. The RLCD has no backlight, so thermal
// load is low; these are insurance for sustained Wi-Fi TX.


// ===========================================================================
// 6. SELF-CHECK
// ===========================================================================
//  Cheap invariants, asserted at render time. tools/validate.py runs the full
//  set, including the ones that need geometry.

assert(wall >= 4 * nozzle, "wall must be at least 4 extrusions wide");
assert(bottom_wall >= wall, "bottom wall thinner than the rest of the shell");
assert(mic_aper_h >= 4 * nozzle,
       "microphone slot roof is too shallow to arch; it will need support");
assert(grille_pitch - grille_slot_h >= 2 * nozzle,
       "speaker grille webs are under two beads wide");
assert(grille_slot_h >= 2 * nozzle,
       "speaker grille slots are under two beads wide");
assert(shell_screw_boss_d > shell_screw_insert_bore + 2, "insert boss wall too thin");
//  Largest pocket radius that still clears a board corner of radius
//  board_corner_r with c mm of per-side clearance:  r <= (c + (sqrt(2)-1)*
//  (board_corner_r + c)) / (sqrt(2)-1) ... solved numerically below for the
//  actual clearance rather than hard-coded.
pocket_clear   = (board_pocket_w - board_w) / 2;
pocket_r_limit = (board_corner_r + pocket_clear * sqrt(2)) / (sqrt(2) - 1) - board_corner_r / (sqrt(2) - 1) + pocket_clear;
assert(board_pocket_r <= board_fit * sqrt(2) / (sqrt(2) - 1),
       "board pocket corner arc leaves material where the board's SQUARE corner goes");
assert(expansion_win_h > 6.603, "expansion window will not clear the header body");
assert(board_pocket_w >= board_w, "board pocket narrower than the board");
assert(board_pocket_h >= board_h, "board pocket shorter than the board");
//  The pocket must clear BOTH candidate bodies, not just the primary one.
assert(kbd_pocket_w >= kbd_body_w + 0.6 && kbd_pocket_w >= kbd_body_w_max + 0.6,
       "keyboard pocket does not clear both candidate bodies in width");
assert(kbd_pocket_h >= kbd_body_h + 0.6 && kbd_pocket_h >= kbd_body_h_max + 0.6,
       "keyboard pocket does not clear both candidate bodies in height");
assert(kbd_depth >= kbd_body_t + 0.4 && kbd_depth >= kbd_body_t_max + 0.4,
       "keyboard pocket does not clear both candidate bodies in depth");
//  ... and it must clear the upper bound set by a working reference enclosure.
assert(kbd_pocket_w >= kbd_pocket_w_ata + 0.6,
       "keyboard pocket tighter than a tray known to accept a real unit");
// ---- fastener pattern, moved here from cyberdeck.scad ----------------------
//  These were derived in the model file, which meant the magnet stations below
//  could not see them without restating the arithmetic - the exact duplication
//  this file exists to prevent. They are pure derived scalars; the values are
//  unchanged, and tools/check_golden.py proves it.
inner_w           = body_w - 2 * wall;                     // clear interior width
boss_flank        = (inner_w - board_pocket_w) / 2;        // free strip each side
boss_cx           = board_pocket_w/2 + boss_flank/2 + 0.4; // outboard of the bay
//  Half-height of the back opening, which is not centred on the part: the
//  bottom wall is thicker. See backplate() in cyberdeck.scad.
plate_half_h      = (body_h - bottom_wall - wall - 2*fit_slide) / 2;
plate_cy          = (bottom_wall - wall) / 2;
//  The back opening itself, which the plate sits in with fit_slide all round.
//  It is NOT centred on the part either, and cutting it symmetrically about
//  y = 0 ate 1.2 mm of the thicker bottom wall - taking the tongue groove's
//  whole lower lip with it, so the groove opened straight through to the back
//  face and the tongue was captured by nothing.
cavity_h          = body_h - bottom_wall - wall;
cavity_cy         = plate_cy;
plate_edge_margin = 0.6;   // [DESIGN] material left outboard of a countersink

//  THE UPPER ROW IS NOT plate_half_h MINUS A MARGIN, AND WAS BREAKING OUT.
//  plate_half_h is the plate's MID-THICKNESS section. Its outer FACE is rolled
//  in by plate_edge_soft, and - the part that actually bites - boss_cx = 51.24
//  lands inside the face's corner blend, which starts at x = 48.63. There the
//  outline runs diagonally, so the vertical extent is not the clearance. At
//  63.125 the Ø4.0 countersink had 0.022 mm of plate to bite on and broke
//  through the rim at both top corners. See docs/DATUMS.md C-31.
//
//  boss_cx cannot move inboard to escape the blend: the board pocket puts a
//  floor of board_pocket_w/2 + shell_screw_boss_d/2 = 50.05 on it, and the
//  blend starts at 48.63. So the row comes down instead.
//
//  This value is the highest y at which a countersink rim plus
//  plate_edge_margin still fits inside the rendered plate FACE, found by
//  true perpendicular distance - not by a superellipse extent, and not by
//  offsetting the outline, which at n = 3.2 is optimistic by 3.1 mm. It is
//  pinned rather than derived because the honest derivation needs a
//  point-to-segment minimum over rse_poly(), which lives in lib/util.scad and
//  which tools/params.py cannot evaluate. validate.py measures the real
//  clearance on every run, so a change to the plate's shape fails the gate
//  rather than silently moving the screws back over the edge.
//  62.25 is the highest the row can sit and still keep a full countersink rim
//  plus plate_edge_margin inside the plate's face outline, solved against the
//  ANALYTIC face superellipse. It cannot be measured off the rendered plate,
//  because at 63.125 the countersink has already merged into the exterior
//  outline - the mesh you would measure is the defect. (Measuring it that way
//  first gave 58.5, which then collided with the microSD tunnel and looked
//  like an over-constrained design. It is not: the tunnel needs 61.425 and
//  this clears it by 0.825.)
boss_row_hi = 62.25;   // [DERIVED] solved against the plate's face outline

boss_rows = [ board_bay_cy - board_pocket_h/2 + shell_screw_boss_d/2,
              boss_row_hi ];

// ============================================================================
//  9.  VARIANT 2 - THE MAGNETIC FRONT COVER
// ============================================================================
//  v1.0 is a printed, frozen design (tests/golden/v1.json). Everything in this
//  section is ADDITIVE and gated on `variant`, so setting variant = 1 rebuilds
//  v1 byte for byte. tools/check_golden.py proves that on every run; nothing
//  below may change a value v1 already had.
variant = 2;   // [DESIGN] 1 = v1.0 as printed, 2 = adds the magnetic cover

//  ---- THE ONE MEASUREMENT THE COVER TURNS ON --------------------------------
//  The shell's edges roll into its faces, so the FRONT FACE is narrower than
//  body_w. rse_soft() insets each face by edge_soft * roll_f(1, edge_roll), and
//  roll_f lives in lib/util.scad where tools/params.py cannot evaluate it. The
//  coefficient is therefore evaluated once and pinned here, and validate.py
//  asserts the rendered shell still matches it - so if the roll is ever
//  retuned, the assertion fails rather than the magnets quietly moving.
face_roll_1       = 0.984;   // [DERIVED] roll_f(1, edge_roll) at edge_roll = 0.28
front_face_inset  = edge_soft * face_roll_1;        // [DERIVED] = 1.1808
front_face_half_w = body_w / 2 - front_face_inset;  // [DERIVED] = 56.944
front_face_half_h = body_h / 2 - front_face_inset;  // [DERIVED] = 68.944

//  ---- THE MAGNETS -----------------------------------------------------------
//  Ø5 x 2 mm N52 NdFeB discs, Ni-Cu-Ni plated. A genuine catalogue standard
//  (supermagnete S-05-02-N and equivalents), stocked at +-0.10 mm on BOTH
//  diameter and thickness.
//
//  THE NUMBER THAT DECIDES THE DESIGN: force collapses with the gap. A pair of
//  these makes ~6.7 N in contact, 3.85 N through 0.8 mm, 2.15 N through 1.6 mm
//  and 1.30 N through 2.4 mm. Every 0.1 mm of skin removed is worth more than
//  another magnet. So the stack is ASYMMETRIC: a thin 0.8 mm skin on the
//  enclosure's show face, and NO skin at all on the cover's inner face, which
//  nobody ever sees. Total gap 0.8 mm, not 1.6.
//
//  THE OTHER NUMBER: shear is only ~20% of pull (this SKU publishes 1.33 N
//  against 6.67 N) and it comes from surface friction, not from the magnet. A
//  cover shoved into a bag is loaded in shear. So the magnets are NOT the shear
//  path - the two register platforms are. Magnets carry lift-off only.
magnet_d     = 5.00;   // [VENDOR] Ø5 x 2 mm N52 disc
magnet_h     = 2.00;   // [VENDOR]
magnet_tol   = 0.10;   // [VENDOR] +- on both diameter and thickness
magnet_pull_contact = 6.67;  // [VENDOR] N, pair in contact
magnet_pull_08      = 3.85;  // [DERIVED] N, pair across 0.8 mm
magnet_shear_frac   = 0.206; // [VENDOR] 1.33 N of 6.67 N

//  The magnet's own tolerance band (0.20 mm) is WIDER than PLA's usable
//  press-fit window (0.05-0.10 mm diametral), so no single bore diameter grips
//  a whole bag of them. The bore is therefore drawn generous and gripped by
//  crush ribs, which absorb the tolerance by deforming. This is also why the
//  bore is NOT magnet_d - something: that stack is consumed before printing
//  starts.
magnet_bore  = magnet_d + 0.50;    // [DESIGN] = 5.50 drawn. FDM holes print
                                   //   0.1-0.3 undersize, so this lands near
                                   //   5.30 as printed; the ribs, not the bore,
                                   //   set the grip. ONE compensation, here.
magnet_rib_h = 0.35;               // [DESIGN] rib tips close the bore to 4.80,
                                   //   so even the smallest disc in the band
                                   //   (4.90) sees 0.10 mm of diametral
                                   //   interference and the largest (5.10)
                                   //   sees 0.30 - well inside what six ribs
                                   //   absorb by deforming, and nowhere near
                                   //   the hoop strain that cracks a solid bore

//  RIB WIDTH, WHICH WAS NOT FREE AND WAS NOT CHECKED. The rib used to be cut
//  by a cylinder of radius magnet_rib_h centred ON the bore wall, which makes
//  the bump 2 * magnet_rib_h = 0.70 mm wide at its base. Against a 0.80 mm
//  nozzle that is 0.85 of ONE extrusion: the slicer cannot resolve it, so what
//  printed was a smear of whatever width the bead happened to be, and none of
//  the designed 0.10-0.30 mm interference was under the designer's control.
//  See docs/DATUMS.md C-35.
//
//  The height stays. The base widens to two clean extrusions by cutting with a
//  LARGER cylinder pushed further OUTSIDE the bore wall. Radius and offset are
//  solved from the height and the width below, so they cannot drift apart when
//  either moves - which is the whole point of doing it here rather than in the
//  module.
magnet_rib_w = 2 * nozzle;         // [DESIGN] = 1.60, two clean extrusions
magnet_rib_n = 6;                  // [DESIGN] crush ribs around the bore. WAS
                                   //   8, which fails twice at this width: the
                                   //   gaps between ribs fall to 0.56 mm, below
                                   //   one extrusion, so the slicer bridges
                                   //   them into a solid ring with no crush
                                   //   relief left; and the cutters themselves
                                   //   overlap (2.985 mm apart, 3.001 mm of
                                   //   summed radius), which deforms the ribs
                                   //   before they are even sliced. Six leaves
                                   //   1.28 mm gaps and 0.90 mm of cutter
                                   //   clearance.

//  Solving the cutter. Given bore radius R, rib height h and base width w, the
//  bump is the part of the bore left uncut by a circle of radius r centred at
//  distance c from the axis. Its innermost point must sit at R - h, and its
//  base must meet the wall at +-w/2. Those two conditions fix r and c.
magnet_rib_y = magnet_rib_w / 2;                                   // [DERIVED]
magnet_rib_x = sqrt(pow(magnet_bore/2, 2) - pow(magnet_rib_y, 2)); // [DERIVED]
magnet_rib_a = magnet_rib_x - (magnet_bore/2 - magnet_rib_h);      // [DERIVED]
magnet_rib_r = (pow(magnet_rib_a, 2) + pow(magnet_rib_y, 2))
               / (2 * magnet_rib_a);        // [DERIVED] = 1.5004, cutter radius
magnet_rib_c = magnet_rib_r + (magnet_bore/2 - magnet_rib_h);
                                            // [DERIVED] = 3.9004, from the axis
magnet_seat_clear = 0.15;          // [DESIGN] so the disc can bottom on the skin
magnet_pocket_h   = magnet_h + magnet_seat_clear;   // [DERIVED] = 2.15
magnet_skin       = 0.80;          // [DESIGN] 4 layers over the disc, show face
magnet_boss_wall  = 1.60;          // [DESIGN] 4 extrusions around the bore
magnet_boss_d     = magnet_bore + 2 * magnet_boss_wall;   // [DERIVED] = 8.70
//  front_t is 2.4 and the stack needs 2.95, so the pocket grows INWARD as a
//  local boss rather than thinning the show face.
magnet_boss_rise  = max(0, magnet_skin + magnet_pocket_h - front_t);  // = 0.55

//  HOW THE DISC GETS IN. The stations sit in the solid rib between the board
//  pocket and the side wall - 13.65 mm of material from the back-plate seating
//  plane to the show face - so a blind pocket buried in the middle of it is a
//  SEALED VOID. The first build made exactly that: four enclosed cavities, a
//  mesh in five bodies, and no way to fit a magnet.
//
//  So each pocket gets an access shaft down to the back-plate seating plane.
//  The disc drops in from the back, falls to the mouth of the ribbed section
//  and is pushed home with a 4 mm rod; the back plate then covers the shaft and
//  nothing is visible from anywhere. The shaft is clearance, not a fit - the
//  ribs at the top do all the gripping - and it removes ~0.3 cm3 of otherwise
//  dead material from each corner.
magnet_shaft_d = magnet_d + 1.00;   // [DESIGN] = 6.00, free drop

//  ---- WHERE THEY GO, AND WHY THERE IS ONLY ONE ANSWER -----------------------
//  Measured on the rendered front face, the solid bands are:
//      left of the display   10.64 mm        right of the display  13.84 mm
//      between the apertures  5.29 mm        below the keyboard     3.17 mm
//      above the display      2.96 mm
//  A Ø5 pocket with 1.6 mm of surround needs 8.50. Only the two display flanks
//  can take one. Placing magnets anywhere else on this face is not a style
//  choice that was rejected; it does not fit.
//
//  Within a flank the x is bounded on both sides: inboard by the board pocket,
//  outboard by the shell's own skin. The station is the middle of that window.
magnet_x_min = board_pocket_w / 2 + magnet_boss_d / 2;          // clears the bay
magnet_x_max = front_face_half_w - magnet_boss_d / 2;           // keeps its skin
magnet_x     = (magnet_x_min + magnet_x_max) / 2;               // [DERIVED]

//  In y the limit is the fastener bosses, which stand at the same x. Two Ø6.6
//  bosses and a Ø8.50 magnet boss need half of each plus a margin between
//  centres; the stations sit exactly there, which is also as far apart as they
//  can get, and spacing is what resists peel.
magnet_boss_keepout = shell_screw_boss_d / 2 + magnet_boss_d / 2 + 1.0;
magnet_y_lo = boss_rows[0] + magnet_boss_keepout;   // [DERIVED] =  8.575
magnet_y_hi = boss_rows[1] - magnet_boss_keepout;   // [DERIVED] = 53.600

//  ONE list, consumed by the shell AND by the cover. Hard-coding these twice is
//  exactly the surgery-on-every-script failure this section exists to avoid.
function magnet_sites() = [ for (sy = [magnet_y_lo, magnet_y_hi],
                                 sx = [-magnet_x, magnet_x]) [sx, sy] ];

//  POLARITY: uniform, every disc the same way up, because the cover is already
//  keyed by geometry and cannot be fitted rotated. The two register platforms
//  are different sizes and neither the x stations nor the y stations are
//  symmetric about the device centre, so a cover offered up backwards simply
//  does not drop in. Alternating polarity (the iPad Smart Cover's answer) buys
//  nothing here and makes assembly a coin toss on every disc.

//  ---- THE COVER -------------------------------------------------------------
//  Nothing on the front face stands proud: the RLCD glass is 2.65 mm below the
//  outer face and the keycaps 2.80 mm at worst case. So a flat cover touches
//  neither, and the only thickness question is stiffness - a thin cover
//  deflects onto the glass under a thumb. At 3.0 mm a 30 N press stays well
//  clear, and the extra section also removes any need for ribs, which the
//  aperture clearance would cap at ~2 mm anyway.
//  Thickness is DERIVED, not chosen: it is exactly what a buried magnet needs.
//  The pocket opens on the cover's INNER face, so the disc sits against the
//  chassis with nothing between them but the chassis's own 0.8 mm skin - the
//  gap stays 0.8, not 1.6 - and the 0.8 mm left on the far side is the cover's
//  show face. Change magnet_h or magnet_skin and the cover follows.
cover_t         = magnet_pocket_h + magnet_skin;   // [DERIVED] = 2.95
cover_gap       = 0.15;   // [DESIGN] shadow gap per side; flush is a tolerance trap
//  ---- THE COVER IS A SHELL THAT HOOKS THE RIM -----------------------------
//  Version 1 was a flat plate on four magnets. It printed warped and could not
//  clasp - a 116 x 140 mm plate 2.95 mm thick is exactly what curls on an FDM
//  bed, and four magnets cannot pull that curl flat across a 520 mm perimeter.
//
//  Version 2 answered that with six sprung grip fingers. It measured well and
//  it was wrong: discrete snap features are fragile, and they read as cheap
//  consumer plastic. A mechanism had been bolted ONTO the object instead of the
//  object BEING the mechanism.
//
//  This is the third answer and it has no features at all. The shell's flank
//  tapers 1.023 mm per side over the last 3.60 mm before the front face -
//  measured, not assumed - so that rim is already an undercut running the whole
//  way round. A shell with one continuous eased lip hooks under all 460 mm of
//  it at once. Nothing local, nothing sprung, nothing to snap off. It goes on
//  with a single press: the lip rides a 1.5 mm ramp of the shell's own taper,
//  the whole shell breathing a few tenths as it passes, and the magnets pull
//  the last of it home.
//
//  DEPTH IS SET BY THE PORTS, not by preference. The USB-C opening's top edge
//  is at chassis z = 12.78, so a wall deeper than 4.07 mm begins to cover it.
//  3.60 leaves 0.47 mm and still buys a 1.023 mm undercut.
cover_wall_d   = 3.60;   // [DESIGN] wall depth below the face
cover_wall_t   = 2.00;   // [DESIGN] wall thickness
cover_clear    = 0.35;   // [DESIGN] chamber clearance over the shell
cover_hook     = 0.70;   // [DESIGN] of the 1.023 mm the rim offers. See the
                         //   note below on why this is not the engagement.
                         //   the 1.023 mm available. Half, so the ramp stays
                         //   gentle and the shell never has to be forced.
cover_lip_t    = 0.80;   // [DESIGN] straight land on the lip
cover_lip_entry = 0.20;  // [DESIGN] just enough ease to find the rim.
//
//  THE HOOK IS MEASURED AT THE MOUTH, BUT THE LAND SITS ABOVE IT, and the
//  shell has already narrowed by then. A 0.50 hook behind a 0.60 entry
//  chamfer left 0.17 mm of real engagement at the middle of the land - a
//  third of the number it was being quoted as. A warp test found it: the lip
//  let go at 0.20 mm of splay, inside what a part might creep to.
//
//  Every 0.1 mm of entry chamfer is 0.1 mm of engagement given away, so the
//  entry is cut to 0.20 and the hook deepened to 0.70. Engagement across the
//  land now runs 0.65 down to 0.38 mm instead of 0.31 down to 0.10.

//  [MEASURED on the rendered chassis] half-sizes at the mouth plane, chassis
//  z = body_t - cover_wall_d. These are what the lip hooks.
cover_rim_hw   = 57.951;
cover_rim_hh   = 69.951;

cover_mouth_w  = 2 * (cover_rim_hw - cover_hook);       // [DERIVED] = 114.502
cover_mouth_h  = 2 * (cover_rim_hh - cover_hook);       // [DERIVED] = 138.502
cover_cham_w   = body_w + 2 * cover_clear;              // [DERIVED] = 116.95
cover_cham_h   = body_h + 2 * cover_clear;              // [DERIVED] = 140.95
cover_w        = cover_cham_w + 2 * cover_wall_t;       // [DERIVED] = 120.95
cover_h        = cover_cham_h + 2 * cover_wall_t;       // [DERIVED] = 144.95
cover_corner_r = corner_blend + cover_clear + cover_wall_t;
cover_flare    = (cover_cham_w - cover_mouth_w) / 2;    // [DERIVED] radial step
//  The flare's AXIAL rise is not equal to its radial step. At 1:1 it is 45
//  degrees on the straight runs but measured 40.2 at the corners, where the
//  superellipse takes a smaller radial step for the same rise. 1.35:1 puts the
//  shallowest part of it at 52 degrees, clear of the 45 rule everywhere.
cover_flare_rise = cover_flare * 1.35;                  // [DERIVED]

//  The shell gets its own, tighter edge roll. The deck's own 1.20 / 0.28 makes
//  a surface that flares outward as it rises off the bed at about 42 degrees -
//  fine on the deck, which prints the other way up, marginal here.
cover_edge_soft = 0.80;   // [DESIGN]
cover_edge_roll = 0.24;   // [DESIGN]
//  ---- THE INNER FACE DOES NOT TOUCH ---------------------------------------
//  Two large flat faces meeting is what rocks when either one bows, and PLA
//  bows: on the bed, and again over months as it relaxes. A lip can be perfect
//  and the cover will still sit proud in the middle.
//
//  So the middle is not a mating surface at all. The inner face is recessed
//  across everything except a narrow perimeter land - which the wall stiffens
//  and keeps true - and four pads at the magnets, which must stay at full
//  height because 0.5 mm of extra gap costs roughly half the pull.
//
//  The consequence is the point: warp in the centre of the plate has nothing to
//  bear against, so it cannot lift the edges or rock the part. It is floating
//  over a void BY DESIGN, rather than floating because it does not fit.
cover_land     = 5.00;   // [DESIGN] perimeter contact band
cover_recess   = 0.50;   // [DESIGN] how far the middle is held clear. Deeper
                         //   than any warp this part will realistically take,
                         //   and it costs nothing - the plate is 2.95 thick.
cover_pad_d    = magnet_boss_d;   // [DERIVED] magnet pads stay at full height

assert(cover_recess < cover_t - 1.2,
       "recess leaves under 1.2 mm of plate over the magnet pockets");
assert(cover_land >= 3.0,
       "perimeter land too narrow to seat on without digging in");

//  ONE affordance for removal, and it is a form, not a hole: a very shallow,
//  very wide scallop on the bottom edge. Wide-and-shallow reads as drawn;
//  small-and-round reads as a hole punched in a finished object.
cover_relief_r = 210.0;  // [DESIGN] scallop radius
cover_relief_d = 1.60;   // [DESIGN] bite into the edge -> a 51 mm wide sweep

assert(cover_hook < 1.023,
       "lip reaches deeper than the rim's undercut; it would foul on the way on");
assert(body_t - cover_wall_d > 12.78,
       "cover wall is deep enough to cover the USB-C opening");
//  The flare is 45 degrees by construction - the same number is used for the
//  radial step and the axial rise - so this asserts it is a real step rather
//  than a coincidence that it vanished. It is NOT hook + clear: the rim at the
//  mouth plane is already 0.174 mm narrower than the shell's widest section.
assert(cover_flare > 0.3,
       "chamber and mouth have collapsed together; there is no lip left to hook");
assert(cover_wall_d > cover_lip_entry + cover_lip_t + cover_flare_rise,
       "lip zone is deeper than the wall; there is no chamber for the deck to sit in");

cover_reg_depth = 0.00;   // [SUPERSEDED] the skirt carries shear now; see C-38
cover_reg_clear = 0.30;   // [DESIGN] per side, at the outer face
//  The platforms are RIMS, not slabs. A rim locates exactly as well as a solid
//  block, adds stiffness where a flat plate wants it most - around the two big
//  spans - and keeps 20 g of PLA out of a part that hangs off four small
//  magnets. It also leaves nothing to trap grit against the glass.
cover_reg_rim   = 2.50;   // [DESIGN] wall of each register platform
//  A platform must taper with the aperture it enters, or it wedges. Each one
//  is the aperture's OUTER-face opening, less clearance, tapering at the same
//  rate over the depth it actually enters - so it self-centres instead of
//  jamming, and every figure follows the aperture it mates with.
cover_reg_draft_display = display_aper_draft * cover_reg_depth / front_t;
cover_reg_draft_kbd     = kbd_aper_draft     * cover_reg_depth / front_t;

cover_reg_w_display = display_aper_w + 2*display_aper_draft - 2*cover_reg_clear;
cover_reg_h_display = display_aper_h + 2*display_aper_draft - 2*cover_reg_clear;
cover_reg_blend_display = aper_blend_display + display_aper_draft
                          - cover_reg_clear - cover_reg_draft_display;

cover_reg_w_kbd = kbd_aper_w + 2*kbd_aper_draft - 2*cover_reg_clear;
cover_reg_h_kbd = kbd_aper_h + 2*kbd_aper_draft - 2*cover_reg_clear;
cover_reg_blend_kbd = aper_blend_kbd + kbd_aper_draft
                      - cover_reg_clear - cover_reg_draft_kbd;

//  RELEASE. One deliberate affordance, at the top right - the flank opposite
//  the keyboard service window - so the intended peel starts furthest from the
//  hooked keyboard end. A scallop, not a lever: it says how the cover comes off
//  without adding a mechanism.

//  A locating rib that stands taller than the gap to the LARGEST body stops
//  that body entering at all, and one that strays into a corner blend or the
//  service window bears on nothing.
//  ---- variant 2 assertions ---------------------------------------------
assert(variant == 1 || variant == 2, "variant must be 1 or 2");
assert(magnet_x_min <= magnet_x_max,
       "no room on the display flank for a magnet boss: it cannot clear both the board pocket and the shell skin");
assert(magnet_y_lo < magnet_y_hi,
       "magnet y stations collapsed: the fastener bosses leave no run between them");
assert(magnet_skin >= 0.6,
       "magnet skin thinner than 3 layers will telegraph the disc on the show face");
assert(magnet_bore > magnet_d + magnet_tol,
       "magnet bore is inside the disc's own tolerance band; crush ribs cannot absorb that");
assert(magnet_rib_h > magnet_tol * 2,
       "crush ribs shorter than the disc's tolerance band cannot grip the whole batch");
//  C-35. Both halves of the feature have to survive the slicer: the rib itself
//  and the gap beside it. Either one below a single extrusion and the ring
//  prints as something other than what is drawn.
assert(magnet_rib_w >= nozzle,
       "crush rib narrower than one extrusion: the slicer decides its width, not this file");
assert(PI * magnet_bore / magnet_rib_n - magnet_rib_w >= nozzle,
       "gaps between crush ribs are under one extrusion; they will bridge into a solid ring");
assert(2 * magnet_rib_c * sin(180 / magnet_rib_n) > 2 * magnet_rib_r,
       "rib cutters overlap each other; the ribs would be malformed before slicing");
assert(cover_reg_depth + 0.8 <= 2.65,
       "cover register platform would reach the display glass");
assert(cover_t - magnet_pocket_h >= 0.6,
       "cover too thin to bury a magnet without telegraphing it");

//  The plunger must clear the board and land on the switch, and the flange must
//  fit its counterbore with travel left over.
assert(btn_post_dy + btn_post_h / 2 <= btn_band_hi,
       "button plunger reaches into the PCB's own thickness");
assert(btn_post_dy - btn_post_h / 2 >= btn_band_lo,
       "button plunger overhangs the switch body and would press nothing");
assert(btn_post_w <= button_body_w,
       "button plunger is wider than the switch body it presses");
assert(btn_travel >= 2 * board_mount_float + btn_rest_clear + btn_switch_throw,
       "button cannot cross the board's mounting float and still work the switch");
assert(btn_reach < (board_pocket_h - board_h) / 2 + 0.19 - board_mount_float,
       "button plunger would preload a switch on a board floated toward the wall");
assert(btn_cb_depth <= wall - dish_depth - 1.2,
       "button flange counterbore leaves too little wall outboard of it");
assert(kbd_rib_r_x < (kbd_pocket_w - kbd_body_w_max) / 2,
       "keyboard X locating rib is taller than the gap to the largest body");
assert(kbd_rib_r_y < (kbd_pocket_h - kbd_body_h_max) / 2,
       "keyboard Y locating rib is taller than the gap to the largest body");
assert(max([for (d = kbd_rib_dy) abs(d) + kbd_rib_r_x])
           <= kbd_pocket_h / 2 - kbd_pocket_corner_r,
       "keyboard X locating rib runs into a pocket corner blend");
assert(max([for (d = kbd_rib_dx) abs(d) + kbd_rib_r_y])
           <= kbd_pocket_w / 2 - kbd_pocket_corner_r,
       "keyboard Y locating rib runs into a pocket corner blend");
assert(batt_cowl_rise >= batt_protrusion - back_t + batt_cowl_wall,
       "battery cowl too shallow for the holder protrusion");

//  ---- THE COWL IS NOT ALONE ON THE BACK PLATE -------------------------------
//  Everything below exists because the cowl once lay on top of two fastener
//  countersinks and an access window and nothing noticed. A hole is not open
//  just because it was cut; something can be sitting on it. See DATUMS.md C-25.
//
//  x-extent of a superelliptical outline at a given offset from its centreline.
//  Corner points are (ax + c*cos(t)^(2/n), ay + c*sin(t)^(2/n)), so eliminating
//  t gives x = ax + c*(1 - (v/c)^n)^(1/n) with v the offset into the corner.
function rse_x_at(w, h, cr, n, y) =
    let (c  = min(cr, w/2 - 0.01, h/2 - 0.01),
         ax = w/2 - c,
         ay = h/2 - c,
         v  = abs(y) - ay)
    v <= 0 ? w/2
  : v >= c ? ax
           : ax + c * pow(1 - pow(v / c, n), 1 / n);

//  The cowl's footprint is widest AT THE PANEL, where the foot flare is at full
//  size - that is the section that has to miss everything else on the plate.
cowl_foot_w = batt_cowl_w + 2 * batt_cowl_foot;
cowl_foot_h = batt_cowl_h + 2 * batt_cowl_foot;
cowl_foot_c = batt_cowl_base_r + batt_cowl_foot;

//  1. the two LOWER M2.5 board screws, in the cowl's own frame
cowl_screw_dy = -board_mount_pitch_y/2 - batt_off_y;              // = -11.65
cowl_screw_dx = board_mount_pitch_x/2;                            // =  42.75
//  The cowl DOES still reach these two screws at a 6.0 corner - the old
//  horizontal-extent assert here fails by design now - and that is fine,
//  because each one gets a relief bore straight through the cowl instead. That
//  decouples the two constraints the corner radius could not satisfy at once:
//  the corner stays small enough to keep a wall on the cavity, and the screws
//  get their access cut locally rather than by reshaping the whole cowl.
//
//  Both conditions are stated as clearances, not as x-extents, because an
//  x-extent is what hid the breach in the first place. The real minimum wall is
//  measured from the rendered plate by validate.py's COWL checks.
assert(cowl_screw_relief_d >= board_cs_head_d + 0.2,
       "cowl screw relief is not wide enough to pass the M2.5 countersunk head");
assert(cowl_screw_dx - cowl_screw_relief_d/2 - cowl_cav_w/2 >= 0.4,
       "cowl screw relief breaks into the battery cavity");

//  2. the expansion-header window, whose lower edge is the cowl's ceiling
cowl_win_lo = expansion_win_y - (expansion_win_h + 2*fit_slide)/2 - batt_off_y;
assert(cowl_foot_h/2 <= cowl_win_lo - 0.4,
       "battery cowl foot buries the expansion-header window");

//  3. the cavity must still be at FULL SECTION where the holder is deepest -
//     not merely deeper than it. This is the check the depth-only one replaced.
assert(batt_cowl_crown * (batt_cowl_rise - batt_cowl_wall)
       >= batt_protrusion - back_t + batt_cowl_clear,
       "cowl crown starts before the cavity has cleared the holder");

//  4. the holder's plan form is exactly SQUARE (DATUMS C-21), so the cavity's
//     rounded corner must not bite into the holder's corner point.
cowl_cav_w = batt_cowl_w - 2*batt_cowl_wall;
cowl_cav_h = batt_cowl_h - 2*batt_cowl_wall;
assert(pow((batt_bay_w/2 - (cowl_cav_w/2 - batt_cowl_base_ri)) / batt_cowl_base_ri, form_n)
     + pow((batt_bay_h/2 - (cowl_cav_h/2 - batt_cowl_base_ri)) / batt_cowl_base_ri, form_n)
       <= 1.0,
       "cowl cavity corner bites the holder's square corner");
assert(batt_cowl_base_r + batt_cowl_foot <= min(cowl_foot_w, cowl_foot_h)/2 - 0.05,
       "cowl base corner is larger than the cowl: rse_poly would silently clamp it");
assert(shell_screw_cs_head_h < back_t - 1.0, "countersink leaves too little plate under the head");
assert(kbd_aper_w < kbd_pocket_w, "keyboard would fall through the front face");
assert(kbd_aper_h < kbd_pocket_h, "keyboard would fall through the front face");
assert(display_aper_w >= display_active_w, "front face clips the display");
assert(display_aper_h >= display_active_h, "front face clips the display");
assert(body_t >= back_t + board_depth + front_t, "not deep enough for the board");
assert(window_w > 50 && window_h > 50,
       "reference acrylic outline is degenerate");
//  If an acrylic window is ever wanted, this is the bound it has to satisfy and
//  it does NOT today: the sheet must fit the board pocket in plan AND the
//  pocket must be deepened by window_t + clearance to take it.
assert(window_w > board_pocket_w,
       "reference acrylic is WIDER than this board pocket: provenance, not a part (DATUMS C-09)");
assert(form_n > 2.0, "form_n <= 2 is a plain arc, not a continuous corner");
assert(corner_blend < min(body_w, body_h) / 2, "corner blend larger than the part");
//  2.0 mm, stated as a LENGTH rather than as a bead count. It was written as
//  "5 * nozzle", which was 2.00 mm at a 0.4 nozzle and silently became a demand
//  for 4.00 mm the moment the nozzle changed - a check that moves its own goal
//  posts is not a check. The physical quantity it protects, the material left
//  at the shell's arris once the roll has withdrawn the face, is unchanged;
//  2.00 mm is 2.5 beads at 0.8 and was 5 at 0.4.
assert(wall - edge_soft >= 2.0,
       "edge roll thins the shell arris below 5 extrusions");
assert(cavity_blend < corner_blend, "cavity corner is fuller than the shell corner");
//  The display aperture's corner must not cut into the panel's SQUARE active
//  area corner. Inset of a superelliptical corner at 45 deg is
//  blend * (1 - cos(45)^(2/n)).
assert(display_aper_w/2 - aper_blend_display * (1 - pow(cos(45), 2/form_n))
       > display_active_w/2,
       "display aperture corner clips the active area");
assert(display_aper_h/2 - aper_blend_display * (1 - pow(cos(45), 2/form_n))
       > display_active_h/2,
       "display aperture corner clips the active area");
assert(grille_count * grille_slot_h + (grille_count - 1) * (grille_pitch - grille_slot_h)
       <= grille_field_h + 0.01,
       "grille field exceeds the board's own grille opening");

echo(str("cYbErDeCk envelope [", preset, "]: ",
         body_w, " x ", body_h, " x ", body_t, " mm"));

// =============================================================================
//  CONCRETE JACKET  (variant 3)
// =============================================================================
//  The printed chassis is NOT replaced. It stays exactly as validated and
//  becomes the permanent core: it keeps every tolerance, every heat-set insert
//  and every 115 checks that were run against it. Concrete is cast AROUND it as
//  an outer jacket, doing only what concrete is good at - mass, face and edge.
//
//  WHY NOT CAST THE WHOLE ENCLOSURE. Plain concrete has no useful tensile
//  strength and the wall here is 3.2 mm. Cast at that section it would craze on
//  the first drop and would not hold an M2 heat-set insert at all - you cannot
//  heat-set into stone. Both problems disappear if the plastic stays.
//
//  The jacket is captive by geometry, not by adhesive: it wraps the front face
//  and all four sides, and its apertures are smaller than the body, so it
//  cannot be slid off in any direction. Cast in place is the whole fixing.
conc_t         = 7.00;   // [DESIGN] jacket wall. GFRC is reliable from about 6;
                         //   below that it chips at edges, above 8 the object
                         //   passes 600 g and stops being a handheld.
conc_chamfer   = 1.60;   // [DESIGN] front edge break. Cast concrete will not
                         //   hold a sharp arris - it spalls on demould.
conc_draft_deg = 2.00;   // [DESIGN] per-side draft toward the open back so the
                         //   casting releases from the collar. 1 deg is the
                         //   floor for a rough mould face; 2 is safe.
conc_aper_relief = 0.60; // [DESIGN] concrete apertures sit proud of the
                         //   plastic ones, so the reveal reads as a deliberate
                         //   plastic edge rather than as a bad register.
conc_aper_draft  = 1.20; // [DESIGN] per-side flare of the aperture blockouts,
                         //   outward toward the show face, so they pull.
conc_top_open  = true;   // [DESIGN] leave the top edge free of concrete. The
                         //   three buttons and both microphones open through
                         //   that edge; burying them would mean lengthening
                         //   every cap. A plastic control strip along the top
                         //   is the honest answer and reads as deliberate.
conc_density   = 2.10;   // [VENDOR] g/cm3, GFRC with acrylic fortifier

//  ---- the mould ------------------------------------------------------------
conc_mold_base   = 6.00; // [DESIGN] face plate thickness
conc_mold_wall   = 6.00; // [DESIGN] collar wall
conc_flange      = 14.0; // [DESIGN] bolt flange width
conc_bolt_d      = 4.50; // [STANDARD] M4 clearance
conc_pin_d       = 4.00; // [DESIGN] alignment dowel
conc_pin_h       = 6.00; // [DESIGN]
conc_mold_gap    = 0.15; // [DESIGN] collar-to-face-plate slip fit

conc_w         = body_w + 2 * conc_t;              // [DERIVED] = 130.25
conc_h         = body_h + 2 * conc_t;              // [DERIVED] = 154.25
conc_stack     = body_t + conc_t;                  // [DERIVED] = 23.85
conc_blend     = corner_blend + conc_t;            // [DERIVED]
conc_draft     = conc_stack * tan(conc_draft_deg); // [DERIVED] total per side

assert(conc_t >= 6.0,
       "concrete jacket under 6 mm: GFRC chips at the edges below this");
assert(conc_chamfer >= 1.0,
       "a sharp cast arris spalls on demould; break the front edge");
assert(conc_draft_deg >= 1.0,
       "less than 1 degree of draft and the casting will not leave the collar");


// =============================================================================
//  BACK FACE: FLOOD-COATED WITH TWO-PART ACRYLIC, USING A JIG
// =============================================================================
//  The back plate is finished with poured self-levelling acrylic, and that only
//  works inside a dam - without one the resin runs off the edge, starves the
//  perimeter and leaves a lip of bare plastic exactly where the eye goes.
//
//  THE DAM CANNOT BE PART OF THE PLATE. It was built that way first and the
//  audit rejected it: a perimeter wall must clear the four M2.5 countersinks
//  AND the 82.80 mm battery cowl, and on a 109.25 mm plate there is no ring
//  that does both. Printed over a countersink it is the C-25 defect exactly -
//  something lying on top of a screw - and it would also have sealed the
//  fasteners under the coat, which is worse than ugly.
//
//  So the dam is a JIG. The plate drops into a frame that stands proud of it,
//  the resin levels inside that, and the frame comes off once the coat has
//  gelled. It touches no validated geometry, it sits entirely outboard of every
//  fastener because it surrounds the plate rather than crossing it, and it is
//  reusable.
pour_dam_clear = 0.30;   // [DESIGN] slip fit around the plate
pour_dam_wall  = 3.00;   // [DESIGN] frame wall
pour_dam_rise  = 2.00;   // [DESIGN] how far the frame stands above the plate
                         //   face. Two-part acrylic self-levels at roughly
                         //   0.8-1.0 mm; 2.0 leaves headroom for the meniscus
                         //   and for a second coat.
pour_dam_floor = 2.00;   // [DESIGN] frame floor, so the plate sits level

assert(pour_dam_rise >= 1.5,
       "pour dam under 1.5 mm gives a self-levelling coat no headroom");

// =============================================================================
//  CONCRETE: THUMB RELIEF AT THE KEYBOARD
// =============================================================================
//  The jacket puts 7 mm of concrete in front of the chassis face, and the
//  keycaps sit just behind that face. Left square, the keyboard would be at the
//  bottom of a 7 mm well and the outer keys would be unreachable by a thumb.
//
//  So the concrete ramps away from the keyboard aperture. A straight chamfer,
//  not a cove: over the same depth a chamfer clears far more lateral room, and
//  lateral room is what a thumb actually needs. The same ramp serves the other
//  posture - resting on the battery cowl, leaning back, typed with fingers.
conc_kbd_relief = 5.50;  // [DESIGN] per-side ramp at the keyboard, leaving
                         //   conc_t - this = 1.50 mm of land at the aperture
                         //   edge. Below about 1.5 a cast arris spalls.
assert(conc_t - conc_kbd_relief >= 1.5,
       "keyboard ramp leaves under 1.5 mm of concrete land; the edge will spall");

// =============================================================================
//  CARRY CASE  (cad/carrycase.scad)
// =============================================================================
//  Not a cover. A sleeve the whole deck slides into, vertically, like a side
//  bag: front, back, both flanks and the bottom, open only at the top. Every
//  port is buried. It is meant to be flocked inside and filled, sanded and
//  polished outside until it reads as one industrial object, so every external
//  surface here is continuous and every internal one carries a flock allowance.
//
//  THE COWL IS THE GUIDE. The deck's battery cowl is 83.93 x 28.23 mm at its
//  foot and stands 11.00 mm off the back - measured on the rendered plate, not
//  assumed. A channel that width running the full height of the cavity lets the
//  deck slide in and keys it in X and in rotation at the same time. The bump
//  stops being a problem to accommodate and becomes the location feature.
case_flock  = 0.80;   // [DESIGN] flock pile, per surface. Nylon flock lands
                      //   0.5-1.0; 0.8 is the middle and it is the difference
                      //   between a deck that slides and one that binds.
case_clear  = 0.40;   // [DESIGN] clearance on top of the flock
case_pad    = case_flock + case_clear;              // [DERIVED] = 1.20

//  ---- IT IS TWO PARTS, BOLTED -----------------------------------------------
//  One piece could be printed. It could not be reached into - the magnet
//  pockets opened into a cavity 155 mm deep with nothing to get at them by.
//  A plane parallel to the face fixes that and pays four more times: both
//  halves print flat and face-down, the magnet pockets open upward on the bed,
//  the inside becomes two open trays to flock, and the back stops needing a
//  spine to stand on.
//
//  The seam is not hidden. A 0.6 mm chamfer each side makes it a 1.2 mm shadow
//  gap, which is the only honest thing to do with a joint you cannot fill.
case_wall   = 3.60;   // [DESIGN] front and back. WAS 4.00. These two faces are
                      //   solid slabs - 4.00 mm is five extrusions at a 0.8
                      //   nozzle, so nothing in them is infill - and they are
                      //   a quarter of the filament. The floor is the magnet
                      //   skin: case_mag_skin must clear 4 layers, which puts
                      //   the wall at 3.35 minimum.

//  ---- THE WALL IS UNIFORM, AND THAT IS WHY THE FASTENERS CAN GO ROUND -------
//  v3 put the fasteners in two rectangular rails on the flanks. Two faults,
//  and the owner named both: "the rectangular shapes where the holes are, plus
//  the squircle-esque shape come together at this weird angle the geometry
//  clashes ... the screw holes need to also attach all the way around."
//
//  They are the same fault. A rail is a STRAIGHT bar laid against an outline
//  that is straight in the middle and curved at the ends, so its ends always
//  land somewhere the body is turning and the wedge between them reads as a
//  mistake. And a rail can only exist where the body is straight, which is why
//  the fasteners could only be on the sides.
//
//  Make the wall one thickness the whole way round and both go away together.
//  There is no rail to clash, because the fasteners sit ON the outline - see
//  THE FASTENER RING below - and the outline goes round, so they go round.
//  ---- THIN EVERYWHERE, THICK ONLY AT THE FASTENERS ---------------------------
//  Every version until now carried a wall thick enough for a fastener ALL THE
//  WAY ROUND, because a uniform wall is the only kind a plain offset can make.
//  That put 13 mm of PLA everywhere to serve nine holes, and the flanks were
//  measured at 36 per cent of the filament.
//
//  The wall is 6 mm now and SWELLS to 16.5 at each of the nine sites. The swell
//  is not a pad stuck on: the plan outline itself is pushed outward with a
//  smooth falloff, so the surface has no junction anywhere - which is also what
//  a river rock is.
case_side   = 6.00;   // [DESIGN] the thin wall, between the swells
//  THE MOUTH IS SET BY A HAND, THE FLOOR TAKES UP THE SLACK. Holding the deck's
//  proportion fixes the case's HEIGHT; it does not say where to spend it. Spend
//  it at the mouth and the deck sits 24 mm down a hole, which needs a scallop
//  cut in the front to get it out - and a scallop big enough to matter eats the
//  silhouette. Spend it at the floor and it is invisible, it needs no scallop,
//  and it puts 28 mm of solid PLA at the end you actually drop the thing on.
case_rim    = 12.00;  // [DESIGN] how far the mouth stands above the seated deck.
                      //   12 is a finger pad: enough to pinch the deck's top
                      //   edge front and back and draw it out.

//  [MEASURED on the rendered back plate]
case_cowl_w = 83.93;  // cowl foot, along X
case_cowl_r = 11.00;  // how far it stands off the back face

case_cav_w  = body_w + 2 * case_pad;                // [DERIVED] = 118.65
case_cav_hw = case_cav_w / 2;
case_w      = case_cav_w + 2 * case_side;           // [DERIVED] = 130.65
case_slot_w = case_cowl_w + 2 * case_pad;           // [DERIVED] =  86.33
case_z_fr   = body_t + case_pad;                    // [DERIVED] cavity front
case_z_bk   = -case_pad;                            // [DERIVED] cavity back
case_z_cowl = -case_cowl_r - case_pad;              // [DERIVED] channel floor

//  THE FLOOR IS 12, NOT 21, AND THE PROPORTION RULE IS GONE.
//  case_floor used to be solved so that case_h/case_w matched the deck's own
//  ratio. That was my idea, not a requirement, and it was buying a number
//  nobody looks at with 9 mm of solid plastic across the full 145 x 38 section.
//  The case is now as big as it has to be and no bigger.
case_floor  = 13.00;  // [DESIGN] enough to land on, and no less than the flank so
                      //   the fastener ring rounds the corner at the same margin

case_y_bot  = -body_h/2 - case_pad;                 // [DERIVED] deck lands here
case_y_top  =  body_h/2 + case_rim;                 // [DERIVED] the mouth
case_y_floor= case_y_bot - case_floor;              // [DERIVED] = -84.325
case_h      = case_y_top - case_y_floor;            // [DERIVED] = 166.45
case_cy     = (case_y_top + case_y_floor) / 2;      // [DERIVED] =   -1.10
case_cav_r  = corner_blend + case_pad;              // [DERIVED] =  12.40

//  THE BACK IS FLAT, AND THAT IS A PRINT FINDING BEFORE IT IS A STYLE ONE.
//  The obvious back is a slab with a raised spine carrying the cowl channel.
//  Printed back-down the spine crown is the first layer and the slab bottom
//  sits 11.00 mm above it - a downward-facing flat face 20 mm wide running the
//  full height, 3,100 mm2 a side, needing support. Flaring the spine out to
//  meet the slab does not fix it: 11 mm of rise over 28 mm of run is 21 deg,
//  half of what FDM will hold. So the back drops to the channel floor
//  everywhere. It costs 11 mm of depth, it prints with nothing under it, and a
//  solid rectangular block is the more honest object anyway.
case_z0     = case_z_cowl - case_wall;              // [DERIVED] = -15.80 back
case_z1     = case_z_fr   + case_wall;              // [DERIVED] =  21.65 front
//  THE SPLIT IS NOT IN THE MIDDLE, AND THAT IS THE POINT.
//  It sat at exactly 50 % of the depth, so the seam read as a crack down the
//  centre of a brick rather than as a line anyone chose. It is free to move -
//  the only constraints are that the cowl channel stays in the back half
//  (z > case_z_bk) and the magnet pockets stay in the front (z < case_z_fr),
//  which leaves the whole window from -1.20 to +18.05.
//
//  At +11.00 the halves are 11.05 and 27.20: a 1:2.46 datum line at a
//  proportion someone picked. It also makes the screw work - the front half is
//  the only thing a screw has to cross before it reaches its insert, and
//  11.05 + 8 of thread is 19.05, so M5 x 20 spans it with 0.95 to spare.
case_split_z = 8.65;                                // [DESIGN] front half 13.00
//  ---- NO PLINTH, AND WHY IT IS RECORDED --------------------------------------
//  A stepped foot was built and taken out again. It cannot coexist with a
//  fastener ring that goes all the way round, and the arithmetic is flat:
//
//    the ring crosses the floor 6.50 mm in from the bottom edge, so a bore
//    there has 3.80 mm of metal to that edge. case_bolt_keep wants 3.00. The
//    plinth therefore gets 0.80 mm, which disappears under sanding.
//
//  It gets worse than merely subtle. A plinth shorter than the corner radius
//  (13.94) sits entirely inside the bottom corner's curve and never reads as a
//  plinth at all, so the version that WOULD read has to be ~18 mm tall - which
//  puts the two bottom-corner fasteners inside it too.
//
//  A 15 mm wall buys 1.80 mm of plinth. That is the trade if it is wanted: 4 mm
//  on the case width for a foot.
//
//  Two print findings from the attempt, worth keeping. Insetting the DEPTH as
//  well - the more correct plinth - cannot be printed: the halves lie on their
//  faces, so a Z inset is a ledge pointing at the bed, 377 mm2 of it measured.
//  And tapering that ledge made it worse, not better: 2.5 mm of rise over 3.3
//  of run is 53 degrees off vertical, past the limit rather than under it.

case_seam_ch = 0.30;  // [DESIGN] a hairline at the joint, not a shadow gap:
                      //   the object reads as one piece, so this is only
                      //   enough relief to stop a few tenths of print
                      //   mismatch showing as a step. WAS 0.60.

//  ---- RIVER ROCK -------------------------------------------------------------
//  A river stone worn flat on two sides, which is the only rock this can be.
//
//  THE TWO FACES ARE FLAT AND THAT IS NOT A STYLE CHOICE. Both halves print
//  face-down, and that is what makes the two-part split pay: one flat bed face
//  and one open tray each, no bridge, no support. A face that blended smoothly
//  into the curved flank would leave a near-horizontal DOWNWARD-facing band all
//  the way round the rim - the one overhang FDM cannot do unsupported. Crowning
//  the face outward is the same fault at 5 degrees over 75 mm. So the plateau
//  stays: 23,665 mm^2 of it, 88% of the bounding rectangle. Measured, not
//  estimated - the version that claimed "no flat band and no arris" had exactly
//  this plateau and a 90-degree edge round it. See C-49.
//
//  EVERYTHING BETWEEN THE FACES IS CURVE. case_roll at 0.50 means the inset is
//  falling the whole way from one face to the middle and rising again to the
//  other: no straight run, no girth line, no parting crease.
//
//  AND THE RIM IS A DELIBERATE FACET, not the 90-degree arris it used to be.
//  case_face_ang is off HORIZONTAL, so it is also the overhang angle when that
//  face is on the bed: 45 is the FDM limit, so 55 is a facet you can cut and
//  still not need support. It is the one chamfer on the object and it is meant
//  to be seen.
//
//  The roll is not free: it pulls the FRONT FACE in, and the bolt heads have to
//  sit in it. That is what sizes the swells - see case_boss_amp.
case_soft    = 4.00;  // [DESIGN] how far each face draws in
case_roll    = 0.50;  // [DESIGN] the whole half-depth, so nothing is flat
case_face_ch  = 4.00; // [DESIGN] the facet at each face rim, as rise
case_face_ang = 55;   // [DESIGN] degrees off horizontal; 45 is the overhang limit
case_face_run = case_face_ch / tan(case_face_ang);   // [DERIVED] = 2.80

assert(case_face_ang > 46,
       "the rim facet is a near-flat ceiling when that face is on the bed");
assert(case_face_run < case_soft,
       "the rim facet eats the whole face inset before the roll starts");

//  THE CORNER ECHOES THE DECK, IT DOES NOT INHERIT IT. Offsetting the deck's
//  corner outward by the wall gives 28.60 mm on a 150 mm body - proportionally
//  more than twice as round as the deck, which is why an early version read as
//  a pebble. Holding the RATIO instead gives a corner that is the same fraction
//  of width the deck's is, on the same exponent.
case_r      = corner_blend * case_w / body_w;       // [DERIVED] =  12.59

//  ---- THE FASTENER RING ------------------------------------------------------
//  The fasteners are not a list of coordinates. They are the case's own outline,
//  inset to the middle of the wall and sampled at even arc length, so they
//  follow the superellipse round the bottom corners instead of stopping where a
//  straight rail would have to. Nothing can drift off the form, because the
//  ring IS the form. cad/carrycase.scad builds it; these set its shape.
//  Back on the wall's centreline: with a 1 mm chamfer rather than a 3 mm roll
//  there is no longer a reason to bias it inboard.
//  The ring is an OUTSET OF THE CAVITY, not an inset of the outline. With a
//  wall that varies from 6 to 16.5 there is no single inset that lands a bore
//  6.50 mm outboard of the cavity everywhere, which is what the 3 mm metal
//  margin needs.
case_bolt_out = 6.50;                          // [DESIGN] outboard of the cavity
case_ring_w   = 2 * (case_cav_hw + case_bolt_out);           // = 131.65
case_ring_bot = case_y_floor + case_bolt_out;
case_ring_topline = case_y_top - case_bolt_out;
case_ring_h   = case_ring_topline - case_ring_bot;
case_ring_cy  = (case_ring_topline + case_ring_bot) / 2;
case_ring_r   = case_cav_r + case_bolt_out;                  // = 18.90
case_bolt_m   = 3;               // [DESIGN] pitches per side. WAS 6, which with
                                 //   one at bottom dead centre made thirteen -
                                 //   an absurd number, and it was. Three gives
                                 //   SEVEN: dead centre, each bottom corner,
                                 //   each mid-flank, and each flank TOP.
                                 //
                                 //   Six were tried, sampled on half pitches so
                                 //   nothing sat at dead centre. It is the worse
                                 //   arrangement: it puts the topmost fastener
                                 //   44 mm below the mouth and leaves the one
                                 //   end of the ring that is already open
                                 //   unclamped. Seven costs one bolt and holds
                                 //   the mouth shut.

//  WHY THE RING IS A U AND NOT A CLOSED LOOP. Nothing is being conceded here:
//  a fastener parallel to Z needs material through the WHOLE depth, and across
//  the mouth there is none - the deck's own cross-section has to pass through
//  there. The flanks and the floor have full-depth metal, the mouth cannot.
//  So the ring runs as far up both flanks as it can and stops.
case_bolt_top_back = 16.00;      // [DESIGN] how far short of the flank's end
                                 //   the ring stops. Two reasons, both real:
                                 //   it leaves 20 mm of metal above the top
                                 //   fastener, and it gives the strap boss
                                 //   straight flank to terminate on.

case_bolt_d     = 5.00;   // [STANDARD] M5, as asked for
case_bolt_clear = 5.40;   // [STANDARD] ISO 273 medium fit
//  THE HEADS SIT FLUSH. A 4.80 mm head in a 5.00 mm counterbore lands 0.20
//  below the surface, and a shallow dish blends its rim into the curve so it
//  reads as an inset rather than a drilled hole.
//
//  That counterbore is what fixes the screw length: it eats 5.00 mm of the
//  13.00 mm front half, so 8.00 of thread reaches the insert and M5 x 16 is
//  the size. M5 x 20 would bottom out in a 10 mm insert.
case_bolt_head_h = 4.80;  // [VENDOR] as specified
case_cb_d        = 9.00;  // [DESIGN] clears an ISO 4762 O8.50 head
case_cb_deep     = 5.00;  // [DESIGN] 0.20 below flush
//  THE DISH IS SIZED BY ITS FOOTPRINT, NOT BY HOW DEEP IT LOOKS. A sphere of
//  radius R cutting d deep leaves a footprint 2*sqrt(2Rd - d^2) wide, so r30 at
//  1.2 mm is 16.80 mm across - nearly twice the counterbore it was blending -
//  and it cut straight out through the rolled edge. r11 at 1.00 is 9.17 across
//  and leaves a 1.42 mm rim. Built and measured as r30 first; it broke out.
case_dish_r      = 11.00; // [DESIGN] the concave blend at the rim
case_dish_d      =  1.00; // [DESIGN] how deep it dishes
case_dish_w      = 2 * sqrt(2*case_dish_r*case_dish_d - pow(case_dish_d,2));
case_bolt_len   = 16.00;  // [STANDARD]
case_insert_d   =  7.00;  // [VENDOR] M5 heat-set insert, outside diameter once
                          //   it has melted in - this is what the wall has to
                          //   carry, not the drilled bore
case_insert_bore=  6.20;  // [VENDOR] the printed hole it is driven into
case_insert_len = 10.00;  // [VENDOR] as specified
case_bolt_head_d= 8.50;   // [STANDARD] ISO 4762 socket cap
case_nut_af     = 8.00;   // [STANDARD] ISO 4032 / DIN 934 M5, across flats
case_nut_t      = 4.70;   // [STANDARD] ISO 4032 M5, m_max. WAS 4.00, which is
                          //   the DIN 934 figure under an ISO 4032 label - a
                          //   legal ISO nut stands up to 0.50 mm proud of a
                          //   pocket cut for it. Taking the larger of the two
                          //   makes the pocket accept either.
case_fit        = 0.20;   // [DESIGN] clearance on the hex: +0.10 a side. NOT a
                          //   press fit - the hex only has to key the nut
                          //   against rotation. The screw pulls it onto its
                          //   seat, so it does not need to be held there.
case_bolt_keep  = 3.00;   // [DESIGN] least metal from a bore to any surface
case_nut_h      = case_nut_t + case_fit;                     // [DERIVED] = 4.90
case_nut_cd     = (case_nut_af + case_fit) / cos(30);        // [DERIVED] = 9.47
case_insert_z   = case_split_z - case_insert_len;            // [DERIVED] = -1.35
case_bolt_seat  = case_z1 - case_cb_deep;                    // [DERIVED] = 16.65
case_bolt_tip   = case_bolt_seat - case_bolt_len;            // [DERIVED] = 0.65
case_bolt_grip  = case_split_z - case_bolt_tip;              // [DERIVED] = 8.00
case_bolt_stack = case_cb_deep + case_bolt_len;              // [DERIVED] = 21.00
case_bolt_n     = 2 * case_bolt_m + 1;                       // [DERIVED] = 7

//  Locating pins. Thirteen screws clamp but each floats 0.20 mm in its
//  clearance hole, and a 0.20 mm step at a seam that gets sanded is a seam you
//  can feel. Four printed pins, on the ring, halfway between screws. They carry
//  nothing; they only stop the halves sliding while the screws go in.
case_pin_d   = 4.00;  case_pin_h = 3.00;  case_pin_fit = 0.30;
case_pin_ks  = [0.5, 3.5];   // [DESIGN] where on the ring, in screw pitches

//  ---- THE SWELLS -------------------------------------------------------------
//  The counterbore, not the bolt, sizes these. A flush head needs its O9.00
//  counterbore plus a 1.50 mm rim to sit INSIDE the front face, and the roll
//  pulls that face in by case_soft. So the swell has to reach
//      bolt_x + cb/2 + rim - (cav + side) + soft  =  10.50 mm
//  which takes the local wall to 16.50 where a fastener is and leaves it at
//  6.00 where none is.
//
//  The falloff is cos-squared over case_boss_reach, and overlapping swells are
//  combined as 1 - prod(1 - f) rather than summed, so two sites near each other
//  blend instead of stacking to twice the amplitude.
case_boss_amp   = 10.50;  // [DERIVED->DESIGN] see above
case_boss_reach = 18.00;  // [DESIGN] how far along the outline it dies away.
                          //   26 was tried: nine swells at that reach overlap
                          //   and the wall ends up thick everywhere anyway,
                          //   which is the opposite of the point.
case_boss_lift  =  6.50;  // [DESIGN] the swell peaks over the bore, which sits
                          //   this far outboard of the cavity

//  ---- THE STRAP LUG IS A HOLE ------------------------------------------------
//  Eight versions of this now. The last one trapped a D-ring's bar in a bore
//  that straddled the parting plane, with tapered reliefs at each end for the
//  arch to come out of. It was clever and it was wrong: fiddly to print, fiddly
//  to assemble, and it made the strap depend on two 6 mm windows.
//
//  This is a hole. It runs front to back through the flank, so a cord or a
//  split ring wraps the full 13 mm of wall and hangs outward, and the load goes
//  into the whole height of the flank above it rather than into any feature.
//  The parting plane cuts across it, so each half prints it as a plain vertical
//  bore with nothing overhanging.
//
//  O7.00 is what the wall allows: 3.00 mm of metal either side, which is the
//  same margin every fastener bore gets. A bigger hole needs a local pad, and
//  a pad is the thing that has been rejected seven times.
case_lug_d  = 7.00;   // [DESIGN] takes 6 mm cord or a split ring
case_lug_y  = 62.00;  // [DESIGN] high on the flank, so a strap hangs flat
case_lug_x  = case_cav_hw + case_bolt_out;                   // = 65.825
case_lug_mat = (case_side + case_boss_amp - case_lug_d) / 2; // = 4.75 a side

//  ---- NO THUMB SCALLOP, AND WHY IT IS RECORDED --------------------------------
//  A 24 mm mouth was tried, with an 80 x 22 mm arc cut in the front to reach
//  the deck. It worked and it was wrong: on a 150 mm face that arc is not a
//  detail, it is the silhouette, and it turned a brutalist slab into a tote
//  bag. Shrinking it to a subtle 8 mm dish keeps the silhouette and stops
//  solving the problem - you cannot reach 24 mm down through an 8 mm relief.
//
//  Moving the height into the FLOOR removes the problem rather than styling
//  around it. Recorded so nobody adds the scallop back without first asking
//  why the mouth is 12 mm.

//  ---- MAGNETS: WHAT THEY ACTUALLY DO -----------------------------------------
//  In the one-piece sleeve these were indefensible - blind pockets 155 mm down
//  a tube that nobody could reach. Split, they open upward on the bed and are
//  dropped in by hand, so the objection is gone.
//
//  What has NOT gone is the arithmetic. The case disc sits flush at the cavity
//  face, so the closest it can ever be to the deck's is the deck's own 0.80 mm
//  skin plus 0.80 of flock plus 0.40 of clearance: 2.00 mm, and that is the
//  floor, not a target. Two ways to price it, both anchored on vendor data:
//    inverse square off the 0.80 mm figure  ->  0.62 N a pair,  2.46 N for four
//    two-point fit through contact and 0.80 ->  2.08 N a pair,  8.32 N for four
//  The deck weighs about 3.4 N and leaves along Y, so what resists is shear at
//  20.6 per cent - 0.51 to 1.71 N - plus friction from the clamp, mu about 0.4
//  on flock, another 1.0 to 3.3 N. Even reading every number optimistically
//  that is comparable to the deck's weight, not a multiple of it.
//
//  So they are a SEAT, not a latch: they tell your hand the deck has bottomed
//  and they stop it rattling. Retention is the cowl channel, the flock, and
//  carrying it mouth-up. Recorded here so nobody later reads four magnets as a
//  reason to trust the case upside down.
case_mag_gap   = magnet_skin + case_pad;                     // [DERIVED] = 2.00
case_mag_skin  = case_z1 - (case_z_fr + magnet_pocket_h);    // [DERIVED] = 1.45

assert((case_side + case_boss_amp - case_lug_d) / 2 >= 3.0,
       "strap hole leaves under 3 mm of wall either side at its swell");
assert(case_split_z > case_z_bk + 1 && case_split_z < case_z_fr - 1,
       "the split plane cuts the cowl channel or the magnet pockets");

assert(case_rim >= 10.0 && case_rim <= 16.0,
       "mouth too deep to pinch the deck out of, or too shallow to hold it");
assert(case_bolt_stack <= case_z1 - case_insert_z,
       "the screw and its counterbore are deeper than the metal they go into");
//  The C-43 guard, restated for a through-bolt: the nut has to bear on the far
//  side of the back half, not somewhere inside the front one.
assert(case_bolt_grip >= 1.2 * case_bolt_d,
       "under 1.2 diameters of thread engaged in the insert");
assert(case_bolt_tip > case_insert_z + 0.5,
       "screw bottoms out in its insert before the joint closes");
//  The wall is thin BETWEEN the swells; what has to hold is the wall AT one.
assert((case_side + case_boss_amp - case_insert_d) / 2 >= 3.0,
       "under 3 mm of metal round a heat-set insert, even at a swell");
//  The DISH, not the counterbore, is the wide one. Sizing the swell against the
//  counterbore alone passed while the dish cut out through the rolled edge.
assert(case_boss_amp - case_soft
       >= case_bolt_out + max(case_cb_d, case_dish_w)/2 + 1.0 - case_side,
       "the roll pulls the front face in past the counterbores or their dishes");
assert(case_cb_deep > case_bolt_head_h,
       "the head stands proud of its counterbore");
assert(case_split_z > case_z_bk && case_split_z < case_z_fr,
       "parting plane misses the cavity, so one half has no tray to flock");
assert(case_mag_skin >= 4 * layer_h,
       "under four layers of skin over the case magnet");
assert(case_r < case_side + case_pad + corner_blend,
       "case corner is rounder than a plain offset of the deck's");
assert(case_slot_w < case_cav_w,
       "cowl channel is wider than the cavity it runs in");
assert(case_pad >= 0.8,
       "no room for flock; the deck will bind once the inside is flocked");
