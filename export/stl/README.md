# Printable parts

Regenerated from source by `./tools/build.sh`. **Do not edit these by hand** —
every dimension in them comes from `cad/parameters.scad`, and a hand-edited STL
silently detaches from the datum set and from all 118 checks.

| File | Size (mm) | Volume | ~PLA | Notes |
|---|---|---|---|---|
| `case/carrycase-front.stl` | 149.85 × 175.93 × 16.00 | 150 cm³ | ~186 g | print **face down**; holds the four magnets; 13.00 of body + 3.00 of locating pin |
| `case/carrycase-back.stl` | 151.60 × 177.08 × 24.45 | 298 cm³ | ~243 g | print **back down**; takes the seven heat-set inserts |

They bolt together with **7 × M5 × 16 socket cap into 7 × M5 × 10 heat-set
inserts**, in a ring that outsets the deck cavity by 6.50 mm and so follows the
case's own superellipse round the flanks and the floor. The heads lie flush: a
Ø9.00 × 5.00 counterbore under an r11.00 dish cut 1.00 mm into the face.

Each needs a **152 × 177 mm** bed. Both are one watertight body with **0 mm² of
near-flat ceiling** — the only overhang in either is the 1.80 mm annular roof of
each counterbore, and all seven of those open on the bed.

An earlier one-piece `case/carrycase.stl` is gone: it could not be reached into
to fit the magnets, and its spined back needed support. See
[DATUMS.md C-41](../../docs/DATUMS.md#corrections).

There is no separate window or screen protector: the display sits 2.65 mm
below the outer face behind a 2.4 mm front panel, and that recess is the
protection. An earlier revision exported a `window.stl` copied from the
reference's acrylic template; it was 0.72 mm larger than this design's board
pocket in both axes and had no seat anywhere, so it could not be fitted. See
[DATUMS.md C-09](../../docs/DATUMS.md#corrections).

All three are watertight, single-body manifolds — asserted on every run by the
`MESH` class in `tools/validate.py`.

## Printing

0.4 mm nozzle, 0.2 mm layers, **no supports on any part**.

Wall thicknesses are integer multiples of the nozzle width so every wall prints
as solid perimeters with no sparse infill in the load path. **Do not reduce the
perimeter count** — that is where the strength is.

| Part | Orientation | Why |
|---|---|---|
| chassis | front face on the bed | the aperture draft prints as a chamfer rather than a bridge, and the visible face gets the bed finish |
| backplate | flat face on the bed, cowl upward | the cowl's sides are vertical and its cap is a dome, so it needs no support; the countersinks face the bed and print clean |
| buttons | caps down | — |

Full build order, BOM and fastener spec in [docs/ASSEMBLY.md](../../docs/ASSEMBLY.md).

## Before you print a full set

**Nothing here has been printed or fitted to hardware.** It is asserted
internally consistent and confirmed against a working reference design, which
is not the same thing.

Print the **chassis alone** first and offer the board and keyboard up to it. The
measurement to take first is the **keyboard's thickness**: the pocket is 11.0 mm
deep and sits exactly on its assertion against a 10.6 mm upper band, and the
manufacturer's drawing never says whether its stated 10.2 mm includes the
keycaps. That is the only axis in the design with no slack left.

See [docs/DATUMS.md](../../docs/DATUMS.md#measure-these-before-a-final-print).

## Regenerating

```sh
pip install -r tools/requirements.txt
./tools/build.sh            # renders every part, then runs all 71 checks
```

The build is deterministic: a clean run reproduces these files byte for byte.
If they show as modified afterwards, the geometry actually moved — that is a
signal, not noise.
