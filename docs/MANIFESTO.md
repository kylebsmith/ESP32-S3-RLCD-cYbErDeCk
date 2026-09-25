# The syntax, on one page — and two reviews that tried to break it

*The language first, in as few words as it can be said. Then what is wrong with it,
from two adversarial readings that were told to be harsh and given no power to change
anything. §3 was left unimplemented on purpose until the owner had played it; the
first decision against it was made on 2026-09-25, and each entry now says whether it
is **decided**, with what was measured, or still **open**.*

---

## 1. The manifesto

**A lane is one line of text.** It answers *when*, *how much*, and *where*. There is
nothing else in the language.

```
>kick x...x...x...x...        when: a hit. where: a drum.
>disc 0..3..9..3..            when: a hit. how much: a digit. where: a circle.
```

**A drum and a circle are the same sentence.** Sound and picture are one binding
table, not two subsystems. Everything that works on one works on the other, and the
day a third output arrives — a wire, a screen, a light — it is a row in that table.

**A step is one character and what is attached to it.** The playhead lights the
whole of it, so you can always see what is sounding. It used to be *one character
is one step*, which was the constraint most of §3 was about; the goal was kept and
the mechanism dropped (§3.6).

**Symbolic, direct, never an acronym.** `%` is a percentage. `[]` groups. `<>`
alternates. A performer under stage light reads shapes, not words.

**Referential structures nest.** `[x[xx]]` is a group inside a group, to any depth.
Nesting is the only mechanism for hierarchy, so there is one thing to learn.

**Run a line again to mute it.** The whole performance gesture, and the only one.
Type a name alone to delete the lane.

**Time is the hinge.** Everything else in this instrument is negotiable. The clock
is not: 3 µs standard deviation on the local grid, and under 100 µs of phase between
two decks. Cross-modal work that is not tight is not cross-modal, it is two things
happening near each other.

**Refuse, do not truncate — and do not guess.** A pattern too long to hold is
rejected with a number; a character that is not a step is rejected with its position,
boxed in the document. Silence is the one failure a performer cannot debug, and a
typo that plays is the one they cannot hear.

### The whole of it

```
  x  hit          .  rest         _  hold the note before it
  0-9             how much: velocity, degree, value, or amount
  x%15            fifteen per cent chance on that step or group
  [xx]            a group: subdivides the step it occupies, any depth
  [0,4,7]         a chord: every member at once
  <a b>           alternates: a different member each bar
  /2  *2          this lane's own rate, at the end of the line
  u d l r         which way — in front of the pattern, or as a step
  name:2          a second lane on the same binding
  name:x          a part of that lane, not the lane itself
  >name = note 36 what a name means - the names are yours
```

Thirty-six verbs, all of them on [VERBS.md](VERBS.md). The lane names are not
among them: they are definitions, and the boot document holds sixteen.

---

## 2. Who read it

Two reviewers, both told to find what is wrong and neither given permission to touch
the code. They were given the docs and the firmware and nothing else — no defence of
past decisions, no list of what had already been argued.

- **A performer.** Twenty years on stage, TidalCycles and Strudel. Judges by what can
  be typed in two seconds under pressure, read at a glance in one ink, and mistyped at
  2 a.m.
- **A language designer.** Scheme, APL, Forth. Judges by orthogonality — how few
  primitives generate how much behaviour — and has contempt for vocabulary that grows
  to cover a gap a better core would have closed.

They agreed, independently, on two things. Both are now fixed, and they are the only
things in this document that were acted on: **the picture sources were shapes where
they should have been fields**, and **the draw order was frozen in a table where it
should have been stated by `route`**. See §4.

---

## 3. What is wrong — and what has been decided

Ordered by how much damage each does. Each entry: the complaint, then the proposal,
then — where it has been acted on — **Decided**, with the date and what the deck
measured. The complaints are left as they were written; a finding that turned out
to be wrong is corrected underneath it, not erased.

**1. `,` is spent on a ghost note, and chords are therefore inexpressible.**
`,` is a hit at a third velocity. It is also, in every Tidal and Strudel document ever
written, the character for a chord — `[0,4,7]`. Two costs for one decision: `pad`
("voice, long") cannot play a triad, and at thirty columns `,` and `.` differ by one
or two lit pixels at the baseline while one is a note and the other is silence.
→ *`,` becomes stack. The ghost note goes to `o`, or goes away — see 5.*

**Decided 2026-09-25 — `,` is stack, and the ghost note went away.** `[0,4,7]` is a
chord: every member starts together and the playhead lights from its first note to
its last. A stack's members are sequences, so `[02,45]` moves inside its step. On
the deck, `>pad [0,2,4]...` in D minor played D3 F3 A3 on one timestamp. The ghost
did not move to `o`: a digit is velocity now (§3.7), so `3` is a ghost and `o`
would have been a second spelling of it. `,` outside brackets is refused with
"a chord goes in []".

**2. A pattern has no syntax errors.** Anything that is not a rest is a hit, so
`>hat x...x...x...x;..` plays the semicolon and the deck says nothing. Unbalanced
brackets are absorbed: `>hat [x.x.x.x.` compiles as one step subdivided seven ways —
eight steps typed, a nonsense septuplet played, no message. Meanwhile `>box[z]` is
loudly refused. Same language, opposite philosophies, and the loud half guards the
mistake nobody makes.
→ *Reject an unknown step character and an unbalanced bracket, the way overflow is
already rejected.*

**Decided 2026-09-25 — refused, with the character.** Every malformed pattern is now
refused in one line that fits the status bar, and the offending character is
**boxed in the document** (a bar above and below — nothing else on the panel draws
that) until the next edit. On the deck: `x...x...x;..` → *';' is not a step - x
hits*; `[x.x.` → *'[' is never closed*; `X...` → *X is gone: 9 is loud*. A refusal
changes nothing, so a typo mid-song leaves the lane playing what it played. The
characters people bring from elsewhere are told where they went — `?`, `-`, `~`,
`*`, `/`, `!`, `@` each have their own sentence.

It also found something. The shipped guide had been **playing its own comments**:
`>echo 8    then add:` compiled "then add:" as nine more hits, and the cheat-sheet
lines — `>sync on    MIDI clock out` — failed when run, because the comment became
part of the argument. `tools/test_ui_text.c` now compiles every lane line in the
guide against the shipping compiler and refuses a comment inside a command line.

**3. `[]` means two things.** It groups inside a pattern and it selects a part of a
name. The argument that moved probability off the bracket — *"a probability is a
property OF a step; a group is a step that CONTAINS steps"* — convicts this exactly:
`disc[x]` is a property of a lane, not a lane containing lanes. `disc2` is then a
*third* notation for naming a sub-part, and it silently reserves trailing digits in
every name for ever.
→ *One address grammar with the separator SUBSTRATE.md already defines: `disc:2:x`.
`[]` goes back to grouping alone.* The objection that `.` cannot separate because it
is a rest was over-applied — the pattern parser never sees the name, so the two
grammars are lexically disjoint.

**Decided 2026-09-25 — done as proposed.** `disc:2` is the second circle, `disc:x`
its position, `disc:2:x` the second one's; `disc:1` is `disc`, so one lane has one
spelling. `[]` only groups. The old spellings are told their new ones rather than
refused blankly — `>disc2` answers *disc2 is disc:2 now* — because the documents on
the owner's boards use them. One thing nobody had noticed: `[` and `]` are pattern
characters in an OSC address, so `disc[x]` had never been a valid `/deck/` path;
`:` is.

**4. `>disc 2` and `>disc2` are one space apart and one of them deletes a lane.**
A small circle, or the destruction of the second circle.
→ *Falls out of 3.*

**Decided with 3.** `>disc2` is not a name any more, so it cannot silently become
one: it says what it is now, and deletes nothing.

**5. Marks that are provably the same mark.** `?` is exactly `%50` — and `?%15` sets
the same bits as `x%15`, teaching a distinction that does not exist. `.` `-` `_` are
three spellings of one rest.
→ *Delete `?`. Keep one rest, and give `_` its universal tracker meaning — **tie** —
because nothing in this language can express note length. A performer reaches for
that inside the first hour.*

**Decided 2026-09-25 — done as proposed.** `?` is gone (`x%50`), `-` is gone, `.` is
the rest, and `_` holds the note before it. A tie adds the steps it spans to the
voice's own gate rather than replacing it, so a bass stays a bass: on the deck
`>bass 0__.` sounded 423 ms against a predicted 180 + 2 × 121. A tie after a chord
holds the chord; inside one member of a stack it holds only that member; a tie that
would hold a note on only some of the bars it plays — `0<_ .>` — is refused, because
one note has one length.

**6. `%NN` already broke the invariant used to refuse everything else.** Multi-step
proposals were refused on the grounds that one character per step is what keeps the
playhead honest. `x%15` is four characters for one step. The rule is broken, so the
refusals it justified are void.
→ *Decide it once: either steps may be multi-character — which buys chords, note
length and velocity together — or `%` goes. Half of each is the worst outcome and is
the current state.*

**Decided by the owner, 2026-09-25: a step is one character plus optional
modifiers, and the playhead lights the whole span.** The evidence it was decided
on: of the 258 mini-notation strings in Strudel's own example tunes, parsed by
Strudel itself, 42 % put a modifier on a step and 23 % stack a chord. `x%15` now
lights all four characters. The refusals the old rule justified are void, and the
one in [MAP.md](MAP.md) §9.3 is re-argued there.

**7. A digit means three things, and on a drum lane it means nothing at all.**
`>kick 0...9...` is two identical full-velocity hits: the degree is compiled and never
read. Velocity is a three-value enum spelled `x X ,` instead. In the pictures, "0
none, 9 full" fails in four places — `flip 0` is maximum, `spin 1` and `spin 2` do
nothing, `fold` maps ten digits onto three states.
→ *Make the digit the step's scalar on every binding, velocity included. `X` and `,`
then go.*

**Decided 2026-09-25 — done for velocity.** On a drum `9` is 127 and `1` is 14, and
`0` is the quietest hit rather than silence — a rest is `.`, and a digit is always
an event. `x` is the lane's own level, 100. Measured on the deck: `>kick 9...5...`
sent 127 and 71. On a voice the digit stays the degree; a voice's per-step velocity
will be a parameter lane, the way a circle's position is. The picture half — `flip 0`,
`spin 1`, `fold` — is untouched here and still open.

**8. The seventeen sound names are data; the sixteen picture names are code.**
`kick` is a row — `(note, channel 10, 36)`. `disc` is a function pointer. So a player
can add neither, and cannot add `conga` at all, which contradicts the promise that the
system is extensible without a compiler.
→ *Make a lane name an alias for (binding, parameters) and put the alias table in the
boot document as a region. Sixty-nine names become roughly thirty-eight, and
expressive power rises, because the vocabulary becomes editable text.* This is the
largest single change available and the one most likely to be right.

**Decided 2026-09-25 — done, as lines rather than a region.** A name is defined by a
line like any other: `>kick = note 36`, `>bass = voice 2 ch 1 gate 180`,
`>cut = cc 74`, `>circle = disc`. The boot document ships sixteen of them, and a
boot document written before this gets them added at its top, the owner's lines
kept below. A *region* — a `:::table` fenced block — was the proposal, and it was
not followed, for the reason the boot document itself gives: it is "a guide that
happens to run by itself — no config format, no parser, no second syntax". A
definition is a line; Ctrl+Enter on it takes effect at once; `>kick = note 35`
retunes the kick that is playing (verified on the deck: 36, then 35 on the next
hit). The picture names stay the primitives' own, since a primitive is code, and
an alias to one is allowed.

The count, by the snippet: **sixty-nine verbs became thirty-six**, with none added
— the proposal guessed thirty-eight. And it was not a vocabulary cut: a player can
now add a conga, point a spare controller at their synth (`>fx = cc 20`), and move
the kick to the note their drum machine wants, none of which was possible.

**9. `<>` has an invisible arithmetic cliff.** Alternation is flattened at compile
time, which costs no runtime state and costs `lcm` slots instead: a sixteen-step lane
with a two-way and a three-way alternation needs ninety-six slots and is refused at
sixty-four. One byte of cycle counter in the fire path makes it unbounded.
→ *Spend the byte.*

**Decided 2026-09-25 — and it cost nothing.** Alternation is no longer flattened.
Each note carries a cycle class — it plays when `cycle % per == ph` — and the cycle
comes from the global tick, so there is no counter at all. A long lane may alternate
(`x...` × 8 then `<3 5>` was refused at 66 slots and is 33 now), alternation costs no
slots, and nested alternation advances only when it is chosen: `<0 <1 2>>` plays
0 1 0 2, as Strudel does, where the flattening played 0 2 0 2. The byte bought
something else while it was being spent: the clock computes each slot's tick exactly,
so a quintuplet no longer drifts (see [MAP.md](MAP.md) §5).

**10. Direction is a parameter wearing four hit characters.** `u d l r` attach to a
line or to a step, with an unwritten shadowing rule between them. An axis and a sign
is a parameter lane, which would delete four letters from the hit alphabet and three
pieces of machinery.

**11. Degrees stop at 9.** About an octave and a half, no octave verb, no accidental
outside the scale, no reverse. All first-hour reaches.

**12. The instance rule was not scoped, and `cc` printed a name it refused.**
Both were bugs rather than design, and both are fixed — §4.

---

## 4. What was taken from this, and what was left

**Acted on**, because two independent readings reached it separately and because it
was a ceiling rather than a preference:

- The picture sources are **fields**, not shapes. `star` is gone — its amount was a
  count of spokes, the only amount in the language that was not a magnitude, and it
  composed with nothing. `turn` is the angle as a field. `mask` is a level through a
  field and `edge` is a contour of one, so a ring, an outline, a wedge, a contour map
  and a rotating radar sweep are reachable, and five of those six were not. `shake`
  and `tile` paid for them. Sixteen names before, sixteen after.
- **`route` states the draw order.** A routed lane draws after the lane it follows, so
  `thin` then `grow` and `grow` then `thin` are both expressible. Sixteen primitives in
  a frozen chain is not composition; it is a mixer with sixteen mute buttons.
- Two real defects: `>bpm140` silently reported the tempo it had not set (the instance
  rule was not scoped to lanes), and `>lanes` printed `cc20`, a name the language then
  refused.

**Left alone**, deliberately: everything in §3. Each item there changes what a
performance looks like on the glass, and the two decks are about to be played rather
than read. A syntax change made the week before first use is a syntax change nobody
has evidence for. The evidence is the point of the next fortnight.

**The one to do first** when that fortnight is over is §3.6 — decide whether a step
may be more than one character. Nine of the eleven other entries are downstream of
that answer, and it is currently answered both ways.

[NEXT.md](NEXT.md) is the development brief that acts on this document: the same items in
dependency order, plus the requirements that arrived after it was written — finite
repetition, wireless encoder satellites, the RP2040 visualization node, and why a Strudel
conformance corpus is worth building where a Strudel compiler is not.
