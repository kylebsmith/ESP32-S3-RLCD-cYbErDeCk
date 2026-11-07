// ===========================================================================
//  v2/tools/render/deck_params.scad
//  ECHO-ONLY. Dumps every datum the three.js scene needs, read straight out of
//  v2/cad/parameters.scad. Emits no geometry.
//
//      openscad --export-format=echo -o out.echo v2/tools/render/deck_params.scad
//
//  WHY THIS FILE EXISTS AT ALL
//  ---------------------------
//  The renderer needs numbers: where the keyboard sits, how thick the acrylic
//  is, which z the wiring plane is at. There are exactly two ways to get them
//  into JavaScript and only one of them is honest.
//
//    * retype them into scene.js  -- creates a second copy of the single
//      source of truth, which drifts silently and makes the render a picture
//      of a deck that no longer exists;
//    * ask OpenSCAD.             -- this file.
//
//  So NO DIMENSION IS WRITTEN HERE. Every line below is a name defined in
//  parameters.scad (or arithmetic on such names, shown inline), echoed out.
//  A typo becomes "unknown variable" and `undef` in the dump, which
//  render.py treats as fatal -- the same rule export.py applies.
//
//  The only numeric literals permitted in this file are the ones that appear
//  inside a derivation already written in parameters.scad or carrier.scad, and
//  each one carries the file and the reason.
// ===========================================================================

include <../../cad/parameters.scad>

//  One echo per datum, "K <name>=<value>". render.py parses that and nothing
//  else, so adding a line here is all it takes to expose a new parameter.
module K(n, v) { echo(str("K ", n, "=", v)); }

// --------------------------------------------------------------- the stack
K("acrylic_t",     acrylic_t);
K("acrylic_z0",    acrylic_z0);
K("carrier_t",     carrier_t);
K("carrier_rim_h", carrier_rim_h);
K("back_t",        back_t);
K("deck_t",        deck_t);
K("deck_top_z",    deck_top_z);
K("deck_bot_z",    deck_bot_z);
K("pcb_t",         pcb_t);
K("board_rear_z",  board_rear_z);
K("pcb_front_z",   pcb_front_z);

// ---------------------------------------------------------------- the plan
K("body_w",     body_w);
K("body_h",     body_h);
K("carrier_w",  carrier_w);
K("carrier_d",  carrier_d);
K("corner_r",   corner_r);
K("band_mid",   band_mid);
K("interior_y1", interior_y1);

// ------------------------------------------------------- the control field
K("cols",      cols);
K("enc_y",     enc_y);
K("pot_y",     pot_y);
K("slide_x",   slide_x);
K("bore_d",    bore_d);
K("knob_d",    knob_d);
K("knob_proud", knob_proud);
K("knob_h",     knob_h);
K("pat_a",     pat_a);
K("pat_b",     pat_b);

// -------------------------------------------------------------- the 562 pot
K("pot_web_z",   pot_web_z);
K("pot_shoulder_below", pot_shoulder_below);
K("pot_z_bottom", pot_z_bottom);
K("pot_above",   pot_above);

// ---------------------------------------------------------- the fader slot
K("slider_slot_l", slider_slot_l);
K("slider_slot_w", slider_slot_w);
K("slider_cap_w",  slider_cap_w);

// ------------------------------------------------------------ the display
K("disp_mount_z",  disp_mount_z);
K("disp_standoff", disp_standoff);
K("disp_front_z",  disp_front_z);
K("disp_x",        disp_x);
K("disp_stand",    disp_stand);
K("disp_active",   disp_active);

// ------------------------------------------------------------- the Feather
K("feather",             feather);
K("feather_x",           feather_x);
K("feather_holes_usb",   feather_holes_usb);
K("feather_holes_span",  feather_holes_span);

// ------------------------------------------------------------ the keyboard
K("kbd_frame",     kbd_frame);
K("kbd_frame_y",   kbd_frame_y);
K("kbd_seat_z",    kbd_seat_z);
K("kbd_cap_top_z", kbd_cap_top_z);
K("kbd_recess",    kbd_recess);
K("kbd_void",      kbd_void);
K("kbd_void_y",    kbd_void_y);
K("kbd_tilt_deg",  kbd_tilt_deg);

// ------------------------------------------------- the fasteners, for labels
K("screw_x",   screw_x);
K("screw_dy",  screw_dy);
K("foot_h_min", foot_h_min);

// ===========================================================================
//  PLACEMENTS -- where each reference body goes in the deck frame.
//
//  A placement is six numbers: [tx, ty, tz, rx, ry, rz], degrees, applied as
//
//      p_deck = T(tx,ty,tz) . Rz(rz) . Ry(ry) . Rx(rx) . p_body
//
//  and scene.js composes exactly that matrix, in that order. Every rotation
//  here is a multiple of 90 deg, so every one of them is a PROPER rotation
//  (determinant +1) and no mesh is mirrored: a mirrored mesh has inverted
//  winding, renders inside-out, and is the classic way a render stops being
//  of the part.
//
//  Each body's own frame is quoted from the header of the .scad that defines
//  it; the derivation that maps it into the deck frame is shown.
// ===========================================================================
module P(n, t, r) { echo(str("P ", n, "=", concat(t, r))); }

// -------------------------------------------------------------- the keyboard
//  k9pro_internals.scad frame:  x 0..287.000 from the LEFT outer edge,
//  y 0..97.500 with +y REARWARD, z = 0 at the PCB underside (the seat).
//  The deck frame agrees in handedness and in the sense of every axis, so the
//  map is a pure translation:
//      deck_x = body_x - kbd_frame[0]/2
//      deck_y = body_y + kbd_frame_y - kbd_frame[1]/2
//      deck_z = body_z + kbd_seat_z
//  which is parameters.scad's own kbd_dx()/kbd_dy() composed with the
//  model->internals conversion printed in k9pro_internals.scad's header
//  (x = model_x + 10.500, y = 105.500 - model_z, z = model_y + 3.700).
P("keyboard",
  [-kbd_frame[0]/2, kbd_frame_y - kbd_frame[1]/2, kbd_seat_z],
  [0, 0, kbd_tilt_deg]);

// --------------------------------------------------------------- the display
//  waveshare_rlcd42.scad frame: x 0..92.500 (x = 92.500 is the USB-C edge),
//  y 0..69.100, z = 0 at the PCB's REAR face with +z toward the display.
//  carrier.scad places the board CENTRED on [disp_x, band_mid] in plan with
//  no rotation (its disp_18650_cut and disp_screw_cut are both written as
//  disp_x + <body x> - disp_pcb[0]/2), and its four SMTSO standoffs land on
//  disp_mount_z, which puts the PCB's rear face disp_standoff above that.
//
//  The body's frame runs from a CORNER, not from its centre, so the
//  translation is [disp_x - disp_pcb[0]/2, band_mid - disp_pcb[1]/2, ...] and
//  NOT [disp_x, band_mid, ...]. Writing the latter put the board a whole
//  half-width out into the rear wall, which is how this placement was wrong
//  on the first run: the scene bounds reached y = 118.062 on a part whose
//  footprint stops at body_h/2 = 95.188, and that is what caught it.
//
//  THE SIZE IS THE PCB'S, NOT THE STAND BASE'S. disp_stand[] is the acrylic
//  stand base and it is 1.003 wider in Y than the board it sits on
//  (parameters.scad section 7), so it is the wrong number for centring the
//  board model. parameters.scad's own disp_pcb_sz() is the right one.
K("disp_pcb_sz", disp_pcb_sz());
P("display",
  [disp_x - disp_pcb_sz()[0]/2, band_mid - disp_pcb_sz()[1]/2,
   disp_mount_z + disp_standoff],
  [0, 0, 0]);

// --------------------------------------------------------------- the Feather
//  feather_rp2040_dvi.scad frame: x 0..50.800 (x = 0 is the USB-C end,
//  x = 50.800 the HDMI end), y 0..22.860, z = 0 at the PCB's BOTTOM face.
//  carrier.scad turns it 90 deg so the HDMI reaches the rear rim, and
//  feather_bosses() puts the two M2.5 at feather_y - feather_holes_span/2,
//  i.e. the USB end toward -y. Rz(+90) maps body +x to deck +y, which is the
//  rearward sense the HDMI needs. Its bosses' tops are at z = 0, so the PCB's
//  bottom face lands on board_rear_z.
//      tx = feather_x + feather[0]/2   (Rz(90) sends body y -> deck -x)
//      ty = feather_y - feather[1]/2   (Rz(90) sends body x -> deck +y)
//  feather_y lives in carrier.scad, so this placement is emitted there.

// -------------------------------------------- the 5880 encoders, x 3 columns
//  House-library mesh frame, MEASURED off the mesh by render.py: x 0..25.400,
//  y 0..25.400, z -21.400..+4.530, where z = 0 is the PCB's FRONT face and
//  +z runs REARWARD (the shaft is at -21.400, confirming enc_above = 21.400;
//  the right-angle STEMMA QT at +4.530 confirms pcb_t + enc_qt_below).
//  So the library's +z is the deck's -z: Rx(180) is the proper rotation that
//  fixes it, and it also sends library +y to deck -y, so the translation
//  below is written for the mesh's MAX corner, not its min.
//      deck_z = pcb_front_z - body_z   -> tz = pcb_front_z
//  pat_a is the board's square footprint, so pat_a/2 centres it on the column.
for (i = [0 : len(cols) - 1])
    P(str("encoder", i),
      [cols[i] - pat_a/2, enc_y + pat_a/2, pcb_front_z],
      [180, 0, 0]);

// -------------------------------------------------- the 562 pots, x 3 columns
//  House-library mesh frame: the shaft axis is the origin in plan and +z is
//  the direction the shaft points, i.e. the deck's +z. No rotation.
//  Z is anchored on the mesh's own minimum -- the can's bottom -- against
//  pot_z_bottom, which parameters.scad derives as pot_web_z -
//  pot_shoulder_below. render.py measures the mesh and solves
//      tz = pot_z_bottom - measured_body_zmin
//  so this placement carries only the plan position and the anchor name.
for (i = [0 : len(cols) - 1])
    P(str("pot", i), [cols[i], pot_y, 0], [0, 0, 0]);

// ------------------------------------------------------------ the NeoSlider
//  House-library mesh frame: x -38.100..38.100, y -10.795..10.795 (already
//  centred, and it is pat_b), z -15.500..+4.530 with the same rearward +z as
//  the encoder. carrier.scad places it with rotate(90) at [slide_x, band_mid],
//  so the composition is Rz(90) . Rx(180) -- determinant +1, no mirror.
P("neoslider", [slide_x, band_mid, pcb_front_z], [180, 0, 90]);
