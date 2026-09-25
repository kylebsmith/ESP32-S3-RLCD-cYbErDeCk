# Every verb, on one page

*The whole language. Thirty-six verbs, counted from the command table itself —
if this page and the firmware disagree, the firmware is right and this page is
stale. [MAP.md](MAP.md) §0 has the snippet that counts them. The lane names are not
verbs: they are yours, defined in the boot document.*

*For what was wrong with this language and what replaced it, see
[MANIFESTO.md](MANIFESTO.md) — two adversarial reviews, and which of their
proposals have been decided.*

A performer cannot search. That is the entire reason this page has to fit on one:
if it does not, the language is too big, and that is a design failure rather than
a documentation one.

---

## The shape of every line

```
>name pattern                 a LANE: when, how much, and where it goes
>name = note 36               what a NAME means - yours to change or add
>verb argument                everything else
```

```
  x  hit          .  rest         _  hold the note before it
  0-9             HOW MUCH: velocity on a drum, degree on a voice,
                  value on a controller, amount on a picture
  x%15            fifteen per cent chance on that step, or group
  [xx]            a group: SUBDIVIDES the step it occupies, any depth
  [0,4,7]         a chord: every member at once
  <a b>           ALTERNATES: a different member each bar
  /2  *2          this lane's own rate, at the end of the line
  !4              four passes, then it stops - and 'name:end' fires
  u d l r         which way - move warp ramp turn only; a step's own wins
```

A step is one character and what is attached to it; the playhead lights all of
it. **Anything else is refused** — the bar says why and the character is boxed —
rather than played as a hit.

`Ctrl+Enter` runs the line. `Enter` always makes a line. Run a line unchanged to
mute that lane; run it again to bring it back. Type a lane's name alone to delete
it.

---

## Lanes — the names are yours

The whole point: **a drum and a circle are the same sentence.** A lane compiles
text into *when* and *how much*; its name says *where*. The boot document names
sixteen sounds; the sixteen pictures answer to their own names.

| sound — defined in the boot document | | pictures — **fields** | | pictures — **operators** | |
|---|---|---|---|---|---|
| `kick` `snare` `hat` `ohat` | `= note 36` … a drum on ch 10 | `disc` | round — distance from a point | `mask` | keep what is this bright — a **level** |
| `clap` `tom` `rim` `crash` | drums | `box` | square — the corners disc lacks | `edge` | keep where it changes — a **contour** |
| `bass` | `= voice 2 ch 1 gate 180` | `turn` | the **angle** around the point | `echo` | keep the last frame — trails |
| `lead` `pad` `arp` | voices: degrees in the key | `ramp` | linear, along an axis | `move` | shift, wrapping |
| `cut` | `= cc 74`, filter | `grid` | periodic — a lattice | `spin` | quarter turns |
| `res` `mod` `rev` | CC 71, 1, 91 | `noise` | no geometry — the entropy | `warp` | bend lines on an axis |
| | | | | `grow` `thin` | dilate, erode |
| | | | | `flip` `fold` | invert; mirror 1–3 folds |

**Your own, with a line:** `>conga = note 63`, `>fx = cc 20 ch 2`,
`>strings = voice 3 ch 5 gate 600`, `>circle = disc`. `>kick = note 35` retunes the
kick that is already playing; `>conga =` forgets the name and its lanes. A name is
up to eight letters and cannot be a verb or a picture.

**Sixteen lanes at once**, any mix. `route` connects any two.

**A count ends a lane, and an end starts another.** `>intro x.x.x.x. !2` plays two
passes and stops — typed mid-song it waits for its own downbeat. `>route crash
intro:end` is a crash as it ends; `>route verse intro:end` with `>verse ... !8` *starts*
the verse there: a routed lane with a count is a **cue**, one without is a sidechain.
`>play` starts the arrangement from the top.

**An address picks one.** `disc:2` is a second circle, `disc:x` a circle's position,
`disc:2:x` the second one's — `x` and `y` are a picture's parts; `vel` is a sound's
level and `oct` a voice's octave: `>bass:oct <2 3>...`. A part is a lane like any
other. `>route disc:2 kick` routes them apart. (`disc2` and `disc[x]` were the old spellings; the deck tells you
the new one.)

### A shape is a field through a threshold

The picture half of this language has **six fields and ten operators**, and none of
the six is a shape. A field answers *how far is this cell from the thing* in its own
geometry; `mask` and `edge` cut a shape out of the answer. That is why there is no
`ring` and no `star`:

```
>disc 8            a filled circle
>disc 8  >edge 1   a RING - the outline, and nothing inside
>disc 8  >mask 9   a smaller, harder circle - the level resizes it
>box 6   >edge 1   a rectangle outline
>turn 9  >edge 1   spokes
>turn 2  >spin 1   a RADAR SWEEP, and with echo one that trails
>ramp d  >edge 9   a contour map
```

`star` used to be a verb. Its amount was a count of spokes — the only amount in
this language that was not a magnitude — and nothing composed with it. `turn` is the
angle as a field, so the amount means *how much of the circle*, and spin turns it.

**`route` states the draw order.** A routed lane draws *after* the lane it follows,
so `>route thin disc` then `>route grow thin` is despeckle and reversing the two is
close-the-gaps. Both were unreachable when the table decided. A document with no
routes draws in table order, whatever order the lines were typed.

---

## The clock — 6

| | |
|---|---|
| `bpm 124` | tempo |
| `scale dmin` | `c` `f#mix` `apent` `ebblues` — root, then a mode |
| `swing 58` | 50 straight, 67 triplet |
| `sync on` | MIDI clock out |
| `play` `stop` | |

## Connecting things — 7

| | |
|---|---|
| `route disc kick` | the circle fires on the kick, at its velocity. Any two lanes. Chains. |
| `send` | list destinations; `send mon on` |
| `usb on` | be a USB MIDI device — needs a computer. Reboots. |
| `din 17` | **MIDI on a wire — needs no computer.** Drives an SP404, a eurorack brain, anything with MIDI IN. Wire it first: [HARDWARE.md](HARDWARE.md) |
| `osc 10.0.0.5 9000` | `/deck/<lane>` over the network |
| `wifi <ssid> <pass>` | join; `wifi off`, `wifi forget` |
| `host deck 12345678` | *be* the network |

## Looking — 6

| | |
|---|---|
| `lanes` | what is playing, what it follows, `-` for muted |
| `jitter` | timing, in microseconds. `jitter reset` |
| `split on` | the picture, below the code. `split 8` for rows |
| `density low` \| `high` | 30 columns chunky, 60 compact |
| `dump` | a document to the console |
| `frame` | the picture over OSC |

## Documents — 10

| | |
|---|---|
| `new` | a scratch buffer |
| `name lullaby` | file it — that is what "save" means here |
| `open lullaby` `list` `close` | |
| `run lullaby` | run every line, without leaving this page |
| `save` | write now |
| `guide` `prose` | mark what a buffer is |
| `help` | all of this, on the deck |

`Ctrl-L` / `Ctrl-J` walk documents. `Ctrl-O` returns from output. `Ctrl-G` the guide.

## When something is wrong — 7

| | |
|---|---|
| `panic` | silence everything, now |
| `mute hat bass` | `mute` alone puts all back |
| `solo kick` | |
| `kbd` | what is typing; `kbd forget` to pair a different keyboard |
| `battery` | find the sense pin |
| `ssh me@host pass ls` | a terminal, from the deck |
| `flash now` | reboot to the ROM loader |

---

## The five things worth knowing

1. **A lane is a lane.** `>kick x...x...` and `>disc x...x...` differ only in
   where they go. Everything that works on one works on the other.
2. **Run a line again to mute it.** The whole performance gesture. Type a name
   alone to delete the lane.
3. **`route` is the sidechain, generalised.** `>route disc kick` makes the circle
   fire *on* the kick. It chains: `kick → disc → grow`.
4. **Rate is per lane.** `/2` on one line is half-time for that line only — which
   is why polyrhythm needs no feature.
   And **`[]` subdivides, `<>` alternates.** They compose in either order:
   `[x<x .>]` is a doubled step whose second half comes and goes.
5. **`din` is the one that needs no computer.** Everything else makes the deck a
   device, a peripheral or a client, and all three need a host.
