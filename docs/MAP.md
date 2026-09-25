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
