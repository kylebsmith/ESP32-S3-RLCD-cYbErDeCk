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

Candidate cells, against a 400 × 300 field with 4 × 2 byte blocks:

| Cell | Grid | Fits exactly? | Byte-aligned? | Notes |
|------|------|---------------|---------------|-------|
| **8 × 12** | **50 × 25** | **yes, both axes** | **yes** (2 × 6 bytes) | recommended default |
| 8 × 16 | 50 × 18 | 12 px wasted vertically | yes | classic VGA font metrics |
| 12 × 16 | 33 × 18 | 4 px h, 12 px v wasted | yes | large/accessible mode |
| 5 × 10 | 80 × 30 | yes, both axes | **no** (5 ∤ 4) | classic 80×30; cells straddle bytes |

`8 × 12 → 50 × 25` divides both axes exactly, wastes no pixels, and each cell
is exactly 12 bytes. A per-cell dirty bitmap for the whole screen is
50 × 25 = 1250 bits = **157 bytes**, which is free. An 80-column mode remains
available at 5 × 10 at the cost of unaligned cell writes.

## Pin map `[CODE]` H6, corroborated by H7

### Display (SPI3_HOST)
| Signal | GPIO |
|--------|------|
| SCK | 11 |
| MOSI | 12 |
| CS | 40 |
| DC | 5 |
| RST | 41 |
| TE | 6 |

### I²C bus (I2C_NUM_0) — codec, RTC, temp/humidity
| Signal | GPIO |
|--------|------|
| SDA | 13 |
| SCL | 14 |

### Audio (I²S)
| Signal | GPIO |
|--------|------|
| MCLK | 16 |
| BCLK | 9 |
| WS | 45 |
| DIN (mics → ES7210) | 10 |
| DOUT (→ ES8311 → speaker) | 8 |
| PA enable | 46 |

### Buttons
| Button | GPIO | Source |
|--------|------|--------|
| BOOT | 0 | `[CODE]` H6 |
| KEY (user) | 18 | `[CODE]` H5 |
| PWR | — | hard-wired to PMIC; long-press off, click on `[VENDOR]` H1 |

### Battery sense
ADC1 channel 3, 12 dB attenuation, oneshot + calibration. `[CODE]` H5

## Peripherals

| Part | Function | Address / bus | Source |
|------|----------|---------------|--------|
| ES8311 | audio codec (playback) | I²C `0x18` | `[CODE]` H5 |
| ES7210 | ADC, echo cancellation (dual mic) | I²C `0x40` | `[CODE]` H5 |
| PCF85063 | RTC, with separate backup cell | I²C | `[VENDOR]` H1 |
| SHTC3 | temperature / humidity | I²C | `[VENDOR]` H1 |
| — | microSD, SDMMC 1-bit, FAT32 | dedicated | `[VENDOR]` H1 |
| — | 18650 holder + charge/discharge management | — | `[VENDOR]` H1 |
| — | 2 × 8 header, 2.54 mm pitch | — | `[VENDOR]` H1 |

Audio runs at 24 kHz in/out in the vendor's own voice application. `[CODE]` H6

**USB**: the ESP32-S3 has one USB-OTG peripheral. It can be a USB **device**
(so MIDI-over-USB to a host is available) or a host, but not both at once, and
it is the same port used for flashing and logs.

## USB, power role and expansion

| Item | Value | Source |
|------|-------|--------|
| USB-C CC1 / CC2 | **5.1 kΩ pulldown on each** → sink (device) role only | `[VENDOR]` H4 |
| Charger | ETA6098 switching charger; SW / PMID / BATS with L1 = 2.2 µH, 3 A | `[VENDOR]` H4 |
| USB-OTG ↔ USB-Serial/JTAG | share the **integrated transceiver by time-division multiplexing** when only the internal PHY is used | `[VENDOR]` H8 |
| Both at once | possible **only with an external PHY** — "USB OTG using one of the transceivers while USB Serial/JTAG using the other" | `[VENDOR]` H8 |
| USB Serial/JTAG class | **hardwired CDC-ACM + JTAG**, fixed function | `[VENDOR]` H8 |

### What this means for the design `[DERIVED]`

- **USB MIDI out works.** MIDI to a host needs USB-OTG in *device* mode, which
  is exactly what the port already is. No PHY conflict, no VBUS sourcing, no
  extra parts. The creative-coding goal is unobstructed.
- **USB Serial/JTAG can never carry MIDI** — it is fixed-function CDC-ACM. A
  MIDI device must come from USB-OTG, which means giving up the console on the
  internal PHY while MIDI is enumerated. That is a mode switch, not a blocker.
- **USB host for a wired keyboard is the expensive path**, and it is the one
  path this design does not need: it would contend for the same PHY *and*
  require sourcing 5 V that the port's CC resistors say the board does not
  offer. Since the keyboard is BLE HID, the conflict never arises.

### 2 × 8 expansion header (P1, 2.54 mm) `[VENDOR]` H4

Exposed nets: `VCC3V3`, `VBUS`, `GND` ×2, `GPIO0`, `GPIO1`, `GPIO2`, `GPIO3`,
`GPIO17`, `GPIO18`, `U0TXD`, `U0RXD`, `ESP32_SDA`, `ESP32_SCL`, `USB'_N`,
`USB'_P`.

`GPIO0` is shared with BOOT and `GPIO18` with KEY, so the genuinely
uncommitted lines are few. Both **I²C and UART0 are broken out**, which is the
practical expansion route — any future peripheral should prefer one of those
two buses over claiming raw GPIOs.

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

A window of *N* column addresses × *M* row addresses costs `N × 3 × M` bytes.
One 8 × 12 character cell spans at most 2 column addresses and exactly 6 row
addresses:

| Update | Bytes | Wire time @ 20 MHz |
|--------|-------|--------------------|
| Full frame | 15,000 | 6.0 ms |
| One 8 × 12 character cell | **36** | **14 µs** |

Roughly a **400× reduction** for the common case of typing a character. The
earlier conclusion stands and strengthens: full-frame pushes were already fast
enough for a comfortable editor, and windowed updates now make a keystroke
essentially free, which is what matters for battery life rather than for
latency.

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
