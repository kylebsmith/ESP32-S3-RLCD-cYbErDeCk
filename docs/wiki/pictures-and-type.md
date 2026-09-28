# Pictures and type — the redesign, discussed

*Part of [the deck, top to bottom](README.md). 2026-09-28. A discussion for the owner, not a
build. Every picture on this page was drawn by the deck's own picture engine
(`firmware/components/viz/viz.c`); only the proposals are mocked, and each image says
which part. `tools/mock_pictures.py` redraws them all.*

The brief: get creative with the pictures and the letters, without giving up any speed,
because the clock and the control come first.

## The rule that frees the design

**At one bit on a fixed grid, how a glyph looks costs nothing to run.**

- **Drawing a cell** copies 36 bytes from a cache of pre-turned glyphs — 3.8 µs, whatever
  the bits are ([editor.md](editor.md) §2.4).
- **Pushing to the panel** costs the area that changed, not what is drawn in it.

So a redrawn letter or tile costs zero microseconds. What does cost is:

| | the cost |
|---|---|
| **how many glyphs** | memory: 36 bytes each in the cache; all 124 today are 4,464 bytes |
| **how big the grid** | work per frame, per cell or dot |
| **any per-pixel path** | work per pixel, which the grid exists to avoid |
| **flash writes** | never, while playing |

Be as inventive with the bitmaps as you like, and careful with the grid. One line to keep
it straight: **Swiss in the letter, punk in the layout.** Legibility is measured letter by
letter; the expression lives in how type is placed, scaled, inverted, layered and turned
into picture.

## 1. What the reference found

- **In the default face, the picture pane is 28 × 4 cells** — 336 × 96 pixels
  ([pictures.md](pictures.md) §1.7). Four rows of 24 pixels cannot hold a round shape: a
  disc is drawn as a rectangle, and a ring as a box.
- **The picture's resolution is tied to the text's size.** The compact face gives a
  58 × 10 pane. So choosing letters you can read costs the pictures most of their
  resolution.
- **Twelve of the twenty-eight tiles are never drawn by anything**: 141–146 and 148–155
  — the halves, square, diamond, ring, diagonals and arcs. The engine writes only the nine
  tones, the four sparkles and the small disc (147).
- **The round tiles are tall ovals**, and the tile generator's comment says the arithmetic
  makes them round (`tools/font_tiles.py`, `_dot`). It halves the vertical distance, which
  on square pixels makes an oval twice as tall as it is wide.

## 2. Proposal A — the pictures get their own pixels: square 4 × 4 dots

![Today's default pane against square dots](../img/pictures-dots.png)

*The same 336 × 96 pixels, the same engine, the same seven scenes: a disc, a masked disc, a
ring, a box, a turn, the radar and the orbit from `orbitals`. Left, today. Right, the
engine built with square dots — a copy of `viz.c` with three edits, listed in
`tools/mock_pictures.py`.*

**What.** The picture engine draws on its own grid of 4 × 4-pixel square dots, whatever
face the text is in:

| | today | with 4 × 4 dots |
|---|---|---|
| the default pane | 28 × 4 cells | **84 × 24 dots** |
| the largest compact pane | 58 × 17 cells | 87 × 51 dots |
| the whole text area | — | 90 × 72 dots |

**Why 4 × 4:**

- **A dot is one period of the 4 × 4 ordered dither**, so the nine tones stay the nine
  tones: a field of one tone is seamless across dots, as it is across tiles now.
- **A dot is exactly two framebuffer bytes, in every orientation**, because every cell
  already is whole bytes. So drawing a dot is two stores from a table of nine — no
  per-pixel work at all.
- **A dot is square, so a circle is a circle.** The engine stops counting vertical
  distance twice.
- **The dots line up with the 12 × 24 cells**, three by six to a cell. In the compact face
  the pane starts two pixels off the dot grid, and gives them up.

**What it costs** — estimates, to be measured on the deck before anything is believed:

| | cost |
|---|---|
| engine | scales with the dots: 2,016 in the default pane against today's largest frame of 1,440 cells — about 1.4 × the worst case now, roughly **1 ms a frame** from the measured 0.6 ms for seven lanes over 1,060 cells |
| drawing | about 4,000 byte stores a frame — tens of microseconds |
| pushing | **unchanged**: the pane's area |
| the view node | 11 + 2,016 bytes a frame, against 1,071 today; the node draws dots instead of glyphs, which is simpler |
| the clock | nothing: all of it is on core 0, at a frame a step |

**What changes, and what does not.**

- **The engine:** vertical distance counts once instead of twice, in three places; the
  frame may be larger; the pane is drawn as dots rather than as glyph codes.
- **The language does not change:** `>disc 8` is still `>disc 8`.

**The dither stays on the panel, not on the shape.** When `move` or `echo` shifts a shape,
the tone pattern does not travel with it; it is applied where the dot lands. That is the
rule Panic gives for its one-bit panel, where a pattern moved one pixel flickers
([research.md](research.md) §3).

**What it gives up: the picture is no longer made of characters.** Three answers, which can
all be true:

- **A grain.** The same frame, read three by six dots to a cell, becomes today's tone
  tiles again. So "cells" can stay a look you choose, not a limit you live with.
- **Octants, if the picture must stay characters.** Divide each 12 × 24 cell into 2 × 4
  blocks of 6 × 6 pixels — the grid of Unicode's octants, and teletext's mosaics before
  them. The picture is then still a grid of characters, and the view node still receives
  characters. It is coarser (56 × 16 in the default pane, the middle column above), and 256
  block patterns do not fit the free codes 156–255 beside the letters.
- **The letters come back into the picture** — Proposal C.

![The largest picture today against 4 x 4 dots](../img/pictures-panel.png)

*The largest picture the editor gives today, the compact face split to 17 rows, against 4 ×
4 dots on the same pixels. At this size the difference is smaller. The default pane
above is where it matters.*

## 3. Proposal B, tried and set aside — smoothing the cells

![Smoothing today's cells](../img/pictures-smoothing.png)

This draws each cell from the tones at its four corners, interpolated, then either cut
into the nine tones or cut once at half. It rounds edges, but it cannot add rows a
four-row frame does not have: the ring disappears, and the orbit smears. It is kept here
so nobody tries it twice.

## 4. Proposal C — type as a picture: `stamp`

![Type as picture, on 4 x 4 dots](../img/pictures-stamp.png)

*Mocked: the discs and ring are the engine's, on square dots; the type is the deck's own
12 × 24 and 6 × 12 faces, one font pixel to one dot, laid in by the mock-up tool.*

**What.** A seventeenth primitive, `stamp`, that writes the deck's own letters into the
picture. In the default pane the 12 × 24 face, at one pixel a dot, is exactly 24 dots tall:
**a word fills the pane.**

**Its text is the name of whatever fired it.** `>route stamp kick` puts the word `kick` on
the screen with every kick, at the kick's velocity. The picture says what played — the
thesis made literal: the audience sees cause and effect without being asked to read code
([THESIS.md](../THESIS.md)). An alternative is the line's own pattern, so the rhythm
becomes type.

**What it costs:** one store for every inked dot, and no field to compute — the cheapest
primitive there could be.

**What it gives:** it composes with everything already there.

- `echo` leaves a trail of words;
- `move` drifts them;
- `mask` and `flip` knock them out of a field;
- `fold` mirrors them.

**Precedent:** in ORCA, the letters on the grid are both the program and the picture
([GRAPHICS.md](../GRAPHICS.md) §8).

## 5. The tiles, after the dots

| tiles | today | after |
|---|---|---|
| **tones** 128–136 | the picture's greys | the dots' greys: unchanged |
| **sparkles** 137–140 | `noise` | drawn as dot patterns |
| **small disc** 147 | `disc` at a radius of one cell | a disc one dot wide needs no glyph |
| **the other twelve** 141–146, 148–155 | never drawn | leave the picture engine; become symbols for the text, or go |

The twelve could become symbols the screen lacks. The pane's border is drawn with `+ - |`
today and could get real line-drawing corners. The status bar could get marks for the
battery, the radio and MIDI going out. Any round ones should be drawn round in pixels:
a bullet in a tall cell, not an oval.

## 6. The letters — what to redraw, and how to know

**What the 12 × 24 face already gets right.** Printed from the font itself:

```
  0           O           1           l           I           5           S           8           B
  ..######..  ..######..  ....##....  ....##....  .########.  ##########  ..######..  ..######..  ########..
  ##......##  ##......##  ..####....  ....##....  ....##....  ##........  ##......##  ##......##  ##......##
  ##..##..##  ##......##  ....##....  ....##....  ....##....  #########.  .########.  .########.  #########.
  ##......##  ##......##  ....##....  ....##....  ....##....  ##......##  ##......##  ##......##  ##......##
  ..######..  ..######..  .########.  ....######  .########.  ..######..  ..######..  ..######..  ########..
```

(Rows 4, 6, 11, 16 and 19 of 24, the ten body columns.) A dotted zero; a one with a flag and a foot; an `l` with a tail;
an `I` with bars; a flat-topped 5 beside a round S; an 8 beside a flat-backed B. And
because the face is monospace with a 10-pixel body, `m` fits one cell and `rn` takes two:
the classic confusions are answered by design.

**What the research says about this face** ([research.md](research.md) §3):

- **Keep the 2-pixel stem.** On a 16-pixel cap it is 1/8 of the height — the heaviest
  whole pixel inside the 1/12–1/6 band the FAA sets for screens, where heavier is
  preferred on dark-on-light displays like this one. Playdate's guide, for the nearest
  panel there is, asks for at least 2 pixels. Bolder still would read worse: weight helps
  small letters only up to a point.
- **The size is right.** The cap is 3.39 mm — about 23 arcminutes at 50 cm, the FAA's
  preferred 22–24.
- **Make the zero narrower than the O.** Today the 0 and the O share one outline and
  differ only by the dot inside (above). On a dot-matrix display a round zero with a mark
  inside was misread as O; a narrow plain zero halved the errors (Vartabedian 1969).
  Readers agreed most on a zero narrower than O (Wendt 1969). The FAA's labelling rules
  say the same. **This is the one change the evidence asks for directly**: an 8-pixel
  zero, centred, with the dot kept or dropped by the reading test.
- **Treat the 6 × 12 as a label face, not a code face.** Its 7-pixel cap is about 10
  arcminutes, the FAA's floor for non-critical text, and its 1-pixel stem is thinner than
  Playdate's minimum. Terminus, a bitmap face that ships exactly these two sizes, carries
  its author's advice to avoid its own 6 × 12. A 2-pixel stem would not rescue it: on a
  7-pixel cap that is over-bold, which also reads worse.
- **Proof in confusion sets, not glyph by glyph**: 0/O/D/Q, 1/l/I/|, 5/S, 8/B, 2/Z, 6/8/9,
  7/1, rn/m. The worst pairs depend on the design, not on the letters (Maddox et al. 1977).
  Short strings like `x...9..x` and `kick` are where one bad pair shows.

**So the redesign starts with evidence, not taste.** [CMF.md](../CMF.md) sets out the reading
test:

- random strings, read off the panel at arm's length, in daylight and indoors;
- scored as errors per hundred characters, glyph by glyph;
- the errors form a confusion table: which glyph is read as which.

Redraw only the glyphs that the table shows people mistaking, starting with the zero. A face
that looks better and reads worse does not ship.

**The precedents for type drawn on a grid.**

- Crouwel's New Alphabet (1967) drew letters for cathode-ray screens on a 5 × 9 grid, with
  45° corners.
- Licko drew Emigre's first faces for the Macintosh's low resolution, and said "you read
  best what you read most".
- Kare drew the Macintosh's first bitmap faces.
- Gerstner (*Designing Programmes*, 1964) and Müller-Brockmann (*Grid Systems*, 1981)
  made the grid the design.
- Maeda's *Design By Numbers* (1999), from which Processing grew, made a small program the
  picture.

The deck stands in that line: a face drawn for its panel, pixel by pixel, and a grid that is
both the text and the picture.

**What is free to explore, because it costs nothing to run:**

- **Hierarchy by scale.** A document's section lines drawn twice the size, as 24 × 48 —
  the 12 × 24 face doubled, 144 bytes a glyph in the cache. Swiss typography builds
  hierarchy from size and weight contrast, not from new faces.
- **Glyphs tuned for what the language types most.** In a pattern line those are `.`, `x`,
  the digits, `[` `]` `<` `>` `%` `!` `/` `*` `_`. Could a rest read quieter and a hit
  louder? The same code draws prose too, so any change is tested on both.
- **The compact face as a face of its own**, not the small copy of the large one. It lost
  the default for being thin; for `+out`, `>lanes` and `>help` it could carry two-pixel
  verticals wherever five columns allow.
- **One curve.** The enclosure's corner is a superellipse, n = 3.2 ([CMF.md](../CMF.md)).
  The same curve could round the letters' bowls and the picture's shapes. Honestly: at
  12 × 24 a bowl's corner is two or three pixels, so the difference is a pixel. It is a
  principle, not something anyone will see.

## 7. Around the table — what the best of the others would bring

The owner's picture: the designers of the systems that matter, around one table, each
laying down the one thing their system does best. Each idea below was checked against the
system's own documentation or its designers' papers, 2026-09-28 (the sources are in
[research.md](research.md) §4). The right-hand column says what it becomes on the deck.

| seat | the idea it does best | on the deck |
|---|---|---|
| **Max** (Puckette) | a deterministic scheduler in logical time; the patch on screen documents everything that happens | **have it:** the clock owns a core, and the document is the whole score. Presentation mode — a performance face separate from the working one — is what the view node could become |
| **Pure Data** | each message cascade completes at one logical time and is never reordered; abstractions take `$1` arguments | **have it:** routes fire in rank order on one tick. Definitions are abstractions *without* arguments; with them, `>ring = disc edge` would be a composition — a question for the user tests |
| **SuperCollider** | language and synth are separate processes; events travel as time-stamped bundles sent ahead | **the same split, the other trade:** the deck's two cores divide the same way, but nothing is sent ahead — an edit lands on the next step |
| **ChucK** | time is a value; it stands still until the code advances it | **have it, declared:** the step is the unit of time, and a pattern *is* time |
| **Sonic Pi** | sleep advances virtual time, so loops do not drift; editing a `live_loop` updates it without missing a beat; built for classrooms | **have it:** an edit lands on the next step; refusals say why; the zine teaches. **Adopt:** its help beside every word — Tab completion shows each word's help ([completion.md](completion.md)) |
| **Impromptu / Extempore** | temporal recursion: a function schedules its own next call, and re-evaluating it hot-swaps the loop | **have it, declared:** counts and cues are temporal recursion without the function |
| **ixi lang** | a one-line score for each agent, rewritten on screen as the music changes; built so a tune appears in seconds | **have it:** one line is one lane, and the playhead shows the score moving. **Heed:** Magnusson's own survey found that constraints this tight suit sketching and improvising, but can limit in the long run. Depth has to come from composition — routes, definitions, other devices — not from more syntax |
| **TidalCycles** | a whole cycle in one quoted string; `(3,8)` for Euclid | **have it:** the notation, 51 of 51 corpus patterns. **The gap:** Euclid ([next.md](next.md) §3) |
| **Elektron** | per-step parameter locks; trig conditions — A:B, FILL, probability | **have it:** part lanes are locks written as lanes, and `%` is probability. **Idea:** a FILL held on a pad |
| **Ableton Live** | clips launched on a quantized grid; Follow Actions choose what plays next | **have it:** a count waits for its downbeat, and `name:end` routes are follow actions |
| **monome** | the grid does nothing by default — the software defines it; its lights are independent of its keys | **have it:** a knob is a name the deck defines. **Adopt:** make a satellite a *destination* too, so lanes can light its LEDs |
| **Bret Victor** | an immediate connection between a change and its effect — which he himself calls a prerequisite, not the point | **have it:** Ctrl+Enter lands on the next step, and the playhead shows it |
| **ORCA** | the grid is the program *and* the picture | **adopt:** `stamp` (§4) |
| **Playdate, teletext** | a one-bit panel where only changed lines are sent and grey is ordered dither; the character set as the graphics set | **adopt:** 4 × 4 dots (§2) |

**What the table would tell the deck to do next**, in its own order:

1. **Square dots** — Playdate's economics on this panel. Adopt on a number.
2. **`stamp`** — ORCA's idea, and the thesis made visible.
3. **Help at the cursor** — Sonic Pi's, through Tab completion.
4. **Euclid** — Tidal's missing mark.
5. **Satellites that listen** — monome's decoupled lights, as a destination.
6. **Definitions with arguments** — Pd's abstractions. Only if the user tests ask; it is
   exactly the kind of depth ixi lang's author warns about.

## 8. The owner's decisions

1. **Dots, octants or cells** — 4 × 4 dots (recommended, and cells kept as a grain), or
   octants if the picture must stay made of characters.
2. **`stamp`'s text** — the name of what fired it (recommended), or the line's pattern.
3. **The twelve unused tiles** — become symbols, or go.
4. **The letters** — draw the narrower zero now, then run the reading test and redraw only
   what it shows people mistaking.
5. **Section lines at twice the size** — yes or no.
