# Printable parts

Regenerated from source by `./tools/build.sh`. **Do not edit these by hand** —
every dimension in them comes from `cad/parameters.scad`, and a hand-edited STL
silently detaches from the datum set and from all 71 checks.

| File | Size (mm) | Volume | ~PLA | Notes |
|---|---|---|---|---|
| `chassis.stl` | 116.60 × 139.10 × 16.60 | 42.2 cm³ | 52 g | the monocoque: front face, all four walls, the spine |
| `backplate.stl` | 109.60 × 134.70 × 13.20 | 46.9 cm³ | 58 g | structural closure + battery cowl; the board bolts to this |
| `buttons.stl` | 26.00 × 4.80 × 6.59 | 0.4 cm³ | 0.5 g | three caps on a sprue |
| `window.stl` | 94.22 × 70.82 × 2.00 | 13.3 cm³ | 16.5 g | optional printed screen protector; the real one is 2 mm laser-cut acrylic, template in `cad/` |

All four are watertight, single-body manifolds — asserted on every run by the
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
