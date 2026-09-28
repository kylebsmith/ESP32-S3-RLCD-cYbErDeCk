# The editor and the screen — keys, marks, the grid, the panel

*Part of [the deck, top to bottom](README.md). Snapshot: commit `85d1e6a`, 2026-09-28.
Source is the truth; surprises and disagreements are in [errata.md](errata.md). The
proposal for Tab completion is in [completion.md](completion.md).*

**Path shorthand.** `viz.c` and `viz.h` are `firmware/components/viz/{viz.c,include/viz.h}`. `textgrid.*`, `tgfont.h` and `font*` are under `firmware/components/textgrid/`. `st7305*` is under `firmware/components/st7305/`. `editor.c`, `main.c`, `cell_attr.h`, `ui_text.h`, `ask.h`, `view.*`, `view_wire.h` and `serialkbd.c` are under `firmware/main/`. `builtins.c`, `cmd.c` and `lane_name.h` are under `firmware/components/cmd/`. `seq.c`, `seq.h` and `seq_pattern.h` are under `firmware/components/seq/`. `kbd.h`, `ble_kbd.c` and `serialkbd_map.h` are under `firmware/components/kbd/`. `buffer.c` and `docstore.h` are under `firmware/components/docstore/`. `VIEW.md` and `HARDWARE.md` are in `docs/`.

**Evidence marks.**
- **[probe]** means the claim was confirmed by running the shipping source on the host. The sources compiled were `viz.c`, `editor.c` + `cmd.c`, `textgrid.c` + fonts, and `serialkbd_map.h`. They were compiled read-only against stubs in a scratch harness outside the repo.
- **[read]** means the claim comes from reading the code only.
- **UNVERIFIED** means the claim cannot be confirmed from code: hardware or optical behaviour, or numbers that appear only in docs.

## The numbers

| Thing | Value | Where |
|---|---|---|
| Panel, logical | 400 × 300 landscape; native 300 × 400 | `st7305_addr.h:23-26` |
| Framebuffer | 15,000 B, controller order, internal DMA RAM, plus a 15,000 B gather buffer | `st7305_addr.h:28-30`, `st7305.c:331-332` |
| Faces | 12×24 "chunky" (default), 6×12 "compact". Both cover codes 32–155 | `tgfont.h:41-44`, `font12x24.c:2988-2991`, `font6x12.c:140-148` |
| Editor grid, chunky | 30 × 12 at origin (20,12): 11 text rows + 1 status row | `editor.c:167-189` [probe] |
| Editor grid, compact | 60 × 24 at origin (20,12): 23 text rows + 1 status row | same [probe] |
| Flush grids (no margins) | 33 × 12 (12×24) and 66 × 25 (6×12). Only 33×12 is ever used, for the boot test card | `textgrid.h:9-10`, `main.c:517` |
| Picture frame, maximum | 60 × 24 cells (`VIZ_W`, `VIZ_H`) | `viz.h:64-65` |
| Picture frame, default split | 28×4 chunky, 58×10 compact | [probe] |
| Cell cost, CPU | 3.8 µs pre-turned; per-row path 86.3 µs (measured per docs) | `HARDWARE.md:183-190`, `editor.c:658-661` |
| Cell cost, wire | 36 B for a 12×24 cell, 9 B for a 6×12 cell. A full frame is 15,000 B in 4.75 ms at 24 MHz (measured per docs) | `HARDWARE.md:153-157` |
| Clock | 96 PPQN; a sixteenth is 24 ticks | `seq.h:75-76` |

## 1. The editor (`editor.c`)

### 1.1 Layout

- **Grid.** Text rows are 0..`TEXT_ROWS`−1 and the status bar is row `STATUS_ROW` = last (`editor.c:182-184`). Column count is `TEXT_COLS` (30 or 60).
- **With the split on,** the text keeps all columns and gives up rows: text rows = `pane.y` (`editor.c:458-467`). Text rows given up are blanked (`editor.c:644-650`).
- **Order.** The pane is drawn after the text, then the status bar, then everything is rendered once (`editor.c:642-664`). Rendering and pushing are separate calls (`editor_present`, `editor.c:690-714`).

### 1.2 Event sources

Both keyboards feed one queue (`kbd.h:19-33`, `serialkbd.c:13-15`).

**BLE HID** (`ble_kbd.c:162-191`):
- Usages map to Enter (0x28 and keypad 0x58), Esc (0x29), Backspace (0x2A), Tab (0x2B), the arrows, Home (0x4A) and End (0x4D).
- Everything else becomes `KBD_EV_CHAR` with its modifiers attached.
- Key repeat is synthesised: 400 ms delay, 45 ms period, capped at 400 repeats (`ble_kbd.c:50-52`).
- A key that maps to nothing is logged (`unmapped key: …`) rather than dropped silently.

**"Command modifiers"** are `KBD_COMMAND_MODS` = **either Ctrl, or Left Alt**. Right Alt (AltGr) is *not* a command modifier (`kbd.h:49-60`) [probe]. Everywhere below, "Ctrl" means Ctrl or LAlt.

**Serial console** (`serialkbd_map.h:47-98`) [probe]:

| Bytes | Event |
|---|---|
| CR | Enter |
| LF | **Ctrl+Enter**, the only way to run a line over a cable |
| 0x08, 0x7F | Backspace |
| `\t` | Tab |
| 1–26 | Ctrl+letter |
| `ESC [ A–D` | arrows |

Consequences:
- **Ctrl-J (0x0A) arrives as Ctrl+Enter and runs the line**, so "previous document" is unreachable over serial.
- Ctrl-H is Backspace, Ctrl-I is Tab, Ctrl-M is Enter.
- **Esc needs `ESC ESC`.** A lone ESC swallows the next byte.
- Other escape sequences leak or vanish: Delete (`ESC [ 3 ~`) types `~`; Home (`ESC [ H`) does nothing. Home and End do not exist over serial.

### 1.3 Keys

Keys are handled in `editor_handle` (`editor.c:1093-1185`), in this order:
1. Wrap the text once.
2. If a password prompt is open, every key goes to it (§1.14).
3. Any key except Up and Down resets the sticky goal column (`editor.c:1119-1127`).
4. Any key that is not **pure motion** clears the refused-character box (`editor.c:1129-1144`). Pure motion is Left, Right, Up, Down, Home, End, and Ctrl + a, e, b, f, p or n.
5. Ctrl+Enter runs the line.
6. Ctrl+character goes to `handle_ctrl`.
7. Plain keys.

| Key | Action | Where |
|---|---|---|
| **Enter** (plain, Shift or RAlt) | **Always inserts `\n`**, in every buffer, guide included | `editor.c:1159-1174` |
| **Ctrl+Enter / LAlt+Enter** (also keypad Enter; LF over serial) | Runs the **logical** line under the cursor (§1.5); inserts nothing [probe] | `editor.c:1146-1149` |
| any Enter | also forces an autosave on the next pass | `main.c:774-776` |
| **Tab** (any modifiers: Ctrl+Tab, Shift+Tab too) | **Inserts two spaces**, `doc_insert(' ')` twice. Nothing else [probe] | `editor.c:1175` (the only Tab handler in the firmware) |
| Backspace (any modifiers) | Deletes the character before the cursor, across newlines | `editor.c:1176` |
| Left / Right | One document character, across newlines [probe] | `editor.c:1177-1178` |
| Up / Down | One **display** (wrapped) row, keeping the sticky goal column [probe] | `editor.c:767-773`, `727-748`, `1179-1180` |
| Home / End (BLE only) | Start or end of the **display** row | `editor.c:1181-1182`, `750-759` |
| Esc | Nothing outside a password prompt (it still clears the box and the goal column) | `editor.c:1183` |
| printable (32–126) | Inserted | `editor.c:1158` |

### 1.4 Ctrl chords

`handle_ctrl` lowercases the letter, so Shift is ignored (`editor.c:779-865`, `1151-1155`). Chords not listed do nothing and insert nothing [probe: `^X ^C ^S ^V ^I`].

| Chord | Action | Where |
|---|---|---|
| **^A** / **^E** | Start / end of the **display** row, same as Home/End | `editor.c:838-839` |
| **^B** / **^F** | One character left / right | `editor.c:840-841` |
| **^P** / **^N** | Up / down one display row. **Unlike the arrows, these lose the goal column**: `editor_handle` resets it before calling (col 21 → 3 → 3) [probe] | `editor.c:842-843`, `1120-1127` |
| **^K** | Deletes from the cursor to the end of the **display** row, not the logical line [probe] | `editor.c:844-853` |
| **^W** | Deletes non-word characters, then the word (`[A-Za-z0-9_]`), before the cursor | `editor.c:854-862`, `761-765` |
| **^Z** / **^Y** | Undo / redo. The log is 128 steps; typing coalesces up to 48 characters per step, broken by motion, newline, or switching between insert and delete | `editor.c:836-837`, `undo.c:9-13`, `28-29` |
| **^L** / **^J** | Next / previous document (§1.13) | `editor.c:833-834` |
| **^O** | Toggle between `+out` and the previous buffer (§1.11) | `editor.c:803-822` |
| **^G** | Jump to the guide (§1.12) | `editor.c:782-802` |

### 1.5 Running a line and refusals

`run_current_line` (`editor.c:1038-1091`):
1. **Copy the line.** It copies the logical line (newline to newline) into a **128 B buffer, so only the first 127 characters run** (`editor.c:922-941`, `1040-1041`).
2. **Check for a command.** Leading spaces and tabs are skipped. If the next character is not `>`, it shows `not a command - start with >` (`ui_text.h:28`) for 3 s.
3. **Run it** with `cmd_run_line(line, CMD_BY_HANDS, …)`, which has every capability (`cmd.c:19-28`).
4. **Cut a password typed on the line.** If the command reports one (`cmd_last_secret_col`), the line is cut from that column (`editor.c:1062-1066`, `975-997`).
5. **Box a refused character.** A single-line refusal that names a column sets `s_err_off`, and that character is **boxed** (under + over bars) until the next non-motion key (`editor.c:1067-1079`).
6. **Report.** Output of more than one line moves the view to `+out` (§1.11). Otherwise the status shows the command's message, or `ok`, for 4 s (`editor.c:1084-1090`).

**Bug: stray boxes** [probe]. The refused-cell test (`editor.c:612-613`) lacks the `< end` bound that the playhead test has (`editor.c:603-605`). Every earlier display row whose start offset lies within `TEXT_COLS` characters of the refused offset therefore also gets a box on a **blank** cell past its end. With `a`, `>bpm 1`, `>x` above a refused `>disc 3q..`, three stray boxed blanks appeared.

### 1.6 The status bar

`status_bar()` (`editor.c:298-364`) shows the first of these that applies:

1. **A password prompt:** `<question>: ****` (§1.14).
2. **A live message** (`s_msg` until `s_msg_until`).
3. `output - Ctrl-O goes back` (`UI_OUT_BACK`, `ui_text.h:40`), while in a `+` buffer and before `s_msg2_until`.
4. **The default line:**
   - Format: `"%-11.11s%c %3d:%-3d %s%s%s%s"` (`editor.c:334-341`).
   - Fields: name (or `scratch`), dirty `*`, line:col, modifiers held (`^` Ctrl, `A` LAlt, `G` RAlt, `S` Shift), `K`/`-` keyboard connected, `S`/`-` SD card, and a battery bar ` ####` of 4 cells shown only when the battery level is known (`editor.c:322-333`).
   - Example [probe]: `scratch    *   1:11  K- ##..`.

Whatever is shown is **clipped to `TEXT_COLS`** (`editor.c:343`). A message longer than 30 characters is cut at chunky density (for example `never type a password on a lin`) [probe]. `s_msg` itself holds 63 characters (`editor.c:110`).

**Bar colour** (`editor.c:345-363`) [probe]: the whole bar is **inverted (dark) in a normal document** and **normal (light) in a `+` (output) buffer**.

**The line number counts display rows, not logical lines, from the wrap window** (§1.10). Once the cursor is more than 6,000 B into a document, the count restarts: logical line 701 showed as `601` [probe].

**Messages and their durations:**

| Text | Duration | Where |
|---|---|---|
| `editor_message(...)` (boot notices and similar) | 6 s | `editor.c:136-140` |
| run result, or `ok` | 4 s | `editor.c:1087-1088` |
| `not a command - start with >` | 3 s | `editor.c:1047-1050` |
| `guide: Ctrl+Enter runs a line` / `no guide buffer` | 4 s / 3 s | `editor.c:794-800`, `ui_text.h:33-34` |
| Ctrl-O target name, or `scratch` | 2.5 s | `editor.c:817-819` |
| Ctrl-L/J target name, or `only one document` | 1.5 s | `editor.c:962-970` |
| output move: the command's message (or `output`), then `output - Ctrl-O goes back` | 2.2 s, then until 5.2 s | `editor.c:1022-1024` |
| password prompt result, or `stopped - nothing sent` | 4 s | `editor.c:1111-1113` |

`_Static_assert` checks three of the four `UI_*` strings against `UI_NARROW_COLS` = 30. `UI_OUT_BACK` (25 characters) is not asserted (`ui_text.h:220-224`).

### 1.7 The cursor

- The cursor is the only `TG_INVERSE` cell in the text area (`editor.c:619-628`).
- **Blink.** It toggles every 500 ms during the 15 s after the last key, then stays solid so the panel can drop to LPM (`main.c:60-66`, `915-928`). Any key makes it solid immediately (`main.c:778-783`).
- **Repaint.** A blink repaints only the cursor cell, through `cell_attr()`, so the playhead and word bars survive the blink (`editor.c:676-688`).

### 1.8 The playhead

`playhead_span` (`editor.c:386-437`) returns a span to mark only when all of these hold:
- The transport is running.
- The line is a recognised lane line.
- The lane exists, is not muted and has slots.
- **The line's text hash equals the hash of the text the lane was compiled from.** An edited but un-rerun line shows no playhead.

The span is the step currently sounding in *that lane's* own slot and cycle: from its first character to its last, including modifiers such as `%NN`; a chord spans first note to last (`seq_pattern.h:790-805`).

It is drawn as **`TG_UNDER`** on those characters, including across wrapped continuation rows and on a top row that continues a line starting above the viewport (`editor.c:536-583`, `603-614`).

The editor redraws for the playhead when the **global sixteenth** changes (`main.c:838-842`, `seq.c:947-948`). A lane faster than a sixteenth is therefore only sampled at sixteenths unless something else, such as a picture frame, triggers a redraw [read].

### 1.9 The command-word mark

- On the first display row of a logical line, `cmd_recognise` marks the command word, definition name or lane address with **`TG_OVER`** (a top bar) (`editor.c:549-566`, `cmd.c:134-174`). The `>` itself is not marked.
- An unrecognised word is left looking like prose.
- The comment at `editor.c:551-553` still says "inverse word"; that is stale.

### 1.10 Wrapping and scrolling

**Greedy word wrap** at `TEXT_COLS` (`editor.c:215-282`) [probe]:
- A row breaks after its last space. The space stays at the end of the upper row.
- A word longer than the row is broken hard.
- **A logical line of exactly `TEXT_COLS` characters is followed by a blank display row.** At chunky density this affects every 30-dash `+out` rule and the 30-character guide line `x.x. /2 half speed, *2 double.`.
- **End / ^E on a soft-wrapped row lands at column 1 of the *next* display row.** The row's end offset is the next row's start.

**The wrap window.** Wrapping starts at most `WRAP_BACK` = 6,000 B before the cursor, advanced to a line start, and covers at most `MAX_LINES` = 2,048 display rows (`editor.c:63-69`). Line numbers are relative to that window (§1.6).

**Scrolling** (`editor.c:505-530`):
- The view is anchored by a document offset (`s_top_offset`), not a line index.
- It scrolls only as far as needed to keep the cursor on screen. There is no centring, no margin and no horizontal scroll [probe].
- `editor_set_density` resets the view to the top (`editor.c:185`).

### 1.11 `+out` and Ctrl-O

**What `+out` is:**
- Every `cmd_out` line is appended to `+out`, which is created on demand (`cmd.c:68-98`).
- A **30-dash rule** is appended before each command's first line (`cmd.c:89-91`).
- The first output line becomes the status message if the command set none (`cmd.c:94-97`).
- `+out` is transient: it is never journalled, and autosave only marks it clean (`docstore.h:71-75`, `main.c:976-978`).
- Its status bar is light (§1.6).

**When the view moves to `+out`.** Output of more than one line moves the view, unless you are already in `+out` (`editor.c:1001-1028`, `1084-1086`) [probe]. The view lands on this command's rule (`s_top_offset` = the old end of `+out`); the status shows the message for 2.2 s, then `output - Ctrl-O goes back` until 5.2 s. If you are already in `+out`, the text is appended at the end and the cursor follows it (`buffer.c:272-275`).

**Ctrl-O** (`editor.c:803-822`):
- From `+out`, it returns to the buffer you came from (`s_prev_buf`).
- From anywhere else, it jumps to `+out`. If `+out` does not exist yet, nothing happens and no message is shown.
- The cursor position in each buffer is kept.

### 1.12 The guide and its version mark

- **Ctrl-G** selects the buffer named `guide`, puts the cursor and view at offset 0, and shows `guide: Ctrl+Enter runs a line` for 4 s. With no guide it shows `no guide buffer` (`editor.c:782-802`).
- **At boot**, `ensure_guide_buffer` creates the guide from `GUIDE_TEXT` (`ui_text.h:120-218`) if it is missing (`main.c:327-396`).
- **Version check.** If the existing guide lacks `>play` or the mark **`guide 3`** (`GUIDE_MARK`, `ui_text.h:118`; the last line of `GUIDE_TEXT`), it is rewritten: new text on top, then `-- previously in this guide --`, then the old text (`main.c:345-380`).
- **Enter runs nothing** in the guide either. The comment at `docstore.h:45` ("guide Enter EXECUTES the line") is stale; the buffer kind no longer decides what Enter does (`builtins.c:379-382`).

### 1.13 Document switching

- **Ctrl-L / Ctrl-J** step to the next or previous buffer, wrapping around, skipping `+` buffers and empty slots (`editor.c:948-971`) [probe]. There are 8 buffers (`docstore.h:22`).
- They show the target's name (or `scratch`) for 1.5 s, or `only one document`.
- **They always move the cursor and view to the top of the target (offset 0).** Ctrl-G does too. Only Ctrl-O keeps each buffer's position [read + probe].
- `>new` also blanks the picture and forgets all lanes (`builtins.c:283-299`).

### 1.14 The password prompt

(`ask.h`; wired in at `editor.c:192-203` and `1099-1117`)

- **Commands that ask.** `>wifi <ssid>`, `>host <ssid>` and `>ssh user@host <cmd>` ask for a password instead of reading one from the line (`builtins.c:1925-1935`, `1869-1876`).
- **On screen.** The status bar shows `<ssid or host, max 24 chars> password: ` followed by **one `*` per character typed**, clipped to 30 columns. Characters past the edge are not shown [probe].
- **Keys while the prompt is open.** Every key goes to the prompt and none reaches the document (`ask.h:62-87`) [probe]:

| Key | Effect |
|---|---|
| printable 32–126 | appended, up to `ASK_MAX` = 63 |
| Backspace | removes the last character |
| Enter, with or without modifiers | gives the answer to the command; its reply is shown for 4 s |
| Esc, Ctrl-C, Ctrl-G (or LAlt versions) | cancel: `stopped - nothing sent` |
| Tab, arrows, Home/End, Ctrl-O, other chords | ignored |

- **Wiping.** The secret is wiped through a volatile pointer on start, submit and cancel (`ask.h:37-57`).
- **Hidden hint.** The command's own hint, `type it, Enter. Esc stops` (`builtins.c:1933`, `1875` for ssh), is set as the status message but **never visible**: the prompt takes priority, and the answer replaces the message [probe].
- **A password typed on the line** is refused with `no passwords on a line - cut`, and the line is cut from that point (`builtins.c:1911-1915`, `editor.c:1064-1066`).

### 1.15 Tab today, for the Tab-completion discussion

**What Tab does now.** `KBD_EV_TAB` inserts exactly **two spaces** at the cursor, at `editor.c:1175`. That is its only handler. The modifiers are ignored, so Ctrl+Tab and Shift+Tab also insert two spaces [probe].

Where Tab comes from:
- BLE usage 0x2B (`ble_kbd.c:170`).
- Serial `\t`, which includes Ctrl-I (`serialkbd_map.h:81`).
- Ctrl-I over BLE is a Ctrl chord and does nothing.

Other paths:
- In the password prompt, Tab is ignored (`ask.h:84-85`).
- Tab is not "motion", so it clears the refused box and the goal column (`editor.c:1119-1144`).
- Tab passes through the same undo coalescing as typing: each space goes through `doc_insert` → `undo_record_insert` (`buffer.c:318-324`, `undo.c:103-115`).

**Where Enter and Ctrl+Enter are handled.** Both are in `editor_handle`: Ctrl/LAlt+Enter at `editor.c:1146-1149` (→ `run_current_line`, `1038`); plain Enter at `editor.c:1159-1174`. Also relevant: the autosave nudge (`main.c:774-776`), the serial mapping (`serialkbd_map.h:73-79`) and the prompt (`ask.h:65-66`).

**Hooks a completion feature would reuse:**

| Hook | What it gives |
|---|---|
| `current_line()` (`editor.c:922-941`) | the logical line under the cursor |
| `cmd_recognise()` (`cmd.c:134-174`) | the command-word span |
| `cmd_table()` (`cmd.c:60-66`) | the verbs and their help text (`builtins.c:2308-2343`) |
| `viz_prim_name()` / `viz_prim_count()` | picture names |
| user-defined names (`s_alias`) | static in `builtins.c:471-472`; the only public checks are `cmd_lane_known()` (`builtins.c:499-512`) and the `>help` listing (`builtins.c:879-899`) |

**Constraints a completion feature must respect:**
- **Never insert a literal `\t`.** Patterns treat it as spacing and rely on "Tab inserts two spaces" (`seq_pattern.h:145-148`), and the text grid would draw `?` (§2.1).
- Keep messages at 30 characters or fewer (`ui_text.h:4-19`).
- Remember that the serial Ctrl-I is Tab.

## 2. The text grid (`textgrid`)

### 2.1 Faces

| Face | Cell | Glyph design | Data |
|---|---|---|---|
| `12x24` (default) | 12 × 24 | body in columns 0–9, columns 10–11 gap; 2 px stems; cap height rows 4–19; descenders rows 20–22 (`tools/font12x24_art.py:1-10`) | `font12x24.c:2988-2991`, stride 2 |
| `6x12` | 6 × 12 | 5 px body, column 5 gap; row 0 leading; cap height rows 2–8; descenders rows 9–10 (`font6x12.h:7-13`) | `font6x12.c:140-148`, stride 1 |

- **Range 32–155** (`first` 32, `last` 155). 127 is a blank hole (`font6x12.h:22-26`, `font12x24.h:4-6`).
- **Codes 128–155 are tiles**, generated as geometry by `tools/font_tiles.py` (`font6x12.c:106-109`).
- **Out of range falls back to `'?'`** (`tgfont.h:24-33`). The editor maps `\n` to a space before drawing (`editor.c:592`), but a TAB or other control character in a document renders as `?` [probe].
- **Layout rules** (`tg_set_layout`, `textgrid.c:44-94`, enforced with `ESP_ERR_INVALID_ARG`):
  - Cell height and origin y must be multiples of 12 (the CASET quantum).
  - Cell width and origin x must be even (the RASET quantum).
  - The grid must fit 400×300 and 66×25.

### 2.2 Densities

- `editor_set_density(level)` (`editor.c:167-190`): level 0 is 12×24 → 30×12; level ≥ 1 is 6×12 → 60×24. Both sit inside a 20 px left and right margin and a 12 px top margin, with **no bottom margin and no gap**.
- `>density`, from `c_density` (`builtins.c:401-438`):
  - `h`/`d`/`6…` select high (6×12); `l`/`c`/`1…` select low (12×24). `>density 1` means **low**, even though `editor_set_density(1)` would mean high.
  - Reply: `low 30x12` / `high 60x24` (grid size including the status row).
  - Errors: `density low | high`, `density: layout refused`.
- The flush 33×12 grid, from `tg_set_font` (`textgrid.c:96-104`), is used only at boot for the test card and splash (`main.c:517-523`, `684`). The 66×25 flush grid is never set up [read].
- The layout sketch in the header comment (`editor.c:4-22`: "10 rows", a gap, a 12 px bottom margin) and "30 x 10" at `main.c:704` are **stale**.

### 2.3 Attributes

Attributes are bits, so they compose (`textgrid.h:29-61`; `cell_attr()` in `cell_attr.h:26-31` is the single place they are combined):

| Bit | Name | Drawn as | Owner |
|---|---|---|---|
| 1 | `TG_INVERSE` | whole cell inverted | the edit cursor only (and the status bar and passkey screen) |
| 2 | `TG_UNDER` | bar across the bottom | the playhead |
| 4 | `TG_OVER` | bar across the top | a recognised command word |
| 2\|4 | under + over | "boxed" | a refused character (`editor.c:612-618`) |

**Bar height** = max(2, face height / 6): **4 px on 12×24, 2 px on 6×12** (`textgrid.c:205-213`) [probe]. Bars are XOR-ed *after* inversion, so a bar on the cursor shows as paper cut out of the block (`textgrid.c:223-234`) [probe].

### 2.4 The pre-turned glyph cache

(`textgrid.c:19-30`, `274-380`)

- **Why it works.** Every cell is whole framebuffer bytes in every orientation: 6×6 B for 12×24, 3×3 B for 6×12. So each glyph is turned once into those bytes, the first time it is drawn in the current orientation, into `s_turned[124×36]`, with a have-bit per glyph.
- **Attribute masks.** There are eight XOR masks (`s_turn_mask[8][36]`), one per attribute combination. Each is the per-row path's own output with that attribute XOR the plain cell, so a cell is `dst = glyph ^ mask[attr]`, 36 or 9 byte stores.
- **Rebuilding.** The cache is rebuilt when the orientation changes, including a KEY press (`textgrid.c:332-353`), and after any `tg_set_layout` (`textgrid.c:82`).
- **When it applies.** Scale 1 only, and the whole face must fit, which both faces do (`textgrid.c:79-81`). `tg_set_turned(false)` forces the per-row path; only the boot bench uses it (`main.c:139`).
- **Tested.** `tools/test_textgrid.c` checks that both paths produce identical bytes.

### 2.5 Damage tracking and cost

- **Per-cell dirty bits.** `tg_put` marks a cell dirty **only if the character or attribute changed** (`textgrid.c:133-144`) [probe: an identical re-put redrew 0 cells]. `tg_clear` marks only the cells that change; `tg_invalidate` marks everything.
- **Render.** `tg_render` draws every dirty cell, declares each as one damage rectangle to the driver, clears the dirty bits and returns the count (`textgrid.c:387-406`). The editor renders in `editor_draw` and pushes separately in `editor_present` (`editor.c:662-664`, `698-714`).
- **Cost of one cell** (measured per docs, UNVERIFIED here):
  - CPU: 3.8 µs pre-turned, including 0.67 µs of damage bookkeeping; 86.3 µs on the per-row path.
  - First frame after a layout or orientation change: 9.8 ms.
  - Wire: 36 B (12×24) or 9 B (6×12).
  - Wall time: ≈390 µs per keystroke, dominated by transaction overhead rather than 3.6 µs of bandwidth.
  - Sources: `HARDWARE.md:153-168`, `183-195`.
- `tg_draw_text_px` and `tg_text_width_px` (`textgrid.c:189-203`) have **no callers** [read]. The comment at `editor.c:284-285` says the status bar is drawn at its own pixel row; that is stale, as it is grid row `STATUS_ROW`, drawn with `tg_put` (`editor.c:361-363`).

## 3. The panel driver (`st7305`)

### 3.1 Bus, pins and initialisation

| Item | Value | Where |
|---|---|---|
| Pins | SCK 11, MOSI 12, CS 40 (driven by hand), DC 5, RST 41 | `st7305.c:26-30` |
| SPI | SPI2; writes at 24 MHz; reads (RDDID) at 6 MHz, 3-wire; transfers chunked at 4,092 B | `st7305.c:32-35`, `648-695` |
| Bus lock | recursive mutex shared by the main task, the timer task and the NimBLE task | `st7305.c:66-86` |
| Init | reset; register sequence taken from SolarOS; sleep-out; inversion off (0x20); full window; HPM (0x38); display on (0x29) | `st7305.c:202-278` |

`st7305_set_inverted` (INVON/INVOFF) exists but is never called [read].

### 3.2 Power policy

The policy is `ST7305_POWER_AUTO` (`st7305.c:61`). `st7305_set_power_policy`, `_set_hpm` and `_set_lpm` have **no callers outside the driver** [read], so AUTO is the only policy in use.

1. Every window push first ensures HPM (0x38), then restarts a one-shot **1,000 ms** idle timer (`st7305.c:37`, `561-566`).
2. The timer callback only sets a flag (`st7305.c:158-176`).
3. `st7305_service()`, called from the main loop (`main.c:818`), then sends LPM (0x39) (`st7305.c:178-184`).
4. Rates per the code comments: HPM 32 Hz, LPM 1 Hz, FRCTRL = 0x12 (`st7305.c:39-42`, `240-242`). UNVERIFIED on glass.

**What keeps the panel in HPM:**
- Cursor blink: every 500 ms for 15 s after the last key (`main.c:65-66`, `915-928`).
- The playhead: a redraw every sixteenth while playing (`main.c:838-842`).
- Every picture frame (`main.c:872-873`).

### 3.3 The damage list

(`st7305.c:48-60`, `400-444`)

- Up to **4 rectangles**, in logical coordinates, inclusive (`DMG_MAX`).
- **Fold-in.** A new rectangle merges into the *first* existing one it touches or comes within slack of: **2 px in x, 12 px in y**, the controller quanta seen from the logical side (`st7305.c:417-425`). There is no cascade: an enlarged rectangle is not re-merged with the others.
- **Full list.** When all four are in use, the pair whose union adds the least area is merged, and the new rectangle takes the freed slot (`st7305.c:432-443`).
- `st7305_clear` and `st7305_flush_full` reset the list to the whole panel (`st7305.c:529-535`, `634-640`).

### 3.4 Window quantum and push

- **Quantising.** Each rectangle is mapped through the orientation to native coordinates, then rounded out to **12 native-x px (one CASET unit, 3 bytes)** and **2 native-y px (one RASET row address)** (`st7305_addr.h:91-108`, `st7305.c:613-630`).
- **Addressing.** CASET is mirrored and reversed: `{0x3C−end, 0x3C−start}` (`st7305_addr.h:99-100`). RASET is `{first/2, last/2}`.
- **Sending.** Each rectangle is one CASET (0x2A) + RASET (0x2B) + RAMWR (0x2C). Full-width windows are sent straight from the framebuffer; narrower ones are gathered into `s_stage` first (`st7305.c:550-599`).
- **Result.** `st7305_flush` pushes every rectangle and reports the total bytes (`st7305.c:601-632`). The damage list is cleared before pushing, so a failed push is not retried (`st7305.c:610-612`).

### 3.5 Orientation

There are four mappings (`st7305.h:39-54`, `st7305_addr.h:37-45`):

| Mode | Mapping |
|---|---|
| 0 | nx = y, ny = x |
| 1 | nx = y, ny = 399−x (horizontal mirror) |
| 2 | nx = 299−y, ny = 399−x (180° rotation) |
| 3 | nx = 299−y, ny = x (vertical mirror) |

- **Defaults.** The driver's static default is 1 (`st7305.c:64`), but boot loads NVS `deck/orient` and falls back to **3** (`main.c:82-108`, `515-516`).
- **KEY (GPIO 18).**
  - A tap cycles the mode (+1 mod 4), saves it, clears the panel and redraws everything (`main.c:793-811`).
  - A 2 s hold forgets all keyboard bonds (`main.c:52`, `798-801`).
  - Holding KEY at boot forces USB MIDI off (`main.c:629-635`).
- A logical row is always one native column, which is what makes the row blit and the turned cache possible (`st7305_addr.h:59-79`).
