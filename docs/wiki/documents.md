# Documents — buffers, the journal, autosave, undo

*Part of [the deck, top to bottom](README.md). Snapshot: commit `85d1e6a`, 2026-09-28.
Surprises are in [errata.md](errata.md).*

## 1. Documents (`docstore/`, `main.c`, `builtins.c`)

### 1.1 Buffers (`docstore.h:21-23`; `buffer.c`)

- **Buffers.** `DOC_MAX_BUFFERS` = **8** resident buffers. Each gets a 128 KB gap buffer, allocated **lazily** in PSRAM (falling back to the default heap) the first time it is selected (`buffer.c:9-13,46-94`). Slot 0 is created as scratch at boot (`buffer.c:291-298`).
- **Names** are unique among resident buffers and hold at most 23 characters (`DOC_NAME_MAX` 24); longer names are cut silently by `snprintf` (`buffer.c:192-210`).
- **Unloaded buffers** report the length stored on flash (`buffer.c:116-126`).
- **A record that will not read** refuses to open, rather than showing an empty buffer: `cannot load '%s': %s - leaving it unopened` (`buffer.c:72-81`).

### 1.2 Journal (`journal.c`; `partitions.csv:5-10`)

- **Partition**: `notes`, data subtype `0x40`, at `0x310000`, **512 KB** = 128 sectors of 4 KB.
- **Header v3**, 48 bytes (`journal.c:49-57`; confirmed by compiling on the host). The format comment at `journal.c:6` omits `kind` and `reserved`.

  | field | type |
  |---|---|
  | `magic` | u32 `0x334B4544` "DEK3" |
  | `seq` | u32 |
  | `len` | u32 |
  | `crc` | u32, `esp_rom_crc32_le(0, payload, len)` |
  | `kind` | u8 |
  | `reserved` | u8[7] |
  | `name` | char[24] |

  - Old formats are read but never written: v2 "DEK2" with a 40-byte header, and v1 "DECK" with 16 bytes, which is always scratch (`journal.c:40-42,212-249`).
- **Record size**: `ceil((48+len)/4096)` whole sectors, aligned to sectors (`journal.c:195-199`).
- **Save** (`journal.c:424-502`):
  - A transient buffer is only marked clean.
  - Otherwise: find a free run of sectors → **erase** → write the payload → write the header **last** → only then release the sectors of the previous record with the same name (`journal.c:130-166`).
  - The payload is copied to internal RAM first, because a PSRAM source splits the write into roughly 160 flash operations (`journal.c:454-469`).
  - No free run returns `ESP_ERR_NO_MEM`, logged as `journal full: %u of %u sectors hold live documents` and shown on the panel as `JOURNAL FULL - free a document` (`main.c:1013-1019`).
- **Boot scan** (`journal.c:287-398`):
  - The newest valid record wins, per name.
  - A record that fails its CRC is skipped, logged as `record at %u fails CRC - ignoring…`.
  - Up to `JOURNAL_MAX_NAMES` = **24** names are tracked. Past that: `archive full: '%s' is held but unreachable`.
  - More than 8 archived documents: `%d documents archived, %d can be open at once`.
  - The scratch record loads into slot 0. Each named record claims a free slot **in physical sector order**, until the slots run out.
- **Torn writes, by design.** A power cut costs at most the newest snapshot (`journal.c:22-27`). The self-test that proves this is compiled out (`main.c:569-588`).

### 1.3 Scratch vs named

| | scratch | named |
|---|---|---|
| name | `""` (`buffer.c:183-190`) | set by `>name`, which saves at once (`builtins.c:304-317`) |
| journalled | yes, under the name `""` | yes, under its name |
| SD mirror file | `scratch.txt` (`mirror_path.h:47-49`) | `<name, sanitised>.txt` |
| at boot | the newest `""` record goes into slot 0 | claims a slot while slots remain |

Consequences that follow from the code (**UNVERIFIED** on hardware):

- **Only one scratch buffer survives a reboot.** Every scratch buffer (slot 0 and each `>new`) journals under the same name `""`. Each save therefore releases the other scratch buffer's record (`journal.c:130-158`), so only the scratch saved last comes back. This contradicts "every buffer is journalled and crash-safe… named or not" (`docstore.h:32-36`).
- **`>name` copies; it does not move.** The old `""` record (or the old name's record) stays live, so after a reboot the scratch buffer comes back holding the pre-naming text, beside the named document.
- **An archived name can be overwritten.** The name check covers resident buffers only (`buffer.c:199-205`). `>name X` on a buffer, where X is a closed or non-resident archived document, supersedes X's record and releases its sectors.
- **Past 7 named documents, the rest cannot be opened.** `>open` searches resident buffers only (`builtins.c:319-346`), and closing a buffer does not load another. There is no command that deletes a document from the journal.
- **`>name +x` makes the buffer transient**, so it is never saved again. Nothing forbids the `+` (`buffer.c:192-210`).

### 1.4 Transient `+` buffers

- **What is transient.** Any buffer whose name begins with `+` is neither journalled nor mirrored (`docstore.h:71-76`; `journal.c:429-433`; `sdmirror.c:123-127`; `mirror_path.h:18-24`).
- **`+out`**:
  - It is created on first use (`doc_buf_ensure`) and occupies one of the 8 slots.
  - All command output goes there, each command's lines preceded by a rule of 30 dashes (`cmd.c:68-98`), and so do SSH replies.
  - When it reaches 128 KB, further appends are cut off silently; nothing trims it (`buffer.c:253-277`).
- **Behaviour in the editor.**
  - Ctrl-L and Ctrl-J skip `+` buffers (`editor.c:948-970`); Ctrl-O toggles into and out of `+out` (`editor.c:803-822`).
  - The status bar is drawn un-inverted in a `+` buffer (`editor.c:357-362`).
  - `>save` in one says `<name> is output, not a document` (`builtins.c:350-358`).
  - `doc_save_all_dirty()` skips `+` buffers (`buffer.c:411`).

### 1.5 Autosave (`main.c:53-59,959-1024`)

| constant | value | role |
|---|---|---|
| `AUTOSAVE_MS` | 1000 | minimum idle since the last edit before any save (`main.c:1003`) |
| `AUTOSAVE_MIN_CHARS` | 24 | a save is worth it if the length changed by at least 24 since the last save |
| `AUTOSAVE_IDLE_MS` | 6000 | …or the typist has been idle 6 s |
| `force_save` | — | set by any Enter, including Ctrl+Enter (`main.c:774-776`), and by the transport stopping (`main.c:964-974`) |

- **Rule:** `(force || Δlen ≥ 24 || idle ≥ 6 s) && !seq_running()`, and then `idle ≥ 1 s` (`main.c:999-1003`).
- On success it logs `saved %u bytes, seq %u, %lld us` and mirrors to the SD card. On failure: `JOURNAL FULL - free a document` or `SAVE FAILED`.
- A dirty transient buffer is only marked clean (`main.c:976-978`).
- **Why not while playing** (`main.c:984-998`): a journal write was **measured at 13 000–18 600 µs**, with both cores' caches off and the clock callback in flash. That is the only term above the ~6 ms at which a percussive onset is heard as displaced (see also `docs/GRAPHICS.md:175-179`; `STATUS.md:63,391-392`: "saved 59 bytes, seq 205, 16219 us" caused the one late tick in 3008).
  - Edits made while playing stay in RAM until `>stop`. The stop edge is detected in the main loop; the comment's "`seq_stop()` forces the save" (`main.c:996-997`) is not literally true.
- **Only the current buffer is autosaved.** Switching documents (Ctrl-L, Ctrl-J, `>open`) saves nothing (`editor.c:948-970`; `builtins.c:319-346`). A buffer left dirty stays in RAM until it is current again, or until `>usb`, `>flash now` or an esptool reset runs `doc_save_all_dirty`. `saved_len` is one variable shared by all buffers (`main.c:723,978,1008`).

### 1.6 SD mirror (`sdmirror.c`; `mirror_path.h`)

- **Bus.** 1-bit SDMMC: CLK 38, CMD 21, D0 39, internal pull-ups, no card-detect. It mounts `/sdcard` at 400 kHz, then retries at the default speed; it never formats; at most 4 files open (`sdmirror.c:40-115`).
  - A missing card is not fatal: `no SD card responded - the slot is probably EMPTY…; journal only`.
- **Files.** `/sdcard/<name>.txt`, where characters outside `[A-Za-z0-9_-]` become `_`. So `a b` and `a_b` collide. Scratch writes to `scratch.txt`. Long names are enabled (`sdkconfig.defaults:76-78`).
- **Write order.** `mirror.tmp` → `fflush` + `fsync` → `remove(target)` → `rename` (`sdmirror.c:139-163`). A cut between remove and rename leaves only `mirror.tmp`.
- **When it runs.** After every successful autosave, after `>save`, and in `doc_save_all_dirty`. The panel shows `S` when a card is mounted (`editor.c:334-340`).
- **Stale comments.** `docstore.h:12` says the mirror runs "on newline or a 1 s timer". `docstore.h:109` gives the file as `notes.txt`.

### 1.7 Undo and redo (`undo.c`; Ctrl-Z / Ctrl-Y, `editor.c:836-837`)

- **One global log**: 128 steps, runs of up to 48 characters, in PSRAM (`undo.c:28-58`).
- **Coalescing.** An insert run continues while it is contiguous and closes after a space, newline or tab. A delete run grows leftwards with consecutive backspaces (`undo.c:103-161`).
- **Ownership.** Each operation is tagged with the **slot index** of its buffer. Undo and redo act only if the top operation belongs to the current slot; otherwise they log `nothing to undo in this buffer` and do nothing (`undo.c:75-86,197-230`). An edit in document A therefore cannot be undone while B's newer edits sit on top of the log.
- **What resets or bypasses it.** The log is reset only by `doc_set_text()`, at boot (`buffer.c:401-405`). Appends to `+out` are not recorded (`buffer.c:253-277`).
- **Two findings from the code, UNVERIFIED on hardware:**
  - A slot freed by `>close` and reused by `>new` inherits the old buffer's operations.
  - The secret-cut path (`editor.c:975-996,1062-1066`) deletes the password with `doc_backspace()`, which **records it in the undo log**. Ctrl-Z would restore it, and the next autosave would journal and mirror it.

### 1.8 What each command does to buffers

| command | buffers | lanes and flash |
|---|---|---|
| `>new` | claims a free slot as a scratch buffer and selects it: `new scratch buffer %d`, or `no free buffer` | **forgets every lane and picture** (`seq_forget_all`, `viz_forget_all`; `builtins.c:283-300`) |
| `>name <n>` | renames the current buffer: `filed as %s`, or `cannot use that name` | journal write, not gated by the transport |
| `>open <n>` / `>open <digit>` | selects the buffer. The digit form reads **one character**, so `>open 12` opens buffer 1, and a name that starts with a digit cannot be opened by name. Errors: `no buffer %d`, `no document called '%s'` (`builtins.c:319-346`) | — |
| `>close` | frees the RAM; **unsaved edits are lost**; the journal keeps the last save, which returns at the next boot. `cannot close the last buffer` (`builtins.c:369-377`; `buffer.c:212-228`). This contradicts "closing" being journalled in `main.c:997-998` | — |
| `>run [n]` | runs every line as GUIDE (SYSTEM lines refused), then returns to the original buffer. `run cannot run itself`, `no document '%.20s'`, `%d ran, %d refused`, `%d line(s) ran` (`builtins.c:91-149`) | — |
| `>save` | saves the current buffer and mirrors it: `saved %u bytes`, or `save failed` | not gated |
| `>list` | `"%c%d %-16s %5u%s"` (`*` marks current, `(scratch)`, ` *` marks dirty) (`builtins.c:56-73`) | — |
| `>dump [n]` | prints numbered lines to `+out` and the console; refuses `+out` itself (`builtins.c:1176-1234`) | — |
