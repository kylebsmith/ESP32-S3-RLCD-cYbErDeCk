# STATUS — night shift, 2026-09-20

Firmware for steps 1–4 is written, on the board, and the acceptance test
passes **except for the one part that needs the BLE keyboard** — which never
came on air, so it could not be tested at all.

The single most important line in this file: **I could not see the screen.**
Nothing about what the panel displays is verified below unless you told me so
yourself.

There is a short note already typed into the deck, waiting for you.

---

## Flash it

```bash
. ~/esp/esp-idf/export.sh && cd firmware && idf.py -p /dev/cu.usbmodem2141101 flash monitor
```

Watch the log without reflashing:

```bash
. ~/esp/esp-idf/export.sh && cd firmware && idf.py -p /dev/cu.usbmodem2141101 monitor
```

Button-free, as required. Every flash tonight worked first time and BOOT was
never touched. Exit the monitor with `Ctrl-]`. On Linux the port is
`/dev/ttyACM0`. If it ever fails to enter the bootloader, add
`--before default_reset` to a direct `esptool` call.

**The monitor is also a keyboard.** Type into it and the characters go into
the document. That is how everything below was tested.

---

## Verified on the hardware

Every row has log evidence from the board on your desk.

| What | Evidence |
|---|---|
| Builds, flashes button-free, boots reliably | ~15 flash/boot cycles, no failures |
| Display initialises, every SPI transaction returns OK | `init ok: 400x300 logical, 15000 B framebuffer, 24 MHz` |
| Framebuffer is in internal DMA-capable SRAM, not PSRAM | `framebuffer 15000 B at 0x3fcea8d0 (internal=1 dma=1)` |
| **Full frame: 4.75 ms** @ 24 MHz | 20-run average, repeatable to ±6 µs across every build |
| **One 6×12 character costs exactly 9 bytes** | measured on the wire |
| **One 12×24 character costs exactly 36 bytes** | measured on the wire |
| Text grid initialises at 33 × 12 | `font 12x24 x1 -> cell 12x24, grid 33x12` |
| **Typing works, wraps, and redraws** | 408 characters typed in over the cable |
| **A document survives a chip reset** | `restored 408 bytes, seq 2` |
| **A deliberately torn write is rejected; the previous snapshot loads** | `PASS 2/2 … torn-write recovery works` |
| Autosave fires on newline and after a 1 s pause | `saved 180 bytes, seq 11, 3934 us` |
| The journal appends across sectors and wraps | cursor walked 0 → 4096 → 8192 → 45056 |
| BLE host starts, scans, decodes advertisements | ~48 distinct advertisers logged |
| Memory headroom with everything running | 212 KB internal free, 8.25 MB PSRAM free |

### The acceptance test, minus the radio

Type a paragraph → reset the board → the paragraph is still there. **That now
works**, with the USB cable standing in as the keyboard. The only untested
link in that chain is BLE HID itself.

### The byte counts are the good news

9 bytes for a 6×12 cell and 36 for a 12×24 cell are exactly what the window
arithmetic predicts. Getting both right, at two different cell sizes, against
a real controller, **corroborates the CASET mirroring rule by a route
independent of the driver it came from.** That was the highest technical risk
named in the handoff, and it is retired.

It is also now guarded. `tools/test_st7305_addr.c` compiles the firmware's own
`st7305_addr.h` — not a copy of it — and asserts that writing the leftmost
12 px alone emits `CASET = {0x2A, 0x2A}`. It additionally asserts that a naive
non-mirrored implementation would emit `{0x12, 0x12}` **and that the check can
tell them apart**, so it is a check proven to catch the bug rather than one
that merely passes. It runs in CI with no board attached, along with a check
that both bitmap faces still regenerate byte-for-byte from their art.

### The persistence test is self-driving

`main/selftest.c` walks stages across genuine chip resets using a counter in
NVS. Stage 1 writes a snapshot, **deliberately corrupts its payload**, and
stage 2 asserts the *previous* snapshot comes back. Both stages passed. It has
wiped itself and will not run again.

---

## Written but NOT verified

### Everything optical
I have no camera. The test card, the chunky typeface, ink/paper polarity and
contrast are **unseen by me**. Your look at the first build reported the image
mirrored and 6×12 text too small; both are addressed, neither confirmed.

### Orientation — the most likely thing still wrong
"All mirrored, everything reading reversed and backwards" is a horizontal
mirror on the logical-x → native-y axis, so the default is now `ORIENT_1`
(`ny = 399 − x`). **That is a hypothesis.**

You do not need me to fix it: **tap KEY (GPIO18) to cycle all four
orientations**, saved to NVS instantly, so once it reads right it stays right.
The top-left cell says `TOP LEFT` and the bottom-right says `BOTTOM RIGHT`;
when those sit in the corners they name and read forwards, it is correct. The
host-side check proves all four mappings are collision-free bijections, so
whichever you land on is a valid mapping.

### The BLE keyboard — completely untested
**No keyboard ever advertised.** Across several scans I logged 48 distinct
advertisers; not one carried the HID service (0x1812) or a keyboard
appearance. The names were neighbours' devices — `EMPRESS`, `S18 …LE`,
`Dime3_LE`, `N06SY`. The Rii 518BT was almost certainly powered off.

So the stack is proven to *scan and decode*, and everything past that is
unexercised: pairing, bonding, HID discovery, boot-protocol negotiation,
report decoding, key repeat, auto-reconnect, and the pairing-recovery gesture.

### True power-cut survival
Verified: full chip reset, and deliberate record corruption. **Not** verified:
yanking power mid-flash-write. The journal is built for it — sector-aligned
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
table); internal pull-ups; D1–D3 explicitly `GPIO_NUM_NC`; no card-detect and
no write-protect; probe clock (400 kHz) *and* default (20 MHz). The card never
answers `SEND_OP_COND`, the very first command after reset — so this is the
card or the electrical path, not FAT and not the filesystem layer.

**Your writing is not at risk.** Per the handoff's trap 6 the card was only
ever the export mirror; the flash journal is the source of truth and it is the
half that is verified. The deck runs fine with no card.

Worth trying next, in order: reseat the card; try a different one (some are
fussy in 1-bit mode); check whether the slot's power is gated by a GPIO absent
from the manifest; and note that SolarOS reaches storage on this board through
a `storage_expansion` driver rather than a board-native one, which hints the
slot may not be a plain always-on SDMMC.

---

## What changed in the design, and why

**The grid is 33 × 12, not 66 × 25.** `docs/HARDWARE.md` recommended 6×12
because it is aligned on both axes and cheapest per character — both still
true, both beside the point once you looked at it. The swap was free because
of an asymmetry the document states but does not draw out: **cell height must
be a multiple of 12 (the CASET quantum), but cell width only has to be even
(the RASET quantum is 2 px).** Width is cheap. Now recorded in
`docs/HARDWARE.md` as a measured correction.

`tg_set_font(&tg_font_6x12, 1)` still gives 66 × 25, and the call *refuses* a
misaligned cell rather than letting a line's damage window spill into its
neighbours.

**A new 12 × 24 typeface**, purpose-drawn: 2 px stems, 16-row cap height, real
descenders — built for a low-contrast reflective panel. Both faces generate
from reviewable ASCII art, never hand-edited hex:

```bash
python3 tools/make_font.py --12x24 > firmware/components/textgrid/font12x24.c
```

Monospace is structural, not stylistic: every glyph occupies the same cell and
advances the same distance, so **ASCII art lines up exactly** — which matters
for the Orca and live-visualiser direction you described.

**The USB cable is a keyboard.** Added so the editor could be tested at all
without a radio, but it earns its place anyway: if the BLE keyboard is flat or
lost, the deck is still usable from any terminal over the same cable that
flashes it. Both sources feed one queue and one event taxonomy, per
`docs/OS.md`.

**On the RP2040/HDMI idea:** nothing here forecloses it. Damage is tracked per
cell and resolved to a typed window push, so the same stream that drives the
panel can be serialised to another device later — what `docs/OS.md` means by a
damage list that "makes the deck remotable, screenshottable and testable in CI".
I built no transport; I just did not design it out.

---

## Things I suspect

- **The panel does not read back.** `RDDID` now executes cleanly but returns
  `00 00 00`. That is consistent with the 23-pin FPC carrying a single
  bidirectional data line — there may be no readable path at all on this
  board. Nothing depends on it; it logs one line at boot and is otherwise
  inert. I left it in because "the panel answered" would be useful evidence if
  it ever does.
- **Per-character latency is ~390 µs against 3.6 µs of wire time.** The cost of
  a keystroke is per-transaction overhead — GPIO toggles and command setup —
  not bandwidth. Nowhere near mattering, but that is where to look if it does.
- **`UP`/`DOWN` move by a whole line width rather than preserving the column.**
  An honest placeholder, not a considered design.
- **Autosave-on-newline burns a flash sector per line.** Correct and cheap
  (~3 ms, 128 slots, wraps safely), but a long writing session cycles the
  partition every couple of minutes. Fine for now; worth revisiting before
  this is someone's daily driver.

---

## The one thing I would do next

**Turn the Rii 518BT on and watch the log.**

```bash
. ~/esp/esp-idf/export.sh && cd firmware && idf.py -p /dev/cu.usbmodem2141101 monitor
```

Every advertiser is logged with name, address, appearance and UUID count, and
a matching one is tagged `<-- KEYBOARD`. That single observation decides which
morning you are having: either the HID path works and you have a writing
device, or it does not and the log says exactly where it stopped — scan,
connect, pair, discover, or subscribe.

Everything downstream is already proven through the cable. The keyboard is the
last unknown.
