# Night shift handoff

*Written by the session that produced everything in `docs/`. Read this first,
then the documents it points at. You have hardware; that session did not.*

## Situation

The owner has gone to bed. On their desk, plugged in right now:

- A **Waveshare ESP32-S3-RLCD-4.2** development board, connected by USB-C.
- A **16 GB microSD card** already seated in the board's slot.

They want to wake up to something they can pick up and use. They will **not be
available** to press BOOT or RESET, so every flash you do must be
button-free. That is fine: the ESP32-S3 has a USB-Serial/JTAG peripheral and
`esptool` can drive it into download mode over the USB CDC control lines.

```
idf.py -p <PORT> flash monitor          # macOS: /dev/cu.usbmodem*   Linux: /dev/ttyACM0
```

If a flash ever fails to enter the bootloader, `--before default_reset` is the
flag that matters. Do not ask them to hold a button.

## Do this first

```bash
mkdir -p /Distractions && cd /Distractions
git clone https://github.com/kylebsmith/ESP32-S3-RLCD-cYbErDeCk.git
cd ESP32-S3-RLCD-cYbErDeCk
git checkout claude/cyberdeck-enclosure-design-4vzwq6
git checkout -b claude/firmware-night-1
```

Then read, in this order, and **do not start coding until you have**:

| Read | Why |
|---|---|
| `docs/SUBSTRATE.md` | the conceptual core. One data structure, one noun, three verbs. Everything else is downstream. |
| `docs/HARDWARE.md` | every hardware number, traced to a vendor source. **The ST7305 window-addressing section is the most valuable thing in this repository.** |
| `docs/OS.md` | the firmware design, what is rejected and why, and the build order. |
| `docs/METHODOLOGY.md` | how this project works. It is not optional; the standards below come from it. |

`docs/DATUMS.md` is the enclosure's 35 recorded corrections. You do not need it
tonight, but skim the *shape* of a few records — that is the expected standard
for recording anything you discover.

## What is already established — do not re-research it

The previous session spent a long time on this, all from primary sources.
Re-deriving it wastes your night.

- Panel is a **Sitronix ST7305**, 400×300 landscape, **1 bpp, no backlight**.
  Framebuffer is **15,000 bytes** and **must live in internal DMA SRAM** — SPI
  DMA requires it. Documents and scrollback go in PSRAM.
- **Text grid is 66 × 25 at 6 × 12 px.** This is derived, not chosen: 12 px is
  the panel's own line-height quantum in landscape. One character costs 9
  bytes, one full-width line 600, a full frame 15,000.
- **HPM** self-refresh is 16/25.5/32/51 Hz; **LPM** is 0.25–8 Hz.
- Keyboard is a **Rii 518BT over BLE HID (HOGP)** via NimBLE. This is
  confirmed — the owner scanned the unit and saw an nRF5 advertising the HID
  service. An earlier analysis claimed it was Bluetooth-Classic-only; that
  claim is **wrong** and is recorded as such. Do not act on it.
- **USB-OTG shares one PHY with USB-Serial/JTAG.** EP0 + 6 endpoints, ≤5 IN.
  Not your problem tonight — you want the console, not MIDI.
- Pin map, complete and corroborated, is in `docs/HARDWARE.md`. Display:
  **SCK 11, MOSI 12, CS 40, DC 5, RST 41, TE 6.** I²C: **SDA 13, SCL 14**.
  Buttons: **BOOT 0, KEY 18**. Battery ADC: **ADC1 ch3**.

## The one thing most likely to be got wrong

The ST7305 datasheet Waveshare links is for a **different, smaller part**: it
specifies a 264×320 RAM and a CASET range that does not match this panel. Its
command set, power modes and frame-rate tables are correct; **its address
ranges are not.**

The real mapping, verified against a working Apache-2.0 driver *and*
re-derived independently:

```
column address = 0x12 + x/12     range 0x12..0x2A    1 address = 12 px = 3 bytes
row address    = y/2             range 0x00..0xC7    1 address = 2 lines
mirror base    = 0x12 + 0x2A = 0x3C

CASET (0x2A) = { 0x3C - addr_end, 0x3C - addr_start }   <-- reversed and mirrored
RASET (0x2B) = { first_y/2, last_y/2 }                  <-- must be 2-line aligned
```

**At full width the mirroring is an identity** — `{0x3C-0x2A, 0x3C-0x12}` is
`{0x12, 0x2A}` — so every full-frame driver is silently correct and never
discovers the rule. It only bites when a window narrows. To write the leftmost
12 px alone you must send `CASET = {0x2A, 0x2A}`, **not** `{0x12, 0x12}`.

One RAM byte is **4 px wide × 2 px tall**; the top (smaller-y) pixel is the
higher bit.

**Strongest recommendation of this handoff:** vendor the ST7305 driver from
`github.com/nilseuropa/solar_os` (`src/drivers/rlcd_st7305.c`, **Apache-2.0**)
rather than writing one. It is proven on this exact panel, it implements the
windowing and the HPM/LPM policy, and it will save you the whole night.
`boards/manifests/solar_term.toml` in the same repo is a validated pin map for
this exact board — take it as data.

## Traps that will silently ruin the night

1. **`CONFIG_FREERTOS_HZ` defaults to 100**, so `vTaskDelay(1)` sleeps 10 ms.
   Set it to 1000 in `sdkconfig.defaults` now, not later.
2. **Writing while the panel is in LPM can delay the visible update by up to a
   full refresh period (~1 s).** Kick to HPM on the first keydown. Prior art
   defaults to HPM and makes auto-LPM opt-in — that is the wrong default here,
   but get it working before you get it clever.
3. **SPI reads are 5× slower than writes** (30 ns vs 150 ns cycle). Any read
   path at the write clock fails intermittently.
4. **The framebuffer must not live in PSRAM.** See above.
5. **A driver labelled ST7306 may still be correct** for this panel; Zephyr's
   board definition declares that part. Do not let the name mislead you.
6. **The SD card is an export medium, never the source of truth.** Losing power
   during a FAT write can corrupt the directory, not merely truncate a record.
   Journal to internal flash; sync to SD on newline or a 1 s timer, never per
   keystroke, and write `.tmp`-then-rename.
7. **Trailing whitespace is significant** in any Orca content — see
   `docs/SUBSTRATE.md` for the fence format that makes losing it structurally
   impossible. Not tonight's problem, but do not design it out.

## Build tonight — and only this

`docs/OS.md` has the full build order. **Tonight is steps 1 to 4**, which is
the point at which the device becomes worth carrying. Resist everything else;
MIDI, OSC, Wi-Fi, Orca and the network hub are all explicitly *not* tonight.

1. **Display brought up.** ST7305 over SPI, full-frame push working, then
   windowed update, then the HPM/LPM policy.
2. **Text grid.** 66 × 25 at 6 × 12, a real bitmap font (hand-hinted — at
   1 bpp any TTF rasteriser produces mush), damage tracking so a keystroke
   redraws a character and not a frame, and a status line.
3. **BLE HID keyboard.** NimBLE HOGP host, bonds persisted in NVS,
   auto-reconnect, and a **physical pairing-recovery gesture** — a long hold on
   KEY (GPIO18) that forgets all keyboards. That gesture is not optional: it is
   the only way out of a broken pairing with no screen affordance.
4. **A text buffer they can actually type into, that survives power loss.**
   Gap buffer, cursor, backspace, newline, scrolling. Autosave to flash, mirror
   to SD.

If you finish early, do **not** start MIDI. Harden what exists: reconnect
after reset, bond survival, what happens when the card is missing, what happens
when the keyboard is off.

## Done, defined

The owner should be able to: power it on, have the keyboard connect by itself,
type a paragraph, see it on the screen crisply, pull the power, power it back
on, and **find their paragraph still there.**

That is the whole acceptance test. Everything else is a bonus.

## How this project works

These are the repository's standards and they are why it is in good shape.
Follow them.

- **Measure, do not assume.** A check that computes from parameters instead of
  measuring the artefact is worse than no check. Every defect this project has
  found was in something no check interrogated.
- **Every fix ships with a check proven to fail on the old state.** Not "a test
  that passes" — demonstrate it catches the bug, then demonstrate it passes.
- **Record corrections.** If you discover something that contradicts a document
  here, fix the document and write the record. `python3 tools/check_docs.py`
  keeps the docs honest and runs in CI; keep it green.
- **Commit messages explain *why*, at length.** Look at `git log` for the
  house style. End every commit with the two attribution lines already in use.
- **Never claim something is tested when it is not.** If you could not verify
  it on hardware, say so in those words.

## Before you stop

Write **`STATUS.md`** at the repo root, for the owner to read with coffee. It
must say, plainly:

- What works, verified on the hardware in front of you.
- What is written but **unverified**, and why.
- What is not started.
- The exact command to flash it, and the exact command to watch the log.
- Anything you broke, or suspect.
- The single next thing you would do.

Then commit and push `claude/firmware-night-1`.

If you hit something genuinely blocking — a part that will not initialise, a
dependency that will not build — **do not burn the night on it.** Record it in
`STATUS.md` with what you tried, and build the next thing in the list. Four
working subsystems and one honest blocker is a far better morning than one
subsystem and a mystery.

Good luck. The hard thinking is already done and it is all in `docs/`. Tonight
is about making it real.
