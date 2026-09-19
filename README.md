# cYbErDeCk

A single-form, overbuilt, minimum-footprint handheld terminal enclosure for the
**Waveshare ESP32-S3-RLCD-4.2** and a **Rii 518BT** mini Bluetooth keyboard.

Built for live use — musical live coding, control surfaces, and as a portable
node on a local network — rather than for a desk.

<p align="center">
  <img src="docs/img/chassis-front.png" width="42%" alt="Chassis, front">
  <img src="docs/img/backplate-iso.png" width="42%" alt="Back plate">
</p>

```
116.6 × 139.1 × 16.6 mm   (+ 9.0 mm battery cowl)
2 printed structural parts · 4 screws · 52 automated checks
```

---

## What it is

Two rigid rectangles have to sit side by side, so most of the size of this thing
is arithmetic, not taste:

```
width  = wall + max(keyboard 110.2, board 93.5) + wall
height = wall + keyboard 59.4 + spine + board 70.1 + wall
depth  = back plate + board stack 11.0 + front face
```

`tools/validate.py` asserts each of those three equalities on every run. **The
enclosure cannot be made smaller without thinning a wall or crushing a part.**

What *is* a design choice is that it reads as one object. The entire front face,
all four side walls and the inter-bay spine are a single continuous body; the
joint moves to the back, where a structural plate closes the shell into a
torsion box. There is no bezel line across the face you touch.

| | Reference design | This design |
|---|---|---|
| Envelope | 151.65 × 116.77 × 18.00 | **116.6 × 139.1 × 16.6** |
| Battery bulge | +12.0 mm, separate clip-on cover | +9.0 mm, integral grip ridge |
| Wall | 2.90 mm | **3.20 mm** (8 extrusions, solid perimeters) |
| Split | bezel screwed onto a tray | monocoque front, joint at the back |
| Footprint area | 17 708 mm² | **16 218 mm² (−8.4 %)** |

12.55 mm shorter, 1.4 mm thinner, *and* thicker-walled.

## Build it

```sh
pip install -r tools/requirements.txt

openscad -D 'part="chassis"'   -o export/stl/chassis.stl   cad/cyberdeck.scad
openscad -D 'part="backplate"' -o export/stl/backplate.stl cad/cyberdeck.scad
openscad -D 'part="buttons"'   -o export/stl/buttons.stl   cad/cyberdeck.scad

python3 tools/validate.py          # 52 checks; non-zero exit if any fail
```

0.4 mm nozzle, 0.2 mm layers, no supports. Full instructions, BOM and print
orientations in **[docs/ASSEMBLY.md](docs/ASSEMBLY.md)**.

Two presets: `overbuilt` (3.2 mm walls, default) and `compact` (2.4 mm).

## How it is grounded

Every dimension is traceable to a source a third party can check. Nothing is
estimated, and nothing is traced off a picture.

- **Board geometry** comes from Waveshare's own published CAD package — Creo
  STEP assembly, dimensioned DXF, dimensioned PDF.
- **Keyboard geometry** comes from Riitek's product spec and FCC ID
  `YIZRT-RII518`, cross-checked against two independent third-party enclosures.
- **`tools/measure_reference.py`** re-derives every measured datum numerically
  from reference artefacts: planar sectioning with explicit world-preserving
  transforms, scan-line wall probing, ray-cast depth profiling, DXF group-code
  parsing.
- **`tools/validate.py`** renders the parts from source and booleans them
  against component mock-ups built from the datums. It is a gate, not a report.

The mounting-hole pattern — the datum everything else hangs off — was
established **three times independently**: from two unrelated enclosure designs
by different authors, and from the factory drawing. All three give
**85.500 × 62.100 mm**, agreeing to 0.001 mm.

Three further datums measured from a third-party enclosure *before* the factory
CAD was located matched it exactly: microphone offsets ±32.500, battery offset
−19.400, speaker grille height 10.45.

### It also corrected the internet

The figure `92.5 × 70.1 × 13.5 mm`, repeated across distributor pages for this
board, **conflates three different objects**. 92.50 is the PCB length; 70.10 is
the *removable moulded stand base*, which overhangs the PCB by exactly 1.00 mm
on one long edge (the PCB is 69.10); and 13.50 excludes the battery holder,
which protrudes a further 5.45 mm.

Discarding that stand base — removable, and not load-bearing once you notice the
standoffs are on the PCB — is what makes this deck thinner than the reference.

Similarly, the panel is not 4.2 inches (it is 4.173, exactly 84.80 × 63.60 mm),
and its active area is **not centred** on the board: it sits 1.60 mm off.

Full record, including every correction and every remaining gap, in
**[docs/DATUMS.md](docs/DATUMS.md)**.

## What is *not* proven

**Nobody has built this.** It is asserted internally consistent by 52 automated
checks; it has not been printed, and the components have not been offered up to
a physical chassis.

Before committing to a full set, print the chassis alone and check the four
measurements listed in
[docs/DATUMS.md](docs/DATUMS.md#measure-these-before-a-final-print). The one to
check first is **O-04**: the factory drawing and the reference enclosure
disagree by about 2.5 mm on the height of the side buttons.

There is also no FEA and no drop testing. The stiffness argument in
[docs/DESIGN.md](docs/DESIGN.md) is reasoning from section geometry — closed box
versus open channel, joint moved out of the peak-bending plane — not simulation.

## Documentation

| | |
|---|---|
| **[DATUMS.md](docs/DATUMS.md)** | every dimension, its provenance, five recorded corrections, six open items |
| **[DESIGN.md](docs/DESIGN.md)** | why it is shaped this way, and what was traded away |
| **[METHODOLOGY.md](docs/METHODOLOGY.md)** | how the numbers were obtained and how to reproduce them |
| **[ASSEMBLY.md](docs/ASSEMBLY.md)** | BOM, print settings, build order |
| **[PROVENANCE.md](docs/PROVENANCE.md)** | what was taken from whom, and on what basis |

## Repository layout

```
cad/
  parameters.scad     single source of truth — every dimension, tagged and cited
  cyberdeck.scad      chassis, back plate, button sprue
  lib/util.scad       geometry helpers
  lib/components.scad worst-case component envelopes, for fit checking
tools/
  measure_reference.py  metrology harness — regenerates every measured datum
  validate.py           52-check design audit — the build gate
docs/                 datum sheet, design rationale, methodology, assembly
export/reports/       machine-readable measurement and validation output
```

`cad/parameters.scad` is the only place a number may appear. A bare dimension
anywhere else in the CAD is a bug.

## Credits

This design measures, but does not copy,
**[nilseuropa/solar_term](https://github.com/nilseuropa/solar_term)** — the
SolarTerm enclosure and the [SolarOS](https://github.com/nilseuropa/solar_os)
project it houses. Their work is what got these two components living together
in the first place, and their keyboard tray is machined tightly enough
(−0.02 mm) that reading it out validated the manufacturer's own figure.

No geometry from that project is used here, and it carries no licence of its
own. See [PROVENANCE.md](docs/PROVENANCE.md).

Board CAD published by [Waveshare](https://docs.waveshare.com/ESP32-S3-RLCD-4.2).

## Licence

[MIT](LICENSE) for the original contents of this repository. The dimensional
values are measurements of third-party products; they are facts, and are not
claimed as property.
