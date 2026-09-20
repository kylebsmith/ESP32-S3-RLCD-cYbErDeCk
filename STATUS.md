# STATUS — night shift, 2026-09-20

**The acceptance test passes, in full, on the hardware.** Observed:

```
saved 220 bytes, seq 36            last save before the power was pulled
--- Waiting for the device to reconnect...      the power cut
restored 220 bytes, seq 36         byte-for-byte, same sequence number
```

Power on, the keyboard connects by itself, type a paragraph, pull the power,
power on, the paragraph is still there. Every link in that chain is now
observed rather than inferred, including the keyboard re-pairing and
re-subscribing with no intervention.

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
| Text grid with real margins | `grid 30x10 at (20,12)` — 20 px sides, 12 px top |
| Keyboard pairs, bonds, encrypts and subscribes | `encryption change, status 0` … `subscribed to report 5 of 5` |
| **Typing works, wraps, and redraws** | 408 characters typed in over the cable |
| **A document survives a chip reset** | `restored 408 bytes, seq 2` |
| **A deliberately torn write is rejected; the previous snapshot loads** | `PASS 2/2 … torn-write recovery works` |
| Autosave fires on newline and after a 1 s pause | `saved 180 bytes, seq 11, 3934 us` |
| The journal appends across sectors and wraps | cursor walked 0 → 4096 → 8192 → 45056 |
| BLE host starts, scans, decodes advertisements | ~48 distinct advertisers logged |
| **Keystrokes decode and reach the document** | `report handle 36 len 8: 00 00 17 …` -> 't' |
| **A real power cut, not a reset** | `saved 220 … seq 36` -> unplug -> `restored 220 bytes, seq 36` |
| Keyboard re-pairs and re-subscribes unattended after a power cut | full discovery walk, twice, no button pressed |
| **The SD card mounts** | `SD mounted: 00000, 14910 MB` |
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

### The BLE keyboard — connects and subscribes; typing still unproven
**Resolved since the first draft.** The Rii pairs, bonds, encrypts and is
subscribed. Verified from the log:

```
found a keyboard (unnamed), connecting
connected, starting encryption
encryption change, status 0
HID service at 21..53
characteristic discovery done: 5 report(s), protocol mode present
subscribed to report 2 of 5 / 3 of 5 / 5 of 5
```

Two real bugs were behind the original "connects but types nothing":

1. **Nested GATT procedures.** NimBLE allows one procedure in flight per
   connection, and descriptor discovery was being started from inside the
   characteristic-discovery callback. It failed with `EBUSY`, the CCCD was
   never written, and the link looked perfectly healthy.
2. **A double-advance in the subscription walk.** Descriptor discovery reports
   a CCCD *and then* reports completion; both paths advanced the index, so
   every characteristic with a CCCD advanced twice and the walk ran off the
   end — "subscribed to report 7 of 5" while real reports were skipped.

Also worth recording: this keyboard **refuses the boot-protocol write** with
ATT Write Not Permitted (status 259), so it stays in report protocol where the
boot keyboard report never notifies. Collapsing to the boot report — the tidy
thing to do — would have been silently wrong. The firmware subscribes to every
notifiable report instead and decodes any report of three bytes or more.

**The keyboard types.** Verified on the hardware:

```
report handle 36 len 8: 00 00 17 00 ...   't'
report handle 36 len 8: 00 00 0b 00 ...   'h'
report handle 36 len 8: 00 00 08 00 ...   'e'
cyberdeck: saved 177 bytes, seq 22
```

Reports arrive on handle 36 - a report-protocol input report in the 8-byte
boot-style layout - are decoded, and land in the document.

Three things were wrong, and the one that looked most likely was not the one
that mattered:

1. **Nested GATT procedures** (fixed earlier): descriptor discovery started
   inside the characteristic-discovery callback, failed with `EBUSY`, and no
   CCCD was ever written.
2. **A double-advance in the subscription walk** (fixed earlier).
3. **A stale bond.** The keyboard had bonded during the era when nothing was
   ever subscribed, and kept reusing that bond, so the fixed code never got a
   clean pairing. Clearing it and re-pairing is what actually released the
   keystrokes.

Also corrected: Protocol Mode is a *write-without-response* characteristic, so
the original write request was answered with ATT Write Not Permitted (259).

**A theory that was wrong, recorded because it was acted on:** that the link
needed to be MITM-authenticated and the deck had to display a passkey. The
firmware now asks for authenticated pairing and can show a passkey on its own
screen, but the keyboard still negotiates Just Works - `encryption change,
status 0`, no passkey action ever raised - so authentication was never the
problem. The passkey display is kept because it is correct behaviour for a
peer that does demand it, not because it fixed anything here.

### Orientation — settled
The owner cycled KEY until it read correctly in the enclosure and landed on
**orientation 3**. That value is now honoured permanently: a stored
orientation is evidence about the physical build, which the firmware does not
have and must never overwrite, so the shipped default applies only when
nothing has been chosen. The host-side check proves all four mappings are
collision-free bijections.

### True power-cut survival
Verified: full chip reset, and deliberate record corruption. **Not** verified:
yanking power mid-flash-write. The journal is built for it — sector-aligned
records, erase-before-write, CRC per record — but only a real power cut proves
a real power cut.

---

## The SD card: not a blocker, an empty slot

For most of this session the card would not initialise:

```
sdmmc_init_ocr: send_op_cond (1) returned 0x107   (ESP_ERR_TIMEOUT)
```

**There was no card in the slot.** The owner confirmed it afterwards. A timeout
on `SEND_OP_COND` - the very first command after reset - is precisely what an
empty slot looks like, so the driver was behaving correctly the whole time and
the pin map from `solar_term.toml` was right.

Recorded because the reasoning went wrong in an instructive way. Everything
that *was* checked - both clock rates, internal pull-ups, explicit `GPIO_NUM_NC`
on the unused data lines, no card-detect - was checked against the hypothesis
"the bus is misconfigured", and each negative result made that hypothesis look
worse without ever promoting the simpler one. The failure mode is identical for
a missing card and a broken bus, and nothing in the firmware could tell them
apart. **A card-detect line would have, and this board does not route one** -
so the honest fix is for the log to name both possibilities rather than imply
a fault. ESP-IDF's own hint said as much and it was read past.

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

## The audit, and what it found

Before building step 5 on top of this, five reviewers went through the tree by
subsystem and every finding was put to two adversarial verifiers instructed to
refute by default. Nineteen survived; all nineteen are fixed.

The one that mattered most had been sitting in plain sight and was reported by
the owner in three words:

**The editor never reached the panel.** `editor_draw()` called `tg_render()`,
which draws the dirty cells and *clears the dirty bits*; `main` then called
`tg_flush()`, which renders again, sees zero dirty cells, and concludes there
is nothing to push. Every edit was drawn perfectly into the framebuffer and
never clocked out. The only things that ever reached the glass were the test
card and the passkey screen, which call `st7305_flush_full()` directly.

The owner said "the text isn't reacting" and that was diagnosed as a keyboard
problem. It was not. Render and present are now separate calls with the reason
written at the call site.

**A keystroke was pushing 13,800 bytes** — 92 % of a full frame — because the
status bar and the hairline rule span the full width and damage was a single
union rectangle. `docs/OS.md` specifies a damage *list* and this is why: with
four rectangles it is **1,272 bytes**, and a cursor blink is 36. Measured
before and after, on the panel.

**The SPI bus had no mutual exclusion** while three tasks reached it - the
main task drawing, the esp_timer callback dropping to LPM, and the NimBLE host
task. `st_cmd_data` holds CS low across a command and its payload, so an
interleaved command lands inside that window and the panel receives a spliced
transaction. Now serialised with a recursive mutex.

**The journal's write cursor followed the last record by offset rather than
the newest by sequence.** On a wrapped journal that put the cursor after a
stale record, and the next save would have erased the sector holding the
newest one. This is the finding that could have lost writing.

Also fixed: the self-test could erase a real document; the view froze silently
past 2,048 display lines (wrapping is now a bounded window around the cursor);
notifications from any characteristic were decoded as keystrokes, including
the battery service; a lost key-up repeated until the BLE supervision timeout;
several discovery failures left the deck connected-but-dead for ever; and the
passkey screen was drawn from the NimBLE task straight into the main task's
framebuffer.

## A lesson about instrumentation

The deck was reported as frozen and was not. It was idle.

Two decisions, each defensible alone, went quiet at the same moment. The
cursor stops blinking fifteen seconds after the last keystroke, on purpose, so
the panel can drop to low power - which makes a working screen look dead. And
every log line was capped or conditional: the push probe stopped after exactly
24 lines, the advertisement log capped at sixteen, and the scan heartbeat only
printed while NOT connected. Connected and idle produced total silence.

So the screen looked frozen and the log looked frozen, and neither was.

The fix is a heartbeat from the MAIN loop, every ten seconds, carrying the
things that distinguish idle from hung - document length, undo depth, keyboard
state, pushes and bytes since the last beat, and free heap:

```
alive: doc 64*, undo 10, kbd up, 26 push/15696 B, heap 199571
```

The main task printing it is itself the proof that the loop is turning. Quiet
now means idle and cannot mean broken. Counters replaced the capped probe, so
the numbers keep flowing rather than stopping just when something goes wrong.

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
- **Autosave burns a flash sector per save, and saves are frequent.** Observed
  in the log: typing slowly produces one save per character (`saved 210`,
  `211`, `212`, … one sector each). 128 slots wrap safely and each save costs
  only ~3-5 ms, so nothing is at risk — but a slow typist cycles the whole
  partition every 128 characters. The fix is a minimum-change or
  minimum-interval threshold on top of the existing idle timer. Worth doing
  before this is a daily driver, not urgent.

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
