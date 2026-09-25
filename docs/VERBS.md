# Every verb, on one page

*The whole language. Sixty-nine names, generated from the command table itself —
if this page and the firmware disagree, the firmware is right and this page is
stale. [MAP.md](MAP.md) §0 has the snippet that counts them.*

A performer cannot search. That is the entire reason this page has to fit on one:
if it does not, the language is too big, and that is a design failure rather than
a documentation one.

---

## The shape of every line

```
>name pattern                 a LANE: when, how much, and where it goes
>name argument                everything else
```

```
  x  hit          X  loud         ,  quiet        ?  maybe (half)
  .  rest         -  rest         _  rest
  0-9             a scale degree, a controller value, or an amount
  x%15            fifteen per cent chance on that step
  [xx]            a group: SUBDIVIDES the step it occupies, any depth
  <a b>           ALTERNATES: a different member each bar
  /2  *2          this lane's own rate, at the end of the line
  u d l r         which way, in front of the pattern or as a step
```

`Ctrl+Enter` runs the line. `Enter` always makes a line. Run a line unchanged to
mute that lane; run it again to bring it back. Type a lane's name alone to delete
it.

---

## Lanes — 33 names, one behaviour

The whole point: **a drum and a circle are the same sentence.** A lane compiles
text into *when* and *how much*; its name says *where*.

| sound | | | pictures — sources | | pictures — operators | |
|---|---|---|---|---|---|---|
| `kick` | drum, note 36 | | `noise` | sparkle field | `echo` | keep last frame — trails |
| `snare` | drum | | `disc` | filled circle | `move` | shift, wrapping |
| | | | `box` | rectangle outline | `spin` | quarter turns |
| | | | `star` | spokes from centre | | |
| `hat` | drum | | `ramp` | dithered gradient | `warp` | bend lines on an axis |
| `ohat` | drum | | `grid` | a lattice | `shake` | tear lines sideways |
| `clap` | drum | | | | `grow` | dilate — marks bloom |
| `tom` | drum | | | | `thin` | erode — edges eat inward |
| `rim` | drum | | | | `flip` | invert the frame |
| `crash` | drum | | | | `tile` | repeat, 1–4 copies |
| `bass` | voice, low | | | | `fold` | mirror, 1–3 folds |
| `lead` | voice | | | | | |
| `pad` | voice, long | | | | | |
| `arp` | voice, short high | | | | | |
| `cut` | CC 74, filter | | | | | |
| `res` | CC 71 | | | | | |
| `mod` | CC 1 | | | | | |
| `rev` | CC 91 | | | | | |
| `cc` | any controller | `cc 74 0..9..` | | | | |

**Sixteen lanes at once**, any mix. `route` connects any two.

**A trailing digit makes another one.** `disc2`, `disc3`, `kick2` — a different
name is a different lane on the same binding, so you can route them apart:
`>route disc2 kick2`. Nothing in the table changes; it is a naming rule.

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
