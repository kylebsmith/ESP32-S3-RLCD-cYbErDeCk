# The verbs — every command, from the source

*Part of [the deck, top to bottom](README.md). Snapshot: commit `85d1e6a`, 2026-09-28.
Surprises and disagreements are in [errata.md](errata.md). Updated 2026-09-29 for
`toggle`, `clear`, `map`, the re-run that removes, and `>name = ch`: those parts cite
functions by name, since the line numbers moved.*

Firmware at `85d1e6a` (branch `claude/pieces-and-satellites`). The source is the truth;
`docs/` is secondary, and where the two disagree it is listed in §4.

**37 verbs**: the table `s_builtins[]`, `builtins.c:2308-2343`, registered once by
`cmd_init()` (`builtins.c:2347-2350`). The count checks out against the snippet in
`docs/MAP.md:32-37`. Two more command functions are compiled but **not in the table**,
so they cannot be reached: `c_out` (`builtins.c:383-393`) and `c_usbtest`
(`builtins.c:1236-1300`).

**File key.** `cmd.c`, `builtins.c`, `cmd.h`, `lane_name.h` and `secret_line.h` are in
`firmware/components/cmd/` (headers under `include/`). `main.c`, `editor.c`, `ask.h`,
`ui_text.h`, `battery.c`, `vitals.c` and `view.c` are in `firmware/main/`. Every other
file is under `firmware/components/<component>/`: `seq.c`/`seq.h`/`seq_scale.h`,
`net.c`/`osc.c`/`osc_pack.h`, `ensemble.c`, `blemidi.c`, `ble_kbd.c`/`kbd.h`,
`dinmidi.c`, `usbdev.c`, `buffer.c`/`journal.c`/`sdmirror.c`/`mirror_path.h`/`docstore.h`,
`viz.c`/`viz.h` and `ssh.c`.

**Terms.**
- **Prints** means lines that `cmd_out()` appends to the `+out` buffer; they are also
  logged.
- **Status** means the final value of `ctx->msg`, the one-line result shown on the
  status line.
- Strings are quoted verbatim from the code. `%d` and similar are filled in at run time.
- **Inferred** means read from the code but not observed on hardware.
- **UNVERIFIED** means it cannot be settled from the code.

## 1. The dispatcher (`cmd.c`)

### 1.1 Who runs a line

| caller | who uses it | where |
|---|---|---|
| `CMD_BY_HANDS` | Ctrl+Enter on the line under the cursor. The serial cable feeds the same editor (`kbd.h:106-108`). | `editor.c:1146-1149` → `editor.c:1061` |
| `CMD_BY_GUIDE` | the document named `boot`, one line at a time at startup | `main.c:307-322` (called at `main.c:695`) |
| `CMD_BY_GUIDE` | `>run` | `builtins.c:121` |
| `CMD_BY_AGENT` | **nothing.** It is defined (`cmd.h:53-57`) and has a permission set (`cmd.c:25`), but nothing in the firmware calls with it. | — |

### 1.2 From a line to an action: `cmd_run_line()`, `cmd.c:214-313`

1. **The sigil.** Leading spaces and tabs are skipped. If the next character is not `>`,
   the line is prose: it returns `CMD_DONE` and prints nothing (`cmd.c:229-234`).
   Ctrl+Enter on such a line never reaches the dispatcher; the editor shows
   `"not a command - start with >"` instead (`editor.c:1047-1050`, `ui_text.h:28`).
2. **Blank lines and comments.** Spaces and tabs after the `>` are skipped. A bare `>`,
   or `>` followed by `#`, returns `CMD_DONE` silently (`cmd.c:240-245`). A `#` line
   without `>` is just prose; the shipped `boot` text uses those (`ui_text.h:102-103`).
3. **The first word** runs up to a space, a tab, `=` or the end of the line
   (`first_word`, `cmd.c:110-118`). Everything after it, with leading spaces and tabs
   skipped, is `ctx->arg`: **one unparsed string**. Trailing whitespace is not trimmed
   (`cmd.c:248-254`).
4. **`=` is checked first.** If `arg` starts with `=`, the line is a definition and the
   verb table is not searched (`cmd.c:256-262`). So `>conga=note 63` is the same as
   `>conga = note 63`. A line such as `>bpm = note 3` reaches `cmd_define()` and is
   refused with `"%s is a command"` (`builtins.c:785-788`).
5. **Verb lookup** is an exact, full-length, case-sensitive match against the 34 names
   (`cmd.c:262-264`). There is no prefix matching and no instance digit, so `>bpm140`
   is not `bpm`.
6. **Capability check** (see §1.3). A refusal prints `"%s: not permitted here"` and
   returns `CMD_ERROR` (`cmd.c:265-269`).
7. **The verb runs** with `ctx.name` set to its table name. Its `err_at` and `secret_at`
   offsets are converted into columns of the original line (`cmd.c:270-283`).
8. **Not a verb.** If the line is a definition, or `cmd_lane_known(word)` is true, it is
   handled as a definition or a lane (§1.7). Either needs `CMD_CAP_EDIT`; the refusal is
   `"%.*s: not permitted here"` (`cmd.c:287-304`).
9. **Otherwise**, `old_spelling()` prints a hint (§1.8) and the line returns `CMD_ERROR`
   (`cmd.c:306-312`).

**How a lane is recognised** (`cmd_lane_known`, `builtins.c:499-512`):
- The base is the word up to its first `:`, at most 8 characters. A longer base means
  "not a lane".
- The word is a lane if the base is a **defined name** or a **picture**. Defined names
  live in a 32-slot alias table (`builtins.c:468-484`), which includes knob and pad
  inputs.
- The 16 pictures are `echo move spin warp noise disc box turn ramp grid mask edge grow
  thin flip fold` (`viz.c:62-68`).
- The part after the `:` is validated later, by `cmd_lane()`.

**How the editor marks lines.** It calls `cmd_recognise()` (`cmd.c:134-174`) and marks a
verb, a definition (`"="`) or a lane differently from prose. An unknown first word looks
like prose (`cmd.h:104-115`; `editor.c:561`, `editor.c:579`).

**Line length.** Every caller copies the line into 128 bytes:
- The editor keeps the first 127 characters and drops the rest (`editor.c:922-941`,
  `editor.c:1040`).
- The boot runner and `>run` cut at 127, **drop the 128th character**, and treat the
  remainder as a new line (`main.c:307-322`; `builtins.c:112-131`).

**Robustness.** `cmd_run_line` dereferences `line` at `cmd.c:229` before its NULL check
at `cmd.c:237`, so the check can never catch anything.

### 1.3 Capability classes

The bits (`cmd.h:43-48`):

| bit | value | meaning (header) |
|---|---|---|
| `CMD_CAP_READ` | 0x01 | inspects state, changes nothing |
| `CMD_CAP_EDIT` | 0x02 | mutates a buffer |
| `CMD_CAP_STORE` | 0x04 | writes flash or the card |
| `CMD_CAP_NET` | 0x08 | reaches off the device |
| `CMD_CAP_SYSTEM` | 0x10 | changes device state: pairing, power |

What each caller may use (`caller_caps`, `cmd.c:19-28`):

| caller | permitted bits |
|---|---|
| `BY_HANDS` | all (`0xFFFFFFFF`) |
| `BY_GUIDE` | READ, EDIT, STORE, NET — no SYSTEM |
| `BY_AGENT` | READ, EDIT, STORE — no NET, no SYSTEM |
| any other value | READ |

A verb is refused if **any** of its bits is missing from the caller's set (`cmd.c:265`).
The check applies to the whole verb, so read-only forms of SYSTEM verbs (a bare `>send`,
`>kbd`, `>usb` or `>din`) are also refused to `boot` and `>run`.

The 37 verbs by class:

| class | verbs |
|---|---|
| READ (7) | battery, dump, lanes, jitter, help, list, open |
| EDIT (18) | bpm, scale, swing, sync, split, route, play, stop, panic, mute, solo, toggle, clear, map, new, run, close, density |
| EDIT and STORE (1) | name |
| STORE (1) | save |
| NET (5) | wifi, host, osc, ssh, frame |
| SYSTEM (5) | send, kbd, usb, din, flash |

**Where the class does not match what the code does:**
- `battery` is READ, but `battery use` writes NVS (`battery.c:117-126`).
- `sync` is EDIT, but `lead` and `follow` bring up Wi-Fi and ESP-NOW
  (`ensemble.c:688-746`). The comment at `cmd.c:15-17` says an agent "may not touch the
  radio".
- `run` is EDIT, but it runs every line as `BY_GUIDE` whoever called it
  (`builtins.c:121`). An agent could therefore reach NET verbs by way of a document.
- `wifi forget`, `ssh forget` and ssh's first-contact key store all write NVS, yet all
  three are NET, not STORE.
- `open` is READ, but it changes which buffer is current.

### 1.4 What a command reports

**Printing.** `cmd_out()` (`cmd.c:68-98`) formats a line of up to 159 characters, logs it
(`ESP_LOGI`, tag `"cmd"`) and appends it to `+out`, creating that buffer if needed.
- Before the **first** printed line of each command, it appends a separator line of 30
  dashes: `"------------------------------"`.
- The first printed line also becomes `ctx->msg` if the command has not set one.

**Status.** `ctx->msg` is 96 bytes. Many verbs overwrite the first-line default with
`snprintf`; the **Status** entries below give the final value.

**Hand-run lines** (Ctrl+Enter) also get this from the editor (`editor.c:1038-1091`):
- The status line shows `msg`, or `"ok"` if `msg` is empty.
- If the command printed **more than one line**, the view jumps to `+out` at the first new
  line. The status line shows `msg` first, then `"output - Ctrl-O goes back"`
  (`editor.c:1001-1028`, `ui_text.h:40`).
- If it printed at most one line, `err_at` boxes the offending character in the
  document.
- `secret_at` cuts the line from that column to the end (`editor.c:1064-1066`).

**`boot` and `>run`** throw away `msg`, `err_at` and `secret_at` (`main.c:315`;
`builtins.c:121`).

**Return codes.** Verbs return `CMD_DONE`, `CMD_ERROR`, or `CMD_PENDING`. Only `wifi`,
`host` and `ssh` return `CMD_PENDING`, and only after asking for a password.

### 1.5 Asking for a password (`wifi`, `host`, `ssh`)

- **No password on a line.** `wifi` and `host` take exactly one word (`net_name`,
  `builtins.c:1906-1923`; `first_word_rest`, `secret_line.h:20-45`).
  - A second word is refused with status `"no passwords on a line - cut"`. `secret_at` is
    set so the editor deletes it.
  - A word wrapped in `<…>` is unwrapped (`secret_line.h:51-60`).
- **The prompt.** The verb calls `cmd_ask_secret(question, fn)` (`cmd.c:46-49`) and
  returns `CMD_PENDING` with status `"type it, Enter. Esc stops"`.
  - The editor's asker (`editor.c:197-203`) takes over the status line as
    `"<question>: ****"` (`ask.h:91-105`).
  - It accepts up to 63 printable ASCII characters (`ask.h:24`, `ask.h:79-82`).
  - Enter submits. Esc, Ctrl-C or Ctrl-G cancel, with status `"stopped - nothing sent"`
    (`ask.h:59-78`; `editor.c:1101-1114`). The text is wiped either way.
- **Not at boot.** The asker is installed by `editor_init()`, which runs **after** the
  boot document (`main.c:695` and `main.c:704`; `editor.c:205-209`). At boot, therefore,
  `cmd_ask_secret` returns false and the verb reports `"nothing here can ask"`
  (`builtins.c:1929-1932`, `builtins.c:1871-1874`).

### 1.6 How arguments are parsed

The style of parsing explains which typos a verb accepts:

| style | behaviour | used by |
|---|---|---|
| exact `strcmp` | a trailing space makes the word fail to match | `now`, `on`, `off`, `reset`, `forget`, `lead`, `follow`, `alone`, `in … off` |
| `whole_number()` (`builtins.c:917-930`) | `strtol`, then only whitespace, then a range check | bpm, swing, osc ports, ssh ports |
| `atoi` | lenient: `17x` reads as 17, a word reads as 0 | din, split rows |
| first character only | the rest of the word is ignored | density |
| `two_words()` (`builtins.c:1476-1487`) | the first word, then **the rest of the line**, spaces included, as the second | osc, route |

### 1.7 Lanes and definitions

These are the dispatcher's other two outcomes. They are not verbs.

**Definitions: `cmd_define`, `builtins.c:767-876`; grammar in `lane_name.h:172-290`.**

Forms:
- `>n = note N [ch C] [gate G]`
- `>n = voice O [ch C] [gate G]`
- `>n = cc N [ch C]`
- `>n = knob` or `>n = pad`
- `>n = <picture>` (an alias for a picture)
- `>n =` (forget the name)

Defaults (`lane_name.h:218-233`):
- note: channel 10, gate 40 ms
- voice: channel 1, gate 150 ms
- cc: channel 1

Refusals and results:
- An instance or a part in the name: `"define the plain name: %s"`.
- The name of a verb: `"%s is a command"`. The name of a picture:
  `"%s is a picture already"`.
- Parse errors:
  - `"voice takes an octave 0-8"`, `"note takes 0-127"`, `"cc takes 0-127"`
  - `"ch is 1-16"`, `"gate is 1-5000 ms"`, `"'%.10s'? ch N or gate N"` (this last one also
    catches `gate` on a cc)
  - `"knob takes nothing else"`, `"pad takes nothing else"`
  - `"note, voice, cc or a picture"`, `"a picture takes nothing after"`,
    `"no picture is that long"`, `"no picture called %s"`
- Tables full: `"%d names is all there is"` (32); `"%d inputs is all there is"` (16).
- Forgetting: `"no name %s"`, or `"%s is not a name now"`, which also forgets that name's
  lanes.
- Success: status `"%s =%s"`, and every live lane of that name is re-bound
  (`builtins.c:745-764`).

**Lanes: `cmd_lane`, `builtins.c:663-743`.**
- The address grammar and its errors are in `lane_name.h:78-166`:
  - `"a name is letters: conga"`, `"a name is 8 letters at most"`
  - `"a second one is :2 to :99"`, `"a part is a word: disc:x"`
  - `"number, then part: disc:2:x"`, `"a part is :x now, not [x]"`
- Binding errors (`builtins.c:517-592`):
  - `"%s? try: help"`
  - `"%s is a knob: route from it"`, or the same with `pad`
  - `"no :%s - a picture has x, y"`
  - `"a voice has :vel and :oct"`, `"a drum has :vel"`, `"%s has no parts"`
- **A bare name deletes the lane**: status `"%s gone"`, or `"no %s"` if there was none
  (`builtins.c:690-694`).
- **Re-run removes** (`rerun_silences`, since 2026-09-29). If the caller is `BY_HANDS`,
  the transport is running, the lane is not muted, and the pattern text is byte-for-byte
  what was last compiled, the lane is **removed** (`seq_forget`) instead of recompiled.
  Status `"%s off - again for on"`: the same line once more compiles it afresh. It used
  to mute and keep the slot.
- A direction (`u d l r`) on anything except move, warp, ramp or turn is refused and
  boxed: `"u d l r: move warp ramp turn"` (`builtins.c:700-724`).
- No free lane (16 in total, `seq.h:39`): `"16 lanes is all there is."` and
  `"free one: type its name alone"` (`builtins.c:646-652`).
- A pattern error prints the compiler's reason, boxed (`builtins.c:653-657`). Too many
  notes gives `"%d notes: %d fit a lane"` (`seq.c:1321`).
- **Success.** The lane is unmuted, and a picture lane turns the split on
  (`builtins.c:732-735`). Status `"%s %s"`, or `"%s %s in %s"` for a voice, where the last
  field is the key.

### 1.8 `old_spelling()`: hints for unknown words (`cmd.c:176-212`)

Checked in this order:

1. The word contains `[` and the text before it is a lane:
   `"%.*s[%.*s] is %.*s:%.*s now"`, for example `disc[x] is disc:x now`. The bracket
   contents run to `]`, or to the end of the word if there is none.
2. The word ends in digits and the text before them is a lane:
   `"%.*s is %.*s:%.*s now"`, for example `disc2 is disc:2 now` or `kick2 is kick:2 now`.
3. The word starts with `cc` (any such word, for example `cc74`):
   `"cc is a kind now: >fx = cc 74"`.
4. The word is exactly `guide` or `prose`: `"%.*s is gone: > marks a line"`.
5. Anything else: `"%.*s? try: help"`.

## 2. The verbs, in table order

Each heading gives the table row: the capability class and the help text shown by
`>help`.

### 2.1 `bpm` · EDIT · "tempo" · `builtins.c:2128-2144`

**Purpose:** set or show the tempo.

**Forms**
- `>bpm` reports the tempo.
- `>bpm N` sets it. N must be a whole number from 20 to 300, followed only by whitespace
  (`builtins.c:2136`). Otherwise it prints `"bpm is a number, 20-300"`, returns
  `CMD_ERROR`, and the tempo is unchanged.

**Status:** `"%d bpm"`.

**Effects** — `seq_bpm` (`seq.c:1482-1520`):
- Clamps to 20–300.
- If playing, re-anchors the grid while keeping the bar position (lanes do not restart).
  If stopped, resets the tick to 0.
- Re-arms the timer.
- **Clears the `>jitter` statistics** (`seq.c:1519`).

**Interactions:** on a following deck, the leader's tempo overrides `>bpm` at the next
correction while playing (`seq.c:1000-1005`, `seq.c:1048-1051`). The shipped `boot`
document sets 124 (`ui_text.h:98`).

### 2.2 `scale` · EDIT · "dmin | c | f#mix | apent" · `builtins.c:901-912`

**Purpose:** set or show the key that voice lanes play in.

**Forms**
- `>scale` reports the key.
- `>scale <spec>` parses the spec with `seq_scale_parse` (`seq_scale.h:67-103`):
  - A root letter a–g, in either case.
  - Optionally `#`, or `b` for flat — but `b` counts as a flat only if it does not start a
    mode name, so `cblues` is C blues.
  - Then a lowercase mode, matched **by prefix** (`seq_scale.h:49-57`):
    `chrom blues pent maj5 maj min dor phr lyd mix loc`. With no mode the key is minor.
  - Because the match is a prefix, anything after a matching mode is not checked:
    `dminor` is accepted as D minor.
- An invalid spec prints four lines and returns `CMD_ERROR`, with the key unchanged:
  - `"a root a-g, then # or b, then one of:"`
  - `"  maj min dor phr lyd mix loc"`
  - `"  pent maj5 blues chrom"`
  - `"e.g. dmin  c  f#mix  apent  ebblues"`

  The status is the first of those lines.

**Status:** `"key of %s"`, where `%s` is the spec as typed, cut to 11 characters
(`seq.c:249`, `seq.c:262`).

**Effects:**
- Sets the root and the mode.
- Melodic lanes transpose at their next note, because a degree is resolved when it sounds
  (`seq.h:239-248`).
- The default is `cmin` (`seq.c:247-249`); `boot` sets `dmin` (`ui_text.h:99`).

### 2.3 `swing` · EDIT · "50 straight, 67 triplet" · `builtins.c:932-946`

**Forms**
- `>swing` reports the swing.
- `>swing N` sets it; N must be a whole number from 50 to 75. Otherwise it prints
  `"swing is 50-75: 67 is triplet"` and returns `CMD_ERROR`.

**Status:** `"swing %d%s"`, with `" (straight)"` appended at 50 and `" (triplet)"` at
66–68.

**Effects:** `seq_swing` clamps to 50–75 (`seq.c:1453-1458`). Odd sixteenths are delayed
(`seq.h:294-296`). The statistics are not reset. `boot` sets 50 (`ui_text.h:100`).

### 2.4 `sync` · EDIT · "on | off | lead | follow | alone" · `builtins.c:959-1046`

**Purpose:** two things under one verb (`builtins.c:948-958`). `on` and `off` control MIDI
clock out; `lead`, `follow` and `alone` control the ESP-NOW ensemble of decks.

**Forms**

**`>sync on` / `>sync off`** call `seq_sync` (`seq.c:1461-1468`).
- If the transport is running, turning clock on sends Start (0xFA) and turning it off
  sends Stop (0xFC).
- While clock is on, `play` sends Song Position 0 (0xF2) then Start (`seq.c:1552-1557`),
  and `stop` sends 0xFC (`seq.c:1562-1564`). Clock bytes (0xF8) go out at 24 PPQN
  (`seq.h:75-77`).
- Status: `"midi clock on"` or `"midi clock off"`.

**`>sync lead` / `>sync follow`** call `ensemble_set` (`ensemble.c:669-759`). On the first
call this:
- initialises netif and the Wi-Fi driver, **sets Wi-Fi to station mode**, and starts
  Wi-Fi;
- **turns Wi-Fi power save off**;
- initialises ESP-NOW with a broadcast peer.

On every call it resets the ensemble statistics.
- Failure prints `"the radio would not start"` and returns `CMD_ERROR`.
- Success prints `"leading the ensemble."` (or `"following the ensemble."`), then
  `"no network needed - the decks"`, `"talk to each other directly."` and
  `"one deck leads, the rest follow."`.
- Status: `"sync lead"` or `"sync follow"`.

**`>sync alone`** calls `ensemble_set(ENSEMBLE_OFF)`; its result is ignored. This de-initialises
ESP-NOW and ends follow mode (`ensemble.c:674-685`). **Wi-Fi is not stopped and power save
is not restored**: `ensemble.c` has no `esp_wifi_stop` and no second `set_ps`. Status:
`"sync alone"`.

**`>sync`** alone prints a report:
- `"midi clock out: %s"`.
- If the ensemble is on, `"%s, %d other deck%s"` (leading or following).
- **When following:**
  - `"off by %d us, %u packets"`
  - `"best trip %d us, %u skipped"`
  - `"probes agreed within %d us"`
  - once the leader's count is known, one of: `"in the leader's count"`,
    `"steps together, bar %d off"`, `"steps %d pulses apart"`
  - `"%u replies, %u twice, %u lost ack"`
  - `"%u corrections, %u stale"`
- **When leading:** `"%u packets since asked"`.
- **When off:** `"playing alone"`.
- It always ends with `"sync on | off   midi clock"` and `"sync lead | follow | alone"`.

Reading the report resets the packets-heard counter (`ensemble.c:796`). Status:
`"clock on"` or `"clock off"`.

**Anything else** prints `"sync on | off | lead | follow | alone"` and returns
`CMD_ERROR`.

**Interactions**
- A follower takes the leader's tempo and its bar count. `>play` on a follower stays
  silent for up to about one second, until the count arrives (`seq.c:1549-1550`,
  `seq.h:379-385`).
- Setting station mode (`ensemble.c:703`) would end an access point started by `>host`.
  Inferred, UNVERIFIED on hardware.
- While the ensemble is on, the keyboard scan shares the radio (`main.c:865-867`).
- `>osc in off` turns power save back on only if the ensemble is off
  (`builtins.c:2032-2036`).

### 2.5 `send` · SYSTEM · "where events go; send mon on" · `builtins.c:2057-2126`

**Destinations.** Registered at `main.c:544-556`, `main.c:666-677` and `view.c:34-37`.
All start **off** (`seq.c:1607`).

| name | help as listed | what it is |
|---|---|---|
| `ble` | "BLE MIDI (off by default)" | BLE MIDI. The main loop keeps advertising in step with this flag, and turning it off disconnects a connected host (`main.c:820-824`; `blemidi.c:226-245`). |
| `mon` | "echo notes to console" | logs note on/off and CCs — not clock, not CC 123 — with tag `"midi"` (`main.c:223-243`) |
| `din` | "DIN/TRS MIDI - set >din <gpio>" | UART MIDI; silent until `>din <gpio>` (`dinmidi.c:98-100`) |
| `osc` | "OSC /deck/<lane>" | UDP; silent until `>osc <ip>` (`osc.c:94-96`) |
| `view` | "the picture to an HDMI node" | frames to the console as `ESC ] view;<base64> BEL` (`view.c:9-14`, `view.c:53-76`) |
| `usb` | "USB MIDI (native)" | exists only after booting into USB MIDI mode, and is switched on then (`main.c:666-677`) |

**Forms**

**`>send`** prints two lines for each destination:
- `"%-4s %s"`: the name, then `ON` or `off`.
- `"     %.25s"`: the help text, cut to 25 characters.

Then it prints two USB lines:
- `"usb: %s"`, where `%s` is `"act%d dev%d midi%d s%u d%u"`: USB mode active, device
  mounted, MIDI interface mounted, messages sent, messages dropped (`usbdev.c:172-183`).
- The legend `"act=mode dev=host midi=bound"`.

Status: `"%d destination%s"` — 5 in serial mode, 6 in USB MIDI mode.

**`>send <name>`**: status `"%s is %s"`, on or off. An unknown name reports `off`, not an
error (`seq.c:1631-1635`).

**`>send <name> on|off`**:
- Status `"%s on"` or `"%s off"`.
- An unknown name prints `"no destination called '%s'. try just: send"` and returns
  `CMD_ERROR`.
- Any other state word prints `"send <name> on | off"` (for `view`,
  `"send view on|off|80x30|mode"` and the six modes) and returns `CMD_ERROR`.

**`>send view <W>x<H>`** turns view on at that size.
- W must be 4–80 and H 2–30 (`VIZ_W`, `VIZ_H`); otherwise it prints
  `"view is 4x2 to %dx%d cells"`, which reads 80x30.
- `on` pins the frame at W×H, or 80×30 if no size is given and the view was off. The preview then shows a
  sample of that frame, and the split is turned on.
- `off` returns sizing to the pane; the split is left as it is.
- Code: `builtins.c:2093-2119`; `viz.c:180-187`.

**`>send view <mode>`** - plain, scan, phosphor, feedback, riso, poster - turns view on
and names how the HDMI node draws it (`viz_out_mode`; `docs/VIEW.md`).

**Parsing:** `sscanf "%15s %15s"` (`builtins.c`, `c_send`). Further words are ignored.

**Effects:** it flips a flag and nothing more, except for `ble` (the radio, via the main
loop) and `view` (frame size and split).

**Interactions**
- `>din <gpio>`, `>din off` and `>osc <ip>` also flip their destination's flag
  (`builtins.c:170`, `builtins.c:222`, `builtins.c:2052`).
- Being SYSTEM, `send` is refused in `boot` and in `>run`: `"send: not permitted here"`.
  A boot document therefore cannot switch on ble, mon, view or din.
- The `to:` line of `>lanes` lists the enabled destinations.

### 2.6 `wifi` · NET · "wifi <ssid> | off - asks the pass" · `builtins.c:1937-1968`

**Forms**

**`>wifi`** prints:
- the network status (`net.c:40-48`): `"wifi off"`, or `"%s %.12s %s"` — `join` or
  `host`, the SSID (up to 12 characters), and the IP address or `-`;
- `"remembered: %.17s"`, if an SSID is stored;
- `"wifi <ssid> - then it asks"`, `"for the password."`, `"wifi off | wifi forget"`.

Status: the network status line.

**`>wifi forget`** erases the NVS keys `deck/ssid` and `deck/pass` (`net.c:113-123`). The
radio is left alone. Status: `"network forgotten"`.

**`>wifi off`** calls `esp_wifi_stop` and clears the link state (`net.c:251-260`). The
driver and the stored credentials are kept, so the deck **rejoins at the next boot**
(`main.c:687-693`). Status: `"wifi off"`.

**`>wifi <ssid>`** takes one word of up to 32 characters, then asks for the password
(§1.5).
- An empty or over-long word gives status `"wifi <ssid>"`.
- The prompt is `"%.24s password"`; the verb returns `CMD_PENDING`.
- On Enter it calls `net_join` (`net.c:172-208`):
  - It **writes the SSID and password to NVS before joining** (`net.c:159-170`,
    `net.c:190-193`).
  - It stops whatever Wi-Fi mode was running and switches to station mode.
  - It requires WPA2-PSK if a password was typed, and allows an open network if not.
  - It connects, and on every disconnect it retries, indefinitely (`net.c:56-61`).
- Status: `"joining %.20s"`, or `"could not start the radio"`.

**Notes**
- An SSID containing a space, or one literally named `off` or `forget`, cannot be joined.
- In `boot`, `>wifi <ssid>` gives `"nothing here can ask"`. The remembered network is
  rejoined before `boot` runs in any case (`main.c:691`).
- While Wi-Fi is on, the keyboard scan shares the radio (`main.c:865-867`).

### 2.7 `battery` · READ · "find the sense pin" · `builtins.c:1496-1531`; `battery.c`

**Forms**

**`>battery`** prints:
- `"cell %dmV = %d%%"`, if a channel is configured and reads a value;
- a scan of the candidate ADC1 pins — GPIO 1–4 and 6–10, with GPIO 5 excluded
  (`battery.c:14-18`) — as `"g%d:%dmV "` entries, wrapped at 28 characters per line
  (`"%.28s"`);
- `"unplug USB, run again: the one"`, `"that moves is it. then:"`,
  `"  battery use <gpio>"`.

Status: `"scanned ADC1"`.

**`>battery use <gpio> [divider×10]`**:
- Any argument that begins with `use` matches (a 3-character `strncmp`).
- `sscanf "%d %d"`; the GPIO defaults to 0 and the divider to 20 (a 2:1 network).
- There is **no validation**: `deck/bat_gpio` and `deck/bat_div` are written to NVS
  exactly as given (`battery.c:117-126`).
- In RAM, only GPIO 1–10 is used (anything else means "forgotten"), and a divider outside
  10–100 falls back to 20.
- Prints `"GPIO%d: %dmV = %d%%"`, or `"GPIO%d reads nothing"`.
- Status: `"battery on GPIO%d"`. `use 0`, or `use` alone, forgets the pin.

**Effects**
- An NVS write, from a READ verb.
- The scan configures every candidate pin as an ADC input (`battery.c:89-96`). What that
  does to a pin with another job — for example a `>din` pin in the 1–10 range — is
  UNVERIFIED.

### 2.8 `kbd` · SYSTEM · "what is typing | kbd forget" · `builtins.c:243-281`

**Forms**

**`>kbd`** prints:
- `"a keyboard is connected"` or `"no keyboard connected"`;
- `"state: %.20s"` — one of `starting`, `discovering`, `connecting`, `pairing`,
  `connected`, `no reports`, `scanning` or `reconnecting` (`ble_kbd.c:83`, 503, 520, 653,
  754, 774, 858, 915, 964);
- `"bonds: %d remembered"`, or `"bonds: the store did not say"`;
- if not connected, three more lines: `"it is paired but not in range"` (if there are
  bonds) or `"nothing has ever paired here"`, then `"the cable is a keyboard too"` and
  `"kbd forget to pair another"`.

Status: `"%s, %d bond%s"`, where the first field is `connected` or `not connected`.

**`>kbd forget`** calls `kbd_forget_all` (`ble_kbd.c:1022-1041`), which:
- ends the current BLE connection;
- clears the **whole** NimBLE bond store (`ble_store_clear()`);
- erases the NVS key `deck/peer`;
- starts scanning again.

It prints `"bonds dropped, scanning again."`, `"put the keyboard in pairing"`,
`"mode now. any HID keyboard"` and `"works - full size included."`. Status:
`"forgotten - pair one now"`.

**Anything else** prints `"kbd          what is typing"` and
`"kbd forget   pair a different one"`, and returns `CMD_ERROR`; the status is the first
line.

**Notes:** the bond count is every bonded peer in the NimBLE store (`ble_kbd.c:678-687`).
Holding KEY for 2 s does the same as `kbd forget` (`main.c:710-711`; `kbd.h:91-99`).

### 2.9 `host` · NET · "host <ssid> - be the net" · `builtins.c:1970-1984`

**Forms**

**`>host`** prints `"host <ssid> - then it asks"`, `"for a password. under 8"`,
`"chars, or none, and it hosts"` and `"open, and says so."`. Status: `"host <ssid>"`.

**`>host <ssid>`** follows the same one-word rule as `wifi`, then asks `"%.24s password"`
and returns `CMD_PENDING`. On Enter it calls `net_host` (`net.c:210-249`), which:
- stops whatever Wi-Fi mode was running;
- starts an access point on channel 6, for at most 4 clients;
- uses WPA2-PSK if the password is at least 8 characters, and is **open** otherwise;
- serves at 192.168.4.1.

Status: `"hosting %.12s 192.168.4.1"`, `"hosting %.12s OPEN"`, or
`"could not start the radio"`.

**Notes**
- **Nothing is remembered**: `net_host` writes no NVS. After a reboot the deck rejoins the
  remembered station network instead, if there is one.
- There is **no `host off`**. `>host off` asks for a password for a network named "off";
  use `>wifi off` instead.
- In `boot`, `>host` gives `"nothing here can ask"`, so a boot document cannot start a
  hosted network.

### 2.10 `osc` · NET · "osc <ip> <port> - /deck/<lane>" · `builtins.c:1986-2055`; `osc.c`

**Forms**

**`>osc`** prints:
- `"osc <ip> <port>"`, `"sends /deck/<lane> i i"`, `"%u msgs in %u packets"`;
- if listening: `"in on %d: %u read, %u used"`, followed by `"  %u datagrams refused"`
  if any were refused;
- if not listening: `"osc in <port> - /deck/<name>"` and `"sets a knob or a pad"`.

Status: `"osc 192.168.4.2 9000"`. This is a fixed example, not the current state.

**`>osc <ip> [<port>]`**:
- The arguments are split by `two_words`. The port is optional and defaults to 9000; it
  must be a whole number from 1 to 65535, otherwise `"a port is 1-65535"`.
- `net_osc_target` **closes the existing socket first** (`osc.c:61-88`), then parses a
  dotted IPv4 address with `inet_pton` and opens a non-blocking UDP socket.
- A bad address, or a failed `socket()`, both give `"'%s' is not an address"`.
- Success switches on the `osc` destination. Status: `"osc -> %.15s:%ld"`.

What gets sent (`osc.c:90-131`, `osc.c:156-170`):
- note-ons with velocity above 0, and CCs, as `/deck/<lane> ii` (d1, d2);
- the step marker, as `/deck/step i`;
- not note-offs, and not clock.

There is one datagram per drained step.

**`>osc in <port>`**:
- The port must be a whole number from 1 to 65535.
- The network must be up (joined with an address, or hosting); otherwise status
  `"osc in needs wifi"`.
- `net_osc_listen` (`osc.c:222-265`) stops any previous listener (waiting up to 1.5 s),
  binds UDP on all addresses, and starts the task `oscin`.
- It **turns Wi-Fi power save off**.
- Status: `"osc in on %ld"`; on failure, `"could not listen"`.

**`>osc in off`** stops listening. Power save goes back on only if the ensemble is off.
Status: `"osc in off"`.

**`>osc in`** with no port, or with anything invalid, gives status
`"osc in <port> | off"` and returns `CMD_ERROR`.

**Incoming messages.** `/deck/<name>` with a value, scaled to 0–127, sets the input with
that name (a knob or pad definition). An unknown name is counted but creates nothing
(`osc.c:189-199`).

**Notes**
- There is **no `osc off`**. `>osc off` closes the current target socket (a side effect of
  `net_osc_target`), then reports `"'off' is not an address"`, leaving the destination
  flag on. The clean way to stop sending is `>send osc off`.
- Sending out does not check `net_up()`. Whether `socket()` works before Wi-Fi has ever
  been started is UNVERIFIED.

### 2.11 `ssh` · NET · "ssh user@host <command>" · `builtins.c:1801-1877`; `ssh.c`

**Forms**

**`>ssh`** prints nine lines: `"ssh user@host <command>"`, `"asks for the password: it"`,
`"never goes on the line."`, `"the reply lands in +out."`, `"a host's key is kept the"`,
`"first time, and a changed"`, `"key is refused before any"`,
`"password is sent. if you"`, `"changed it: ssh forget <host>"`. Status:
`"ssh user@host ls"`.

**`>ssh forget <host>[:<port>]`**:
- It takes exactly one word after `forget`; otherwise status `"ssh forget <host>"` and
  `CMD_ERROR`.
- The port must be 1–65535 (status `"a port is 1-65535"`) and defaults to 22.
- It erases that key from the NVS namespace `sshkeys` (`ssh.c:141-156`).
- Prints `"key forgotten: %.20s:%d"`, or `"no key kept for %.20s:%d"`.
- Status: `"key forgotten"` or `"no key kept"`. This form needs no network.

**`>ssh <user>@<host>[:<port>] <command>`** checks, in order:
1. The network is up; otherwise it prints `"no network. try: wifi <ssid>"` with status
   `"ssh needs wifi"`.
2. No other session is running; otherwise status `"one ssh at a time"`.
3. A command is present and the address is well formed; otherwise status
   `"ssh user@host <command>"`. The user and host must be non-empty. The user is silently
   cut to 32 characters and the host to 63 (`builtins.c:1780-1799`). The command is cut
   to 127 characters.

It then asks `"%.24s password"` (with the host) and returns `CMD_PENDING`. On Enter:
- If the typed password appears anywhere in the command, **nothing is sent**, with status
  `"that password is on the line"`.
- Otherwise `ssh_start` runs, with status `"ssh: connecting"`, `"one ssh at a time"`, or
  `"ssh could not start"` (`builtins.c:1762-1777`).

**The session** runs in its own task and writes to `+out` (`ssh.c:199-372`):
- First, `"$ <command>"`.
- Then either the reply — up to 200 lines, then `"... truncated at 200 lines"` — or one of
  these failures:
  - `"cannot resolve that host"`, `"no answer in 5 seconds"`, `"connection refused"`
  - `"handshake failed"`, `"the host showed no key - refused"`
  - `"THE HOST KEY HAS CHANGED."` / `"refused - no password sent."` /
    `"if you changed it yourself:"` / `">ssh forget <host>"`
  - `"password refused"`, `"server refused a channel"`, `"could not run it"`
- It also prints the host key's type and fingerprint, and either
  `"first time here: keeping it"` or `"the key it had last time"`.

**Effects:** the first contact with a given host and port writes the key fingerprint to
NVS (`sshkeys`) when the session ends (`ssh.c:450-459`). A changed key is refused before
any password is sent.

### 2.12 `frame` · NET · "send the frame over osc" · `builtins.c:1701-1751`

**Forms**

**`>frame` while a picture is live** (`viz_active`: something has been drawn and not yet
blanked; `viz.c:241-245`):
- The frame is converted to ASCII: tones become one of `" ..::*#@@"`, and other tiles
  become `*` or `#` (`viz.c:1077-1115`).
- It is sent as one OSC message, `/deck/frame s`, in its own datagram
  (`osc.c:133-154`).
- Status: `"sent a %d-byte frame"`.

**`>frame` with no picture live** sends the current document, silently cut at 1023 bytes.
Status: `"sent %u bytes as a frame"`.

**`>frame <name>`** sends that document, even while a picture is live. An unknown name
gives `"no document called '%s'"`.

**Errors**
- No OSC target: `"no osc target. try: osc <ip> <port>"`.
- Any other failure on the document path: `"frame too large or send failed"`.
- **Picture path bug:** if the send fails, or the message is too big for the 1100-byte
  buffer (`osc.c:141-146`) — for example a 60×24 frame, which is 1464 bytes — the verb
  returns `CMD_ERROR` but the status still reads `"sent a %d-byte frame"`
  (`builtins.c:1710-1716`).

### 2.13 `split` · EDIT · "split on | off | <rows>" · `builtins.c:1556-1596`

**Forms**
- `>split` toggles the split.
- `>split on` and `>split off` set it.
- `>split <n>`: if `atoi` gives more than 0, the picture gets n rows and the split is
  turned on. Otherwise it prints `"split on | off | <rows>"` and returns `CMD_ERROR`.

**Layout** (`viz.c:257-283`):
- The picture is always **below** the code, at full width.
- With no row count it takes about half the rows, rounded up; with n it takes n+2 (the
  border counts).
- The code always keeps at least 4 rows, and the picture is at most 24 rows high.

**Status:** `"split off"`, or `"view %dx%d below"`.

**Notes**
- A picture lane turns the split on by itself (`builtins.c:733-735`). Documents should
  therefore say `split on` or `split off`, not a bare `split`, which toggles
  (`builtins.c:1563-1569`).
- The reported size is the frame size from the **last draw**, because `viz_size()` is set
  when the pane is drawn (`editor.c:478-481`). It can lag a change by one draw, and while
  `>send view` is on it reports the pinned output size. Inferred.

### 2.14 `route` · EDIT · "route disc kick" · `builtins.c:1617-1699`

**Purpose:** make one lane (the follower) fire whenever another (the source) fires. It
works for any pair: sounds, pictures, parts and inputs.

**Forms**

**`>route`** prints `"route <lane> <lane it follows>"`, `"route disc kick"`,
`"route grow disc   viz drives viz"`, `"route disc:x bass a part follows"` and
`"route disc       unroutes"`. Status: `"route disc kick"`.

**`>route <follower> <source>`**:
- Both are parsed as addresses and put in canonical form, so `disc:1` is the same as
  `disc`. Address errors are the same as in §1.7.
- If the follower lane does not exist yet, it is bound first (the binding errors of §1.7
  apply). `seq_route` then compiles a placeholder one-step pattern `"x"`, so the lane is
  live (`seq.c:1435-1447`).
- A lane routed to itself: `"%s cannot follow itself"`.
- A follower that could not be found: `"no lane '%.18s' to drive"` and
  `"write it first, then route it"`.
- The source is checked; a problem gives a warning, but the verb still returns
  `CMD_DONE`:
  - `x:end` where lane `x` does not exist: `"no lane '%s' yet - it will"` and
    `"stay silent until there is one"`.
  - `x:end` where lane `x` has no count: `"%s never ends - give it !n"`.
  - Any other source that is neither a lane nor an input: `"nothing called %s yet -"`
    and `"silent until a lane or input is"`.
- Status: `"%s <- %s"`.

**`>route <follower>`** removes the route. Status: `"%s unrouted"`.

**Semantics** (`seq.c:1414-1449`; `fire_lanes`, `seq.c:597-641`):
- A routed lane **without** a count ignores its own steps and fires on every hit of the
  source, at the source's value (a sidechain).
- A routed lane **with** a count (`!n`) is a cue: each source hit starts it for its count.
- For pictures, a routed lane is drawn after its source (`viz.c:55-61`).
- Forgetting a lane clears its route (`seq.c:1194-1198`), and so does `>new`.

**Notes**
- The second word is **the rest of the line**, so a third word makes the source invalid.
- If all 16 lane slots are full, a new follower cannot be bound. The failure of
  `seq_lane_bind` is ignored (`builtins.c:1657`), so the message is the misleading
  `"no lane '<name>' to drive"`.
- Removing the route from a follower that `>route` itself created leaves the placeholder
  `"x"`, which then plays **every step**. Inferred from `seq.c:1439-1447`.

### 2.15 `usb` · SYSTEM · "usb on | off - MIDI over the cable" · `builtins.c:1415-1467`; `usbdev.c`

**Forms**

**`>usb`**, or any argument other than exactly `on` or `off`, prints:
- `"usb is %s%s"` — on or off, with `", host attached"` if mounted;
- `"usb on   MIDI + console, 1 cable"`, `"usb off  back to serial"`;
- `"a power cycle always returns"`, `"to serial. put >usb on in boot"`,
  `"to have it every time."`;
- `"%u failed attempt(s) this power cycle"`, if there were any.

Status: `"usb %s"`.

**`>usb on`**:
- If there have already been 3 failed attempts this power cycle, it prints
  `"USB failed 3x this power cycle."` and `"unplug the deck and try again."`, with status
  `"usb: too many failures"`, and returns `CMD_ERROR`.
- Otherwise it records the intent **in RTC memory, not flash** (`usbdev.c:64-83`,
  `usbdev.c:214-222`), then:
  1. saves every changed document (journal and SD card);
  2. stops the transport;
  3. writes the vitals record to NVS (`deck/vitals`), marked as deliberate
     (`vitals.c:25-50`);
  4. announces `"USB MIDI - rebooting now"`;
  5. waits 600 ms;
  6. **resets the chip without running shutdown handlers**
     (`esp_rom_software_reset_system`, `builtins.c:1114-1118`).

**`>usb off`** clears the intent, then does the same: saves, stops, writes vitals,
announces `"serial console - rebooting now"`, and reboots.

`on` and `off` both reboot **even if the deck is already in that mode**.

**After `on`** (`usbdev.c:11-24`, `usbdev.c:84-89`):
- The deck becomes a CDC+MIDI composite device on trial: a host must mount it within
  8 s, or it reverts to serial.
- After 3 failures it gives up until the next power cycle.
- A power cycle, holding KEY at boot, or a crash all return it to serial
  (`main.c:629-660`).
- In USB MIDI mode the `usb` destination exists and is on.

**Notes**
- The reboot loses the lanes, tempo, sync and play state, because only `boot` re-runs;
  `docs/TESTING.md:531-534` says the same.
- **"put >usb on in boot" does not work**: `boot` runs as GUIDE, `usb` is SYSTEM, and
  the line is refused with `"usb: not permitted here"`. If it were allowed, it would
  reboot on every boot.

### 2.16 `din` · SYSTEM · "din <gpio> - MIDI with no host" · `builtins.c:166-225`; `dinmidi.c`

**Forms**

**`>din`, not running**, prints six lines: `"din <gpio>  MIDI out, no host"`,
`"drives an SP404, a eurorack"`, `"brain, any MIDI IN at all."`,
`"needs a resistor loop - see"`, `"docs/HARDWARE.md before you"`,
`"trust it. e.g. >din 17"`. Status: `"din <gpio>"`.

**`>din`, running**, prints `"din GPIO%d, %u bytes sent"`, then either
`"the wire is busy"` or `"nothing sent since last asked"`. Status: `"din GPIO%d %uB"`.
Reading the count resets it (`dinmidi.c:25-30`).

**`>din off`** deletes the UART driver and switches the `din` destination off. Status:
`"din off"`.

**`>din <gpio>`**:
- The number is read with `atoi` and must be 1–48; otherwise `"a GPIO number, 1-48"`.
- These board pins are refused with `"GPIO%d is %s"`:

  | GPIO | used by |
  |---|---|
  | 11 | "panel SCK" |
  | 12 | "panel MOSI" |
  | 5 | "panel DC" |
  | 40 | "panel CS" |
  | 41 | "panel RST" |
  | 18 | "the KEY button" |

- `dinmidi_start` (`dinmidi.c:32-81`) uses UART1 at 31250 baud, 8N1, transmit only. The
  same pin again does nothing; a different pin moves the output.
- A failure prints `"GPIO%d refused: %s"`, with the error name.
- Success switches the `din` destination on. Status: `"din on GPIO%d"`.

**Effects and notes**
- It configures a GPIO as UART TX and writes nothing to flash, so the pin has to be set
  again after every reboot. Being SYSTEM, it cannot be set from `boot`.
- Other pins in use are not refused — for example SD card pins 21, 38 and 39
  (`battery.c:15-16`). What happens if one is used is UNVERIFIED.
- The deck's own 0xF9 step marker is not sent on the wire (`dinmidi.c:101-108`).

### 2.17 `flash` · SYSTEM · "flash now - reboot to ROM loader" · `builtins.c:1120-1170`

**Forms**

**`>flash`**, or anything other than exactly `now`, prints `"this reboots the deck into the"`,
`"ROM loader and ends the session."` and `"type:  >flash now"`. Status:
`"flash needs: >flash now"`; returns `CMD_ERROR`.

**`>flash now`**:
1. Saves every changed document (journal and SD card).
2. Stops the transport.
3. Writes the vitals goodbye to NVS.
4. Prints `"download mode. flash now:"`, `"  idf.py -p PORT flash"`,
   `"no button, no paperclip. the deck STAYS in"`,
   `"download mode until it is flashed or unplugged:"` and
   `"RTC_CNTL_FORCE_DOWNLOAD_BOOT survives a CPU reset."`.
5. Announces `"DOWNLOAD MODE - flash now"`.
6. Waits 600 ms.
7. Hands the USB PHY back to USB-Serial-JTAG.
8. Sets `RTC_CNTL_FORCE_DOWNLOAD_BOOT`.
9. Calls `esp_restart()`.

**Notes**
- The download bit survives CPU resets. The deck waits in download mode until it is
  flashed with a normal reset or unplugged (`builtins.c:1071-1087`). The app clears the
  bit at boot (`main.c:477`).
- **From USB MIDI mode** this still uses `esp_restart()`, which `builtins.c:1090-1113`
  says never finishes in that mode. The docs record this as an open problem — run
  `>usb off` first (`docs/OS.md:656-661`; `docs/TESTING.md:527-529`;
  `docs/NEXT.md:74`).

### 2.18 `dump` · READ · "print a document to the console" · `builtins.c:1176-1234`

**Forms:** `>dump` (the current document) or `>dump <name>`.

**Prints:** every line as `"%3d|%s"`, numbered from 1. A line longer than 95 characters
is split there, and the 96th character is dropped. Output goes to `+out` and to the log.

**Status:** `"%u bytes"`.

**Errors**
- `"no document called '%s'"`.
- Dumping `+out` itself is refused: `"dump writes into %s"`.
- If the document's length changes during the walk, it prints
  `"document grew under dump"` and stops.

**Effects:** selects the named buffer for the walk, then switches back.

### 2.19 `play` · EDIT · "start the clock" · `builtins.c:2146-2151`; `seq.c:1526-1558`

**Status:** `"playing at %d"` (the bpm).

**Effects:** play **starts from the top**.
- The position and tick go to 0, the grid is re-anchored, and the jitter statistics are
  cleared.
- Every counted lane (`!n`) waits again — for its own downbeat, or for its source if it
  is routed.
- A lane that had finished its count is un-finished and unmuted. **Lanes muted by hand
  stay muted.**
- If MIDI clock is on, it sends Song Position 0, then Start.
- On a following deck, the deck stays silent for up to about one second, until the
  leader's count arrives.

**Notes:** there is no guard, so `>play` while playing restarts from the top.

### 2.20 `stop` · EDIT · "stop the clock" · `builtins.c:2153-2158`; `seq.c:1560-1567`

**Status:** `"stopped"`.

**Effects**
- If MIDI clock is on and the transport was running, it sends Stop (0xFC).
- The transport stops.
- Pending note-offs are sent at once, followed by CC 123 (all notes off) on all 16
  channels.
- The lanes are kept.

**Interaction:** autosave is held off while playing. Once the transport stops, the main
loop saves the current document (`main.c:964-974`, `main.c:984-1002`).

### 2.21 `lanes` · READ · "what is playing" · `builtins.c:2160-2252`

**Prints one line per lane**, in table-slot order (not document order). The columns are:

| column | content |
|---|---|
| 1 | `-` if the lane is muted (by `mute`, `solo`, `toggle` or `map` — but not if it finished its count), otherwise a space |
| 2 | the lane's canonical name, padded to 5 |
| 3 | the pattern, exactly as typed |
| 4 | `<- source`, if the lane is routed |
| 5 | for a counted lane: ` p/n` while playing pass p of n; ` waits` before it starts, or while stopped; ` done` once finished |

The three line formats are `"%c%-5s %s%s"`, `"%c%-5s %s <- %s%s"` (a counted cue), and
`"%c%-5s <- %s"` (an uncounted routed lane, whose own pattern is not shown because it is
ignored).

**Inputs** (`>x = knob` or `>x = pad`) get their own lines:
- `" %-5s %s, not heard yet"`, or
- `" %-5s %s %3u .%u.%u"`: the kind, the last value (0–127), and the last two octets of
  the sender's IPv4 address.

**Footer**
- `"%d %s sw%d%s"`: bpm, key, swing, and `" clk"` if MIDI clock is on.
- `"to: <enabled destinations>"`, or `"to: nowhere - try: send ble on"`.
- `"%u events dropped - the transport is behind"`, if any were dropped.

**Status:** `"%d lane%s %s"`, ending in `playing` or `stopped`.

**Note:** there is **no binding column**, despite the comment at `builtins.c:2178-2181`.

### 2.22 `jitter` · READ · "timing, measured in us" · `builtins.c:1356-1405`

**Forms**

**`>jitter reset`** clears the clock and transport statistics only (`seq.c:178-182`). The
dropped-event count, the BLE packing counters and the vitals are left alone. Status:
`"jitter counters cleared"`.

**Anything else** prints the report:
- **Clock timing** — tick versus the ideal grid — then **transport timing** — the queue
  wait before a message is handed to its destination. Each block is:
  - `"%-5s no samples yet"`, or `"%-5s n%-6u sd%4dus late%u"`;
  - `"      spread %d us"`;
  - `"      mean %+d us"`;
  - `"      widest one at t+%us"` — the uptime, in seconds, at which the maximum occurred;
  - `"      <.1:%u <.25:%u <.5:%u <1:%u <2:%u <5:%u >5:%u "` — millisecond buckets,
    measured from the tightest sample (`seq.h:528-534`).
- `"ble   %u msgs in %u packets (%u.%02ux)"`, if any BLE packets were sent.
- A legend: `"clock = tick vs the ideal grid"`, `"xport = queue wait before sending"`,
  `"mean = where ticks sit on it"`, `"sd/spread are MICROseconds."`,
  `"t+ is when, not how long."`, `"first 8 ticks after play skipped"`.
- Up to 4 lines on how the previous run ended (`vitals.h:67-74`).
- `"%u events dropped"`, if any were dropped.

Status: `"clock sd %d us over %u ticks"`.

**Interactions**
- `>play` and `>bpm N` also reset the statistics (`seq.c:1547`, `seq.c:1519`).
- On a following deck, sd includes the deliberate grid corrections
  (`docs/NEXT.md:76-77`).

### 2.23 `panic` · EDIT · "silence everything" · `builtins.c:2300-2306`

**Effects:** calls `seq_stop()` — which stops the transport, sends MIDI Stop if clock is
on, and silences all notes — then `seq_all_notes_off()` again. The pending note-offs and
CC 123 on all 16 channels are therefore sent **twice**. Lanes and mutes are kept, and
`>play` starts again from the top.

**Status:** `"all notes off"`.

### 2.24 `mute` · EDIT · "mute hat bass | mute = all on" · `builtins.c:2262-2298`

### 2.25 `solo` · EDIT · "solo kick | solo = all on" · the same function, `c_mute`

**Forms**
- `>mute a b …` mutes each named lane.
- `>solo a b …` mutes every lane **not** named and unmutes the named ones.
- `>mute` or `>solo` with no names unmutes every lane.

**Names** are separated by spaces and must be exact canonical lane names, such as `disc`,
`disc:2` or `bass:vel`. `disc:1` does not match `disc`. Unknown names are ignored without
a message.

**Status:** `"%s: %d lane%s changed"`, where the first field is `mute` or `solo`, or
`all on` when no names were given.

**Interactions**
- Running a muted lane's line again compiles it and unmutes it; only a playing,
  unchanged line is removed by the re-run (§1.7).
- `toggle` and `map` set the same mute flag, so `>mute` with no names also brings those
  lanes back.
- A lane that finished its count is muted (`seq.c:653-657`). `>mute` or `>solo` with no
  names — or `>solo` naming that lane — unmutes it. It then plays another full count from
  its next downbeat, while its `done` flag stays set. Inferred.
- `>play` does not unmute lanes muted by hand.
- Muting is not deleting: typing a lane's bare name frees its slot.

### 2.26 `toggle` · EDIT · "toggle kick hat - off, then on" · `c_toggle`

**Forms**
- `>toggle a b …` flips each named lane: one that plays goes silent, one that is silent
  comes back. It keeps its slot, its pattern and its place in the bar, so **the line is
  the switch** — run it for the drop, run it again for the return.
- **It lands on the one** (2026-09-29): the change waits, pending, for the first tick of
  the next bar, and the clock makes it before any lane fires (`seq_toggle.h`, `seq.c`
  `tick`). Run again before the bar, it takes the change back. With the clock stopped
  it acts at once, so a piece can set itself up before `>play`. `tools/test_toggle.c`.
  A pair — `>toggle pad pad:2` with one of them silent — swaps two versions of a part
  on the one.
- No names: `"toggle what? toggle kick hat"`, an error.

**Names** are exact canonical lane names, as for `mute` (`lane_named`).

**Status:** `"toggle: %d off, %d on - on the one"` while playing, without the tail
when stopped; `"none of those is playing"` (an error) when no name matched.

**In a piece** it is every section change: `>toggle kick hat clap lead bass:2 arp`
empties ORBITALS for its eclipse on the one, and the same line brings it back;
`>toggle pad pad:2 bass bass:2 lead lead:2` moves a track to its second progression
([pieces/README.md](../../pieces/README.md)).

### 2.27 `clear` · EDIT · "every lane gone; the page stays" · `c_clear`

`>clear` forgets every lane and picture (`seq_forget_all`, `viz_forget_all`) and keeps
the document on screen — the clean slate `>new` gives, without a new page. Names and
their definitions stay. Status `"%d lane%s gone"`. It answers the owner's open question
of 2026-09-26 ([next.md](next.md) §2).

### 2.28 `map` · EDIT · "map cut - only it sends, to learn" · `c_map`

**For MIDI learn.** A DAW learns the next controller it hears; with a set playing it hears
everything. `>map cut` mutes every lane that sends MIDI except the one named; move it,
let the DAW learn it, and `>map` alone brings the rest back.

- Pictures keep drawing: they send no MIDI. That is the one difference from `>solo cut`.
- An unknown name: `"no lane called %s"`, an error, and nothing changes.
- Status `"map: only %.12s - learn, then >map"`, or `"map off - all back on"`.

### 2.29 `help` · READ · "list the commands" · `builtins.c:42-54`, `builtins.c:879-899`

**Prints**
- The 34 table rows, as `"%-8s %s"` (name, then help), in table order.
- `"names:"`, followed by every defined name (in slot order, inputs included) and then
  the 16 pictures, wrapped at 29 columns with indented continuation lines.
- `"your own: >conga = note 63"`.

**Status:** `"%d commands"`, which reads `34 commands`.

### 2.30 `list` · READ · "list open buffers" · `builtins.c:56-73`

**Prints** one line per resident buffer, from the 8 slots (`docstore.h:22`), as
`"%c%d %-16s %5u%s"`:
- `*` for the current buffer, otherwise a space;
- the slot index;
- the name, or `(scratch)`;
- the length in bytes — the length on flash if the buffer is not loaded yet;
- ` *` if it has unsaved changes.

Unnamed, empty slots are skipped unless one is the current buffer.

**Status:** `"%d buffer%s"`.

**Note:** `>open <digit>` takes the slot index shown here.

### 2.31 `new` · EDIT · "a fresh scratch buffer" · `builtins.c:283-300`

**Effects**
- Claims a free slot for an unnamed scratch buffer and switches to it.
- **Forgets every lane**: `seq_forget_all` clears patterns and routes, but the transport
  keeps running.
- **Blanks the picture** (`viz_forget_all`).
- Names, inputs, tempo, key, swing and destinations are not touched.

**Status:** `"new scratch buffer %d"`. If all 8 slots are in use: `"no free buffer"`.

**Interaction:** only `new` clears lanes; `open`, `close` and `run` do not.

### 2.32 `name` · EDIT + STORE · "file this buffer under a name" · `builtins.c:304-317`

**Form:** `>name <anything>`. The name is the whole rest of the line, spaces included.

**Errors**
- An empty name: `"name what? e.g. name lullaby"`.
- A name another buffer already has: `"cannot use that name"` (`buffer.c:197-205`).

**Effects**
- Renames the current buffer, keeping at most 23 characters (`buffer.c:206`).
- Writes a journal snapshot with `doc_save`. Its return value is **ignored**, and **no SD
  mirror** is made here.

**Status:** `"filed as %s"`, showing the whole argument, untruncated.

**Notes**
- A name starting with `+` turns the buffer into machine output, which is never
  journalled or mirrored (`buffer.c:279-282`; `journal.c:429-433`). The status still says
  `"filed as +…"`.
- On the SD card the file is `/sdcard/<name>.txt`, with every character outside
  `[A-Za-z0-9_-]` replaced by `_` (`mirror_path.h:35-50`).

### 2.33 `open` · READ · "switch to a named document" · `builtins.c:319-346`

**Forms**
- **`>open <digit>…`**: only the **first character** is used, as a slot index 0–9, so
  `>open 12` opens slot 1. Status: that buffer's name, or `"scratch"`. A missing slot
  gives `"no buffer %d"`.
- **`>open <name>`**: an exact match against the resident buffers. Status: the name. If
  the switch fails, `"cannot open %s"`; if nothing matches, `"no document called '%s'"`,
  which is also what a bare `>open` gets.

**Notes**
- A document whose name starts with a digit cannot be opened by name.
- Only the 8 resident slots are searched. A closed document comes back only when the
  journal reloads it at the next boot (`journal.c:389`).
- Lanes are not touched.

### 2.34 `run` · EDIT · "run a document without leaving this one" · `builtins.c:91-149`

**Forms**
- `>run` runs the current document.
- `>run <name>` switches to that document first (`doc_buf_find`). An unknown name, or a
  failed switch, gives `"no document '%.20s'"`.

**Effects:** walks the document once, with its length read before the walk. Each
non-empty line goes through the dispatcher as **`CMD_BY_GUIDE`**. Afterwards it **always
switches back** to the buffer that was current before.

**Status:** `"%d ran, %d refused"` with `CMD_ERROR` if any line returned `CMD_ERROR`;
otherwise `"%d line%s ran"`.

**Notes**
- **What counts as "ran":** every non-empty line that returns `CMD_DONE` — which includes
  **prose lines and `>#` comments**. `CMD_PENDING` counts as neither ran nor refused.
- **GUIDE authority:** SYSTEM verbs in the document are refused and counted as refused.
  NET verbs run, and one that needs a password opens the prompt. A later prompt replaces
  an earlier one that is still open, because `editor_ask` does not check for an active
  prompt (`editor.c:197-203`). Inferred.
- **No nesting:** a `>run` line inside a running document gives
  `"run cannot run itself"`.
- The re-run toggle never fires here (it is `BY_HANDS` only), so running a document twice
  recompiles its lanes rather than silencing them.
- The walk reads **the current buffer** by offset (`builtins.c:112-131`;
  `buffer.c:306-313`). A line that switches buffers (`>new`, `>open`, `>close`) therefore
  changes what the rest of the walk reads. Inferred.
- An old-style `>wifi <ssid> <pass>` line is refused but **not cut from the document**;
  only the editor path cuts it (`editor.c:1064-1066`).

### 2.35 `save` · STORE · "write this buffer now" · `builtins.c:348-367`

**Effects**
- If the current buffer's name starts with `+`, nothing is written: status
  `"%s is output, not a document"`, returning `CMD_DONE`.
- Otherwise `doc_save` writes a new journal snapshot — **even if nothing changed**
  (`journal.c:424-470`) — and then mirrors to the SD card if one is mounted (a failed
  mirror is ignored).

**Status:** `"saved %u bytes"`. If the journal fails (for example, it is full):
`"save failed"`, returning `CMD_ERROR`.

**Note:** `save` writes immediately, **even while playing**. Autosave deliberately waits
for stop, because a journal write stalls the clock for 13–18.6 ms (`main.c:984-1002`).

### 2.36 `close` · EDIT · "forget this buffer" · `builtins.c:369-377`

**Effects:** closes the **current** buffer; any argument is ignored. Its RAM is freed and
the lowest-numbered remaining buffer becomes current. The journal keeps the last
snapshot.

**Status:** `"closed"`. With only one buffer left: `"cannot close the last buffer"`.

**Notes**
- **It does not save first** (`buffer.c:212-228`). Changes made since the last autosave or
  save are lost — which includes everything edited while playing. The comment at
  `main.c:997-998` says closing journals the document; it does not.
- The closed document's lanes keep playing.

### 2.37 `density` · EDIT · "low | high (use high to split)" · `builtins.c:401-438`

**Form:** only the **first character** of the argument matters.
- `h`, `d` or `6` selects **high**: the 6×12 font, 60 columns.
- `l`, `c` or `1` selects **low**: the 12×24 font, 30 columns.
- Anything else, including no argument, prints `"density low | high"` and returns
  `CMD_ERROR`.
- A layout failure prints `"density: layout refused"`.

**Status:** `"%s %dx%d"`, which reads `"low 30x12"` or `"high 60x24"` — the grid's
columns by rows, status row included (`editor.c:167-190`; a 400×300 panel with 20 px side
margins and a 12 px top margin).

**Notes**
- The setting is not saved anywhere.
- **In `boot` it has no lasting effect**: `editor_init()` runs after the boot document and
  sets low (`main.c:695`, `main.c:704`; `editor.c:208`). Inferred from the start-up order.
  The shipped boot text uses `>density chunky`, which is low anyway (`ui_text.h:101`).

## 3. All verbs at a glance

- **GUIDE** means the verb may run from `boot` or `>run`.
- **Agent** applies `cmd.c:25`; nothing calls as an agent today.
- **Writes flash** covers the journal partition, NVS and the SD card.

| # | verb | caps | GUIDE | agent | writes flash | reboots | radio |
|---|---|---|---|---|---|---|---|
| 1 | bpm | EDIT | yes | yes | – | – | – |
| 2 | scale | EDIT | yes | yes | – | – | – |
| 3 | swing | EDIT | yes | yes | – | – | – |
| 4 | sync | EDIT | yes | yes | – | – | **yes**: `lead`/`follow` start Wi-Fi (station mode, power save off) and ESP-NOW; `alone` stops only ESP-NOW |
| 5 | send | SYSTEM | no | no | – | – | **yes**: `ble on`/`off` starts or stops BLE MIDI advertising (via the main loop) |
| 6 | wifi | NET | yes¹ | no | NVS: SSID and password on join; erased by `forget` | – | **yes**: join, off |
| 7 | battery | READ | yes | yes | NVS: `use` | – | – |
| 8 | kbd | SYSTEM | no | no | NVS: `forget` (bond store, `deck/peer`) | – | **yes**: `forget` drops the BLE link and rescans |
| 9 | host | NET | yes¹ | no | – | – | **yes**: access point |
| 10 | osc | NET | yes | no | – | – | UDP only; `osc in` changes Wi-Fi power save |
| 11 | ssh | NET | yes¹ | no | NVS `sshkeys`: key kept on first contact; erased by `forget` | – | uses the Wi-Fi link |
| 12 | frame | NET | yes | no | – | – | UDP only |
| 13 | split | EDIT | yes | yes | – | – | – |
| 14 | route | EDIT | yes | yes | – | – | – |
| 15 | usb | SYSTEM | no | no | journal + SD (changed documents); NVS vitals | **yes** (`on` and `off`, always) | – |
| 16 | din | SYSTEM | no | no | – | – | – |
| 17 | flash | SYSTEM | no | no | journal + SD (changed documents); NVS vitals | **yes** (`now`, into the ROM loader) | – |
| 18 | dump | READ | yes | yes | – | – | – |
| 19 | play | EDIT | yes | yes | – | – | – |
| 20 | stop | EDIT | yes | yes | indirectly: autosave follows | – | – |
| 21 | lanes | READ | yes | yes | – | – | – |
| 22 | jitter | READ | yes | yes | – | – | – |
| 23 | panic | EDIT | yes | yes | – | – | – |
| 24 | mute | EDIT | yes | yes | – | – | – |
| 25 | solo | EDIT | yes | yes | – | – | – |
| 26 | help | READ | yes | yes | – | – | – |
| 27 | list | READ | yes | yes | – | – | – |
| 28 | new | EDIT | yes | yes | – | – | – |
| 29 | name | EDIT+STORE | yes | yes | journal | – | – |
| 30 | open | READ | yes | yes | – (reads the journal) | – | – |
| 31 | run | EDIT | yes | yes² | whatever its lines do | – | whatever its lines do |
| 32 | save | STORE | yes | yes | journal + SD | – | – |
| 33 | close | EDIT | yes | yes | – | – | – |
| 34 | density | EDIT | yes³ | yes | – | – | – |

¹ Allowed, but at boot nothing can ask for the password yet, so the verb reports
`"nothing here can ask"`. `wifi off`, `wifi forget` and `ssh forget` do work at boot.
² The lines run as GUIDE, so a document can reach NET verbs that an agent itself may not
use.
³ It runs, but `editor_init()` sets density back to low after `boot`.
