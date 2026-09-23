# Printable parts

Regenerated from source by `./tools/build.sh`. **Do not edit these by hand** —
every dimension in them comes from `cad/parameters.scad`, and a hand-edited STL
silently detaches from the datum set and from all 118 checks.

| File | Size (mm) | Volume | ~PLA | Notes |
|---|---|---|---|---|
| `chassis.stl` | 116.60 × 139.10 × 16.85 | 42.9 cm³ | 53 g | the monocoque: front face, all four walls, the spine |
| `backplate.stl` | 109.60 × 133.30 × 13.45 | 48.4 cm³ | 60 g | structural closure + battery cowl; the board bolts to this |
| `buttons.stl` | 26.00 × 4.80 × 6.59 | 0.4 cm³ | 0.5 g | three caps on a sprue |

The carry case is a separate object and prints separately:

| File | Size (mm) | Volume | ~PLA | Notes |
|---|---|---|---|---|
| `case/carrycase-front.stl` | 162.65 × 162.45 × 22.12 | 202 cm³ | 138 g | print **face down**; holds the four magnets. 22.12 not 19.13 because the four locating pins stand 3 mm proud of the joint |
| `case/carrycase-back.stl` | 162.65 × 162.45 × 19.12 | 258 cm³ | 176 g | print **back down**; holds the eight nuts |

They bolt together with **8 × M5 × 25 socket cap and 8 × M5 nut**. Each needs a
163 × 162 mm bed. An earlier one-piece `case/carrycase.stl` is gone: it could
not be reached into to fit the magnets, and its spined back needed support. See
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
