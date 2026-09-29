# Pieces

An EP of four tracks, each a document on the deck. Open one and run it top to
bottom, a line at a time. Every track ends in silence, so any can follow any.

| document | tempo, key | what it is | the screen |
|---|---|---|---|
| `grid` | 124, F minor | Swiss techno: a strict kick, offbeat hats, a rolling bass and a hook over F minor 9 – D♭ maj 7 – A♭ maj 7 – E♭; the lift climbs B♭m – Cm – D♭ – E♭ | `plain`, with the deck's own cell grid drawn on it until the drop |
| `offset` | 132, A dorian | broken two-step, swung: the dorian vamp A m9 – D9, then C maj7 – Bm7 – Em7 – D7 falling back; the bass on Bass2 | `riso`, the plates slipping on the snare |
| `drill` | 165, E phrygian | punk drill'n'bass: power chords E – F – G – F, a pumping bass on Bass3, a riff; the turn goes C – D – E | `scan`, the screen flipping on the snare from the drop |
| `orbitals` | 124, D minor | the piece: a radar, a planet and a sun that never leaves D. Night, first light, a day that lifts the harmony, an eclipse, and night again | `poster` throughout, the paper brightening with the day |

## How a track plays

**Everything is written before `>play`**, so nothing drops out by accident. After
that a section is one line, and it **lands on the next bar's first beat**, however
early it is run: `>toggle` waits for the one (`seq_toggle.h`).

- **One key a track.** Harmony moves by progression, with the voices led, not by
  changing key. The only key change on the EP is ORBITALS' eclipse and its return.
- **Two versions of the harmony.** `pad`, `bass` and `lead` each have a second,
  `pad:2` and so on, and one line swaps the whole song between them — `>toggle pad
  pad:2 bass bass:2 lead lead:2` — and the same line swaps it back.
- **Drums are one lane a part.** A break is `>toggle kick hat clap`, and the same
  line brings them back. Their variations are written as a second line under a
  heading: run it to switch, run the first to switch back. A drum switch carries
  on from the step it is on.
- **Every lane is a bar or four bars long,** so nothing drifts against the bar;
  the planet's orbit in ORBITALS is the only thing at twelve, and it is a picture.
- **The screen is chosen once a track,** and it moves only with the music: the
  grid leaves at GRID's drop, the plates slip on OFFSET's snare, DRILL's screen
  flips on its snare after the drop, and ORBITALS' paper lightens at first light
  and darkens in the eclipse.

## What to plug in

| channel | what |
|---|---|
| 1 | Bass1: `bass` in `grid` and `orbitals` |
| 2 | lead |
| 3 | pad |
| 4 | arp |
| 5 | Bass2: `bass` in `offset` |
| 6 | Bass3: `bass` in `drill` |
| 10 | a General MIDI drum kit |
| 16 | **the HDMI screen's colour**: `lines`, `skew`, `inv` and `day` are controllers here ([VIEW.md](../docs/VIEW.md)) |

Each track defines its own voices, because octave and gate are part of the sound.
**More voices:** any new name is another part on another channel, for example
`>bass2 = voice 1 ch 7`; `>bass = ch 5` moves a playing part. The deck holds 16
lanes and 32 names at once.

**Timing, measured 2026-09-29:** the deck's MIDI at 165 bpm with 32nd hats, ratchets
and the view on landed within 0.11 ms of the grid (0.03 ms rms). If a dense
passage sounds loose in a DAW, look at the DAW's audio buffer first: it plays
incoming MIDI in chunks of its buffer, 10–20 ms at 512–1024 samples.

## Whose is it

Every line is ours, written for this deck; nothing is copied from a recording. The
vocabulary is shared — a four-on-the-floor kick, a backbeat, a two-step, the
i – VI – III – VII of a thousand records, power chords — and the forms are ours.
`.000.000.000.<000 777>` was the owner's line. Until 2026-09-29 the pieces were
sketches after named live coders, genre studies, then one long set with a key
change a section; the owner found the keys, the dropouts and the screen changes
arbitrary, and this EP replaced them.

**Checked, every line:** `tools/test_pieces.c` runs each track through the deck's
own compiler, names, key and pictures, alone, twice over, and as the EP in order.
`-s` prints the notes each section plays: every chord, bass note and lead note
was read against the others that way. Nothing here has been heard by the author;
it has been read.
