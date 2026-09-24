#!/usr/bin/env python3
"""
check_case.py - the carry case, measured against the deck it has to swallow.

The case is an accessory, not part of the enclosure, so it is not in
validate.py's build gate. It still gets checked, and it gets checked the same
way everything else here does: by measuring the rendered mesh rather than by
trusting the parameters that produced it.

Since C-42 the fasteners are a ring sampled off the case's own outline, so
NOTHING here reads their coordinates from parameters.scad. The bores are found
in a section of the actual part, counted, and measured for the metal around
them and the evenness of their spacing. If the ring drifts off the form, or a
bore creeps up on the cavity round a corner where no straight-line check would
look, that is what finds it.

    python3 tools/check_case.py
"""
from __future__ import annotations
import os, subprocess, sys, tempfile
import numpy as np, trimesh
from shapely.geometry import Polygon

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from params import load_with_defaults                      # noqa: E402

FAILED = []


def check(name, ok, detail):
    print(f"  [{'PASS' if ok else 'FAIL'}] {name}  --  {detail}")
    if not ok:
        FAILED.append(name)
    return ok


def render(tmp, scad, part, extra=()):
    out = os.path.join(tmp, f"{part}.stl")
    run = ["xvfb-run", "-a"] if subprocess.run(["which", "xvfb-run"],
           capture_output=True).returncode == 0 else []
    cmd = run + ["openscad", "-D", f'part="{part}"', *extra, "-o", out,
                 os.path.join(ROOT, "cad", scad)]
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=1800)
    if not os.path.exists(out):
        raise RuntimeError(f"openscad produced no {part}:\n{r.stderr[-900:]}")
    # A deleted module does not stop OpenSCAD: it warns on stderr and renders
    # the part without it. That is how a strap boss once measured, rendered and
    # photographed as a finished part while not existing at all. build.sh has
    # treated this as fatal for the enclosure for a long time; it is fatal here
    # too, so running this file directly is no weaker than running the gate.
    bad = [l for l in r.stderr.splitlines()
           if "WARNING: Ignoring" in l or "unknown variable" in l
           or "undefined operation" in l]
    if bad:
        raise RuntimeError(f"{part} rendered with pieces missing:\n  "
                           + "\n  ".join(sorted(set(bad))[:6]))
    return trimesh.load(out, force="mesh")


def plan(m, z):
    """The part's plan section at height z, as shapely polygons."""
    sec = m.section(plane_origin=[0, 0, z], plane_normal=[0, 0, 1])
    if sec is None:
        return []
    return list(sec.to_2D(to_2D=np.eye(4))[0].polygons_full)


def ceilings(m, bed_at_max, below_deg, ignore=None, ignore_r=0.0):
    """Area of faces that point at the bed, steeper than the FDM limit.

    Expressed in the print frame: `bed_at_max` says the part is laid on its
    highest Z face and grows downward, which is how the front half goes on.
    Faces resting on the bed itself are excluded - they are the first layer,
    not an overhang.
    """
    n, a, c = m.face_normals, m.area_faces, m.triangles_center
    if bed_at_max:
        bed, cos = m.bounds[1][2], n[:, 2]
    else:
        bed, cos = m.bounds[0][2], -n[:, 2]
    onbed = np.abs(c[:, 2] - bed) < 0.06
    ang = np.degrees(np.arccos(np.clip(cos, 0, 1)))
    keep = (cos > 1e-6) & ~onbed & (ang < below_deg)
    if ignore is not None and len(ignore):
        # A counterbore roof is a flat ceiling and it is fine: it is a narrow
        # annulus round a hole, which bridges. It is excluded here and measured
        # on its own terms by the ledge-width check instead, so this number
        # stays a number about UNSUPPORTED SPANS rather than about fasteners.
        d = np.linalg.norm(c[:, None, :2] - np.asarray(ignore)[None, :, :2], axis=2)
        keep &= d.min(axis=1) > ignore_r
    return float(a[keep].sum())


def ring_order(pts):
    """Walk the fastener ring nearest-neighbour, END to END.

    The ring is a U, not a loop - there is no metal across the mouth to put a
    fastener in - so the walk has to start at one of its ENDS. Starting at
    bottom dead centre walks out to one end and then jumps 134 mm across the
    open mouth to pick up the other side, and that jump reads as a 236 per cent
    spacing error in a part that is evenly spaced.
    """
    left = list(range(len(pts)))
    cur = max(left, key=lambda i: (pts[i][1], pts[i][0]))
    order = [cur]
    left.remove(cur)
    while left:
        nxt = min(left, key=lambda i: np.hypot(*(pts[i] - pts[cur])))
        order.append(nxt)
        left.remove(nxt)
        cur = nxt
    return order


def main():
    p = load_with_defaults(os.path.join(ROOT, "cad", "parameters.scad"))
    tmp = tempfile.mkdtemp(prefix="cyberdeck-case-")
    print("-- CARRY CASE --  rendering")
    front = render(tmp, "carrycase.scad", "front")
    back = render(tmp, "carrycase.scad", "back")
    # Bolted together, then measured as one object. A concatenation will not
    # do: the halves share a face at the joint, and a ray crossing two
    # coincident surfaces flips parity twice, so contains() quietly reports
    # solid material as open air. That cost this file two false failures.
    case = front.union(back)
    deck = trimesh.util.concatenate([
        render(tmp, "cyberdeck.scad", "chassis", ("-D", "variant=2")),
        render(tmp, "cyberdeck.scad", "backplate", ("-D", "variant=2"))])
    print()

    for lbl, m in (("front", front), ("back", back)):
        check(f"the {lbl} half is one solid body",
              m.is_watertight and m.body_count == 1,
              f"watertight {m.is_watertight}, {m.body_count} body(ies)")

    it = front.intersection(back)
    clash = float(it.volume) if it is not None and len(it.faces) else 0.0
    check("the halves do not interpenetrate",
          clash <= 1.0, f"{clash:.3f} mm^3 of overlap")

    # The joint is compared two ways, because the obvious single number is
    # wrong twice over. Net AREA differs by design - the back face is where the
    # nut pockets and pin holes open - and area is far too blunt a proxy for
    # outline anyway: narrowing this section by 0.6 mm moves its area by 0.4
    # per cent, which any sane tolerance would wave through. So the outlines
    # are compared as RINGS, in millimetres of deviation.
    zs = p["case_split_z"]
    rings, contact = [], []
    for m, z in ((front, zs + 0.05), (back, zs - 0.05)):
        polys = sorted(plan(m, z), key=lambda q: abs(q.area), reverse=True)
        rings.append(Polygon(polys[0].exterior) if polys else Polygon())
        contact.append(sum(abs(q.area) for q in polys))
    dev = rings[0].exterior.hausdorff_distance(rings[1].exterior)
    check("the halves meet over one mating face",
          dev < 0.10 and min(contact) > 4000,
          f"outlines deviate {dev:.3f} mm at worst; "
          f"{min(contact):.0f} mm^2 of metal in contact")

    # ---- THE FASTENER RING, found in the part and not read from anywhere ----
    polys = sorted(plan(front, zs + 1.0), key=lambda q: abs(q.area), reverse=True)
    wall = polys[0]
    holes = [Polygon(r) for r in wall.interiors]
    # Classified by POSITION, not by area: the D-ring bore is O5.20 against a
    # bolt's O5.40, which is 7 per cent apart - any area tolerance loose enough
    # to find the bolts would swallow the D-ring too.
    def near_lug(h):
        return (abs(abs(h.centroid.x) - p["case_lug_x"]) < 3.0
                and abs(h.centroid.y - p["case_lug_y"]) < 3.0)
    dr = [h for h in holes if near_lug(h)]
    bore_a = np.pi * (p["case_bolt_clear"] / 2) ** 2
    bores = [h for h in holes if not near_lug(h)
             and abs(h.area - bore_a) < 0.25 * bore_a]
    other = [h for h in holes if h not in dr and h not in bores]

    check("every fastener in the ring is present and nothing else is",
          len(bores) == int(p["case_bolt_n"]) and len(dr) == 2,
          f"{len(bores)} bores of an expected {int(p['case_bolt_n'])}, "
          f"{len(dr)} strap holes, {len(other)} other opening(s) "
          f"(locating pins)")

    # Distance from each bore to the wall's own boundary. That boundary is the
    # cavity on one side and open air on the other, so ONE number covers both -
    # including round the bottom corners, where the wall is not a straight run
    # and a check written in x and y would never look.
    edge = wall.exterior
    metal = [edge.distance(b) for b in bores]
    check("every bore in the ring keeps its metal, corners included",
          len(metal) > 0 and min(metal) >= p["case_bolt_keep"],
          f"least metal round a bore {min(metal):.2f} mm at "
          f"({bores[int(np.argmin(metal))].centroid.x:+.1f}, "
          f"{bores[int(np.argmin(metal))].centroid.y:+.1f}), "
          f"wanted {p['case_bolt_keep']:.1f}")

    cen = np.array([[b.centroid.x, b.centroid.y] for b in bores])
    order = ring_order(cen)
    steps = [float(np.hypot(*(cen[order[i + 1]] - cen[order[i]])))
             for i in range(len(order) - 1)]
    spread = (max(steps) - min(steps)) / max(np.mean(steps), 1e-9)
    # Measured as chords, which under-read across the bottom corners where the
    # ring is actually following an arc - so the low end of this range is
    # geometry, not error.
    check("the ring is evenly spaced the whole way round",
          spread < 0.15,
          f"{len(steps)} gaps, {min(steps):.1f}-{max(steps):.1f} mm "
          f"(mean {np.mean(steps):.1f}), spread {spread * 100:.1f}%")

    # The complaint that started C-42 was that the fasteners were only on the
    # sides. This is the check for that, and it is about where they ARE, not
    # about how many there are.
    onfloor = [c for c in cen if c[1] < p["case_y_bot"]]
    corners = [c for c in cen
               if c[1] < p["case_y_bot"] and abs(c[0]) > p["case_cav_hw"]]
    check("the ring turns the corners instead of stopping at the flanks",
          len(onfloor) >= 3 and len(set(np.sign([c[0] for c in corners]))) == 2,
          f"{len(onfloor)} fasteners below the deck, {len(corners)} of them "
          f"out past the cavity in both bottom corners")

    # ---- the rest ----
    worst, worst_at = 0.0, None
    for dy in (70, 55, 40, 25, 10, 0):
        d = deck.copy()
        d.apply_translation([0.0, float(dy), 0.0])
        v = 0.0
        for half in (front, back):
            i = half.intersection(d)
            v += float(i.volume) if i is not None and len(i.faces) else 0.0
        if v > worst:
            worst, worst_at = v, dy
    check("the deck slides its whole travel without touching",
          worst <= 1.0,
          f"worst interference {worst:.3f} mm^3"
          + (f" at {worst_at} mm above seated" if worst_at is not None else ""))

    zfi, bwd = p["z_front_inner"], p["board_w_display_front"]
    pcb = zfi - bwd
    sites = [("USB-C", 1, p["board_cy"] + p["usbc_off_y"], pcb + p["usbc_w_centre"]),
             ("microSD", 1, p["board_cy"] + p["tf_off_y"], pcb + p["tf_w_centre"]),
             ("keyboard window", -1,
              p["kbd_bay_cy"] + p["kbd_pocket_h"] / 2 - p["kbd_access_from_edge"]
              - p["kbd_access_w"] / 2,
              p["z_back_inner"] + p["kbd_access_above_floor"] + p["kbd_access_h"] / 2)]
    # "Buried" is a thickness, not a yes/no, and a point probe cannot tell the
    # difference. The strap slot passes through the OUTER half of the wall
    # directly outboard of the microSD port, so a probe 13 mm deep lands in
    # fresh air and calls a port with 7.5 mm of metal over it exposed. What
    # matters is how much metal a ray from the port has to cross to get out, so
    # that is what is measured.
    thick = []
    for lbl, sx, y, z in sites:
        o = np.array([[sx * (p["case_cav_hw"] - 0.5), y, z]])
        d = np.array([[float(sx), 0.0, 0.0]])
        hits, _, _ = case.ray.intersects_location(o, d, multiple_hits=True)
        xs = sorted(abs(h[0] - o[0][0]) for h in hits)
        solid = sum(xs[i + 1] - xs[i] for i in range(0, len(xs) - 1, 2))
        thick.append((lbl, solid, len(xs)))
    check("every side opening is buried in wall",
          all(t >= 6.0 and n % 2 == 0 for _, t, n in thick),
          ", ".join(f"{lbl} behind {t:.1f} mm" for lbl, t, _ in thick))

    # The front wall has to be unbroken over the whole face of the deck. This
    # started life as a check on the thumb scallop; the scallop is gone (see
    # parameters.scad) and the check is kept, because it is the one that says
    # the screen is actually behind something.
    gx, gy = np.meshgrid(np.linspace(-p["body_w"]/2 + 1, p["body_w"]/2 - 1, 17),
                         np.linspace(-p["body_h"]/2 + 1, p["body_h"]/2 - 1, 21))
    cover = np.column_stack([gx.ravel(), gy.ravel(),
                             np.full(gx.size, p["case_z1"] - 0.4)])
    cov = front.contains(cover)
    check("the front wall is unbroken over the whole deck",
          bool(np.all(cov)),
          f"{int(cov.sum())}/{cov.size} probes over the deck's face covered")

    check("the mouth is shallow enough to pinch the deck out of",
          p["case_rim"] <= 16.0 and p["case_floor"] >= p["case_side"],
          f"mouth {p['case_rim']:.1f} mm over the deck, floor "
          f"{p['case_floor']:.1f} mm under it - the proportion's slack went "
          f"to the end you drop it on")

    # THE C-43 GUARD, RESTATED FOR A HEAT-SET INSERT. A screw clamps nothing
    # unless what it threads into is anchored in the half its head is NOT on.
    # C-43 records a hex pocket at the parting face: the nut rose out of it and
    # bore on the FRONT half's own parting face, so head and nut both reacted
    # against one part and the joint carried zero load - with every check
    # passing. An insert is anchored by its knurls in the BACK half, so what is
    # measured here is that its bore really is in the back half, full depth,
    # with metal all round it.
    ring, deep = [], []
    r_ins = p["case_insert_d"] / 2 + 1.0
    for b_ in cen:
        for ang in range(0, 360, 30):
            ring.append([b_[0] + r_ins * np.cos(np.radians(ang)),
                         b_[1] + r_ins * np.sin(np.radians(ang)),
                         p["case_insert_z"] + p["case_insert_len"] / 2])
        for z in np.linspace(p["case_insert_z"] + 0.3, zs - 0.3, 6):
            deep.append([b_[0], b_[1], z])
    walled = back.contains(np.array(ring))
    open_b = back.contains(np.array(deep))
    check("every insert bore is in the back half, full depth, with metal round it",
          bool(np.all(walled)) and not bool(np.any(open_b)),
          f"{int(walled.sum())}/{len(ring)} probes solid at r={r_ins:.2f}, "
          f"{int((~open_b).sum())}/{len(deep)} clear down "
          f"{p['case_insert_len']:.1f} mm of bore; "
          f"{(p['case_side'] - p['case_insert_d']) / 2:.2f} mm of metal a side")

    axis = []
    for b_ in cen:
        for z in np.linspace(p["case_z1"] - 0.5, zs + 0.3, 9):
            axis.append([b_[0], b_[1], z])
    bore = front.contains(np.array(axis))
    check("every screw crosses the front half and grips its insert",
          not bool(np.any(bore)) and p["case_bolt_grip"] >= 1.2 * p["case_bolt_d"],
          f"{int((~bore).sum())}/{len(axis)} probes clear over "
          f"{p['case_z1'] - zs:.2f} mm of front half; M5 x "
          f"{p['case_bolt_len']:.0f} grips {p['case_bolt_grip']:.2f} mm "
          f"= {p['case_bolt_grip'] / p['case_bolt_d']:.2f} diameters")

    # The strap lug is now a plain through-hole, so what matters is the metal
    # either side of it and that it clears every fastener.
    lx, ly, ld = p["case_lug_x"], p["case_lug_y"], p["case_lug_d"]
    near = min(float(np.hypot(abs(b_[0]) - lx, b_[1] - ly)) for b_ in cen)
    thru = []
    for sx in (-1, 1):
        for z in np.linspace(p["case_z0"] + 0.5, p["case_z1"] - 0.5, 12):
            half = front if z > zs else back
            thru.append(not half.contains(np.array([[sx * lx, ly, z]]))[0])
    check("the strap lug is a clean hole through the flank",
          all(thru) and near > ld / 2 + p["case_bolt_clear"] / 2 + 4.0,
          f"{sum(thru)}/{len(thru)} probes open end to end, O{ld:.2f} with "
          f"{p['case_lug_mat']:.2f} mm of wall a side; nearest fastener "
          f"{near:.1f} mm away")

    mp, mo = [], []
    for (mx, my) in [(s * p["magnet_x"], y) for s in (-1, 1)
                     for y in (p["magnet_y_lo"], p["magnet_y_hi"])]:
        for z in np.linspace(p["case_z_fr"] + 0.2,
                             p["case_z_fr"] + p["magnet_pocket_h"] - 0.2, 4):
            mp.append([mx, my, z])
        mo.append([mx, my, p["case_z1"] - 0.4])
    mvoid, skin = front.contains(np.array(mp)), front.contains(np.array(mo))
    check("every magnet pocket is open to the tray and still skinned",
          not bool(np.any(mvoid)) and bool(np.all(skin)),
          f"{int((~mvoid).sum())}/{len(mp)} pocket probes open, "
          f"{int(skin.sum())}/4 skins intact at {p['case_mag_skin']:.2f} mm, "
          f"gap to the deck {p['case_mag_gap']:.2f} mm")

    for lbl, m, at_max in (("front", front, True), ("back", back, False)):
        flat = ceilings(m, at_max, 15.0)
        shallow = ceilings(m, at_max, 44.0)
        check(f"the {lbl} half prints face down with nothing under it",
              flat < 20.0 and shallow < 250.0,
              f"{flat:.0f} mm^2 near-flat ceiling, {shallow:.0f} mm^2 under 44 deg"
              f" - the rolled edge included, which is the thing most likely to "
              f"need support here")

    check("the split is not in the middle",
          abs((zs - p["case_z0"]) / (p["case_z1"] - p["case_z0"]) - 0.5) > 0.08,
          f"front {p['case_z1'] - zs:.2f} / back {zs - p['case_z0']:.2f} "
          f"= 1:{(zs - p['case_z0']) / (p['case_z1'] - zs):.2f}")

    fw, fh = case.extents[0], case.extents[1]
    tot = (front.volume + back.volume) / 1000.0
    print(f"\n  case {p['case_w']:.1f} x {p['case_h']:.1f} x {case.extents[2]:.1f} mm, "
          f"{fw:.1f} over the strap bosses")
    print(f"  front {front.volume/1000:.0f} cm^3 + back {back.volume/1000:.0f} cm^3"
          f"  =  {tot:.0f} cm^3 (~{tot * 1.24 * 0.55:.0f} g at 55% of solid)")
    print(f"  each half needs a {fw:.0f} x {fh:.0f} mm bed")
    print(f"  hardware: {len(cen)} x M5 x {p['case_bolt_len']:.0f} socket cap, "
          f"{len(cen)} x M5 x {p['case_insert_len']:.0f} heat-set insert, "
          f"4 x {p['magnet_d']:.0f}x{p['magnet_h']:.0f} disc")
    print()
    if FAILED:
        print(f"  {len(FAILED)} check(s) failed.")
        return 1
    print("  all checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
