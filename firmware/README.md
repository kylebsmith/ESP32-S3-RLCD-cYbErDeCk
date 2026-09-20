# cYbErDeCk OS — firmware

Steps 1–4 of the build order in [../docs/OS.md](../docs/OS.md): display, text
grid, BLE HID keyboard, and a text buffer that survives power loss.

## Build and flash

Flashing is button-free — the ESP32-S3's USB-Serial/JTAG enters download mode
over the USB control lines, so nobody has to hold BOOT.

```sh
. ~/esp/esp-idf/export.sh
cd firmware
idf.py -p /dev/cu.usbmodem2141101 flash monitor      # macOS
idf.py -p /dev/ttyACM0            flash monitor      # Linux
```

If it ever fails to enter the bootloader, add `--before default_reset` to a
direct `esptool` invocation. Exit the monitor with `Ctrl-]`.

## Layout

| Path | What |
|---|---|
| `components/st7305` | the panel driver: framebuffer, window addressing, HPM/LPM, orientation |
| `components/textgrid` | the character grid, the two bitmap faces, per-cell damage |
| `components/kbd` | NimBLE HOGP keyboard host, bonds, key repeat |
| `components/docstore` | gap buffer, the power-safe flash journal, the SD mirror |
| `main` | boot, the editor, the test card, the persistence self-test |

`components/st7305` is derived from SolarOS under Apache-2.0 — see [NOTICE](NOTICE).

## The two grids

Cell **height** must be a multiple of 12 because that is the CASET quantum in
landscape; cell **width** only has to be even, because the RASET quantum is
2 px. That asymmetry is the design freedom: width is cheap, height comes in 12s.

| Face | Cell | Grid | Use |
|---|---|---|---|
| `tg_font_12x24` | 12 × 24 | **33 × 12** | the default — a reflective panel with no backlight needs the weight |
| `tg_font_6x12` | 6 × 12 | 66 × 25 | dense work; too small to read comfortably on the real panel |

Switch with `tg_set_font(&tg_font_6x12, 1)`. The call refuses a misaligned
cell rather than letting one line's damage window spill into its neighbours.

Both faces are generated from reviewable ASCII art:

```sh
python3 tools/make_font.py          > firmware/components/textgrid/font6x12.c
python3 tools/make_font.py --12x24  > firmware/components/textgrid/font12x24.c
```

## Buttons

| Gesture | Effect |
|---|---|
| KEY (GPIO18) tap | cycle the display orientation, saved to NVS |
| KEY held 2 s | forget every keyboard bond and rescan |

The orientation cycle exists because the landscape mapping has a two-way
ambiguity per axis that depends on how the glass sits in the module, and no
datasheet settles it. A wrong guess costs a button press, not a reflash.
