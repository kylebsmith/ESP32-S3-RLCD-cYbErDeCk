//  carrycase.scad - a two-part sleeve the whole deck slides into.
//
//  Front half and back half, split on a plane parallel to the face, drawn
//  together by seven M5 socket screws into heat-set inserts. Every port is
//  buried. Meant to be flocked inside and filled, sanded and polished outside.
//
//  THE FORM IS A RIVER ROCK. Two things make it one, and neither is styling
//  applied afterwards:
//    - the section rolls continuously from face to face, with no flat band
//      anywhere and no arris
//    - the wall is 6 mm and SWELLS to 16.5 at each of the nine fastener and
//      strap sites, by pushing the plan OUTLINE outward rather than by adding a
//      pad, so there is no junction to crease at
//  The swells are also what pays for the thin wall: 13 mm everywhere was
//  serving nine holes, and the flanks measured 36 per cent of the filament.
//
//  HOW IT HOLDS THE DECK. The cowl channel, which keys it in X and rotation for
//  its whole travel; the flock; and gravity, because it is carried mouth-up.
//  The four magnets are a seat at the end of that travel, not a latch - the
//  numbers are in parameters.scad and they are not flattering.
//
//  PRINTING. Front half face-down, back half back-down. Both are then a flat
//  bed face and an open tray, no bridge, no support.
//
//  Coordinates are the deck's own, so alignment needs no arithmetic.

include <parameters.scad>
use <lib/util.scad>
use <cyberdeck.scad>

part = "assembly";   // front | back | assembly | fitcheck
$fn = 64;

//  ===========================================================================
//  THE FASTENER RING
//  ===========================================================================
//  An OUTSET OF THE CAVITY, not an inset of the outline. With a wall that goes
//  from 6 to 16.5 there is no single inset of the outline that lands every bore
//  6.50 mm outboard of the cavity, which is what the 3 mm metal margin needs.
//
//  The right half of the path: bottom dead centre, along the floor, round the
//  corner, up the flank, stopping short of the mouth. The corner comes out of
//  rse_poly itself, so the fasteners sit on a real curve and cannot drift.
function ring_path() =
    let (W  = case_ring_w,
         H  = case_ring_h,
         C  = case_ring_r,
         q  = 32,
         o  = rse_poly(W, H, C, form_n, q),
         ay = H / 2 - C,
         q4 = [ for (i = [3 * (q + 1) : 4 * (q + 1) - 1]) o[i] ])
    [ for (p = concat([[0, -H / 2]], q4,
                      [[W / 2, ay - case_bolt_top_back]]))
        [p[0], p[1] + case_ring_cy] ];

function _cum(p, i = 1, a = [0]) =
    i >= len(p) ? a : _cum(p, i + 1, concat(a, [a[i-1] + norm(p[i] - p[i-1])]));

function _at(p, a, s, i = 1) =
    i >= len(p)  ? p[len(p) - 1]
  : a[i] >= s    ? p[i-1] + (p[i] - p[i-1])
                   * ((s - a[i-1]) / max(a[i] - a[i-1], 1e-9))
                 : _at(p, a, s, i + 1);

function ring_at(f) =
    let (p = ring_path(), a = _cum(p)) _at(p, a, a[len(a) - 1] * f);

function _mirrored(r) =
    concat(r, [ for (q = r) if (q[0] > 0.01) [-q[0], q[1]] ]);

function bolt_sites() =
    _mirrored([ for (k = [0 : case_bolt_m]) ring_at(k / case_bolt_m) ]);

function pin_sites() =
    _mirrored([ for (k = case_pin_ks) ring_at(k / case_bolt_m) ]);

//  ===========================================================================
//  THE FORM
//  ===========================================================================
//  Built as ONE polyhedron. A stack of hulls cannot do it: the outline dips
//  back between swells, so it is not convex and hull() would fill the dips.
function boss_sites() =
    concat(bolt_sites(),
           [ for (sx = [-1, 1]) [sx * case_lug_x, case_lug_y] ]);

//  Swells combine as 1 - prod(1 - f), not as a sum, so two sites near each
//  other blend instead of stacking to twice the amplitude.
function _swell_at(p, sites, i = 0, acc = 1) =
    i >= len(sites) ? case_boss_amp * (1 - acc)
    : let (d = max(0, norm(p - sites[i]) - case_boss_lift),
           f = pow(cos(min(90, 90 * d / case_boss_reach)), 2))
      _swell_at(p, sites, i + 1, acc * (1 - f));

//  Two corrections live in this function, both found by measuring the rendered
//  section rather than by reading it.
//
//  ONE: the roll inset is applied to the BASE outline only. Subtracting it from
//  the swell as well applies it twice - the face outline came out at 65.61 where
//  the bolts sit at 65.825, so four of the seven counterbores cut out through
//  the edge. The swell is a constant push and rolls with the outline it pushes.
//
//  TWO: the push is along the outline's NORMAL, not radially from its centre.
//  Radially, a point high on the flank is mostly sideways from the centre, so
//  only part of a 10.50 mm swell arrives in x - it measured 68.22 where 71.83
//  was wanted. rse_poly runs counter-clockwise, so the outward normal of a
//  tangent (tx, ty) is (ty, -tx).
//  THREE: ONE outline, offset along its own normals. Every layer used to be a
//  separately built superellipse, narrowed by 2*ins with its corner radius
//  narrowed by ins - and shrinking the corner MOVES EVERY VERTEX ALONG THE
//  FLANK. Vertex j sat at a different y on each layer, so it sampled the swell
//  at a different place, and the swell shrank by about what the inset gave
//  back: a 4.00 mm facet cut at 55 degrees delivered 0.47 mm of run instead of
//  2.80 and came out at 83 degrees, which is an arris with extra steps.
//
//  Offsetting one outline keeps vertex j on one ray for the whole depth, so the
//  swell is genuinely constant and the inset is genuinely the inset.
function pebble_poly(ins, sites, q) =
    let (ref = rse_poly(case_w, case_h, case_r, form_n, q),
         n = len(ref))
    [ for (j = [0 : n - 1])
        let (p  = [ref[j][0], ref[j][1] + case_cy],
             a  = ref[(j + n - 1) % n],
             b  = ref[(j + 1) % n],
             tg = [b[0] - a[0], b[1] - a[1]],
             tl = max(norm(tg), 1e-9),
             nx = tg[1] / tl,
             ny = -tg[0] / tl,
             s  = _swell_at(p, sites) - ins)
        [p[0] + nx * s, p[1] + ny * s] ];

//  The section profile, as INSET against distance from the nearer face.
//
//  Three regions, and the first of them is the one the render caught. The
//  version before this ran the smoothstep the whole way, and a smoothstep's
//  derivative is ZERO at its ends - so the flank left the face VERTICALLY and
//  met the flat cap at a 90-degree arris, 1782 edges of it all the way round.
//  It measured 90.0 degrees at worst. That is a curved rectangle, whatever the
//  middle of the section does.
//
//    d <= case_face_ch          a straight facet at case_face_ang off horizontal
//    up to the half-depth       the roll, taking what the facet left to zero
//    beyond                     full girth
function edge_profile(u) =
    let (t   = case_z1 - case_z0,
         d   = min(u, 1 - u) * t,
         mid = t * case_roll)
      d >= mid          ? 0
    : d <= case_face_ch ? case_soft - d / tan(case_face_ang)
    : (case_soft - case_face_run)
      * (1 - sstep((d - case_face_ch) / (mid - case_face_ch)));

//  The layer heights are NOT evenly spaced: a facet 4.00 mm tall sampled on an
//  0.85 mm grid comes out as five steps, which is a stair, not a chamfer. Both
//  ends of the facet get a layer of their own, so it renders as one ruled
//  surface, and the rest of the depth is divided evenly.
function body_levels(nm) =
    let (a = case_z0 + case_face_ch, b = case_z1 - case_face_ch)
    concat([case_z0, a],
           [ for (i = [1 : nm - 1]) a + i * (b - a) / nm ],
           [b, case_z1]);

module full_body(nm = 40, q = 32) {
    na = 4 * (q + 1);
    sites = boss_sites();
    zs = body_levels(nm);
    nz = len(zs) - 1;
    layers = [ for (z = zs)
                 pebble_poly(edge_profile((z - case_z0) / (case_z1 - case_z0)),
                             sites, q) ];
    pts = [ for (i = [0 : nz], j = [0 : na - 1])
              [layers[i][j][0], layers[i][j][1], zs[i]] ];
    // wound as rse_soft winds: clockwise seen from OUTSIDE
    sides = [ for (i = [0 : nz - 1], j = [0 : na - 1])
                [ i*na + j, (i+1)*na + j,
                  (i+1)*na + (j+1)%na, i*na + (j+1)%na ] ];
    bottom = [ for (j = [0 : na - 1]) j ];
    top    = [ for (j = [na - 1 : -1 : 0]) nz*na + j ];
    polyhedron(points = pts, faces = concat(sides, [bottom], [top]),
               convexity = 12);
}

//  One half, cut from that. case_seam_ch is a hairline, not a shadow gap: the
//  object reads as one piece, so the joint gets only enough relief to stop a
//  few tenths of print mismatch showing as a step.
module half_body(flip) {
    ch   = case_seam_ch;
    d    = flip ? -1 : 1;
    t    = case_z1 - case_z0;
    big  = 40;
    grow = case_boss_amp;
    intersection() {
        full_body();
        hull() {
            // rse_plate extrudes UPWARD from its origin, so the mirrored half's
            // pinch plate would sit 0.01 mm past the parting plane and the
            // halves would interfere across the whole joint.
            translate([0, case_cy, case_split_z - (d < 0 ? 0.01 : 0)])
                rse_plate(case_w + 2*grow - 2*ch, case_h + 2*grow - 2*ch, 0.01,
                          case_r + grow - ch, form_n);
            translate([0, case_cy, case_split_z + d * ch])
                rse_plate(case_w + big, case_h + big, 0.01, case_r + big/2, form_n);
            translate([0, case_cy, case_split_z + d * (t + 1)])
                rse_plate(case_w + big, case_h + big, 0.01, case_r + big/2, form_n);
        }
    }
}

//  ===========================================================================
//  SUBTRACTIONS
//  ===========================================================================
//  Everything the deck occupies, plus the flock allowance, running out the top.
module case_cavity() {
    over = 40;
    h    = (case_y_top + over) - case_y_bot;
    cy   = (case_y_top + over + case_y_bot) / 2;
    union() {
        translate([0, cy, case_z_bk])
            rse_plate(case_cav_w, h, case_z_fr - case_z_bk, case_cav_r, form_n);
        // the cowl channel, full height, which is what makes it a guide
        translate([0, cy, case_z_cowl])
            rse_plate(case_slot_w, h, case_z_bk - case_z_cowl + 0.01,
                      max(case_cav_r - 4, 2), form_n);
    }
}

//  THE STRAP LUG IS A HOLE. Front to back through the flank, so a cord or split
//  ring wraps the full wall at its swell and hangs outward, and the load goes
//  into the whole height of the flank above it rather than into any feature.
//  The parting plane cuts across it, so each half prints it as a plain vertical
//  bore with nothing overhanging.
module lug_holes() {
    for (sx = [-1, 1])
        translate([sx * case_lug_x, case_lug_y, case_z0 - 1])
            cylinder(d = case_lug_d, h = case_z1 - case_z0 + 2);
}

//  THE HEADS SIT FLUSH, IN A DISH. A 4.80 mm head in a 5.00 mm counterbore
//  lands 0.20 below the surface; the shallow sphere blends the counterbore's
//  rim into the curve so it reads as an inset rather than a drilled hole.
module head_dishes() {
    for (b = bolt_sites()) {
        translate([b[0], b[1], case_bolt_seat])
            cylinder(d = case_cb_d, h = case_cb_deep + 2);
        translate([b[0], b[1], case_z1 + case_dish_r - case_dish_d])
            sphere(r = case_dish_r, $fn = 96);
    }
}

//  Screw clearance: the counterbore down to the insert. The screw crosses ONE
//  half, which is what lets it be M5 x 16 on a 37 mm object.
module bolt_holes() {
    for (b = bolt_sites())
        translate([b[0], b[1], case_insert_z])
            cylinder(d = case_bolt_clear, h = case_z1 + 1 - case_insert_z);
}

//  The insert bore, in the BACK half, driven from its parting face. The brass
//  anchors in the plastic by its knurls, so tension runs head -> front half ->
//  insert -> back half with nowhere to short-circuit. See C-43.
module insert_bores() {
    for (b = bolt_sites())
        translate([b[0], b[1], case_insert_z])
            cylinder(d = case_insert_bore, h = case_insert_len);
}

module pins(d, h, z) {
    for (b = pin_sites()) translate([b[0], b[1], z]) cylinder(d = d, h = h);
}

//  Magnet pockets, opening at the cavity face of the front half - the face
//  pointing at the ceiling while it prints. Glued in flush, flocked over.
module case_magnets() {
    for (m = magnet_sites())
        translate([m[0], m[1], case_z_fr - 0.01])
            cylinder(d = magnet_bore, h = magnet_pocket_h + 0.01);
}

//  ===========================================================================
//  PARTS
//  ===========================================================================
module case_front() {
    difference() {
        union() {
            half_body(false);
            pins(case_pin_d, case_pin_h + 0.01, case_split_z - case_pin_h);
        }
        case_cavity();
        lug_holes();
        bolt_holes();
        head_dishes();
        case_magnets();
    }
}

module case_back() {
    difference() {
        half_body(true);
        case_cavity();
        lug_holes();
        bolt_holes();
        insert_bores();
        pins(case_pin_d + case_pin_fit, case_pin_h + 0.2,
             case_split_z - case_pin_h - 0.2 + 0.01);
    }
}

if      (part == "front")    case_front();
else if (part == "back")     case_back();
else if (part == "assembly") { case_front(); case_back(); }
else if (part == "fitcheck") {
    color("DimGray")   case_back();
    color("Gainsboro") case_front();
    color("SteelBlue", 0.5) { chassis(); backplate(); }
}
else assert(false, str("unknown part: ", part));
