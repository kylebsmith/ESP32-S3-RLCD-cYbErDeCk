# The syntax, on one page — and two reviews that tried to break it

*The language first, in as few words as it can be said. Then what is wrong with it,
from two adversarial readings that were told to be harsh and given no power to change
anything. **Nothing in §3 is implemented.** It is a list of what to do next and why,
kept in one place so the next decision is made against the whole of it rather than
against whichever complaint is loudest that day.*

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

**One character is one step.** The playhead can sit on the character that is
sounding. This is the constraint that keeps a pattern readable at thirty columns on a
screen with one ink and no backlight, and it is the constraint most of §3 is about.

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

**Refuse, do not truncate.** A pattern too long to hold is rejected with a number.
Silence is the one failure a performer cannot debug.

### The whole of it

```
  x  hit          X  loud         ,  quiet        ?  maybe (half)
  .  rest         -  rest         _  rest
  0-9             a scale degree, a controller value, or an amount
  x%15            fifteen per cent chance on that step
  [xx]            a group: subdivides the step it occupies, any depth
  <a b>           alternates: a different member each bar
  /2  *2          this lane's own rate, at the end of the line
  u d l r         which way — in front of the pattern, or as a step
  name2           a second lane on the same binding
  name[x]         a parameter of that lane, not the lane itself
```

Sixty-nine verbs, all of them on [VERBS.md](VERBS.md).

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

## 3. What is wrong — not implemented, on purpose

Ordered by how much damage each does. Each entry: the complaint, then the proposal.

**1. `,` is spent on a ghost note, and chords are therefore inexpressible.**
`,` is a hit at a third velocity. It is also, in every Tidal and Strudel document ever
written, the character for a chord — `[0,4,7]`. Two costs for one decision: `pad`
("voice, long") cannot play a triad, and at thirty columns `,` and `.` differ by one
or two lit pixels at the baseline while one is a note and the other is silence.
→ *`,` becomes stack. The ghost note goes to `o`, or goes away — see 5.*

**2. A pattern has no syntax errors.** Anything that is not a rest is a hit, so
`>hat x...x...x...x;..` plays the semicolon and the deck says nothing. Unbalanced
brackets are absorbed: `>hat [x.x.x.x.` compiles as one step subdivided seven ways —
eight steps typed, a nonsense septuplet played, no message. Meanwhile `>box[z]` is
loudly refused. Same language, opposite philosophies, and the loud half guards the
mistake nobody makes.
→ *Reject an unknown step character and an unbalanced bracket, the way overflow is
already rejected.*

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

**4. `>disc 2` and `>disc2` are one space apart and one of them deletes a lane.**
A small circle, or the destruction of the second circle.
→ *Falls out of 3.*

**5. Marks that are provably the same mark.** `?` is exactly `%50` — and `?%15` sets
the same bits as `x%15`, teaching a distinction that does not exist. `.` `-` `_` are
three spellings of one rest.
→ *Delete `?`. Keep one rest, and give `_` its universal tracker meaning — **tie** —
because nothing in this language can express note length. A performer reaches for
that inside the first hour.*

**6. `%NN` already broke the invariant used to refuse everything else.** Multi-step
proposals were refused on the grounds that one character per step is what keeps the
playhead honest. `x%15` is four characters for one step. The rule is broken, so the
refusals it justified are void.
→ *Decide it once: either steps may be multi-character — which buys chords, note
length and velocity together — or `%` goes. Half of each is the worst outcome and is
the current state.*

**7. A digit means three things, and on a drum lane it means nothing at all.**
`>kick 0...9...` is two identical full-velocity hits: the degree is compiled and never
read. Velocity is a three-value enum spelled `x X ,` instead. In the pictures, "0
none, 9 full" fails in four places — `flip 0` is maximum, `spin 1` and `spin 2` do
nothing, `fold` maps ten digits onto three states.
→ *Make the digit the step's scalar on every binding, velocity included. `X` and `,`
then go.*

**8. The seventeen sound names are data; the sixteen picture names are code.**
`kick` is a row — `(note, channel 10, 36)`. `disc` is a function pointer. So a player
can add neither, and cannot add `conga` at all, which contradicts the promise that the
system is extensible without a compiler.
→ *Make a lane name an alias for (binding, parameters) and put the alias table in the
boot document as a region. Sixty-nine names become roughly thirty-eight, and
expressive power rises, because the vocabulary becomes editable text.* This is the
largest single change available and the one most likely to be right.

**9. `<>` has an invisible arithmetic cliff.** Alternation is flattened at compile
time, which costs no runtime state and costs `lcm` slots instead: a sixteen-step lane
with a two-way and a three-way alternation needs ninety-six slots and is refused at
sixty-four. One byte of cycle counter in the fire path makes it unbounded.
→ *Spend the byte.*

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
