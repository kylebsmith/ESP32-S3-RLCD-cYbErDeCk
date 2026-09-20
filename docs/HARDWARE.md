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
   of display RAM, but the panel needs 400 × 300 = 120,000 bits, and the
   vendor driver allocates the full 15,000 bytes. H3 §7.4's stated address
   range (X = 19…40, Y = 0…159) also disagrees with the `CASET` values the
   vendor actually writes (XS = 0x12, XE = 0x2A). The datasheet is v0.2 and
   parts of §7.4 read as boilerplate from a smaller part. **The window →
   byte-address mapping must be established empirically before any
   partial-update path is trusted.** Full-frame pushes are unaffected and
   remain the safe default.
2. **Measured current** in HPM vs LPM vs sleep is not given in H3 and has not
   been measured on the bench. The power argument for LPM is currently
   qualitative.
3. **Schematic net names** have not been cross-read against the pin map above;
   the pin map rests on vendor example code, which is strong but secondary.
