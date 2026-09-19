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
//  The single most load-bearing datum set in the project. The mounting-hole
//  pattern below was recovered INDEPENDENTLY from two unrelated published
//  enclosure designs and both derivations agree to 0.001 mm. See
//  tools/measure_reference.py section 2 and docs/DATUMS.md D-01.

// --- outline ---------------------------------------------------------------
board_w        = 92.5;    // [VENDOR][PROVISIONAL] PCB width  (along +X)
board_h        = 70.1;    // [VENDOR][PROVISIONAL] PCB height (along +Y)
board_depth    = 13.0;    // [MEASURED] pocket depth of the reference design,
                          //   i.e. the clear volume the board assembly needs
                          //   from the front inner face rearward. The vendor
                          //   quotes 13.5 mm total assembly thickness, which
                          //   includes the display glass sitting in the
                          //   front-face aperture, so 13.0 mm of pocket is
                          //   consistent, not contradictory.

// The pocket is specified DIRECTLY from the reference rather than as
// board + clearance, because the reference is a built and validated design and
// board_w/board_h are still [PROVISIONAL].
board_pocket_w = 94.6;    // [MEASURED] ref. 94.519, rounded up
board_pocket_h = 71.2;    // [MEASURED] ref. 71.116, rounded up

// --- mounting --------------------------------------------------------------
board_mount_pitch_x = 85.5;   // [MEASURED] two independent derivations agree
board_mount_pitch_y = 62.1;   // [MEASURED] two independent derivations agree
board_screw         = 2.5;    // [VENDOR] M2.5, supplied with the board
board_screw_clear   = 2.7;    // [MEASURED] bore used by both reference designs
board_boss_d        = 6.0;    // [DESIGN] standoff outer diameter
board_standoff_h    = 1.2;    // [DESIGN] lifts the PCB off the pocket floor to
                              //   clear rear-side solder joints

// --- display ---------------------------------------------------------------
display_active_w  = 85.344;  // [DERIVED] 400 px x (4.2 in / hypot(400,300) px)
display_active_h  = 64.008;  // [DERIVED] 300 px x the same pixel pitch
display_aper_w    = 86.8;    // [MEASURED] reference bezel inner aperture
display_aper_h    = 65.6;    // [MEASURED] reference bezel inner aperture
display_aper_draft = 1.45;   // [MEASURED] the reference opens out from 86.8 x
                             //   65.6 to 68.5 x 89.7 across its 2.0 mm bezel.
                             //   That is a 1.45 mm per-side flare; reproduced
                             //   here so the frame does not visually clip the
                             //   panel at an angle.

// Display centre, relative to the board mounting-pattern centre.
display_off_x = 0.0;   // [MEASURED] aperture is centred on the mount pattern
display_off_y = 0.0;   // [MEASURED]

// Optional 2 mm acrylic window (laser-cut). Outline taken from the reference
// DXF, which is an exact 2D source: 70.8163 x 94.2193 mm. Note the reference
// DXF is authored in the board's own portrait frame, so its X/Y are
// transposed relative to this file's device frame.
window_w = 94.2193;  // [MEASURED] from plexiglass.dxf, transposed
window_h = 70.8163;  // [MEASURED] from plexiglass.dxf, transposed
window_t = 2.0;      // [VENDOR] nominal cast-acrylic sheet
window_corner_r = 1.0;  // [MEASURED] the DXF corners are 4-segment chamfers
                        //   approximating a ~1 mm radius

// --- edge features, positions relative to the mount-pattern centre ---------
//  All recovered by sectioning the reference caseback normal to the board's
//  long axis; see tools/measure_reference.py and docs/DATUMS.md D-07.

// Three tactile buttons on the TOP edge (PWR / BOOT / KEY).
button_pitch   = 10.0;   // [MEASURED] exactly 10.000 between centres
button_count   = 3;      // [VENDOR] PWR, BOOT, KEY
button_aper_w  = 5.4;    // [MEASURED] aperture width  (along X)
button_aper_h  = 4.4;    // [MEASURED] aperture height (along Z)
button_cap_w   = 5.2;    // [MEASURED] reference cap cross-section
button_cap_h   = 4.0;    // [MEASURED]
button_flange  = 0.4;    // [MEASURED] per-side retaining flange behind the wall
button_z_below_front = 6.9;  // [MEASURED] button centre, below the front
                             //   face's INNER surface

// Dual microphone array, also on the top edge, symmetric about board centre.
mic_offset_x = 32.5;   // [MEASURED] exactly +/-32.500 from the mount centre
mic_aper_w   = 5.4;    // [MEASURED]
mic_aper_h   = 2.5;    // [MEASURED]
mic_z_below_front = 6.55;  // [MEASURED]

// 18650 holder, on the board's rear face. Runs ACROSS the device.
batt_bay_w = 79.0;     // [MEASURED] agrees exactly between both references
batt_bay_h = 22.1;     // [MEASURED] agrees exactly between both references
batt_off_x = 0.0;      // [MEASURED] centred on the board
batt_off_y = -19.4;    // [MEASURED] offset toward the keyboard

// --- 18650 battery cowl -----------------------------------------------------
//  An 18650 cell is 18.4 mm in diameter. The deck is 18.6 mm thick overall, of
//  which only 13.0 mm is board pocket, so the cell CANNOT be contained: it must
//  protrude through the back. The reference handles this with a separate
//  clip-on cover; this design makes the cowl integral to the back plate, which
//  removes a part, removes two clips that can break, and turns the bulge into a
//  grip ridge that falls under the fingers when the deck is held in two hands.
//
//  Height is taken from the reference cover rather than from the cell, because
//  the reference cover's datum plane (the caseback's outer face) is the same
//  plane as this back plate's outer face - so it transfers exactly, without
//  needing to know where the PCB sits inside the board.
batt_cowl_enable = true;   // [DESIGN]
batt_cowl_w      = 83.0;   // [MEASURED] reference Battery_cover.stl footprint
batt_cowl_h      = 28.0;   // [DESIGN] reference cover is 27.0; widened by
                           //   1.0 mm so the cavity still clears the cell at
                           //   the depth where the cell is widest. Costs
                           //   nothing: the cowl is a bulge on the back and
                           //   does not touch the device footprint.
batt_cowl_rise   = 12.0;   // [MEASURED] reference cover dome height above the
                           //   caseback outer face
batt_cowl_wall   = 2.0;    // [MEASURED] reference cover wall thickness
batt_cowl_cap_r  = 3.0;    // [DESIGN] the cowl's sides stay vertical until the
                           //   last 3 mm. A cowl that tapers over its whole
                           //   rise pinches the cell - see lib/util.scad ridge().
cell_dia_max     = 18.6;   // [STANDARD] 18650, worst case: protected cells run
                           //   to 18.6 mm; bare flat-tops are 18.4 mm
cell_len_max     = 69.0;   // [STANDARD] protected/button-top 18650 worst case

// Speaker, fired through the back plate.
grille_slot_w  = 16.0;   // [MEASURED] slot length, along X
grille_slot_h  = 1.3;    // [MEASURED] slot width
grille_pitch   = 3.05;   // [MEASURED]
grille_count   = 4;      // [MEASURED]
grille_off_x   = 0.0;    // [MEASURED]
grille_off_y   = 16.0;   // [MEASURED] above the board centre

// 2 x 8, 2.54 mm expansion header. Access window in the back plate.
expansion_win_w = 24.0;  // [MEASURED] reference access window
expansion_win_h = 29.4;  // [MEASURED]
expansion_pitch = 2.54;  // [VENDOR]
expansion_rows  = 2;     // [VENDOR]
expansion_cols  = 8;     // [VENDOR]
// Reference card outline, for the clearance check only.
expansion_card_w = 76.2;    // [VENDOR] SolarLink reference card
expansion_card_h = 48.26;   // [VENDOR] SolarLink reference card

// --- ports whose EDGE is known but whose LABEL is inferred -----------------
//  Two openings exist in the reference's right-hand board-bay wall. Their
//  geometry is measured; which is USB-C and which is the microSD slot has not
//  been confirmed against vendor documentation, so both are cut oversize.
//  See docs/DATUMS.md "Open items" O-02.
side_port_a_w = 17.0;   // [MEASURED][PROVISIONAL] opening, along Y
side_port_a_y = 0.0;    // [MEASURED][PROVISIONAL] centre rel. mount centre
side_port_b_w = 15.0;   // [MEASURED][PROVISIONAL]
side_port_b_y = 19.25;  // [MEASURED][PROVISIONAL]
side_port_h   = 6.0;    // [MEASURED][PROVISIONAL] opening height, along Z
side_port_z_below_front = 6.5;  // [MEASURED][PROVISIONAL]

// The reference has these openings on ONE side only: scanning the opposite
// board-bay wall over the same depth range finds no openings at all. They are
// reproduced on the +X side here.
side_port_side = 1;     // [MEASURED] +1 = +X wall, -1 = -X wall

// The board is narrower than the shell interior, because the KEYBOARD sets the
// device width. That leaves a solid flank either side of the board bay, so a
// side port is not a hole in a wall - it is a tunnel through that flank. This
// is the depth it has to cross.
// (value is derived in cyberdeck.scad from the bay geometry)


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
m3_boss_wall    = 2.0;   // [DESIGN] material around an insert. 2.0 mm is the
                         //   usual minimum to stop the boss splitting as the
                         //   insert is driven.
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
assert(board_pocket_w >= board_w, "board pocket narrower than the board");
assert(board_pocket_h >= board_h, "board pocket shorter than the board");
assert(kbd_pocket_w >= kbd_body_w, "keyboard pocket narrower than the keyboard");
assert(kbd_pocket_h >= kbd_body_h, "keyboard pocket shorter than the keyboard");
assert(kbd_depth >= kbd_body_t, "keyboard pocket shallower than the keyboard");
assert(batt_cowl_rise >= cell_dia_max * 0.5, "battery cowl too shallow for a cell");
assert(m3_cs_head_h < back_t - 1.0, "countersink leaves too little plate under the head");
assert(kbd_aper_w < kbd_pocket_w, "keyboard would fall through the front face");
assert(kbd_aper_h < kbd_pocket_h, "keyboard would fall through the front face");
assert(display_aper_w >= display_active_w, "front face clips the display");
assert(display_aper_h >= display_active_h, "front face clips the display");
assert(body_t >= back_t + board_depth + front_t, "not deep enough for the board");
assert(vent_slot_w >= 4 * nozzle, "vent slots too narrow to print");
assert(abs(side_port_side) == 1, "side_port_side must be +1 or -1");

echo(str("cYbErDeCk envelope [", preset, "]: ",
         body_w, " x ", body_h, " x ", body_t, " mm"));
