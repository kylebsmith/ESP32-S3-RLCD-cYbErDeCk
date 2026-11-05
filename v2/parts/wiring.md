# Wiring the control surface to the ESP32-S3-RLCD-4.2

*An enclosure and assembly input. The user asked a direct question — "does this
dev board have the proper breakout on the back? Check if there are enough pins
and the correct pins." This document answers it from the vendor schematic, the
vendor silkscreen, the Espressif datasheet, the Adafruit board schematics and
the I²C specification. Every number carries a source tag. Anything that could
not be traced to a primary source is marked **NOT SOURCED** and listed in §8 —
it is not estimated.*

---

## 0. Verdict up front

1. **Yes — the breakout is on the back, and it is a 2 × 8 FEMALE 2.54 mm
   header, not a STEMMA QT socket.** `[VENDOR W1]` `[SCHEM W4 P1]` It carries
   `3V3 · GND · GP0 · GP1 · GP2 · GP3 · GP17 · GP18` on one row and
   `VBUS · GND · GP19 · GP20 · TXD · RXD · SDA · SCL` on the other. **There is
   no JST-SH 4-pin I²C connector anywhere on this board.** The only JST-SH on
   it is a 2-pin `SH1.0` for the RTC backup cell. §1.

2. **The board's I²C bus is already broken out, on GPIO13 (SDA) and GPIO14
   (SCL), at header pins 14 and 16.** `[SCHEM W4]` That is two pins for the
   whole STEMMA chain, and they clash with nothing in the committed list. The
   panel is SPI on GPIO 5/11/12/40/41 and shares nothing with it. §2.1.

3. **Three ADC-capable pins are free, and they are the right ones.** GPIO1,
   GPIO2 and GPIO3 are `ADC1_CH0/CH1/CH2` `[DS E8 §App.]`, they are blank in
   Waveshare's own allocation table `[VENDOR W3]`, and they are on header pins
   7, 9 and 11. **ADC1 is the unit that survives Wi-Fi; ADC2 is the one that
   does not** `[IDF]` — and every free pin here is ADC1. The pots need **no
   extra board**. §3.

4. **The ADS1115 is not needed and should not be bought.** The native route
   costs nothing, uses pins that are already there, and 12 bits with ±4 LSB DNL
   `[DS E8 T5-5]` is **32 counts per MIDI CC step** — the resolution argument
   against the native ADC collapses the moment the destination is 7-bit MIDI.
   The ADS1115 would also add a fifth 10 kΩ pull-up pair to a bus that has
   little room left. §3.3.

5. **The pot divider must NOT run to the full 3.3 V rail without a top-leg
   resistor.** The S3's widest attenuation has an *effective measurement range
   of 0 – 2900 mV* `[DS E8 T5-6]`. A pot straight across 3V3 saturates for the
   **top 36° of a 300° sweep**. One **1.5 kΩ** resistor in each pot's top leg
   puts full travel at 2870 mV and fixes it for three resistors. §3.5.

6. **The pull-up arithmetic is fine, and it is fine for a reason nobody
   expects.** `parts.md` §8.3 feared that nine boards' 10 kΩ pull-ups in
   parallel would sink the bus. They do not — but **the Waveshare board already
   fits 2.2 kΩ** (R20/R22) `[SCHEM W4]`, which dominates everything else. The
   effective pull-up with three encoders and the slider is **1170 Ω, sinking
   2.48 mA at V_OL = 0.4 V**, inside the 3 mA I²C limit `[I2C T9]`. **Cut
   nothing.** But the headroom is exactly **one more 10 kΩ board**; a sixth
   breaks spec at 3.06 mA. §2.4.

7. **The real risk on this chain is rise time, not pull-up current, and the
   culprit is the BSS138 level shifters on the encoders.** Each 5880 puts a
   BSS138 drain on each bus line `[SCHEM A1]`, and the onsemi capacitance curve
   shows C_OSS climbing from 13 pF at 25 V to **40–90 pF below 1 V**
   `[DS B1 F8]` — which is exactly where an I²C line lives. Three encoders can
   therefore put 135–270 pF on each line before a single millimetre of cable.
   The 400 kHz budget is 303 pF. **Run the bus at 100 kHz**, where the budget
   is 4× larger and the cost is nothing: polling four seesaw devices at 100 Hz
   uses 32 % of a 100 kHz bus. §2.5.

8. **Power is not close to a limit.** Seven NeoPixels at Adafruit's worst case
   are **420 mA** `[AF uber]`; the rail is a **TPS63020 buck-boost rated 2 A at
   3.3 V for V_IN > 2.5 V** `[DS T1]`, and an 18650 never goes below 3.0 V. The
   regulator also has its own soft-start — the current limit ramps from 400 mA
   `[DS T1 §7.4.1]` — so nothing needs one added. The *first* thing to run out
   is the **JST-SH contact at 1 A** `[JST]`, not the regulator. §4.

9. **The smallest part that makes this build plug-together is one cable:
   Adafruit 4209**, STEMMA QT JST-SH 4-pin to male Dupont headers, 150 mm
   `[AF 4209]`. Four of its pins go into header 1 (3V3), 3 (GND), 14 (SDA) and
   16 (SCL) and the chain starts there. That is the whole gap. §5.

---

## 0.1 Sources

| ID | Source | How read |
|----|--------|----------|
| W1 | `docs.waveshare.com/ESP32-S3-RLCD-4.2` — product page, Features / Onboard Resources | fetched 2026-11-05, text extracted |
| W2 | W1 → board photo `ESP32-S3-RLCD-4.2-HW-*.webp` | fetched, read as an image; silkscreen read directly |
| W3 | W1 → `ESP32-S3-RLCD-4.2-IntfIntro-*.webp` — Waveshare's own GPIO allocation table | fetched, read as an image |
| W4 | `files.waveshare.com/wiki/ESP32-S3-RLCD-4.2/ESP32-S3-RLCD-4.2-schematic.pdf` | fetched; words extracted with coordinates (`pdftotext -bbox-layout`) and the netlist reconstructed by position |
| W5 | W1 → `ESP32-S3-RLCD-4.2-ProductSize-V2-*.webp` — V2 dimension drawing | fetched, read as an image |
| E8 | `espressif.com/.../esp32-s3_datasheet_en.pdf`, ESP32-S3 Series Datasheet **v2.2** | fetched, text extracted |
| IDF | `docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/adc/adc_oneshot.html` | fetched 2026-11-05 |
| I2C | NXP **UM10204**, I²C-bus specification and user manual (2021) | fetched, text extracted |
| T1 | TI **SLVS916I**, TPS63020/TPS63021, Jul 2010 rev Oct 2019 | fetched, text extracted |
| A1 | Adafruit **I2C QT Rotary Encoder** schematic, `adafruit_products_REI2C_sch.png` (5146 × 3546) | fetched, read as an image |
| A2 | Adafruit **NeoSlider** schematic, `adafruit_products_NS_sch.png` | fetched, read as an image |
| A3 | Adafruit **ADS1115 ADC rev C** schematic, `arduino_compatibles_schem.png` | fetched, read as an image |
| B1 | onsemi **BSS138** datasheet, `bss138-d.pdf` | fetched; Table p2 and Figure 8 read |
| AF | Adafruit product pages 5880, 5295, 4209 and the NeoPixel Überguide *Powering NeoPixels* | fetched 2026-11-05 |
| JST | `jst-mfg.com` SH series product page | fetched 2026-11-05 |
| R | this repository — `docs/HARDWARE.md`, `firmware/components/st7305/st7305.c`, `firmware/main/main.c`, `firmware/main/battery.c`, `firmware/components/net/net.c` | read in place (GPIO numbers and Wi-Fi usage only) |

Tags: `[VENDOR]` vendor states it · `[SCHEM]` read off a schematic ·
`[SILK]` read off the board silkscreen in a vendor photo · `[DS]` datasheet ·
`[CHART]` read off a datasheet graph, lower confidence · `[DERIVED]` computed
here from tagged values · `[AF]` Adafruit page text · `[OPEN]` not established.

---

## 1. What the board physically exposes

### 1.1 Everything on the board, and where it is

Read off the vendor photo `[W2]` with the board back-side up, the USB-C and TF
card on the **left** edge and the 18650 holder across the **bottom**:

| # | Item | Where | Type |
|---|---|---|---|
| 1 | ESP32-S3-WROOM-1-N16R8 | upper right, antenna out to the right edge | soldered module, castellated pads on its long edges |
| 4 | **BOOT** button | top edge, left of centre | side button |
| 5 | **PWR** button | top edge, centre | side button |
| 6 | **KEY** button | top edge, right of centre | side button |
| 9 | **SPK** header | left-centre, below the TF slot | **MX1.25 2-pin** `[VENDOR W1]` |
| 10 | **RTC** backup cell | left-centre, right of SPK | **PH1.0 2-pin**, "supports only PH1.0 rechargeable RTC battery" `[VENDOR W1]`; drawn as `SH1.0 2P` J7 on the schematic `[SCHEM W4]` |
| 11 | **2 × 8 pin 2.54 mm FEMALE header** | centre of the board, immediately above the battery holder | the expansion breakout — §1.2 |
| 12 | 18650 battery holder | the whole bottom half | sprung metal contacts, cell inserted from the back |
| 13 | MIC1 / MIC2 | top-left and top-right corners | — |
| 14 / 15 | CHG / WRN LEDs | left edge | — |
| 16 | **USB-C** | left edge, lower | **one port only** |
| 17 | TF card slot | left edge, upper | push-push |
| — | `DN` / `DP` test points | centre-left, below the Waveshare logo | 2 exposed pads on USB D−/D+; drawn as TP3/TP4 `[SCHEM W4]` |
| — | `INT` / `CLK` test points | upper centre, beside the RTC chip | 2 exposed pads |

Board outline **92.50 × 70.10 mm**, mounting **4 × M2.50 on 85.50 × 62.10**,
3.50 mm in from the right edge and 4.50 mm up from the bottom; total assembled
thickness **14.1 mm** on the V2 revision `[VENDOR W5]`. *(The 4 × M2.50 callout
on that drawing is dimensioned on the assembled unit, not on the bare PCB —
treat the hole pitch as assembly-level until a caliper confirms it on the bare
board. §8.)*

**Nothing is on the front but the panel.** The front face is the 4.2″ reflective
LCD with its acrylic spacer. Every connector, button and pad listed above is on
the back. That is the answer to the user's question in one line: **the breakout
really is on the back, facing the carrier plate.**

### 1.2 The 2 × 8 header, pin by pin

The schematic designator is **P1**, annotated `2*8 (2.54mm)` `[SCHEM W4]`. The
pin order below was read from the schematic by coordinate and then **confirmed
independently against the silkscreen in the vendor photo** `[SILK W2]` — the
two agree on all sixteen.

With the board back-up and the battery holder at the bottom, **the odd row is
the lower one** (silk `3V3 GND GP0 GP1 GP2 GP3 GP17 GP18`, pin 1 at the
USB-C end) and **the even row is the upper one** (silk
`VBUS GND GP19 GP20 TXD RXD SDA SCL`).

| Pin | Silk | Net | GPIO | Committed to | Free? |
|----|------|-----|------|---|---|
| 1 | `3V3` | VCC3V3 | — | TPS63020 output rail | **power out** |
| 2 | `VBUS` | VBUS | — | USB-C 5 V, present only when plugged | **power out, intermittent** |
| 3 | `GND` | GND | — | — | **ground** |
| 4 | `GND` | GND | — | — | **ground** |
| 5 | `GP0` | GPIO0 | 0 | **BOOT button** (Key1 + 100 nF) and a boot strapping pin `[DS E8 T3-1]` | no |
| 6 | `GP19` | USB'_N | 19 | **USB-C D−**, via R13 22 Ω | no (if you want USB) |
| 7 | `GP1` | GPIO1 | 1 | **nothing** | **FREE — ADC1_CH0** |
| 8 | `GP20` | USB'_P | 20 | **USB-C D+**, via R15 22 Ω | no (if you want USB) |
| 9 | `GP2` | GPIO2 | 2 | **nothing** | **FREE — ADC1_CH1** |
| 10 | `TXD` | U0TXD | 43 | UART0 TX / ROM console | **FREE in practice** |
| 11 | `GP3` | GPIO3 | 3 | **nothing**; JTAG-source strapping pin, *Ignored* at default eFuses `[DS E8 T3-5]` | **FREE — ADC1_CH2** |
| 12 | `RXD` | U0RXD | 44 | UART0 RX | **FREE in practice** |
| 13 | `GP17` | GPIO17 | 17 | **nothing fitted** — R7 between SDCS and GPIO17 is marked **NC** `[SCHEM W4]` | **FREE (digital only, ADC2)** |
| 14 | `SDA` | ESP32_SDA | 13 | **the board I²C bus** | **use it — that is the point** |
| 15 | `GP18` | GPIO18 | 18 | **KEY button** (Key4 + 100 nF) | no |
| 16 | `SCL` | ESP32_SCL | 14 | **the board I²C bus** | **use it** |

It is a **socket**, so whatever you plug in needs male pins. See §6.3 for what
that means for the carrier plate.

### 1.3 The complete GPIO table

Waveshare publish their own allocation table `[W3]`; the column headings are
`LCD · USB · RTC · SD · ES8311 · QMI8658C · SYS · OUT`. It is reproduced here
merged with what this repository already claims `[R]`, with the ADC unit from
the Espressif datasheet `[DS E8]`. `QMI8658C` and the `TP_*` nets are template
columns inherited from a sibling product — see the note below the table.

| GPIO | ADC | Vendor assignment | Repo use | On P1? | Free? |
|---|---|---|---|---|---|
| 0 | — | SYS | BOOT button | pin 5 | no — strapping + button |
| 1 | **ADC1_CH0** | *(blank)* | — | **pin 7** | **YES** |
| 2 | **ADC1_CH1** | *(blank)* | — | **pin 9** | **YES** |
| 3 | **ADC1_CH2** | *(blank)* | — | **pin 11** | **YES** |
| 4 | ADC1_CH3 | `BAT_ADC` | battery sense | no | no |
| 5 | ADC1_CH4 | `LCD_RS` | panel DC `[R st7305.c]` | no | no |
| 6 | ADC1_CH5 | `LCD_TE` | tearing-effect in `[R]` | no | no |
| 7 | ADC1_CH6 | `TP_INT` → LCD FPC pin 18 | — | no | **electrically free, module pad only** |
| 8 | ADC1_CH7 | `I2S_DSDIN` | audio | no | no |
| 9 | ADC1_CH8 | `I2S_SCLK` | audio | no | no |
| 10 | ADC1_CH9 | `I2S_ASDOUT` | audio | no | no |
| 11 | ADC2_CH0 | `LCD_SCL` | panel SPI SCK `[R]` | no | no |
| 12 | ADC2_CH1 | `LCD_SDA` | panel SPI MOSI `[R]` | no | no |
| 13 | ADC2_CH2 | `ESP32_SDA` + `RTC_SDA` + `TP_SDA` | — | **pin 14** | **I²C — use as I²C** |
| 14 | ADC2_CH3 | `ESP32_SCL` + `RTC_SCL` + `TP_SCL` | — | **pin 16** | **I²C — use as I²C** |
| 15 | ADC2_CH4 | `RTC_INT` | — | no | no |
| 16 | ADC2_CH5 | `I2S_MCLK` | audio | no | no |
| 17 | ADC2_CH6 | `OUT` only; R7 to SDCS is **NC** | — | **pin 13** | **YES, digital only** |
| 18 | ADC2_CH7 | `KEY` | KEY button `[R main.c]` | pin 15 | no |
| 19 | ADC2_CH8 | `USB'_N` | USB D− | pin 6 | only if USB is given up |
| 20 | ADC2_CH9 | `USB'_P` | USB D+ | pin 8 | only if USB is given up |
| 21 | — | `MOSI` (SD) | SD card | no | no |
| 22–37 | — | **do not exist / taken by flash + octal PSRAM** on the N16R8 module | — | no | no |
| 38 | — | `SCK` (SD) | SD card | no | no |
| 39 | — | `MISO` (SD) | SD card | no | no |
| 40 | — | `LCD_CS` | panel CS `[R]` | no | no |
| 41 | — | `LCD_RESET` | panel RST `[R]` | no | no |
| 42 | — | `TP_RESET` → LCD FPC pin 17 | — | no | **electrically free, module pad only** |
| 43 | — | `U0TXD` | ROM console | **pin 10** | **yes, in practice** |
| 44 | — | `U0RXD` | ROM console | **pin 12** | **yes, in practice** |
| 45 | — | `I2S_LRCK` | audio; VDD_SPI strapping `[DS E8 T3-1]` | no | no |
| 46 | — | `PA_CTRL` | audio amp enable; boot strapping | no | no |

**Two corrections to `docs/HARDWARE.md` fall out of this table.** They are
recorded here and left for whoever owns that file.

- HARDWARE.md says "The 23-pin LCD FPC carries only GND, VCC3V3, SCL, SDA, CS,
  RS, TE and RESET." The FPC is `FH35C-21S-0.3SHW` and its pins **16–20 are
  `TP_VCC`, `TP_RESET`, `TP_INT`, `TP_SDA`, `TP_SCL`** `[SCHEM W4]`. The
  backlight conclusion is unaffected — there is still no LEDA/LEDK — but it
  means the **I²C bus has a live stub running to the LCD connector**, to a touch
  layer this panel does not have. That stub is bus capacitance you did not
  choose, and it is already in the board. §2.5.
- `firmware/main/battery.c` leaves the battery-sense pin for the owner to
  declare. The schematic answers it: **`BAT_ADC` is GPIO4** (`ADC1_CH3`), fed
  from `VBAT` through **R21 = 200 kΩ ±1 % over R23 = 100 kΩ ±1 %**, i.e. a
  **3:1 divider, not 2:1** `[SCHEM W4]`. 4.2 V at the cell reads 1.400 V at the
  pin, comfortably inside the 0–2900 mV window. Recorded as a hardware fact.

### 1.4 What the board does not have

- **No STEMMA QT / Qwiic / 4-pin JST-SH connector.** Searched the whole
  schematic: the single `SH1.0` instance is J7, **2-pin**, the RTC backup cell
  `[SCHEM W4]`. The build is therefore *one soldered or crimped transition*
  followed by plug-together STEMMA — not plug-together end to end.
- **No reset button.** Three buttons only: BOOT (GPIO0), KEY (GPIO18) and PWR,
  and PWR is not a GPIO — it is a latch on Key3 driving AO3401/AO3400 FETs in
  the B+ path `[SCHEM W4]`, matching the vendor text "long press to power off,
  single click to power on" `[VENDOR W1]`. `CHIP_PU` has no button on it.
- **No second USB port.** The schematic draws two Type-C blocks, but only one
  carries real designators (H1, with TVS1 and the two 5.1 kΩ CC resistors); the
  second carries `J?`, `C?`, `D?`, `R?` — Altium's un-annotated marker
  `[SCHEM W4]`. The vendor text lists one "Type-C Interface" `[VENDOR W1]` and
  the photo shows one `[W2]`.
- **The one USB-C is native USB-OTG, not a UART bridge.** D−/D+ go through 22 Ω
  series resistors straight to GPIO19/GPIO20 `[SCHEM W4]`, which are the S3's
  own `USB_D−`/`USB_D+` `[DS E8]`. There is no CH343/CP210x/FT232 anywhere on
  the schematic. Flashing and the serial log run over USB-Serial-JTAG on those
  two pins. **Consequence for the build: do not take GPIO19/20 for anything.**
- **No battery JST connector.** The main cell is an **18650 in a holder**
  `[VENDOR W1]` `[W2]`, with an ETA6098 charger and an S8261 + 8205 protection
  pair. The only battery *connector* is the PH1.0 RTC backup.

### 1.5 Pins you can only get by soldering

**GPIO7** and **GPIO42** are free — they carry `TP_INT` and `TP_RESET`, which
land on LCD FPC pins 18 and 17 of a panel with no touch layer. They are not on
P1. The only ways to them are the WROOM-1 module's castellated pads, which on a
soldered-down module are exposed solder fillets along the module's long edges,
or the 0.3 mm FPC connector. **Neither is a breakout.** Treat these as a
last-resort reserve, not as capacity.

---

## 2. The I²C question

### 2.1 The pins, and confirming the panel is not on them

`ESP32_SDA = GPIO13`, `ESP32_SCL = GPIO14` `[SCHEM W4]` `[W3]`, out at **P1
pins 14 and 16**.

**The panel does not share this bus, and the net names are a trap.** Waveshare
call the panel's lines `LCD_SCL` (GPIO11) and `LCD_SDA` (GPIO12) — but those
are the **SPI clock and MOSI**, which `firmware/components/st7305/st7305.c`
drives as `PIN_SCK 11` / `PIN_MOSI 12` on a `spi_bus_config_t` `[R]`, with CS on
40, DC on 5 and RST on 41. Different pins, different peripheral, no interaction.
`docs/HARDWARE.md` is right and the confirmation is two independent reads.

Neither GPIO13 nor GPIO14 appears in the committed list in `docs/HARDWARE.md`
("GPIO 5, 11, 12, 18, 40 and 41 are refused by name"). **No clash.**

### 2.2 Who is already on the bus before you plug anything in

| Device | Address | Source |
|---|---|---|
| PCF85063ATL RTC | 0x51 | `[SCHEM W4]` places U6 on ESP32_SDA/SCL; address `[OPEN]` here, see §8 |
| SHTC3 temp/humidity | 0x70 | `[SCHEM W4]` places U5 on the same pair; address `[OPEN]` |
| ES8311 audio codec | — | `[SCHEM W4]` U9 on the same pair; address `[OPEN]` |
| ES7210 ADC | — | `[SCHEM W4]` on the same pair; address `[OPEN]` |
| *(dead stub)* | — | LCD FPC pins 19/20, `TP_SDA`/`TP_SCL`, no touch layer fitted |

**Four devices are already there.** `parts.md` §8.3 budgeted nine devices on an
empty bus; the real count is nine *external* on top of four *internal*, and the
addresses of the four internal ones must be checked against the map before
anything is jumpered. The map in `parts.md` §8.3 uses 0x29–0x4A, so a PCF85063
at 0x51 and an SHTC3 at 0x70 would sit clear of it — **but those two addresses
are asserted from part knowledge, not from a datasheet read in this session, so
they are in §8 until `i2cdetect` or a datasheet confirms them.**

### 2.3 Addresses for the four boards this build actually uses

`parts.md` §8.3 already allocates these and nothing here changes it. Restated
for the smaller part list:

| Address | Device | Jumper |
|---|---|---|
| 0x30 | NeoSlider 5295 | none — as shipped |
| 0x36 | Encoder 5880 #1 | none |
| 0x37 | Encoder 5880 #2 | **close A0** (solder, reversible) |
| 0x38 | Encoder 5880 #3 | **close A1** (solder, reversible) |

Four devices, four addresses, two reversible solder jumpers, **no knife work**
— the destructive cut-to-change jumpers belong to the NeoSlider, and with one
slider it stays at its default 0x30 and is never cut. The encoder's address
jumpers are SJ1/SJ8/SJ9 to GND on the schematic `[SCHEM A1]`.

### 2.4 Pull-ups — the arithmetic, and why nothing gets cut

**What is actually fitted, read off schematics rather than assumed:**

| Board | Per line | Where | Source |
|---|---|---|---|
| ESP32-S3-RLCD-4.2 | **2.2 kΩ** (R20 on SDA, R22 on SCL, to VCC3V3) | in the codec block | `[SCHEM W4]` |
| Encoder 5880 × 3 | **10 kΩ to VCC** on the bus side, plus a second 10 kΩ to 3.3 V on the seesaw side of a BSS138 pair | block labelled "I2C Pullups & Shift" | `[SCHEM A1]` |
| NeoSlider 5295 | **10 kΩ to VCC** (R3, R4). **No level shifter** — the seesaw is directly on the bus | same block name, resistors only | `[SCHEM A2]` |
| *(ADS1115 1085, if ever used)* | **10 kΩ to VCC** (R16$2 SDA, R16$3 SCL) | — | `[SCHEM A3]` |

Only the **bus-side** resistors load the line. The encoders' second 10 kΩ pair
sits behind the BSS138 and is invisible to the chain.

```
Rp = 1 / ( 1/2200  +  4 × 1/10000 )  =  1170 Ω            [DERIVED]
I_sink at V_OL = 0.4 V  =  (3.3 − 0.4) / 1170  =  2.48 mA [DERIVED]
```

The I²C spec requires a device to pull to **V_OL ≤ 0.4 V at 3 mA**
`[I2C T9]`, and gives `Rp(min) = (V_DD − V_OL,max) / I_OL` = 2.9 / 3 mA =
**967 Ω** `[I2C §7.1 Eq.2]`.

| Chain | Rp | I_sink @ 0.4 V | Verdict |
|---|---|---|---|
| board alone | 2200 Ω | 1.32 mA | — |
| **+ 3 encoders + slider (this build)** | **1170 Ω** | **2.48 mA** | **in spec, 21 % margin** |
| + ADS1115 | 1048 Ω | 2.77 mA | in spec, 8 % margin |
| + one more 10 kΩ board | 948 Ω | **3.06 mA** | **out of spec** |

**So: cut nothing, lift nothing.** `parts.md` §8.3's warning was the right
instinct aimed at the wrong number — the parallel 10 kΩ pull-ups are a minor
term next to the 2.2 kΩ the dev board already carries. But the result is also a
**hard ceiling: five external STEMMA boards, and the fifth is the last one.**
If a sixth is ever wanted, lift the pull-ups on one board (the NeoSlider's are
plain resistors, R3/R4, easy to remove) rather than cutting anything.

The ESP32-S3 can sink 28 mA at V_OL = 0.495 V `[DS E8 T5-4]`, so the master is
nowhere near its limit; the binding device is whichever seesaw has the weakest
open-drain stage, and the 3 mA spec figure is the honest proxy for it.

### 2.5 Bus capacitance, and why this chain wants 100 kHz

`tr = 0.8473 × Rp × Cb` `[I2C §7.1]`, with `tr ≤ 300 ns` for Fast-mode and
`≤ 1000 ns` for Standard-mode `[I2C T10]`.

With **Rp = 1170 Ω**:

| Mode | tr budget | **Cb allowed** |
|---|---|---|
| 400 kHz Fast-mode | 300 ns | **303 pF** `[DERIVED]` |
| 100 kHz Standard-mode | 1000 ns | 1009 pF `[DERIVED]`, but the spec caps Cb at **400 pF** `[I2C T10]` |

**Now the load.** The spec allows each device 10 pF `[I2C T9, Ci]`:

| Contributor | Per line | Source |
|---|---|---|
| ESP32-S3 pin | 2 pF typ | `[DS E8 T5-4, C_IN]` |
| 4 on-board slaves (RTC, SHTC3, ES8311, ES7210) | ≤ 40 pF | `[I2C T9]` spec ceiling |
| NeoSlider seesaw, directly on the bus | ≤ 10 pF | `[I2C T9]` |
| **3 × BSS138 drain (the encoders)** | **135 – 270 pF** | `[DS B1 F8]` `[CHART]` |
| LCD FPC stub + board traces | **NOT SOURCED** | §8 |
| 4 STEMMA QT cables, ~300 mm of chain | **NOT SOURCED** | §8 |
| **Subtotal before any wiring** | **187 – 322 pF** | `[DERIVED]` |

**The BSS138 number is the finding.** The onsemi table gives C_OSS = 13 pF, but
at `V_DS = 25 V, V_GS = 0 V` `[DS B1 p2]`. Figure 8 of the same datasheet plots
C_OSS against V_DS and it climbs steeply as V_DS falls — roughly **40–45 pF at
1 V and above 60 pF below 0.5 V** `[CHART, read off DS B1 F8, ±10 pF]`. An I²C
line idles with both sides of the shifter at the rail, i.e. **V_DS ≈ 0**, which
is the worst point on that curve. Three encoders put three such drains on each
line. That single term can consume the entire 400 kHz budget on its own.

**Decision: run the bus at 100 kHz.** It is free. At 100 kHz:

- `tr` at Cb = 400 pF is **397 ns** — 40 % of the 1000 ns budget `[DERIVED]`.
- Even at an out-of-spec 600 pF, `tr` = 595 ns, still inside the timing
  `[DERIVED]`.
- A seesaw encoder read is about **9 bytes** (2 address + 3 write + restart +
  4 read) = 81 bit-times = **0.81 ms**. Four devices swept = **3.24 ms**.
  Sweeping all four at **100 Hz uses 32 % of the bus** `[DERIVED]`. For three
  knobs, a slider and a human hand that is not a constraint. *(The seesaw's own
  required delay between a write and the following read is a firmware matter
  and out of scope here.)*

**If 400 kHz is ever wanted, it is a measurement, not an opinion:** scope SCL at
the far end of the chain and require `tr ≤ 300 ns` from 0.3 V_DD to 0.7 V_DD.
The lever that would make it comfortable is **fewer BSS138 drains** — one
5752 Quad Rotary in place of three 5880s — but whether the 5752 carries
shifters is **NOT SOURCED** (§8), and the layout has already settled on three
separate encoders.

### 2.6 Chain order

Shortest total wire, which is also the cleanest thing to look at through the
acrylic:

```
P1 pins 1/3/14/16  ──QT──  ENC 1 (0x36) ──QT──  ENC 2 (0x37) ──QT──  ENC 3 (0x38) ──QT──  SLIDER (0x30)
```

Every one of these boards has **two** STEMMA QT sockets, on the back face at the
two short ends, openings facing outward along the long axis `[STEP, parts.md
§1.1]`. Daisy-chaining is in-one-socket-out-the-other; no splitter, no hub.

---

## 3. The three analogue pots

### 3.1 Which pins can read an analogue voltage, and which are free

ESP32-S3 ADC map, from the datasheet's pin table `[DS E8]`:

```
ADC1_CH0 … CH9  =  GPIO1  … GPIO10
ADC2_CH0 … CH9  =  GPIO11 … GPIO20
```

Cross that against §1.3 and **exactly three ADC-capable pins are free and
reachable**:

| GPIO | ADC channel | P1 pin | Why it is safe |
|---|---|---|---|
| **1** | **ADC1_CH0** | **7** | blank in the vendor allocation table `[W3]`; appears nowhere on the schematic but at the module and at P1 `[SCHEM W4]`; not a strapping pin |
| **2** | **ADC1_CH1** | **9** | same |
| **3** | **ADC1_CH2** | **11** | same, **except** it is the JTAG-source strapping pin — see below |

A fourth free pin, **GPIO17 on P1 pin 13**, is `ADC2_CH6` and therefore not
usable for a pot (§3.2). Keep it as the spare digital line.

**GPIO3 and the strapping question, settled from the datasheet.** GPIO3 selects
the JTAG signal source. Table 3-5 lists the combinations, and with the default
eFuses — `EFUSE_DIS_PAD_JTAG = 0`, `EFUSE_DIS_USB_JTAG = 0`,
`EFUSE_STRAP_JTAG_SEL = 0`, which the datasheet states is the shipped state —
the GPIO3 column reads **"Ignored"** `[DS E8 T3-5]`. Its default configuration
is **"Floating"** with no bit value `[DS E8 T3-1]`. **A pot divider on GPIO3
changes nothing at boot on a chip whose eFuses have not been burnt.** The one
circumstance that would break this is someone burning
`EFUSE_STRAP_JTAG_SEL = 1`, which is one-way and would have to be deliberate.

### 3.2 ADC2 and Wi-Fi — the trap, and why it does not bite here

ESP-IDF states it plainly under *Hardware Limitations*: **"ADC2 is also used by
Wi-Fi."** `[IDF]` The oneshot driver arbitrates, but a read that loses the
arbitration returns **`ESP_ERR_TIMEOUT`** and "the ADC raw result is invalid"
`[IDF]`. Espressif's own ADC characterisation is taken **"with disabled Wi-Fi"**
`[DS E8 §5.5]`.

**This firmware runs Wi-Fi.** `firmware/components/net/net.c` creates both a STA
and an AP netif and calls `esp_wifi_init`/`esp_wifi_start`, and
`firmware/components/ensemble/ensemble.c` brings the radio up independently
`[R]`. So ADC2 is live territory and must be avoided for a continuously-polled
control.

**It does not bite, because every free pin on this board is ADC1.** GPIO1, 2
and 3 are `ADC1_CH0/CH1/CH2`. ADC1 is untouched by the radio. The board also
already uses `ADC1_CH3` for battery sense (§1.3), so with three pots the deck
uses four of ADC1's ten channels and has six spare.

### 3.3 Native ADC vs an ADS1115, compared honestly

| | **3 × native ADC1** | **1 × ADS1115 (1085)** |
|---|---|---|
| Extra boards | **none** | one 25.40 × 17.78 × 4.53 mm board `[STEP, parts.md]` |
| Extra bus devices | none | one, at 0x48 |
| Extra pull-up load | none | **+10 kΩ/line** `[SCHEM A3]` → Rp 1170 → 1048 Ω, margin 21 % → 8 % (§2.4) |
| Resolution | 12 bit, 4096 counts over 0–2900 mV | 16 bit, programmable gain |
| Linearity | DNL ±4 LSB, INL ±8 LSB `[DS E8 T5-5]` | better, by a lot |
| Calibrated total error | **±10 mV at ATTEN2, ±50 mV at ATTEN3** `[DS E8 T5-6]` | — |
| Rate | sampling rate ≤ 100 kSPS `[DS E8 T5-5]` | 860 SPS shared across channels `[AF]` |
| Pins physically reachable? | **yes — P1 pins 7, 9, 11** | yes, on the QT chain |
| **Analogue wire the carrier must route** | 3 wiper runs, pot cluster → P1 | **still 3 wiper runs**, pot cluster → ADS1115; only shorter, and only if the ADS1115 is mounted in the pot cluster rather than at the board |
| Parts count | **0** | 1 board + 1 QT cable + 4 standoffs + 1 address to track |

**Recommendation: native ADC1 on GPIO1, GPIO2, GPIO3.** Three reasons, in
order of weight:

1. **The resolution argument does not survive contact with the destination.**
   4096 counts spread over 128 MIDI CC values is **32 ADC counts per CC step**
   `[DERIVED]`. The S3's ±4 LSB DNL is **one eighth of a CC step**. Even the
   ±50 mV total error at the widest attenuation `[DS E8 T5-6]` is a static
   offset, not jitter, and a knob is a relative control anyway. 16 bits buys
   nothing a listener can hear.
2. **It is zero extra parts**, which is the user's stated constraint.
3. **The ADS1115 does not remove the analogue runs, it only relocates their
   far end.** A pot is three wires wherever it terminates. The only version
   that genuinely shortens them puts the ADC inside the pot cluster — which
   adds a board to the most crowded part of the carrier plate, in exchange for
   resolution that cannot be used.

The honest cost of the native route: three analogue wires crossing the carrier
plate, and they must be routed away from the NeoPixel data line and the panel
SPI (§6.2). That is a layout discipline, not a part.

### 3.4 Pin assignment

| Pot | GPIO | ADC | P1 pin | Wire colour suggestion |
|---|---|---|---|---|
| POT 1 | **GPIO1** | ADC1_CH0 | **7** | — |
| POT 2 | **GPIO2** | ADC1_CH1 | **9** | — |
| POT 3 | **GPIO3** | ADC1_CH2 | **11** | — |

### 3.5 What each pot needs electrically

A panel-mount pot is a **three-terminal divider**: one end to a reference, the
wiper to the ADC, the other end to ground. `parts.md` §5.3 already specifies the
part — **10 kΩ linear, taper B** — and that value is right here too: low enough
that the ADC's input leakage is irrelevant, high enough that three of them cost
under a milliamp.

**The circuit, per pot:**

```
   VCC3V3 (P1 pin 1)
        │
      [ 1.5 kΩ ]          <-- the ceiling resistor; see below
        │
        ├──────────── CW end  ┐
                              │
        10 kΩ linear pot      ├── wiper ───┬──── GPIO1 / 2 / 3  (P1 pin 7 / 9 / 11)
                              │            │
        ├──────────── CCW end ┘         [ 100 nF ]
        │                                  │
   GND (P1 pin 3 or 4) ────────────────────┴──── GND
```

**Why the 1.5 kΩ.** At the widest attenuation the S3's *effective measurement
range is 0 – 2900 mV* `[DS E8 T5-6]`. A pot fed from 3.3 V delivers up to
3300 mV, so the **top 400 mV — 12 % of travel, 36° of a 300° shaft rotation
`[DS alpha RD901F, parts.md §5.2]` — reads full scale and the knob goes dead**.
That is a defect you would feel on the first sweep. A 1.5 kΩ resistor in the top
leg puts full travel at

```
3300 mV × 10000 / (10000 + 1500) = 2870 mV      [DERIVED]
```

— just under the ceiling, with 30 mV to spare for tolerance. 1.2 kΩ gives
2946 mV, which is over; 1.8 kΩ gives 2797 mV, which works but throws away 2.5 %
of range. **Use 1.5 kΩ, 1 % if you have it.** Current per pot becomes
3.3 V / 11.5 kΩ = **287 µA**; three pots draw **0.86 mA** `[DERIVED]`.

**Why the 100 nF.** Espressif's own ADC characterisation is specified **"with an
external 100 nF capacitor connected to the ADC"** `[DS E8 §5.5]` — the numbers
in Table 5-5 are not valid without it. Fit it at the **ADC end** of the wiper
run, not at the pot, so it also terminates whatever the wire picked up crossing
the carrier plate. The wiper's worst-case source impedance with this network is
about **2.8 kΩ at mid-travel** `[DERIVED]`, giving τ = 2.8 kΩ × 100 nF =
**280 µs** — settled in roughly 1.2 ms, which for a hand on a knob is instant.

**Three wires per pot, nine wires total**, of which the three 3V3 ends and the
three grounds can be bussed in the pot cluster so only **five** reach P1: one
3V3, one GND, three wipers.

---

## 4. Power

### 4.1 The NeoPixel census

| Board | Qty | NeoPixels each | Total | Source |
|---|---|---|---|---|
| Encoder 5880 | 3 | **1** | 3 | `[AF 5880]` "There's a NeoPixel on board"; `[SCHEM A1]` one WS2812B |
| NeoSlider 5295 | 1 | **4** | 4 | `[AF 5295]` "four under-lighting NeoPixels"; `[SCHEM A2]` WS2812B chain |
| | | | **7** | |

Adafruit's figures: *"Each individual NeoPixel draws up to 60 milliamps at
maximum brightness white (red + green + blue)"*, and for planning *"we've been
using 1/3 this (20 mA per pixel) as a gross rule of thumb with no ill effects"*
`[AF uber]`.

| Case | Per pixel | **7 pixels** |
|---|---|---|
| **Worst case, all white, full brightness** | 60 mA | **420 mA** |
| Adafruit's realistic rule of thumb | 20 mA | **140 mA** |
| A single colour at half brightness, which is what an instrument surface actually looks like | ~10 mA | **~70 mA** `[DERIVED]` |

### 4.2 Everything else the control surface draws

| Item | Current | Source |
|---|---|---|
| 7 NeoPixels, worst case | **420 mA** | above |
| 4 seesaw MCUs + their AP2112K LDOs | **NOT SOURCED** — §8. Bounded below 40 mA even at a generous 10 mA each | — |
| 3 pots | 0.86 mA | §3.5 `[DERIVED]` |
| I²C pull-ups, both lines low | ≤ 4.96 mA, and only while a line is actually low | §2.4 `[DERIVED]` |
| 4 "ON" LEDs (D1 on each 5880 via R7 10 kΩ; D2 on the 5295 via R1 10 kΩ, defeatable at SJ4) | ≤ 0.33 mA each by inspection of the 10 kΩ, so **≤ 1.3 mA total** | `[SCHEM A1, A2]` `[DERIVED]` |
| **Control surface total, worst case** | **≈ 466 mA** | `[DERIVED]` |
| **Control surface, realistic** | **≈ 185 mA** | `[DERIVED]` |

The unsourced seesaw term is under 9 % of the worst case, so **no conclusion
below depends on it.**

### 4.3 Can the rail carry it

**The regulator is a TPS63020DSJR** — a single-inductor buck-boost, `VSYS` in,
**`VCC3V3` out**, with a 1.5 µH 5 A inductor (L6) and 3 × 22 µF of output
capacitance `[SCHEM W4]`. **Header pin 1 taps `VCC3V3` directly**, the
converter's own output net `[SCHEM W4]`.

TI's numbers `[DS T1]`:

| | |
|---|---|
| Input range | 1.8 – 5.5 V |
| **Output current for V_IN > 2.5 V, V_OUT = 3.3 V** | **2 A** |
| Description also states | "Output currents up to 3 A are supported" |
| Average switch current limit I_SW | 3500 / **4000** / 4500 mA |
| Operating quiescent current | 25 µA |

An 18650 runs **4.2 V down to 3.0 V** `[R battery.c curve]`, which is above the
2.5 V threshold for the whole discharge — so **the full 2 A applies from full
to empty.** It is a buck-boost, so 3.3 V holds even as the cell falls below it.

The rest of the rail: ESP32-S3 Wi-Fi TX peaks at **340 mA** (802.11b, 1 Mbps,
21 dBm) `[DS E8 T5-7]`, plus the ST7305 panel (unmeasured — `docs/HARDWARE.md`
Open §2), plus the codecs and speaker amp.

```
340 mA (Wi-Fi TX peak)  +  466 mA (control surface worst case)  =  806 mA
against 2000 mA                                      →  1.19 A of headroom
```

and the Wi-Fi figure is a duty-cycled transmit peak, not a continuous draw.
**The rail is not the constraint and is not close to being the constraint.**

### 4.4 What *is* the first thing to run out: the connector

The whole chain's V+ passes through **one JST-SH contact** on the first QT
cable. JST rate the SH series at **1 A AC/DC (AWG #28)**, 50 V, 1 mm pitch,
conductor sizes AWG 32/30/28 `[JST]`.

```
466 mA worst case / 1000 mA contact rating  =  47 %      [DERIVED]
```

Acceptable, with the margin halved. **But it is the binding number in this
system, and it is worth stating plainly: if the surface ever grows — more
pixels, 5 V pixels, another lit board — the JST-SH contact fails before the
TPS63020 notices.** The fix if that day comes is to feed the chain's V+ from P1
pin 1 with a separate 24 AWG wire soldered to the *second* QT socket of the
first board, leaving the daisy-chain cables carrying only SDA/SCL/GND — not a
bigger regulator.

Adafruit do not publish the gauge of their QT cables `[AF 4209]`, so the voltage
drop along 300 mm of chain is **NOT SOURCED** (§8). It is very unlikely to
matter: at AWG 28 the round-trip drop over the first 100 mm cable at 466 mA
would be tens of millivolts.

### 4.5 Soft-start

**Nothing needs one added — the regulator already has it.** TI: *"After being
enabled, the device starts operating. The average current limit ramps up from an
initial 400 mA following the output voltage increasing… the output voltage
overshoot at start-up, as well as the inrush current, is kept at a minimum.
There is no timer implemented."* `[DS T1 §7.4.1]` The ramp is slaved to the
output voltage rather than to a clock, which is the behaviour you want when the
load is five boards' worth of bulk capacitance.

The one case to avoid is **hot-plugging a QT connector into a live rail**, which
bypasses the soft-start by definition: the chain's bulk caps charge through the
contact as it mates. Assemble the chain before power-up, and if a board must be
swapped in the field, power down first. No part is needed; it is a procedure.

---

## 5. The verdict

**Yes. The board has what this build needs, it is on the back, and the gap is
one cable.** The 2 × 8 female 2.54 mm header on the rear face — immediately
above the 18650 holder, accessible through the window in Waveshare's own back
shell — brings out 3V3, two grounds, the board's I²C bus on GPIO13/GPIO14 at
pins 14 and 16, and three completely uncommitted GPIOs at pins 7, 9 and 11 that
happen to be `ADC1_CH0`, `ADC1_CH1` and `ADC1_CH2` — the ADC unit that keeps
working while Wi-Fi is up, which is the single trap this question had to clear.
Three encoders and a NeoSlider need two of those pins between them; three pots
need the other three; that is five signal pins plus power and ground out of
sixteen, leaving GPIO17, both UART0 pins and VBUS spare. The board exposes **no
STEMMA QT connector of any kind** — the only JST-SH on it is a two-pin RTC
backup socket — so the one thing missing is the transition from 2.54 mm pins to
the 1 mm chain, and the smallest part that supplies it is **one Adafruit 4209
cable (STEMMA QT JST-SH 4-pin to male Dupont headers, 150 mm)**, whose four male
pins push straight into header pins 1, 3, 14 and 16. Everything downstream of
that cable is plug-together. Nothing else needs buying: no ADS1115, no I²C
multiplexer, no level shifter, no second regulator, no pull-up surgery — the
only passives in the whole build are **three 1.5 kΩ resistors and three 100 nF
capacitors**, one pair per pot, and they exist to stop the top 36° of each knob
reading full-scale rather than to make anything work at all.

---

## 6. The build, wire by wire

### 6.1 Connection list

| From | To | Wire |
|---|---|---|
| P1 pin 1 `3V3` | 4209 red → first encoder's QT socket | chain V+ |
| P1 pin 3 `GND` | 4209 black → first encoder's QT socket | chain GND |
| P1 pin 14 `SDA` | 4209 blue → first encoder's QT socket | GPIO13 |
| P1 pin 16 `SCL` | 4209 yellow → first encoder's QT socket | GPIO14 |
| ENC 1 second socket | ENC 2 | QT cable |
| ENC 2 second socket | ENC 3 | QT cable |
| ENC 3 second socket | NeoSlider | QT cable |
| P1 pin 1 `3V3` | pot cluster 3V3 bus → 1.5 kΩ → each pot CW end | second wire on pin 1 |
| P1 pin 4 `GND` | pot cluster GND bus → each pot CCW end, and the three 100 nF | — |
| POT 1 wiper | P1 pin 7 `GP1` | 100 nF to GND at the P1 end |
| POT 2 wiper | P1 pin 9 `GP2` | 100 nF to GND at the P1 end |
| POT 3 wiper | P1 pin 11 `GP3` | 100 nF to GND at the P1 end |

**Nine wires leave P1**: 2 × 3V3, 2 × GND, SDA, SCL, and three wipers. Plus four
QT cables between boards.

### 6.2 Three routing rules for the carrier plate

The wiring is meant to be seen through the acrylic, so these are composition
constraints as much as electrical ones:

1. **The three wiper runs are the only high-impedance lines in the build**
   (≈2.8 kΩ, §3.5). Keep them out of the NeoPixel data line's path — the
   WS2812B data edge is the fastest signal on the carrier plate — and off the
   panel's SPI, which runs at 20–24 MHz `[R, docs/HARDWARE.md]`. Running the
   three wipers as one parallel bundle with the pot-cluster ground return
   alongside them is both the quieter arrangement and the more legible one.
2. **The I²C pair wants to be the shortest path, not the prettiest.** Every
   extra millimetre is capacitance against the budget in §2.5, and that budget
   is the one with the least margin in this whole document. If a flourish costs
   wire, spend it on the pot runs, which do not care.
3. **The chain's V+ and GND are the one pair carrying real current** (up to
   466 mA, §4.2). They are also the pair whose route is least constrained.

### 6.3 The one mechanical problem: P1 is a socket

The header is **female** `[VENDOR W1]` `[W2]`, so nine wires and a QT cable all
have to arrive as male pins on a 2.54 mm grid, in a 2 × 8 pattern, in the middle
of the board's back face, with an 18650 holder immediately below it. Loose
Dupont leads into a socket is exactly the kind of wiring that looks like a
mistake through clear acrylic.

The clean answer for this enclosure is a **2 × 8 male pin header carried by the
carrier plate** — pressed into a printed boss or soldered to a small piece of
1.6 mm FR4 matching the board's own stock `[parts.md §1]` — with the deck's
nine wires terminated on its upper tails and fanned out from there. The board
then drops onto a single 16-pin connector, the fan-out becomes a deliberate
feature instead of nine individual leads, and the dev board can still be
unplugged without unpicking the loom. The exact boss geometry belongs to the
layout document, not here; what this document fixes is that **the mating part is
a 2 × 8 male 2.54 mm header, and the pin order is the table in §1.2.**

### 6.4 Shopping list, delta against `parts.md` §11

| Qty | Part | PID / value | Why |
|---|---|---|---|
| 3 | I²C STEMMA QT Rotary Encoder **with encoder** | Adafruit **5880** | unchanged |
| 1 | NeoSlider | Adafruit **5295** | one, not two |
| 3 | Panel-mount pot, 10 kΩ **linear (taper B)** | per `parts.md` §5.3 | unchanged |
| **1** | **STEMMA QT JST-SH 4-pin to male headers, 150 mm** | **Adafruit 4209** | **the only new part this analysis adds** |
| 4 | STEMMA QT cables, length to suit the layout | Adafruit 4399 / 4210 / 4401 | chain |
| **3** | **1.5 kΩ resistor, 1 %** | — | pot ceiling, §3.5 |
| **3** | **100 nF capacitor** | — | ADC input, §3.5 |
| 1 | 2 × 8 male 2.54 mm header | — | mates with P1, §6.3 |
| **0** | ~~ADS1115~~ | ~~1085~~ | **not needed — §3.3** |
| **0** | ~~I²C multiplexer~~ | — | **not needed** |

---

## 7. Corrections and confirmations for other documents in this repo

| Document | Statement | This document |
|---|---|---|
| `docs/HARDWARE.md` | panel is SPI on 11/12/40/5/41 | **confirmed** from the vendor allocation table and the schematic |
| `docs/HARDWARE.md` | LCD FPC carries only GND, VCC3V3, SCL, SDA, CS, RS, TE, RESET | **incomplete** — pins 16–20 are `TP_VCC/TP_RESET/TP_INT/TP_SDA/TP_SCL` `[SCHEM W4]`. No backlight pin, so the backlight conclusion stands |
| `docs/HARDWARE.md` | GPIO 5, 11, 12, 18, 40, 41 refused | **confirmed, and GPIO4/6/8/9/10/13/14/15/16/19/20/21/38/39/45/46 should join the list** — see §1.3 |
| `docs/HARDWARE.md` "MIDI on a wire" | the DIN pin "is the owner's to declare" | **P1 pin 10 (`U0TXD`, GPIO43) and pin 12 (`U0RXD`, GPIO44) are brought out and silkscreened `TXD`/`RXD`** `[SILK W2]`. Recorded as a hardware fact; the choice stays the owner's |
| `firmware/main/battery.c` | battery pin and divider to be declared by the owner | hardware says **GPIO4**, divider **200 kΩ / 100 kΩ = 3:1** `[SCHEM W4]` |
| `v2/parts/parts.md` §8.3 | "nine boards in parallel divides the effective pull-up by roughly nine… be ready to run at 100 kHz or lift pull-ups" | **half right.** The pull-ups are fine (2.48 mA, §2.4) and nothing should be lifted — but **100 kHz is the right speed anyway**, for the different reason in §2.5 |
| `v2/parts/parts.md` §8.4 | recommends the ADS1115 over native ADC | **reversed, with reasons** — §3.3. Its ADC2/Wi-Fi warning was correct and is the reason the answer is GPIO1/2/3 specifically |
| `v2/parts/parts.md` §10 item 7 | "Per-board I²C pull-up values across all nine boards" listed as NOT SOURCED | **now sourced** for the four boards in this build, from the vendor schematics: §2.4 |
| `v2/parts/parts.md` §10 item 1 | NeoSlider travel NOT SOURCED | still not travel, but the Adafruit page calls it *"a 75mm long slide potentiometer"* `[AF 5295]`, and the schematic symbol is named `SLIDE_POT_75MM` `[SCHEM A2]`. **75 mm is the pot's length, not its travel** — the caliper job stands |

---

## 8. NOT SOURCED

Stated loudly, per the project's rule that an unsourced number is a bug.

**Electrical, measure or fetch before trusting:**

1. **STEMMA QT cable wire gauge and capacitance per unit length.** Adafruit do
   not publish either `[AF 4209, AF tech specs]`. The gauge bounds the chain's
   V+ drop (§4.4); the capacitance is a term in the §2.5 budget. *Measure:* an
   LCR meter across one cable's SDA–GND, or scope the rise time on the
   assembled chain, which answers the question that actually matters.
2. **BSS138 C_OSS at V_DS = 0–3.3 V.** Figure 8 was read off the graph
   `[CHART]`, giving 40–90 pF against the table's 13 pF at 25 V. The exact
   number is not in any table in the datasheet. *This is the largest uncertain
   term in §2.5.* It is what makes 100 kHz the recommendation rather than a
   preference.
3. **Bus capacitance of the assembled chain.** The direct test: scope SCL at the
   far end and read `tr` from 0.3 V_DD to 0.7 V_DD; `Cb = tr / (0.8473 × 1170)`.
   Under 303 pF means 400 kHz is available; over it means it is not.
4. **PCF85063, SHTC3, ES8311 and ES7210 I²C addresses.** Four devices are on the
   bus before anything is plugged in (§2.2), read from the schematic, but their
   addresses were not read from datasheets in this session. *Get them from
   `i2cdetect` on the real board before finalising the jumper map.*
5. **Seesaw board logic current.** The ATSAMD09 datasheet returned HTTP 403 from
   this network (microchip.com and the Farnell mirror both). The term is bounded
   under 40 mA for four boards and no conclusion depends on it, but the real
   number should come from a USB power meter on the bench.
6. **ETA6098 input current limit.** Governs how much the deck can draw while
   running from USB rather than the cell. `docs/HARDWARE.md` already lists this
   as open; it is still open.
7. **P1 connector current rating per pin.** Waveshare do not name the part
   `[VENDOR W1]`. Only matters for pin 1, which carries 466 mA worst case.
8. **Whether the 5752 Quad Rotary carries BSS138 level shifters.** Decides
   whether swapping three 5880s for one 5752 would buy back the 400 kHz budget
   (§2.5). Not fetched; the layout has settled on three separate encoders so it
   was not pursued.
9. **PCB mounting-hole pitch on the bare board.** The 85.50 × 62.10, 4 × M2.50
   figures `[VENDOR W5]` are dimensioned on the assembled unit including the
   back shell. Caliper the bare board before cutting the carrier plate.

**Mechanical, for whoever draws the carrier plate:**

10. **P1's exact position on the PCB in mm.** Visible in the photo `[W2]` and in
    the back-shell cutout `[W5]`, but not dimensioned by Waveshare. Caliper from
    the board's own datum — the 92.50 × 70.10 outline and the four M2.5 holes.
11. **Header body height above the PCB**, which sets how far the carrier plate
    must stand off the dev board.

---

## 9. One-line summary

**2 × 8 female header on the back · SDA = GPIO13 pin 14 · SCL = GPIO14 pin 16 ·
pots on GPIO1/2/3 = ADC1_CH0/1/2 at pins 7/9/11 · 3V3 pin 1 · GND pins 3/4 ·
bus at 100 kHz · no pull-ups cut · one Adafruit 4209 cable and six passives,
and nothing else.**
