# The system — how the firmware is put together

*Part of [the deck, top to bottom](README.md). Snapshot: commit `85d1e6a`, 2026-09-28,
ESP-IDF 5.5.4. Every claim cites `file:line`; a bare file name (`seq.c:1072`) is unique
under `firmware/`. Source is the truth: where a comment or a document disagrees, the
disagreement is listed in [errata.md](errata.md). UNVERIFIED means the repository alone
cannot confirm it.*

## The shape in one picture

```
 keys   BLE HID (nimble_host, CPU0) ─┐
        USB-Serial-JTAG (serialkbd) ─┼─► kbd queue, 256 ─► MAIN LOOP (CPU0, prio 1)
        TinyUSB CDC (USB-MIDI mode) ─┘                      editor ─► cmd ─► seq / docstore / net / ssh / viz
 clock  esp_timer task (CPU1, prio 22): seq tick() ─► midi queue, 64 ─► "midi" task (CPU1, prio 6)
            │                                                   └─► dests: ble mon din osc view [usb] + flush
            └─► viz_mark / viz_mark_param (record only) ─► main loop: viz_service ─► pane + view_frame
 store  docstore: gap buffers (PSRAM) ─► journal (flash "notes" partition) ─► SD mirror (/sdcard)
```

## 1. Firmware map

### 1.1 Components — `firmware/components/` (14)

| Component | Files (lines) | Responsibility | Public interface | REQUIRES |
|---|---|---|---|---|
| **st7305** | st7305.c 695 · st7305.h 126 · st7305_addr.h 110 | ST7305 reflective-LCD driver (derived from SolarOS, `firmware/NOTICE`). SPI2 @ 24 MHz write / 6 MHz 3-wire read (st7305.c:32-34), hand-driven CS, recursive bus mutex (st7305.c:72-86), 15,000 B framebuffer in controller order, 4-rectangle damage list (st7305.c:58), CASET-mirrored window push (st7305.c:550-599), HPM/LPM with 1 s idle drop (st7305.c:37,158-184), 4 orientations | `st7305_init/framebuffer/pixel/fill/pixel_raw/fill_raw/row_bits_raw/clear/damage/flush/flush_full/set_power_policy/service/set_hpm/set_lpm/set_inverted/read_id/set_orientation` (st7305.h:56-122); pure geometry + `st7305_window()` (st7305_addr.h:23-108) | driver esp_timer (st7305/CMakeLists.txt:3) |
| **textgrid** | textgrid.c 417 · font12x24.c 2991 · font6x12.c 148 (both generated) · textgrid.h 113 · tgfont.h 44 · font6x12.h 37 · font12x24.h 6 | Character grid on the panel. Enforces cell height/origin-y multiple of 12, width/origin-x even (textgrid.c:53-62). Per-cell dirty bitmap; attributes `TG_INVERSE` cursor, `TG_UNDER` playhead, `TG_OVER` recognised command (textgrid.h:29-61). Draws from a pre-turned glyph cache (36 byte stores a cell) with a per-row fallback (textgrid.c:240-380). Faces 12×24 and 6×12, codes 32-155, 128-155 = tiles | `tg_set_layout/set_font/origin_x/y/cols/rows/cell_w/h/font_name/clear/put/puts/fill/puts_right/render/flush/invalidate/set_turned/draw_text_px/text_width_px` (textgrid.h:71-113); `tg_font_12x24`, `tg_font_6x12` (tgfont.h:41-44) | st7305 |
| **kbd** | ble_kbd.c 1108 · keymap.c 52 · keymap.h 4 (private) · kbd.h 116 · serialkbd_map.h 100 | Initialises NimBLE and runs a HOGP keyboard central: directed reconnect to the NVS-remembered peer, else scan (ble_kbd.c:956-972). Passkey-entry MITM + SC bonding (ble_kbd.c:1082-1094). Boot-protocol reports, synthesised repeat 400/45 ms (ble_kbd.c:50-52), exit-suspend keep-alive, 60 s dead-link rebuild (ble_kbd.c:76). Scan duty cut to 30 ms/160 ms while Wi-Fi/ESP-NOW is on (ble_kbd.c:898-920). One 256-deep input queue for every key source (ble_kbd.c:1049). Pure serial byte→event mapper | `kbd_set_ble_hooks/init/poll/connected/share_radio/mods/forget_all/bond_count/inject/on_passkey/state_name`, `kbd_event_t` (kbd.h:19-116); `skb_feed` (serialkbd_map.h:47-98) | bt esp_timer nvs_flash esp_hw_support |
| **blemidi** | blemidi.c 376 · blemidi.h 66 | BLE-MIDI 1.0 GATT peripheral on the same NimBLE host. **Off by default** — no advertising until `>send ble on` (blemidi.c:226-258). One notification per step with clock timestamps (blemidi.c:294-376); requests a 7.5 ms interval | `blemidi_register` (GATT hook), `blemidi_start` (sync hook), `set_enabled/enabled/send/flush/connected/packing` (blemidi.h:25-66) | bt esp_timer |
| **dinmidi** | dinmidi.c 126 · dinmidi.h 53 | 31,250-baud MIDI out on UART1, on a declared pin (`>din <gpio>`) (dinmidi.c:14-16,32-81). Byte counts from `midi_len.h`, drops 0xF9 (dinmidi.c:106-114) | `dinmidi_start/stop/pin/running/send/bytes` (dinmidi.h:40-53) | esp_driver_uart seq |
| **usbmux** | usbmux.c 50 · usbmux.h 56 | Owns the RTC_CNTL USB PHY mux bits; hands the PHY back to USB-Serial-JTAG (usbmux.c:30-40) | `usbmux_release_to_usj`, `usbmux_take_for_otg`, `usbmux_try_count/bump/clear` (usbmux.h:43-56; the try API is unused) | soc hal esp_hw_support |
| **usbdev** | usbdev.c 580 · usbdev.h 55 | TinyUSB CDC-ACM + MIDI composite, VID 0x303A / PID 0x4001 (usbdev.c:90-137). USB-MIDI intent lives in RTC_NOINIT, so power loss clears it (usbdev.c:149-151); 3 tries max, 8 s trial timer (usbdev.c:85-91). Console moves to CDC (usbdev.c:548-559). The esptool DTR/RTS pattern reboots to the ROM loader (usbdev.c:384-432). Timer callbacks only set flags; `usbdev_poll()` does the work (usbdev.c:343-382) | `usbdev_boot/poll/mounted/midi_send/midi_flush/want/wanted/tries/packing/status` (usbdev.h:15-55) | kbd docstore usbmux esp_timer nvs_flash driver soc espressif__esp_tinyusb |
| **seq** | seq.c 1640 · seq.h 572 · seq_pattern.h 908 · seq_clock.h 46 · seq_scale.h 121 · midi_len.h 59 | The sequencer. 16 lanes × 64 slots, ≤96 compiled events a lane (seq.h:39-127); bindings note/cc/viz; parts vel/oct; routes; OSC inputs. 96 PPQN clock on a grid-anchored one-shot esp_timer (seq.c:840-958). MIDI clock every 4th tick, SPP, 0xF9 step marker, swing, ensemble follow/adopt. ≤8 destinations drained by the `midi` task (seq.c:193-241). Self-measured clock and transport jitter (seq.h:520-547). Pure headers: pattern compiler, next-tick arithmetic, scales, MIDI lengths | seq.h:199-572 (lanes, bpm/play/stop, timebase/nudge/follow, inputs, `seq_dest_*`, stats, draw/param hooks) | esp_timer |
| **viz** | viz.c 1115 · viz.h 213 | 16 drawing primitives (viz.c:9,870-876): fields noise disc box turn ramp grid; levels mask edge; bends echo move spin warp grow thin flip fold (ui_text.h:186-190). Frame ≤ 60×24 cells (viz.h:65-66) in tile glyphs 128-155. Marks are recorded from the clock hooks; the frame is computed in the main loop, in table/route order (viz.c:1003-1061). Split-pane geometry; ASCII (`viz_text`) and raw (`viz_frame`) export | viz.h:70-213 | seq |
| **docstore** | buffer.c 420 · journal.c 556 · sdmirror.c 165 · undo.c 231 · docstore.h 144 · mirror_path.h 52 | ≤8 gap buffers of 128 KB, in PSRAM and loaded lazily (buffer.c:46-80; docstore.h:21-23). Kinds prose/guide; `+name` buffers are transient, never journalled. Append-only, CRC'd, sector-aligned snapshot journal in the `notes` partition, with per-name live-sector allocation and no wrap (journal.c:1-27,78-112,424-506). SD mirror, 1-bit SDMMC at 400 kHz, `/sdcard/<name>.txt` via `mirror.tmp` rename (sdmirror.c:41-49,59-110). Coalescing operation-log undo/redo (undo.c:28-58) | docstore.h:56-144; `mirror_path()` (mirror_path.h:21-50); `sdmirror_init()` not in the header, declared at main.c:70 | esp_partition fatfs esp_driver_sdmmc sdmmc vfs |
| **cmd** | cmd.c 324 · builtins.c 2350 · cmd.h 152 · lane_name.h 292 · secret_line.h 61 | Dispatcher: `>` sigil, first word, unparsed rest (cmd.c:214-315). Caller capabilities: HANDS all, GUIDE read/edit/store/net, AGENT read/edit/store (cmd.c:19-29). Definitions (`>kick = note 36`) and lane lines; old-spelling hints; secret-asker and announce hooks. **34 built-in verbs** (builtins.c:2308-2343). Pure headers: lane address/definition grammar, secret-line split | cmd.h:89-152; `lane_name_parse/lane_def_parse…` (lane_name.h:78-292); `first_word_rest/unbracket` (secret_line.h) | docstore textgrid seq usbmux blemidi usbdev net ssh viz dinmidi ensemble kbd **main** (cmd/CMakeLists.txt:3) |
| **net** | net.c 260 · osc.c 274 · net.h 93 · osc_pack.h 90 · osc_parse.h 222 | Wi-Fi: join as STA or host a softAP; credentials in NVS `deck`; rejoin at boot; power-save control (net.c:71-260). OSC-out destination `/deck/<lane>`, `/deck/step`, `/deck/frame`, batched per step (osc.c:53-187). OSC-in listener task → seq inputs (osc.c:189-266). Pure pack/parse headers | net.h:31-93; `osc_*` (osc_pack.h:25-90, osc_parse.h:37-222) | esp_wifi esp_netif esp_event nvs_flash lwip esp_timer freertos |
| **ensemble** | ensemble.c 894 · ensemble.h 84 · ens_count.h 37 | Shared clock across decks over ESP-NOW. The leader beacons 8×/s; followers send unicast probes 20×/s, keep min-RTT samples and trim seq's grid via `seq_nudge/adopt` (ensemble.c:119-192,821-894). Runs from the main loop, never the clock | `ensemble_set/role/state/service/floor_rtt/skipped/count_off/spread/counts` (ensemble.h:40-84); `ens_count_ahead/ens_phase` (ens_count.h:22-37) | seq esp_wifi esp_netif |
| **ssh** | ssh.c 469 · ssh.h 54 · ssh_fp.h 51 · priv/libssh2_config.h 20 · vendor/ (libssh2 1.11.1, 461 tracked files) | Runs one command on a host with libssh2 over mbedTLS, in its own PSRAM-stack task. Output reaches `+out` through a message buffer. The host key is pinned on first use in NVS `sshkeys` (ssh.c:96-155). The password is asked for, never read from a line | `ssh_start/service/busy/status/forget` (ssh.h:38-54); `ssh_fp_text/slot` (ssh_fp.h:19-51) | mbedtls lwip esp_timer; PRIV docstore nvs_flash; defines incl. `HAVE_O_NONBLOCK=1` (ssh/CMakeLists.txt:17-37) |

### 1.2 `firmware/main/` — the app component

| File (lines) | Responsibility | Interface |
|---|---|---|
| main.c 1026 | `app_main`: the boot sequence and main loop; the ble/usb/mon destinations; boot and guide documents; passkey screen; KEY button; autosave; heartbeat; render benchmarks | `app_main` (main.c:432), `kbd_on_passkey` (410), `editor_now_ms` (398) |
| editor.c 1185 · editor.h 36 | The editor. Two densities: 12×24 gives 30 cols × 11 text rows + status; 6×12 gives 60 × 23 + status (editor.c:167-189). Bounded greedy wrap: 2048 lines, 6000 B window (editor.c:63-69). Status bar, playhead spans, viz split pane, Ctrl chords. Ctrl+Enter runs a line and Enter always inserts (editor.c:1135-1162). `+out` view; secret prompt | editor.h:8-36 |
| ask.h 105 | Pure secret-prompt state: stars on screen, wiped on Enter and on Esc | `ask_start/feed/render/end/wipe` (ask.h:37-105) |
| battery.c 176 · battery.h 31 | ADC1 one-shot on a declared GPIO (NVS `bat_gpio`, `bat_div`); 18650 discharge curve; `>battery` scans the free channels | `battery_percent/mv/scan/use` (battery.h:21-31) |
| cell_attr.h 33 | One composition of the cursor, playhead and command bits | `cell_attr()` (cell_attr.h:26-31) |
| selftest.c 142 · selftest.h 5 | 4-stage journal power-cut test across resets. **Compiled in but disabled** (`DECK_JOURNAL_SELFTEST 0`, main.c:585) | `selftest_run/reset` |
| serialkbd.c 77 · serialkbd.h 3 | Installs the USB-Serial-JTAG driver (RX 256 / TX 4000) and routes the console through it. Task `serialkbd` → `skb_feed` → `kbd_inject` | `serialkbd_init` |
| splash.c 95 · splash.h 4 | "KILROY" boot screen; shows the doc count, free internal heap and battery (splash.c:79-86) | `splash_show` |
| testcard.c 66 · testcard.h 2 | Test card with orientation tell-tales and the build ID | `testcard_draw` |
| ui_text.h 226 | Fixed strings with 30-column `_Static_assert`s (ui_text.h:220-224). `BOOT_NAMES`/`BOOT_TEXT` (74-104); `GUIDE_TEXT` "guide 3" (118-218) | macros |
| view.c 94 · view.h 8 | The `view` destination. Packs `viz_frame` into DKV1, base64-wraps it in `ESC ] view;…BEL` and writes it to the console: a non-blocking USJ write, else stdio (view.c:53-94) | `view_init/view_frame`, `view_frames/view_dropped` |
| view_wire.h 81 | Pure DKV1 pack + base64; the node reads it with `view/deckview/view_read.h` | `view_wire_pack/base64` |
| vitals.c 113 · vitals.h 74 · vitals_verdict.h 62 | NVS `deck/vitals` record, written every boot, every 60 s in USB-MIDI mode, and on a deliberate restart. Yields a 4-line verdict on the previous run | `vitals_begin/started/previous/loop/goodbye/report`; `vitals_verdict()` |
| CMakeLists.txt 5 · idf_component.yml 13 | Registers `main`; depends on `espressif/esp_tinyusb ^2.0.0` (locked 2.3.0, tinyusb 0.21.0~2 — dependencies.lock:3-33) | — |

**Project CMake** (`firmware/CMakeLists.txt:34-37`) overrides TinyUSB's `CFG_TUD_MIDI_TX_EPSIZE=4`, so every note flushes at once rather than waiting for 64 bytes.

## 2. Boot sequence — `app_main` (main.c:432-753)

| # | Step | Where |
|---|---|---|
| 1 | RTC_NOINIT boot-loop counter: reset on power-on, bumped on panic/WDT resets; sets `s_crashed_last_boot` | main.c:447-460 |
| 2 | Log the reset reason (`boot: …`) | main.c:466-475 |
| 3 | Clear `RTC_CNTL_OPTION1_REG`, the force-download bit left by `>flash` | main.c:477 |
| 4 | `usbmux_release_to_usj()` — give the PHY back to USB-Serial-JTAG; the only restore that survives a panic | main.c:484 |
| 5 | Log the build ID; `report_memory("boot")` | main.c:486-487 |
| 6 | `nvs_flash_init()` (erase and retry on no-free-pages / new-version) | main.c:489-494 |
| 7 | `vitals_begin()` — read and log the previous run's last record | main.c:500 |
| 8 | `st7305_init()` — GPIO, SPI2 bus, two devices, mutex, 2 × 15,000 B DMA buffers, reset + register init, idle timer. **On failure `app_main` returns** | main.c:502-505; st7305.c:280-354 |
| 9 | `st7305_read_id()` RDDID, logged; nothing depends on it | main.c:506-513 |
| 10 | Orientation from NVS `deck/orient` (default 3; saved if absent) | main.c:515-516, 86-118 |
| 11 | `tg_set_font(12×24)` — the flush 33 × 12 grid used for the test card and splash | main.c:517 |
| 12 | `bench_render()` (scheduler suspended) → `testcard_draw()` → `bench()` (20 full flushes) → 2.5 s delay | main.c:521-524 |
| 13 | `cmd_set_announce`; `seq_set_draw_hook(viz_mark)`, `seq_set_param_hook(viz_mark_param)` | main.c:530-537 |
| 14 | Register destinations `ble`, `mon`, `din`, `osc`; `view_init()` adds `view` | main.c:544-556; view.c:34-37 |
| 15 | `seq_init()` — midi queue (64), `midi` task (CPU1), `seq` one-shot timer started | main.c:562-564; seq.c:1055-1089 |
| 16 | `cmd_init()` — register the 34 built-ins | main.c:565; builtins.c:2346-2350 |
| 17 | `doc_init()` — gap buffers, find `notes`, scan the journal with a 128 KB scratch buffer | main.c:566-568; journal.c:400-422 |
| 18 | Journal self-test skipped (`DECK_JOURNAL_SELFTEST 0`) | main.c:585-588 |
| 19 | `sdmirror_init()` — mount `/sdcard`; a missing card is not fatal | main.c:589 |
| 20 | `report_memory("after docstore")` | main.c:590 |
| 21 | `ensure_guide_buffer()` — create the `guide`, or prepend new text above my | main.c:592, 327-396 |
| 22 | `kbd_set_ble_hooks(blemidi_register, blemidi_start)`; `kbd_init()` — queue, `nimble_port_init`, security, GATT, `nimble_host` and `kbd_repeat` tasks | main.c:594-597; ble_kbd.c:1043-1108 |
| 23 | KEY (GPIO18) as input with pull-up, 2 ms settle; if held → USB MIDI forced off | main.c:605-635 |
| 24 | If this boot followed a crash and USB MIDI was wanted → clear the intent | main.c:656-660 |
| 25 | `usbdev_boot()` (TinyUSB, console → CDC, trial timer); `vitals_started()` | main.c:666-667; usbdev.c:497-580 |
| 26 | USB mode: add and enable `usb`. Otherwise: `serialkbd_init()` | main.c:668-680 |
| 27 | `report_memory("after BLE")` | main.c:681 |
| 28 | `splash_show()` | main.c:684 |
| 29 | `net_rejoin()` — only if an SSID is remembered | main.c:691-693 |
| 30 | `run_boot_document()` — create or upgrade `boot`, then run every line as `CMD_BY_GUIDE` | main.c:695, 263-325 |
| 31 | "crashed N× – unplug to clear" on the status bar if the loop counter > 0 | main.c:697-702 |
| 32 | `editor_init()` → `editor_set_density(0)`; clear, draw, present | main.c:704-711; editor.c:205-209 |
| 33 | Subscribe `app_main` to the task watchdog (fallback init: 10 s, panic) | main.c:733-746 |
| 34 | Enter the main loop | main.c:755 |

## 3. Tasks, cores, timers, loop, watchdog

### 3.1 FreeRTOS tasks

| Task | Created at | Core | Prio | Stack | Lifetime |
|---|---|---|---|---|---|
| `main` (app_main → editor loop) | IDF | CPU0 (`CONFIG_ESP_MAIN_TASK_AFFINITY_CPU0`, sdkconfig:1692) | 1 (`ESP_TASK_MAIN_PRIO`) | 8192 (sdkconfig.defaults:60) | always |
| esp_timer (runs the `seq` tick and other callbacks) | IDF | **CPU1** (sdkconfig.defaults:136-137); ISR on CPU0 (sdkconfig:1745) | 22 (`ESP_TASK_TIMER_PRIO` = 25-3) | 3584 (sdkconfig:1738) | always |
| `midi` | seq.c:1072 | CPU1 | 6 | 4096 | always |
| `nimble_host` | ble_kbd.c:1105 → IDF `nimble_port_freertos.c:43-44` | CPU0 (`BT_NIMBLE_PINNED_TO_CORE=0`) | 21 (configMAX_PRIORITIES-4) | 4096 (sdkconfig:704) | always |
| BT controller | IDF / libbt | CPU0 (`BT_CTRL_PINNED_TO_CORE_0`, sdkconfig:939) | UNVERIFIED | UNVERIFIED | always |
| `kbd_repeat` | ble_kbd.c:1106 | unpinned | 5 | 2560 | always; 15 ms loop (ble_kbd.c:335) |
| `serialkbd` | serialkbd.c:74 | unpinned | 5 | 3072 | serial mode only |
| `TinyUSB` | usbdev.c:521 → esp_tinyusb `tinyusb_task.c:145-151` | **CPU1** | 5 | 4096 | USB-MIDI mode only |
| `oscin` | osc.c:257-258 | CPU0 | 3 | 3072 | on `>osc in <port>` (builtins.c:2024) |
| `ssh` | ssh.c:421-422 (`xTaskCreatePinnedToCoreWithCaps`) | CPU0 | 1 | 16384, **in PSRAM** | one per `>ssh`; deletes itself (ssh.c:391) |
| Wi-Fi / lwIP `tcpip` / `sys_evt` | IDF | Wi-Fi CPU0 (sdkconfig:1773); tcpip unpinned (2146) | IDF | sys_evt 4096 | on `>wifi`, `>host`, `>sync lead/follow` |

The docs say CPU1 is "the realtime half" (sdkconfig.defaults:116-119). In source, CPU1 also runs `TinyUSB` in USB-MIDI mode, and the unpinned priority-5 tasks can be scheduled there.

### 3.2 esp_timer timers

| Name | Created | Kind / period | Callback does |
|---|---|---|---|
| `seq` | seq.c:1076-1088; re-armed by `arm_next` (seq.c:840-848) | One-shot re-armed every tick, `ESP_TIMER_TASK`. Period 60e6/bpm/96 µs, i.e. 5,040 µs at 124 bpm (seq.c:960-965); minimum wait 50 µs (seq_clock.h:24). Runs while stopped too, so note-offs still drain | The sequencer tick (§3.3) |
| `st7305_idle` | st7305.c:345-349; armed per push (st7305.c:564-565) | One-shot, 1000 ms | Sets `s_want_lpm`; `st7305_service()` in the loop sends LPM (st7305.c:158-184) |
| `usbtrial` | usbdev.c:568-574 | One-shot, 8000 ms; re-armed while no host is seen | Flags revert / "waiting" (usbdev.c:439-462) |
| `usbreboot` | usbdev.c:416-425 | One-shot, 50 ms | Flags `ACT_REBOOT` on the esptool reset pattern |
| `usbtest` | builtins.c:1288-1297 | One-shot, 2 s | **Dead code** — `c_usbtest` is `__attribute__((unused))` and not in the verb table (builtins.c:1257-1265) |

### 3.3 Where the sequencer clock runs

`tick()` (seq.c:850-958) runs on the **esp_timer task, CPU1, priority 22**. Per tick:

1. drain scheduled note-offs and apply newly compiled lanes;
2. anchor the grid on the first tick after play; record the dispatch-vs-grid deviation (first 8 ticks excluded; seq.c:116,900-921);
3. emit `0xF8` every 4th tick if `>sync on`;
4. `fire_lanes()`;
5. emit `0xF9` step marker every 24 ticks;
6. re-arm on the grid (`seq_clock_wait`, seq_clock.h:29-37).

`emit()` only enqueues. On overflow it drops the oldest non-clock event (seq.c:285-315). The `midi` task (CPU1, prio 6) drains a whole step, calls every enabled destination, then each destination's flush, and logs drops (seq.c:193-241). Drawing lanes only *record* through the hooks; the picture is built in the main loop (main.c:844-877).

The autosave journal write is suppressed while the transport runs. The reason given: a flash write stalls the cache, so the clock cannot run (main.c:984-1002). The write is forced on stop (main.c:967-974).

### 3.4 Main loop (main.c:755-1025), ~200 turns/s

| Stage | What | Where |
|---|---|---|
| WDT | `esp_task_wdt_reset()` | main.c:756-758 |
| input | Drain `kbd_poll(&ev, 5)`; the 5 ms block **is the loop's idle time** | main.c:766-783 |
| passkey | Draw the pairing passkey the NimBLE task recorded | main.c:785-791 |
| KEY | Tap = cycle orientation (saved); hold 2 s = `kbd_forget_all()` | main.c:794-812 |
| deferred work | `usbdev_poll()`, `st7305_service()`, `blemidi_set_enabled(seq_dest_is_on("ble"))` | main.c:817-824 |
| playhead | Position change → redraw | main.c:838-842 |
| services | `vitals_loop`, `ensemble_service`, `kbd_share_radio` | main.c:861-867 |
| **pictures** | `viz_service()`, timed into `prof_viz` | main.c:869-871 |
| **view** | If a frame was made: `view_frame()`, timed into `prof_view` | main.c:872-877 |
| ssh | `ssh_service()` moves lines into `+out` | main.c:881-892 |
| **draw** | `editor_draw()` → `prof_draw` | main.c:894-897 |
| **push** | `editor_present()` → `st7305_flush` → `prof_push`; FAULT if cells were rendered but 0 bytes pushed | main.c:898-913 |
| blink | 500 ms, only within 15 s of the last edit | main.c:916-928 |
| heartbeat (10 s) | `alive: doc…, undo…, kbd…, push/B, render, heap <internal free>`; `usb: …`; **`loop: N/s; us in 10 s: pictures, view (sent, dropped), draw, push`** | main.c:933-957 |
| autosave | Save when not playing, ≥1 s idle, and (newline, or ≥24 chars changed, or ≥6 s idle). Mirrored to SD after the save. `+out` is never written | main.c:967-1024, 53-59 |

Recorded heartbeat samples, docs say: idle 199 turns/s; heaviest scene 192 turns/s, 52 ms pictures / 40 ms view / 150 ms draw / 189 ms push per 10 s (`docs/GRAPHICS.md:57-65`).

### 3.5 Watchdog and crash handling

| Mechanism | Setting / code |
|---|---|
| Task WDT | Initialised by IDF (`ESP_TASK_WDT_INIT=y`, sdkconfig:1710); 10 s; panics (sdkconfig.defaults:73-74); idle tasks on both CPUs watched (sdkconfig:1714). `app_main` subscribes at main.c:733, with a fallback init at main.c:734-743 |
| Interrupt WDT | 300 ms (sdkconfig:1707) |
| Panic | Silent reboot (`ESP_SYSTEM_PANIC_SILENT_REBOOT`, sdkconfig.defaults:103); no core dump (sdkconfig:1827); stack canary on (sdkconfig:1899) |
| After a crash | RTC counter → status-bar message (main.c:697-702). A crash clears the USB-MIDI intent (main.c:656-660). USB-MIDI gives up after 3 tries (usbdev.c:497-512) |
| Why ssh has its own task | A blocking `connect()` on the editor task would trip the 10 s TWDT (ssh.c:29-35) |

## 4. Memory

### 4.1 Static footprint (last build, `esp_idf_size` on `build/cyberdeck.map`)

| Region | Used |
|---|---|
| D/IRAM | **220,015 B used, 121,745 B remain (64.4 %)**: .data 25,116 · .bss 80,840 · IRAM .text 114,059 |
| Dedicated IRAM | 16,384 B (100 %) |
| Flash | 1,236,096 B (.text 1,014,388 · .rodata 221,452); image 1,391,655 B in a 3 MB `factory` partition |

### 4.2 Placement of the objects that matter

| Object | Size | Where | Source |
|---|---|---|---|
| Panel framebuffer | 15,000 B | internal, DMA-capable heap (SPI DMA cannot reach PSRAM) | st7305.c:324-332; st7305_addr.h:30 |
| Panel gather ("stage") buffer | 15,000 B | internal DMA heap | st7305.c:332 |
| **Glyph cache** (pre-turned face) `s_turned` + 8 attribute masks | 124 × 36 = 4,464 B + 288 B | internal .bss; rebuilt when the orientation changes | textgrid.c:19-30, 332-353 |
| Text grid `s_txt`, `s_att`, dirty bits | 1,650 + 1,650 + 207 B | internal .bss | textgrid.c:15-17 |
| Fonts | 5,952 B (12×24), 1,488 B (6×12) | flash rodata | font12x24.h:6; font6x12.h:29 |
| viz frame and previous frame | 2 × 1,464 B | internal .bss | viz.c:125-128 |
| view buffers (cells, frame, line, b64) | 1,440 + 1,451 + 1,952 + 1,952 B | internal .bss (static) | view.c:57-58, 83-84 |
| seq lane table `s_lanes` | 16,256 B | internal .bss (largest symbol) | seq.c:21 (last-build `nm`) |
| Editor wrap table `s_line_start` | 8,192 B | internal .bss | editor.c:63, 71 |
| Document buffers | 128 KB each, ≤ 8, lazily loaded | **PSRAM**, falling back to default heap | buffer.c:46-56; docstore.h:21-22 |
| Journal scan scratch at boot | 128 KB, freed after | `malloc` → PSRAM (anything >4 KB goes there) | journal.c:415-420 |
| Save staging buffer | length of the doc | **internal on purpose**: a PSRAM source becomes ~160 bounced flash writes | journal.c:452-467 |
| **Undo log** | 128 steps × `undo_op_t` (≈60 B → ≈7.5 KB, computed) | **PSRAM**, falling back to internal | undo.c:28-58 |
| **SSH task stack** | 16,384 B | **PSRAM** (needs `SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY`, sdkconfig:2972) | ssh.c:45, 421-422 |
| SSH message buffer | 8,192 B | PSRAM | ssh.c:404 |
| libssh2 session (~80 KB, docs say) | — | PSRAM via custom allocators, internal fallback | ssh.c:81-93 |
| mbedTLS | — | internal only (`MBEDTLS_INTERNAL_MEM_ALLOC`, sdkconfig:2230) | — |
| NimBLE | — | internal (`BT_NIMBLE_MEM_ALLOC_MODE_INTERNAL`, sdkconfig:698) | — |
| Wi-Fi / lwIP | — | try PSRAM (`SPIRAM_TRY_ALLOCATE_WIFI_LWIP`, sdkconfig.defaults:27) | — |
| kbd queue / midi queue | 256 × 8 B / 64 × 12 B (computed) | internal heap | ble_kbd.c:1049; seq.c:1060 |
| USJ driver rings | RX 256 / TX 4000 (under 4 KB so malloc keeps it internal) | internal | serialkbd.c:57-63 |

The PSRAM malloc threshold is 4096 B; 32,768 B of internal RAM is reserved (sdkconfig.defaults:26; sdkconfig:1604).

### 4.3 Heap figures

- **Logged by the firmware.** Format: `"<when>: internal free %u B, largest %u B, PSRAM free %u B"` (main.c:72-78). Emitted at `boot` (487), `after docstore` (590) and `after BLE` (681). The framebuffer placement is logged at st7305.c:337-340. The 10 s heartbeat carries `heap <internal free>` (main.c:937-944), the splash shows `Nk free` (splash.c:79-86), and every vitals record stores `heap_free` (vitals.c:32). The current build's boot values are **UNVERIFIED** — no captured log is in the repo.
- **Recorded values, docs say:**

  | Figure | Source |
  |---|---|
  | 212 KB internal free, 8.25 MB PSRAM free (2026-09-20, before Wi-Fi/SSH existed) | measured 2026-09-20 |
  | Heartbeat sample `heap 199571` | measured 2026-09-20 |
  | ~98 KB internal free before Wi-Fi; 15–16.5 KB with the station up; 14.2 KB with the AP up (2026-09-25) | `docs/NETWORK.md:131-133` |
  | ~16 KB with the station up | ssh.c:39-40 |

## 5. Configuration

`firmware/sdkconfig` is **gitignored** (`.gitignore:35`); only `sdkconfig.defaults` is tracked. The values below come from defaults, or from the local `sdkconfig`, which agrees with the last build's `build/config/sdkconfig.h`.

| Setting | Value | Where |
|---|---|---|
| Target / flash | esp32s3; 16 MB, QIO, 80 MHz | sdkconfig.defaults:4-7 |
| **CPU frequency** | **160 MHz** — the IDF default; not raised to the 240 MHz part maximum | sdkconfig:1633 (not in defaults) |
| **FreeRTOS tick** | **1000 Hz** ("the one that ruins sequencers") | sdkconfig.defaults:16 |
| PSRAM | Octal, 80 MHz, malloc on, always-internal ≤ 4096 B, reserve internal 32 KB, Wi-Fi/lwIP try PSRAM, memtest on | sdkconfig.defaults:22-27; sdkconfig:1601-1604 |
| **Console** | USB-Serial-JTAG, no secondary (UART num -1). In USB-MIDI mode `usbdev` moves it to TinyUSB CDC at runtime | sdkconfig.defaults:33; usbdev.c:548-559 |
| TinyUSB | 1 CDC + 1 MIDI; `CFG_TUD_MIDI_TX_EPSIZE=4` override | sdkconfig.defaults:88-90; firmware/CMakeLists.txt:34-37 |
| Bluetooth | NimBLE: central + observer + **peripheral** + broadcaster; 2 connections, 4 bonds, 16 CCCDs, NVS persistence, legacy + SC pairing; host and controller on CPU0 | sdkconfig.defaults:39-55; sdkconfig:701, 939 |
| Main task | Stack 8192, CPU0; system event task 4096 | sdkconfig.defaults:60-61 |
| **Task watchdog** | Enabled at init, **10 s, panic**, both idle tasks watched; INT WDT 300 ms; silent panic reboot | sdkconfig.defaults:73-74, 103; sdkconfig:1707-1714 |
| **esp_timer** | Task on **CPU1** (experimental option), 3584 B stack; ISR on CPU0 | sdkconfig.defaults:136-137; sdkconfig:1738, 1745 |
| Compiler | `-O2` (PERF), warn on write-strings | sdkconfig.defaults:64-65 |
| FAT | LFN on heap, max 64 (per-document mirror names) | sdkconfig.defaults:77-78 |
| Security | NVS encryption off, flash encryption off — the Wi-Fi password sits in plain NVS (net.c:166-167) | sdkconfig:2409, 494 |
| **Partition table** | Custom `partitions.csv` (sdkconfig.defaults:8-9) | see below |

**`firmware/partitions.csv`**

| Name | Type/Sub | Offset | Size | Use |
|---|---|---|---|---|
| nvs | data/nvs | 0x9000 | 0x6000 (24 KB) | NVS (see below) |
| phy_init | data/phy | 0xF000 | 0x1000 | RF calibration |
| factory | app | 0x10000 | 0x300000 (3 MB) | firmware (1.39 MB used) |
| **notes** | **data / 0x40** | **0x310000** | **0x80000 (512 KB = 128 sectors)** | **the document journal**, found by `esp_partition_find_first(DATA, 0x40, "notes")` (journal.c:407). Records are `ceil((48+len)/4096)` sectors (partitions.csv:5-9) |

Flash above 0x390000 (~12.4 MB) is unallocated.

**NVS keys.** Namespace `deck` holds `orient` (main.c:95), `peer` and `pair_ver` (ble_kbd.c:926, 1071), `ssid` and `pass` (net.c:166-167), `vitals` (vitals.c:16), `bat_gpio` and `bat_div` (battery.c:123-124) and `tstage` (selftest.c:28). NimBLE bonds are stored separately (`BT_NIMBLE_NVS_PERSIST`). Namespace `sshkeys` holds `k<fnv32>` host fingerprints (ssh.c:107; ssh_fp.h:42-51). USB-MIDI intent is deliberately **not** in NVS but in RTC_NOINIT (usbdev.c:62-84).

## 6. Hardware bill

| Item | What | Source |
|---|---|---|
| Board | Waveshare **ESP32-S3-RLCD-4.2**: ESP32-S3-WROOM-1-N16R8 (dual LX7 ≤ 240 MHz, 512 KB SRAM, 16 MB flash, 8 MB octal PSRAM, Wi-Fi + BLE 5, no BT Classic); 4.2″ 400×300 1-bpp reflective LCD, Sitronix ST7305, no backlight | docs/ASSEMBLY.md:27; docs/HARDWARE.md:31-60 |
| Keyboard | **Rii 518BT** mini Bluetooth keyboard, BLE HID / HOGP (not K18, not RT518S) | docs/ASSEMBLY.md:28; docs/HARDWARE.md:43-46 |
| Cell | 1 × **18650** Li-ion, flat-top or protected, ≤ Ø18.6 × 69.0 mm | docs/ASSEMBLY.md:30 |
| Other parts | microSD 0–1 (FAT32); speaker with MX1.25 lead; 4 × M2×4 heat-set inserts; 4 × M2×6 CSK (chassis↔plate); 4 × M2.5×8 CSK (board standoffs); 0.5 mm foam gasket | docs/ASSEMBLY.md:31-48 |
| Printed | chassis ~53 g, backplate ~58 g, buttons ~0.5 g, cover ~60 g (variant 2 only) | docs/ASSEMBLY.md:17-22 |
| Enclosure variants | **v1** (`variant = 1`, as printed, locked by the golden file). **v2** (`variant = 2`, current: adds a magnetic front cover and 8 × Ø5×2 N52 magnets). **Concrete jacket** (docs call it "variant 3"; separate `concrete.scad`). **Carry case** (two halves, 7 × M5×16 into M5×10 inserts) | parameters.scad:1170, 1476; docs/ASSEMBLY.md:22, 29; docs/CONCRETE.md:1-4; export/stl/README.md:7-14 |

**Pins as used by the firmware:**

| Function | Pins | Source |
|---|---|---|
| Panel | SCK 11, MOSI 12, CS 40, DC 5, RST 41 | st7305.c:26-30 |
| KEY button | GPIO18 | main.c:51 |
| SD (1-bit) | CLK 38, CMD 21, D0 39 | sdmirror.c:41-43 |
| Battery sense | Unknown; I declare it | battery.h:9-20 |
| DIN MIDI | UART1 on a declared pin | dinmidi.c:14 |

The docs describe the vendor code as "SPI3_HOST @ 20 MHz" (docs/HARDWARE.md:61); the firmware runs SPI2 @ 24 MHz (st7305.c:32-33).

## 7. Flash and NVS writes that can happen while the transport runs

Why it matters: a flash write disables the cache on both cores. `CONFIG_SPI_FLASH_AUTO_SUSPEND` is off (`sdkconfig:2483`), and the tick path runs from flash (`main.c:986-989`). `docs/GRAPHICS.md:175-179` asks for this list. "Gated" means the write cannot happen while `seq_running()`.

| # | writer (medium) | trigger | path | gated? |
|---|---|---|---|---|
| 1 | journal autosave (flash partition) | editing | `main.c:979-1011` → `journal.c:484-489` | **yes**, `main.c:999-1002` |
| 2 | journal, `>name` | command | `builtins.c:310-314` | **no** |
| 3 | journal + SD, `>save` | command | `builtins.c:359-364` | **no** |
| 4 | journal + SD, `>usb on/off` | command | `builtins.c:1453-1454`: saves **before** `seq_stop` | **no**, but the deck reboots next |
| 5 | journal + SD, `>flash now` | command | `builtins.c:1149-1150`: saves before `seq_stop` | **no**, reboot next |
| 6 | journal + SD, esptool reset over CDC | host DTR/RTS | `usbdev.c:318-341`: the sequencer is never stopped | **no**, reboot next |
| 7 | **NVS `vitals`** | **every 60 s in USB MIDI mode** | `main.c:861` → `vitals.c:87-100` | **no**. The record changes every time, so this is a real write; NVS page reclaim will eventually erase a 4 KB sector. The size of that stall is **UNVERIFIED** |
| 8 | NVS `orient` | KEY tap | `main.c:802-806` → `main.c:110-118` | **no** |
| 9 | NVS `peer` | every keyboard (re)subscribe, including the rebuild after 60 s with no key reports, which is likely mid-set | `ble_kbd.c:279-286,509-511,922-929` | **no**. An identical value is skipped by NVS (IDF `nvs_flash/src/nvs_storage.cpp:470-506`); a new keyboard writes |
| 10 | NimBLE bond store (NVS) | pairing, re-pairing | `sdkconfig.defaults:52`; `ble_kbd.c:872-882` | **no** (the store's writes are IDF-internal, **UNVERIFIED**) |
| 11 | NVS bonds + `peer` erased | `>kbd forget`, KEY held 2 s | `builtins.c:245-246`; `main.c:798-801` → `ble_kbd.c:1028-1035` | **no** |
| 12 | NVS `ssid` and `pass` | `>wifi <ssid>` answered (also at boot, where it is skipped as identical) | `builtins.c:1885-1891` → `net.c:193,159-170` | **no** |
| 13 | NVS erase `ssid` and `pass` | `>wifi forget` | `net.c:113-121` | **no** |
| 14 | NVS `sshkeys/k…` | end of the first session with a host | `main.c:883` → `ssh.c:450-458,124-139` | **no** |
| 15 | NVS erase `sshkeys/k…` | `>ssh forget` | `ssh.c:141-156` | **no** |
| 16 | NVS `bat_gpio`, `bat_div` | `>battery use` | `battery.c:117-126` | **no** |
| 17 | Wi-Fi driver NVS (mode) | `>sync lead\|follow` started before `net.c` | `ensemble.c:697-704`; `sdkconfig:1772` | **no**. Whether it writes at all is **UNVERIFIED** (closed driver) |
| 18 | journal via lines in a running document (`>save`, `>name` are allowed in `>run`, `cmd.c:23-24`) | `>run` of a piece containing `>play` … `>save` | — | **no** |
| — | boot-only writes: `vitals_started`, `pair_ver`, `orient` on first boot, the `boot`/`guide` documents, PHY calibration (`sdkconfig:1544`, **UNVERIFIED**) | boot | `main.c:667`; `ble_kbd.c:1067-1080`; `main.c:104-106,277,303,378,392` | before any `>play` |
| — | SD mirror (FAT over SDMMC) | with 1, 3–6 | `sdmirror.c:117-165` | not a flash-cache stall; costs time on the main task |

**Conclusion.** Only row 1, the autosave, honours "the deck does not write to flash while it is playing" (`main.c:984-998`).

- Row 7 fires by itself, once a minute, in the very mode recommended for performance.
- Rows 8, 9 and 12–16 fire on user or radio events with no transport check.
- Rows 4–6 add a stall of 13–18.6 ms per dirty buffer (`main.c:986`) while the clock is still playing, just before a reboot.

## 8. Services — heartbeat, vitals, battery, flash, panic, KEY

### 8.1 Boot sequence, as it affects these services (`main.c:432-753`)

1. **Boot-loop counter** in RTC_NOINIT (magic `0xB0070009`). A panic, task-watchdog, interrupt-watchdog or other watchdog reset increments it and marks "crashed last boot"; power-on clears it (`main.c:439-460`).
2. **Reset reason** logged: `boot: <power on|software|panic|task watchdog|…>` (`main.c:462-475`).
3. `RTC_CNTL_OPTION1_REG` = 0, which clears a leftover force-download bit (`main.c:477`). Then `usbmux_release_to_usj()` (`main.c:479-484`).
4. **NVS init**; the partition is erased if it is full or from a newer version (`main.c:489-494`). Then `vitals_begin()` (`main.c:500`).
5. **Panel**:
   - Init and RDDID.
   - Orientation from NVS; the first boot **writes** it (`main.c:88-118`).
   - Render benchmarks and the test card, then a 2.5 s pause (`main.c:521-524`).
6. **Destinations**: `ble`, `mon`, `din`, `osc`, `view`. Then `seq_init`: the `midi` task on core 1 and the one-shot timer (`main.c:544-564`; `seq.c:1055-1088`).
7. `cmd_init`; `doc_init` (the journal scan); SD mount; `ensure_guide_buffer` (which may write the journal) (`main.c:565-592`).
8. **NimBLE and keyboard**, with the BLE-MIDI hooks (`main.c:594-597`).
9. **KEY** configured with a pull-up and 2 ms settle. Held → USB MIDI off (`main.c:598-635`). Crashed last boot → USB MIDI off (`main.c:637-660`).
10. **`usbdev_boot()`** → `vitals_started()`, **one NVS write per boot** → either the `usb` destination (enabled) or `serialkbd_init()` (`main.c:666-680`).
11. **Splash** (`KILROY`, then `N docs  NNk free[  NN%]`) (`splash.c:47-95`). `net_rejoin()` (`main.c:687-693`).
12. **`run_boot_document()`**, as GUIDE. It creates `boot` if missing and prepends the lane names if their mark is absent; both cases write the journal (`main.c:258-325`).
13. `crashed %ux - unplug to clear` if the counter is above 0 (`main.c:697-702`). Then `editor_init()`, which installs the password asker.
14. **Task watchdog** armed on the main loop: `task watchdog armed on the editor loop`, or `UNAVAILABLE - hangs will be silent` (`main.c:725-746`).

### 8.2 Heartbeat, every 10 s (`main.c:930-957`)

```
alive: doc <len>[*], undo <depth>, kbd <up|state>, <n> push/<n> B, render <us> us/<n> cells, heap <internal free>
usb: act<0|1> dev<0|1> midi<0|1> s<sent> d<dropped>
loop: <n>/s; us in 10 s: pictures <us>, view <us> (<sent> sent, <dropped> dropped), draw <us>, push <us>
```

The loop also logs `FAULT: rendered %u cells, pushed 0 bytes` (`main.c:908-911`).

### 8.3 Vitals (`vitals.c`, `vitals.h`, `vitals_verdict.h`)

- **Record.** NVS `deck` / `vitals`, a 24-byte blob (`vitals.h:28-39`; confirmed by compiling on the host):

  | field | meaning |
  |---|---|
  | `uptime_s` | seconds since boot |
  | `loops` | main-loop iterations |
  | `ticks` | sequencer ticks |
  | `heap_free` | internal free heap |
  | `usb_mode` | 1 in USB MIDI mode |
  | `running` | transport running |
  | `reset_reason` | this boot's reset reason |
  | `valid` | record written |
  | `deliberate` | written by a goodbye |

- **When it is written** (`vitals.c:25-105`):
  - `vitals_started`: once per boot.
  - `vitals_loop`: **every 60 s, only in USB MIDI mode, whether or not the transport is running** (`vitals.c:87-100`, called from `main.c:861`).
  - `vitals_goodbye`: before the reboots of `>usb` and `>flash now`, always with `running=false` (`builtins.c:1153,1457`).
- **What it reports.** At boot it logs up to 4 lines, plus `  ticks %u, reset then %d, now %d`. `>jitter` prints the same lines (`builtins.c:1384-1395`). The lines come from `vitals_verdict.h:26-62`:
  - `no previous run recorded`
  - `last run: serial, not watched` + `  only USB MIDI runs are`
  - `last run: <s>s, USB MIDI|serial`, `  <n> loops/s, <n>K heap`, then one of `  STOPPED DEAD - see OS.md`, `  RESTART HUNG - see OS.md` or `  restarted on purpose`
  - `tools/test_vitals.c` passed when built here.
- **Gap.** The esptool reboot (`usbdev.c:318-341`) and the trial revert (`usbdev.c:367-373`) write no goodbye. The next boot will therefore report a flash or revert from USB MIDI mode as `STOPPED DEAD` (inferred).

### 8.4 Battery (`battery.c`, `battery.h`; `builtins.c:1489-1531`)

- **Candidate pins.** ADC1 channels 0–9 (GPIO1–10), excluding GPIO5, which the panel uses. Each reading is the average of 8 samples at 12 dB attenuation with curve-fit calibration; without calibration it falls back to `raw*3100/4095` (`battery.c:14-18,51-115`).
- **Percentage.** An 18650 curve from 3000 mV = 0 % to 4150 mV = 100 %, interpolated (`battery.c:25-49`). It is **−1 until a pin is chosen**, and then no bar or percentage is shown.
- **`>battery`**:
  - If a pin is configured, first `cell %dmV = %d%%`.
  - Then the scan `g<gpio>:<mV>mV …` in 28-character lines, then `unplug USB, run again: the one` / `that moves is it. then:` / `  battery use <gpio>`.
  - Status `scanned ADC1`.
- **`>battery use <gpio> [div×10]`**:
  - Stores `bat_gpio` and `bat_div` in NVS `deck` (default divider 20, i.e. 2:1). `use 0` forgets it. A GPIO outside 1–10 silently counts as "unset" (`battery.c:117-132`).
  - It prints `GPIO%d: %dmV = %d%%` or `GPIO%d reads nothing`, with status `battery on GPIO%d`.
- **Display.**
  - The status bar shows ` ####` (4 cells) at the right when the level is known (`editor.c:322-340`).
  - The splash shows `NN%` (`splash.c:78-89`).
  - With a pin set, every status-bar draw takes 8 ADC readings.

### 8.5 `>flash now` (`builtins.c:1051-1170`)

- **Without `now`** it prints `this reboots the deck into the` / `ROM loader and ends the session.` / `type:  >flash now`, status `flash needs: >flash now`.
- **With `now`**:
  1. `doc_save_all_dirty()`, **before** `seq_stop()`.
  2. `vitals_goodbye`.
  3. Five lines: `download mode. flash now:`, `  idf.py -p PORT flash`, `no button, no paperclip. the deck STAYS in`, `download mode until it is flashed or unplugged:`, `RTC_CNTL_FORCE_DOWNLOAD_BOOT survives a CPU reset.`
  4. The panel shows `DOWNLOAD MODE - flash now`, then a 600 ms wait.
  5. `usbmux_release_to_usj()`, set `RTC_CNTL_FORCE_DOWNLOAD_BOOT`, `esp_restart()`.
- The bit survives a CPU reset and is cleared by esptool's default system reset (`builtins.c:1071-1087`).
- **Deadlocks from USB MIDI mode**; use `>usb off` first (`docs/OS.md:656-661`; `docs/TESTING.md:527-529,549-550`).
- `docs/COMMANDS.md:296` still describes plain `>flash`.
- `>usbtest` (`builtins.c:1236-1300`) is compiled but not in the command table.

### 8.6 Panic

- **`>panic`**: `seq_stop()` followed by `seq_all_notes_off()`, so pending offs plus 16× CC 123 go out **twice**, and the transport stops. Prints `all notes off` (`builtins.c:2300-2306`).
- **A crash**: `CONFIG_ESP_SYSTEM_PANIC_SILENT_REBOOT=y` (`sdkconfig.defaults:92-103`) reboots with no backtrace. The next boot counts it (§8.1), shows `crashed Nx - unplug to clear`, and drops USB MIDI mode.

### 8.7 Watchdogs

- **Task watchdog** (`sdkconfig:1709-1714`): initialised at startup, **panics**, 10 s timeout, watches both idle tasks. `main.c:733-746` subscribes the main loop; if subscribing fails it initialises one itself (10 s, panic, idle mask 0). The loop resets it every pass (`main.c:756-758`).
- **Interrupt watchdog**: 300 ms, CPU1 checked (`sdkconfig:1706-1708`).
- Both lead to the crash path in §8.6.

### 8.8 KEY button (GPIO18)

| gesture | action | writes NVS? |
|---|---|---|
| tap | cycles the panel orientation | **yes**, at any time (`main.c:802-811`) |
| hold 2 s | forgets every bond (§7) | yes |
| held at boot | USB MIDI off (§2.4) | no |
