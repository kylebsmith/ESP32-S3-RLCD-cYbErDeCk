# Keychron K9 Pro — the chosen keyboard, pinned down

*The keyboard is decided (`envelope.md` brief, decision 1). This document exists
to answer one question the user asked — "is it the same footprint as the K7
Pro?" — and to hand the enclosure every number it needs to cut a bay.*

**Scope.** Enclosure only. No firmware is proposed, read or touched.

**Source tags.** `[VENDOR]` Keychron's own product page · `[MANUAL]` Keychron's
own user-manual PDF · `[FIRMWARE]` Keychron's own QMK fork (geometry only —
the layout JSON is a dimensioned drawing that happens to be code) ·
`[VENDOR-CAD]` Keychron's own hardware-design repository · `[DS]` component
datasheet · `[DERIVED]` arithmetic, shown · `[3P]` third party · **NOT SOURCED**
listed in §13 rather than guessed.

---

## 0. The answer, in one line

**Not the same footprint. The K9 Pro is exactly one key column narrower than the
K7 Pro — 19.00 mm — and identical in depth to the millimetre. 287 × 97.5 vs
306 × 97.5 mm. Same chassis family, one column deleted.**

Everything else follows from that, and the second finding is almost as good:
**Keychron publishes official factory CAD for the K9 Pro** — top case, bottom
case, plate, keycaps, stabilisers and a full assembled model. §12 has the exact
URLs. This environment cannot reach the host, but a browser can.

---

## 1. The footprint question, settled two independent ways

### 1.1 Published outlines

| | Width | Depth | Source |
|---|---|---|---|
| K9 Pro | **287 mm** (11.29″) | **97.5 mm** (3.83″) | `[VENDOR]` keychron.com, keychron.de, keychron.be, keychron.com.es — identical on all four |
| K7 Pro | **306 mm** (12.04″) | **97.5 mm** (3.83″) | `[VENDOR]` keychron.com |
| **Δ** | **−19.00 mm** | **0.00 mm** | `[DERIVED]` |

### 1.2 The proof: switch field plus bezel

Keychron's own QMK fork carries the exact key geometry for both boards, in key
units. `[FIRMWARE]`

```
K9 Pro  keyboards/keychron/k9_pro/ansi/info.json   LAYOUT_61_ansi  61 keys
        rows 0..4, x spans 0 -> 15.00      =>  15u x 5u
K7 Pro  keyboards/keychron/k7_pro/info.json        LAYOUT_69_iso   69 keys
        rows 0..4, x spans 0 -> 16.00      =>  16u x 5u
```

At the MX standard 19.05 mm pitch:

```
K9 Pro field = 15 x 19.05 = 285.75 mm   x   5 x 19.05 = 95.25 mm
K7 Pro field = 16 x 19.05 = 304.80 mm   x   5 x 19.05 = 95.25 mm

K9 Pro bezel = (287.0 - 285.75)/2 = 0.625 mm per side (W)
                (97.5 -  95.25)/2 = 1.125 mm per side (D)
K7 Pro bezel = (306.0 - 304.80)/2 = 0.600 mm per side (W)
                (97.5 -  95.25)/2 = 1.125 mm per side (D)
```

**The two boards have the same bezel to within 0.025 mm in width and exactly the
same bezel in depth.** They are the same chassis with one column added or
removed. The published 19.00 mm width difference is Keychron rounding one
19.05 mm key unit.

This also upgrades the pitch from `[derived]` in `keyboard.md` §4.1 to
**confirmed**: 19.05 mm, from the vendor's own layout definition.

> **Enclosure consequence of the bezel number.** The K9 Pro offers only
> **0.625 mm of frame per side in width** outside the switch field. v1's bay
> retains its keyboard with a lip (`kbd_lip_x = 1.00`, `parameters.scad:1014`).
> A 1.00 mm lip on the long edges has nowhere to land here. Retention has to
> come from the depth axis (1.125 mm per side), from the ends, or from below.
> *(Qualifier: the LSA keycap plan size is NOT SOURCED — §13 — so how far the
> outermost caps overhang the field edge is not established.)*

---

## 2. Outside dimensions — all four heights

Keychron is still the only vendor in this study that publishes heights both with
and without keycaps, and for the K9 Pro it publishes all four. `[VENDOR]`

| | K9 Pro | Source |
|---|---|---|
| Dimension | **287 × 97.5 mm** (11.29 × 3.83″) | `[VENDOR]` |
| Height **without** keycap, **front** | **7.6 mm** (0.29″) | `[VENDOR]` |
| Height **without** keycap, **rear** | **13.3 mm** (0.52″) | `[VENDOR]` |
| Height **incl.** keycap, **front** | **19.8 mm** (0.77″) | `[VENDOR]` |
| Height **incl.** keycap, **rear** | **23.5 mm** (0.92″) | `[VENDOR]` |
| Typing angle | **3.4°, 5°, 8°** | `[VENDOR]` |
| Weight | **410 g / 0.90 lbs** | `[VENDOR]` |
| Number of keys | **61 (60%)** | `[VENDOR]`, `[MANUAL]` p9, `[FIRMWARE]` |

Keycap protrusion above the frame: **12.2 mm at the front, 10.2 mm at the rear**
`[DERIVED]` (19.8 − 7.6; 23.5 − 13.3). The front and rear figures differ by
2.0 mm because LSA is a *sculpted* profile — the rows are not the same height
(§5). This is exactly the pair of numbers a bay needs, and it is available
without owning the keyboard.

### 2.1 A data-integrity warning you must not skip

**Keychron's own US page (keychron.com) currently publishes two WRONG rows for
this board.** Its spec block reads:

```
Size          Dimension: 287 x 97.5 mm  |  Dimension (Aluminum frame): 316.6 x 126.91 mm
Front Height  Height without keycap (front): 7.6 mm   |  Height incl. keycap: 32 mm
Back Height   Height without keycap (rear): 13.3 mm   |  Height incl. keycap: 42.07 mm
```

The right-hand column is impossible for this keyboard. A 32 mm front height on a
board whose own frame is 7.6 mm at the front implies a 24.4 mm keycap; LSA is
~4.4 mm (§5). Those figures closely match Keychron's **K2** — a normal-profile
75% board the company publishes at *317 × 129 mm (aluminium frame), 30 mm front
and 40 mm rear with keycaps* `[VENDOR]` keychron.de. It is a template artefact
on the US store.

**The correct block survives on Keychron's own regional stores** — keychron.de,
keychron.be and keychron.com.es all still carry the full uncontaminated
`SPECIFICATION / CONNECTIVITY AND POWER / PHYSICAL UNIT` block quoted in §2, and
a third party ([green-keys](https://green-keys.info/en/keyboard/keychron-k9-pro/))
reproduces the same four heights independently. Three vendor stores plus one
third party against one broken store page: **use 19.8 / 23.5.**

If you ever re-check these numbers, re-check them on a regional store, not
keychron.com.

---

## 3. The wedge — and a correction to `keyboard.md` §5.2

### 3.1 The arithmetic

```
case rise, front to rear = 13.3 - 7.6 = 5.70 mm
over the published depth =              97.50 mm
angle = atan(5.70 / 97.50)            = 3.346 deg
```

**Keychron's own lowest stated typing angle is 3.4°.** `[VENDOR]` The derived
case slope and the vendor's stated minimum agree to **0.05°**.

### 3.2 Why that agreement matters more than the angle itself

The lowest typing angle is what the board gives with no feet deployed — resting
on its own bottom. If the derived top-surface slope equals that angle measured
over the full outline depth, then **the entire 5.70 mm of wedge lives in the top
surface and the bottom is flat.** A flat bay floor seats this keyboard.

That is a *correction* to `keyboard.md` §5.2, which says of this class "a flat
pocket floor will not seat them — the floor needs a matching ramp." For the
K9 Pro, on the evidence above, the ramp belongs on the **deck's top face around
the bay**, not on the bay floor.

**Honest limit:** this is consistent with a flat bottom, it does not prove one.
The bottom-surface profile is in §13 and is settled by one look at
`K9-Pro-Top-And-Bottom-Case.stp` (§12).

### 3.3 The same arithmetic on the K7 Pro does *not* close

```
K7 Pro: atan((13.2 - 8.8) / 97.5) = 2.584 deg   vs stated minimum 3.37 deg
        the stated 3.37 deg implies a run of 4.4/tan(3.37) = 74.72 mm,
        not the published 97.5 mm depth
```

The K7 Pro's published figures are internally inconsistent — its two numbers are
measured over different baselines, and Keychron does not say which. The K9 Pro's
are self-consistent. **On the one geometric property a bay most depends on, the
K9 Pro's vendor data is the better data.** `[DERIVED]`

### 3.4 Keycap-plane slope is not the case slope

```
K9 Pro keycap plane: atan((23.5 - 19.8)/97.5) = 2.173 deg
K7 Pro keycap plane: atan((23.2 - 18.2)/97.5) = 2.936 deg
```

Neither equals its own case slope, because LSA's row sculpting changes cap
height per row and because the front/rear height datums are the case's extreme
edges, not the outermost key rows. **Do not use the keycap-plane slope to set a
ramp.** Use 3.346° from the case, and treat the aperture as a flat opening.

### 3.5 Bay geometry, assembled

Applying the repo's own fit rule (`kbd_pocket_w = keyboard_w + 1.35`,
`kbd_pocket_h = keyboard_d + 0.95`, `parameters.scad:748-749`):

| | mm | Note |
|---|---|---|
| Bay plan size | **288.35 × 98.45** | `[DERIVED]` from the v1 rule |
| Bay floor | **flat** | `[DERIVED]` §3.2, pending §13 |
| Frame top above floor | **7.6 front / 13.3 rear** | `[VENDOR]` |
| Keycap tops above floor | **19.8 front / 23.5 rear** | `[VENDOR]` |
| Deck face flush all round | **ramp 3.346° over 97.5 mm** | `[DERIVED]` |
| Caps proud of a flush-ramped deck | **12.2 front / 10.2 rear** | `[DERIVED]` |

The three ways to resolve the deck's top face, costed:

- **Deck at 13.3 mm (flush with the rear frame).** The front of the keyboard
  sits 5.7 mm below the deck surface — a 5.7 mm wall right where the heels of
  the hands land. Rejected on ergonomics.
- **Deck at 7.6 mm (flush with the front frame).** The rear of the frame stands
  5.7 mm proud, rising toward the control band. Cheap, and the step faces away
  from the hands.
- **Deck ramped 3.346°.** Flush all round. This is the "perfect lines, insane
  symmetry" answer in the brief, and it costs one 3.35° ramp across 97.5 mm of
  the deck's top face. No internal volume, no bay-floor complexity.

---

## 4. Layout and key count — ANSI, and the number row is on the base layer

| | | Source |
|---|---|---|
| Layout | **60%, ANSI** | `[VENDOR]` "Layout: 60%, ANSI" |
| Keys | **61** | `[VENDOR]`, `[MANUAL]` p9, `[FIRMWARE]` `LAYOUT_61_ansi` |
| Grid | **15u × 5 rows** | `[FIRMWARE]` |
| Pitch | **19.05 mm** | `[FIRMWARE]` + §1.2 reconciliation |
| Multimedia keys | 12 | `[VENDOR]` |

**ANSI is not a variant you have to be careful to order — it is the only one
Keychron sells.** All six K9 Pro SKUs are ANSI (§9). By contrast the K7 Pro is
sold in ANSI, ISO, ISO-DE, ISO-Nordic, ISO-UK and JIS `[3P]` keeb-finder, so the
K7 Pro carries a real purchasing hazard against `keymap.c`'s US-ANSI table that
**the K9 Pro simply does not have.** That is a quiet but genuine win for this
choice.

### 4.1 Number row, confirmed from the vendor's own keymap

`[FIRMWARE]` `k9_pro/ansi/rgb/keymaps/default/keymap.c`, base layer, row 0:

```
KC_ESC, KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0,
KC_MINS, KC_EQL, KC_BSPC
```

Digits are unshifted base keys. So are `[ ] \ ; ' , . /` and `- =`. Every
character `docs/VERBS.md` requires is base-or-shift — no AltGr, no layer dive
for digits. The discriminator `keyboard.md` §0 set is satisfied.

### 4.2 Bottom-row convention — the standard ANSI 60%

From `[FIRMWARE]`, row 4 widths in key units:

```
1.25 | 1.25 | 1.25 | 6.25 (space) | 1.25 | 1.25 | 1.25 | 1.25     = 15.00u
```

That is the **classic GH60 / Poker 6.25u-spacebar ANSI 60% bottom row** — the
single most widely supported 60% convention there is. Aftermarket LSA bottom-row
caps, were any to exist, would fit. The other rows: 1.5u Tab + 1.5u Backslash,
1.75u Caps + 2.25u Enter, 2.25u LShift + 2.75u RShift, 2u Backspace. All
standard.

### 4.3 What is *not* there

**No dedicated arrow keys and no function row.** Row 4 is
`Ctrl / Opt / Cmd / Space / Cmd / Fn / Fn1 / Ctrl` — eight keys, no arrow
cluster `[FIRMWARE]`, `[MANUAL]` p11. Arrows live on the Fn layer
(`KC_UP/LEFT/DOWN/RIGHT`), F1–F12 on the Fn1 layer. This is stated as a layout
fact, not a firmware question: it is the price of the 19 mm, and the user has
already accepted a 60%. Whether the deck's language wants arrows is out of scope
here.

---

## 5. Switch and keycap stack

### 5.1 The switch — Gateron KS-33 Low Profile 2.0, Brown

| Property | Value | Source |
|---|---|---|
| Family | Gateron Low Profile 2.0, **KS-33** | `[VENDOR]` |
| **Total height** | **12.2 mm** | `[VENDOR]` "The new Gateron KS-33 Low Profile 2.0 Switch is 12.2 mm in height" |
| Pre-travel (brown) | **1.70 mm** | `[3P]` kbd.news switch db |
| **Total travel** (brown) | **3.0 mm** | `[3P]` kbd.news switch db |
| Actuation force (brown) | **55 g**, two-stage spring | `[3P]` kbd.news switch db |
| Stem | POM, **MX stem** | `[VENDOR]` |
| Pins | 3-pin | `[VENDOR]` |
| Housing | PC upper (transparent), Nylon PA66 base | `[VENDOR]` |

> **Conflict, stated:** kbd.news gives the housings as POK top / POM bottom;
> Keychron gives PC top / PA66 base. Immaterial to the enclosure; recorded
> because the sources disagree.

> **Conflict, stated:** the older KS-33 (non-2.0) is listed at 3.20 mm travel /
> 50 g. `keyboard.md` §4.3 currently records the family as "1.7 / 3.0 / 45 g
> act." — the 45 g is wrong for brown; brown is **55 g**.

### 5.2 Against Cherry MX — and why "31% slimmer" does not survive arithmetic

`parts.md` §4 has Cherry's own figures `[DS cherry-mx]`:

```
Cherry MX: 11.6 mm PCB face -> top of stem
         +  5.0 mm pins below the PCB
         = 16.6 mm total
Gateron KS-33 LP 2.0: 12.2 mm total          [VENDOR]
shorter by 4.4 mm = 26.5% of the MX total    [DERIVED]
```

**Do not compare 12.2 against 11.6.** Cherry's 11.6 excludes the pins; Gateron's
12.2 is a bare "height" with no stated datum. On the only like-for-like
comparison available — total switch height — the low-profile switch is **4.4 mm
and 26.5% shorter**, not the 31% Keychron's marketing copy claims `[VENDOR]`.
The 31% most likely refers to the older KS-27, which Keychron's own switch page
says the KS-33 is *"slightly higher"* than. Whether Gateron's 12.2 mm includes
its pins is **NOT SOURCED** (§13).

Travel is the bigger difference, and it is the one the hands feel:
**3.0 mm against Cherry's 4.0 mm, actuating at 1.70 mm against Cherry's 2.0.**

### 5.3 The keycaps — LSA

| Property | Value | Source |
|---|---|---|
| Profile | **LSA** — Low-profile Spherical Angled, Keychron proprietary | `[VENDOR]` |
| **Overall height** | **≈ 4.4 mm** | `[VENDOR-CAD]` repo `Keycap Profiles/LSA Profile/README.md` |
| Material | double-shot PBT | `[VENDOR]` |
| Wall | 1.4–1.6 mm | `[VENDOR-CAD]` |
| Top | spherical (dished both axes) | `[VENDOR-CAD]` |
| Sculpting | **angled rows — each row a different tilt** | `[VENDOR-CAD]` |
| Stem | low-profile MX — **not** standard MX | `[VENDOR-CAD]`, `[VENDOR]` |

Keychron's own comparison puts LSA at ~4.4 mm against Cherry's ~9.4 mm and
OEM's ~11.9 mm `[VENDOR-CAD]`. The row sculpting is what makes the front and
rear keycap protrusions differ by 2.0 mm (§2), and it is why §3.4 says not to
derive a ramp from the keycap plane.

**Ecosystem warning:** LSA is Keychron-only and the stem is not MX-compatible.
There are no third-party LSA sets `[VENDOR-CAD]`, and Keychron states the board
"is currently compatible with Gateron low-profile mechanical switches only"
`[VENDOR]`. If a cap or switch is ever lost, Keychron is the only supplier.

---

## 6. Radio — BLE 5.1, and the ESP32-S3 question answered

| | | Source |
|---|---|---|
| **Bluetooth version** | **5.1**, Broadcom | `[VENDOR]` "stable Broadcom Bluetooth 5.1" |
| Bluetooth device name | `Keychron K9 Pro` | `[VENDOR]`, `[MANUAL]` p9 |
| **Paired device slots** | **3** | `[VENDOR]`; `[FIRMWARE]` `BT_HST1/2/3` on Fn+Q/W/E |
| Range | within 10 m, open space | `[MANUAL]` p9 |
| Polling, wireless | 90 Hz (≈11 ms) | `[VENDOR]` |
| Polling, wired | 1000 Hz | `[VENDOR]` |
| NKRO | yes, both modes | `[VENDOR]` |
| Mode toggle | **BT / OFF / Cable** — three positions | `[MANUAL]` p10 ③ |

**Bluetooth 5.1 is a Low Energy specification. The board is BLE; it is not
Classic-only; the ESP32-S3 can host it.** This is the same radio and the same
HOGP path `STATUS.md` already records working with a BLE keyboard, and the 90 Hz
(~11 ms) interval sits inside the 7.5–15 ms window `docs/OS.md` designs for.

### 6.1 No trackpad — and the one honest caveat

The board is 61 keys with no pointing device of any kind `[MANUAL]` p10, p11;
`[FIRMWARE]`. Neither default keymap (RGB or White) contains a single mouse
keycode — zero occurrences of `KC_MS*`/`MS_BTN*` `[FIRMWARE]`. So no pointer
reports are emitted by the shipping firmware, and the host's "any notification
≥ 3 bytes is a keyboard report" decoder is not at risk.

**Caveat, for completeness:** `info.json` has `"mousekey": true`, so QMK's mouse
feature is compiled in. A *remapped* key could be bound to a mouse keycode in
VIA/Launcher. Nothing does so out of the box. Don't bind one.

### 6.2 Three host slots is a real feature, not a spec-sheet line

`keyboard.md` §5.2 praised ZMK's five pinned profiles because a bond can be
pinned to the deck and nothing else can steal it mid-set. The K9 Pro's three
slots on Fn+Q/W/E are the same mechanism, one slot short. Dedicate a slot to the
deck and leave it there.

---

## 7. Weight and power

| | | Source |
|---|---|---|
| Weight | **410 g** (0.90 lb) | `[VENDOR]` |
| Battery | **1550 mAh** Li-polymer, rechargeable, built in | `[VENDOR]`, `[MANUAL]` p9 |
| Runtime, backlight off | **100 h** (DE/US) — but **130 h** on keychron.be | `[VENDOR]`, conflicting |
| Runtime, RGB lowest | **33 h** (DE) — but **82 h** on keychron.be | `[VENDOR]`, conflicting |
| Charge time | **≈ 3 hours** | `[MANUAL]` p9 |
| Charge port | USB Type-C | `[MANUAL]` p9 |
| MCU | STM32L432, Cortex-M4, 128 KB flash | `[VENDOR]`, `[FIRMWARE]` |

**410 g sits in the band `keyboard.md` §7 identified as the sweet spot for a held
instrument, and it is 26 g lighter than the K7 Pro.**

**It runs on its own battery.** It must: the deck cannot host it over USB
(`keyboard.md` §3 closed the USB-host route), so the link is BLE and the
keyboard powers itself. The deck does not need to supply it to work.

**The deck *may* feed it 5 V to charge.** The mode toggle has a dedicated
**Cable** position, which means USB in the **BT** position is charge-only — the
keyboard does not switch to wired HID just because power appears. So a 5 V feed
from the deck into the keyboard's USB-C keeps it topped up while it talks BLE.
The toggle must stay on **BT**; put it on **Cable** and the board stops
advertising and the deck loses its keyboard.

> **Honest limit.** That the BT position makes USB charge-only is the plain
> reading of a three-position BT/OFF/Cable toggle and of Keychron's documented
> "the keyboard can be charged in Bluetooth on/off mode" `[3P]` manual mirror.
> It is **not** stated in so many words in the manual PDF. Verify on a unit
> before designing a charge feed. (§13.)

The **OFF** position is worth noting on its own: it powers the keyboard down
without unpairing. A deck that stores its keyboard wants that.

---

## 8. The aluminium frame — it is a top frame, not a base

The user chose this board partly for the aluminium. Here is what the aluminium
actually is.

**Keychron's own documentation is explicit.** Its "Differences of Low-Profile
Keyboards" page, describing the K series and K Pro series:

> *"The K series and K Pro series are equipped with an **ABS bottom case and
> Aluminum frame**."* `[VENDOR]`

and in the same page's comparison table, for both series: *"Aluminum frame + ABS
bottom case"*. The K9 Pro's own spec row reads **"Frame Material: ABS+Aluminum
frame"** and its build **"ABS+Aluminum frame, tray mount"** `[VENDOR]`.

So:

- The **aluminium is the top frame** — the visible ring around the keys, which
  is also the surface the four published heights are measured to.
- The **bottom case is ABS**. It is not a structural base plate, it is a plastic
  tray.
- The mount is **tray mount**: the PCB screws to bosses in the bottom case.
  `[3P]` keeb-finder, consistent with `[VENDOR]` "tray mount".

### 8.1 Can it be stripped to seat thinner? Not the way you'd hope

This was flagged as "the single biggest potential win on thickness", so it
deserves a straight answer: **the win is smaller than it looks, and the honest
figure is not available without the CAD.**

- The whole case is **only 7.6–13.3 mm thick** `[VENDOR]`. There is not much to
  remove. Even deleting the entire bottom case perfectly would recover at most
  those 13.3 mm, and the PCB, battery and plate have to go somewhere.
- Removing the ABS bottom case removes **the tray the PCB and the 1550 mAh
  battery are screwed into**, and the mount is tray mount, so the bottom case
  *is* the structure. The aluminium top frame is the cosmetic face, not the
  chassis. Stripping this board means designing a replacement tray — i.e.
  integrating, which `keyboard.md` §5.2 costed and set aside.
- `keyboard.md` §5.2 already established there is **no vendor-supported bare-PCB
  SKU**; Keychron "barebone" means case + PCB without switches and caps.
- Teardown evidence specific to the K9 Pro could not be found. Keychron
  publishes disassembly guides for the K8 Pro and K10 Pro but **not for the
  K9 Pro** `[VENDOR]`, and no third-party K9 Pro teardown surfaced. Screw sizes
  from the K10 Pro guide (M3×6 bottom case, KA2.3×8 plate) are **not**
  transferable to a different chassis and are not quoted as if they were.

**The real conclusion:** the bay is already thin. 13.3 mm of case plus 10.2 mm
of keycap at the rear is less stack than the control band needs for an MX switch
and cap (§11.2). **The keyboard is not what sets v2's thickness.** Stripping it
buys little, costs the battery mount and the warranty, and should not be done.

### 8.2 Feet

Three typing angles — **3.4°, 5°, 8°** `[VENDOR]` — so there are two stages of
foot beyond the resting position. A foot is visible on the rear edge in the
manual's overview drawing `[MANUAL]` p4. Their type (flip-out vs stick-on),
size, position and whether they are removable are **NOT SOURCED** (§13) and
matter, because a deployed foot fouls a flat bay floor. The published 7.6/13.3
heights are the un-deployed, feet-retracted condition — that is the condition
§3 derives from and the condition a bay should assume.

### 8.3 Where the ports and switches are — and why it matters for arrangement A

From Keychron's own overview drawing `[MANUAL]` p10, all on the **back (far)
edge** of the keyboard:

| | Feature | Position |
|---|---|---|
| ① | Bluetooth indicator | top-left of the key field |
| ② | OS toggle — Win/Android ↔ Mac/iOS | back edge, left end (above `esc`) |
| ③ | Mode toggle — **BT / OFF / Cable** | back edge, left end (above `1`/`2`) |
| ④ | **USB-C charging port** | back edge, further right |

> **This is a real constraint on arrangement A.** Arrangement A puts the
> keyboard at the **near** edge of the deck, so its **back edge faces into the
> deck**, toward the control band. A bay buries the USB-C port and both toggles
> against the deck's interior wall. Three things have to leave that wall: a
> charge port, a mode toggle that must stay on **BT** but must stay reachable
> to reach **OFF**, and an OS toggle. The spine between the keyboard bay and the
> control band is where they have to come out.

The toggles are positively located above `esc` and the `1`/`2` keys in the plan
view, i.e. within roughly the first two key units of the left end. **The exact
positions along the edge are NOT SOURCED** (§13) — the manual's overview drawing
is stylised and does not hold a consistent scale between its two axes, so
scaling millimetres off it would be inventing precision. The STEP file (§12)
settles all four in one look.

---

## 9. Availability, and the variants

**Still listed, currently sold out.** As of this check (2026-11-05):

- keychron.com shows the product live at **$67.99** (regular $94.99) with
  **all six of its variants marked `"available": false`** in the page's own
  product JSON, the button reading **"Sold out"**, a **"Notify Me When
  Available"** form, and the line *"is backordered and will ship as soon as it
  is back in stock."* `[VENDOR]`
- keychron.de and keychron.be carry the full spec block and list it out of
  stock. `[VENDOR]`
- `[3P]` keeb-finder (checked Nov 2026) shows it out of stock at both Keychron
  HK and Keychron DE.

**It is not discontinued.** Keychron's own hardware-design repository, in a list
headed *"As of April 9, 2026, the Keychron K Pro collection includes the
following K Pro products"*, includes the K9 Pro. `[VENDOR-CAD]`

### 9.1 The six SKUs — and the one that matters

There are exactly six, all ANSI, read from the page's own product JSON
`[VENDOR]`:

| SKU | Backlight | Switch | Colour |
|---|---|---|---|
| `K9P-A1` | White | Gateron LP Mechanical | Red |
| `K9P-A3` | White | Gateron LP Mechanical | Brown |
| `K9P-B1` | RGB | Gateron LP Mechanical | Red |
| `K9P-B3` | RGB | Gateron LP Mechanical | Brown |
| `K9P-H1` | RGB | Gateron LP Mechanical **(Hot-Swappable)** | Red |
| **`K9P-H3`** | **RGB** | **Gateron LP Mechanical (Hot-Swappable)** | **Brown** |

> **Buy `K9P-H3` — RGB Backlight, Hot-Swappable, Brown.** Hot-swap is offered **only**
> on the RGB version — Keychron's own copy: *"users can hot-swap … (for RGB
> Backlight Hot-Swappable Version only)"* `[VENDOR]`. There is no
> White + Hot-Swappable SKU. Since the brief wants tactile brown **and** the
> value of hot-swap is that a switch can be changed without soldering, the RGB
> hot-swappable brown is the only SKU that delivers both.
>
> Only **Red and Brown** ship on this board, despite the switch family also
> offering Blue. No clicky option.

### 9.2 The successor, for the record

A **K9 Max** exists (2025, 426 g, adds 2.4 GHz) `[3P]` keeb-finder. It is **not
a drop-in substitute on the axis the user cares about**: keeb-finder lists its
case material as ABS, against the K9 Pro's ABS + aluminium. It is also listed
thicker (25 mm vs 20 mm in keeb-finder's own rounded figures). Keychron's CAD
repo notes K9 Max files were added 2026-06-17 `[VENDOR-CAD]`. If the K9 Pro
proves unobtainable, the K9 Max is the fallback — but it is a different
keyboard, and the aluminium frame goes away.

---

## 10. K9 Pro vs K7 Pro — the direct comparison

| | **K9 Pro** | **K7 Pro** | Δ (K9 − K7) | Source |
|---|---|---|---|---|
| **Width** | **287 mm** | **306 mm** | **−19.00 mm** | `[VENDOR]` |
| **Depth** | **97.5 mm** | **97.5 mm** | **0.00 mm** | `[VENDOR]` |
| Switch field | 285.75 × 95.25 | 304.80 × 95.25 | −19.05 × 0 | `[FIRMWARE]`+`[DERIVED]` |
| Bezel per side, W / D | 0.625 / 1.125 | 0.600 / 1.125 | +0.025 / 0 | `[DERIVED]` |
| Height, no keycap, front | **7.6 mm** | 8.8 mm | **−1.2 mm** | `[VENDOR]` |
| Height, no keycap, rear | **13.3 mm** | 13.2 mm | +0.1 mm | `[VENDOR]` |
| Height, w/ keycap, front | 19.8 mm | **18.2 mm** | **+1.6 mm** | `[VENDOR]` |
| Height, w/ keycap, rear | 23.5 mm | **23.2 mm** | +0.3 mm | `[VENDOR]` |
| Keycap protrusion, front / rear | 12.2 / 10.2 | 9.4 / 10.0 | +2.8 / +0.2 | `[DERIVED]` |
| **Case wedge (derived)** | **5.70 mm, 3.346°** | 4.40 mm, 2.584° | +1.30 mm, +0.76° | `[DERIVED]` |
| Typing angles | 3.4 / 5 / 8° | 3.37 / 4.4 / 7.39° | — | `[VENDOR]` |
| Derived slope vs stated min | **agree (0.05°)** | **disagree (0.79°)** | — | `[DERIVED]` |
| **Weight** | **410 g** | 436 g | **−26 g** | `[VENDOR]` |
| Keys / layout | **61, 60% ANSI** | 68, 65% ANSI | −7 keys | `[VENDOR]` |
| Grid | 15u × 5 | 16u × 5 | −1u | `[FIRMWARE]` |
| Number row | **yes, base layer** | yes, base layer | — | `[FIRMWARE]` |
| Dedicated arrows | **no** | **yes** | − | `[FIRMWARE]` |
| Layouts Keychron sells | **ANSI only** | ANSI, ISO, ISO-DE, ISO-Nordic, ISO-UK, JIS | — | `[VENDOR]`, `[3P]` |
| Switch | Gateron LP 2.0 (KS-33) | Gateron LP mechanical | same family | `[VENDOR]` |
| Keycaps | LSA double-shot PBT | LSA double-shot PBT | same | `[VENDOR]` |
| Frame | ABS + aluminium, tray mount | ABS + aluminium, tray mount | same | `[VENDOR]` |
| Bluetooth | **5.1**, 3 devices | **5.1**, 3 devices | same | `[VENDOR]` |
| Polling, wireless / wired | 90 Hz / 1000 Hz | 90 Hz / 1000 Hz | same | `[VENDOR]` |
| Battery | 1550 mAh | 1550 mAh | same | `[VENDOR]` |
| MCU | STM32L432 | STM32L432 | same | `[VENDOR]` |
| Hot-swap | RGB version only | RGB version only | same | `[VENDOR]` |
| Price (Keychron US) | $67.99 (reg. $94.99) | $83 `[3P]` | − | `[VENDOR]`, `[3P]` |
| Stock at Keychron | **sold out / backorder** | in stock | − | `[VENDOR]` |
| Official CAD | **yes** (§12) | **yes** (§12) | — | `[VENDOR-CAD]` |

### The user's question, answered without hedging

> *"I think it looks like the same footprint as the K7 but that needs to be
> confirmed."*

**Refuted, precisely.** Same depth — 97.5 mm, identical. **Not** the same width:
the K9 Pro is **19.00 mm narrower**, which is exactly one 19.05 mm key column —
the 65%'s navigation column, deleted. (The key count drops by 7 rather than 5,
because the 65% also carves its arrow cluster out of the bottom two rows:
K7 Pro rows run 15/15/14/14/10, K9 Pro 14/14/13/12/8 `[FIRMWARE]`. The *width*
difference is one column; the *key* difference is one column plus the arrows.)
Same chassis family,
same bezels, same frame, same switches, same caps, same radio, same battery,
same MCU. It is the K7 Pro minus one column, 26 g lighter, 1.2 mm thinner at the
front of the case, and 1.6 mm taller over the front keycaps.

---

## 11. The deck, computed

### 11.1 Arrangement A, by the repo's own identity

`cad/parameters.scad` makes the deck a function of the keyboard, and
`tools/validate.py` asserts it:

```
parameters.scad:748-749   kbd_pocket_w = keyboard_w + 1.35
                          kbd_pocket_h = keyboard_d + 0.95
parameters.scad:904       body_w = wall + max(kbd_pocket_w, board_pocket_w) + wall
parameters.scad:913       body_h = bottom_wall + kbd_pocket_h + spine
                                   + board_pocket_h + wall
with wall = 3.2, spine = 3.2, bottom_wall = wall + 1.2 = 4.4,
     board_pocket_w = board_w_max + 2*board_fit = 92.70 + 1.20 = 93.90
     board_pocket_h = board_h_max + 2*board_fit = 69.10 + 1.20 = 70.30
  =>  body_w = max(kbd_pocket_w, 93.90) + 6.40
      body_h = kbd_pocket_h + 81.10
```

Validated on v1: `109.85 + 6.40 = 116.25` and `59.15 + 81.10 = 140.25` — the v1
deck exactly, and `tools/validate.py:125-126` asserts both every run.

> **Minor correction to `keyboard.md` §5.1**, which restates the board pocket as
> `93.5`. The exact value is **93.90** (`board_w_max = 92.70`,
> `board_fit = 0.60`, `parameters.scad:158, 161`). It changes nothing — the
> keyboard wins the `max()` in every case considered — but it is quoted here
> correctly.

Substituting the K9 Pro:

```
kbd_pocket_w = 287.00 + 1.35 =  288.35
kbd_pocket_h =  97.50 + 0.95 =   98.45

body_w = max(288.35, 93.90) + 6.40 = 288.35 + 6.40 = 294.75
body_h =  98.45 + 81.10                            = 179.55
```

| Deck | W × H (mm) | vs K7 Pro | vs v1 |
|---|---|---|---|
| **K9 Pro** | **294.75 × 179.55** | **−19.00 W, 0.00 H** | +154 % W, +28 % H |
| K7 Pro | 313.75 × 179.55 | — | +170 % W, +28 % H |
| v1 (Rii 518BT) | 116.25 × 140.25 | — | — |

*(`keyboard.md` §10.3 rounds the K7 Pro deck to 313.8 × 179.6; the exact value
is 313.75 × 179.55. Same identity, same inputs.)*

**`display_module_w = 91.00` (`parameters.scad:210`) does not enter.** The width
is `max(288.35, 93.90)`, and the board pocket's 93.90 — itself already wider
than the 91.00 display — loses to the keyboard by **194.45 mm**. **The keyboard
alone sets the deck's width**, and the display never gets near the `max()`. That
is true for the K7 Pro too; the K9 Pro simply makes the deck 19 mm less wide.

### 11.2 The honest caveat: this identity has only two bands

`body_h = bottom_wall + kbd_pocket_h + spine + board_pocket_h + wall` describes
v1's **two** bands — keyboard, then board. **Arrangement A has three**: display
far, controls middle, keyboard near. So 179.55 is the *floor*, not the answer.
The third band adds one more spine and its own depth:

```
body_h(A) = 179.55 + spine + ctrl_band_h = 182.75 + ctrl_band_h
```

Bracketing `ctrl_band_h` from the two constraints already fixed:

| Case | `ctrl_band_h` | `body_h(A)` | control centreline from kbd top edge |
|---|---|---|---|
| **Minimum** — one 76.20 × 21.590 breakout row + 0.60 fit/side | **22.79** | **205.54** | 14.59 mm |
| **Maximum** — the ~45 mm thumb-reach limit (decision 3) | **83.60** | **266.35** | 45.00 mm |

**So arrangement A with a K9 Pro is 294.75 mm wide and between 205.5 and
266.4 mm deep** `[DERIVED]`. The thumb-reach rule is *not* binding at the
minimum — a single breakout row puts the control centreline 14.6 mm from the
keyboard, well inside 45 mm. There is room for roughly **three** stacked
breakout rows before thumb reach bites. Which is the layout study's call, not
this document's.

### 11.3 What the keyboard does to thickness — nothing

Worth stating because `envelope.md` says *"thickness, not footprint, is the
number to defend"*:

```
K9 Pro, bottom to keycap top:  19.8 front / 23.5 rear          [VENDOR]
NeoKey 1x4 breakout + MX + standard 8 mm cap:
    1.57 (PCB) + 17.5 (cap top above PCB face) = 19.07         [DERIVED from parts.md]
```

The keyboard band and the control band land within a couple of millimetres of
each other. **The keyboard is not the thick thing.** The pots, with their
bushings and bodies behind the panel (`envelope.md`, the counterbore section),
remain the number to defend.

---

## 12. The CAD — it exists, and these are the exact files

The user said: *"if we can get a 3D model that would be insane."*

**Keychron open-sourced its factory CAD.** On 2026-04-08 it published
`Keychron/Keychron-Keyboards-Hardware-Design` — 171 device models, 790+ design
files, STEP / DWG / DXF / PDF `[VENDOR-CAD]`. **The K9 Pro has its own folder,
with files:**

| File | What it is |
|---|---|
| `K9-Pro-Top-And-Bottom-Case.stp` | **the enclosure — settles the bottom profile, feet and port positions** |
| `K9-Pro-US-Full-Model-20241128.stp.zip` | complete assembled model, case + plate + keycaps, ANSI |
| `K9-Pro-Keycap.stp` | the LSA caps |
| `k9-Pro-Stabilizer.stp` | stabiliser housings and wires |
| `K9-Pro-US-Top-Case-20230427.dwg` | 2D top-case drawing |
| `K9-Pro-US-Top-Case.pdf` | **printable dimensioned top-case drawing** |

Download base (append the filename):

```
http://www.keychron.co:34568/keychron-github-assets/
    Keychron-Keyboards-Hardware-Design/K-Pro-Series/K9%20Pro/
```

Index page (reachable, and the authoritative list):
`https://raw.githubusercontent.com/Keychron/Keychron-Keyboards-Hardware-Design/main/K-Pro-Series/K9%20Pro/README.md`

> **I could not download them.** The files are not in git — they are served from
> Keychron's own host on **port 34568**, which this environment's egress policy
> blocks (`Failed to connect to www.keychron.co port 34568`). `https`, the bare
> host and `https://keychron.co` were all tried; only `http://www.keychron.co/`
> on port 80 answered, with 503. **A normal browser will fetch them.** Keychron's
> own README anticipates this: *"If a download link does not respond, copy the
> link and paste it directly into your browser's address bar."*
>
> Put them in **`v2/parts/models/`** — gitignored, confirmed
> (`.gitignore:52`). **Do not commit them.**

**Licence.** Keychron Source-Available Licence — *"This project is
source-available. Commercial use is strictly prohibited."* Studying the CAD,
and designing original compatible accessories, is explicitly permitted;
manufacturing or selling keyboards from the files is not `[VENDOR-CAD]`. A deck
that holds a K9 Pro is an accessory, not a keyboard. **Reference only, and not
ours to redistribute.**

The K7 Pro's folder is equally complete (`K7-Pro-US-Top-Case.stp`,
`K7-Pro-US-Full-Model-20231226.stp`, `K7-Pro-US-Aluminum-Plate.pdf`, …) should a
side-by-side ever be wanted.

### 12.1 What *was* retrieved

The official **user manual PDF**, 40 pages, Keychron's own, is saved at
`/home/user/ESP32-S3-RLCD-cYbErDeCk/v2/parts/models/datasheets/keychron-K9-Pro-user-manual.pdf`
(13.7 MB, from Keychron's Shopify CDN). It is the source for §4.3, §6, §7 and
§8.3. It is in the gitignored models tree.

---

## 13. NOT SOURCED — get the calipers, or open the STEP

Listed because the enclosure cannot be drawn on an unverified figure. **Every
one of the first four is settled by `K9-Pro-Top-And-Bottom-Case.stp` (§12), with
no keyboard in hand.**

| Unknown | Why it matters | How to close it |
|---|---|---|
| **Bottom-surface profile — flat or wedged?** | §3.2 derives *flat* from a 0.05° agreement. If it is wedged the bay floor must be ramped instead of the deck face. The single highest-value unknown. | Open the case STEP. Or a straightedge on a unit. |
| **Feet** — type, size, position, removable? | A deployed or stick-on foot fouls a flat bay floor | Case STEP, or look at a unit |
| **USB-C port position along the back edge** | Arrangement A buries the back edge; the spine needs a cut-out in the right place | Case STEP. The manual drawing is not to scale (§8.3) |
| **Toggle positions and travel, back edge** | Same cut-out; and **OFF** must stay reachable with the keyboard seated | Case STEP |
| **Plate thickness and plate-to-frame datum** | Relates the 12.2 mm switch to the 7.6/13.3 frame heights; needed to predict a stripped stack | Case STEP, or `K9-Pro-US-Top-Case.pdf` |
| **LSA keycap plan size (1u footprint)** | Decides whether a retaining lip clears the outermost caps — §1.2 | `K9-Pro-Keycap.stp`, or calipers |
| **Whether Gateron's 12.2 mm includes the pins** | §5.2's 26.5 % comparison assumes it is a total; if it is body-only the gap is wider | Gateron's own datasheet — not found; or calipers on a loose switch |
| **Does USB in the BT toggle position charge without switching to wired HID?** | Decides whether the deck can feed the keyboard — §7 | Plug a charger in with the toggle on BT and watch the host stay connected |
| **Bottom-case screw pattern and count** | Only matters if stripping is ever reconsidered (§8.1 says don't) | Case STEP. Keychron publishes **no** K9 Pro disassembly guide |
| **Runtime: 100 h or 130 h; 33 h or 82 h RGB** | Cosmetic for the enclosure; recorded because Keychron's own stores disagree (§7) | Measure, or ignore |
| **Gateron KS-33 brown force/travel from Gateron** | §5.1's 1.70 / 3.0 / 55 g is `[3P]` kbd.news, not a manufacturer datasheet | Gateron's own datasheet; none found |

---

## 14. Sources

**Keychron, own pages**
- [K9 Pro product page (US)](https://www.keychron.com/products/keychron-k9-pro-qmk-via-wireless-custom-mechanical-keyboard) — layout, dimensions, weight, build, battery, variants, stock. **Its two keycap-height rows are wrong — see §2.1.**
- [K9 Pro, keychron.de](https://keychron.de/en/products/keychron-k9-pro-qmk-via-wireless-custom-mechanical-keyboard) · [keychron.be](https://keychron.be/products/keychron-k9-pro-qmk-via-wireless-custom-mechanical-keyboard) · [keychron.com.es](https://keychron.com.es/products/keychron-k9-pro-qmk-via-wireless-custom-mechanical-keyboard) — the **uncontaminated** full spec block: all four heights, typing angle, BT 5.1, MCU
- [K7 Pro product page](https://www.keychron.com/products/keychron-k7-pro-qmk-via-wireless-custom-mechanical-keyboard) — the comparison column
- [Keychron K2](https://keychron.de/en/products/keychron-k2-wireless-mechanical-keyboard) — 317 × 129 mm aluminium frame, 30/40 mm with keycaps; the likely origin of the contaminated rows
- [Gateron Low Profile 2.0 switch set](https://www.keychron.com/products/low-profile-gateron-mechanical-switch-set) — KS-33, **12.2 mm**, MX stem, 3-pin, compatibility list
- [Differences of Low-Profile Keyboards](https://www.keychron.com/pages/differences-of-low-profile-keyboards) — **"ABS bottom case and Aluminum frame"**, BT 5.1, 3 devices

**Keychron, own engineering artefacts**
- [Keychron-Keyboards-Hardware-Design — K9 Pro](https://raw.githubusercontent.com/Keychron/Keychron-Keyboards-Hardware-Design/main/K-Pro-Series/K9%20Pro/README.md) — the CAD file list and download URLs (§12)
- [K Pro Series index](https://raw.githubusercontent.com/Keychron/Keychron-Keyboards-Hardware-Design/main/K-Pro-Series/README.md) — "60% Low-profile, 61-key"; the April 2026 current-lineup list
- [LSA Profile reference](https://raw.githubusercontent.com/Keychron/Keychron-Keyboards-Hardware-Design/main/Keycap%20Profiles/LSA%20Profile/README.md) — **≈4.4 mm**, spherical, sculpted rows, 1.4–1.6 mm wall
- [Repo root README](https://raw.githubusercontent.com/Keychron/Keychron-Keyboards-Hardware-Design/main/README.md) — 171 models, licence
- [QMK fork — k9_pro ANSI layout](https://raw.githubusercontent.com/Keychron/qmk_firmware/wireless_playground/keyboards/keychron/k9_pro/ansi/info.json) — `LAYOUT_61_ansi`, 15u × 5u, bottom-row widths
- [QMK fork — k9_pro default keymap](https://raw.githubusercontent.com/Keychron/qmk_firmware/wireless_playground/keyboards/keychron/k9_pro/ansi/rgb/keymaps/default/keymap.c) — number row on base, `BT_HST1/2/3`, no mouse keycodes
- [QMK fork — k7_pro](https://raw.githubusercontent.com/Keychron/qmk_firmware/wireless_playground/keyboards/keychron/k7_pro/info.json) — 16u × 5u
- **K9 Pro User Manual PDF** — `v2/parts/models/datasheets/keychron-K9-Pro-user-manual.pdf`, from `https://cdn.shopify.com/s/files/1/0059/0630/1017/files/Keychron_K9_Pro_User_Manual.pdf` — p9 specs, p10 overview, p11–12 layers

**Third party**
- [keeb-finder: K7 Pro vs K9 Pro](https://keeb-finder.com/keyboards/compare/keychron-k7-pro-qmk-via-wireless-custom-mechanical-keyboard-vs-keychron-k9-pro-qmk-via-wireless-custom-mechanical-keyboard) — layouts sold, stock, prices
- [keeb-finder: K9 Max vs K9 Pro](https://keeb-finder.com/keyboards/compare/keychron-k9-max-qmk-wireless-custom-mechanical-keyboard-vs-keychron-k9-pro-qmk-via-wireless-custom-mechanical-keyboard) — the successor
- [green-keys: K9 Pro](https://green-keys.info/en/keyboard/keychron-k9-pro/) — independent confirmation of the four heights
- [kbd.news: Gateron KS-33 Low Profile 2.0 Brown](https://kbd.news/switch/Gateron-KS-33-Low-Profile-2.0-Brown-Chocolate-63.html) — 1.70 / 3.0 mm, 55 g
- [Notebookcheck: Keychron open-source announcement](https://www.notebookcheck.net/Keychron-announces-open-source-initiative-publishes-free-3D-models-for-all-keyboard-and-mouse-products.1269317.0.html) — repo, licence, date

**In this repo**
- `v2/docs/envelope.md` — the brief, the counterbore reasoning, "thickness is the number to defend"
- `v2/research/keyboard.md` §4.1, §4.3, §5.1, §5.2, §10.3 — the K7 Pro baseline and the deck identity reused here
- `v2/parts/parts.md` §1, §4 — Cherry MX `[DS]` figures, the 76.200 × 21.590 breakout outline
- `cad/parameters.scad:210, 748-749, 904, 913, 1014` — the identities and the v1 lip
