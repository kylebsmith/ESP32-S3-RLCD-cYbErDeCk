# The view node — the deck's picture on HDMI

*[NEXT.md](NEXT.md) §6. An Adafruit Feather RP2040 DVI draws the deck's picture on
any HDMI screen, from frames the deck sends. The sketch is `view/deckview`; this
page is its wire format and what is and is not yet true of it.*

---

## What it is

The deck's picture lanes draw a frame of cells — its own tiles, codepoints 128–155,
nine tones and the shapes — and the panel shows it in the split. With
`>send view on` the deck also sends each frame to the view node, which draws it on
any HDMI screen **in one of five modes**. The deck only names the mode; the node does
all the drawing, so no mode costs the deck anything, and every mode is worked out
from the frames and their ticks alone — the same performance draws the same
pictures.

**Every pixel is the deck's** (2026-09-29). The node draws each cell with the deck's
own 6×12 tile, bit for bit, and each of its pixels is a 2×2 block on the screen:
**53×20 cells fill it**, and that is what `>send view on` sends. Nothing is blended,
resampled or re-dithered. The owner, looking at the first modes: the node's smoothing
"blurred and mushed" the "chunky blocks, rigid ass lovely pixels". It was the node's
own doing — `plain` spread each grey across its neighbours before dithering it again,
and `feedback` zoomed and turned the frame — so the smoothing is gone, and phosphor
and feedback, which only worked by resampling, are retired. Their numbers stay taken
on the wire and draw plain.

The node draws 320×240 at eight bits a pixel through a palette, doubled to
**640×480 at 60 Hz**, the mode every HDMI screen takes. A screen that is not 640×480
scales it again itself, and most smooth when they do: if the blocks look soft on
the screen and sharp here, look in its menu for *1:1*, *just scan* or *integer
scaling*.

| mode | what the node draws |
|---|---|
| `plain` | the deck's cells, tile for tile |
| `scan` | each row of cells one line across the screen, stepped up by each cell's grey in whole pixels, hiding what is behind it — Rutt and Etra's scan processor on the deck's grid |
| `riso` | two plates out of register: this step, and the step before last, drifting with the bar in whole pixels |
| `poster` | a live Swiss poster: the piece's name, the section's number in red, tempo, bar and step, the section's name, the lanes in play with the step each is on lit red, and the picture cell for cell |
| `code` | the document itself, round the cursor, over the picture dimmed |

![The five modes and three colour settings, drawn by the node's own code](img/view-modes.png)

The sheet above is the node's code, not a mock-up: `tools/node_sim.cpp` builds
`view/deckview/deckview.ino` on a computer, feeds it the engine's frames packed as
the deck packs them, and writes what it draws; `tools/node_shots.py` makes the
pictures. `tools/view_demo.py` drives a real node through every mode with no deck.

**Colour is played, not picked.** Controllers 1–8 on **MIDI channel 16** colour the
screen, and the deck forwards their last values with every control frame. A
controller is a lane like any other, so colour takes patterns, routes, counts and
`>toggle`:

| controller | does | with nothing sent |
|---|---|---|
| `cc 1` | ink hue | warm white |
| `cc 2` | paper hue | — |
| `cc 3` | saturation: 0 is monochrome | monochrome |
| `cc 4` | day: 0 is light on black, 127 black on paper, and every grey between | light on black (riso: paper) |
| `cc 5` | invert, from 64 | off |
| `cc 6` | the glyphs' own hue — sparkles, letters, the small disc (riso: the second plate) | the ink's |
| `cc 7` | riso's drift, or scan's lift | 4 px, 30 px |
| `cc 8` | the deck's cell grid, drawn on the screen, this bright | off |

```
>day = cc 4 ch 16
>day 0011223344556677 /16      first light: black to paper over the section
>inv = cc 5 ch 16
>inv 90......9090....          the screen flips on the kick
```

The same controllers go to every MIDI output as well. **`>send view off` clears
them**, so each act of a set starts from the node's own colours.

**`plain` is the panel, now.** Every glyph and tile the node draws is the deck's own:
`view/deckview/deckfont.h` is generated from the same art as the panel's faces
(`tools/make_font.py --view`, diffed in CI).

## On the deck

| | |
|---|---|
| `>send view on` | frames go out at **53×20** cells, the node's screen one to one; if the view is already on, it keeps its size |
| `>send view scan` | on, drawn as scan lines — likewise `plain`, `riso`, `poster`, `code` |
| `>send view 80x30` | on, at any size up to 80×30; the node shows the middle 53×20 |
| `>send view` | which: `view is on, scan` |
| `>send view off` | stop; the panel's split decides the size again, and the colours clear |

A mode is one more argument to the destination, not a new word. Written into a
section of a piece, it changes with the piece.

**The deck's preview keeps its own shape.** When the view sets the size, the
engine draws at the view's size and the split shows a sample of it — the preview
stays what it was asked to stay, an approximation of the output. With the view off
nothing about the split changes.

## The wire format

Two kinds of frame, and the deck sends both every step: a control frame, then the
picture.

```
'D' 'K' 'V' '1'   magic: a picture
tick  u32 LE      the deck's pulse this frame was drawn for
w, h  u8, u8      the frame in cells
cells w*h bytes   row by row: 32–126 text, 128–155 the deck's tiles
sum   u8          XOR of every byte after the magic

'D' 'K' 'C' '2'   magic: a control frame ('DKC1' is the same without par)
tick  u32 LE      as above
mode  u8          0 plain, 1 scan, 4 riso, 5 poster, 6 code (2 and 3 retired: plain)
n     u8          lines of text, 0–12
len   u16 LE      bytes of text
par   8 x u8      the colour controllers, cc 1-8 on channel 16: 0-127, 255 unset
text  len bytes   n lines, each: from u8, to u8, its characters, '\n' -
                  [from, to) is the span to light, the step a lane is on
sum   u8          XOR of every byte after the magic
```

**The mode rides ahead of every picture**, so a node that joins late, or loses a
control frame, is right again a step later. The lines are sent only for the poster:
the piece's name, the section the cursor is in, the tempo and scale, and up to
seven lanes in play, each with the span the editor lights for its step — so the
poster and the panel never disagree about where the music is.

**Every frame carries its tick**, so a node that joins the ensemble can show a frame
when it was meant to be shown rather than when it arrived — the wireless node rides
the deck's clock instead of guessing. The modes already use it: feedback's zoom on
the beat, riso's drift with the bar, the poster's bar and step.

**One lost byte costs one frame.** The node keeps the bytes since a frame's magic,
and when a frame is refused it reads them again one byte later, finding the magic
the torn frame had swallowed. One reader serves both kinds. Packing is
`firmware/main/view_wire.h`, reading is `view/deckview/view_read.h`, and
`tools/test_view_wire.c` runs the one through the other in CI — torn frames of both
kinds, junk with a false magic in it, 20 KB of nothing but torn frames, the largest
picture, a corrupted control frame that must leave the mode as it was, and the two
headers' limits held equal.

**Room for both.** At 53×20 the picture goes out as 1,428 bytes of base64 (3,225 at
the largest, 80×30), and the poster's or the code's lines as at most about 620 with
the colours, through the console's 4,000-byte ring. The ring stays
under 4,096 bytes so it is kept in internal RAM, because the USB interrupt touches it
and PSRAM is not there while the flash is being written.

## How it gets there — today, and not yet

**Today a computer relays it.** The deck writes each frame to its console as a
terminal escape — `ESC ] view;<base64> BEL` — which a terminal swallows, and
`tools/viewrelay.py` lifts the frames out and writes them to the node's USB serial.
The frames are the deck's own; only the cable between the two is stood in for.

**Not yet: the deck driving the node directly over USB-C, on the deck's battery.**
The data half is possible: the ESP32-S3 can be a USB host, and the node already
reads the wire format from its USB serial. **The power half is not, on this
board.** Its USB-C port is wired as a power sink (the CC resistors,
[HARDWARE.md](HARDWARE.md) open question 3), and its charger, the ETA6098, is
charge-only by its published feature list; its sibling the ETA6095 is the one with
a boost. So the deck cannot send 5 V down the cable. Two ways round it:

1. **The node on its own 3.7 V LiPo**, in the Feather's battery socket, with the
   deck as USB host for the data. Simple. **Unverified:** whether the Feather's HDMI
   connector has its 5 V pin powered on battery alone. It probably takes it from USB
   only, and some screens will not see a source without it.
2. **A small 5 V boost inside the deck**, from its 18650, feeding the node's side of
   the cable, with the deck as USB host. One cable, any screen. A hardware change.

Either way the deck's USB-host firmware is not written yet. When it is, only the
transport in `firmware/main/view.c` changes: the bytes are already the wire format.

## The wire — deck to node with no computer `[PLAN]` 2026-09-29

The owner wants the deck to power the node and send it everything over a short
cable from the pins on the deck's back. **A UART is the link:** one data wire from
the deck to the node, plus ground and power. It is simpler than USB host, faster
than the relay, and deterministic, because a byte on a UART always takes the same
time and nothing else shares the wire.

**The bytes do not change.** The node already reads DKV1, DKC1 and DKC2 frames and
resynchronises on the magic after a torn frame (`view_read.h`, tested in CI). On a
raw wire the base64 and the terminal escape go, and the frames go out as binary.

| link | a picture (2,411 B) | worst case, picture + code lines (3,448 B) | verdict |
|---|---|---|---|
| UART 2 Mbaud | 12.1 ms | 17.2 ms | **start here** |
| UART 3 Mbaud | 8.0 ms | 11.5 ms | if 2 Mbaud measures clean |
| I2C 400 kHz (STEMMA QT as I2C) | ~60 ms | ~86 ms | too slow: two-thirds of a sixteenth at 165 bpm |
| I2C 1 MHz | ~24 ms | ~34 ms | slower than a UART, and a bus to arbitrate |
| today: USB to the computer, the relay, USB | several ms, jittered by two USB stacks and Python | | what the wire replaces |

The table is the largest frame, 80×30. At 53×20, what `>send view on` sends now, a
picture is 1,071 bytes: **5.4 ms at 2 Mbaud**. At 165 bpm a sixteenth is 91 ms, so
the wire is busy 6 % of the time, 13 % at the largest frame, 19 % with the code
view's lines. SPI would be faster again, but the node would have to be an SPI
target while its cores are busy making DVI; a UART lands in a hardware FIFO and a
DMA channel, which is enough.

**The deck's back header** is P1 on the vendor schematic (H4): 2 × 8 pins at
2.54 mm, lying horizontally on the back, with ESP32_SCL at the top right (the
owner, 2026-09-29). Seen that way:

```
  top    VBUS  GND  USB'_N  USB'_P  U0TXD  U0RXD  SDA     SCL        pins 2 4 6 ... 16
  bottom 3V3   GND  GPIO0   GPIO1   GPIO2  GPIO3  GPIO17  GPIO18     pins 1 3 5 ... 15
```

The orientation rests on one photo and one sentence: **check pin 1 is 3.3 V with
a meter before anything is plugged in.**

**Which pins, and why.** The view's TX is **GPIO18**. These are ruled out:

- GPIO0 and GPIO3 are strapping pins.
- U0TXD prints the boot ROM's text at every reset.
- SDA and SCL are the board's own I2C bus.
- GPIO17 is `>din 17`, the MIDI-on-a-wire output.
- GPIO1–3 are ADC1 channels, which `>battery` may be using for the battery sense.

The optional return line, from the node back to the deck, is GPIO2, and only if
the battery sense is not on it. VBUS (pin 2) carries 5 V only while the deck's
USB-C is plugged in, so on battery it is dead.

**Two ways to build the cable. Both are the same circuit.**

```
  DECK P1                              FEATHER RP2040 DVI
  -------                              ------------------
  pin 3   GND  ----------------------  GND
  pin 1   3V3  ----------------------  3V   (powers the node; see power below)
  pin 15  GPIO18, UART TX 2 Mbaud ---  RX   = GPIO1, UART0      (way A)
                                   or  SDA  = GPIO2, PIO UART   (way B, the QT port)
  pin 9   GPIO2, optional -----------  TX   = GPIO0             (node present / acks)
```

Three pins in the bottom row: 3V3 and GND at the left end, GPIO18 at the right
corner.

- **Way A, 0.1-inch ribbon.** Pins pushed into the deck's header, four wires to
  the Feather's GND, 3V, RX and TX. The Feather's hardware UART, nothing clever.
- **Way B, a STEMMA QT cable.** Adafruit sells a JST SH 4-pin cable ending in
  0.1-inch male pins: the pins go into the deck's header, and the keyed end clicks
  into the Feather's QT port. The QT port is wired for I2C (GPIO2/3), which the
  RP2040's hardware UART cannot use, but a **PIO UART** can: PicoDVI takes one of
  the two PIO blocks and the other is free. One keyed cable carries power,
  ground and a 2 Mbaud UART, with no soldering on the node. The owner's word
  for it: "insane". The cable's colours on P1: black to pin 3, red to pin 1,
  blue (SDA) to pin 15, and yellow (SCL) to pin 9 if the return line is used.

The ESP32-S3 can route a UART to any pin, which is why GPIO18 serves. Both
sides are 3.3 V logic: no level shifter.

**Never feed the Feather from the deck while its own USB is plugged into a
computer.** Two supplies would fight on its 3V rail. While testing on USB, wire
only GND and the data line.

**Power.** The node draws roughly 100 mA at 3.3 V running DVI (**unmeasured**; to
measure before it is wired). The deck's 3V3 into the Feather's 3V pin runs it. Two
things are still open:

1. **The deck's 3.3 V regulator headroom** with WiFi on, which peaks at a few
   hundred mA. From the schematic (H4 in [HARDWARE.md](HARDWARE.md)).
2. **HDMI's 5 V pin.** Fed 3.3 V, the Feather has no 5 V for the HDMI connector,
   and some screens will not see a source without it. The fix, when a screen
   refuses: a small 5 V boost from the deck's battery into the Feather's USB pin,
   which then feeds both the Feather's regulator and HDMI 5 V. The deck's own USB-C
   cannot supply it: its charger is charge-only ([HARDWARE.md](HARDWARE.md)
   question 3).

**Deterministic, and how far.** On the wire a frame's latency is its length over
the baud rate plus microseconds: fixed. The one jitter left is the node's
vsync, up to 16.7 ms, because a finished picture waits for the next 60 Hz
frame. The frames already carry their tick, so a later step can send each
picture a step early and have the node show it on its own tick, which removes
that too.

**What gets written**

- **Deck** (`view.c`): the packed frame goes to a UART as well as the console.
  It uses a driver ring that never blocks, and a frame that does not fit is
  dropped whole and counted, as on the console today. The pin is set once, e.g.
  `>view pin 17`, and kept in NVS.
- **Node** (`deckview.ino`): read `Serial1` (way A) or a PIO UART on GPIO2
  (way B) into the same reader as the USB serial. USB keeps working beside it.
- **Check:** `tools/test_view_wire.c` already proves the bytes. The new test is
  on the bench: frames a second, 0 refused, and the deck's `view` time in the
  heartbeat, before and after, through the relay and then on the wire.

**Next:** build the cable, meter pin 1, and then the firmware on both ends. The
UART, `>view pin 18`, and the node reading `Serial1` or GPIO2 are small. The
schematic stays the vendor's: this page names its pins and does not copy it.

## Measured, 2026-09-29 — every pixel the deck's

- **On this computer, the node's own code** (`tools/node_sim.cpp`): ORBITALS' night
  and orbit at 53×20, every mode, every frame packed as the deck packs it — 48 frames,
  **0 refused**, and the screens are the sheet at the top of this page. The colour
  settings on it were set as controllers would set them.
- Build: **91 KB of flash, 55 KB of RAM** before the framebuffers, down from 94 KB
  and 91 KB: the greys, their corners and the banded ink were the smoothing, and they
  are gone.
- `tools/test_view_wire.c` reads DKC2 with its colours, DKC1 with none, refuses a
  colour with a bit wrong, and does not compile against the reader before it.
- **In USB MIDI mode, measured live** while the owner played, 80×30 riso through the
  relay: **4.1 frames a second of 8.27, and each held the editor's loop 30.4 ms**
  (1.25 s of every 10); the loop fell to 166 turns a second. Stdio waited on the CDC's
  512-byte transmit buffer until the host had every byte. Fixed in the firmware, not
  yet on the deck: a frame is queued whole into a 4 KB buffer or dropped and counted,
  never waited for (`usbdev_console_write_whole`), and at 53×20 it is half the size.
  **Unmeasured until it is flashed.**
- **Flashed, and measured the same way, 53×20 in USB MIDI mode:** the view costs the
  editor 66 ms of every 10 s where it cost 3,459, and the loop runs 188 turns a second
  where it ran 119, 0 frames dropped.
- **The notes were never late**, before or after: at 165 bpm with 32nd hats, ratchets
  and a kick on the beat, the deck's USB MIDI measured at the host sat within 0.12 ms
  of the grid with the view on and 0.16 ms with it off on the old firmware (0.05 and
  0.07 ms rms), and within 0.11 ms on the new (0.03 ms rms). The clock and the MIDI
  path do not wait for the editor, so the view's stall slowed typing, not the music.
- **Unverified until it is on a screen:** how the blocks look through a given
  monitor's own scaler (see above), and the deck end to end with the colour lanes.

## Measured, 2026-09-28 — the glyphs

- The node with the glyph layer, fed ORBITALS' night from the deck's own engine at
  80×30 (the radar and `>noise 1`, about 237 sparkles a frame): 8.5 frames a
  second in every mode, **0 refused**. Build: 94 KB of flash, 88 KB of RAM before
  the framebuffers.
- **Found on the way, not changed:** `echo` counts any glyph as solid, so every
  sparkle fades into a grey block over seven steps. In the night that leaves 58 %
  of the frame grey smudge, on the panel as on the screen; with the sparkles left
  to twinkle it is 28 %, all of it the radar's own trail. The small disc must
  keep fading — on a small pane it is ORBITALS' planet, and its trail is the
  comet — so the change on offer is for the four sparkles only. The owner's call.

## Measured, 2026-09-28 — the six modes

- **The node alone**, fed the engine's own frames from the computer by
  `tools/view_demo.py`: **2,097 frames, 0 refused**, 8.3 a second against 124 bpm's
  8.27 sixteenths, stepping through all six modes and back; the poster received its
  nine lines.
- Build: 93 KB of flash and 80 KB of RAM, before the two 77 KB framebuffers the
  display takes when it starts. Uploaded with no button.
- The deck: builds and flashes; 88 KB of internal RAM left. The engine's scratch
  moved off the main task's 8 KB stack, since an 80×30 frame makes each copy 2.4 KB.
  **Unverified until it is run:** the deck end to end with the modes, and what
  the larger frame costs its editor loop.

## Measured, 2026-09-25

- First light: the deck's picture on HDMI through the relay, `53×20`, frames of
  1071 bytes, **one per sixteenth** — 86 frames in about 10.5 seconds at 124 bpm,
  8.2 a second against 8.27 sixteenths, and **0 refused** by the node.
- Build: 86 KB of flash, 58 KB of RAM with the reader's buffers, before the two
  38 KB framebuffers. Uploaded with no button, by the core's 1200-baud reset.

- **Sending a frame cost the deck's editor 31 ms**, measured by the loop profile in
  the heartbeat: 2.57 seconds of every 10 with the view on, and the editor loop fell
  from 199 turns a second to 142. Through stdio the console driver takes the frame a
  character at a time and blocks whenever its 1 KB ring is full, which a 1,436-byte
  frame always was. Now the frame goes to the driver in one call that never waits,
  into a 4,000-byte ring: **0.5 ms a frame**, 192 turns a second, 0 frames dropped
  in 234, and the node's refusal count did not move. If the ring has no room the frame is
  **dropped whole, never torn**, and the heartbeat counts it (`sent`, `dropped`).
  A frame can now land inside another task's log line; the relay keeps the text.
- **Unverified then:** the view in USB MIDI mode. Measured 2026-09-29: 30 ms a
  frame, half the frames lost - see the top of this list.

**Unmeasured, and the brief asks for it measured:** what the node costs the
deck's battery. It needs the direct link and a way to power the node from the deck
(above).

**Unverified:** anything about how the picture looks, beyond the owner's first
look above — there is no camera here. The node reports frames drawn and refused,
not pixels.
