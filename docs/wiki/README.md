# The deck, top to bottom

*A reference for the whole instrument: every verb, every character of the language, every
picture, key, output and document. It also covers how each of them combines with the
rest, what the code does that the words around it do not, and what to build next. Written
2026-09-28 from the source at commit `85d1e6a`, plus the swing fix of the same day.
Updated 2026-09-29: `toggle`, `clear`, `map`, the re-run that removes, words in `< >`,
`>name = ch`, spin as a rate, the seventh view, and three findings on the deck.*

The deck is a handheld live-coding instrument. You type a line, press Ctrl+Enter, and on
the next sixteenth note the line is playing: a drum on a MIDI channel, a controller on a
synth, a circle on the screen. The clock has one of the chip's two cores to itself. Every
other job — the editor, the pictures, the radio — runs on the other core.

```
 keys ─► editor ─► a line ─► the dispatcher ─┬─► a verb         (37 of them)
 (BLE or cable)    (core 0)                  ├─► a definition   >kick = note 36
                                             └─► a lane         >kick x...x...
                                                   │ compiled once, on core 0
                                                   ▼
 the clock (core 1, 96 ticks a beat) ─► events ─► usb · din · ble · osc · mon
                                   └──► marks  ─► the picture (core 0) ─► panel · view node
```

## How to read it

Start with the page for the part you are touching. **Source is the truth.** Each claim
cites `file:line`. Three marks say how a claim was checked:

- **[sim]** or **[probe]** — the shipping code was compiled on a laptop and run;
- **inferred** — the claim was read from the code;
- **UNVERIFIED** — only the deck itself can settle it.

| page | what it covers | the one thing to know |
|---|---|---|
| [language.md](language.md) | how a line becomes a lane: every character of a step, groups, stacks, alternation, ties, odds, rates, counts; names, addresses, routes, inputs; what each kind of lane sends; every limit and every refusal, word for word | a digit means "how much", and what "how much" means depends on what the lane is bound to |
| [verbs.md](verbs.md) | the dispatcher, all 37 verbs in table order — forms, checks, what each prints, what each changes — and a table of which may run from `boot`, which write flash, reboot or use the radio | `boot` and `>run` run as the guide: no SYSTEM verbs, and no password prompt at boot |
| [pictures.md](pictures.md) | the sixteen primitives amount by amount, the pipeline and how `route` changes it, positions, frame and pane sizes, `>frame`, the view node's wire | the frame is rebuilt from nothing each step, so `move`, `spin` and `warp` act only on what `echo` carried |
| [editor.md](editor.md) | every key and chord, running a line, the status bar, cursor, playhead, wrapping, `+out`, the guide, the password prompt; the text grid, its faces and cache; the panel driver | Enter always inserts a new line; Ctrl+Enter runs the line |
| [outputs.md](outputs.md) | the six destinations and exactly what each sends; USB, serial, BLE and DIN; Wi-Fi, OSC in and out; the ensemble; SSH; the keyboard | every enabled destination sees the same stream, in one fixed order |
| [documents.md](documents.md) | buffers, the journal, scratch and named documents, autosave, the SD mirror, undo | autosave waits for the clock to stop; `>save` does not |
| [system.md](system.md) | the firmware map, the boot sequence, tasks and cores, memory, configuration, the hardware bill; **every flash write that can happen while playing**; the heartbeat and vitals | eighteen things can write flash while playing, and only autosave checks |
| [tools.md](tools.md) | CI, the nineteen host checks, every script, every document in `docs/` | CI checks everything but the firmware build |

## The words

| word | meaning |
|---|---|
| **lane** | one line playing: a name and a pattern, `>kick x...x...` |
| **step** | one character of a pattern with what is attached to it: a sixteenth at the top level |
| **slot** | the finest division a lane's pattern needs; a lane has at most 64 a cycle |
| **cycle** | one pass through a lane's pattern. The messages call it a "bar"; it is not sixteen steps |
| **binding** | what a lane drives: a drum (`note`), a voice, a controller (`cc`), a picture |
| **name** | a word defined to mean a binding: `>kick = note 36`. The names are my; 17 ship |
| **address** | `name[:instance][:part]` — `disc:2:x`, `bass:vel` |
| **part** | a lane that sets something about another: `:vel`, `:oct`, `:x`, `:y` |
| **count** | `!n` at the end of a line: play *n* cycles, then stop and say `name:end` |
| **route** | `>route follower source`: the follower fires when the source does |
| **sidechain** | a routed lane with no count — it fires only when its source fires |
| **cue** | a routed lane with a count — each trigger starts it for *n* cycles |
| **rank** | how many routes a lane is from one that follows nothing; lanes fire in rank order |
| **input** | a knob or a pad: `>knob1 = knob`, set over OSC, routed like a lane |
| **destination** | where events go: `usb din ble osc mon view` |
| **primitive** | one of the sixteen picture words, `disc` to `fold` |
| **tone** | one of nine ordered-dither levels, glyphs 128–136 |
| **frame / pane** | the picture the engine draws / the rectangle of screen it is shown in |
| **refusal** | a line the deck will not run: the character is boxed and the reason is on the status bar |

## The numbers

| | | |
|---|---|---|
| clock | 96 ticks a beat; a step is 24 ticks; 20–300 bpm | tick s.d. 4–5 µs, 6,175 of 6,175 within 100 µs with pictures running |
| language | 37 verbs, 16 lanes, 64 slots a cycle, 4 brackets deep, 160 leaves, 96 hits a lane | 32 names, 16 inputs, rates 1–32, counts 1–255, swing 50–75 % |
| pictures | 16 primitives, a frame of at most 60 × 24 cells | about 0.6 ms a frame for seven lanes; about 4 % of core 0 in the heaviest scene |
| screen | 400 × 300, one bit, reflective; the editor's grid is 30 × 12 or 60 × 24 | a cell costs 3.8 µs to draw; a full frame 4.75 ms to push |
| documents | 8 buffers, 24 named in the journal, 128 undo steps | a journal write stalls both cores 13–18.6 ms |
| USB MIDI | measured at the host | 0.03 ms jitter |

## The four laws

The code keeps all but the last.

1. **Pictures at the rate of the music** — a frame a step, never a frame a tick.
2. **A grid of tones, never free pixels** — work is per cell or per dot, not per pixel.
3. **Push by area** — the panel costs what changed, not what it shows.
4. **Nothing writes flash while it plays.**
