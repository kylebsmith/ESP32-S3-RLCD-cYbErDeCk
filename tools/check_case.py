#!/usr/bin/env python3
"""
check_case.py - the carry case, measured against the deck it has to swallow.

The case is an accessory, not part of the enclosure, so it is not in
validate.py's build gate. It still gets checked, and it gets checked the same
way everything else here does: by measuring the rendered mesh rather than by
trusting the parameters that produced it.

Since C-41 it is two bolted halves, so most of these now measure the pair:
that they meet, that they do not interpenetrate, that each one prints face
down with nothing under it, and that the fasteners and magnets that motivated
the split can actually be reached.

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
    return trimesh.load(out, force="mesh")


def ceilings(m, bed_at_max, below_deg):
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
    return float(a[(cos > 1e-6) & ~onbed & (ang < below_deg)].sum())


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

    # Two halves that overlap cannot be bolted together; two that miss leave a
    # gap the strap load has to jump. Both are measured, not assumed.
    it = front.intersection(back)
    clash = float(it.volume) if it is not None and len(it.faces) else 0.0
    check("the halves do not interpenetrate",
          clash <= 1.0, f"{clash:.3f} mm^3 of overlap")

    # The joint is compared two ways, because the obvious single number is
    # wrong twice over. Net AREA differs by design - the back face is where
    # eight nut pockets and four pin holes open - and area is far too blunt a
    # proxy for outline anyway: narrowing this section by 0.6 mm moves its area
    # by 0.4 per cent, which any sane tolerance would wave through. So the
    # outlines are compared as RINGS, in millimetres of deviation, and the
    # contact area is checked separately for being large.
    zs = p["case_split_z"]
    rings, contact = [], []
    for m, z in ((front, zs + 0.05), (back, zs - 0.05)):
        sec = m.section(plane_origin=[0, 0, z], plane_normal=[0, 0, 1])
        polys = sec.to_2D(to_2D=np.eye(4))[0].polygons_full if sec else []
        polys = sorted(polys, key=lambda q: abs(q.area), reverse=True)
        rings.append(Polygon(polys[0].exterior) if polys else Polygon())
        contact.append(sum(abs(q.area) for q in polys))
    dev = rings[0].exterior.hausdorff_distance(rings[1].exterior)
    check("the halves meet over one mating face",
          dev < 0.10 and min(contact) > 4000,
          f"outlines deviate {dev:.3f} mm at worst; "
          f"{min(contact):.0f} mm^2 of metal in contact")

    # The whole point of the cowl channel: the deck must slide its full travel
    # without touching anything. A fit that only works when seated is not a
    # sleeve, it is a puzzle.
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

    # Bomb-proof means the openings are behind material, not merely covered by
    # something the size of a lid.
    zfi, bwd = p["z_front_inner"], p["board_w_display_front"]
    pcb = zfi - bwd
    sites = [("USB-C", 1, p["board_cy"] + p["usbc_off_y"], pcb + p["usbc_w_centre"]),
             ("microSD", 1, p["board_cy"] + p["tf_off_y"], pcb + p["tf_w_centre"]),
             ("keyboard window", -1,
              p["kbd_bay_cy"] + p["kbd_pocket_h"] / 2 - p["kbd_access_from_edge"]
              - p["kbd_access_w"] / 2,
              p["z_back_inner"] + p["kbd_access_above_floor"] + p["kbd_access_h"] / 2)]
    pts = np.array([[sx * (p["body_w"] / 2 + p["case_pad"] + p["case_side"] / 2), y, z]
                    for _, sx, y, z in sites])
    inside = case.contains(pts)
    check("every side opening is buried in wall",
          bool(np.all(inside)),
          ", ".join(f"{lbl} {'buried' if i else 'EXPOSED'}"
                    for (lbl, *_), i in zip(sites, inside)))

    # A lug that is not a hole is decoration; a lug with a thin web is the
    # version we already threw away. Probe the slot open and its walls solid.
    lx, ly = p["case_lug_x"], p["case_lug_y"]
    mat = p["case_lug_mat"]
    open_pts, wall_pts = [], []
    for s in (-1, 1):
        for z in (p["case_z0"] + 3, zs + 4, p["case_z1"] - 3):
            open_pts.append([s * lx, ly, z])
            wall_pts.append([s * (lx + p["case_lug_slot_w"] / 2 + mat / 2), ly, z])
            wall_pts.append([s * (lx - p["case_lug_slot_w"] / 2 - mat / 2), ly, z])
    is_open = ~case.contains(np.array(open_pts))
    is_wall = case.contains(np.array(wall_pts))
    depth = p["case_z1"] - p["case_z0"]
    check("both strap slots are through-holes in solid bosses",
          bool(np.all(is_open)) and bool(np.all(is_wall)),
          f"{int(is_open.sum())}/6 slot probes open, {int(is_wall.sum())}/12 wall "
          f"probes solid, {mat:.2f} mm each side x {depth:.1f} mm deep "
          f"= {mat * depth:.0f} mm^2 in shear")

    # The nut has to be enclosed. If its pocket breaks into the cavity it will
    # push flock into the deck's path and the screw will have nothing to pull on.
    bx, cd = p["case_bolt_x"], p["case_nut_cd"]
    ring = []
    for s in (-1, 1):
        for y in p["case_bolt_ys"]:
            for a in range(0, 360, 30):
                r = cd / 2 + 0.6
                ring.append([s * bx + r * np.cos(np.radians(a)),
                             y + r * np.sin(np.radians(a)),
                             zs - p["case_nut_h"] / 2])
    enclosed = back.contains(np.array(ring))
    check("every nut pocket is enclosed in material",
          bool(np.all(enclosed)),
          f"{int(enclosed.sum())}/{len(ring)} probes solid around 8 pockets, "
          f"{bx - cd / 2 - p['case_cav_hw']:.2f} mm to the cavity, "
          f"{p['case_rail_x'] - bx - cd / 2:.2f} mm to the outside")

    # The screw must actually reach from the front face to the nut.
    axis = []
    for s in (-1, 1):
        for y in p["case_bolt_ys"]:
            for z in np.linspace(p["case_z1"] - 0.5, zs - p["case_nut_h"] + 0.3, 7):
                axis.append([s * bx, y, z])
    bore = case.contains(np.array(axis))
    check("every screw bore runs front face to nut",
          not bool(np.any(bore)),
          f"{int((~bore).sum())}/{len(axis)} probes clear over "
          f"{p['case_bolt_stack']:.2f} mm of stack, M5 x {p['case_bolt_len']:.0f}")

    # The magnets were the reason for splitting the case. Prove they are open
    # to the bed-facing side of the front half and still have skin over them.
    mp, mo = [], []
    for (mx, my) in [(s * p["magnet_x"], y) for s in (-1, 1)
                     for y in (p["magnet_y_lo"], p["magnet_y_hi"])]:
        for z in np.linspace(p["case_z_fr"] + 0.2,
                             p["case_z_fr"] + p["magnet_pocket_h"] - 0.2, 4):
            mp.append([mx, my, z])
        mo.append([mx, my, p["case_z1"] - 0.4])
    void, skin = front.contains(np.array(mp)), front.contains(np.array(mo))
    check("every magnet pocket is open to the tray and still skinned",
          not bool(np.any(void)) and bool(np.all(skin)),
          f"{int((~void).sum())}/{len(mp)} pocket probes open, "
          f"{int(skin.sum())}/4 skins intact at {p['case_mag_skin']:.2f} mm, "
          f"gap to the deck {p['case_mag_gap']:.2f} mm")

    # Each half lies on one flat face and grows away from it. A near-horizontal
    # face pointing at the bed is the defect that killed the spined back.
    for lbl, m, at_max in (("front", front, True), ("back", back, False)):
        flat = ceilings(m, at_max, 15.0)
        shallow = ceilings(m, at_max, 44.0)
        check(f"the {lbl} half prints face down with nothing under it",
              flat < 20.0 and shallow < 200.0,
              f"{flat:.0f} mm^2 near-flat ceiling, {shallow:.0f} mm^2 under 44 deg")

    fw, fh = case.extents[0], case.extents[1]
    tot = (front.volume + back.volume) / 1000.0
    ratio = p["case_h"] / p["case_w"]
    deck = p["body_h"] / p["body_w"]
    check("the case is still the deck's proportion",
          abs(ratio / deck - 1) < 0.005,
          f"case {p['case_w']:.2f} x {p['case_h']:.2f} is {ratio:.4f}, "
          f"deck {p['body_w']:.2f} x {p['body_h']:.2f} is {deck:.4f}")

    print(f"\n  case {p['case_w']:.1f} x {p['case_h']:.1f} x {case.extents[2]:.1f} mm, "
          f"{fw:.1f} over the rails")
    print(f"  front {front.volume/1000:.0f} cm^3 + back {back.volume/1000:.0f} cm^3"
          f"  =  {tot:.0f} cm^3 (~{tot * 1.24 * 0.55:.0f} g printed)")
    print(f"  each half needs a {fw:.0f} x {fh:.0f} mm bed")
    print(f"  hardware: 8 x M5 x {p['case_bolt_len']:.0f} socket cap, 8 x M5 nut, "
          f"4 x {p['magnet_d']:.0f}x{p['magnet_h']:.0f} disc")
    print()
    if FAILED:
        print(f"  {len(FAILED)} check(s) failed.")
        return 1
    print("  all checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
