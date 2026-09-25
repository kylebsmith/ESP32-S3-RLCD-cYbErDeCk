# The view node — the deck's picture on HDMI

*[NEXT.md](NEXT.md) §6. An Adafruit Feather RP2040 DVI draws the deck's picture on
any HDMI screen, from frames the deck sends. The sketch is `view/deckview`; this
page is its wire format and what is and is not yet true of it.*

---

## What it is

The deck's picture lanes draw a frame of cells — its own tiles, codepoints 128–155,
nine tones and the shapes — and the panel shows it in the split. With
`>send view on` the deck also sends each frame to the view node, which draws it at
640×480, one bit per pixel, **in the deck's own glyphs**: `view/deckview/deckfont.h`
is generated from the same art as the panel's faces (`tools/make_font.py --view`,
diffed in CI), so a tile on the screen is the tile on the panel, bit for bit.

The node picks whichever face and whole-number scale fills the screen best and
centres the frame: a cell is always a square block of its tile, never smeared.

**Light ink on black.** The panel is dark ink on reflective paper; the screen emits
light, so the node draws ink as light. The owner's first look, 2026-09-25: "an
inverted version of the display — that's amazing." Kept as the default.

## The size

| on the deck | |
|---|---|
| `>send view on` | frames go out at **53×20** cells — the whole 640×480 screen in the 12×24 face |
| `>send view 40x12` | at any size up to 60×24 |
| `>send view off` | stop, and the panel's split decides the size again |

**The deck's preview keeps its own shape.** When the view sets the size, the
engine draws at the view's size and the split shows a sample of it — the preview
stays what it was asked to stay, an approximation of the output. With the view off
nothing about the split changes.

## The wire format

```
'D' 'K' 'V' '1'   magic
tick  u32 LE      the deck's pulse this frame was drawn for
w, h  u8, u8      the frame in cells
cells w*h bytes   row by row: 32–126 text, 128–155 the deck's tiles
sum   u8          XOR of every byte after the magic
```

**Every frame carries its tick**, so a node that joins the ensemble can show a frame
when it was meant to be shown rather than when it arrived — the wireless node rides
the deck's clock instead of guessing. Today the node shows each frame on arrival
and reports the tick it last drew.

**One lost byte costs one frame.** The node keeps the bytes since a frame's magic,
and when a frame is refused it reads them again one byte later, finding the magic
the torn frame had swallowed. The reader this replaced lost the next frame too.
Packing is `firmware/main/view_wire.h`, reading is `view/deckview/view_read.h`, and
`tools/test_view_wire.c` runs the one through the other in CI — a torn frame, junk
with a false magic in it, 20 KB of nothing but torn frames, the largest frame.

## How it gets there — today, and not yet

**Today a computer relays it.** The deck writes each frame to its console as a
terminal escape — `ESC ] view;<base64> BEL` — which a terminal swallows, and
`tools/viewrelay.py` lifts the frames out and writes them to the node's USB serial.
The frames are the deck's own; only the cable between the two is stood in for.

**Not yet: the deck driving the node directly over USB.** That is the intended link
and it is not built. When it is, the bytes are already the wire format; only the
transport in `firmware/main/view.c` changes.

## Measured, 2026-09-25

- First light: the deck's picture on HDMI through the relay, `53×20`, frames of
  1071 bytes, **one per sixteenth** — 86 frames in about 10.5 seconds at 124 bpm,
  8.2 a second against 8.27 sixteenths, and **0 refused** by the node.
- Build: 86 KB of flash, 58 KB of RAM with the reader's buffers, before the two
  38 KB framebuffers. Uploaded with no button, by the core's 1200-baud reset.

**Unmeasured, and the brief asks for it measured:** whether the node can run from
the deck's battery over USB-C, and what that costs the deck in runtime. It needs the
deck powering the node, which needs the direct link.

**Unverified:** anything about how the picture looks, beyond the owner's first
look above — there is no camera here. The node reports frames drawn and refused,
not pixels.
