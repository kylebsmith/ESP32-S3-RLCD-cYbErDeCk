# The thesis, as it is being built

*[NEXT.md](NEXT.md) §12 asks for a running note: every time a decision is made for
legibility rather than for power, write it down, because that is the paper writing
itself. This is that note. It records decisions and where they are argued; it does
not argue them again.*

---

## The claim

A laptop performer projects their code to prove they are doing anything — the
overhead camera of live coding. The owner's diagnosis is that this answers a lack of
causal understanding in the audience. The claim this device tests is that **a
handheld deck makes agency visible without projection**: the gesture and its result
are in the same place, so the audience gets causality for free instead of being
asked to read code.

That makes two audiences for every decision: the **performer**, reading a 30-column
reflective screen at arm's length, and the **room**, which sees the deck, the hands,
and whatever the deck chooses to put on a bigger screen. A decision is *for
legibility* when it changes what one of them can tell apart.

## The running note

### 2026-09-25

**The playhead lights the whole step.** A step is one character and its modifiers,
and the mark that shows which step is sounding covers all of it — so the running
position is visible *in the text the performer wrote*, not in a separate display.
The owner's decision; [MANIFESTO.md](MANIFESTO.md) §3, entry 6.

**A chord lights from its first note to its last.** `,` stopped being a ghost note
and became a chord, and the mark follows. Entry 1.

**One mark for each thing.** `?`, `-`, `X` and the ghost `,` are gone; `.` is the
only rest; a digit is how much. At thirty columns `,` and `.` differ by a pixel or two
at the baseline, and one of them was a note — indistinguishable to a performer at arm's
length and to anyone reading over a shoulder. Entries 1, 5 and 7.

**A mistake is shown to the performer and never played to the room.** A refused
line names its character and boxes it in the document, and a refusal changes nothing:
a typo mid-song leaves the lane playing what it played. Entry 2.

**The names are the performer's.** `>kick = note 36`: the page reads as music —
kick, bass, cut — not as channels and controller numbers, so a line can be followed
by someone who has never seen the language. Entry 8.

**A routed lane plays at its source's level.** `>route cut kick` moves the filter
*with* the kick's dynamics, so the relation is heard, not only declared. Fixed
because routed lanes had ignored the source's amount; [MAP.md](MAP.md) §9.7.

**The projected picture is the deck's own, bit for bit.** The HDMI node draws the
deck's frames in the deck's glyphs, generated from the same art as the panel
([VIEW.md](VIEW.md)). What the room sees is the instrument's output, not a rendering
of the performer's code — the opposite of the overhead camera. Light ink on black,
kept after the owner's first look: "an inverted version of the display."

**Nothing secret is ever on the screen.** A password is asked for on the status line
and shown as stars, and never becomes a line ([NETWORK.md](NETWORK.md), *SSH, as
built*). On an instrument whose screen is part of the performance, a secret on a line
is a secret shown to the room — the flip side of making everything legible.

**The picture drops a frame before the editor drops a keystroke.** When the view's
output cannot keep up, the frame is dropped whole ([VIEW.md](VIEW.md)). The performer's
hands come first; the room loses a frame of a picture, never a gesture.

**A deck that joins waits to hear where the others are.** Two decks started by two
players now share the step and the bar, not only the tempo, and a following deck is
silent until it knows the leader's count, so its first note lands on the leader's
step. A join is heard as a second player coming in, not as a flam
([NETWORK.md](NETWORK.md), *And the count*).

**A knob is a name on the page.** An input — a phone's fader today, a satellite's
encoder later — is defined like a drum, `>knob1 = knob`, and connected by the same
word, `>route cut knob1`. Whoever reads the screen reads what the gesture controls,
in the performer's own words, rather than a mapping hidden in a controller's
settings ([MAP.md](MAP.md) §9.8). A pad lands on the step, so a press is heard in
time, not when its packet arrived.

### For power, not legibility — listed so the line stays honest

**The pre-turned face** made a cell 22 times cheaper to draw
([HARDWARE.md](HARDWARE.md), *The drawing was the budget*). It changed nothing anyone
can see: the framebuffer is byte-identical, which is what its check proves.

### Before this note began

**Three marks, three treatments.** The solid block belongs to the cursor and nothing
else; the playhead is a bar under the cell; a recognised command word is a bar over
it. They compose, so a cursor on the playhead shows both. Made from the owner's
reports of losing the cursor while a lane ran —
`firmware/components/textgrid/include/textgrid.h`.
