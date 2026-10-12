# The set

Four tracks and a set list, each a document on the deck, about forty minutes. Open
`set` and run a heading to go to a track.

| | document | tempo, key | what it is | the screen |
|---|---|---|---|---|
| 1 | `grid` | 128, F minor | Swiss techno into 2-step: one organ stab over a moving bass, so four chords from one shape — Fm9, D♭maj9♯11, B♭9sus, Cm(♭13) | code, then `plain`: a square on the kick, where the bass is, framed on the clap |
| 2 | `offset` | 132, A dorian | UK garage: a 2-step, swung hats, an organ stab giving Am9, D9sus, Cmaj7, G6, chopped blips on Lead2 | poster, then `riso`: a dot on the kick at the bass, slipped by the snare |
| 3 | `weight` | 140, C♯ phrygian | Atlanta trap, the peak: a sliding 808 locked to the kick, snare on three, 32nd and triplet hat rolls, cowbell, a dark choir C♯m – A – F♯m – D, an eerie bell loop, flute moments, a cut to silence and a drop | code, then `plain`; the snare flips the screen, the drop goes `sort` |
| 4 | `orbitals` | 124, D minor | the closer: Dm9 – B♭maj7 – Fmaj7 – C6 under orbiting arps, a melody, an eclipse, a lift | code for the night, `poster` from first light, `latent` in the eclipse |

## Playing it

- **The first heading starts a track** from silence: it names its sounds and
  controllers, sends each controller its home value, and plays the intro.
- **F1–F8 play the parts.** Each track's `>keys` line names them — kick, clap or
  snare, hats, open hats, perc, bass, stabs, arp — and a key takes its part out or
  brings it back on the next bar's one. F9 and F10 jump to the heading above or
  below, so a scene is one key away while the patterns stay editable at the top.
- **A heading is a scene and lands on the one.** Run it any time in the bar before.
- **Run a scene again and its lanes go out** on the one; again, back. Every kit is
  also a break.
- **Versions, by heading,** grouped under `kits`, `bass`, `chords`, `arps`: kits
  (`hard`, `two-step`, `glitch`, `rolls`, `triplets`, `bounce`, `dust`…), basses, stabs and arps to
  switch between while it plays.
- **Moments end by themselves.** Hooks, chops, riffs and the melody are counted
  (`!4`): they play their bars and stop, so the lead never loops the whole track.
  Fills, crashes, rolls and stutters are the same.
- **Builds and drops.** `build` rolls into the bar line with the filter opening;
  `drop` slams the drums back with a crash. In WEIGHT, `cut, silence` empties the
  bar before it.
- **One key a track.** ORBITALS' lift is new chords in the same key — Gm9, Am7,
  B♭maj7, Cadd9 — not a key change.
- **Arps against the bar.** `arp phase` is 12 steps over 16 and comes round every
  three bars; `kit drift` has three-bar hats over four-bar chords.
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
| 6 | Bass3: WEIGHT's 808 |
| 7 | Lead2: OFFSET's chops |
| 10 | General MIDI drums: kick, snare, clap, hats, rim, tom, crash, and `perc` — maracas in GRID, conga in OFFSET, cowbell in WEIGHT, claves in ORBITALS |
| 11 | the arp |
| 16 | the HDMI screen's colour |

Controllers, from the `midi` page: `cut` (Bass1), `shine` (Lead1), `glow` (stabs),
`bite` (Bass2), `drive` (Bass3). `spark` is the arp's filter on channel 11 here —
learn it by running any `spark` scene with the DAW in map mode.

**Checked, every line:** `tools/test_pieces.c` runs each document through the deck's
own compiler, alone, twice over and in set order; `-s` prints every note a scene
plays, and each chord, bass and lead was read against the others that way.
