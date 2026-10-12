# The set in Ableton

The deck plays notes and controllers on one USB MIDI port, `cyberdeck`, a channel
per part. Ableton is where they become sounds. This is the template the scores
were written for: one track a channel, and inside each track a rack that turns
one channel into several voices.

## 1. Connect

- **Settings → Link, Tempo & MIDI**, the `cyberdeck` input: **Track** on, **Remote**
  on (for mapping its controllers), **Sync** on.
- On the deck, `>sync on`. Ableton's tempo then follows the deck: 128, 132, 150 and
  124 as the set moves, with every delay and LFO in time. Press Ableton's **EXT**.
- Nothing has to go back to the deck.

## 2. One track a channel

Make a MIDI track for each channel, input `cyberdeck`, the channel below, and
**Monitor: In**, so it plays without being armed.

| ch | track | what plays on it |
|---|---|---|
| 1 | Bass1 | GRID's and ORBITALS' bass |
| 2 | Lead1 | the hooks in GRID, WEIGHT, ORBITALS |
| 3 | Keys | every stab and pad |
| 5 | Bass2 | OFFSET's bass |
| 6 | 808 | WEIGHT's 808 |
| 7 | Chops | OFFSET's chops |
| 10 | Drums | a Drum Rack on the General MIDI notes |
| 11 | Bell | WEIGHT's bell, ORBITALS' glass arp |

Take parts out on the deck, not in Ableton: the deck's mutes land on the one.

## 3. Several voices on one track

Put an **Instrument Rack** on each track and give it more than one chain. A chain
is a voice; the rack decides which voices a note reaches, four ways:

- **Layer.** Every chain plays every note. The 808 as a sine sub plus a distorted
  layer high-passed at about 120 Hz, so the low end stays clean while the top
  growls.
- **Velocity zones.** Soft notes reach one chain, hard notes another. On drums the
  deck's digits are velocities, so `9` can be a clap with a snare layered under it
  and `3` the clap alone. On a voice, a `:vel` line sets it: `>lead:vel 9.5.7.3.`.
- **Key zones.** Low notes one chain, high notes another. The deck moves a voice
  between them with its octave: WEIGHT's drop runs `>lead:oct 5 /16`, so give the
  octave above the hook its own chain — a pitched vowel, a supersaw — and the
  drop changes the sound as well as the register.
- **Chain select.** Map a macro to the Chain Selector and set the chains' zones to
  overlap with fades: one deck controller then crossfades voices. `>send glow 8`
  can take the keys from an electric piano into a wide supersaw.

Put the rack's **MIDI effects** in front of it for variety the deck does not write:
a **Velocity** device with a little Random on hats and voices (the deck's voices
play at one level unless a `:vel` line moves them), **Note Echo** for a pitched
stutter you can open with a macro, **Chord** on a chain that only the drop
reaches.

## 4. Controllers into macros

Each track's rack gets macros, and the deck's controllers move them:

| deck | message | map it to |
|---|---|---|
| `cut` | cc 74, ch 1 | Bass1: filter, a little drive |
| `shine` | cc 74, ch 2 (ch 7 in OFFSET) | Lead1 / Chops: brightness, delay send |
| `glow` | cc 74, ch 3 | Keys: filter and reverb send, or the chain crossfade |
| `bite` | cc 74, ch 5 | Bass2: filter, the reese opening up |
| `drive` | cc 20, ch 6 | 808: distortion (Roar or Saturator) |
| `pump` | cc 21, ch 3 | Keys: how much the kick ducks them |
| `spark` | cc 74, ch 11 | Bell: brightness, delay feedback |

To map one: press **⌘M**, click the macro, then on the deck send that controller
alone — `>send glow 9`, or `>map glow`, which quiets every other lane until you run
`>map` again. Map the same controller to more than one thing and it becomes one
gesture: `pump` to the keys' ducking, the 808's drive and the lead's brightness is
the whole drop on one line.

Controllers arrive as steps (0–9 → 0–127). A sweep like `>glow <4 5 6 7> /16 !4`
moves once a bar: a staircase you hear, so make the steps the point.

**The pump.** Put a **Compressor** on Keys with its sidechain from the Drums track,
the kick's chain only (Audio From: Drums, the kick pad). Map its **Dry/Wet** to the
`pump` macro. WEIGHT sends `pump 0` in the verse and `pump 9` in the drops, and the
ducking follows whatever the kick plays — half time, then the bounce.

## 5. Track by track

- **808 (ch 6).** Mono, legato, glide about 60 ms. WEIGHT writes its slides: a tie
  that runs into the next note overlaps it and glides, a rest before a note does
  not (gate 250 ms against 200 ms eighths). `drive` on the distortion.
- **Keys (ch 3).** One rack for the whole set: an electric piano chain for GRID and
  OFFSET's organ stabs (short release), a supersaw chain with a slow attack for
  WEIGHT and ORBITALS, `glow` between them. The pump compressor after the rack.
- **Lead1 (ch 2).** A sung vowel in Simpler, Classic mode, mono with glide, so a
  hook sounds like a voice; a quiet sine an octave up as a second chain. In key
  zones, as above, for WEIGHT's octave.
- **Chops (ch 7).** Sampler with a different syllable on each key zone, so OFFSET's
  chops are words, cut up. Short release; the deck's 60 ms gate does the chopping.
- **Drums (ch 10).** A Drum Rack on the General MIDI notes: 36 kick, 37 rim, 38
  snare, 39 clap, 42 closed hat, 46 open hat (one choke group with 42), 49
  crash, 64 low conga, 70 maracas, 75 claves. Velocity layers inside the pads.
- **Bell (ch 11).** An FM bell (Operator, short decay), a synced echo and a long
  reverb. WEIGHT's bell plays twelve steps against sixteen and on chance, so it
  never repeats the same bar; leave it room.

## 6. Recording the set

Arm every track and record into the Arrangement: the notes, and the controllers as
clip envelopes, so a performance comes out as a session you can mix. **Capture
MIDI** gets back what the deck played before you pressed record. If a track sounds
early or late against the others, its **Track Delay** in milliseconds lines it up.
