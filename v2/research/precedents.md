# Precedents — control surface for cYbErDeCk v2

*Research for the v2 control surface. Every claim here carries a URL. Where a
number is derived rather than cited, it says so. Nothing in this file proposes
firmware, syntax or verbs: the software hook already exists (`docs/MAP.md` §9.8)
and this is an enclosure and control-surface study.*

**The three findings that change the design:**

1. **An absolute pot lies the moment it is re-aimed at a different lane**, and
   it cannot be stepped blind — two real objections worth designing around.
   (The third reason originally given here, that the device has nothing for a
   pot to do, was wrong: `seq.h` already defines `SEQ_BIND_CC`. Struck — see
   the correction in §4. The pots stay.)
2. **At 13 mm key pitch, three key pitches (39 mm) is exactly MIL-STD-1472H's
   minimum edge-to-edge separation for a 14 mm knob** — so the encoder row and a
   ten-column thumb keyboard come out the same width, and the panel grid is
   derived rather than chosen. §6, §8.
3. **You cannot align four encoders to an 84.8 mm display.** 84.8 / 4 = 21.2 mm
   pitch, about half the ergonomic minimum. Either the display grows, or the
   controls are identified by engraved glyph + rim texture instead of by
   position — which is what OP-1 does with colour, and what norns was criticised
   for not doing at all. §5, §8.

---

## 1. What live coders actually reach for

### The best single piece of evidence

A Tidal performer's own four-knob mapping, posted when control busses shipped
([Tidal Club / lurk](https://uzu.lurk.org/t/tidal-1-7-control-busses-midi-control-input/2934)):
a Launchkey Mini's four knobs went to **`djf` (DJ filter), `squiz` (distortion),
`room` (reverb amount), `size` (reverb size)** — CC 21–24. Not tempo, not
transpose, not density. Four effect parameters. The thread's motivation is worth
quoting: one performer "always missed that continuos control of params", another
says immediate control was what they "really missed when I first started getting
into tidal".

Tidal's own documentation picks the same class of parameter for its examples:
`djfbus`, `roombus`, `squizbus`, `lpf`/`lpq`, `hpf`, `pan`
([control busses](https://tidalcycles.org/docs/reference/control_busses/)). Note
what the same page says **cannot** go on a bus: `n`, `note`, `gain`, `speed`,
`legato`, `sound`, `accelerate`. The architecture itself separates *continuously
sweepable* from *per-event* parameters, and the sweepable set is: filter,
distortion, space, pan.

Strudel's documentation reaches for the same two on its first example:
`.lpf(cc(0).range(0, 1000)).lpq(cc(1).range(0, 10))`
([MIDI & OSC](https://strudel.cc/learn/input-output/)).

### Discrete gestures, from the same sources

- **Engage/bypass a transformation.** Ghales keeps a transform (`chop`,
  `timeLoop`) applied through `all` but bypassed, with a function switching
  between `id` and the transform: "assign the control parameter to be a midi
  control (such as a button) you suddenly have a highly interactive performance
  element" ([Tidal profile](https://tidalcycles.org/blog/tidal_profile_ghales/)).
  The same post states the whole case for a hardware surface: "Using a separate
  controller for variables and using code to consume them makes a lot more sense
  personally."
- **Mute / solo per voice.** This deck's own primary performance gesture —
  "Run a line again to mute it. The whole performance gesture" (`docs/VERBS.md`)
  — plus the `mute` and `solo` verbs. Sixteen lanes.
- **Scene / pattern slot recall.** Teletype's keyboard-less path is the canonical
  hardware precedent: "Press the SCENE RECALL button… Use the PARAM knob to
  highlight your desired preset. Hold the SCENE RECALL button for 1 second"
  ([Teletype manual](https://monome.org/docs/teletype/manual.pdf)). One knob, one
  button, no keyboard.
- **Crossfade between two states.** Octatrack's crossfader "interpolates between
  the parameter values of scene A and scene B"
  ([Octatrack manual](https://www.elektron.se/wp-content/uploads/2024/09/Octatrack-User-Manual_ENG-OS1.40A_220204.pdf)).
- **Commit a knob value into the code.** Teletype binds `ctrl-alt` to "insert
  knob value scaled to 0..31" and `ctrl-shift` to "insert knob value scaled to
  0..1023" in Tracker mode ([manual](https://monome.org/docs/teletype/manual.pdf)).
  This is the only precedent I found for a physical control writing *text*, and
  it maps exactly onto this language, where "a digit is how much".

### Tempo: a measured range, not a guess

An information-retrieval study of the Algorave 10th-anniversary streams found
average tempo **122 ± 18 BPM, range 81–161 BPM**
([Chalmers](https://research.chalmers.se/publication/535526/file/535526_Fulltext.pdf)).
A tempo control needs roughly 80–170 BPM, not 20–999. The same study tracks
"event density" as a primary expressive dimension, with sets building to "two
overt climaxes" of density.

### What the literature says about *why* a knob beats code

The NIME 2018 workshop paper is the clearest statement, and it is worth reading
in full before you cut anything from the panel
([Re-engaging the Body and Gesture in Musical Live Coding](https://embodiedlivecoding.github.io/nime2018-workshop/workshop-paper.html)):

> "If in this example the sound source's volume is mapped to a typical gestural
> control such as a fader or knob, the process is somewhat different. As a
> performer adjusts the volume using the gestural control, they are able to
> quickly hear the audible result of their action, and continue or modify the
> overall gesture as desired. The translation of musical intent to audible result
> is a dynamic feedback loop between physical action and sonic effect."

and, on the cost of the code path:

> "the length of this feedback cycle—formation of a musical idea, devising the
> corresponding algorithm, evaluating the computer's result, and repeat—may
> impede the exploration of diverse musical ideas"

The counterweight, which should be read as a warning against over-fitting the
panel: Nilson argues live coding and conventional instrumental control are
different practices and should be valued as such, and that "it helps to separate
control from physicality first"
([NIME 2007](https://nime.org/proceedings/2007/nime2007_112.pdf)).
Salazar's survey of the same question is
[here](https://ccrma.stanford.edu/~spencer/publications/Salazar2017-SearchingForGestureAndEmbodimentInLiveCoding.pdf).

### Other systems, briefly

- **Sonic Pi** routes MIDI CC into the Time State; `get` "fetch[es] the last
  value without blocking the calling thread"
  ([tutorial 11](https://sonic-pi.net/tutorial-11.html)). A community "MIDI learn
  for Sonic Pi" helper exists
  ([in_thread](https://in-thread.sonic-pi.net/t/midi-learn-for-sonic-pi/7018)),
  which is evidence of demand rather than a shipped feature.
- **Hydra** has five function families — source, colour, geometry, blend,
  modulate — and four outputs `o0`–`o3`, with audio reactivity achieved by
  binding an FFT bin to any numeric parameter ([hydra](https://hydra.ojack.xyz/),
  [Hydra Book](https://hydra-book.glitches.me/)). The relevant lesson for this
  deck's picture half: *any* numeric argument is a modulation target, so the
  useful control is a generic "amount", not a named effect.
- **Orca** makes no sound; it emits MIDI, OSC and UDP, and has been built into
  dedicated Raspberry Pi boxes
  ([MOD WIGGLER build thread](https://www.modwiggler.com/forum/viewtopic.php?t=257717),
  [lines](https://llllllll.co/t/orca-livecoding-tool/17689)).

### The parameters from the brief that I think are *not* worth a control

- **Euclidean k/n.** `docs/STRUDEL.md` already records that `x(3,8)` is the
  "joint most-used thing" missing from this language. It is also two integers,
  not one continuous value, so it needs either two controls or a 2-D control.
  Don't put hardware on a notation that does not exist yet.
- **Transpose.** The language has `scale` and per-lane `:oct`. Both are
  discrete, both are typed in one or two characters, and a knob that slews
  transpose produces nonsense between detents.
- **FX sends.** No audio path in this firmware (`firmware/components/` is
  blemidi, dinmidi, seq, ensemble, net, ssh, viz, textgrid, cmd, kbd, st7305,
  usbdev, usbmux, docstore). A "send" here is a CC lane, which the encoders
  already cover.
- **Undo.** Keyboard work. Ctrl-Z belongs on a key you already have.

---

## 2. The precedents

| Precedent | Does well | Steal | Avoid |
|---|---|---|---|
| **monome Teletype** ([docs](https://monome.org/docs/teletype/), [manual](https://monome.org/docs/teletype/manual.pdf)) | The only mature dedicated live-coding hardware. **One** knob (`PARAM`, 14-bit 0–16383), one CV `IN`, one button, a USB keyboard, 8 trigger scripts + `M` + `I`. `PARAM.SCALE min max` narrows the knob's span. | (a) **Keyboard-less scene recall**: knob to highlight + hold one button 1 s. (b) `ctrl-alt` / `ctrl-shift` **insert the knob's value into the text as a literal**. (c) Four status icons (slew / delay / stack / metro) instead of a status page. | Its restraint is enforced by a 4 HP panel, not chosen. One knob is too few once you have a display worth labelling against. |
| **monome norns** ([docs](https://monome.org/docs/norns/), [SOS review](https://www.soundonsound.com/reviews/monome-norns)) | 3 encoders + 3 keys + 128×64 OLED, fully script-assigned. The purest "variable UI" device. | The K1-as-shift convention that emerged by itself. Flush-mounted display. | **This is the cautionary precedent for the whole v2 idea.** SOS: "There is no text or iconography anywhere on the unit"; "There's also no obvious alignment between the encoders/buttons and the screen", which rules out soft keys; "a pretty sparse performance interface"; and because scripts assign controls differently, "not all scripts cleanly adhere" to navigation conventions. A variable UI with no engraving and no alignment is unlearnable. |
| **monome grid** | 128 unlabelled varibright keys; meaning entirely in software. | Uniform, unlabelled, dense discrete surface as a *second* instrument, not the primary one. | A grid replaces the QWERTY argument; it does not answer "what do the knobs do". |
| **Ableton Push 2** ([manual](https://www.ableton.com/en/manual/using-push-2/)) | "the eight encoders above the display", with banks of eight buttons above *and* below the display, so every column is addressable; 8×8 pads at the near edge. Device Mode auto-maps the selected device to the eight encoders; Shift fine-tunes. | **Physical column alignment is what makes a soft label unambiguous.** Also: Shift = fine, on every encoder, for free. | Push's continuous controls are at the *far* edge, which only works on a deep desk panel. On a handheld the far edge is outside the thumb envelope (§6). |
| **Bitwig remote controls** ([KVR](https://www.kvraudio.com/forum/viewtopic.php?t=537009)) | Pages of 8; "there's never any binding or learning of MIDI messages, it just automatically controls the Remote Controls"; pages can be saved as a device default. | **A default mapping that ships with the thing being mapped.** The physical control is never meaningless. | Pages need a page indicator; without one the controls lie. |
| **Elektron Digitakt** ([SOS](https://www.soundonsound.com/reviews/elektron-digitakt)) | Five parameter-page buttons *under* the screen (TRIG / SRC / FLTR / AMP / LFO); "subtly rubberised, endless encoders [with] a noticeably higher resolution than those on previous devices"; 16 trig keys at the near edge doubling as a chromatic keyboard; secondary functions printed in orange. | Page buttons as a *physical* bank indicator. Endless encoders throughout. Secondary legends in a second colour, on the panel, permanently. | Low-res screen; the deck already beats it there. |
| **Elektron Octatrack** ([SOS](https://www.soundonsound.com/reviews/elektron-octatrack-dps1), [manual](https://www.elektron.se/wp-content/uploads/2024/09/Octatrack-User-Manual_ENG-OS1.40A_220204.pdf)) | The crossfader "interpolates between the parameter values of scene A and scene B". A single fader that morphs a whole state. | **The scene crossfader is the one slider that earns its place on any performance instrument.** Put it where a hand finds it blind. | "the encoders are a bit close together, meaning that it's fingertips-only operation most of the time". This is the exact failure mode of cramming four encoders into 85 mm. |
| **Teenage Engineering OP-1** ([layout guide](https://teenage.engineering/guides/op-1/original/layout)) | Four colour encoders; "The four color encoders are related to the graphical interface on the display"; "A green graphical element or text hints that the green encoder will change its value or position." T1–T4 under the screen, keyboard at the near edge. | **Identity by colour, not by position.** This is how you get a variable UI without physical alignment — and it is the fix for norns. | The deck's display is 1-bit reflective, so *colour* cannot come from the screen. Substitute an engraved glyph + a rim texture (§8). |
| **TE OP-Z / PO series** | No screen at all on OP-Z (it borrows a phone); 16 keys carry everything. | A 16-key row is a general-purpose addressing surface: steps, slots, lanes, mutes. | Depending on a phone. The PO series' bare-PCB controls are novelty, not ergonomics. |
| **TE TX-6** ([product](https://teenage.engineering/products/tx-6), [Elektronauts](https://www.elektronauts.com/t/new-teenage-engineering-products/16943/3904), [Equipboard](https://equipboard.com/items/teenage-engineering-tx-6)) | CNC aluminium, custom encoders and faders, 90 × 62 mm. Proof that a slim dense surface is manufacturable. | Tiny faders *are* playable at that scale — the criticism lands on the knobs, not the faders. | **The clearest "do not do this".** Users report the knobs are "way too small for anything else than 'set and forget'"; "closely placed, tiny knobs can be challenging to operate"; and on feel, "imprecise and 'jump' between values". If you shrink four encoders to fit under the display, this is the review you get. |
| **Critter & Guitari Organelle** ([manual](https://raw.githubusercontent.com/critterandguitari/Organelle_Manual/master/manualORIG.md)) | Four parameter knobs on the left, a Selector encoder and Vol on the right, display between, 24 maple keys across the near edge. "Each knob is typically assigned to a parameter." Aux key at the far left of the key row. | Side-by-side display and controls in one far band, keys across the near edge. A dedicated *selector* encoder distinct from the parameter knobs. | One reviewer notes "there is no labeling on the top panel indicating any of the aux controls" — same disease as norns. |
| **Soma Pulsar-23** ([product](https://somasynths.com/pulsar-23/)) | 55 knobs, 11 switches, 23 modules, "over 100 inputs and outputs", 30 alligator-clip cables, and "the use of the artist's body conductivity to create patches and cross modulations". Density that is still legible because it is grouped into named modules. | **Density with grouping.** Also: construction as finish — the "Turret version uses custom turrets", the cheap one "used screws instead of expensive custom turrets", and both are sold openly. | Patch cables and body conductivity are not handheld. |
| **Soma Terra** ([product](https://somasynths.com/terra/)) | "TERRA has no display"; a "6 LED-sensor triangle in the center" that *shows and sets* parameter data; a three-axis motion sensor for "modulation through hand and body movement"; wooden housing from three pieces, brass logo inlay, and "joints between different parts of the wood may be visible". An "extremely simple interface that gives you instant access to all its functions in several touches". | (a) The **motion sensor as a legitimate modulation source on a held instrument** — Soma ships exactly the IMU idea. (b) Visible joints and one precious inlay as the whole decorative budget. (c) One element that is both the readout and the control. | Its stated aim is to free musicians from "long exhausting programming". This deck *is* programming. Terra's no-display answer is the opposite of this instrument's thesis; take the feel, refuse the conclusion. |
| **Soma Lyra-8 / Kreimer's "Romantic Engineering"** ([Soma](https://somasynths.com/), [Synthtopia](https://www.synthtopia.com/content/2023/12/21/synth-design-master-class-with-vlad-kreimer-of-soma-laboratory/), [FutureMusic](https://futuremusic.com/2026/05/soma-labs-lyra-8-review/)) | "Organismic" instruments; the philosophy is explicit and precedes the panel. Reviewers note Lyra-8 has no MIDI control and that this is hard to get used to at first. | The idea that the instrument's **character is a design requirement**, stated up front, not an emergent property. | No external control. This deck's whole value is that it *is* the controller. |
| **Bastl** ([wiki](https://electronicmusic.fandom.com/wiki/Bastl_Instruments), [Skis](https://patch.audio/en/product/bastl-instruments-skis-full-diy-kit-wooden-panel/)) | Oak front panels with "handmade custom knobs"; wood as the panel material, not a trim. | Wood as a *panel*, which is exactly where a 3D-printed deck can insert a real material cheaply. | Faux-woodgrain graphics (their later move) read as cheap where real wood reads as considered. |
| **Make Noise 0-Coast / Strega, Erica Perkons, Landscape** | Knob-per-function, no menus; Landscape's touch contacts as controls. | Knob-per-function as a discipline: if a parameter has no knob, it has a *name*, and naming is this deck's strength. | Eurorack panel conventions assume a 128.5 mm tall rack-mounted plate and a seated player with both arms free. None of that holds here. |
| **Dirtywave M8 / Polyend Tracker** | Handheld text/grid instruments with no knobs at all, driven by chorded buttons. | Proof that a handheld text instrument can be fully playable with discrete controls only — i.e. the encoders are a luxury, the keys are the instrument. | Chorded UIs are unlearnable without a printed card; the deck's one-page `VERBS.md` is the better answer. |

---

## 3. The variable-UI idea — prior art, in one page

The brief's "the UI itself can be variable" is already solved in this codebase,
so this section is context only.

`docs/MAP.md` §9.8 records the decision: an input is "a name with a value,
defined like any other name, and it adds no verb" — `>knob1 = knob`,
`>pad1 = pad`, `>route cut knob1` — with the value held on the deck, the knob
firing its routes on the next tick and a pad on the next step. `>lanes` already
prints each input with its value and its source. `seq_input_set(name, value,
from)` in `firmware/components/seq/include/seq.h` is callable "From any task",
so a panel driver needs no new software architecture.

The prior art worth knowing:

- **Tidal `cF` / `cI` / `cS` / `cT` / `cR`** — the closest match. `/ctrl` over
  OSC on port 6010 carries a key and a value; a pattern reads it with a default
  until the first message arrives: `d1 $ sound "bd" # speed (cF 1 "hello")`
  ([Controller Input](https://tidalcycles.org/docs/working-with-patterns/Controller_Input/)).
  Two details this deck should know: raw MIDI is high-resolution, so the docs
  recommend `segment`ing it; and linear 0–1 feels wrong on a cutoff, where the
  community answer is `rangex`, the exponential range
  ([lurk](https://uzu.lurk.org/t/logarithmic-midi-control-in-tidal/4486)).
  Teletype's answer to the same problem is `PARAM.SCALE min max` — narrow the
  span rather than bend the curve.
- **Strudel** binds a CC reading straight into a parameter:
  `.lpf(cc(0).range(0, 1000))`, with `midin()` opening the port and `midimaps`
  handling only the *output* direction
  ([docs](https://strudel.cc/learn/input-output/)).
- **Push Device Mode** and **Bitwig remote controls** are the same idea with a
  shipped default: the control is auto-mapped to whatever is selected, and
  "there's never any binding or learning of MIDI messages"
  ([Ableton](https://www.ableton.com/en/manual/using-push-2/),
  [KVR](https://www.kvraudio.com/forum/viewtopic.php?t=537009)).

**The enclosure's job, given all that:** make the physical side legible. The
norns review is the proof that it does not happen by itself — unlabelled,
unaligned, script-assigned controls produce a device where "it's unclear what the
box does". Every control on v2 must carry a permanent engraved identity *and* a
software-assignable meaning. The engraving is not a default; it is the control's
name, the way `kick` is a lane's name.

---

## 4. Recommended control complement — and what to cut

### Verdict on the proposed set

> **Correction (checked against the firmware, not inferred from it).** The
> claim below that "there is nothing for them to do" is wrong, and the reason
> it is wrong matters: this deck is a **MIDI instrument**, not a synth. It has
> no audio path because it was never going to have one — it drives external
> gear over `blemidi` and `dinmidi`. And `firmware/components/seq/include/seq.h`
> already defines **`SEQ_BIND_CC`**, documented as "a controller: 0 is 0 and 9
> is 127", with a `cc` field on the binding struct. A continuous controller
> sending MIDI CC is a first-class citizen of this firmware *today*. Absence of
> a DAC says nothing about whether a knob has a job; a sequencer that sends CC
> is exactly the thing a knob has a job on.
>
> Reasons (1) and (2) below — that an absolute pot lies when re-aimed, and that
> it cannot be stepped blind — stand on their own and are worth designing
> around. Reason (3), the well geometry, is a real constraint and is why §7
> exists. But "nothing to do" is struck. **The pots stay; the user confirmed
> standard panel-mount pots deliberately.**


| Proposed | Verdict | Why |
|---|---|---|
| **3 pots** | **Kept** — but read the correction above this table first. | Three independent reasons, any one of which is sufficient. (1) A pot's absolute position *lies* the moment the control is re-aimed at another lane — which is the entire premise of v2. The industry fix is soft-takeover/pickup mode, which is an invisible mode, and the panel can't show it. (2) No detents, so no blind stepping and no "one more" gesture. (3) The pleasant pot knob is big: the canonical Davies 1900H is **12.7 mm diameter × 16.0 mm tall** ([Digi-Key](https://www.digikey.com.au/en/products/detail/davies-molding-llc/1900-H/6566304)), which cannot nestle in a slim panel without a well so wide it destroys the pitch (§7). And there is nothing for them to do: there is **no audio path anywhere in this project** — `firmware/components/` has no I²S, DAC or codec driver and `docs/HARDWARE.md` does not mention audio at all — so there is no master level; and the display is reflective, so there is no backlight to dim. |
| **3 encoders** | **Keep, make it 4.** | Endless + detented = re-mappable with no value jump, blind stepping, and a free discrete action per control via push. Four because the observed real-world mapping is four effect parameters (§1) and because four is the figure both OP-1 and Organelle landed on independently; eight (Push, Bitwig, Elektron) needs a desk. |
| **2 sliders** | **Keep one. The second is redundant.** | MIL-STD-1472H is unusually blunt about faders: "Slide-levers may be used for low-force, continuous-adjustment, or gross-mode selection (**do not use for precise setting**)" (Table XI, [PDF](https://cvgstrategy.com/wp-content/uploads/2023/04/MIL-STD-1472G.pdf)). Exactly one quantity in a live-coding set has that shape — the coarse, bipolar, see-it-across-the-room A/B morph, i.e. the Octatrack crossfader. A second fader invites "two cutoffs", which the standard says faders are bad at, and which the encoders do better. |
| **4 keys** | **Keep the count. Change what they are.** | 4 keys cannot address 16 lanes without a bank modifier, and a bank modifier is an invisible mode. So stop thinking of them as pads. They are **engraved function keys for the gestures you need when a hand is off the keyboard or when typing is too slow**: `play`/`stop`, mute-the-lane-under-the-cursor, commit, scene. Per-lane mute belongs on the QWERTY number row, which already exists and is free. Add a **fifth, guarded** key for `panic` — `docs/OS.md` already requires "a physical, no-screen-needed way to recover", and MIL-STD-1472H 5.1.4.2.1.1.4 requires "A channel or cover guard… when accidental actuation of the control must be prevented". |
| **IMU** | **Keep, demote to a system sensor.** | Soma ships this exact idea on Terra ("a three-axis motion sensor… allows modulation through hand and body movement"), so it is not a bad instinct. But on a *handheld*, orientation is reading posture, not a free control axis: tilting takes the display off-axis and the thumbs off home. And the sensor is documented as problematic — a NIME hand-posture study notes IMUs "suffer from drift and bias issues" and calls them "the most intrusive in terms of disrupting the performer's natural hand state" ([NIME 2018](https://nime.org/proceedings/2018/nime2018_paper0054.pdf)); integrating acceleration "can lead to considerable drift" ([PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC7583031/)); heading drift needs a magnetometer, which is unreliable indoors and on a stage ([PMC](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC6471153/)). **So: no continuous musical parameter on the IMU.** Use it for coarse, latched, non-musical events — a deliberate shake as a second `panic`, face-down to blank, and (the real win) orientation sensing so the deck can flip left-handed. |
| **ToF** | **Cut from v2.0. Reserve the footprint.** | To use it a hand must leave the keyboard — so it competes head-on with an encoder and loses on precision, on blind repeatability and on return-to-value: no detents, no end-stops the hand can feel, no visible position. It also cannot be always-live on a device you hold, because your own forearm will trigger it. Its one real advantage is that **the room can see the gesture**, which is a genuine deficit of live coding — the audience watches you type. If that matters, keep it, arm it explicitly, and never let it hold anything you would miss. Priority last of the six. Leave an I²C pair and a top-edge window. |

### What is missing from the proposed set

1. **Push-click on every encoder.** A free discrete action per control, zero panel
   area. This is how you get four more keys without four more holes.
2. **A top-edge control reachable while both thumbs stay on the keys.** This is
   the real hole. Every control in the proposed list requires a thumb or a hand to
   leave the keyboard. On a two-handed thumb-typed deck the only controls you can
   move *while typing* sit under the index fingers, on the top edge or the back —
   the shoulder-button position. Two thumbwheels there (one per hand) would be the
   single biggest expressive gain in the whole design, and they are the thing that
   makes this a performance instrument rather than a laptop with knobs. The body is
   also the right one for a slim panel: MIL-STD-1472H explicitly blesses it —
   "Continuously adjustable thumbwheel controls may be used as an alternative to
   rotary knobs when the application will benefit from the compactness of the
   thumbwheel device" (5.1.4.1.2.3). It is the Braun T3 tuning wheel and the
   camera command dial.
3. **A commit key.** The Teletype trick (knob value → text literal) has no
   equivalent anywhere else and it fits this language's "a digit is how much"
   exactly. It needs a key, and it is the most interesting key on the panel.
4. **A distinct selector.** Organelle separates its Selector encoder from its four
   parameter knobs for a reason: navigation and parameter editing should not share
   a control, or you lose your place. Satisfy it with encoder 4 + the scene key
   rather than new hardware.

### Recommended complement

| Count | Control | Default | Why it earns the space |
|---|---|---|---|
| 4 | Detented push encoder, face, in one row under the display | `cut` (CC 74), `res` (CC 71), `rev` (CC 91), `bpm` | The measured real-world mapping is four effect parameters (§1). Endless so re-aiming never jumps. Push = latch/unlatch that encoder's route. Encoder 4 doubles as the scene selector. |
| 2 | Thumbwheel, top edge, one per hand | left: the amount of the lane under the cursor; right: `cut` | The only controls movable without leaving the keys. MIL-blessed slim body. |
| 1 | Fader, 45 mm travel, near-left / front edge | A/B scene morph | Octatrack's one great idea; the only quantity whose shape is "coarse and bipolar". |
| 4 | Engraved function key, face, flanking the encoder row | `play`/`stop`, mute-this-lane, commit, scene | The gestures that must beat typing. |
| 1 | Guarded key, top edge | `panic` | Required by `docs/OS.md`; guard required by MIL 5.1.4.2.1.1.4. |
| 1 | IMU (internal) | orientation / flip / shake-to-panic | Nearly free; wrong for continuous control. |
| 0 | Pots | — | See above. |
| 0 | ToF | — | Footprint reserved only. |

Twelve controls. Per-lane mute, pattern slot select, transpose, scale and tempo
entry all stay on the QWERTY, where they are two to six characters.

---

## 5. Arrangement — band order and the hand journey

### What the precedents actually do

| Device | Far edge → near edge | Source |
|---|---|---|
| Push 2 | 8 encoders → 8 buttons → **display** → 8 buttons → 8×8 pads | [manual](https://www.ableton.com/en/manual/using-push-2/) |
| OP-1 | **display** → T1–T4 track keys → 24-key keyboard. The four colour encoders are "related to the graphical interface on the display"; the guide's numbered key does not state their row, so read the position from a photo before relying on it. | [layout](https://teenage.engineering/guides/op-1/original/layout) |
| Digitakt | **screen** → 5 parameter-page buttons → encoders → 16 trig keys | [SOS](https://www.soundonsound.com/reviews/elektron-digitakt) |
| Organelle | one far band — knobs 1–4 at the left, **display**, Selector then Vol at the right → Aux key + 24 maple keys across the near edge. (The manual places the knobs "on the left" and Vol rightmost but does not fix the display's exact position.) | [manual](https://raw.githubusercontent.com/critterandguitari/Organelle_Manual/master/manualORIG.md) |
| Octatrack | **screen** + encoders → trig keys, with the crossfader on its own away from the encoder cluster | [manual](https://www.elektron.se/wp-content/uploads/2024/09/Octatrack-User-Manual_ENG-OS1.40A_220204.pdf) |
| norns | **screen**, with 3 encoders and 3 keys beside it and no alignment | [SOS](https://www.soundonsound.com/reviews/monome-norns) |
| Terra | no display; a 6-LED sensor triangle in the centre → 12 note sensors | [product](https://somasynths.com/terra/) |

**The one invariant across all of them: the surface the hands strike is at the
near edge, the display is at the far edge, and the continuous controls sit
adjacent to the display — never beyond the striking surface.** Nobody puts knobs
below the keys.

### Judgement on the four options

- **A — display / controls / keyboard. Correct, and this is what I recommend.**
  Three independent devices converged on it (OP-1, Digitakt, Organelle). It is
  also the only option the thumb envelope permits: on a two-handed thumb-typed
  deck, 90 % of adults reach 45.6–49.4 mm from the near edge, with mean maximum
  thumb reach between 51.8 mm and 61.4 mm depending on the population
  ([Ergoweb summary of the reach-envelope study](https://ergoweb.com/thumb-reach-distances-and-envelopes-for-handheld-devices/)).
  The control band's centreline must therefore sit **within ~45 mm of the
  keyboard's top edge**, which only option A satisfies by construction.
- **B — display / keyboard / controls. Wrong.** No precedent anywhere. It puts
  the controls beyond the striking surface, so every reach crosses the keys, and
  it puts the controls two bands from the display, so the display can never
  plausibly label them.
- **C — controls / display / keyboard. Push does this, and it is wrong here.**
  Push is a desk instrument: the player's hands rest on the pads, the arms are
  free, and the far edge of a ~300 mm-deep panel is a short reach for a seated
  player. On a 150–200 mm handheld held in two hands, the far edge is 45–60 mm
  *beyond* the thumb envelope and cannot be reached without re-gripping. **Keep
  C's good idea and move it into Z:** controls that must not interrupt the eye
  path from display to hands belong on the **physical top edge**, under the index
  fingers, not on the far end of the face. That is where the thumbwheels and
  `panic` go.
- **D — display and controls side by side, keyboard below. Half-right; it is the
  Organelle, and it fails on this display.** It is the correct answer when the
  display is small relative to the body width. Here it is not: 84.8 mm of active
  area in a ~150–200 mm body. A side-by-side band spends ~60–110 mm of prime real
  estate on two or three controls and destroys any chance of column alignment.
  **Reject as the primary order; adopt it locally** — the fader and the scene key
  can sit beside the display rather than below it, which is exactly what Octatrack
  does with its crossfader.

### The hand journey this produces

```
   ┌──────────── top edge: thumbwheel L · panic (guarded) · thumbwheel R ───┐
   │                                                                        │
   │   ┌───────────────────── display ──────────────────────┐    fader      │   eyes
   │   │                                                    │      ║        │
   │   └────────────────────────────────────────────────────┘      ║        │
   │        ◉ cut      ◉ res      ◉ rev      ◉ bpm          ▪ ▪    ║        │   one thumb
   │                                                                        │
   │   ┌──────────────── keyboard (both thumbs, home) ───────────────────┐  │   both thumbs
   └───┴─────────────────────────────────────────────────────────────────┴──┘
```

- **Nothing leaves the keys** for the thumbwheels — index fingers are already
  wrapped round the top edge.
- **One thumb lifts ~30–40 mm** for the encoders and the function keys, inside the
  reach envelope, and the other thumb keeps the grip.
- **A full hand releases** only for the fader and scene recall — both deliberate,
  both bottom-right, both findable blind by the edge.

---

## 6. Hard numbers

The authoritative free source for control geometry is **MIL-STD-1472**. The copy I
used is [here](https://cvgstrategy.com/wp-content/uploads/2023/04/MIL-STD-1472G.pdf)
(the file is named …1472G but its running header reads **MIL-STD-1472H**; cite it
as H). The FAA's Human Factors Design Standard chapter 6 restates the same tables
and is a readable cross-check, but its exhibits are raster images:
[Ch6 Controls and visual indicators](https://hf.tc.faa.gov/hfds/download-hfds/hfds_pdfs/Ch6_Controls_and_visual_indicators.pdf).

### 6.1 Minimum edge-to-edge separation, MIL-STD-1472H Table III

All values are one-hand, bare-handed.

| | Toggle | Push button¹ | Continuous rotary | Rotary selector | Thumbwheel |
|---|---|---|---|---|---|
| **Toggle** | fig 16 | 13 mm | 19 mm | 19 mm | 13 mm |
| **Push button¹** | 13 mm | fig 14 | **13 mm** | 13 mm | 13 mm |
| **Continuous rotary** | 19 mm | **13 mm** | fig 12/10 | 25 mm | 19 mm |
| **Rotary selector** | 19 mm | 13 mm | 25 mm | fig 6 | 19 mm |
| **Thumbwheel** | 13 mm | 13 mm | 19 mm | 19 mm | fig 8 |

¹ "For push buttons not separated by barriers."

### 6.2 Knobs — MIL-STD-1472H Figure 10

| | H, height | D, diameter |
|---|---|---|
| Fingertip grasp, min | **13 mm** | **10 mm** |
| Fingertip grasp, max | 25 mm | 100 mm |
| Thumb-and-finger encircled, min | 13 mm | 25 mm |
| Palm grasp, min | — | 38 mm |

Separation, edge to edge: **one hand individually — min 25 mm, optimum 50 mm**;
two hands simultaneously — min 50 mm, optimum 125 mm.
Max torque: **32 mN·m** for knobs ≤ 25 mm diameter, 42 mN·m above.
Rims: "Very small knobs shall have knurled surfaces"; "Intermediate and larger
size knobs shall have serrated rims for low torque applications"; "rim
indentation rather than serration… for high torque applications" (5.1.4.1.2.1.4).
And the licence to go small: "Within these ranges, knob size is relatively
unimportant, provided the resistance is low and the knob can be easily grasped";
"When panel space is extremely limited, knobs shall approximate the minimum
values."

### 6.3 Thumbwheels (continuous) — MIL-STD-1472H Figure 12

| | Rim exposure E | Width W | Separation S | Resistance |
|---|---|---|---|---|
| Min | 25 mm (preferred)* | 3 mm (preferred)* | 25 mm (arr. A) / 50 mm (arr. B) | — |
| Max | 100 mm | 23 mm | — | 3.3 N |

\* The note is the important part: "Preferred; some miniature applications may
require less." The rim "shall be serrated or provided with a high friction
surface."

### 6.4 Push buttons — MIL-STD-1472H Figure 14 (non-keyboard)

Fingertip diameter **min 10 mm, max 25 mm**. Displacement **min 2 mm, max 6 mm**.
Separation, single finger: **min 13 mm, preferred 50 mm**; single-finger
sequential or different fingers: min 6 mm, preferred 13 mm.
And: "The push button surface **shall be concave (indented)** to fit the finger"
(5.1.4.2.1.1.2) — recessing a key is the standard, not a compromise.

### 6.5 Keyboards — MIL-STD-1472H Table VII

| | Key top (square) | Separation between adjacent key tops |
|---|---|---|
| Min | **10 mm** | **6.4 mm** |
| Preferred | **13 mm** | 6.4 mm |
| Max | 19 mm | — |

Displacement, alphanumeric: min 1.3 mm, max 6.3 mm. Resistance 0.25–1.5 N,
preferred 0.5–0.6 N. Membrane keys: preferred displacement 0.7 mm, resistance
≥ 2.0 N, and they "shall also incorporate positive tactile feedback".

Pitch: key top + separation = **16.4 mm minimum, 19.4 mm at the preferred key
top** — which is why the industry number is 19 mm. ANSI/HFES 100 and ISO call for
"19 mm ± 1 mm" centre-to-centre horizontally and vertically for alphanumeric and
numeric zones ([key-spacing study](https://www.researchgate.net/publication/247154064_The_Effect_of_Keyboard_Key_Spacing_on_Typing_Speed_Error_Usability_and_Biomechanics_Part_1)),
with ANSI/HFES 100-2007 deferring the layout itself to ISO/IEC 9995
([quoted here](https://e3w2q.github.io/14/)). Outside those zones, pitch may
differ but "should be at least 15 mm", with a recommended 12 mm strike surface
([Deskthority, reporting the standards](https://deskthority.net/viewtopic.php?t=7152)).

### 6.6 Slide levers / faders — MIL-STD-1472H Table XI

Handle depth D **min 13 mm** (the long dimension, perpendicular to travel, so it
serves as a pointer); width W **min 6.5 mm**; height H **min 16 mm**; separation S
**min 19 mm**. Resistance 2.8 N–110 N. Functional increase shall be up / right /
forward. And the use statement: "low-force, continuous-adjustment, or gross-mode
selection (do not use for precise setting)."

### 6.7 Blind operation, and why spacing cannot solve it here

> **5.1.1.6 Blind operation.** "Where 'blind' operation is necessary, hand
> controls shall be shape coded, or separated from adjacent controls by at least
> **125 mm (5 in)**."

125 mm is most of this panel's width. **So on a device this size, every control
the performer uses without looking must be shape- or size-coded.** The standard
then bounds the coding vocabulary:

- Size coding: "No more than **three** different sizes" (5.1.1.4.3); knob
  diameter differences "**not less than 13 mm**" (5.1.1.4.3.2); knob thickness
  differences "not less than 10 mm" (5.1.1.4.3.3).
- Shape coding: "not more than **10**" shapes, identifiable by hand and eye
  "regardless of the position and orientation", and tactually identifiable with
  gloves (5.1.1.4.4.1).
- Location coding: controls for similar functions "shall be in the same relative
  location from workstation to workstation, from panel to panel" (5.1.1.4.2.2) —
  which, read against a software-remappable surface, is an argument for the
  engraved identity never moving even when the meaning does.

### 6.8 Thumb reach, for band placement

Maximum thumb reach means on handheld devices: **51.8 mm** (young mothers,
SD 7.1), **58.9 mm** (adolescent females), **61.4 mm** (young fathers); 90 % of
participants reached **45.6–49.4 mm** from the bottom edge
([Ergoweb](https://ergoweb.com/thumb-reach-distances-and-envelopes-for-handheld-devices/)).
The comfortable far limit "equaled to the thumb length when control angle was
about 60°" ([Springer](https://link.springer.com/chapter/10.1007/978-3-319-21070-4_5)).
Target placement within the envelope matters too: in a one-handed study the
centre was most comfortable and the south-east region least
([Microsoft Research](https://www.microsoft.com/en-us/research/wp-content/uploads/2006/01/parhi-mobileHCI06.pdf)).

**Use 45 mm as the hard limit** from the keyboard's top edge to the control row's
centreline.

### 6.9 Real parts, with depths — the numbers that constrain the enclosure

| Part | Numbers | Source |
|---|---|---|
| **Bourns PEC11R** 12 mm incremental encoder | Body **12.5 × 13.4 mm**; **7.0 mm deep behind the mounting surface**; bushing **M7 × 0.75**, 5.5 mm or 6.5 mm above panel depending on variant; shaft 6.0 +0/−0.1 mm; detent torque **30–90 gf·cm (2.9–8.8 mN·m)**, well inside MIL's 32 mN·m; 12 or 24 detents; IP40; 5 g | [datasheet](https://www.bourns.com/docs/Product-Datasheets/PEC11R.pdf) |
| **Davies 1900H** — the canonical synth knob | **12.70 mm** (0.500 in) base diameter, **16.00 mm** (0.630 in) tall, 0.250 in (6.35 mm) shaft, 6-32 set screw; clone sellers give the top as 11.9 mm, i.e. slightly tapered. The larger 1914 is 0.75 in diameter × 0.50 in. | [Digi-Key](https://www.digikey.com.au/en/products/detail/davies-molding-llc/1900-H/6566304) |
| **Bourns PTA** slide pot | Travel options **15, 20, 30, 45, 60 mm**; 2 × M2 × 0.4 mounting; dust-cover option in the ordering code | [datasheet](https://static.chipdip.ru/lib/158/DOC011158765.pdf) |
| **Alps RS60N11 / RSA0N11** | 60 mm and 100 mm travel families; the low-profile RSA0N11S body is 128 × 8.4 × 8 mm | [Alps](https://tech.alpsalpine.com/prod/e/html/potentiometer/slidepotentiometers/rsn1m/rsn1m_list.html) |
| **Kailh Choc v1** low-profile key switch | **18 × 17 mm** PCB spacing; total travel **3.0 ± 0.5 mm**, actuation 1.5 ± 0.5 mm; switch body ~8.6 mm thick (PG1350 outline quoted as 15 × 15 × 8 mm); v1 keycaps 17.5 × 16.5 mm; v1 and MX-low-profile cannot share a PCB | [Kailh](https://www.kailh.net/products/kailh-choc-v2-low-profile-switch-set), [Keycapsss](https://keycapsss.com/switchestester/switches/65/kailh-low-profile-choc-switches-v1), [comparison](https://botmonster.com/hardware/low-profile-mechanical-switches-kailh-cherry-gateron/) |
| **Rii 518BT** (v1's keyboard) | 108.5 × 58.2 × 10.2 mm, per `README.md`; pocket 109.85 mm, per `cad/parameters.scad:748`. A ten-column QWERTY across that width is ≈ **10.5 mm pitch** *(derived)* — at MIL's bare minimum 10 mm key top and well under the 13 mm preferred. | repo |

### 6.10 Community practice, for sanity

| | Pitch | Source |
|---|---|---|
| MOTM-style small-knob panel, 16 mm pots | 25.4 mm vertical × 21.6 mm horizontal (1.0″ × 0.85″) | [pugix](https://pugix.com/synth/small-knob-panel-group/) |
| Serge-style | ~19–25 mm | [MOD WIGGLER](https://www.modwiggler.com/forum/viewtopic.php?t=76426) |
| Eurorack DIY | 30 mm horizontal (1 pot per 6 HP), 25.4 mm vertical | [MOD WIGGLER](https://www.modwiggler.com/forum/viewtopic.php?t=76426) |
| Guitar pedals | ~20 mm between pot centres is already "a little too close for comfort" for a 20 mm knob | [PedalPCB](https://forum.pedalpcb.com/threads/big-knobs-how-big-have-you-gone.1734/) |

Note the gap: a 16 mm-knob panel at 21.6 mm pitch gives only 5.6 mm edge-to-edge,
far below MIL's 25 mm. Modular panels are routinely four times tighter than the
standard because the player is seated with both arms free and *looking* at the
panel. A handheld played standing, used partly blind, should respect the standard.

### 6.11 Derived recommendations for this device

All derived; the inputs are cited above.

| Dimension | Recommendation | Derivation |
|---|---|---|
| Key pitch (QWERTY) | **13 mm**, thumb-typed; 18 mm only if you commit to ten-finger | 13 mm is MIL's *preferred key top* used as the pitch, giving a 10-column field of 130 mm. 18 mm (Choc) is inside ANSI's 19 ± 1 mm and gives 180 mm — a ~196 mm-wide device, i.e. a small tablet. A performance instrument played standing has nowhere to rest wrists, so thumb-typing is the right posture and 13 mm the right pitch. |
| Key top / travel | 11 mm cap on 13 mm pitch (2 mm gap); 2–3 mm travel; concave tops | MIL Table VII min key top 10 mm; separation min 6.4 mm is *not* achievable at 13 mm pitch — **state this departure** and buy it back with travel and a hard tactile click (MIL 5.1.4.2.1.1.3.1: "Tactile feedback shall be the primary form of positive indication"). |
| Encoder knob diameter | **14 mm**, 9–11 mm graspable height | Between the Davies 12.7 mm and the 16 mm-pot convention; MIL Fig 10 fingertip min D 10 mm, min H 13 mm. |
| Encoder pitch | **39 mm = 3 × key pitch** | 14 mm knob + MIL's 25 mm one-hand minimum = 39 mm exactly. Four encoders span 3 × 39 + 14 = **131 mm**, which is the same width as a 10-column 13 mm keyboard (130 mm). The row and the keyboard are the same object width — derived, not chosen. |
| Encoder → nearest key | **≥ 30 mm centre-to-centre** | MIL Table III rotary↔push button = 13 mm edge-to-edge; 7 (knob radius) + 13 + 5.5 (key cap half) = 25.5 mm, rounded up for the hand's approach path. |
| Function key pitch | **19.5 mm = 1.5 × key pitch**, 13 mm caps, concave | MIL Fig 14 single-finger separation min 13 mm → 13 + 13 = 26 mm would be ideal; 19.5 mm is the compromise, is within ANSI's 19 ± 1 mm key unit, and keeps the panel grid whole. |
| Fader travel | **45 mm** | Bourns PTA offers 45 mm; 60 mm will not fit beside an 84.8 mm display in the same band. |
| Fader cap | 13 mm along travel × 7 mm across, 1 mm concave dish on top | MIL Table XI D min 13 mm, W min 6.5 mm. |
| Thumbwheel | 20–24 mm diameter, 6–8 mm wide, 90–120° of arc exposed, serrated rim | MIL Fig 12: W min 3 mm "some miniature applications may require less", rim must be serrated or high-friction, max resistance 3.3 N. |
| Control row centreline | **≤ 45 mm** above the keyboard's top edge | §6.8 — the 90 % thumb reach band. |
| Tempo control span | 80–170 BPM | Measured Algorave range 81–161 BPM. |

---

## 7. Recessed and low-profile controls

**There is no standard for this.** I searched for one. The counterbore literature
is about fastener heads, not fingers; the cabinet-hardware literature is the
closest ergonomic analogue and says "most hands need at least about 3/4 inch
[19 mm] of vertical clearance to comfortably hook fingers under a recessed pull"
([Knobs.co](https://knobs.co/blogs/articles/finger-pulls-for-handleless-flat-front-cabinets)),
which is a useful order of magnitude and nothing more. Every ergonomic standard I
found assumes a control standing proud. So the guidance below is derived, and the
derivation is shown.

### 7.1 The governing insight: grip is an area problem, not a height problem

MIL's knob minimum height is 13 mm (Fig 10). But what the standard is really
specifying is **13 mm of graspable cylinder** — it says nothing about where the
panel plane cuts that cylinder. The thumbwheel table proves the point from the
other side: a wheel only **3 mm wide** is acceptable provided its *rim exposure*
is generous, because a thumb grips the arc it can touch. A thumbwheel trades
height for arc. **That is the only way to be both slim and grippable, and it is
why the top-edge thumbwheels in §4 are the most important recommendation in this
document.**

### 7.2 Derived rules for a recessed rotary

- **Annular gap.** For a knob of diameter *D* in a well of diameter *W*, the
  finger reaches the knob's side only through the gap *g* = (*W* − *D*)/2. Allow
  **g ≥ 6 mm** for a fingertip edge, **g ≥ 10 mm** for a finger pad. A 14 mm knob
  therefore wants a **26–34 mm well**, which consumes most of a 39 mm pitch.
  **Conclusion: a deep well and a dense knob row are mutually exclusive.** Pick
  one. If you want them nestled, go to 45–50 mm pitch (still inside MIL's 50 mm
  optimum) and accept three encoders instead of four, or use thumbwheels, which
  need no annular access at all.
- **Chamfer, don't bore.** A vertical-walled well of depth *r* blocks everything
  below *r*. Chamfer the well mouth at **30–45°** so the approach angle of the
  finger matches the wall, and the usable contact arc is recovered at the knob's
  upper rim.
- **Exposed height.** Keep **≥ 8 mm of graspable cylinder above the chamfer
  line** and recess **no more than one third of the knob's height**. (13 mm MIL
  minimum less a one-third recess ≈ 8.7 mm.)
- **Protection without burial — the cheap trick.** The purpose of recessing is
  that the deck can be laid face-down or dropped on its face without loading a
  shaft. That requires only that **the knob's top sits ≥ 0.5 mm below the plane of
  the highest surrounding land** — and the land can be *raised* instead of the
  knob *sunk*. A 1.5–2 mm rib around the control band, or using the display bezel
  and the top edge as the two high points of the whole face, buys the full
  protection benefit at zero cost in grip. **Do this first; only recess further if
  the profile still demands it.**
- **Torque budget.** The PEC11R's 30–90 gf·cm (2.9–8.8 mN·m) is far below MIL's
  32 mN·m ceiling, which matters because a recessed knob is gripped with less
  mechanical advantage. Do not choose a high-detent-torque encoder for a recessed
  position.

### 7.3 Keys

For keys, recessing is not a compromise — it is the standard: "The push button
surface **shall be concave (indented)** to fit the finger" (MIL 5.1.4.2.1.1.2),
with the fallback "When a concave surface is impractical, the surface shall
provide a high degree of frictional resistance". Displacement 2–6 mm fingertip.

For `panic`, the standard requires a guard: "A **channel or cover guard** shall be
provided when accidental actuation of the control must be prevented"
(5.1.4.2.1.1.4), and "When a cover guard is in the open position, it shall not
interfere with operation of the protected device or adjacent controls"
(5.1.4.2.1.1.5). The dimensional precedent is the legend-switch barrier:
**width ≥ 3 mm, depth ≥ 5 mm** (Fig 17). A 5 mm-deep, 3 mm-wide collar around a
key sunk 2.5 mm below the panel is enough that a flat object cannot press it but
a deliberate fingertip can.

### 7.4 Slider channel

- Slot width = cap width **+ 1.0 mm** total.
- Recess the channel floor **1.5–2 mm** below the panel plane so grit falls past
  the cap rather than onto the resistive track; the Bourns PTA family has a
  dust-cover option in the ordering code
  ([datasheet](https://static.chipdip.ru/lib/158/DOC011158765.pdf)).
- Cap proud **2–3 mm** with a 1 mm concave dish on top (borrowing the push-button
  rule), against MIL Table XI's 16 mm minimum handle height — **this is a
  deliberate departure**, and the reason is that Table XI's 16 mm is specified so
  the handle "serve[s] as a pointer" on an instrument panel read from a distance.
  Here the deck's own display is the pointer.
- Travel direction: MIL requires functional increase = up / right / forward.

### 7.5 Precedents worth looking at, and the one to fear

- **Braun T3** (1958): the tuning dial sits on the lower face with the perforated
  chassis above it — a partly-shrouded wheel, not a proud knob
  ([Powerhouse](https://collection.powerhouse.com.au/object/428711),
  [MoMA](https://www.moma.org/collection/works/4134)).
- **Braun ET66** (1987, Dietrich Lubs): number keys polished, function keys
  **matt and convex** so they "catch the light slightly more" and stand out; back
  ribbed to stop it slipping in the hand
  ([Port](https://www.port-magazine.com/design/counter-culture/)). Note the
  tension with MIL: Braun used *convex* for visual prominence, MIL requires
  *concave* for tactile fit. For blind use, concave wins; use **finish** (matt vs
  polished) for the visual grouping instead of curvature.
- **TE TX-6**: 90 × 62 mm, CNC aluminium, custom encoders and faders — and the
  warning. Users report the knobs are "way too small for anything else than 'set
  and forget'", "closely placed, tiny knobs can be challenging to operate", and
  "imprecise and 'jump' between values"
  ([Elektronauts](https://www.elektronauts.com/t/new-teenage-engineering-products/16943/3904),
  [Equipboard](https://equipboard.com/items/teenage-engineering-tx-6)). The faders
  escape the criticism; the knobs do not. **If you shrink four encoders to fit
  under the display, this is the review you get.**
- **Octatrack**: "the encoders are a bit close together, meaning that it's
  fingertips-only operation most of the time"
  ([SOS](https://www.soundonsound.com/reviews/elektron-octatrack-dps1)).
  Independent confirmation of the same failure at a larger scale.

---

## 8. Design language

### 8.1 What makes a panel read as German industrial

**Typography.** Braun standardised on **Akzidenz-Grotesk** across all
communication — "the basic font for all forms of communication… was restricted to
the sans-serif font Akzidenz-Grotesk"
([Fonts In Use](https://fontsinuse.com/designers/3002/braun)). But the panel story
is more interesting than that, and it is the single most useful thing in this
section: on the earlier devices Braun used **Nobel** on the controls, and Hans
Gugelot held that Nobel was "much better suited to labeling the control elements
than Futura, Akzidenz Grotesk"; from the SK5 onward Braun used Akzidenz-Grotesk
"among others"; and on the RT 20 the **dial numerals are a different typeface
from the labels above the buttons**
([Fonts In Use, Braun RT 20](https://fontsinuse.com/uses/6957/braun-rt-20)).

Three rules follow:
1. **Panel labels and numeric scales are different typographic jobs.** Label
   faces want open apertures and a large x-height at 5–7 pt engraved; scale
   numerals want a narrower, more geometric face that holds at 4 pt. Don't use one
   face for both just because it is tidier.
2. **The screen's bitmap font is a third system.** `docs/OS.md` fixes the text
   cell at 12 px line height because that is the panel's own quantum. Do not try
   to match the engraving to it; make them deliberately different and let the
   engraving be the permanent layer.
3. Set the labels in **lowercase**. This language's whole surface is lowercase
   (`>kick`, `>cut`, `route`), and a panel in caps will fight it.

**Colour.** Braun's palette "was predominantly black and white, but making expert
use of touches of bright, bold colours"
([Pixartprinting](https://www.pixartprinting.co.uk/blog/braun-dieter-rams/)); the
Cooper Hewitt describes the same as an "economic use of color"
([Cooper Hewitt](https://www.cooperhewitt.org/2018/02/24/ahead-of-the-times/)). The
ET66 is the worked example: functions grouped by colour (brown and green) and the
single most-used button in "nature's high-contrast black on yellow"
([Port](https://www.port-magazine.com/design/counter-culture/)). TE does the same
thing with one element — on the OP-1 "the single orange knob is the loudest
element on the whole machine"
([MusicRadar](https://www.musicradar.com/reviews/teenage-engineering-op-1)).

**The colour budget for this deck is unusually tight and unusually valuable**,
because the display is 1-bit reflective: the panel is where *all* the colour on
the instrument lives. Spend it on exactly three things — one neutral body, one
accent on `play`, one warning on `panic` — and nothing else.

**The Lectron lesson.** The Braun Lectron system (1967–69) was a set of magnetic
modules "freely organised on a conductive plate to form a variety of functional
circuits", with the circuit symbol printed on the top of each block
([Make](https://makezine.com/article/maker-news/the-braun-lectron-system-retro-circuit-dominoes/),
[Wright](https://www.wright20.com/auctions/2018/07/dieter-rams-the-jf-chen-collection/174)).
*(The Rams/Greubel attribution is disputed — Egger-Bahn sold the system from 1966
and one source states Rams "had NOTHING to do with the creation"; cite it as a
Braun product, not as a Rams design.)* The relevant idea is that **the thing's
function was printed on the thing, at the thing's own scale, and the grid came
from the module** — which is exactly the discipline a remappable control surface
needs.

### 8.2 Swiss grid, applied to a panel rather than a page

Müller-Brockmann is credited with formalising the grid "as a controlling
principle for page layout"
([Wikipedia](https://en.wikipedia.org/wiki/Grid_(graphic_design))), and the
feature of his method that transfers is that **the grid is derived from the
content's own metrics, not imposed**: "The horizontal and vertical rhythms of his
modular grids are based on the metrics of the typography: font family, size,
leading, words per line", and all "modular decisions about grid rows and vertical
gutters" come from the baseline text grid. He also insisted that how the grid sits
in the page — the margins — matters as much as the modules
([Ben Morris](https://www.ben-morris.com/josef-muller-brockmann-grid-based-layout-and-web-design/)).
Broader context: [Swiss Style](https://en.wikipedia.org/wiki/Swiss_Style_(design)).

**The translation for v2 is one sentence: the panel module is the key pitch.**

It is the only dimension on the instrument that the hand measures directly, and
everything else should be a whole multiple or a clean half of it. At 13 mm:

| Feature | Grid | Check |
|---|---|---|
| Encoder pitch | 3 × 13 = **39 mm** | = MIL's 25 mm minimum for a 14 mm knob, exactly |
| Function-key pitch | 1.5 × 13 = **19.5 mm** | ≈ the ANSI key unit |
| Control-row centreline above keyboard | 3 × 13 = **39 mm** | inside the 45 mm thumb limit |
| Encoder row span | 10 × 13 = **130 mm** | = the 10-column keyboard width |
| Label baseline | every 13/2 = 6.5 mm | gives a 6.5 mm label band under each control |

That is the whole grid, and it is derived from one measured human quantity. The
same arithmetic at 18 mm (Choc) pitch gives a 54 mm encoder pitch and a 180 mm row
— comfortable, but a ~196 mm-wide instrument.

**One thing the grid must not do**, and this is where norns failed: a grid gives
alignment between *panel* features, not between the panel and the *screen*. 84.8 mm
of display over a 130 mm control row cannot put a label above each knob. SOS on
norns: "There's also no obvious alignment between the encoders/buttons and the
screen", so soft keys are ruled out
([SOS](https://www.soundonsound.com/reviews/monome-norns)). Push's eight encoders
*are* aligned to eight display columns and eight buttons above and below
([Ableton](https://www.ableton.com/en/manual/using-push-2/)) — that alignment is
the entire reason Device Mode is legible.

**The fix, since the display cannot grow to 130 mm cheaply: identify each encoder
by an engraved glyph plus a distinct rim treatment, and have the screen name them
by glyph rather than by position.** This is OP-1's mechanism with colour
substituted for glyph — "A green graphical element or text hints that the green
encoder will change its value or position"
([TE](https://teenage.engineering/guides/op-1/original/layout)) — and it is
MIL-legal: shape coding, ≤ 10 shapes, "identifiable by hand and eye regardless of
the position and orientation", with a documented rim vocabulary (knurled /
serrated / indented, 5.1.4.1.2.1.4). Glyphs survive a 1-bit display and a
remapping; positions do not.

### 8.3 Soma's idiom, concretely — and the conflict with v1

What "Soma" means dimensionally, from their own pages: Pulsar-23 is **55 knobs,
11 switches, 23 modules, over 100 I/O** on one flat plate, in saturated matt
colours (orange, black, white, mint, pink, "ZANZIBAR blue"), with the construction
method sold as a visible choice — turrets or screws
([Soma](https://somasynths.com/pulsar-23/)). Terra is a wooden housing "made from
three pieces of wood" with a brass logo "encrusted" into it, where "joints between
different parts of the wood may be visible" and that is stated as a feature, not a
defect ([Soma](https://somasynths.com/terra/)). Kreimer's framing of all of it is
"Romantic Engineering", taught as *The Psychology of Synthesis*, and it is explicit
that imperfection is part of the method
([Synthtopia](https://www.synthtopia.com/content/2023/12/21/synth-design-master-class-with-vlad-kreimer-of-soma-laboratory/)).

So the Soma feel, as a specification: **matt, saturated, no gradients; dense but
regularly grouped controls; large legible silkscreen; exposed fasteners; one warm
material (wood, brass) against the cold one; and nothing smoothed into a shell —
the panel reads as a plate with hardware mounted through it.**

**That last clause is in direct conflict with v1.** The v1 `README.md` states the
design language as "one radius system, nothing applied… curvature continuity
everywhere a surface turns, so it reads as grown rather than extruded", with "no
bezel line across the face you touch" and the structural joint moved to the back.
That is Rams-organic. Soma is a plate. **v2 cannot be both, and this needs
deciding before any geometry.**

My recommendation: keep the shell Rams, and make the **control band a distinct
inserted plate** — the only joint line on the face, deliberate, stated, and
carrying all the engraving. That satisfies Soma (a plate with things through it),
satisfies Rams's "where the language breaks it says so" discipline the repo already
practises for the display aperture, and gives you one honest place to put a real
material: an oak or brass control plate in a printed shell, which is Bastl's
"front panel from oak wood, handmade custom knobs"
([patch.audio](https://patch.audio/en/product/bastl-instruments-skis-full-diy-kit-wooden-panel/))
and Terra's brass inlay at the same time.

### 8.4 Teenage Engineering's reductionism, usefully

TE's stated aim is ease of use, with colour coding and symbols "designed for easy
reading" to make shaping sound "intuitive and non-technical"
([TE](https://teenage.engineering/products/op-1/original),
[layout](https://teenage.engineering/guides/op-1/original/layout)). The transferable
move is not minimalism — it is that **a small number of controls each hold many
meanings, and the identity that holds them together is not a label but a
perceptual constant** (a colour, here a glyph). The TX-6 shows the limit of the
same instinct: it is beautiful and its knobs are, by owners' accounts, too small
to play.

---

## 9. Open questions

1. **Display width vs aligned controls.** 84.8 mm of active area cannot label a
   130 mm control row by position. Grow the display to ~130 mm active, or commit
   to glyph coding (§8.2)? This is the single biggest unresolved question and it
   changes the bill of materials.
2. **Thumb-typed at 13 mm or ten-finger at 18–19 mm?** This one decision sets the
   width (≈150 mm vs ≈196 mm) and therefore the whole panel grid. I argue thumb-typed
   because the instrument is played standing, but the "upgrade the keyboard" brief
   may mean the opposite.
3. **Where does the control PCB live?** A PEC11R needs **7.0 mm behind the
   mounting surface** plus a 5.5–6.5 mm bushing. v1's internal stack is 11.00 mm
   with board depth 11.25 mm (`v2/docs/v1-datums.txt`). Does the control band get
   its own cavity beside the display board, or does the deck thicken?
4. **Thumbwheel sensing is unspecified.** Horizontal-axis encoder, belt to a
   vertical PEC11R, or a magnetic rotary sensor (AS5600 class) behind a plain
   wheel? The magnetic option needs almost no depth and no shaft seal, which may
   decide it. Needs a parts study in `v2/parts/`.
5. **Is audio out ever coming?** Today there is none: no I²S/DAC/codec driver in
   `firmware/components/`, no mention of audio in `docs/HARDWARE.md`. If it is on
   the roadmap, say so now, because the whole pots verdict in §4 reopens and the
   top edge needs a jack.
6. **Panic guard: recess, barrier, or two-key chord?** A chord costs no panel area
   and no part, but it is not "physical, no-screen-needed" in the spirit of
   `docs/OS.md`. I lean recess + collar.
7. **ToF: reserve the footprint or delete it?** Reserving costs two pads and a
   window; deleting costs nothing and risks nothing.
8. **Does the IMU belong to the case or the instrument?** If it is only orientation
   and shake, it is a case feature and needs no panel presence at all. If anyone
   wants it as a modulation source, it needs an arming control and that is a key.
9. **Euclid.** `docs/STRUDEL.md` names `x(3,8)` as the joint most-wanted missing
   notation. If it lands, a two-integer control becomes the highest-value
   continuous control on the panel. Do not design hardware for it yet, but do not
   fill the top edge so completely that there is nowhere to put it.
10. **What does a local control print as its source in `>lanes`?** The existing
    display shows the last two octets of whoever set an input
    (`knob1 knob 84 .4.2`). A panel control has no address. This is a naming
    question for the engraving as much as for the firmware — the panel should use
    whatever word `>lanes` will print.
