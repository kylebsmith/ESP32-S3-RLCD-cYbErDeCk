# Testing the deck

A pass over everything the device does. Each step is one thing, and each says
what you should see — if you don't see it, that's the bug, and the step number
is enough to report it.

Ctrl+Enter runs the line the cursor is on. Ctrl-O comes back from any output
page. Ctrl-L and Ctrl-J walk between documents.

### Driving it from a terminal, and the one trap in it

Every step here works over the USB cable with no keyboard paired: **CR is Enter and LF
is Ctrl+Enter**, so a terminal that sends CR opens a line and a script that sends `\n`
runs one. Bursts are fine — a 26-character line typed as one write arrives intact.

**Ctrl-O is a toggle, not a "go back".** From the document it goes *to* `+out`; from
`+out` it comes back. And a command only moves you to `+out` if it printed more than one
line, so where Ctrl-O lands depends on what the last command was. A script that sends it
unconditionally ends up typing into the output pane and running whatever line the cursor
happened to be on — which looks exactly like dropped characters, and was diagnosed as
dropped characters twice before anyone read the pane. Track the pane from what the deck
says about it: it logs `buffer 3 '+out' selected` when it moves you.

## 1. It boots

Power on. You should see **KILROY** appear a letter at a time, a rule sweep
under it, then a line of numbers — documents and free memory — then the editor.

If the screen stays blank, a button is held down: every reset reports
`boot:0x22 (DOWNLOAD)` when GPIO0 is low. Release it and power-cycle.

## 2. The guide plays

Ctrl-L until the status bar reads `guide`. Run each `>` line from the top.
By `>play` you should have an accented kick, a soft hat whose last hit comes and
goes, a tied bassline in D minor, two held chords on the pad, and a filter sweep.

An inverted bar should sweep along each pattern line in time with the sound,
covering the whole step that is sounding — all of `x%50`, all of a chord.

## 3. Editing while it runs

With it still playing, change a character in the kick line and Ctrl+Enter.
It should change on the next bar without stopping.

Then Ctrl+Enter on that same line **without touching it** — the lane goes
silent. Again — it comes back.

## 4. Probability

```
>hat x%3x%3x%3x%3
>hat x%97x%97x%97x%97
```

Near-silence, then near-constant. `>lanes` prints each line exactly as it was
compiled.

## 4b. How much, how long, all at once

```
>send mon on
>kick 9...5...
>bass 0__.3...
>pad [0,2,4]...
>play
```

The console shows each note with its velocity and the time it went out. The kick
alternates 127 and 71. The bass's first note is held two extra sixteenths — its
off comes about 420 ms after its on at 124 bpm, where the next is about 180. The
pad plays three notes on one timestamp. `>send mon off` to stop the log.

## 4c. Mistakes are refused

```
>hat x...x...x;..
>kick X...x...
>hat [x.x.
```

Each is refused in one line on the status bar — *';' is not a step*, *X is gone: 9
is loud*, *'[' is never closed* — and the character is boxed in the document until
you edit. Whatever was playing keeps playing.

## 5. Time ratios

```
>hat x.x.x.x. /2
>hat x.x.x.x. *2
```

Half speed, then double. The kick keeps its own tempo — each lane divides the
clock separately.

## 6. Mute and solo

```
>mute hat bass
>solo kick
>mute
```

Several at once. Bare `>mute` or `>solo` brings everything back.

## 7. USB MIDI

```
>usb on
```

The deck reboots and comes back as one USB device carrying MIDI, the console
and the keyboard. In a DAW, pick **cyberdeck usb** and record-arm a track with
a drum rack — drums are General MIDI on channel 10, so the kick lands on the
first pad.

`>usb off` to come back. A power cycle also always returns to serial: the mode
lives in RTC memory, so there is no setting that can fail to save.

## 8. MIDI clock

```
>sync on
```

In Ableton: Preferences → Link/Tempo/MIDI, enable **Sync** on the cyberdeck
input, set the transport to External.

## 9. Network

```
>host deck
```

The status line asks for a password: type it and press Enter. It shows a star
for each character and **never enters the document** - a password on a line would
be journalled, mirrored to the card and copied to the DGX. Under eight characters,
or none, and the deck hosts an open network and says so. Esc stops without
starting anything. The deck becomes a Wi-Fi network at `192.168.4.1`. Join it from
a laptop.

Or join an existing one:

```
>wifi <ssid>
>wifi
```

It asks for the password the same way. The second command shows the address once
it has one. Credentials are remembered **as soon as Enter is pressed**, in NVS and
never in a document, and re-joined at every boot. `>wifi forget` clears them.

A password still typed on the line the old way - `>wifi home hunter2` - is
refused, and cut from the line before autosave can keep it.

## 10. OSC out

On the laptop:

```bash
python3 tools/osc_listen.py
```

On the deck:

```
>osc <the laptop's ip> 9000
>play
```

You should see `/deck/step` with the bar position and `/deck/<lane>` for every
hit, by lane name.

## 10b. OSC in

On the deck, with it on the same network as the laptop:

```
>osc in 9000
>knob1 = knob
>pad1 = pad
>route cut knob1
>route kick pad1
>play
```

On the laptop:

```bash
python3 tools/osc_send.py <the deck's ip> 9000 /deck/knob1 0.5
```

```bash
python3 tools/osc_send.py <the deck's ip> 9000 /deck/pad1
```

`>lanes` shows `knob1 knob  64` and the last two parts of the laptop's address; the
filter moves, and the kick plays on the next step after the pad. A phone app that
sends OSC works the same way: a fader from 0 to 1 on `/deck/knob1`. **A phone has not
been tried** — two decks have, and this script from a laptop on a home network: every
value right, 22 ms median from sending to the filter moving.

## 11. Visuals, in the same document

```
>kick 9...x...9...x...
>bass 0...5...3...7...
>echo 8
>noise 2.4.2.4.
>move d
>route disc kick
>route grow disc
>play
```

**A drawing primitive is a lane, exactly like a drum.** `>disc 9` and `>kick x...`
are the same sentence with different destinations, so everything that works on
one works on the other: a bare name drops it, re-running an unchanged line mutes
it, `/2` halves it, and `>lanes` lists them together. There is no `viz` keyword —
it was an alias during the collapse and it is gone.

The lower half of the screen becomes a live frame inside a stroked border,
advancing on the same clock as the music. `>split on` and `>split off` are
explicit; a bare `>split` toggles. `>split 8` gives the picture eight rows.

**Three sources and five operators.** A source puts ink down; an operator bends
whatever the sources drew. That is the whole design — complexity comes from
combining them, not from having more of them.

| draws | |
|---|---|
| `noise` | a random field |
| `disc` | a filled circle from the centre |
| `ramp` | a gradient along an axis |
| `grid` | a lattice, from a frame to a dense mesh |

| bends | |
|---|---|
| `echo` | keeps the last frame, one ink step dimmer — **trails** |
| `move` | shifts the frame, wrapping |
| `warp` | displaces lines along an axis — waves, glitch |
| `shake` | tears lines sideways at random — glitch, where `warp` bends |
| `grow` | dilates: every mark blooms into its neighbours |
| `thin` | erodes: edges eat inward. `grow` + `thin` is an outline |
| `flip` | inverts the whole frame — the cheapest strobe there is |
| `tile` | repeats the frame across, 1–4 copies |
| `fold` | mirrors it, 1–3 folds — kaleidoscope |

**The picture is drawn with our own glyphs, not with punctuation.** Codepoints
128–155 are a tile library generated by `tools/font_tiles.py` and built into both
faces: nine tones (an ordered dither from nothing to solid), four sparkles, half
blocks, a diamond, a disc, a ring, diagonals, and four quadrant arcs that tile
2×2 into one circle twice the size. Nine even tones are what let `echo` actually
fade — `@ # * : : . .` and gone, eight stages — where six punctuation marks
jumped straight from `:` to nothing. `>frame` still sends plain ASCII over OSC,
since the tiles mean nothing to a receiver.

**Patterns nest.** A bracket subdivides the step it occupies, to any depth:

```
>kick x..[xx]          the last step becomes two half-steps
>hat  [xxx]...         a triplet in the first step of four
>hat  [xx][xxx]        two against three, in one bar, from one line
>kick x.[x[xx]].       the second of a pair splits again
```

**Patterns alternate.** `<a b>` plays a different member each bar:

```
>kick x...<x .>...     a hit on the two, every other bar
>hat  x.x.x.x.<[xxxx] x>...  a roll on the 9th step, every other bar
>bass 0...<3 5>...     the value changes
>snare <x%15 x%90>     and so do the odds
>disc  <9 3>           pictures alternate too
```

A lane's bar is its own length — one character at the top is one sixteenth — so
`<>` swaps once per pass of the line it is in, not once per four beats. Angle
brackets pick one; square brackets subdivide. They compose in either order:
`[x<x .>]` is a doubled step whose second half comes and goes, and `<[xx] x>` is
two hits one bar and one the next. Groups of different length run their own
cycles — `<0 1><2 3 4>` takes six bars to repeat — and a group inside a group
advances only when it is chosen: `<0 <1 2>>` plays 0 1 0 2.

Probability is `%`: `x%15` is a fifteen-per-cent chance on that step, and `[..]%50`
puts odds on a whole group, multiplying with any inside it. Notes that start
together share one roll, so a chord with odds plays whole or not at all. Odds
travel with the alternative, so `x%15<3%20 5%80>` keeps each one's own. The
bracket was spent on the parameter before; a group had the better claim on it.

Nesting is resolved when the line compiles, so a nested lane costs the clock
nothing. What cannot fit is refused rather than shortened. A five- or seven-way
split keeps exact time — `>hat [xxxxx]...` lands its bar on the kick's, every
bar.

**A digit is always how much: 0 none, 9 full.** In every primitive. A `u`, `d`,
`l` or `r` is which way, either in front of the pattern (`>ramp u 4.6.9.6.`)
or as a step of it (`>move d....d...`). Speed is the pattern, so `/2` and
`*2` halve and double a visual lane exactly as they do a drum.

Old shapes are combinations now. Rain is `noise` + `move d` + `echo`. A bar is
`ramp`. A wave is `ramp` + `warp`. Start with `>echo 9` and then add a
source — that one line is the difference between a blinking shape and an
animation.

**Run a visual line again to mute it**, exactly as a drum lane works. Running it
a third time brings it back. An empty pattern (`>disc`) removes the lane
outright and takes its routing with it.

`>route disc kick` makes the disc **fire on every kick**, at the size of that
hit's velocity. Routing is *when* as well as *how much*: a routed lane ignores
its own pattern and follows its source. Put `>echo 8` above it and the pulse
gets a tail. **A part of a primitive is a lane too.** `disc:x` is the circle's position
across the frame, and it is a lane like any other — it has a pattern, it
alternates, it nests, and it can be routed:

```
>disc      4                   a small circle
>disc:x    0..3..6..9..        swept across
>disc:y    <2 7>               and up and down, bar to bar
>route disc:x bass             or driven by the bass note
>disc:2:x  9...0...            the second circle, its own path
```

The old spellings — `disc[x]`, `disc2` — are answered with the new ones rather than
played.

## 11b. The names are yours

```
>conga = note 63
>conga x..x..x.
>kick = note 35
>conga =
```

The first two play note 63 on channel 10. The third moves the kick that is already
playing to note 35 — the next hit is 35. The last forgets the name and silences its
lane. `>help` lists every name there is. `>bpm = note 3` and `>disc = note 3` are
refused: a name cannot be a command or a picture.

```
>send mon on
>bass 0...
>bass:oct <2 4>...
>bass:vel 9...3...
>play
```

The bass alternates D2 and D4 bar by bar, at 127 then 42. `>bass:oct` alone drops
the part and the bass goes back to octave 2. A lone `<2 4>` would change every
sixteenth — a lane's bar is its own length — and under a four-step bass it reads 2
every time.

`>kick x..u` is refused and the `u` boxed: only `move`, `warp`, `ramp` and `turn`
have a way.

## 11c. Counts, ends and cues

```
>send mon on
>kick x...
>intro = note 60
>verse = note 62
>intro x.x. !2
>verse 9... !2
>route verse intro:end
>route crash intro:end
>play
```

The intro plays two passes — four hits — and `>lanes` then reads `done`. On the next
kick the verse starts and the crash hits, all three on one timestamp; the verse plays
its two passes and reads `waits`. `>stop` then `>play` plays the intro again from the
top. Typed while playing, a counted line waits for its own downbeat.

`x` and `y` are the only parts for now — deliberately short, or it becomes a flag
grammar. 0 is the left or top edge, 9 the right or bottom, and the shape's centre
goes there.

**Anything that plays can drive anything else**, and chains work:

```
>route disc kick      the circle fires on the kick, at that hit's velocity
>route grow disc      the bloom follows the circle
>route warp cut       the bend follows the filter sweep
```

`kick → disc → grow` is two hops across three kinds of destination. Routing a
primitive that has no lane yet creates it, so `>route grow disc` needs no pattern
written first. A lane cannot follow
itself (it would re-trigger for ever), and routing to a name that is not a lane
yet says so rather than going quietly silent. Unroute with `>route disc`.

`>lanes` now lists the visual half as well — which primitives are live, what
each follows, and a leading `-` for muted. If a route looks dead, that listing
is where to look first.

## 12. ASCII frames

Make a document with some ASCII art, then:

```
>frame
```

The listener prints it between `--- frame ---` markers. Editing the drawing and
running `>frame` again is the whole of visual coding on this device.

## 13. SSH

**Untested against a real server** - the decks had no network with an sshd on it.
Use a throwaway account on the laptop, never a real password, as docs/NEXT.md §10
asks. Turn on Remote Login (macOS: Settings → General → Sharing). Then with the
deck on the same network:

```
>ssh you@192.168.1.42 ls
```

The status line asks for the password; it never goes on the line. The session
runs beside the editor - you can keep typing - and when it ends the reply is shown
in `+out`. Ctrl-O comes back.

**The host's key is kept the first time** and printed as `ssh-keygen -lf` prints
it. Check it by eye against the laptop's own, from the key type the deck names:
the deck cannot use ed25519 host keys, so it will be the ECDSA or RSA one:

```
ssh-keygen -lf /etc/ssh/ssh_host_ecdsa_key.pub
```

From then on a different key is **refused before any password is sent**. If you
changed the key yourself, `>ssh forget 192.168.1.42` and connect again.

What was exercised, 2026-09-25, on two decks with one hosting a test network: an
address nobody answers reports `no answer in 5 seconds` at 5.0 s while the editor
keeps its 194 turns a second; a closed port reports `connection refused`.

## 14. Battery

```
>battery
```

Unplug USB, run it again. The channel whose voltage **moved** is the cell.
Then:

```
>battery use 4
```

Persistent, no reflash. A four-cell bar appears at the right of the status row.
If the divider isn't 2:1, `>battery use 4 30` for 3:1.

## 15. Settings survive

Ctrl-L to `boot`. It holds ordinary commands that run at startup:

```
>bpm 124
>scale dmin
>density dense
```

Edit it, power cycle, and they take effect. There are two densities and the
panel decides that, not taste: `>density low` is 30 columns of the chunky
12x24 face, `>density high` is 60 columns of 6x12. Cell height has to be a
multiple of 12 and cell width has to be even, so no legible middle size exists
— an 8x24 face was built, read thin, and was thrown away.

`high` is the one to use with the split: 60 columns leaves 39 for code with the
view taking a third, so pattern lines stop wrapping. `>split 12` or
`>split 20` sets the view width directly.

## 16. An ensemble

Two decks, or ten. One leads, the rest follow:

```
   deck A          deck B
   >sync lead      >sync follow
   >play           >play
```

**No network, no password, no router** — the decks talk to each other directly over
ESP-NOW. `>sync` on a follower reports how far off it is and how good the link is:

```
following, 1 other deck
off by -39 us, 110 packets
best trip 2459 us, 12 skipped
```

Typically a few hundred microseconds, occasionally up to about 2 ms — a pulse at
124 bpm is 5040 µs. The leader's tempo is followed at once; `>bpm 96` on the leader
and the followers change with it. `>sync alone` to play by yourself again.
Outputs stay independent: deck A can drive a drum machine over `din` while deck B
drives a projector over `osc`, from the same clock.

See [NETWORK.md](NETWORK.md) for how it works and what it was measured at.

## 17. MIDI with no computer

```
>din 17
>kick x...x...x...x...
>bpm 124
>play
>din
```

`>din` reports bytes actually written — `din GPIO17, 80 bytes sent / the wire is
busy` means real MIDI left the pin. That is the only transport that needs no host
at all, so it is the one that drives an SP404, a eurorack MIDI-to-trigger module,
or anything else with a MIDI IN.

**It needs a resistor loop before you trust it** — a MIDI output is a current
loop, not a logic level. See [HARDWARE.md](HARDWARE.md). `>din off` stops it.

**Fixed: the deck used to hang leaving USB MIDI mode.** `esp_restart()` never
completed from that mode — it runs shutdown handlers and TinyUSB's teardown
deadlocks — so `>usb off` saved every document and then stopped dead, and only a
PWR hold recovered it. It reboots without the handlers now, and the `>usb on` →
`>usb off` round trip works. See [OS.md](OS.md).

Still open: **`>flash now` from USB MIDI mode** has the same deadlock and cannot
use the same fix, because it needs a CPU-only reset to preserve
`FORCE_DOWNLOAD_BOOT`. Do `>usb off` first, then `>flash now`.

Also worth knowing: **`>usb on` reboots, and the reboot loses the lanes, the
tempo, `sync` and `play`** — the document survives but nothing is re-run. Put the
whole piece in a document and `>run` it after the reboot, or put `>usb on` in
`boot` so the deck comes up that way.

The other transports all need something else to be the host: `>usb on` makes the
deck a USB MIDI *device* (so: a computer or tablet), `>send ble on` needs a BLE
MIDI host, `>osc <ip> <port>` needs something listening on the network.

## 18. It recovers

If it crashes it now reboots in about two seconds and the screen says
`crashed Nx - unplug to clear`. That counter clears on a real power cycle:
unplug and hold PWR, since the battery means unplugging alone doesn't
power it down.

## Known gaps

- `>flash now` doesn't reach the bootloader while in USB MIDI mode. Use the
  BOOT button to reflash from there.
- `>lanes` shows a pattern from its compiled form, so spacing you typed for
  readability isn't echoed back.
- No SSH key auth yet — password only, asked for and never on a line.
- An SSH session has never been run against a real server: see §13.
