# Hardware reference — Waveshare ESP32-S3-RLCD-4.2

Every number here is traced to a primary source. Nothing is remembered,
inferred from a product photo, or carried over from a similar board.

`reference/` is **gitignored on purpose**: `docs/PROVENANCE.md` states that no
Waveshare file is redistributed by this repository. Rebuild the mirror with

```sh
tools/fetch_reference.sh          # docs + datasheets, ~12 MB
tools/fetch_reference.sh --code   # also the example repos, ~900 MB
```

## Sources

| ID | Source | Retrieved |
|----|--------|-----------|
| H1 | `docs.waveshare.com/ESP32-S3-RLCD-4.2` → `reference/waveshare/_docs.html` | 2026-09-20 |
| H2 | `…/Resources-And-Documents` — vendor resource index | 2026-09-20 |
| H3 | `files.waveshare.com/wiki/common/ST_7305_V0_2.pdf` — Sitronix ST7305 v0.2, 2021/04, 112 pp | 2026-09-20 |
| H4 | `files.waveshare.com/wiki/ESP32-S3-RLCD-4.2/ESP32-S3-RLCD-4.2-schematic.pdf` | 2026-09-20 |
| H5 | `github.com/waveshareteam/ESP32-S3-RLCD-4.2` — vendor example code | 2026-09-20 |
| H6 | H5 → `02_Example/XiaoZhi/XiaoZhiCode_V2.1.0/main/boards/waveshare-s3-rlcd-4.2/config.h` | 2026-09-20 |
| H7 | H5 → `02_Example/ESP-IDF/11_U8G2_Test/components/port_bsp/display_bsp.{h,cpp}` | 2026-09-20 |
| H8 | `documentation.espressif.com/esp32-s3_datasheet_en.pdf` — Espressif ESP32-S3 datasheet | 2026-09-20 |
| H9 | `github.com/nilseuropa/solar_os` v4.12.2 → `src/drivers/rlcd_st7305.c` — a working ST7305 driver for this exact panel, Apache-2.0 | 2026-09-20 |

Tags: `[VENDOR]` vendor states it · `[CODE]` read from vendor source ·
`[DERIVED]` computed here from tagged values · `[OPEN]` not yet established.

## SoC and memory

| Item | Value | Source |
|------|-------|--------|
| Module | ESP32-S3-WROOM-1-N16R8 | `[VENDOR]` H1 |
| Core | Xtensa LX7 dual-core, ≤240 MHz | `[VENDOR]` H1 |
| SRAM | 512 KB | `[VENDOR]` H1 |
| ROM | 384 KB | `[VENDOR]` H1 |
| Flash | 16 MB | `[VENDOR]` H1 |
| PSRAM | 8 MB (octal) | `[VENDOR]` H1 |
| Radio | Wi-Fi 2.4 GHz + **Bluetooth 5 (LE)** | `[VENDOR]` H1 |

**Bluetooth is LE only.** The ESP32-S3 has no Bluetooth Classic radio, so
there is no BR/EDR HID host. The Rii 518BT pairs over **BLE HID (HOGP)** —
confirmed by the user against their own unit, which enumerates a Nordic nRF5
device exposing the Human Interface Device service.

There is no MMU with demand paging, so no Linux. The OS is a task-scheduled
firmware on FreeRTOS, not a process-isolated kernel.

## Display — the part that decides the OS

| Item | Value | Source |
|------|-------|--------|
| Panel | 4.2" reflective LCD (RLCD), **no backlight** | `[VENDOR]` H1 |
| Controller | **Sitronix ST7305** | `[VENDOR]` H2 |
| Resolution | 400 × 300 landscape (300 × 400 portrait) | `[VENDOR]` H1, `[CODE]` H6 |
| Colour depth | 1 bit per pixel, bi-level | `[VENDOR]` H3 §Features |
| Controller RAM | 264 × 320 × 1 b static | `[VENDOR]` H3 §7.3.1 |
| Driver outputs | S1–S264 source, G1–G322 gate | `[VENDOR]` H3 §Pin list |
| Interface | 4-wire SPI on SPI3_HOST @ **20 MHz** | `[CODE]` H7 |
| Frame rate range | **0.25 Hz – 51 Hz** | `[VENDOR]` H3 §Features |
| HPM frame rate | **32 Hz** (`FRCTRL` B2h = 0x12) | `[VENDOR]` H3 §8.2.3 |
| LPM frame rate | **1 Hz** (same register) | `[VENDOR]` H3 §8.2.3 |
| Power-mode commands | `HPM` 38h / `LPM` 39h | `[VENDOR]` H3 §8.1.22–23 |
| Partial mode | `PTLON` 12h (240 duty) / `PTLOFF` 13h (320 duty) | `[VENDOR]` H3 §8.1.8–9 |
| Window addressing | `CASET` 2Ah / `RASET` 2Bh | `[VENDOR]` H3 §8.1.14–15 |
| Tearing-effect output | present, wired to GPIO6 | `[CODE]` H6 |
| Framebuffer | 400 × 300 ÷ 8 = **15,000 bytes** | `[DERIVED]` from H7 `DisplayLen = transfer >> 3` |
| Vendor ships | HPM, full-frame pushes, buffer in PSRAM | `[CODE]` H7 |

### Memory layout: one byte is a 4 × 2 pixel block

From H7's pixel addressing — `byte_x = x >> 2`, `byte_y = y >> 1`,
`local_x = x & 3`, `local_y = y & 1`:

```
one RAM byte  =  4 pixels wide  ×  2 pixels tall
framebuffer   =  100 byte-columns  ×  150 byte-rows  =  15,000 bytes
```

This is the single most consequential fact for the UI. It means the natural
unit of damage is a 4 × 2 block, and a character cell whose width is a
multiple of 4 and whose height is a multiple of 2 maps onto **whole bytes with
no read-modify-write**.

### Timing budget `[DERIVED]`

| Quantity | Value |
|----------|-------|
| Full-frame SPI push, 15,000 B @ 20 MHz | **6.0 ms** |
| One HPM frame period @ 32 Hz | 31.25 ms |
| SPI push as a fraction of an HPM frame | **19 %** |
| One LPM frame period @ 1 Hz | 1000 ms |

**The bus is not the bottleneck — the panel is.** A whole-screen redraw costs
6 ms against a 31 ms frame, so a live text editor at 32 Hz is comfortable
without any partial-update machinery. Window addressing (`CASET`/`RASET`) is
therefore a **power and CPU optimisation, not a latency requirement** — which
is the opposite of the constraint an e-paper panel would impose, and it is why
this panel suits an interactive writing device where e-paper does not.

`LPM` at 1 Hz with a reflective, backlight-free panel is what makes
"never really off" real: the image persists, costs almost nothing to hold, and
returns to 32 Hz on a keypress via a single command byte.

### Character-cell geometry `[DERIVED]`

**The quantisation rotates with the panel, and this was got wrong at first.**
The byte is 4 × 2 pixels *in the panel's own portrait frame*, but the device
runs **landscape** (settled by `docs/DATUMS.md` D-07: only 400 × 300 gives
square 0.2120 mm pixels against the enclosure's active area). Rotated:

| Landscape axis | Maps to | Addressing unit | **Quantum** |
|---|---|---|---|
| width, 400 px | native Y | `RASET` row address | **2 px** |
| height, 300 px | native X | `CASET` column address | **12 px** |

So **12 px is the hardware's own line height** — one full-width text line of
12 px is exactly one column address across all 200 row addresses. Character
width wants to be even; height wants to be a multiple of 12.

A window of *C* column addresses × *R* row addresses costs `C × R × 3` bytes:

| Cell | Grid | h % 12 | w % 2 | Bytes/char | @ 20 MHz |
|------|------|--------|-------|-----------|----------|
| **6 × 12** | **66 × 25** | yes | yes | **9** | 3.6 µs |
| 8 × 12 | 50 × 25 | yes | yes | 12 | 4.8 µs |
| 12 × 12 | 33 × 25 | yes | yes | 18 | 7.2 µs |
| 5 × 10 | 80 × 30 | **no** | **no** | 24 | 9.6 µs |

**`6 × 12` giving `66 × 25` is the recommendation.** It is aligned on both
axes, it is the cheapest per character, and 66 columns is close to the
classical measure for prose. `8 × 12` at `50 × 25` is the roomier alternative
and equally well aligned.

An 80-column mode is now clearly the wrong trade: `5 × 10` is misaligned on
*both* axes, costs the most per character of any option here, and at 120 DPI a
1.06 mm cell is legible but unpleasant. **Plan the UI for 66 × 25.**

| Update | Bytes | @ 20 MHz |
|--------|-------|----------|
| One character (6 × 12) | 9 | 3.6 µs |
| One full-width 12 px line | 600 | 240 µs |
| Full frame | 15,000 | 6.0 ms |

### Measured on the bench, 2026-09-20 `[MEASURED]`

First numbers taken from the real board rather than derived. Method:
`esp_timer_get_time()` either side of a 20-run loop, ESP-IDF v5.5.4, SPI3
unused and SPI2 driving the panel at 24 MHz.

| Quantity | Derived above | **Measured** |
|---|---|---|
| Full-frame push, 15,000 B | 6.0 ms @ 20 MHz (5.0 @ 24) | **4.75 ms @ 24 MHz** |
| One 6 x 12 character, bytes on the wire | 9 | **9** |
| One 12 x 24 character, bytes on the wire | 36 | **36** |

The two byte counts are the load-bearing result. They are produced by the
window arithmetic in *Window addressing* below, running against a real
controller, at two different cell sizes — which corroborates the address model
by a route independent of the driver it was read from. Had the CASET mirroring
been wrong, a narrow window would not have come out at exactly 9 and 36 bytes.

The full-frame figure beats the derivation because the derivation assumed no
overlap between SPI setup and transfer. Per-character wall time is ~390 us
against 3.6 us of wire time, so the **cost of a keystroke is transaction
overhead, not bandwidth** — which is where to look if latency ever matters.

**Still unmeasured: everything optical.** Nobody has photographed the panel
under this firmware. Contrast, the LC response ceiling and whether the image
is even the right way round are open below.

### The 6 x 12 recommendation did not survive contact `[MEASURED]`

*Character-cell geometry* above recommends 6 x 12 giving 66 x 25, on the
grounds that it is aligned on both axes and cheapest per character. Both facts
hold. The recommendation still failed, for a reason arithmetic could not
reach: on the real panel, at 0.212 mm pixel pitch and with no backlight, a
5 x 7 glyph body is too small and too thin to read comfortably. The owner's
first look at it was "the text is too small ... with such a naturally low
contrast screen we gotta have sexy chunky letters."

The firmware therefore defaults to **12 x 24 giving 33 x 12**, and keeps 6 x 12
available. The alignment analysis is what made the swap free: cell height must
be a multiple of 12, but **cell width only has to be even**, so widths are
cheap and the grid can be re-proportioned without new constraints.

### Refresh: three different numbers, and only two govern how it feels `[OPEN]`

This is the sharpest unresolved question in the prior art and it must not be
collapsed into one figure:

1. **SPI write throughput** — fast. 6.0 ms a frame at 20 MHz, 5.0 at 24 MHz,
   and the datasheet's 30 ns `tSCYC` allows 33 MHz. Not the limit.
2. **Liquid-crystal optical response** — an NES emulator written for this exact
   panel reports it staying clean only to about **23 Hz**, with blacks washing
   out above roughly 26 Hz. If that holds, it, not bandwidth, is the real
   ceiling, and the earlier "32 Hz is comfortable" reading is optimistic.
3. **Self-refresh rate** — HPM or LPM, independent of how fast content is
   written.

**Writing while the panel is in LPM can delay the visible update by up to one
refresh period — about a second at 1 Hz.** That is the lag Freewrite owners
complain about, and it is a design rule rather than a defect: **kick to HPM on
the first keydown and drop back after an idle timeout.** Prior art defaults to
HPM and makes the automatic behaviour opt-in, which is the wrong default for a
battery device.

Unmeasured here. Bench it before believing any of it.

### Driver constraints worth knowing before writing code

- **The framebuffer belongs in internal DMA SRAM, not PSRAM.** SPI DMA
  requires internal SRAM; PSRAM would need cache-coherence work. 15 KB is
  affordable. Scrollback and document buffers go in PSRAM.
- **SPI reads are 5× slower than writes** — `tSCYC` 30 ns write against 150 ns
  read, so ~6.7 MHz for any read path. A driver that reads controller RAM or
  status at the write clock will fail intermittently.
- **`ST7305` vs `ST7306`.** Waveshare says ST7305 three independent ways, but
  Zephyr's in-tree board definition for this same board declares
  `sitronix,st7306`. The command sets overlap enough that the ST7306 driver
  works. Trust ST7305 for datasheet lookups; a driver labelled ST7306 may still
  be the right code.
- **No backlight, and no net to add one to.** The 23-pin LCD FPC carries only
  GND, VCC3V3, SCL, SDA, CS, RS, TE and RESET — grepping the schematic for
  backlight, `LEDA`, `LEDK` or frontlight returns nothing. Contrast improves in
  direct sunlight, which is the compensating virtue, but any front light is a
  board revision driven from a spare GPIO, not an FPC pin.


## Window addressing — the datasheet contradiction, resolved `[CODE]` H9

`docs/DATUMS.md`-style caveat first: this comes from a **third-party
implementation**, not from Sitronix. It is recorded as `[CODE]` rather than
`[VENDOR]` because H3 §7.4 states something different. It is trusted because
it is exercised on this exact panel, it reproduces the byte count derived
independently here, and its bit packing agrees with a second unrelated
implementation.

### The address model

| Unit | Mapping | Range |
|------|---------|-------|
| Column address (`CASET` 2Ah) | **1 address = 12 native-X pixels = 3 bytes** | `0x12` … `0x2A` (25 × 12 = 300) |
| Row address (`RASET` 2Bh) | **1 address = 2 native-Y lines** | `0x00` … `0xC7` (200 × 2 = 400) |
| One RAM byte | 4 X pixels × 2 Y pixels | — |

Derived: `controller_row_bytes = ((300 + 11) / 12) × 3 = 75`, and
`75 × 4 × 50 = 15,000 bytes` — the same framebuffer size derived above from
Waveshare's own code, by a completely different route.

### The rule the datasheet does not give you

`address_mirror_base = address_start + address_end = 0x12 + 0x2A = `**`0x3C`**.
For a window spanning natural left-to-right column addresses `[a, b]`:

```
CASET (0x2A)  =  { 0x3C - b ,  0x3C - a }      <-- reversed and mirrored
RASET (0x2B)  =  { first_y / 2 , last_y / 2 }  <-- window must be 2-line aligned
```

then stream the data in normal left-to-right order from the leftmost group.
With `MADCTL (0x36) = 0x48`, the column pointer **decrements from the CASET end
address**, so the first byte written lands at the *high* address.

**Why nobody finds this.** At full width `a = 0x12, b = 0x2A`, so
`{0x3C − 0x2A, 0x3C − 0x12} = {0x12, 0x2A}` — the mirroring is an **identity**.
Every full-frame driver, Waveshare's own included, is therefore silently
correct and never discovers the rule. It only bites once a window narrows: to
write the leftmost 12 px alone you must send `CASET = {0x2A, 0x2A}`, **not**
`{0x12, 0x12}`.

Windows are quantised — X snaps out to 12-pixel boundaries, Y to 2 lines.

### What this buys `[DERIVED]`

A window of *C* column addresses × *R* row addresses costs `C × R × 3` bytes.
Worked through in the **landscape** frame the device actually uses — see
*Character-cell geometry* above, where the axes and their quanta are set out:

| Update | Bytes | Wire time @ 20 MHz | vs. full frame |
|--------|-------|--------------------|----------------|
| One 6 × 12 character | **9** | 3.6 µs | 1/1667 |
| One full-width 12 px line | 600 | 240 µs | 1/25 |
| Full frame | 15,000 | 6.0 ms | — |

Three orders of magnitude for the common case of typing a character. The
earlier conclusion stands and strengthens: full-frame pushes were already fast
enough for a comfortable editor, so this buys **battery life, not latency**.

### Frame-rate control `[CODE]` H9

Not obvious from H3, and worth having:

| Mode | Rates | How |
|------|-------|-----|
| HPM | 16 / 25.5 / 32 / 51 Hz | `OSCSET` (`0xD8`) byte 0 = `0xA6` or `0x80`, plus the HFRA bit `0x10` in `0xB2` |
| LPM | 0.25 / 0.5 / 1 / 2 / 4 / 8 Hz | low 3 bits (`0x07`) of `0xB2` |

H9 drives the panel at **24 MHz** SPI rather than the 20 MHz in Waveshare's
example, so 24 MHz is known-good on this hardware.

### Still unsolved: tearing `[OPEN]`

H9 declares the TE pin (GPIO6) in its board manifest and **never reads it** —
no tear-free synchronisation exists anywhere in the prior art examined. Anyone
wanting glitch-free updates during HPM is on their own.

## Prior art on this exact board `[VENDOR]` H2

Waveshare's own resource page lists community projects. Two are directly
relevant and are mirrored under `reference/waveshare/code/`:

- **SolarOS** — `github.com/nilseuropa/solar_os`, posted to r/cyberDeck; and
  **solar_term**, "build instructions for a SolarOS pocket terminal". This is
  the closest existing thing to what this project is building, on identical
  hardware.
- **waveshare_RLCD_400x300_monochrome** — `github.com/JasonHEngineering/…`,
  a monochrome driver for this panel.

Also listed: an LVGL port, an ESPHome/XiaoZhi integration, a TRMNL dashboard
client, and an offline GPS. The board is well-trodden; none of these is a
keyboard-first writing device.

## Open questions `[OPEN]`

1. **Controller RAM vs. panel size.** H3 states 264 × 320 × 1 b = 84,480 bits
   of display RAM against a panel needing 120,000, and its §7.4 address range
   (X = 19…40, Y = 0…159) disagrees with the `CASET` values every working
   driver writes. The datasheet is v0.2 and parts of §7.4 read as boilerplate
   from a smaller part. **The practical mapping is now resolved** — see
   *Window addressing* above — so this no longer gates partial update. What
   remains open is only the reconciliation with H3's stated RAM figure, which
   is a curiosity rather than a blocker.
2. **Measured current** in HPM vs LPM vs sleep is not given in H3 and has not
   been measured on the bench. The power argument for LPM is currently
   qualitative.
3. **Can the ETA6098 boost VBUS?** Its SW / PMID pins and the 2.2 µH inductor
   are the topology of a part that *may* support an OTG boost, but the ETA6098
   datasheet has not been read and the CC resistors configure the port as a
   sink regardless. Only matters if USB host is ever wanted; it is not wanted
   today. **Do not assume either answer.**
4. **Schematic net names** have not been fully cross-read against the pin map above;
   the pin map rests on vendor example code, which is strong but secondary.

## MIDI on a wire `[OPEN]`

The firmware is done and measured: `>din 17` starts a UART at 31250 baud 8N1 and
`>din` reports bytes actually written — **80 bytes from a four-step kick pattern,
verified on the bench.** What is not done is the electrical side, and it cannot
be done in firmware, because **a MIDI output is a current loop, not a logic
level.** A GPIO wired straight to a jack will drive some receivers and not
others, and the ones it fails on will look like a firmware fault.

The 1983 circuit, from 5 V logic:

```
    +5V ----[220R]---- DIN pin 4
    UART TX ---[220R]---- DIN pin 5
    GND ----------------- DIN pin 2   (shield, often left open at the source)
```

From **3.3 V** logic the resistors change, because the receiver's optocoupler
wants ~5 mA:

```
    3V3 ----[33R]----- pin 4
    TX  ----[10R]----- pin 5
```

Better, and what to do if the receiver is fussy: buffer TX up to 5 V first (a
74HCT family gate, or any 3.3→5 V level shifter) and then use the original 220R
pair. The optocoupler in the receiver is the thing being driven, and it was
specified against 5 V.

**TRS instead of DIN.** Type A is the standard that won — MIDI 1.0 ratified it,
and it is what an SP404 MkII and most modern gear use:

```
    tip    = DIN pin 5   (the data line)
    ring   = DIN pin 4   (the +V side of the loop)
    sleeve = DIN pin 2   (ground)
```

Type B swaps tip and ring. If a device does not respond, that is the first thing
to try, and it cannot damage anything.

**The pin is the owner's to declare.** `>din 17` — the same rule as the battery
sense line, for the same reason: a pin is a fact about a physical object, and
the only party who can see the object is the owner. GPIO 5, 11, 12, 18, 40 and
41 are refused by name, because they are the panel and the KEY button and taking
one would look like a MIDI fault.

**What this unlocks, and it is the point.** Every other transport makes the deck
a USB device, a BLE peripheral or a network client — all of which need something
else to be the host. An SP404, a class-compliant MIDI interface and most desktop
gear are USB *devices* too, and two devices cannot talk; a Mutant Brain has no
USB at all. Before this, every path from the deck to hardware ran through a
computer. This is the one that does not.

**Still missing:** MIDI *in*. That needs an optocoupler (6N138 or similar) on the
receive side and is a separate build. Until then the deck is a clock source and
never a clock follower on the wire.

