# One substrate

*The conceptual core. Everything in [OS.md](OS.md) is downstream of this.*

## The claim

**There is one data structure in this machine: a rectangle of characters.**

Not "the OS has a text editor". Not "files are text". One structure, and
everything is an instance of it — a note, an Orca patch, a menu, a routing
table, the screen itself, the thing under the cursor right now.

The whole pipeline is already this type and nothing else:

```
    keyboard  ─emits→  characters
    storage   ─holds→  characters
    display   ─shows→  characters
```

The keyboard produces cells. The panel is 66 × 25 cells and cannot draw
anything smaller, because at 1 bit per pixel there is no antialiasing, no
sub-pixel positioning, no greyscale — the cell is the atom whether you want it
to be or not. The hardware and the idea turn out to agree, which is usually a
sign the idea is the right one rather than a preference being imposed.

A machine with one type needs one editor, one undo, one search, one clipboard
and one navigation grammar. Not four of each, wearing different hats.

## One noun, three verbs

Everything reduces to a single primitive:

> **A region** — a rectangle of characters, in a buffer, with an address.

And a selection is a region. A patch is a region. A command is a region one
line tall. A note is a region the size of its buffer. A route is a region one
row tall. There is no second kind of thing.

Three verbs act on any region, and between them they are the entire system:

| Verb | Key | What it means | Is also |
|---|---|---|---|
| **Run** | `Enter` | interpret this region | executing a menu line; starting a patch |
| **Pipe** | `\|` | replace this region with a transform of it | sort; humanise; an assistant rewrite |
| **Plumb** | `Go` | do the obvious thing with this | open `note.md:27`; audition `C#4`; set `120bpm`; else search for it |

That is the whole interaction model. Everything else is which region you have
and what mode it is in.

## Two axes of meaning

This is the structural insight, and it is what lets prose and Orca be the same
object rather than two objects that happen to both be text.

A character grid can carry meaning on two independent axes:

- **Sequence** — order matters, position does not. Prose. Reflow is free;
  wrapping changes nothing. The grid is a convenience.
- **Position** — where a character sits *is* the meaning. Orca. Reflow is
  destruction; a character one column left is a different program. The grid is
  the substance.

Same array. Same bytes. They differ in exactly one bit of interpretation, and
that bit decides one thing: **does Enter reflow, or does it move?**

```
prose mode   Enter → split the line, everything below reflows
grid  mode   Enter → move the cursor down, nothing moves
```

Once that is settled, every other key is shared. `w` moves by word in both.
Search works in both. Undo works in both. The selection grammar works in both.
One editor, one keymap, one switch.

## Buffer kinds

A buffer is a character rectangle plus a kind. The kind is metadata, not a
conversion — **any buffer can change kind without its bytes moving.**

| Kind | Axis | Enter does | Example |
|---|---|---|---|
| `prose` | sequence | splits and reflows | a note, a lyric, a plan |
| `grid` | position | moves down | an Orca patch |
| `guide` | sequence | splits, like prose — see below | a menu you wrote by typing |
| `table` | both | next row | routing, parameter maps |

**One correction from building it.** This table originally had `guide` make
Enter *execute* the line. Implemented, that made a guide impossible to edit:
at the end of a command there was no way to add a line after it. Execution is
now marked in the text — a command line begins with `>` — and run with
Ctrl+Enter, in any buffer. The kind still decides reflow, which is what the
two-axes argument above is really about; it no longer decides execution. See
[COMMANDS.md](COMMANDS.md).

**A second correction, 2026-09-25: the kind decides nothing.** It is journalled with
every document and restored, and no code reads it — every buffer wraps and reflows
the same way. So `>guide` and `>prose`, which set it by hand, were deleted
([MAP.md](MAP.md) §0). The table above is the design; one kind is what shipped, and a
second one has to arrive with the behaviour that reads it.

`guide` is the one that does the most work, and it is stolen wholesale from
Plan 9's Acme. A guide is a plain buffer of command lines where Enter runs the
line under the cursor. That single rule means:

- **A menu is a text file.** Adding a menu item costs typing a line.
- **Documentation is executable.** The example in the manual is the button.
- **There is no separate command surface.** The buffer already is one.
- **The user writes their own interface.** Not "customises" — writes.

The discipline this demands is real and worth naming: **every action in the
system must be reachable by a name in a table**, never only by a keybinding.
That is a constraint on every feature ever added, and it is the price of the
whole idea. It is worth it, because a keybinding you cannot remember is a
feature you do not have, and a name you can type is a feature you can find.

## Embedding, and the trailing-whitespace problem

A note should be able to *contain* a patch that runs. This is org-babel's idea
and it is the right one, but there is a specific trap here that would silently
destroy work.

**Orca patches are raw rectangular ASCII in which trailing characters are
significant.** A patch is mostly dots. Any tool that strips trailing
whitespace — an editor, a formatter, a linter, a git hook, a laptop — corrupts
the program without reporting anything, and the failure shows up later as a
patch that no longer runs.

Preserving trailing whitespace is not a solution, because it requires every
tool in the chain to cooperate and they will not.

**So do not preserve it. Reconstruct it.** The fence declares the rectangle's
extent, and the reader pads every row to that width:

```
:::orca 24x9 name=bass
..D4....:14C............
........................
:::
```

Read: pad every row to 24, accept short rows. Write: emit exactly 24.

Now trailing whitespace *cannot* be lost, because it is never load-bearing —
the width is. The file survives being edited on a laptop, passed through git,
opened in an editor that trims, and mailed to someone. A destructive, silent,
very likely failure becomes structurally impossible rather than merely
discouraged.

The same fence carries anything else rectangular, and the `name=` and other
keys are how a region gets addressed from elsewhere in the document.

## The document is the piece

Here is the payoff, and it is the reason this is an *idea machine* rather than
a sequencer with a notepad attached. A piece is **one file**: the thinking, the
patches, the routing and the measurements, in the order a person actually
works.

```
# Rust Belt Lullaby
bpm 96   key Dmin

A slow thing. The bass should feel late — always behind
the beat, never on it. The hats are the only thing in time.

:::orca 24x9 name=bass
..D8....:14D............
........................
:::

:::orca 24x5 name=hats
..D2..:19F..............
........................
:::

:::table route
patch  dest      chan  offset
bass   din       1     +0ms
hats   usb       2     -8ms
:::

The -8ms is because the laptop is about 8 ms behind the
DIN synth. Measured with a loopback, not guessed. If the
interface changes, re-measure before blaming the patch.
```

Open it and the machine *is* that piece. The prose is not documentation about
the work; it is in the same object as the work, at the same altitude, editable
with the same keys. The note that says *"the bass should feel late"* sits six
lines above the patch that makes it late and the table that says by how much.

Three years later that file still opens, on anything, and still says why.

## Addressing

If everything is a region, everything needs an address, and the addresses
should be things a person would type anyway:

| Address | Means |
|---|---|
| `lullaby.md` | a document |
| `lullaby.md:27` | a line |
| `lullaby.md#bass` | a named region |
| `din:1` | a MIDI destination |
| `C#4` | a note |
| `120bpm` | a tempo |

The **plumber** is one key that looks at the selected text, matches it against
a user-editable rules file, and does the obvious thing — and its most important
rule is the last one: **a bare word degrades to a literal search.** It never
fails, never reports "unrecognised", never makes the user guess a syntax. Worst
case it shows you where else that word appears, which is rarely useless.

Because the rules are a text file, the owner extends the system without a
firmware rebuild. That is the same move as the guide file, applied to
selection instead of to lines.

## The screen is the page is the patch

66 × 25 is the display. It is also the natural size of a patch, and of a page.

That is not a coincidence to be worked around, it is a property to lean on.
Orca's own desktop default is smaller than this, and every other small-screen
Orca port needs a scrolling viewport. **Here the whole program fits.** What you
see is not a window onto the work; it is the work.

A machine where the unit of display, the unit of storage and the unit of
thought are the same size does not need a concept of "view".

## What this buys

- **One editor to learn.** Not four, not four sets of shortcuts.
- **One undo, one search, one clipboard**, across prose, patches and routing.
- **Everything is greppable, diffable and syncable** — by any tool, forever.
- **It survives the device.** Files open on a laptop. If this machine is lost
  or a board revision changes everything, the work is untouched. Nothing is
  trapped in a binary format only this firmware understands.
- **The user can extend it without a compiler**, because menus and rules are
  text they already know how to edit.
- **It is cheap.** One renderer, one buffer type, one damage model. On a
  512 KB part that is not an aesthetic argument, it is why it fits.

## What it costs

Stated honestly, because a design that only lists its advantages is a sales
pitch.

- **Text is a poor representation for continuous things.** Automation curves,
  waveforms, envelopes. Those want a different substrate and will be awkward
  here, or will live as numbers that are harder to feel than a drawn curve.
- **Two editing semantics in one editor** is a real complexity, and the mode
  indicator had better always be visible, because typing prose into a grid
  silently makes a different program.
- **Fixed-extent grids need explicit resize**, which is a concept prose does
  not have and a place where the illusion of one editor leaks.
- **Discoverability rests entirely on the palette and the guide files.** If
  those are weak the whole system is unusable, because there are no menus to
  fall back on. This is the single largest risk in the design.
- **The "name in a table" discipline taxes every future feature.** It will be
  tempting to add something reachable only by a chord. Doing that once starts
  the rot.

## Open questions

| # | Question |
|---|---|
| 1 | Does a `grid` region inside a `prose` buffer edit in place, or open as its own buffer? In-place is more honest to the idea; its own buffer is almost certainly easier to implement and to use. |
| 2 | What is the minimum viable `table` kind? It may just be `guide` with columns, in which case there are three kinds, not four. |
| 3 | How are multiple patches in one document scheduled — all running, or one active? This is a musical question as much as a technical one. |
| 4 | Retrieval at scale. Thousands of files, no mouse: frecency plus full-text plus the palette is the cheap answer, but it is unproven at that size on this hardware. |
| 5 | Does the fence syntax survive round-tripping through the tools the owner actually uses? Worth testing early with a real file and a real laptop. |
