# Assembly

Read this before printing. The RLCD panel is fragile and must never be used as
leverage while fitting a cable, a cell or a printed part.

> **This design has not been built.** All **113** automated checks pass, but
> nobody has held these components against a printed chassis. Print the
> **chassis alone** first, offer the board and keyboard up to it, and check the
> four measurements in
> [DATUMS.md](DATUMS.md#measure-these-before-a-final-print) before committing to
> a full set.

## Bill of materials

### Printed

| Part | Qty | Mass (PLA) | Notes |
|---|---|---|---|
| `chassis` | 1 | ~53 g | front face down |
| `backplate` | 1 | ~58 g | cowl up |
| `buttons` | 1 | ~0.5 g | printed as one sprue |
| `cover` | 1 | ~60 g | **variant 2 only** — inner face up, no supports |

### Hardware

| Item | Qty | Notes |
|---|---|---|
| Waveshare ESP32-S3-RLCD-4.2 | 1 | **not** an e-paper or other ESP32-S3 display board |
| Rii 518BT mini Bluetooth keyboard | 1 | not the K18, and not the RT518S |
| Ø5 × 2 mm N52 neodymium disc | 8 | **variant 2 only** — 4 in the shell, 4 in the cover |
| 18650 lithium cell | 1 | flat-top or protected, up to Ø18.6 × 69.0 |
| microSD card | 0–1 | FAT32 |
| Speaker with the MX1.25 2-pin lead supplied with the board | 1 | |
| M2 × 4 brass heat-set insert, Ø3.2 OD | 4 | Ø3.2 is the bore; a Ø4.0 insert is M3 |
| M2 × 6 countersunk screw, ISO 10642 | 4 | chassis ↔ back plate. See the note below — **not** M2 × 8 |
| M2.5 × 8 countersunk screw, ISO 10642 | 4 | into the board's own standoffs |

**On the two screw lengths.** Both are worked from the stack, not chosen:

- **M2 × 6, not longer.** The plate is 3.20 mm and the insert is 4.00 mm, so a
  6 mm screw engages 2.80 mm — 1.4 × diameter, comfortably past the 1 × minimum.
  M2 × 8 would put 4.80 mm past the plate into a 4.00 mm insert: the last
  0.80 mm spins in the relief below it and buys nothing.
- **M2.5 × 8 for the board, not the screws in the box.** The screws supplied
  with the board are 4.90 mm overall and countersunk, so through a 3.20 mm
  plate they leave **1.70 mm** biting the standoff — 0.68 × diameter, below the
  usual 1 × minimum, and these four screws carry the board and an 18650. At
  8 mm the engagement is 4.80 mm into a 7.00 mm standoff, 1.9 × diameter. Keep
  the stock screws for a test fit; do not ship on them.
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
- **backplate** — flat face on the bed, cowl upward. That face is genuinely
  flat now: the keyboard keeper used to be a 0.25 mm raised pad here, which made
  it the only thing touching the bed and left 5777 mm² — 48% of the underside —
  printing over open air. The step lives on the chassis instead. See
  [DATUMS.md C-32](DATUMS.md#c-32--the-plates-bed-face-was-a-pad-over-open-air).
  The cowl's sides are
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
straight and flush. The bosses are **Ø6.6 around a Ø3.2 bore, 6.5 mm deep** —
the Ø7.4/Ø4.0 figures this step used to give were the M3 revision's and were
never updated. Let the insert melt in under its own weight rather than forcing
it, or the boss will split.

### 2b. Fit the magnets — variant 2 only

Eight Ø5 × 2 mm N52 discs, four in the chassis and four in the cover. Do this
with the chassis **front face down** on the bench.

Each shell station is a Ø6.0 access shaft running down to the back-plate
seating plane, ending in a ribbed pocket under the show face. Drop a disc into
the shaft, let it fall to the mouth of the pocket, and push it home with a 4 mm
rod until it bottoms on the 0.8 mm skin. It should take a firm push — eight
crush ribs close the 5.50 mm bore to 4.80, so the disc sees 0.10–0.30 mm of
interference depending where it falls in its own ±0.10 mm tolerance. If it
pushes in with no resistance at all, raise `magnet_rib_h`; if it will not start,
lower it. That one parameter is the whole adjustment.

The cover's four pockets are open on its **inner** face — press the discs in
flush. There is deliberately no plastic between the cover's discs and the
chassis's: the only gap in the magnetic circuit is the shell's own 0.8 mm skin,
which is what makes four small magnets enough.

**Polarity: every disc goes in the same way up.** Mark one pole before you
start and keep it consistent — all four shell discs facing out, all four cover
discs facing out. You cannot fit the cover backwards, so alternating polarity
would buy nothing and turn every disc into a coin toss.

Nothing is visible from outside when the back plate goes on.

### 3. Fit the button sprue

Drop the three caps into the top-edge apertures from inside. The retaining
flange seats in the counterbore behind each aperture, so nothing stands proud
of the wall's inner face, and the board stops them falling in once it is
fitted. Only the plungers go past the wall, and they are offset so they land on
the switch bodies and miss the PCB's edge entirely. The free travel — **0.60
mm** — comes from the counterbore being 1.00 mm deep and the flange 0.40 thick,
not from the 0.50 mm the board leaves behind the wall. See
[DATUMS.md O-08](DATUMS.md#o-08--the-button-sprue-did-not-fit-behind-the-wall--closed) for why it is that tight.

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

It will feel tight, and it is meant to. Eight half-round locating ribs — four
on the long walls, four on the short ones — take the play out of the pocket so
the keyboard cannot slide and the front lip stays even all the way round. They
are tapered at the entry end; push the keyboard in square and it will seat.

The power switch and the charging port must fall within the service window on
the **left** side, as you look at the front of the device. There is only one
window, because everything that needs reaching is on that one short edge and
the keys only read one way up. If they end up on the right, the keyboard is in
upside down.

### 5b. Fit the cover — variant 2 only

Offer it up square and let it go from a millimetre out; the two register
platforms find their apertures and pull it straight. To remove it, get a thumb
into the scallop on the top edge and peel — do not try to slide it off.

The platforms, not the magnets, are what stop it sliding. Four pairs make about
15 N of pull across the 0.8 mm skin but only about 3 N of shear, which is a
fraction of what a bag strap can apply; the platforms carry all of it.

### 6. Connect the speaker

Plug the MX1.25 lead in **before** the back plate goes on, and route the cable
so nothing can pinch it.

### 7. Fit the cell

Insert the 18650 with the polarity shown on the Waveshare holder. It will stand
proud of the PCB — that is expected; the cowl covers it.

### 8. Close it up

Lower the back plate bottom-edge first so the tongue enters the groove in the
chassis bottom wall, then swing the top down. The plate's flat inner face
should meet the keyboard — the 0.25 mm step that holds it forward is a ledge in
the chassis bay now, not a pad on the plate — and the cowl should clear the
cell.

The tilt is not optional and it is not large. Searched as a rigid-body motion
against the rendered chassis — rotation about X plus translation in Y and Z,
every configuration checked on fourteen cross-sections — the plate needs about
**1.5°** of tilt and a **1.0 mm** slide along −Y to get the tongue under the
groove's lip; flat-on it is 0.70 mm too tall for the opening. With the board and
the keyboard already in the chassis the clearance path needs up to **7.5°**,
because the cowl has to swing over the 18650. Removal is the same path
reversed, and the board, cell and plate do come out as one bolted module.

If it does not sit flush, **stop** and find out why rather than pulling it down
with the screws.

Fit the four **M2 × 6** countersunk screws and tighten evenly, just until the
plate is seated. (This step read M2 × 8 while the bill of materials read M2 × 6
and explained why; 6 is the right one.)

### 9. Bolt the board to the plate

Four **M2.5 × 8** countersunk screws, from outside the back plate, up into the
board's own SMTSO standoffs. They are what carries the board and the cell, and
they are the reason the board, cell and plate come out as one module.

Drive the two nearest the cowl first: they sit 0.60 mm below the 18650 holder
and are the tightest pair on the plate.

### 10. Check before power-on

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

Fit **M2 × 12** in place of M2 × 6 and clamp a bracket, strap yoke or stand
clamp under the heads. The bracket has to be at least **2.3 mm** thick: the
insert bore is 6.5 mm deep under a 3.2 mm plate, so a 12 mm screw with nothing
under its head bottoms in the bore before it is tight. `accessory_pattern()` in `cad/cyberdeck.scad` prints the
hole pattern to drill a bracket to match.

There is deliberately no 1/4"-20 socket. The reason is in
[DESIGN.md](DESIGN.md#accessory-mounting-the-four-screws-are-the-rig-points).

## Servicing

Four screws release the back plate, and the board, cell and plate come out as a
module — verified as a rigid-body path, not merely as a clearance.

**The keyboard comes out of the open bay, not through the finger hole.** The
Ø19 → Ø13 hole in the back plate is inherited from the reference, where it
pushes a keyboard out of a separate tray. Here it cannot eject anything: with
the plate on, the keyboard is captured by the front-face lip on all four edges
and pushing it forward does nothing; with the plate off, the hole has left with
the plate. It survives as finger access to break the keyboard free of its
pocket once the plate is off, and that is all it does. Lift or tip the keyboard
out of the open bay.

Changing the cell means removing the back plate — the cowl is integral. That is
a deliberate trade for robustness; see
[DESIGN.md](DESIGN.md#the-battery-has-to-stick-out-so-it-was-made-useful).
