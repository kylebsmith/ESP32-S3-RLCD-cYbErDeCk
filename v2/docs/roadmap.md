# Avenues, recorded — not this iteration

Concepts raised for later, written down so they are not lost and so the
current enclosure does not accidentally foreclose them. **None of this is being
designed now.** The user's own framing: *"Take these concepts down as avenue for
next steps… I dont know if we are ready… One iteration at a time though."*

Each entry says what it would cost the enclosure, because that is the part this
repo can answer, and because a decision taken now in ignorance of these is the
expensive kind.

---

## 1. A three-MCU split

> *"we might consider adding a Teensy 4.1 that might take over the role of the
> actual control engine so we can increase the quality of our clocking and our
> current ESP32 acts as more of the operating system and orchestrator while our
> other boards take care of the multi-modal output."*

| board | role |
|---|---|
| ESP32-S3 (have) | the OS and orchestrator — editor, display, radio, storage |
| RP2040 Feather w/ video (this iteration) | video output |
| Teensy 4.1 (proposed) | the control engine: clock, sequencer timing, MIDI out |

The argument for it is real and it is about **jitter, not throughput**. The
ESP32-S3 runs FreeRTOS with Wi-Fi and BLE stacks that take interrupts at times
the sequencer does not control, and `docs/HARDWARE.md` already records that the
radio is doing real work. A dedicated MCU whose only job is to emit clock has
no such competition. Whether the current jitter is actually audible is a
**measurement nobody has taken**, and it is the thing to do before buying
anything: scope the existing MIDI out against a reference clock and look at the
distribution, not the mean.

**What it costs the enclosure**

- A third board on the carrier, Teensy 4.1 being 61.0 × 17.8 mm nominal
  (unverified here) — small, but it needs its own area and its own run.
- A third wiring colour in a plate whose wiring is a visible surface. The
  current carrier has two bus families (I²C chain, analogue trunk) and they
  read because there are two. A third is still legible; a fourth probably is
  not, and that is a composition limit as much as an electrical one.
- An inter-MCU link, which is more pins off a 2×8 header that `wiring.md`
  already has 9 of 16 spoken for.

**What to decide first:** measure the jitter. If it is inaudible the Teensy is
a board, a bus and a failure mode bought for nothing.

---

## 2. A sonification engine, and a second display

> *"down the road could allow us to expand one more time additionally to add
> another board integrated that is the sonification engine (potentially another
> full display with all available settings to the user) so the device is
> ridiculously standalone"*

This is the one that changes the object rather than the board count. A second
full display is not a component, it is a **second face**, and the current slab
has one 15 u control field above one keyboard. A second display either:

- takes the control band's place, and the controls move to a third band —
  which the handheld studies already costed at roughly +39 mm of depth per
  band; or
- goes on a **second plane** — a hinged or angled upper section, which is a
  different object and a different enclosure, not a revision of this one.

Worth saying plainly: the deck's width is already fixed at 294.75 mm by the
keyboard, and the current slab is 185.45 deep. There is room to grow downward
in depth and essentially none sideways.

**What to decide first:** whether the second display is a *settings* surface
(which the main display could show as a mode, for free) or a genuinely
simultaneous second view (which is the only thing that justifies the hardware).

---

## 3. The 8000 mAh pack

> *"I have a thick 8000 MaH battery to throw in there alongside the battery our
> dev board already has so this thing can be a tank and stay wireless for an
> extended session."*

The relevant word is **thick**. Everything else in this enclosure is a
thickness argument, and a large pouch cell is the first component that would
set the deck's depth rather than fit inside it.

**What it costs the enclosure**

- The slab is currently a shallow tray: a 12.70 acrylic top, a carrier plate
  and a back panel. A pack of that capacity is typically 8–12 mm thick on its
  own, which may be free under the keyboard bay, where there is already a void,
  and is definitely not free under the control band, where the breakouts and
  their wiring live.
- **Under the keyboard is the obvious home** and should be checked first: the
  bay is 288.35 × 98.45 and the keyboard's own case is only 7.60–13.25 mm tall
  with a wedged bottom, so what is under it is mostly air.
- Two cells in one object is a charging question, not a space question. They
  cannot simply be paralleled unless they are matched and share a BMS, and the
  Waveshare's own charger (ETA6098, input current limit still open in
  `docs/HARDWARE.md`) was not designed to manage a second pack.

**What to decide first:** the pack's real dimensions, and whether it charges
independently or through the deck. If independently, it is a power bank with a
nice home and the electrical question disappears.

---

## 4. Edge-lit display illumination

> *"we might even consider putting a small LED strip in the edge to somehow
> light up the display… diffused sexy little edge lighting… but dont worry
> about that part too much as of right now."*

Recorded, not designed. The display is **reflective with no backlight** — that
is the whole reason the deck can run at 1 Hz off a small cell
(`docs/HARDWARE.md`), and it is also why it is unreadable in the dark. Front
lighting is the correct answer for a reflective panel, and a 12.70 mm clear
slab sitting directly over it is, in principle, a light guide already.

The one thing that decides it: whether light can be coupled into the slab's
edge and turned down onto the display **without** the slab also glaring into
the player's eyes. That is a question about the extraction features — the
texture or print that kicks light out of total internal reflection — and it is
a real optical design problem, not a strip of LEDs in a groove.

---

## What this iteration must not foreclose

1. **Keep spare pins.** `wiring.md` uses 9 of the 2×8's 16. Do not spend the
   rest casually; an inter-MCU link needs them.
2. **Keep the carrier plate a separate part.** Every expansion above adds
   boards to it. A carrier that can be reprinted without re-cutting the acrylic
   or the back panel is the thing that makes a second iteration cheap.
3. **Leave the volume under the keyboard bay alone.** It is the only large void
   in the object and both the pack and a third board will want it.
