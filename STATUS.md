# STATUS — night shift, 2026-09-20

Firmware for steps 1–4 is written and on the board. **Two of the four
subsystems are verified on the hardware, one is blocked, and one could not be
tested because the keyboard never came on air.**

The single most important line in this file: **I could not see the screen.**
Nothing about what the panel actually displays is verified below unless you
told me so yourself.

---

## Flash it

```bash
. ~/esp/esp-idf/export.sh && cd firmware && idf.py -p /dev/cu.usbmodem2141101 flash monitor
```

Watch the log without reflashing:

```bash
. ~/esp/esp-idf/export.sh && cd firmware && idf.py -p /dev/cu.usbmodem2141101 monitor
```

Button-free, as required — the USB-Serial/JTAG enters download mode over the
USB control lines. Every flash tonight worked first time; BOOT was never
touched. Exit the monitor with `Ctrl-]`. On Linux the port is `/dev/ttyACM0`.

---

## Verified on the hardware

Each of these has log evidence from the board on your desk.

| What | Evidence |
|---|---|
| Builds, flashes button-free, boots reliably | ~10 flash/boot cycles, no failures |
| Display initialises, every SPI transaction returns OK | `st7305: init ok: 400x300 logical, 15000 B framebuffer, 24 MHz` |
| Framebuffer is in internal DMA-capable SRAM, not PSRAM | `framebuffer 15000 B at 0x3fcea8d0 (internal=1 dma=1)` |
| **Full frame: 4.75 ms** @ 24 MHz, 15,000 B | 20-run average, repeatable to ±6 µs across builds |
| **One 6×12 character costs exactly 9 bytes** | measured on the wire |
| **One 12×24 character costs exactly 36 bytes** | measured on the wire |
| Text grid initialises at 33 × 12 | `textgrid: font 12x24 x1 -> cell 12x24, grid 33x12` |
| **A document survives a chip reset** | `PASS 1/2: snapshot A survived a chip reset (140 bytes restored)` |
| **A torn write is rejected and the previous snapshot loads** | `PASS 2/2: the corrupted snapshot B was rejected and snapshot A loaded instead` |
| BLE host starts, scans, parses advertisements | ~48 distinct advertisers logged and decoded |
| Memory headroom with everything running | 212 KB internal free, 8.25 MB PSRAM free |

### The byte counts are the good news

9 bytes for a 6×12 cell and 36 for a 12×24 cell are exactly what the window
arithmetic predicts. Getting both right, at two different cell sizes, against
a real controller, **corroborates the CASET mirroring rule by a route
independent of the driver it was read from.** Had the mirroring been wrong, a
narrow window would not have landed on those numbers. That was the highest
technical risk in the handoff and it is now retired.

### The persistence test is real, and it is self-driving

`main/selftest.c` walks four stages across genuine chip resets using a counter
in NVS. Stage 1 writes a snapshot, **deliberately corrupts its payload**, and
stage 2 asserts that the *previous* snapshot comes back. That is the
torn-write case reproduced on purpose rather than hoped about — a check proven
to catch the failure, per the repository's own standard. It has already run to
completion and wiped itself; it will not run again.

---

## Written but NOT verified

Believe none of this until you have looked.

### Everything optical
I have no camera and no way to see the panel. The test card, the chunky
typeface, ink/paper polarity and contrast are all **unseen by me**. Your one
look at the first build reported the image was mirrored and 6×12 text was too
small; both are addressed below, and neither fix has been confirmed by eye.

### Orientation — most likely thing still wrong
You reported "it's all mirrored — everything reading reversed and backwards."
That is a horizontal mirror on the logical-x → native-y axis, so the default is
now `ORIENT_1` (`ny = 399 − x`). **This is a hypothesis, not a confirmation.**

You do not need me to fix it: **tap KEY (GPIO18) to cycle all four
orientations.** The choice is saved to NVS immediately, so once it reads right
it stays right through power cycles. The top-left cell says `TOP LEFT` and the
bottom-right says `BOTTOM RIGHT` — when those are in the corners they name and
read forwards, it is correct.

### The BLE keyboard — completely untested
**No keyboard ever advertised.** I logged 48 distinct BLE advertisers over
several scans; not one carried the HID service (0x1812) or a keyboard
appearance. Names seen were `EMPRESS`, `S18 …LE`, `Dime3_LE`, `N06SY` — all
neighbours' devices. The Rii 518BT was almost certainly powered off.

So the BLE stack is proven to *scan and decode*, and everything past that is
unexercised code: pairing, bonding, HID discovery, boot-protocol negotiation,
report decoding, key repeat, auto-reconnect, and the pairing-recovery gesture.

### The editor — untested, because nothing can type into it
Gap buffer, word wrap, cursor, backspace, scrolling and the status line are
written and compile, but with no key source none of it has run. The
persistence half beneath it *is* verified, just not through the keyboard.

### True power-cut survival
Verified: full chip reset, and deliberate record corruption. **Not** verified:
yanking power mid-flash-write. The journal is designed for it — sector-aligned
records, erase-before-write, CRC per record — but only a real power cut proves
a real power cut.

---

## Blocked

### The SD card will not initialise

```
sdmmc_init_ocr: send_op_cond (1) returned 0x107   (ESP_ERR_TIMEOUT)
```

Tried, all failing identically: 1-bit SDMMC on CLK 38 / CMD 21 / D0 39 (the
pin map from `solar_term.toml`, corroborated by the board manifest's own pin
table); internal pull-ups enabled; D1–D3 explicitly `GPIO_NUM_NC`; no
card-detect and no write-protect pin; probe clock (400 kHz) *and* default
(20 MHz). The card never answers `SEND_OP_COND`, which is the very first
command after reset — so this is the card or the electrical path, not FAT, not
the filesystem layer.

**This does not put your writing at risk.** Per the handoff's trap 6, the SD
card was only ever the export mirror; the flash journal is the source of truth
and it is the half that is verified. The deck runs fine with no card.

Worth trying next, in order: reseat the card; try a different card (some
cards are fussy about 1-bit mode); check whether the slot's power is gated by
a GPIO not listed in the manifest; and note that SolarOS reaches storage on
this board through a `storage_expansion` driver rather than a board-native
one, which hints the slot may not be a plain always-on SDMMC.

---

## What changed in the design, and why

**The grid is now 33 × 12, not 66 × 25.** `docs/HARDWARE.md` recommended 6×12
on the grounds that it is aligned on both axes and cheapest per character —
both still true, and both beside the point once you looked at it. The swap was
free because of an asymmetry worth remembering: **cell height must be a
multiple of 12 (the CASET quantum), but cell width only has to be even (the
RASET quantum is 2 px).** Width is cheap. That is now recorded in
`docs/HARDWARE.md` as a measured correction.

`tg_set_font(&tg_font_6x12, 1)` still gives you 66 × 25 whenever you want it,
and the call *refuses* a misaligned cell rather than letting a line's damage
window spill into its neighbours.

**There is a new 12 × 24 typeface**, purpose-drawn with 2 px stems, 16-row cap
height and real descenders — built for a low-contrast reflective panel. Both
faces are generated from reviewable ASCII art in `tools/`, never hand-edited
hex:

```bash
python3 tools/make_font.py --12x24 > firmware/components/textgrid/font12x24.c
```

Monospace is structural, not stylistic: every glyph occupies the same cell and
advances the same distance, so **ASCII art will line up exactly** — which
matters for the Orca and live-visualiser direction you described.

**On the RP2040/HDMI idea:** nothing here forecloses it, and the renderer is
deliberately shaped for it. Damage is tracked per cell and resolved to a
typed window push, so the same stream that drives the panel can be serialised
to another device later — which is what `docs/OS.md` means by a damage list
that "makes the deck remotable, screenshottable and testable in CI". I did not
build any transport tonight; I just did not design it out.

---

## Things I suspect

- **`st7305_read_id()` does not work** and is the one thing I wrote that I know
  is wrong. I configured a half-duplex 3-wire read and the SPI driver rejects
  it (`SPI half duplex mode is not supported when both MOSI and MISO phases
  are enabled`). It may not be fixable at all: the 23-pin FPC carries a single
  bidirectional data line, so reading needs MOSI turned around mid-transaction.
  It is harmless — nothing depends on it — but it logs a warning at boot.
- **Per-character latency is ~390 µs against 3.6 µs of wire time.** The cost of
  a keystroke is per-transaction overhead (GPIO toggles, command setup), not
  bandwidth. Nowhere near mattering yet, but that is where to look if it ever
  does.
- The `UP`/`DOWN` keys currently move by a whole line width rather than
  preserving the column. Honest placeholder, not a considered design.

---

## The one thing I would do next

**Turn the Rii 518BT on and watch the log.**

```bash
. ~/esp/esp-idf/export.sh && cd firmware && idf.py -p /dev/cu.usbmodem2141101 monitor
```

Every advertiser is logged with its name, address, appearance and UUID count,
and a matching one is tagged `<-- KEYBOARD`. That single observation decides
which of two very different mornings you are having: either the HID path works
and you have a writing device, or it does not and the log says exactly where it
stopped — scan, connect, pair, discover, or subscribe.

Everything downstream of that — the editor, wrapping, autosave-on-newline — is
written and waiting for a key source. The persistence underneath it is already
proven.

If it connects: type a paragraph, pull the power, plug it back in. That is the
whole acceptance test, and the only part of it still unproven is the keyboard.
