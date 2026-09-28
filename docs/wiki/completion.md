# Tab completion — a proposal

*Part of [the deck, top to bottom](README.md). 2026-09-28. A design, not built. It answers
the owner's question: press Tab to fill in a word, Tab again to step through the
possibilities, Enter to confirm — possible, and reasonable?*

## The answer

**Yes — possible, cheap, and worth doing, and as asked: Tab fills in, Tab steps through,
Enter confirms.** Two findings shape it.

1. **Enter confirms, and only confirms.** While a word is offered, Enter keeps it and
   inserts nothing, so you carry on typing on the same line. Esc gives back exactly what you
   typed. Any other key keeps the word and then does its usual job. This is what zsh's menu
   selection and fish's completion pager do, and what Strudel, Sonic Pi and Max do with
   their lists ([research.md](research.md) §5).

   **It is also a deliberate exception** to the editor's rule that Enter always inserts a
   new line (`editor.c:1159-1174`). That rule was written because Enter once ran lines in
   the guide, which made it impossible to add a line after a command — a mode that never
   ended. This one lasts until the next key, the status bar says it is on, and a second
   Enter adds the line. §8 keeps the alternative open.
2. **It is for finding words, not for typing fast.** The first word of a line can be one
   of 67 words: 34 verbs, 17 boot names and 16 pictures. They average 4.0 letters, so
   completion saves an expert about 1.3 keystrokes a word (32 %) at best, and 15 of the 67
   are too short to gain anything. What it gives a newcomer is larger: the deck lists its
   own vocabulary as you type, with a line of help for each word, and you can no longer
   misspell a word you have seen. The savings grow with long words — user names, document
   names, routes like `disc:x`.

## 1. What Tab does

**Where the cursor is decides what is offered** — the deck already parses every line, so
completion can be as precise as the grammar:

| the cursor is in | Tab offers |
|---|---|
| the first word after `>` | the 34 verbs, the defined names (17 at boot, up to 32), the 16 pictures, and the addresses of live lanes (`disc:2`, `bass:vel`) |
| a word after `name:` | the parts that name can take: `vel` for drums; `vel`, `oct` for voices; `x`, `y` for pictures; and `end` as a route source |
| the word after `=` in a definition | `note voice cc knob pad` and the 16 pictures |
| a verb's argument | its own words: `>send` → the destinations, then `on off`; `>sync` → `on off lead follow alone`; `>split`, `>usb` → `on off`; `>open`, `>dump`, `>run` → document names; `>route`, `>mute`, `>solo` → lane addresses (and for a route's source, inputs and `name:end`); `>scale` → after the root, the eleven modes; `>kbd`, `>wifi`, `>ssh` → `forget`; `>flash` → `now`; `>density` → `low high`; `>jitter` → `reset`; `>osc` → `in` |
| a pattern, prose, a number, or the start of a line | nothing — **Tab inserts two spaces, as today**, so indentation and spacing in patterns are unchanged |

**The keys:**

| key | when nothing is offered | while a word is offered |
|---|---|---|
| **Tab** | one match: fills it in. Several: fills in the letters they share; the next Tab offers the first. None: two spaces | offers the next; after the last, gives back what you typed; then round again |
| **Shift+Tab** | as Tab | offers the previous |
| **Enter** | a new line, as today | **keeps the word; nothing else** |
| **Esc** | nothing, as today | takes the offer back — the line is exactly what you typed |
| **Backspace** | deletes, as today | takes the offer back |
| **any other key** — Ctrl+Enter, space, a letter, an arrow | as today | keeps the word, **then does what it always does**: Ctrl+Enter runs the line, space and letters are typed after the word |

**Tab on a word already complete** (`>kick` Tab) changes nothing and shows the word's help
— `kick = note 36` — so Tab also answers *what is this?*

**On the screen.** The offered word appears **in the line, where it will stand**, as it does
in every shell and editor checked. The novice's Tab then shows the expert's line. The
status bar says what is happening, clipped to the 30 columns every status message has:
`2/4 din  MIDI with no host  ⏎ keeps`.

**The order is fixed**: alphabetical within each position, never by use. The same letters
and the same number of Tabs always give the same word, so the Tab count becomes a habit
of the hand. A list that reorders itself by use would move words while the hand is
learning them.

## 2. Help for every word

| word | help shown | where it comes from |
|---|---|---|
| a verb | its table help: `din <gpio> - MIDI with no host` | `cmd_t.help`, already in `builtins.c:2308-2343` |
| a name | its definition: `bass = voice 2 ch 1 gate 180` | the name table |
| a picture | one line each, sixteen new strings: `disc — a round field` | new, about 400 bytes of flash |
| a part | `vel — the level, from here on` | new, four strings |
| a document | its size: `ground  1350 bytes` | the buffer table |
| a mode | its intervals: `dor  0 2 3 5 7 9 10` | `seq_modes[]` |

## 3. What it costs

- **Core 0 only, between keystrokes.** A Tab is one pass over at most about 130 words — a
  few microseconds. The clock's core never sees it.
- **One undo step per kept word.** Offered letters replace one another in place without
  touching the undo log. Keeping a word records its letters as one step. Taking an offer
  back leaves the log as it was.
- **No new verb, no allocation, no flash write.** About 1 KB of help strings in flash.
- **Zero cost when Tab is not pressed.**

## 4. Where it lives

- **`firmware/main/complete.h`** — a pure header, like `ask.h` and `cell_attr.h`, so the
  host can test it: given a line, a column and a vocabulary, it returns the word's span,
  the candidates in order, and the letters they share.
- **The vocabulary**, gathered in `editor.c` from what the deck already knows:
  - verbs from `cmd_table()`;
  - pictures from `viz_prim_name()`;
  - live lanes from `seq_lanes()`;
  - parts from `lane_name.h`;
  - documents from `doc_buf_name()`;
  - modes from `seq_modes[]`.

  One accessor is new: the names live in a static table (`builtins.c:471-472`), so
  `cmd_name_at(i)` has to be added.
- **The keys**: a small state in `editor_handle` — offering or not, which candidate, the
  word's span — about forty lines. The BLE keyboard already reports Shift with Tab
  (`ble_kbd.c:170`); the serial map needs `ESC [ Z` (back-tab) added.
- **Constraints the reference turned up** ([editor.md](editor.md) §1.15):
  - never insert a literal tab — patterns treat it as spacing, and the grid draws it as
    `?`;
  - keep every message to 30 columns;
  - over the serial cable, Ctrl-I *is* Tab.

## 5. How it is checked

`tools/test_complete.c`, in CI, like the other pure headers:

- a table of lines, cursor positions and what Tab offers:
  - `>ki|` → `kick`;
  - `>disc:|` → `x y`;
  - `>send m|` → `mon`;
  - `>scale d|` → the eleven modes after the root;
  - `>scale dm|` → `dmaj dmaj5 dmin dmix`;
- Enter on an offer keeps the word and inserts no `\n`; the next Enter inserts one;
- Ctrl+Enter on an offer runs the line with the whole word in it;
- Esc restores the line byte for byte, and so does the Tab after the last candidate;
- no path ever inserts `\t`;
- the order is the same on every call.

The owner's rule — every change comes with a check that fails on the old code — is met
by the editor-level cases: today, Tab after `>ki` inserts two spaces.

## 6. What it is not

**Not a menu.** [MAP.md](../MAP.md) §9.4 refuses "anything with a menu", and this passes
that bar:

- **The document holds only text**, exactly what an expert would have typed.
- **Nothing appears unless Tab is pressed.**
- **Nothing is chosen from a list on the screen:** the word is in the line, and the
  status bar names it.
- **There is no verb.**

It is the deck reading its own vocabulary back to you, one word at a time.

## 7. What the evidence says

Checked against the sources on 2026-09-28; the full entries are in
[research.md](research.md) §5.

- **Completion is used constantly where it exists.** Content assist was one of the five
  most-used commands for every one of 41 Java developers — as common as the basic editing
  commands (Murphy, Kersten & Findlater 2006).
- **Saved keystrokes are not saved time.** Scanning and choosing among suggestions costs
  attention:
  - in word prediction, the attention cost "largely overwhelmed" the keystrokes saved
    (Koester & Levine 1996);
  - suggestions shown more assertively saved keys and cost time (Quinn & Zhai 2016);
  - among 37,000 phone typists, those who used prediction typed slower (Palin et al.
    2019).

  **So: offer only when Tab is pressed, from a small set, and never interrupt typing.**
- **Filter by typing, then accept, is the common pattern** in code completion, and most
  opened lists end without a choice. One failure developers hit is a list that swallows
  keys meant for the editor (Mărășoiu, Church & Blackwell 2015). **So: every key but
  Tab, Enter, Esc and Backspace passes straight through.**
- **A list that reorders itself costs people their bearings.**
  - Menus reordered by use were slower at first, and 81 % of people preferred fixed ones
    (Mitchell & Shneiderman 1989).
  - A fixed split menu beat an adaptive one (Findlater & McGrenere 2004).
  - Ordering by recent use was the slowest design tested (Cockburn, Gutwin & Greenberg
    2007).
  - Marking-menu users never graduate when item positions change (Kurtenbach & Buxton
    1994).

  **So: a fixed order.**
- **The novice's action should rehearse the expert's** (Kurtenbach 1993, restated by
  Cockburn et al. 2014). Typing a word's first letters and pressing Tab is typing the word,
  shorter, and the offered word appears where the expert's would.
- **Seeing the faster way is not enough.** Word printed its shortcuts beside every menu
  item, and still only a few percent of experienced users preferred them (Lane et al.
  2005). **So: help beside the word, at the cursor, when it is wanted.**
- **The precedents agree on the keys** ([research.md](research.md) §5):
  - Readline and zsh fill in the shared letters first; zsh starts cycling on the second
    Tab by default.
  - Readline's menu completion, and Emacs's `dabbrev-expand`, give back what you typed
    after the last candidate.
  - zsh's menu selection and fish's pager: Enter only accepts, Esc or Ctrl-G restores, and
    any other key accepts and acts.
  - Vim's Enter does different things depending on *how* the match was chosen, and its
    manual needs an extra paragraph to explain it. That is the reason the deck's Enter
    means one thing while a word is offered, and the status bar says so.

## 8. The owner's decisions

1. **What Enter does to an offer.** *Recommended, and asked for:* Enter keeps the word and
   inserts nothing. *Alternative:* Enter keeps the word and inserts its new line, so the
   editor's rule holds without exception.
2. **What the first Tab does with several matches.** *Recommended:* fill in the shared
   letters, and cycle from the second Tab, as zsh does by default. *Alternative:* offer the
   first whole word at once — faster for a vocabulary this small, but the first Tab then
   guesses.
