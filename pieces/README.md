# Pieces

Three sets, typed onto the deck as documents. Open one and run it top to bottom,
a line at a time — the order of the lines is the arrangement.

| document | tier | what it is |
|---|---|---|
| `ground` | one | Four grooves done properly: house, lofi, techno, liquid drum and bass. Steps, velocity, ties, chords, rolls, odds, alternation, a controller, swing. |
| `lift` | two | Five sketches after live coders — DJ_Dave, Switch Angel, Yaxu, Kindohm, Olivia Jack. Instances, parts, a four-bar build in one line, a cue that drops the kick on the downbeat, a gate on a controller, polymeter that repeats after 315 bars, 7/8, pictures routed from drums. |
| `orbitals` | three | The piece. One chord line that is never edited, under seven lights: phrygian night, minor, dorian, lydian day, an eclipse, dorian, night. The bass is the sun and never leaves D. A twelve-step arpeggio against the bar, and a planet whose x and y run at sixteen and twelve steps, so the orbit it draws is the polyrhythm. |

**What to plug in.** The boot names: a General MIDI drum kit on channel 10, bass
on 1, lead on 2, pad on 3, arp on 4. `cut` is controller 74 on channel 1; the
pieces define their own controllers where they need one (`chop`, `rise`, `glow`).
Each sketch defines the voices it uses — octave and gate are part of the sound.

**Every set ends in silence**, so any can follow any. Stopping halfway and
opening another leaves lanes playing: `>new` is the only clean slate, because the
deck has no verb that clears the lanes and keeps the page.

**Checked, every line:** `tools/test_pieces.c` runs each set through the deck's
own compiler, names, key and pictures, alone and twice over, and `-s` prints the
notes each section plays — which is how these were written.
