# Errata — where the code surprises, and where the words are wrong

*Part of [the deck, top to bottom](README.md). Snapshot: commit `85d1e6a`, 2026-09-28.
Five read-throughs of the source for this wiki each ended with a list of what did not
agree. This page is those lists merged, with duplicates removed and each item sorted by
what it costs a performer.*

**Evidence.** **[sim]** or **[probe]** means the shipping source was compiled on a laptop
and run. **Inferred** means read from the code, not run. **UNVERIFIED** means only the
deck can settle it. Nothing here was observed on the deck unless it says so.

---

## 1. What you would hear, or what you would lose

| # | what | evidence | where |
|---|---|---|---|
| 1 | **Swing drops notes in very fine subdivisions.** The "too fine for the clock" check ignores swing, but swing squeezes every second sixteenth to 24 − s ticks. When a sixteenth holds more slots than that, some land on the same tick and only one fires. `[xxxx] *4` loses 48 of 512 slots at swing 73 %; a 24-way split loses slots from swing 53 %. None of the three pieces comes close: their finest lane has 4 slots a sixteenth. **A regression from the swing change of 2026-09-26 — fixed 2026-09-28** and measured on the deck: 224 of 256 before, 256 after ([next.md](next.md) §1.0) | [sim], and reproduced separately | `seq_pattern.h:676-684`, `728-766` |
| 2 | **A note-off cuts a later note of the same pitch.** Offs are keyed by channel and note only, so a long gate or a tie ends the next strike of that pitch early: the boot `pad` (gate 420 ms) playing `0.0.` at 120 bpm sounds its second note for ~172 ms | [sim] | `seq.c:271-347` |
| 3 | **`>close` does not save first**, so everything edited while playing is lost; the comment at `main.c:997-998` says closing journals the document | read | `buffer.c:212-228` |
| 4 | **Every unnamed buffer journals under one name**, so only the last-saved scratch survives a reboot | inferred | [documents.md](documents.md) §1.3 |
| 5 | **Undo can restore a cut password.** The line cut after a password was typed on it is recorded in the undo log; Ctrl-Z puts it back and the next autosave stores it | inferred | [documents.md](documents.md) §1.7 |
| 6 | **Eighteen flash writers can run while playing**, and only autosave checks. The worst: the vitals record, every 60 s in USB-MIDI mode; `>save` and `>name`; a KEY tap (orientation) | read; the stall sizes are UNVERIFIED | [system.md](system.md) §7 |
| 7 | **DIN MIDI can block every destination.** `uart_write_bytes` waits when its 512-byte ring is full; the `midi` task serves all destinations in turn, so a burst on DIN would delay USB | read, from ESP-IDF source | `dinmidi.c:115-121` |
| 8 | **Unrouting a lane that `>route` created leaves its placeholder `x` playing every sixteenth** | [sim] | `seq.c:1428-1447` |
| 9 | **`>mute` or `>solo` with no names brings back a lane that finished its count**; it replays while `>lanes` says `done` and no playhead is drawn | [sim] | `builtins.c:2286-2293` |
| 10 | **A muted sidechain keeps its trigger** and fires once, off the grid, when unmuted | [sim] | `seq.c:591`, `603-608` |
| 11 | **A routed `:vel` or `:oct` part lags one note**: it ranks above its unrouted parent and fires after it | [sim] | `seq.c:586-596` |
| 12 | **A refused re-run of a voice line still resets its octave**, because the lane is re-bound before it is compiled; contradicts `seq.h:215-216` | [sim] | `builtins.c:725-731`, `seq.c:1404` |
| 13 | **Possible crash:** `>osc <ip>` before Wi-Fi or the ensemble has ever started calls `socket()` with no lwIP thread, and lwIP asserts are on | UNVERIFIED | `osc.c:78`; `net.c:76` |
| 14 | **Possible hangs:** the esptool reset and the USB trial revert still use `esp_restart()` in USB-MIDI mode, the path `>usb` was moved off because it deadlocks; `>flash now` too. None writes the vitals goodbye, so the next boot reads "STOPPED DEAD" | read; UNVERIFIED on hardware | `usbdev.c:340`, `372`; `builtins.c:1090-1118` |

## 2. Surprises — behaviour that is consistent but not what you would guess

**The language** ([language.md](language.md)):

- `disc:2:x` moves the same position as `disc:x`: positions belong to the primitive, not
  the instance (`viz.c:104-111`) [sim]. `lane_name.h:13` and `docs/VERBS.md:84-86` say
  otherwise.
- A sidechain still reads its own first event for degree, hold and direction, and one
  whose pattern is all rests never fires (`seq.c:597-610`) [sim]; `seq.h:283-287` says it
  ignores its steps.
- `>route disc kick ` with a trailing space fails with `a name is letters: conga`,
  because the source is the rest of the line (`builtins.c:1476-1487`) [sim].
- Phantom lanes: `>route x` on a lane that does not exist creates one that holds a slot
  (`builtins.c:1651-1663`).
- An input defined when the 32 name slots are full is created but cannot be removed
  (`builtins.c:832-866`). Inferred.
- Lanes `>route` creates have direction 0, not `d`, so a routed `turn` starts at 12
  o'clock (`seq.c:1132`). UNVERIFIED on screen.
- Drum velocity and controller value round digits 5–8 differently: 71/70, 85/84, 99/98,
  113/112 (`seq.c:423` vs `522`) [sim].
- Mode names match by prefix: `cmaj7` is C major and `dminor` is D minor; `chrom` alone is
  refused although `seq.h:240` lists it (`seq_scale.h:49-57`, `75-84`) [sim].
- OSC `T` is 1, not 127; `F` is a pad release; integer 1 is 1, float 1.0 is 127
  (`osc_parse.h:133-134`, `208-220`) [sim].
- `:1` is accepted as the plain name though the message says `:2 to :99`
  (`lane_name.h:116-127`) [sim].
- An input pressed while stopped fires on the first tick after `>play`; a knob republishes
  on every set, changed or not [sim].
- Silent caps: a tie holds at most 16 notes; the 96-hit limit counts every alternative
  [sim].
- Undocumented forms that compile: `,` inside `<>` stacks alternations; `%` on `.` and
  `_` does nothing [sim].
- "Bar" in messages means one cycle of the lane, not sixteen steps (`repeats in %d bars`,
  `_ holds it only some bars`).
- `>x = cc 74 gate 10` is refused with a hint that offers `gate` (`lane_name.h:248-255`).

**The pictures** ([pictures.md](pictures.md)):

- Unrouted `move`, `spin` and `warp` act only on what `echo` carried over, because they
  run before the shapes are drawn: `turn` with `spin` alone never rotates [probe].
- A route re-orders, re-times and re-amounts at once: `>route disc kick` puts the disc
  after every unrouted operator, so `fold`, `mask` and `edge` stop applying to it [probe].
- A step on which only a position lane fires draws an empty frame [probe]. Positions
  are not rescaled when the frame changes size, so a shape can sit off-screen [probe].
- Amounts that are not "more is more" [probe]: `move 9` and `x` shift 1 cell while 7 and
  8 shift 3; `fold` 8–9 equals 4–7; `warp` 1–2 does nothing; `thin` 1–9 are identical;
  `edge` 0 ≡ 1 and 8 ≡ 9; `grid` 0 ≡ 1 and 9 fills the frame; `flip 0` fills the frame;
  `disc` 0–4 draw one identical glyph in the default pane.
- `warp`'s phase is the tick modulo 16: at sixteenths it flips between two mirror images,
  at eighths it never moves; u ≡ d and l ≡ r [probe].
- `:x` and `:y` are accepted on all sixteen primitives and read by three — `disc`,
  `box`, `turn` (`viz.c:540`, `742`, `772`).
- **Twelve of the twenty-eight tiles are never drawn by anything**: 141–146 and 148–155
  (halves, square, diamond, ring, diagonals, arcs). The engine writes only the tones,
  the sparkles and the small disc, 147.
- The picture marks cross from core 1 to core 0 without a lock; one could be lost in a
  narrow window (`viz.c:1040-1041`). Never observed; UNVERIFIED.

**The editor** ([editor.md](editor.md)):

- **Stray boxes (a bug):** a refused character also boxes blank cells on the short rows
  above it; the test at `editor.c:612-613` lacks the end-of-row bound the playhead test
  has [probe].
- The status bar's line number counts wrapped rows, and restarts past 6,000 bytes: line
  701 showed as 601 [probe].
- ^K, ^A/^E and Home/End act on the wrapped row, not the line; End on a wrapped row lands
  on the next row [probe]. ^P/^N lose the goal column the arrows keep [probe].
- A line of exactly 30 characters is followed by a blank row — every `+out` rule [probe].
- ^L, ^J and ^G always jump to the top of the target document.
- Over the serial cable, Ctrl-J is Ctrl+Enter (it runs the line), Esc needs pressing
  twice, and Delete types `~` [probe].
- The prompt hint `type it, Enter. Esc stops` is never visible [probe].
- **Tab inserts two spaces whatever the modifiers**; a literal tab in a document draws
  as `?` [probe].

**The verbs** ([verbs.md](verbs.md)):

- `>density` in the boot document never takes effect: `editor_init()` runs after it and
  sets low (`main.c:695`, `704`).
- `>wifi`, `>host` and `>ssh` in the boot document report `nothing here can ask` — the
  prompt does not exist yet — so a boot document cannot start a hosted network, though
  `builtins.c:1469-1475` says it can bring the deck onto one.
- `>usb on` in the boot document is refused (`usb` is SYSTEM, `boot` is GUIDE), though the
  verb's own output, `usbdev.c:80-83` and `docs/TESTING.md:533-534` suggest it.
- Capability labels leak: `battery use` is READ but writes NVS; `sync lead` is EDIT but
  starts the radio; `run` gives its lines GUIDE authority, so an agent could reach NET
  verbs through a document; `wifi forget` and `ssh forget` erase NVS under NET. Nothing
  calls as `CMD_BY_AGENT`.
- `>frame` reports `sent a N-byte frame` when the send failed or the frame was too large
  (`builtins.c:1710-1716`).
- There is no `osc off`: `>osc off` closes the socket, prints `'off' is not an address`,
  and leaves the destination on. `>send osc off` is the way.
- `no passwords on a line - cut` is true only for hand-run lines: through `>run` or `boot`
  the password stays in the document.
- `>run` counts prose lines and comments as "ran". `>open 12` opens slot 1, and a
  document whose name starts with a digit cannot be opened by name.
- `>route` with all 16 lanes in use reports `no lane … to drive`.
- `>send <unknown>` reports "off" rather than an error.
- `>play` while playing restarts from the top; `>play` and `>bpm N` clear the `>jitter`
  statistics.
- `sync lead`/`follow` switch Wi-Fi to station mode, which would end a `>host` network;
  `sync alone` leaves Wi-Fi up with power save off. Inferred.
- `cmd_run_line` dereferences the line before its NULL check (`cmd.c:229`, `237`).

**The outputs** ([outputs.md](outputs.md)):

- OSC out concatenates a tick's messages into one datagram without the `#bundle`
  wrapper OSC 1.0 requires; only the deck's own parser and `tools/osc_listen.py` are
  known to read it.
- USB sends three bytes for every status, so program change and aftertouch would be the
  wrong length; the test that says USB, BLE and DIN agree checks DIN only
  (`usbdev.c:251-252`; `tools/test_midi_wire.c:4-5`). Nothing the deck emits today is
  affected.
- The SSH host key is kept even when the login fails; `ssh_status` stays
  `ssh: connecting` after several failures.
- `>wifi off` does not stop the rejoin at the next boot; only `>wifi forget` does.
- The `din` refusal list omits the SD card's pins.

## 3. Words that disagree with the code

**Documents:**

- `docs/COMMANDS.md`: `ble` is on at boot (it is off, `seq.c:1607`); plain `>flash` (it
  needs `>flash now`); an `out` verb (not in the table); "no moment where work exists but
  is not yet safe" (§1 of this page); orientation listed under SYSTEM; a verb that turns a
  guide back into prose (gone).
- `docs/VERBS.md:43-44`: re-running a line mutes it — only while playing, and only by
  hand. `docs/VERBS.md:33`: `<a b>` alternates "each bar" — each cycle of the lane.
- `docs/OS.md:369` calls USB MIDI the default; the power-on default is serial.
- `docs/NETWORK.md:412-415` gives the version-2 ensemble protocol; `:482-483` gives a
  26-byte packet (it is 27); `:673-674` cites `OS.md` for 49.6 clocks/s, which `OS.md`
  does not have.
- `docs/TESTING.md`: Ctrl-J for navigation (cannot be typed over the cable); `>lanes`
  shows the compiled form (it shows the text); `/deck/step` is "the bar position" (it is
  the step modulo 128); SSH "never run against a real server" at `:554` against `:412`.
- `README.md:110`: `>frame` sends the document — it sends the picture when one is live.
  `README.md` and `tools/zine_refs.py` called the zine "eight pages"; it is sixteen —
  corrected with this wiki.
- Check counts: 113 (`docs/ASSEMBLY.md:6`), 118 (`export/stl/README.md:5`), 119
  (`README.md:25`, `validation.json`).
- `docs/CONCRETE.md:4` says `variant = 3`; `cad/parameters.scad:1476` allows 1 or 2.
  `parameters.scad:1168` says `check_golden.py` runs "on every run"; nothing runs it.
- `tools/requirements.txt` omits Pillow and pyserial, which the zine, CMF, mock-up and
  relay tools need. `tools/corpus/strudel.txt:1` begins with Strudel's console banner.
- `STATUS.md:81` says the journal wraps; it no longer does. `firmware/README.md` lists
  4 of 14 components.
- `docs/GRAPHICS.md` §5 gave the picture pane wrong; corrected there 2026-09-28.

**Comments in the firmware** (each is stale, not wrong in effect):

- `seq.h`: lanes compile "to a bitmask" (`:21`); alternation lays the pattern "once per
  cycle" (`:40-50`); 24 PPQN (`:57-63`); "eight lanes" (`:319`; also `seq.c:184`,
  `osc.c:41-42`).
- `seq.c:731-732` and `:962-963`: the sixteenth at 120 bpm is 124,992 µs, not 125,000;
  at 124 bpm 120,960, not 120,967.
- `viz.h:123` "thirteen primitives" (sixteen); `viz.c:61`, `1044` cite `order_marks()`
  (it is `chain_ranks()`); `viz.c:77-79` says a second mark overwrites the first (both
  draw); `viz.c:169`, `main.c:846` cite `viz_tick` (`viz_mark`); `viz.c:309-331` lists
  `tile` and `>viz`; `viz.h:203` says `viz_frame` can return 0.
- `editor.c:4-22`, `main.c:704`: a 30 × 10 layout (it is 30 × 11 and a status row);
  `editor.c:284-285`, `551-553`: pixel-row status bar, "inverse word"; `editor.c:164-165`:
  density level 1 is high (a typed `1` selects low).
- `docstore.h:45`: Enter executes in a guide; `docstore.h:109`: the mirror is
  `notes.txt` (it is one file per document); `docstore.h:9`, `main.c:960` cite
  `docs/HANDOFF.md` (it is `./HANDOFF.md`); `journal.c:6` omits `kind` and `reserved`.
- `builtins.c:1533-1544`: the old SSH design; `:1558-1562`: `split` in columns, "a
  third"; `:2178-2181`: a binding column in `>lanes`.
- `main.c:245-251`: the static assert counts five destinations; six register.
- `ensemble.c:14` "twenty bytes" (27); `:98-101` every packet broadcast (probes and
  replies are unicast); `:863` 10 probes a second (20).
- `blemidi.c:263-266`: five messages in 20 bytes (four); `ble_kbd.c:1099-1100`: the
  peripheral role is compiled out (it advertises); `kbd.h:111-113`: a weak default
  `kbd_on_passkey` (none).
- `dinmidi.c:116-121`: "NEVER BLOCK" (it can, §1 #7); `usbdev.c:17`: an ARMED state that
  does not exist; `midi_len.h:10-13`: the length rule "exists once" (three copies).
- `sdkconfig.defaults:132-135`: one esp_timer callback (four live, one dead).

## 4. Dead code

`c_out` and `c_usbtest` with its timer (compiled, not in the verb table);
`usbmux_try_*`; `seq_set_hooks`; `seq_nudge`; `net_osc_step`; `usbdev_packing`;
`blemidi_enabled`; `tg_draw_text_px`, `tg_text_width_px`; the panel's power-policy and
inversion setters; `s_status_shown`, `s_chrome_dirty`; the blank-columns loop at
`editor.c:633-635`; `BEAT_EVERY_US`. The circular component dependency — `cmd` requires
`main` — is not dead, but it is backwards (`cmd/CMakeLists.txt:3`).

## 5. What only the deck can settle

The stall each NVS write causes; DIN blocking under a burst; OSC datagram overflow; the
hangs in USB-MIDI mode; `>osc` before lwIP is up; the ensemble dropping a hosted network;
what the Wi-Fi driver and the NimBLE store write to NVS; `>din` on the SD, USB or flash
pins; third-party OSC receivers reading concatenated messages; the journal cases in §1
#4–5; the doc-only figures (0.03 ms USB jitter, BLE 7.5 ± 1.8 ms, 49.6 clocks/s); and
everything in the picture tables that is marked "on screen".
