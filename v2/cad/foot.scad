// ===========================================================================
//  cYbErDeCk v2 - foot.scad
//  The four feet the whole instrument stands on.
//
//      openscad -D 'part="3d"' -o export/stl/foot.stl v2/cad/foot.scad
//
//  IT EXISTS BECAUSE OF ONE DATUM AND ONE COMPONENT. parameters.scad 3:
//
//      foot_h_min = cowl_h() + 1.270 = 5.090
//
//  The display's 18650 holder reaches 1.820 BELOW the deck's own underside and
//  cannot be contained, so cowl.scad covers it and the cowl hangs 3.820 below
//  the back panel. A foot shorter than 5.090 and the deck does not stand on
//  its feet at all -- it stands on its own battery cowl, off centre, on three
//  points and a rock.
//
//  WHERE THEY GO, AND IT IS NOT FOUR EQUAL CORNERS
//  -----------------------------------------------
//  The back panel is 3.175 mm laser-cut acrylic. A laser cannot counterbore,
//  so the panel has no seat for anything, and a foot bonded to it is a foot
//  that peels off the one part that is also the service access. Every foot
//  therefore lands on a fastener that already exists -- a screw passing up
//  through the panel into an M2.5 heat-set insert in the carrier's floor --
//  and the ten holes in parameters.scad's back_screw_pts() are the whole menu.
//  Four of them, picked for the tipping problem below:
//
//      [ 144.780,  80.7125]   slab_screw_pts()[0]   the rear pair, on the
//      [-144.780,  80.7125]   slab_screw_pts()[3]     slab's own screw line
//      [ -82.550, -63.500]    back_screw_kbd[2]     the front pair, the
//      [  82.550, -63.500]    back_screw_kbd[3]       furthest forward there is
//
//  IT PRESSES ON OVER THE SCREW HEAD AND IT IS NOT IN THE CLAMP PATH, and the
//  first draft of this part got that wrong. Drawn the obvious way -- a
//  counterbore in the foot, the head inside it, the foot clamped between the
//  head and the panel -- the whole preload of four of the panel's ten screws
//  runs through TPU:
//
//      O4.500 head on a O5.500/O2.900 counterbore floor   17.15 mm2
//      a hand-tight M2.5, about 354 N                     20.6 MPa
//
//  TPU 95A creeps at 20 MPa. So those four screws would lose their preload
//  over weeks, the back panel's clamp -- whose whole job is fighting the
//  carrier's bow -- would relax at four of ten points, and nothing would ever
//  show it. A TPU part does not belong in a clamp path.
//
//  So the head bears on the PANEL, exactly as it does at the other six holes,
//  and the foot wraps it: a BLIND pocket in the foot's top face, O4.300 on a
//  O4.500 x 2.500 head, which is 0.200 of diametral interference and is the
//  retention, with 0.675 of air above the head so it can NEVER bottom in the
//  pocket -- if it bottomed, the clamp would be back in the TPU. The standing
//  load goes desk -> sole -> top face -> panel -> carrier, all of it in
//  compression and none of it through the screw: 2.921 N per foot over
//  384.661 mm2 of sole [MEASURED off export/stl/foot.stl] is 0.0076 MPa,
//  1/2700 of the stress that killed the first draft.
//    Two consequences worth having. The foot's height no longer depends on
//  how hard anybody tightened anything -- it is the printed height, full
//  stop. And the screws stay M2.5 x 8 at all ten holes, because the foot adds
//  nothing to the stack.
//    Nine of the ten holes are SLOTS (parameters.scad 6: one round datum, N
//  slots, because PMMA and a printed carrier disagree about length by
//  +/-1.5 mm over the screw span) and the foot does not care -- the slot's
//  float is taken up between the panel and the screw, and the foot is located
//  by the head.
//
//  TIPPING IS A LAYOUT PROBLEM AND IT WAS MEASURED, NOT ASSUMED
//  -----------------------------------------------------------
//  This is a desk instrument with a keyboard at the near edge: the heels of
//  the hands land on y = -95.1875, forward of ANY available fastener. Take the
//  enclosure's own mass and centroid off the exported meshes -- slab 416.55 g,
//  back panel 210.53 g, carrier 564.21 g at 1.19 / 1.19 / 1.27 g/cm3, total
//  1191.29 g with its centroid at y +7.6517 [MEASURED 2026-11-07 from
//  export/stl/*.stl] -- and moments about the front contact line give the
//  downward force at the front edge that lifts the rear feet:
//
//      front contact            tips at
//      y = -63.500  (a round pad on the screw)        26.23 N  =  2.675 kgf
//      y = -90.805  (this part's toe)                262.46 N  = 26.763 kgf
//
//  2.675 kgf is a palm resting on the front edge. So a foot that is just a
//  puck on the bolt makes the deck teeter every time somebody leans on it,
//  and the fix is not a bigger foot but a LONGER one: the pad is a stadium
//  34.290 long with the screw 6.985 from its heel, so its toe reaches
//  y = -90.805, which is 4.3825 inside the panel's front edge and 2.4775
//  behind the carrier's own front face -- so the toe bears on panel that is
//  backed by the carrier's 2.495 front rim, not on the 1.905 of unsupported
//  lip outboard of it.
//    The figure is a LOWER BOUND and deliberately: the keyboard, the display,
//  the Feather, the six controls and every screw are left out of it, and all
//  of them sit behind y = -63.500, so each one only raises the threshold.
//    One part, four off, all four fitted in the SAME orientation, toe toward
//  -Y. At the rear positions the toe then points inboard, which is free: the
//  rear feet are 14.475 from the rear edge and nothing tips backwards. The
//  part is drawn in the orientation it is fitted in, with the screw axis on
//  the origin, so placing it is a translate and never a rotate -- a rotate is
//  a thing that gets forgotten.
//
//  ROCKING IS NOT A LAYOUT PROBLEM AND NO ARRANGEMENT FIXES IT
//  ----------------------------------------------------------
//  Three points define a plane. A rigid body on FOUR contacts bears on all
//  four only if the four soles are coplanar to within the structure's own
//  flexure, and that is kinematics -- moving the feet does not change it.
//  Stating it plainly, with the numbers, because the alternative is a comment
//  that claims the feet "self-level" and does not:
//
//    * TPU 95A's tensile modulus is 56 +/- 3 MPa along the build Z, measured
//      on 3D-PRINTED samples to ASTM D3039 [VENDOR UltiMaker TPU 95A TDS
//      V2.01 09/2025]. Z is the right column: the part prints top-face-down,
//      so the load axis IS the build axis. (The 26 MPa in circulation is the
//      pre-2022 sheet and is retired.)
//    * As a solid pad that is E*A/h = 56 * 384.661 / 5.715 = 3769 N/mm. The
//      deck's own weight is 2.921 N per foot, so self-weight settles the pad
//      0.0008 mm. IT DOES NOT SELF-LEVEL, and because the foot is press-on
//      there is no preload to level it with either -- which is the honest
//      version of a story the first draft told the other way round.
//
//  So the residual rock is exactly the worst non-coplanarity of the four
//  soles as BUILT: four printed pad heights, plus the back panel's flatness
//  over the 165.100 x 144.213 mm rectangle the feet stand on. Neither is
//  sourced in this repository and neither is guessed at here. The procedure
//  that closes it: print the four feet in one job, micrometer the four
//  heights, and shim the short ones with PET film under the top face -- the
//  top face is flat and square to the sole for exactly that reason. If the
//  measured spread turns out worse than a shim should carry, the sole is flat
//  and unobstructed and a bought self-adhesive elastomer bumper with a
//  published load-deflection curve drops onto it without redrawing anything.
//
//  WHAT IT IS MADE OF, AND WHY IT IS THE ONLY PART THAT IS NOT PETG OR PMMA
//  -----------------------------------------------------------------------
//  TPU 95A, 96 Shore A / 48 Shore D [VENDOR, same TDS, ISO 7619-1]. The cost
//  is real and is stated: a second filament and a second print job for 4
//  parts weighing a few grams. What buys it is friction. The deck is TYPED
//  ON -- lateral impulses, 61 keys, both hands -- and a rigid PETG pad on a
//  desk slides, so the instrument would walk away from the user while it was
//  being played. The coefficient of friction of printed TPU on a desk surface
//  is NOT SOURCED; closing it is a spring gauge and a drag test on the real
//  pads, which is also the only way to learn it for a specific desk.
//
//  PRINT IT SOLE DOWN, which is the orientation the pocket decides. The
//  pocket is blind in the top face, so sole-down it opens UPWARD and there is
//  no bridge anywhere; the only overhang in the part is the sole's 0.635
//  chamfer, at 45 degrees. Top-face-down it would ask for a O4.300 bridge
//  across the pocket in a material that bridges badly. Sole-down also puts
//  the sole on the bed, which is the flattest surface a printer makes, and
//  the sole is the one face whose flatness this part is for.
//
//  AND THE COST IN HEIGHT, recorded because somebody will want it: the deck's
//  front edge stands deck_t + 5.715 = 37.465 mm above the desk.
// ===========================================================================

include <parameters.scad>
use <lib/util.scad>

part = "3d";

//  EVERY LOCAL IS PREFIXED ft_. parameters.scad 3 declares foot_h_min and
//  nothing else about the foot; writing `foot_h` here as a bare name would be
//  one keystroke from redefining a datum for the whole scope, which is the
//  mistake that silently redefined the deck's depth in knob.scad.
ft_h       = 5.715;         // [DERIVED] 9 x grid, and it is FORCED rather
                            //   than chosen: foot_h_min is 5.090 and 8 x grid
                            //   is 5.080, which misses it by 0.010. 9 x grid
                            //   is the smallest grid multiple that clears the
                            //   cowl, and the 0.625 it clears by is on top of
                            //   the 1.270 of air foot_h_min already carries.
ft_w       = 13.970;        // [DERIVED] 22 x grid, and it is bracketed from
                            //   above: the rear feet sit on screw_x = 144.780
                            //   and the panel's edge is at body_w/2 = 152.300,
                            //   so a half-width over 7.520 hangs off the deck.
                            //   6.985 leaves 0.535.
ft_toe     = 27.305;        // [DERIVED] 43 x grid, screw to toe. Solved from
                            //   the tipping figures in the header: it is what
                            //   puts the front contact at y = -90.805.
ft_heel    = 6.985;         // [DERIVED] 11 x grid = ft_w/2, so the heel is a
                            //   semicircle centred on the screw. At the rear
                            //   positions that leaves 7.490 to the panel's
                            //   rear edge; any more and the rear feet hang
                            //   over it.
ft_l       = ft_toe + ft_heel;      // [DERIVED] 34.290 = 54 x grid
ft_cham    = 0.635;         // [DESIGN] 1 x grid, 45 deg at the sole. A square
                            //   TPU edge catches and peels when the deck is
                            //   dragged across a desk, which is the one thing
                            //   that is certain to happen to it.

//  THE HEAD. These two belong in parameters.scad 6 beside the clearance
//  diameters and they are NOT there -- back_t's own comment already leans on
//  the 2.500 in prose ("a cap head is 2.500 tall") without declaring it. They
//  are ft_-prefixed locals here rather than bare names for the usual reason,
//  and the value is reported upward rather than added to a file this one does
//  not own.
ft_head_d  = 4.500;         // [STANDARD] ISO 4762 M2.5, dk
ft_head_h  = 2.500;         // [STANDARD] ISO 4762 M2.5, k. The same 2.500
                            //   parameters.scad 3 already quotes.
ft_pocket_d = ft_head_d - 0.200;    // [DESIGN] 4.300. 0.200 of diametral
                            //   interference = 4.4 % strain on the pocket's
                            //   wall, which TPU takes and returns. This is
                            //   the whole retention: push it on, pull it off.
ft_pocket_h = ft_head_h + 0.675;    // [DERIVED] 3.175 = 5 x grid, and the
                            //   0.675 is the number that matters: the head
                            //   must never reach the pocket's floor, because
                            //   the moment it does, the clamp load is back in
                            //   the TPU and the creep argument above applies.
                            //   It is also exactly the relief the back panel
                            //   leaves over the display's own screw heads.
ft_lead    = 0.400;         // [DESIGN] a 45 deg lead-in at the pocket's
                            //   mouth. Pressing a 0.200-interference TPU
                            //   pocket onto a hex-socket head by hand with no
                            //   lead-in is how the mouth tears.

//  THE SOLID FLOOR UNDER THE POCKET: ft_h - ft_pocket_h = 2.540, and it is
//  what makes the sole an unbroken surface. There is no screw hole through
//  this part at all, which is why the sole is 384.741 mm2 rather than 361 and
//  why a bought bumper can be stuck to it later.
//  [MEASURED] 384.6606 mm2 over 98 faces, off the exported mesh.

//  The plan: a stadium, inset by i. A stadium and not a rounded rectangle
//  because a one-screw pad has no corner worth having -- and rrect() clamps
//  r to w/2 - 0.001, so asking for exactly w/2 gives the stadium and not a
//  degenerate hull.
module ft_plan(i) {
    translate([0, (ft_heel - ft_toe)/2])
        rrect(ft_w - 2*i, ft_l - 2*i, ft_w/2 - i);
}

module foot_3d() {
    difference() {
        //  Sole on z = 0, top face on z = ft_h, screw axis on the origin.
        //  The plan is CONVEX, so hull() of the chamfer slice and the body is
        //  exact rather than an approximation of a chamfer.
        hull() {
            linear_extrude(eps) ft_plan(ft_cham);
            translate([0, 0, ft_cham])
                linear_extrude(ft_h - ft_cham) ft_plan(0);
        }
        //  The head's pocket, blind, down from the TOP face. Both cuts
        //  overshoot the top face by 1.000 rather than stopping on it: a cut
        //  that ends exactly on a face leaves coplanar faces, which is the
        //  other half of the family of CGAL failures this repo has paid for.
        //  The thinnest wall is (ft_w - ft_pocket_d)/2 - ft_lead = 4.435.
        translate([0, 0, ft_h - ft_pocket_h])
            cylinder(d = ft_pocket_d, h = ft_pocket_h + 1);
        translate([0, 0, ft_h - ft_lead])
            cylinder(d1 = ft_pocket_d, d2 = ft_pocket_d + 2*ft_lead,
                     h = ft_lead);
        translate([0, 0, ft_h - 0.001])
            cylinder(d = ft_pocket_d + 2*ft_lead, h = 1);
    }
}

foot_3d();
