# Keyboard options for v2

*An enclosure input. The deliverable is dimensions: what the chassis has to be
built around. Everything else here supports that choice.*

**Scope.** The keyboard is not being dropped — it is being upgraded. No
firmware work is proposed or scoped below; the firmware is read only far enough
to answer *"would this keyboard plausibly work with what already runs"*, and
anything that would need code is flagged in one line as a risk and left there.
The footprint is **not** constrained: v2 may be larger than v1, and a better
keyboard that grows the device is an accepted trade.

---

## 0. Verdict up front

**Hosting a standard BLE keyboard is already solved on this device.** It is not
a research question and it is not a risk. `firmware/components/kbd/` contains a
working HID-over-GATT (HOGP) **central** on ESP-NimBLE, and `STATUS.md` records
the acceptance test observed on hardware: power on, the keyboard connects by
itself, type a paragraph, pull the power, power on, the paragraph is still
there — including the keyboard re-pairing and re-subscribing with no
intervention.

So the enclosure question is open in the way that matters: **pick a keyboard
worth building a chassis around, and the chassis can be built around it.**

Three findings drive the recommendation:

1. **Buy ANSI, not ISO.** `keymap.c` is a US-ANSI table. It has no entry for
   HID usage `0x64` (the ISO key beside left shift), so on an ISO/UK board that
   key produces nothing. This is a purchasing constraint, not a design one.
2. **Buy a keyboard with no trackpad or trackpoint.** The host subscribes to
   every notifiable report characteristic and decodes any notification of three
   bytes or more as a keyboard report. A board that also streams pointer
   reports is a stray-input risk. *(Risk only — not scoped.)*
3. **The number row is the discriminator, not the symbol set.**
   `docs/VERBS.md` needs `> = . _ % [ ] , < > / * ! :`, letters, and **digits
   0–9** as first-class values ("HOW MUCH: velocity on a drum, degree on a
   voice"). Every one of those is base-or-shift on a 60%/65% ANSI board. On a
   40% board the digits themselves move to a layer — which is why the 40%
   class, despite being the right *size*, is the wrong *layout* here.

**Recommendation: Keychron K7 Pro** (65%, BLE 5.1, ANSI, 306 × 97.5 mm).
**Fallback: NuPhy Air60 V2** (60%, BLE 5.1, 297.2 × 107.2 mm).
Reasoning in §6.

> **On width:** §10 chases the one thing that would beat the K7 Pro — a
> narrow board that still has a number row. The short version is that
> **nothing finished exists under 280 mm**, but 66–75 mm of width *is*
> available from a PCB kit you assemble once. §10.3 has the deck sizes and
> §10.4 costs the build.

---

## 1. What the device already hosts — the compatibility test

Read from `firmware/components/kbd/ble_kbd.c` and `keymap.c`. Stated so the
keyboard choice can be checked against it; **no changes proposed.**

| Property | What the code does | Consequence for buying |
|---|---|---|
| Stack | ESP-NimBLE central (`host/ble_hs.h`, `nimble_port_freertos`) — not Bluedroid, not `esp_hid_host` | Any standard HOGP peripheral is in scope |
| Device selection | Connects to anything advertising HID service `0x1812` **or** appearance `0x03C1` (keyboard), any directed advert, or any bonded address | Not locked to the Rii by name, VID or PID |
| Pairing pinned? | Yes — one identity address stored (`s_peer`), reconnect by directed connect to that identity; `kbd_forget_all()` on a long KEY hold | A pinned bond is the stage-safe behaviour and it is already there |
| Reconnect | Two paths: directed connect to the known identity (20 s), and a forever-scan for a new keyboard | Handles directed advertising and rotating private addresses |
| Keymap | Generic HID usage → ASCII, `0x04–0x38` plus keypad `0x54–0x63`. **No Rii-specific branches** | Generic. But US-ANSI only — see §0 |
| Report decode | Boot-keyboard *shape* `{mods, reserved, keycodes…}`, keycode count taken from length, anything under 3 bytes ignored | **Does not** read the Report Map (`0x2A4B`) or the Report Reference descriptor (`0x2908` — defined, unused) |
| Reports subscribed | Every notifiable `0x2A4D` Report and `0x2A22` Boot Keyboard Input, up to 8 | Prefer a keyboard-only board; see risk 2 in §0 |
| Key repeat | Synthesised on the host (400 ms delay, 45 ms period) because BLE HID repeat is device-dependent | Keyboard-independent — no autorepeat behaviour to shop for |
| Liveness | HID Exit Suspend at subscribe and periodically on idle; 60 s of silence tears the link down and rebuilds (~5 s) | Already defends against a keyboard that parks itself mid-set |
| Radio sharing | `kbd_share_radio()` drops scan duty to a 30 ms window every 160 ms when BLE MIDI is up | Coexistence already handled |

**Two risks to carry, one line each, not scoped:**

- A board set to **NKRO** sends a bitmap rather than a keycode list, which this
  decode would misread — so keep a programmable board on its 6KRO default
  ([ZMK's `CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE` is 6 by default](https://zmk.dev/docs/config/system)).
- A board that also emits **pointer or ≥3-byte consumer reports** can inject
  stray input, per risk 2 in §0.

---

## 2. The protocol filter — what is usable at all

The ESP32-S3 has **no Bluetooth Classic radio**. Espressif's own stack
documentation states it twice, for both hosts: *"ESP-Bluedroid for ESP32-S3
supports Bluetooth LE only. Classic Bluetooth is not supported."* and
*"ESP-NimBLE supports Bluetooth LE only. Classic Bluetooth is not supported."*
([ESP-IDF Bluetooth overview](https://docs.espressif.com/projects/esp-idf/en/v5.3/esp32s3/api-guides/bluetooth.html))
The repo's own `docs/OS.md` carries the same as a `[FACT]`.

| Class | Usable? | Why |
|---|---|---|
| **BLE HID / HID-over-GATT (HOGP)** | **Yes** | This is what the device hosts today |
| Bluetooth Classic HID | **No** | No BR/EDR radio on the S3, at all |
| 2.4 GHz proprietary dongle | **No** | The dongle is a USB HID device; it needs a USB host — see §3 |
| USB-C wired | **No**, as built | Single USB-C is the charge port, configured as a sink — see §3 |

**Buying rule.** A spec sheet saying *Bluetooth 4.0 / 4.2 / 5.x* or *BLE* is in
scope. A spec sheet saying *Bluetooth 3.0* or earlier, or offering **only** a
proprietary 2.4 GHz nano-receiver (Logitech Unifying and similar), is out. A
board offering BLE **and** a 2.4 GHz dongle is fine — use the BLE mode and
ignore the dongle.

---

## 3. USB host and wired — closed, in one paragraph

Espressif supports USB HID hosting on the S3 in principle: the
[`usb_host_hid` component](https://components.espressif.com/components/espressif/usb_host_hid)
lists `esp32s3` among its targets and the
[USB Host HID example](https://raw.githubusercontent.com/espressif/esp-idf/master/examples/peripherals/usb/host/hid/README.md)
supports S2/S3 (boot protocol only). It is nonetheless **not available on this
board as built**, for three stacked reasons, and none of them is firmware. The
internal PHY is shared: *"By default, USB_SERIAL_JTAG module is connected to
the internal PHY of the ESP32-S3"*, and taking it for USB-OTG costs the CDC
console on that port
([ESP-IDF USB-OTG console](https://docs.espressif.com/projects/esp-idf/en/v5.3/esp32s3/api-guides/usb-otg-console.html)).
A host must **supply** 5 V VBUS, which the SoC does not do — Espressif's own
ESP32-S3-USB-OTG board needs a boost converter, a load switch, a 500 mA
MIC2005A current limiter and overcurrent sensing to manage it
([user guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-usb-otg/user_guide.html)).
And the deck's single USB-C is the charge port with its CC resistors
configuring it as a **sink**; `docs/HARDWARE.md` already lists "Can the ETA6098
boost VBUS?" as an open question and says *"Do not assume either answer."*
**Treat wired USB and 2.4 GHz dongles as closed. BLE is the transport.**

---

## 4. Candidates — exact dimensions

Every number below is either quoted from the source linked in the last column
or marked **[derived]** / **[unverified]**. Nothing is estimated silently.
Depth is the front-to-back dimension of the keyboard as it sits under your
hands — the axis that drives deck height.

### 4.1 Confirmed outside dimensions

| Keyboard | Width × Depth (mm) | Thickness (mm) | Weight | Keys / layout | Pitch | Protocol | ANSI? | Source |
|---|---|---|---|---|---|---|---|---|
| **Rii 518BT** *(v1 incumbent)* | 108.5 × 58.2 | 10.2 | n/p | thumb QWERTY | n/p | **BLE HOGP** | ANSI | repo `docs/DATUMS.md`, FCC `YIZRT-RII518` |
| **Keychron K7 Pro** | **306 × 97.5** | **18.2 front / 23.2 rear** incl. keycaps; **8.8 / 13.2** without | **436 g** | 68, 65% | 19.05 [derived] | **BLE 5.1** + USB-C | ANSI (ISO also sold) | [keychron.com](https://www.keychron.com/products/keychron-k7-pro-qmk-via-wireless-custom-mechanical-keyboard) |
| **Keychron K11 Max** | **346 × 115** | n/p | **520 g** | 68, 65% Alice | 19.05 [derived] | **BLE** (3 devices) + 2.4 GHz + wired | ANSI | [keychron.com](https://www.keychron.com/products/keychron-k11-max-qmk-via-wireless-custom-mechanical-keyboard) |
| **NuPhy Air60 V2** | **297.2 × 107.2** | **13.5 or 20 — conflicting** | **463 g** | 64, 60% | 19.05 [derived] | **BLE 5.1** + 2.4 GHz + USB-C | ANSI | retailer listings only ([example](https://www.desertcart.nl/products/723730286-nuphy-air60-v2-portable-wireless-keyboard-60-low-profile-mechanical)) |
| **Logitech MX Keys Mini** | **295.99 × 131.95** | **20.97** | **506.4 g** | not published | ~19 [unverified] | **BLE** (+ optional Bolt) | ANSI | [logitech.com](https://logitech.com/en-hk/shop/p/mx-keys-mini) |
| **Apple Magic Keyboard** (USB-C) | **279 × 114.9** | **10.9 max / 4.1 min** | **239 g** | not published | ~19 [unverified] | Bluetooth — **version not published** | ANSI | reseller listings; [Apple specs](https://support.apple.com/en-vn/112443) omit dimensions |
| **Vortex Core Plus** | **269 × 92** | **15** | **~552 g** | 50, 40% | 19.05 [derived] | Bluetooth — **version not stated** — + 2.4 GHz dongle + USB-C, 2×AAA | ANSI | [green-keys review](https://green-keys.info/en/vortexgear-core-plus-review%ef%bd%9can-always-available-40-keyboard-in-an-attractive-aluminum-enclosure/) |
| **Vortex Core** | **248 × 76.2** | **25.5** | **400 g** | 47, 40% | 19.05 [derived] | **USB wired only** → unusable | ANSI | [TechPowerUp](https://techpowerup.com/review/vortex-core/single-page.html) |
| **Keyboardio Atreus** | **243 × 100** | **28** | **310 g** | 44, columnar | ~19 [unverified] | **USB-C wired only** → unusable | ANSI | [shop.keyboard.io](https://shop.keyboard.io/products/keyboardio-atreus) |
| **OLKB Planck V6** | **234 × 81** | **33** | **510 g** | 47/48, 12×4 ortho | 19.05 | **USB-C wired only** → unusable | ortho | Drop/retailer listing |
| **OLKB Preonic** | **231 × 104** | **23 [unverified]** | **~508 g** | 60, 12×5 ortho | 19.05 | **USB-C wired only** → unusable | ortho | retailer listing |
| **ZSA Voyager** | **not published** | "our thinnest board yet" — no figure | n/p | 52, split columnar | 18 × 17 (Choc) | **USB-C wired only** → unusable | columnar | [zsa.io/voyager](https://www.zsa.io/voyager) |

### 4.2 Integrable parts, for a designed-in keyboard rather than a bought one

| Part | Dimensions (mm) | Protocol | Note | Source |
|---|---|---|---|---|
| **nice!nano v2** controller | **34.1 × 18.3 × 3.3** (1.6 mm PCB, mid-mount USB-C) | **BLE** (nRF52840), runs ZMK | The whole wireless half of a custom board, 3.3 mm thick | [mechboards.co.uk](https://www.mechboards.co.uk/products/nice-nano-v2) |
| **Corne / CRKBD (Choc)** | not published; 3 rows × 6 cols + 3 thumbs per half → **~108 × 51 mm** switch field per half [derived from 18 × 17 pitch] | **BLE HOGP** via nice!nano + ZMK | Bare PCB by nature; 42 keys, **no number row** | [Deskthority](https://deskthority.net/wiki/Corne), pitch per [kbd.news](https://kbd.news/switch-comparison/Gateron-KS-27-Low-Profile-Blue-vs-kailh-choc-white-166-67.html) |
| **Solder Party BB Q20** | **not published** — not in the vendor docs or any retail listing found | **I2C / USB HID — no BLE** | BlackBerry Q20 thumb keyboard + optical trackpad, RP2040, Qwiic/PMOD. Would need wiring and code, and the trackpad is the stray-report risk | [solder.party/docs/bbq20kbd](https://solder.party/docs/bbq20kbd/), [Lectronz](https://www.lectronz.com/products/123) |

### 4.3 Key switch travel, where published

| Switch | Pre-travel | Total travel | Force | Used by |
|---|---|---|---|---|
| Gateron Low Profile 2.0 (KS-33) | 1.7 mm | 3.0 mm | 45 g act. / 60 g bottom | the current Gateron low-profile family |
| Vortex Core Plus as shipped | 1.7 ± 0.4 mm | 3.2 ± 0.2 mm | 50 ± 15 gf | Vortex Core Plus |
| Kailh Choc V1 | 1.50 mm | 3.0 mm | varies | Corne Choc, ZSA Voyager |
| Scissor (Apple / Logitech) | — | ~1–2 mm [unverified] | — | Magic Keyboard, MX Keys Mini |
| Rubber dome | — | — | — | **Rii 518BT (what you have)** |

Sources: [kbd.news switch comparison](https://kbd.news/switch-comparison/Gateron-KS-27-Low-Profile-Blue-vs-kailh-choc-white-166-67.html),
[green-keys Core Plus review](https://green-keys.info/en/vortexgear-core-plus-review%ef%bd%9can-always-available-40-keyboard-in-an-attractive-aluminum-enclosure/).

---

## 5. What each candidate does to the deck — bay vs dock

### 5.1 The arithmetic, reused

`cad/parameters.scad` makes the v1 deck a function of the keyboard, and
`tools/validate.py` asserts it every run:

```
kbd_pocket_w = keyboard_w + 1.35        (parameters.scad:748)
kbd_pocket_h = keyboard_d + 0.95        (parameters.scad:749)
body_w       = max(kbd_pocket_w, 93.5) + 6.40
body_h       = kbd_pocket_h + 81.10     (bottom wall + spine 3.2 + board 70.1 + wall)
```

Check: `109.85 + 6.40 = 116.25` and `59.15 + 81.10 = 140.25` — v1 exactly.
Holding that layout (keyboard bay in front, board bay behind, one spine) and
substituting each candidate:

| Keyboard in a v1-style bay | Deck W × H (mm) | vs v1 (116.25 × 140.25) | Min deck depth |
|---|---|---|---|
| Rii 518BT | 116.2 × 140.2 | — | 16.85 (actual) |
| **OLKB Preonic** | 238.8 × 186.1 | +106 % W | ~30 |
| **Vortex Core Plus** | **276.8 × 174.1** | **+138 % W, +24 % H** | **~22** |
| **NuPhy Air60 V2** | 304.9 × 189.2 | +162 % W | ~21–28 (height unresolved) |
| **Keychron K7 Pro** | **313.8 × 179.6** | **+170 % W, +28 % H** | **~27–29** |
| Keychron K11 Max | 353.8 × 197.1 | +204 % W | n/p |
| Apple Magic Keyboard | 286.8 × 196.9 | +147 % W | ~18 |
| Logitech MX Keys Mini | 303.8 × 214.0 | +161 % W, +53 % H | ~28 |

**Read this honestly:** any full-size-pitch keyboard in a bay turns the deck
from a 116 mm handheld into a **~280–315 mm two-handed object** — a lectern or
strap instrument, not something held in one hand. That is a legitimate choice
and the user has accepted growth; it just needs to be a decision rather than a
surprise.

### 5.2 Bay, dock, or integrate

**Bay — keyboard sunk into the chassis, keycaps proud through an aperture
(what v1 did).** Needs two numbers that almost no vendor publishes: *case
thickness under the keycap tops* and *keycap protrusion*.

> **Keychron is the only vendor in this set that publishes both.** The K7 Pro
> is 8.8 mm (front) / 13.2 mm (rear) without keycaps and 18.2 / 23.2 mm with
> them — so **~9.4–10.0 mm of keycap protrusion** over an 8.8–13.2 mm case
> body. That is exactly the pair a bay design needs, and it is available
> without owning the keyboard. No other candidate here gives it.

Caveat for any bay: these boards are **wedged**. The K7 Pro rises 4.4 mm front
to rear across 97.5 mm (≈2.6°); the Magic Keyboard goes 4.1 → 10.9 mm. A flat
pocket floor will not seat them — the floor needs a matching ramp or a stepped
shelf, and flip-out feet must be removed or accommodated.

**Dock — keyboard sits in a cradle on the chassis, retained by a lip or
magnets, connected only by BLE.** This suits the 270–310 mm class far better
than a bay: it costs no internal volume, imposes no wedge-matching, keeps the
keyboard separately chargeable, and lets the keyboard come off to be typed on
at a comfortable distance while the deck stands up. The chassis grows in
footprint but not in depth. For the K7 Pro or Air60 V2 **this is the lower-risk
mechanical path**, and it is the one that survives a keyboard being replaced
later.

**Integrate — design the switch plate into the v2 chassis.** The only genuinely
BLE-capable route here is **ZMK on a nice!nano v2** (34.1 × 18.3 × **3.3 mm**),
which is a standard HOGP peripheral and whose layers resolve *on the keyboard*,
so the host sees plain HID usages. Two properties make it attractive beyond
mechanics: ZMK holds **five independent host profiles**, and *"pairing saves
bond information to the selected profile"* and *"will not replace this
automatically when you initiate pairing with another device"* — i.e. the bond
can be pinned to the deck and nothing else can steal it mid-set
([ZMK Bluetooth](https://zmk.dev/docs/features/bluetooth)). The cost is that
there is no purchasable 60%-with-number-row ZMK board in this set; Corne-class
boards are 42 keys with **no number row**, which §0 rules out for this
language. Integrating means designing the matrix, and that is a bigger project
than v2's keyboard decision.

**Removable-case / bare-PCB availability, asked directly:**

- Keychron K7 Pro, K11 Max, NuPhy Air60 V2, Vortex Core Plus — plastic or
  aluminium screw-together cases, so a plate-and-PCB assembly can in principle
  be lifted out and integrated. **No vendor-supported bare-PCB SKU.** Keychron
  "barebone" means case + PCB without switches and keycaps, not a bare PCB.
- Apple Magic Keyboard, Logitech MX Keys Mini — sealed consumer units. **Bay or
  dock only, never integrated.**
- Corne / Voyager-class — bare PCB is the *normal* way they are sold. Only the
  Corne has a BLE option (nice!nano + ZMK); the Voyager is wired-only.
- Solder Party BB Q20 — a module, which is the most integration-friendly thing
  here, but **I2C/USB only, no BLE**, and it has a trackpad.

---

## 6. Recommendation

### First choice — Keychron K7 Pro (65%, ANSI)

`306 × 97.5 mm`, `18.2 / 23.2 mm` with keycaps, `8.8 / 13.2 mm` without,
`436 g`, 68 keys, Bluetooth 5.1 + USB-C, low-profile Gateron switches (Keychron's
listing does not state the generation; the current family is 1.7 mm pre-travel,
3.0 mm total).

- **Right layout.** A 65% keeps the number row and dedicated `[`, `]`, `\|`,
  `;:`, `,<`, `.>`, `/?`, `-_`, `=+`. Every character `docs/VERBS.md` needs,
  plus the editor's `|`, is base-or-shift. No layer diving for digits.
- **No AltGr trap.** The Rii delivers its Fn layer to the host as AltGr —
  `kbd.h` records that this silently swallowed `>`, *"the one character the
  command syntax requires."* A board with a real number row never needs AltGr
  for a symbol, so that whole failure class disappears by layout.
- **Lowest mechanical risk of the full-pitch boards**, because it is the only
  one that publishes with- and without-keycap heights (§5.2) — the bay can be
  drawn before the keyboard arrives.
- **Standard HOGP**, advertises as `Keychron K7 Pro`, and the existing host
  connects on HID service UUID or keyboard appearance.
- **Remappable** via QMK/VIA, and the saved keymap applies in wireless mode —
  retailer spec notes remapping is done *"on wired mode only"* and then *"the
  keymap will work on both wireless and wired modes"*
  ([K10 Pro listing](https://www.neweggbusiness.com/p/9B-0GA-099M-00009)).
  Useful if any symbol wants moving, and it costs nothing on the deck side.
- Thinnest real-mechanical option here at 18.2 mm front including keycaps.

**Costs.** The deck becomes ~314 × 180 mm in a bay, or keeps a smaller body
with a 306 mm cradle if docked. Wireless polling is **90 Hz** (~11 ms), which
sits inside the 7.5–15 ms interval `docs/OS.md` already designs for. No 2.4 GHz
fallback — BLE or the cable, and the cable is not available to this deck.

### Second choice — NuPhy Air60 V2 (60%, ANSI)

`297.2 × 107.2 mm`, `463 g`, 64 keys, Bluetooth 5.1 + 2.4 GHz + USB-C.

Thinner than the K7 Pro if the 13.5 mm figure is the real one, 9 mm narrower,
and still has a number row. Two reasons it is second, not first: **its height
is unresolved** — retailer listings give both 13.5 mm and 20 mm for the same
product and NuPhy's own page would not serve (HTTP 429), so a bay cannot be
dimensioned from published data; and several buyers report wireless dropouts
(on the 2.4 GHz receiver specifically). Measure a unit before committing
geometry.

### Worth a look if the deck should stay small — Vortex Core Plus

`269 × 92 × 15 mm`, `~552 g`, 50 keys, Bluetooth (version unstated — confirm it
is not Classic) + 2.4 GHz + USB-C, **2×AAA**.
The smallest BLE board here with a real case, only 15 mm thick, and AAA cells
mean a flat keyboard is a ten-second swap on stage rather than a dead set. It
is ruled out of first place by **layout, not size**: a 40% has no number row,
and digits are first-class values in this language. If the digits were ever
moved onto the deck's own physical controls, this becomes the best-shaped
candidate in the table.

### Rejected, and why — in one line each

- **Vortex Core, Keyboardio Atreus, OLKB Planck, OLKB Preonic, ZSA Voyager** —
  USB wired only; §3 closes that path.
- **Apple Magic Keyboard** — Apple does not publish the Bluetooth version, so
  HOGP cannot be confirmed from primary sources, and it is a sealed unit.
- **Logitech MX Keys Mini** — confirmed BLE and genuinely good to type on, but
  **131.95 mm deep** makes the deepest deck in the table (214 mm) and it is
  sealed.
- **Keychron K11 Max** — Alice split layout at 346 mm; the ergonomics argue
  against a chassis-mounted board and the width is the worst here.
- **Solder Party BB Q20** — most integration-friendly part found, but no BLE,
  no published dimensions, and a trackpad that is the stray-report risk.
- **Any Bluetooth 3.0 or 2.4-GHz-receiver-only keyboard** — §2.

---

## 7. Key feel, standing up

For a device typed on while standing, three things matter more than switch
flavour:

1. **Travel, not actuation force.** The Rii's rubber dome gives no clear
   actuation point, so standing — with no wrist reference and the deck moving —
   you cannot feel whether a key fired. 3.0 mm of mechanical travel with a
   1.7 mm actuation point gives a definite event at a known depth. This is the
   single biggest improvement available.
2. **Low profile over full height.** Low-profile Gateron/Choc keeps the keycap
   tops 9–10 mm above the case instead of 15–18 mm, which keeps the deck thin
   and keeps the hands closer to the display sightline. It costs nothing in
   feel at 3.0 mm travel.
3. **Mass as stability, up to a point.** A 436–552 g keyboard resists being
   pushed around far better than a 10 g thumb board, but all of it is weight
   you hold or wear. 436 g (K7 Pro) is near the sweet spot; 552 g (Core Plus,
   aluminium) is noticeably more.

Also: **sculpted or spherical keycaps help more than switches do.** The K7 Pro
uses LSA (low-profile spherical-angled) caps, which locate a fingertip without
looking — and on a device with no backlight (`docs/OS.md`: *"The device is
unusable in the dark"*, a `[FACT]`) being locatable by touch is not a luxury.
Flat chiclet caps (Magic Keyboard, MX Keys Mini) are the weakest here on
exactly that axis.

---

## 8. Numbers that are still missing, and how to close them

Listed because the enclosure cannot be drawn on an unverified figure.

| Unknown | Why it matters | How to close it |
|---|---|---|
| **NuPhy Air60 V2 height** — 13.5 vs 20 mm | Decides whether it beats the K7 Pro on thickness, and whether a bay is possible | Measure a unit, or get NuPhy's spec page to load (it returned HTTP 429) |
| Air60 V2 / Core Plus **with-vs-without keycap** heights | Required for any bay aperture | Measure; no vendor publishes them |
| **Key pitch**, all candidates | Confirms keycap and aperture spacing | All marked [derived] are back-computed from published outside width ÷ layout units and are consistent with 19.05 mm; confirm on a unit |
| **Rii 518BT mass and pitch** | Baseline for the comparison | Not published in the FCC filing or vendor drawing; weigh and measure the unit on hand |
| Apple Magic Keyboard **Bluetooth version** | Decides usable vs not | Not in Apple's specs; would need a scan of the advertising data |
| **Vortex Core Plus Bluetooth version** | Decides usable vs not — if it is Classic, §2 rules it out entirely | The review found gives no version; check Vortexgear's own spec sheet or scan the advertising data |
| Solder Party BB Q20 **outside dimensions** | Only integrable module found | Not in vendor docs or any retail listing; take from the published design files |
| MX Keys Mini / Magic Keyboard **key counts** | Minor — layout completeness | Count from a unit or a layout diagram |

---

## 9. Sources

**Device and stack**
- [ESP-IDF Bluetooth overview (ESP32-S3)](https://docs.espressif.com/projects/esp-idf/en/v5.3/esp32s3/api-guides/bluetooth.html) — BLE only, no Classic, both hosts
- [ESP-IDF BLE HID Host example](https://raw.githubusercontent.com/espressif/esp-idf/master/examples/bluetooth/esp_hid_host/README.md) — ESP32-S3 in the supported-target table
- [`usb_host_hid` component](https://components.espressif.com/components/espressif/usb_host_hid) — targets include esp32s3
- [ESP-IDF USB Host HID example](https://raw.githubusercontent.com/espressif/esp-idf/master/examples/peripherals/usb/host/hid/README.md) — S2/S3, boot protocol only
- [ESP-IDF USB-OTG console](https://docs.espressif.com/projects/esp-idf/en/v5.3/esp32s3/api-guides/usb-otg-console.html) — internal PHY assignment
- [ESP-IDF USB examples: PHY pin mapping](https://raw.githubusercontent.com/espressif/esp-idf/master/examples/peripherals/usb/README.md) — D+ = GPIO20, D− = GPIO19
- [ESP32-S3-USB-OTG user guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-usb-otg/user_guide.html) — boost, load switch, 500 mA limit for VBUS
- [ZMK Bluetooth](https://zmk.dev/docs/features/bluetooth) — HOGP, five profiles, bond pinning
- [ZMK system configuration](https://zmk.dev/docs/config/system) — `CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE` default 6

**In this repo**
- `firmware/components/kbd/ble_kbd.c`, `keymap.c`, `include/kbd.h`
- `STATUS.md` — the observed acceptance test and the three bugs behind "connects but types nothing"
- `docs/OS.md` — `[FACT]` table, the AltGr/Fn finding, pairing recovery
- `docs/HARDWARE.md` — the open ETA6098 VBUS question
- `docs/VERBS.md` — the symbol set the language actually needs
- `cad/parameters.scad:748-749`, `README.md` — the bay arithmetic reused in §5.1

**Keyboards**
- [Keychron K7 Pro](https://www.keychron.com/products/keychron-k7-pro-qmk-via-wireless-custom-mechanical-keyboard) · [K11 Max](https://www.keychron.com/products/keychron-k11-max-qmk-via-wireless-custom-mechanical-keyboard) · [VIA remap wired-only note](https://www.neweggbusiness.com/p/9B-0GA-099M-00009)
- [Logitech MX Keys Mini](https://logitech.com/en-hk/shop/p/mx-keys-mini) · [MX Keys Mini for Business specs](https://hub.sync.logitech.com/mx-keys-mini/post/specifications---mx-keys-mini-for-business-AZ2NmSOwjILQMsA)
- [Apple Magic Keyboard specs](https://support.apple.com/en-vn/112443)
- [NuPhy Air60 V2 — retailer listing](https://www.desertcart.nl/products/723730286-nuphy-air60-v2-portable-wireless-keyboard-60-low-profile-mechanical)
- [Vortex Core review (TechPowerUp)](https://techpowerup.com/review/vortex-core/single-page.html) · [Vortex Core Plus review (green-keys)](https://green-keys.info/en/vortexgear-core-plus-review%ef%bd%9can-always-available-40-keyboard-in-an-attractive-aluminum-enclosure/)
- [Keyboardio Atreus](https://shop.keyboard.io/products/keyboardio-atreus)
- [ZSA Voyager](https://www.zsa.io/voyager)
- [Corne (Deskthority)](https://deskthority.net/wiki/Corne) · [Choc vs MX spacing](https://kbd.news/switch-comparison/Gateron-KS-27-Low-Profile-Blue-vs-kailh-choc-white-166-67.html)
- [nice!nano v2 (Mechboards)](https://www.mechboards.co.uk/products/nice-nano-v2)
- [Solder Party BB Q20 docs](https://solder.party/docs/bbq20kbd/) · [Lectronz listing](https://www.lectronz.com/products/123)

---

# 10. The ortholinear question — can the narrow class have a radio?

*Added after the first pass. §4 left a specific trade open: the ortho class is
the right size (Preonic 231 × 104 mm) and the right layout (digits on a real
row) and the only thing wrong with it was the radio. This section closes it.*

## 10.1 The answer, plainly

**There is no purchasable, finished BLE keyboard with a number row under
280 mm.** Every assembled board that has a number row and a usable radio is
either ≥296 mm wide, or a large split that is worse on depth and height than
the K7 Pro:

| Assembled BLE + number row | Size | Why it does not help |
|---|---|---|
| Keychron K7 Pro | 306 × 97.5 mm | the §6 recommendation — nothing narrower exists assembled |
| NuPhy Air60 V2 | 297.2 × 107.2 mm | 9 mm narrower, height unresolved |
| Logitech MX Keys Mini | 295.99 × 131.95 mm | 132 mm deep |
| **MoErgo Glove80** | **372 × 168 mm, 60 mm high, 600 g** | wider than the K7 Pro *and* 60 mm tall |
| **Dygma Defy** | **164.16 × 210.02 × 35.36 mm** (per Dygma's spec sheet) | 210 mm deep per half, ~1 kg, columnar — no number row confirmed |
| **Kinesis Advantage360 Pro** | not published | contoured keywells; cannot sit in or on a flat chassis |

**But the narrow class does exist with a radio — as a PCB kit, not a product.**
Three real routes, all with a number row, all in the 230–250 mm class:

| Board | Layout | Radio | Bought how |
|---|---|---|---|
| **splitkb Halcyon Lily58 / Elora** | column-staggered split, 58 / 62 keys, **number row** | **Vendor-supported.** splitkb's own nRF52840 Wireless Halcyon Controller, ZMK | **PCB arrives pre-soldered and tested**; controller is a drop-in |
| **splitkb Aurora Helix** | **ortholinear** split, up to 64 keys, **number row**, MX spacing | ZMK build — nice!nano listed as a compatible controller | PCB kit; you solder the matrix |
| **Keebio Nyquist** | **ortholinear** split 5 × 12, **number row** | ZMK mod — Pro Micro footprint takes a nice!nano | PCB kit; you solder; stock firmware is QMK/AVR |

So the honest shape of the answer is **not** "no". It is: **the 75 mm of width
is available, and the price of it is that you assemble the keyboard.** §10.4
costs that out.

## 10.2 The geometry, derived exactly

Ortholinear is the one layout whose size is arithmetic rather than a vendor
claim. MX spacing is 19.05 mm, so:

```
5 x 12 MX ortho switch field  = 228.60 x 95.25 mm     (12 x 19.05, 5 x 19.05)
one 6 x 5 half                = 114.30 x 95.25 mm
```

That derivation checks against a shipping product: the Preonic's published
outside size is **231 × 104 mm**, which is the 228.60 × 95.25 field plus
**1.20 mm of bezel per side in width and 4.38 mm per side in depth**. So the
field dominates and the outline is predictable to a millimetre or two.

**Two Aurora Helix halves butted together are exactly a Preonic** — same
228.60 mm of switch field, in two pieces of 114.30 mm. And 114.30 mm is within
2 mm of the v1 deck's own width of 116.25 mm, which is a pleasing coincidence
rather than a design input, but worth noticing.

## 10.3 What each arrangement does to the deck

Same v1 arithmetic as §5.1 (`body_w = max(pocket_w, 93.5) + 6.40`,
`body_h = pocket_d + 81.10`), and `display_module_w = 91.00 mm`
(`cad/parameters.scad:210`).

| Arrangement | Keyboard footprint | **Deck W × H** | vs K7 Pro |
|---|---|---|---|
| **A** — Keychron K7 Pro, one bay | 306 × 97.5 | **313.8 × 179.6** | — |
| **B** — 5 × 12 ortho pair butted, Preonic outline | 231 × 104 | **238.8 × 186.1** | **−75.0 mm W**, +6.5 mm H |
| **B′** — same, generous PCB bezels + thumb row | 240 × 110 | **247.8 × 192.1** | −66.0 mm W, +12.5 mm H |
| **C** — split halves **flanking** the 91 mm display | 325 × 104 | **332.8 × 186.1** | **+19.0 mm W** — worse |

**The width saving is real and it is 66–75 mm.** It comes from deleting the
65% layout's navigation column and the staggered layout's wasted edges, and it
costs ~7–13 mm of depth because an ortho board's five rows sit slightly deeper
than a 65%'s.

**Arrangement C is a trap.** "Halves either side of the display" sounds like the
split-native answer and it is the *widest* option in this entire document —
19 mm wider than simply bolting on a K7 Pro. A split only pays off when the
halves are **butted or stacked**, not spread. If the halves are to be spread,
they should come off the chassis entirely and sit where the hands want them,
which is a different instrument from the one being designed.

## 10.4 Costing the build honestly

Two routes, both reaching a BLE board with a number row in the 230–250 mm class.

### Route 1 — splitkb Halcyon (vendor-supported wireless)

| Part | Price |
|---|---|
| Halcyon Lily58 **or** Elora PCB — *pre-soldered, fully tested* | €61.94 ex-VAT |
| Wireless Upgrade Bundle: 2 × nRF52840 controllers + module connection kit + USB-C dongle | €95.00 |
| *(or just 2 controllers + module connection kit)* | *€61.94* |
| 58–62 MX switches @ ~€0.35 | ~€22 |
| 58–62 keycaps (blank 1u set) | ~€25–40 |
| LiPo Power Board + cell, or Coin Cell Power Board | ~€10–20 |
| Case | separate; 3D-printed in-house costs filament |
| **Parts total** | **~€180–240** |

**Effort: an afternoon, not a project.** The PCB arrives *"pre-soldered, fully
tested"* — there is no matrix to solder. The wireless controller is *"a drop-in
replacement for the wired RP2040 controller"* and the module uses a **CE- and
FCC-certified** nRF52840 part. Switches push into hotswap sockets, keycaps
press on, and the layout is changed in **ZMK Studio** or by compiling from
splitkb's own [`zmk-halcyon-config`](https://github.com/splitkb/zmk-halcyon-config),
which lists the Halcyon Wireless controller and the Elora rev2 / Lily58 rev2
shields as supported boards. This is the least-risk way to get a radio onto a
narrow board.

### Route 2 — Aurora Helix + nice!nano (the true ortholinear)

| Part | Price |
|---|---|
| Aurora Helix PCB kit (MX or Choc hotswap) | €31.36 ex-VAT |
| 2 × nice!nano v2 @ ~$25 | ~€46 |
| 2 × controller socket (recommended) | ~€5 |
| 64 MX switches @ ~€0.35 | ~€22 |
| 64 keycaps (blank ortho/DSA set) | ~€25–40 |
| 2 × LiPo cell + 2 × power switch | ~€12 |
| Case | separate |
| **Parts total** | **~€145–185** |

**Effort: a real soldering project, one or two evenings if you solder well.**
The kit ships as bare PCBs plus **70 × 1N4148 diodes, 7 × 4.7 kΩ resistors,
2 TRRS jacks, 2 reset buttons** and 64 hotswap sockets — all of which you fit
yourself (the Choc hotswap variant substitutes SMD parts, which is harder, not
easier). Then a ZMK config has to be written and built, because splitkb
supports the Aurora line under QMK **or** ZMK but ships no wireless controller
for it. nice!nano is listed as compatible, which is not the same as supported.

### Would I recommend it to someone who wants a finished instrument?

**Route 1: yes, with one caveat. Route 2: no.**

Route 1 is the only path in this document that gets a number row into a
~240 mm width with a vendor-supported radio and no matrix soldering. The
caveat is that it is **not ortholinear** — Lily58 and Elora are
column-staggered, and the Elora's arc thumb cluster adds depth. If the
attraction of the ortho class was the grid itself rather than the width, Route 1
does not deliver it.

Route 2 delivers the actual grid, and it is the cheapest option on the page,
but the deliverable here is an **instrument the user performs with on stage**.
A hand-soldered 70-diode matrix with a community-maintained firmware target is
a thing that fails in a venue, and it fails in a way nobody can diagnose under
stage lights. The existing deck already earned its reliability the hard way —
`STATUS.md` records three separate bugs behind a single "connects but types
nothing" — and adding a self-built keyboard puts a second unproven link in the
same chain. **Not for a performance instrument.**

So the real decision is two lines long:

- **Want it finished, and will accept 306 mm** → Keychron K7 Pro, §6. Deck
  ~314 × 180 mm.
- **Want it narrow, will accept assembling a keyboard once and losing the
  ortho grid** → Halcyon Lily58 with the Wireless Halcyon Controller. Deck
  ~239–248 × 186–192 mm.

Nothing in between exists to buy.

## 10.5 Risks specific to this route

One line each, not scoped — the same discipline as §1.

- **ZMK must stay on its 6KRO default**, per §1; `CONFIG_ZMK_HID_KEYBOARD_REPORT_SIZE`
  is 6 unless changed, which matches what the deck decodes.
- **Dongle mode is not useful here.** splitkb's coin-cell battery figure of
  *"six to nine months"* is quoted *"when paired with the dongle"* — a dongle
  is a USB device and §3 closes USB host, so this deck needs **direct BLE** and
  the **LiPo Power Board**, with correspondingly shorter life.
- **Keycap legends.** All these kits ship bare; a blank or ortho keycap set has
  to be chosen, and `docs/OS.md` notes the deck has no backlight, so legends
  and sculpting matter more here than on a lit desk.
- **Split means two batteries and two radios.** ZMK links the halves over BLE
  as well, so a split doubles the things that can go flat mid-set. A unibody
  board is one radio and one cell.
- **No vendor publishes an outside dimension for any PCB kit in §10.1.** The
  switch-field derivation in §10.2 is exact, but the PCB outline must come from
  the vendor's design files or a measured unit before any pocket is cut.

## 10.6 Sources added for this section

- [splitkb Aurora Helix](https://splitkb.com/products/aurora-helix) — ortho split, up to 64 keys, number row, MX spacing, nice!nano listed compatible, QMK or ZMK, kit contents and price
- [splitkb Halcyon Lily58](https://splitkb.com/products/halcyon-lily58) · [Halcyon Elora](https://splitkb.com/products/halcyon-elora) — 58 / 62 keys, number row, pre-soldered PCB assembly, €61.94 ex-VAT
- [Wireless Halcyon Controller](https://splitkb.com/products/wireless-halcyon-controller) — nRF52840, ZMK, €28.92 single / €61.94 pair + module kit / €95.00 upgrade bundle
- [splitkb: Introducing Halcyon Wireless](https://blog.splitkb.com/introducing-halcyon-wireless/) — drop-in replacement for the wired RP2040 controller, CE/FCC-certified module, coin-cell vs LiPo power boards, dongle
- [splitkb Aurora series build guide](https://docs.splitkb.com/product-guides/aurora-series) · [keyboard kits index](https://splitkb.com/collections/keyboard-kits)
- [MoErgo Glove80 technical specification](https://www.moergo.com/pages/glove80-technical-specification) — 372 × 168 × 60 mm, 600 g, 80 keys, BLE 5.0, ZMK, 2 × nRF52840 · [pricing](https://www.moergo.com/collections/glove80-keyboards)
- [Dygma tech specs](https://support.dygma.com/hc/en-us/articles/22597472943517-What-are-the-tech-specs-of-Dygma-keyboards) — Defy 164.16 × 210.02 × 35.36 mm, ~1 kg, BT up to 5 devices + RF
- [Kinesis Advantage360 Professional](https://kinesis-ergo.com/shop/adv360pro/) — 76 keys, BLE, ZMK, assembled
- [Keebio Nyquist (QMK readme)](https://cgit.krebsco.de/qmk_firmware/plain/keyboards/keebio/nyquist/readme.md) — "split 60% split 5x12 ortholinear keyboard made and sold by Keebio", Pro Micro hardware
- [Keebio Quefrency](https://keeb.io/collections/all/products/quefrency-rev-2-pcbs-60-65-split-staggered-keyboard) — split 60/65% with number row, **wired only**, no vendor wireless
- [Mechboards Lily58 kit](https://mechboards.co.uk/products/lily58-kit?country=US) — confirms the pattern: pre-built variants are **wired** RP2040; the **wireless** variant is a solder kit
- [nice!nano v2 (Mechboards)](https://www.mechboards.co.uk/products/nice-nano-v2) — 34.1 × 18.3 × 3.3 mm, nRF52840 · [reach.dog pricing](https://reach.dog/shop/little-keyboards/products/nice-nano) $24.99
