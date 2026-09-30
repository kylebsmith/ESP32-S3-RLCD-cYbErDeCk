# The pictures — sixteen primitives, the frame, the view

*Part of [the deck, top to bottom](README.md). Snapshot: commit `85d1e6a`, 2026-09-28.
Source is the truth.*

**Path shorthand.** `viz.c` and `viz.h` are `firmware/components/viz/{viz.c,include/viz.h}`. `textgrid.*`, `tgfont.h` and `font*` are under `firmware/components/textgrid/`. `st7305*` is under `firmware/components/st7305/`. `editor.c`, `main.c`, `cell_attr.h`, `ui_text.h`, `ask.h`, `view.*`, `view_wire.h` and `serialkbd.c` are under `firmware/main/`. `builtins.c`, `cmd.c` and `lane_name.h` are under `firmware/components/cmd/`. `seq.c`, `seq.h` and `seq_pattern.h` are under `firmware/components/seq/`. `kbd.h`, `ble_kbd.c` and `serialkbd_map.h` are under `firmware/components/kbd/`. `buffer.c` and `docstore.h` are under `firmware/components/docstore/`. `VIEW.md` and `HARDWARE.md` are in `docs/`.

**Evidence marks.**
- **[probe]** means the claim was confirmed by running the shipping source on the host. The sources compiled were `viz.c`, `editor.c` + `cmd.c`, `textgrid.c` + fonts, and `serialkbd_map.h`. They were compiled read-only against stubs in a scratch harness outside the repo.
- **[read]** means the claim comes from reading the code only.
- **UNVERIFIED** means the claim cannot be confirmed from code: hardware or optical behaviour, or numbers that appear only in docs.

## 1. The pictures (`viz`)

### 1.1 How a frame is made

1. **Lanes.** A picture lane is an ordinary seq lane bound with `SEQ_BIND_VIZ` and a primitive index (`builtins.c:560-571`). Its name is a primitive's own name, or an alias from `>circle = disc` (`builtins.c:549-554`, `lane_name.h:35`). Running a picture lane turns the split on (`builtins.c:733-735`).
2. **Marks, from the clock.** On each event, `fire_event` computes an amount and a direction and calls the draw hook `viz_mark`. For an `:x`/`:y` lane it calls the parameter hook `viz_mark_param` instead (`seq.c:490-507`; hooks set at `main.c:536-537`). This runs in the esp_timer task, pinned to CPU1 (`sdkconfig.defaults:137`). Both hooks only record:
   - Up to 16 draw marks `{prim, amt, dir}` per frame (`viz.c:96`, `925-944`).
   - Up to 16 position marks (`viz.c:913-923`).
   - Any further marks are dropped silently (`viz.c:917`, `935-937`).
   - `s_mark_tick` is set by draw marks only (`viz.c:942`).
3. **Drawing, in the main loop.** `viz_service()` runs every main-loop pass (`main.c:870`) and does nothing unless something was marked (`viz.c:1005-1008`). When something was:
   1. It copies the finished frame to `s_prev` and clears the frame to tone 0 (code 128) (`viz.c:1014-1015`).
   2. It applies all position marks (`viz.c:1028-1038`).
   3. It draws every draw mark in pipeline order (§1.3) (`viz.c:1040-1058`).
   4. It returns `true`, which triggers a pane redraw and a view frame (`main.c:872-877`).
4. **Consequences** [probe]:
   - **The frame is rebuilt from nothing on every serviced step.** A primitive that did not fire on this step is absent unless `echo` fired and carried it.
   - **A step on which only a position lane (`:x`/`:y`) fires draws nothing:** the position waits for the next drawing (fixed 2026-09-29; it used to blank the frame).
   - Two marks of one primitive in one frame draw twice, in arrival order (`viz.c:1051-1056`).
   - The comment at `viz.c:77-79` says a second mark overwrites the first. That is stale.
5. **`viz_active()`** becomes true on the first frame in which anything drew. Only `viz_forget_all()` clears it (`viz.c:243-245`, `1059`, `290-299`), and that runs only on `>new` (`builtins.c:296-297`).
6. **Unlocked handoff (UNVERIFIED).** `viz_mark` runs on CPU1 while `viz_service` runs in the editor loop. `nm = s_nmark; s_nmark = 0;` (`viz.c:1040-1041`) has no lock, so a mark landing between those two statements would be lost. This has not been observed.

### 1.2 Amount, `x`, direction, routing

| Input | Amount handed to the primitive | Where |
|---|---|---|
| digit `0`–`9` | that digit. `0` is an event, not a rest ("a digit is always an event") | `seq.c:415-417`, `491-492` |
| `x` | 9 | `seq.c:492`, `seq_pattern.h:81` |
| a direction step `u d l r` | 9. A direction step is a hit with value `x`, so it cannot carry a digit | `seq_pattern.h:555`, `585-589` |
| routed lane | the source's last value v (0–127), as `(v·9+63)/127`. The lane fires **only** when its source fires, and its own steps are not consulted | `seq.c:491`, `597-609`, `392-404` |

The amount is clamped to 0–9 (`seq.c:493-494`, `viz.c:939`).

**Direction.** Each event uses the step's own direction if it has one, else the lane's direction (`seq.c:495`). The lane's direction is one written in front of the pattern (`>ramp u 4`), and defaults to **`'d'`** (`seq.c:1336-1338`, `seq_pattern.h:645-652`).

Only `move`, `warp`, `ramp` and `turn` accept a direction (`viz.c:895-903`). On any other primitive, or on an `:x`/`:y` lane, a direction is refused with `u d l r: move warp ramp turn` and the character is boxed (`builtins.c:700-723`).

Routing a picture's value between lanes: `>route disc kick` means disc fires on each kick, at the kick's velocity as 0–9 (`builtins.c:1598-1603`, `seq.c:392-404`). A picture lane publishes its own amount as `amt·127/9` (`seq.c:506`).

### 1.3 Pipeline order and `route`

Table order is the default draw order (`viz.c:62-68`, `870-876`):

`echo · move · spin · warp` → `noise · disc · box · turn · ramp · grid` → `mask · edge` → `grow · thin · flip` → `fold`

**Route changes it** (`chain_ranks`, `viz.c:966-1001`):
- A lane's rank is the number of route hops to a lane that follows nothing.
- A primitive's rank is the highest rank among its instance lanes (`viz.c:999`).
- Parameter lanes are not ranked (`viz.c:979`).
- Frames draw rank 0 first, then 1, and so on. Table order applies within a rank (`viz.c:1046-1058`).
- The comments at `viz.c:61` and `viz.c:1044` refer to **`order_marks()`, which does not exist.** The function is `chain_ranks()`, as `viz.h:41` correctly says.

**Consequences** [probe]:
- **`move` and `warp` sit before the sources.** Unrouted, they only transform what `echo` laid down from the last frame. **`spin` sits there too but acts on the sources**: it sets the rate, and each source is turned as it draws. Before 2026-09-29 it turned the whole frame by quarter turns, and cells near the centre, or in a corner the turn mapped onto itself, never moved — my "set of static pixels"; now `spin 3` leaves 0 cells the same from step to step. Without `echo` or a route they act on an empty frame. `ramp 3 r` + `spin 3` in one frame showed no rotation; with `>route spin ramp` the ramp turned 90°.
- **A route reorders, re-times and re-amounts all at once.** A routed operator fires only with its source, at its source's amount (§1.2). For example, `>turn 2` + `>route spin turn` gives spin amount 2, which is 0 quarter turns: no rotation.
- **Routing a picture from a non-picture lane moves it to rank 1.** The guide's own `>route disc kick` does this (`ui_text.h:208`). The disc then draws after every unrouted operator, so `fold`, `mask`, `edge`, `grow`, `thin` and `flip` no longer apply to it. [probe: `fold` stopped mirroring a disc once the disc was routed from a `kick` lane]

### 1.4 Glyph codes the pictures use

| Code | Meaning | Written by |
|---|---|---|
| 128–136 | nine tones, empty (128 = `TONE_0`) to solid (136 = `SOLID`) | all fields; the empty frame is 128, not `' '` (`viz.c:151-153`, `301-307`) |
| 137, 138, 139, 140 | sparkles: speck, 4-point star, 8-point star, burst | `noise` only (`viz.c:155-156`, `519-528`) |
| 147 | small filled disc | `disc` when its radius ≤ 1 (`viz.c:555`) |
| 141–146, 148–155 | half blocks, square, diamond, ring, diagonals, arcs | in both fonts (`viz.c:140-145`), but **no firmware code writes them** [read] |

`tone_of()` maps 128–136 to 0–8 and `' '`/NUL to 0. **Everything else, including sparkles and 147, counts as solid (8)** for `echo`, `mask`, `edge`, `grow`, `thin` and `flip` (`viz.c:159-164`).

### 1.5 The sixteen primitives

Notation: *a* = amount 0–9 (`x` = 9); *w*×*h* = live frame; *(px,py)* = position (§1.6). Vertical distance counts double wherever geometry is measured, because a cell is 1:2 (`viz.c:541-548`, `778-781`). Every mapping in the table was checked amount by amount [probe].

| # | Name | What it does to the frame | Amount → parameter | Dir | Pos |
|---|---|---|---|---|---|
| 0 | **echo** | Lays `s_prev` back down, each tone reduced by *fall*. Writes only where the result > 0 (`viz.c:428-444`) | fall = 1+(9−a)/3: a0 → 4, a1–3 → 3, a4–6 → 2, a7–9 → 1. A solid cell lasts 1/2/3/7 frames | – | – |
| 1 | **move** | Shifts the whole frame n cells, wrapping (`viz.c:454-473`) | n = (a==9) ? 1 : (a+2)/3: a0 → 0 (no-op), a1–3 → 1, a4–6 → 2, a7–8 → 3, **a9/x → 1** | u up, d down (default), l left, r right | – |
| 2 | **spin** | **A rate** (2026-09-29): each step adds a × 10° to a running angle (`draw_spin`), and the pictures that draw this step — noise, disc, box, turn, ramp, grid — are turned by it before the rest of the chain sees them (`turn_sources`); the history `echo` keeps is not turned | a0 holds the angle; a3 is 30° a step, a full turn in 12 steps; a9 is 90° a step | – | – |
| 3 | **warp** | Slides each column vertically (u/d) or each row horizontally (l/r) by a triangle wave of period 16 cells. Phase is (index + tick) mod 16 (`viz.c:482-507`) | peak shift = ⌊4a/9⌋: a0–2 → 0 (no visible effect), a3–4 → 1, a5–6 → 2, a7–8 → 3, a9 → 4 | **axis only: u ≡ d, l ≡ r** [probe] | – |
| 4 | **noise** | Writes w·h·a/9 random cells, with replacement (≈63% of cells at a9), as sparkles: 50% speck, 25% star4, 12.5% star8, 12.5% burst (`viz.c:512-529`) | a0 → nothing | – | – |
| 5 | **disc** | Round field: solid to ⅔r, then tones fading to 1 at the rim (`viz.c:538-563`, `ink_field` `410-419`) | r = a·min(w/2,h)/9. r ≤ 1 → one glyph 147 (so **disc 0 draws a dot**). At 28×4, a0–4 all draw that same glyph; at 58×10, r = 0,1,2,3,4,5,6,7,8,10 | – | yes |
| 6 | **box** | Square (Chebyshev) field, same fill rule (`viz.c:740-753`) | same r. r < 1 → one solid cell | – | yes |
| 7 | **turn** | Angle field around the point: cells within *sweep* clockwise of the start angle are inked, tone 8 at the start fading to 1; centre cell solid; no radius limit (`viz.c:769-792`, `turn_of` `390-400`) | sweep = a/9 of a full turn (a9 = 360°); a0 → nothing. The "diamond angle" is monotonic but not linear | start angle: u 12 o'clock, r 3, **d 6 (the default)**, l 9 | yes |
| 8 | **ramp** | Bands of tone: solid at the anchored edge, fading to 1 (`viz.c:571-596`) | reach = a·len/9 rows or columns (at least 1 if a > 0); a0 → nothing | d: from the top, downward (default); u: from the bottom, upward; r: from the left, rightward; l: from the right, leftward | – |
| 9 | **grid** | Solid lines wherever x or y is a multiple of *every*, anchored at (0,0) (`viz.c:624-633`) | every = 10−max(a,1): **a0 ≡ a1 → 9**, … a8 → 2, **a9 → 1 (whole frame solid)** | – | – |
| 10 | **mask** | Clears cells with tone < keep (`viz.c:803-816`) | keep = 1+(d−1)·7/8 with d = clamp(a,1,9): a0–2 → 1 (keep all ink), a3 → 2 … a9 → 8 (solid only) | – | – |
| 11 | **edge** | An inked cell becomes solid if it touches empty or the frame border ("ink against nothing"), or if its tone minus its lowest neighbour's tone ≥ drop. Every other cell clears (`viz.c:830-865`) | drop = max(1, 9−clamp(a,1,9)): a0 ≡ a1 → 8 (outline only) … a8 ≡ a9 → 1 (every tone step) | – | – |
| 12 | **grow** | Each empty cell with an inked 4-neighbour gets that neighbour's tone minus a drop. One ring per frame (`viz.c:638-655`) | a0 → nothing; drop = 1+(10−a)/3: a1 → 4, a2–4 → 3, a5–7 → 2, a8–9 → 1 | – | – |
| 13 | **thin** | Clears every inked cell that has an empty 4-neighbour or sits on the frame border (`viz.c:660-676`) | a0 → nothing; **a1–9 identical** | – | – |
| 14 | **flip** | Inked → empty; empty → tone lv (`viz.c:681-690`) | lv: a0 → 8, a1–8 → a, a9 → 8. **flip 0 turns an empty frame solid** | – | – |
| 15 | **fold** | Mirrors, 1 to 3 times: left half onto right, then top half onto bottom, then left onto right again (`viz.c:603-619`) | folds = 1+a/4: a0–3 → 1, a4–7 → 2, a8–9 → 3, but **the third fold is a no-op, so fold 8–9 ≡ fold 4–7** [probe] | – | – |

The "0 is none … 9 is full" rule (`viz.c:309-331`) does not hold for `disc`, `box`, `grid`, `flip`, `echo` and `edge`: each still draws at 0. The amount table in that comment also still lists the removed `tile` and the old `>viz` verb. It is stale.

### 1.6 Positions (`:x`, `:y`)

- **Address and scale.** `>disc:x 0..9..` is a parameter lane (`VIZ_PARAM_X`/`_Y`, `viz.h:142-171`; grammar `lane_name.h:10-13`). It sets the position as a cell: x = a·(w−1)/9, y = a·(h−1)/9. 0 is the left or bottom edge and 9 (or `x`) is the right or top, so a higher note routed to `:y` is higher (2026-09-29; 0 was the top). Routed from a voice, a position takes the degree played (`viz.c:1033-1037`) [probe].
- **Shared by instances.** Positions are stored per primitive, in `s_place[NGEN]` (`viz.c:104-111`), not per lane. `disc`, `disc:2` and `disc:2:x` all share one position.
- **Default and persistence.** The default is the centre, (w/2, h/2) (`viz.c:338-348`). A position persists until `>new` (`viz.c:294`).
- **Only three primitives read it:** `disc`, `box` and `turn` (`place_x()` calls at `viz.c:540`, `742`, `772`).
- **`:x`/`:y` are accepted on all sixteen primitives and silently ignored by the other thirteen** (`builtins.c:560-571`). Only an unknown part is refused, with `no :%s - a picture has x, y` (`builtins.c:567`).
- **Positions are never rescaled when the frame is resized** [probe]. A shape positioned at x = 9 in a 58-wide frame sits at column 57. After a resize to 28 wide it stays at column 57, off-frame and invisible, until the next position mark.

### 1.7 Frame, pane and split sizes

- **Live frame.** `s_w × s_h`, 28×10 before any layout (`viz.c:171`). `resize()` clamps to 4..60 × 2..24 and **blanks the frame** when the size changes (`viz.c:210-220`).
- **`viz_size(w,h)`** is called by the editor with the pane's inside every draw (`editor.c:477-481`). It is ignored while an output size is pinned (`viz.c:202-208`).
- **`viz_out_size(w,h)`** pins the frame to w×h for the view node; `(0,0)` unpins (`viz.c:180-187`). The pane then shows a nearest-neighbour sample, `viz_cell_fit`: source (x·s_w/pw, y·s_h/ph) (`viz.c:189-200`, `editor.c:493-498`) [probe]. After unpinning, the frame keeps the pinned size until the next editor draw [probe].
- **`viz_pane(cols, rows)`** (`viz.c:263-283`) always stacks: code on top, the picture full-width underneath.
  - The pane is off if the split is off, cols < 8, or rows < 7.
  - Height h = split rows + 2, or (rows+1)/2 when no row count is set.
  - h is reduced so the code keeps at least `CODE_ROWS_MIN` = 4 rows (`viz.c:261`), and capped at 26. If h < 3 the pane is off.
  - The pane includes a one-cell border, drawn `+`, `-`, `|` (`editor.c:483-501`).

| Density (text area) | Default pane → frame | `>split n` → frame | Largest frame |
|---|---|---|---|
| chunky (30×11) | rows 5–10 → **28×4** | n = 1–4 → 28×n | **28×5** (any n ≥ 5) |
| compact (60×23) | rows 11–22 → **58×10** | n → 58×n | **58×17** (any n ≥ 17) |

([probe]; `editor.c:451-467`)

**Turning the split on and off:**
- `>split` toggles; `>split on`, `>split off` and `>split <rows>` also work (`builtins.c:1570-1584`).
- Running any picture lane turns it on (`builtins.c:733-735`), and so does `>send view on`/`WxH` (`builtins.c:2116`).
- With the split off, frames are still generated, and still sent to the view if it is on. The frame keeps its last size.

**Split messages:** `split off`, or `view %dx%d below` (`builtins.c:1586-1594`). The size printed is **the frame size before the change**, because the resize happens at the next draw. It is the pinned output size while the view is on [read]. The usage message is `split on | off | <rows>` (`builtins.c:1577`).

### 1.8 `>frame` (OSC)

`>frame` sends the picture with no argument while `viz_active()` is true; otherwise it sends the current document, or `>frame <doc>` for a named one, capped at 1,023 B (`builtins.c:1701-1751`). It needs the NET capability (`builtins.c:2320`). The packet is OSC `/deck/frame` with one string argument, in a 1,100 B buffer (`firmware/components/net/osc.c:133-154`).

The picture is sent as ASCII rows ending in `\n`, via `viz_text` (`viz.c:1077-1115`):

| Code | Sent as |
|---|---|
| tones 0–8 | `" ..::*#@@"` |
| sparkles | `*` |
| arcs and any other code ≥ 128 | `#` |

Messages: `sent a %d-byte frame`, `sent %u bytes as a frame`, `no osc target. try: osc <ip> <port>`, `frame too large or send failed`.

A picture of more than 1,083 characters overflows the buffer, for example a 60×24 view frame (1,464 characters). The command returns an error but still says `sent a N-byte frame` (`builtins.c:1710-1716`) [read].

## 2. The view to the HDMI node

**Enabling it** (`builtins.c:2093-2125`; `VIEW.md:27-31`):

| Command | Effect |
|---|---|
| `>send view on` | streams at **80×30** (the node's whole screen, two square dots to a cell); pins the frame with `viz_out_size`; turns the split on; if already on, keeps its size |
| `>send view <mode>` | on, and the node draws it that way: plain, scan, riso, poster, code (`docs/VIEW.md`); colour from controllers 1-8 on channel 16 |
| `>send view WxH` | 4×2 up to 80×30; out of range gives `view is 4x2 to 80x30 cells` |
| `>send view off` | stops and unpins (the pane decides the size again) |
| `>send view` | shows `view is on, <mode>` / `view is off` |
| `>send` | lists destinations; this one's help text is cut to 25 characters: `the picture to an HDMI no` (`view.c:36`, `builtins.c:2071`) |

Other replies: `view on, <mode>`, `view off`, usage `send view on|off|53x20|mode` and the five modes.

**Capability.** `send` needs `CMD_CAP_SYSTEM` (`builtins.c:2313`), so it is **refused from the boot document**, which runs as GUIDE (`cmd.c:23-24`, `main.c:315`): `send: not permitted here`.

**Wire format** (`view_wire.h:5-10`, `26-54`; `VIEW.md:40-46`):

| Field | Size | Content |
|---|---|---|
| magic | 4 B | `'D' 'K' 'V' '1'` |
| tick | u32, little-endian | the 96 PPQN tick of the frame's last draw mark (`viz.c:1009-1010`, `942`); a frame made only by position marks keeps the previous tick [probe] |
| w, h | 1 B each | frame size in cells |
| cells | w·h B | row by row: 32–126 text, 128–155 tiles (`viz_frame`, `viz.c:1063-1075`) |
| sum | 1 B | XOR of every byte after the magic |

Total length = 11 + w·h (2,411 B at 80×30). A control frame, `'D''K''C''1'`, goes ahead of each - the mode, and the poster's lines (`docs/VIEW.md`). `viz_frame` always returns w·h; the header's "0 if nothing has been drawn" (`viz.h:203`) is inaccurate.

**Transport today** (`view.c:9-14`, `53-76`; `VIEW.md:60-69`):
- The console carries `ESC ] view;<base64> BEL \n`. `tools/viewrelay.py --deck <port> --view <port>` lifts it out and writes it to the node's USB serial.
- The write is one non-blocking `usb_serial_jtag_write_bytes` into a 4,000 B TX ring (`serialkbd.c:59-63`). If it does not fit, the frame is **dropped whole** (`view_dropped++`), never torn.
- In USB MIDI mode it goes through stdio instead. That path is UNVERIFIED (`VIEW.md:88-90`).
- A direct deck→node USB link is not built.

**Frame rate.** One frame per main-loop pass in which `viz_service()` produced a frame (`main.c:869-877`). That is one per step in which any picture or position lane fired; with nothing marked, nothing is sent. Measured per docs: one per sixteenth, 8.2/s at 124 bpm, 0 refused (`VIEW.md:73-75`). The heartbeat reports `sent` / `dropped` (`main.c:950-954`).

**The node** (Feather RP2040 DVI, 640×480, light ink on black, best face and whole-number scale). This is described only in `VIEW.md:9-23` and `view/deckview/`, and was not checked against the deck firmware.

*How verified.* Scratch harnesses outside the repository compiled `viz.c`, `editor.c` +
`cmd.c` + `viz.c` + the fonts, `textgrid.c` + the fonts, and `serialkbd_map.h`, and drove
them against a fake document store, a captured text grid and a fake lane table.
The sequencer, BLE, SPI and the panel itself were not exercised.
