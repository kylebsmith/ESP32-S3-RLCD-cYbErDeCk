# Satellites — the cable, the bus, and the first two nodes

*A brief to build from: four encoders on one node, eight buttons on another, joined
to the deck by six-pin magnetic cables and working without them. The owner builds
these; this page is what to buy, how to wire it, and what goes over the wire. Nothing
here is built yet.*

Tags: `[FACT]` exists on the deck and was measured · `[DECIDED]` settled, with the
reason · `[DERIVED]` computed here, not measured · `[VENDOR]` a seller's number ·
`[OPEN]` not decided, or not known.

---

## The one idea

**The cable carries the bus. The patch is a line of text.**

A modular synth's cable *is* the patch: it carries one signal from one jack to
another, and the patch is the tangle. Here a cable carries power, the clock and every
control on every node, and what a knob *does* is written in the document:

```
>knob1 = knob
>route cut knob1
```

So nodes can be plugged in any order, a patch is saved and reopened like any other
document, and changing what a knob does mid-song is editing a line, not re-cabling.
Plugging a node in still means something physical — it powers it, clocks it and pairs
it — but the meaning of the patch lives where every other meaning on this device lives.

## What already exists `[FACT]`

A satellite's controls are **input names** ([MAP.md](MAP.md) §9.8): `>knob1 = knob`
holds a value 0–127 on the deck, `>pad1 = pad` fires on a press, and `route` connects
either to anything. A knob acts on the next tick; a pad lands on the next step, swing
included, because a button that plays a note must land on the grid. The value lives on
the deck, so unplugging a node loses nothing. OSC already feeds these names: 22 ms from
a laptop on home Wi-Fi, 280 of 280 messages deck to deck ([NETWORK.md](NETWORK.md)).

**So a transport has exactly one job: turn what a node sends into
`seq_input_set(name, value, from)`.** Everything below is that transport.

## The shape `[DECIDED]` 2026-09-26

- **Connector:** six-pin magnetic pogo — the owner's choice.
- **Bus:** CAN at 500 kbit/s, daisy-chained from the deck: deck → node → node.
- **Unplugged:** the same frames over ESP-NOW, on the node's own battery.
- **USB-C on every node:** charging, flashing, and standing alone as a USB MIDI
  controller for a computer, no deck needed.
- **Both nodes are ESP32-S3.** It has the CAN controller (TWAI), USB OTG for MIDI, and
  exactly four hardware pulse counters — one per encoder. The ESP32-C3 has the CAN
  controller but no pulse counter and no USB OTG, so it cannot be the USB MIDI option
  (ESP-IDF 5.5 `soc_caps.h`: S3 `SOC_PCNT_UNITS_PER_GROUP 4`, `SOC_USB_OTG_SUPPORTED`;
  C3 neither).

---

## The connector

**Why magnetic pogo.** It breaks away instead of dragging the deck off the table or
ripping out a jack; it finds its own seat with one hand in the dark; nothing wears a
socket loose; and it is unmistakably *not* USB-C, so nobody can push a 20 V charger
into the bus.

**Buy against:** single row, 2.54 mm pitch, six contacts, gold-plated. Sellers rate
parts like these at 3 A per contact at 12 V and 10,000 matings `[VENDOR]` S1 — far
beyond what this needs. **Keyed:** hold one half the wrong way round against the other;
it must push away. If it does not, the pinout below still makes a reversed plug
harmless — but buy keyed ones.

**Gender:** flat pads on every device, spring pins on both ends of every cable. A cable
is then the same both ways round, and every port on a device is flush.

**The cable:** six conductors, with pins 3 and 4 as a **twisted pair** — that is what
CAN needs. Shielded 26 AWG six-core, or any cable with a twisted pair in it. The luxury
is in the finish, and the mechanical-keyboard world already solved it: braided
(paracord) sleeving, heat-shrink at the ends, coiled on a rod with a heat gun. That is
the look of the aviator cables that community pays a lot for, on a connector nobody
else is using.

## The pinout

| pin | name | what it carries |
|---|---|---|
| 1 | **VBUS** | 5 V from the upstream side — switched on only when DETECT sees a node |
| 2 | **GND** | |
| 3 | **CAN_H** | twisted with pin 4 |
| 4 | **CAN_L** | |
| 5 | **SYNC** | a pulse on every sixteenth, from the deck |
| 6 | **DETECT** | who is on the other end |

**Reversed, it does nothing at all.** Flipped, 1↔6, 2↔5, 3↔4: the deck's DETECT meets
the node's VBUS pad, which is unpowered, so the deck never sees a node and **never turns
power on**; CAN_H and CAN_L swap, which costs traffic and damages nothing; the deck's
SYNC output meets ground through its series resistor. An unkeyed plug put in upside
down is inert, not smoking.

### DETECT — power only goes onto the wire when a node is there

Upstream (the deck, or a node's DOWN port): **10 kΩ to 3.3 V**, read by an ADC.
Downstream (a node's UP port): **10 kΩ to ground**.

| the ADC reads | means | VBUS |
|---|---|---|
| about 1.65 V | a node | **on** |
| about 3.3 V | nothing plugged in | off |
| about 0 V | a short — a paperclip, a key on the magnet | off, and the deck says so |

This is what the sixth pin is for. **A magnetic port attracts metal**, and a live one
on a desk is a short waiting for a paperclip. USB-C's CC resistors solve the same
problem the same way.

### SYNC — the grid, on a wire

A 3.3 V pulse from the deck through **330 Ω** (so a short cannot hurt the pin): 5 ms
on every sixteenth, **15 ms on the first sixteenth of the bar**, so a node finds bar
one without asking. It marks the **grid**, not the groove — swing goes over CAN, where
a node that wants it can have it. **Nodes only ever listen.**

It is for: LEDs that land with the step, a future node with a 3.5 mm jack putting out
a eurorack clock, and timing a press against the grid rather than against a packet.

## The bus — CAN, 500 kbit/s

**Why CAN.** Both ends have the controller built in; each needs only a ~$2
transceiver (SN65HVD230, 3.3 V — a well-worn pairing with ESP32s: sources S2 and S3). It is
differential, so a cable lying across a power brick does not matter. It is multi-drop
with **arbitration**: no master, no polling — whoever has something to say says it,
and the lowest ID wins, which is how a button beats a knob. CRC and retransmission are
done in hardware.

**Why not the others.** I²C is not for cables or hot-plugging. RS-485 is differential
too but needs a master and a protocol, which CAN gives for free. USB would need a hub
and a host, and the deck's one port is already its MIDI and its console. MIDI DIN is
31.25 kbit/s and point to point. SPI is not for cables.

**Numbers** `[DERIVED]`: a frame with five data bytes is about 90–105 bits with
stuffing, so **about 0.2 ms** on the wire at 500 kbit/s. Four nodes sending every
5 ms is 800 frames a second, under a fifth of the bus.

**Topology:** a daisy chain. Every node has an **UP** port (towards the deck) and a
**DOWN** port, and passes GND, CAN_H, CAN_L and SYNC straight through, so order does
not matter. Not a star: CAN wants one line with short stubs.

**Termination:** 120 Ω across CAN_H–CAN_L at **each end** of the chain. The deck
always terminates — it is always one end. The last node terminates: on the first build
with a small switch marked **END**; later the node reads its own DOWN port's DETECT
and switches its terminator itself.

## Power

- **VBUS is 5 V nominal**, and a node must run from **3.6 to 5.5 V**. That leaves the
  deck free to feed its battery voltage straight through later.
- **On the deck:** a current-limited switch on each port (a 500 mA USB power switch —
  TI's TPS2051B is one), enabled only by DETECT.
- **On each node:** a ~250 mA polyfuse, a reverse-polarity FET, and an ideal-diode OR
  between VBUS, its own USB-C and its battery.
- **Budget** `[DERIVED]`: under 100 mA a node with the radio off — an idling S3, a
  transceiver, a few LEDs.
- **Whether the deck has 5 V on battery is not known.** [HARDWARE.md](HARDWARE.md)
  open question 3: the ETA6098 may or may not boost. Either a small 5 V boost
  converter on the deck side, or VBUS fed from the battery directly. `[OPEN]`

## Wired and wireless are one node

- **Plugged in:** CAN, and **the radio off**. That is a feature, not a saving: every
  node on a cable is one less transmitter contending with the keyboard and the
  ensemble. The keyboard's scan alone once cut OSC to 5 messages in 23 seconds, and
  280 of 280 arrived once it learned to share the radio ([NETWORK.md](NETWORK.md)).
- **Unplugged:** ESP-NOW, the same frames, on the node's battery.
- **Plugging in is pairing.** The first time a node is on the cable, the deck tells it
  its own address. From then on the node knows where to send when the cable is out.
  No pairing mode, no button held, nothing typed. A node that has never been plugged
  in joins the way a deck does ([NEXT.md](NEXT.md) §5).
- **USB-C:** charge, flash, and be a class-compliant USB MIDI controller for a
  computer on its own — the "USB as an option" the owner asked for.

| path | from press to the deck | tag |
|---|---|---|
| the cable | about 0.2 ms on the wire | `[DERIVED]` |
| ESP-NOW | one-way delay varied over 3.3 ms, on a bench | `[FACT]` NETWORK.md |
| OSC over home Wi-Fi | 22 ms median | `[FACT]` NETWORK.md |

All three are fine for a knob. A pad is quantised to the next step whichever way it
arrives, so the cable buys **consistency** more than speed.

---

## The frames

Standard 11-bit IDs, at most 8 data bytes. **ID = type (4 bits) · node (7 bits)** —
the type sits in the high bits, so it decides arbitration.

| type | ID | from | bytes | when |
|---|---|---|---|---|
| **CLOCK** | `0x000` | deck | step in bar · bar (2) · tempo ×10 (2) · running | every sixteenth |
| **PRESS** | `0x080` + node | node | index · velocity, 0 = release | a pad goes down or up |
| **TURN** | `0x100` + node | node | four signed detent counts · push bits | when moved, at most every 5 ms |
| **LEVEL** | `0x180` + node | node | index · value (14 bits) | a fader, a bend sensor |
| **LIGHT** | `0x280` + node | deck | one brightness per LED, up to 8 | feedback |
| **WELCOME** | `0x300` + node | deck | the deck's radio address (6) | an answer to HELLO |
| **HELLO** | `0x380` + node | node | kind · count · bank · version · id (4) | at power-up, then every 2 s |

**A node's number is its kind and its bank:** kind 1 knobs, 2 pads, 3 levels; bank
1–15. The first knob node is `0x11`, the first pad node `0x21`. Numbers are unique by
construction, which CAN requires — two senders of one ID corrupt each other's frames.

**The names a node feeds are its own setting, not the plug order.** Knob bank 1 feeds
`knob1`–`knob4`, bank 2 `knob5`–`knob8`; pad bank 1 feeds `pad1`–`pad8`. The bank is
set on the node — a small DIP switch, or over USB — and defaults to 1. Two nodes
claiming one bank: `>lanes` says so, and the second is ignored until one moves.

**An encoder sends how far it turned; the deck keeps the value.** TURN carries
*deltas*, so the deck adds them to the held value and clamps at 0 and 127 — the value
still lives on the deck, as [NEXT.md](NEXT.md) §5 asked. **Acceleration is the node's
job:** only the node knows how fast the hand moved, so a slow turn sends 1 a detent
and a flick sends up to 8, and the full range is under one turn when you want it and
127 careful detents when you do not.

**The document still defines the names** — a packet creates nothing, as with OSC
([MAP.md](MAP.md) §9.8). `[OPEN]` **Two nodes are twelve names, and that is most of
the room:** the deck holds 32 names and the boot document uses 16; it holds 16 inputs.
A third node will move one of those limits.

## The first two nodes

### Knobs — four encoders

- **EC11-type encoders with a push switch**, 20–24 detents, and knurled aluminium
  knobs on the 6 mm shaft. The feel is most of the instrument.
- **Counted in hardware:** the S3's four pulse counters decode four encoders with no
  interrupts, so a busy radio cannot eat a detent.
- **Pins:** 8 for the encoders, 4 for the pushes, 2 for CAN, 1 for SYNC, 1 for END or
  DOWN-port DETECT — **16**. Check a board's free GPIO count before buying it.
- What the push does is `[OPEN]`: fine and coarse, or a pad of its own.

### Pads — eight buttons

- **Mechanical keyboard switches** under keycaps, for the feel. The step after that is
  **hall-effect (magnetic) switches**: their travel is analogue, so how fast one goes
  down is a velocity, and how far is pressure — a pad that knows how hard it was hit.
- **Pins:** 8 switches, 1 LED chain under the keys (WS2812-type, one data pin), 2 CAN,
  1 SYNC, 1 END — **13**.
- A plain switch has no velocity: it sends 127, and a routed lane plays at its
  source's level ([MAP.md](MAP.md) §9.7), so the pad is simply full.

### Later — bend sensors, piano tabs

A flex sensor is a voltage divider into an ADC: a **LEVEL**, fourteen bits over the
wire, feeding a knob name. Nothing on the deck changes for it. What its names are
called is `[OPEN]`.

---

## What the deck needs `[OPEN]` — none of it built

**Firmware,** in the order it can be tested:

1. `node_frame.h` — pure packing and parsing of the frames above, checked on the host
   the way `osc_parse.h` is: a malformed frame is refused whole.
2. `seq_input_nudge(name, delta, from)` beside `seq_input_set()` — the one change the
   sequencer needs — with a host check that it clamps.
3. A TWAI task: frames to names, HELLO and WELCOME, error counters, and recovery from
   bus-off **without a reset** — ground rule: no hangs.
4. The SYNC pulse from the clock, and DETECT with the port's power switch.
5. Node frames over ESP-NOW, beside the ensemble's.
6. `>lanes` showing which node set each input, the way it shows an OSC sender's
   address today.

**The pins are the owner's to declare**, as they are for `>din 17`: a pin is a fact
about a physical object. The deck side needs five — TWAI TX, TWAI RX, SYNC, DETECT on
an ADC1 pin, and the port's power enable. **Taken, do not use:** 5, 11, 12, 40, 41 (the
panel), 6 (its tearing signal), 18 (KEY), 21, 38, 39 (the SD card), 19 and 20 (USB),
the strapping pins 0, 3, 45, 46, and 35–37 (the module's octal PSRAM); 26–32 are the
flash and are not brought out. The board's audio and battery-sense circuits take
others — read H6 ([HARDWARE.md](HARDWARE.md) sources) before choosing. **How that
declaration is spelled is the owner's to decide.**

**Hardware on the deck side:** an SN65HVD230, a 120 Ω terminator, one port, the power
switch, and perhaps a boost converter.

## How to know it works — measure, then claim

1. **Hot-plug a hundred times while playing:** no reset, no stuck note, `>lanes` right
   every time.
2. **A paperclip across every pair of an empty port:** VBUS stays off on a meter, and
   the deck reports a short.
3. **An unkeyed plug, reversed:** nothing powers, nothing warms.
4. **Press to MIDI out**, a hundred presses, wired and wireless, with Wi-Fi and the
   keyboard on: the deck's own timestamps against the press, timed from SYNC.
5. **An hour of play:** the TWAI error counters stay at zero; if bus-off ever happens,
   it recovers without a reset.
6. **Cables of 0.3, 1 and 2 m, a chain of three.**

## Shopping list — search terms, not links, because listings move

| what | search for |
|---|---|
| connector pairs | 6 pin magnetic pogo connector 2.54mm male female |
| CAN transceivers | SN65HVD230 CAN transceiver module |
| node boards | ESP32-S3 board USB-C LiPo charging (count the free GPIO) |
| encoders | EC11 rotary encoder with switch, 20 detent |
| knobs | knurled aluminium knob 6mm D shaft |
| buttons | mechanical keyboard switches, or hall effect magnetic switches |
| cable | 6 core shielded cable 26AWG with twisted pair |
| finish | paracord cable sleeving, heat shrink |
| protection | 120 ohm resistors, 250mA polyfuse, TPS2051B |

## Sources

| ID | Source | Retrieved |
|---|---|---|
| S1 | `promaxpogopin.com/magnetic-connector-6pin/` — 6-pin magnetic connector: 3 A per contact at 12 V, 10,000 matings, under 50 mΩ | 2026-09-26 |
| S2 | `github.com/okhsunrog/can_wizard` — a CAN sniffer on an ESP32-C3 with an SN65HVD230: the pairing, working | 2026-09-26 |
| S3 | `wiki.seeedstudio.com/xiao-can-bus-expansion/` — a CAN board for Seeed's ESP32 modules | 2026-09-26 |
| S4 | ESP-IDF 5.5 `components/soc/esp32s3/include/soc/soc_caps.h` and the C3's — pulse counters, TWAI, USB OTG | read locally |
