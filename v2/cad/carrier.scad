// ===========================================================================
//  cYbErDeCk v2 - carrier.scad
//  The printed carrier plate: the floor, the rim, and every mount in the deck.
//
//  THIS PART'S TOP FACE IS A VISIBLE SURFACE. Under 12.700 mm of CLEAR acrylic
//  every board, every boss and every wire is seen, and the brief is explicit
//  that the interior design matters as much as the panel:
//
//      "the traces laid out actually in the 3D print pointed up at the user so
//       you can see the wiring moving through the board organized flawlessly"
//
//  So two rules govern the whole file:
//
//    1. THE PLANE STAYS A PLANE. Nothing stands on z = 0 in the open. Every
//       breakout's rear face lands on bosses whose tops are at the BOTTOM of a
//       seat pocket, so the fastening reads as holes in the plane rather than
//       pillars on it. v2/parts/stack.md 5.4: "the moment a standoff pillar
//       appears on the visible plane, the plane stops being a plane."
//    2. EVERY CHANNEL IS ORTHOGONAL, ON THE GRID, AND VISIBLE. No channel
//       passes under a board, because a channel you cannot see is the one
//       thing this construction cannot afford. The route geometry is the same
//       geometry v2/layouts/carrier.py draws.
//
//  PRINT ORIENTATION: top face UP, on the bed, so the wiring plane is the
//  ironed surface and the pockets are all downward-facing. The rim then prints
//  as a 9.525 mm upstand, the bosses as short columns, and nothing needs
//  support except the pot webs, which bridge their own pockets.
//
//      openscad -D 'part="3d"' -o export/stl/carrier.stl v2/cad/carrier.scad
//
//  THE PART IS 300.790 x 186.565 AND IT IS DRAWN 1.905 INBOARD OF THE ACRYLIC
//  ON PURPOSE. The acrylic holds +/-0.230 and this holds +/-0.5 % = +/-1.523
//  on 304.600, so a flush edge would mismatch by up to 1.753 mm, visibly, and
//  sometimes the wrong way. Inboard, the tolerance stops being a step and
//  becomes a reveal. v2/docs/fabrication.md 0.11.
// ===========================================================================

include <parameters.scad>
use <lib/util.scad>

part = "3d";

// ---------------------------------------------------------------------------
//  PLAN HELPERS
// ---------------------------------------------------------------------------
function pat_a_sq() = [pat_a, pat_a];

module outline_2d() { rrect(carrier_w, carrier_d, carrier_inset_r); }

module interior_2d() {
    translate([0, (interior_y0 + interior_y1)/2])
        square([interior_w, interior_h], center = true);
}

//  model (X, Z) of the keyboard -> deck (x, y). Declared in parameters.scad as
//  kbd_dx()/kbd_dy(); used here so no keyboard position is ever retyped.
function kxy(p) = [kbd_dx(p[0]), kbd_dy(p[1])];

function kbd_centre() = [0, kbd_frame_y];

// ---------------------------------------------------------------------------
//  ADDITIVE: the floor and the perimeter rim
// ---------------------------------------------------------------------------
module floor_solid() {
    translate([0, 0, -carrier_t]) linear_extrude(carrier_t) outline_2d();
}

module rim_solid() {
    linear_extrude(carrier_rim_h) difference() { outline_2d(); interior_2d(); }
}

//  The spine between the keyboard bay and the control band. It is only 1.973
//  wide and it earns its place twice: it stops the keyboard's toggle caps --
//  which stand 1.110 proud of the frame's own rear wall -- from reaching into
//  the band, and it gives the acrylic a bearing line across the middle of a
//  288 mm span. It is notched through at the USB-C.
module spine_solid() {
    difference() {
        translate([0, (kbd_spine_y0 + band_y0)/2])
            linear_extrude(carrier_rim_h)
                square([interior_w, band_y0 - kbd_spine_y0], center = true);
        translate([kbd_dx(123.500), kbd_spine_y0 - 1, -1])
            cube([kbd_corridor_w, (band_y0 - kbd_spine_y0) + 2,
                  carrier_rim_h + 2]);
    }
}

// ---------------------------------------------------------------------------
//  ADDITIVE: the keyboard's seat plinth
// ---------------------------------------------------------------------------
//  It replaces the ABS tray's 33 support pads, which carry Keychron's
//  282 x 93 mm, 1.200 mm thick PCB on 261.350 mm2 -- 1.177 % of its area. A
//  printed plate can offer a PLANE where an injection-moulded tray could only
//  offer ribs, so the seat is continuous wherever the board's underside is
//  clear and pocketed only where it is not. That is strictly better support
//  than the part it replaces, and it is the one place where reproducing the
//  vendor exactly would have been the worse choice.
module kbd_plinth_solid() {
    translate(kbd_centre())
        linear_extrude(kbd_seat_plane_z)
            rrect(kbd_plinth[0], kbd_plinth[1], kbd_frame_r - kbd_plinth_inset);
}

//  The 7 screwed bosses and the 4 unscrewed plate props, at Keychron's own
//  positions, so the stripped assembly bolts in with its own screws through
//  its own holes. O4.000 passes the PCB's O5.000 holes on the way to the plate.
module kbd_bosses_solid() {
    for (p = kbd_screw_mxz) translate(concat(kxy(p), [kbd_seat_plane_z - eps]))
        cylinder(d = kbd_boss_od, h = kbd_boss_top_z - kbd_seat_plane_z + eps);
    for (p = kbd_prop_mxz) translate(concat(kxy(p), [kbd_seat_plane_z - eps]))
        cylinder(d = kbd_boss_od, h = kbd_boss_top_z - kbd_seat_plane_z + eps);
}

//  Ribs across the socket-field pocket, on the clear band half way between
//  switch rows. Each is 1.270 = exactly 3 x 0.4233 extrusion widths, so it
//  prints solid with no infill and no gap-fill.
module kbd_ribs_solid() {
    for (mz = kbd_rib_mz)
        translate([kbd_dx(kbd_socket_field_cx), kbd_dy(mz), kbd_socket_z])
            linear_extrude(kbd_seat_plane_z - kbd_socket_z)
                square([kbd_socket_field[0], rib_t], center = true);
}

// ---------------------------------------------------------------------------
//  ADDITIVE: the three pot towers
// ---------------------------------------------------------------------------
//  A panel-mount pot is carried by its panel and the 562's bushing is 6.600,
//  so the slab cannot carry it. The carrier grows a web instead: a 3.810 plate
//  whose UNDERSIDE at z = 5.080 is the face the pot's shoulder bears on, with
//  the nut above it, inside the knob's O12.000 counterbore. The body hangs in
//  a blind pocket with 2.230 of floor under it.
pot_tower_sz  = [24.130, 31.750];       // [DESIGN] 38 x 50 grid
pot_pocket_sz = [19.050, 26.670];       // [DESIGN] 30 x 42 grid -- sized to the
                                        //   LUG SPAN (25.336), not the 16.000
                                        //   body, because the lugs are the
                                        //   widest thing and they sit at
                                        //   z 0.884..1.880, inside this wall

module pot_tower_solid() {
    for (cx = cols) translate([cx, pot_y])
        linear_extrude(pot_web_z + pot_web_t)
            rrect(pot_tower_sz[0], pot_tower_sz[1], corner_r);
}

// ---------------------------------------------------------------------------
//  ADDITIVE: the six M3 bosses that take the slab
// ---------------------------------------------------------------------------
//  wall = 8.125 is thick enough for a O4.000 M3 insert bore in the abstract
//  (4.000 + 2 x 1.600 = 7.200) and the POSITION is still wrong: screw_x is
//  only 0.605 outboard of the interior face, so the bore crosses it by 1.395
//  and leaves ZERO wall inboard. So the boss is mandatory, not cosmetic -- and
//  it is drawn as a local thickening of the rim's inner face rather than a
//  free-standing pillar, which is also what rule 1 above requires.
//  v2/docs/fabrication.md 2.4.
module slab_boss_solid() {
    for (p = slab_screw_pts()) translate(p)
        cylinder(d = boss_m3_od, h = carrier_rim_h);
}

// ---------------------------------------------------------------------------
//  SUBTRACTIVE: the keyboard
// ---------------------------------------------------------------------------
//  The annulus the frame's skirt hangs in. No groove is drawn: the rim's own
//  inner face is the outer wall and the plinth's face is the inner one.
module kbd_annulus_cut() {
    translate([0, kbd_frame_y, kbd_annulus_z])
        linear_extrude(carrier_rim_h - kbd_annulus_z + 1)
            difference() {
                rrect(kbd_pocket[0], kbd_pocket[1], kbd_frame_r);
                rrect(kbd_plinth[0], kbd_plinth[1],
                      kbd_frame_r - kbd_plinth_inset);
            }
}

//  THE POCKET LEAVES AN ISLAND UNDER EVERY BOSS, and it stops exactly AT the
//  seat plane. Drawn without the islands it deletes the bottom of all eleven
//  plate bosses and leaves them floating in mid-air -- which is what the first
//  render did, and the only reason it was caught is that the mesh came back as
//  twelve separate bodies instead of one. A boss that floats looks identical
//  in a picture. Keychron's own bosses sit in the webs between switch cutouts,
//  which run about 9.8 mm wide, so a O7.000 island fits inside every one.
kbd_island_d = kbd_boss_od + 2*1.500;   // [DESIGN] O7.000, boss + 1.500 of web

module kbd_socket_cut() {
    translate([0, 0, kbd_socket_z])
        linear_extrude(kbd_seat_plane_z - kbd_socket_z) difference() {
            translate([kbd_dx(kbd_socket_field_cx),
                       kbd_dy(kbd_socket_field_cz)])
                square(kbd_socket_field, center = true);
            for (p = concat(kbd_screw_mxz, kbd_prop_mxz))
                translate(kxy(p)) circle(d = kbd_island_d);
        }
}

//  The cell. No compression, no screws through the bay, nothing that can pinch
//  when it closes -- and 0.600 of swelling room, because a pouch gains
//  thickness with age.
module kbd_cell_cut() {
    translate(concat(kbd_cell_xy, [kbd_cell_z]))
        linear_extrude(kbd_seat_plane_z - kbd_cell_z + 1)
            square([kbd_cell[0] + 1.000, kbd_cell[1] + 1.000], center = true);
}

//  The two toggle slider caps are the DEEPEST things on the keyboard's board
//  -- 4.560 below the seat, deeper than the sockets and deeper than the USB-C
//  -- and they also stand 1.110 behind the frame's own rear wall.
module kbd_toggle_cut() {
    z = kbd_seat_z - kbd_toggle_below;
    translate([kbd_dx(2.715) - 1.000, kbd_dy(15.856) - 1.000, z])
        cube([kbd_toggle_relief[0] + 2.000,
              (kbd_dy(6.890) - kbd_dy(15.856)) + 2.000,
              kbd_seat_plane_z - z + 1]);
}

//  Pilot holes for the stock self-tapping screws, cut DOWNWARD into the boss
//  from its top. IT USED TO REMOVE NOTHING. The translate was
//  kbd_boss_top_z + 1 with the cylinder extruded 10.000 UP, so it occupied
//  z +8.681..+18.681 while the boss top is +7.681: the whole cut stood in air
//  above the part, and the paragraph below described a feature the part did
//  not have. MEASURED off export/stl/carrier.stl before the fix, the +7.681 UP
//  plane was 138.1314 mm2 over 11 regions with ZERO interior rings, and
//  11 x the O4.000 96-gon's 12.5574 is 138.1314 exactly -- eleven solid discs.
//  check_cad.py's own row for that plane calls itself THE ROW THAT CATCHES THE
//  FLOATING-BOSS BUG, and it could not catch this one, because an area is the
//  same whether the discs are solid or whether the pilots were never drawn:
//  only the ring count tells them apart, and it recorded the area.
//    O1.700 leaves 1.150 of wall in a O4.000 boss, which is under the house
//  1.600 -- but that figure is for a O4.000 INSERT bore, not a 2 mm
//  self-tapper, and PETG self-taps acceptably at this size. The stock screws
//  are already the right length, so the first build uses them and buys no new
//  hardware.
//    TWO OF THE SEVEN HAVE LESS WALL THAN THAT, and it is pre-existing.
//  [MEASURED off the export by intersecting every triangle with a horizontal
//  plane and taking the distance from the boss axis to the nearest material
//  boundary] bosses 3 and 5, at (+/-47.500, -0.3125), measure 1.999 at z
//  +7.000 and only 1.692 from z +6.000 down, because kbd_cell_cut's overshoot
//  ceiling at kbd_seat_plane_z + 1.000 = +6.181 already undercuts them by
//  0.308 -- the condition check_cad.py's +6.181 row records in its own words as
//  "they undercut the spine and two bosses". So those two carry 0.842 of wall
//  below +6.181 against 1.150 at the other five, and only the pilot's top
//  1.500 is at the full wall. The cut does not create that; it is what the
//  undercut means for a boss that is finally hollow.
module kbd_pilot_cut() {
    for (p = kbd_screw_mxz)
        translate(concat(kxy(p), [kbd_boss_top_z - kbd_boss_pilot_depth]))
            cylinder(d = kbd_boss_pilot_d, h = kbd_boss_pilot_depth + 1);
}

// ---------------------------------------------------------------------------
//  SUBTRACTIVE: the display -- which is also the brain
// ---------------------------------------------------------------------------
//  The pocket floor is where the four SMTSO-M2.5-7ET standoffs land. Waveshare
//  ships a stand base that bolts to those same four; it is DISCARDED and this
//  pocket floor replaces it, which is why the pocket is sized to the PCB
//  (92.500 x 69.100) and not to the base's 70.103.
module disp_pocket_cut() {
    translate([disp_x, band_mid, disp_mount_z])
        linear_extrude(carrier_rim_h - disp_mount_z + 1)
            square([disp_pcb_sz()[0] + 2*pocket_fit, disp_pcb_sz()[1] + 2*pocket_fit],
                   center = true);
}

//  M2.5 clearance, from below. The screw heads live inside O5.500 holes in the
//  back panel, so no counterbore is needed in a floor that has only 3.175 left
//  under this pocket.
module disp_screw_cut() {
    for (sx = [-1, 1], sy = [-1, 1])
        translate([disp_x + sx * 85.500/2, band_mid + sy * 62.100/2,
                   -carrier_t - 1])
            cylinder(d = screw_m25_clear_d,
                     h = (disp_mount_z + carrier_t) + 2);
}

//  P1, the 2x8 expansion header, is on the board's REAR face and reaches 8.700
//  behind it -- 4.845 below the wiring plane, with only 1.505 of floor under
//  it. So it gets a through-slot, which is wanted anyway: the display's 16-way
//  is the one harness that cannot start on the visible plane, and the slot
//  lets it dive under the board and come straight back up.
//  NOTE the orientation. The header's long axis is along U = our x, 20.320
//  long and 5.080 across, centred 0.101 left and 0.698 forward of the board's
//  own centre. v2/layouts/carrier.py draws it 90 degrees to this.
module disp_hdr_cut() {
    translate([disp_x - 0.101, band_mid - 0.698, -carrier_t - 1])
        linear_extrude(carrier_t + disp_mount_z + 2 + abs(disp_mount_z))
            square([22.860, 7.620], center = true);
}

//  The 18650 holder reaches 15.200 behind the board's rear face -- 6.500
//  deeper than anything else on it, and 1.820 BELOW the deck's own underside.
//  It cannot be contained; v1 built a cowl around exactly this fact, and the
//  cowl is a separate printed part this file does not draw. What this file
//  owes it is the window.
module disp_18650_cut() {
    translate(concat(cowl_xy(), [-carrier_t - 1]))
        linear_extrude(carrier_t + 10) square(cowl_window(), center = true);
}

// ---------------------------------------------------------------------------
//  SUBTRACTIVE: the breakout seats
// ---------------------------------------------------------------------------
//  Each breakout drops into a pocket 3.460 deep -- the 2.960 right-angle
//  STEMMA QT behind its rear face, plus 0.500 -- and lands on four boss tops
//  at z = 0. The pocket does two jobs the brief asked for separately: it is
//  the XY registration, and it puts the mounting bosses BELOW the plane
//  instead of on it. The QT's side is not in any source we have, so the whole
//  footprint is relieved rather than guessing which edge it leaves from.
brk_seat_d = 3.460;                     // [DERIVED] 2.960 + 0.500

//  Same trap as the keyboard's: drawn as a plain box this pocket swallows its
//  own four bosses. The islands make each boss part of the pocket's WALL
//  instead -- a scallop, not a pillar.
//    And the island is 1.000 mm wider than the boss, which is not slop. On
//  Adafruit's 25.400 pattern a O6.350 boss at +/-10.160 reaches exactly
//  13.335, and the 26.670 pocket's wall is at exactly 13.335: the circle is
//  TANGENT to the square, the 2D difference degenerates to a point contact,
//  and CGAL answers "the given mesh is not closed" and silently drops the
//  part. The NeoSlider's 16.510 row does the same thing on the other axis.
//  Two exact tangencies, both arising from two correct imperial numbers
//  meeting, and the extra 1.000 makes the island cross the wall instead of
//  kissing it.
brk_island_d = boss_m2_od + 1.000;

module brk_seat(sz, holes) {
    translate([0, 0, -brk_seat_d]) linear_extrude(brk_seat_d) difference() {
        square([sz[0] + 2*pocket_fit, sz[1] + 2*pocket_fit], center = true);
        for (sx = [-1, 1], sy = [-1, 1])
            translate([sx * holes[0]/2, sy * holes[1]/2])
                circle(d = brk_island_d);
    }
    for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * holes[0]/2, sy * holes[1]/2, 0])
            insert_pocket(insert_m2_hole_d, insert_m2_depth, 0);
}

module brk_bosses(sz, holes) {
    for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * holes[0]/2, sy * holes[1]/2, -brk_seat_d])
            cylinder(d = boss_m2_od, h = brk_seat_d);
}

// ---------------------------------------------------------------------------
//  SUBTRACTIVE: the Feather, and the only hole in an outside wall
// ---------------------------------------------------------------------------
//  Turned 90 degrees, rear edge butted to the interior rear wall so the HDMI
//  reaches an edge. Its four mounting holes are NOT a rectangle -- 19.050 at
//  the USB end, 17.780 at the HDMI end, and two different drills -- so it
//  mounts the simple way: a pocket on the board's outline, two M2.5 at the USB
//  end on the standard spacing, and the HDMI end held by the pocket wall.
//
//  The HDMI exits through the CARRIER's rim only. Its shell spans z -0.636 ..
//  7.789, entirely below acrylic_z0 = 9.525, so the slab is never cut for it:
//  the port's mouth sits 1.905 back from the acrylic's rear edge, inside the
//  reveal, with the slab overhanging it.
feather_y   = interior_y1 - feather[1]/2;
feather_seat_d = 1.270;                 // [DERIVED] the HDMI's 0.636 + 0.634

module feather_cut() {
    translate([feather_x, feather_y, -feather_seat_d])
        linear_extrude(feather_seat_d) difference() {
            square([feather[0] + 2*pocket_fit, feather[1] + 2*pocket_fit],
                   center = true);
            for (sx = [-1, 1])
                translate([sx * feather_holes_usb/2, -feather_holes_span/2])
                    circle(d = boss_m25_od);
        }
    //  HDMI through the rear rim
    translate([feather_x, interior_y1 + (carrier_d/2 - interior_y1)/2 + 1,
               (-1.136 + 8.289)/2])
        cube([15.013 + 2.000, (carrier_d/2 - interior_y1) + 4, 8.289 + 1.136],
             center = true);
}

module feather_bosses() {
    for (sx = [-1, 1])
        translate([feather_x + sx * feather_holes_usb/2,
                   feather_y - feather_holes_span/2, -feather_seat_d])
            cylinder(d = boss_m25_od, h = feather_seat_d);
}

module feather_inserts() {
    for (sx = [-1, 1])
        translate([feather_x + sx * feather_holes_usb/2,
                   feather_y - feather_holes_span/2, 0])
            insert_pocket(insert_m25_hole_d, insert_m25_depth, 0);
}

// ---------------------------------------------------------------------------
//  SUBTRACTIVE: the pots
// ---------------------------------------------------------------------------
module pot_cut() {
    for (cx = cols) translate([cx, pot_y]) {
        //  the body pocket, blind, 2.230 of floor under it
        translate([0, 0, pot_z_bottom])
            linear_extrude(pot_web_z - pot_z_bottom)
                rrect(pot_pocket_sz[0], pot_pocket_sz[1], corner_r);
        //  the bushing, through the web
        translate([0, 0, pot_web_z - 1])
            cylinder(d = pot_bore_d, h = pot_web_t + 2);
    }
}

// ---------------------------------------------------------------------------
//  SUBTRACTIVE: the wiring, which is the interior's whole point
// ---------------------------------------------------------------------------
module chan(pts, w) {
    for (i = [0 : len(pts) - 2])
        hull() for (k = [i, i + 1])
            translate([pts[k][0], pts[k][1], -chan_qt_d])
                cylinder(d = w, h = chan_qt_d + 1);
}

module wiring_cut() {
    eL = [for (x = cols) x - pat_a/2];
    eR = [for (x = cols) x + pat_a/2];
    fader_in = band_mid - 29.210;
    //  I2C: ONE chain, left to right. The daisy chain and the signal flow are
    //  the same line, which is the whole reason the plate reads as a diagram.
    chan([[hdr_x, band_mid + 10.160], [exit_x, band_mid + 10.160],
          [exit_x, enc_y], [eL[0], enc_y]], chan_qt_w);
    chan([[eR[0], enc_y], [eL[1], enc_y]], chan_qt_w);
    chan([[eR[1], enc_y], [eL[2], enc_y]], chan_qt_w);
    //  the last leg goes UNDER the Feather and into the fader's lower short
    //  end; the NeoSlider has a STEMMA socket at both ends, so this costs
    //  cable and not a compromise
    chan([[eR[2], enc_y], [eR[2], fader_in], [slide_x, fader_in]], chan_qt_w);
    //  UART0 to the Feather, along the rear edge
    chan([[hdr_x, band_mid + 5.080], [exit_x, band_mid + 5.080],
          [exit_x, uart_y], [feather_x, uart_y]], chan_pot_w);
    //  the analogue trunk and three stubs. GPIO1/2/3 = ADC1_CH0/1/2, the ADC
    //  unit that keeps working while Wi-Fi is up. No ADS1115.
    chan([[hdr_x, band_mid - 10.160], [exit_x, band_mid - 10.160],
          [exit_x, trunk_y], [cols[2], trunk_y]], chan_pot_w);
    for (cx = cols) chan([[cx, trunk_y], [cx, pot_y - 7.620]], chan_pot_w);
}

// ---------------------------------------------------------------------------
//  SUBTRACTIVE: the inserts
// ---------------------------------------------------------------------------
module slab_insert_cut() {
    for (p = slab_screw_pts()) translate(concat(p, [0]))
        insert_pocket(insert_m3_hole_d, insert_m3_depth, carrier_rim_h);
}

//  The back panel gets M2.5 and not M3 for a geometric reason: an M3 pocket is
//  6.700 deep and the floor is 6.350, so pressing M3 from below would break
//  the visible wiring plane by 0.350 at every one of them. M2.5 is 5.000 and
//  leaves 1.350 of floor above every pocket.
module back_insert_cut() {
    for (p = back_screw_pts())
        translate([p[0], p[1], -carrier_t])
            rotate([180, 0, 0])
                insert_pocket(insert_m25_hole_d, insert_m25_depth, 0);
}

//  FOUR CAPTIVE M2.5 NUTS AROUND THE 18650 WINDOW, AND NOT FOUR INSERTS. The
//  reason is back_insert_cut()'s own arithmetic one line up, applied where it
//  was never applied: an insert_m25 pocket is 5.000 deep and its ceiling lands
//  at -1.350, which is fine under the 6.350 floor and is a THROUGH-HOLE under
//  disp_pocket_cut(), where the floor is 3.175 and its own top is -3.175. Three
//  of the four points this loop used to visit had no pocket in the mesh at all;
//  parameters.scad 12's cowl_screw_pts() has the four measurements and the
//  depth map that says no fourth position exists. nut_m25_pocket_d is 2.200, so
//  this ceiling is -4.150 -- 0.975 under the display pocket's floor and 2.150
//  under a wiring channel's -- and the floor's thickness stops being a
//  constraint.
//    WHAT THE JOINT IS NOW, because carrier.scad used to claim something else
//  here and the claim has to change with the mechanism. The pocket is BLIND in
//  this floor and OPEN at the back panel, so the panel is what retains the nut
//  and the screw pulls the nut ONTO the panel: the clamp closes cowl to panel
//  from both of the panel's faces, and this floor only stops the nut turning.
//  So it is still one fastener stack and still nothing bonded -- but the
//  preload no longer runs through the 3.175 of floor the display board sits on,
//  which is the one thing the old geometry did do and the reason it broke it.
//    PRESSED FROM THIS FACE, which is the face the ten insert bores already
//  open on, so it adds no overhang and no bridge to the print. Assembly gains
//  one step and fabrication.md has to carry it: invert the carrier with the
//  boards in, drop four nuts into these four pockets, then fit the panel.
//  Nothing geometric can check that a nut is present, and a loose M2.5 nut
//  inside a closed deck is a conductive object on the visible wiring plane.
//    THE HEXAGON IS DRAWN TO nut_m25_pocket_af ACROSS THE FLATS, so $fn = 6 on
//  a circle of af/cos(30) puts vertices at 0, 60, ... and a flat at 90. Every
//  clearance in parameters.scad 12 is taken on the across-CORNERS 3.0022 and
//  not on the 2.600 across-flats, so the phase this renders at cannot spend a
//  web: at the tightest neighbour the as-drawn phase measures 2.2009 of PETG
//  and the worst phase 1.7993, both over insert_m25_wall.
module cowl_nut_cut() {
    for (p = cowl_screw_pts())
        translate([p[0], p[1], -carrier_t - 0.01])
            linear_extrude(nut_m25_pocket_d + 0.01)
                circle(d = nut_m25_pocket_af / cos(30), $fn = 6);
}

// ---------------------------------------------------------------------------
//  THE PART
// ---------------------------------------------------------------------------
module carrier_3d() {
    difference() {
        union() {
            floor_solid();
            rim_solid();
            spine_solid();
            kbd_plinth_solid();
            kbd_bosses_solid();
            kbd_ribs_solid();
            pot_tower_solid();
            slab_boss_solid();
            for (cx = cols) translate([cx, enc_y]) brk_bosses(pat_a_sq(), [pat_a_holes, pat_a_holes]);
            translate([slide_x, band_mid]) rotate(90) brk_bosses(pat_b, pat_b_holes);
            feather_bosses();
        }
        kbd_annulus_cut();
        kbd_socket_cut();
        kbd_cell_cut();
        kbd_toggle_cut();
        kbd_pilot_cut();
        disp_pocket_cut();
        disp_hdr_cut();
        disp_18650_cut();
        disp_screw_cut();
        for (cx = cols) translate([cx, enc_y]) brk_seat(pat_a_sq(), [pat_a_holes, pat_a_holes]);
        translate([slide_x, band_mid]) rotate(90) brk_seat(pat_b, pat_b_holes);
        feather_cut();
        feather_inserts();
        pot_cut();
        wiring_cut();
        slab_insert_cut();
        back_insert_cut();
        cowl_nut_cut();
    }
}

if (part == "3d") carrier_3d();
else              carrier_3d();
