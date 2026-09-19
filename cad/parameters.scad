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
batt_cowl_w      = 83.0;   // [MEASURED] reference Battery_cover.stl footprint
batt_cowl_h      = 30.0;   // [DESIGN] reference cover is 27.0. Widened so the
                           //   cavity still clears the 22.1 mm holder flange
                           //   AFTER the crown has begun easing in. Costs
                           //   nothing in footprint - the cowl is a bulge on
                           //   the back, not part of the plan form.
batt_cowl_wall   = 2.0;    // [MEASURED] reference cover wall thickness
batt_cowl_clear  = 0.5;    // [DESIGN] clearance over the holder
//  The cowl is a swelling BLENDED OUT of the back panel, not a box with a cap
//  sitting on it. batt_cowl_foot is the tangent fillet where it meets the
//  panel, which is what removes the base line; batt_cowl_cap is how much the
//  section eases in toward the crown, as a fraction of the minor axis.
batt_cowl_foot   = 3.2;    // [DESIGN] foot fillet, blended into the panel
batt_cowl_foot_f = 0.20;   // [DESIGN] fraction of rise the foot fillet takes
batt_cowl_cap    = 0.34;   // [DESIGN] crown easing, fraction of the minor axis
batt_cowl_crown  = 0.50;   // [DESIGN] fraction of rise before the crown starts.
                           //   Not styling: the cell occupies the lower half of
                           //   the cavity, so the sides must stay parallel
                           //   until they are clear of it.
batt_cowl_cap_r  = 3.0;    // [DESIGN] retained for the cavity's ridge()
batt_cowl_base_r  = 6.0;   // [DESIGN] outer plan-view corner radius
//  CORRECTED RATIONALE. This used to say "the holder's corners are R2.0".
//  They are not: in Waveshare's STEP the holder's plan form is exactly square
//  at every height through the body - 0.0000 mm deviation from its bounding
//  rectangle, 504 of 576 edges are straight lines, and every circle in the
//  part lies on the cell axis. The value is right for a different reason: a
//  cavity corner must stay small enough not to bite into a square-cornered
//  body sitting inside it. The number stands; the reason it was given for did
//  not. See docs/DATUMS.md C-21.
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
//  ... plus headroom for the crown itself, which the earlier straight-sided
//  ridge did not need. Without it the crown starts inside the cell's envelope.
batt_cowl_head   = 2.5;    // [DESIGN] clear rise above the holder for the crown
batt_cowl_rise   = (batt_protrusion - back_t) + batt_cowl_clear + batt_cowl_wall + batt_cowl_head;

// Onboard speaker grille.
//  Waveshare's own grille field is 14.70 (U) x 10.45 (V), and the reference
//  enclosure reproduces the 10.45 with four 1.3 mm slots on a 3.05 pitch. The
//  acoustic opening is kept inside that same field, but restyled as a finer,
//  more numerous set: five slots at 1.2 mm on a 2.3125 pitch span exactly the
//  same 10.45 mm. Finer perforation on a tighter pitch is the Braun grille
//  idiom, and it is the one perforated element on the object.
grille_slot_w  = 14.0;    // [DESIGN] within Waveshare's 14.70 field
grille_slot_h  = 1.2;     // [DESIGN] 3 extrusions
grille_pitch   = 2.3125;  // [DESIGN] 4 gaps x 2.3125 + 1.2 = 10.45 exactly
grille_count   = 5;       // [DESIGN]
grille_field_h = 10.45;   // [VENDOR] Waveshare's grille height, matched exactly
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
kbd_pocket_w = 110.2;   // [DESIGN]
kbd_pocket_h =  59.4;   // [DESIGN]
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
//  RETAGGED [MEASURED] -> [DESIGN]. Neither of these is a measured value: the
//  window is an outward-rounded ENVELOPE that CONTAINS the reference notch
//  (32.074 x 7.596), not a reading of it. Calling a chosen envelope a
//  measurement is exactly the conflation this file keeps correcting.
kbd_access_w          = 34.0;   // [DESIGN] along the keyboard's short axis
kbd_access_h          = 8.0;    // [DESIGN] along the deck's thickness
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
aper_blend_kbd  = kbd_body_corner_r_max - min(kbd_lip_x, kbd_lip_y);  // = 10.2
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
//  Instead the four M3 back-plate screws ARE the accessory mounting points.
//  They thread into brass inserts in the chassis, not into plastic, so they
//  are the strongest anchors on the device. Fit M3 x 12 in place of the
//  standard M3 x 8 and sandwich a bracket, strap yoke or stand clamp under the
//  heads. The pattern is given by accessory_pattern() in cyberdeck.scad.
accessory_screw_len_std = 8;    // [DESIGN] normal build
accessory_screw_len_rig = 12;   // [DESIGN] with a bracket under the heads

// Ventilation over the ESP32-S3 module. The RLCD has no backlight, so thermal
// load is low; these are insurance for sustained Wi-Fi TX.


// ===========================================================================
// 6. SELF-CHECK
// ===========================================================================
//  Cheap invariants, asserted at render time. tools/validate.py runs the full
//  set, including the ones that need geometry.

assert(wall >= 4 * nozzle, "wall must be at least 4 extrusions wide");
assert(shell_screw_boss_d > shell_screw_insert_bore + 2, "insert boss wall too thin");
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
assert(batt_cowl_rise >= batt_protrusion - back_t + batt_cowl_wall,
       "battery cowl too shallow for the holder protrusion");
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
assert(wall - edge_soft >= 5 * nozzle,
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
