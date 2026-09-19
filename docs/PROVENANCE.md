# Provenance and licensing

This project measures other people's work. That obliges it to be precise about
what was taken, what was not, and on what basis.

## What this project is

A parametric enclosure for two commercially available components. Its inputs are
**facts about those components** — where the mounting holes are, how thick the
board is, how wide the keyboard is. Its output is original geometry authored
from those facts.

## The reference enclosure

The starting point was a MakerWorld model, `makerworld.com/models/3251467`
("SolarTerm: ShareWave ESP32 S3 RLCD4.2 Case", by JustinWu, licensed BY-NC-SA),
which credits and derives from **[nilseuropa/solar_term](https://github.com/nilseuropa/solar_term)**.
That upstream repository is the artefact actually used here.

### What was taken

Numbers describing **the Waveshare board and the Rii keyboard**:

- the board's mounting-hole pattern, pocket size and edge-feature positions;
- the keyboard's pocket size and depth;
- the display aperture and the acrylic window outline.

These are measurements of third-party hardware. The SolarTerm author did not
create the Waveshare board's 85.5 × 62.1 mm hole pattern; they measured it, as
this project did, and as anyone with calipers would. Facts about a product's
dimensions are not authorship.

Two things make this defensible rather than merely convenient:

1. **Every load-bearing datum was independently re-derived.** After the
   Waveshare CAD package was located, the board geometry was rebuilt from the
   manufacturer's own drawing. Where the reference and the factory drawing both
   speak, they agree — see [DATUMS.md D-06](DATUMS.md#d-06--board-edge-features).
   The reference is corroboration now, not the source.
2. **The measurement is reproducible.** `tools/measure_reference.py` recovers
   every `[MEASURED]` datum numerically from the reference files. Nothing was
   traced, eyeballed or copied by hand.

### What was not taken

**No geometry.** Not a mesh, not a profile, not a feature tree, not an STL.
Nothing in `cad/` is derived from, traced over, or offset from any SolarTerm
file. The architecture is deliberately different:

| | SolarTerm (ATA) | This project |
|---|---|---|
| Split | bezel screwed onto a tray | monocoque front, joint at the back |
| Printed structural parts | 2 + button covers | 2 + button sprue |
| Board mounting | into the enclosure | into the board's own SMTSO standoffs |
| Stand base | retained | discarded |
| Battery cover | separate, clipped | integral cowl |
| Envelope | 151.65 × 116.77 × 18.00 | 116.25 × 138.85 × 16.85 |

The reference solves the same problem in a different way. That is what makes it
useful as a cross-check.

### Licensing position

`nilseuropa/solar_term` carries **no LICENSE file**, so no licence is granted
and its files are all-rights-reserved by default. That is precisely why no
geometry from it appears here, and why it is treated strictly as a measuring
instrument. The MakerWorld remix is BY-NC-SA; again, nothing was copied from it.

If the SolarTerm author considers any part of this an overreach, open an issue
and it will be corrected.

### Credit

The SolarTerm authors did the original work of getting these two components to
live together, and did it well enough that their pockets could be used as
instruments.

An earlier revision of this file credited their keyboard tray as a deliberate
−0.02 mm press fit. That reading was wrong — it compared their tray against a
rounded vendor figure rather than the real body, and the tray is an ordinary
+0.70 mm clearance fit (see [DATUMS.md C-06](DATUMS.md#corrections)). The credit
stands on better grounds: because their deck is *built and working*, its pockets
are hard physical bounds on a component whose vendor rounds its own figures, and
that is worth more than a coincidence.

## Waveshare

Board geometry comes from Waveshare's own published CAD package
(`ESP32-S3-RLCD-4.2-3dFile.rar`: Creo STEP assembly, dimensioned DXF,
dimensioned PDF), linked from their documentation's Resources page. It is
published by the manufacturer for exactly this purpose — designing things that
fit the board.

No Waveshare file is redistributed here. `cad/parameters.scad` records
dimensions read from those files, with the entity or drawing label each came
from cited in the comment.

## Riitek

Keyboard dimensions come from Riitek's own product page and from the FCC filing
for FCC ID `YIZRT-RII518`. FCC filings are public records.

## Standards

ISO 273 (clearance holes), ISO 4762 (socket-cap screws) and ISO 10642
(countersunk screws) are cited for fastener fits. Only the derived numbers are
reproduced, not the standard texts.

## This project's licence

See [LICENSE](../LICENSE). The enclosure geometry, the metrology harness and
the validation harness are original work and are offered permissively.

The datum *values* are measurements of third-party products. They are facts, and
facts are not owned — by this project or anyone else. Use them.
