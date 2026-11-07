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
//  AND THE HEIGHT IS FORCED, NOT CHOSEN, which is the only interesting thing
//  about it: 8 x grid is 5.080 and misses foot_h_min by 0.010, so the smallest
//  grid multiple that clears the cowl at all is the next one up. The 0.625 it
//  clears by sits on top of the 1.270 of air foot_h_min already carries. It
//  lived in foot.scad as the ft_h local, reported upward because that file does
//  not own this one.
foot_h         = 9 * grid;      // [DERIVED] 5.715, and [MEASURED] -- the
                                //   exported foot.stl spans z 0.000..5.715
                                //   exactly, which export.py prints on every
                                //   run. Also the deck's front edge stands
                                //   deck_t + foot_h = 37.465 off the desk.
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
//  knob_rib_h IS MEASURED FROM THE BORE WALL, NOT FROM THE SHAFT, and the
//  comment here used to conflate the two: it read "bites 0.300 into a O6.000
//  shaft", which is not what the part does. The bore is knob_bore_d = 6.100,
//  so a 0.300 rib puts its crest at r 2.750 -- measured off the artefact as
//  exactly 2.7500, as the axis-to-plane distance of the six crest faces in
//  export/stl/knob_pot.stl over z 6.350..14.350 (the crest is a flat chord, so
//  a vertex radius reads 2.8147 at its corners and is the wrong thing to
//  measure). Against a nominal O6.000 shaft, r 3.000, that is 0.250 of RADIAL
//  INTERFERENCE and 0.500 diametral -- not 0.300.
//    THE COMMENT IS CORRECTED AND THE VALUE IS NOT, deliberately. Raising
//  knob_rib_h to 0.350 would buy the 0.300 the old comment claimed, at 20 %
//  more interference, and there is nothing to justify it with: the pot shaft's
//  real form is declared NOT SOURCED in the block above knob_bore_d -- the
//  house model shows a perfect O6.000 cylinder and the brief says non-D-shaft,
//  which is why there are six ribs rather than a smooth press fit, so that the
//  knob is "correct either way" on a round shaft or a T18 spline. An
//  interference target against a shaft whose form is unknown cannot be
//  derived, so the artefact stays the record and the prose is brought to it.
//  If the real shaft is a T18 spline the crest engages the spline's crests and
//  0.250-on-nominal is not the governing number anyway.
knob_rib_h     = 0.300;         // [DESIGN] the rib's stand-off from the bore
                                //   wall. 0.300 off knob_bore_d/2 = 3.050
                                //   puts the crest at r 2.750 [MEASURED
                                //   2.7500 off export/stl/knob_pot.stl], i.e.
                                //   0.250 radial interference on a nominal
                                //   O6.000 shaft.
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

//  THE ROEHM EDGE SHORTFALL ON THESE SIX BORES: EXAMINED, COSTED, ACCEPTED.
//  check_cad.py measures it and deliberately does not gate it, and says the
//  decision is this file's. It is, so here it is.
//
//  WHAT IS ACTUALLY THERE. [MEASURED] off export/dxf/slab.dxf, nearest cut to
//  each bore CENTRE, against 1.5 x bore_d = 33.000:
//    upper row, y +68.013   27.100  an adjacent bore's wall; the sheet's rear
//                                   edge is 27.175, barely farther. short 5.900
//    lower row, y +29.913   23.533  the keyboard VOID's rear edge.  short 9.467
//  All three bores in a row read identically; check_cad names
//  (+74.295,+29.912) only because its sort has to pick one. Note what the
//  nearest cut is NOT: for neither row is it the sheet edge that governs, and
//  for the upper row the limit is the knob PITCH -- which is the one thing
//  fabrication.md 1.4's applied block does check, and it passes there because
//  1.4 reads centre-to-CENTRE (38.100 >= 33.000) where check_cad reads
//  centre-to-WALL (27.100 < 33.000). Same rule, two readings, and the
//  difference is 11.000.
//
//  THE RULE'S PRIMARY STATEMENT, read from the source rather than from memory
//  [ROEHM acrylite-kb] -- the ACRYLITE knowledge base article whose URL
//  v2/parts/acrylic.md 8.4 carries, quoted in acrylic.md 5.3:
//      "the distance from the hole center to the edge of the sheet should be
//       at least 1.5 times the diameter of the drilled hole"
//  It is written for DRILLING holes to support sheet by POINT FASTENING. The
//  page says nothing about sawing or laser cutting. And the part that decides
//  how much weight it can carry here: IT GIVES NO RATIONALE FOR THE EDGE
//  DISTANCE. The only reason it states anywhere is for the companion
//  hole-DIAMETER rule -- "adequate clearance for thermal and moisture
//  expansion and contraction" -- plus a rubber grommet "to prevent cracking
//  along the drill hole". So the rule's BASIS IS NOT SOURCED. It cannot be
//  shown to be a drilling-stress rule rather than a service-robustness one,
//  and it is therefore NOT waved away on the grounds that a laser does not
//  wedge the sheet the way a drill does. It is treated as binding and then
//  answered on geometry.
//
//  IT CANNOT BE MET. That is arithmetic, not reluctance. Both rows live in one
//  corridor, between the keyboard void's rear edge and the sheet's rear edge,
//  and they are pitch apart inside it:
//      corridor    95.1875 - 6.3800                    =  88.8075  [MEASURED]
//      rule needs  33.000 + pitch 38.100 + 33.000      = 104.100   [DERIVED]
//      short by                                           15.2925
//  A 15.2925 deficit in an 88.8075 corridor cannot be translated away, so each
//  move was costed rather than gestured at:
//    MOVE THE BAND -- centring it in the corridor gives both rows 25.354 and
//      merely EQUALISES the shortfall at 7.646. It also moves band_mid
//      +1.82125, which drags the display, the fader, the six slab screws, the
//      back panel's screw line and the cowl window with it. A worst-case
//      improvement from 9.467 to 7.646 does not buy that.
//    DROP THE PITCH -- the rule wants the rows <= 22.8075 apart against
//      38.100. At 22.8075 two O22.000 bores in a column leave 0.8075 of web
//      and the O25.400 brims overlap outright. It would manufacture a worse
//      violation than the one it cures.
//    SHRINK THE BORE -- 23.533 satisfies 1.5 x O for O <= 15.688, and the bore
//      has to pass knob_body_d = 18.000. Refused by the part.
//    MOVE THE VOID FORWARD -- it would have to travel the whole 15.2925, which
//      is 2.69 x the entire front rail (5.6825, section 8a item 5), and the
//      carrier's front rim at 2.495 is already the thinnest wall in that part.
//      Refused by the part.
//    GROW THE DECK -- 190.375 + 15.2925 = 205.6675 deep, and at the 8:5 the
//      deck is held to, 329.068 wide: +24.468 of instrument. The ratio is the
//      user's; this is not the thing to spend it on.
//
//  ACCEPTED, on a load-path fact rather than on optimism: THESE SIX BORES
//  CARRY NOTHING. The rule is stated for a hole that POINT-FASTENS the sheet,
//  one a bolt is pulled through. Nothing is bolted through a O22.000 bore and
//  nothing bears on its wall -- the knob is a flanged plug standing in a 2.000
//  annular gap, the pot's shoulder and nut clamp to the CARRIER's pot_web
//  (section 7: pot_web_z is "the face the pot's shoulder bears on", with 0.635
//  of air left over the web precisely so it cannot foul the slab), and the
//  encoder stands on a breakout behind the panel. The acrylic's only job at a
//  bore is to be a hole, which is the same reason the bores can afford the
//  laser's taper.
//  The robustness reading is what remains, and the mitigation is structural
//  [DESIGN]: the slab is bolted flat to the carrier on a 289.560 x 63.500
//  screw grid over a ribbed 6.350 floor with the 3.175 back panel beneath, so
//  the web between a bore and the void is a supported plate in compression,
//  not a free edge in bending. On the only other published figure that speaks
//  to it -- "minimum web / gap between cuts >= 1 x thickness", acrylic.md 4.4,
//  and that tag is [3P fab-guides], the weakest in the book -- the worst LAND
//  of 12.533 against 12.700 is 0.167 short, i.e. 98.7% of the guideline rather
//  than the 71% that 23.533/33.000 looks like.
//  THIS IS AN ACCEPTED SHORTFALL AND NOT A CLOSED ONE. If a test coupon ever
//  cracks from a bore toward the keyboard void, the only move left is growing
//  the deck; the four cheaper ones are costed above and all refuse.

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
//  3.175 mm of acrylic. The laser's kerf tapers; cut visible-face-up the hole
//  is wide at the top and narrow at the bottom, and the narrow face is the one
//  the screw has to pass:
//      narrow = drawn + kerf 0.200 - machine 0.230 - 2 t tan(taper)
//  O3.400 through 12.700 finishes O2.927 at 1.0 deg and O2.483 at 2.0. An M3
//  does not go through the slab at all. The SAME hole through the back panel
//  finishes O3.259 at 1.0 deg and O3.148 at 2.0 and would be fine -- which is
//  the proof this is a thickness problem and not a hole problem. stack.md 3.2
//  solved it once for M4 by drawing O5.60 and the fix was lost when the design
//  moved to M3.
//    THOSE TWO FIGURES USED TO READ "O3.160 ... through the 3.000 back panel".
//  back_t became 3.175 in section 3 and the taper term did not follow it:
//  2 x 3.175 x tan 2 deg is 0.2217, not a 3.000 sheet's 0.2095. The conclusion
//  did not change, which is exactly why nobody caught it.  [DERIVED]
//  v2/docs/fabrication.md 0.1, 0.14, 1.2
screw_m3_clear_slab = 4.445;    // [DERIVED] 7 x grid. Narrow face O3.528 worst
                                //   case: 0.528 diametral over an M3 shank.
screw_m3_clear_back = 3.400;    // [STANDARD] dia + 0.4. IT IS NO LONGER THE
                                //   BACK PANEL'S HOLE -- section 6b. The name
                                //   stays because knob_mould.scad reads it for
                                //   the mould's four M3 clamp bolts, which pass
                                //   through a PRINTED tool where no laser
                                //   taper applies.
screw_m25_clear_d   = 2.900;    // [STANDARD] dia + 0.4, the same column of the
                                //   same clearance-hole table that gives M3 its
                                //   3.400: M2.5 reads 2.7 / 2.9 / 3.1 where M3
                                //   reads 3.2 / 3.4 / 3.6  [3P
                                //   fastener-tables, acrylic.md 8.4; the row
                                //   corroborated against a second table]. The
                                //   COLUMN NAMES (fine / medium / coarse) are
                                //   not confirmed by any table retrievable
                                //   from here, so this cites a row and not
                                //   "ISO 273 medium". Section 6b is what uses
                                //   it, and the ten holes it belongs to.

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
screw_slot_l_slab = screw_m3_clear_slab + screw_slot_float;   // [DERIVED] 7.445
screw_slot_l_back = screw_m25_clear_d   + screw_slot_float;   // [DERIVED] 5.900,
                                //   and it was 6.400 off
                                //   screw_m3_clear_back. Section
                                //   6b: the back panel's screw is M2.5, so the
                                //   slot gets narrower with the hole. THE FLOAT
                                //   IS UNCHANGED AND THAT IS NOT LAZINESS --
                                //   screw_slot_float is a displacement budget,
                                //   not a clearance, and neither of the two
                                //   terms that set it knows the screw's
                                //   diameter.

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
// 6b. THE BACK PANEL'S SCREW IS M2.5, AND THE PANEL WAS DRILLED FOR M3
// ---------------------------------------------------------------------------
//  backpanel.scad drilled all ten back_screw_pts() at screw_m3_clear_back --
//  O3.400, 6.400 slots -- while carrier.scad's back_insert_cut() presses
//  insert_m25_hole_d M2.5 inserts at the SAME ten points. There is no question
//  of the patterns differing: back_screw_pts() is ONE function and both files
//  call it. Section 9b and carrier.scad each explain in prose why the insert is
//  M2.5 and not M3 (an M3 pocket is 6.700 deep against a 6.350 floor and would
//  break the visible wiring plane by 0.350 at every one of them), and neither
//  noticed that the plate above them had been drawn for the other screw.
//    BOTH READINGS OF THAT DISAGREEMENT ARE DEFECTS, which is why it is
//  written down rather than just corrected:
//      the screw is M2.5 -- foot.scad: "the screws stay M2.5 x 8 at all ten
//        holes" -- and then every hole was 0.500 diametral oversize and the
//        head bore on less panel than it had to;
//      the screw is M3 -- backpanel.scad's own comment reasoned out loud about
//        the hole "an M3 passes" -- and then it does not enter an M2.5 insert
//        at all and the deck cannot be closed.
//    THE PANEL'S OTHER FOUR M2.5 HOLES WERE ALREADY RIGHT, which is the
//  internal proof of which reading is the design's: backpanel.scad drills
//  cowl_screw_pts() at screw_m25_clear_d into the same insert_m25 pockets
//  (carrier.scad's cowl_insert_cut). Fourteen holes in one plate, one screw,
//  and ten of them drawn for a different one.
//
//  THE DATUM IS THE HOLE THIS ACTUALLY COST SOMETHING. Nine of the ten are
//  slots whose job is to let a screw through while the plate moves; ONE is
//  round and its whole job is to locate the panel (screw_datum_i above). On a
//  nominal O2.500 M2.5 shank, with the laser's own narrow face from the block
//  above at t = back_t:
//      O3.400 as drawn   0.4500 radial play    finished 0.3241 at 2.0 deg
//      O2.900 as drawn   0.2000 radial play    finished 0.0741 at 2.0 deg
//  So the located position tightens by 0.250 mm per side. That is the whole of
//  what the datum was giving away, and it is not a rounding error against a
//  carrier reveal drawn to 1.028..2.782 (carrier_inset, section 5b).
//  Rohm's edge rule gets easier too: 1.5 x hole wants 4.350 from the sheet
//  edge instead of 5.100, against the 7.520 there is.  [DERIVED]
//
//  THE HEAD. Section 6 had no M2.5 head in it at all, while back_t's comment
//  in section 3 already leans on "a cap head is 2.500 tall" in prose and
//  foot.scad sizes its whole retention pocket off both numbers as ft_head_d /
//  ft_head_h locals. Three places, one fact, nowhere declared. Declared here.
screw_m25_head_d = 4.500;       // [STANDARD] ISO 4762 M2.5, dk
screw_m25_head_h = 2.500;       // [STANDARD] ISO 4762 M2.5, k
                                //   acrylic.md 2.5 already records the pair as
                                //   "an ISO 4762 socket cap (O4.5 x 2.5)" from
                                //   [3P fastener-tables]; that table's ISO 4762
                                //   page was re-read for this entry and gives
                                //   ONE nominal per dimension and no max/min,
                                //   so neither number carries a tolerance here
                                //   and neither should be treated as a max.

//  THE BEARING AREA, WHICH IS THE PART THAT CRACKS THE PANEL.
//  At the nine SLOTS the head spans a hole narrower than itself, so it does
//  not bear on an annulus at all -- it bears on the two pieces of its own disc
//  that overhang the slot. Computed as the exact set difference
//  disc(r) \ stadium(L, W), two independent ways that agree to 1e-5:
//  Gauss-Legendre quadrature on the 1-D section, and shapely polygon algebra
//  at 16384 segments per circle.
//
//      head O4.500 over the old 3.400 x 6.400 slot      2.2208 mm2
//      head O4.500 over the new 2.900 x 5.900 slot      3.8300 mm2   1.725 x
//      O6.000 washer over the new 2.900 x 5.900 slot   12.9691 mm2   3.386 x
//      head O4.500 on the O2.900 round datum            9.2991 mm2
//      O6.000 washer on the O2.900 round datum         21.6691 mm2   2.330 x
//
//  THE TWO-SEGMENT CLOSED FORM 2[r^2 acos(d/r) - d sqrt(r^2 - d^2)] IS ONLY
//  SOMETIMES RIGHT, and on the first row it is right by luck. It assumes the
//  slot's parallel sides span the whole head, i.e. sqrt(r^2 - d^2) <= (L-W)/2;
//  past that the slot's end caps are narrower than the slot and solid material
//  intrudes under the head there. At r 2.250, d 1.700 the test reads
//  1.4739 <= 1.500 and the form is exact. At d 1.450 it reads 1.7205 > 1.500
//  and the form is 0.14 % low. For a O6.000 washer, r 3.000, it is 10.73 % low
//  (11.5778 against 12.9691). DO NOT CARRY THE CLOSED FORM TO A BIGGER
//  WASHER; it is the one case where it is most wrong.  [DERIVED]
//
//  THE DECISION: SIX OF THE TEN HOLES GET A WASHER UNDER THE HEAD, AND THE
//  OTHER FOUR MUST NOT.
//  DIN 125 A / ISO 7089 A 2.5 -- d1 2.7, d2 6.000, 0.500 thick -- at
//  back_screw_pts() indices 1, 2, 4, 5, 6 and 7.
//    THE FOUR EXCLUDED ONES ARE THE FEET, indices 0, 3, 8 and 9 = foot_pts()
//  (section 12), and a washer there is not a smaller benefit, it is a failure.
//  foot.scad presses the foot onto the screw HEAD through a blind O4.300 pocket
//  whose mouth opens to O5.100 across its 0.400 lead-in, and the foot's top
//  face bears flat on the panel -- that face IS the standing load path, desk to
//  sole to panel. A O6.000 washer cannot enter a O5.100 mouth. It would land on
//  the foot's top face instead, stand the whole 13.970 x 34.290 pad 0.500 off
//  the panel on one washer, and leave the deck resting on four washers. That is
//  the residual-rock failure foot.scad's own section describes, except 0.500
//  deep instead of a print tolerance, and it would also eat the head-to-pocket-
//  floor air that keeps the clamp out of the TPU: 0.675 becomes 0.175.
//    So at those four the hole correction is the whole of the fix and the slot
//  bearing stays 3.8300 mm2 -- 1.725 x the as-drilled 2.2208, and these are
//  the only four holes where the panel's underside is backed by anything at
//  all: 416.6374 mm2 of TPU top face around the head [MEASURED off
//  export/stl/foot.stl, the 196 up-facing facets at z = 5.715; the 384.6606
//  foot.scad quotes is the SOLE, which is smaller by the sole chamfer and is
//  not the face that touches the panel].
//    If the coupon in acrylic.md 11.3 ever says 3.8300 is not enough, the
//  fix is in foot.scad and not here: open the pocket to clear a washer and take
//  the washer's 0.500 back out of ft_pocket_h, which costs retention.
//    AND THE DATUM IS NOT A FOOT, which is luck worth recording: index 1 is the
//  round hole, so the one hole that locates the panel is also one of the six
//  that gets the washer. Its washer's edge clears foot 0's plan by 1.4450 mm
//  [DERIVED] -- the tightest washer-to-foot approach in the panel, and the
//  first number to go negative if ft_toe or screw_dy moves.
//    IT IS NOT DECIDED ON A STRESS, AND IT CANNOT BE. What 3.175 mm of
//  laser-cut PMMA survives under a screw head is NOT SOURCED in this
//  repository, and acrylic.md 11.3 says so in terms -- "no manufacturer
//  publishes a bearing or torque limit for this joint" -- while 5.2 adds
//  "nothing published says what 1.5 mm of laser-cut acrylic survives, and I
//  will not invent a number". The preload is NOT SOURCED too (below). The only
//  PMMA mechanical property recorded anywhere in this repo is E = 3300 MPa
//  [ROHM xt-gs-ds]: there is no strength of any kind in it, in any file. A
//  washer justified by a compressive strength here would be two invented
//  numbers wearing a division sign.
//    SO THE ARGUMENT IS A RATIO, WHICH SURVIVES BOTH UNKNOWNS. Whatever the
//  preload is and whatever the allowable turns out to be, the washer divides
//  the bearing stress by 3.386 at a slot and by 2.330 at the datum, on top of
//  the 1.725 the hole correction already bought. Hole by hole, against the
//  2.2208 mm2 the panel as drilled would have had at the same preload:
//      index 1, datum, washered       21.6691 mm2   1/9.757
//      indices 2, 4, 5, 6, 7 washered 12.9691 mm2   1/5.840
//      indices 0, 3, 8, 9  the feet    3.8300 mm2   1/1.725
//  The panel's WORST bearing is therefore 3.8300 and not 2.2208, and no factor
//  here needs the absolute number to be worth having.
//    AND THE DIRECTION IS THE MATERIAL MAKER'S OWN, not a preference. Rohm's
//  list of the causes of the tensile stress that crazes PMMA includes
//  "deformation during fastening (clamping, drilling, screw union)"
//  [ROHM mach 8], on a cut edge the same document calls monomer-exposed;
//  acrylic.md 5.2's conclusion for the pot nut is "a washer is necessary and
//  not sufficient", and this is that joint with a smaller head on a thicker
//  sheet.
//    IT IS ALSO FREE IN THE THREAD STACK AT THOSE SIX, and buys margin there.
//  M2.5 x 8 (foot.scad's length) through 3.175 of panel leaves 4.825 into a
//  5.000 pocket -- the tip stands 0.175 off the pocket floor. The washer makes
//  that 4.325 and 0.675, with the full 4.000 of insert engaged either way. So
//  the six washered screws and the four bare ones are the same part number and
//  neither bottoms.  [DERIVED]
//    THE COWL'S OWN FOUR SCREWS NEED NONE, and they are not in these ten:
//  their heads bear on the printed cowl's flange, not on PMMA, and the panel
//  sees that flange over its whole 7.620 width.
//    ONE HONEST LIMIT ON THE 12.9691. A 0.500 washer bridging a 2.900 slot
//  dishes, so that figure is the washer's FOOTPRINT and not a uniform pressure
//  distribution: the real peak sits over the slot's two long edges. It is an
//  upper bound on area and so a lower bound on peak stress, and it is still
//  3.386 x better than no washer. If the coupon in acrylic.md 11.3 ever says
//  this joint needs more, a thicker washer, or an M3 washer's O7.000 on the
//  same M2.5 screw, drops in at the six without redrawing the panel -- but
//  O7.000 would cut the index-1 clearance above from 1.4450 to 0.9450, so
//  check that before fitting one.
//
//  NOTHING DRAWS THESE TWO AND THAT IS CORRECT: a bought washer is not a cut
//  feature and not a printed one, so no part file reads them and no checker
//  gates them. They are here because an assembly decision with arithmetic
//  behind it has to live somewhere a reader of this file will meet it.
screw_m25_washer_od = 6.000;    // [VENDOR] DIN 125 A / ISO 7089 A 2.5, d2,
screw_m25_washer_t  = 0.500;    // [VENDOR] and its nominal thickness. Tagged
                                //   VENDOR and not STANDARD on purpose: the
                                //   figures are a Wuerth DIN 125 A datasheet
                                //   corroborated across four suppliers
                                //   (d1 2.7 / d2 6.0 / 0.5), and the standard
                                //   itself was not retrievable from here. One
                                //   supplier publishes d2 as a 5.7..6.0 band
                                //   and one publishes d1 2.84, so 6.000 is the
                                //   nominal and not a minimum -- which matters
                                //   only if the bearing ratio above is ever
                                //   taken to three figures.

//  THE PRELOAD IS NOT SOURCED, AND TWO ARGUMENTS IN THIS REPOSITORY LEAN ON
//  IT. foot.scad's header rejects its own first draft on "a hand-tight M2.5,
//  about 354 N", and the bearing areas above would be stresses if they had a
//  force. There is no tightening torque anywhere in this repository and no
//  published one to fetch: searched for a recommended torque for an M2.5 screw
//  into a brass heat-set insert in a printed thermoplastic and found none from
//  the insert vendor this project cites ([3P ruthex-3djake]) nor from the house
//  library. What exists is other products -- M2.5 self-tapping insert screws
//  at 0.9..1.5 N m, PEM moulded-in M2.5 inserts at 0.7..0.79 N m torque-out --
//  and neither of those is this joint.
//    AND A TORQUE ALONE WOULD NOT CLOSE IT, because torque becomes preload
//  only through a friction coefficient, which is a second number needing its
//  own tag:
//      T = F [ P/(2 pi) + mu_t d2/(2 cos 30) + mu_b Dm/2 ]
//  For M2.5, P 0.45 and d2 = 2.500 - 0.6495 P = 2.2077, the lead term is
//  0.0716 mm against mu_t 1.2746 + mu_b 1.8500 mm (Dm = 3.700, the O4.500 head
//  over the O2.900 hole; 3.600 once it is on the washer's d1). So friction is
//  77.7 % of the torque at mu 0.08 and 91.6 % at mu 0.25, and F per N m runs
//  3110 N down to 1173 N -- A FACTOR OF 2.65 ON THE ANSWER, set entirely by a
//  number nobody here has measured. mu for a plated steel screw in a brass
//  insert, and for a steel washer on PMMA, is NOT SOURCED as well. (For scale:
//  foot.scad's 354 N corresponds to 0.114 N m at mu 0.08 and 0.302 N m at
//  mu 0.25. Not absurd -- and not determined either.)
//    THE PROCEDURE THAT CLOSES IT, cheapest step first:
//      1. Put a ceiling on it empirically, on the coupon acrylic.md 11.3
//         already asks for: drive an M2.5 into a pressed insert in a PETG boss
//         with a 0..0.5 N m dial wrench and record the torque at which the
//         insert strips. That one measurement is also the open NOT SOURCED
//         item "M2.5 insert pull-out in PETG", and it caps the joint whatever
//         mu is.
//      2. Measure the preload instead of inferring it, which skips mu
//         altogether: a load cell, or the cheap version -- clamp a PMMA coupon
//         of known section and read its compression on a 1 um indicator
//         against E = 3300 MPa [ROHM xt-gs-ds].
//      3. Only with 1 and 2 is there a stress to compare against the panel's
//         own destructive test, and only then does a figure like 354 N earn a
//         tag.
//  Until those exist this file states the AREAS, which are exact, and refuses
//  the stress.  [NOT SOURCED]

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
disp_x        = grid_x - grid_w/2 - gap - disp_board[0]/2;   // -90.065

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
kbd_cap_field    = [284.325, 94.200];   // [CAD-MESH] keycap outer edges, and
                                        //   this is now a MEASUREMENT of all
                                        //   61 caps rather than arithmetic on
                                        //   a nominal cap. Every vertex above
                                        //   the cap's widest section (model
                                        //   y = +5.200) in
                                        //   models/k9pro-internals/
                                        //   k9pro-stripped-assembly.stl spans
                                        //   model X -9.1000..275.2250 and
                                        //   Z 9.9000..104.1000.
                                        //   The same mesh returns the frame as
                                        //   exactly 287.0000 x 97.5000 against
                                        //   a [CAD-EXACT] 287.000 x 97.500, so
                                        //   it does not undersize its extrema
                                        //   and that is the control for this
                                        //   number.
                                        //   WHAT THIS REPLACED: "15u - 1u + an
                                        //   18.000 cap" = 284.000 x 94.000.
                                        //   18.000 is in no record in this
                                        //   repo. THE CAP IS 18.2000 SQUARE
                                        //   [CAD-EXACT], read off Keychron's
                                        //   own B-rep with AddOptimal (1u
                                        //   18.2000 x 18.2000; the 1.25u key
                                        //   22.9500 x 18.2000, and 22.950 =
                                        //   18.200 + 0.25 x 19.000, so the cap
                                        //   and the 19.000 lattice agree
                                        //   exactly). It leaves a 0.800
                                        //   inter-cap gap, which is what a
                                        //   keyboard actually has.
                                        //   AND THE TWO BIGGER FIGURES IN THIS
                                        //   REPO ARE BOTH BOUNDING-BOX
                                        //   ARTEFACTS, not measurements:
                                        //   OCCT's plain BRepBndLib::Add
                                        //   inflates a box by the shape's
                                        //   tolerance and returns 18.2252,
                                        //   which is exactly what the
                                        //   extraction's detail.json carries;
                                        //   and manifest.json's 18.7314 --
                                        //   which keyboard-strip.md 5.2 step 3
                                        //   repeats as "18.731 square" -- is a
                                        //   coarser one. That manifest is
                                        //   provably inflated: it records the
                                        //   frame as 287.3004 x 97.8004 where
                                        //   the frame is 287.000 x 97.500 both
                                        //   [CAD-EXACT] and [VENDOR].
                                        //   In Z the lattice is clean
                                        //   (every row is a 1u-deep cap) and
                                        //   76.000 + 18.200 = 94.200 exactly.
                                        //   In X the envelope is set by the
                                        //   WIDE caps, not the 1u lattice --
                                        //   row 4's outer 1.25u key reaches
                                        //   275.2250 -- which is why X is
                                        //   284.325 and not 284.200, and why
                                        //   the field is not centred (below).
kbd_cap_field_d  = [0.0625, -0.250];    // [CAD-MESH] the cap field's centre in
                                        //   DECK axes relative to the frame's,
                                        //   and it is not zero on either axis.
                                        //   From the spans above: X centre
                                        //   133.0625 vs the frame's 133.000;
                                        //   Z centre 57.0000 vs the frame's
                                        //   56.750 -- and keyboard-strip.md
                                        //   1.6's [CAD-EXACT] row table gives
                                        //   that Z centre independently (rows
                                        //   19/38/57/76/95, so 57.000).
                                        //   THE SIGN MATTERS AND WAS WRONG
                                        //   HERE: kbd_dy() has d/dmz = -1, so
                                        //   +0.250 in model Z is -0.250 in
                                        //   deck y, i.e. the field sits 0.250
                                        //   TOWARDS THE FRONT of the frame,
                                        //   not rearward. The frame's Z bezel
                                        //   is 1.500 at the rear (9.500 -
                                        //   8.000) and 1.000 at the front
                                        //   (105.500 - 104.500), which is the
                                        //   same fact read off the extents.
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
//    3. the recess is acrylic_t + kbd_plate_gap - kbd_cap_h = 3.493, so it is
//       FORCED by half-inch acrylic against a low-profile keycap, not chosen.
//       The gap ADDS and does not subtract: the plate top sits kbd_plate_gap
//       BELOW the acrylic's underside, so the cap top drops by the gap as well
//       and the acrylic above it gets thicker. Written the other way round --
//       acrylic_t - kbd_cap_h - gap -- it gives 2.223, and it is wrong by
//       2 * kbd_plate_gap. ("gap" is also this file's own 28.575 mm board
//       clearance, which is why the operand is named in full here.)
//       The derived kbd_recess below has always been right;
//    4. it collapses the carrier under the keyboard from a 9.000 mm plinth to
//       a 2.015 mm seat, and lands the socket-field pocket floor on the
//       wiring plane to within 0.076 mm;
//    5. it widens the slab's governing web: a smaller void takes less out of
//       the sheet, so the front rail goes 4.400 -> 5.6825. [MEASURED] off
//       export/dxf/slab.dxf -- the void's front straight edge at y -89.5050
//       against the outline's at -95.1875, and the min distance between the
//       two loops anywhere is the same 5.6825, so the front rail IS the
//       narrowest web in the part. 4.400 is front_wall, which is what the
//       rail would be in the FLUSH branch, where the void is kbd_pocket.
//       (The 5.625 this line used to carry was the rail for a 96.000-deep
//       void; at kbd_void[1] = 95.885 it is 5.6825.)
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

kbd_plate_top_z  = acrylic_z0 - kbd_plate_gap;              //  8.890
kbd_seat_z       = kbd_plate_top_z - kbd_seat_below;        //  5.190
kbd_cap_top_z    = kbd_plate_top_z + kbd_cap_h;             // 18.732
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
//    2. keeping 3.37914 deg costs kbd_frame[1] * tan(kbd_wedge_stock_deg)
//       = 97.500 * 0.05904559 = 5.757 mm of extra carrier depth at the rear,
//       on a deck_t of 31.750. [DERIVED] The operand is named because this
//       line twice was not: the cost is measured over the RETAINED FRAME's
//       own depth, 97.500, because the frame is the part being tilted and its
//       rear rim is what the carrier has to grow to clear. It is NOT the
//       plate's 95.500 outline depth (keyboard-strip.md 1.6), which lies
//       inside the frame and sets nothing -- and 95.500 * tan = 5.639 is
//       exactly the figure this line used to carry, so that is where it came
//       from. v2/layouts/README.md quotes the 5.639 downstream and needs the
//       same correction;
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
//  after the laser's +/-0.230. Centred on the frame, not on the cap field,
//  whose centre sits 0.250 TOWARDS THE FRONT of it and 0.0625 to +x --
//  kbd_cap_field_d, and that is a measurement, not an assumption.
kbd_void      = [285.750, 95.885];      // [DESIGN] 450 x grid, 151 x grid
kbd_void_y    = kbd_frame_y;

//  THE VOID'S CORNER RADIUS. Bracketed from both sides: too big and the
//  void's corner swings outside the frame's R4.400 and the overlap goes
//  negative; too small and the squarer corner closes on the corner keycap.
//
//  THIS PARAGRAPH USED TO CARRY TWO NUMBERS AND BOTH WERE READ ON THE WRONG
//  CORNER. It said the cap clearance was +0.139 and the window closed at
//  r <= 3.512. Both are the LOOSE corner: they follow from a Y gap of
//  1.1925 = 0.9425 + 0.250, i.e. the cap field displaced and then measured on
//  the side the displacement helps. The field is displaced TOWARDS THE FRONT,
//  so the governing corner is the FRONT one at 0.9425 - 0.250, and against a
//  perfectly square cap it does not clear at all. Measured off
//  export/dxf/slab.dxf with the artefact's own 96-gon arc, square cap, field
//  displaced by kbd_cap_field_d:
//      an 18.000 cap   FRONT -0.2110   REAR +0.1376  <- the old "0.139"
//      the real 18.200 cap, at the measured envelope corner       -0.4376
//  (read CONCENTRICALLY, which is what check_cad.py computes, an 18.000 cap
//  gives -0.0316 -- also not the governing corner, just a different wrong one.)
//
//  SO WHY IS R3.175 STILL RIGHT? Because the clash is in the SQUARE CAP, not
//  in the part, and the square cap is no longer the best available fact. Every
//  vertex of all 61 caps above the cap's widest section, mapped by this file's
//  own kbd_dx/kbd_dy and measured against the exported polygon:
//      WORST REAL CLEARANCE  +0.3691  at deck (+141.2242, -88.6843)
//      = model X 274.224, Z 103.872 -- the front-right 1.25u cap's own rounded
//      corner, which is the corner that governs. Per quadrant: front +0.3691
//      and +0.3745, rear +0.6500 and +0.6995.
//  The gap between -0.4376 and +0.3691 is the keycap's rounded corner, and the
//  reason the old note could call that "NOT SOURCED" is that it was looking
//  for the wrong kind of number. THE CAP HAS NO CORNER RADIUS TO SOURCE: its
//  77 faces include six cylinders and NOT ONE of them has a vertical axis, so
//  the plan corner is a swept blend on a tapered shell, not a fillet, and its
//  effective plan radius varies with height. A support-function fit of the
//  plan envelope over 720 directions gives an effective rc = 1.683 at rms
//  0.0068 (1.742 with the half-width pinned to 9.100) -- useful for intuition
//  and NOT what the +0.3691 rests on. That figure is every cap VERTEX against
//  the exported polygon, so it needs no radius, no fit and no idealised cap at
//  all. The honest statement is not "assume a radius" but "stop needing one".
//
//  AND THE SQUARE-CAP BRANCH IS NOT AVAILABLE ANYWAY. Shrinking r until the
//  void clears a square cap is the move that removes the dependency, and it
//  was solved rather than assumed. Both inequalities, against the measured
//  cap and the [CAD-EXACT] R4.400 frame:
//      clears a SQUARE cap at the measured envelope corner   r <= ~2.11
//      corner overlap still positive after a +0.230 cut      r >= 2.522
//  THOSE DO NOT INTERSECT. There is no corner radius that clears a square cap
//  and also keeps the frame overlap this file already commits to -- so the
//  dependency on the cap's corner cannot be removed, only measured, and it is
//  measured. The window is empty for every cap width at or above 18.200, so no
//  value on the 0.635 grid fits it either: 2.540 (4 x grid) leaves the square
//  cap at -0.1748, and 1.905 (3 x grid) has the frame overlap already negative
//  at -0.0238 before any kerf.
//
//  3.175 (5 x grid) is also where the real trade-off is flattest. Taking the
//  worse of (real cap clearance, corner overlap after a +0.230 cut):
//      2.800 -> 0.1135    3.000 -> 0.1945    3.175 -> 0.2640    3.500 -> 0.2431
//  so 3.175 is at the max-min and on the grid, which is the rare case where
//  the tidy number is also the right one. It gives 0.494 of frame overlap
//  nominal and 0.264 after the laser's worst outboard excursion.
kbd_void_r    = 3.175;          // [DESIGN]

//  Straight-edge clearances, per side. ASYMMETRIC in both axes, because the
//  cap field is not centred on the frame and the void is. The old 2-vector
//  computed as if they were concentric and so could not show the tight side.
kbd_cap_clear = [(kbd_void[0] - kbd_cap_field[0])/2 - kbd_cap_field_d[0],
                 (kbd_void[0] - kbd_cap_field[0])/2 + kbd_cap_field_d[0],
                 (kbd_void[1] - kbd_cap_field[1])/2 + kbd_cap_field_d[1],
                 (kbd_void[1] - kbd_cap_field[1])/2 - kbd_cap_field_d[1]];
                                // [DERIVED] +x, -x, FRONT, REAR
                                //   0.650, 0.775, 0.5925, 1.0925
                                //   front is the tight one and it is the one
                                //   the corner argument above turns on
kbd_overlap   = [(kbd_frame[0] - kbd_void[0])/2,
                 (kbd_frame[1] - kbd_void[1])/2];       // 0.625, 0.8075

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
//  above it is 5.6825 wide (the void shrank, so the front rail grew from
//  front_wall's 4.400; [MEASURED] off export/dxf/slab.dxf, section 8a item 5),
//  and the wrist load is compression on a 2.495 x 9.525 PETG web with
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
// 12. LAZY DATUMS: THE 18650 COWL, AND WHERE THE FOUR FEET GO
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

//  The window, shared by all three parts. 87.000 x 21.103 is the 18650
//  holder's own footprint on the rear face, [MEASURED] -- v2/parts/
//  board-models.md 2.8, which reads it off the vendor STEP by B-rep query
//  ("[step]" in that document's tag vocabulary) as U 2.748..89.748 and
//  V 4.600..25.703. That is the source for BOTH numbers here and for the
//  centres below; this section used to state the spans without naming where
//  they came from.
//    ONE CAVEAT, AND IT IS THE DOCUMENT'S OWN. board-models.md 5 item 6
//  records that v1 carries 77.800 for the holder's length along U,
//  caliper-confirmed on hardware, against the 87.000 the STEP gives. The two
//  are measuring different things -- body against footprint including the end
//  contacts -- and that is NOT RECONCILED. 87.000 is the conservative value
//  in every role it plays: a window too wide only enlarges a cover's own
//  aperture, so the unreconciled 9.200 costs nothing here. It would matter if
//  this window ever became a structural edge; it is not one.
function cowl_window() = [87.000 + 2*pocket_fit, 21.103 + 2*pocket_fit];

//  Where it is. [MEASURED], same source: the spans above put the holder's
//  centre at (46.248, 15.152) on the display's own board, against a board
//  centre of (46.250, 34.550) -- so the holder is centred in U to within
//  0.002 and sits 19.398 off centre in V, hard against the 18650 edge.
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

//  WHERE THE FOUR FEET GO. A FUNCTION FOR THE SAME REASON AS EVERYTHING ELSE
//  IN THIS SECTION: it reads slab_screw_pts() and back_screw_kbd out of
//  section 9b, so as a variable it would have to live after them and could
//  never be moved. It also has to be callable from a part file and from
//  export.py's echo dump, which evaluates this file with OpenSCAD itself
//  rather than parsing it.
//
//  EVERY FOOT LANDS ON A FASTENER THAT ALREADY EXISTS -- a screw up through
//  the back panel into an M2.5 insert in the carrier's floor -- because the
//  panel is 3.175 laser-cut acrylic, a laser cannot counterbore, and a foot
//  bonded to the one part that is also the service access is a foot that
//  peels off. So back_screw_pts() is the whole menu and these are four of its
//  ten. foot.scad's header has the tipping arithmetic that picks these four
//  and not four corners; in one line, the front pair are the furthest forward
//  holes that exist, because the heels of the hands land forward of any of
//  them.
//      [ 144.780,  80.7125]   slab_screw_pts()[0]   the rear pair, on the
//      [-144.780,  80.7125]   slab_screw_pts()[3]     slab's own screw line
//      [ -82.550, -63.500]    back_screw_kbd[2]     the front pair
//      [  82.550, -63.500]    back_screw_kbd[3]
//  [DERIVED, and echoed: OpenSCAD returns exactly those four pairs from the
//  expression below.] Placing a foot is translate([x, y, deck_bot_z - foot_h])
//  with NO rotation -- foot.scad is drawn in the orientation it is fitted in,
//  sole on z = 0 and screw axis on the origin, and all four go on the same way
//  round with the toe toward -Y.
function foot_pts() = [slab_screw_pts()[0], slab_screw_pts()[3],
                       back_screw_kbd[2],   back_screw_kbd[3]];
