# The second MCU: the video board, and where it goes

*An enclosure and assembly input. The user asked for "the RP2040 Feather board
that comes with a HDMI output -- this will also need to be mounted in the
enclosure as it will be responsible for the video output and we will have to
wire it to our main Dev board as well."*

*This document identifies the board exactly, measures every dimension the
carrier plate needs from Adafruit's own CAD model and fab print, chooses the
link to the ESP32-S3 from the pins that are actually left, works the power
through, and places it in the `v2/layouts/slab.py` deck. Every number carries a
source tag. Anything that could not be traced to a primary source is marked
**NOT SOURCED** and listed in §8 -- it is not estimated.*

---

## 0. Verdict up front

1. **It is Adafruit 5710, "Adafruit Feather RP2040 with DVI Output Port - Works
   with HDMI", $14.95.** `[VENDOR F1]` It is the **only** RP2040 Feather with a
   video output -- Adafruit's own Feather category page lists ten RP2040/RP2350
   Feathers and 5710 is the single one that mentions DVI, HDMI or video
   `[VENDOR F9]`. §1.1, §1.3.

2. **The signal is DVI-D, not HDMI, out of a full-size HDMI Type A socket.**
   The socket is a **Molex 47151-0001** -- "0.50mm Pitch HDMI Receptacle, Right
   Angle, 19 Circuits" `[SCHEM F4]` `[3P F7]` -- so the *connector* is HDMI and
   takes an ordinary HDMI cable, but what comes out of it is DVI TMDS. Adafruit
   are explicit: **"Note it doesn't do audio, just graphics!"** `[VENDOR F1]`.
   It is **full size**, not mini, not micro: the receptacle measures
   **15.00 mm** across in Adafruit's own CAD model `[STEP F5]` and the vendor
   photograph is unmistakably a Type A shell `[F8]`. §1.2.

3. **The Feather hole pattern on this board is NOT a rectangle, and anyone who
   assumes it is will be 0.625 mm out on two bosses.** Measured off the CAD
   model: all four Ø2.50 holes sit 0.100 in from the nearest short edge, but
   the **left (USB) pair is 0.100 in from the long edges and the right (HDMI)
   pair is 0.075 in** -- 17.780 mm apart on the left, **19.050 mm apart on the
   right**, 45.725 mm between the columns `[STEP F5]`. Adafruit moved the right
   pair outboard to clear the HDMI shell. **Drill the trapezoid, not the
   rectangle.** §2.2.

4. **The video connector is on a short end, it faces along the board's long
   axis, and it is centred on the width to 0.02 mm.** Envelope
   **12.15 (X) × 15.00 (Y) × 8.385 (Z)**, protruding **0.880 mm past the PCB's
   right edge** and **0.630 mm below the PCB's bottom face**, standing
   **6.185 mm above the PCB's top face** `[STEP F5]`. **It is the tallest thing
   on the board**, and the whole placement follows from it. §2.4, §5.2.

5. **6.185 mm of connector against 6.35 mm of air is the number this whole
   document turns on.** `v2/parts/stack.md` §2.6 puts the acrylic underside at
   **Z = +6.35** over a wiring plane at **Z = 0.00**, with breakout PCB front
   faces sunk to **−0.15**. A 5710 pocketed like every other breakout would put
   its HDMI shell top at **+6.035 -- clearing the acrylic by 0.315 mm**
   `[DERIVED]`. It fits. **But the plug does not**, and that is why the board
   does not go on the wiring plane. §5.2.

6. **I²C is dead for this link, and not for the obvious reason.** At the
   100 kHz this build settled on (`wiring.md` §2.5), a 15,000-byte frame takes
   **1.20 s** `[DERIVED]`. Worse, `wiring.md` §2.4 fixed a **hard ceiling of
   five external 10 kΩ boards** and the 5710 carries four 10 kΩ pull-ups
   (R18–R21) plus **two more BSS138 drains** on SDA/SCL for the HDMI DDC
   level-shift `[SCHEM F4]` -- the exact capacitance term §2.5 called its
   largest uncertainty. Putting the video board on the STEMMA chain would
   damage the control surface to no purpose. §3.2.

7. **UART0 is free on this build, and it is free because of one line in
   `firmware/sdkconfig.defaults`, not because of anything about the chip.**
   `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` `[REPO]` puts the IDF console on
   USB-Serial-JTAG, so GPIO43/GPIO44 carry nothing. **But the ROM still prints
   a boot message on U0TXD at 115200 bps at every reset** `[3P ET]`, and the
   eFuse that controls it, `UART_PRINT_CONTROL`, defaults to *Enable*
   `[3P EF]`. So the link must be framed with a sync word, and **that sdkconfig
   line is now load-bearing** -- moving the console back to UART0 would kill
   the video. Say so where someone will read it. §3.4.

8. **Three signal pins remain on the Waveshare 2×8 and the right answer uses
   all three: UART0 TX + RX + one handshake.** After `wiring.md`'s allocation
   the free pins are **GPIO43 (pin 10), GPIO44 (pin 12), GPIO17 (pin 13)**, plus
   VBUS on pin 2. At the ESP32-S3's 5 Mbps UART ceiling `[DS E8 §4.2.1.1]` that
   is 500 kB/s -- **33 full 1-bit frames a second**, against a panel that
   refreshes at 32 Hz. SPI would be faster but spends all three pins and leaves
   no flow control, and the RP2040 is an SPI *slave* capped at clk_peri/12
   `[DS R1 §4.4]` while one core and both PIOs are already committed to DVI.
   **UART, framed, with GPIO17 as hardware flow control.** §3.3, §3.5.

9. **There is no correct power pin for this board on the deck's header, and
   that is the real power finding.** The Feather's 3V3 is an LDO *output*
   (RT9080-3.3 `[SCHEM F4]`, "500mA peak… you can't do it continuously from 5V
   as it will overheat the regulator" `[VENDOR F2]`), and Adafruit say feeding
   it is **"Not recommended… You may end up damaging your Feather"**
   `[VENDOR F2]`. The right pin is **BAT**, which wants 3.0–4.2 V -- and the
   Waveshare 2×8 brings out only 3V3 and VBUS, **never VBAT** `[REPO
   wiring.md §1.2]`. §4.3.

10. **The rail is not the constraint.** A planning figure of **≈250 mA worst
    case** `[DERIVED, §4.1]` on top of `wiring.md` §4.3's 806 mA still leaves
    **47 % headroom on the TPS63020's 2 A** `[DERIVED]`. And the DVI output's
    own 48.9 mA of TMDS current is **paid for by the monitor**, not by the deck
    -- it is sourced by the sink's termination supply and sunk into the
    Feather's ground `[DERIVED, conditional -- §8.3]`. §4.2.

11. **It goes in the 28.575 mm gap between the encoder grid and the fader,
    turned 90°, HDMI out the REAR wall, sunk BELOW the wiring plane in a
    through-window in the carrier.** Centre **x = +101.600** (= 4.000 in,
    160 × 0.635, 3.175 mm of carrier on one side and 2.540 on the other), PCB
    rear edge against the interior rear wall at **y = +89.525**, board spanning
    **+38.725 … +89.525**, leaving **25.400 mm** of clear floor in front of it.
    §5.3.

12. **One thing in `v2/layouts/carrier.py` has to move or bridge.** The
    `E3-FADER` I²C leg runs horizontally across that gap at **y = +85.715**,
    straight through the proposed footprint. Bridge it over the window on the
    1.20 mm roof the carrier already uses, or drop it to y = +31.750 and accept
    ~104 mm more cable -- immaterial at 100 kHz. §6.

---

## 0.1 Sources

| ID | Source | How read |
|----|--------|----------|
| F1 | `adafruit.com/product/5710` -- product page | fetched 2026-11-06 |
| F2 | `learn.adafruit.com/adafruit-feather-rp2040-dvi` -- the product guide, pulled as `?view=all` markdown (278 KB) | fetched 2026-11-06 |
| F3 | F2 → `adafruit_products_FRPDVI_fab_print.png`, 1404 × 826 | fetched; read as an image, then measured numerically (pad centroids extracted by colour and clustered) |
| F4 | F2 → `adafruit_products_FRPDVI_sch.png`, 7646 × 5146 | fetched; read as an image at full resolution in five crops |
| F5 | `raw.githubusercontent.com/adafruit/Adafruit_CAD_Parts/main/5710 Feather RP2040 DVI/5710 Feather RP2040 DVI.stl` -- Adafruit's own CAD model, 47,922 triangles | fetched; bounding boxes, component clusters, hole positions and board outline computed from the mesh |
| F6 | `learn.adafruit.com/adafruit-feather/feather-specification` -- the Feather standard, and its own Proto FeatherWing fab print | fetched 2026-11-06; text extracted, drawing read as an image |
| F7 | Molex product document for **47151-0001**, generated 03/10/2015, mirrored at `res.ameya360.com` (Mouser's copy returned an empty reply through this proxy) | fetched; text extracted |
| F8 | F2 → `adafruit_products_FRPDVI_HDMI_end.jpg`, `adafruit_products_FRPDVI_STEMMA_QT.jpg` | fetched, read as images |
| F9 | `adafruit.com/category/777` -- the Feather category listing | fetched 2026-11-06 (first 100 KB of 187 KB) |
| R1 | **RP2040 Datasheet**, Raspberry Pi, PDF built 2025-02-20 | fetched; text extracted, Figure 172 rendered at 170 dpi and read |
| E8 | **ESP32-S3 Series Datasheet**, Espressif | fetched; text extracted |
| ET | `docs.espressif.com/projects/esptool/.../boot-mode-selection.html` | fetched 2026-11-06 |
| EF | `docs.espressif.com/projects/esptool/.../espefuse/summary-cmd.html` | fetched 2026-11-06 |
| RT | Distributor parametric data for **RT9080-33GJ5** (LCSC, DigiKey, element14). **Richtek's own PDF was not retrieved** -- see §8.6 | search results only |
| REPO | this repository: `v2/parts/wiring.md`, `v2/parts/stack.md`, `v2/layouts/{slab,carrier,parts,_common}.py`, `v2/tools/layout.py`, `docs/HARDWARE.md`, `firmware/sdkconfig.defaults` | read in place |

Tags: `[VENDOR]` vendor states it · `[SCHEM]` read off the schematic ·
`[FAB]` measured off the fab print · `[STEP]` measured from the vendor CAD
model · `[DS]` datasheet · `[CHART]` read off a datasheet graph, lower
confidence · `[DERIVED]` computed here from tagged values · `[3P]` third party
· `[REPO]` read from this repository · `[NOT SOURCED]` not established.

---

## 1. Which board, exactly

### 1.1 The identification

| | |
|---|---|
| Product name | **Adafruit Feather RP2040 with DVI Output Port - Works with HDMI** `[VENDOR F1]` |
| Product ID | **5710** `[VENDOR F1]` |
| Price | $14.95 (1–9), $13.46 (10–99), $11.96 (100+) `[VENDOR F1]` |
| Vendor dimensions | 53.0 × 23.3 × 7.7 mm / 2.1 × 0.9 × 0.3 in `[VENDOR F1]` |
| Weight | 7.2 g `[VENDOR F1]` |
| MCU | RP2040, dual Cortex-M0+ `[VENDOR F1]` |
| Flash | 8 MB QSPI, U1 on the schematic `[SCHEM F4]` |
| Board revision | **"Feather RP2040 DVI rev A", 3/21/23** -- from the schematic's own title block `[SCHEM F4]` |

Reseller SKUs for the same thing: `ADA5710` (Pi Hut, Pimoroni), Micro Center
MPN 5710. `[3P, search results]`

### 1.2 DVI, not HDMI -- what the signal actually is

**This is the detail that decides what it can be plugged into, so it is worth
being exact about.**

- The **connector** is a full-size **HDMI Type A receptacle**: Molex
  **47151-0001**, "0.50mm Pitch HDMI Receptacle, Right Angle, 0.76 µm Gold
  Plating, Front Solder Tab Length 1.80mm, Shell Nickel Plated, without Back
  Cover, Black Housing, 19 Circuits" `[3P F7]`, drawn as `X2` on the schematic
  with the part number printed beside it `[SCHEM F4]`. Molex rate it at
  **0.5 A max per contact, 40 V DC**, surface-mount termination, and list it as
  mating with their 88768 HDMI cable assembly `[3P F7]`. **An ordinary HDMI
  cable plugs straight in.**
- The **signal** is **DVI-D TMDS**. The schematic's own block is labelled
  **"DVI OUTPUT"** and the connector carries `TXC±`, `TX0±`, `TX1±`, `TX2±`,
  `DCLK`/`DDAT` (the DDC pair), `HOTPLUG`, `CEC`, `+5V`, `UTILITY` and the four
  shields `[SCHEM F4]`. Adafruit: *"a digital video output (a.k.a DVI) that
  will work with any HDMI monitor or display. Note it doesn't do audio, just
  graphics!"* `[VENDOR F1]`.
- **What that costs you, concretely:**
  - **No audio.** HDMI carries audio in the data islands during blanking; DVI
    does not generate them. The deck's audio stays on its own path.
  - **No HDCP, no InfoFrames.** A monitor will sync; **a capture card, a video
    switcher or an HDMI-to-something scaler may not**, because some of them
    expect HDMI InfoFrames before they will lock. For a live-coding rig whose
    output may end up in a capture chain, this is the thing to test before
    building it in. `[DERIVED from the above; not tested here -- §8.1]`
  - **No EDID negotiation in the signal path** -- but the DDC pair *is* wired:
    the connector's SCL/SDA run to the Feather's own SCL/SDA through a BSS138
    level shifter with 4 × 10 kΩ pull-ups (R18–R21) so you can read the
    display's EDID EEPROM `[SCHEM F4]` `[VENDOR F2]`. Hot Plug Detect comes
    back through R16 = 100 kΩ to `HPD_3V` and lands on **pin 1 of the 16-pin
    header**, so the firmware can tell when a display is connected
    `[SCHEM F4]` `[VENDOR F2]`.
  - **CEC** (connector pin 14) and **UTILITY** (pin 17) are broken out to
    through-hole pads labelled `C` and `U` on the silk `[VENDOR F2]`
    `[SCHEM F4]`.
- **Resolution and colour**, verbatim: *"an internal framebuffer of 320x240 or
  400x240 16-bit pixels that is then continuously blitted out as pixel-doubled
  640x480 or 800x480 digital video"* `[VENDOR F2]`. And the cost:
  *"the DVI video generation uses one full core, both PIOs, and 150K (320x240)
  or 190K (400x240) of SRAM. It's kinda maxed out"* `[VENDOR F2]`.
- **The GPIOs it eats**: the schematic maps **GPIO16–GPIO23** to CKN/CKP,
  D0N/D0P, D1N/D1P, D2N/D2P `[SCHEM F4]` -- "the 8 unused pins" of the Feather
  footprint `[VENDOR F2]`. Each of the eight runs through a **220 Ω** series
  element in the R14/R15 resistor arrays (silk-noted "220 Pack") `[SCHEM F4]`.
  That resistor-network pseudo-differential driver is the whole trick, and it
  is also why §4.2's current arithmetic comes out the way it does.
- **Connector size**: **full size**. The receptacle's modelled envelope is
  **15.00 mm wide** `[STEP F5]` against a 22.86 mm board, and the vendor's
  end-on photograph shows the Type A trapezoid with the 19-contact tongue
  `[F8]`. It is **not** mini (Type C) and **not** micro (Type D).

### 1.3 The other candidates, so you can correct me

Adafruit's Feather category page `[VENDOR F9]` lists these RP2040/RP2350
Feathers. **5710 is the only one with a video output of any kind; 6000 has an
HSTX port but needs a separate adapter to become DVI.**

| PID | Board | Video? |
|---|---|---|
| **5710** | **Feather RP2040 with DVI Output Port - Works with HDMI** | **yes -- DVI over HDMI Type A, on board** |
| 4884 | Feather RP2040 | no |
| 5980 | Feather RP2040 Adalogger - 8MB Flash with microSD - STEMMA QT | no |
| 5714 | Feather RP2040 with RFM95 LoRa Radio - 915 MHz | no |
| 5724 | RP2040 CAN Bus Feather with MCP2515 | no |
| 5727 | RP2040 Feather ThinkInk for 24-pin E-Paper | e-paper, not video |
| 5712 | Feather RP2040 RFM69 Packet Radio | no |
| 5650 | Feather RP2040 SCORPIO - 8 Channel NeoPixel Driver | no |
| 5768 | RP2040 Prop-Maker Feather with I2S Audio Amplifier | no |
| **6000** | **Feather RP2350 with HSTX Port** | 22-pin HSTX only -- **needs Adafruit 6055, the "RP2350 22-pin FPC HSTX to DVI Adapter", plus an FPC cable** `[3P, search]` |

Outside the Feather footprint, if the brief ever changes:

| PID | Board | Note |
|---|---|---|
| 4984 | **DVI Breakout Board for HDMI Source Devices** | the half of 5710 that is not a Feather; Adafruit describe 5710 as *"our RP2040 Feather and DVI Breakout board… glued together"* `[VENDOR F2]` |
| 6055 | RP2350 22-pin FPC HSTX to DVI Adapter | $4.95; FPC cable not included `[3P, search]` |
| 6130 | Feather RP2350 with HSTX Port **and 8 MB PSRAM** | same footprint as 6000, $15.50 `[3P, search]` |
| 6003 / 6267 | Metro RP2350 (no PSRAM / 8 MB PSRAM) | Metro footprint, HSTX over ribbon to 6055 `[3P, search]` |
| 6200 | Fruit Jam (RP2350B) | DVI connector **on the board**, no adapter; much larger `[3P, search]` |

**PIDs 4884, 5980, 5714, 5724, 5727, 5712, 5650, 5768, 6000 and 5710 come from
Adafruit's own category listing `[VENDOR F9]`. The second table's PIDs come
from search results and were not confirmed on adafruit.com in this session
(§8.2).**

**If the brief is "a Feather-format RP2040 with a video port", 5710 is the
answer and there is no second candidate.** The one reason to reconsider is
that the RP2350 route (6000/6130 + 6055) is newer, has a hardware serialiser
instead of bit-banged PIO, and would not need the 1.89× overclock of §4.1 --
but it is two parts and an FPC cable instead of one board, and it is not what
was asked for.

---

## 2. Every dimension the carrier needs

Unless noted, everything in this section is **measured from Adafruit's own
STL** `[STEP F5]`, in **board coordinates**: origin at the PCB's lower-left
corner looking at the component side, **X along the 50.80 mm axis with the
HDMI at X+**, **Y along the 22.86 mm axis**, **Z = 0 at the PCB's bottom
face**. Those axes are the model's own; nothing was rotated.

### 2.1 The PCB

| | mm | source |
|---|---|---|
| Outline | **50.800 × 22.860** | `[STEP F5]`, matching 2.00 × 0.90 in `[FAB F3]` `[VENDOR F6]` |
| Thickness | **1.570** | `[STEP F5]` (PCB flats at Z = 0.000 and Z = 1.570) |
| Corner radius | **≈ 1.90** (0.075 in) | `[STEP F5]`, from the edge scan |
| Vendor's quoted size | 53.0 × 23.3 × 7.7 | `[VENDOR F1]` -- the 53.0 is the PCB **plus the two connector overhangs**, see §2.3 |

**The vendor's 53.0 mm is not the PCB.** The PCB is 50.800; the USB-C hangs
1.176 off one end and the HDMI 0.880 off the other, giving
50.800 + 1.176 + 0.880 = **52.856 mm overall** `[DERIVED from STEP F5]`, which
is what Adafruit rounded to 53.0. **Cut the carrier pocket to 50.800 and relieve
locally for the two overhangs.** The vendor's 23.3 mm likewise is not the
width: nothing overhangs in Y, and the board is **22.860** exactly.

### 2.2 The Feather standard's hole pattern, and where this board departs from it

**The published standard** `[VENDOR F6]`, quoted:

> "The 'classic' Feather and Wing size is 0.9" x 2.0" with 0.1" holes at each
> corner. There is one 16-pin breakout strip on the bottom side, centered 1.0"
> from the left edge. There is one 12-pin breakout strip on the top side, 1.2"
> from the left side. The spacing between the two strips is 0.8""
>
> "Don't change the GPIO spacing or location, or you will not maintain
> compatibility with Wings!"

The spec's own Proto FeatherWing drawing dimensions it as **2 × 0.9** overall,
**1.8** between the hole columns, **0.7** between the hole rows, and **0.1**
from each edge to a hole centre `[VENDOR F6]`. The phrase *"0.1" holes"* is
ambiguous between a diameter and an inset; the drawing settles the inset, and
the CAD model settles the diameter.

**Measured on 5710** `[STEP F5]` -- hole centres found by scanning the PCB top
face for voids on a 0.05 mm grid, so each figure is ±0.025 mm:

| Hole | X | Y | Ø |
|---|---|---|---|
| USB end, lower | **2.540** | **2.540** | **2.50** |
| USB end, upper | **2.540** | **20.320** | **2.50** |
| HDMI end, lower | **48.260** | **1.905** | **2.50** |
| HDMI end, upper | **48.260** | **20.955** | **2.50** |

```
column pitch   48.260 - 2.540  =  45.720  =  1.800 in      [DERIVED]  spec
row pitch, USB end                 17.780  =  0.700 in      [DERIVED]  spec
row pitch, HDMI end                19.050  =  0.750 in      [DERIVED]  NOT spec
```

**The right-hand pair is 0.075 in from each long edge, not 0.100 in.** The
reason is visible in the model: the HDMI shell occupies Y = 3.912 … 18.912
(§2.4), so a hole at the spec's 2.540 with Ø2.50 would leave **0.12 mm**
between the hole and the shell `[DERIVED]`. Adafruit moved the pair outboard
by 0.635 mm and took the clearance to 0.76 mm. The end-on photograph shows the
result -- the corner hole is visibly tight to the board edge `[F8]`.

**Consequence for the carrier plate: the four M2 bosses are an isosceles
trapezoid, not a rectangle.** `stack.md` §5.4 already settled that these
Ø2.50 board holes take **M2, not M2.5** (M2.5 clearance is Ø2.7–2.9 and will
not pass a Ø2.50 hole) -- that ruling applies unchanged here. Four
**ruthex RX-M2×4** inserts, boss OD 6.35.

### 2.3 Height, above and below

All Z figures are from the model, referenced to the PCB's **bottom** face at
Z = 0 `[STEP F5]`:

| Item | Z range | above PCB top (1.570) | below PCB bottom |
|---|---|---|---|
| **HDMI receptacle** (X2) | **−0.630 … 7.755** | **+6.185** | **0.630** |
| Battery JST PH (X1G$1) | 1.570 … 6.370 | +4.800 | 0 |
| STEMMA QT (CONN1G$1) | 1.570 … 5.870 | +4.300 | 0 |
| USB-C (X6) | 0.570 … 4.770 | +3.200 | 0 |
| Reset / Boot buttons, with plunger | 1.570 … 3.470 | +1.900 | 0 |
| RP2040 QFN-56, 7.00 × 7.00 | 1.570 … 2.570 | +1.000 | 0 |
| **Whole board** | **−0.630 … 7.755** | **+6.185** | **0.630** |

```
overall thickness        7.755 - (-0.630)  =  8.385 mm           [STEP F5]
vendor's "7.7 mm"        7.755 - 0          =  7.755 mm          [DERIVED]
```

So Adafruit's 7.7 mm is measured from the PCB's bottom face and **ignores the
0.630 mm of HDMI shield tab hanging below it.** The carrier needs a local
relief under the connector footprint to at least **−2.35** relative to a PCB
bottom at −1.72, and `stack.md` §2.6 already puts the channel floors at
**−3.00**, so the standard channel depth covers it with 0.65 mm to spare
`[DERIVED]`.

**The headers are not in the model and the board ships with them loose.**
`[STEP F5]` shows nothing in the two header rows. If male pins are soldered
in, their tails add roughly 3 mm below the PCB, which **exceeds the −3.00
channel floor** and would force a deeper relief. **Do not fit the headers.**
Solder the four or five wires this build actually uses straight to the pads --
that is also the right answer for a board seen through clear acrylic.

### 2.4 The video connector: where it is and which way it faces

**This is the dimension the placement turns on, so here it is in full.**

```
HDMI receptacle, modelled envelope                               [STEP F5]

  X   39.530 ... 51.680      length 12.150, of which 0.880 is
                             PAST the PCB edge at X = 50.800
  Y    3.912 ... 18.912      width  15.000
  Z   -0.630 ...  7.755      height  8.385

  Y centre (3.912 + 18.912)/2 = 11.412   against a board centre
  of 22.860/2 = 11.430 -- centred to 0.018 mm           [DERIVED]
```

- **It is on a short end** -- the X+ end, the end away from the USB-C.
- **It faces along the board's long axis**, straight out of that short end.
  So **the board must sit perpendicular to whichever enclosure wall the port
  exits**, exactly as the Waveshare board does for its own side ports.
- **It is centred on the board's width to 0.018 mm**, so the aperture in the
  wall is on the board's own centreline. No offset to carry.
- **It protrudes 0.880 mm past the PCB outline**, so a pocket cut to the PCB
  outline needs a 15.0 × 0.9 mm notch at that end.
- Through-hole shield tabs reach **0.630 mm below the PCB**.
- Two of the four PCB mounting holes are inside the connector's shadow in Y --
  see §2.2.

The guide's own words: *"The DVI (digital video output) connector is located on
the right side of the board"* `[VENDOR F2]`.

### 2.5 USB, battery, STEMMA QT

| | position (board coords) | envelope | notes |
|---|---|---|---|
| **USB-C** `X6` | X −1.176 … 6.720, Y 6.960 … 15.900 | 8.940 wide, **3.200 above the PCB** | **opposite short end from the HDMI**, protruding **1.176 mm** past the X = 0 edge, Y-centred. Type C, power and data `[VENDOR F1]`. The board's only USB port. `[STEP F5]` |
| **Battery** `X1G$1` | X 6.970 … 14.870, Y 14.850 … 22.450 | 7.90 × 7.60, **4.800 above the PCB** | **2-pin JST PH**, on the long edge at Y+, centre X = **10.920 = 0.430 in** -- which is the Feather spec's "0.425 in from the left edge" `[VENDOR F6]` and the fab print's own `0.43` dimension `[FAB F3]`. **It is a TOP-ENTRY socket** -- the contacts face up, visible in the vendor photo `[F8]`. `[STEP F5]` |
| **STEMMA QT** `CONN1G$1` | X 30.480 … 34.080, Y 13.100 … 19.100 | 3.60 × 6.00, **4.300 above the PCB** | JST SH 4-pin, labelled `I2C` on the silk, *"towards the top of the board, a bit right of center"* `[VENDOR F2]`. **Also TOP-ENTRY** `[F8]`. Wired to the Feather's SCL/SDA = RP2040 **I2C1** `[VENDOR F2]`. `[STEP F5]` |

**Both sockets face UP, and that matters here.** A JST PH plug stands roughly
6–7 mm above its socket and a JST SH plug roughly 2–3 mm; either one on a board
pocketed at the wiring plane would stand well past the acrylic underside at
+6.35 `[REPO stack.md §2.6]`. **Nothing can be plugged into the top of this
board once the acrylic is on.** §5.2 resolves it; §4.4 says what to do about
the battery.

Charger and power path, read off the schematic `[SCHEM F4]`:

- **U3 = MCP73831T-2ACI/OT** LiPo charger, VDD from VBUS.
- **R8 = 5.1 kΩ** on PROG. The schematic prints its own table beside the part
  -- `10K = 100mA · 5.0K = 200mA · 2.0K = 500mA · 1.0K = 1000mA` -- so this
  board charges at **≈ 200 mA**.
- **Q3 = DMG3415UFY** P-channel FET passes VBAT to `VHI` when VBUS is absent;
  **D4 = PMEG2020AEA** Schottky passes VBUS to `VHI` when it is present. A
  hot-swap OR, exactly as the guide describes.
- **U2 = RT9080-3.3** LDO, `VHI` → **+3V3**, EN pulled up 10 kΩ (R3) to VHI and
  broken out, 3 × 10 µF of decoupling.
- Status NeoPixel (WS2812B) on **GPIO4**, powered from **+3V3** `[SCHEM F4]`
  `[VENDOR F2]`. Red `D13` LED. Amber `chg` LED via R2 = 10 kΩ.

### 2.6 Castellations: there are none

Scanned the PCB outline in the model at 0.05 mm along both long edges: the only
voids are the four corner radii. **No castellated pads anywhere.** `[STEP F5]`
The electrical interfaces are the **two 0.1 in header rows**, the **three
through-hole pads** `HPD` (on the 16-pin row), `CEC` and `U`, and the two
plug-in sockets of §2.5. Nothing else.

### 2.7 The two headers, pin by pin

Hole positions measured from the model `[STEP F5]`, cross-checked against the
fab print `[FAB F3]` and the schematic `[SCHEM F4]`:

```
16-pin row (JP1)   Y = 1.270 (0.050 in from the long edge)
                   X = 6.350 ... 44.450, 2.540 pitch, 16 holes
                   = 0.250 in ... 1.750 in, centred 1.000 in from the left

12-pin row (JP3)   Y = 21.590 (0.050 in from the other long edge)
                   X = 16.510 ... 44.450, 2.540 pitch, 12 holes
                   = 0.650 in ... 1.750 in, centred 1.200 in from the left

row separation     21.590 - 1.270 = 20.320 = 0.800 in      [DERIVED]
drill              approx O1.0                             [STEP F5]
```

**Note the rows are 0.050 in from the long edges, not 0.100 in** -- 0.9 in of
board with 0.8 in between the rows leaves 0.05 in a side `[DERIVED from
VENDOR F6]`. The fab print's `0.80` dimension is the row separation; its `0.70`
is the mounting-hole row pitch `[FAB F3]`.

**JP1, the 16-pin row, pin 1 at the HDMI end** `[SCHEM F4]`:

| Pin | Net | X (mm) |
|---|---|---|
| 1 | **HPD_3V** (Hot Plug Detect, through R16 100 kΩ) | 44.450 |
| 2 | TX -- GPIO0, UART0 TX | 41.910 |
| 3 | RX -- GPIO1, UART0 RX | 39.370 |
| 4 | MISO -- GPIO8 | 36.830 |
| 5 | MOSI -- GPIO15 | 34.290 |
| 6 | SCK -- GPIO14 | 31.750 |
| 7 | D25 -- GPIO25 | 29.210 |
| 8 | D24 -- GPIO24 | 26.670 |
| 9 | A3 -- GPIO29 / ADC3 | 24.130 |
| 10 | A2 -- GPIO28 / ADC2 | 21.590 |
| 11 | A1 -- GPIO27 / ADC1 | 19.050 |
| 12 | A0 -- GPIO26 / ADC0 | 16.510 |
| 13 | **GND** | 13.970 |
| 14 | **+3V3** | 11.430 |
| 15 | **+3V3** | 8.890 |
| 16 | RESET | 6.350 |

**Pins 15 AND 14 are both +3V3** -- the classic Feather's ARef position is tied
to the rail on this board; the schematic shows one junction feeding both, with
GND alone on pin 13 `[SCHEM F4]`. The silk reads
`Rst 3.3V GND A0 A1 A2 A3 24 25 SCK MO MI RX TX HPD` -- fifteen labels over
sixteen holes, the unlabelled one being the second 3V3. **Do not count silk
labels to find GND; count holes.**

**JP3, the 12-pin row, pin 1 at the USB end** `[SCHEM F4]`:

| Pin | Net | X (mm) |
|---|---|---|
| 1 | **VBAT** (the JST PH net) | 16.510 |
| 2 | EN (RT9080 enable, pull up; ground it to kill 3V3) | 19.050 |
| 3 | **VBUS** (USB 5 V) | 21.590 |
| 4 | D13 -- GPIO13 | 24.130 |
| 5 | D12 -- GPIO12 | 26.670 |
| 6 | D11 -- GPIO11 | 29.210 |
| 7 | D10 -- GPIO10 | 31.750 |
| 8 | D9 -- GPIO9 | 34.290 |
| 9 | D6 -- GPIO6 | 36.830 |
| 10 | D5 -- GPIO5 | 39.370 |
| 11 | SCL -- GPIO3, I2C1 | 41.910 |
| 12 | SDA -- GPIO2, I2C1 | 44.450 |

**GPIO16–GPIO23 are not on either header.** They are the eight DVI lines
`[SCHEM F4]`. GPIO4 is the NeoPixel. So what is reachable is: A0–A3, D5, D6,
D9–D13, D24, D25, SCK, MOSI, MISO, RX, TX, SCL, SDA -- **21 signal pins**, of
which the deck will use **two or three**.

---

## 3. How it talks to the ESP32-S3

### 3.1 What is actually being sent

The deck holds a **400 × 300 one-bit framebuffer = 15,000 bytes**
`[REPO docs/HARDWARE.md]`, pushed to its own panel in a **measured 4.75 ms at
24 MHz**, against an HPM frame period of 31.25 ms `[REPO HARDWARE.md]`. The
Feather holds **320 × 240 or 400 × 240 sixteen-bit pixels** -- 153,600 or
192,000 bytes -- and blits them out pixel-doubled `[VENDOR F2]`.

**Those two framebuffers are not the same kind of object, and that is the
design decision hiding inside the wiring question.** There are two protocols
available:

| | what crosses the wire | bytes per update |
|---|---|---|
| **A. Mirror** | the deck's 1-bit frame, as pixels | **15,000** per full frame; 600 per full-width 12 px line; **36 per 12 × 24 character** `[REPO HARDWARE.md]` |
| **B. Semantic** | "put this glyph in this cell", "lane 3 is this picture", "clock tick" | tens of bytes per event |

Protocol A on this link gets you a 400 × 300 monochrome image stretched across
640 × 480 of 16-bit colour -- **it throws away everything the video board is
for.** Protocol B costs almost nothing on the wire and lets the RP2040 render
at its own resolution in its own colour depth. **The link should be sized for
B and merely capable of A**, because A is the honest worst case and a
bench-test path.

The deck's own damage tracking (`st7305_damage`, 0.67 µs a cell
`[REPO HARDWARE.md]`) already produces exactly the stream protocol A would
need, so the worst case is a real option and not a hypothetical.

### 3.2 The three interfaces, costed

```
full frame = 15,000 bytes                                        [REPO]
UART 8N1 carries 10 bits per byte                                [DERIVED]
```

| Interface | Rate | Bytes/s | **Full frame** | One 12 × 24 glyph (36 B) | Pins |
|---|---|---|---|---|---|
| **I²C @ 100 kHz** (this build's bus) | 100 kbit/s | ≈ 12,500 | **1.20 s** | 2.9 ms | 0 extra (shares SDA/SCL) |
| I²C @ 400 kHz | 400 kbit/s | ≈ 50,000 | 300 ms | 0.72 ms | 0 extra |
| **UART @ 1 Mbaud** | 1 Mbit/s | 100,000 | **150 ms** (6.7 fps) | 0.36 ms | 2 |
| **UART @ 5 Mbaud** (S3 ceiling) | 5 Mbit/s | 500,000 | **30 ms (33 fps)** | 0.072 ms | 2 |
| **SPI @ 4 MHz** | 4 Mbit/s | 500,000 | 30 ms | 0.072 ms | 3 |
| **SPI @ 10 MHz** (RP2040 slave ceiling) | 10 Mbit/s | 1,250,000 | **12 ms (83 fps)** | 0.029 ms | 3 |

All `[DERIVED]` from: ESP32-S3 UART *"at a speed of up to 5 Mbps"*
`[DS E8 §4.2.1.1]`; RP2040 UART max *"UARTCLK / 16 … 7.8 Mbaud at 125 MHz"*
`[DS R1]`; RP2040 **SPI slave** *"SSPCLK must be at least 12 times faster than
the maximum expected frequency of SSPCLKIN… at the maximum SSPCLK (clk_peri)
frequency on RP2040 of 133MHz … a peak bit rate of 133 / 12 = ~11.083Mbps"*
`[DS R1 §4.4]`.

**I²C is out, on three independent grounds:**

1. **It is 40× too slow at the speed this build runs.** `wiring.md` §2.5 chose
   **100 kHz** deliberately, because three 5880 encoders put 135–270 pF of
   BSS138 drain capacitance on each line. 1.20 s a frame is not a video link.
2. **The pull-up budget is already spent.** `wiring.md` §2.4: the effective
   pull-up with the board's own 2.2 kΩ and four 10 kΩ breakouts is 1170 Ω at
   2.48 mA, and *"the headroom is exactly one more 10 kΩ board; a sixth breaks
   spec at 3.06 mA"*. The 5710 carries **R18–R21, four 10 kΩ resistors**
   `[SCHEM F4]`.
3. **It would add two more BSS138 drains to the bus.** The 5710's
   SDA/SCL go through a BSS138 level-shift pair to the HDMI DDC lines
   `[SCHEM F4]` -- which is precisely the component `wiring.md` §2.5 identified
   as *"the largest uncertain term"* in the capacitance budget. Adding it to
   fix a throughput problem it cannot fix is the worst available trade.

**SPI is the fastest and it is the wrong shape here.** It needs SCK + MOSI +
CS = three pins, which is every free pin the deck has (§3.3), leaving nothing
for flow control -- and flow control is exactly what this link needs, because
the RP2040 has **one core and both PIOs committed to DVI** `[VENDOR F2]` and
must service the slave peripheral with what is left. An SPI slave that is not
ready and cannot say so drops bytes silently.

**UART is the right shape.** Two pins, full duplex, **hardware flow control is
in the ESP32-S3's own feature list** `[DS E8 §4.2.1.1]`, and at 5 Mbaud it
still clears a full 1-bit frame in 30 ms -- one HPM frame period of the deck's
own panel. The deck already speaks UART in two places (MIDI at 31250 baud, the
serial keyboard) `[REPO docs/HARDWARE.md]`, so no new peripheral vocabulary is
introduced.

### 3.3 Which pins are left on the 2×8

`wiring.md` §6.1 commits **nine of the sixteen**: pin 1 (3V3), pins 3 and 4
(GND), pin 14 (SDA), pin 16 (SCL), pins 7, 9, 11 (GPIO1/2/3, the pot wipers).
What is left:

| Pin | Silk | GPIO | Status | Usable for the video link? |
|---|---|---|---|---|
| 2 | `VBUS` | — | USB 5 V, **only while the deck's USB-C is plugged** | power only, and intermittent -- §4.3 |
| 5 | `GP0` | 0 | **BOOT button** + boot strapping pin `[REPO wiring.md §1.2]` | **no** |
| 6 | `GP19` | 19 | **USB D−**, 22 Ω to the Type-C | **no** -- it is how you flash and log |
| 8 | `GP20` | 20 | **USB D+** | **no** |
| 10 | `TXD` | **43** | **U0TXD** -- see §3.4 | **YES** |
| 12 | `RXD` | **44** | **U0RXD** -- see §3.4 | **YES** |
| 13 | `GP17` | **17** | nothing fitted (R7 to SDCS is marked NC) `[REPO wiring.md §1.2]`; digital only, ADC2 | **YES** |
| 15 | `GP18` | 18 | **KEY button** | **no** |

**Three signal pins. Exactly three.** GPIO43 and GPIO44 are the ESP32-S3's
default **U0TXD / U0RXD** -- the datasheet's pin list gives pin 49 = GPIO43 =
U0TXD and pin 50 = GPIO44 = U0RXD, both with **WPU and IE** at reset, i.e. they
idle high, which is a UART mark `[DS E8]`. GPIO17 is an ordinary digital pin.

**Three is enough for the recommendation and not enough for anything bigger.**
If a fourth signal is ever needed (a reset line, a second handshake), it does
not exist on this header: `wiring.md` §1.5 shows the only remaining pins,
GPIO7 and GPIO42, reach nothing but the WROOM module's castellated fillets and
a touch-panel FPC stub. **That is the honest capacity statement, and it is the
one that matters for the three-MCU idea in §7.**

**One more pin problem that is easy to miss: both grounds are already spoken
for.** `wiring.md` §6.1 puts the STEMMA chain's GND on pin 3 and the pot
cluster's GND on pin 4. The video link needs a ground reference too, so **one
of those two pins has to carry two wires**, or the Feather's ground joins the
pot cluster's ground bus on the carrier. Electrically trivial -- the UART
return carries microamps -- but it is a change to the 2×8 male header's
fan-out that `wiring.md` §6.3 describes, and it should be drawn rather than
improvised.

### 3.4 Is UART0 the console? No -- and that is a configuration line, not a fact of the chip

`wiring.md` §1.2 marks pins 10 and 12 **"FREE in practice"** with the note
"UART0 TX / ROM console". That is right, and it is worth saying exactly why,
because the reason is fragile.

**At the application level, UART0 is genuinely free.**
`firmware/sdkconfig.defaults` sets **`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`**
with the comment *"Console on the built-in USB-Serial/JTAG, so that flashing
and monitoring need no buttons and no external adapter"* `[REPO]`. The IDF
bootloader log, the application log and this project's own `>` shell all go to
the USB-Serial-JTAG peripheral on GPIO19/20. **Nothing in the firmware writes
to UART0.**

**At the ROM level, it is not free, and it never will be unless an eFuse is
burned.** esptool's own boot-mode documentation states that *"the second line
printed by the ESP32-S3 ROM (at 115200bps) is a reset & boot mode message"*
`[3P ET]`, and espefuse's ESP32-S3 summary lists
**`UART_PRINT_CONTROL`** -- *"Set the default UART boot message output mode"*
-- with a current value of **Enable** `[3P EF]`.

So **every reset of the deck puts a short burst of 115200-baud ROM text onto
GPIO43.** Consequences:

- **Frame the protocol.** A sync word, a length and a checksum. At a 1–5 Mbaud
  link rate the ROM's 115200-baud burst arrives as garbage bytes and must be
  discarded, not interpreted. You want framing anyway; this makes it
  compulsory.
- **Do not burn the eFuse.** `UART_PRINT_CONTROL` is one-way, and the ROM log
  on UART0 is the only window into a boot failure on a board whose entire
  console lives on the USB peripheral. The cost of keeping it is a few
  discarded bytes per reset.
- **Record the dependency.** `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` is now a
  hardware decision, not a convenience. If anyone ever sets
  `CONFIG_ESP_CONSOLE_UART_DEFAULT` to get a serial log, **the video link
  stops working and the symptom will look like a wiring fault.** That belongs
  as a comment beside the sdkconfig line, and it is the one firmware-adjacent
  thing this document asks for. *(This document does not modify firmware.)*

**And the direct answer to the question as asked:** taking the UART0 pins would
indeed be a poor idea on a board whose console is on UART0. **On this board it
is not, because of that one config line** -- so the user should be told that
the pins are free *conditionally*, not absolutely.

### 3.5 The recommendation

```
  ESP32-S3-RLCD-4.2  P1              Feather RP2040 DVI  5710
  ------------------------------     ----------------------------
  pin 10  TXD  GPIO43  -------->     JP1 pin 3   RX   GPIO1   (UART0 RX)
  pin 12  RXD  GPIO44  <--------     JP1 pin 2   TX   GPIO0   (UART0 TX)
  pin 13  GP17 GPIO17  <-------->    JP3 pin 10  D5   GPIO5   (flow control
                                                               / ready)
  pin 3 or 4   GND     -------->     JP1 pin 13  GND          (shared with an
                                                               existing GND)
```

- **UART0 ↔ UART0.** On the Feather, `RX` = GPIO1 and `TX` = GPIO0 are its
  main UART0 pins and are `Serial1` in Arduino `[VENDOR F2]`. Both are 3.3 V
  logic on both sides -- **no level shifter**.
- **Start at 1 Mbaud**: 100,000 bytes a second, against a semantic protocol
  whose busiest case is a few hundred bytes a frame, and it still clears a
  full-width 12 px text line in **6.0 ms** and a 12 × 24 glyph in **0.36 ms**
  `[DERIVED]`. Raise it only if a
  measurement asks for it; the ceiling is 5 Mbaud on the ESP32-S3 side
  `[DS E8]` and 7.8 Mbaud on the RP2040 side `[DS R1]`.
- **GPIO17 is the handshake.** Either as the ESP32's CTS input driven by the
  Feather (the ESP32-S3 has hardware flow control `[DS E8 §4.2.1.1]`), or as a
  plain "video ready / display attached" line that the Feather drives from
  `HPD_3V`. The second is arguably the better use: it lets the deck know a
  monitor is plugged in without any protocol at all, and the 5710 brings HPD
  out on JP1 pin 1 specifically for that `[VENDOR F2]`.
- **Four wires leave the 2×8 for the video board**, on top of `wiring.md`
  §6.1's nine. The header fan-out goes from nine wires to thirteen.
- **Keep SPI in reserve.** If a pixel mirror is ever wanted, the same three
  pins become SCK/MOSI/CS and the handshake is lost; that trade should be made
  against a measurement, not in advance.

**What this does NOT do:** it does not put the Feather on the I²C chain, so
`wiring.md` §2.4's five-board ceiling is untouched and the fifth slot stays
available. State that plainly, because it is the question anyone will ask next.

---

## 4. Power

### 4.1 What the board draws

**There is no vendor figure for this board's current, and none was found.**
(§8.4.) What follows is a bounded derivation, and it should be replaced by a
USB power meter reading on the bench.

**The RP2040 is overclocked to roughly double its rated maximum to do this.**
The datasheet: *"clk_sys (and clk_peri) have a maximum frequency of 133MHz
across all process, voltage and temperature variations"*, and
*"up to 133MHz (or 200MHz at 1.15V, see Section 2.15.3)"* `[DS R1]`. PicoDVI
runs **252 MHz** for 640×480p60 `[3P, PicoDVI author, via search]` --

```
252 / 133 = 1.89 x the datasheet maximum                        [DERIVED]
252 / 200 = 1.26 x even the raised-DVDD figure                  [DERIVED]
```

**Core current.** The datasheet's Figure 172 plots DVDD current against core
frequency for FFT on both cores, across three temperatures and three DVDD
voltages -- **and it stops at 150 MHz**, where the nine curves read
**30.0 to 38.5 mA** `[CHART, DS R1 Fig. 172]`. A linear fit to the hottest,
highest-voltage curve (6.5 mA at 10 MHz, 38.5 mA at 150 MHz) gives
0.2286 mA/MHz with a 4.2 mA intercept, so:

```
DVDD at 252 MHz  ~=  48 to 62 mA   [CHART, EXTRAPOLATED PAST THE
                                    PUBLISHED RANGE -- see section 8.5]
```

**Peripheral adders**, from the datasheet's own per-peripheral table
`[DS R1 Table 635]`, which gives µA/MHz of DVDD:

| Block | µA/MHz | at 252 MHz |
|---|---|---|
| PIO0 | 12.3 | 3.10 mA |
| PIO1 | 12.5 | 3.15 mA |
| DMA | 2.6 | 0.66 mA |
| IO + Pads | 23.6 | 5.95 mA |
| XIP | 37.6 | 9.48 mA |
| **total** | **88.6** | **22.3 mA** `[DERIVED]` |

**Everything else on the board:**

| Item | Current | Source |
|---|---|---|
| Status NeoPixel, full white | **60 mA** | `[AF Überguide, as cited in REPO wiring.md §4.1]` |
| Status NeoPixel, a single colour at a sane brightness | ~10 mA | `[DERIVED, same basis as wiring.md §4.1]` |
| `chg` LED via R2 = 10 kΩ, `D13` red LED | ≤ 0.5 mA each by inspection | `[SCHEM F4]` `[DERIVED]` |
| 8 MB QSPI flash in XIP | included in the 37.6 µA/MHz above | `[DS R1 T635]` |
| TMDS drive | **see §4.2 -- the monitor pays for it** | `[DERIVED]` |

```
PLANNING FIGURE, to be replaced by a measurement:          [DERIVED]

  typical, video running, NeoPixel dim    ~=  100 mA
  worst case, NeoPixel full white         ~=  150 mA
  worst case with margin for the 1.89x
    overclock the chart cannot reach      ~=  250 mA
```

**Against the Feather's own regulator this is comfortable and against its
thermal limit it is not far off.** Adafruit: *"We use a 500mA peak regulator.
While you can get 500mA from it, you can't do it continuously from 5V as it
will overheat the regulator"* `[VENDOR F2]`. At 250 mA from a **3.3 V** feed
rather than 5 V, the RT9080's dissipation is far lower than the case Adafruit
are warning about -- which is one more argument for §4.4's recommendation.

### 4.2 What the DVI output costs, and who pays for it

This is the part that is easy to get backwards, so here is the arithmetic.

Each of the eight TMDS lines leaves an RP2040 GPIO through a **220 Ω** series
element `[SCHEM F4]`. A DVI/HDMI **sink** terminates each line with 50 Ω to its
own AVcc (nominally 3.3 V). **That termination value is standard but was not
read from the DVI specification in this session -- see §8.3.** If it holds:

```
one line driven LOW:  I = 3.3 / (220 + 50) = 12.2 mA              [DERIVED]
TMDS is differential: exactly ONE of each pair is low at a time
4 pairs x 12.2 mA  =  48.9 mA, continuously, while a display is attached
```

**That current is sourced by the monitor's termination supply, flows down the
HDMI cable, through the RP2040's low-side pad transistor, and returns to the
monitor on the cable's ground.** It does **not** load the Feather's 3.3 V rail
and it does **not** load the deck's TPS63020. What it does do:

- put ~49 mA of continuous DC into the **Feather's ground**, which argues for a
  short, fat ground between the Feather and the HDMI connector's shield -- the
  board already does that, and it is a reason **not** to improvise a ground
  path through the carrier;
- make the **HDMI cable's ground the current return**, so the deck and the
  monitor share a ground the moment a cable is plugged in. On a battery deck
  that is a new ground path and a new ground loop. Worth knowing before
  chasing a hum in the audio path.
- The **line driven high** sits at AVcc and sources approximately nothing;
  the dynamic term is pad switching at up to 126 MHz on eight pads, which is
  inside the IO+Pads figure of §4.1 only approximately. `[NOT SOURCED -- §8.5]`

**The honest conclusion: the DVI output is not free in CPU, PIO, SRAM or
thermals, but the one place it is nearly free is the deck's 3.3 V rail.**

### 4.3 Can the Waveshare's 3V3 rail feed it?

**Short answer: it could carry the current easily, and it is the wrong pin.**

**The current is not the problem.** `wiring.md` §4.3's budget:

```
340 mA Wi-Fi TX peak + 466 mA control surface worst case = 806 mA
+ 250 mA video board worst case                          = 1056 mA
against the TPS63020's 2000 mA at 3.3 V for V_IN > 2.5 V  [DS, wiring.md section 4.3]
-> 944 mA of headroom, 47 %                               [DERIVED]
```

An 18650 stays above 2.5 V for its whole discharge, so the full 2 A applies
`[REPO wiring.md §4.3]`. **The rail is still not the constraint.**

**The pin is the problem, and there are two of them.**

1. **The Feather's 3V3 is an output, and Adafruit say not to drive it.**
   Verbatim: *"Connect an external 3.3V power supply to the 3V and GND pins.
   **Not recommended**, this may cause unexpected behavior and the EN pin will
   no longer work. Also this doesn't provide power on BAT or USB… **You may end
   up damaging your Feather.**"* `[VENDOR F2]` Back-driving the RT9080's output
   is a thing people do and get away with; it is not a thing this project
   should write down as a design.
2. **The deck's header has no battery-voltage pin.** The 2×8 brings out
   **3V3 (pin 1)** and **VBUS (pin 2)** and nothing else on the power side
   `[REPO wiring.md §1.2]`. The 18650's `B+` is not on the header. So the pin
   the Feather actually wants -- **BAT, 3.0–4.2 V** -- is not available without
   soldering to the dev board.

**And a trap worth spelling out.** The Feather's `BAT` pin is tied directly to
the MCP73831's VBAT output `[SCHEM F4]`. If `BAT` is wired to anything on the
deck **and** the Feather's own USB-C is ever plugged in, the charger will try
to charge whatever is on that net at ~200 mA. Wired to the deck's 3V3 rail,
that means pushing current back into the TPS63020's output. Wired to the
deck's 18650, it means **two chargers on one cell** -- the ETA6098 on the
Waveshare and the MCP73831 on the Feather, neither aware of the other.
**Neither is acceptable as a standing arrangement.**

### 4.4 The recommendation

In order of preference:

1. **Give the Feather its own cell on its own JST PH, and let the deck's USB-C
   and the Feather's USB-C both exist.** Fully isolated power, nothing to get
   wrong, and it is what the hardware was designed for. Two costs: (a) the JST
   PH is **top-entry** (§2.5), so the plug cannot live under the acrylic -- the
   cell goes in the sub-carrier volume and the lead comes to the board through
   the carrier, or the two wires are soldered to JP3 pin 1 and a GND pad
   instead of plugged in; (b) the charger is **200 mA** `[SCHEM F4]`, so the
   8000 mAh pack the user mentions would take **40 h** from the Feather's own
   USB-C `[DERIVED]`. Charge a pack that size externally, or on the deck.

2. **Feed `BAT` from the deck's 18650 `B+` through a Schottky**, and **never
   plug the Feather's USB-C while it is wired in.** One cell, one charger, one
   power switch -- the "tank" the user is after. The Schottky blocks the
   MCP73831 from ever pushing into the cell. Cost: a soldered tap on the
   Waveshare's battery holder, which is a modification to the dev board and
   should be a deliberate decision, not a convenience. The diode drop
   (≈0.3 V) still leaves 2.7–3.9 V into the RT9080, whose dropout at these
   currents is well inside that -- **but the RT9080 dropout figure here is
   distributor data, not Richtek's own PDF (§8.6).**

3. **VBUS (pin 2) → the Feather's `USB` pin.** Rejected. VBUS exists only while
   the deck's USB-C is plugged `[REPO wiring.md §1.2]`, so there would be no
   video on battery -- which defeats the point of the deck -- and Adafruit warn
   that back-powering the USB pin *"could confuse or damage your computer"*
   `[VENDOR F2]`.

4. **3V3 → the Feather's `3V3`.** Rejected on Adafruit's own words above. It
   will work. It is not what this project writes down.

**Does it need its own battery connector?** It **has** one, and the right
answer for a first build is to use it -- option 1 -- because it keeps the two
boards electrically independent while the protocol in §3 is still being
written. Move to option 2 only once the link is proven and the user commits to
a single-cell deck.

---

## 5. Where it goes

### 5.1 The band, measured

Rebuilt from `v2/layouts/slab.py`, `v2/layouts/_common.py` and
`v2/tools/layout.py` `[REPO]`, in the panel's own centred coordinates
(origin at the deck's centre, +y toward the rear):

```
deck      294.750 x 185.450                      half-height 92.725
walls     3.200 side and rear, 4.400 bottom
interior  288.350 wide, walls at x = +/- 144.175
band      76.200 tall, y = +13.325 (front) ... +89.525 (rear interior wall)
mid       y = +51.425
```

What occupies the band, left to right:

| | x from | x to | width | y from | y to |
|---|---|---|---|---|---|
| interior wall | — | −144.175 | — | — | — |
| **free** | −144.175 | −137.080 | **7.095** | +13.325 | +89.525 |
| ESP32 board bay (93.90 × 70.30) | −137.080 | −43.180 | 93.900 | +16.275 | +86.575 |
| **free -- gap A** | −43.180 | −14.605 | **28.575** | +13.325 | +89.525 |
| encoder / pot grid | −14.605 | +86.995 | 101.600 | +19.675 | +83.175 |
| **free -- gap B** | +86.995 | +115.570 | **28.575** | +13.325 | +89.525 |
| fader bay (21.59 × 76.20) | +115.570 | +137.160 | 21.590 | +13.325 | +89.525 |
| **free** | +137.160 | +144.175 | **7.015** | +13.325 | +89.525 |

`[DERIVED from REPO slab.py: GRID_X 36.195, GRID_W 101.600, GAP 28.575,
DISP_BD 93.900 × 70.300, SLIDE_X 126.365, BAY_H 21.590]`

**The Feather's 50.800 × 22.860 fits nowhere in that table lying along x, and
fits in either 28.575 mm gap lying along y.** That is the whole placement
problem in one line: **turn it 90°.**

### 5.2 The Z budget decides it before the XY does

`stack.md` §2.6's block `[REPO]`:

```
  acrylic UNDERSIDE                     Z = +6.35
  CARRIER TOP FACE = wiring plane       Z =  0.00   <- datum
  breakout PCB front faces              Z = -0.15
  breakout PCB bottoms                  Z = -1.72
  wiring channel floors                 Z = -3.00
  carrier bottom face                   Z = -7.62
```

**Pocketed like every other breakout, the 5710 just fits:**

```
PCB front face          -0.15
HDMI shell top          -0.15 + 6.185  =  +6.035
acrylic underside                         +6.35
clearance                                  0.315 mm            [DERIVED]
```

**0.315 mm is a fit and it is not a design.** And it does not survive contact
with the thing that has to plug in:

- An **HDMI Type A plug** mates into a shell whose opening sits in the middle
  of the connector's 6.185 mm height, i.e. roughly **Z +1.4 to +6.0**. The
  plug's **overmould** is a much larger object than the shell -- it is the
  part your hand holds. **Its envelope is NOT SOURCED (§8.7) and must be
  measured on the actual cable**, but on any ordinary HDMI cable it is taller
  than the 6.35 mm of air between the wiring plane and the acrylic. **It
  cannot get in.**
- The battery JST PH and the STEMMA QT are both **top-entry** (§2.5). Neither
  can be plugged with the acrylic on.
- And `stack.md` §3.4 is explicit that **a laser cannot pocket 12.70 mm
  acrylic**, so "relieve the underside of the slab locally" is not available.
  The only acrylic move is a **through-notch from the slab's rear edge** --
  which is a visible bite out of the front panel, 12.70 mm deep, and a
  decision about the object's face rather than about its wiring.

**So the board does not go on the wiring plane. It goes below it**, in a
through-window in the carrier, in the same sub-carrier volume that the
Waveshare board's own 14.1 mm of body already has to occupy (the dev board is
14.1 mm thick `[REPO wiring.md §1.1]` in a carrier 7.62 mm thick, so that
volume exists whether or not the Feather uses it).

```
RECOMMENDED Z                                                   [DERIVED]

  wiring plane                                   0.00
  Feather PCB front face                        -6.00
  HDMI shell top                 -6.00 + 6.185 = +0.185   (level with the
                                                           plane, visible)
  Feather PCB bottom             -6.00 - 1.570 = -7.570
  HDMI shield tabs               -7.570 - 0.630 = -8.200  (0.58 below the
                                                           carrier's own
                                                           bottom face)
  plug centreline                               ~-3.5     (clear of the
                                                           acrylic entirely)
```

At that height the acrylic is **untouched** -- no notch, no cutout, consistent
with the user's "the acrylic will also entirely go over the display" -- and the
board reads through the slab as a **recessed well**, which is the look the user
described wanting. The carrier's own 7.62 mm of thickness is cut through under
the board, and four **M2 bosses stand up from the back panel** (or hang from
the carrier's underside) on the trapezoid of §2.2.

### 5.3 The site

**Gap B -- between the encoder grid and the fader -- turned 90°, HDMI out the
rear wall.**

```
  x centre     +101.600   = 4.000 in = 160 x 0.635, so it lands on the
                            layout's own grid                   [DERIVED]
  x extent     +90.170 ... +113.030
  clearance    3.175 mm to the grid's right edge (+86.995)
               2.540 mm to the fader bay's left edge (+115.570)

  y            PCB rear edge butted to the interior rear wall at +89.525
  y extent     +38.725 ... +89.525
  clear floor  +13.325 ... +38.725 = 25.400 mm in front of the board

  HDMI aperture, centred on the board:  x +94.100 ... +109.100   [DERIVED
                                        from the 15.00 mm envelope, section 2.4]
```

**Why gap B and not gap A:**

| | gap A (board ↔ grid) | gap B (grid ↔ fader) |
|---|---|---|
| width | 28.575 | 28.575 |
| wiring already crossing it `[REPO carrier.py]` | **two runs** -- the I²C first leg doglegs at x = −41.910 and runs horizontally at y = +70.475; the ADC trunk doglegs at the same x and runs horizontally at y = +17.770 | **one run** -- `E3-FADER` crosses horizontally at y = +85.715 |
| distance to the 2×8 header exit (x = −41.910) | ~13 mm | ~145 mm |
| clash with the Feather's footprint | the I²C leg at +70.475 runs **through** it; the exit strip to the encoder bay is only 2.540 mm wide | one run, at the very top, **bridgeable** |

Gap A is closer to the header and that is its only virtue. It is crossed twice,
and the escape route between a Feather at x −28.575 and the encoder bay at
−14.605 is 2.540 mm -- enough for a 2 mm channel and not for the 3 mm one
`carrier.py` uses for the ADC trunk `[REPO]`. **Gap B is crossed once, at the
top, where a bridge is cheap.** The extra ~130 mm of UART wire costs nothing:
a 1 Mbaud 3.3 V signal over 150 mm of wire inside a grounded enclosure is not
a transmission-line problem, and unlike the I²C pair this run has no
capacitance budget to spend (`wiring.md` §6.2 rule 2 applies to I²C, not here).

**Flat, not raised, not inverted.** Components up, HDMI up, PCB parallel to the
carrier, sunk 6 mm (§5.2). Inverting it would point the HDMI mouth at the back
panel and bury the only connector that has to reach a wall.

**Why the rear wall and not an end wall:** the deck's left end is already the
dev board's port face -- USB-C and the microSD slot are on that edge
`[REPO wiring.md §1.1, README]` -- and the right end is 7.015 mm of material
behind the fader. The rear wall is **288.35 mm of otherwise unused edge**, and
putting the video out there keeps the two I/O faces separate: **power and
storage on the left end, video at the back.** For a desk instrument that is
also the right ergonomics -- the HDMI cable, which is the stiffest thing that
will ever be plugged into this object, leaves away from the player.

### 5.4 The aperture

```
  rear wall thickness                   3.200                  [REPO layout.py]
  connector face, with the PCB rear
    edge on the interior wall           +89.525 + 0.880 = +90.405
  outer wall face                       +92.725
  plug must enter                       2.320 mm of wall       [DERIVED]
```

- Cut the wall slot to the connector's **15.00 × ~6.2 mm** envelope plus
  clearance -- draw **16.0 × 7.2** and let the shell locate the plug, not the
  plastic.
- **The overmould is the governing dimension and it is not sourced (§8.7).**
  Measure the cable you will actually use and open the slot to it, or counterbore
  the outer face. A **right-angle (down-facing) HDMI cable** is strongly
  preferred on a desk instrument: it halves the depth the connector sticks out
  and stops the cable levering on a surface-mount part rated 0.5 A per contact
  `[3P F7]`.
- The connector is **centred on the board to 0.018 mm** (§2.4), so the slot is
  centred on x = +101.600. No offset.

---

## 6. Conflicts with the existing documents

Listed so they are decided rather than discovered.

1. **`v2/layouts/carrier.py`, the `E3-FADER` route.** It runs
   `(+86.995, +70.475) → (+86.995, +85.715) → (+126.365, +85.715)`, crossing
   the proposed footprint at **y = +85.715** `[REPO]`. Two fixes: **bridge it**
   over the carrier window on the 1.20 mm roof `stack.md` §5.5 already uses, or
   **drop `fader_in` to +31.750**, which adds about 104 mm of QT cable. At
   100 kHz with a 400 pF ceiling and a measured-capacitance plan already in
   `wiring.md` §8.3, 104 mm is immaterial. **The bridge is preferred because it
   keeps `carrier.py`'s "the daisy chain and the signal flow are the same line"
   intact.**

2. **`carrier.py`'s own rule: "no channel is allowed to pass under a board."**
   A bridge over a *window* is not a channel under a *board* -- but it is close
   enough that it deserves a sentence in that file rather than a silent
   exception.

3. **Both GND pins on the 2×8 are allocated** (`wiring.md` §6.1 uses pins 3 and
   4). The video link's ground has to double up on one of them, or join the pot
   cluster's ground bus. The 2×8 male header's fan-out goes from **nine wires
   to thirteen**. §3.3.

4. **`stack.md` §2.6's Z block has no row for a second board, and no row for
   the sub-carrier volume at all** -- the Waveshare board's own 14.1 mm of
   body `[REPO wiring.md §1.1]` is not in it either, though a 7.62 mm carrier
   cannot contain it. **That volume should be drawn before the Feather is put
   in it.** It is the one piece of this placement resting on a dimension the
   repository has not written down.

5. **`stack.md` §5.4's pocket recipe assumes side-entry STEMMA QT sockets**
   ("local 2.96 mm relief under each STEMMA QT socket… the connector drops into
   the channel it feeds"). **The 5710's QT and JST PH are top-entry** (§2.5),
   so that recipe does not transfer. Nothing is plugged into either on this
   board in the recommended design; if that changes, the relief is on the wrong
   face.

6. **`v2/layouts/README.md` says the display board's "left edge lands 1.975 mm
   off the interior wall".** Recomputing from `slab.py`'s own constants gives
   **7.095 mm** (§5.1). One of the two is stale -- most likely the README,
   written against an earlier deck width. **It does not change this placement**
   (the left margin is unusable either way) but it should be reconciled by
   whoever owns that file, because it is a number someone will trust.

7. **`v2/layouts/slab.py` does not currently render.** `v2/tools/layout.py`
   calls `mod.build(Panel, _ctl, display, plate, bay, keyboard, route)` with
   seven arguments; `slab.py`'s `build` takes six (`carrier.py`'s takes seven)
   `[REPO]`. Every geometric figure in §5.1 was therefore computed from the
   module constants by hand rather than read off the emitted SVG, which is a
   weaker check than the repo's own convention wants. **Not fixed here --
   this document does not modify code** -- but recorded, because §5.1's table
   would normally have been verified against the drawing.

8. **The I²C ceiling is NOT consumed.** `wiring.md` §2.4's "five external
   STEMMA boards, and the fifth is the last one" is untouched by this board,
   because §3.2 keeps it off the bus. Four are used; the fifth slot remains.

---

## 7. The three-MCU avenue, noted rather than designed

The user asked for these to be written down as next steps, not built now.
Three things this analysis already establishes about them:

- **The 2×8 header has three signal pins and they are now spent.** §3.3. A
  Teensy 4.1 as a clock engine, or a fourth board as a sonification engine,
  **cannot be wired to the ESP32 through the expansion header at all** once
  the video board has UART0 and GPIO17. The only remaining pins, GPIO7 and
  GPIO42, reach nothing but the WROOM module's castellated fillets
  `[REPO wiring.md §1.5]`. **The next MCU must therefore either hang off the
  existing I²C bus (one slot left, §6 item 8), or chain off the Feather rather than
  the ESP32, or the deck needs a different main board.** That is a real
  architectural fork and it is worth knowing before the parts are bought.
- **A chained topology is the one that scales.** ESP32 (orchestrator) → UART →
  RP2040 (video) → its own UART1 / SPI / I²C1 → the next board. The 5710 has
  **21 free signal pins on its headers** (§2.7) and only three are used. The
  orchestrator stays at the top; the boards below it fan out. **The pin
  shortage is at the ESP32, not downstream.**
- **The second battery changes the power question, not the pin question.** §4.4
  option 2 (one cell, one charger, a Schottky per board) is the arrangement
  that scales to three or four boards. Option 1 (a cell each) does not -- four
  MCP73831s at 200 mA on four cells is four charge states to reason about.
  **If the 8000 mAh pack is the plan, design the single-rail distribution
  now**, because it is the thing that gets harder to retrofit with each board
  added.

---

## 8. NOT SOURCED

Stated loudly, per the project's rule that an unsourced number is a bug.

1. **Whether a DVI signal out of an HDMI connector will lock a capture card, a
   video switcher or a scaler.** §1.2 reasons that a monitor will sync and that
   some HDMI sinks expect InfoFrames; **none of this was tested, and no
   specification was read.** For a live rig whose output may be captured, this
   is the first thing to try with the real board and the real destination.
2. **PIDs 4984, 6055, 6130, 6003, 6267 and 6200** (§1.3, second table) come
   from search results and were **not confirmed on adafruit.com in this
   session**. The first table's ten PIDs were.
3. **The DVI/HDMI sink's TMDS termination (50 Ω to AVcc).** §4.2's 48.9 mA
   rests on it. It is the standard value and it was **not read from the DVI
   1.0 or HDMI specification here.** *Measure:* a current meter in the
   Feather's ground lead with and without a monitor attached answers it
   directly.
4. **The 5710's total current draw.** No vendor figure exists and none was
   found. §4.1's 100/150/250 mA are derived and bounded, not measured.
   *Measure:* a USB power meter on the Feather's own USB-C, with DVI running
   into a real monitor, NeoPixel off and NeoPixel white.
5. **RP2040 DVDD current above 150 MHz.** Figure 172 stops there
   `[DS R1]`; §4.1's 48–62 mA at 252 MHz is a **linear extrapolation past the
   published range** of a curve measured on different silicon running a
   different workload. The dynamic pad current of eight TMDS pads switching at
   up to 126 MHz is also not separable from the IO+Pads figure. **This is the
   largest uncertain term in §4.**
6. **RT9080-3.3 dropout and quiescent current.** Richtek's own PDF was not
   retrieved. Distributor data gives 600 mA, 0.31 V typical dropout at 600 mA,
   and disagrees with itself on I_Q (2 µA vs 4 µA) `[3P RT]`. §4.4 option 2
   leans on the dropout figure. *Fetch:* `richtek.com` product page → datasheet.
7. **The HDMI Type A plug's overmould envelope.** §5.4's wall slot depends on
   it and §5.2's rejection of the flush mounting is argued from it. The Molex
   document gives the receptacle's electrical ratings but no mating-plug
   envelope `[3P F7]`, and Molex's own dimensional drawing was not retrieved.
   *Measure:* calipers on the cable you will actually use, and buy the
   right-angle one.
8. **The Molex 47151-0001 body dimensions from Molex.** §2.3's and §2.4's
   envelope is from **Adafruit's CAD model**, which is a model of the assembly,
   not the connector maker's drawing. The two agree on "full size Type A" and
   on 19 circuits; they have not been reconciled dimension by dimension.
   *Fetch:* Molex's "Drawing (PDF)" for the part, referenced but not
   downloaded `[3P F7]`.
9. **The sub-carrier volume's depth** (§5.2, §6.4). Required for the
   recommended Z and **not written down anywhere in this repository.** It is
   bounded below by the Waveshare board's 14.1 mm assembled thickness
   `[REPO wiring.md §1.1]` and by the back panel's 3.00 mm `[REPO stack.md
   §2.6]`, but the actual clear height between the carrier's bottom face and
   the back panel has never been stated.
10. **Whether the 5710's battery JST and STEMMA QT are the parts assumed.**
    §2.5's envelopes are measured from the model and the entry direction is
    read from a vendor photograph `[F8]`; **no JST part number appears on the
    schematic** for either, so the plug heights quoted in §5.2 are from part
    knowledge, not a datasheet. They matter only if something is ever plugged
    in.
11. **The ESP32-S3 ROM's UART0 output pins.** esptool documents the 115200 bps
    boot message `[3P ET]` and the datasheet gives U0TXD = GPIO43 `[DS E8]`,
    but **no single source read here states that the ROM print goes to those
    pins.** The inference is sound and the mitigation in §3.4 (framing) is
    cheap enough that it should be taken either way. *Confirm:* a scope or a
    USB-TTL adapter on P1 pin 10 across a reset.
12. **The mounting-hole positions of §2.2 come from the CAD model, not from a
    caliper on a real board.** They are internally consistent (clean imperial
    values: 0.100 in from the short edges, 0.100/0.075 in from the long edges,
    1.800 in between columns) and corroborated by the fab print's own drill
    circles `[FAB F3]`, which is why item 3 of §0 states them as a finding. **Caliper
    the board when it arrives, before cutting the carrier** -- the same rule
    `wiring.md` §8.9 applies to the Waveshare board.

---

## 9. One-line summary

**Adafruit 5710 · DVI-D out of a full-size HDMI Type A (Molex 47151-0001), no
audio · 50.80 × 22.86 × 1.57 PCB, connector 15.00 wide and 6.185 above the
PCB on the far short end, centred to 0.02 mm · mounting holes Ø2.50 on a
TRAPEZOID, 45.72 × 17.78 at the USB end and 45.72 × 19.05 at the HDMI end · no
castellations, nothing on the long edges but two 0.1 in rows 0.05 in in · link
it on UART0 at GPIO43/44 with GPIO17 as the handshake, framed, because the ROM
still prints on U0TXD · never on I²C · power it on BAT, never on 3V3 · and put
it turned 90° in the 28.575 mm gap at x = +101.600, HDMI out the rear wall,
sunk 6 mm BELOW the wiring plane so the acrylic stays whole and the plug can
reach it.**
