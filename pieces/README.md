# The set

Four tracks and a set list, each a document on the deck, about forty minutes. Open
`set` and run a heading to go to a track.

| | document | tempo, key | what it is | the screen |
|---|---|---|---|---|
| 1 | `grid` | 128, F minor | Swiss techno into 2-step: one organ stab over a moving bass, so four chords from one shape — Fm9, D♭maj9♯11, B♭9sus, Cm(♭13) | code, then `plain`: a square on the kick, where the bass is, framed on the clap |
| 2 | `offset` | 132, A dorian | UK garage: a 2-step, swung hats, an organ stab giving Am9, D9sus, Cmaj7, G6, chopped blips on Lead2 | poster, then `riso`: a dot on the kick at the bass, slipped by the snare |
| 3 | `drill` | 170, E phrygian | breaks: drill, jungle, shredded and half-time kits, a reese, power chords, a glitch arp | code, then `plain`; the drop goes negative and `sort` |
| 4 | `orbitals` | 124, D minor | the closer: Dm9 – B♭maj7 – Fmaj7 – C6 under orbiting arps, a melody, an eclipse, a lift | code for the night, `poster` from first light, `latent` in the eclipse |

## Playing it

- **`-- set` starts a track** from silence: it names its sounds and controllers,
  sends each controller its home value, and plays the intro.
- **A heading is a scene and lands on the one.** Run it any time in the bar before.
- **Run a scene again and its lanes go out** on the one; again, back. Every kit is
  also a break.
- **Versions, by heading.** Each track has kits (`kit home`, `two-step`, `glitch`,
  `half`; `jungle`, `shred`; `drums dust`, `pulse`…), basses, stabs and arps to
  switch between while it plays.
- **Moments end by themselves.** Hooks, chops, riffs and the melody are counted
  (`!4`): they play their bars and stop, so the lead never loops the whole track.
  Fills, crashes, rolls and stutters are the same.
- **Key moves land on the one.** `lift, up a tone`, `dark, phrygian`, `shade`,
  the eclipse, and ORBITALS' lift, which moves to F major and turns the same notes
  into Fmaj9 – Dm7 – Am7 – C7/E.
- **Arps against the bar.** `arp phase` is 12 steps over 16 and comes round every
  three bars; `arp five` in DRILL every five; `kit drift` has three-bar hats over
  four-bar chords.
- **Sweeps are counted too** (`>cut <2 3 4 5 6 7 8 9> /16 !8`), so they end and free
  their lane. One at a time: each track runs 13 lanes and keeps 3 for moments.
- **`>stop` is the second line** of every document.

## What to plug in

| channel | what |
|---|---|
| 1 | Bass1: GRID, ORBITALS |
| 2 | Lead1 |
| 3 | stabs and pads |
| 5 | Bass2: OFFSET |
| 6 | Bass3: DRILL's reese |
| 7 | Lead2: OFFSET's chops, DRILL's drop |
| 9 | DRILL's power chords |
| 10 | General MIDI drums: kick, snare, clap, hats, rim, tom, crash, and `perc` — maracas in GRID, conga in OFFSET, cowbell in DRILL, claves in ORBITALS |
| 11 | the arp |
| 16 | the HDMI screen's colour |

Controllers, from the `midi` page: `cut` (Bass1), `shine` (Lead1), `glow` (stabs),
`bite` (Bass2), `drive` (Bass3). `spark` is the arp's filter on channel 11 here —
learn it by running any `spark` scene with the DAW in map mode.

**Checked, every line:** `tools/test_pieces.c` runs each document through the deck's
own compiler, alone, twice over and in set order; `-s` prints every note a scene
plays, and each chord, bass and lead was read against the others that way.
