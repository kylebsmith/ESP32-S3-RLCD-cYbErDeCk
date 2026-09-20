# Commands, buffers and the archive

*How the deck is driven. Downstream of [SUBSTRATE.md](SUBSTRATE.md), which
argues that there is one data structure; this is what acts on it.*

Claims are tagged `[FACT]` (verified on the hardware), `[JUDGEMENT]` (a
decision, with its reason) or `[OPEN]`.

## The calling convention `[JUDGEMENT]`

Fixed deliberately and early, because it is the one part of this design that
genuinely cannot be retrofitted. Adding quoting or flags later would change
the meaning of guide files already written, and **guide files are user data** —
that is the migration that hurts.

```
  the command      the first word of the line
  the argument     the rest of the line, ONE unparsed string
  the input        the selection, implicitly
  the return       DONE, or PENDING with a job id
```

A command that wants structure parses its own argument. There is no argv, no
flag grammar, no quoting. `name Rust Belt Lullaby` files a document under a
name with spaces in it and needs no escaping to do so.

**`PENDING` exists before anything uses it.** If commands were assumed
synchronous now, the first network command would be written blocking and the
run loop would have to be torn up to fix it. Declaring the return type up front
costs one enum today and keeps SSH, an LLM call and a long export off the
main loop later.

## Capabilities `[JUDGEMENT]`

Every command declares what it touches; every caller declares who it is.

| Capability | Means |
|---|---|
| `READ` | inspects state, changes nothing |
| `EDIT` | mutates a buffer |
| `STORE` | writes flash or the card |
| `NET` | reaches off the device |
| `SYSTEM` | changes device state: pairing, power, orientation |

| Caller | May reach |
|---|---|
| `HANDS` | everything — the owner is holding it |
| `GUIDE` | READ, EDIT, STORE, NET — the owner, one step removed |
| `AGENT` | READ, EDIT, STORE |

The reason is the assistant. `docs/OS.md` demotes an LLM to "a filter in the
table" — but a filter with access to the command table would otherwise hold
**exactly the authority of the owner's hands**, including forgetting keyboard
bonds and re-pointing the radio. Tagging the caller is what makes an on-device
agent safe to add rather than something that has to be argued about later. It
also un-forecloses it: without this the honest answer to "can the agent run
commands?" is no.

Tightening the table is easy. Loosening it is a decision somebody has to make
on purpose, which is the point.

## Buffer kinds: one bit of interpretation `[FACT]`

`SUBSTRATE.md` says the kind of a buffer decides exactly one thing — what
Enter does. That is implemented literally:

**Superseded, and recorded rather than rewritten.** The kind used to decide
what Enter did: in a `guide`, Enter ran the line. That shipped, and it made
the guide uneditable — standing at the end of a command there was no way to
add a line after it, because the key that adds lines was busy running things.
An editor whose Enter key sometimes does not insert a line is not an editor.

The rule now:

| Key | Does | Where |
|---|---|---|
| `Enter` | inserts a newline | everywhere, always |
| `Ctrl+Enter` | runs the line, if it begins with `>` | everywhere |

The **sigil** replaced the kind for this purpose. Once a command is marked in
the text, the machine can tell a command from prose by reading, and does not
need a mode; two mechanisms for one distinction was one too many.

The kind survives because it still has to decide prose-versus-grid reflow for
Orca patches, which is the distinction `SUBSTRATE.md` actually cares about —
does Enter reflow, or does it move? That question is still live. "Does Enter
execute?" is not.

## The guide is a text file `[FACT]`

A `guide` buffer is a plain document where Enter executes the line under the
cursor. There is no separate command surface, because the buffer already is
one. Consequences, all of them the point:

- **A menu is a text file.** Adding a menu item costs typing a line.
- **The owner writes their own interface**, not "customises" it.
- **Documentation is executable** — the example in the manual is the button.

The deck writes a starter guide on first boot, because a device whose commands
are undiscoverable has, in practice, no commands.

**`Ctrl-G` jumps to the guide, and that binding is not a violation of the
name-in-a-table discipline — it is what makes it possible.** Everything must
be reachable by name, but *reaching the place where names are typed* cannot
itself require typing a name. That circle has to be broken by a gesture.

## The save model: scratch by default, save promotes `[JUDGEMENT]`

Decided by the owner, and it is the right way round.

**Every buffer is journalled and crash-safe from the first keystroke, named or
not.** Naming a buffer is what files it in the archive; it is *not* what makes
it durable.

So:

- "I did not want to save this" never costs data.
- The archive only ever contains things deliberately put there.
- There is no moment where work exists but is not yet safe — which is the
  moment every classic editor has, and the one a power cut finds.

`name <something>` promotes the current scratch buffer into the archive. That
is the whole of "save as". `save` forces a write now; it is rarely needed,
because autosave already ran.

## The journal holds the archive `[FACT]`

One append-only log holds every document. Records carry a name, and the scan
takes **the newest valid record per name** — so each document has its own
history in the same log and there is no second data structure to keep
consistent.

None of the power-cut properties depend on the name. They come from the sector
alignment, the erase-before-write and the per-record CRC, all unchanged and
all still verified by `main/selftest.c`.

The header carries `kind` and seven reserved bytes. Every previous format
change cost a migration and another reader; the reserved space means the next
addition is ignored by an older build rather than shifting the payload
underneath it.

Readers exist for all three formats, so nothing written by an earlier build is
stranded:

| Magic | Header | Meaning |
|---|---|---|
| `DECK` | 16 B | pre-name; loads as the scratch buffer |
| `DEK2` | 40 B | named, untyped; loads as prose |
| `DEK3` | 48 B | named, typed, extensible — current |

Buffers load **lazily**: a document restored at boot knows its name, length
and record offset but holds no text until it is selected. Eight buffers cost
eight small structs rather than a megabyte of PSRAM nobody asked for.

## Output is a buffer, not a scrollback `[FACT]`

`cmd_out` appends to a buffer called `+out`. That is not a detail — it is the
difference between this and a terminal.

A terminal's scrollback is the one thing on the machine that is *not* a
document: you cannot edit it, undo it, name it, search it with the same keys,
or pipe a piece of it anywhere. `SUBSTRATE.md` claims there is one data
structure; if command output lived in a log or a scrollback, the claim would
simply be false.

So output is a buffer like any other — same editor, same arrow keys, same
undo, same rendering. `Ctrl-O` goes there; so does the `out` command.

**A name beginning with `+` marks a buffer the machine wrote.** Such buffers
are never journalled, because an archive that fills up with command output is
an archive nobody trusts. They are otherwise completely ordinary: a buffer
need not be a file, exactly as a scratch buffer need not be.

## There is no command *environment* `[JUDGEMENT]`

Worth stating flatly, because it is the thing most likely to be got wrong
later. A guide buffer is **not a mode, a shell, a REPL or a place you go.**

- It is stored by the same journal.
- It is edited by the same editor, with the same keys, the same undo, the
  same wrap and the same cursor.
- It is listed beside every other document.
- It can be renamed, archived, or turned back into prose with one command.

The *only* difference between a guide and a page of prose is which of Enter
and Ctrl+Enter runs the line. One bit. That is precisely what `SUBSTRATE.md`
specifies when it says prose and grid "differ in exactly one bit of
interpretation, and that bit decides one thing."

What this keeps from a terminal: text in, text out; a command you can edit
before running it; composability.

What it refuses: a modal place you have to enter and leave; history that is
write-once and not editable; output you cannot touch; a command line that
vanishes the moment it runs. **Your commands are a document you keep** —
there is no history mechanism because the document *is* the history, and it is
yours to organise, rename and archive like any other writing.

## Patterns: one grammar, whatever the destination `[FACT]`

A lane is a line of characters. The characters are the same whether the lane
is a kick drum, a bassline or — when the destination exists — a frame trigger
on another board.

| Char | Means |
|---|---|
| `.` `-` `_` | rest |
| `x` and anything else | a hit at the lane's velocity |
| `X` | accent — three-quarters of the way to full |
| `,` | ghost — a third |
| `0`–`9` | on a melodic lane, the scale degree; `0` is the root |

```
>bpm 124
>scale dmin
>kick X...x...X...x...
>hat  x,x,x,x,x,x,x,x,
>bass 0...3...5...3...
>play
```

**Degrees, not note names.** A degree cannot be out of key, so the player
chooses shape — the musical decision — and the key is one word changed once.
`>scale fmin` transposes every melodic lane on the next step, because the
degrees are what is stored and the note is resolved when it sounds.

Spaces inside a pattern are ignored, so `x... x... x... x...` is legal and
reads better at four-column groupings.

### Swing `[VERIFIED]`

`>swing 50` is straight; `67` is triplet; `75` is the limit. Only **odd**
sixteenths move — the downbeat never does, which is the difference between a
groove and a tempo change. A pattern that only hits even steps is unaffected
by swing, which is correct and surprises people.

Measured on the deck at 124 bpm, as inter-onset intervals in ms:

| `>swing` | Intervals | Offbeat sits at |
|---|---|---|
| 50 | 125 / 125 | 50.0 % |
| 67 | 83 / 167 | 66.8 % |
| 75 | 62 / 188 | 75.2 % |

### Destinations `[FACT]`

Max/MSP splits its world in half: `~` objects are audio, `jit.` objects are
video, and the two halves have different rules and, in practice, different
users. That split is an artefact of how the two subsystems were built, not a
law of nature, and it is why a patch that makes sound cannot easily make a
picture.

There is no audio path and no visual path here. There is a **lane**, and there
are **destinations**, and the destination decides what a lane means. The same
`x...x...x...x...` is a kick on a MIDI destination and a frame trigger on a
network one. Adding live visuals is adding a destination, not adding a second
half of the system.

```
>send            list them and their state
>send mon on     print notes to the console
>send ble off    stop paying for a radio you are not using
```

Registering a destination does not enable it: a destination that switched
itself on at boot would be a radio nobody asked for. `ble` is the exception,
because the radio is already up for the keyboard.

### `>flash` — the escape hatch, written before it is needed `[VERIFIED]`

The rule on this project is that the owner is never asked to hold BOOT. Today
that holds because the ESP32-S3's USB-Serial-JTAG has reset logic in hardware
and esptool drives it. **Any firmware that reconfigures the USB peripheral —
a USB MIDI device, say — takes that hardware away.** So the software route to
the ROM loader exists and is proven *before* anything touches USB.

`>flash` saves the buffer, sets `RTC_CNTL_FORCE_DOWNLOAD_BOOT` and restarts.
Verified on hardware: `rst:0xc (RTC_SW_CPU_RST), boot:0x2 (DOWNLOAD(USB/UART0))`,
then flashed with `--before no_reset` — nothing in that path uses the
USB-Serial-JTAG reset logic.

**One property the owner has to know, found by testing rather than reading.**
`RTC_CNTL_OPTION1_REG` is in the RTC power domain and `esp_restart()` is a CPU
reset, so the bit *survives*. The deck re-enters download mode on every
subsequent reset until a full system reset clears it — which is what
`--before default_reset`, and therefore a plain `idf.py flash`, performs.
Nothing in ESP-IDF clears it; the only writers in the whole tree are IDF's own
USB console and this firmware. The deck is therefore never stuck, but it does
**wait**, silently, and a deck waiting in download mode looks exactly like a
dead one. The panel says so before it goes, and the app clears the bit at boot.

## Verified on the hardware `[FACT]`

Observed on the deck, 2026-09-20:

```
Ctrl-G  -> loaded 'guide' (25 bytes) on demand
Enter   -> help / list / new / name / open / save / close / guide / prose
arrow down, Enter -> *4 guide  25
arrow down, Enter -> buffer 5 '(scratch)' selected
```

and, across a reset, `archive: 'lullaby'`, `'rustbelt'` restored by name.

## Open questions `[OPEN]`

| # | Question |
|---|---|
| ~~1~~ | ~~Command output goes to the log and a status line.~~ **Closed.** Output is a buffer — see below. |
| 2 | The selection does not exist yet, so the implicit input is always empty and `\|` (Pipe) cannot be implemented. Selection is the next primitive, not another command. |
| 3 | Whole-buffer snapshots cost a flash sector per save. The undo log is already an operation log, which is already a redo log; deltas between periodic snapshots would cut writes by an order of magnitude. |
| 4 | `DOC_MAX_BUFFERS` is 8 and `DOC_NAME_MAX` is 24. Both are arbitrary and neither is enforced anywhere a person would see a useful error. |
