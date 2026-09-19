# Caliper checklist

For someone holding the actual Waveshare ESP32-S3-RLCD-4.2 and Rii 518BT.

Everything in this project is asserted consistent and confirmed against a
working third-party design. **Neither of those is the same as being right.**
Each measurement below closes a specific open item, and the list is ordered by
what it costs you to be wrong.

Record what you get — including where it agrees — and the numbers go back into
`cad/parameters.scad` with `[MEASURED]` provenance.

---

## 1 · Keyboard thickness — **the one with no slack**

> The pocket is **11.00 mm**. The body is 10.20 nominal, 10.60 at the assumed
> upper band. That leaves **+0.40 mm**, and it is the only axis in the design
> sitting exactly on its assertion.

Measure **twice**, because the ambiguity is the whole point — the manufacturer's
drawing says `10.2mm/0.4inch` and never says what it is measured to:

| | Where | Expect |
|---|---|---|
| **a** | across the **moulding only**, at a corner clear of any key | ~10.2 |
| **b** | from the base to the **crown of a keycap**, mid-board | 10.2 – 10.6 |

Lay it face-down on glass and measure to the highest keycap for **b**. Include
the rubber feet if it has them, separately.

**If b > 10.60** the pocket is too shallow and `kbd_depth` must grow, which
grows the whole deck. **Tell me before printing.**

## 2 · 18650 holder protrusion past the standoff seating plane

> Currently **8.20 mm**. This, not the cell diameter, is what the battery cowl
> has to swallow. If it is under-measured the back plate will not close.

With the board face-down on a flat surface **resting on its four standoffs**,
measure from that surface to the deepest point of the battery holder. The
standoff tips are the datum, not the PCB.

## 3 · Board stack: display glass front → standoff seating plane

> Currently **10.75 mm**, in an 11.00 mm pocket, the 0.25 taken up by squeezing
> a 0.5 mm foam gasket.

Same setup: board resting on its standoffs, measure to the front face of the
display glass. If this is over 10.75 the gasket squeeze disappears and the
display is trapped against the front panel.

## 4 · 18650 holder footprint — **actively disputed, you can settle it today**

> `batt_bay_w × batt_bay_h` is **77.80 × 22.10**. A re-derivation from
> Waveshare's own CAD says the second figure should be **21.10**.

Measure the holder's footprint on the PCB back face, across the mounting flange
(the wider of the two dimensions), along the board's short axis.

**21.10 or 22.10?** One millimetre, and it drives a real cut in the back plate.

## 5 · Keyboard outline

> **108.5 × 58.2 mm**, with an *assumed* ±0.30 moulding tolerance that no
> document states.

Measure **across the flats**, not corner to corner, at three places per axis.
The pocket is 110.20 × 59.40, so anything up to 109.0 × 58.8 is fine.

## 6 · Keyboard corner radius

> **6.5 mm**, band 5.9 – 7.0. It derives the front aperture's corner, so getting
> it wrong either eats the opening or loses the retaining lip.

Easiest method without radius gauges: push the keyboard into an inside corner
(a steel square, or the corner of a table). **The distance along each edge from
the square's corner to where the keyboard's edge stops being straight IS the
radius.** Do it on two adjacent edges; they should agree.

## 7 · PCB outline, and confirm the stand base comes off

> **92.50 × 69.10 mm.** The circulating "70.1" is the *removable moulded stand
> base*, which overhangs the PCB by 1.00 mm on one long edge.

Measure the bare PCB with the stand base removed. Confirm the base and its
kickstand detach and that the four SMTSO-M2.5-7ET standoffs are soldered to the
**PCB**, not to the base — the whole thinness argument rests on that.

## 8 · Mounting pattern

> **85.50 × 62.10 mm.** Already established three ways and agreeing to
> 0.001 mm, so this is confirmation rather than discovery — but it is thirty
> seconds and everything else hangs off it.

Centre to centre, diagonally opposite holes, both diagonals.

## 9 · Side buttons and microphones — height below the PCB back plane

> Buttons **0.70 mm**, microphones **0.50 mm** below the back plane.
> Open item **O-04**: the factory drawing and the reference enclosure disagree
> by about 2.5 mm here, and the resolution rests on a chain of three inferences.

With the board back-face down on a flat surface, measure to the **centreline**
of each side switch and of each microphone port. If these are wrong the buttons
miss their apertures in Z.

## 10 · Which switch is which

> Open item **O-03**. Three side switches; the design does not currently know
> which is PWR, which is BOOT and which is KEY.

Power the board and try them, or read the silkscreen. It changes no geometry —
the three apertures are identical — but it belongs in the assembly guide.

---

## Reporting

A photo of the calipers per reading is fine; so is a list. What matters is
**which of the ten**, and whether you measured the moulding or the keycaps in
§1. If any of §1, §2, §3 or §4 disagrees with the value quoted above, say so
before you start a full print — those four change geometry.
