# Combinations — how every part meets every other

*Part of [the deck, top to bottom](README.md). Snapshot: commit `85d1e6a`, 2026-09-28, plus
the swing fix of the same day. The deck has few parts. Its power, and nearly all of its
surprises, are in how they combine. Each table below crosses two kinds of part and says
what happens in every cell. The detail and the source lines are on the page each table
names; anything marked ✱ is a surprise, and [errata.md](errata.md) has it.*

---

## 1. A line, and who runs it

A line is one of five things, and three things can run it
([verbs.md](verbs.md) §1, [language.md](language.md) §1).

| the line | Ctrl+Enter (**hands**) | the `boot` document at startup (**guide**) | `>run <doc>` (**guide**) |
|---|---|---|---|
| prose — no `>` | `not a command - start with >` | skipped | counted as "ran" ✱ |
| `>` alone, or `>#` | nothing | nothing | counted as "ran" ✱ |
| a verb | runs; every capability | READ, EDIT, STORE, NET only: `send usb din kbd flash` are refused | the same as boot |
| a verb that asks a password (`wifi host ssh`) | the prompt opens | `nothing here can ask` ✱ — the prompt does not exist yet | the prompt opens |
| a password typed on the line | refused, and the line is cut | refused, **not cut** ✱ | refused, **not cut** ✱ |
| a definition `>x = …` | defines; re-binds live lanes | defines | defines |
| a lane `>kick x...` | compiles — or, if unchanged and playing, **mutes** (the re-run toggle) | compiles | compiles; never toggles |
| `>density high` | takes effect | overwritten by the editor's start ✱ | takes effect |

## 2. A mark, and another mark — the step grammar

What composes inside one pattern ([language.md](language.md) §2):

| | with `%NN` odds | with `_` tie | inside `[ ]` group | inside `[a,b]` stack | inside `< >` alternation |
|---|---|---|---|---|---|
| **a hit** `x` `0-9` `u d l r` | one roll per slot per lane, shared by every note on that slot | the tie extends it | shares the group's width equally | sounds with the other members | plays in its turn |
| **a rest** `.` | compiles, does nothing | the tie acts as a rest | as a hit | as a hit | as a hit |
| **a tie** `_` | compiles, does nothing | ties chain: `0__` holds three slots | holds the note before the group: `x[_x]` | `[03,4_]` holds only the 4 | `<0 3>_` holds whichever played; `0<_ .>` is refused — a tie that holds only some bars |
| **a group** `[ab]` | multiplies into every leaf inside | holds its last item | nests, to depth 4 | a member is a sequence: `[02,45]` is 0+4, then 2+5 | an alternative can be a group |
| **a stack** `[a,b]` | multiplies | holds every member's tail, up to 16 notes (silently) ✱ | nests | stacks in stacks | `,` inside `< >` stacks alternations ✱ (undocumented) |
| **an alternation** `<ab>` | multiplies | holds whichever played | nests | as a member | nested ones advance only when chosen: `<0 <1 2>>` is 0, 1, 0, 2 |

**At the end of a line**: one rate (`/n` or `*n`, 1–32) and one count (`!n`, 1–255), in
any order. `/2 *3` and `!2 !3` are refused. A direction written first (`u 4.4.`) is the
lane's; a step's own `u d l r` wins over it.

## 3. A mark, and the kind of lane it is on

The same characters mean different things on different bindings
([language.md](language.md) §7):

| mark | drum (`note`) | voice | controller (`cc`) | picture | picture `:x` `:y` | sound `:vel` | sound `:oct` |
|---|---|---|---|---|---|---|---|
| digit *d* | velocity (d·127+4)/9; `0` is 1, never silence | degree *d* in the key, climbing octaves past the mode; velocity is the lane's level | value d·127/9 ✱ rounds unlike velocity | amount *d* | position *d* of 0–9 across the frame | the parent's level from here on | the parent's octave, max 8 |
| `x` | the lane's level (100 unless `:vel`) | the root, degree 0 | **nothing is sent** — the controller holds | amount 9 | 9: the far edge | 100 | the name's octave |
| `.` | rest | rest | rest | nothing drawn | rest | rest | rest |
| `_` | holds the note: gate + tie length | holds | no note to hold — silent (inferred) | nothing to hold (inferred) | — | — | — |
| `u d l r` | refused: `u d l r: move warp ramp turn` | refused | refused | a direction on `move warp ramp turn`; refused on the other twelve | refused | refused | refused |
| `[a,b]` chord | two notes at once | a chord of degrees | two values at once; the receiver keeps the last | the primitive draws twice | the last mark wins | — | — |
| `%NN` | per slot, one roll per lane | the same | the same | the same | the same | the same | the same |

**Positions** belong to the primitive, not the lane: `disc`, `disc:2` and `disc:2:x` share
one ✱, and only `disc`, `box` and `turn` read them ([pictures.md](pictures.md) §1.6).

## 4. A lane, and another lane — routes

`>route <follower> <source>` ([language.md](language.md) §9).

### 4.1 Counted, routed: the four kinds of lane

| | not routed | routed |
|---|---|---|
| **no count** | plays its pattern for ever | a **sidechain**: fires only when its source fires, at its source's value; its own timing, odds and alternation are ignored, but it still reads its first event for degree and hold ✱ |
| **`!n`** | waits for its own downbeat, plays *n* cycles, mutes, and publishes `name:end` | a **cue**: every trigger from its source (re)starts it for *n* cycles, then it publishes `name:end` and waits |

`name:end` is a source like any other, so `>route b a:end` chains sections: when `a` ends,
`b` starts — sequences of sections with no new verb.

### 4.2 What value the follower receives

The source publishes one number, 0–127; the follower scales it to what it can use
([language.md](language.md) §7):

| source ↓ publishes | → drum | → voice | → cc | → picture | → picture `:x`/`:y` | → `:vel` / `:oct` |
|---|---|---|---|---|---|---|
| drum: its velocity | velocity *v* | its own first degree, velocity *v* | value *v* | amount (v·9+63)/127 | position, the same | the same scale |
| voice: its velocity | as above | as above | as above | as above | as above | as above |
| cc: the value sent | as above | as above | as above | as above | as above | as above |
| picture or part: amount·127/9 | as above | as above | as above | as above | as above | as above |
| `name:end`: 127 | full velocity | velocity 127 | 127 | amount 9 | 9 | 9 |
| knob or pad: its value | as above | as above | as above | as above | as above | as above |

### 4.3 A route, and everything else about a lane

| with | what happens |
|---|---|
| **odds** | a sidechain ignores them |
| **alternation** | a sidechain ignores it; a cue restarts it with each section |
| **swing** | a sidechain fires when its source fires, so it takes its source's groove |
| **mute** | a muted sidechain keeps its trigger and fires once, off the grid, when unmuted ✱ |
| **parts** | within one rank, parts fire before lanes, so `>bass:vel` and `>bass` agree on step one; a *routed* part is a rank higher and lags one note ✱ |
| **ranks** | rank = route hops from a lane that follows nothing; a tick runs rank 0 parts, rank 0 lanes, rank 1 parts … up to 16 hops; a routed lane fires on the same tick as its source |
| **pictures** | a route also moves the picture later in the draw, after every unrouted operator — so `fold`, `mask` and `edge` stop applying to a disc routed from a kick ✱ |
| **removing the source** | followers keep their route and fall silent; they wake when a lane of that name returns |
| **unrouting** | the lane plays its own pattern again — for a lane `>route` created, a placeholder `x` on every sixteenth ✱ |
| **itself** | refused: `disc cannot follow itself`; longer loops are not refused |

## 5. A lane, and a command that changes state

| command | plain lane | counted lane | routed lane | muted lane | picture |
|---|---|---|---|---|---|
| **the same line again**, by hand, playing | mutes it (`kick silent`) | mutes it | mutes it | runs, and unmutes | mutes it |
| **a changed line** | recompiles on the next tick; keeps its phase from play | re-arms: waits for its downbeat | recompiles; still routed | runs, and unmutes | recompiles; the split turns on |
| **the bare name** `>kick` | removed; a sounding note still gets its off | removed | removed; its own route cleared | removed | removed |
| `>mute kick` | skipped entirely: no events, no trigger used | neither starts nor finishes; its count keeps time | keeps its trigger ✱ | — | stops drawing |
| `>mute` or `>solo`, no names | unmuted | a finished count is unmuted and replays while it says `done` ✱ | unmuted | unmuted | unmuted |
| `>play` | from the top | re-armed; a finished one un-finished and unmuted — hand mutes stay | re-armed | hand mutes stay | — |
| `>stop` | kept; offs sent, then CC 123 on all 16 channels | kept | kept | kept | the last frame stays |
| `>panic` | as stop, with the offs and CC 123 sent twice | | | | |
| `>new` | **forgotten** — every lane | forgotten | forgotten | forgotten | the picture is blanked; positions reset |
| `>open`, Ctrl-L/J, `>close`, `>run` | untouched: the lanes keep playing | | | | |
| `>kick =` (remove the name) | every lane of that base is forgotten | | followers of it keep their route, silent | | |
| `>kick = note 35` (redefine) | re-bound while playing | | | | an alias re-binds too |
| `>bass = voice 3` (redefine a voice) | re-bound; its octave resets to the name's, undoing a `:oct` part until its next event ✱ | | | | |
| `>bpm N` | keeps the musical position while playing; resets the tick when stopped; clears `>jitter` | counts keep their origin | | | |
| `>scale` | voices transpose at their next note; drums, cc and pictures unaffected | | | | |
| `>swing` | the offbeat sixteenth moves; see §6 | | | | |

## 6. Time, and time

| | what happens |
|---|---|
| **swing × subdivision** | the offbeat sixteenth of every eighth starts *s* ticks late and is squeezed to 24 − *s*; everything inside a sixteenth goes with it. A lane with more slots in a sixteenth than the squeeze leaves room for swings only as far as its slots allow, so none is lost — the fix of 2026-09-28 ([next.md](next.md) §1.0) |
| **swing × rate** | eighths do not swing however they are spelled: `x.x.x.x.x.x.x.x.` and `xxxxxxxx /2` are the same eighths |
| **swing × inputs** | a pad fires on the next *swung* sixteenth; a knob on the next tick |
| **swing × the playhead** | the playhead moves on the grid, not the groove |
| **swing × pictures** | a picture lane fires at swung times like any lane, and its frame is drawn when marked |
| **swing × the clock out** | MIDI clock and the step marker are tick-based and never swing |
| **alternation × count** | alternation counts the lane's own passes, so a cue's `<0 2 4>` restarts with every section |
| **rate × count** | `!n` counts cycles of the lane at its own rate |
| **tie × cycle** | a tie never carries across the end of the cycle |
| **gate × tempo** | a gate is milliseconds; a tie is converted to time at the tempo when the note starts |
| **gate × the same pitch** | a note's off can cut the next strike of the same pitch short ✱ |
| **tempo × ensemble** | a follower takes the leader's tempo at the next correction, and waits up to a second after `>play` for its count |
| **a lane written mid-play** | joins at the phase counted from play: `123` written at sixteenth 5 starts on its `3`. A counted lane waits for its own downbeat instead |

## 7. A picture, and another picture

The frame is rebuilt from nothing on every step that marks anything, in this order
([pictures.md](pictures.md) §1.3):

```
echo move spin warp  →  noise disc box turn ramp grid  →  mask edge  →  grow thin flip  →  fold
history and motion       fields: what to draw             levels        shaping             repetition
```

So each operator acts on what the stages before it drew, and the classic pairs follow:

| pair | result |
|---|---|
| `disc` + `mask` | a hard-edged disc, its size set by the level |
| `disc` + `edge` | a ring |
| `ramp` + `edge` | contour lines |
| `noise` + `mask` | a few bright stars |
| `echo` + any field | trails: the last frame, a tone fainter |
| `echo` + `move` | trails that drift; `move` alone moves only what `echo` carried ✱ |
| `echo` + `spin` + `turn` | the radar of `orbitals`; `spin` without `echo` rotates nothing ✱ |
| any + `flip` | the negative; `flip 0` on an empty frame fills it ✱ |
| `turn` + `fold` | a symmetric fan |
| a routed field | drawn after every unrouted operator — out of their reach ✱ |
| `disc` + `disc:2` | two discs drawn separately — at one shared position ✱ |
| a position lane alone on a step | an empty frame ✱ |

## 8. An event, and an output

Every enabled destination sees the same stream, in registration order `ble mon din osc
view usb` ([outputs.md](outputs.md) §1.2):

| event | `ble` | `mon` | `din` | `osc` | `usb` | `view` |
|---|---|---|---|---|---|---|
| note on | ✓ timestamped | a console line | ✓ | `/deck/<lane> ,ii note vel` | ✓ | — |
| note off | ✓ | a console line | ✓ | not sent | ✓ | — |
| controller | ✓ | a console line (not CC 123) | ✓ | `/deck/<lane> ,ii cc val` | ✓ | — |
| CC 123 × 16 on stop | ✓ | not printed | ✓ | sixteen `/deck/cc` (inferred) | ✓ | — |
| clock `F8`, start, stop, position | ✓ with `>sync on` | not printed | ✓ | not sent | ✓ | — |
| the step marker | dropped | not printed | dropped | `/deck/step ,i n` (mod 128) | dropped | — |
| a picture frame | — | — | — | only by `>frame`, as text | — | `DKV1` cells, once for every step on which a picture lane fires |

One tick's OSC messages go out as one datagram, concatenated without `#bundle` ✱.

## 9. Editing, and playing

| while playing | what happens |
|---|---|
| typing | autosave waits for `>stop` — a journal write stalls the clock 13–18.6 ms |
| `>save`, `>name` | **write at once**, stall and all ✱ ([system.md](system.md) §7) |
| `>close` | the buffer goes, unsaved edits with it ✱; its lanes keep playing |
| switching documents | the lanes keep playing; nothing is saved |
| `>new` | a fresh page, and every lane forgotten — the only command that clears them |
| `>run <piece>` | its lines run as the guide; they recompile rather than toggle |
| Ctrl-Z | undoes in this buffer only; refuses if the last change was in another |
| a KEY tap | the orientation is saved to flash ✱ |
