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

**Sixty-five verbs `[FACT]`**, counted from the command table itself:

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

Sixty-five names, grouped by what they actually touch:

### Lanes that make sound — 17 names, one behaviour
```
kick snare hat ohat clap tom rim crash      bound to a drum note on ch 10
bass lead pad arp                           bound to a note + octave + gate
cut res mod rev                             bound to a CC number
cc                                          bound to any CC number
```
All seventeen call the same two functions: `seq_lane_note`/`seq_lane_melodic`/
`seq_lane_ctrl` to set the binding, then `seq_lane()` to compile the pattern.
The names are *presets*, not features.

### Lanes that make pictures — 13 names, one behaviour
```
echo move warp shake         operators: they bend what is there
noise disc ramp grid         sources: they put ink down
grow thin flip tile fold
```
Peers of the drums. Each is a name bound to a primitive exactly as `kick` is a
name bound to note 36, and they go through the same `seq_lane()` as everything
else. There is no `viz` keyword.

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

### Escape hatches and the rest — 7
```
panic flash battery mute solo route ssh
```

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

**Probability moved to `%`.** `x%15` is a fifteen-per-cent chance on that step;
`?` alone is still a half. The bracket is the only punctuation a player already
reads as grouping, and a group — a step that *contains* steps — has a better
claim on it than a parameter *of* a step. Recommendation (1) from the previous
version of this section, taken.

**It is a compile-time transform, and that is the important part.** The clock
reads a flat bitmask at a uniform rate and knows nothing else
([SUBSTRATE.md](SUBSTRATE.md): the realtime core never parses text), so nesting
is resolved in `seq_pattern_walk()` by **flattening the tree onto that same
grid**. `x..[xx]` becomes eight slots at half the step length with hits at 0, 6
and 7. There is no second code path for a nested lane and nothing new that can
be late — the sequencer is byte-for-byte as unaware of nesting as it was of
`/2`.

Each top-level step is given `div` slots, where `div` is the least common
multiple of what its members need, so every leaf lands exactly on a slot
boundary. `[xx][xxx]` needs 6 per step and 12 in total.

**What does not fit is refused, not truncated** — `[xxxxx][xxxx][xxx]` would need
180 slots. A flat pattern is still *clamped* at 32, and the difference is not a
compromise: truncating a flat line loses the tail and nothing else, one
character one step, while a nested bar's subdivision is a property of the whole
bar, so dropping the end changes the meaning of everything before it.

The walk also collapsed the last duplicated traversal in the system. `seq_lane()`
used to walk the characters itself and the editor walked them backwards to place
the playhead; both call `seq_pattern_walk()` now, so they cannot disagree about
which characters are steps.

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
5. **Sixty-five verbs down, not up**, and counted with the snippet in §0 rather
   than by eye. The collapse spent twelve of them buying one lane system; nothing
   else may spend any without deleting its own.
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

### 9.3 Addressable parameters — `route disc.x bass` `[OPEN]` **the answer to x,y**

The visuals have no positioning: every source draws centred or full-frame, and
the only movement is `move`, which translates the whole frame.

The wrong fixes, and why:

- **`disc 5,3`** — two numbers in a step. Breaks one-character-per-step, which is
  what keeps the playhead on the character that is sounding.
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

Open decision: `.` as the separator, and which parameters each binding exposes.
Keep that list short and per-binding, or it becomes the flag grammar
[COMMANDS.md](COMMANDS.md) exists to refuse.

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

