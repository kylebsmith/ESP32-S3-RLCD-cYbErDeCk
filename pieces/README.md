# The set

Six scores and a set list, each a document on the deck, about an hour. Open `set`
and run a heading to go to a track.

| | document | tempo, key | what it is | the screen |
|---|---|---|---|---|
| 1 | `grid` | 128, F minor | techno into 2-step: one organ stab moving over the bass, so four chords from one hand — Fm9, D♭maj9♯11, B♭9sus, Cm(♭13). The grid counts itself in; a hook and its answer, a hold, the hard kit | code, then `plain`: a square on the kick that moves with the bass, inverted in the hold |
| 2 | `offset` | 132, A dorian | UK garage: a swung 2-step under an organ stab moving in the bar — Am9, D9sus, Cmaj7, G6 — and a voice cut into chops. An MC calls the rewind as it strips down, then the drop | `poster`, then `riso`, inverted in the strip |
| 3 | `weight` | 150, F♯ minor | pluggnb into hyperpop: Dmaj9, C♯m7, Bm9, F♯m9 under a sung hook, an 808 that slides, a clap on three. A bar of air, then the bounce with the hook an octave up and the keys pumping; a weightless bridge — Amaj9, E6/9, Dmaj9, C♯7sus — and the drop again | code, `plain`, `sort` for the drops, `poster` for the bridge |
| 4 | `heat` | 145, G locrian | South Florida trap, ignorant on purpose: one word shouted on the tresillo with a brass hit on the tritone, a distorted 808 that slides up a tritone every fourth bar, flat loud hats. The chant slips out of step, everything stops, then a switch to Miami bass nobody asked for, and back | code, `plain`, `riso` for Miami, `sort` |
| 5 | `pressure` | 140, E phrygian | dark techno: a kick and its rumble, one spoken word, an E minor stab with a flat ninth every three sixteenths against the four, acid; then the kick itself thrown off the grid, a bar of nothing, and "harder" on every beat | code, `scan`, inverted for the nothing, `sort` |
| 6 | `orbitals` | 124, D minor | the closer: Dm9, B♭maj7, Fmaj7, C6 under glass, a slow tune, orbiting arps, an eclipse in half time with one sung vowel, a lift to new chords in the same key | code for the night, `poster` from first light, `latent` in the eclipse |

## Playing it

- **Each track is a score: walk down it.** The first heading names the sounds and
  controllers, sends each controller its home value and starts. Every numbered
  heading under it is the next section: F10 goes to it, Ctrl+Enter runs it.
- **Everything lands on the next bar's one.** A part a section brings in starts
  there, a part it changes plays its old pattern until there, and `>mute` takes
  parts out there. Run a section any time in the bar before.
- **The melodies are where they happen.** The hook, its answer, a new chord loop, a
  different bass: each sits in the section where it starts, so the score reads
  top to bottom and nothing is a jump away. They loop, locked to the chords,
  until a later section changes them or a key takes them out.
- **F1–F8 take parts out and bring them back**, on the one. Each track's `>keys`
  line names them. A section's `>mute` and lines still say where everything goes,
  whatever the keys did before; `>mute` alone brings everything back.
- **A section run again takes its lanes out**, on the one; again, back.
- **Counted lines end by themselves**: a crash `!1`, a word `!1`, a sweep `!6`.
- **The 808s slide where a tie runs into the next note** and not where a rest
  comes first, on a mono synth with legato glide; the acid the same.

## The VOX bank

`vox` is a lane of sampled words on channel 12. Each track defines it as one
note — its bank — and the digit picks the word: the deck sends the digit as the
velocity, and the sampler picks the sample by velocity. `>vox 1..1..1.` in HEAT is
"HEAT" three times on the tresillo; `>vox 1...2...3...4... !1` in GRID counts one
bar in. Every chop shares its bank's note, so each word cuts the last. Banks,
words, and how to build them in Ableton: [ABLETON.md](ABLETON.md), §5.

## What to plug in

| channel | what |
|---|---|
| 1 | Bass1: GRID, ORBITALS |
| 2 | Lead1: the hooks in GRID, WEIGHT, ORBITALS |
| 3 | stabs and pads: GRID, OFFSET, WEIGHT, ORBITALS |
| 4 | Stab: PRESSURE |
| 5 | Bass2: OFFSET |
| 6 | the 808: WEIGHT, HEAT |
| 7 | Lead2: OFFSET's chops |
| 8 | Brass: HEAT |
| 9 | Rumble: PRESSURE |
| 10 | General MIDI drums: kick, snare, clap, hats, rim, crash, and `perc` — maracas in GRID, low conga in OFFSET, rim in WEIGHT, cowbell in HEAT, claves in ORBITALS |
| 11 | WEIGHT's bell, ORBITALS' glass arp |
| 12 | VOX: note 36–41 is the track's bank, the velocity is the word |
| 13 | Acid: PRESSURE |
| 16 | the HDMI screen's colour |

Controllers: `cut` (Bass1; the rumble in PRESSURE), `shine` (Lead1; the chops in
OFFSET), `glow` (stabs and pads), `bite` (Bass2; the acid in PRESSURE), `drive`
(the 808), `pump` (cc 21 on channel 3: how hard the kick ducks the keys in
WEIGHT), `spark` (channel 11). The Ableton template, with several voices on one
channel and the controllers on macros: [ABLETON.md](ABLETON.md).

**Checked, every line:** `tools/test_pieces.c` runs each document through the deck's
own compiler, alone, twice over and in set order; `-s` prints every note a section
plays, and each chord, bass and lead was read against the others that way. Each
score was also walked on the deck, F10 and Ctrl+Enter four bars apart, with the
MIDI recorded: every new note on the parts a section writes sounded on or after
its bar line.
