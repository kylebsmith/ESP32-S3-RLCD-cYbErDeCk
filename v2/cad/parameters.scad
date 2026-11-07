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

$fn = 96;                       // [DESIGN] NOT cosmetic. At 64 every circle
                                //   exports as a 64-gon whose vertices sit ON
                                //   the nominal circle: the bore's vertex
                                //   diameter is 22.0001 and its INSCRIBED
                                //   diameter is 21.9735, so the material is
                                //   0.013 proud at every chord and a checker
                                //   that measures vertex radii reads 22.000
                                //   and passes. 96 puts the sagitta at 0.006
                                //   at r = 11.000. v2/docs/fabrication.md 0.10
eps = 0.01;                     // coplanar-face nudge

nozzle      = 0.40;             // [DESIGN] carrier prints on a 0.4
layer_h     = 0.120;            // [DESIGN] the shipping part. stack.md 5.5
                                //   said 0.120 and this file said 0.200 -- a
                                //   straight contradiction between two current
                                //   documents, resolved toward 0.120 because
                                //   the carrier's top face IS the visible
                                //   wiring plane and gets ironed.
layer_h_fit = 0.200;            // [DESIGN] fit checks and test corners only

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
U           = 19.050;           // [VENDOR] 0.75 in, the MX key unit, and
                                //   the module the CONTROL field is laid on
kbd_U       = 19.000;           // [CAD-EXACT] and it is NOT U. All 61 of the
                                //   K9 Pro's switch holes land on an exact
                                //   19.000 lattice in both axes, read off the
                                //   analytic hole axes in Keychron's own
                                //   assembly. This repo believed 0.75 in
                                //   across five documents and was 0.750 mm out
                                //   on the field. v2/parts/keyboard-strip.md 1.6

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

//  There was a `field = 15 * U` here, commented "the K9 Pro switch field".
//  It was wrong twice: the keyboard's pitch is 19.000, not 19.050, and nothing
//  in the CAD ever read the value. The keyboard's real field is in section 8.

// ---------------------------------------------------------------------------
// 3. THE LAYER STACK
// ---------------------------------------------------------------------------
//  THIS SECTION WAS RE-DERIVED FROM SCRATCH, because the number it used to
//  rest on was wrong by 2.780 mm.
//
//  v2/parts/knobs.md 1.1 read Waveshare's 4.35 / 7.00 / 2.75 thickness chain
//  in the wrong order and concluded the display stands 5.53 mm above its PCB's
//  front face. Measured against the solids it stands 2.750, the middle 7.000
//  is literally the 7 mm standoff, and the chain closes on 14.100 exactly.
//  acrylic_z0 = 6.350 was "the figure that clears the display by 1.270" and so
//  it was derived from a 2.78 mm error. A conclusion that survives a 2.78 mm
//  correction was not being tested by its own arithmetic, so the whole stack
//  is re-derived here from measured part heights.
//  v2/parts/board-models.md 0 items 7 and 8, and 2.4.
//
//  THE RULE THAT SETS IT: every breakout's REAR face lands on its mounting
//  bosses at z = 0, the carrier's top face. That face is the visible wiring
//  plane -- the brief asks for the routing to be seen through the acrylic --
//  so it stays a plane, and the boards stand on it rather than sinking into
//  wells that would fragment it. The brief also asks for the boards to mount
//  "by the 4 screws on the corners", which is what a boss quad is.
//
//  So the governing height is the tallest thing standing on a board:
//
//      part                             above its PCB   above the plane
//      5880 encoder body, O16.7           6.500 [MEAS]      8.070
//      5295 NeoSlider pot body            6.500 [MEAS]      8.070
//      5710 Feather, HDMI receptacle      6.219 [MEAS]      7.789
//      ----------------------------------------------------------------
//      tallest                                              8.070
//
//  and acrylic_z0 has to clear 8.070. On the grid, 9.525 (15 x grid) does it
//  with 1.455 of air. 8.890 would leave 0.820, which is less than the slab's
//  own flatness allowance, and the next one down collides.
//
//  WHAT CAME OUT OF IT, AND IT WAS NOT AIMED AT:
//
//      back panel      3.175   =  1/8 in
//      carrier floor   6.350   =  1/4 in
//      carrier rim     9.525   =  3/8 in
//      acrylic        12.700   =  1/2 in
//                     ------
//      deck           31.750   =  1 1/4 in  EXACTLY
//
//  1 : 2 : 3 : 4 in eighths of an inch, summing to 1.25 in. Every term is
//  independently justified below -- none of them was chosen to make the
//  pattern -- and the pattern is the check that they agree. It also means the
//  structure BELOW the wiring plane (3.175 + 6.350 = 9.525) is exactly as
//  thick as the structure above it (9.525), which is why the deck reads
//  balanced in section.

acrylic_t   = 12.700;           // [DESIGN] 0.5 in clear, laser cut. The brief.
carrier_rim_h = 9.525;          // [DERIVED] 0.375 in = 15 x grid. Clears the
                                //   8.070 mm tallest control body by 1.455.
                                //   This IS acrylic_z0; see below.
carrier_t   = 6.350;            // [DERIVED] 0.25 in = 10 x grid, and it is the
                                //   smallest eighth of an inch that works. The
                                //   three deepest things in the floor are the
                                //   562 pot's body (4.120 below the plane, so
                                //   2.230 of floor under it), the Waveshare's
                                //   P1 header (4.845, so 1.505) and the
                                //   keyboard frame's skirt groove (3.705, so
                                //   2.645). At 5.715 the pot breaks through.
back_t      = 3.175;            // [DERIVED] 0.125 in = 5 x grid. Was 3.000
                                //   [PROVISIONAL] and had no reason; this one
                                //   has two. It is the second rigid plate that
                                //   fights the carrier's bow from below
                                //   (v2/docs/fabrication.md 2.5 item 1, which
                                //   says it must not be REDUCED), and it has
                                //   to swallow the display's four M2.5 screw
                                //   heads in O5.500 clearance holes: a cap
                                //   head is 2.500 tall and 3.175 leaves 0.675,
                                //   so the deck still sits flat.

//  The acrylic's underside is the rim's top. One number, two names, because
//  the rim exists precisely to span from the wiring plane to the acrylic: that
//  gap is where every board and all the wiring live, and the slab has to land
//  on something at its edge.
acrylic_z0  = carrier_rim_h;    // [DERIVED] 9.525
carrier_h   = carrier_t + carrier_rim_h;        // 15.875 overall
deck_t      = back_t + carrier_h + acrylic_t;   // 31.750 = 1.250 in
deck_top_z  = acrylic_z0 + acrylic_t;           // 22.225
deck_bot_z  = -(carrier_t + back_t);            // -9.525

//  WHAT EACH BOARD'S Z IS, measured, so the carrier's pockets are derived and
//  not guessed. All relative to the wiring plane at z = 0.
//  v2/parts/board-models.md, and the 02_controls meshes in the house library.
pcb_t          = 1.570;         // [MEASURED] every Adafruit breakout. Declared
                                //   HERE and not in section 7, because section
                                //   3 uses it: OpenSCAD evaluates assignments
                                //   in order and a forward reference is undef,
                                //   which renders, exports, and is silently
                                //   wrong. export.py treats that warning as
                                //   fatal; this is the ordering that avoids it.
board_rear_z   = 0;             // [DESIGN] the rule above: rear face on bosses
pcb_front_z    = pcb_t;         // 1.570

disp_standoff  = 7.030;         // [MEASURED] SMTSO-M2.5-7ET, O5.500 female
                                //   M2.5, standing off the display's REAR
                                //   face. The enclosure screws up into these
                                //   and supplies no bosses of its own, so
                                //   these ARE the mount -- and Waveshare's own
                                //   stand base, which bolts to the same four,
                                //   is DISCARDED. The carrier is the stand
                                //   base. That is also why the pocket below is
                                //   sized to the PCB and not to the base.
disp_hdr_below = 8.700;         // [MEASURED] P1, the 2x8 expansion header,
                                //   behind the PCB's rear face. It is on the
                                //   REAR, facing away from the visible wiring
                                //   plane, so the display's 16-way is the one
                                //   harness that has to dive under the floor.
                                //   It comes straight back up through the same
                                //   slot; about 10 mm of it is hidden.
disp_18650_below = 15.200;      // [MEASURED] the 18650 holder, and it is the
                                //   deepest feature on the board by 6.500. v1
                                //   built a cowl around this fact.
disp_mount_z   = -3.175;        // [DESIGN] -0.125 in. The standoffs land here,
                                //   putting the PCB's rear face at 3.855, its
                                //   front at 5.455 and the display's own front
                                //   face at 8.205 -- 1.320 under the acrylic.
                                //   The pocket is 3.175 deep in a 6.350 floor,
                                //   exactly half, and P1's relief is 4.845
                                //   with 1.505 left.
disp_front_z   = disp_mount_z + disp_standoff + 1.600 + 2.750;      // 8.205
disp_clear     = acrylic_z0 - disp_front_z;                         // 1.320

//  The 18650 holder cannot be contained: it reaches below the deck's own
//  underside and the deck has to stand off the desk by more than that.
disp_18650_z   = disp_mount_z + disp_standoff - disp_18650_below;   // -11.345
disp_18650_proud = deck_bot_z - disp_18650_z;   // [DERIVED] 1.820 below the
                                                //   deck's own underside
foot_h_min     = cowl_h() + 1.270;      // [DERIVED] 5.090. A foot shorter
                                        //   than this and the deck rocks on
                                        //   its own battery cowl. Section 12.
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
//  THE COUNTERBORE HAS TWO JOBS AND THE OLD O8.000 x 5.000 DID NEITHER.
//  It has to swallow (a) the encoder's bushing, whose top lands at 15.070 --
//  1.545 ABOVE a 5.000-deep counterbore's floor, so the knob would stand off
//  on the bushing and never reach the slab; and (b) the 562 pot's M7 NUT,
//  about O10 across the flats, which cannot enter O8.000 at all, so the pot
//  could not be clamped with a knob fitted.
//    O12.000 clears an M7 nut and still leaves 3.000 of knob wall at O18.000.
//    6.350 deep clears the encoder's bushing top (15.070) by 0.805 and the
//    pot's nut top (10.890) by 4.985.
//  Both fall out of the stack in section 3; neither was free.
knob_counterbore_d = 12.000;    // [DERIVED] 2.000 over an M7 nut's O10
knob_counterbore_h = 6.350;     // [DERIVED] 0.25 in, 10 x grid
knob_shaft_bore_h  = 8.000;     // [DESIGN] above the counterbore floor. The
                                //   encoder's shaft tip lands at 22.970 and
                                //   the pot's at 20.680, so 8.000 takes the
                                //   deeper of the two with 0.905 to spare.

//  TWO BORES, AND THE SECOND ONE IS NOT A STYLE CHOICE.
//  The encoder is a PEC11: O6.0 flatted to 4.5 over a 7.00 mm window, so its
//  knob is a plain D-bore and knobs.md 's "one bore geometry serves every
//  shaft" holds for it.
//  The pot is NOT. The brief says non-D-shaft pots, and the house model of the
//  562 shows a PERFECT O6.000 cylinder over z 11..16 -- no flat, no spline --
//  which is a vendor model and not a measurement of the real shaft's form.
//  Either way the knob cannot use a grub screw, and the reason is geometric
//  rather than a preference:
//
//      pot shaft tip   pot_web_z + pot_above  =  20.680
//      deck's top face                           22.225
//
//  the shaft stops 1.545 BELOW the deck's own surface, so there is nowhere on
//  the knob that is both outside the acrylic AND over the shaft. No knob
//  height fixes it; the knob is 6.350 proud and the grub would still be inside
//  the bore. So the pot knob grips on its bore alone, and it is given six
//  internal ribs rather than a smooth press fit: ribs bite a round shaft AND a
//  T18 spline, which makes the knob correct either way while the shaft's real
//  form is NOT SOURCED.
knob_bore_d    = 6.100;         // [DERIVED] O6.0 shaft + 0.100
knob_bore_flat = 4.600;         // [DATASHEET] pec11 p3, flatted to 4.5 + 0.100
knob_rib_n     = 6;             // [DESIGN] on the pot knob only
knob_rib_h     = 0.300;         // [DESIGN] bites 0.300 into a O6.000 shaft
knob_rib_w     = 1.200;         // [DESIGN] 3 x 0.4 nozzle at the root, so it
                                //   prints as solid perimeter and not as a
                                //   single under-extruded line

//  The grip is entirely the proud part and the rim, because a finger cannot
//  enter the bore: MIL-STD-1472H fig 57 puts minimal finger access to the
//  first joint at 32 mm and the annular gap here is 2.000. So the brim does
//  all the work and it gets flutes.
knob_flutes    = 12;            // [DESIGN]
knob_flute_d   = 3.175;         // [DESIGN] 0.125 in cutter, 5 x grid
knob_flute_cut = 0.800;         // [DESIGN] how far each flute bites the rim
knob_mark      = [2.000, 1.000];    // [DESIGN] the indicator: one groove
                                    //   across the top face. The panel carries
                                    //   no engraving at all, so the knob's own
                                    //   line is the only legend on the deck.

// ---------------------------------------------------------------------------
// 5b. FITS
// ---------------------------------------------------------------------------
//  One place for "how much bigger than the part is the hole it drops into",
//  because a fit guessed per-feature is a fit that is wrong somewhere.

pocket_fit    = 0.635;          // [DESIGN] 1 grid per side, board into pocket.
                                //   stack.md 5.4 said 0.500; at Protolabs'
                                //   prototyping +/-0.500 that is a negative
                                //   clearance -- the board will not go in. One
                                //   grid gives 0.185 at an XY-calibrated
                                //   +/-0.250. v2/docs/fabrication.md 3 row 3
slide_fit     = 0.250;          // [STANDARD] per side, a printed part sliding
                                //   into a printed or metal feature  [HOUSE]
carrier_inset = 1.905;          // [DERIVED] 3 x grid. The acrylic holds
                                //   +/-0.230 and the print holds +/-0.5 % =
                                //   +/-1.523 on 304.600, so a FLUSH edge
                                //   mismatches by up to 1.753 mm, visibly, and
                                //   sometimes the wrong way. Draw the carrier
                                //   inboard all round and the tolerance stops
                                //   being a step and becomes a reveal of
                                //   1.028 .. 2.782.  v2/docs/fabrication.md 0.11

// ---------------------------------------------------------------------------
// 6. FASTENERS
// ---------------------------------------------------------------------------
//  CNC Kitchen standard, as the asset library publishes it.  [STANDARD]

insert_m3_hole_d  = 4.000;
insert_m3_depth   = 6.700;
insert_m3_wall    = 1.600;
insert_m2_hole_d  = 3.200;      // [STANDARD]
insert_m2_depth   = 4.000;
insert_m2_wall    = 1.300;

//  The library publishes M2, M3, M4, M5 and NO M2.5 row. This file used to
//  take the M2 line for M2.5. That is not conservative, it is a crack: the
//  library's own M2.5 insert measures O4.600 x 4.000 -- the SAME knurl as the
//  M3's -- so into a O3.200 bore it is 1.400 mm of diametral interference
//  against the 0.600 that knurl is designed for. It wants the M3 bore. The
//  depth rule holds on every verifiable row (depth = insert length + 1.000)
//  and two vendors independently publish 4.00.
//  v2/docs/fabrication.md 0.3, 2.3
insert_m25_hole_d = 4.000;      // [DERIVED] was 3.200, the M2 row. Wrong.
insert_m25_depth  = 5.000;      // [DERIVED] 4.000 insert + 1.000
insert_m25_wall   = 1.600;      // [STANDARD] M3's, because it is M3's knurl

//  A boss OD is its bore plus twice the minimum wall, rounded UP onto the
//  module -- stack.md 5.4's own convention, carried forward.
boss_m2_od   = 6.350;           // [DERIVED] 3.200 + 2*1.300 = 5.800 -> 10*grid
boss_m25_od  = 7.620;           // [DERIVED] 4.000 + 2*1.600 = 7.200 -> 12*grid
boss_m3_od   = 7.620;           // [DERIVED] same bore, same wall

//  A CLEARANCE HOLE IS NOT ONE NUMBER, because 12.700 mm of acrylic is not
//  3.000 mm of acrylic. The laser's kerf tapers; cut visible-face-up the hole
//  is wide at the top and narrow at the bottom, and the narrow face is the one
//  the screw has to pass:
//      narrow = drawn + kerf 0.200 - machine 0.230 - taper 0.887   (at 2.0 deg)
//  O3.400 through 12.700 finishes O2.927 at 1.0 deg and O2.483 at 2.0. An M3
//  does not go through the slab at all. The SAME hole through the 3.000 back
//  panel finishes O3.160 and is fine -- which is the proof this is a thickness
//  problem and not a hole problem. stack.md 3.2 solved it once for M4 by
//  drawing O5.60 and the fix was lost when the design moved to M3.
//  v2/docs/fabrication.md 0.1, 0.14, 1.2
screw_m3_clear_slab = 4.445;    // [DERIVED] 7 x grid. Narrow face O3.528 worst
                                //   case: 0.528 diametral over an M3 shank.
screw_m3_clear_back = 3.400;    // [STANDARD] dia + 0.4, and at 3.000 mm thick
                                //   that still works
screw_m25_clear_d   = 2.900;

//  BOTH acrylic plates bolt into bosses printed in ONE part, and the two
//  materials disagree about length. Over the 289.560 mm screw span:
//      PMMA, heat and moisture, 5 mm/m                1.448    [ROHM gsxt]
//      FDM boss pitch at +/-0.5 %                   +/-1.448    [protolabs]
//  Two independent terms, both about +/-1.5 mm, against six round holes and
//  zero float. So: ONE round hole as the datum and FIVE SLOTS, each slot
//  pointing back at the datum so the error only ever grows along the slot.
//  stack.md 6.1 asked for exactly this and the CAD had six round holes.
//  v2/docs/fabrication.md 0.2, 3 row 7
screw_datum_i     = 1;          // [DESIGN] which hole is the round one. Both
                                //   plates list their holes as
                                //       for (sx = [1, -1]) for (y = ...)
                                //   so index 1 is the +x column's MIDDLE hole
                                //   in both. Putting the datum mid-column
                                //   halves the travel the slots must absorb,
                                //   because the error grows away from it in
                                //   two directions instead of one.
screw_slot_float  = 3.000;      // [DERIVED] +/-1.500 of travel, against the
                                //   1.448 the two terms actually need
screw_slot_l_slab = screw_m3_clear_slab + screw_slot_float;   // 7.445
screw_slot_l_back = screw_m3_clear_back + screw_slot_float;   // 6.400

screw_x     = 144.780;          // [DERIVED] 228 x grid. The only screw line
                                //   that is symmetric, on the grid, and leaves
                                //   M3 the 5.10 edge distance acrylic wants.
                                //   M4 leaves 5.27 against 6.75 and does not
                                //   fit; that is why these are M3.
screw_dy    = 31.750;           // [DESIGN] 1.25 in, three per side

//  The back panel's fastener pattern is in section 9b, with the reason it
//  changed: it now mirrors this same screw line instead of being banished into
//  the keyboard region, because the re-derived stack is deep enough to take
//  both sets of inserts on one axis.
//  Rohm's rule: a hole's centre stands off a free edge by 1.5 x the HOLE
//  diameter. SendCutSend's "1.5 x MATERIAL thickness" was the looser of the
//  two at 3 mm and becomes 19.050 mm at 12.700, which no fastener in a deck
//  with an 8.125 mm wall can ever meet. At this thickness the two rules
//  invert, and Rohm's is the rule of record.
//  v2/docs/fabrication.md 0.9, 1.4
acrylic_edge_min = 1.5 * screw_m3_clear_slab;   // [STANDARD] 6.668, and
                                                //   body_w/2 - screw_x = 7.520

// ---------------------------------------------------------------------------
// 7. BOARD FOOTPRINTS, all [MEASURED] from vendor STEP unless noted
// ---------------------------------------------------------------------------


pat_b         = [76.200, 21.590];   // NeoSlider / NeoKey / Quad Rotary
pat_b_holes   = [38.100, 16.510];
pat_a         = 25.400;             // the 1 inch breakout
pat_a_holes   = 20.320;
hole_d        = 2.500;

//  Measured off the house library's own meshes, by taking the z of every
//  horizontal planar face with area > 3 mm2 -- a step's POSITION, not a
//  bounding box. In those files z = 0 is the PCB's FRONT (component) face and
//  the components run to negative z, which is why a bbox alone tells you
//  nothing about where a body ends.
//    /home/user/kylebsmith/cmtl-3d-assets/02_controls/
//      adafruit_5880_rotary-encoder-i2c-stemma-qt   the INDIVIDUAL encoder
//      adafruit_5295_neoslider-stemma
//      adafruit_562_10k-potentiometer
enc_above     = 21.400;         // [MEASURED] PCB front face to shaft tip; the
                                //   datasheet-derived 21.500 was 0.100 out
enc_body_h    = 6.500;          // [MEASURED] and it CONFIRMS the datasheet:
                                //   the body's top is a 104.936 mm2
                                //   down-facing plane at exactly 6.500. This
                                //   is the number that sets acrylic_z0.
enc_bushing   = 7.000;          // [MEASURED] 6.500 .. 13.500, O6.8. The
                                //   datasheet's 5.000 came from the PEC11 "LB"
                                //   row and is wrong for this part by 2.000 --
                                //   more than the margin the old O8.000 x
                                //   5.000 counterbore had, which is exactly
                                //   how the error stayed hidden.
enc_qt_below  = 2.960;          // [MEASURED] right-angle STEMMA QT behind the
                                //   PCB's rear face; needs a floor relief
slider_above  = 15.500;         // [MEASURED] shank tip from the PCB front face
slider_body   = 6.500;          // [MEASURED] pot body front face -- 6.000 was
                                //   0.500 out, and the correction ties the
                                //   NeoSlider's body height to the encoder's
                                //   exactly, which is what makes one
                                //   acrylic_z0 serve both
slider_shank  = 5.000;          // [MEASURED] shank width
slider_qt_below = 2.960;        // [MEASURED] the same rear connector

// ---------------------------------------------------------------------------
//  THE 562 PANEL POT, and why the carrier has to grow a web for it
// ---------------------------------------------------------------------------
//  A panel-mount pot is carried by its panel, so the BUSHING LENGTH is the
//  maximum panel thickness at that hole. The slab is 12.700 and the bushing is
//  6.600, so the pot cannot be carried by the slab -- which is why the brief's
//  "the potentiometers should be panel mounted" has to mean mounted to a web in
//  the CARRIER, with the knob filling the slab's bore above it. All of the
//  pot's Z follows from three measured numbers.
pot_body      = [16.000, 16.000];   // [MEASURED] a 16 mm pot
pot_lug_span  = 25.336;             // [MEASURED] across the solder lugs, which
                                    //   is wider than the body and is what the
                                    //   floor's pocket actually has to clear
pot_shoulder_below = 9.200;         // [MEASURED] body depth BELOW the clamping
                                    //   face (z 2.000 .. -7.200)
pot_bushing   = 6.600;              // [MEASURED] 2.000 .. 8.600
pot_above     = 15.600;             // [MEASURED] clamping face to shaft tip
pot_bore_d    = 7.200;              // [DESIGN] M7 x 0.75 bushing + 0.200
pot_nut_h     = 2.000;              // [STANDARD] M7 hex nut, washer allowed for
pot_nut_d     = 10.000;             // [STANDARD] across the flats

//  The web is bracketed from BELOW by the floor (the pot's body must not break
//  through it) and from ABOVE by the acrylic (the web must not foul the slab):
//      pot_web_z >= pot_shoulder_below - carrier_t  =  2.850
//      pot_web_z <= acrylic_z0 - pot_web_t          =  5.715
//  5.080 (8 x grid) sits in the middle: 2.230 of floor under the pot, 0.635 of
//  air over the web.
pot_web_t     = 3.810;          // [DESIGN] 6 x grid, inside the 4.100 of panel
                                //   the bushing allows once a nut and washer
                                //   are counted
pot_web_z     = 5.080;          // [DESIGN] 8 x grid, the web's UNDERSIDE --
                                //   the face the pot's shoulder bears on
pot_web_od    = 20.320;         // [DESIGN] 32 x grid = 0.8 in, enough to bear a
                                //   16 mm square flange around a O7.200 hole
pot_z_bottom  = pot_web_z - pot_shoulder_below;  // -4.120, inside the floor

//  THE POCKET IS SIZED TO THE ACRYLIC STAND BASE, NOT THE PCB. The board is
//  92.500 x 69.100; the display's bonded stand base is 92.500 x 70.103, which
//  is 1.003 mm wider in Y than the board it sits on. A pocket drawn to the
//  board does not accept the module at all.
//  v2/parts/board-models.md 2.1, v2/docs/fabrication.md 3 row 4
disp_stand    = [92.500, 70.103];   // [MEASURED] vendor STEP, and the base
                                    //   overhangs the PCB by 1.000 on the
                                    //   18650 edge ONLY -- v1 flagged that
                                    //   trap and the B-rep confirms which edge
disp_board    = [disp_pcb_sz()[0] + 2*pocket_fit,
                 disp_pcb_sz()[1] + 2*pocket_fit];  // 93.770 x 70.370
                                    //   Sized to the PCB, NOT the stand base.
                                    //   The base bolts to the same four SMTSO
                                    //   standoffs the enclosure uses, so it is
                                    //   discarded and the carrier IS the stand
                                    //   base. fabrication.md 3 row 4 assumed
                                    //   the base stays and asked for 71.373;
                                    //   it does not stay.
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
//  The K9 Pro comes out of its case. Every number here is read off an
//  ANALYTIC surface of Keychron's own 37.7 MB assembly -- a plane's position,
//  a cylinder's axis, a wire's extent -- not sampled and not classified.
//  Full derivation: v2/parts/keyboard-strip.md.
//
//  THE FINDING THAT CHANGED THE DESIGN, AND IT IS NOT WHAT WAS EXPECTED:
//  there is no separate switch plate. The plate IS the top 1.200 mm of the
//  aluminium top frame, one part, so stripping keeps the whole
//  287.000 x 97.500 shell and its 1.000 mm skirt. Throwing the ABS tray away
//  saves 0.979 mm of height and nothing more, because it is the frame's
//  skirt, not the tray, that reaches down. What stripping buys is a FLAT seat
//  and a chassis we control. It does not buy thickness, and this file no
//  longer pretends it does.
//
//  Two earlier beliefs in this repo are retired here by measurement:
//  K9PRO-PLATE is not the switch plate (it is a 0.500 mm SECC steel shield
//  under the PCB, discarded), and the "plate assembly y -6.88..-0.97" figure
//  came from an un-meshed bounding box that overshot by 8.4 mm in Y.

kbd_frame     = [287.000, 97.500];      // [CAD-EXACT] the retained frame, and
                                        //   it IS the switch plate: one part
kbd_frame_r   = 4.400;                  // [CAD-EXACT] its corner radius
kbd_frame_cx  = 133.000;                // [CAD-EXACT] its plan centre in the
kbd_frame_cz  =  56.750;                //   model's own axes (X 8..276.5 etc)

kbd_switch_field = [15*kbd_U, 5*kbd_U]; // [DERIVED] 285.000 x 95.000, switch
                                        //   CENTRES on the measured lattice
kbd_cap_field    = [284.000, 94.000];   // [DERIVED] keycap outer edges:
                                        //   15u - 1u + an 18.000 cap
kbd_cap_h        =  9.842;              // [CAD-EXACT] keycap top above the
                                        //   plate top, and the cap plane is
                                        //   FLAT (per-row maxima spread
                                        //   0.145 over the whole field).
                                        //   Keychron's published 19.8 mm
                                        //   front-with-keycaps is front height
                                        //   + switch height, not a
                                        //   measurement: 2.2 out. Retired.
kbd_plate_t      =  1.200;              // [CAD-EXACT]
kbd_pcb_t        =  1.200;              // [CAD-EXACT] 1.200, not 1.600 --
                                        //   which is why Keychron needs 33
                                        //   support points under it
kbd_seat_below   =  3.700;              // [CAD-EXACT] the PCB's underside
                                        //   below the plate top. Two exact
                                        //   planar faces, at -2.500 and
                                        //   -3.700: FLAT and horizontal, no
                                        //   slope, everywhere. This is the
                                        //   headline number the last session
                                        //   could not quote.
kbd_boss_below     = 1.209;             // [CAD-EXACT] plate-seat boss tops
kbd_pcb_boss_below = 3.709;             // [CAD-EXACT] PCB-seat pad tops

//  Everything that hangs below the seat, deepest last. These set the pockets.
kbd_socket_below  = 2.091;              // [CAD-EXACT] 61 hot-swap sockets
kbd_usbc_below    = 3.168;              // [CAD-EXACT] the receptacle's floor
kbd_toggle_below  = 4.560;              // [CAD-EXACT] the slider caps, and
                                        //   they are the deepest thing on the
                                        //   board -- deeper than the USB-C
kbd_skirt_rear    = 8.595;              // [DERIVED] skirt rim below the seat,
kbd_skirt_front   = 2.838;              //   rear and front: it is inclined
kbd_skirt_t       = 1.000;              // [CAD-EXACT] its wall

//  The socket field, which is the governing pocket. On a 19.000 pitch the
//  15.101 x 13.991 socket footprints do not touch -- they leave 3.899 across
//  and 5.009 along -- so per-key pockets are possible. One pocket with ribs
//  on the lattice is simpler and stronger, and a printed plate can offer a
//  continuous seat where an injection-moulded tray could only offer ribs.
kbd_socket_field = [278.751, 89.991];   // [DERIVED] model X -8.514..270.237,
                                        //   Z 12.959..102.950
kbd_socket_field_cx = 130.862;          // [DERIVED] its own centre, which is
kbd_socket_field_cz =  57.955;          //   NOT the frame's

// ---------------------------------------------------------------------------
// 8a. WHERE THE KEYBOARD SITS IN Z -- AND IT IS RECESSED
// ---------------------------------------------------------------------------
//  v2/docs/fabrication.md 5.1 called this "the deciding number, and it is in
//  no file in this repo". It is this one, and fixing it picks the branch for
//  the acrylic's keyboard void. The two branches are genuinely different
//  parts, so both are written down:
//
//    FLUSH     plate top level with the deck's top face. The void must then be
//              BIGGER than the frame so the frame passes up into it, the
//              keycaps stand 9.842 proud, and there is no lip anywhere -- the
//              typing plane is perfect, and the aluminium frame reads as a
//              flush inlay in clear acrylic. It is the prettier part.
//              BUT the frame's four rear apertures sit 3.650..7.510 below the
//              plate top, which is INSIDE the acrylic's thickness, about
//              1.1 mm from solid acrylic. Nothing can be plugged in. The
//              keyboard runs on its own cell and talks BLE, and it is charged
//              through that USB-C (v2/research/keyboard-k9pro.md 7), so this
//              branch means lifting a 12.700 mm slab to charge the
//              instrument. The acrylic cannot be relieved locally either: a
//              laser cuts through, so a port relief is an open hole in the
//              deck's top face, 9.5 mm off centre, beside a knob bore.
//
//    RECESSED  plate top just UNDER the acrylic and the void SMALLER than the
//              frame, so the slab overlaps the frame's rim. The apertures come
//              out below the acrylic into a channel in the carrier and stay
//              serviceable. The keycaps sit 3.493 below the deck's top face.
//
//  RECESSED, and the margin is not close:
//    1. it is the only branch that keeps the instrument chargeable;
//    2. "all parts recessed ... it allows things to sink back into the
//       enclosure and therefore is more robust" is the brief's own rule, and
//       the keyboard is a part;
//    3. the recess is acrylic_t - kbd_cap_h - gap, so it is FORCED by
//       half-inch acrylic against a low-profile keycap, not chosen;
//    4. it collapses the carrier under the keyboard from a 9.000 mm plinth to
//       a 2.015 mm seat, and lands the socket-field pocket floor on the
//       wiring plane to within 0.076 mm;
//    5. it widens the slab's governing web: a smaller void takes less out of
//       the sheet, so the front rail goes 4.400 -> 5.625.
//
//  The cost, stated plainly, is in 8c: 3.295 mm has to come off the rear
//  skirt on a mill.

kbd_mount        = "recessed";          // [DESIGN] "recessed" | "flush"
kbd_plate_gap    = 0.635;               // [DESIGN] one grid of air between the
                                        //   plate top and the acrylic, so the
                                        //   slab NEVER bears on the keyboard.
                                        //   Two bearing planes that have to be
                                        //   coplanar is a tolerance trap; the
                                        //   slab lands on the carrier's rim,
                                        //   and only on the carrier's rim.

kbd_plate_top_z  = acrylic_z0 - kbd_plate_gap;              //  5.715
kbd_seat_z       = kbd_plate_top_z - kbd_seat_below;        //  2.015
kbd_cap_top_z    = kbd_plate_top_z + kbd_cap_h;             // 15.557
kbd_recess       = acrylic_z0 + acrylic_t - kbd_cap_top_z;  //  3.493

// ---------------------------------------------------------------------------
// 8b. THE TYPING ANGLE IS NOW OURS, AND IT IS ZERO
// ---------------------------------------------------------------------------
//  Stock, the K9 Pro's wedge is in its ABS TRAY: the tray's outer face is one
//  plane at 3.37914 deg, and so are the steel shield, the tray's inner floor
//  and the frame's skirt rim -- one plane family, four parts. An earlier
//  session sectioned the case and concluded the TOP was wedged. It is not.
//  Throw the tray away and the plate is flat, and the angle stops being the
//  keyboard's and becomes a decision. layouts/README.md's "no ramp is needed
//  anywhere" was true of the keyboard mounted whole and stops being true the
//  moment the tray comes off.
//
//  Zero, and the first reason is the only one that matters:
//    1. positive tilt EXTENDS the wrist, and wrist extension is the posture
//       that hurts. Flat or negative is better -- which is why the ergonomic
//       market sells negative tilt and nobody sells positive;
//    2. keeping 3.37914 deg costs kbd_frame[1] * tan = 5.639 mm of extra
//       carrier depth at the rear, on a deck that is 28.050 thick in total;
//    3. height was the thing worth defending and it was defended: dropping
//       the keyboard through the carrier costs 3 mm over a bare keyboard
//       rather than 9.

kbd_wedge_stock_deg = 3.37914;  // [CAD-EXACT] the tray's plane. Retired, kept
                                //   because the number was wrong in this repo
                                //   twice and is now read off the plane's own
                                //   normal rather than sampled
kbd_tilt_deg        = 0;        // [DESIGN] ours, and it is flat

// ---------------------------------------------------------------------------
// 8c. IN PLAN, AND THE SKIRT
// ---------------------------------------------------------------------------

kbd_fit       = [1.350, 0.950];         // [REPO] v1's fit rule, carried
kbd_pocket    = [kbd_frame[0] + kbd_fit[0],
                 kbd_frame[1] + kbd_fit[1]];        // 288.350 x 98.450
kbd_top_y     = -body_h/2 + front_wall + kbd_pocket[1];         // 7.6625
kbd_frame_y   = kbd_top_y - kbd_fit[1]/2 - kbd_frame[1]/2;      // -41.5625
kbd_frame_rear_y = kbd_frame_y + kbd_frame[1]/2;                //  7.1875

//  model (X, Z) -> deck (x, y). Model X runs the way ours does; model Z runs
//  the opposite way, with the rear edge at 8.000.
function kbd_dx(mx) = mx - kbd_frame_cx;
function kbd_dy(mz) = kbd_frame_rear_y - (mz - 8.000);

//  THE ACRYLIC'S VOID. Smaller than the frame on all four sides, so the slab
//  overlaps the frame's rim and closes the kbd_plate_gap under it. Both
//  numbers are on the grid and both are bracketed: bigger than the KEYCAPS by
//  enough to clear them, smaller than the FRAME by enough to still overlap
//  after the laser's +/-0.230. Centred on the frame, not on the switch field,
//  whose centre is 0.250 rearward of it.
kbd_void      = [285.750, 95.885];      // [DESIGN] 450 x grid, 151 x grid
kbd_void_y    = kbd_frame_y;

//  The void's corner radius is bracketed from BOTH sides and it is the one
//  number in section 8 that is a compromise rather than a consequence.
//  Too big and the void's corner swings outside the frame's R4.400 and the
//  overlap goes negative; too small and the squarer corner collides with the
//  corner keycap. Solving both inequalities for a SQUARE 18.000 cap:
//      r must be <= 3.512   (cap clears)   and  >= ~2.0  (frame overlaps)
//  3.175 (5 x grid) sits near the top of that window, which is the right end
//  to favour: it gives 0.494 of frame overlap, and the cap clearance it
//  leaves -- 0.139 against a perfectly square cap -- is only that small
//  because a square cap is a fiction. At a 1.000 mm cap corner radius it is
//  0.546 and at 1.500 it is 0.741, i.e. the same as the straight edges.
//  The keycap's own corner radius is NOT SOURCED; check_cad.py reports the
//  square-cap figure so the assumption cannot hide.
kbd_void_r    = 3.175;          // [DESIGN]
kbd_cap_clear = [(kbd_void[0] - kbd_cap_field[0])/2,
                 (kbd_void[1] - kbd_cap_field[1])/2];   // 0.875, 0.943
kbd_overlap   = [(kbd_frame[0] - kbd_void[0])/2,
                 (kbd_frame[1] - kbd_void[1])/2];       // 0.625, 0.808

//  THE SKIRT WAS THE REAL COST OF STRIPPING, AND THE RE-DERIVED STACK PAID IT.
//  The frame is one part, so keeping the plate keeps the skirt, and the skirt
//  hangs 12.295 below the plate top at the rear. Against the OLD stack -- a
//  12.350 carrier under a 6.350 acrylic_z0 -- its rim landed 0.580 BELOW the
//  carrier's own bottom face, and a closed-loop groove through a 6.000 floor
//  would have cut the floor into two islands joined nowhere. That branch
//  needed 3.295 mm milled off an irreplaceable aluminium frame on a plane
//  1.490 clear of the lowest port aperture.
//  With carrier_t = 6.350 and acrylic_z0 = 9.525 the rim lands at -3.405,
//  which is 2.945 clear of the floor's underside. NOTHING IS MACHINED, and
//  that is the single biggest thing section 3's correction bought.
kbd_skirt_z    = kbd_plate_top_z
                 - (kbd_seat_below + kbd_skirt_rear);     // [DERIVED] -3.405
kbd_annulus_z  = kbd_skirt_z - 0.300;   // [DESIGN] -3.705. The floor the skirt
                                        //   hangs over, with 0.300 of air, so
                                        //   the frame is located vertically by
                                        //   its plate bosses and NEVER by its
                                        //   own rim -- two bearing planes that
                                        //   must be coplanar is a trap.

//  And there is no groove feature to draw, which is better than drawing one.
//  The annulus between the seat plinth's outer face and the carrier's own rim
//  IS the groove: the rim's inner face holds the frame to 0.675 per side in X
//  and 0.475 in Y, and that is the keyboard's whole registration, so the 7
//  screws only ever have to pull it down.
kbd_plinth_inset = kbd_skirt_t + slide_fit;     // [DERIVED] 1.250
kbd_plinth    = [kbd_frame[0] - 2*kbd_plinth_inset,
                 kbd_frame[1] - 2*kbd_plinth_inset];      // 284.500 x 95.000

//  The seat plane is the PCB-boss plane, not the PCB's underside: Keychron's
//  pads stop 0.009 short of the board, and reproducing that gives the board
//  0.009 of air instead of a fight with the 61 switches that clamp it.
kbd_seat_plane_z = kbd_plate_top_z - kbd_pcb_boss_below;  // 5.181
kbd_boss_top_z   = kbd_plate_top_z - kbd_boss_below;      // 7.681

//  The socket-field pocket, and the ribs in it. Adjacent hot-swap footprints
//  leave a 5.009 mm clear band in model Z half way between switch rows, so a
//  1.270 rib fits there four times with 1.870 of air each side.
kbd_socket_z   = kbd_seat_z - kbd_socket_below;           // 3.099
kbd_rib_mz     = [29.4545, 48.4545, 67.4545, 86.4545];    // [DERIVED]

//  The USB-C corridor. Even recessed, a plug cannot enter from directly behind
//  the frame: the annulus is only 1.703 deep there and a USB-C plug needs
//  about 15. So the spine wall is notched through at the receptacle, and the
//  keyboard is charged by a right-angle plug on a permanent lead that runs
//  rearward under the band -- fitted once, with the slab off. THAT is what the
//  recessed branch buys and the flush branch cannot: in the flush branch the
//  aperture is inside the acrylic's thickness and no plug fits at all.
kbd_corridor_w = 20.000;        // [DESIGN] a USB-C overmould plus room to work
kbd_spine_y0   = 8.890;         // [DESIGN] the spine wall's front face, 0.593
                                //   clear of the toggle caps, which stand
                                //   1.110 BEHIND the frame's rear wall

// ---------------------------------------------------------------------------
// 8d. THE BOSSES -- reproduce these and it bolts in with its own screws
// ---------------------------------------------------------------------------
//  7 self-tapping screws from ABOVE, through tapered square holes in the
//  plate, into cored bosses; plus 16 snap latches. The PCB is screwed to
//  NOTHING -- it is clamped by 61 switches and rests on 33 pads totalling
//  261.350 mm2, which is 1.177 % of its area.
//
//  The positions are not symmetric, because they live in the webs between
//  switch cutouts and ANSI is not a symmetric layout. Only #3 and #5 mirror.
//  All 7 sit well inside the keycap field, so they stay reachable through the
//  acrylic's void with keycaps pulled -- which is the only reason a recessed
//  keyboard is still serviceable.

kbd_screw_mxz   = [[ 16.540, 34.500], [ 33.300, 72.500], [ 85.500, 15.500],
                   [128.320, 72.500], [180.500, 15.500], [223.300, 72.500],
                   [247.540, 34.500]];          // [CAD-EXACT]
kbd_prop_mxz    = [[ 47.560, 15.490], [ 99.750, 53.500], [175.800, 53.490],
                   [209.000, 34.490]];          // [CAD-EXACT] unscrewed props
kbd_pcbboss_mxz = [[ 38.115, 37.949], [ 43.299, 56.949], [ 71.300, 75.949],
                   [137.625, 56.949], [146.736, 75.949], [232.750, 56.949],
                   [253.395, 87.949]];          // [CAD-EXACT]

kbd_boss_od       = 4.000;      // [CAD-EXACT] the stock boss
kbd_boss_pilot_d  = 1.700;      // [CAD-EXACT] its cored pilot. That leaves
                                //   1.150 of wall, below the house 1.600 --
                                //   but 1.600 is for a O4.000 INSERT bore,
                                //   not a 2 mm self-tapper. PETG self-taps
                                //   acceptably at this size and the stock
                                //   screws are already the right length, so
                                //   self-tap on the first build. An M2 insert
                                //   would need the boss stepped out to
                                //   O7.000 below the PCB.
kbd_pcb_hole_d    = 5.000;      // [CAD-EXACT] the PCB's own holes, which the
                                //   O4.000 boss passes through

// ---------------------------------------------------------------------------
// 8e. THE PORTS, which are NOT lost
// ---------------------------------------------------------------------------
//  All four rear features exit through the TOP frame's own rear wall at model
//  Z 8.000 -- a part we keep -- so no port presentation is lost in the strip.
//  They now face into our interior and the carrier has to lead them out.
//  [name, width, model X centre, top below plate, bottom below plate]
kbd_ports = [["usbc", 9.640, 123.500, 3.650, 7.510],
             ["mode", 9.100,  28.500, 3.750, 7.250],
             ["os",   7.300,   9.500, 3.750, 7.250],
             ["led",  1.000, 140.520, 4.780, 5.780]];     // [CAD-EXACT]

kbd_port_wall_t  = 1.000;       // [CAD-EXACT] the frame's rear wall
kbd_usbc_lead_in = 1.492;       // [DERIVED] the receptacle's mouth sits this
                                //   far behind the frame's outer face, so a
                                //   plug's overmould already has 1.492 of
                                //   lead-in and the carrier must add NOTHING
                                //   in front of model Z 8.000
kbd_toggle_relief = [33.283, 8.966];    // [DERIVED] the slider caps' envelope,
                                        //   model X 2.715..35.998,
                                        //   Z 6.890..15.856 -- and they stand
                                        //   1.110 BEHIND the frame's rear
                                        //   wall, which puts them 0.635 into
                                        //   the 3.200 spine. 2.565 left.

kbd_cell       = [130.000, 32.000, 3.000];  // [DERIVED] the 1550 mAh pouch
kbd_cell_swell = 0.600;         // [DESIGN] a pouch gains thickness with age;
                                //   10 % of 3.000 plus clearance. No
                                //   compression, no screws through the bay,
                                //   nothing that can pinch when it closes.

//  The cell stays where Keychron put it, because the ONE thing that would let
//  it move -- the battery lead's length -- is NOT SOURCED. Model X 75..205,
//  Z 17.632..49.753. Declared here, after the cell itself: OpenSCAD evaluates
//  in order and a forward reference is a silent undef.
kbd_cell_xy    = [7.000, -18.505];      // [DERIVED] deck centre of the bay
kbd_cell_z     = kbd_seat_z - (kbd_cell[2] + kbd_cell_swell);     // 1.590// ---------------------------------------------------------------------------
// 9. WIRE CHANNELS IN THE CARRIER'S TOP FACE
// ---------------------------------------------------------------------------

//  Under 12.700 mm of CLEAR acrylic every board, every boss and every wire is
//  seen, so the wiring is laid out and checked rather than left to whoever
//  builds it. These are the SAME numbers v2/layouts/carrier.py draws, and
//  check_cad.py's job is to notice if the two ever drift apart.
//
//  The bus topology IS the layout (v2/parts/wiring.md):
//    I2C   one chain -- 3V3 / GND / SDA GPIO13 / SCL GPIO14 -- leaving the
//          Waveshare's rear 2x8 and running LEFT TO RIGHT through the three
//          encoders and into the fader. The daisy chain and the signal flow
//          are the same line, which is the whole reason it reads as a diagram.
//    POTS  three wipers on GPIO1/2/3 = ADC1_CH0/1/2, the ADC unit that keeps
//          working while Wi-Fi is up, plus a shared 3V3/GND pair. One trunk
//          below the pot row and three stubs up to it. No ADS1115.

chan_qt_w     = 4.000;          // [DESIGN] a STEMMA QT cable, 4 conductors
chan_qt_d     = 2.000;          // [DESIGN] and 2.000 is deep enough that the
                                //   cable sits BELOW the plane, so the plane
                                //   stays a plane
chan_pot_w    = 3.000;          // [DESIGN] a pot's 3 wires
chan_pot_d    = 2.000;

cols          = [grid_x - pitch, grid_x, grid_x + pitch];   // -1.905, 36.195, 74.295
enc_y         = band_mid + pitch/2;     // 68.0125
pot_y         = band_mid - pitch/2;     // 29.9125
hdr_x         = -50.800;        // [REPO] the 2x8 male header, UNDER the display
exit_x        = -41.910;        // [REPO] just clear of the display's right edge
pot_trunk_dy  = -33.655;        // [REPO] 53 x grid, below the pot row
trunk_y       = band_mid + pot_trunk_dy;    // 15.3075
uart_y        = (body_h/2 - wall) - 3.175;  // 83.8875, along the rear edge

// ---------------------------------------------------------------------------
// 9b. THE CARRIER'S OWN GEOMETRY
// ---------------------------------------------------------------------------
carrier_inset_r = 1.000;        // [DESIGN] the printed outline's corner. The
                                //   acrylic's 2.000 offset inward by 1.905
                                //   would be 0.095, which is a stress riser
                                //   and prints as a sharp corner; 1.000 costs
                                //   0.905 of extra reveal at the four corners
                                //   only.
carrier_w     = body_w - 2*carrier_inset;       // 300.790
carrier_d     = body_h - 2*carrier_inset;       // 186.565

interior_h    = body_h - front_wall - wall;     // 177.850
interior_y0   = -body_h/2 + front_wall;         // -90.7875, the front face
interior_y1   =  body_h/2 - wall;               //  87.0625, the rear face

//  The front rim comes out at 2.495 and that is forced, not chosen: the
//  carrier is inset 1.905 all round and its inner face cannot move rearward
//  because the keyboard frame's front face is already at -90.3125. The acrylic
//  above it is 5.625 wide (the void shrank, so the front rail grew from
//  4.400), and the wrist load is compression on a 2.495 x 9.525 PETG web with
//  the 6.350 floor and the 3.175 back panel under it. It carries; it is
//  recorded because it is the thinnest wall in the part.
rim_front_w   = (body_h/2 - carrier_inset) - (-interior_y0);    // 2.495
rim_side_w    = wall - carrier_inset;           // 6.220
rim_rear_w    = wall - carrier_inset;           // 6.220

//  Ribs. The six slab screws are two columns 289.560 apart, three per column
//  over a 63.500 span, so NOTHING fastens the middle 289.560 mm of either long
//  edge. That span, not the material, is the argument for ribs, and the X ribs
//  are the ones doing the work. 1.270 is exactly 3 x 0.4233 extrusion widths,
//  so a rib prints solid with no infill and no gap-fill -- which is why it
//  must stay 1.270 and not become 1.200 or 1.500.
rib_t         = 1.270;          // [DERIVED] 3 lines at a 0.4 nozzle
rib_h         = 3.175;          // [DESIGN] 0.125 in, on the floor's UNDERSIDE
rib_pitch     = 25.400;         // [DESIGN] 1 in, the same module as the combs

//  The back panel is a service panel, not the structural grid the slab is, and
//  it gets M2.5 rather than M3 for a geometric reason: an M3 insert pocket is
//  6.700 deep and the floor is 6.350, so pressing M3 from below would break
//  through the visible wiring plane by 0.350 at every one of them. M2.5 is
//  5.000 deep and leaves 1.350 of floor above every pocket, so the plane stays
//  a plane. v2/docs/fabrication.md 2.3 for the bore and depth.
//    Six of them mirror the slab's own screw line -- which only became
//  possible when the stack grew: two 6.700 M3 inserts nose to nose need 13.400
//  and the old 12.350 carrier could not take them, which is why the back
//  panel's screws used to be banished into the keyboard region. Now the slab's
//  inserts occupy 2.825..9.525 and the back panel's -6.350..-1.350, with
//  4.175 of clear air between.
//    Four more sit inside the KEYBOARD's seat plinth, where the carrier is
//  locally 11.540 thick, because the back panel's real job is fighting the
//  carrier's bow and six screws in the control band leave the keyboard half of
//  a 304 mm plate unfastened. Their positions are chosen to clear all 18 of
//  the keyboard's own bosses and the cell bay.
back_screw_kbd = [[-127.000, -25.400], [127.000, -25.400],
                  [ -82.550, -63.500], [ 82.550, -63.500]];  // [DESIGN]
                                // on the inch grid, and each one checked
                                // against all 18 of the keyboard's bosses and
                                // against the cell bay: the nearest approach is
                                // 12.2 mm, to the screw boss at x -116.460

//  Both plates list their holes in the SAME order -- the +x column first, top
//  to bottom -- so screw_datum_i = 1 is the +x column's middle hole in both,
//  and the slots that radiate from it radiate the same way on both. Functions,
//  not variables, so the order of this file does not matter to them.
function slab_screw_pts() =
    [for (sx = [1, -1], dy = [screw_dy, 0, -screw_dy])
        [sx * screw_x, band_mid + dy]];
function back_screw_pts() = concat(slab_screw_pts(), back_screw_kbd);

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

// ---------------------------------------------------------------------------
// 12. THE 18650 COWL
// ---------------------------------------------------------------------------
//  The Waveshare board's 18650 holder reaches 15.200 behind the PCB's rear
//  face -- 6.500 deeper than anything else on it -- which puts it 1.820 BELOW
//  the deck's own underside. It cannot be contained and v1 built a cowl around
//  exactly this fact. So three parts have to agree about one window: the
//  carrier's floor, the back panel, and the cowl that covers it.
//
//  EVERY ENTRY HERE IS A FUNCTION, DELIBERATELY. These depend on disp_x,
//  band_mid and pocket_fit, which are assigned in sections 4, 5b and 7 -- and
//  OpenSCAD evaluates assignments in order, so a variable here would be a
//  silent undef. Functions are lazy, so they can live at the end of the file
//  where they read, and still be called from section 3.
function cowl_clear()  = 0.500;         // [DESIGN]
function cowl_floor()  = 1.500;         // [DESIGN] it is a cover, not a
                                        //   structure: 1.500 is 3.54 lines
function cowl_wall()   = 2.000;         // [DESIGN]
function cowl_flange() = 7.620;         // [DERIVED] 12 x grid. It was 6.350
                                        //   and the checker refused it: at
                                        //   6.350 the screw's edge stands
                                        //   3.175 - 1.450 = 1.725 off the
                                        //   window, under the 2.000 hard floor
                                        //   (10 x the 0.200 kerf). 7.620 gives
                                        //   2.360.
function cowl_h()      = disp_18650_proud + cowl_clear() + cowl_floor();  // 3.820
function disp_pcb_sz() = [92.500, 69.100];      // [MEASURED]

//  The window, shared by all three parts.
function cowl_window() = [87.000 + 2*pocket_fit, 21.103 + 2*pocket_fit];

//  Where it is. The holder spans U 2.748..89.748 and V 4.600..25.703 on the
//  display's own board, so its centre is (46.248, 15.152) against a board
//  centre of (46.250, 34.550).
function cowl_xy() = [disp_x   + 46.248 - disp_pcb_sz()[0]/2,
                      band_mid + 15.152 - disp_pcb_sz()[1]/2];

//  EDGE MIDPOINTS, NOT CORNERS, and the checker is why. On the corners the
//  two lower screws land 5.306 from the display's own O5.500 screw-head
//  clearance holes, which leaves a 1.104 mm web in 3.175 mm acrylic -- below
//  the 2.000 hard floor and worse than the part's recorded baseline. It is the
//  same vendor collision that makes those two head holes merge into the window
//  in the first place: the 18650 holder's edge runs through the display's
//  lower mounting holes. Moved to the midpoints, the nearest approach to
//  anything is 13.07.
function cowl_screw_pts() =
    [[cowl_xy()[0] + cowl_window()[0]/2 + cowl_flange()/2, cowl_xy()[1]],
     [cowl_xy()[0] - cowl_window()[0]/2 - cowl_flange()/2, cowl_xy()[1]],
     [cowl_xy()[0], cowl_xy()[1] + cowl_window()[1]/2 + cowl_flange()/2],
     [cowl_xy()[0], cowl_xy()[1] - cowl_window()[1]/2 - cowl_flange()/2]];
