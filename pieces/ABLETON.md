# The set in Ableton

The deck plays notes and controllers on one USB MIDI port, `cyberdeck`, a channel
per sound. Ableton is where they become sounds. This is the template the scores
were written for: one track a channel, and inside each track a rack that turns
one channel into several voices.

## 1. Connect

- **Settings → Link, Tempo & MIDI**, the `cyberdeck` input: **Track** on, **Remote**
  on (for mapping its controllers), **Sync** on.
- On the deck, `>sync on`. Ableton's tempo then follows the deck through the set —
  128, 132, 150, 145, 140, 124 — with every delay and LFO in time. Press **EXT**.
- Nothing has to go back to the deck.

## 2. One track an instrument type, one chain a track

Make a MIDI track for each channel, input `cyberdeck`, the channel below, and
**Monitor: In**, so it plays without being armed. Never **All Channels**: a track
on all channels plays every part through one instrument, and every track becomes
the same pad.

Six tracks share these channels — every chord on 3, every kit on 10 — so each
Ableton track is an **instrument type** and holds a rack with **one chain per
track that uses it**. Each score's first section runs `>patch N`: the deck sends
controller 3 at N's level on channels 1–15, and every rack switches to that
track's chain before a note plays. Starting HEAT turns the drums into HEAT's kit
and the 808 into HEAT's 808; starting ORBITALS turns the keys into its glass pad.

| ch | track | 1 GRID | 2 OFFSET | 3 WEIGHT | 4 HEAT | 5 PRESSURE | 6 ORBITALS |
|---|---|---|---|---|---|---|---|
| 1 | Bass1 | short techno bass | | | | | warm sub, long |
| 2 | Lead1 | organ lead | pluck | sung vowel, glide | | | soft sine |
| 3 | Keys | short organ stab | garage organ | e-piano + supersaw | | | slow glass pad |
| 4 | Stab | | | | | dark saw stab | |
| 5 | Bass2 | | reese, sub | | | | |
| 6 | 808 | | | clean, sliding | blown out | | |
| 8 | Brass | | | | orchestra hit | | |
| 9 | Rumble | | | | | rumble | |
| 10 | Drums | tight techno kit | garage kit | plugg kit | Florida 808 kit | industrial kit | soft kit |
| 11 | Bell | | | FM bell | | | glass FM arp |
| 12 | Samples | C1 | C♯1, D1 | D♯1 | E1, F1, F♯1 | G1, G♯1 | A1, A♯1 |
| 13 | Acid | | | | | 303 | |

**Setting up a switch.** In the rack, show the **Chain** select editor and give
each chain the zone of its patch: patch N arrives as N × 127 / 9, so 1 is 14, 2 is
28, 3 is 42, 4 is 56, 5 is 70, 6 is 84 — zone N runs from 14N − 6 to 14N + 7.
Map the Chain Selector to a macro, press **⌘M**, click the macro, and run
`>patch 1` on the deck: Ableton learns controller 3. Do the same on every
rack with the same `>patch` line; one controller mapped many times moves them all.
A channel only one track uses needs no switch. On the drums, the chains are whole
Drum Racks inside an Instrument Rack. The samples need no switch: each track has
its own notes.

While a track plays, `>patch` lands on the next one like everything else, so a
section can change every sound at once — none of the scores do yet.

Take parts out on the deck, not in Ableton: the deck's mutes land on the one.

## 3. Several voices on one track

Inside a track's chain, more voices. A rack decides which voices a note reaches:

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

Put the rack's **MIDI effects** in front of it for variety the deck does not write:
a **Velocity** device with a little Random on hats and voices (the deck's voices
play at one level unless a `:vel` line moves them), **Note Echo** for a pitched
stutter you can open with a macro, **Chord** on a chain that only the drop
reaches.

## 4. Controllers into macros

Each track's rack gets macros, and the deck's controllers move them:

| deck | message | map it to |
|---|---|---|
| `cut` | cc 74, ch 1 (ch 9 in PRESSURE) | Bass1: filter, a little drive; the rumble's filter |
| `shine` | cc 74, ch 2 | Lead1: brightness, delay send |
| `glow` | cc 74, ch 3 | Keys: filter and reverb send, or the chain crossfade |
| `bite` | cc 74, ch 5 (ch 13 in PRESSURE) | Bass2: the reese opening; the acid's cutoff |
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

## 5. The samples

Channel 12 is one **Drum Rack**: a sample on a note, one to three a track. Drop
each one on its pad, set it to one-shot, and pitch anything tonal to its track's
key. The deck plays a sample like a drum, so the digit is how loud, and running
it again cuts it and starts it over: `9.9.9...` stutters the start of a phrase.

| pad | track (key) | sample | where |
|---|---|---|---|
| C1 (36) | GRID (F minor) | a computer voice counting "one, two, three, four" | counts the track in, then the hard kit |
| C♯1 (37) | OFFSET (A dorian) | "pull up" | as it strips down |
| D1 (38) | OFFSET | "very demure, very mindful" | on the drop |
| D♯1 (39) | WEIGHT (F♯ minor) | the Roblox "oof" | the one of each drop |
| E1 (40) | HEAT (G locrian) | "shadow wizard money gang" | the call, every other bar; stuttered, and drifting out of step |
| F1 (41) | HEAT | "we love casting spells" | the answer, the other bar; alone in the dead stop |
| F♯1 (42) | HEAT | an MLG air horn | the Miami switch, the last drop |
| G1 (43) | PRESSURE (E phrygian) | "we live in a society", pitched down | alone in the bar of nothing |
| G♯1 (44) | PRESSURE | "bass boosted", yelled | on the one of the last drop |
| A1 (45) | ORBITALS (D minor) | Carl Sagan: "that's here. that's home. that's us." | through the eclipse |
| A♯1 (46) | ORBITALS | the Windows XP shutdown, pitched to D | the last sound of the set |

Swap any of them for your own on the same pad; the scores only know the note.
These are other people's recordings: fine on a stage, cleared before a release.

## 6. Track by track

- **808 (ch 6).** Mono, legato, glide about 60 ms. WEIGHT and HEAT write their
  slides: a tie that runs into the next note overlaps it and glides, a rest before
  a note does not (the gate, 250 ms, is longer than an eighth). `drive` on the
  distortion; HEAT runs it at 6 and 9.
- **Keys (ch 3).** Four chains on the patch: GRID's short organ stab, OFFSET's
  garage organ, WEIGHT's electric piano with a supersaw under it, ORBITALS' slow
  glass pad. `glow` is each one's filter. The pump compressor after the rack.
- **Lead1 (ch 2).** A sung vowel in Simpler, Classic mode, mono with glide, so a
  hook sounds like a voice; a quiet sine an octave up as a second chain, in key
  zones as above, for WEIGHT's octave.
- **Brass (ch 8).** HEAT's hit is G, D♭, G — a power chord with a tritone where the
  fifth should be. An orchestra hit or a brass stab, loud, short.
- **Stab (ch 4).** PRESSURE's stab: E minor with a flat ninth on top, played every
  three sixteenths against the four, so it comes round every three bars. A short
  saw chord into a long dark reverb.
- **Rumble (ch 9).** Three short E1s after every kick. A sine with a fast decay
  into heavy distortion and reverb, low-passed; `cut` opens it.
- **Acid (ch 13).** A 303-style mono synth with glide: consecutive notes overlap
  and slide, a rest before a note does not. `bite` on the cutoff, resonance high.
- **Drums (ch 10).** A Drum Rack on the General MIDI notes: 36 kick, 37 rim, 38
  snare, 39 clap, 42 closed hat, 46 open hat (one choke group with 42), 49 crash,
  56 cowbell, 64 low conga, 70 maracas, 75 claves. Velocity layers inside the pads.
- **Bell (ch 11).** An FM bell (Operator, short decay), a synced echo and a long
  reverb. WEIGHT's bell plays twelve steps against sixteen and on chance, so it
  never repeats the same bar; leave it room.

## 7. Recording the set

Arm every track and record into the Arrangement: the notes, and the controllers as
clip envelopes, so a performance comes out as a session you can mix. **Capture
MIDI** gets back what the deck played before you pressed record. If a track sounds
early or late against the others, its **Track Delay** in milliseconds lines it up.
