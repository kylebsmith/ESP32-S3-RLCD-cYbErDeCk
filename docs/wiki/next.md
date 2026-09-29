# Next — the best next steps, in order

*Part of [the deck, top to bottom](README.md). 2026-09-28. A brief, like
[NEXT.md](../NEXT.md) before it, and ordered the same way: the early steps protect
what the later ones stand on. Every step says what would prove it done.*

**The aim.** The most capable creative toolbox that fits in a hand does not come from
the most features on the deck. It comes from **one grammar driving everything around
it**: the deck is the brain; satellites are its hands; the view node is its eye; a
synth is its voice; laptops, phones and other decks are its friends. Each new thing is
a **destination** or an **input** — never a second language. What follows keeps the
brain deterministic, makes it faster to play, gives the pictures their own resolution,
and then grows the ecosystem.

## The five, if only five

1. **Stop the flash writes that happen while playing** (§1.1–1.2), measured on the deck
   before and after — as the swing fix (§1.0) now has been.
2. **Put the deck in front of five to eight people** (§2) before adding anything.
3. **Tab completion, and the two open editor decisions** (§3) — hand speed for
   beginners, no new verbs.
4. **Pictures on square 4 × 4 dots, and type as a picture** (§4) — adopted only on a
   number from the deck.
5. **The satellites and a direct link to the view node** (§5) — the hands and the
   eye, on hardware the owner is building.

---

## 1. Keep the promise: nothing the deck does may move a note

The clock's record is the instrument's foundation: tick standard deviation 4–5 µs,
6175 of 6175 ticks within 100 µs with pictures and the view running
([GRAPHICS.md](../GRAPHICS.md) §1–2). What could still move a note is not drawing. It
is flash, blocking I/O on the output task, and code the tick fetches from flash — and,
as it turned out, one arithmetic mistake.

### 1.0 Swing lost notes in the finest rolls — fixed, and measured on the deck

The swing change of 2026-09-26 squeezes the second sixteenth of each eighth to 24 − s
ticks. A lane with more slots in a sixteenth than that put two on one tick, and only
one sounded: `[xxxx] *4` lost 48 of 512 at swing 73 %, and a 24-way split lost slots from
swing 53 % ([errata.md](errata.md) §1 #1). None of the three pieces is affected; their
finest lane has 4 slots a sixteenth.

- **The fix** (`seq_pattern.h`, `seq_pattern_swung`): **a lane swings only as far as its
  slots allow.** A lane with *fit* slots a sixteenth keeps at most 24 − *fit* ticks of
  swing. Every lane with 12 or fewer slots a sixteenth — everything the pieces, the guide
  and the Strudel corpus contain — is exactly as it was.
- **The check** (`tools/test_seq_pattern.c` §14 f–g) walks every subdivision the clock
  accepts at every swing from 50 to 75 and fails on the old header. All nineteen host
  checks pass with the fix.
- **Measured on the deck, 2026-09-28**, with `>cut [9999999999999999] !16` — sixteen
  passes of sixteen controller messages — at 60 bpm into `>send mon on`, counting what
  arrived:

  | | old firmware (`67f6439`) | fixed (`2c16aa9`) | predicted on the host |
  |---|---|---|---|
  | swing 67 % | 256 of 256 | — | 256 |
  | swing 75 % | **224 of 256** | **256 of 256** | 224, then 256 |

  A plain sixteenth lane at swing 67 % still alternates 333 ms and 167 ms at 60 bpm —
  the same 8-tick offbeat as before the fix. Nothing was dropped from the queue.

### 1.1 The vitals record is written while playing — in the default transport

`vitals_loop()` writes an NVS record **once a minute in USB-MIDI mode, whether or not
the clock is running** (vitals.c:87-100, called from main.c:860). A flash write stops
the caches of both cores. A journal write was measured at 13–18.6 ms, which is why
documents are not saved while playing (main.c:984-1002). USB MIDI is the native
transport ([README](../../README.md)), so this runs on the default path.

- **Measure first, per the owner's rule:** a `>jitter` capture across several minutes
  of play in USB-MIDI mode. Look for a late tick at the 60-second marks.
- **Then choose, because it is a real trade.** The record exists to diagnose a
  USB-MIDI hang, and a hang cured by unplugging leaves only what is in flash. Either
  skip the write while the transport runs, or keep the running record in RTC memory
  and commit it on stop and on a deliberate restart.
- **Done when** a check of the write predicate fails on today's code, and the capture
  shows no late tick at the minute.

### 1.2 Every other flash writer reachable while playing

The table is now written: **eighteen writers, and only autosave checks the transport**
([system.md](system.md) §7). The ones a performer can hit in a set:

- **`>save` and `>name`** write the journal at once, even while playing — a stall of
  13–18.6 ms each, and `>run` of a piece can carry them;
- **a tap of the KEY button** saves the orientation;
- **a keyboard reconnect** writes its address — and the link is rebuilt after 60 s
  without a key, which is likely mid-set (NVS skips an unchanged value);
- **`>usb` and `>flash now`** save every changed document *before* they stop the clock.

The rest — `>wifi`, `>ssh`, `>battery use`, `>kbd forget`, pairing — are rare in a set,
and none is gated either. **Policy:** a write asked for while playing is deferred to the
stop, and the status bar says so; a command that reboots stops the clock first. **Done
when** every row of that table has its gate, the table is in [OS.md](../OS.md), and the
four laws are written there too:

1. pictures at the rate of the music;
2. a grid of tones, never free pixels;
3. push by area;
4. nothing writes flash while it plays.

### 1.3 The DIN output can block every destination

`dinmidi_send()` says it never blocks. But `uart_write_bytes()` waits with
`portMAX_DELAY` when the ring is full (dinmidi.c:115-121). The `midi` task drains
every destination in turn (seq.c:193-241), so a burst on DIN would delay USB. **Measure**
a dense DIN burst against USB timestamps at the host. **Then** write non-blocking and
count drops, as the other destinations do.

### 1.4 Where the tick's code lives

The tick runs from flash unless placed in IRAM. A cache miss on core 1 is a stall.
**Measure** the tick while core 0 does flash-heavy work before moving anything. IRAM is
full on the dedicated region and 64 % used overall ([system.md](system.md) §4.1).

### 1.5 The build, in CI

CI checks nineteen host tests, the fonts, the documents and the CAD, but **never
compiles the firmware** ([tools.md](tools.md) §1.1). Add a job in the
`espressif/idf:v5.5.4` container. It is the cheapest insurance on the list.

### 1.6 Work that can be lost

Three ways to lose what was typed, all from the code, none yet reproduced
([errata.md](errata.md) §1):

- **`>close` does not save first**, so edits made while playing are gone;
- **every unnamed buffer journals under one name**, so only the last-saved scratch
  survives a reboot;
- **undo can restore a password** that was cut from a line, and the next autosave
  stores it.

Reproduce each on the deck, then fix it with a check that fails on the old code. The
password one comes first, because the documents mirror to the SD card and leave the
device ([NEXT.md](../NEXT.md) §1, rule 6).

### 1.7 Small defects the reference turned up

These are listed in [errata.md](errata.md), each with its source line:

- a `>density` in the boot document that boot then overwrites;
- a `>usb on` the boot document is invited to carry but may not run;
- boxes drawn on blank cells above a refused character;
- a `>frame` that reports success when the send failed;
- a destination-count guard that counts five of six;
- three implementations of a MIDI length that "exists once";
- a note-off that cuts a later note of the same pitch;
- a routed lane whose placeholder plays every sixteenth once it is unrouted.

Each fix comes with a check that fails on the old code.

---

## 2. Test with people before building more

[GRAPHICS.md](../GRAPHICS.md) §9 has the protocol:

- five to eight people, each alone with the deck and the zine, thinking aloud;
- measured: time to first sound, to first variation, to first picture and to the
  first recovery from a refusal; refusals per task; the ideas people reach for;
- small rounds, changing the deck between them.

Two things to add for this round:

- **A session log on the console**, never in flash: every run, refusal and Tab press,
  with a time. The numbers then come from the deck, not from notes.
- **The research question as a hypothesis.** Offer half the testers a named word and
  half a parameterised form for the same picture, and count errors and time
  ([research.md](research.md) §1). The literature predicts a trade; only the deck's own
  users can say where it falls here.

**This gates §3–§4.** A feature the tests do not ask for waits.

---

## 3. Hand speed

1. **Tab completion** — [completion.md](completion.md). Words come from the deck's own
   tables. Tab cycles; Esc puts back what you typed; any other key keeps the word and
   does what it always does, so Enter still means a new line. The status bar shows the
   word's one-line help. There is no new verb and no cost on the clock's core.
2. **The two open decisions** from 2026-09-26, the owner's to make: a gesture that
   clears the lanes but keeps the page, and a *run-and-move-to-the-next-line* chord,
   because a wrapped line run top to bottom is run twice today and the second run
   silences it.
3. **Euclid, `x(3,8)`** — the largest piece of Strudel's notation not yet here
   ([MAP.md](../MAP.md) §9.2). The objection on record is that `(3,8)` is a rhythm you
   cannot see. The playhead answers half of that, since it shows each hit as it falls.
   Decide with the testers.

## 4. Pictures get their own pixels; type becomes a picture

[pictures-and-type.md](pictures-and-type.md) has the mock-ups, drawn by the engine.
In the default face the picture pane is 28 × 4 cells, so a disc is drawn as a
rectangle. The proposal:

1. **4 × 4 square dots, banded** — decided 2026-09-28. Independent of the text size: 84 ×
   24 in the default pane, 90 × 72 across the whole text area. A dot is one period of the
   tone matrix and exactly two framebuffer bytes, so banding is a 13 KB lookup kept in
   RAM. **Adopt on a number**: frame time on the deck, jitter unchanged, push unchanged.
   The screen is Bayer, decided 2026-09-28. The view node can do far more with the same
   frames (scan, phosphor, feedback, riso, a live poster), mocked in
   [pictures-and-type.md](pictures-and-type.md).
2. **`stamp`** — the picture says what played: a routed stamp shows its source's name,
   at the source's strength.
3. **The type overhaul**, decided by the reading test in [CMF.md](../CMF.md): errors per
   hundred characters, by glyph, in daylight and indoors.

## 5. The ecosystem — hands, eye, voice, friends

| | what | the deck side | status |
|---|---|---|---|
| **hands** | satellites: 4 encoders, 8 buttons, on the six-pin magnetic cable, CAN — and a destination too, so lanes can light their LEDs, as monome's grid decouples its lights from its keys | `>knob1 = knob`, `>pad1 = pad`, routes (exist) | briefed ([SATELLITES.md](../SATELLITES.md)); the owner builds |
| **eye** | the RP2040 view node on a direct USB tether, drawing the dots banded at its own resolution, with **frame interpolation as a switch**; later wireless — an RP2040 with an ESP32-S3 companion, or a faster link — joining the ensemble as a follower | the `view` destination (exists): each frame carries its tick | relayed through a laptop today ([VIEW.md](../VIEW.md)) |
| **ears** | MIDI in: clock follow, notes and CCs as inputs | inputs exist; MIDI in needs an optocoupler | open ([NEXT.md](../NEXT.md) §2) |
| **voice** | a sound node (ESP32-P4 or Teensy) | a destination like any other | the owner's hardware roadmap; sound stays off the S3 |
| **friends** | laptops and phones over OSC; other decks over ESP-NOW | OSC in and out; `>sync` (exist) | done |
| **memory** | documents as the exchange format: pieces, zines, sets | the SD mirror (exists) | a pieces library and zine #1 are open |

**What makes it an ecosystem rather than a pile of gadgets:** every node joins the
way a deck does, says what it is, and is addressed by name in the one lane grammar.
Unplugging a satellite loses nothing, because the value lives on the deck.

## 6. The paper

The thesis is already written down ([THESIS.md](../THESIS.md)): hardware co-locates the
gesture and the result, so an audience gets causality without being asked to read code.
What the paper still needs:

| | status |
|---|---|
| timing: tick jitter, USB-MIDI jitter at the host, drawing cannot move the clock | measured |
| a note is not late under load, including the flash writers in §1 | §1, then measured |
| time to first sound against a baseline system | §2 |
| an audience study of causality: deck with satellites against a laptop with projected code | not designed |
| pictures and type: frame cost and legibility, before and after | §4 |

## 7. What not to do

Refusals keep the ecosystem small enough to hold in a hand:

- **Combinators** such as `every` and `jux` ([MAP.md](../MAP.md) §9.0, §9.4);
- **menus** — completion is not one ([completion.md](completion.md) §6);
- **layers or a second picture pane**;
- **colour**;
- **sound synthesis on the S3**;
- **a second routing system beside `route`**;
- **any feature that costs battery while idle**, unless it is a command.

**A verb count that goes up needs a verb deleted** ([NEXT.md](../NEXT.md) §13).
