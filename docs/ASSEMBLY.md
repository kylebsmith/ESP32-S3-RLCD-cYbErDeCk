# Assembly

Read this before printing. The RLCD panel is fragile and must never be used as
leverage while fitting a cable, a cell or a printed part.

> **This design has not been built.** It is asserted internally consistent by 52
> automated checks, but no one has held these components against a printed
> chassis. Print the **chassis alone** first, offer the board and keyboard up to
> it, and check the four measurements in
> [DATUMS.md](DATUMS.md#measure-these-before-a-final-print) before committing to
> a full set.

## Bill of materials

### Printed

| Part | Qty | Mass (PLA) | Notes |
|---|---|---|---|
| `chassis` | 1 | ~51 g | front face down |
| `backplate` | 1 | ~58 g | cowl up |
| `buttons` | 1 | ~0.5 g | printed as one sprue |

### Hardware

| Item | Qty | Notes |
|---|---|---|
| Waveshare ESP32-S3-RLCD-4.2 | 1 | **not** an e-paper or other ESP32-S3 display board |
| Rii 518BT mini Bluetooth keyboard | 1 | not the K18, and not the RT518S |
| 18650 lithium cell | 1 | flat-top or protected, up to Ø18.6 × 69.0 |
| microSD card | 0–1 | FAT32 |
| Speaker with the MX1.25 2-pin lead supplied with the board | 1 | |
| M2 × 5 brass heat-set insert, Ø4.0 OD | 4 | |
| M2 × 8 countersunk screw, ISO 10642 | 4 | M2 × 12 if rigging a bracket |
| M2.5 screw, supplied with the board | 4 | into the board's own standoffs |
| Adhesive foam gasket, 0.5 mm | 1 strip | around the display aperture |

## Printing

Defaults are set for a 0.4 mm nozzle at 0.2 mm layers. Wall thicknesses are
integer multiples of the extrusion width so every wall prints as solid
perimeters with no sparse infill in the load path — **do not reduce the
perimeter count**; that is where the strength is.

| Setting | Value |
|---|---|
| Nozzle | 0.4 mm |
| Layer height | 0.2 mm |
| Perimeters | 4 or more (walls are 8 extrusions thick) |
| Infill | 25 % gyroid |
| Material | PETG or ASA preferred; PLA is fine but softens in a hot car |
| Fuzzy skin | outer walls only, amplitude 0.15-0.25 mm, point distance 0.6 mm |

On finish: use a wood-, stone- or hemp-filled filament, or a matte PETG in a
warm neutral. Avoid gloss - it turns the rolled edges into hard specular lines
and undoes the edge treatment entirely. Fuzzy skin on the outer walls gives the
side surfaces a fine mineral grain while the front face, printed against the
bed, stays smooth where the hand and eye land. Leave the brass inserts visible.
See [DESIGN.md](DESIGN.md#material-and-finish).

Orientation:

- **chassis** — front face on the bed. The aperture draft then prints as a
  chamfer rather than a bridge, and the visible face gets the bed finish.
- **backplate** — flat face on the bed, cowl upward. The cowl's sides are
  vertical and its cap is a dome, so no supports are needed. The countersinks
  face the bed and print clean.
- **buttons** — caps down.

No part needs support material.

## Assembly

### 1. Strip the board

Remove Waveshare's moulded stand base and its fold-out kickstand. Both are
removable and neither is load-bearing: the four SMTSO-M2.5-7ET standoffs are
surface-mounted to the **PCB**, not to the base.

This is what makes the deck 2.75 mm thinner and 1.0 mm narrower than it would
otherwise be. Keep the parts — they are how you use the board on a desk again.

### 2. Heat-set the inserts

Four M2 inserts into the chassis bosses, driven from the **back**. Keep them
straight and flush. The bosses are Ø7.4 around a Ø4.0 bore; let the insert melt
in under its own weight rather than forcing it, or the boss will split.

### 3. Fit the button sprue

Drop the three caps into the top-edge apertures from inside. The retaining
flange sits behind the wall; the caps cannot fall out once the board is in.

The three switches are PWR, BOOT and KEY. **Which is which is not established** —
see [DATUMS.md O-03](DATUMS.md#o-03--which-switch-is-pwr-which-is-boot-which-is-key).
All three apertures are identical, so the enclosure does not care; label them
once you have found out.

### 4. Gasket and seat the display

Run the 0.5 mm foam gasket around the inside of the display aperture. It absorbs
the tolerance stack and isolates the panel from shock.

Lay the chassis face down. Lower
the board in **display first**, holding the PCB — never the panel. Check the
three buttons and both microphone ports line up with their apertures.

### 5. Fit the keyboard

Drop the keyboard into the lower bay from behind, key side forward. It is
captured by the front-face lip on all four edges.

Check that the power switch and the charging port fall within a side service
window. There is one on **each** side, so if they do not, take the keyboard out
and turn it round.

### 6. Connect the speaker

Plug the MX1.25 lead in **before** the back plate goes on, and route the cable
so nothing can pinch it.

### 7. Fit the cell

Insert the 18650 with the polarity shown on the Waveshare holder. It will stand
proud of the PCB — that is expected; the cowl covers it.

### 8. Close it up

Lower the back plate bottom-edge first so the tongue enters the groove in the
chassis bottom wall, then swing the top down. The keeper pad should meet the
keyboard and the cowl should clear the cell.

If it does not sit flush, **stop** and find out why rather than pulling it down
with the screws.

Fit the four M2 × 8 countersunk screws and tighten evenly, just until the plate
is seated.

### 9. Check before power-on

- all three buttons move freely and spring back
- no cable is trapped
- the microSD slot and USB-C port are reachable
- nothing rattles

Then follow the SolarOS installation instructions from the
[upstream project](https://github.com/nilseuropa/solar_os), or flash whatever
firmware you prefer.

## Rigging

The four M2 back-plate screws are the accessory mounting points. They thread
into brass, not plastic, so they are the strongest anchors on the device.

Fit **M2 × 12** in place of M2 × 8 and clamp a bracket, strap yoke or stand
clamp under the heads. `accessory_pattern()` in `cad/cyberdeck.scad` prints the
hole pattern to drill a bracket to match.

There is deliberately no 1/4"-20 socket. The reason is in
[DESIGN.md](DESIGN.md#accessory-mounting-the-four-screws-are-the-rig-points).

## Servicing

Four screws release the back plate, and the board, cell and plate come out as a
module. Push the keyboard out through the finger hole in the back plate.

Changing the cell means removing the back plate — the cowl is integral. That is
a deliberate trade for robustness; see
[DESIGN.md](DESIGN.md#the-battery-has-to-stick-out-so-it-was-made-useful).
