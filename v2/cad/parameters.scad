// ===========================================================================
//  cYbErDeCk v2 - parameters.scad
//  SINGLE SOURCE OF TRUTH for every dimension in the v2 enclosure.
// ===========================================================================
//
//  RULES OF THIS FILE
//  ------------------
//  1. No number appears anywhere else in the v2 CAD. A bare dimension in a
//     model file is a bug.
//  2. Every datum carries a provenance tag:
//
//       [MEASURED]    recovered from a reference artefact (a vendor STEP, a
//                     rendered mesh) by a tool in v2/tools. Re-derivable.
//       [VENDOR]      published by the manufacturer.
//       [DATASHEET]   from a component datasheet, with page.
//       [DERIVED]     arithmetic on the above. The arithmetic is shown.
//       [STANDARD]    a published standard or a fitting spec.
//       [DESIGN]      a choice made here. Rationale inline.
//       [PROVISIONAL] not yet corroborated. Must be caliper-checked before a
//                     final cut. v2/tools/check_cad.py reports these.
//
//  3. The SVG layout specs in v2/layouts are the same geometry in another
//     language. v2/tools/check_cad.py compares this file against the datums
//     the layout tool emits and fails if they have drifted apart.
//
//  COORDINATE SYSTEM
//  -----------------
//      origin  = centre of the deck footprint, on the carrier's TOP face
//      +X      = width,  left -> right as the user sits at it
//      +Y      = depth,  keyboard at -Y (near), display at +Y (far)
//      +Z      = up, out of the carrier towards the user
//
//  This is a DESK instrument. It lies flat. Z is the stacking axis:
//
//      back panel   z = -(carrier_t + back_t) .. -carrier_t
//      carrier      z = -carrier_t .. 0
//      components   z = 0 upward
//      acrylic      z = acrylic_z0 .. acrylic_z0 + acrylic_t
//
// ===========================================================================

// ---------------------------------------------------------------------------
// 0. BUILD CONFIGURATION
// ---------------------------------------------------------------------------

$fn = 64;                       // 64 for exports, 32 while iterating
eps = 0.01;                     // coplanar-face nudge

nozzle      = 0.40;             // [DESIGN] carrier prints on a 0.4
layer_h     = 0.20;             // [DESIGN] 0.2 for a readable top face

// ---------------------------------------------------------------------------
// 1. THE GRID
// ---------------------------------------------------------------------------
//  Adafruit lay boards out in inches and the keyboard's key unit is 0.75 in,
//  so the grid is imperial. Finding which imperial grid took three attempts
//  and the two rejected ones are recorded in v2/docs/envelope.md, because the
//  failure mode -- checking a module against the one dimension it was derived
//  from -- is the instructive part.

grid        = 0.635;            // [DERIVED] 0.025 in. Divides every board
                                //   dimension AND the half-pitches that an
                                //   even, centred control row produces.
grid_board  = 1.270;            // [DERIVED] 0.050 in, Adafruit's own grid
U           = 19.050;           // [VENDOR] 0.75 in, the MX key unit

// ---------------------------------------------------------------------------
// 2. THE DECK
// ---------------------------------------------------------------------------
//  Solved for the wall so the footprint is EXACTLY 8:5, the same ratio the
//  control grid has:
//      body_w = 288.35 + 2w     body_h = 106.05 + band + w
//      5*body_w == 8*body_h  =>  w = 8.125
//  5 x 304.600 = 8 x 190.375 = 1523.000, exactly.

wall        = 8.125;            // [DERIVED] sides and rear; overbuilt by design
front_wall  = 4.400;            // [DESIGN] the near edge, where the heels of
                                //   the hands land. The one wall that must NOT
                                //   be overbuilt.
spine       = 3.200;            // [DESIGN] the rib between keyboard and band

interior_w  = 288.350;          // [DERIVED] keyboard pocket width
body_w      = interior_w + 2*wall;      // 304.600
band        = 4 * U;            // [DERIVED] 76.200, the NeoSlider board on end
body_h      = front_wall + 98.450 + spine + band + wall;   // 190.375

field       = 15 * U;           // [DERIVED] 285.750, the K9 Pro switch field

// ---------------------------------------------------------------------------
// 3. THE LAYER STACK
// ---------------------------------------------------------------------------

acrylic_t   = 12.700;           // [DESIGN] 0.5 in clear, laser cut
carrier_t   = 6.000;            // [PROVISIONAL] the printed floor's thickness
back_t      = 3.000;            // [PROVISIONAL] laser-cut back panel

//  The acrylic's underside is set by the TALLEST thing under it, and since the
//  slab runs over the display with no cutout that is the display module.
//  Waveshare publish 14.1 total and 4.35 LCD-to-acrylic; their dimension
//  drawing carries an unlabelled 7.00; the three close on a 2.75 middle term,
//  making the display assembly 5.53 above its PCB front face. 6.350 (0.25 in)
//  clears it by 1.270.
acrylic_z0  = 6.350;            // [DERIVED] v2/parts/knobs.md 1.1

//  The carrier is a floor with a perimeter rim. The rim has to span the gap
//  between the floor's top face and the acrylic's underside, because that gap
//  is where the boards and the wiring live and the acrylic has to land on
//  something at its edge.
carrier_rim_h = acrylic_z0;     // [DERIVED] 6.350, the rim above the floor
carrier_h     = carrier_t + carrier_rim_h;      // 12.350 overall

//  So the deck is, from the desk up:
//      back panel   3.000
//      carrier      12.350   (6.000 floor + 6.350 rim)
//      acrylic      12.700
//                   ------
deck_t        = back_t + carrier_h + acrylic_t;  // 28.050


// ---------------------------------------------------------------------------
// 4. THE CONTROL FIELD
// ---------------------------------------------------------------------------

band_y0     = -body_h/2 + front_wall + 98.450 + spine;  // band's near edge
band_mid    = band_y0 + band/2;

knob_d      = 25.400;           // [DESIGN] 1 in brim. pitch + knob = 63.500
                                //   against the display's 63.600 active
                                //   height, so the knobs stand exactly as
                                //   tall as the screen.
pitch       = 2 * U;            // [DERIVED] 38.100, across and down
grid_x      = 36.195;           // [DERIVED] 57 x grid: the position that both
                                //   balances the end margins and lands every
                                //   knob column on the grid
grid_w      = 2*pitch + knob_d; // [DERIVED] 101.600 = 4.000 in, 8:5 exactly
grid_h      = pitch + knob_d;   // [DERIVED]  63.500 = 2.500 in

gap         = 1.5 * U;          // [DERIVED] 28.575, both gaps

// ---------------------------------------------------------------------------
// 5. THE RECESSED KNOB AND ITS BORE
// ---------------------------------------------------------------------------
//  A finger cannot enter the bore: MIL-STD-1472H fig 57 puts minimal finger
//  access to the first joint at 32 mm and the annular gap is 2.00. So the grip
//  is entirely the proud part and the rim, which makes the knob a flanged plug
//  rather than a cylinder.  v2/parts/knobs.md 2.3

knob_body_d = 18.000;           // [DESIGN] the part inside the bore
bore_d      = 22.000;           // [DESIGN] 2.00 annular gap
knob_brim_d = knob_d;           // [DERIVED] the brim IS the panel's circle
knob_proud  = 6.350;            // [DESIGN] 0.25 in standing above the slab
knob_h      = U;                // [DESIGN] 19.050, one key unit
knob_bore_shaft_d = 6.000;      // [DATASHEET] PEC11 D-shaft
knob_counterbore_d = 8.000;     // [DESIGN] clears the bushing
knob_counterbore_h = 5.000;     // [DESIGN] its floor stops switch overtravel

// ---------------------------------------------------------------------------
// 6. FASTENERS
// ---------------------------------------------------------------------------
//  CNC Kitchen standard, as the asset library publishes it.  [STANDARD]

insert_m3_hole_d  = 4.000;
insert_m3_depth   = 6.700;
insert_m3_wall    = 1.600;
insert_m25_hole_d = 3.200;      // M2.5 takes the M2 line in that table
insert_m25_depth  = 4.000;

screw_m3_clear_d  = 3.400;      // [STANDARD] dia + 0.4
screw_m25_clear_d = 2.900;

screw_x     = 144.780;          // [DERIVED] 228 x grid. The only screw line
                                //   that is symmetric, on the grid, and leaves
                                //   M3 the 5.10 edge distance acrylic wants.
                                //   M4 leaves 5.27 against 6.75 and does not
                                //   fit; that is why these are M3.
screw_dy    = 31.750;           // [DESIGN] 1.25 in, three per side

//  The back panel fastens UP into the same perimeter rim, from below, at the
//  same x line so the fastener geometry reads as one system front and back.
//  Different y, so the two sets of inserts do not meet inside a 12.350 rim:
//  an M3 insert is 6.700 deep and two of them nose to nose need 13.400.
back_screw_y = [-20.000, -55.000, -85.000];     // [DESIGN] in the key region
acrylic_edge_min = 1.5 * screw_m3_clear_d;   // [STANDARD] 5.100

// ---------------------------------------------------------------------------
// 7. BOARD FOOTPRINTS, all [MEASURED] from vendor STEP unless noted
// ---------------------------------------------------------------------------

pcb_t         = 1.570;          // every Adafruit breakout

pat_b         = [76.200, 21.590];   // NeoSlider / NeoKey / Quad Rotary
pat_b_holes   = [38.100, 16.510];
pat_a         = 25.400;             // the 1 inch breakout
pat_a_holes   = 20.320;
hole_d        = 2.500;

enc_above     = 21.500;         // [DATASHEET] PCB face to shaft tip, 6.5 + 15.0
enc_body_h    = 6.500;          // [DATASHEET] PCB face to mounting surface
enc_bushing   = 5.000;          // [DATASHEET] pec11 p3, LB for a 15 mm shaft
slider_above  = 15.500;         // [MEASURED] shank tip from the PCB face
slider_body   = 6.000;          // [MEASURED] pot body front face
slider_shank  = 5.000;          // [MEASURED] shank width

disp_board    = [93.900, 70.300];   // pocket, = board + 2*fit
disp_active   = [84.800, 63.600];   // [VENDOR]
disp_x        = grid_x - grid_w/2 - gap - disp_board[0]/2;   // -90.130

slide_x       = grid_x + grid_w/2 + gap + pat_b[1]/2;        // 126.365

feather       = [22.860, 50.800];   // [STANDARD] Feather, turned 90 deg
feather_x     = 101.600;            // [DERIVED] 160 x grid
feather_holes_usb  = 19.050;    // [VENDOR] NOT a rectangle: 19.050 at the USB
feather_holes_hdmi = 17.780;    //   end, 17.780 at the HDMI end,
feather_holes_span = 45.720;    //   45.720 between columns

// ---------------------------------------------------------------------------
// 8. THE KEYBOARD, STRIPPED
// ---------------------------------------------------------------------------
//  The K9 Pro is being taken out of its case and its plate + PCB + switches
//  mounted directly to the carrier. These are the stock case's own numbers,
//  measured by sectioning Keychron's STEP; the internals' own geometry is
//  resolved in v2/parts/keyboard-strip.md and lands in section 8b.

kbd_switch_field = [285.750, 95.250];   // [DERIVED] 15u x 5u at 19.050
kbd_case_w    = 287.000;        // [VENDOR] the stock case, for reference
kbd_case_d    =  97.500;        // [VENDOR]
kbd_wedge_deg =   3.374;        // [MEASURED] 26 sections of the STEP, zero
                                //   deviation from a straight line at every one
kbd_pocket    = [288.350, 98.450];      // [DERIVED] v1's fit rule
kbd_top_y     = -body_h/2 + front_wall + kbd_pocket[1];

// ---------------------------------------------------------------------------
// 9. WIRE CHANNELS IN THE CARRIER'S TOP FACE
// ---------------------------------------------------------------------------

chan_qt_w     = 4.000;          // [DESIGN] a STEMMA QT cable, 4 wires
chan_qt_d     = 2.000;
chan_pot_w    = 3.000;          // [DESIGN] a pot's 3 wires
chan_pot_d    = 2.000;

// ---------------------------------------------------------------------------
// 10. THE FADER'S SLOT
// ---------------------------------------------------------------------------
//  The NeoSlider's cap is recessed into the slab the way the knobs are, so the
//  slab needs a slot rather than a bore.
//
//  slider_travel is the one number the vendor STEP CANNOT give: its cover is
//  modelled as solid material across the full 69 mm of the pot body and the
//  shank simply passes through it, so there is no opening in the file to
//  measure. Every z level between the cover and the shank tip returns one run,
//  the shank, and no gap anywhere. Calipers on the real part settle it.
slider_travel = 40.000;         // [PROVISIONAL] v2/parts/parts.md section 3
slider_cap_w  = 10.000;         // [DESIGN] the printed cap's width
slider_slot_w = slider_cap_w + 2.000;             // [DESIGN] 2.00 annular gap
slider_slot_l = slider_travel + slider_cap_w + 2.000;

// ---------------------------------------------------------------------------
// 11. COSMETIC
// ---------------------------------------------------------------------------
corner_r    = 2.000;            // [DESIGN] just enough to kill a cutting edge
                                //   on a 12.70 slab. Square would be more
                                //   brutal and less kind to a forearm.
