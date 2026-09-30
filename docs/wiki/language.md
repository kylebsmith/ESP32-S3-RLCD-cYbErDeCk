# The language — lines, steps, names, lanes, routes

*Part of [the deck, top to bottom](README.md). Snapshot: commit `85d1e6a`, 2026-09-28.
Source is the truth; this page quotes it.*

**Conventions**

- Citations are `file:line`, paths below.
- **[sim]** — also checked by *running the shipping code* on the host: the pure headers
  compiled as-is, and `seq.c` compiled verbatim with minimal ESP-IDF/FreeRTOS shims,
  ticks driven by hand. Nothing in the repo was modified (appendix).
- **UNVERIFIED** — follows from the code but was not observed on the device, or cannot
  be settled from code.
- *cycle* — one pass of a lane's pattern (its `slots`). The code and its messages also
  call this a "bar"; it is **not** a 16-step bar.
- Status-bar messages are quoted as their `printf` format strings, exactly.

| Short name | Path |
|---|---|
| `seq_pattern.h` | `firmware/components/seq/include/seq_pattern.h` |
| `seq_scale.h` | `firmware/components/seq/include/seq_scale.h` |
| `seq_clock.h` | `firmware/components/seq/include/seq_clock.h` |
| `seq.h` / `seq.c` | `firmware/components/seq/include/seq.h`, `firmware/components/seq/seq.c` |
| `lane_name.h` | `firmware/components/cmd/include/lane_name.h` |
| `builtins.c` / `cmd.c` | `firmware/components/cmd/builtins.c`, `firmware/components/cmd/cmd.c` |
| `ui_text.h` / `main.c` / `editor.c` | `firmware/main/…` |
| `osc.c` / `osc_parse.h` | `firmware/components/net/osc.c`, `firmware/components/net/include/osc_parse.h` |
| `viz.c` / `viz.h` | `firmware/components/viz/viz.c`, `firmware/components/viz/include/viz.h` |

## 1. How a line reaches a lane

- A line runs only if its first non-blank character is `>`; `>` followed by nothing or `#` is a no-op (`cmd.c:229-245`).
- The first word ends at space, tab or `=` (`cmd.c:110-118`). The rest, leading blanks stripped, is `arg`: one unparsed string. **Trailing blanks are kept** (`cmd.c:247-254`); the editor does not trim them (`editor.c:922-941`).
- `=` decides first: if `arg` starts with `=`, the line is a definition even when the word is a verb (`cmd.c:256-260`, `290-295`). `>conga=note 63` works (`cmd.c:108-109`).
- Else an exact verb runs (`cmd.c:262-285`); 37 verbs (`builtins.c:2308-2343` at `85d1e6a`, 34 then; `toggle`, `clear` and `map` since 2026-09-29).
- Else, if the word's base (up to `:`, at most 8 chars) is a defined name or a picture, the line goes to `cmd_lane` (`cmd.c:290-295`; `builtins.c:499-512`). Definitions and lanes need `CMD_CAP_EDIT` (`cmd.c:287-293`); otherwise `"%.*s: not permitted here"`.
- Else the old-spelling hints (`cmd.c:179-212`): `"%.*s[%.*s] is %.*s:%.*s now"` (`disc[x]`), `"%.*s is %.*s:%.*s now"` (`disc2`), `"cc is a kind now: >fx = cc 74"`, `"%.*s is gone: > marks a line"` (guide/prose), else `"%.*s? try: help"`. A base longer than 8 is never "known", so a 9-letter lane word gets `"…? try: help"`, not the length message (`builtins.c:503-509`).
- Line length: the editor reads at most 127 chars of the current line (`editor.c:1040-1041`, `922-941`); `>run` and the boot document split lines longer than 127 into chunks (`builtins.c:113-131`, `main.c:307-319`).

## 2. The step grammar (`seq_pattern.h`)

One compiler, shared by the clock and the playhead (`seq_pattern.h:1-11`). A step is a
head character plus what is attached to it (`seq_pattern.h:12-43`).

### 2.1 Heads

| Head | Leaf kind | Value | Cite |
|---|---|---|---|
| `x` | hit | `SEQ_VAL_X` (0xFF) = "the lane's own level" | `seq_pattern.h:23`, `81`, `555` |
| `0`–`9` | hit | 0–9, "how much" (meaning per binding, §7) | `seq_pattern.h:24-25`, `585-586` |
| `.` | rest (the only rest) | — | `seq_pattern.h:26`, `556-559` |
| `_` | tie | — | `seq_pattern.h:27`, `561-583` |
| `u` `d` `l` `r` | hit with a direction, value `x` | dir set | `seq_pattern.h:28-29`, `587-589` |

- Head set: `seq_pattern.h:152-156`. **Spacing** = space or tab, ignored between items (`seq_pattern.h:143-150`); `x... x...` has the same timing as `x...x...` but a different playhead mapping and toggle hash [sim].
- Anything else is refused with its position (§2.10).

### 2.2 `%NN` — odds

- Allowed right after a head, or right after a closing `]`/`>` (`seq_pattern.h:298-304`).
- `%` + 1–3 digits, value 0–100 (`seq_pattern.h:158-175`). `%0` never plays; `%100` always; `x%050` = 50 [sim].
- **One per step**: `x%50%50` → `"'%' is not a step - x hits"` at the second `%` [sim].
- Accepted on `.` and `_` (`.%50`, `x_%50` compile) but has no effect: rests and ties never become events (`seq.c:1250`) [sim].
- Malformed (`x%`, `x%101`, `x%1000`) → PERCENT at the `%` [sim].

### 2.3 Groups, stacks, alternation

- **`[ab]` group**: occupies one step (or one share of its parent) and divides it equally among its items (`seq_pattern.h:30`, `595-622`).
- **`[a,b,c]` stack**: `,` separates members; every member spans the whole group and starts together (a chord). Members are sequences: `[02,45]` plays 0+4, then 2+5 (`tools/test_seq_pattern.c:190-201`) [sim].
- **`<ab>` alternation**: every *item* is one alternative; spacing is optional (`<35>` ≡ `<3 5>`) [sim]. Alternative *j* of *k* plays when `(cycle / per) % k == j`, compiled as the class `cycle % (per·k) == ph + per·j` (`seq_pattern.h:502-531`). Nested alternation advances only when chosen: `<0 <1 2>>` → 0, 1, 0, 2 (`seq_pattern.h:49-51`) [sim].
- **Words in `<>`** (2026-09-29, `seq_pattern_words`): two or more spaced words of one length *k* take *k* steps and are played whole, column by column — `.000.000.000.<000 777>` is sixteen steps, and the last three are `000` one cycle and `777` the next. Words of different lengths are refused, boxed at the word: `"words in < > need one length"`. One word, or words of one step, alternate step by step as before (`<35>` ≡ `<3 5>`), so nothing written earlier reads differently; the Strudel corpus and every piece were checked. `tools/test_seq_pattern.c` §10b. I asked for it: `<000 777>` "was sequentially moving through those instead of picking either or".
- **`,` inside `<>`** stacks alternations: `<0 1, 4 5 6>` plays one of each at once, period 6 [sim] (`seq_pattern.h:356-384`, `491-533`). Not in the header's list.
- `,` outside brackets → COMMA (`seq_pattern.h:291-293`). Empty group or member (`[]`, `<>`, `[ ]`, `[x,]`, `[x,,x]`) → EMPTY_GROUP (`seq_pattern.h:307-335`) [sim].
- A group or alternation can take `%NN`; it multiplies into everything inside (§2.9).

### 2.4 Nesting depth

`SEQ_PATTERN_MAX_DEPTH` = 4 (`seq_pattern.h:71`). `[` and `<` count alike: `[[[[x]]]]` and `<[<[x]>]>` compile; a fifth opener is refused at that opener (`seq_pattern.h:264-274`, `397-400`) [sim].

### 2.5 A direction in front

If the first non-blank character is `u`/`d`/`l`/`r`, **and** the next is spacing, **and** more follows, it is the lane's direction and not a step (`seq_pattern.h:645-653`). `u 4.4.` → dir `u`, 4 slots; `u` alone is a step (trailing blanks are trimmed first) [sim]. Without one the lane's direction is `d` (`seq.c:1336-1338`). A step's own `u d l r` wins over the lane's (`seq.c:495`). Only `move`, `warp`, `ramp`, `turn` (without a part) accept either; on any other binding the line is refused with `"u d l r: move warp ramp turn"`, boxed at the direction or the first such step (`builtins.c:700-724`; `viz.c:895-903`).

### 2.6 Trailers `/n` `*n` `!n`

- Whitespace-separated tokens at the end of the line, any order (`seq_pattern.h:195-241`). `/n` = rate divisor, `*n` = rate multiplier, `!n` = play n cycles then stop.
- A token is the operator plus 1–3 digits (2–4 chars) (`seq_pattern.h:219-223`): `/02` ok, `/0002` refused [sim].
- Ranges: rate 1–32 (`SEQ_PATTERN_MAX_RATE`), count 1–255 (`SEQ_PATTERN_MAX_COUNT`) (`seq_pattern.h:75-76`, `224-229`).
- **One rate and one count.** `/2 /3`, `/2 *3` and `!2 !3` → TWICE, boxed at the *earlier* token (scanning runs from the end) (`seq_pattern.h:230-236`) [sim]. A rate is therefore n or 1/n, never n/m.
- Scanning stops at the first token not starting with `/ * !`, and at a token at the very start of `arg` (`seq_pattern.h:208-218`). So `x.x./2` → `"/ goes last: x.x. /2"` at 4; `>kick /2` → the same at 0; `!4 /2` → `"! goes last: x.x. !4"` at 0 [sim].
- `plen` = characters before the trailers, trailing spacing trimmed (`seq_pattern.h:239-240`, `640`).

### 2.7 How slots, div, steps and per are computed

Two passes over the same recursive shape: MEASURE then PLACE (`seq_pattern.h:243-253`).

- `need(step) = 1`; `need([m1, m2…])` = lcm of each member sequence's need; `need(<…>)` = lcm of every alternative's need; `need(sequence of k items) = k × lcm(need(item))` (`seq_pattern.h:341-418`).
- `slots = need(top)`, `steps` = number of top-level items, `div = slots / steps` (`seq_pattern.h:672-675`). Every top-level step gets `div` slots; items inside a group share its width equally (`seq_pattern.h:608-618`); every alternative spans the whole item.
- `per` = lcm over the pattern of each alternation's cycle, where a member of *k* items contributes `k × lcm(per of its items)` (`seq_pattern.h:365-384`); refused above 240 (`seq_pattern.h:668-671`), and again at placement if `per × k > 240` at a `<` (`seq_pattern.h:514-518`).
- lcm saturates at 100000 (`seq_pattern.h:185-191`).
- Examples [sim]: `x..[xx]` → 8 slots, div 2 · `[xx][xxx]` → 12, div 6 · `[xxxxx][xxxx][xxx]` → `"needs 180 slots, 64 fit"` · `x...x...x...x...x...x...x...x...<3 5>` → 33 slots, per 2 (alternation costs no slots) · `<0 1><2 3 4><5 6 7 8 9><x x x x x x x>` → per 210 ok; eleven alternatives in the last → `"repeats in 330 bars: 240 max"`.

### 2.8 Ties

- `_` extends every note that ends exactly where the tie starts (the *tail*) by the tie's width (`seq_pattern.h:420-426`, `561-583`).
- The tail: after a hit, that hit; after a group, its last item's tail (`[xx]_` holds the second x); after a stack, the union of its members' tails (`[0,4]_` holds both; `[03,4_]` holds only the 4); after an alternation, the union of its alternatives (`<0 3>_` holds whichever played); after a rest, nothing (`x._` — the tie acts as a rest). Ties chain: `0__` sounds 3 slots (`seq_pattern.h:489-536`, `556-559`; `tools/test_seq_pattern.c:204-231`) [sim]. A tie at the start of a group holds the note before the group: `x[_x]` → the x sounds 3 slots [sim].
- `_` before any leaf at all → TIE_START (`_x..`, `[_x]`, `<_ x>`) (`seq_pattern.h:540-543`) [sim]. `._` is fine. A tie never carries across the end of the cycle.
- A note whose cycles meet the tie's cycles but are not contained in them → TIE_ALT: `0<_ .>`, `0<_ x>`, `[0<_ .>]` (`seq_pattern.h:563-577`) [sim].
- One tie holds at most 16 notes (`SEQ_PATTERN_MAX_TAIL`, `seq_pattern.h:74`, `468-473`); extras are **silently** not held (18-note chord + `_` → 16 held) [sim].
- Ties are not events; the hit carries `hold = len − width` slots (`seq.c:1256`), turned into time at note-on (§7.6).

### 2.9 Odds

- A leaf's odds = `combine(parent, own)`, `combine(a, b) = (a·b + 50) / 100`, "always" counting as 100, and always×always = always (`seq_pattern.h:451-461`, `483-485`). `[x%50x]%50` → 25 and 50 [sim].
- **One roll per slot**, per lane: `roll = xorshift32() % 100`, drawn lazily at the first event of that slot (in this cycle's class) that has odds, shared by every event on the slot; an event plays iff `roll < prob` (`seq.c:663-683`). `[0%30,4%60]` never plays the 0 without the 4 [sim, 2000 steps: both 621, only 4 609, only 0 **0**, none 770].
- RNG: xorshift32 seeded `0x9E3779B9` at boot, never reseeded (`seq.c:365-384`).
- A routed (sidechain) lane ignores odds entirely (`seq.c:597-610`) [sim].

### 2.10 Refusals (`seq_pattern_error_text`, `seq_pattern.h:809-868`)

Every message is ≤ 30 columns (`tools/test_seq_pattern.c:310-327`). `err_at` is the character the editor boxes (`cmd.c:296-298`).

| Code | Trigger | Boxed at | Exact message |
|---|---|---|---|
| `SEQ_PAT_EMPTY` | nothing but spacing | — | `"ok"` — not an error; `seq_lane` removes the lane (`seq.c:1299-1307`) |
| `SEQ_PAT_BAD_CHAR` | not a head/bracket/`,`/modifier | the char | `X` → `"X is gone: 9 is loud"`; `?` → `"? is gone: x%%50 is maybe"`; `-` → `"- is not a rest here: ."`; `~` → `"~ is not a rest here: ."`; `*` `/` → `"%c goes last: x.x. %c2"`; `!` → `"! goes last: x.x. !4"`; `@` → `"@ is not here: hold with _"`; other printable → `"'%c' is not a step - x hits"`; else `"not a step - x hits"` |
| `SEQ_PAT_OPEN` | `[`/`<` never closed | the opener | `"'%c' is never closed"` |
| `SEQ_PAT_CLOSE` | `]`/`>` with nothing open | the closer | `"'%c' closes nothing"` |
| `SEQ_PAT_MISMATCH` | `[` closed by `>`, `<` by `]` | the closer | `"'%c' closes the wrong one"` |
| `SEQ_PAT_EMPTY_GROUP` | empty group or member | end of the empty member | `"empty brackets"` |
| `SEQ_PAT_COMMA` | `,` outside brackets | the comma | `"a chord goes in []: [0,4,7]"` |
| `SEQ_PAT_PERCENT` | `%` not 1–3 digits ≤ 100 | the `%` | `"%% takes 0-100: x%%25"` |
| `SEQ_PAT_TIE_START` | `_` with no leaf before it | the `_` | `"_ needs a note before it"` |
| `SEQ_PAT_TIE_ALT` | tie holds a note only some cycles | the `_` | `"_ holds it only some bars"` |
| `SEQ_PAT_DEEP` | fifth nested bracket | that bracket | `"brackets go %d deep at most"` (4) |
| `SEQ_PAT_SLOTS` | slots > 64 | pattern start | `"needs %d slots, %d fit"` (needed, 64) |
| `SEQ_PAT_LEAVES` | > 160 leaves | the 161st leaf | `"over %d steps"` (160) |
| `SEQ_PAT_PER` | period > 240 | pattern start, or the `<` | `"repeats in %d bars: %d max"` |
| `SEQ_PAT_TRAILER` | `/ * !` token not a number in range | the token | `!` → `"!n is 1-%d times"` (255); else `"a rate is /1-/%d or *1-*%d"` (32) |
| `SEQ_PAT_TWICE` | a second rate or count | the earlier token | `"one rate and one count only"` |
| `SEQ_PAT_FINE` | `div·rnum > 24·rden` | pattern start | `"too fine for the clock"` |
| (default) | — | — | `"not a pattern"` |

Refusals above the compiler:
- `seq_lane`: more than 96 hits, **counted across all alternatives** → `"%d notes: %d fit a lane"`, `ESP_ERR_INVALID_SIZE`, nothing boxed (`seq.c:1311-1331`). Ten `<0123456789>` → `"100 notes: 96 fit a lane"` although 10 sound per cycle [sim].
- `cmd_lane`: `"u d l r: move warp ramp turn"` (§2.5).
- A refused pattern changes nothing in the lane's events (`seq.c:1295-1297`) — but see §12 #7 about the binding.

## 3. Time

### 3.1 The clock

- 96 PPQN; a step is a sixteenth = 24 ticks; MIDI clock `0xF8` every 4th tick when `>sync on` (`seq.h:64-77`; `seq.c:925-927`). A `0xF9` step marker goes into the event queue every 24 ticks (`seq.c:947-955`).
- `>bpm` takes a whole number 20–300, else `"bpm is a number, 20-300"`; reply `"%d bpm"` (`builtins.c:2128-2144`). `seq_bpm` clamps 20–300 (`seq.c:1482-1486`) [sim]. Default 120 (`seq.c:23`); the boot document sets 124 (`ui_text.h:98`).
- Tick period = `(uint64)(60,000,000 / bpm / 96)` µs, truncated (`seq.c:960-965`) [sim]: 20 bpm 31,250 · 120 bpm 5,208 (sixteenth 124,992) · 124 bpm 5,040 (sixteenth 120,960) · 300 bpm 2,083.
- A one-shot timer re-armed each tick at the grid's due time; it runs while stopped so note-offs drain and staged lanes land (`seq_clock.h:4-37`; `seq.c:839-848`, `1085-1088`).
- Tempo change while running keeps the musical position (the grid is re-anchored); while stopped it resets the tick to 0 (`seq.c:1487-1514`; `seq_clock.h:39-46`) [sim].
- Per tick: service note-offs → apply a staged lane → (running) follower bookkeeping → MIDI clock → inputs → lanes → tick hook → step marker (`seq.c:850-958`).

### 3.2 Where a slot falls

- Every lane counts slots from tick 0 at `>play`. Global slot index *g*: `slot = g mod slots`, `cycle = g div slots` (`seq_pattern.h:743-766`).
- Unswung start of *g*: the tick nearest `g·Q/D` = `⌊(2gQ + D) / 2D⌋`, with `Q = 24·rden`, `D = div·rnum` (`seq_pattern.h:695-705`, `728-741`); within ½ tick of ideal (`tools/test_seq_pattern.c:391-416`).
- A cycle lasts `steps × 24 × rden / rnum` ticks, independent of subdivision (`[xxxxx]...` = 96 ticks) [sim].
- `seq_pattern_slot_at` answers "does a slot start exactly on this tick": it starts from the last slot whose unswung start ≤ tick and walks back through slots swing delayed onto it (`seq_pattern.h:750-765`).
- A lane written mid-play joins at the phase counted from play: `123` written at sixteenth 5 starts on its `3` [sim]. Counted lanes differ (§8).

### 3.3 Rates

`/n` makes each slot n times longer, `*n` n times shorter — the lane's own clock (`seq.h:140-147`). `x.x. /2` → a slot every 48 ticks; `*2` → 12; `/3` → 72 [sim]. FINE refuses a slot shorter than one tick, `div·rnum > 24·rden` (`seq_pattern.h:676-684`): `[xxxxxxx] *4` (28), `[xxxxx] *5` (25), `[xx] *24` (48) refused; `[xxxxxx] *4`, `x.x. *24` accepted [sim]. **FINE ignores swing** (§12 #1).

### 3.4 Swing

- `>swing` takes 50–75, else `"swing is 50-75: 67 is triplet"`; reply `"swing %d"` plus `" (straight)"` at 50 or `" (triplet)"` at 66–68 (`builtins.c:932-946`). `seq_swing` clamps 50–75 (`seq.c:1453-1458`) [sim]. Default 50 (`seq.c:35`).
- `swing_ticks = ⌊pct·48/100⌋ − 24`, clamped 0–22 (`seq.c:357-363`); the slot math clamps again to 0–23 (`seq_pattern.h:732-733`). Reachable values [sim]:

| % | 50–52 | 53–54 | 55–56 | 57–58 | 59–60 | 61–62 | 63–64 | 65–66 | 67–68 | 69–70 | 71–72 | 73–74 | 75 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| ticks | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |

- **The warp** (`seq_pattern.h:706-741`): within each eighth (48 ticks) the first sixteenth is stretched to `24+s` ticks and the second starts `s` late, squeezed to `24−s`; anything inside a sixteenth keeps its proportional place; rounded to the nearest tick.
- **Moves:** the second sixteenth of every eighth (`xxxx` at 67 % → 0, 32, 48, 80 [sim]); sub-steps inside a sixteenth (`[xxx]` at s = 8 → 0, 11, 21 | 32, 37, 43 [sim]); pads (next *swung* step, `seq.c:826-833`); sidechain lanes, which fire when their source fires.
- **Does not move:** the start of every eighth; eighth-note lanes however spelled (`xxxxxxxx /2` ≡ `x.x.x.x.x.x.x.x.`, `tools/test_seq_pattern.c:482-513`) [sim]; the playhead (`seq_pattern.h:768-770`); knobs (next tick); `0xF8`/`0xF9` (tick-based).

### 3.5 Alternation by cycle

An event plays when `cycle % per == ph` (`seq.c:671`), against the lane's own cycle — or, for a counted lane, its local pass number, so an alternation restarts with each section (`seq.c:619-647`). `<0 2 4> !3` typed mid-song plays 0, 2, 4 [sim].

### 3.6 The playhead

- `seq_lane_now`: false when stopped or `slots == 0`; position from `seq_pattern_slot_now` (unswung, `seq_pattern.h:768-782`); for a counted lane remapped to its own passes and false while waiting, done or past its count (`seq.c:1350-1372`).
- `seq_pattern_mark`: the span from the first character of the first leaf covering that slot (in this cycle's class) to the end of the last; rests and ties included; `x%15` lights four characters; a chord lights first note to last (`seq_pattern.h:784-805`).
- The editor marks a lane line only while running, when the lane exists by canonical name, is not muted, has slots, the line's argument hashes to the lane's `src` (unedited), and the text compiles (`editor.c:386-436`). It does not check routing (§12 #13).

## 4. Names and definitions

### 4.1 A name

A lowercase letter, then lowercase letters or digits, at most 8 (`LANE_BASE_MAX`) (`lane_name.h:50`, `86-101`). A name may not be a verb or a picture (§4.3).

### 4.2 Definition syntax (`lane_def_parse`, `lane_name.h:218-290`)

```
>name = note N  [ch C] [gate G]    a drum         N 0-127
>name = voice O [ch C] [gate G]    a voice        O = octave 0-8
>name = cc N    [ch C]             a controller   N 0-127
>name = knob                       an input holding a value
>name = pad                        an input that fires on a press
>name = <picture>                  another name for a picture
>name =                            remove the name
```

- **Moving a sound: `>name = ch C [gate G]`** (2026-09-29, `lane_def_shift`). A definition that starts with `ch` or `gate` keeps everything else the name had: `>bass = ch 5` sends the bass that is playing to channel 5, same octave, same gate — Bass2 in the DAW — and replies with the whole definition, `bass = voice 2 ch 5 gate 110`. A name that is not a sound: `"%.8s: no sound to move"`. `tools/test_lane_name.c`.
- `ch` 1–16 and `gate` 1–5000 ms, in any order, repeatable (last wins) [sim] (`lane_name.h:240-257`). `gate` is not accepted on `cc` (`lane_name.h:248`).
- **Defaults:** note → ch 10, gate 40 ms; voice → ch 1, gate 150 ms; cc → ch 1 (`lane_name.h:218-233`).
- Kind words are exact lowercase. Words are cut to 15 characters (`lane_name.h:224`, `193-203`).
- An alias must name one of the 16 pictures (`viz.c:62-68`) — not another alias or a sound: `>ring = circle` → `"no picture called %s"` (`builtins.c:849-856`).

| Refusal (`lane_def_parse`) | Exact message |
|---|---|
| number missing / out of range | `"note takes 0-127"`, `"voice takes an octave 0-8"`, `"cc takes 0-127"` |
| bad or missing `ch` value | `"ch is 1-16"` |
| bad `gate` value | `"gate is 1-5000 ms"` |
| any other keyword, or `gate` on cc | `"'%.10s'? ch N or gate N"` |
| anything after knob/pad | `"%s takes nothing else"` |
| kind word with a non-lowercase char | `"note, voice, cc or a picture"` |
| anything after a picture name | `"a picture takes nothing after"` |
| lowercase word longer than 8 | `"no picture is that long"` |

### 4.3 `cmd_define` (`builtins.c:767-876`), in order

1. Address invalid → `lane_name_error_text` (§5).
2. Instance or part given → `"define the plain name: %s"`.
3. A verb → `"%s is a command"`; a picture → `"%s is a picture already"` (`builtins.c:781-792`).
4. One leading `=` skipped; `lane_def_parse` error → its message.
5. If the name was a knob/pad and the new kind differs (including removal) → the input is removed (`builtins.c:804-809`).
6. Removal → §4.6. Knob/pad → §4.7.
7. Picture alias: unknown picture → `"no picture called %s"`.
8. New name and table full → `"%d names is all there is"` (32, `ALIAS_MAX`, `builtins.c:468-472`).
9. Store: `chan` as 0–15, `num` = note / octave / cc / picture index, `gate` (`builtins.c:868-872`); rebind live lanes (§4.5); reply `"%s =%s"` (name, text after `=`).

### 4.4 The boot names (`ui_text.h:74-93`)

| Name | Line | Name | Line |
|---|---|---|---|
| kick | `>kick = note 36` | bass | `>bass = voice 2 ch 1 gate 180` |
| snare | `>snare = note 38` | lead | `>lead = voice 4 ch 2 gate 120` |
| hat | `>hat = note 42` | pad | `>pad = voice 3 ch 3 gate 420` |
| ohat | `>ohat = note 46` | arp | `>arp = voice 5 ch 4 gate 90` |
| clap | `>clap = note 39` | cut | `>cut = cc 74` |
| tom | `>tom = note 45` | res | `>res = cc 71` |
| rim | `>rim = note 37` | mod | `>mod = cc 1` |
| crash | `>crash = note 49` | rev | `>rev = cc 91` |

Drums take ch 10, gate 40; cc take ch 1. The block starts with the mark `"THE NAMES ARE YOURS"` (`ui_text.h:73`). A boot document without it gets the block inserted at its top (`main.c:286-306`). The boot document runs line by line with guide authority at startup (`main.c:307-319`). A new one is written from `BOOT_TEXT`: the names block, then `>bpm 124`, `>scale dmin`, `>swing 50`, `>density chunky` (`ui_text.h:95-103`; `main.c:266-278`). The boot voice called `pad` is unrelated to the input kind `pad` (`>x = pad`).

Pictures are not definitions: 16 answer to their own names — `echo move spin warp noise disc box turn ramp grid mask edge grow thin flip fold` (`viz.c:9`, `62-68`).

### 4.5 Redefining a name changes what is playing

`rebind_lanes` re-binds every live lane with that base (instances and parts) whose address still binds (`builtins.c:745-764`, `873`), via `seq_lane_bind`, which sets every binding field and clears the rest (`seq.c:1394-1412`). So `>kick = note 35` moves the running kick, and `>kick = voice 2` turns its digits from velocity into degree. Side effect: the lane's octave resets to the name's (`seq.c:1404`, `1410`). A part the new kind lacks (`bass:oct` after `>bass = note 40`) fails `binding_of`, is not rebound, and keeps its old binding (`builtins.c:759-761`).

### 4.6 Removal — `>name =`

- Not defined → reply `"no name %s"` (status DONE) (`builtins.c:810-814`).
- Otherwise every lane with that base (all instances and parts) is forgotten, then the name freed: `"%s is not a name now"` (`builtins.c:815-830`). An input is removed too (`builtins.c:804-809`).
- Lanes routed *from* the removed name keep their route and go silent (`seq_forget` clears only the forgotten lane's own route, `seq.c:1194-1198`).

### 4.7 Inputs — `>name = knob` / `>name = pad`

`seq_input_define` (16 max: `"%d inputs is all there is"`), then every lane with that base is forgotten — an input has no pattern (`builtins.c:832-848`). knob → knob keeps its value; a kind change removes and re-creates it (value 0) (`seq.c:746-770`). An input cannot be played as a lane: `"%s is a %s: route from it"` (`builtins.c:543-548`). Behaviour: §10.

## 5. Addresses (`lane_name.h:6-26`, `78-166`)

`base[:instance][:part]`

- **instance**: all digits, 1–99; `:1` and `:01` are the plain name (§12 #20) [sim].
- **part**: 1–5 lowercase letters (`LANE_PART_MAX`).
- Instance before part, each at most once.
- **Canonical** = `base[:inst if > 1][:part]` — the lane's name everywhere: the lane table, `>route` (both sides), the playhead (`lane_name.h:142-149`; `builtins.c:678-682`, `1630-1644`; `editor.c:392-399`). `disc:1` and `disc` are one lane.

| Code | Example | Exact message |
|---|---|---|
| `LN_BASE` | `Disc`, `2disc`, `a-b`, `kick ` (trailing space) | `"a name is letters: conga"` |
| `LN_LONG` | `abcdefghi` | `"a name is %d letters at most"` (8) |
| `LN_INST` | `disc:0`, `disc:100` | `"a second one is :2 to :%d"` (99) |
| `LN_PART` | `kick:velocity`, `kick:v2` | `"a part is a word: disc:x"` |
| `LN_ORDER` | `disc:x:2`, `disc:2:3`, `disc:x:y`, `disc:`, `disc::x` | `"number, then part: disc:2:x"` |
| `LN_BRACKET` | `disc[x]` | `"a part is :x now, not [x]"` |
| other | — | `"not a name"` |

**Parts by binding** (`binding_of`, `builtins.c:514-592`):

| Binding | Parts | Refusal for any other part |
|---|---|---|
| drum (`note`) | `vel` | `"a drum has :vel"` |
| voice | `vel`, `oct` | `"a voice has :vel and :oct"` |
| cc | none | `"%s has no parts"` |
| picture / picture alias | `x`, `y` (`viz.c:905-911`) | `"no :%s - a picture has x, y"` |
| knob / pad | not a lane | `"%s is a %s: route from it"` |
| undefined base (via `>route`) | — | `"%s? try: help"` |

`:end` is meaningful only as a route *source* (§9). Picture instances draw separately (`viz.c:86-98`), but a picture's position is per primitive (§12 #12).

## 6. Lanes

### 6.1 `>name pattern` — `cmd_lane` (`builtins.c:660-743`), in order

1. Parse the address → its error (§5).
2. `binding_of` → its error (§5).
3. Lane name := canonical address.
4. Empty `arg` → remove (§6.4): `"%s gone"` or `"no %s"`.
5. Re-run (§6.3) → the lane is removed: `"%s off - again for on"`.
6. Direction check (§2.5).
7. `seq_lane_bind` — creates the lane if new; no free lane → `"%d lanes is all there is."` + `"free one: type its name alone"` (`builtins.c:646-652`, `725-727`).
8. `seq_lane` compile → refusal message, boxed (`builtins.c:653-657`, `728-731`).
9. `seq_mute(name, false)` — **a successful run always unmutes** (`builtins.c:732`).
10. A picture turns the preview split on (`builtins.c:733-735`).
11. Reply `"%s %s in %s"` (voices: name, pattern, key) or `"%s %s"` (`builtins.c:736-741`).

### 6.2 Compile and stage (`seq.c:1293-1348`, `1225-1277`, `687-725`)

- Compiles first into a static work area; nothing is touched until the pattern is good (`seq.c:1295-1298`).
- `SEQ_PAT_EMPTY` removes the lane without rerank or route clear (`seq.c:1299-1307`); `cmd_lane` never reaches this (an all-blank argument is empty).
- Refused: a lane that never compiled and is not routed is released (`seq.c:1315`, `1325-1329`) [sim].
- Accepted: hits become events sorted by slot with a per-slot index (`seq.c:1240-1277`); the clock copies them at the top of its next tick, stopped or not (`seq.c:855-858`); `stage_wait` waits up to 40 RTOS ticks, then applies directly (`seq.c:1225-1238`). Applying resets the count: `idle = true`, `done = false` (`seq.c:717-721`).
- Then `dir` (default `d`), the text as typed (≤ 99 chars kept, `seq.h:124-127`), `src` = FNV-1a hash of the whole argument (`seq.c:1336-1345`; `seq_pattern.h:882-906`), rerank.
- 16 lanes (`seq.h:32-39`). A new lane starts at ch 10, level 100, gate 40 before binding (`seq.c:1129-1138`); `seq_lane_bind` uses gate 40 when the binding gives 0 (`seq.c:1406`).

### 6.3 The re-run removes (`rerun_silences`, 2026-09-29)

Ctrl+Enter **removes the lane** instead of compiling iff all hold:

1. The caller is my hands (`CMD_BY_HANDS`; the editor's run key, `editor.c:1061`) — never the boot document, `>run`, or an agent.
2. The transport is running.
3. A lane with the canonical name exists.
4. It is not muted.
5. Its `src` equals the FNV-1a hash of `arg` — byte for byte, spacing, direction, trailers and trailing blanks included.

Then the lane is forgotten (§6.4) and the status says `"%s off - again for on"`: the same line once more compiles it afresh, from its next slot. Otherwise the line compiles and (step 9) unmutes. Lanes created by `>route` have `src = 0` and never re-run (`seq.c:1443-1444`).

**Why it changed.** Until 2026-09-29 the re-run muted and kept the slot. Playing, I said: "there is not concrete way to eliminate lanes". A muted lane still held one of sixteen, and nothing on the page said so. To silence a lane and keep it, there are four words now: `>mute`, `>solo`, `>toggle` (a block, and the same line brings it back) and `>map` (all but one, for MIDI learn); `>clear` drops them all and keeps the page ([verbs.md](verbs.md) §2.24–2.28).

### 6.4 Removing a lane

A bare name → `seq_forget` (`seq.c:1181-1212`): the lane stops, its own route and trigger are cleared, a part resets its parent (`vel` → 100, octave → the name's), ranks are recomputed. A sounding note still gets its note-off (`seq.h:325-326`). Lanes that follow it keep their route and wake when a lane of that name returns [sim]. `>new` forgets every lane (`builtins.c:283-299`).

### 6.5 Mute and solo (`builtins.c:2254-2298`)

- `>mute a b` mutes the named lanes; `>solo a b` mutes every other lane and unmutes the named; either with no argument unmutes all.
- Names match canonical lane names exactly, space-separated.
- Reply `"%s: %d lane%s changed"` — `mute` / `solo` / `all on`.
- The clock skips a muted lane completely: no events, no trigger consumed, a counted lane neither starts nor finishes (`seq.c:591`). A running count is timed from its origin, so after unmuting it resumes where time says (`seq.c:645-660`) — UNVERIFIED on device.

### 6.6 Play, stop, panic

- `>play` (`seq.c:1526-1558`): every counted lane re-armed (`idle`, trigger cleared; a finished one is un-done and unmuted — a hand mute stays); tick 0; grid re-anchored on the first tick; stats reset; a follower waits up to 1 s for the leader's count; with sync, `0xF2` then `0xFA`. Reply `"playing at %d"`.
- `>stop` (`seq.c:1560-1567`): `0xFC` if sync; then every pending note-off at once plus CC 123 on all 16 channels (`seq.c:1569-1582`) [sim]. Reply `"stopped"`.
- `>panic`: stop, then all notes off again; `"all notes off"` (`builtins.c:2300-2306`).

### 6.7 `>lanes` (`builtins.c:2160-2252`)

- Sidechain: `"%c%-5s <- %s"`. Cue: `"%c%-5s %s <- %s%s"`. Other: `"%c%-5s %s%s"`. `%c` is `-` when muted and not done.
- The trailing `%s` of a counted lane: `" %d/%u"` (pass, count), `" waits"`, or `" done"` (`seq.c:1374-1391`).
- Inputs: `" %-5s %s, not heard yet"` or `" %-5s %s %3u .%u.%u"` (value, last two octets of the sender).
- Then `"%d %s sw%d%s"` (bpm, key, swing, `" clk"`), `"to: %s"` (or `"nowhere - try: send ble on"`), and `"%u events dropped - the transport is behind"` when non-zero. Reply `"%d lane%s %s"` (`playing` / `stopped`).

## 7. What each binding emits (`fire_event`, `seq.c:462-545`)

| Binding | digit *d* | `x` | routed, source value *v* (0–127) | publishes |
|---|---|---|---|---|
| drum | note N, velocity `(d·127+4)/9`, min 1 | velocity = lane level (100 unless `:vel`) | velocity *v* (1–127) | the velocity |
| voice | degree *d* at the lane octave, velocity = lane level | degree 0 (root) | its own first event's degree, velocity *v* | the velocity |
| cc | value `d·127/9` | **nothing sent (holds)** | value `v & 0x7F` | the value |
| picture | amount *d* | amount 9 | amount `(v·9+63)/127` | `amount·127/9` |
| picture `:x`/`:y` | position *d* (0 = left/top, 9 = right/bottom) | 9 | `(v·9+63)/127` | `amount·127/9` |
| sound `:vel` | parent level `(d·127+4)/9` | parent level 100 | from `(v·9+63)/127` | `(d or 9)·127/9` |
| sound `:oct` | parent octave `min(d, 8)` | the name's octave | from `(v·9+63)/127` | `(d or 9)·127/9` |

Digit tables [sim]:

| digit | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---|---|---|---|---|---|---|---|---|---|
| drum velocity (`seq.c:418-425`) | 1 | 14 | 28 | 42 | 56 | 71 | 85 | 99 | 113 | 127 |
| cc value (`seq.c:521-522`) | 0 | 14 | 28 | 42 | 56 | 70 | 84 | 98 | 112 | 127 |

### 7.1 Drum

Note-on `0x90|ch`, note N; velocity above; `0` is the quietest hit, never silence (`seq.c:410-425`, `532-543`) [sim: `9...x...0...5...` → 127, 100, 1, 71].

### 7.2 Voice — degrees in the key

- Note = `seq_degree_note(root, mode, digit, octave)` = `12·(octave + 1 + ⌊deg/n⌋) + root + iv[deg mod n]`, clamped 0–127 (`seq_scale.h:105-119`). Degrees past the mode climb octaves; C4 = 60 (`seq.h:255-256`). The key is resolved at note-on, so `>scale` transposes on the next note (`seq.h:239-249`; `seq.c:266-269`, `532-534`).
- Velocity is the lane's level (100, or `:vel`), never the digit (`seq.c:527-541`).
- `dmin`, octave 2, `0123456789` → 38 40 41 43 45 46 48 50 52 53; `x` → 38 [sim]. High octaves clamp (`bmin` octave 8 → …126 127 127 127 127 127) [sim].
- **`>scale` spec** (`seq_scale.h:59-103`; `builtins.c:901-912`): a root letter (either case), then `#` (sharp) or `b` (flat, unless `b` starts a mode: `cblues` = C blues, `bb` = B♭ minor), then a mode — none means minor. Modes (`seq_scale.h:32-44`): `chrom` 12 · `blues` 0 3 5 6 7 10 · `pent` 0 3 5 7 10 (minor pentatonic) · `maj5` 0 2 4 7 9 · `maj` · `min` · `dor` · `phr` · `lyd` · `mix` · `loc`. Mode names are case-sensitive and matched **by prefix** (§12 #5). Refusal prints `"a root a-g, then # or b, then one of:"`, `"  maj min dor phr lyd mix loc"`, `"  pent maj5 blues chrom"`, `"e.g. dmin  c  f#mix  apent  ebblues"`; success `"key of %s"`. Default `cmin` (`seq.c:247-249`); the boot document sets `dmin`.

### 7.3 Controller

Control change `0xB0|ch`, number N, value above; `x` sends nothing, so the controller holds (`seq.c:509-525`) [sim: `0x3x6x9x` → 0, 42, 84, 127, nothing between].

### 7.4 Picture

Amount clamped 0–9; a lane calls the draw hook `(prim, amount, dir, tick)`, a part the parameter hook `(prim, param, amount)`; the hooks only record (`seq.c:485-507`; `main.c:536-537`). Direction = the step's, else the lane's (`seq.c:495`). Position part: `x = amount·(w−1)/9`, `y = amount·(h−1)/9` in cells (`viz.c:1031-1036`) [sim for amounts].

### 7.5 Parts of a sound (`seq.c:467-484`)

- A part plays nothing; it sets its parent's level or octave "from here on", then publishes.
- The parent is looked up at fire time: the name before the last `:` (`bass:2:vel` → `bass:2`) (`seq.c:441-460`); no parent → nothing set.
- **Within a route rank, parts fire before lanes** (`seq.c:575-596`), so `>bass:vel 9...` and `>bass 0...` agree on step one [sim: 127 then 42; `:oct 4...x...` → 62 then 38]. A *routed* part is a rank higher than an unrouted parent and fires after it (§12 #8) [sim].
- Forgetting a part resets its parent (§6.4) [sim].

### 7.6 Gates, ties, note-offs

- `gate_us = gate_ms·1000 + hold · period · 24 · rden / (div · rnum)`, capped at 60 s; the hold is converted at the tempo at note-on (`seq.c:427-439`) [sim: 124 bpm, gate 180, `0__.` → off 84 ticks after on].
- Note-offs are scheduled, never paired by the player: a 64-entry table (`SEQ_MAX_LANES × 4`), serviced at the start of every tick (1-tick resolution); full table → the off is sent at once (`seq.c:271-347`).
- An off is keyed only by channel and note: **it fires on time even if the same pitch has been struck again**, cutting the newer note short [sim: `pad` gate 420, `0.0.` at 120 bpm → the second note sounds 33 ticks ≈ 172 ms].
- Stop and panic send every pending off at once (§6.6).

### 7.7 Transport (brief)

Events go to a 64-deep queue drained by a task that hands each to every enabled destination, then flushes; a full queue drops the oldest non-clock event and counts it (`seq.c:48-62`, `183-241`, `285-315`, `1060`).

## 8. Counts and cues (`seq.c:547-564`, `611-661`; `seq.h:191-196`)

- `!n` sets the count, 1–255.
- **Unrouted counted lane.** After compile, `>play`, a route change or an ensemble count move it is *idle*: it waits for its own downbeat — a global slot index that is a multiple of its slot count, measured from play — and records that as its origin (`seq.c:633-641`) [sim: `xxxx !1` typed at tick 123 starts at 192]. Passes count from the origin; alternation uses the local pass (§3.5). On the downbeat pass *n*+1 would take, it becomes idle, `done` and muted, plays nothing on that slot, and publishes `name:end` (`seq.c:645-660`) [sim: `x.x.x.x. !2` → 8 hits; end at tick 384].
- **`name:end`** sets the trigger, value 127, on every lane routed from exactly `name:end` (`seq.c:553-564`).
- **Cue** = routed *and* counted: every trigger from its source (re)starts it at its next slot start, origin reset — a trigger mid-pass restarts it [sim]; after *n* passes it goes idle (not muted, not done), publishes `name:end`, and waits (`seq.c:624-632`, `653-658`) [sim: `>route snare tom:end` with `..x. !4` → 4 snares after the tom].
- Routed without a count = sidechain (§9).
- Re-arming: `>play` (done cleared, unmuted); re-running the line; any `>route` on it (`seq.c:1430-1432`); a follower's count move re-arms unrouted, unfinished ones (`seq.c:873-880`) [sim for play].
- `>lanes` shows `k/n`, `waits`, `done` (§6.7). While waiting or done there is no playhead (`seq.c:1356-1369`).

## 9. Routes — `>route <lane> <source>` (`builtins.c:1598-1699`; `seq.c:1414-1449`, `386-408`, `1144-1179`, `570-610`)

- No argument prints `"route <lane> <lane it follows>"`, `"route disc kick"`, `"route grow disc   viz drives viz"`, `"route disc:x bass a part follows"`, `"route disc       unroutes"`; reply `"route disc kick"`.
- The first word is the lane; **the whole rest of the line is the source** (`two_words`, `builtins.c:1476-1487`). Both are parsed as addresses and made canonical (`builtins.c:1630-1644`).
- Missing lane: `binding_of` must succeed (defined name or picture, not an input), `seq_lane_bind` creates it (`builtins.c:1645-1658`); `seq_route` gives an unplayed routed lane a one-step `x` pattern: text blank, `src` 0, unmuted (`seq.c:1435-1447`) [sim].
- Self (same canonical name) → `"%s cannot follow itself"` (`seq.c:1420-1427`; `builtins.c:1660-1663`). Longer loops are not refused; ranks stop counting at 16 hops (`seq.c:1159`).
- Lane still missing after binding (table full) → `"no lane '%.18s' to drive"` + `"write it first, then route it"` (`builtins.c:1664-1671`).
- Warnings, status still DONE (`builtins.c:1672-1695`): `name:end` with no lane `name` → `"no lane '%s' yet - it will"` + `"stay silent until there is one"`; lane without a count → `"%s never ends - give it !n"`; any other missing source that is not an input → `"nothing called %s yet -"` + `"silent until a lane or input is"`. Reply `"%s <- %s"` or `"%s unrouted"`.
- `seq_route` sets the route, clears the trigger, sets `idle`, reranks (`seq.c:1428-1433`).
- **Sidechain** (routed, no count): fires when triggered and only then — its own timing, odds and alternation ignored; it plays its **first event** (`ev[0]`) for degree, hold and direction; a pattern with no hits never fires (`seq.c:591`, `597-610`) [sim: `.5.3` routed from a kick plays degree 5 each kick; `....` never fires; `x%0` fires every kick].
- **Trigger and value**: whatever a lane emits it publishes to every lane routed from its exact name — note → velocity; cc → value sent; picture/part → `amount·127/9`; `name:end` → 127; input → its value (`seq.c:392-408`, `482`, `506`, `524`, `543`, `561`, `805-816`). The receiver uses it per §7 [sim: kick 127/42/100/1 → rim 127/42/100/1; kick 127 → picture 9, kick 100 → 7].
- **Order** — rank = route hops from a lane that follows nothing (`:end` stripped; a source that is not a lane ends the count) (`seq.c:1144-1179`). Each tick runs `2 × (maxrank+1)` passes: rank *r* parts, then rank *r* lanes (`seq.c:586-596`). A routed lane fires on the same tick as its source [sim: kick → disc → grow ranks 0, 1, 2].
- **Unroute** (`>route x`): route cleared; the lane plays its own pattern again — for a lane `>route` created, that is `x` on every sixteenth (§12 #3) [sim].
- Removing a source leaves followers routed (§6.4).

## 10. Inputs and OSC (`seq.c:733-837`; `seq.h:392-427`; `osc.c:172-218`; `osc_parse.h`; `builtins.c:1986-2055`)

- Table of 16 (`SEQ_MAX_INPUTS`); each holds value 0–127, the sender's IPv4, a set count.
- `seq_input_set` (any task): unknown name → false; count and sender recorded; **pad with 0 = release, nothing fires**; otherwise value clamped ≤ 127 and marked pending (`seq.c:781-795`).
- The clock, while running, before any lane: a pending **knob** publishes on this tick; a pending **pad** only on a tick that starts a (swung) sixteenth (`seq.c:818-837`) [sim: set at tick 27 → knob at 27, pad at 32 with swing 67].
- Several sets before one publish → only the last value. **Every set republishes**, even an unchanged value [sim]. Pending values survive `>stop`: a pad pressed while stopped fires on the first tick after `>play` [sim].
- Publishing = the trigger to lanes routed from that exact name (`seq.c:805-816`). A knob routed to a drum strikes it once per set [sim].
- **OSC in**: `>osc in <port>` (1–65535) or `>osc in off`; errors `"osc in <port> | off"`, `"osc in needs wifi"`, `"could not listen"`; replies `"osc in on %ld"` / `"osc in off"`; listening turns Wi-Fi power save off (`builtins.c:2009-2041`).
- UDP, 512-byte buffer (`osc.c:204`); a malformed datagram is refused whole (`osc_parse.h:190-203`). Address must be `/deck/<name>` with a non-empty name; everything after `/deck/` is the name (`osc.c:189-199`). Addresses under 48 chars (`osc_parse.h:25`, `74`); bundles to depth 4 (`osc_parse.h:26`, `148-151`).
- **Value** (`osc_parse.h:205-220`): no numeric argument → 127; else the last numeric argument; a float/double in [0, 1] is scaled by 127; anything else clamped 0–127; rounded half up. Numeric tags: `i h f d T F` (`T` = 1, `F` = 0) (`osc_parse.h:91-139`) [sim: `f 0.73` → 93, `i 74 i 93` → 93, `i 1` → 1, `T` → 1, `F` → 0 = pad release].

## 11. Limits

| Limit | Value | Where | When exceeded |
|---|---|---|---|
| Lanes | 16 | `seq.h:39` | `"%d lanes is all there is."` / `"free one: type its name alone"` |
| Slots per cycle | 64 | `seq_pattern.h:70`, `seq.h:51` | `"needs %d slots, %d fit"` |
| Bracket depth | 4 | `seq_pattern.h:71` | `"brackets go %d deep at most"` |
| Leaves (hits, rests, ties, every member and alternative) | 160 | `seq_pattern.h:72` | `"over %d steps"` |
| Alternation period | 240 cycles | `seq_pattern.h:73` | `"repeats in %d bars: %d max"` |
| Notes one tie holds | 16 | `seq_pattern.h:74` | silently not held |
| Rate | `/1`–`/32`, `*1`–`*32`, one only | `seq_pattern.h:75`, `230-236` | `"a rate is /1-/%d or *1-*%d"` / `"one rate and one count only"` |
| Count | `!1`–`!255`, one only | `seq_pattern.h:76` | `"!n is 1-%d times"` |
| Odds | `%0`–`%100`, 1–3 digits, one per step | `seq_pattern.h:158-175` | `"%% takes 0-100: x%%25"` |
| Shortest slot | 1 tick: `div·rnum ≤ 24·rden` (swing not counted) | `seq_pattern.h:680` | `"too fine for the clock"` |
| lcm | saturates at 100000 | `seq_pattern.h:190` | reported as that number |
| Hits per lane (all alternatives) | 96 | `seq.h:123` | `"%d notes: %d fit a lane"` |
| Pattern text kept | 99 chars | `seq.h:127` | cut in `>lanes` |
| Command line | 127 chars | `editor.c:1040`; `builtins.c:113`; `main.c:307` | rest ignored (editor) or run as another line |
| Name | 8 letters/digits, starts with a letter | `lane_name.h:50` | `"a name is %d letters at most"` |
| Part | 5 lowercase letters | `lane_name.h:51` | `"a part is a word: disc:x"` |
| Instance | 1–99 (`:1` = plain) | `lane_name.h:52` | `"a second one is :2 to :%d"` |
| Lane address | 17 chars + NUL (`LANE_NAME_MAX` 18 ≤ `SEQ_NAME_MAX` 20) | `lane_name.h:54`; `seq.h:55`; `builtins.c:474` | — |
| Defined names (sounds, aliases, inputs) | 32 | `builtins.c:471` | `"%d names is all there is"` |
| Inputs | 16 | `seq.h:411` | `"%d inputs is all there is"` |
| Note / cc number | 0–127 | `lane_name.h:235` | `"note takes 0-127"` / `"cc takes 0-127"` |
| Voice octave | 0–8 (`:oct` 9 → 8) | `lane_name.h:235`; `seq.c:479` | `"voice takes an octave 0-8"` |
| Channel | 1–16 | `lane_name.h:244` | `"ch is 1-16"` |
| Gate | 1–5000 ms | `lane_name.h:249` | `"gate is 1-5000 ms"` |
| Note length incl. ties | 60 s | `seq.c:438` | clamped |
| Pending note-offs | 64 | `seq.c:281` | off sent at once |
| Event queue / drain burst | 64 / 32 | `seq.c:1060`, `187` | oldest non-clock event dropped, counted |
| Destinations | 8 | `seq.h:471` | `ESP_ERR_NO_MEM` |
| Tempo | 20–300 bpm | `builtins.c:2136`; `seq.c:1484-1485` | `"bpm is a number, 20-300"` |
| Swing | 50–75 % = 0–12 ticks (internal clamps 22 and 23) | `builtins.c:936`; `seq.c:357-363`, `1455-1456`; `seq_pattern.h:732-733` | `"swing is 50-75: 67 is triplet"` |
| PPQN / ticks per step / MIDI clock divisor | 96 / 24 / 4 | `seq.h:75-77` | — |
| Route rank | ≤ 17 (hop count stops at 16) | `seq.c:1159` | — |
| Key text kept | 11 chars | `seq.c:249` | cut |
| Error text | ≤ 30 columns | `seq_pattern.h:809-812` | — |
| Stage hand-off wait | 40 RTOS ticks | `seq.c:1232` | applied directly |
| Follower wait after play | 1 s | `seq.c:1549-1550` | plays alone |
| OSC address / datagram / bundle depth | < 48 chars / 512 B / 4 | `osc_parse.h:25-26`; `osc.c:204` | refused |
| OSC in port | 1–65535 | `builtins.c:2016` | `"osc in <port> \| off"` |

## How this page was checked

The pure headers (`seq_pattern.h`, `seq_scale.h`, `lane_name.h`, `osc_parse.h`) were compiled
unmodified on a laptop, and `seq.c` was compiled verbatim with small ESP-IDF and FreeRTOS
shims, its tick driven by hand; those runs are marked **[sim]**. The harness is not in the
repository. `builtins.c` was read, not run. Nothing on this page was observed on the deck
unless it says so.
