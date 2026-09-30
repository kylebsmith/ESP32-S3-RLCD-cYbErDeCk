# Pieces

An EP of four tracks, each a document on the deck. Open one and run its
headings top to bottom. Every track ends in silence, so any can follow any.

| document | tempo, key | what it is | the screen |
|---|---|---|---|
| `grid` | 124, F minor | Swiss techno. A: Fm9 – A♭maj7/F – D♭maj7 – D♭maj7♯11 – B♭m7 – B♭m6 – C7sus4 – Cm, a bar each; the lift climbs B♭m7 – Cm7 – D♭maj7 – E♭ under **the arp**: F minor pentatonic in fives against the bar, so it lands anew each bar and only repeats after twenty | `plain`, black on paper: a square on the beat stands where the bass is (across) and as high as the lead - the arp, in the lift; the clap outlines it; each chord lights the floor; the lift brings the grid forward, the break turns the page negative |
| `offset` | 132, A dorian | broken two-step: Am9 – Am7 – D7 – D9 – Em7 – Em7 – Cmaj7 – D6; the turn walks down C – B – A – G, the lead on Lead2 | `riso`: a dot on the kick, at the bass and the lead, its trail slipped right by the snare; the turn pulls the plates apart |
| `drill` | 165, E phrygian | punk drill'n'bass: power chords E – F – G – F – E – F – A – G on their own synth (channel 9), a pumping bass on Bass3, a riff | `plain`: hats are grain, the kick a block at the bass, the riff bends the rows, the snare flips it; the drop inverts, lead on Lead2 |
| `orbitals` | 124, D minor | a radar, a planet, a sun over a D pedal. Seven sections, numbered, so the poster prints them; the one key change on the EP is the eclipse | `code` for the night, then `poster` from first light: the planet goes where the arp and the lead go; the eclipse turns the accent blue |

## How a track plays

**A heading runs its section.** Put the cursor on `-- lift` and press ctrl+enter:
every `>` line indented under it runs, top to bottom. The `-- set` block sets the
whole track up and starts it; after that each heading is a scene.

**A scene lands on the one.** `>toggle`, `>scale` and `>send view <colour>` wait for
the next bar's first beat while playing, so run a scene any time in the bar
before and its sound, its key and its picture change together.

**Run a block again and it is a switch.** A kit — `-- drums glitch` — run a second
time, unchanged, takes its lanes out on the one; once more brings them back. So
every kit is also a break, and every layer a key.

**Controllers start at home.** Each `-- set` names its controllers and sends them
their starting value — `>send cut 3` — so a track that ended with the filter wide
open or the drive high cannot leave the next one there. A sweep is counted
(`>cut <2 3 4 5 6 7 8 9> /16 !8`): eight bars up, then it ends and its lane is free.

- **One key a track**, harmony moving by progression; ORBITALS' eclipse is the
  only key change.
- **Two versions of the harmony.** `-- lift` / `-- back` (and `-- turn` / `-- back`,
  `-- turn` / `-- home`) swap `pad`, `bass`, `lead` for `pad:2` and so on, and move
  the picture's routes with them.
- **Kits are headings**: `-- drums home`, `broken`, `glitch`, `half` in GRID, `skip`,
  `drift`, `half` in OFFSET, `drill`, `four`, `shred`, `half` in DRILL, `orbit`,
  `dust`, `pulse` in ORBITALS - each changes every drum, and the kick's picture
  with it, from the step they are on. `drift` has hats three bars long against an
  eight-bar harmony: it comes round every twenty-four.
- **Fills count**: `>tom … !1` waits for the bar, plays once and ends.
- **Voices move**: `>lead = ch 7` plays the lead on another synth, `= ch 2` back.
  ORBITALS moves the arp to 8 for the day.
- **Controllers** are the MIDI page's names - `cut` (Bass1), `shine` (Lead1),
  `glow` (pad), `spark` (Arp1), `bite` (Bass2), `drive` (Bass3) - learnt once,
  then every track uses them.
- **Blocks nest**: a heading indented under another runs with it, and alone.
- **Tab indents**, two spaces; a line run inside a block is never the second run
  that removes a lane, so a scene can be run again safely.

## What to plug in

| channel | what |
|---|---|
| 1 | Bass1: `bass` in `grid` and `orbitals`, and ORBITALS' `roll` |
| 2 | Lead1 |
| 3 | pad |
| 4 | Arp1: ORBITALS, and GRID's lift |
| 5 | Bass2: `bass` in `offset` |
| 6 | Bass3: `bass` in `drill` |
| 7 | Lead2: the lead after `>lead = ch 7` |
| 8 | Arp2: ORBITALS' arp in the day |
| 9 | DRILL's power chords |
| 10 | a General MIDI drum kit |
| 16 | the HDMI screen's colour, and the `midi` page's learn lines |

The deck holds 16 lanes; each track uses at most 16, pictures included.

**Timing.** Three things were wrong, and all three are fixed, each with a check
that fails on the old code:

- **Note length.** A note-off ended whatever note of its pitch was sounding, so a
  held note cut the next one short - ORBITALS lost 573. A note-off now ends only
  its own note (`seq_offs.h`, `tools/test_offs.c`).
- **Tempo.** The tick was 5,040 µs where 124 bpm needs 5,040.32, so the deck
  played 124.008 - 3.5 ms a minute ahead of a DAW at 124, measured, and 14 ms a
  minute at 165. Every tick is now worked out exactly from its number
  (`seq_clock.h`, `tools/test_clock.c`, `tools/clock_audit.py`).
- **Thick steps.** Each message went to USB on its own, and past sixteen in one
  step the rest were dropped. A step now leaves in one transfer. (The host still
  stamps a step's notes about 0.09 ms apart; the deck's own count puts every
  event on its way within 0.5 ms of its tick.)

Sync your DAW to the deck (`>sync on`, and Sync on in the DAW), or the two
crystals still part by a few parts per million.

## Whose is it

Every line is ours, written for this deck; nothing is copied from a recording. The
vocabulary is shared — a four-on-the-floor kick, a backbeat, a two-step, the
i – VI – III – VII of a thousand records, power chords — and the forms are ours.
`.000.000.000.<000 777>` was my line. Until 2026-09-29 the pieces were
sketches after named live coders, genre studies, then one long set with a key
change a section; I found the keys, the dropouts and the screen changes
arbitrary, and this EP replaced them.

**Checked, every line:** `tools/test_pieces.c` runs each track through the deck's
own compiler, names, key and pictures, alone, twice over, and as the EP in order.
`-s` prints the notes each section plays: every chord, bass note and lead note
was read against the others that way. Nothing here has been heard by the author;
it has been read.
