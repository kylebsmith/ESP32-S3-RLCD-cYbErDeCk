# The map

*Every verb the deck has, what it touches, and which of them should not exist.
Downstream of [SUBSTRATE.md](SUBSTRATE.md), which argues there is one data
structure, and [OS.md](OS.md), which argues the clock owns the machine. This
document exists to be **subtractive**: it is the place a proposed feature comes
to be refused.*

Claims are tagged `[FACT]` (verified on hardware), `[JUDGEMENT]` (a decision,
with its reason) or `[OPEN]`.

---

## 0. The thing this document is for `[JUDGEMENT]`

This device gets frozen. Not deprecated, not maintained — **finished**, and then
it is an object rather than a project. That is the whole premise, and it has one
consequence that governs everything below: *the cost of a feature is not the
code, it is the permanent widening of the surface that can never be narrowed
again.*

Software that ships forever can afford to accumulate. Software that is going to
stop has to converge. So the test for any addition is not "is this useful" —
almost anything is useful — it is:

> **Does this let me delete something? Or does it collapse two things into one?**

If neither, it is refused, however good it is.

**Thirty-six verbs `[FACT]`**, counted from the command table itself:

```
    python3 - <<'EOF'
    import re; s=open('firmware/components/cmd/builtins.c').read()
    i=s.index('static const cmd_t'); t=s[i:s.index('};',i)]
    print(len(set(re.findall(r'\{\s*"([a-z0-9]+)"\s*,\s*c_', t))))
    EOF
```

That command is here because the number was wrong twice. Earlier revisions of
this document said sixty-three, from a grep that also matched the `{ "kick", 36 }`
note-lookup tables — data rows, not verbs. A document whose job is to refuse
features on the strength of a count has to be able to show its arithmetic.

**And it went stale a third time, which the snippet did not prevent.** `box`,
`star`, `spin` and `cc` were added, the snippet was not re-run, and three sections
of this document went on saying sixty-five while the firmware said sixty-nine — a
review found it, not the author. A snippet only helps a document that is run against
it, so §0, §2 and §8.5 all take their number from the same place now. The
primitive rework in §9.5 traded three names for three and did not change it.

**Sixty-nine became thirty-six on 2026-09-25, by the snippet, and none were added.**
The thirty-three lane names stopped being verbs: a sound name is a line in the boot
document — `>kick = note 36` — and a picture answers to its own name, so the one lane
command serves all of them ([MANIFESTO.md](MANIFESTO.md) §3.8). That is criterion 5
of §8 honoured by thirty-three, where [NEXT.md](NEXT.md) guessed thirty-one.

---

## 1. The one idea `[JUDGEMENT]`

A **lane** is one line of text that answers three questions:

```
    WHEN      the pattern       x..x..x.
    HOW MUCH  the digits        0..3..9.
    WHERE     the binding       a note, a controller, a shape, a wire
```

That is the entire language. Everything else is either a way of writing a lane,
a way of looking at lanes, or a place lanes go.

**The audio and the visual world are not two worlds.** `>kick x...x...` and
`>disc x...x...` are the same sentence with different destinations. A kick is a
lane bound to note 36 on channel 10; a circle is a lane bound to a drawing
primitive; a filter sweep is a lane bound to CC 74; an OSC message is a lane
bound to a path. The deck does not know or care which — it compiles text into
*when* and *how much*, and hands the result to a binding.

This is what makes the device a controller brain rather than a sequencer with a
visualiser bolted on. The outputs are swappable because the language never
mentions them. Add a DIN jack, an RP2040 over HDMI, a light rig, a plotter — the
language does not change, because it never described the output in the first
place.

**Corollary, and it is the thing to hold on to:** if a feature request can only
be satisfied by teaching the language about a specific output, the request is
wrong. Teach the *binding* about the output.

---

## 2. The surface, as it stands `[FACT]`

Thirty-six verbs, grouped by what they actually touch — and the names, which are not
verbs.

### Lanes — no verbs, one command, and the names are yours

A line whose first word is a **name** is a lane. A name is defined, and the boot
document defines these sixteen at startup:

```
kick snare hat ohat clap tom rim crash      >kick = note 36      a drum on ch 10
bass lead pad arp                           >bass = voice 2 ...  degrees, octave, gate
cut res mod rev                             >cut = cc 74         a controller
```

and the sixteen pictures answer to their own names:

```
disc box turn ramp grid noise    FIELDS: distance from a thing, as a tone
mask edge                        THRESHOLDS: a level through a field, a contour of it
echo move spin warp              memory and motion
grow thin flip fold              shaping
```

Not shapes — see §9.5 for why that distinction is the whole of the third design.
Every lane, sound or picture, goes through `cmd_lane()`: resolve the name to a
binding, `seq_lane_bind()`, then `seq_lane()` to compile. A player adds a name with
a line — `>conga = note 63` — and can move one: `>kick = note 35` retunes the kick
that is already playing. An address picks a second one or a part of one: `disc:2`,
`disc:x`, `disc:2:x` (§9.3). There is no `viz` keyword and no `cc` verb; a
controller without a name gets one: `>fx = cc 20`.

### The clock — 6
```
bpm scale swing sync play stop
```

### Where events go — 6
```
send      enable or disable a destination
usb       become a USB MIDI device (reboots)
din       MIDI on a wire, no host needed
osc       set an OSC target
wifi host join a network, or be one
```

### Looking at things — 6
```
lanes jitter dump density split frame
```

### Documents — 10
```
help list new name open run save close guide prose
```

### Escape hatches and the rest — 8
```
panic flash battery kbd mute solo route ssh
```

(`kbd` was in the table and missing from this list, which added up to thirty-five.)

---

## 3. What is redundant, and the verdict

### 3.1 Two lane systems that are one lane system `[FACT]` `[OPEN]`

`seq_lane_t` and `vlane_t` are, field for field:

| | seq | viz |
|---|---|---|
| step bitmask | `mask` | `mask` |
| maybe-steps | `chance` | `chance` |
| per-step odds | `prob[]` | `prob[]` |
| per-step value | `deg[]` | `val[]` |
| step count | `steps` | `steps` |
| ticks per step | `tps` | `tps` |
| muted | `muted` | `muted` |
| pattern hash | `src` | `hash` |
| routed from | — | `src` |

Two structs, two compile loops (both walking the same `seq_pattern.h`), two mute
mechanisms, two re-run toggles, two budgets (8 lanes vs 13 primitives), two
listings. Every bug found in this area during development was found **twice** —
once in each — or found in one and left standing in the other for days. The
routing bug, the mute-does-not-clear-the-frame bug, and the bare-name-drop
behaviour were each fixed in one half long before the other.

**Verdict: collapse them.** See §4. This is the single largest deletion
available and it removes the `viz` keyword entirely.

### 3.2 Seventeen drum and voice names `[JUDGEMENT]`

`kick`, `snare`, `hat` … are one function with a lookup table. This is **not**
redundancy and stays. A name that carries a sensible default note, octave and
gate is the difference between an instrument and a MIDI implementation chart —
`>bass 0...5...` sounding like a bass before anything is configured is the point.
They cost one table row each and no language surface.

### 3.3 Four ways to silence a lane `[JUDGEMENT]`

```
>mute kick          explicit, reversible, a performance gesture
>solo kick          derivable from mute, but one keystroke in a hurry
>kick               bare name: the lane is GONE, frees its slot
re-run the line     toggle, the live-coding gesture
```

Four, and all four survive — they mean genuinely different things and each has
been reached for. But they must behave *identically across audio and visual*,
which they did not until recently and which §4 makes structural rather than
maintained.

### 3.4 Destination configuration is four shapes `[OPEN]`

```
>send <name> on|off      the general mechanism
>usb on                  ... which also reboots
>din <gpio>              ... which also configures
>osc <ip> <port>         ... which also configures
```

`send` is the enabler; the others are *configuration* that implicitly enables.
That is defensible but undiscoverable: nothing tells you `>send` is the thing
they have in common. **Candidate:** `>send din 17`, `>send osc 10.0.0.5 9000`,
`>send usb on` — one verb, and each destination parses its own argument, which is
already the calling convention ([COMMANDS.md](COMMANDS.md)). Costs: breaks boot
documents already written. Not decided.

### 3.5 View verbs mixed into the language `[JUDGEMENT]`

`density` and `split` describe *the screen*, not the piece. They are correctly
separate verbs and should never gain pattern syntax. Noted here so that the next
time something wants to be "a visual parameter", it is asked which of the two it
is: a property of the piece (a lane) or a property of the monitor (a view verb).

---

## 4. The unification `[FACT]` — done

One lane table. A lane holds a pattern and a **binding**:

```c
typedef enum {
    BIND_NOTE,      /* channel, note, velocity curve, gate  */
    BIND_CC,        /* channel, controller number           */
    BIND_VIZ,       /* which drawing primitive              */
    BIND_OSC,       /* a path                               */
} bind_kind_t;
```

What this buys, concretely:

- **One budget.** Today: 8 music lanes *and* 13 visual primitives, separately.
  After: N lanes, and the player decides the mix. A piece that is mostly visual
  stops being penalised for it.
- **Routing between anything and anything**, for free, because there is one
  `last_value` field on one struct. `>route disc kick` and `>route cut disc` and
  `>route lead bass` all work by the same mechanism. Today the third is
  impossible and the second was impossible until this week.
- **`viz` disappears.** `>disc x...x...` — one fewer keyword, and the visual
  primitives become peers of `kick` rather than arguments to a subsystem.
- **One mute, one re-run toggle, one listing, one compile path.** Every bug in
  this area becomes findable once.
- **A new output is a new `bind_kind_t`.** The RP2040 over HDMI, a light rig, a
  plotter: a binding, not a language change. This is the medium-agnosticism,
  made structural instead of aspirational.

Cost: a real refactor of `seq.c` and `viz.c`, and `>viz noise 2` stopped working
in documents already saved. It was kept as an alias through the collapse and is
now deleted — one name, one way to write it. An alias that survives to the freeze
is an alias that survives for ever.

**Done, and measured on the deck.** Ten lanes in one listing — `kick hat bass
cut` alongside `echo noise move disc grow warp` — and three route chains that
cross bindings:

```
    disc  <- kick      a circle follows a drum
    grow  <- disc      a bloom follows the circle      <- was impossible
    warp  <- cut       a bend follows a filter sweep   <- was impossible
```

`kick → disc → grow` is a two-hop chain across three different bindings, which
is the emergent behaviour the separate tables could not express: routing existed
in the visual half only, so a drum could drive a circle and a circle could drive
nothing at all.

Clock after the collapse, with all ten lanes running: **sd 3 µs, spread 79 µs,
zero late, 5294 of 5294 ticks inside 0.1 ms.** Unchanged — the drawing lanes go
through the same `fire_lanes()` loop as the notes and still only *mark* the
frame, leaving the picture to the main loop.

Verb count went **53 → 65**: `viz` is gone and the thirteen primitives took their
own names. That is a widening of the *table* and not of the language —
`>disc` is a name bound to a destination exactly as `>kick` is, which is the
argument §3.2 already makes for the seventeen drum names. What was actually
deleted is a whole second lane system: one struct, one compile loop, one mute,
one re-run toggle, one routing mechanism, one budget, one listing.

Two traps it re-created and which are now closed: `>route grow disc` failed on a
fresh document because seq will not invent a lane and does not know what a
primitive is (the command layer binds it first now), and the listing printed
`x.x.x.x.` where `2.4.2.4.` had been typed, because it tested `melodic || ctrl`
and a drawing lane is neither.

---

## 5. Nesting `[FACT]` — done

**A bracket subdivides the step it occupies, to any depth.** One rule, recursive,
and it is how a bar is already read on paper:

```
    x..[xx]          four steps; the last is two half-steps
    [xxx]...         a triplet in the first step of four
    [xx][xxx]        two against three, in one bar, from one line
    x.[x[xx]].       the second of a pair splits again
```

**Probability moved to `%`.** `x%15` is a fifteen-per-cent chance on that step
or group. The bracket is the only punctuation a player already reads as grouping,
and a group — a step that *contains* steps — has a better claim on it than a
parameter *of* a step. Recommendation (1) from the previous version of this
section, taken. (`?` was kept as "a half" until 2026-09-25, when it went —
[MANIFESTO.md](MANIFESTO.md) §3.5.)

**It is a compile-time transform, and that is the important part.** The clock
never parses text ([SUBSTRATE.md](SUBSTRATE.md)), so nesting is resolved in
`seq_pattern_compile()` by **laying the tree onto a uniform grid of slots**.
`x..[xx]` is eight slots at half the step length with hits at 0, 6 and 7. There is
no second code path for a nested lane — the sequencer is as unaware of nesting as
it is of `/2`.

What the clock reads changed on 2026-09-25, when a step stopped being one character
([MANIFESTO.md](MANIFESTO.md) §3.6): it was four bitmasks and three per-slot tables,
and it is now a list of **events** sorted by slot, each with its amount, its odds, the
slots a tie holds it, and a cycle class for alternation. A chord is several events on
one slot. The compiled lane is **handed to the clock** rather than written under it —
the clock runs on the other core, and a list read through an index cannot be torn
safely the way a bitmask could.

Each top-level step is given `div` slots, where `div` is the least common
multiple of what its members need, so every leaf lands exactly on a slot
boundary. `[xx][xxx]` needs 6 per step and 12 in total.

**A slot's tick was wrong for any split that does not divide 24, and it drifted.**
The clock has 24 ticks to a sixteenth, and a slot was `24 / div` ticks in integer
arithmetic — so a five-way split got 4 ticks where it needed 4.8, and `[xxxxx]...`
looped in 80 ticks instead of 96 — sixteen ticks, 81 ms at 124 bpm, early every
bar against everything else. That figure is **arithmetic, not measured**: the old
firmware was never run with a quintuplet on the deck; the host check shows the
80-tick loop, and the fix below was measured. Slot *g* now starts on the tick
nearest `g × 24 × rden / (div × rnum)`, computed from the global tick — so every bar
is exactly a bar (484 ms at 124 bpm, six bars running) and each note is within half
a tick of where it belongs: the five onsets measured 0, 25, 50, 71, 96 ms against an
ideal 0, 24.2, 48.4, 72.6, 96.8. A split so fine that two slots would share a tick
is refused.

**What does not fit is refused, not truncated** — `[xxxxx][xxxx][xxx]` would need
180 slots, and is refused with that number.

**Reversed 2026-09-25: a flat pattern is no longer clamped either.** This section
argued that truncating a flat line "loses the tail and nothing else", so a line of
seventy steps played its first sixty-four. The fact that changed is that every other
malformed pattern is now refused with its reason (MANIFESTO §3.2), which left the
clamp as the one place in the language where typed steps silently did nothing — the
failure [MANIFESTO.md](MANIFESTO.md) §1 calls undebuggable. It says *needs 70 slots,
64 fit* now.

The walk also collapsed the last duplicated traversal in the system. `seq_lane()`
used to walk the characters itself and the editor walked them backwards to place
the playhead; both call `seq_pattern_compile()` now, so they cannot disagree about
which characters are steps. The editor then asks the lane where it is —
`seq_lane_now()` — because it used to take the global sixteenth modulo the lane's
length, and a `/2` lane's playhead ran at twice the speed of its sound.

## 6. Two decks `[OPEN]`

Two performers, two surfaces, outputs to different media, one shared time. Three
things have to be true and they are independent:

1. **Shared clock.** The hard requirement. Options, in order of ambition:
   - MIDI clock from one deck's DIN out to the other's DIN in. Needs an
     optocoupler and a MIDI *input*, which does not exist yet. Cheapest, and
     jitter-free in the way DIN MIDI always is.
   - A UDP beat packet over SoftAP. One deck hosts, the other joins — both
     already possible. ~100 lines, reuses the OSC path, and the accuracy is
     whatever WiFi gives, which is worse than a wire.
   - **Ableton Link.** Solves this *and* the laptop case *and* every other Link
     app at once, which is why it is the right target even though it is the most
     work: UDP multicast peer discovery plus a shared beat/phase timeline.
2. **Shared surface.** Optional and separable. The document is text, so this is
   "send the buffer" — the `>frame` path already sends a rectangle of characters
   over OSC. Sending the *document* instead is the same mechanism.
3. **Independent bindings.** Already true, and it is the payoff: deck A's `kick`
   goes to a drum machine over DIN, deck B's `kick` drives a projector over OSC,
   and neither line of text mentions either.

The order matters: **1 without 2 is a duet, 2 without 1 is a shared text editor.**

---

## 7. MIDI reality `[FACT]`

Measured, not assumed. The deck's destinations:

| sink | what it is | drives hardware directly? |
|---|---|---|
| `usb` | USB MIDI **device** | no — needs a host |
| `ble` | BLE MIDI peripheral | only a BLE MIDI host |
| `osc` | UDP over WiFi | only something listening |
| `din` | MIDI on a UART, 31250 8N1 | **yes** |
| `mon` | console echo | no |

There is no USB **host** and no MIDI **input** of any kind.

**What this means in a studio.** A USB MIDI device cannot talk to another USB
MIDI device: an SP404, a class-compliant MIDI interface, and most desktop gear
are devices, exactly like the deck. A Mutant Brain has no USB at all — it wants
five-pin DIN. So before `din` existed, *every* path from this deck to hardware
ran through a computer.

`din` removes that, and it is the reason it was worth building before anything
else in this document. The firmware is done; **the wiring is not, and cannot be
done in firmware** — a MIDI output is a current loop, not a logic level. See
[HARDWARE.md](HARDWARE.md). `>din 17` declares the pin; `>din` reports bytes
actually written, so the wire can be proven busy without a scope.

**Still missing, and the honest list:**
- MIDI **in** (a clock or a controller into the deck) — needs an optocoupler.
- USB **host** (plug a controller into the deck) — the S3 can do it, but the
  single USB PHY is already spoken for by USB MIDI and the console.

---

## 8. What "done" looks like `[JUDGEMENT]`

The freeze criteria, written now so that "one more feature" has something to
argue against later:

1. ~~**One lane table.**~~ Done, and the `viz` alias is deleted with it.
2. ~~**Nesting decided and implemented.**~~ Done. One rule, recursive, resolved
   at compile time.
3. **One shared clock mechanism** that covers both a second deck and a laptop.
4. **MIDI in and out on a wire**, so the deck needs no computer at all.
5. **Thirty-six verbs down, not up**, and counted with the snippet in §0 rather
   than by eye — *by running it*, which is the part that failed. The collapse spent
   twelve of them buying one lane system; nothing else may spend any without deleting
   its own. §9.5 was the first change to honour that literally: `turn`, `mask` and
   `edge` in, `star`, `shake` and `tile` out, same number either side. The names
   becoming definitions (§2) was the second, and it went from sixty-nine to
   thirty-six with nothing added.
6. **Every verb in one printed page**, because a performer cannot search.
7. **No verb that exists only to work around another verb.**

When those are true the language stops. Outputs may still be added after that —
that is what a binding is for, and adding one is not a language change. But the
*text* is finished, and the device is an artifact.

---

## 9. Proposals, judged `[JUDGEMENT]`

Candidate additions, measured against §0: *does this delete something, or collapse
two things into one?* Ordered by emergence per character, which is the only
exchange rate that matters on a thumb keyboard.

### 9.0 What this device is not, so the comparison is honest

| | its core idea | why the deck does not chase it |
|---|---|---|
| **PD / Max** | the program is a graph you draw | no mouse, one screen, and the graph *is* the interface. A text deck imitating a patcher gets the costs and none of the payoff. |
| **SuperCollider** | a language plus a DSP server | the deck makes no sound at all and never will — the synth voice is a separate box. SC's domain is orthogonal, not competing. |
| **TidalCycles / Strudel** | a pattern is a *function of time*, composed with combinators | **this is the real relative**, and the line to hold is notation versus functions. See below. |

**The line: take Tidal's NOTATION, refuse Tidal's COMBINATORS.**

`[xx]`, `*2`, `?` and per-step odds are already here and they are notation — marks
inside a pattern, resolved at compile time onto a flat grid. `every 4 (fast 2) $
jux rev $ ...` is function composition, and adopting it means adopting a language:
parser, evaluator, error reporting, and a document that is code rather than a
score. That is Tidal's job and Tidal is better at it than this will ever be.
Strudel exists and runs in a browser; competing with it on a 1-bit screen is a
losing move and an uninteresting one.

So: anything expressible as a **mark in a pattern** is fair game. Anything needing
a **function applied to a pattern** is refused.

*Measured 2026-09-25* ([STRUDEL.md](STRUDEL.md)): of the 216 distinct patterns in
Strudel's own example tunes, 145 are notation this deck has, and the deck plays all
145 in Strudel's rhythm exactly, checked note by note against Strudel itself. The
largest things it does not have are Euclid (§9.2) and a note spread over passes.

### 9.1 Per-cycle alternation — `<a b>` `[FACT]` — done

A step that takes a different value each bar:

```
    >bass 0...<3 5>...        the third step is 3 this bar, 5 the next
    >disc <9 3>               breathes big, small, big, small
    >kick x...<x .>...        a hit that is there every other bar
```

**Why it is first.** Everything the deck does now repeats exactly. This is the
single cheapest source of *variation over time* — the thing that separates a
pattern from a loop — and it is pure notation, one more bracket type. Tidal's
`<>` is exactly this and it is the most-used piece of its mini-notation for good
reason.

**It cost no runtime state at all**, which is better than the estimate above. That
estimate assumed a per-slot alternative table indexed by a bar counter in the fire
path. The right answer was the one nesting already uses: **expand at compile
time.** Lay the pattern down once per cycle with the group resolved differently
each time, and the lane's ordinary wrap does the alternation — a sixteen-step lane
alternating two ways is simply a thirty-two slot lane, on the same flat bitmask at
the same uniform rate. No variant table, no cycle counter, nothing new that can be
late.

The budget moved to pay for it: `SEQ_MAX_STEPS` is 64 and the four bitmasks are
64-bit, which is eight bytes a lane and lets a sixteen-step lane alternate three
ways or nest and alternate together.

Composes with everything, verified on hardware: `[x<x .>]`, `<[xx] x>`,
`<a b><c d e>` (six bars to repeat), `x%15<3%20 5%80>` (odds travel with the
alternative), and `>disc <9 3>` — a drawing lane alternating, because a drawing
lane is a lane.

**Superseded 2026-09-25: alternation is a cycle class now, not slots.** The
flattening above was right about runtime state and wrong about its cost — it spent
`lcm` slots, and [MANIFESTO.md](MANIFESTO.md) §3.9 named the cliff: a sixteen-step
lane with a two-way and a three-way alternation needed ninety-six slots and was
refused at sixty-four. Each note now carries the class of cycles it plays on
(`cycle % per == ph`), and the cycle is the global tick divided by the lane's length
— so the "one byte of cycle counter" the manifesto proposed turned out to be none.
Alternation costs no slots at all.

And the flattening had a second fault nobody had found, because nothing compared it
with anything: it passed the cycle number down unchanged, so `<0 <1 2>>` played
0 2 0 2 — the inner group advancing on every bar rather than every time it was
chosen. Strudel plays 0 1 0 2; so does the deck now. The comparison is what §11 of
[NEXT.md](NEXT.md) proposes a corpus for, and this is the first thing one found.

One bug worth recording: `<[xx] x>` first compiled with the group given a single
slot, silently dropping half of it. The rule "how wide is one item" had been
written twice — once in `seq_pattern_span` and once in `seq_pattern_walk` — and
only one copy learned about `<>`. Two copies of one rule is exactly the drift this
header exists to prevent, and it had drifted inside itself. There is one copy now.

### 9.2 Euclidean rhythm — `x(3,8)` `[OPEN]`

Three hits spread as evenly as possible over eight steps.

```
    >kick x(3,8)       the tresillo
    >hat  x(7,16)
    >rim  x(5,8)
```

**Why it is second.** It generates most of the world's rhythms from four
characters, it is a *mask generator* so it fits the flat grid exactly, and the
algorithm is twenty lines. Enormous ratio.

The argument against: `x..x..x.` is eight characters and says the same thing more
plainly, on a device whose whole premise is that the text *is* the score. `(3,8)`
is a rhythm you cannot see. That is a real objection and it is why this is second
rather than first.

### 9.3 Addressable parameters — `disc:x` `[FACT]` — done

The visuals have no positioning: every source draws centred or full-frame, and
the only movement is `move`, which translates the whole frame.

The wrong fixes, and why:

- **`disc 5,3`** — two numbers in a step. Breaks one-character-per-step, which is
  what keeps the playhead on the character that is sounding.

  *Re-argued 2026-09-25.* That rule is gone ([MANIFESTO.md](MANIFESTO.md) §3.6), so
  this refusal needs a reason that survives it, and it has one: `,` now means
  **simultaneous** — `[5,3]` on a disc lane is two circles at once, amounts 5 and 3,
  exactly as `[0,4,7]` is three notes at once. Spending the same mark on
  "coordinates" would give one character two meanings depending on the lane. A
  position is a parameter, and a parameter is a lane (below). The refusal stands;
  its old reason does not.
- **A `>at 3,7` verb** — pairs lanes by convention. Fragile, and a new verb that
  deletes nothing.
- **More primitives with position baked in** — `discleft`, `discright`. This is how
  a vocabulary rots.

**The right fix generalises the thing that already works.** A lane has
*parameters*, and `route` can target one:

```
    >route disc.x bass         the bass note moves the circle horizontally
    >route disc.y lead         the lead moves it vertically
    >route disc.r kick         and the kick still sets its radius
    >route hat.vel cut         a filter sweep drives hi-hat velocity
```

Why this is the right shape: it is the **binding collapse applied to
parameters.** §4 made a lane's destination addressable and that one change bought
routing across every pair, one budget, one listing. This does the same to a
lane's *inputs* — and it costs no new verb, no new pattern syntax, and no new
concept. `x,y` positioning falls out of it, and so does everything else anybody
will ask for next.

**Done, and it went further than routing.** The separator is `[]`, not `.`, because
the bracket is already the referential mark in this language and `.` is a rest — a
separator that means "nothing happens" everywhere else is a bad separator.

And the realisation that made it better than planned: **a parameter is a lane.**
`>disc[x] 0..3..6..9..` is not just a route target, it is a lane with a pattern —
so position alternates, nests, takes odds and a rate, and can be routed, all
because it was never a special case. Verified on hardware:

```
 disc      4
 disc[x]   <- bass        routed
 disc[y]   27             from <2 7>
 disc2[x]  9...0...       the second instance, its own path
```

`x` and `y` are the only parts, deliberately. A short list per binding, or it
becomes the flag grammar [COMMANDS.md](COMMANDS.md) exists to refuse.

`>box[z]` is refused with "no part called that" rather than ignored, because a
part that silently does nothing is a lane that silently does nothing.

**Respelled 2026-09-25: a part is `disc:x`, an instance is `disc:2`, and `[]` only
groups** ([MANIFESTO.md](MANIFESTO.md) §3.3). The bracket was chosen here as "the
referential mark", and that was the mistake: inside a pattern `[]` means *contains*,
and a part is a property *of* the lane, not a lane inside it — the same argument that
had already moved probability off the bracket. A trailing digit was a third notation
for the same idea and reserved every name's last character. `:` is what
[SUBSTRATE.md](SUBSTRATE.md) already uses for "a part of" (`lullaby.md:27`, `din:1`),
it never meets the pattern grammar, and it is legal in an OSC address, where `[` and
`]` are pattern characters. The objection to `.` above still stands; the objection
to *every* separator did not. On the deck: `>disc[x] 0..9..` answers *disc[x] is
disc:x now*, `>disc2 x...` answers *disc2 is disc:2 now*, and `>route disc:y kick`
routes. The listing above now reads `disc:x`, `disc:y`, `disc:2:x`.

### 9.3b More primitives, and two refusals `[FACT]`

Three added, each held to the bar in `viz.h` — *does it bring an axis the others do
not have?*

| | axis nothing else had |
|---|---|
| `spin` | **rotation.** `move` translates, `fold` mirrors, `warp` displaces — all leave orientation alone, so a shape could never turn. Quarter turns only: an arbitrary angle needs interpolation, and on a grid where a cell is twice as tall as it is wide there is no interpolation that does not smear. A quarter turn is exact. |
| `box` | **hard corners.** It was in the first set and folded away during the collapse on the grounds that a disc through `warp` is nearly a box. That was wrong — a warped disc has no corners, and a corner is what a box is for. |
| `star` | **angular rays.** `disc` is a radial area and `grid` is orthogonal lines; nothing drew anything at an angle. Through `spin` it turns, which is the pair this set was missing. |

**Two were refused**, and the reasons matter more than the additions:

- **`dots`** — an ordered dot field. `ramp` is already an ordered dither and `grid`
  at a high amount is already a regular field. It does not clear the bar.
- **`edge`** — keep only the boundary. That is `grow` composed with `thin`, and
  §9.4 refuses `a!3` on exactly that ground. Refusing this one and accepting
  `a!3`'s refusal has to be the same decision or neither means anything.

### 9.4 Refused

- **Combinator syntax** (`every`, `jux`, `off`, `superimpose`). Functions, not
  notation. §9.0.
- **`a!3`** — repeat a step. `[xxx]` already says it.
- **A second visual pane, or layers with z-order.** A pipeline with thirteen
  operators already composes; layers would be a second compositional model beside
  the one that works.
- **Sample or preset selection** (`kick:3`). The deck does not make sound; what a
  note means is the receiver's business, and teaching the language about sample
  banks is teaching it about one specific output. §1's corollary.
- **Anything with a menu.**


### 9.5 The sources were shapes, and should have been fields `[JUDGEMENT]`

Two adversarial reviews — [MANIFESTO.md](MANIFESTO.md) §2 — were given the docs and
the firmware and told to find what was wrong. They reached the same two conclusions
separately, and the owner had already reached one of them unaided: *"star? wtf is
that? we should be going more fundamental to allow for more expressive
possibilities."*

**The complaint, stated precisely.** `star`'s amount was a **count of spokes**, three
to twelve. Every other amount in this language is a magnitude — that is what "a digit
is always how much" means — so `star` was the one place the rule did not hold. And it
composed with nothing: `grow` made it a blob, `thin` erased a one-cell spoke, `spin`
on a four-spoke star is the identity. It was not a primitive. It was one picture with
a verb in front of it.

It was not uniquely guilty. `disc`, `box`, `star`, `grid` and `ramp` were all *shapes*,
and a shape bakes its own hard edge in, so the only figure a source could ever draw is
the one its author chose. §9.3b's own table gives the game away: each of the three
additions is justified by *the picture it makes* rather than by an operation.

**What replaced it.** A source is a **field** — it answers "how far is this cell from
the thing", in its own geometry, as a tone. A *shape* is then a field plus a
threshold, and the threshold is what was missing:

| | |
|---|---|
| `disc` | euclidean distance — round |
| `box` | chebyshev distance — square, the corners disc cannot have |
| `turn` | the **angle** around the point: amount is how much of the circle |
| `ramp` | distance along one axis |
| `grid` | distance to the nearest lattice line |
| `noise` | no geometry at all — the entropy, irreducible |
| `mask` | keep what is at least this bright — a **level** through the field |
| `edge` | keep where the field changes fast — a **contour** of it |

Now reachable, five of the six for the first time: a ring (`disc` `edge`), a
rectangle outline (`box` `edge`), spokes (`turn` `edge`), a hard-edged wedge (`turn`
`mask`), a contour map (`ramp` `edge`), and a **rotating radar sweep** with a fading
tail (`turn` `spin` `echo`) — which is the one that says the trade was worth making,
because the old set could not turn anything continuously at all.

**Three in, three out.** `star` (above), `shake` (`warp` with a random displacement
instead of a smooth one — the same idea stated twice, and reachable by routing `warp`
from `noise`) and `tile` (repetition of the frame, where `fold` mirrors it and `grid`
now supplies periodicity as a field). Sixteen names before, sixteen after, criterion 5
honoured for the first time.

**§9.3b refused `edge`, and that refusal was wrong on a fact.** It said `edge` is
"`grow` composed with `thin`". It is not. `grow ∘ thin` is a morphological *close* — it
fills gaps and leaves a solid shape solid. A boundary is `inked − eroded`, a
**difference**, and there is no difference operator in this pipeline, so `edge` was
never reachable by composition and the refusal rested on arithmetic nobody checked.

That matters for the other half of the same paragraph, which said refusing `edge` and
refusing `a!3` had to be the same decision. They are not the same decision, because
they turn on different facts: `[xxx]` genuinely *is* `a!3`, so that refusal stands.
`grow thin` was never `edge`. A synonym and a wrongly-assumed synonym are not alike,
and the way to tell them apart is to compute the composition rather than describe it.

### 9.6 The draw order was a ceiling `[JUDGEMENT]`

Both reviews, independently, said the same thing about it: sixteen primitives in a
frozen chain is not composition, it is a mixer with sixteen mute buttons. The table in
`viz.c` decided the order, so `thin` then `grow` — which despeckles — and `grow` then
`thin` — which closes gaps — were one table entry apart and only ever one of them was
reachable. §9.4 refuses layers on the grounds that "a pipeline with thirteen operators
already composes", and that premise was false while the order was fixed.

The mechanism was already in the language. `route` states a relationship between two
lanes, so a **routed lane now draws after the lane it follows**: `>route thin disc`
then `>route grow thin`. A lane's rank is how many route hops it is from a lane that
follows nothing, and within a rank the table still decides — which keeps the promise
`tools/test_viz.c` checks, that a document of unrouted lanes draws the same whatever
order its lines were typed in. Order became something a performer can state and could
not state before, and nothing that worked before behaves differently.

### 9.7 Finite repetition — `!4`, and what an ending is for `[FACT]` — done

**A lane that plays n times and then stops** ([NEXT.md](NEXT.md) §4). The spelling
was the owner's to choose, 2026-09-25, from four with real collisions: `!4`, because
`!` already means *repeat* to anyone arriving from Tidal or Strudel and only the scope
differs — the whole lane, over time, rather than one step squeezed into its space.
`@4` read as "at" and is Strudel's note length, which `_` now is here; a bare `4`
would change what `>bass 0... 4` means, since patterns may be spaced; `#4` is a
comment. A Strudel-style `x!3` inside a pattern is refused and told where `!` goes.

**What a count counts** is passes of the lane's own length. Typed mid-song it waits
for its own downbeat, so the first pass is a whole one and "four times" is four; at
`>play` every lane's downbeat is tick 0. With `/2` a pass is twice as long, and with
`<a b>` the alternation restarts with the count.

**What happens at the end was designed with the count**, because an ending is only
worth having if it can start something:

- A lane on its own goes quiet and says `done` in `>lanes`. Running its line again
  restarts it — it is silent, so the toggle brings it back — and `>play` re-arms
  every finished lane, so a stopped arrangement plays from the top.
- It is a **source** from then on: `name:end`. `>route crash intro:end` is a crash on
  the downbeat after the intro.
- **A routed lane with a count is a cue.** Its source *starts* it, and it plays its
  own pattern for its count — where a routed lane with no count is a sidechain and
  plays one step per event. So `>route verse intro:end` with `>verse x.x.x.x. !8`
  sequences two sections with no new verb, and `>route fill snare` with
  `>fill [xx]x !1` is a fill after every snare. The count is the whole difference.

Measured on the deck: `>intro x.x. !2` played two passes and read `done`; on its last
downbeat a verse cued from `intro:end` and a crash routed from it both went out on the
same timestamp as the kick; the verse played its two passes and went back to `waits`.
Typed 1.1 s into a bar, `>intro [xx]... !1` waited for its own downbeat.

**To make that same timestamp possible, lanes now fire in route order.** A routed lane
hears its source when the source fires, so the source must fire first; the table used
to decide, and a sidechain typed before its source heard it one tick — 5 ms — late,
by the structure of the code (not measured on the old firmware). Ranks are route
hops, recomputed on every edit, never per tick; on the deck a rim routed from a kick
and typed before it sounded on the kick's own timestamp.

**A routed lane plays at its source's level, on every binding.** The source decides
*how much* as well as *when*: the trigger carries its whole value, 0-127, and a
controller sends it as it is, a note plays at it, a picture and a part scale it to
their nine steps. Before 2026-09-25 that was true only of pictures. On the deck, with
`>kick 9...3...`, `>route cut kick` sent no controller message in a bar of kicks —
a controller read its own first step, which a routed lane fills with `x`, meaning
hold — and `>route rim kick` hit at 100 while the kick went 127 and 42. After, the
cut followed the kick and the rim hit at the kick's velocity.

**Open, and recorded rather than guessed at:** a *loop* of sections — verse after
chorus after verse — needs a lane with two sources, and a lane has one. The
arrangement that exists today is linear, with the last section looping by having no
count.
