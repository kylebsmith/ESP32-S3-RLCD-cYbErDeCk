# cYbErDeCk OS — design

A keyboard-first writing, capture and live-coding environment for the
Waveshare ESP32-S3-RLCD-4.2. Every hardware number it reasons from lives in
[HARDWARE.md](HARDWARE.md); this document is the decisions, each one traceable
to a constraint rather than to taste.

The same discipline as the enclosure applies. Claims are tagged `[FACT]`
(verified against a primary source), `[JUDGEMENT]` (a decision, with its
reason), `[UNMEASURED]` (plausible, not yet benchmarked) or `[OPEN]`.

## The thesis

It should feel like a **cyberdeck, not an appliance**: it boots to something
you can type into, everything is reachable by name, and it keeps working on a
plane with a dead API key. The panel is reflective, so it is readable in
sunlight and costs almost nothing to hold an image — the machine is never
really off, it is just idle.

It is an **idea machine**, and it sits at the centre of a creative-technology
art practice rather than at the edge of one.

**Three co-equal jobs.** None is a side feature of another, and the ordering
below is a lifecycle, not a priority:

1. **Capture** — type or speak an idea, and never lose it. A real text editor
   is central, not a notepad bolted onto an instrument.
2. **Compose** — develop prose, structure and patterns with a grammar suited
   to a thumb keyboard.
3. **Perform** — live-code, emit MIDI to hardware and software, and host the
   network that the rest of the practice connects to.

**Text is the shared substrate**, and that is what makes it one machine rather
than three apps. A note, an Orca patch (literally a rectangular block of
ASCII), a guide file of executable lines, a routing table and an OSC address
map are all text on the same grid. The editor is the OS.

That claim is developed properly in **[SUBSTRATE.md](SUBSTRATE.md)** — one data
structure, one noun, three verbs — and everything below is downstream of it.

## What the hardware decides

These are not preferences. Each one closes off a design that would otherwise
look reasonable. All are established in HARDWARE.md.

| Constraint | Consequence |
|---|---|
| No MMU `[FACT]` | No Linux, no processes, no memory isolation. Reliability comes from *ownership discipline*, not from hardware protection. |
| BLE only, no Bluetooth Classic `[FACT]` | The keyboard is a BLE HID (HOGP) peripheral. Confirmed against the owner's unit, which advertises the HID service. |
| One USB-OTG peripheral, sharing a PHY with USB-Serial/JTAG `[FACT]` | The port is *either* MIDI *or* the console. USB host is off the table. |
| EP0 + 6 endpoints, ≤5 IN `[OPEN]` | CDC + MIDI fits. Adding HID overflows. **This was tagged `[FACT]` and said to be "established in HARDWARE.md". It is not** — HARDWARE.md contains no endpoint budget and mentions USB once, about a current sink. By this repository's own discipline the number is untraceable until it is read out of the ESP32-S3 datasheet, and the USB MIDI decision must not rest on it before then. |
| 1 bpp, strictly `[FACT]` | No antialiasing, ever. Hand-hinted bitmap faces only; any TTF rasteriser producing coverage values produces mush. |
| Byte = 4 × 2 px; landscape quanta 2 px wide, 12 px tall `[FACT]` | **12 px is the hardware's line height.** The text grid is not a style choice. |
| LPM write latency ≈ one refresh period `[FACT]` | Idle at 1 Hz is free, but the first keystroke must kick the panel to HPM or it feels like a Freewrite. |
| Framebuffer must be internal DMA SRAM `[FACT]` | 15 KB of the 512 KB is spoken for. Documents and scrollback go to PSRAM. |
| No backlight and no net to add one `[FACT]` | The device is unusable in the dark. This is a real product limitation, not a footnote. |
| TLS ≈ 40–50 KB free internal heap `[FACT]` | **Exactly one TLS session at a time**, serialised behind one network task. No "sync in the background while talking to Daemon". **This bounds TLS only** — see below. |

**The TLS figure does not generalise to "the network."** It is driven by
mbedTLS's record buffers — `CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN` 16384 and
`..._OUT_CONTENT_LEN` 4096 in this project's own `sdkconfig`, 20,480 B that
exist purely for TLS record framing. SSH frames its own packets and never
allocates them: an SSH client measures at **696 B of static internal SRAM**,
with its 80,728-byte session in PSRAM. The constraint was correctly derived
for TLS and wrongly extended to everything networked. See
[NETWORK.md](NETWORK.md).

## Base: ESP-IDF 5.5.x, directly `[JUDGEMENT]`

Not a fork of anything. The survey turned up four credible bases and each was
rejected for a specific reason:

| Candidate | Why not the base |
|---|---|
| **SolarOS** | 298k lines, Apache-2.0, genuinely excellent — but its contribution policy pushes native apps away toward Python/Lua, so building on it means maintaining a fork of a very large, opinionated tree. **Take its driver, not its OS.** |
| **Draftling** | MIT, targets this exact board, closest in spirit. Single maintainer, and the author states most of it was LLM-generated. Its `git_sync`, `ble_keyboard` and `usb_msc` components are precisely where a silent bug loses notes. **Read it; don't inherit it.** |
| **Tactility** | GPLv3, and LVGL/touchscreen-shaped against a 1-bit panel with no touch. |
| **Zephyr** | Has an in-tree board definition for this exact board, which is worth an evening for the free authoritative pin map — but its own docs mark the board *not actively maintained*, there is no codec driver, and no upstream HID-over-GATT *client*. |

ESP-IDF is the only stack where every part on this board has a vendor driver,
NimBLE HOGP is proven by **five independent working implementations on this
class of hardware**, and TinyUSB does a CDC+MIDI composite.

**Pin to 5.5.x.** The one project worth raiding for voice (xiaozhi-esp32)
requires 6.0.1+, and that version split is the single biggest toolchain fork
available to get wrong. Reach it over HTTP instead (see *Voice*).

### What we vendor, and under what licence

| From | What | Licence |
|---|---|---|
| SolarOS `src/drivers/rlcd_st7305.c` | the ST7305 driver — the only one that solves windowed update | Apache-2.0 |
| U8g2 | glyph rendering and the font pipeline | BSD-2-Clause |
| Orca-c `sim.c` `field.c` `gbuffer.c` `base.h` | the live-coding VM, **1,138 lines** measured on a fresh clone (1,280 with headers); the 1,381 here was wrong | MIT |
| `espressif/elf_loader` | loadable native apps | Apache-2.0 |
| `midilab/uClock` | the musical clock | MIT |

**Licence traps, recorded so nobody trips one later:** Tactility, MicroHydra
and Flipper Zero are **GPL-3.0**; Bruce is **AGPL-3.0**; Beepy's kernel drivers
are **GPL-2.0**; the `wiki.xxiivv.com` and `100r.co` *documentation* is
BY-NC-SA even though uxn's code is MIT. **No LICENSE file at all** — treat as
all-rights-reserved and read only for ideas: `typoena/typewriter`,
`andywarburton/gr3ml1n-cyberdeck`, `shmimel/whale-writer`'s software directory.

## Architecture

### Two cores, one lock-free queue `[JUDGEMENT]`

Borrowed from norns' matron/crone split, which the survey called the single
most important structural idea for a live-coding device.

```
core 1  —  REALTIME       core 0  —  EVERYTHING ELSE
  display DMA               app / script runtime
  MIDI clock + emission     editor, palette, UI logic
  audio I2S + AFE           BLE HID host
                            Wi-Fi, TLS, Daemon
        └──────── lock-free ring ────────┘
```

A script that spins for 200 ms delays only its own redraw. It cannot make a
note late. The cost is real and accepted: **the script layer can never call
DSP or MIDI synchronously** — everything crossing the line is a message, so
the command vocabulary has to be designed up front rather than grown.

### The OS owns the run loop `[JUDGEMENT]`

Apps never own `while(1)`. They register callbacks and return, Playdate-style.
"Quit app" becomes "stop scheduling its callback and run teardown" — a normal
function return, not a thread kill. The status bar, a battery warning and a
Daemon notification can therefore always draw.

Input is dispatched **twice per pass**, before and after the app tick, so a
keystroke arriving mid-tick is serviced in the same pass. One line, and it is
measurably better keystroke latency.

### Ownership, because there is no MMU `[JUDGEMENT]`

This is the load-bearing reliability decision and it must be made on day one —
retrofitting it after ten subsystems exist is miserable.

Every subsystem that hands out a resource — timers, MIDI ports, the mic
stream, display regions, file handles, BLE connections — keeps an
**owner-tagged handle table** and implements `release_all(owner)`. Stopping an
app, or reloading a script, is enumerated teardown, not "run the file again".
Callbacks are redirected to a no-op sink rather than left dangling.

Paired with:

- **Declared stacks.** Each app states `stack_size` in its manifest; the OS
  sizes the task rather than guessing. A `_Static_assert` caps foreground
  stacks, and an internal-SRAM reserve is kept free so a launch fails cleanly
  instead of taking the system with it.
- **Cold state.** No mutable file-scope statics in app translation units.
  State is allocated immediately before start and freed after stop, so a
  hundred compiled-in apps cost approximately zero RAM until launched. Enforce
  it with a build-time checker, not a code-review convention.
- **Crash survival.** Stash the faulting app's ID and PC in RTC memory, which
  survives a software reset, reboot into the launcher, show *"app X faulted"*
  instead of a boot loop, and auto-blacklist after N consecutive faults. A few
  dozen bytes for the highest-value reliability mechanism available.

## Display

### The grid is 66 × 25 at 6 × 12 `[FACT]`

Derived in HARDWARE.md, not chosen: 12 px is the panel's own line-height
quantum in landscape, and an even advance keeps the width aligned. 66 columns
is close to the classical measure for prose. An 80-column mode is misaligned
on both axes and costs the most per character of any candidate — it is the
wrong trade, not a trade.

### Damage, in three layers `[JUDGEMENT]`

| Layer | Unit | Why |
|---|---|---|
| Cell dirty bitmap | one 6 × 12 cell | 66 × 25 = 1650 bits = **207 bytes**, free |
| Row hash | one text line | catches "nothing actually changed" cheaply |
| Driver shadow | one RAM byte | 15 KB in PSRAM; `memcmp` before any SPI |

The reference driver stops short here: its terminal only ever sends
**full-width** bands, so it never exercises a narrow-X window and re-sends
300 × 8 px to change one character. **Drive a real bounding box.** One
character is 9 bytes; one full-width line is 600; a full frame is 15,000.

Renderer API is a **damage list of typed opcodes** — draw-character,
draw-rectangle, with unchanged fields omitted and the receiver reusing the
last value. Two payoffs: it is exactly the shape a slow panel wants, and as a
wire format it makes the deck remotable, screenshottable and testable in CI
with no hardware.

### Power policy `[JUDGEMENT]`

Default to **auto**: any changed cell forces HPM; an idle timeout drops to LPM.
The prior art defaults to HPM and makes auto opt-in, which is the wrong way
round for a battery device. Kick to HPM on the *first keydown*, not on the
first redraw, so the panel is already fast by the time the glyph is ready.

### Rules for a slow panel, adopted as hard constraints `[JUDGEMENT]`

No cursor blink. No smooth scrolling. No animation. Render only changed lines.
Keypress-to-glyph **≤ 200 ms**. Schedule any full refresh for a typing pause.

## Input

Transport is **BLE HID over NimBLE**, with bonds in NVS. The risks are real
and are test items rather than blockers: reconnect after reset, bond survival
across deep sleep, and 2.4 GHz coexistence costing keystroke latency while
Wi-Fi is talking to an API. One implementation in the survey ships a known
"requires reboot after pairing" defect — **test the re-pair path hard.**

**There must be a physical, no-screen-needed way to recover a broken
pairing.** A long hold on the KEY button that forgets all keyboards is the
best-designed version of this in the prior art. Adopt it.

### Solving "the arrow keys are cramped"

The owner's own verdict on this keyboard's arrow cluster matches the reviews.
The answer is not to use them.

- **Meta mode** — one tap of a dedicated key turns the alpha block into
  navigation (`E/S/W/D` arrows, `R/F` home/end, `O/P` page, `Q/A` word). Two
  details make it work rather than trap you: it exits on **any unmapped key**,
  so the worst case is one wasted keystroke and never a stranded device; and a
  ~1 s hold shows a reference overlay. A 66-entry table is 132 bytes a layer.
- **Sticky modifiers with an always-visible indicator.** Tap = next key only,
  tap again = cancel, hold = until release. The indicator is load-bearing;
  sticky modifiers without visible state are a bug generator, and on a
  reflective panel a status corner costs nothing to hold.
- **One-shot layers with a timeout.** The timeout is the difference between a
  sticky modifier and a mode you are stuck in.
- **One held modifier plus one letter is the ceiling for a thumb.** Two
  modifiers is not a chord this device can make. Any design needing
  Shift+Alt+direction is wrong for this hardware.

**BLE-specific caveat `[FACT]`:** connection intervals of 7.5–15 ms jitter
press/release timestamps. Use **≥180 ms hold thresholds** and prefer
**tap-count over tap-duration** wherever a gesture must be reliable.

### One event taxonomy, centrally synthesised

Press / Release / Short / Long / Repeat, produced in one place and queued.
Five event types across three side buttons is fifteen gestures from three
physical switches, and **no app writes timing code** — so there is exactly one
place to tune for BLE jitter. Key repeat is the OS's job, not the keyboard's,
because BLE HID repeat behaviour is device-dependent.

## Editing grammar

**Selection-first, object-verb** (Kakoune/Helix): every motion *sets a
selection*; the verb comes second. Press `w` `w` `w`, watch the highlight
grow, then `d`. On a keyboard with imprecise thumb targets this is the whole
argument — a mis-press costs one keystroke, not an undo — and it deletes a
mode, since visual folds into normal.

**Navigate by typing, not by moving.** `s<regex>` replaces the current
selection with one selection per match inside it. Typing six letters beats
forty arrow presses, and typing is the thing thumbs are actually good at. A
bounded regex VM is ~10 KB flash / ~4 KB RAM; **cap selections and error
rather than OOM.**

**`|` — pipe the selection through a filter.** This is the single most
important structural decision in the document, because it is how everything
else attaches without inventing new UI:

```
select a paragraph → | → daemon:rewrite   → paragraph replaced
select nothing     → ! → daemon:draft     → text inserted
select a list      → | → sort             → sorted
select a phrase    → | → midi:humanise    → humanised
```

There is no "AI menu". Daemon is a filter in a table, and every future
capability is another row. A filter is `(name, fn(in, len, out))` — 12 bytes
an entry, 100 filters is 1.2 KB. Asynchronous filters render the pending
region **dimmed**, and Escape cancels.

## Discovery

Everything must be reachable **by name**, because a keybinding you cannot
remember is a feature you do not have.

- **One palette, many namespaces by sigil** — `>` commands, `@` symbols, `:`
  line, `?` lists every prefix. One key serves files, commands, notes, Daemon
  prompts, MIDI devices and app launch. **`?` is mandatory** — it is the
  palette's own help.
- **fzy's scoring algorithm**, not fzf's: bounded small integer scores, so
  sort by counting sort — no floats, no `qsort`. ~1–3 ms per keystroke for 400
  candidates, run on core 0.
- **Frecency ordering** — recent first, then frequency, then *shorter name
  wins ties*. Halve all counts on saturation to stay adaptive. This turns the
  palette into "press palette, press Enter" for the five things you do daily.
- **which-key, delayed.** The timeout *is* the mechanism: type fast and
  nothing is ever drawn, so the expert pays zero; hesitate and every
  continuation is listed. Keep a dirty-rect save/restore so dismissing the
  overlay does not force a full redraw.
- **The guide file** — a plain-text, user-editable buffer where Enter executes
  the line under the cursor. **A text buffer becomes a menu the user writes by
  typing.** Every app ships one; adding a menu item costs typing a line. The
  discipline cost is the point: every OS action must be reachable by a *name
  in a table*, not only by a binding.
- **The plumber** — one key that does the obvious thing with the selection,
  driven by a user-editable rules file. `note.md:27` opens at a line, a URL
  fetches, `C#4` previews, `120bpm` sets the clock, and **a bare word degrades
  to literal search — it never fails.**

## Assistants, as one filter among many

**Demoted deliberately.** An LLM assistant is *a* filter in the table below,
not a pillar of the device. It must never be on the critical path of capture,
composition or performance, and the machine must be fully useful with the
radio off. What follows is the shape it takes **if** it is wired up at all.

**The device never talks to an LLM directly** `[JUDGEMENT]`. It ships text or
audio to **one endpoint under your control**, which does STT, prompting and
storage.

Three reasons, all of which outrank the convenience of calling an API
directly: API keys stay off a device you might lose; the backend can be
re-pointed without reflashing; and the firmware speaks exactly one protocol,
which is the only way the TLS budget above stays affordable.

Surface area on the device is the `|` filter table plus the palette. Nothing
else needs to know an assistant exists, which is the point: the feature can be
removed entirely without the OS noticing.

## Voice

**Open-vocabulary dictation cannot run on this chip.** `[FACT]` Not "is hard" —
whisper-tiny's INT8 weights alone exceed the 8 MB of PSRAM before activations,
and every "ESP32 Whisper" project does inference on a server. Espressif's own
on-device recogniser is a closed-vocabulary CTC decoder over at most 200
phrases fixed at build time, it cannot start until a wake word fires, and it
would eat over a third of PSRAM. **Budget zero hours here.**

The capture half, though, is genuinely excellent, and the build order reflects
that:

| Tier | What | Why this order |
|---|---|---|
| **0** | Push-to-talk → AFE (`VC` mode, noise suppression + AGC + VAD, **AEC and BSS off**) → **timestamped `.wav` straight to SD**, written `.tmp`-then-rename | **This is the product.** It works in a basement, on a plane, with a dead API key. 32 KB/s is free; a 32 GB card holds ~280 hours. |
| **1** | Live streaming transcript over one WSS, raw 16-bit PCM, interim results painted onto the panel as you speak | The cyberdeck feel. Proven on near-identical hardware with the same codec pair. |
| **2** | Drain queued files to a batch transcription endpoint | Simplest possible failure model: it uploaded or it didn't. A `.wav` with no sibling `.txt` is pending. |
| **3** | Wake word for hands-free capture | Nearly free. Optional. |

Two traps worth naming: the AFE's speech-recognition mode computes gain from
the wake-word engine and **silently sabotages push-to-talk** — use the
voice-communication mode. And the ES7210 and ES8311 **share one I2S bus**, so
capture and playback are locked to a common clock domain; plan the clock tree
before writing code.

The board's advertised "echo cancellation" is a marketing description of
spare ADC channels, not silicon `[FACT]`. Design for near-field and it works.

## Performance, MIDI and the network

> **Under active revision.** The brief widened: the device is also a
> **performance instrument and a network mothership** — DIN MIDI into an
> interface, USB MIDI to a laptop or an Ableton Move, BLE MIDI as a wireless
> option, and its own Wi-Fi network carrying OSC so a laptop and a Raspberry
> Pi can join it. Transport coexistence, clock architecture, routing and the
> hub design are being researched now; what follows is the settled core and
> will be extended rather than replaced.

### Transports

| Path | Latency | Verdict |
|---|---|---|
| **USB MIDI device** (CDC + MIDI composite) | class-of-device ~4 ms `[UNMEASURED]` here | The default. Class-compliant, no drivers. |
| **TRS-A / DIN over UART @31250** | **0.96 ms**, deterministic | **Add it.** One GPIO, a resistor pair, a jack. |
| BLE MIDI | 7.5 ms ±1.8 | Available, but never the clock. |
| USB MIDI *host* | — | Ruled out: needs to source VBUS, and consumes the only port. |

The DIN path is the recommendation both MIDI analyses reached independently:
on a device whose stated priority is robustness, **it is the only MIDI path
that cannot be broken by a host, a driver, a PHY conflict or a Bluetooth
stack.** It costs an enclosure revision, which is a known, bounded price.

### Clock discipline `[FACT]`

- **`CONFIG_FREERTOS_HZ` defaults to 100, so `vTaskDelay(1)` sleeps 10 ms.**
  Set it to 1000. This one line has ruined more sequencers than anything else.
- **Never sequence from a task delay.** Use a hardware timer.
- **Never clock from BLE-MIDI.** Run a local high-resolution clock and
  *phase-lock* to incoming clock rather than being driven by it.
- The USB stack's stream-write **queues**; bytes move when its task runs. Never
  build the musical clock on top of it.

### Orca `[JUDGEMENT]`

The live-coding language is **Orca**, and the fit is close enough to be
uncanny:

- The VM is **four dependency-free C99 files, 1,381 lines**, no allocation
  during a tick, no floating point, ~20–30 KB of flash. `tui_main.c` is over
  half the repo and **you throw all of it away** — confusing "port Orca" with
  "port Orca's terminal app" turns a weekend into a month.
- **The ASCII case bit is the scheduler.** Lowercase runs on a bang, uppercase
  every frame — and shift-vs-no-shift is the one modifier a thumb board is
  genuinely good at. That is a rare alignment between input device and
  language design.
- **The entire program is a char array** you can memcpy, hash, diff, save or
  snapshot for undo. A 16-deep undo ring is 39 KB.
- **The whole patch fits on screen without scrolling** — 66 × 37 at 6 × 8,
  larger than Orca's own desktop default, unlike every other small-screen port.
- An LLM emitting an Orca patch is emitting **plain text on a fixed grid**,
  which is a far easier generation target than code — a direct bridge between
  the two headline features.

No ESP32 port exists. This would be the first.

Two guardrails: Orca's core **calls `malloc` without checking returns**, so
every allocation is replaced with a checked one or a static arena before it
runs on a 512 KB part. And **do not inherit base-36 as the only numeric
type** — every cell is 0–35, so a 0–127 CC needs two cells. Keep a separate
7-bit path for anything talking to real MIDI.

Add `$` (inject another patch at a coordinate) on day one: it is `#include`
for a 2D language and the only composition mechanism Orca has.

## The SD mirror, and what a backup is for

The journal in flash is the source of truth. The SD card is an **export
medium**: the owner copies it to a DGX at home and runs semantic analysis over
the corpus, so what lands on that card is training and retrieval input, not
just a safety copy. Two properties follow, and neither was true until they were
found on hardware.

**One file per document.** The mirror wrote every buffer to a single
`/sdcard/notes.txt`, so switching documents overwrote the previous document's
backup with the current one. The card held exactly one document — whichever was
edited last — while appearing to hold a backup of the work. It is now
`/sdcard/<name>.txt`, one per document, and the unnamed scratch buffer is
mirrored as `scratch.txt` because that is where work starts and the buffer most
likely to hold something unsaved. `[VERIFIED]` — three documents, three
distinct files, read from the console.

This forced `CONFIG_FATFS_LFN_HEAP` on. Under 8.3 short names the documents
`rustbelt` and `rustbeltsave`, both of which exist on the device, truncate to
the same `RUSTBELT.TXT`. `tools/test_mirror_path.c` asserts that collision case
by name so the reason survives the next person reading the config.

**Machine-written buffers are not mirrored.** `doc_save()` already refused to
journal a `+` buffer, but the caller went on to mirror it anyway, so the
contents of `+out` — command output, lane listings, error text — were written
to the card as though they were a document. A corpus salted with command
transcripts is a corpus that has been quietly poisoned, and the failure is
invisible from the device. The guard is now in both paths, and `>save` on a
`+` buffer says *"+out is output, not a document"* instead of reporting a write
that did not happen. `[VERIFIED]` — `+out` current and dirty, save and autosave
both produced no write and no mirror.

Worth naming as a pattern, because it is the second time on this device: **a
refusal that returns success is a refusal the caller cannot see.** `doc_save()`
returned `ESP_OK` after declining to write, which is correct for its caller's
purposes and was enough to make two separate callers announce a save that never
happened.

## Apps and extension

- **One exported symbol**, a pointer to a hierarchical const API struct in
  flash. Adding a function appends a pointer (minor bump, old apps unaffected
  because they never read past their known offset). Strictly simpler than a
  symbol table and it fits the ELF loader.
- **A manifest per app** — declared stack, category (so the launcher is a
  directory listing, with no registry to corrupt), API version, and a
  minimum-PSRAM field. ~80 bytes that buy refusing to run an app that would
  have faulted.
- **Native apps load as ELF from SD.** The vendor's own loader supports
  running from PSRAM — but instruction fetch from PSRAM is cache-backed and
  slow, and touching large PSRAM regions evicts cached flash. **Never run the
  keystroke-echo path, the display blitter or per-sample DSP from a loaded
  ELF.**
- **Declarative parameters.** An app declares `{type, id, name, range,
  action}` and gets its settings screen, persistence and MIDI CC mapping
  generated. The author writes zero UI code. The same declarations drive
  type-to-filter and palette entries.
- **Mods load at boot, outside the app lifecycle**, hooking startup/shutdown
  and app pre/post init. This is the extension point for things that are not
  apps: a global clock source, a key remapper, a status bar, and a Daemon
  connection that survives app switches.

## What this is deliberately not doing

Stated so they do not get relitigated:

- **No on-device speech recognition.** Settled above.
- **No TidalCycles/Strudel port.** Pattern-as-a-pure-function-of-a-timespan
  needs closures, rationals and a GC, and allocates per query at
  audio-adjacent rates. There is no C implementation and no embedded port.
  Orca gives most of the expressive payoff for a weekend of C.
- **No predictive-word bar.** The research on this is clear: keystroke savings
  do not become speed, because scanning the list eats the saving. That only
  reverses when a keystroke is eye-gaze-expensive.
- **No LVGL.** A retained-widget toolkit fights a 1-bit damage-list renderer.
- **No gutting the keyboard.** An earlier analysis recommended desoldering the
  Rii's radio and driving its matrix from an I²C scanner. That recommendation
  rested on the premise that the keyboard is Bluetooth-Classic-only, **which
  is false** — the owner's unit advertises BLE HID. The robustness argument
  survives on its own merits as a *preference*, but it is irreversible and no
  longer forced by anything. **Do not gut a working BLE keyboard.**

## Build order

1. ST7305 driver with windowed update and the HPM/LPM auto policy. Prove the
   damage path on hardware before anything is built on top of it.
2. Text grid, font pipeline, damage list, status bar.
3. BLE HID keyboard, the event taxonomy, Meta mode, sticky modifiers, and the
   physical pairing-recovery gesture.
4. Editor: gap buffer, selection-first grammar, undo.
5. Palette, which-key, guide file, plumber. **Everything by name from here on.**
6. Filters, and Daemon as the first interesting one.
7. Tier-0 voice capture to SD.
8. Clock, USB MIDI, then Orca.
9. App loader, manifest, crash survival.

Each step ends with something usable. Nothing after step 3 is needed to make
the device worth carrying.

## Open questions

| # | Question | How to close it |
|---|---|---|
| 1 | **How fast does the panel actually look?** SPI throughput, LC optical response (~23 Hz reported) and self-refresh are three different numbers. | Bench it. Everything about the feel depends on this. |
| 2 | **Tearing.** The TE pin is declared and unused in every implementation found. | Decide explicitly whether to solve it or to accept tearing. |
| 3 | **USB MIDI latency on this chip is unmeasured** — the quoted figure is a different device. | Loopback and measure before designing a performance around it. |
| 4 | **Battery life figures are all arithmetic**, not bench measurements. | A real cell and a current meter, before any number goes in a README. |
| 5 | **BLE keyboard cannot wake the device from light sleep** in the prior art — only a hardware button or timer. | Decide the idle model before committing to sleep-as-idle. |
| 6 | **Is the darkness a product failure?** There is no backlight and no FPC pin for one. | Decide whether an edge-lit guide off a spare GPIO is a board revision. |

## Provenance

Sections *What the hardware decides*, *Display* and *MIDI transports* derive
from HARDWARE.md, which cites vendor sources directly. The survey of prior art
behind the rest was assembled from public repositories and documentation; where
a claim is load-bearing it was re-verified against primary source or against
code on disk, and where it was not, it is tagged `[UNMEASURED]` or `[OPEN]`
above rather than asserted.

Two claims in that survey were **rejected on evidence**: that this keyboard
cannot do BLE HID (refuted by direct observation of the hardware), and that
USB host and the radio share a PHY (the documented sharing is between USB-OTG
and USB-Serial/JTAG; the risk is real but the stated mechanism is invented).

No vendor file is redistributed by this repository. See
[PROVENANCE.md](PROVENANCE.md).
