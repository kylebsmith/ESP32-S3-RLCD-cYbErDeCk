# The set

Six scores and a set list, each a document on the deck, about an hour. Open `set`
and run a heading to go to a track.

| | document | tempo, key | what it is | the screen |
|---|---|---|---|---|
| 1 | `grid` | 128, F minor | techno into 2-step: one organ stab moving over the bass, so four chords from one hand — Fm9, D♭maj9♯11, B♭9sus, Cm(♭13). A computer voice counts it in; a hook and its answer, a hold, the hard kit | code, then `plain`: a square on the kick that moves with the bass, inverted in the hold |
| 2 | `offset` | 132, A dorian | UK garage: a swung 2-step under an organ stab moving in the bar — Am9, D9sus, Cmaj7, G6 — and a plucked hook. "pull up" as it strips down, "very demure, very mindful" on the drop | `poster`, then `riso`, inverted in the strip |
| 3 | `weight` | 150, F♯ minor | pluggnb into hyperpop: Dmaj9, C♯m7, Bm9, F♯m9 under a sung hook, an 808 that slides, a clap on three. A bar of air, then the bounce with the hook an octave up and the keys pumping; a weightless bridge — Amaj9, E6/9, Dmaj9, C♯7sus — and the drop again, an "oof" where the weight lands | code, `plain`, `sort` for the drops, `poster` for the bridge |
| 4 | `heat` | 160, E major | hyperpop, gliding and crushed: the royal road — Amaj7♯11, B9, G♯m7, C♯m9 — with a D♯ held through all four, an 808 that never stops gliding, a bright hook that glides too. A bouncing verse, a bar of air, four-on-the-floor hype, then chaos — the kick in eighths, the 808 sliding every sixteenth, the hook at double speed, and "shadow wizard money gang", once, answered by "we love casting spells" | code, `plain`, `sort` for the hype, inverted for the chaos |
| 5 | `pressure` | 140, E phrygian | dark techno: a kick and its rumble, an E minor stab with a flat ninth every three sixteenths against the four, acid; then the kick itself thrown off the grid, "we live in a society" in a bar of nothing, and "bass boosted" on the drop | code, `scan`, inverted for the nothing, `sort` |
| 6 | `orbitals` | 124, D minor | the closer: Dm9, B♭maj7, Fmaj7, C6, a slow tune with glass and orbiting arps round it, an eclipse in half time with Carl Sagan over it, a lift to new chords in the same key, the tune and the glass alone at the end, and the set shuts down | code for the night, `poster` from first light, `latent` in the eclipse |

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
- **Counted lines end by themselves**: a crash `!1`, a sample `!1`, a sweep `!6`.
- **The 808s slide where a tie runs into the next note** and not where a rest
  comes first, on a mono synth with legato glide; the acid the same.

## The samples

One to three samples a track, each a name on its own note on channel 12: HEAT's
`>gang = note 40 ch 12` is "shadow wizard money gang", and `>gang <9.9.9...`
stutters the start of it. The digit is how loud, as on a drum. In the DAW they are
one Drum Rack, a sample a pad; the pads and what goes on them: [ABLETON.md](ABLETON.md), §5.

## Its own sounds

Six tracks share the instrument channels — every chord on 3, every kit on 10 —
so each track's first section runs `>patch N`, GRID 1 to ORBITALS 6: controller 3
at N's level on channels 1–15. In the DAW each channel is an instrument type with
a chain per track, and the patch picks the chain, so HEAT's kit and 808 are not
WEIGHT's and ORBITALS' pad is not GRID's organ. Which chains each channel needs:
[ABLETON.md](ABLETON.md), §2.

## What to plug in

| channel | what |
|---|---|
| 1 | Bass1: GRID, ORBITALS |
| 2 | Lead1: the hooks in GRID, OFFSET, WEIGHT, HEAT, ORBITALS |
| 3 | stabs and pads: GRID, OFFSET, WEIGHT, HEAT, ORBITALS |
| 4 | Stab: PRESSURE |
| 5 | Bass2: OFFSET |
| 6 | the 808: WEIGHT, HEAT |
| 9 | Rumble: PRESSURE |
| 10 | General MIDI drums: kick, snare, clap, hats, rim, crash, and `perc` — maracas in GRID, low conga in OFFSET, rim in WEIGHT and HEAT, claves in ORBITALS |
| 11 | WEIGHT's bell, ORBITALS' glass arp |
| 12 | samples: one Drum Rack, C1 to A♯1, a sample a note |
| 1–15 | controller 3: the patch, which track's sounds |
| 13 | Acid: PRESSURE |
| 16 | the HDMI screen's colour |

Controllers: `cut` (Bass1; the rumble in PRESSURE), `shine` (Lead1), `glow` (stabs and pads), `bite` (Bass2; the acid in PRESSURE), `drive`
(the 808), `pump` (cc 21 on channel 3: how hard the kick ducks the keys in
WEIGHT and HEAT), `spark` (channel 11). The Ableton template, with several voices on one
channel and the controllers on macros: [ABLETON.md](ABLETON.md).

**Checked, every line:** `tools/test_pieces.c` runs each document through the deck's
own compiler, alone, twice over and in set order; `-s` prints every note a section
plays, and each chord, bass and lead was read against the others that way. Each
score was also walked on the deck, F10 and Ctrl+Enter four bars apart, with the
MIDI recorded: every new note on the parts a section writes sounded on or after
its bar line.
