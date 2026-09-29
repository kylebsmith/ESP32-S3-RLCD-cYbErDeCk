# Pieces

One set in three acts, typed onto the deck as three documents. Open one and run
it top to bottom, a line at a time: the order of the lines is the arrangement.
Played in order, each act hands its last groove to the next.

| document | act | what it is |
|---|---|---|
| `ground` | one | **The grid, and how to break it.** 93 bpm: a strict grid; then the snare displaced a sixteenth and five- and seven-step percussion drifting against it; then the drums erased and the bass moved to another synth; then a ghost kick every three sixteenths becomes the beat at 124. Steps, velocity, ties, chords, odds, alternation, word alternation, a controller, swing, a block toggle. |
| `lift` | two | **Layers out of register.** 124: lanes of 16, 12, 8 and 5 steps stacked; the bass moved from synth to synth while a second bass takes the channel it left; seven-eight; a cut to silence and a drill at 165 that lands on the bar; a cut back to 124. Instances, parts, routes, counts, polymeter, voices on more channels. |
| `orbitals` | three | The piece. One chord line that is never edited, under seven lights: phrygian night, minor, dorian, a day that brightens one note at a time to lydian, an eclipse, and back to night. The bass is the sun and never leaves D, but its light moves from synth to synth. A twelve-step arpeggio against the bar, and a planet whose x and y run at sixteen and twelve steps round a square sun. |

## How the set moves

No snare-roll builds. Each change is one of these:

- **A key changes by one note, or by none.** Most moves keep all seven notes and
  move the centre: G minor to C dorian to D minor are the same notes. When a note
  does change, it is one at a time: ORBITALS climbs from phrygian to lydian one
  note per line. Only the eclipse changes many notes at once, and it does so while
  the fast lanes are switched off, so it lands on the pad's next bar.
- **Tempos are 4:3 apart**: 93, 124, 165. The three-sixteenth pulse at one tempo
  is the beat at the next. The tempo changes while only the pad sounds.
- **Erase, don't add.** `>toggle kick snare hat` switches a whole block off, and
  the same line brings it back.
- **`!255` makes a line wait for its bar.** The drill and the day both land on the
  one, however early the line is run.
- **Move the sound, not the notes.** `>bass = ch 5` sends the bass that is playing
  to another synth and keeps its octave and gate.

Press `>play` on the one: it starts the act's bar count from the top.

## What to plug in

| channel | what |
|---|---|
| 1 | Bass1: `bass`, and `sub` in `lift` |
| 2 | lead |
| 3 | pad; `glow` is controller 74 here |
| 4 | arp |
| 5 | Bass2: where `>bass = ch 5` sends the bass |
| 6 | Bass3: `>bass = ch 6` |
| 10 | a General MIDI drum kit |
| 16 | **the HDMI screen's colour**: `day`, `inv`, `glint`, `skew`, `lines` are controllers 1-8 here ([VIEW.md](../docs/VIEW.md)) |

`cut` is controller 74 on channel 1, and follows the bass to channel 5 in
`ground`. Each act defines the voices it uses, because octave and gate are part
of the sound. **More voices:** any new name is another part on another channel,
for example `>bass2 = voice 1 ch 7`. The deck holds 16 lanes and 32 names at once.

## Whose is it

The notes, and the set's rules above, are ours. The grooves still stand on
common ground: a four-on-the-floor kick, a backbeat on two and four, and a
rolling bass line are shared by every dance record. Until 2026-09-29 `lift` was
five sketches written in the manner of named live coders, and `ground` was four
genre studies. Both were replaced by this set.

**Checked, every line:** `tools/test_pieces.c` runs each act through the deck's
own compiler, names, key and pictures, alone, twice over, and as the whole set
in order. `-s` prints the notes each section plays, which is how these were
written. Nothing here has been heard yet. It has only been read.
