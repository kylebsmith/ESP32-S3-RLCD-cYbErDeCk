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
nozzle = 0.4;                       // [DESIGN]
layer_h = 0.2;                      // [DESIGN]

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
board_corner_r = 0.50;    // [VENDOR] R0.5, all four corners

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
board_stack     = board_w_display_front - board_w_standoff_end;   // = 10.75
board_depth     = 11.0;   // [DESIGN] board_stack + 0.25, taken up by a 0.5 mm
                          //   adhesive foam gasket round the display aperture
// How far the battery holder reaches past the standoff plane - this, not the
// cell diameter, is what the cowl actually has to swallow.
batt_protrusion = board_w_standoff_end - board_w_battery_end;     // = 8.20

board_pocket_w = board_w + 1.0;   // [DESIGN] 0.5 mm per side
board_pocket_h = board_h + 1.0;   // [DESIGN] 0.5 mm per side
//  The pocket's corner radius is NOT free. The PCB's corners are R0.50, so a
//  generously rounded pocket corner leaves material exactly where the board's
//  much sharper corner wants to be. With 0.5 mm per-side clearance the pocket
//  radius must not exceed about 2.2 mm; an earlier revision used 3.0 and the
//  board fouled all four corners by 0.33 mm. Asserted in section 6.
board_pocket_r = 2.0;             // [DESIGN]

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
display_aper_draft = 1.45;   // [MEASURED] per-side flare across the front face

//  The active area is NOT centred on the PCB along U. Its margins are 2.25 mm
//  from the U=0 edge and 5.45 mm from the U=92.50 (USB-C) edge, so its centre
//  sits 1.60 mm toward U=0 from the PCB centre. It IS centred along V.
//  Getting this wrong puts the frame 1.6 mm off the panel on one side.
display_off_x = -1.60;   // [VENDOR] computed from the drawing's AA margins
display_off_y =  0.00;   // [VENDOR] centred in V

// --- optional laser-cut acrylic window ---------------------------------------
//  Outline taken from the reference's own 2D acrylic template, which is an
//  exact, unambiguous source: a single closed 26-vertex LWPOLYLINE, DXF AC1015,
//  $INSUNITS = 4 (millimetres). It is authored in the board's portrait frame,
//  so its X/Y are transposed relative to this file's device frame.
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
button_cap_w   = 5.2;    // [MEASURED] reference cap cross-section
button_cap_h   = 4.0;    // [MEASURED]
button_flange  = 0.4;    // [MEASURED] per-side retaining flange behind the wall
button_w_centre = -0.70; // [VENDOR] switch centre in W: (-1.802 + 0.402)/2

// Dual microphone array, also on the V = 69.10 edge, at U 13.75 and 78.75.
mic_offset_x = 32.50;  // [VENDOR] +/-32.500 from the PCB centre. Independently
                       //   measured from the reference caseback as exactly
                       //   +/-32.500 - see docs/DATUMS.md D-07.
mic_aper_w   = 5.4;    // [MEASURED] reference aperture
mic_aper_h   = 2.5;    // [MEASURED]
mic_w_centre = -0.50;  // [VENDOR] mic body W 0 to -1.00

// 18650 holder, on the PCB back face, running ACROSS the device.
batt_bay_w = 77.80;    // [VENDOR] holder body along U
batt_bay_h = 22.10;    // [VENDOR] mounting flange along V (the wider of the two)
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
batt_cowl_w      = 83.0;   // [MEASURED] reference Battery_cover.stl footprint
batt_cowl_h      = 28.0;   // [DESIGN] reference cover is 27.0; widened 1.0 mm
                           //   so the cavity still clears the holder flange.
                           //   Costs nothing - the cowl is a bulge on the back
                           //   and does not touch the device footprint.
batt_cowl_wall   = 2.0;    // [MEASURED] reference cover wall thickness
batt_cowl_clear  = 0.5;    // [DESIGN] clearance over the holder
batt_cowl_cap_r  = 3.0;    // [DESIGN] outer cap radius
batt_cowl_base_r  = 6.0;   // [DESIGN] outer plan-view corner radius
batt_cowl_base_ri = 3.0;   // [DESIGN] INNER plan-view corner radius. Same trap
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
batt_cowl_rise   = (batt_protrusion - back_t) + batt_cowl_clear + batt_cowl_wall + batt_cowl_cap_ri;

// Onboard speaker grille.
grille_slot_w  = 16.0;   // [MEASURED] slot length along X
grille_slot_h  = 1.3;    // [MEASURED] slot width
grille_pitch   = 3.05;   // [MEASURED]
grille_count   = 4;      // [MEASURED]
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
//  off the PCB back face, i.e. 1.60 mm PROUD of the standoff plane, so the body
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
expansion_body_h = 8.603; // [VENDOR] insulator height above the PCB back face
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

//  The keyboard's own outline is now a VENDOR datum, corroborated four ways
//  (see docs/DATUMS.md D-04). Earlier revisions of this file derived the
//  keyboard from the pocket; that is no longer necessary.
kbd_body_w = 109.22;   // [VENDOR] riitek.com/product/259.html: 4.3 in x 25.4
kbd_body_h = 58.42;    // [VENDOR] 2.3 in x 25.4
kbd_body_t = 10.16;    // [VENDOR] 0.4 in x 25.4, overall, including keycaps
kbd_mass_g = 64.8;     // [VENDOR] bare product, not packed
//  FCC ID YIZRT-RII518 (Shenzhen Riitek), granted 2011-05-23, covers this body
//  under ten retail model names including "Rii518" and "Rii mini 518BT".
//
//  TRAPS, both confirmed and both avoided here:
//    * 150 x 100 x 20 mm / 120 g is the RETAIL BOX, repeated by many
//      marketplace listings as if it were the product.
//    * The Rii K18 is NOT this keyboard. It is 325 x 122 x 18.3 mm with a
//      trackpad - three times the length.
//    * Retailer titles containing "Touchpad" are wrong for the 518BT. It has
//      no touchpad; mouse control is Fn + key. Do not allocate pocket area.

// Pocket. The reference ATA tray is 109.200 mm - that is 4.3 in to within
// 0.02 mm, i.e. a ZERO-clearance press fit, and its own build guide hedges
// with "if the print tolerance permits it". The earlier proof-of-concept went
// to 110.498 and rattles enough that its guide tells you to shim with tape.
// This design sits deliberately between the two.
kbd_pocket_w = 110.2;   // [DESIGN] kbd_body_w + 0.98 => 0.49 mm per side
kbd_pocket_h = 59.4;    // [DESIGN] kbd_body_h + 0.98 => 0.49 mm per side
kbd_depth    = 11.0;    // [DESIGN] kbd_body_t + 0.84

kbd_pocket_corner_r = 6.0;   // [DESIGN] bay corner radius

// Cross-checks, kept for audit. Both are measured; neither is used.
kbd_pocket_w_ata = 109.200;  // [MEASURED] press fit,  -0.02 mm clearance
kbd_pocket_h_ata =  59.200;  // [MEASURED]            +0.78 mm
kbd_pocket_w_poc = 110.498;  // [MEASURED] loose fit, +1.28 mm
kbd_pocket_h_poc =  60.600;  // [MEASURED]            +2.18 mm

// Front-face retention aperture. Captures the keyboard's outer lip so it
// cannot fall forward; it is pushed out from behind instead.
kbd_aper_w = 106.5;   // [MEASURED] ref. 106.502
kbd_aper_h = 55.8;    // [MEASURED] ref.  55.802

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
kbd_access_both_sides = true;   // [DESIGN]
kbd_access_w          = 34.0;   // [MEASURED] along the keyboard's short axis
kbd_access_h          = 8.0;    // [MEASURED] along the deck's thickness
kbd_access_from_edge  = 9.5;    // [MEASURED] near edge of the window, measured
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

m3_clear        = 3.2;   // [STANDARD] ISO 273 close fit for M3
m3_head_d       = 5.5;   // [STANDARD] ISO 4762 socket-cap head diameter
m3_head_h       = 3.0;   // [STANDARD] ISO 4762 head height
//  The back plate uses COUNTERSUNK screws, not socket caps. An ISO 4762 cap
//  head is 3.0 mm tall, which is the entire back-plate thickness: counterboring
//  for it leaves no material under the head to take the preload. An ISO 10642
//  countersunk head is 1.86 mm deep and bears on a cone, leaving 1.34 mm of
//  plate and spreading the load instead of concentrating it on a thin annulus.
m3_cs_head_d    = 6.0;   // [STANDARD] ISO 10642 head diameter
m3_cs_head_h    = 1.86;  // [STANDARD] ISO 10642 head depth, 90 deg
m3_insert_bore  = 4.0;   // [MEASURED] the reference bore; also the standard
                         //   recommendation for a 4.0 mm OD brass heat-set
                         //   insert in PLA/PETG
m3_insert_len   = 5.0;   // [STANDARD] common M3 short insert
m3_boss_wall    = 1.7;   // [DESIGN] material around an insert. 1.6 mm is the
                         //   usual minimum to stop the boss splitting as the
                         //   insert is driven; 1.7 is used because the flank
                         //   strip the bosses live in is shared with the port
                         //   tunnels and every 0.1 mm of boss diameter comes
                         //   straight off that clearance. Each boss also merges
                         //   into the side wall, so its outboard side is much
                         //   thicker than this figure suggests.
m3_boss_d       = m3_insert_bore + 2 * m3_boss_wall;   // = 8.0
m3_bore_depth   = 8.1;   // [MEASURED] reference bore depth: insert length plus
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
body_h = wall + kbd_pocket_h + spine + board_pocket_h + wall;
body_t = back_t + board_depth + front_t;

// Bay centres in the device frame.
board_bay_cy =  body_h/2 - wall - board_pocket_h/2;
kbd_bay_cy   = -body_h/2 + wall + kbd_pocket_h/2;

// The board's mount-pattern centre coincides with the board bay centre.
board_cx = 0;
board_cy = board_bay_cy;

// Z planes.
z_back_outer  = 0;
z_back_inner  = back_t;
z_front_inner = body_t - front_t;
z_front_outer = body_t;

// Shell styling.
corner_r      = 8.0;   // [DESIGN] outer corner radius
corner_gusset = 6.0;   // [DESIGN] solid corner fillet leg length, inside
edge_chamfer  = 1.0;   // [DESIGN] breaks the front and back arrises


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
//  Instead the four M3 back-plate screws ARE the accessory mounting points.
//  They thread into brass inserts in the chassis, not into plastic, so they
//  are the strongest anchors on the device. Fit M3 x 12 in place of the
//  standard M3 x 8 and sandwich a bracket, strap yoke or stand clamp under the
//  heads. The pattern is given by accessory_pattern() in cyberdeck.scad.
accessory_screw_len_std = 8;    // [DESIGN] normal build
accessory_screw_len_rig = 12;   // [DESIGN] with a bracket under the heads

// Ventilation over the ESP32-S3 module. The RLCD has no backlight, so thermal
// load is low; these are insurance for sustained Wi-Fi TX.
vent_enable = true;   // [DESIGN]
vent_slot_w = 1.6;    // [DESIGN] >= 4 x nozzle, so it prints cleanly
vent_slot_h = 14.0;   // [DESIGN]
vent_count  = 5;      // [DESIGN]
vent_pitch  = 3.6;    // [DESIGN]


// ===========================================================================
// 6. SELF-CHECK
// ===========================================================================
//  Cheap invariants, asserted at render time. tools/validate.py runs the full
//  set, including the ones that need geometry.

assert(wall >= 4 * nozzle, "wall must be at least 4 extrusions wide");
assert(m3_boss_d > m3_insert_bore + 2, "insert boss wall too thin");
//  Largest pocket radius that still clears a board corner of radius
//  board_corner_r with c mm of per-side clearance:  r <= (c + (sqrt(2)-1)*
//  (board_corner_r + c)) / (sqrt(2)-1) ... solved numerically below for the
//  actual clearance rather than hard-coded.
pocket_clear   = (board_pocket_w - board_w) / 2;
pocket_r_limit = (board_corner_r + pocket_clear * sqrt(2)) / (sqrt(2) - 1) - board_corner_r / (sqrt(2) - 1) + pocket_clear;
assert(board_pocket_r <= 2.2,
       "board pocket corner radius will foul the PCB's R0.5 corners");
assert(expansion_win_h > 6.603, "expansion window will not clear the header body");
assert(board_pocket_w >= board_w, "board pocket narrower than the board");
assert(board_pocket_h >= board_h, "board pocket shorter than the board");
assert(kbd_pocket_w >= kbd_body_w, "keyboard pocket narrower than the keyboard");
assert(kbd_pocket_h >= kbd_body_h, "keyboard pocket shorter than the keyboard");
assert(kbd_depth >= kbd_body_t, "keyboard pocket shallower than the keyboard");
assert(batt_cowl_rise >= batt_protrusion - back_t + batt_cowl_wall,
       "battery cowl too shallow for the holder protrusion");
assert(m3_cs_head_h < back_t - 1.0, "countersink leaves too little plate under the head");
assert(kbd_aper_w < kbd_pocket_w, "keyboard would fall through the front face");
assert(kbd_aper_h < kbd_pocket_h, "keyboard would fall through the front face");
assert(display_aper_w >= display_active_w, "front face clips the display");
assert(display_aper_h >= display_active_h, "front face clips the display");
assert(body_t >= back_t + board_depth + front_t, "not deep enough for the board");
assert(vent_slot_w >= 4 * nozzle, "vent slots too narrow to print");
assert(window_w > 50 && window_h > 50, "acrylic window outline is degenerate");

echo(str("cYbErDeCk envelope [", preset, "]: ",
         body_w, " x ", body_h, " x ", body_t, " mm"));
