# Methodology

How the numbers were obtained, and how to reproduce them.

Two claims are made in this repository. This document explains how each is
supported, and — as importantly — where each stops.

1. **The datums are right.** Supported by `tools/measure_reference.py`,
   plus manufacturer CAD, plus cross-derivation.
2. **The model is consistent with the datums.** Supported by
   `tools/validate.py`, which is a gate, not a report.

Neither claim is that the parts have been printed and fitted. They have not.

---

## Reproducing everything

```sh
pip install -r tools/requirements.txt
git clone --depth 1 https://github.com/nilseuropa/solar_term /tmp/solar_term

python3 tools/measure_reference.py --reference /tmp/solar_term \
        --json export/reports/measurements.json
python3 tools/validate.py --json export/reports/validation.json
```

The first regenerates every `[MEASURED]` datum from the reference artefacts. The
second renders the parts from source and audits them. Both are run in CI.

---

## Metrology

### Bounding boxes

The coarse envelope of each reference part, via `trimesh`. Useful for
orientation and sanity, useless for features.

### Planar cross-sectioning

The main instrument. A mesh is sliced at a swept series of levels; each section
polygon's **interior rings are the pockets, bores and apertures**, and their
bounds and centroids give position and size directly.

> **The trap that matters.** `trimesh`'s `Path3D.to_2D()` picks an arbitrary
> in-plane basis, and that basis *drifts from slice to slice*. Sizes survive it;
> absolute positions do not. An early pass of this work produced a table of
> plausible, self-consistent, completely wrong coordinates — the outer boundary
> of a part appeared to move 21 mm across its own thickness. Every section in
> `measure_reference.py` therefore passes an **explicit world-preserving
> transform**:
>
> ```python
> _TO_2D = {2: np.eye(4), 1: ..., 0: ...}   # one per slicing axis
> planar, _ = sec.to_2D(to_2D=_TO_2D[axis])
> ```
>
> If you re-derive any of these numbers with a different tool, check this first.

### Scan-line probing

A pocket that is *open to the exterior* is not a closed interior ring, so
sectioning cannot find it. The ATA keyboard tray is one: its wall breaks for a
port notch. Those are measured by intersecting a line with the section's solid
region and reading the material intervals, which gives wall positions and
thicknesses directly.

### Ray casting

Depths, floor thicknesses and bore depths come from casting a ray along the
thickness axis and pairing the hits into solid intervals. This is what
established the ATA stack-up: 3.0 mm back wall, 13.0 mm board pocket, 4.6 mm
keyboard tray floor, 11.4 mm tray, 2.0 mm bezel.

Ray casting is also robust to a mesh that is not watertight —
`Caseback.stl` has four broken faces — where boolean methods are not.

### DXF group-code parsing

`stl/ata/plexiglass.dxf` is a 2D acrylic template: a single closed
`LWPOLYLINE`, DXF AC1015, `$INSUNITS = 4` (millimetres). A 2D template is an
exact statement of intent with no interpretation in between, which makes it one
of the highest-confidence sources available. Parsed with `ezdxf`; bulge values
are converted to arc radii to recover the corner treatment.

### Manufacturer CAD

The strongest source, and the one that corrected the most. Waveshare publishes a
Creo STEP assembly, a dimensioned DXF and a dimensioned PDF. Values were taken
from drawing `DIMENSION` entities and from B-rep bounding boxes of *named*
solids — `SMTSO-M2_5-7ET`, `SWITCH-TS24CA`, `MIC-4X3X1MM` — so each number is
attributable to a specific part, not to a silhouette.

The axis mapping between the drawing views and the STEP was **not assumed**. It
was pinned by matching asymmetric features — the 2×8 header centre, the three
button centres, the active-area margins — across both. A naive reading mirrors
the long axis and puts the active area 5.45 mm from the wrong edge.

---

## Corroboration strategy

A single source is a hypothesis. The rule applied here: **every datum that
drives geometry is established at least twice, by paths that can fail
independently.**

The mounting-hole pattern is the model case — measured from two unrelated
enclosure designs by different authors, and read from the factory drawing. Three
paths, one answer, 0.001 mm apart.

Where corroboration was impossible the datum is marked `[PROVISIONAL]` and
listed as an open item, and the design is arranged so the gap cannot cause a
clash — the keyboard pocket clears both
candidate bodies rather than assuming the vendor's rounded figure is the real
one; the board pocket carries 0.50 mm per side, more than PCB routing tolerance.

### Conflict resolution

Where sources disagree, precedence is:

```
manufacturer drawing / 2D template
  > manufacturer CAD solid
  > manufacturer spec text
  > measured from a reference artefact
  > peer model
  > photo-derived
```

Every conflict actually encountered is recorded in
[DATUMS.md § Corrections](DATUMS.md#corrections) with the losing value, rather
than silently overwritten. Five of the six corrections there were beliefs acted
on before being caught, and the sixth is worse: a *reasoning* error behind a
number that happened to be right, kept because the lesson generalises.

---

## Validation

`tools/validate.py` renders the parts **from source** and runs 111 checks in
nine classes: `MESH`, `ENVELOPE`, `FIT`, `OPENING`, `OBSTRUCTION`, `STACK`,
`INTERFACE`, `PRINT`, `DATUM`. Exit status is non-zero unless all pass.

Two of those classes exist because they were absent, and both are about the
same blind spot in different directions:

- `OPENING` asks whether a hole was **cut**. It was added after three side
  buttons and two microphones turned out to have no opening at all.
- `OBSTRUCTION` asks whether anything is **lying on** a hole that was cut. It
  was added after the battery cowl's foot flare was found covering two of the
  four board-screw countersinks and 3.6 mm of the expansion window — holes that
  existed, were counted, and could not be used.
- `STACK` measures a depth stack against the space that actually exists between
  the face a part enters and the component it has to reach. An extent check
  says the button sprue is 6.59 mm long; it does not say that only 3.89 mm of
  anything can exist there.

It reads its expected values by parsing `cad/parameters.scad`, so it cannot
drift from the model.

The `FIT` class is the substantive one. Component mock-ups are built strictly
from the datums, rendered, and **booleaned against the real parts**; any shared
volume above 1 mm³ is a failure. The mocks are deliberately drawn as worst-case
envelopes rather than replicas, so a mock that fits guarantees a real part that
fits and never the reverse.

### What it actually caught

Not hypothetical. Each of these was in a committed, rendering, plausible-looking
model:

| Defect | Consequence |
|---|---|
| Keyboard bay left a 1.6 mm web where the keeper pad goes | keyboard could not be loaded at all |
| Side ports cut through the outer wall only | both connectors buried behind 8 mm of plastic |
| Upper countersink broke out of the back plate edge | two of four fasteners unusable |
| ISO 4762 head recess consumed the whole plate | no material under the head to take preload |
| Cowl tapered before reaching the cell | 850 mm³ interference with the battery |
| Board pocket R3.0 fouled the PCB's R0.5 corners | 0.33 mm interference, all four corners |
| Cowl cavity R8 fouled the holder's R2.0 corners | 1.39 mm interference, all four corners |
| Expansion window copied from a *narrower* vendor window | header body would not pass |
| Acrylic window parameters deleted in a rewrite | part rendered 2 x 2 x 1 mm and passed every other check |
| Back-opening corner fuller than the keyboard bay's own | 10.96 mm2 of bay undercut; keyboard trapped, invisible to a clash test |
| Cowl cavity straight inside a crowned outer | cavity punched through the wall; plate in two pieces |
| Tongue rooted on a rolled edge that had drawn back | tongue floated free of the plate |
| Aperture corner squarer than the keyboard's own corner | lip went negative; four open gaps into the pocket, no corner retention |
| Both component pockets cut `+1` proud instead of `+0.01` | 1 mm off the retaining lip, 1.4 mm of a 2.4 mm panel |

Most of those are invisible in a render. That is the argument for
numerical gating over inspection.

Two of those were found only because a check was rewritten to be
two-dimensional. The lip test compared `aper_w < pocket_w` and the same in
height, which measures the flats and never the corners — and the lip fails at
the corners. **A one-dimensional test of a two-dimensional problem passes
confidently and proves nothing.**

The harness has also had its own bugs, which is worth stating: its parameter
parser once joined any line ending in `=` onto the next, so every datum
following a `// =====` section rule vanished and the dependent checks died with
a `KeyError` instead of reporting a result. A validator that fails loudly is
fine; one that silently checks nothing is worse than none.

---

## What the reference audit covers

`tools/audit_reference.py` re-derives the reference's own geometry from its
meshes on every run and compares it feature by feature. For the keyboard — the
component whose geometry this project has least independent access to — that is
eleven dimensions: pocket width, height and depth against both the ATA tray and
the proof-of-concept bay, the front aperture in both axes, the pocket corner
radius, both ends of the eject-port taper, and a second, independent
cross-check of the tray width.

Two of those deserve a note on method, because the obvious approach fails:

- **The tray never forms a closed section ring.** It is open to the exterior at
  the port notch, so the sectioning that measures every other pocket here
  returns nothing. The corner radius is taken instead from the wall triangles
  whose normals point *into* the tray — which excludes the outer skin, the floor
  and the rim — and a circle is least-squared through each corner. Three of the
  four fit to an RMS under 0.05 mm; the fourth is cut by the notch and is
  discarded rather than averaged in. That method reads the tray as
  109.205 mm wide against the scan-line method's 109.200, so the two agree to
  0.005 mm.
- **The eject port is a taper**, so any single section returns whichever
  diameter that depth happens to have. It is swept and both ends are recorded.

**And one that is not audited.** The keyboard's service window is measured once
and recorded, not re-derived each run: the tray is open along that same edge and
the notch could not be separated reliably from the surrounding opening. It is
marked as such rather than left to look like an audited row. This design puts a
window on *both* short edges, so the datum does not gate anything — the
manufacturer's drawing now confirms the port and switch share one edge, and
either window serves it.

## Limits

- **No physical verification.** The largest limit by far.
- **Mocks are envelopes.** They prove no clash against a conservative solid, not
  against a real component's every boss and solder joint.
- **No FEA and no drop testing.** The stiffness claims in
  [DESIGN.md](DESIGN.md) are now computed rather than argued —
  `tools/structure.py` evaluates second moment of area, section modulus and
  Bredt's formula on sections cut from the real mesh — but classical section
  analysis gives **ratios and weak-point location**, which depend only on
  geometry, and not absolute stress, which depends on layer adhesion and strain
  rate. It found one claim overstated: the shell is a closed cell over 12 % of
  its length, not throughout.
- **No print verification.** Wall thicknesses are checked against nozzle
  multiples and overhangs against a draft-angle rule, but nothing has been
  sliced or printed.
- **`Caseback.stl` is not watertight** (4 broken faces). All measurements on it
  use ray casting and raw section geometry, which are unaffected, rather than
  boolean operations, which are not.
