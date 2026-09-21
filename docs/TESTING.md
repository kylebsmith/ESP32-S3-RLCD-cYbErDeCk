# Testing the deck

A pass over everything the device does. Each step is one thing, and each says
what you should see — if you don't see it, that's the bug, and the step number
is enough to report it.

Ctrl+Enter runs the line the cursor is on. Ctrl-O comes back from any output
page. Ctrl-L and Ctrl-J walk between documents.

## 1. It boots

Power on. You should see **KILROY** appear a letter at a time, a rule sweep
under it, then a line of numbers — documents and free memory — then the editor.

If the screen stays blank, a button is held down: every reset reports
`boot:0x22 (DOWNLOAD)` when GPIO0 is low. Release it and power-cycle.

## 2. The guide plays

Ctrl-L until the status bar reads `guide`. Run each `>` line from the top.
By `>play` you should have a kick, a hat with ghost notes, a bassline in D
minor, and a filter sweep.

An inverted bar should sweep along each pattern line in time with the sound.

## 3. Editing while it runs

With it still playing, change a character in the kick line and Ctrl+Enter.
It should change on the next bar without stopping.

Then Ctrl+Enter on that same line **without touching it** — the lane goes
silent. Again — it comes back.

## 4. Probability

```
>hat ?[3]?[3]?[3]?[3]
>hat ?[97]?[97]?[97]?[97]
```

Near-silence, then near-constant. `>lanes` prints an `odds` line showing the
percentages it read.

## 5. Time ratios

```
>hat x.x.x.x. /2
>hat x.x.x.x. *2
```

Half speed, then double. The kick keeps its own tempo — each lane divides the
clock separately.

## 6. Mute and solo

```
>mute hat bass
>solo kick
>mute
```

Several at once. Bare `>mute` or `>solo` brings everything back.

## 7. USB MIDI

```
>usb on
```

The deck reboots and comes back as one USB device carrying MIDI, the console
and the keyboard. In a DAW, pick **cyberdeck usb** and record-arm a track with
a drum rack — drums are General MIDI on channel 10, so the kick lands on the
first pad.

`>usb off` to come back. A power cycle also always returns to serial: the mode
lives in RTC memory, so there is no setting that can fail to save.

## 8. MIDI clock

```
>sync on
```

In Ableton: Preferences → Link/Tempo/MIDI, enable **Sync** on the cyberdeck
input, set the transport to External.

## 9. Network

```
>host deck 12345678
```

The deck becomes a Wi-Fi network at `192.168.4.1`. Join it from a laptop.

Or join an existing one:

```
>wifi <ssid> <password>
>wifi
```

The second command shows the address once it has one. Credentials are
remembered **as soon as they are typed**, and re-joined at every boot.
`>wifi forget` clears them.

## 10. OSC out

On the laptop:

```bash
python3 tools/osc_listen.py
```

On the deck:

```
>osc <the laptop's ip> 9000
>play
```

You should see `/deck/step` with the bar position and `/deck/<lane>` for every
hit, by lane name.

## 11. Visuals, in the same document

```
>kick X...x...X...x...
>bass 0...5...3...7...
>viz noise x?x?x?x?
>viz bar 0..3..9..3..
>route noise bass
>play
```

The right side of the screen becomes a live ASCII frame, advancing on the same
clock as the music. `>split` toggles the preview; the visual lanes keep running
either way.

`>route noise bass` makes the bass note's velocity drive the noise density —
the visual line says *when*, the music lane says *how much*. Unroute with
`>route noise`.

Four generators: `noise`, `bar`, `dot`, `wave`. Digits 0–9 are intensity, and
everything else about the pattern is the same grammar as a drum lane — `?`,
brackets, `/2` and `*2` all work.

## 12. ASCII frames

Make a document with some ASCII art, then:

```
>frame
```

The listener prints it between `--- frame ---` markers. Editing the drawing and
running `>frame` again is the whole of visual coding on this device.

## 13. SSH

Turn on Remote Login (macOS: Settings → General → Sharing). Then with the deck
on the same network:

```
>ssh you@192.168.1.42 yourpassword ls
```

The reply arrives in `+ssh`. Ctrl-O comes back.

The host key is **shown, not verified** — the fingerprint is printed so you can
see it change. The password is on the line, which is why `+ssh` is transient
and never reaches the journal, the SD card or a backup.

## 14. Battery

```
>battery
```

Unplug USB, run it again. The channel whose voltage **moved** is the cell.
Then:

```
>battery use 4
```

Persistent, no reflash. A four-cell bar appears at the right of the status row.
If the divider isn't 2:1, `>battery use 4 30` for 3:1.

## 15. Settings survive

Ctrl-L to `boot`. It holds ordinary commands that run at startup:

```
>bpm 124
>scale dmin
>density dense
```

Edit it, power cycle, and they take effect. `>density low` is 30 columns,
`>density high` is 60. There is no middle yet — it needs a third font face
drawn at about 9x18, and `>density mid` says so rather than quietly picking
one of the two.

## 16. It recovers

If it crashes it now reboots in about two seconds and the screen says
`crashed Nx - unplug to clear`. That counter clears on a real power cycle:
unplug and hold PWR, since the battery means unplugging alone doesn't
power it down.

## Known gaps

- `>flash now` doesn't reach the bootloader while in USB MIDI mode. Use the
  BOOT button to reflash from there.
- `>lanes` shows a pattern from its compiled form, so spacing you typed for
  readability isn't echoed back.
- No SSH key auth yet — password only.
