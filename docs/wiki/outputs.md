# Outputs, the network and the radio — where events go

*Part of [the deck, top to bottom](README.md). Snapshot: commit `85d1e6a`, 2026-09-28.
Documents are on [documents.md](documents.md); every flash write that can happen while
playing is in [system.md](system.md) §7; surprises are in [errata.md](errata.md).*

**Conventions**

- Citations are `file:line`. Short names are expanded in the table below; everything is relative to the repo root.
- Line numbers are those of commit `85d1e6a`.
- **UNVERIFIED** = not confirmable from this repository's code: hardware behaviour, ESP-IDF internals, or a doc claim with no code behind it.
- `IDF:` citations point into ESP-IDF v5.5.4 (the pinned version, `firmware/sdkconfig:394`), read from a local checkout. They are **not** in this repo.
- Network names, passwords and IP addresses that occur in source strings, docs or logs are deliberately left out. They are shown as `<ssid>`, `<pass>` and `<ip>`.

| short name | path |
|---|---|
| `main.c`, `editor.c`, `ask.h`, `ui_text.h`, `serialkbd.c`, `view.c`, `view_wire.h`, `vitals.c`, `vitals.h`, `vitals_verdict.h`, `battery.c`, `splash.c` | `firmware/main/` |
| `builtins.c`, `cmd.c`, `cmd.h`, `lane_name.h`, `secret_line.h` | `firmware/components/cmd/` (+`include/`) |
| `seq.c`, `seq.h`, `midi_len.h`, `seq_clock.h` | `firmware/components/seq/` (+`include/`) |
| `blemidi.c/.h`, `dinmidi.c/.h`, `usbdev.c/.h`, `usbmux.c/.h` | `firmware/components/<name>/` (+`include/`) |
| `net.c`, `osc.c`, `net.h`, `osc_pack.h`, `osc_parse.h` | `firmware/components/net/` (+`include/`) |
| `ensemble.c`, `ensemble.h`, `ens_count.h` | `firmware/components/ensemble/` (+`include/`) |
| `ssh.c`, `ssh.h`, `ssh_fp.h`, `ssh/CMakeLists.txt` | `firmware/components/ssh/` (+`include/`) |
| `ble_kbd.c`, `kbd.h`, `serialkbd_map.h`, `keymap.c` | `firmware/components/kbd/` (+`include/`) |
| `buffer.c`, `journal.c`, `undo.c`, `sdmirror.c`, `docstore.h`, `mirror_path.h` | `firmware/components/docstore/` (+`include/`) |
| `sdkconfig`, `sdkconfig.defaults`, `partitions.csv` | `firmware/` |
| `ci.yml` | `.github/workflows/ci.yml` |

## 0. At a glance

### 0.1 Destinations

Registration order is also dispatch order: `ble, mon, din, osc, view, usb` (`main.c:544-556,669`; `seq.c:216-221`).

| name | registered | state at boot | on | off | what it sends |
|---|---|---|---|---|---|
| `ble` | `main.c:544` | off (`seq.c:1607`, `blemidi.c:27`) | `>send ble on`; the main loop then starts advertising (`main.c:820-824`) | `>send ble off`: drops the link and stops advertising (`blemidi.c:238-244`) | BLE-MIDI 1.0 packets, timestamped (§2.5) |
| `mon` | `main.c:545` | off | `>send mon on` | `>send mon off` | console lines tagged `midi` (§1.4) |
| `din` | `main.c:549` | off | `>din <gpio>` (`builtins.c:222`); `>send din on` only flips the flag and sends nothing until the UART runs | `>din off` stops the UART and the flag (`builtins.c:168-173`); `>send din off` leaves the UART running | raw MIDI bytes on UART1 at 31250 baud (§3) |
| `osc` | `main.c:553` | off | `>osc <ip> [<port>]` (`builtins.c:2052`) or `>send osc on` | `>send osc off` (the socket stays open) | OSC 1.0 messages over UDP, one datagram per drained tick (§1.6) |
| `view` | `view.c` (from `main.c:556`) | off | `>send view on`, `>send view <W>x<H>` or `>send view <mode>` (`builtins.c`, `c_send`) | `>send view off` | its sink ignores events; frames go out from the main loop (§1.7) |
| `usb` | `main.c:669-677`, **USB MIDI mode only** | **on**: the only destination enabled at boot, and only in that mode (`main.c:677`) | automatic | `>send usb off` | USB-MIDI via `tud_midi_stream_write` (§2.4) |

### 0.2 Who may run what

A line typed by hand and run with Ctrl+Enter runs as `CMD_BY_HANDS` (`editor.c:1061`), which may do anything. Two other paths run as `CMD_BY_GUIDE`, limited to READ, EDIT, STORE and NET (`cmd.c:19-28`): the `boot` document at startup (`main.c:315`) and `>run` (`builtins.c:121`). A refused line prints `<name>: not permitted here` (`cmd.c:265-268`).

| command | caps (`builtins.c:2308-2343`) | from `boot` / `>run`? |
|---|---|---|
| `send`, `usb`, `din`, `kbd`, `flash` | SYSTEM | **no** |
| `osc`, `frame` | NET | yes |
| `wifi`, `host` | NET | allowed, but fail at boot: the password prompt does not exist yet (§4.2) |
| `ssh` | NET | fails at boot: `ssh needs wifi` (`builtins.c:1850-1854`) |
| `sync`, `play`, `stop`, `panic`, `new`, `close`, `run`, `bpm`… | EDIT | yes |
| `name` | EDIT and STORE | yes |
| `save` | STORE | yes |
| `battery`, `dump`, `lanes`, `jitter`, `open`, `list`, `help` | READ | yes |

## 1. Destinations

### 1.1 Mechanism

- `seq_dest_add(name, fn, flush, help)` fills a table of `SEQ_MAX_DESTS` = 8 (`seq.h:471`) with names of up to 11 characters (`seq.c:38-44`). New entries start **off** (`seq.c:1596-1609`). A full table returns `ESP_ERR_NO_MEM` (`seq.c:1599-1601`).
- `seq_dest_enable()` only flips a flag. An unknown name returns `ESP_ERR_NOT_FOUND` (`seq.c:1621-1629`), shown as `no destination called '<name>'. try just: send` (`builtins.c:2120-2122`).
- **The clock never calls a transport.** `emit()` queues `{status,d1,d2,queued_us,lane}` on a 64-deep queue without blocking (`seq.c:285-313,1060`). When the queue is full, it drops the oldest non-realtime event (it puts back up to 4 queued ≥0xF8 bytes) and counts the loss (`seq.c:291-311`).
- The task `midi` (core 1, priority 6, 4096 B stack; `seq.c:1072`) wakes, drains up to `SEQ_BURST_MAX` = 32 events (`seq.c:187,209-224`), and hands **each event to every enabled destination in table order**. It then calls each enabled destination's `flush` once (`seq.c:226-230`). It logs `%u events dropped - transport behind` (`seq.c:234-238`).
- Sink arguments:
  - `lane` is the name of the lane that emitted the event, or `""` (`seq.c:218,281,339-343`).
  - `when_us` is `(uint32_t)esp_timer_get_time()` taken at `emit` (`seq.c:290,219`). That is the tick's dispatch time, not the ideal grid time, and it **wraps every 2³² µs ≈ 71.6 min**.
- The `esp_timer` task (the clock) is pinned to CPU1 (`sdkconfig:1743`), as is `midi`. `sdkconfig.defaults:116-118` states the timer task runs at priority 22 against 6 for `midi`. So all of a tick's events are normally queued before the drain starts. This is inferred, **UNVERIFIED** as timing.

### 1.2 The event stream (all destinations see the same stream)

| status bytes emitted | when | source |
|---|---|---|
| `9n note vel` (vel 1–127) | a step on a `note` or `voice` lane | `seq.c:529-542` |
| `8n note 0` | the scheduled off after the gate: drums default to ch 10 with a 40 ms gate, voices to ch 1 with 150 ms, cc lanes to ch 1 (`lane_name.h:219-233`). It is sent immediately if the 64-slot off table is full (`seq.c:281,330-332`). Pending offs are also flushed on stop and panic (`seq.c:1571-1576`) | `seq.c:315-345` |
| `Bn cc val` | a step on a cc lane that has a digit (`digit*127/9`), or the routed value. A step with no digit sends nothing | `seq.c:510-524` |
| `Bn 123 0` × 16 channels | every `seq_stop()`, so `>stop`, `>panic`, `>usb` and `>flash now` | `seq.c:1577-1581` |
| `F8` | every 4th internal tick (96 PPQN ÷ 4 = 24 PPQN) while running with `>sync on` | `seq.c:925-927`; `seq.h:75-77` |
| `F2 00 00` then `FA` | `>play` with sync on | `seq.c:1552-1557` |
| `FA` / `FC` | sync switched on / off while running | `seq.c:1461-1468` |
| `FC` | stop with sync on | `seq.c:1560-1564` |
| `F9 (step & 0x7F)` | once per step (every 24 ticks). This is the deck's internal marker, not MIDI | `seq.c:945-955`; `midi_len.h:50-57` |

Order within one tick: due note-offs → (the count adoption) → `F8` → inputs, then lanes → `F9` (`seq.c:850-957,568-574`).
Tempo runs from 20 to 300 bpm, and a tick lasts 60e6/bpm/96 µs (`seq.c:1482-1485,960-965`).
The boot-document definitions are in `ui_text.h:74-93` (for example `>kick = note 36`, `>bass = voice 2 ch 1 gate 180`, `>cut = cc 74`).

### 1.3 `ble`

See §2.5.

### 1.4 `mon`: the console monitor

The monitor calls `ESP_LOGI("midi", …)` from the `midi` task (`main.c:223-243`):

| event | format (`main.c:234-241`) | rendered example |
|---|---|---|
| note on (`9n`, vel > 0) | `"%-5s on  %3u v%-3u ch%-2u t%u"` | `kick  on   36 v127 ch10 t12345` |
| note off (`8n`, or `9n` with vel 0) | `"%-5s off %3u      ch%-2u t%u"` | `kick  off  36      ch10 t12385` |
| CC other than 123 | `"%-5s cc%-3u = %3u   ch%-2u t%u"` | `cut   cc74  =  42   ch1  t12400` |

- Clock, `F9`, start, stop and SPP are not printed. `t` is `when_us/1000` in ms since boot at queue time, and it wraps after about 71.6 min.
- Measured figures: none. `docs/TESTING.md:59-72` lists the expected output (kick 127/71, bass off about 420 ms after on, pad notes sharing one timestamp).

### 1.5 `din`

See §3.

### 1.6 `osc`: OSC out

- With no socket (`s_sock < 0`), nothing is sent (`osc.c:94-96`).
- `F9` becomes `/deck/step ,i <step & 127>`. The step counter **wraps at 128**; it is not the bar number (`osc.c:104-109`; `seq.c:954`).
- `9n` with velocity > 0, or `Bn`, becomes `/deck/<lane> ,ii <d1> <d2>`: (note, velocity) or (controller, value). An empty lane becomes `/deck/cc` for a CC or `/deck/x` for a note (`osc.c:110-119`).
- Note-offs and all realtime bytes except `F9` are not sent (`osc.c:99-113`). All-notes-off on stop therefore goes out as sixteen `/deck/cc ,ii 123 0`, because `lane` is normally empty outside the clock. This is inferred from `seq.c:1577-1581` and `osc.c:111-116`.
- **Datagram layout.** Messages are appended to one 512-byte buffer per drain (`osc.c:41-45,97`) and sent by `net_osc_flush` in one `sendto` (`osc.c:156-170`).
  - Messages are simply **concatenated**, with no `#bundle`. OSC 1.0 requires a bundle for more than one message, and `osc_parse.h:170-173` admits it.
  - The receivers this repo tests read the concatenated form: `tools/osc_listen.py` via `ci.yml:276-290`, and the deck's own parser.
  - **UNVERIFIED** for third-party receivers.
- **Encoding** (`osc_pack.h:30-88`): NUL-terminated strings padded to a multiple of 4; type tags `,ii`, `,i` or `,s`; int32 big-endian. A message that does not fit is rolled back whole and not counted (`osc_pack.h:56-68`; `osc.c:117-119`).
- A worked example: a kick (note 36, velocity 127) on step 5 → `/deck/kick\0\0` `,ii\0` `00000024 0000007F` (24 B) + `/deck/step\0\0` `,i\0\0` `00000005` (20 B) = one 44-byte datagram.
- **Size limit.** An address is at most `/deck/` + 19 characters (`seq.h:55`), so one message is at most 40 B. Sixteen lanes plus chords can exceed 512 B, and the excess is dropped silently. This is arithmetic, **UNVERIFIED** on the device. The comment at `osc.c:41-42` assumes "eight lanes", but `SEQ_MAX_LANES` is 16 (`seq.h:39`).
- **`>frame`** sends `/deck/frame ,s <text>` in a separate 1100-byte datagram (`osc.c:133-154`):
  - With pictures running, the text is the picture (`builtins.c:1707-1717`).
  - Otherwise it is the document, up to 1023 bytes (`builtins.c:1718-1750`).
  - Messages: `no osc target. try: osc <ip> <port>`, `frame too large or send failed`, `sent %u bytes as a frame`, `sent a %d-byte frame`.
- **Target** (`builtins.c:2041-2054`; `osc.c:61-88`): `>osc <ip> [<port>]`; the port defaults to 9000 and must be 1–65535 (`a port is 1-65535`). The socket is non-blocking UDP. Success prints `osc -> %.15s:%ld`. The target lives in RAM only.
  - **There is no `>osc off`.** `net.h:65` says "port 0 disables", but the command refuses port 0.
  - Typing `>osc off` parses `off` as an address. `net_osc_target` closes the old socket first (`osc.c:63-66`) and then fails with `'off' is not an address` (`builtins.c:2048-2050`). Output stops, but the destination flag stays on.
- `>osc` with no argument prints `osc <ip> <port>`, `sends /deck/<lane> i i`, `%u msgs in %u packets`, then either the input status or `osc in <port> - /deck/<name>` / `sets a knob or a pad` (`builtins.c:1988-2007`). Its status message is a literal example target (address omitted here).
- Measured: with the keyboard scan at full duty, **5 messages in 23 s** arrived out of 280 sent. With the shared scan, **280 of 280** in each of two 20 s windows (`docs/NETWORK.md:265-275`).

### 1.7 `view`: frames to an HDMI node

- The sink ignores events (`view.c:26-32`). A frame is produced only when the main loop's `viz_service()` returns one (`main.c:870-877`).
- **Wire format** (`view_wire.h`; `docs/VIEW.md`): a picture, `'D''K''V''1'`, then `tick` u32 little-endian, `w` u8, `h` u8, `w*h` cell bytes (32–126 text, 128–155 tiles), and `sum` u8 = XOR of every byte after the magic. That is 11 + w·h bytes; 80×30 gives **2,411 B**. Ahead of every picture goes a control frame, `'D''K''C''1'`: the tick, the node's mode, and for the poster up to ten lines with the span each lights.
- **Transport today** is the console: `ESC ] view;<base64> BEL \n` (`view.c:62`).
  - In serial mode it is one non-blocking `usb_serial_jtag_write_bytes(…,0)`. A frame is sent whole or dropped whole and counted in `view_dropped` (`view.c:66-70`).
  - In USB MIDI mode it goes through `stdout`, which is the CDC interface (`view.c:71-74`).
  - `tools/viewrelay.py` relays it (`docs/VIEW.md:60-69`).
- Size (`builtins.c:2093-2117`; `viz.h:64-65`):
  - `on` means 53×20, the node's screen one to one (80×30 until 2026-09-29), unless the view is already on, when it keeps its size.
  - `<W>x<H>` accepts 4×2 to 80×30; outside that it prints `view is 4x2 to 80x30 cells`.
  - `<mode>` - plain, scan, riso, poster, code - turns it on, drawn that way (`viz_out_mode`). Phosphor and feedback were retired 2026-09-29: they resampled the deck's pixels. `code` shows the document itself: its name, tempo and key, and ten lines round the cursor with each lane's step lit (`code_lines`, `view.c`), read in place since 2026-09-29.
  - `off` restores the preview's own size.
  - A bad argument prints `send view on|off|53x20|mode` and the five modes.
- **Colour** is controllers 1-8 on MIDI channel 16, forwarded in every control frame (DKC2); `off` clears them ([VIEW.md](../VIEW.md)).
- **No computer between them, planned:** a 2 Mbaud UART from the deck's back header (GPIO18) to the Feather's RX, with the deck's 3.3 V powering the node — [VIEW.md](../VIEW.md), "The wire".
- The heartbeat reports `(%u sent, %u dropped)` (`main.c:950-954`).
- Measured (`docs/VIEW.md:73-86`): 86 frames in about 10.5 s at 124 bpm with 0 refused. The per-frame cost fell from **31 ms** to **0.5 ms**, and the loop ran 199 → 142 → 192 turns/s (view off → view through stdio → after the fix). There were 0 drops in 234. USB MIDI mode is untested (`docs/VIEW.md:88-90`).

### 1.8 `usb`

See §2.4.

### 1.9 Message length: three tables that disagree

| status | `midi_len.h` (DIN) `:28-48` | BLE `blemidi.c:347-357` | USB `usbdev.c:240` |
|---|---|---|---|
| `8x 9x Ax Bx Ex` | 2 data bytes | 2 | writes 3 bytes (2 data) |
| `Cx Dx` | 1 | 1 | **3 bytes (2 data)** |
| `F1 F3` | 1 | 1 | **3 bytes** |
| `F2` | 2 | 2 | 3 bytes (correct) |
| `F4–F7` | 0 | 0 | **3 bytes** |
| `F8–FF` | 0 | 0 | 1 byte |
| `F9` | dropped (`dinmidi.c:106`) | dropped (`blemidi.c:311`) | dropped (`usbdev.c:234`) |

This is harmless today, because the sequencer only emits `8n 9n Bn F2 F8 F9 FA FC` (§1.2). The USB expression `(status == 0xF2) ? 3 : 3` is redundant, and the table would be wrong for program change or aftertouch. `tools/test_midi_wire.c:4-5` claims that USB, BLE and DIN agree, but it tests only `midi_len.h`.

### 1.10 `>send` (`builtins.c:2057-2126`)

- With no argument, it prints for each destination `"%-4s %s"` (name, `ON`/`off`), then the help text cut at 25 characters, then `usb: <status>` and `act=mode dev=host midi=bound`. Its status message is `N destinations`.
- Two help lines are cut mid-word: `DIN/TRS MIDI - set >din <` and `the picture to an HDMI no`. The help strings are 30 and 27 characters (`main.c:549`, `view.c:36`); this was checked by rendering the format.
- `>send <name>` prints `<name> is on|off`. `>send <name> on|off` prints `<name> on|off`; anything else prints `send <name> on | off`.

## 2. USB

### 2.1 Two mutually exclusive modes

| | **serial** (power-on default) | **USB MIDI** |
|---|---|---|
| owns the PHY | USB-Serial-JTAG (`usbmux.c:30-40`; also forced at boot, `main.c:479-484`) | USB-OTG / TinyUSB |
| console | USJ | CDC-ACM "cyberdeck console" |
| cable keyboard | `serialkbd` task | CDC receive callback, same mapper |
| MIDI | none | `usb` destination |
| flashing without a button | USJ hardware reset | the esptool DTR/RTS pattern over CDC (§2.4) |
| intent stored in | — | `RTC_NOINIT s_want`: survives a soft reset, **cleared by power loss** (`usbdev.c:64-83,148-151,203-210`) |

### 2.2 Serial console and serial keyboard (serial mode)

- `serialkbd_init()` runs only if USB MIDI did not come up (`main.c:678-680`).
  - The USJ driver gets 256 B for receive and 4000 B for transmit. The transmit buffer holds two view frames and stays in internal RAM (`serialkbd.c:57-63`).
  - The console VFS uses the driver (`serialkbd.c:72`).
  - Task `serialkbd`: 3072 B stack, priority 5, not pinned (`serialkbd.c:74`).
  - It logs `typing over USB is live - this terminal is a keyboard`.
- The task reads one byte at a time → `skb_feed()` → `kbd_inject()` (`serialkbd.c:34-53`). That is the same queue as BLE HID: 256 deep (`ble_kbd.c:1049`). An injection is **dropped silently** if the queue is full (`ble_kbd.c:145-150`); only BLE drops are counted.

### 2.3 Byte → key map (`serialkbd_map.h:47-98`)

| byte(s) | event |
|---|---|
| `0x0D` CR | **Enter**: inserts a newline (`editor.c:1159-1174`) |
| `0x0A` LF | **Enter + LCTRL = Ctrl+Enter = run the line** (`serialkbd_map.h:74-78`; `editor.c:1146-1149`) |
| `0x08`, `0x7F` | Backspace |
| `0x09` | Tab (the editor inserts two spaces, `editor.c:1175`) |
| `0x20–0x7E` | that character |
| `0x01–0x1A`, except 08, 09, 0A, 0D | Ctrl + (`'a'+b-1`) |
| `ESC [ A/B/C/D` | Up / Down / Right / Left |
| `ESC ESC` | Esc |
| `ESC` + any byte other than `[` or ESC | **both bytes swallowed** |
| `ESC [` + other | swallowed. So `ESC[H` and `ESC[F` do nothing, and `ESC[3~` (Delete) types a `~` |
| `0x00`, `0x1C–0x1F`, `0x80–0xFF` | nothing (UTF-8 is dropped) |

Consequences, derived from the code:

- **CR vs LF.** A script must send `>cmd\n`. A terminal that sends CRLF inserts a newline and then runs the new empty line, which answers `not a command - start with >` (`ui_text.h:28`).
- **Ctrl-J cannot be typed over the cable.** Byte 0x0A is Run, but Ctrl-J is "previous document" (`editor.c:834`). `docs/TESTING.md:7-8` names Ctrl-J as a way to walk documents.
- Ctrl-H, Ctrl-I and Ctrl-M are also taken, but nothing is bound to them (`editor.c:779-864`).
- Esc needs `ESC ESC`. At the password prompt, Ctrl-C and Ctrl-G also cancel (`ask.h:59-78`).
- Home and End exist only on BLE (`ble_kbd.c:175-176`).
- Pinned by `tools/test_serialkbd_map.c` (`ci.yml:293-302`), which passed when built here.

### 2.4 USB MIDI mode

**`>usb on` / `>usb off`** (`builtins.c:1415-1467`):

1. `usbdev_want(true)` refuses once there have been 3 or more failed attempts this power cycle. It prints `USB failed 3x this power cycle.` / `unplug the deck and try again.` with status `usb: too many failures` (`usbdev.c:214-222`).
2. `doc_save_all_dirty()` runs: journal and SD mirror. This happens **before** `seq_stop()` (`builtins.c:1453-1454`).
3. `seq_stop()`, then `vitals_goodbye(was_usb,…)`.
4. The panel announces `USB MIDI - rebooting now` (or `serial console - rebooting now`) and waits 600 ms.
5. `esp_rom_software_reset_system()` resets without running shutdown handlers (`builtins.c:1090-1118`). This is the fix recorded in `docs/OS.md:550-597`.

`>usb` with no argument prints (`builtins.c:1426-1440`):

- `usb is on|off[, host attached]`
- `usb on   MIDI + console, 1 cable`
- `usb off  back to serial`
- `a power cycle always returns` / `to serial. put >usb on in boot` / `to have it every time.`
- `%u failed attempt(s) this power cycle`

**Boot path** (`main.c:613-680`; `usbdev.c:497-580`):

1. KEY held at boot → USB off, with the message `KEY held - USB MIDI off`.
2. The last boot crashed → USB off, with the message `crashed - USB MIDI off`.
3. `usbdev_boot()`:
   - `tries++`; above 3 it gives up and sets `want=0`.
   - It records whether a USJ host was present, using SOF, before taking the PHY.
   - It installs TinyUSB: full speed, a 4096 B task at priority 5 **on core 1**.
   - It registers a shutdown handler that hands the PHY back.
   - It brings up CDC-ACM with receive and line-state callbacks, and moves `stdout` to CDC.
   - It starts an 8 s trial timer and logs `USB MIDI mode, attempt %u of %u`.
4. Success → the `usb` destination is added and **enabled** (`main.c:669-677`), and the serial keyboard is not started.

**Confirmation and trial.** The state machine is described in `usbdev.c:11-24`. The "ARMED, armed=1" flag it describes does not exist in code; only `s_want` and `s_tries` do.

- The trial is confirmed by `TINYUSB_EVENT_ATTACHED` or by any CDC byte (`usbdev.c:273-275,469-495`). `usbdev_poll` then clears the tries and logs `USB MIDI live` (`usbdev.c:356-365`).
- When the trial expires without confirmation (`usbdev.c:439-463`):
  - If no USB host was seen before the takeover (for example on battery), it re-arms for another 8 s and logs `no host yet; staying armed`. It never reverts in that case.
  - Otherwise it logs `USB did not enumerate - reverting to the console`, sets `want=0`, hands back the PHY, and calls `esp_restart()` (`usbdev.c:367-373`).

**What the port becomes** (`usbdev.c:98-144`):

| field | value |
|---|---|
| class | MISC / common / IAD composite, USB 2.0, bus-powered, 250 mA |
| VID / PID / bcdDevice | `0x303A` / `0x4001` / `0x0100` |
| manufacturer / product / serial | `cyberdeck` / `cyberdeck usb` / `deck-0001` |
| interface 0–1 | CDC-ACM `cyberdeck console`: notification EP `0x81` (8 B), OUT `0x02`, IN `0x82` (64 B) |
| interface 2–3 | MIDI `cyberdeck usb MIDI`: OUT `0x03`, IN `0x83` (64 B) |

- **Console and keyboard over CDC** use the same mapper as §2.3 (`usbdev.c:264-285`).
- **MIDI in from the host is never read**: the firmware has no `tud_midi_*read` call.
- **MIDI out** (`usbdev.c:232-257`):
  - It sends only when `s_active && tud_midi_mounted()`, and carries no timestamp (`main.c:212-221`).
  - A short write counts as `d`. `flush` only counts; its "packets" figure is flushes, and `usbdev_packing()` has no caller.
- **Status** `act%d dev%d midi%d s%u d%u` (`usbdev.c:172-183`) appears in `>send` and in the 10 s heartbeat as `usb: …` (`main.c:945-947`).
- **esptool over CDC** (`usbdev.c:384-428,306-341`):
  - It needs RTS high with DTR low (armed), then RTS falling with DTR high. A plain port close therefore does nothing.
  - A 50 ms timer, then `do_reboot()`: PHY → USJ, set `FORCE_DOWNLOAD_BOOT`, `doc_save_all_dirty()`, `esp_restart()`.

**Ways back to serial:**

| way | reference |
|---|---|
| `>usb off` | `builtins.c:1449-1466` |
| power loss (RTC memory cleared) | `usbdev.c:64-83` |
| hold KEY while booting | `main.c:613-635` |
| any panic or watchdog reboot | `main.c:637-660` |
| 3 unconfirmed boots | `usbdev.c:504-512` |
| trial revert (a host was present, enumeration failed) | `usbdev.c:439-463` |
| `tinyusb_driver_install` failure (falls back without rebooting) | `usbdev.c:530-535` |
| `>flash now` (see the hang risk below) | `builtins.c:1143-1170` |

**It cannot be made automatic, despite three texts saying so.**

- `usbdev.c:80-83`, `builtins.c:1431-1433` and `docs/TESTING.md:533-534` all say to put `>usb on` in `boot`.
- But `boot` runs as GUIDE (`main.c:315`), GUIDE lacks SYSTEM (`cmd.c:23-24`), and `usb` needs SYSTEM (`builtins.c:2323`). The line is refused: `usb: not permitted here`.
- `ui_text.h:61-64` agrees with the code.
- If it were allowed, `>usb on` reboots unconditionally, so the deck would loop.

**Hang risk.** `esp_restart()` from USB MIDI mode can deadlock (`docs/OS.md:550-597`).

- The docs record it as fixed for `>usb` and still open for `>flash now` (`docs/OS.md:656-661`; `docs/TESTING.md:527-529`).
- `esp_restart()` is **also** still used by the esptool path (`usbdev.c:340`) and by the trial revert (`usbdev.c:372`). No doc mentions these two. Whether they hang is **UNVERIFIED**.

**Measured (docs):**

- Jitter at the host, end to end, is 0.03 ms (`README.md:106,121-124`; `docs/GRAPHICS.md:208`; `net.h:9`).
- An untouched deck in USB MIDI mode ran 5 min: 18,601 messages at 62.0/s with no drift (`docs/OS.md:608-610`).
- 49.600 clocks/s for 124 bpm (`docs/NETWORK.md:673-674`). It cites `docs/OS.md`, which does not contain the figure.
- The class-of-device ~4 ms latency figure is marked `[UNMEASURED]` (`docs/OS.md:369`).

### 2.5 BLE MIDI (`blemidi.c`, `blemidi.h`)

- **GATT.** Service `03B80E5A-EDE8-4B33-A751-6CE34EC4C700`; characteristic `7772E5DB-3868-4112-A1A9-F2669D106BF3` with read, write-without-response and notify (`blemidi.h:16-17`; `blemidi.c:14-63`).
  - It is registered through the keyboard's `gatt` hook (`main.c:594`; `ble_kbd.c:1102-1104`) and started from the `synced` hook (`blemidi.c:247-259`).
  - Incoming writes are accepted and ignored (`blemidi.c:39-43`).
- **Off by default.** The deck advertises only while `ble` is on. The main loop copies the destination flag into `blemidi_set_enabled` on every pass (`main.c:820-824`).
  - Off: `BLE MIDI available but off - '>send ble on' to use it`.
  - On: `BLE MIDI on - it shares the radio with the keyboard`.
  - Off again: `BLE MIDI off - the radio is the keyboard's alone` (`blemidi.c:226-259`).
- **Advertising.** Flags and the 128-bit UUID go in the advertisement; the name `cyberdeck` goes in the scan response. It logs `advertising as 'cyberdeck' - pair from Audio MIDI Setup` (`blemidi.c:67-117`).
- **Link.** The deck requests a 7.5 ms interval (6 × 1.25 ms), latency 0 and a 2 s supervision timeout. It logs the granted `link: interval %u us, latency %u, timeout %u ms` and later `link updated: interval %u us, latency %u` (`blemidi.c:119-155,189-198`).
- It sends only when **connected and subscribed** (`blemidi.c:30-33,311-313`).
- **Packet** (`blemidi.c:261-376`):
  - A header `0x80|ts[12:7]`, then per message `0x80|ts[6:0]`, the status, and the data.
  - `ts = ((when_us+500)/1000) & 0x1FFF`, clamped to be non-decreasing inside a packet.
  - A new packet starts when the header changes (every 128 ms) or the packet would exceed `MTU−3` (minimum 5, maximum 64).
  - One notification per flush, which is one per drained tick. Data lengths follow §1.9.
- `>jitter` prints `ble   %u msgs in %u packets (%u.%02ux)` once any packet has gone out (`builtins.c:1371-1377`).
- **Measured.** There is no on-device figure for the granted interval or the packing ratio. `docs/OS.md:371` lists "7.5 ms ±1.8" without a source. `blemidi.h:12` gives "round-trip around 19 ms".
- **Comment errors.**
  - `blemidi.c:263-266`: "a header, then up to five timestamped three-byte messages" in 20 B. 1 + 5×4 = 21, so only four fit.
  - `docs/COMMANDS.md:283-286` says `ble` is on at boot. It is not.
- **Timestamp wrap.** The 32-bit `when_us` wrap (about every 71.6 min) produces one BLE-MIDI timestamp discontinuity. This is inferred, **UNVERIFIED** at the receiver.

## 3. DIN MIDI (`dinmidi.c`, `dinmidi.h`, `builtins.c:151-225`)

- **Command**, `>din <gpio>`:
  - Accepts 1–48 via `atoi`; otherwise it prints `a GPIO number, 1-48` (`builtins.c:196-200`).
  - Refused pins print `GPIO%d is <what>`: **11** `panel SCK`, **12** `panel MOSI`, **5** `panel DC`, **40** `panel CS`, **41** `panel RST`, **18** `the KEY button` (`builtins.c:203-212`; the same list is in `docs/HARDWARE.md:449-453`).
  - **Not refused:**
    - SD card pins 21, 38 and 39 (`sdmirror.c:40-43`). `battery.c:14-17` knows they are taken.
    - The TE line on GPIO6 (`docs/HARDWARE.md:68`).
    - The chip's USB and flash/PSRAM pins. These are not listed anywhere in this repo.
    - What happens if one of these pins is taken is **UNVERIFIED**.
  - A driver error prints `GPIO%d refused: <esp_err_name>` (`builtins.c:213-221`).
  - Success enables `din` and prints `din on GPIO%d`.
- **UART** (`dinmidi.c:11-81`):
  - **UART1** at 31250 baud, 8N1, no flow control, default clock. UART0 is untouched.
  - Transmit only (`uart_set_pin` sets TX alone). The receive buffer is 256 B, required by the driver and unused; the transmit ring is 512 B.
  - Starting on the same pin again does nothing; starting on another pin moves the output.
  - It logs `MIDI out on GPIO%d at %d baud`.
- **Bytes** (`dinmidi.c:93-126`):
  - `F9` is dropped.
  - Otherwise it writes the status plus `d1&0x7F` and `d2&0x7F` as `midi_datalen()` requires.
  - Bytes written are counted.
- **`>din`**:
  - While running: `din GPIO%d, %u bytes sent`, then `the wire is busy` or `nothing sent since last asked`. The counter resets each time it is read (`dinmidi.c:25-30`).
  - While stopped: six help lines, `din <gpio>  MIDI out, no host` … `trust it. e.g. >din 17` (`builtins.c:174-184`).
- **`>din off`** prints `din off`.
- It is not persisted and cannot run from `boot` or `>run` (SYSTEM).
- **"NEVER BLOCK" is not what the call does.** `dinmidi.c:116-121` claims a full buffer loses a byte rather than stalling.
  - IDF's `uart_write_bytes` (IDF `esp_driver_uart/src/uart.c:1629`) goes through `uart_tx_all` (`:1565-1597`), which waits with `portMAX_DELAY` for ring space.
  - Each call stores a 12-byte event item plus a data item in a NOSPLIT ring (IDF `uart.c:1906`, `uart_tx_data_t` at `:122-129`). So 512 B holds roughly 16 three-byte messages, not the ~170 that the comment's "a sixth of a second of slack" assumes.
  - A burst larger than that, such as a stop (pending note-offs + 16× CC 123), **blocks the `midi` task**. That delays `osc`, `view` and `usb`, which come after `din` in the dispatch order.
  - It does not stall the clock. The UART also drains whether or not a cable is attached, contrary to the comment.
  - Derived from source; **UNVERIFIED** on hardware.
- **Circuit** (`docs/HARDWARE.md:407-464`; `dinmidi.h:18-24`):
  - From 5 V: +5 V —220 Ω— pin 4, TX —220 Ω— pin 5, GND — pin 2.
  - From 3.3 V: 33 Ω on pin 4 and 10 Ω on pin 5, or buffer to 5 V and keep the 220 Ω pair.
  - TRS Type A: tip = pin 5, ring = pin 4, sleeve = pin 2. Type B swaps tip and ring.
  - MIDI **in** needs an optocoupler (6N138) and is not built.
- **Measured**:
  - "80 bytes from a four-step kick pattern, verified on the bench" (`docs/HARDWARE.md:409-411`; `docs/TESTING.md:513-514`).
  - "1491 bytes verified" (`docs/NEXT.md:68`).
  - `docs/OS.md:370` gives "0.96 ms, deterministic", a class figure.

## 4. Network (`net.c`, `osc.c`)

### 4.1 State

| flag | meaning | reference |
|---|---|---|
| `s_started` | the driver is up (`wifi_once`) | `net.c:17,71-98` |
| `s_on` | the radio is on | `net.c:21,31` |
| `s_joined` | the station has an IP | `net.c:22,53-69` |
| `s_hosting` | the deck is an access point | `net.c:23` |

`net_up()` means joined or hosting (`net.c:29`). `wifi_once` does four things:

- `esp_netif_init`
- tolerant default event loop
- creates the STA and AP netifs
- `esp_wifi_init`, then **`WIFI_STORAGE_RAM`** (`net.c:91`)

### 4.2 Join: `>wifi <ssid>` (`builtins.c:1879-1968`; `net.c:172-208`)

- **One word only.** A second word is taken as an old-style password: the command refuses with `no passwords on a line - cut` and the editor cuts the line from that column (`builtins.c:1906-1923`; `editor.c:1062-1066`).
  - `<…>` around the name is stripped (`secret_line.h:47-60`).
  - The name is at most 32 characters, and **names with spaces cannot be typed**.
  - The usage message is `wifi <ssid>`.
- **The prompt.**
  - The status line asks `<ssid≤24> password` and echoes stars.
  - It takes at most 63 printable characters. Enter (with any modifier) submits; Esc, Ctrl-C or Ctrl-G cancel with `stopped - nothing sent` (`ask.h:23-105`; `editor.c:1101-1116`).
  - The deck answers `joining %.20s` or `could not start the radio`.
- **At boot the prompt cannot run.** `cmd_set_asker` is called only in `editor_init` (`editor.c:205-207`, `main.c:704`), which runs after the boot document (`main.c:695`). A boot-time `>wifi` therefore gets `nothing here can ask` (`builtins.c:1929-1931`). This contradicts the comment at `builtins.c:1469-1475` that `boot` "can bring the deck onto a network".
- **`net_join`** (`net.c:172-208`):
  1. **Remembers first**: NVS namespace `deck`, keys `ssid` and `pass`, in **plaintext**. NVS and flash encryption are off (`sdkconfig:2409,2793`); see `net.c:159-170,190-193`.
  2. Stops the radio if it is on.
  3. Sets STA mode and the auth threshold: WPA2-PSK if a password was given, otherwise OPEN.
  4. Starts and connects.
- **Events** (`net.c:53-69`):
  - A disconnect retries **immediately and forever**, logging `link lost; retrying`.
  - Getting an IP logs `joined <ssid> as <ip>`.

### 4.3 `>wifi`, `>wifi off`, `>wifi forget`, and the remembered network

| command | effect | reference |
|---|---|---|
| `>wifi` | `net_status` shows `wifi off` or `host\|join <ssid≤12> <ip>`, then `remembered: <ssid≤17>`, then help lines | `builtins.c:1940-1953`; `net.c:40-48` |
| `>wifi off` | `esp_wifi_stop`, driver kept, **credentials kept**, so the network is rejoined at the next boot | `net.c:251-260`; `main.c:687-693` |
| `>wifi forget` | erases `ssid` and `pass` from NVS; the radio is not stopped. Prints `network forgotten` | `net.c:113-123` |
| at boot | `net_rejoin()` logs `rejoining the remembered network`; a bracketed stored name is repaired | `main.c:687-693`; `net.c:125-157` |

### 4.4 Host: `>host <ssid>` (`builtins.c:1970-1984,1894-1903`; `net.c:210-249`)

- It asks for the password the same way as `>wifi`.
- With 8 or more characters it runs WPA2-PSK. With fewer, or none, it runs **OPEN** and logs `password under 8 chars - hosting OPEN instead`.
- Channel 6, at most 4 stations, **AP-only mode**: the station is dropped.
- The reply is `hosting <ssid≤12> <AP address>` or `hosting <ssid≤12> OPEN`. The address is a literal in `net.c:245` and `builtins.c:1901`, omitted here.
- Hosting is not remembered and cannot be started from `boot` (§4.2). There is no NAPT or uplink in the firmware (a grep finds none), although `docs/NETWORK.md:82-85,142-143` calls it essential.

### 4.5 Power-save policy

| event | power save | reference |
|---|---|---|
| joining | untouched, so the IDF default `WIFI_PS_MIN_MODEM` (IDF `esp_wifi/include/esp_wifi.h:674`) | `net.c:172-208` |
| `>osc in <port>` | **off** | `builtins.c:2032-2033` |
| `>osc in off` | back on (MIN_MODEM) **only if the ensemble is off** | `builtins.c:2034-2036` |
| ensemble lead or follow | **off**, and never restored, even by `>sync alone` | `ensemble.c:705-723,674-686` |
| `net_power_save()` before `net.c` has started the driver | does nothing | `net.c:33-38` |

Measured (`docs/NETWORK.md:282-293`): with power save on, 62–265 ms from sending to the filter moving. With it off, 20/20 correct values: **4 ms best, 22 ms median, 55 ms worst**.

### 4.6 OSC in: `>osc in <port>|off` (`builtins.c:2009-2040`; `osc.c:172-274`; `osc_parse.h`)

- It requires `net_up()`; otherwise `osc in needs wifi`. Other messages: `osc in on %ld`, `osc in off`, `osc in <port> | off`, `could not listen`.
- **Listener.** UDP on `INADDR_ANY:<port>` with a 500 ms receive timeout and a 512-byte buffer. Task `oscin`: 3072 B, priority 3, core 0.
  - A restart waits up to 1.5 s for the old task, then fails with `ESP_ERR_TIMEOUT` (`osc.c:201-265`).
  - **There is no authentication** (`osc.c:180-183`).
- **What is accepted** (`osc_parse.h:43-203`):
  - The datagram length must be positive and a multiple of 4.
  - Either a `#bundle` (tag + 8-byte time tag; elements size-prefixed, size > 0, multiple of 4, within the datagram; depth < 4), or messages back to back, each starting with `/`.
  - An address of 1–47 characters. No type-tag string means no arguments.
  - Type tags: `i c r m f` (4 bytes), `h d t` (8 bytes), `s S`, `b`, `T F` (count as 1/0), `N I [ ]` (no data). Any other tag refuses the whole datagram.
  - It is validated in full first, then delivered.
- **Value** (`osc_parse.h:205-220`): the last numeric argument (`i f h d T F`). With none it is 127. A float or double in [0,1] is multiplied by 127; anything else is clamped to 0–127 and rounded.
- **Mapping.**
  - `/deck/<name>` (the rest of the address) → `seq_input_set(name, value, source IPv4)` (`osc.c:189-199`).
  - Only defined inputs count: `>n = knob` or `>n = pad`, up to 16 (`seq.h:393-427`).
  - A knob publishes on the next tick. A pad fires its routes on the next (swung) step, and a pad value of 0 is a release that does nothing (`seq.c:781-795,819-836`).
- **Counters** (`osc.c:187-199,213-215`):
  - *read*: every message parsed.
  - *used*: the name was defined.
  - *refused*: the datagram was malformed.
  - `>osc` shows `in on %d: %u read, %u used` and `  %u datagrams refused`.
  - `>lanes` lists `<name> knob|pad <value> .<o3>.<o4>` (`builtins.c:2205-2226`).
- It is not persisted. From `boot` it fails, because `net_up()` is still false while the join is in flight (inferred).
- Measured: §4.5; `docs/TESTING.md:197-201`.

### 4.7 Radio sharing

`kbd_share_radio(net_radio_on() || ensemble_role() != ENSEMBLE_OFF)` runs on every loop pass (`main.c:865-867`); see §7.

## 5. The ensemble (`ensemble.c`, `ensemble.h`, `ens_count.h`)

### 5.1 `>sync` (`builtins.c:948-1046`); all modes are EDIT, so they are allowed in `boot`

| argument | effect |
|---|---|
| `on` / `off` | MIDI clock out (`seq_sync`, §1.2). Prints `midi clock on\|off` |
| `lead` / `follow` | `ensemble_set()`. Prints `leading\|following the ensemble.`, `no network needed - the decks`, `talk to each other directly.`, `one deck leads, the rest follow.`, status `sync lead\|follow`. On failure: `the radio would not start` |
| `alone` | ensemble off. Prints `sync alone` |
| (none) | the report (§5.8) |
| other | `sync on \| off \| lead \| follow \| alone` |

### 5.2 Bring-up (`ensemble.c:669-763`)

- `esp_netif_init` and `esp_wifi_init`. Both are idempotent in IDF 5.5.4 (IDF `esp_netif/lwip/esp_netif_lwip.c:534`, `esp_wifi/src/wifi_init.c:348-352`).
- `esp_wifi_set_mode(WIFI_MODE_STA)`, `esp_wifi_start`, power save **NONE**.
- `esp_now_init`, add the broadcast peer on channel 0 (the current channel), register the receive and send callbacks, and read the station MAC.
- The role is set and every counter reset. It logs `leading` or `following`.
- Off: callbacks unregistered, `esp_now_deinit`, `seq_follow(false)`, peers cleared. **Wi-Fi is not stopped** (`ensemble.c:674-686`).

### 5.3 Packet (`ensemble.c:22,72-115`)

It is packed and **27 bytes**, confirmed by compiling the struct on the host. `ensemble.c:14` says "twenty bytes" and `docs/NETWORK.md:482-483` says 26.

| field | type | meaning |
|---|---|---|
| `magic` | u32 | `0x4B4C5233` "KLR3" |
| `kind` | u8 | 1 BEACON, 2 PROBE, 3 REPLY |
| `tick` | u32 | the sender's pulse count since play |
| `ahead_us` | i32 | send time − due time of that pulse |
| `tag` | u32 | probe id (PROBE/REPLY) |
| `whom[2]` | u8 | the asking deck's MAC bytes 4–5 (REPLY) |
| `resid_us` | i32 | how long the leader held the probe |
| `bpm` | u16 | |
| `running` | u8 | playing and grid anchored (`ensemble.c:812-813`) |
| `role` | u8 | written, **never read** |

### 5.4 Traffic

- **Leader**:
  - Broadcasts a BEACON every 125 ms (8/s) (`ensemble.c:119,849-861`).
  - Answers probes from the main loop with `resid_us`, by unicast. A peer is added when needed (`ensemble.c:828-847`).
  - Holds one pending probe; a newer one replaces it (`ensemble.c:580-590`).
- **Follower**:
  - Starts probing only after it has heard a beacon (`ensemble.c:556-562,865`).
  - Sends a unicast PROBE to the leader every **50 ms (20/s)** (`ensemble.c:192,863-893`). The comment at `ensemble.c:863` says "ten times a second".
  - Sends no beacon. `BEAT_EVERY_US` (`ensemble.c:128`) is unused, and "a follower broadcasts too" (`ensemble.c:106-113`) is not what the code does.
  - Tags wait in a 4-deep queue matched by the send callback (`ensemble.c:271-299,640-667`).
- **Receive filter**: exact size and magic. Peers are kept by MAC, 8 slots, stale after 5 s (`ensemble.c:142-143,352-387,524-536`).
- A follower adopts the sender of **any** beacon as its leader (`ensemble.c:560-562`).

### 5.5 Phase (`settle()`, `ensemble.c:391-519`)

- `rtt = (rx − air) − resid_us`, accepted in 0–200 000 µs. `air` is the time the send callback reported success.
- An unacknowledged probe is counted as *lost* and dropped (`ensemble.c:654-666`).
- A second copy of a reply is *dup*; a reply to a forgotten probe is *stale*; a reply meant for another deck is ignored (`ensemble.c:596-614`).
- A window is 24 samples. It keeps the 6 with the lowest RTT; with fewer than 3 kept it skips. Otherwise it takes the median and the spread (`ensemble.c:190-191,476-482`).
- Gain by spread:

  | spread | gain |
  |---|---|
  | ≤300 µs just after a tempo change ("snap") | 1 |
  | ≤300 µs | ½ |
  | ≤1200 µs | ¼ |
  | ≤4000 µs | ⅛ |
  | more | skip |

- Then `seq_nudge_by(med/gain, bpm)` slides the grid, and the ticks follow the grid (`seq.c:1043-1053`; `seq_clock.h`). The RTT floor relaxes by `/64+1` for each window applied (`ensemble.c:501-518`).

### 5.6 Tempo

- A beacon from a playing leader → `seq_nudge_by(0,bpm)` → `seq_bpm()` if the tempo differs (`ensemble.c:574-576`; `seq.c:1049-1052`). This is called from the Wi-Fi task.
- A tempo change also discards the current window and sets *snap* (`ensemble.c:545-552`).

### 5.7 Count sharing (`ens_count.h`; `ensemble.c:443-470`; `seq.c:866-890,1023-1041,1548-1550`)

- `ens_phase()` folds the offset into ±½ pulse. `ens_count_ahead()` = their_tick − our_tick − (their_due − our_due − phase)/period (`ens_count.h:18-37`).
- **Adoption rules**:
  - Only replies with **the leader running, this deck running, and RTT under 2000 µs** count.
  - **Three consecutive equal** results set `s_count_off`, which is what `>sync` reports.
  - If that value is non-zero, or the deck is still waiting, it calls `seq_adopt(d)` and sets the agreement counter to −3.
- `seq_adopt` acts at the start of the next tick: `s_tick += d`, the grid slides by −d periods, and counted lanes wait for their next downbeat (`seq.c:866-883`).
- A follower that presses `>play` while its leader is playing stays silent for up to one second of ticks, until the count is adopted (`seq.c:1548-1550,884-886,920-924`).
- `seq_follow(leader running)` is refreshed on every beacon and reply (`ensemble.c:538-543`).

### 5.8 What `>sync` reports (`builtins.c:983-1035`)

```
midi clock out: on|off
leading|following, N other deck(s)          (or: playing alone)
-- following only --
off by <err> us, <n> packets                (packets since last asked; resets)
best trip <floor> us, <n> skipped
probes agreed within <spread> us
in the leader's count | steps together, bar <b> off | steps <p> pulses apart
<r> replies, <d> twice, <l> lost ack
<w> corrections, <s> stale
-- leading only --
<n> packets since asked
sync on | off   midi clock
sync lead | follow | alone
```

The status message is `clock on|off`. The example in `docs/TESTING.md:489-493` matches the first lines.

### 5.9 Caveats (derived from code; **UNVERIFIED** on hardware)

- **The ensemble can take down a hosted network.** `ensemble_set` forces `WIFI_MODE_STA` (`ensemble.c:703`). A deck that is hosting (`WIFI_MODE_AP`, `net.c:239`) therefore loses its access point on `>sync lead` or `>sync follow`, while `net.c` still reports `host`. The comment at `ensemble.c:689-694` says an existing network is left alone.
- **The reverse order breaks the ensemble.** `>host` after `>sync` switches to AP-only, while the ESP-NOW peers sit on the STA interface (zero-initialised `ifidx`).
- **`>sync alone` leaves the radio on.** Wi-Fi stays started with power save off. `net_radio_on()` is false, so the keyboard scan goes back to full duty beside a live Wi-Fi radio.
- **The driver may write NVS.** When the ensemble starts Wi-Fi first, it never calls `esp_wifi_set_storage(WIFI_STORAGE_RAM)` (`ensemble.c:697-704` vs `net.c:91`), and `CONFIG_ESP_WIFI_NVS_ENABLED=y` (`sdkconfig:1772`). The driver may then persist its mode to NVS; see [system.md](system.md) §7.
- **Dead code.** `seq_nudge()` (`seq.c:995-1021`), the old version with the `while` fold, has no caller.

### 5.10 Measured (`docs/NETWORK.md`)

- The phase estimator's history is tabulated at `:427-437`. The shipping version: mean −12 µs, spread 55 µs, worst 35 µs, 22/22 samples within 500 µs; while drawing, −8/25/25 µs, 10/10.
- "32 of 32 samples inside 500 µs, worst 35 µs, across two tempo changes" (`:513-515`).
- Ticks against the grid, after the fix: mean +29 µs, sd 4 (`:541-547`).
- The count: before, 3, 4 and 14 pulses apart, kicks 77 ms apart. After, "in the leader's count" with kicks at 0, −5 and −1 ms (`:595-600`).
- Delivery: leader → follower 86 %, follower → leader about 21 % (`:650-654`).

## 6. SSH (`ssh.c`, `ssh.h`, `ssh_fp.h`, `ssh/CMakeLists.txt`, `builtins.c:1753-1877`)

**Usage.** `>ssh user@host[:port] <command>`. It needs `net_up()` (`no network. try: wifi <ssid>` / `ssh needs wifi`) and allows one session at a time (`one ssh at a time`).

- **Arguments.** The port is 1–65535 and defaults to 22. The user is at most 32 characters, the host 63, the command 127 (`builtins.c:1757-1799,1817-1867`). A malformed address prints `ssh user@host <command>`.
- **The prompt.** It asks `<host≤24> password` and returns `type it, Enter. Esc stops`.
  - If the typed secret occurs anywhere in the command text, **nothing is sent** and it prints `that password is on the line`. This is a substring test, so a short password can trip it (`builtins.c:1762-1777`).
  - Otherwise the result is `ssh: connecting`, `one ssh at a time` or `ssh could not start`.
- **With no argument** it prints nine help lines, from `ssh user@host <command>` to `changed it: ssh forget <host>` (`builtins.c:1803-1815`).

**The session** (`ssh.c:199-392,394-428`):

- **Its own task.** `ssh`, 16 KB stack **in PSRAM**, priority 1, core 0. The libssh2 session is allocated in PSRAM (`ssh.c:45,80-93,421-422`).
- **Order of work:**
  1. `$ <command>`, then resolve the host (IPv4).
  2. Connect with a **5 s** bound (`no answer in 5 seconds`, or `connection refused`).
  3. `libssh2_session_handshake` with a **15 s** library timeout.
  4. Check the host key.
  5. `libssh2_userauth_password`; the password is wiped straight after use.
  6. Open an exec channel, run the command, read the reply.
- **Reply limits.** Lines are split at 127 characters, CR is dropped, and output stops at **200 lines** (`... truncated at 200 lines`).
- **Where the reply goes.** Every line passes through an 8 KB message buffer in PSRAM (`say()` drops a line after 1 s if the buffer is full) to `ssh_service()`, which appends it to **`+out`** from the main loop (`main.c:879-892`; `ssh.c:430-467`). When the session finishes, the view jumps to `+out` at the start of the reply.
- **Status strings** (`ssh.c:25,217-371`):

  | string | when |
  |---|---|
  | `ssh idle` | no session yet |
  | `ssh: connecting` | started |
  | `ssh: no such host`, `ssh: no socket`, `ssh: no answer`, `ssh: refused` | before the handshake |
  | `ssh: no host key`, `ssh: host key changed` | at the key check |
  | `ssh: auth failed` | password refused |
  | `ssh: %d line(s)` | finished |

  A handshake failure, a library-init or session-allocation failure, a channel refusal or an exec failure leaves the status at `ssh: connecting`.

**Host key: trust on first use** (`ssh.c:95-156,266-305`; `ssh_fp.h`):

- **Where it is kept.** NVS namespace **`sshkeys`**, key `k%08x` = FNV-1a over `"host:port"`, value a 32-byte SHA-256 blob. Two colliding hosts share a slot, and the second is refused as a changed key (`ssh_fp.h:38-51`).
- **Displayed as** the key type (`key ssh-rsa`, `key ecdsa-sha2-nistp256|384|521`, or `key of another type`), then the fingerprint `SHA256:<base64>` in `ssh-keygen -lf` format (`ssh_fp.h:16-36`). Both are also logged. ed25519 host keys are not usable with libssh2's mbedTLS backend (`docs/NETWORK.md:74-81`).
- **First contact.** `first time here: keeping it`. The key is written from the main loop **after** the session ends, whether or not the login succeeded (`ssh.c:297-304,450-458`). A failed write adds `the key could NOT be kept` to `+out`.
- **The same key again**: `the key it had last time`.
- **A changed key** is refused before any password is sent:
  - `THE HOST KEY HAS CHANGED.`
  - `refused - no password sent.`
  - `if you changed it yourself:`
  - `>ssh forget <host>`
- **`>ssh forget <host[:port]>`** erases the slot and prints a line: `key forgotten: <host>:<port>` or `no key kept for <host>:<port>` (`builtins.c:1820-1848`).

**The non-blocking fix.**

- `HAVE_O_NONBLOCK=1` is a compile definition (`ssh/CMakeLists.txt:23-33`).
- At run time the firmware logs `the ssh socket is still blocking - libssh2 was built without HAVE_O_NONBLOCK, and a read can wait for ever` if the socket is blocking after the handshake (`ssh.c:258-265`).
- CI greps for the define (`ci.yml:116-122`).
- The history is in `docs/NETWORK.md:233-243`.

**Limits.** Password authentication only; key authentication is proposed, not built (`docs/NETWORK.md:245-249`). Exec only, no terminal (`docs/NETWORK.md:94-110`).

**Measured** (`docs/NETWORK.md`):

- Free internal heap: about 98 KB before Wi-Fi, 15–16.5 KB with the station up, 14.2 KB as an access point (`:131-135`).
- An address nobody answers: `no answer in 5 seconds` at 5.0 s, while the editor kept 194 turns/s (`:193-198`).
- The handshake takes about 1 s with about 14 KB internal free (`:209-212`).
- It has run through a login against OpenSSH 10.3; the 200-line bound holds (`:222-231`).

**Stale texts.** `builtins.c:1533-1544` still describes the password on the line, a blocking session on the editor task, and `+ssh`. `docs/TESTING.md:554` says SSH "has never been run against a real server", which contradicts `docs/TESTING.md:412` and `docs/NETWORK.md:203-231`.

## 7. Keyboard: BLE HID host (`ble_kbd.c`, `kbd.h`, `keymap.c`)

- **Discovery** (`ble_kbd.c:663-674,743-759`). The deck connects to an advertiser that shows the keyboard appearance `0x03C1` or the HID UUID `0x1812`, to any directed advertisement, or to an address that is already bonded. The scan connection times out after 10 s.
- **Pairing.** IO capability *display only*, with bonding, MITM protection and Secure Connections. Both sides distribute ENC and ID keys (`ble_kbd.c:1082-1097`).
  - The passkey is a random 6 digits.
  - It is logged as `TYPE THIS ON THE KEYBOARD: %06u`.
  - The main loop draws `PAIR THE KEYBOARD`, the digits in reverse video, and `Type that on the` / `keyboard, then press` / `Enter.` (`ble_kbd.c:847-860`; `main.c:400-430,785-791`).
  - Numeric comparison is auto-accepted (`ble_kbd.c:861-864`).
  - When the keyboard re-pairs, the stale bond is deleted and pairing retried (`ble_kbd.c:872-882`).
- **Bonds** persist through NimBLE's NVS store (`sdkconfig.defaults:52`; at most 4 bonds, `:50`).
  - `pair_ver` = 2 in NVS `deck`. A different scheme clears all bonds once (`ble_kbd.c:109-110,1064-1080,985-988`).
- **The known keyboard.**
  - NVS `deck` / `peer` holds the identity address. It is saved **on every successful subscription**, that is every (re)connect (`ble_kbd.c:497-513,922-935`), and loaded at sync (`ble_kbd.c:937-951`).
  - A directed connect to it has a 20 s timeout and alternates with scanning (`ble_kbd.c:953-972,779-788,884-888`).
- **The link.**
  - The deck subscribes to every notifiable Report and Boot Keyboard Input characteristic, up to 8 (`ble_kbd.c:540-608`).
  - It writes Exit Suspend (`0x01`) to the HID Control Point when the keyboard has been quiet 4 s, at most every 2 s.
  - It **tears the link down after 60 s with no report** (`ble_kbd.c:54-76,251-326`).
  - Key repeat starts after 400 ms, repeats every 45 ms, and is capped at 400 repeats.
  - The input queue is 256 events; drops are counted and logged as `input queue full - %d event(s) dropped` (`ble_kbd.c:129-138`).
- **States** (`kbd_state_name`): `starting`, `scanning`, `connecting`, `pairing`, `discovering`, `connected`, `no reports`, `reconnecting`.
- **`>kbd`** (`builtins.c:243-281`, SYSTEM):
  - `a keyboard is connected` or `no keyboard connected`.
  - `state: %.20s`.
  - `bonds: %d remembered`, or `bonds: the store did not say`.
  - When not connected, one of `it is paired but not in range` / `nothing has ever paired here`, then `the cable is a keyboard too` and `kbd forget to pair another`.
  - Status message `connected|not connected, N bond(s)`.
- **`>kbd forget`**, or **holding KEY for 2 s** (`main.c:51-52,793-801`):
  - `kbd_forget_all()` terminates the link, runs `ble_store_clear()` (**every** bond, including a BLE-MIDI host's), erases `peer`, and scans again (`ble_kbd.c:1022-1041`).
  - It prints `bonds dropped, scanning again.`, `put the keyboard in pairing`, `mode now. any HID keyboard`, `works - full size included.` and the status `forgotten - pair one now`.
- **Scan sharing** (`kbd.h:82-85`; `ble_kbd.c:895-920,1009-1020`):
  - When Wi-Fi or ESP-NOW is on, the scan uses a **30 ms window every 160 ms**. Otherwise it uses NimBLE's defaults, which the comment describes as 30 ms every 30 ms.
  - A scan already running is restarted with the new duty.
  - The directed connect to the known keyboard is not throttled.
  - Measured: see §1.6 (`docs/NETWORK.md:265-280`). How long a keyboard takes to reconnect while Wi-Fi is on is **UNVERIFIED** (`:279-280`).
- **Stale comment.** `ble_kbd.c:1099-1100` says "we never advertise, and the peripheral role is compiled out". BLE MIDI advertises, and the peripheral role is enabled (`sdkconfig.defaults:43-49`).
