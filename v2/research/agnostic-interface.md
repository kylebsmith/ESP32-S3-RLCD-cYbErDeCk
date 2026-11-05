# The agnostic control surface — how an unlabelled panel stays usable

*An enclosure and control-surface study. Every number carries a source tag.
Where I am reasoning rather than citing, the paragraph says so. Numbers I could
not trace are in §11, **NOT SOURCED**, not estimated. Nothing here proposes
firmware; where the display's content is load-bearing it is stated as a
**constraint the geometry places on the display**, which is a layout fact, not a
software design.*

Source tags: `[MIL]` MIL-STD-1472H, read from the PDF named below and quoted
verbatim from its own text; `[STL]` measured by me from the vendor model in
`/home/user/adafruit/adafruit_cad_parts`; `[REPO]` a committed file in this
repository; `[VENDOR]` a manufacturer page; `[REVIEW]` a published review or a
vendor-hosted customer review; `[DERIVED]` arithmetic on the above, shown;
`[REASONED]` my judgement, labelled as such.

The MIL copy used throughout is
<https://cvgstrategy.com/wp-content/uploads/2023/04/MIL-STD-1472G.pdf> — the
file is named …1472G, its running header reads **MIL-STD-1472H**, and it is
cited as H, exactly as `precedents.md` §6 does. I downloaded it and read the
clause text directly rather than relying on the earlier summary; two of the
numbers in `precedents.md` and `envelope.md` needed correcting as a result, and
both corrections are in §5.

---

## 0. Verdict up front

**1. The premise of this study is wrong, and in the design's favour. You *can*
align four encoders to the 84.80 mm display.** The 21.2 mm figure in
`precedents.md` §8.2 and `envelope.md` ("84.80 / 4 = 21.2 mm per encoder, about
half the ergonomic minimum") divides the display into four equal columns and
then asks the encoders to meet them. That is the wrong direction. The chosen
part fixes the encoder pitch at **19.050 mm** `[STL]`, which is *less* than
21.2, so four encoders span **76.200 mm** — the Quad Rotary's whole board — and
the display's active area is **84.80 mm** `[REPO]`. The control row is
**8.60 mm narrower than the display**, 4.30 mm of margin each side `[DERIVED]`.
Push's mechanism is available. It was never geometrically closed; it was closed
by an arithmetic that assumed the old 39 mm encoder pitch, which the Quad Rotary
makes impossible anyway (`envelope.md`, "Settled by arithmetic").

**2. And the two grids are commensurate at 15 characters.** The panel is
400 × 300 px at a 0.212000 mm pixel pitch, with a 6 × 12 px text cell
`[REPO: docs/HARDWARE.md, docs/OS.md]`. One character cell is 1.2720 mm wide, so

```
15 cells = 90 px = 19.0800 mm        vendor encoder pitch = 19.0500 mm
                                     error = +0.0300 mm  (+0.157 %)      [DERIVED]
4 fields x 15 cells = 76.3200 mm     Quad Rotary board    = 76.2000 mm
                                     error = +0.1200 mm, i.e. 0.06 mm per end
60 of the 66 available columns used; 6 spare = 7.632 mm of margin
```

14 cells misses by −6.52 % and 16 by +6.84 % `[DERIVED]`. **15 is the number,
and it is the only one.** The display's own text grid and the vendor's control
pitch agree to three hundredths of a millimetre, which is one seventh of a
pixel. This is the single most design-changing fact in the document.

**3. Which means the panel can be unlabelled *and* legible, and norns' fate is
avoidable for a reason stronger than good intentions.** Sound on Sound's
complaint about norns is two separate complaints — "nothing is labelled" **and**
"There's also no obvious alignment between the encoders/buttons and the screen"
`[REVIEW]`. The second is the fatal one, and it is the one this geometry fixes.

**4. MIL-STD-1472H never counted labels as a blind-operation code in the first
place.** Read 5.1.1.6 with its own cross-references and the conflict shrinks:

> **5.1.1.6 Blind operation.** "Where 'blind' operation is necessary, hand
> controls shall be shape coded, or separated from adjacent controls by at least
> 125 mm (5 in) **(see 5.1.1.4.2 and 5.1.1.4.4)**." `[MIL]`

5.1.1.4.2 is **location** coding; 5.1.1.4.4 is **shape** coding. Labelling is
not referenced, and MIL's own Table IV marks labelling as *not* improving
"nonvisual identification (tactile and kinesthetic)" while location, shape, size
and mode of operation all do `[MIL]`. **The engraving the user wants to delete
was never doing the blind-operation job.** Deleting it costs exactly one thing,
and Table IV names it precisely: labelling is the *only* coding mode that
"Requires little (if any) training; is not subject to forgetting" `[MIL]`. That
is the whole price, it is a learnability price and not a reachability price, and
it is payable — with a screen legend, with position, and with the fact that this
instrument's *other* surface is a fully-legended 61-key QWERTY.

**5. 61 labelled controls against 13 unlabelled ones.** The Keychron K9 Pro is
61 keys `[VENDOR]`, every one printed. The plate carries 4 encoders + 2 sliders
+ 4 keys + 3 pots = 13 `[REPO: the decided parts list]`. norns has 6 controls
and zero legends anywhere on the unit `[REVIEW]`. This instrument is 82 %
legended by control count `[DERIVED]`, and the legended surface is the one that
holds the naming language (`>kick`, `>cut`). That is a material difference from
the cautionary precedent and it should be said out loud before anyone panics
about norns.

**6. One code is needed, not a vocabulary.** Mode of operation already
distinguishes the four classes for free — nobody confuses a slider with a knob
by touch, and MIL lists mode of operation as a coding type that improves
nonvisual identification `[MIL, table IV]`. The only genuine confusion on this
panel is **pot versus encoder**, because both are knobs. That is **one**
distinction, and the standard codes it twice over at no cost: a diameter step
(13.00 → ≥26.00 mm, §5.3) and a pointer that the standard *requires* on the pot
and honesty forbids on the encoder (§5.4).

**7. The 125 mm spacing route is closed by 0.65 mm, and that is not a rounding
error you can argue away.** Three controls in a row at MIL's 125 mm
edge-to-edge:

```
3 x 13.00 mm knob + 2 x 125 mm  =  289.00 mm
interior width, 294.75 - 2 x 3.20 =  288.35 mm        [DERIVED from keyboard-k9pro.md 11.1]
                                     ---------
                                       SHORT by 0.65 mm
largest knob that would fit:          12.7833 mm
```

and a knob below 13 mm fails 5.1.1.4.4.2, which requires a control distinguished
by shape alone to be ≥13 mm wide `[MIL]`. **So on this panel the two routes of
5.1.1.6 are mutually exclusive: you can have the 125 mm spacing or a
shape-codeable knob, not both.** Take the shape code. The arithmetic, the
alternatives and the stated departure are in §5.6.

**8. What gets engraved: one digit per aperture, and nothing else.** monome's own
documentation calls the controls **E1 E2 E3** and **K1 K2 K3** `[monome docs]`
while the hardware prints nothing `[REVIEW]`. The ordinal names already exist;
they are simply not on the device. Three other instruments print exactly that
and nothing more: Electra One prints **KNOB1–KNOB12**, OP-1 prints **T1–T4** and
**sound 1-8**, Elektron prints **A–H**. The rule that makes this safe is in §8:
**engraving may describe the mechanism; it may never describe the function.** An
ordinal and an end-stop tick are mechanism. "cutoff" is a promise the software
has not made.

---

## 1. The conflict, stated exactly

The directive:

> *"The goal is to make the interface agnostic so that reconfiguration is
> implied by the simplicity."*

The two constraints `precedents.md` found pulling the other way:

- **5.1.1.6**, above: blind-operated controls shape coded or ≥125 mm apart.
- **No display alignment**, so no soft keys, so norns.

### 1.1 MIL Table IV, which is the honest frame for the whole question

Reproduced from the standard's own table `[MIL]`. An X means the mode has that
property.

| | Location | Shape | Size | Mode of operation | Labeling | Color |
|---|---|---|---|---|---|---|
| Improves visual identification | X | X | X | | X | X |
| **Improves nonvisual identification (tactile and kinesthetic)** | **X** | **X** | **X** | **X** | | |
| Helps standardization | X | X | X | X | X | X |
| Aids identification under low illumination and colored lighting | X | X | X | X | (if transilluminated) | (if transilluminated) |
| May aid in identifying control position (settings) | | X | | X | X | X |
| **Requires little (if any) training; is not subject to forgetting** | | | | | **X** | |
| *Disadvantage:* may require extra space | X | X | X | X | X | |
| *Disadvantage:* affects manipulation of the control | X | X | X | X | | |
| *Disadvantage:* limited number of coding categories | X | X | X | X | | X |
| *Disadvantage:* less effective with gloves | | X | X | X | | |
| *Disadvantage:* controls must be viewed | | | | | X | X |

Two rows decide the argument. Labelling is the **only** mode in the training
row — that is the real cost of the user's decision, and it is a cost in *first
hour*, not in *performance*. And labelling is **absent** from the nonvisual row,
alongside colour — which is to say the engraving was never what let you find a
control in the dark. Location, shape, size and mode of operation were.

The brief and the standard are therefore much closer than they look. The
remaining work is to pay the learnability bill, and MIL has nothing to say about
that because MIL's panels do not have screens on them. That part is §4 and §6.

### 1.2 Colour is available here — and MIL rules it out as the primary code anyway

Worth settling now, because `precedents.md` §8.2 proposes substituting an
engraved glyph for OP-1's colour on the grounds that "the deck's display is
1-bit reflective, so *colour* cannot come from the screen". True of the screen,
but OP-1's colour is on the *knobs*, and the chosen parts all carry RGB LEDs:
the 5752 Quad Rotary, the 5295 NeoSlider and the 4980 NeoKey each have four
NeoPixels `[REPO: parts.md §2–4]`. So OP-1's mechanism is physically available
at the control, software-set, and therefore belongs to the layer that *should*
change meaning.

It cannot be the primary identification, and the standard says so three times:

- **5.1.1.4.5** "Colors shall not be the sole means to control coding." `[MIL]`
- **5.1.1.4.5.9.1** "Color coding shall not be used as the primary
  identification medium if the spectral characteristics of the ambient light or
  the user's adaptation to that light varies as the result of factors such as
  solar glare, filtration of light, and variation from natural to artificial
  light." `[MIL]` — that is a stage, exactly.
- **5.1.1.4.5.7** "When color coding must be used to relate a control to its
  corresponding display, the same color shall be used for both the control and
  the display." `[MIL]` — which a 1-bit panel cannot do.

**So colour is the right home for meaning and the wrong home for identity.**
That split is the architecture of the whole answer, and it is pleasing that it
falls out of the standard rather than out of taste.

One measured caveat that has to go in the blocking list: on the 5752 the four
NeoPixels sit **at exactly the encoder centres, underneath the encoder bodies**
`[REPO: parts.md §2.4, measured from the STEP]`, and an EC11 body has no slot
for the light to escape through. Per-encoder colour is **unverified on the
chosen quad board**. The 4991/5880 single-encoder breakouts do not have this
problem. The user's decision list permits either ("5880 or 5752"), so this is a
live trade, not a re-opened decision. §12.

---

## 2. The alignment finding, in full

This section replaces `precedents.md` §8.2's conclusion and `envelope.md`'s open
question 3. It does not touch any decision the user has made.

### 2.1 The measurement

I measured the vendor model directly rather than taking the pitch on trust.
From `5752 Quad Rotary Breakout.stl` `[STL]`, with the board centred on the
origin and +Z toward the back:

| | measured |
|---|---|
| board outline | X −38.100…+38.100, Y −10.795…+10.795 → **76.200 × 21.590** |
| **encoder shaft centres** | **X = −28.575, −9.525, +9.525, +28.575; Y = 0.000** |
| pitch | **19.050 mm exactly**; outermost centres 57.150 apart |
| shaft | Ø **5.998** (nominal 6.00) |
| encoder body | **15.000 × 12.300**, so 4.050 mm of clear air between adjacent bodies |
| front projection | shaft tip at Z = −21.400 → **21.400 above the PCB front face** |
| back projection | Z = +5.870 → 5.870 − 1.570 = **4.300 below the back face** |

The last two lines agree with `parts.md` §1 to the hundredth, from an
independent model and an independent method, which is worth having.

**A by-product that closes `envelope.md`'s open item on the encoder datum.** In
one datum — PCB front face = 0, measuring forward `[STL]`:

```
0.000 -> 1.221   clear
1.221 -> 7.499   encoder body, 15.00 x 12.30
7.499 -> 14.100  threaded bushing, 6.601 long  (consistent with M7 x 0.75 x 6.5)
14.100 -> 21.400 shaft, O6.00, 7.300 long
```

So the panel's mounting face sits **7.50 mm above the PCB front face**, the
bushing offers **6.60 mm** of thread, and with a 4.00 mm plate there are
**7.30 mm of Ø6 shaft plus 2.60 mm of exposed bushing** above the panel for a
knob to grip `[DERIVED]`. The Bourns datasheet's 7.0 mm and the STEP's 21.40 are
not in conflict — they are the body height and the shaft tip measured from the
same face, 7.50 and 21.40.

### 2.2 The alignment arithmetic

```
display active area                               84.80 mm   [REPO]
Quad Rotary board                                 76.20 mm   [STL]
margin each side                                   4.30 mm   [DERIVED]
outermost encoder centre                         +-28.575    [STL]
   ... which is 13.825 mm inside the display edge           [DERIVED]
outermost encoder BODY edge                      +-36.075
   ... which is  6.325 mm inside the display edge           [DERIVED]
```

Every part of the encoder row — centres, bodies, board — lies inside the
display's active width. The display does not need to grow. The bill of materials
does not change.

### 2.3 Why the label columns are pitched at 19.05 and not at 84.80/4

This is the rule that has to be written down or it will be got wrong once:

```
display_width / 4 = 21.200 mm  ->  column centres at +-10.600, +-31.800
vendor pitch      = 19.050 mm  ->  encoder centres at +- 9.525, +-28.575
mismatch: 1.075 mm inner, 3.225 mm outer                       [DERIVED]
```

Divide the display into four equal columns and the outer labels sit 3.225 mm off
their encoders — which is "no obvious alignment" starting to happen again, in
miniature. **Pitch the label fields at 19.050 mm, centred on the deck
centreline, and let the leftover 7.632 mm be margin.** Müller-Brockmann's point
about margins mattering as much as modules `[Ben Morris, cited in precedents.md
§8.2]` is load-bearing here rather than decorative: the margin is what proves
the four fields are a group of four and not a crop of something wider.

### 2.4 The same four columns serve all three bay types

The vendor's Pattern B is the same outline and the same element pitch on all
three boards `[REPO: parts.md §1.1]`, and the NeoKey's key centres are at board
X = 9.525, 28.575, 47.625, 66.675 — that is **±9.525, ±28.575 from the board
centre, identical to the encoders** `[REPO: parts.md §4; STL for the 5752]`.

So one display column scheme — **four fields of 15 characters at 19.050 mm
pitch** — names four encoders, or four keys, or (as one 60-column field) one
slider, with no change to the geometry. **The agnostic grid and the soft-label
grid are the same grid.** That is "reconfiguration implied by the simplicity"
achieved in geometry rather than asserted in styling, and it is the best answer
this study found to the user's brief.

### 2.5 The consequence that constrains the layout: one bay per row gets columns

The display is centred on the deck centreline, and `tools/layout.py`'s check 6
enforces mirror symmetry about it `[REPO]`. A bay is 76.200 mm wide and the
interior is 288.35 mm, so three bays fit side by side (228.60 mm) and four do
not (304.80 mm) `[DERIVED]`. Of the bays in a row, **only the one centred on
x = 0 is inside the display's width**; its neighbours start at |x| ≥ ~85.7 mm
and the display ends at |x| = 42.40 `[DERIVED]`.

**Therefore: exactly one bay per row can be screen-labelled, and it must be the
centred one.** The controls that most need naming are the encoders, so the
encoder bay is the centred bay. Everything else on the plate is named by
position alone, which §6 argues is sufficient and is in any case what the rest
of the world does.

### 2.6 How many bays stack, which the band depth decides

`keyboard-k9pro.md` §11.2 brackets the control band at 22.79–83.60 mm, the upper
figure being the 45 mm thumb-reach limit. With the repo's 0.60 mm per-side fit
and the inter-bay grouping gap derived in §6.3 (two grid modules, 9.525 mm):

| bays stacked | depth | vs 83.60 |
|---|---|---|
| 1 | 22.790 | ok |
| 2 | 55.105 | ok, 28.5 mm spare |
| 3 | **87.420** | **over by 3.82 mm** |

`[DERIVED]`. So **two Pattern-B rows, not three.** `keyboard-k9pro.md` §11.2's
"roughly three stacked breakout rows" was computed without an inter-bay gap;
with a grouping gap that actually groups, it is two. The decided parts list has
four Pattern-B bays (1 × 5752, 2 × 5295, 1 × 4980), so the arrangement is
two rows of up to three — and the encoder bay is centred in one of them.

---

## 3. Instruments that refuse fixed labels

What the panel actually prints, and what carries the meaning instead. Every row
sourced.

| Instrument | What the panel prints | What carries the meaning | The lesson |
|---|---|---|---|
| **monome norns** | "There is no text or iconography anywhere on the unit"; "nothing is labelled"; rear jacks "all unlabelled, although their functions are fixed" `[REVIEW: SOS]` | Each script, and documentation found online. monome's own docs name the controls **E1 E2 E3** and **K1 K2 K3** `[monome docs/norns/play]` — ordinals that exist everywhere except on the object | The ordinal names already exist. Print them. Also: "Each encoder is visually paired with a button, but there's no enforced link between them" `[REVIEW]` — **do not imply a pairing software does not honour** |
| **monome grid** | 128 unlabelled varibright keys | Entirely software; LED brightness is the whole readout | A uniform unlabelled field works when *every* cell is identical and the readout is per-cell |
| **monome arc** | Nothing described; "each knob is encircled by a sixty-four segment light ring, sixteen-step brightness per LED"; "Applications determine the interaction between movement, sound, and light" `[VENDOR]` | The LED ring, per knob | A per-control readout ring is the strongest non-textual legend there is — and the chosen Adafruit boards have one NeoPixel per element, not a ring (§1.2) |
| **TE OP-1** | Ordinals and group names: "T1-T4", "sound 1-8", "color coded encoders", "main modes", "tape transport" `[VENDOR: layout guide]` | Colour: "The four color encoders are related to the graphical interface on the display"; "A green graphical element or text hints that the green encoder will change its value or position" `[VENDOR]` | OP-1 prints **ordinals and groups, never functions** — which is exactly the rule in §8. Its colour link needs 5.1.1.4.5.7's matching display colour, unavailable here |
| **TE OP-Z** | — (no screen; it borrows a phone) `[precedents.md §2]` | 16 keys carry everything | Depending on a second device is not a legend |
| **Faderfox EC4** | Not stated by the vendor; the legend is the screen | A "4 x 20 character OLED-display to show control values (numeric/bar), names and programming data", with editable **4-character** names per encoder, group and setup; setup/group names shown "in a 4x4 matrix" `[VENDOR]`, matching the 4 × 4 encoder field | **Four characters is enough to name a control.** The display's character matrix is congruent with the encoder matrix — Push's answer, generalised to two axes |
| **Intech Studio Grid EN16** | Nothing; "sturdy front panel", one RGB LED per control, programmed in Lua in the Grid Editor `[VENDOR]` | Software only, with no on-device display | The complaint arrives on the vendor's own page: a 4-star customer review asks for "some sort of small OLED screen module… or display what parameters get mapped" `[REVIEW: intech.studio/shop/en16, Alexander C., 14 Mar 2024]`. **Independent confirmation of the norns diagnosis, from a different product and a buyer who likes the thing** |
| **Electra One** | Ordinals: the knobs are labelled **KNOB1 to KNOB12** on the panel `[VENDOR: user guide]` | A touchscreen in three rows of 12, one row live at a time, with page names "positioned on screen to align visually with the knobs"; and a light touch "gives you visual feedback on the display about what MIDI parameter will be modified" `[VENDOR]` | Ordinals on the panel + names on the screen + **touch-to-reveal**. Three layers, none of them an engraved function |
| **Novation Launch Control XL** | Function group names (sends, pan, device) — it is *not* an agnostic panel | Default template, plus "the LEDs underneath the knobs switching from red to green" when the third row changes role `[REVIEW]` | A mode change shown *at the control*, not only on a screen. The NeoPixels can do this |
| **Ableton Push 2** | Button names; the encoder row is unnamed | "the eight encoders above the display"; "The display is divided into columns"; two banks of eight buttons above and below it `[VENDOR: manual]`. Also touch-to-reveal on the Swing knob: "When you touch the knob, the display will show the amount of swing" `[VENDOR]` | Strict column alignment, which §2 shows is available here |
| **Elektron Digitakt / Syntakt** | Pure ordinals: "DATA ENTRY knobs A-H. Used for setting parameter values" `[VENDOR: Syntakt manual; Digitakt's own PDF host would not resolve, §11]`; five page buttons under the screen; secondary functions in orange `[REVIEW: SOS]` | The screen's parameter page, named by the page button that is lit | **Letters, not numbers, and a physical page indicator.** A bank/page without a physical indicator is a control that lies (`precedents.md` §2, on Bitwig) |
| **Soma Terra** | A brass logo inlay; "TERRA has no display" `[VENDOR]` | "a 6 LED-sensor triangle in the center" that both shows and sets parameter data `[VENDOR]` | One element that is both readout and control — the limit case of "the control is its own legend" |
| **Bastl** | Large silkscreen on oak panels, "handmade custom knobs" `[precedents.md §2]` | Printed function names — Bastl does *not* refuse labels | Cited for the material, not for the labelling strategy. Worth saying plainly |
| **Eurorack / Serge** | Jack names and parameter names; the *function* comes from the patch. Tcherepnin's term is **"patch programmability"** — one module changes role depending on its connections `[IDM NYU / learningmodular]` | The cables. The panel prints the mechanism and the patch supplies the meaning | The oldest version of exactly this idea, and note what it prints: **mechanism, never function** |
| **Patchblocks for Eurorack** | — | "when a CV signal is present the knob/pot acts as an attenuator/level control, when no CV signal is present the knob performs the function assigned in the Patch" `[VENDOR, via Synthtopia]` | A knob whose role is genuinely software-assigned, shipping, in a format where nobody prints the function |
| **Mixing consoles, all of them** | A blank strip | The **scribble strip**: "the area on audio mixing boards designed to accommodate notes about what is on a given channel… the space sound engineers use to name the channels" `[Sweetwater glossary]`, written on removable tape — Hosa LBL-505 "Scribble Strip Console Tape", which *Sound on Sound* recommends by name `[SOS]` | **The professional answer to a remappable surface is to give the user somewhere to write.** Not an engraving, not a screen: a blank land and a pen. §8.5 |

### 3.1 The three complaints, and what they have in common

- norns: "no text or iconography anywhere"; "no obvious alignment between the
  encoders/buttons and the screen"; "not all scripts cleanly adhere" to the
  navigation conventions; documentation "sparse and disorganised" `[REVIEW: SOS]`.
- Organelle: "there is no labeling on the top panel indicating any of the aux
  controls" `[precedents.md §2]`.
- Intech EN16: a buyer asking for a screen so he can "display what parameters get
  mapped" `[REVIEW]`.

All three are **unlabelled *and* unaligned *and* without a per-control readout**.
None of them is a complaint about the absence of engraving on its own. The
devices that print nothing and work — grid, arc — have a per-cell or per-knob
readout. The devices that print ordinals and work — Push, OP-1, Electra One,
Elektron, Faderfox — have a screen whose geometry matches the controls.

**The failure mode is not "no labels". It is "no labels and no legend of any
kind".** That is a correctable condition and §2 shows the correction is free.

---

## 4. The screen as legend — what has to be true

### 4.1 The conditions, from the precedents

1. **Geometric congruence.** Push: eight encoders, eight display columns, eight
   buttons above and below `[VENDOR]`. Faderfox EC4: a 4 × 4 character matrix
   over a 4 × 4 encoder field `[VENDOR]`. Both make the mapping un-mistakable by
   construction. §2 establishes this is available at 19.050 mm / 15 columns.
2. **Proximity.** MIL 5.4.2.4: "Labels shall be placed on or very near the items
   that they identify" `[MIL]`. The display's bottom edge is the near edge for
   this purpose, so the label row belongs at the *bottom* of the display, not the
   top.
3. **Four characters is enough.** Faderfox ships 4-character names for encoders,
   groups and setups `[VENDOR]`. This panel has 15 columns per control — 3.75×
   what a shipping product manages `[DERIVED]`. The legend is not short of room;
   it is embarrassingly rich.
4. **A visible bank/page indicator**, or the controls lie — `precedents.md` §2 on
   Bitwig ("Pages need a page indicator; without one the controls lie") and
   Elektron's five lit page buttons `[REVIEW: SOS]`.
5. **It must survive the screen being busy.** This is the one Push does not have
   to solve, because Push's display does nothing else. Here the display's day job
   is text. A legend row that is only present on a dedicated page is not a
   legend; it is a help screen.

### 4.2 The five options the task asked me to evaluate

Stated as what the *geometry* must provide. Which of these the display actually
shows is display content, not enclosure.

| Option | Verdict | Why |
|---|---|---|
| **Position-based naming on screen** (a name at the column above each control) | **Recommended, primary.** | §2: congruence is available, at 15 columns per control with 0.030 mm of pitch error. This is Push's and Faderfox's mechanism, it is the only one with two independent successful precedents, and it costs no panel area and no engraving |
| **A persistent legend strip** (one display row permanently reserved) | **Recommended as the form the above takes.** One 12 px row = 2.544 mm of the 63.60 mm display = **4.0 %** of the panel `[DERIVED]` | 4 % is the cheapest insurance in the project. Reserving it is a *layout* commitment — the enclosure must not place the control band so far from the display that the strip's proximity claim fails (§4.1.2). That is the only enclosure-side requirement |
| **Shape/size reference on screen** (the screen draws the knob's code) | **Reject.** | It spends pixels re-stating something the hand already knows, and it is the OP-1 colour trick with the colour removed — which is where `precedents.md` §8.2 was heading before the alignment finding made it unnecessary |
| **Ordinal numbering** | **Recommended, as the engraved layer** — §8. Not as the screen's job | Precedented four times over (KNOB1–12, T1–T4, A–H, E1/K1). Permanent, mechanism-true, survives the screen being off |
| **A dedicated mapping view** | **Keep, demote.** | Useful; not a legend. See §4.1.5. norns effectively has this (press K3 on a script to read its description `[monome docs]`) and it did not save norns |

### 4.3 And when alignment *is* impossible — which here it is, for the pots

Three pots cannot be display-aligned, and the arithmetic is tight enough to be
worth recording:

```
3 x 13.00 mm + 2 x 25 mm (MIL fig 10, one-hand minimum)  =  89.00 mm
display active area                                      =  84.80 mm
                                                            SHORT by 4.20 mm   [DERIVED]
equal separation that would fit three 13 mm knobs in 84.80:   22.90 mm — below MIL's minimum
```

So the pots are never screen-labelled, and they should not be: their job is
blind, set-and-leave operation (`precedents.md` §4), which wants **separation and
shape coding**, and screen alignment buys nothing for a control you set once and
look at never. The panel therefore has **two tiers, and they want opposite
things**:

| tier | controls | identified by | wants |
|---|---|---|---|
| **played** | the Pattern-B bays — encoders, keys, sliders | screen legend + ordinal + position | *alignment*, tight pitch, the display's width |
| **set** | the three pots | shape + size + location | *separation*, the panel's extremes, no screen at all |

That division is the architecture, it is derived rather than chosen, and it
disposes of the apparent contradiction between "align everything to the display"
and "separate everything by 125 mm" — different controls, different jobs,
different rules, one standard correctly read. (The same move `precedents.md`
§"Settled by arithmetic" made for the encoder pitch.)

---

## 5. Shape and size coding, and the minimum scheme

### 5.1 The vocabulary the standard actually defines

Quoted verbatim `[MIL]`:

- **5.1.1.4.3 Size-coding.** "No more than three different sizes shall be used to
  code controls for discrimination by absolute size."
- **5.1.1.4.3.1** "Controls used for performing the same function on different
  items of equipment shall be the same size."
- **5.1.1.4.3.2 Knob diameter.** "When knob diameter is used as the coding
  parameter, the differences between diameters shall be not less than 13 mm
  (0.5 in)."
- **5.1.1.4.3.3 Knob thickness.** "When knob thickness is the coding parameter,
  the differences between thicknesses shall be not less than 10 mm (0.4 in)."
- **5.1.1.4.4.1.2** "Shapes shall be identifiable by hand and eye regardless of
  the position and orientation of the control knob or handle."
- **5.1.1.4.4.1.4 Number of shapes.** "…shall be not more than 10."
- **5.1.1.4.4.1.7 Control function.** "Shapes shall be assessed to ensure that
  they are clearly associated with the intended control function and have no
  reasonable association to any of the alternate functions available."
- **5.1.1.4.4.2 Control dimensions.** "Minimum control dimensions when users must
  distinguish controls by shape alone shall be as follows: a. Height: 13 mm
  (0.5 in) or larger. b. Width: 13 mm (0.5 in) or larger. c. Depth: 6.5 mm
  (0.25 in) or larger."
- **5.1.4.1.2.1.3 Shapes.** "Knobs that perform the same function shall have the
  same shape." **5.1.4.1.2.1.3.1** "Knob shape shall be determined by the knob's
  function and use."
- **5.1.4.1.2.1.4 Rims.** "All rotary adjustment knobs shall have rims with
  surfaces capable of obtaining a secure grasp."
  **.4.1 Knurled surfaces.** "Very small knobs shall have knurled surfaces to
  provide maximum torqueing capability."
  **.4.2 Serrated rims.** "Intermediate and larger size knobs shall have
  serrated rims for low torque applications."
  **.4.3 Rim identification.** "Intermediate and larger size knobs shall have rim
  indentation rather than serration for a firmer grasp for high torque
  applications."
- **5.1.4.1.2.1.2 Single revolution controls.** "If positions of single
  revolution controls must be distinguished, a pointer or marker shall be
  available on the knob."
- **5.1.4.2.1.1.2 Shape.** "The push button surface shall be concave (indented)
  to fit the finger." **.2.1** "When a concave surface is impractical, the
  surface shall provide a high degree of frictional resistance."

**5.1.1.4.4.1.7 deserves a note**, because on a re-mappable surface it is almost
a prohibition: a shape must be "clearly associated with the intended control
function" and have "no reasonable association to any of the alternate functions".
If the function is assigned in software, no shape can satisfy that — so
**function-associative shapes are off the table here, and the only legal shape
coding is coding by *class*, i.e. by what the control physically is.** Which is
exactly what the brief wants. The standard and the brief agree again.

### 5.2 Two corrections to the repo's MIL numbers

Both found by reading the clause text rather than the summary. Neither changes a
decision; one *strengthens* a decision.

**(a) MIL's minimum knob diameter is 10 mm, not 13 mm.** Figure 10, verbatim
`[MIL]`:

| | A, Fingertip grasp | | B, Thumb and finger encircled | | C, Palm grasp | |
|---|---|---|---|---|---|---|
| | H, height | D, diameter | H, height | D, diameter | D, diameter | L, length |
| Minimum | 13 mm | **10 mm** | 13 mm | 25 mm | 38 mm | 75 mm |
| Maximum | 25 mm | 100 mm | 25 mm | 75 mm | 75 mm | — |

Separation, edge to edge: one hand individually **min 25 mm, optimum 50 mm**; two
hands simultaneously min 50 mm, optimum 125 mm. Torque max 32 mN·m at ≤25 mm
diameter, 42 mN·m above `[MIL]`.

`envelope.md` writes "MIL-STD-1472H minimum knob = 13.00 mm — smallest knob it
permits at all" and derives a 0.05 mm window. The 13 mm is Figure 10's minimum
**height**; the minimum **diameter** for fingertip grasp is 10 mm.

**(b) But 13.00 mm is right anyway, and for a better reason — the user's own
decision makes it binding.** 5.1.1.4.4.2 requires a control distinguished **by
shape alone** to be ≥13 mm wide `[MIL]`. An unlabelled panel is precisely a panel
whose controls are distinguished by shape alone. So:

```
19.05 vendor pitch - 6.00 finger gap = 13.05 mm   largest the pitch allows
5.1.1.4.4.2 minimum width            = 13.00 mm   smallest an unlabelled panel allows
                                       --------
window                                  0.05 mm
```

`envelope.md`'s number and its five-hundredths window survive intact, under a
different clause. **Deleting the labels is what makes 13.00 mm mandatory.** It is
worth knowing that the two decisions are coupled, because if the labels ever came
back, the knob could legally shrink and the window would reopen.

**Height, now that it is the separate requirement it always was:** 5.1.1.4.4.2
also wants ≥13 mm of height and ≥6.5 mm of depth. `precedents.md` §7.1's insight
applies — MIL specifies 13 mm of *graspable cylinder* and says nothing about
where the panel plane cuts it — so the encoder knob is **13.00 mm diameter ×
≥13.0 mm of cylinder**, of which ~9 mm may stand proud and the rest sit below the
top land.

### 5.3 The one size code: the pot knob is ≥26.00 mm

```
encoder knob                    13.00 mm   (fixed by the pitch, 5.2 above)
5.1.1.4.3.2 minimum step      + 13.00 mm
                                --------
pot knob                      >= 26.00 mm   [DERIVED]
```

Three things make this the right answer rather than a grudging one:

1. **It is the correct grasp class.** 26 mm crosses Figure 10's "thumb and finger
   encircled" minimum diameter of 25 mm `[MIL]`. A set-and-leave absolute control
   *should* be encircled, and a played endless control *should* be fingertip. The
   size code and the ergonomics want the same thing.
2. **There is room.** Three Ø26 knobs at MIL's *optimum* 50 mm separation span
   178.00 mm against 288.35 mm of interior `[DERIVED]`. The optimum is affordable
   twice over; only the 125 mm blind threshold is not (§5.6).
3. **It raises the torque ceiling, which the part needs.** Figure 10 allows
   32 mN·m up to 25 mm diameter and **42 mN·m above** `[MIL]`. The RD901F's
   rotational torque is 20–250 gf·cm = **1.96–24.5 mN·m** `[REPO: parts.md §5]`,
   which clears 32 anyway — but crossing 25 mm buys 10 mN·m of headroom for a
   stiffer pot, where a 13 mm knob would have had none. `[DERIVED]`
4. **Diameter, not thickness.** 5.1.1.4.3.3's thickness route needs a 10 mm step,
   so a pot knob ≥23 mm tall against a 13 mm encoder — a tall knob on a handheld
   that gets laid face down, and `precedents.md` §7.2 wants the knob's top *below*
   the highest surrounding land. The pots sit on the panel's outer thirds where
   diameter is free and height is not. **Code with diameter.** `[REASONED]`

**Sizes used for coding: 2 of the 3 MIL permits.** One in reserve, deliberately
unspent.

### 5.4 The free second code: a pointer the standard requires

The recommended pot, the Alpha RD901F, is **300° ± 5°** `[REPO: parts.md §5,
from the Alpha datasheet]` — a single-revolution control, as every standard
panel-mount pot is.
5.1.4.1.2.1.2: "If positions of single revolution controls must be distinguished,
a pointer or marker **shall** be available on the knob" `[MIL]`. The encoders are
endless, so no pointer is required, and a pointer on an endless control would be
a lie told in plastic.

```
pot      large, serrated rim, WITH a pointer / flat     -- required by 5.1.4.1.2.1.2
encoder  small, knurled rim,  NO pointer, continuous    -- endless; a pointer would lie
```

Two independent codes for one distinction, both free, both prescribed, both
tactile. A pointer or a flatted skirt is detectable by a fingertip in one
movement, which is the thing 5.1.1.6 was asking for.

### 5.5 Rim texture: the standard assigns it by size, so it costs no decision

5.1.4.1.2.1.4.1 puts knurling on "very small" knobs and .4.2 puts serration on
"intermediate and larger size knobs… for low torque applications" `[MIL]`. Both
of these controls are low-torque — the PEC11R detent torque is 2.9–8.8 mN·m
against MIL's 32 mN·m ceiling `[precedents.md §6.9]`. So:

| | diameter | rim | clause |
|---|---|---|---|
| encoder | 13.00 | **knurled** | 5.1.4.1.2.1.4.1 |
| pot | ≥26.00 | **serrated** | 5.1.4.1.2.1.4.2 |

Two rim textures, each required by the standard for its own size class. The
tactile vocabulary is a *consequence* of the size code, not an addition to it —
which is the minimalism the brief asked for, arrived at by reading the standard
rather than by styling. Rim **indentation** (.4.3) is for high torque and has no
application here; it stays unused, like the third size.

`precedents.md` §8.2's proposal of "a distinct rim treatment" *per encoder* is
superseded: four different rims among four identical-role controls would violate
5.1.4.1.2.1.3 ("Knobs that perform the same function shall have the same shape")
and 5.1.1.4.3.1 ("Controls used for performing the same function… shall be the
same size"), and it is unnecessary now that the display can name them. **Code the
class, never the instance.** Instances are coded by position (§6).

### 5.6 The 125 mm departure, with the arithmetic and the mitigations

Three same-size pots on this panel cannot all reach 125 mm from their adjacent
neighbours. Every placement I can construct, computed:

```
in a row, 13 mm knobs:   3 x 13 + 2 x 125 = 289.00 vs 288.35 interior -> short 0.65 mm
in a row, 26 mm knobs:   3 x 26 + 2 x 125 = 328.00 vs 288.35          -> short 39.65 mm
body width needed at 13 mm:  295.40 mm; the deck is 294.75            -> short 0.65 mm
in a column: 2 x 125 + 3 x 13 = 289.00 vs an 83.60 mm band            -> hopeless
two flanking the display, 26 mm knobs, at the flank centres +-93.2875:
                         c-to-c 186.575, edge-to-edge 160.575         -> PASSES
third pot, centred, 45.8 mm below the flank pots (estimated band offset):
                         c-to-c 103.9, edge-to-edge 77.9              -> short 47 mm
   (to reach 125 edge-to-edge at that offset needs +-143.9 mm of x; the panel
    offers +-131.175 -> short 12.7 mm)
```

`[DERIVED throughout]`. **Two pots can be location-coded to the letter of
5.1.1.6. Three cannot, by any arrangement on a 294.75 mm panel.**

A strictly compliant three-pot panel would need **three different shapes** among
the pots (5.1.1.6's shape route applied instance-by-instance). That is refused:
it exhausts the shape budget on one distinction, it collides with
5.1.4.1.2.1.3 and 5.1.1.4.3.1 if the three pots have the same role, and it
destroys the "insane symmetry" the brief requires. **This is a stated departure,
and it is bought back three ways:**

1. **The achievable separation is 78–84 % of the threshold, not a fraction of
   it.** Three Ø26 knobs with the centre on the axis `[DERIVED]`:

   | outer centre | edge-to-edge | % of 125 | × the 50 mm optimum | land to wall |
   |---|---|---|---|---|
   | ±123.825 (26 modules) | **97.825** | 78.3 % | 1.96 | 7.350 mm |
   | ±128.5875 (27 modules) | 102.588 | 82.1 % | 2.05 | 2.588 mm |
   | ±131.175 (knob on the wall) | 105.175 | 84.1 % | 2.10 | 0.000 mm |

   ±123.825 is the recommendation: on the grid module, ~98 mm of separation, and
   7.35 mm of land left for the plate's edge. For scale, norns puts three
   encoders inside one hand's span.
2. **The pot's own setting is readable by touch**, via the pointer 5.1.4.1.2.1.2
   requires. A hand that lands on the wrong pot can tell from the pointer's angle
   that it is not the one it wanted — a recovery route a detentless endless
   control cannot offer. `[REASONED]`
3. **Location coding is MIL's own listed alternative** for nonvisual
   identification (Table IV), and 5.1.1.6 cross-references it. Three controls at
   left / centre / right, on a symmetric panel the player is holding by its two
   edges, is about as unambiguous as location coding gets. §6.

### 5.7 Concave versus convex: the Braun/MIL tension, resolved

`precedents.md` §7.5 flagged this and asked for a resolution. The sources, both
verified:

- MIL **5.1.4.2.1.1.2**: "The push button surface **shall be concave (indented)**
  to fit the finger", with **.2.1**: "When a concave surface is impractical, the
  surface shall provide a high degree of frictional resistance." `[MIL]`
- Braun ET66, Dietrich Lubs 1987: "the black buttons with white Akzidenz-Grotesk
  numbers are **polished** while the function buttons are more **matt**",
  "combined with their **convex** shape for your finger-tip", "functions grouped
  by colour – brown and green", and "the most frequently-used button has nature's
  high-contrast black on yellow" `[REVIEW: Port]`.

**Resolution: take Braun's finish, refuse Braun's curvature.**

```
curvature  -> CONCAVE, all keys, no exceptions          MIL 5.1.4.2.1.1.2 "shall"
finish     -> MATT                                      Braun's grouping device, and
                                                        simultaneously MIL .2.1's
                                                        friction fallback
```

The two sources are not really in conflict once you notice that MIL's own
fallback for an un-concave button is *friction*, and matt is friction. Braun was
reaching for visual prominence and got tactile grip as a by-product; MIL demands
the grip and is indifferent to the look. Specify concave **and** matt and both
are satisfied, and the convex form — the only part that is genuinely
incompatible — is the part being dropped. On an instrument used partly in the
dark, tactile fit wins over catching the light. `[REASONED, on two verified
sources]`

Note what Braun's grouping device becomes here: ET66 grouped by **finish** and by
**colour**. Finish is permanent and therefore commits a meaning — so on an
agnostic plate, *all* keys are the same finish, and the grouping is done by the
NeoPixel underneath, which is the layer allowed to change (§1.2).

### 5.8 The code count

| code | values used | MIL ceiling | spare |
|---|---|---|---|
| Mode of operation | 4 — rotary-endless, rotary-limited, linear, momentary push | (not bounded) | — |
| Size | **2** — Ø13.00, Ø≥26.00 | 3 (5.1.1.4.3) | **1** |
| Shape | 1 feature — pointer present / absent | 10 (5.1.1.4.4.1.4) | 9 |
| Rim texture | 2 — knurled, serrated | — (assigned by size) | indentation unused |
| Colour, permanent | **0** | 5 (5.1.1.4.5.1) | 5 |
| Colour, software (NeoPixel) | ≤5 simultaneous | 5 | — |

**One deliberate distinction is coded, twice, and every budget is left with room
in it.** That is the shape of a minimal scheme.

---

## 6. Position is the name — the Swiss-grid argument

This is the strongest answer to the brief, and the evidence for it is better than
I expected.

### 6.1 The proof is sitting inside the cautionary precedent

monome's documentation calls the controls **E1 E2 E3** and **K1 K2 K3**
`[monome docs/norns/play]`, and the conventions it records are entirely
positional: "A quick tap of **K1** is how you swap focus between these layers";
"use **E1** to switch between the different menus"; "**To fine-adjust a
parameter, hold K3 as you turn E3**"; "**K2 / K3**: select previous pair / select
next pair" `[monome docs]`. Meanwhile the hardware prints nothing `[REVIEW: SOS]`.

**norns already names its controls by position. It just does not say so on the
object.** The gap between the thing that works (the ordinal convention) and the
thing that fails (an anonymous panel) is one engraving pass.

And three shipping instruments close that gap exactly:

| | printed on the panel |
|---|---|
| Electra One | **KNOB1 … KNOB12** `[VENDOR]` |
| TE OP-1 | **T1-T4**, **sound 1-8** `[VENDOR]` |
| Elektron Syntakt / Digitakt | **DATA ENTRY knobs A-H** `[VENDOR]` |

None of them prints a function. All of them print a position. **An ordinal is not
a label in the sense the brief forbids — it names the control, not the
assignment**, and MIL 5.1.1.4.2.2 is the clause that says why this is the right
layer to make permanent: "Controls associated with similar functions shall be in
the same relative location from workstation to workstation, from panel to
panel" `[MIL]`. Read against a software-remappable surface: **the position is the
thing that must never move, even when the meaning does.**

### 6.2 What it demands of the layout

Five requirements, all checkable. Numbers derived; the principle is Swiss-grid
and Gestalt proximity, and where I am reasoning I say so.

1. **One reading direction, no exceptions.** Left → right, then far → near. Every
   element's ordinal increases monotonically with x within a bay, and bays
   increase with distance from the display. Checkable as a sort.
2. **One origin.** Ordinal 1 is the leftmost element of the bay nearest the
   display, and nothing else can be. Checkable.
3. **Unambiguous grouping.** The between-group gap must exceed the within-group
   gap, or the hand cannot feel where a bay ends. Within a bay the element clear
   gap is 19.05 − 13.00 = **6.05 mm** `[DERIVED]`. At 1.5× that, the inter-bay
   clear gap wants **≥9.08 mm**; the nearest grid module value above it is **two
   modules, 9.525 mm** `[DERIVED]`. The 1.5 factor is mine `[REASONED]` — Gestalt
   proximity gives the direction, not the number, and I found no standard that
   quantifies it (§11). The consequence is §2.6: two stacked bays, not three.
4. **Symmetry about the centreline — which is also the index mark.** This is the
   best thing in the section. On a two-handed held instrument the player's own
   hands are on the two edges, so **the centreline is palpable without being
   marked**. A symmetric four-element row puts the axis in the gap between
   elements 2 and 3, so the hand finds the centre and counts *outward by two*
   rather than inward from an edge by up to four. Two is inside anyone's
   subitizing range; four is at its edge (the consensus figure is "about four",
   with 3–4 the common finding and the original 1949 limit quoted higher — treat
   the number as soft) `[Springer, Attention Perception & Psychophysics; Kaufman
   et al. 1949 via secondary sources]`. **The brief's aesthetic demand for
   "insane symmetry" is also the usability mechanism**, which is the most
   agreeable finding in this document. `tools/layout.py` check 6 already enforces
   it `[REPO]`.
5. **Consistent ordinal placement.** MIL 5.4.3.1: "When controls are placed at or
   below eye level, the corresponding label should be placed above the controls"
   `[MIL]` — "should", not "shall". Above the aperture is preferred and needs
   ≥4.0 mm of land; the Pattern-B bay offers 10.795 − 6.50 = **4.295 mm** between
   a 13 mm knob's rim and the board edge `[DERIVED]`, so it just fits. If a given
   layout cannot find 4.0 mm above, the ordinal goes below and that is a stated
   departure with a reason: the land above the control belongs to the display's
   legend row, which is the more important legend.

### 6.3 One geometric consequence worth flagging now

A row of four identical elements is symmetric about x = 0. Three pots are not,
unless one sits on the axis — and the axis in the control band is where the
encoder bay has to be (§2.5), because the centred bay is the only one that gets
display columns.

**So: the encoder bay centred on x = 0, three pots symmetric about x = 0, and a
layout that satisfies both, cannot put them in the same row.** Either the pots
occupy their own row (centre pot on the axis) with the encoder bay in another, or
the third pot is declared the panel's single licensed asymmetry —
`tools/layout.py` already supports this through `free=True` `[REPO]`, and v1's
"where the language breaks it says so" discipline already blesses one deliberate
break. This is a real decision for the layout study and it is in §12 as a
blocker, not resolved here.

### 6.4 What position cannot do, stated so it is not oversold

Position names a control; it does not say what the control *does*. Two things
follow and neither is optional:

- The ordinal is only a name if **something else binds the name to a meaning**.
  Here that is the display legend (§4) and the instrument's own language, which
  already prints inputs and their sources (`>lanes`). An ordinal with no binding
  anywhere is norns.
- **Do not imply a grouping the software does not enforce.** SOS on norns: "Each
  encoder is visually paired with a button, but there's no enforced link between
  them" `[REVIEW]`. On this panel the vendor's bay *is* the enforced grouping —
  four elements on one board, one I²C address, one connector `[REPO: parts.md]` —
  so the plate should group by bay and by nothing else. Any visual grouping finer
  or coarser than a bay is a claim the hardware does not back.

---

## 7. The design language, as checkable rules

### 7.1 Rams, verbatim — and the conflict inside his own list

All ten quoted from Vitsœ, which publishes them `[Vitsœ]`. The four that bear on
this panel:

> **4. Good design makes a product understandable.** "It clarifies the product's
> structure. Better still, it can make the product talk. At best, it is
> self-explanatory."
>
> **5. Good design is unobtrusive.** "Products fulfilling a purpose are like
> tools. They are neither decorative objects nor works of art. Their design
> should therefore be both neutral and restrained, **to leave room for the user's
> self-expression**."
>
> **6. Good design is honest.** "It does not make a product more innovative,
> powerful or valuable than it really is. It does not attempt to manipulate the
> consumer with promises that cannot be kept."
>
> **10. Good design is as little design as possible.** "Less, but better –
> because it concentrates on the essential aspects, and the products are not
> burdened with non-essentials. Back to purity, back to simplicity."

**Principle 10 is what the brief is invoking. Principle 4 is the bill.** Rams
does not license an unreadable panel; "as little design as possible" sits in the
same list as "at best, it is self-explanatory", and he put the understandable one
first. So the minimalist argument cannot be used to wave away legibility — by
Rams' own list, a panel that is beautiful and opaque has failed principle 4 to
buy principle 10, which is a trade he never offered.

Principle 5 is the surprise, and it is the brief restated in Rams' words:
"neutral and restrained, to leave room for the user's self-expression" is *why* a
remappable instrument should not commit its meanings in engraving. And principle
6 is the rule that makes §8 work: an engraved end-stop is honest because the
end-stop is really there; an engraved "cutoff" is "a promise that cannot be
kept".

### 7.2 Colour: MIL and Braun agree, which does not happen often

| | says |
|---|---|
| Braun | "predominantly black and white, but making expert use of touches of bright, bold colours" `[Pixartprinting, via precedents.md §8.1]`; Cooper Hewitt calls it an "economic use of color" |
| MIL **5.1.1.4.5.2** | "Controls shall be **black**… or **gray**" `[MIL]` |
| MIL **5.1.1.4.5.1** | "not more than **five** colors shall be used" `[MIL]` |
| MIL **5.1.1.4.5** | "Colors shall not be the sole means to control coding" `[MIL]` |
| MIL **5.1.1.4.5.9.1** | not the primary identification where ambient light varies `[MIL]` — a stage |
| MIL **5.1.1.4.5.4** | "Gloss finishes shall not be used on controls where specular reflection (glare)… could degrade task performance" `[MIL]` — a stage again |

**Recommendation: the permanent colour budget on the plate is one neutral, and
nothing else. Zero accents.** All colour on the panel is the NeoPixel layer,
which is software-set and therefore belongs with the meaning rather than the
identity (§1.2). Braun's "touches of bright, bold colours" are delivered — and
they *move*, which is the entire point of the instrument.

This is not asceticism for its own sake: any permanent accent commits a meaning,
and committing a meaning is the one thing the brief forbids. The single licensed
exception is a safety-critical control, for which 5.1.1.4.5.6 and SAE AS18012
apply; `docs/OS.md`'s requirement for "a physical, no-screen-needed way to
recover" is the candidate, and it is not in the decided parts list, so it is
noted and left alone.

**And the second colour is free.** Laser-engraving **cast** acrylic gives a
bright frosted-white mark, where extruded gives a faint grey one and can soften
and raise an edge; cast also cuts to a smoother, flame-polished edge
`[vendor laser guides — Thunder Laser, Boss Laser, 1laser; all commercial sources,
none with measured data, see §11]`. So **black cast acrylic, engraved** is
literally Braun's "predominantly black and white" at no extra process step: the
white is the engraving and the black is the sheet. Specify cast, not extruded,
and the colour question answers itself.

### 7.3 Typography — and a correction to `precedents.md` §8.1

`precedents.md` §8.1 reports that "on the earlier devices Braun used **Nobel** on
the controls, and Hans Gugelot held that Nobel was 'much better suited to
labeling the control elements than Futura, Akzidenz Grotesk'". I went to the
cited page. It is weaker than that, and the correction matters because the claim
was being used to pick a typeface `[Fonts In Use, Braun RT 20, comment thread]`:

- The Nobel attribution **was withdrawn on that page**. Florian Hardwig: "But do
  any of the shown images depict Nobel?" / "I don't think so." / "I've removed
  Nobel from this Use again."
- What survives: "Nobel is used for the labels on the **SK4** from 1956"
  (Hardwig) — a different product from the RT 20.
- The Gugelot line is **a commenter's paraphrase, not Gugelot's words**: Thomas
  Wilkens writes "From Hans Gugelot's point of view, it was much better suited to
  labeling the control elements than Futura, Akzidenz Grotesk or similar sans
  serif linear antiquas".
- "From **SK5** onwards, Braun used Akzidenz Grotesk, among others" (Wilkens).
- The numerals-differ claim is also a commenter observation and the face is not
  identified: Michael, "The typeface used above the buttons is correct but the
  dial uses a completely different typeface… The numbers are not
  Akzidenz-Grotesk." Hardwig: "The numbers on the dial are using something else."

**So: "Braun used a different face for numerals than for labels" is an
unverified reader observation about one radio, not a Braun rule.** The useful
residue is the *distinction* — panel numerals and panel labels are different
typographic jobs — which stands on its own as reasoning `[REASONED]` and is
anyway nearly moot here, because the recommendation in §8 engraves digits and
nothing else. One job, one face.

The specification therefore says nothing about *which* face, and everything about
size, which is where MIL is quantitative (§8.3).

### 7.4 Jacob Jensen and the Danish side of the brief

The user named Scandinavian minimalism. What the sources actually support
`[Wikipedia: Jacob Jensen; beoworld; V&A; Stylepark]`:

- Jensen was B&O's chief product designer from 1964 and "developed over 200
  products", establishing a "minimalistic, horizontal, and severe design style".
- The **Beomaster 1900** (1976) is the relevant object: controls split into
  **primary and secondary**, with the less-used secondary set hidden **under a
  hinged aluminium panel**; touch-sensitive controls instead of sliders; a V&A
  curator describing "very clean lines… touch sensitive… a completely smooth
  silhouette"; set against a market of "large, brushed aluminium fronted steel
  boxes – festooned with knobs and flashing lights".
- B&O's seven design principles, dated to 1971, are listed as **Authenticity,
  Autovisuality, Credibility, Domesticity, Essentiality, Individuality,
  Inventiveness**. **"Autovisuality" is the one that would matter most here and I
  could not find a definition in any source I trust** — §11. Do not build on it.

The transferable move is **not** "hide the controls" — this instrument's controls
are its reason for existing. It is the **primary/secondary split**: Jensen's
answer to a crowded panel was to decide which controls are played and which are
set, and to give them categorically different treatment. §4.3 arrives at exactly
that division from the arithmetic — played controls aligned and tight, set
controls separated and coded — which is a pleasant convergence and worth naming
as the Danish contribution rather than as a graph of numbers.

### 7.5 Teenage Engineering, stated as the limit case

TE's transferable move, per `precedents.md` §8.4, is that "a small number of
controls each hold many meanings, and the identity that holds them together is
not a label but a perceptual constant". Verified on the layout guide: the four
encoders are colour-coded and the panel prints only **ordinals and group names**
(T1-T4, sound 1-8, main modes, tape transport) `[VENDOR]`.

And the warning, which belongs in the same paragraph: TX-6 owners report knobs
"way too small for anything else than 'set and forget'", "closely placed, tiny
knobs can be challenging to operate", "imprecise and 'jump' between values"
`[precedents.md §7.5, Elektronauts / Equipboard]`. The relevance to *this*
document is specific: the 13.00 mm knob at 19.05 mm pitch is near that failure
mode, and the only thing standing between this panel and that review is the
6.05 mm rim gap and a low-torque encoder. **Both are already specified and both
are checked** (`tools/layout.py` FINGER = 6.00; PEC11R at 2.9–8.8 mN·m). Keep
them.

---

## 8. If the panel prints nothing, what does it print?

### 8.1 Three layers of permanence, and engraving belongs to the middle one

This is the resolution, and it falls out of how the thing is actually made.

| layer | changes | holds |
|---|---|---|
| **shell + boss quads** | never — recutting bosses is a reprint | the two vendor patterns; the geometry of what *can* be fitted |
| **the laser-cut plate** | **whenever the fitted bays change** — it is cheap sheet, and `envelope.md` already says "an arrangement can be re-cut without re-cutting bosses" | the apertures, and therefore the element count, the ordering and the group boundaries |
| **software** | every performance | the meanings, and the NeoPixel colours |

The objection to engraving is that it freezes something that ought to be free.
But engraving goes on **the plate**, and the plate is *already* the layer that
changes when the configuration changes — a bay that stops holding four encoders
and starts holding one slider needs a new aperture anyway, so it gets a new
plate, so it gets new engraving. **Engraving does not fight agnosticism as long
as it only restates what the plate's own apertures already say.**

That is the test, and it is sharp enough to apply:

> **Engraving may describe the mechanism. It may never describe the function.**
>
> Redundant with the geometry: permitted. Additional to the geometry: forbidden.

An ordinal restates an aperture's position in its group — the apertures are
already there and already in that order, so the digit adds no commitment. An
end-stop tick restates a 300° pot's travel limits — the limits are in the part.
"cut" restates nothing; it is a promise, and Rams' principle 6 is the objection.

### 8.2 The options, decided

| Option | Verdict |
|---|---|
| **Ordinal mark, one per aperture** | **Yes.** Four precedents print exactly this (KNOB1–12, T1–T4, A–H, and norns' own E1/K1 convention which is printed only in the docs). Mechanism, not function. Survives the screen being off, gives the user and the documentation a shared vocabulary, and is what `>lanes` can print |
| **A single index dot** | **No — redundant, and the symmetry already does it.** The precedent is real and formal: ETSI ES 201 381 requires "a tactile identifier on the '5' key", defining it as "feedback which can be felt when touching a surface, e.g. a raised dot", and ITU-T E.161 recommends the same so the keypad can be used by touch `[ETSI; ITU-T E.161 summary]`. One mark establishes a whole grid. But on a held, mirror-symmetric panel the player's two hands already establish the axis (§6.2.4), so a dot would mark something the hand knows. Keep the precedent in reserve for the day the layout stops being symmetric |
| **Tick marks around a knob** | **Two only, and only on single-revolution controls.** Two end ticks on each pot, and the slider channel's two travel ends. These satisfy MIL 5.4.2.2.3 ("Direction of motion (increase, decrease) **should** be identified for continuous-motion rotary controls") and partner the pointer 5.1.4.1.2.1.2 requires. **No ticks on the endless encoders** — they have no ends, and printing some would be dishonest. A graduated scale is also refused: a scale commits a *range*, and the range is software's |
| **A group rule** | **No — cut it, don't print it.** MIL 5.4.1.7.1.4 does bless lines of demarcation, but on a flush plate the aperture tile's own edge *is* the demarcation, and §6.2.3's gap rule does the grouping dimensionally. A printed rule around a cut group is the panel saying the same thing twice |
| **Nothing at all** | **No.** It is the one option with three independent failure reports (norns, Organelle, Intech EN16) and no successes that lack a per-control readout. A laser engraves as well as it cuts, so "nothing" would be a choice, and it is the wrong one |

### 8.3 The engraved digit, dimensioned

MIL Table XIX gives 4 mm for "Critical markings with fixed position (e.g.,
numerals on fixed scales, **controls**, emergency instructions)" at or below
3.5 cd/m² — a dim stage — and 2.5 mm above it, with the note "The above values
assume a 710 mm (28 in) viewing distance. For other distances, multiply the above
values by D/710 mm" `[MIL]`. Table XVIII adds a floor of 2.3 mm for any viewing
distance under 0.5 m `[MIL]`.

| viewing distance | 4.0 × D/710 | with the 2.3 mm floor |
|---|---|---|
| 300 mm | 1.690 | **2.300** |
| 400 mm | 2.254 | **2.300** |
| 450 mm | 2.535 | **2.535** |
| 500 mm | 2.817 | **2.817** |
| 710 mm (MIL default) | 4.000 | 4.000 |

`[DERIVED]`. The viewing distance for a two-handed held instrument played
standing is **NOT SOURCED** (§11), so the specification takes **2.6 mm cap
height**, which covers everything out to ~460 mm with the dim-stage figure and
everything out to 710 mm with the lit one.

Supporting numbers, all `[MIL]` + `[DERIVED]`:

- **Stroke width 0.325–0.371 mm** — 5.4.7.4.2 gives ⅐ to ⅛ of the height for
  light characters on a dark background, which an engraved frost on black acrylic
  is.
- **Width-to-height 3:5** (5.4.7.5.1) → 1.56 mm digit width; "1" is one stroke
  wide (5.4.7.5.2b); "4" one stroke wider (5.4.7.5.2c).
- 2.6 mm is deliberately **not** the display's 2.544 mm text cell height.
  `precedents.md` §8.1 argues the engraving and the screen font should be
  different systems rather than a near-match, and a 0.056 mm difference would
  read as a failed match rather than a distinction. Agreed, and 2.6 is far
  enough away to be a choice.

Whether a 0.33 mm stroke resolves in cast acrylic on the intended machine is
**NOT SOURCED** — no vendor source I found gives a measured minimum line width or
kerf. It is a test cut, not a calculation (§11).

### 8.4 So the complete printed content of the plate is

```
13 digits.                                           one per aperture, 2.6 mm
2 end ticks per single-revolution control.           3 pots + 2 sliders = 10 marks
nothing else.
```

No function names. No group rules. No colour. No logo on the control plate — if
the instrument wants a maker's mark, it goes on the shell, which is where Terra's
brass inlay is `[VENDOR]` and where it commits nothing. `[REASONED]`

### 8.5 One thing to add that is not engraving: somewhere to write

The professional answer to a remappable control surface is a century old and it
is not an engraving or a screen. It is the **scribble strip** — "the area on
audio mixing boards designed to accommodate notes about what is on a given
channel… the space sound engineers use to name the channels", written on
removable tape, with *Sound on Sound* recommending Hosa LBL-505 "Scribble Strip
Console Tape" by name `[Sweetwater glossary; SOS]`.

**Recommendation: leave one deliberate blank land per bay, the bay's full
76.20 mm width, un-engraved and matt, sized to take a strip of 19 mm tape.**
`[REASONED]`

It costs nothing, it is the single most honest expression of "reconfiguration is
implied by the simplicity" — the panel is *built* to be relabelled by its user,
on the night, for one set — and it is Rams' principle 5 exactly: "neutral and
restrained, **to leave room for the user's self-expression**". A matt land also
takes a chinagraph pencil, which is what modular players already do. The land is
a layout requirement (width, position, finish) and so belongs in the
specification; what anyone writes on it is nobody's business but theirs.

---

## 9. The specification

One page, checkable, every row traceable to a clause or an arithmetic above.
`[MIL]` = the clause is quoted in §5.1 or §8.3. `[D]` = derived, shown above.
`[R]` = reasoned, flagged.

### 9.1 Distinct control sizes

| | value | source |
|---|---|---|
| Sizes used for coding | **2** | `[D]` §5.8 |
| Ceiling | 3 | `[MIL]` 5.1.1.4.3 |
| Encoder knob diameter | **13.00 mm** (window 13.00–13.05) | `[MIL]` 5.1.1.4.4.2 + the 19.05 pitch, §5.2 |
| Encoder graspable cylinder | **≥13.0 mm**, ~9 mm proud | `[MIL]` 5.1.1.4.4.2 a/c, §5.2 |
| Pot knob diameter | **≥26.00 mm** | `[MIL]` 5.1.1.4.3.2, §5.3 |
| Pot knob height | 13–25 mm | `[MIL]` fig 10 col B |
| Key cap | 13 mm, concave, matt | `[MIL]` 5.1.4.2.1.1.2 + .2.1, §5.7 |
| Rim gap between adjacent turnables | **≥6.00 mm** (have 6.05) | `[REPO]` layout.py FINGER |
| Individual knob wells | **forbidden** — annular gap at this pitch is 3.025 mm against a 6 mm fingertip minimum | `[D]` §9.6 |

### 9.2 The coding scheme

| class | count | diameter | rim | pointer | mode of operation |
|---|---|---|---|---|---|
| encoder | 4 | 13.00 | **knurled** `[MIL]` .4.1 | **none** — endless | rotary endless, detented, push |
| pot | 3 | ≥26.00 | **serrated** `[MIL]` .4.2 | **required** `[MIL]` 5.1.4.1.2.1.2 | rotary, 300° ± 5°, absolute |
| slider | 2 | cap 13 × 7 | — | the cap is the pointer | linear |
| key | 4 | 13 cap | — | — | momentary push, concave |

- Codes: **1 deliberate distinction** (pot ≠ encoder), carried by **2 independent
  free codes** (diameter step, pointer presence). Shapes used: 1 feature of 10.
- **Code the class, never the instance.** All four encoders identical; all three
  pots identical; all four keys identical. `[MIL]` 5.1.4.1.2.1.3, 5.1.1.4.3.1.
- No function-associative shapes. `[MIL]` 5.1.1.4.4.1.7 forbids them on a
  remappable surface — §5.1.
- **Stated departure:** three same-size pots cannot all reach 5.1.1.6's 125 mm.
  Best achievable **97.8 mm** at ±123.825 — 78.3 % of the threshold and 1.96× the
  50 mm optimum. §5.6, with three mitigations.

### 9.3 What is engraved

| | rule |
|---|---|
| **The test** | Engraving may describe the **mechanism**; never the **function**. Redundant with the geometry: permitted. Additional to it: forbidden. §8.1 |
| Function names | **0** |
| Ordinal digits | **1 per aperture, 13 total.** Cap height **≥2.6 mm**; stroke **0.325–0.371 mm**; W:H 3:5 `[MIL]` 5.4.7 / tables XVIII–XIX, §8.3 |
| Ordinal placement | above the aperture where ≥4.0 mm of land exists (bay offers 4.295); else below, as a stated departure `[MIL]` 5.4.3.1 "should" |
| End ticks | **2 per single-revolution control only** — 3 pots + 2 sliders = 10 marks. **None on the encoders.** `[MIL]` 5.4.2.2.3 |
| Graduated scales | **forbidden** — a scale commits a range |
| Group rules | **0** — grouping is by cut and by gap, §6.2.3 |
| Logo / maker's mark on the plate | **0** — it goes on the shell |
| **Blank legend land** | **1 per bay**, 76.20 mm wide, matt, un-engraved, sized for 19 mm tape. §8.5 |

### 9.4 Alignment rules

| | rule | source |
|---|---|---|
| Label column pitch | **19.050 mm**, *not* display_width/4 | `[D]` §2.3 |
| Characters per control | **15** (90 px). 14 misses by −6.52 %, 16 by +6.84 % | `[D]` §0.2 |
| Field block | 4 × 15 = 60 of 66 columns = 76.320 mm; margin 4.24 mm/side | `[D]` §0.2 |
| Screen-labelled bays | **exactly one per row, the one centred on x = 0**, and it is the encoder bay | `[D]` §2.5 |
| Legend row | one 12 px row at the display's **bottom** edge — 4.0 % of the display | `[MIL]` 5.4.2.4, §4.2 |
| Pots | **never screen-aligned** — 3 × 13 + 2 × 25 = 89.00 > 84.80 | `[D]` §4.3 |
| Everything on the module | 4.7625 mm, already checked | `[REPO]` layout.py check 2 |
| Symmetry | mirror about x = 0; at most **one** `free=True` exemption, declared | `[REPO]` check 6 + §6.3 |
| Inter-bay clear gap | **≥9.525 mm** (2 modules) > the 6.05 mm within-bay gap | `[D]`+`[R]` §6.2.3 |
| Bays stacked per band | **2**, not 3 (3 needs 87.42 mm against the 83.60 mm thumb limit) | `[D]` §2.6 |
| Bays side by side | 3 fit (228.60), 4 do not (304.80), in 288.35 | `[D]` §2.5 |

### 9.5 Colour budget

| | value | source |
|---|---|---|
| Permanent colours on the plate | **1** — one neutral. Zero accents | `[MIL]` 5.1.1.4.5.2 + Braun, §7.2 |
| Engraved mark | the frost of cast acrylic — the second "colour", free | §7.2 |
| Software colours (NeoPixel), simultaneous | **≤5** | `[MIL]` 5.1.1.4.5.1 |
| Colour as primary identification | **forbidden** | `[MIL]` 5.1.1.4.5.9.1 — varying stage light |
| Gloss | **forbidden** on controls | `[MIL]` 5.1.1.4.5.4 |
| Sheet | **cast** acrylic, not extruded | §7.2, §11 |
| Licensed exception | one safety-critical control, if the design ever has one | `[MIL]` 5.1.1.4.5.6 |

### 9.6 The ordering convention

1. Reading direction **left → right**, then **far → near**. Monotonic in x within
   a bay; bay index increases with distance from the display.
2. **Ordinal 1** is the leftmost element of the bay nearest the display. One
   origin, no alternatives.
3. Grouping is **by bay and by nothing else** — the bay is the hardware's own
   enforced group (one board, one address, one cable). No finer or coarser visual
   grouping. `[MIL]` 5.1.1.4.2.2 + §6.4.
4. The **centreline is the index**: symmetric layout, axis in the gap between
   elements 2 and 3, count outward by ≤2.
5. The ordinal engraved on the plate must be the **same token the documentation
   and the instrument's own output use**. `precedents.md` open question 10 asked
   what a local control prints as its source in `>lanes`; this is that answer's
   other half, and the two must not diverge. `[R]`

### 9.7 Checks to add to `tools/layout.py`

The repo's discipline is that the checks parse the emitted SVG and measure drawn
geometry rather than reading the spec `[REPO]`. These fit that pattern; each is a
rule above restated as a measurement.

| check | fails when |
|---|---|
| `SIZECODE` | more than 3 distinct touch diameters among `knob`/`encoder`; or two classes differing by < 13.00 mm |
| `CLASSUNIFORM` | two controls of the same `kind` differ in `w`/`h` by > TOL |
| `COLUMNPITCH` | a screen-labelled element's x is not an integer multiple of 19.050 from the display centre, within 0.03 mm |
| `COLUMNFIT` | a screen-labelled element's x lies outside ±42.40 (the display's active half-width) |
| `ONECENTRED` | more than one bay in a row has \|x\| < TOL |
| `GROUPGAP` | the clear gap between two bays is < 9.525 mm, or ≤ the largest within-bay clear gap |
| `ORDER` | ordinals are not monotonic in x within a bay, or the origin is not the leftmost element of the bay nearest the display |
| `BANDDEPTH` | stacked bays plus gaps exceed 83.60 mm, or the control centreline exceeds 45.00 mm from the keyboard's top edge |
| `INKBUDGET` | the drawing contains any engraved text that is not a single digit, or any engraved mark that is not an end tick |
| `TICKHONESTY` | an end tick is drawn against a control whose `kind` is `encoder` |
| `COLOURBUDGET` | more than one permanent fill colour appears on `on_plate` geometry |
| `LEGENDLAND` | a bay has no adjacent blank land ≥76.20 mm wide and ≥19 mm deep (§8.5) |
| `WELLGAP` | a knob is drawn inside a well whose annular gap is < 6.00 mm |
| `FREECOUNT` | more than one item carries `free=True` |

---

## 10. What this changes elsewhere in the repo

Flagged, not edited — the parent session owns the commits.

1. **`envelope.md` "Open, and blocking geometry" item 3** — "Alignment to the
   display… Either the display grows or the panel earns its legibility another
   way" — is **closed**. The display does not grow; the chosen part is narrower
   than it already. §2.
2. **`precedents.md` §8.2's conclusion** — "identify each encoder by an engraved
   glyph plus a distinct rim treatment" — is **superseded**. Per-instance rim
   coding violates 5.1.4.1.2.1.3 and 5.1.1.4.3.1, and is unnecessary now that the
   display can name by column. §5.5.
3. **`precedents.md` §3's closing claim** — "Every control on v2 must carry a
   permanent engraved identity *and* a software-assignable meaning. The engraving
   is not a default; it is the control's name" — is **right in substance and
   wrong in scale**. The permanent identity is an **ordinal**, not a name. §6, §8.
4. **`envelope.md`'s 0.05 mm knob window** stands, but the citation needs
   changing from Figure 10 to 5.1.1.4.4.2. §5.2.
5. **`precedents.md` §6.2's Figure 10 table** is correct; `envelope.md`'s prose
   restatement of it is not. §5.2.
6. **`precedents.md` §8.1's Nobel/Gugelot claim** needs the correction in §7.3 —
   the attribution was withdrawn on the cited page.
7. **`keyboard-k9pro.md` §11.2's "roughly three stacked breakout rows"** becomes
   **two** once the inter-bay gap respects the grouping rule. §2.6.
8. **`envelope.md`'s open encoder-datum question** is closed by the §2.1
   measurement: body top at 7.50 above the PCB front face, 6.60 of bushing, shaft
   tip at 21.40, one datum.

---

## 11. NOT SOURCED — get the calipers, or make a test cut

Honest gaps. None of these is guessed at anywhere above.

1. **Viewing distance for this instrument, played standing.** Every engraved
   character-height number in §8.3 is a function of it. MIL's default is 710 mm;
   a held instrument is plainly nearer. **Measure it** — hold a K9 Pro-sized
   board the way it will be played and put a tape on it. The spec's 2.6 mm covers
   300–460 mm on the dim-stage figure, so the measurement either confirms it or
   raises it.
2. **Minimum engraved line width and kerf in cast acrylic**, on the intended
   machine. Needed to know whether a 0.325–0.371 mm stroke resolves. No vendor
   source I found publishes measured values and all of them sell lasers or
   acrylic. **Test cut.**
3. **Whether the 5752's NeoPixels are visible at all with encoders fitted.**
   `parts.md` §2.4 measured them at exactly the encoder centres, under the bodies.
   The whole software-colour layer in §1.2 and §9.5 depends on this. **Verify on
   hardware** before committing apertures; the 4991/5880 are the fallback.
4. **Pot knob availability at ≥26.00 mm with a serrated rim and a pointer**, on a
   Ø6.0 T18 knurled shaft `[REPO: parts.md §5]`. The size code is only real if the
   knob is orderable. No part number is given above because I did not source one.
5. **NeoSlider travel** — already `parts.md` §10's caliper job, and it bounds the
   slider's two end ticks.
6. **"Autovisuality"** — B&O's 1971 principle list is sourced; the definition is
   not, in any source I would cite. §7.4 does not build on it.
7. **Jenkins (1947) on knob shape coding** — the citation is solid (W. O. Jenkins,
   "The tactual discrimination of shapes for coding aircraft-type controls", in
   Fitts, *Psychology of Equipment Design*, AAF Aviation Psychology Program
   Report No. 19). The commonly-repeated "eight shapes, never confused" result I
   **could not verify** and have not used. MIL's own ≤10 (5.1.1.4.4.1.4) is the
   number this document relies on. MIL Figure 1, "Easily recognizable knob
   shapes", is a raster figure in the PDF and I could not read its shapes.
8. **The F/J homing-bump standard.** Widely described as a convention; I found
   **no** evidence it is required by ISO/IEC 9995, and one source claiming so was
   not credible. §8.2 therefore cites ETSI ES 201 381 and ITU-T E.161 (the "5"
   key tactile identifier), which *are* standards, and not the keyboard bumps.
9. **A quantified Gestalt proximity ratio.** The 1.5× in §6.2.3 is mine. I found
   no standard that gives a number for between-group versus within-group spacing.
10. **The Digitakt manual's own wording on the A–H knobs.** `cdn.www.elektron.se`
    does not resolve from this session's network, so the A–H labelling is cited
    from the **Syntakt** manual, which uses the identical sentence, plus a
    Sweetwater quickstart and third-party copies of the Digitakt manual. If the
    exact Digitakt wording matters, that host needs allowing in the session's
    network settings.
11. **Faderfox EC4's physical panel markings.** The vendor page does not say
    whether the encoders carry printed ordinals, and the manual copy I tried
    returned 404. The display behaviour (4 × 20 characters, 4-character names,
    4 × 4 matrix) is sourced; the panel printing is not.
12. **Push 2's column-to-encoder alignment as a vendor statement.** The manual
    says "The display is divided into columns" and "the eight encoders above the
    display" `[VENDOR]`, which together strongly imply the alignment, but I did
    not find a sentence that states it outright. The inference is
    `precedents.md`'s and mine, and the photographic evidence is not a citation.

---

## 12. Open, and blocking the layout

1. **The three-pot symmetry conflict.** §6.3. The encoder bay must be centred on
   x = 0 to get display columns; three pots can only be mirror-symmetric with one
   on the axis. They cannot share a row. Either the pots take their own row
   (centre pot on the axis, encoder bay in another row) or the third pot is the
   panel's one declared `free=True` asymmetry. **This needs deciding before any
   layout is drawn**, and it is a geometry question, not a taste question.
2. **Where the pots live: flanking the display, or in the control band?** The
   flanks offer 101.775 mm of land each side `[D]` and would let *two* pots
   exceed 125 mm (160.575 mm edge-to-edge), which is the only placement that
   satisfies 5.1.1.6 for any pair. The control band offers symmetry and ~98 mm
   for all three. The pots are set-and-leave and so are licensed to sit outside
   the 45 mm thumb envelope (`precedents.md` §4) — which argues for the flanks,
   i.e. Organelle's local arrangement, which `precedents.md` §5 already said to
   adopt locally. Not resolved here.
3. **Which encoder board.** 5752 gives the shared bay outline, one address, one
   cable and the guaranteed 19.05 pitch, but its NeoPixels are measured to sit
   under the encoder bodies (§11.3). 3–4 × 5880 gives per-knob colour but a
   25.4 mm minimum pitch, which breaks the 19.05 column scheme in §9.4 and
   therefore breaks the display alignment that this whole document rests on.
   **The alignment finding is an argument for the 5752 and against per-encoder
   colour.** The user's decision list permits either, so this is a trade to put
   in front of them, not a decision to take.
4. **Does the legend row survive the display's day job?** §4.1.5. One 12 px row
   of 63.60 mm is 4.0 % of the panel, which is cheap, but whether the display can
   hold a row permanently is a display-content question and explicitly out of
   scope. The enclosure's only stake is that it must not place the control band
   so far from the display that MIL 5.4.2.4's proximity claim fails.
5. **Plate finish.** §7.2 specifies matt and cast. `precedents.md` §8.3 leaves
   open whether v2's control plate is acrylic, oak or brass; the user has decided
   acrylic, and §7.2's "the engraving is the second colour" argument depends on it
   being **cast** and **black**. If the plate ever becomes wood or brass, the
   colour budget in §9.5 needs redoing, because engraved oak is not white.

---

## 13. Sources

**The standard.** MIL-STD-1472H, downloaded and read as text:
<https://cvgstrategy.com/wp-content/uploads/2023/04/MIL-STD-1472G.pdf> (file
named …G, header reads H). Clauses used: 5.1.1.4 and table IV (coding modes),
5.1.1.4.2–.2.2 (location), 5.1.1.4.3–.3.3 (size), 5.1.1.4.4–.4.2 (shape),
5.1.1.4.5–.5.9.2 (colour), 5.1.1.5–5.1.1.6 (labelling, blind operation),
5.1.4.1.2.1–.1.7.3 and figures 9–10 (knobs, rims, pointers, dimensions),
5.1.4.2.1.1.2–.2.1 (concave push buttons), 5.4.1–5.4.3.6 (labelling, hierarchy,
placement), 5.4.7–5.4.7.6.2 and tables XVIII–XIX (character design and height).

**Vendor models and repo files.** `5752 Quad Rotary Breakout.stl` in
`/home/user/adafruit/adafruit_cad_parts`, measured; `v2/parts/parts.md`;
`v2/docs/envelope.md`; `v2/docs/v1-datums.txt`; `v2/research/precedents.md`;
`v2/research/keyboard.md`; `v2/research/keyboard-k9pro.md`; `v2/tools/layout.py`;
`docs/HARDWARE.md`; `docs/OS.md`.

**Instruments.**
monome norns — [Sound on Sound review](https://www.soundonsound.com/reviews/monome-norns),
[docs](https://monome.org/docs/norns/), [play guide](https://monome.org/docs/norns/play/) ·
monome arc — [docs](https://monome.org/docs/arc/) ·
TE OP-1 — [layout guide](https://teenage.engineering/guides/op-1/original/layout) ·
Ableton Push 2 — [manual](https://www.ableton.com/en/manual/using-push-2/) ·
Faderfox EC4 — [product page](https://www.faderfox.de/ec4.html) ·
Intech Studio EN16 — [shop page and customer reviews](https://intech.studio/shop/en16) ·
Electra One — [user interface docs](https://beta-docs-electra-one.web.app/version3/userinterface.html),
[user guide](https://beta-docs-electra-one.web.app/downloads/docs/E1-UserGuide-v4.0.pdf),
[SOS review](https://www.soundonsound.com/reviews/electra-one-mkii) ·
Elektron Syntakt — [interactive manual](https://elektron-manual.vercel.app/en/syntakt/manual/002_2_2-the-syntakt.md);
Digitakt — [manual PDF](https://cdn.www.elektron.se/media/downloads/digitakt/Digitakt_User_Manual_ENG_OS1.50_230303.pdf)
(host does not resolve from this session, §11.10) ·
Novation Launch Control XL — [MusicRadar review](https://musicradar.com/reviews/tech/novation-launch-control-xl-608521),
[Ableton Live guide](https://fael-downloads-prod.focusrite.com/customer/dev/s3fs-public/novation/downloads/25576/lc-xlableton-live-guide.pdf) ·
Soma Terra — [product page](https://somasynths.com/terra/) ·
Serge patch programmability — [IDM NYU](https://idmnyu.github.io/audiolab/serge.html),
[Learning Modular glossary](https://learningmodular.com/glossary/dusg/) ·
Patchblocks for Eurorack — [Synthtopia](https://www.synthtopia.com/content/2015/12/24/patchblocks-for-eurorack-now-available-for-preorder/) ·
Midi Fighter Twister — [DJ TechTools](https://store.djtechtools.com/products/midi-fighter-twister)
(16 endless encoders with RGB rings and four banks; **no source found either way
on printed panel markings**, so it is not used as evidence above).

**Scribble strips.**
[Sweetwater / Full Compass glossary](https://www.fullcompass.com/gearcast/glossary/definition-scribble-strip) ·
[Sound on Sound, "What's the best console tape?"](https://www.soundonsound.com/sound-advice/q-whats-best-console-tape).

**Design language.**
[Vitsœ, Dieter Rams' ten principles](https://www.vitsoe.com/gb/about/good-design) ·
[Port Magazine on the Braun ET66](https://www.port-magazine.com/design/counter-culture/) ·
[Fonts In Use, Braun RT 20](https://fontsinuse.com/uses/6957/braun-rt-20) and
[Braun](https://fontsinuse.com/designers/3002/braun) (the Nobel correction, §7.3) ·
[Wikipedia, Jacob Jensen](https://en.wikipedia.org/wiki/Jacob_Jensen) ·
[BeoWorld, BeoMaster 1900](https://beoworld.org/?p=45666) ·
[V&A, BeoMaster 1900](https://collections.vam.ac.uk/item/O321483) ·
[Stylepark, "The liberation of the box"](https://www.stylepark.com/en/news/the-liberation-of-the-box-jacob-jensen-and-bang-olufsen-design) ·
[B&O design philosophy](https://beo.zone/en/beo-design-philosophy).

**Tactile identifiers and enumeration.**
[ITU-T E.161 summary](https://www.itu.int/dms_pubrec/itu-t/rec/e/T-REC-E.161-200102-I!!SUM-HTM-E.htm) ·
[ETSI ES 201 381](https://www.etsi.org/deliver/etsi_ES/201300_201399/201381/01.01.01_60/es_201381v010101p.pdf) ·
subitizing — [Attention, Perception & Psychophysics](https://link.springer.com/article/10.3758/s13414-011-0135-5),
[Springer PDF on the history of the term](https://link.springer.com/content/pdf/10.3758/BF03207540.pdf).

**Material.**
[Thunder Laser, extruded vs cast acrylic](https://www.thunderlaser.com/laser-wiki/material-applications/extruded-acrylic-vs-cast-acrylic) ·
[Boss Laser knowledge base](https://team.bosslaser.com/?p=327) ·
[1Laser, laser processing acrylic](https://wiki.1laser.com/knowledge-base/popularization/laser-processing-acrylic).
All three are commercial sources with no measured data; see §11.2.
