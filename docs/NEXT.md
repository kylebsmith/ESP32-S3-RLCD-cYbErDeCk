# The next development push

*A brief, not a wish list. It is ordered: §3 gates almost everything else, and doing
§5–§10 before §3 means doing them twice. Read [MANIFESTO.md](MANIFESTO.md) first — this
document assumes it and does not repeat its findings.*

**The goal, in one sentence.** A performer picks this up with no manual and is making
sound and picture inside five minutes — easier to start than EarSketch — and two hours
later is doing things Processing and Strudel cannot, because the same line drives a drum,
a circle, a eurorack module and a knob on somebody else's desk.

---

> **Where this brief stands — 2026-09-25, after the push it describes.** The brief
> below is left as it was written; this block says what became of it.
>
> - **Done, on both boards:** §3 (the step, errors, addressing, names), §4 (`!4`),
>   §11 (the corpus, 51 of 51), §6 (the HDMI node — the deck-to-node cable is still a
>   relay on the laptop), §8 (OSC in, as §5's mechanism: `>knob1 = knob`), §7 (Link
>   not integrated, for the licence), §9, §10 (the defect, and three more).
> - **Waiting on hardware:** §5's satellites, the direct deck-to-node link, MIDI in.
> - **Two claims in this brief did not survive measurement.** §2's "two decks in
>   phase within 35 µs" was the grids, not the notes: a follower's ticks drifted 2 ppm
>   and two decks' steps fell up to 71 ms apart — both fixed ([NETWORK.md](NETWORK.md),
>   *A correction* and *And the count*). §9's "the panel push is almost certainly the
>   budget" was the drawing, at seven times the push — fixed
>   ([HARDWARE.md](HARDWARE.md), *The drawing was the budget*).
> - **Still open:** `>flash now` from USB MIDI mode, and the unverified items listed
>   in [the pull request for this push](https://github.com/kylebsmith/ESP32-S3-RLCD-cYbErDeCk/pull/1).

---

## 1. Ground rules — non-negotiable, carried from previous sessions

1. **Never claim something is tested when it is not.** Say "unverified" in those words.
2. **Every fix ships with a check that provably fails on the old state.** If you cannot
   write one, you have not understood the bug.
3. **Never ask the owner to hold a button.** Every flash is button-free. Two routes exist;
   see [HARDWARE.md](HARDWARE.md).
4. **No hangs. The device must never become unreachable.** If you are stuck in a
   troubleshooting loop, confirm the issue is not blocking, mark it for later, and move on.
5. **Commits carry no co-author trailer.** The owner's instruction.
6. **Secrets never enter a document.** A document mirrors to the SD card and is copied to
   the owner's home DGX with their writing. This is why §10 is a defect report and not a
   feature request.
7. **Measure before you claim.** Three times this week a confident causal story was
   backwards, and each time a counter or a histogram settled it in one step where
   reasoning had failed for several. When a number has no physical meaning — a 24 ms
   round trip for a 26-byte frame — believe the physics over your arithmetic.

---

## 2. Where the thing stands

**Solid, verified, leave alone unless you have a measurement:**

- The clock. 4 µs standard deviation, zero late ticks, and **drawing does not move it**
  (5 µs with six visual lanes and the split on). The two-core split is why.
- Two decks in phase within **35 µs**, 32 of 32 samples inside 500 µs across tempo
  changes, over ESP-NOW with no router. [NETWORK.md](NETWORK.md) has the method and every
  wrong turn.
- One lane table for sound and picture. `>kick x...x...` and `>disc x...x...` differ only
  in where they go.
- Patterns: nesting `[]`, alternation `<a b>`, probability `%`, per-lane rate `/2 *2`,
  instances `disc2`. All flattened at compile time; the realtime core never parses text.
- MIDI out over USB (exact at the host), over DIN on a wire (1491 bytes verified), over
  BLE, over OSC.
- Visual primitives as **fields** through **thresholds**, and `route` states draw order.

**Written but never exercised:** the `STOPPED DEAD` and `RESTART HUNG` vitals verdicts.

**Known open:** `>flash now` deadlocks from USB MIDI mode (needs a CPU-only reset to
preserve `FORCE_DOWNLOAD_BOOT`). MIDI *input* needs an optocoupler. A following deck's
`>jitter` reports sd 231 µs while its own histogram says every tick was inside 100 µs —
the sd is counting the deliberate grid slides; fix the reporting, not the clock.

---

## 3. The decision that gates everything: may a step be more than one character?

**Do this first and do not start §4–§8 until it is settled**, because loop counts, chords,
note length, encoder bindings and the Strudel corpus all want the same answer.

The rule was *one character per step, so the playhead can sit on the character that is
sounding*. It is the rule used to refuse chords, note length and per-step velocity. And
`x%15` is four characters for one step, so **the rule is already broken and the refusals
it justified are void.** Half of each is the worst of both and is the current state.

**Recommendation, with the reasoning, for the owner to accept or reject.** Keep the
*goal* — you can always see what is sounding — and drop the *mechanism*. A step becomes
**one character plus optional modifiers**, and the playhead highlights the **span** rather
than the character. `seq_pattern_item_span()` already computes spans; the editor's
highlight is the only thing that has to learn. That single change buys, in one move:

| now impossible | becomes |
|---|---|
| a chord | `[0,4,7]` — and `,` stops being a ghost note |
| note length | `x__.` — `_` gets its universal tracker meaning, a tie |
| velocity on a melodic lane | the digit becomes the step scalar on every binding |
| loop counts (§4) | somewhere to put them |

And it *deletes* marks: `?` is exactly `%50`; `X` and `,` are a three-value velocity enum
that a digit replaces; two of the three rest spellings go. Manifesto §3.1, §3.5, §3.6 and
§3.7 all resolve together.

**Then, in this order, and each one is in the manifesto with its argument:**

- **§3.2 — a pattern must be able to have a syntax error.** Today anything that is not a
  rest is a hit, so `x...x...x;..` plays the semicolon silently, and an unbalanced `[`
  is absorbed into a nonsense septuplet. Reject unknown step characters and unbalanced
  brackets the way overflow is already rejected. This is the single largest reduction in
  2 a.m. pain available and it costs nothing at runtime.
- **§3.3 — one address grammar.** `[]` currently means both "group" and "part of a name",
  and `disc2` is a third notation for naming a sub-part. Use the separator
  [SUBSTRATE.md](SUBSTRATE.md) already defines: `disc:2:x`. `[]` goes back to grouping
  alone. This also kills §3.4 — `>disc 2` and `>disc2` being one space apart with one of
  them destructive.
- **§3.8 — make a lane name an alias for (binding, parameters), in the boot document.**
  The seventeen sound names are data; the sixteen picture names are function pointers. A
  player cannot add `conga`. Put the alias table in a document region and sixty-nine verbs
  become roughly thirty-eight **while expressive power rises**, because the vocabulary
  becomes editable text. This is the largest single change available and the one most
  likely to be right. It is also what makes §5 and §6 cheap, so do not defer it.
- **§3.9 — one byte of cycle counter** so `<>` stops costing `lcm` slots and hitting an
  invisible cliff at 64.
- **§3.10, §3.11** — direction as a parameter lane rather than four hit characters; an
  octave, so degrees stop at an octave and a half.

**Do not treat the manifesto as a checklist to grind.** Every item is a proposal with an
argument; if an argument is wrong, say so in the document and record why, the way
MAP.md §9.5 records that the earlier refusal of `edge` rested on arithmetic nobody
computed. A reversal with a reason is worth more than a feature.

---

## 4. Finite repetition — new requirement

A lane that plays **n times and then stops**, not forever. Owner's words. It has to
compose with `/2 *2`, with `<a b>`, and with `%`.

The count is a property of the **lane**, not of a step, so it belongs where the rate
already lives: at the end of the line. What it should be *spelled* is a real decision with
real collisions, and it needs the owner:

| candidate | cost |
|---|---|
| `!4` | free here, but `!` is Tidal's *replicate* — confusing for the incoming Strudel player §11 is meant to welcome |
| `@4` | free here, but `@` is Tidal's step weight — same objection |
| bare `4` | adds no symbol at all, which is maximally minimal; but a trailing bare digit is not self-describing and a fat-fingered amount becomes a loop count |
| `#4` | **taken** — `#` starts a comment in a guide document |

Also needed, and the same mechanism: what happens **when it finishes**. Mute itself?
Delete itself? Fire something else? "Fire something else" is `route`, and if a finished
lane can be a route source then sequences of sections become expressible without a single
new verb. That is worth more than the count itself — design them together.

---

## 5. Satellites over ESP-NOW — and they are not a new subsystem

Four rotary encoders and four buttons per gadget, N gadgets, reconfigurable from the
central deck.

**The insight to build on: an encoder is a lane whose events come from hardware instead of
a pattern, and a button is a lane that fires on press.** That is all. `route` already means
"one lane reads another lane's output", so:

```
>route cut knob1          the filter follows a knob
>route disc:x knob2       the circle's position follows another
>route kick pad1          a button triggers the drum
```

**Zero new verbs beyond registering a satellite**, and every existing mechanism — mute,
solo, instances, chaining, per-lane rate — works on a knob for free because a knob is a
lane. Do it any other way and you have built a second routing system beside the one that
works, which is exactly what the `viz` collapse was undone to avoid.

Requirements that follow:
- A satellite joins the way a deck does: **no router, no password, one word typed.** Reuse
  the ESP-NOW transport in `components/ensemble` — it already has the clock, the peer
  table and the staleness window.
- **An encoder is relative, a parameter is absolute.** Decide where the value lives: the
  deck, so that unplugging a satellite does not lose the patch and two satellites can
  share a parameter.
- **A satellite must be able to say what it is**, so `>lanes` can show `knob1 → cut` and
  the owner can tell a dead battery from a bad binding.
- Latency: a knob is a gesture, so tens of milliseconds is fine — do **not** spend the
  clock's accuracy budget on it. A *button that triggers a musical event* is different and
  should be quantised to the grid, not fired on arrival.

---

## 6. The visualization node — RP2040 with DVI

The owner has the Adafruit RP2040 DVI board. It becomes the HDMI output: tethered over
UART or SPI now, and later an RP2040 with an ESP32 companion so it joins the ensemble
wirelessly and shares the same grid.

`viz.c` already says the wire format is deliberately unwritten *because a format invented
before its reader is a format nobody implements*. **The reader now exists, so write it.**
Specifically:

- Send the **glyph bytes** (128–155), not the ASCII downsample. `viz_text()` translates
  tones to `" .:*#@"` for OSC and that is a documented lossy path for monitors; the RP2040
  wants the real tiles. `tools/font_tiles.py` generates them and is the source of truth.
- **Every frame carries the tick it belongs to.** That is what makes the visual node
  share the deck's clock instead of guessing, and it is why the wireless variant should
  join the ensemble as a follower rather than invent a second sync.
- Size and aspect must be **settable and must not change the deck's own preview**, which
  is an approximation of the output and was explicitly asked to stay that way.
- Answer the power question the owner raised: can the RP2040 run off the deck's battery
  over USB-C, and what does that cost in runtime? Measure it, do not estimate it.

---

## 7. Ableton Link — the licence gate comes first

**Get a decision from the owner before writing any code.** Link is dual-licensed GPLv2+
or commercial from Ableton, and this is a project-level choice about the whole repository,
not a technical one.

If it goes ahead, the architecture is already right: Link replaces the transport *under*
`seq_timebase()` and `seq_nudge_by()` and nothing above changes. Nothing is stubbed to
look like Link today, and nothing should be. If it does not go ahead, say so in
[NETWORK.md](NETWORK.md) and stop — the ESP-NOW ensemble already does the thing Link
would be used for between decks, and MIDI clock already does it for a DAW.

---

## 8. OSC in

OSC today is **output only** — `net_osc_send`, `net_osc_frame`, `/deck/<lane>`. Incoming
OSC is the same insight as §5: **an OSC endpoint is a lane source.** `/deck/knob1` from a
phone or a laptop should be indistinguishable, to the rest of the system, from a rotary
encoder on a satellite. If those two end up as two mechanisms, one of them is wrong.

---

## 9. Optimization — with a rule

The owner wants every ounce out of this board, in the spirit of small chips doing
absurdly complex things. Agreed, with one rule: **no optimization lands without a
before-and-after number.** An "obvious" speed-up that nobody measured is how the frame
generator ended up inside the clock callback four separate times.

Places where the technique is known and the work is not done:
- **Dirty rectangles** on the panel push. Partly there. The panel's CASET quantum is 12
  rows and RASET is 2 columns, so the cheapest correct update is quantised, not arbitrary.
- **Fixed point and lookup tables only.** `turn_of()` is already a trig-free monotonic
  pseudo-angle; `isqrt_i()` is integer. Keep it that way, and consider a table for the
  tone ramp and the Bayer dither.
- **The field sources are separable.** `disc`, `box` and `ramp` all compute a per-row
  term and a per-column term; hoisting the row term out of the inner loop is free.
- **Nine tones, one bit.** The ordered dither is the only thing standing between a
  gradient and a stripe pattern. Any optimization that changes dither phase between
  frames will crawl visibly — check it on the glass, not in a test.

Read the demoscene and Game Boy literature for the specific tricks; do not reinvent
them. But profile first: `>jitter` and the loop counter already exist, and the panel push
is almost certainly the budget, not the drawing.

---

## 10. SSH — test it, and fix the defect it has

`>ssh user@host pass <command>` exists, is untested, and the reply lands in `+ssh`. The
owner has offered their laptop as the target.

**Before testing, understand that the verb as written violates ground rule 6.** The
password is a positional argument on a line that lives in a document, and documents mirror
to the SD card and are copied to the owner's DGX. So:

1. **Do not test it with a real password on a real account.** Use a throwaway account, or
   key-based auth, or a local sshd.
2. **Report the defect and propose a fix** rather than quietly working around it. The
   password must come from somewhere a document never sees.
3. The host key is shown and **not verified**, which the help text admits. Decide whether
   that is acceptable for this device and write the decision down either way.

This is genuinely lower priority than §3. It is a wild party trick on a device whose
language is not finished.

---

## 11. Strudel — build the corpus, not the compiler

The owner asked, and flagged it as possibly nonsense. It is not nonsense, but the
tractable version is not the one that first comes to mind.

**A full Strudel compiler is not possible and should not be attempted.** Strudel is
JavaScript with first-class functions; `every 4 (fast 2) $ jux rev` is a composition of
higher-order combinators over a lazy pattern-of-events. Importing that means importing an
interpreter, and [MAP.md](MAP.md) §9.0 already refused the combinator language on purpose
— it is functions, not notation, and this device's entire premise is that a line of text
is a lane rather than an expression.

**The notation is a different matter, and it is already 80 % shared.** `[xx]`, `<a b>`,
`*2`, `?`, `_`, `,` — Strudel's mini-notation and this language's step grammar are
convergent because they are solving the same problem. So the valuable artefact is:

> **A Strudel mini-notation conformance corpus.** A file of Strudel pattern strings, each
> paired with the event list it should produce. A host test compiles each through
> `seq_pattern_walk` and checks the events match. Patterns using anything outside the
> notation are listed as **explicitly out of scope**, with the reason.

That does four things a compiler would not. It **proves the notation is expressive**
rather than asserting it. It **turns every §3 change into a regression test** — the
answer to "may a step be more than one character" is checkable against hundreds of real
patterns instead of a handful of invented ones. It gives an incoming Strudel player a
**one-page "what transfers and what does not"**, which is worth more than an importer they
would not trust. And it costs the device **nothing** — no verb, no bytes, no runtime.

If a paste-in path is still wanted afterwards, it is a script on a laptop that converts
the subset and *tells you what it dropped*, never a silent translation. And the
philosophical objection the owner half-raised is right: this device is the brain of an
ecosystem, not a client of somebody else's language. Take the notation, refuse the
combinators, and say so out loud.

---

## 12. The thesis — write it down while you build

The owner's aside is the sharpest thing in the whole conversation and it is a research
contribution, not a digression:

> a laptop performer has to overlay their code to prove they are doing anything, like the
> overhead cam in DJing — cynicism rooted in a lack of causal understanding from the
> audience.

That is the argument for this device's existence, and it is testable. A handheld deck with
physical satellites makes agency **visible without projection**: the gesture and the
result are co-located, so the audience gets causality for free instead of being asked to
read code. The [NIME/ICMC](MAP.md) target should be built around exactly this, and the
design decisions that serve it — a screen a performer *holds*, encoders someone can *see*
being turned, outputs that are agnostic so the deck is the instrument rather than the
laptop's peripheral — should be recorded as they are made rather than reconstructed later.

Keep a running note. Every time a decision in §3–§9 is made for legibility rather than for
power, that is the paper writing itself.

---

## 13. Order, and what done looks like

```
  1. §3  the step decision, then §3.2 errors, §3.3 addressing, §3.8 aliases
  2. §4  finite repetition, designed together with what-happens-when-it-ends
  3. §11 the Strudel corpus - it makes 1 and 2 verifiable, so it comes early
  4. §5  satellites, which §3.8 has made cheap
  5. §6  the visualization node
  6. §8  OSC in, folded into §5's mechanism
  7. §7  Ableton Link, if the licence question is answered
  8. §9  optimization, continuously, never without a number
  9. §10 ssh, last, and report its defect whatever else happens
```

**Done** means: every host check passes (`check_docs`, `check_fonts`, `test_viz`,
`test_seq_pattern`, `test_midi_wire`, and the new corpus); both boards flashed and
exercised on hardware, not in a simulator; [VERBS.md](VERBS.md) still fits on one page and
its count still comes from the snippet in MAP.md §0 *by running it*; and every reversal of
a previous decision is recorded with the fact that changed, not just the new answer.

**A verb count that goes up needs a verb deleted.** That criterion has been honoured
exactly once. §3.8 is the chance to honour it by thirty.
