# The network: the deck as laptop and router

*Whether the deck can host its own Wi-Fi network, let Raspberry Pis join it,
SSH into them, and route them out through a phone — replacing the laptop and
the router currently carried to every installation.*

**The answer is yes, on this silicon, with no coprocessor.** This document
records the evidence, the numbers, and the four things that will break first.

Tags: `[BUILT]` compiled and run during this assessment · `[MEASURED]` read
from a linker map, `nm`, or an instrumented run · `[PROJECTED]` computed from
measured parts, not observed · `[OPEN]` not established.

## The headline

| Question | Answer |
|---|---|
| SSH client on the ESP32-S3? | **Yes** `[BUILT]` |
| Coprocessor needed? | **No** |
| Anything installed on the Pi? | **No** — but sshd must be *enabled* once |
| SoftAP + station at once, with NAT? | **Yes** `[BUILT]` |
| Does it fit beside the display, editor and BLE? | **Probably** `[OPEN]` — see *the one real gap* |

## What was actually built, not argued `[BUILT]`

Two independent SSH stacks were compiled for `esp32s3` against **this
machine's own ESP-IDF v5.5.4**, the version the project pins:

- **wolfSSH + wolfSSL** built clean as an ESP-IDF component. The same client
  code was then run natively against a stock **OpenSSH 9.9p2** server, where
  it completed key exchange, authenticated by public key, opened an exec
  channel, ran `uname -a`, and returned the output — and separately opened a
  PTY and received the login banner.
- **libssh2 + mbedTLS** via `skuodi/libssh2_esp`, likewise.
- A **full-stack probe** — Wi-Fi AP+STA, `esp_netif_napt_enable()`, NimBLE and
  libssh2 together — compiles, links and fits.

## The numbers `[MEASURED]`

| Item | Cost |
|---|---|
| libssh2 flash, marginal | **73,148 B** (64,156 .text + 8,296 .rodata), IRAM 0 |
| libssh2 **internal** SRAM, static | **696 B** |
| wolfSSH + wolfSSL flash | 152,663 B; **128 B** static RAM |
| `LIBSSH2_SESSION` | **80,728 B in ONE allocation** → lands in PSRAM, ~1 % of 8.2 MB |
| wolfSSL peak heap, one full session | **31,999 B** on an arm64 host, 0 leaked |
| …the same, projected for xtensa | **15–20 KB** `[PROJECTED]` |
| Full-stack probe, internal DIRAM free | 204,589 B — *but see below* |

The app partition is 3 MB with ~600 KB used, so an SSH client is about 3 % of
the flash that is already spare.

## A correction to `docs/OS.md` `[MEASURED]`

`OS.md` states: *"TLS ≈ 40–50 KB free internal heap … **exactly one TLS
session at a time**"*, and treats that as a hard constraint on anything
networked.

**That figure does not bound an SSH client.** It is an mbedTLS number, driven
by mbedTLS's TLS *record* buffers — `CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN=16384`
and `CONFIG_MBEDTLS_SSL_OUT_CONTENT_LEN=4096` in this project's own
`sdkconfig`, 20,480 B of buffers that exist only for TLS record framing. SSH
frames its own packets and never allocates them. Confirmed by reading the
built configuration rather than by inference.

The constraint was correctly derived for TLS and wrongly generalised to "the
network". Recorded here rather than silently corrected in place.

## The four things that break first

Each of these was found by an adversarial pass over the original assessment,
and each is cheap to fix *if known in advance*.

1. **ed25519 host keys.** libssh2's mbedTLS backend hard-codes
   `#define LIBSSH2_ED25519 0`, so it cannot verify an ed25519 host key —
   and stock Raspberry Pi OS generates rsa, ecdsa **and** ed25519. wolfSSH has
   the same trap for a different reason: Espressif's stock `user_settings.h`
   silently disables ed25519 unless three defines are all present, so the deck
   would pin the Pi's *ECDSA* fingerprint rather than the ed25519 one everyone
   actually knows. Three-line fix, verified by rebuilding and diffing the
   advertised algorithm list.
2. **The Pi has no internet.** A SoftAP is an island. The whole point is that
   `claude` runs on the Pi — which needs the network. SSH will work perfectly
   and every agent command will fail. This is why NAPT and the phone uplink
   are not optional extras but part of the minimum viable version.
3. **The phone's band.** The ESP32-S3 radio is 2.4 GHz only. An iPhone
   Personal Hotspot defaults to 5 GHz and only offers 2.4 GHz with *Maximize
   Compatibility* switched on. A one-toggle problem that would otherwise look
   like a driver bug.
4. **No terminal emulator, and SSH does not supply one.** This is the largest
   unbudgeted item for an interactive shell — and it is exactly why the design
   decision below matters.

## The design decision this forces `[JUDGEMENT]`

**Run commands and show output in a buffer. Do not emulate a terminal.**

A VT100 wants 80 columns; the deck has 30. An interactive PTY would mean
writing a terminal emulator, carrying a scrollback model that is not the
document model, and contradicting `SUBSTRATE.md`'s claim that there is one
data structure.

An **exec channel** — `ssh host "command"`, output into a buffer — needs no
emulator, produces text that is already the substrate, and is scrollable,
searchable, undoable and pipeable by every mechanism the editor already has.
It is also the shape of the owner's actual workflow: restart a service, read a
log, ask the agent on the Pi to fix something.

Interactive shells are therefore **rejected, with the reason recorded**, not
merely postponed.

## The one real gap `[OPEN]`

**No board has been flashed.** Every internal-heap figure for the *combined*
system is projected.

The 204,589 B of free DIRAM in the full-stack probe is real, but that probe
contains **no part of this firmware** — no ST7305 driver, no framebuffer, no
text grid, no editor, no docstore, no BLE HID host. The honest statement is
that libssh2 adds 724 B of internal SRAM and ~84 KB of flash to a system whose
Wi-Fi baseline has never been measured on this device, because this firmware
has no `esp_wifi` in it at all.

**Closing this costs one flash and about five minutes.** A probe exists that
prints free internal heap at six stages — boot, after NimBLE, after
`esp_wifi_init`, after `esp_wifi_start`, after NAPT, after session init. It
overwrites the deck, so it is the owner's call.

Until then: feasible on strong evidence, unproven on this board.

## Build order, when this is picked up

1. **Measure first.** Bring up Wi-Fi in *this* firmware and log free internal
   heap at each stage. Everything below is contingent on that number.
2. SoftAP only; confirm a Pi joins and gets a lease.
3. `esp_netif_napt_enable()` on the AP, station on the phone; confirm the Pi
   reaches the internet. *This is the gate on the whole agent workflow.*
4. libssh2 exec channel to a known host, output into a buffer.
5. Host-key verification with the ed25519 fix, and key storage in NVS.
6. `net.hosts`, `net.run` and friends as ordinary rows in the command table —
   at which point the palette, the guide file and the agent all reach them
   with no new plumbing.

## Sources

Everything above was compiled, linked, measured or executed during the
assessment on 2026-09-20, against ESP-IDF v5.5.4 (`dfe53e20`),
xtensa-esp32s3-elf-gcc 14.2.0, libssh2 1.11.2_DEV, wolfSSH master, wolfSSL
v5.9.1-stable and OpenSSH 9.9p2. Raspberry Pi OS sshd defaults were read from
the Debian trixie `sshd_config(5)` for OpenSSH 1:10.0p1.

---

## Shared time: two decks, and Ableton `[OPEN]`

Three problems that look like one and are not. Separating them is most of the
work, because the cheapest answer to two of them already exists.

### What already works, today, with no new code `[FACT]`

**Ableton follows the deck over MIDI clock.** The deck sends Song Position Pointer
zero, then Start, then a timing clock every 24 PPQN, and Stop on stop. Live's
External Sync accepts exactly that. Measured dispatch: **sd 3 µs, spread 79 µs,
zero ticks late in five thousand** — good enough that the earlier complaint about
Live's tempo follower hunting was a jitter problem that has since been fixed, not
a protocol problem.

Two real bugs were found auditing this and both are fixed:

- **Song Position Pointer went out as a bare status byte on DIN.** `0xF2` carries
  two data bytes; the length table returned 0 for everything from `0xF0` up, which
  is right for System Realtime and wrong for System Common. A receiver counts data
  bytes, so it would have waited, swallowed the next status byte, and desynchronised
  on the one message that *begins* a synchronised performance — with no error
  anywhere. `tools/test_midi_wire.c` now pins every status length.
- **The `usb` destination silently failed to register.** `SEQ_MAX_DESTS` was 4,
  `din` took the fourth slot, and `seq_dest_add()`'s error was discarded at every
  call site. The deck came up in USB MIDI mode with a host attached, the heartbeat
  reporting `act1 dev1 midi1`, `>usb` reporting "usb is on, host attached", and
  nothing to route to. Everything said yes and nothing played. There is a
  `_Static_assert` on the count now, so the next transport either fits or fails
  the build.

So the honest status of "can I sync Ableton": **yes, as the clock master, over USB
or DIN.** What MIDI clock cannot do is let *Live* change the tempo, or communicate
bar phase beyond SPP. That is what Link is for.

### Deck to deck `[OPEN]`

Two performers, two surfaces, different outputs, one time. Ranked:

1. **A wire.** One deck's `din` out to the other's MIDI in. Jitter-free in the way
   DIN always is, and needs no network. Blocked only by the deck having no MIDI
   **input** — that needs an optocoupler, and it is the smallest missing piece in
   this whole document.
2. **A UDP beat packet over SoftAP.** One deck hosts, the other joins; both already
   possible. Reuses the OSC path. Accuracy is whatever WiFi gives, which is worse
   than a wire and probably fine at a sixteenth.
3. **Link**, which solves this and Ableton together.

**Order matters: a shared clock without a shared surface is a duet; a shared
surface without a shared clock is a networked text editor.** Do the clock first.

### Ableton Link, honestly `[OPEN]`

What it is: UDP multicast on `224.76.78.75:20808` for peer discovery, unicast
ping/pong to estimate each peer's clock offset, and a shared timeline of tempo,
beat origin and time origin, plus start/stop state. Any peer may change the tempo;
there is no master.

Three things have to be decided before a line is written, and two of them are not
engineering:

1. **Licence.** Link is dual-licensed: **GPLv2+, or a commercial licence from
   Ableton.** Taking the GPL means this firmware becomes GPL. That is a decision
   about the project, not about the clock, and it is the first gate.
2. **C++ and a socket shim.** The reference implementation is C++11 and expects
   asio. ESP-IDF can build C++ and has lwIP, so it is a port rather than a
   rewrite — but it is a port, and this firmware is C throughout.
3. **The measurement path is the hard part.** Tempo sync is easy; *phase* sync
   needs the offset estimation to be right, and getting it subtly wrong gives two
   decks that agree on tempo and drift on the bar — which is worse than no sync at
   all, because it looks like it is working.

**Recommendation.** Do (1) and (2) of *deck to deck* first — the MIDI input, then
the UDP beat packet — because they are small, testable with hardware in hand, and
the UDP work is the same socket and the same timeline model Link needs. Then decide
the licence question deliberately. A half-ported Link that agrees on tempo and
drifts on phase is the worst available outcome, and it is the one that arrives by
starting with Link.

**Not implemented. Not stubbed.** There is no Link code in this repo and nothing
here should be read as saying otherwise.

---

## The ensemble: many decks, one clock `[FACT]`

Implemented and measured. `>sync lead` on one deck, `>sync follow` on the rest,
`>sync alone` to leave. **No router, no password, no association** — which is the
point: a deck that had to be told a password to play with the deck beside it is a
deck nobody plays with.

### Why ESP-NOW and not ESP-MESH

ESP-MESH builds a routing tree and forwards hop by hop, so latency grows with
depth and each deck's timing depends on where it happens to sit in the tree.
ESP-NOW is connectionless: a packet goes out once and every deck in range hears it
directly, and unicast replies need no association. For a room of people playing
together that is the right shape, and it needs no infrastructure at all.

### What JackTrip and Link actually do, and what was taken

- **JackTrip** does not synchronise clocks. It streams audio over UDP into a
  jitter buffer and absorbs variance with depth. The transferable idea is
  "absorb the jitter, do not chase it" — which here is the slow correction loop.
- **Ableton Link**, and NTP and PTP before it, does a **ping-pong and keeps the
  exchange with the smallest round-trip time.** Minimum RTT means minimum queueing
  in both directions, so that exchange's offset estimate is the one least
  corrupted by delay.

The distinction that cost a wrong turn: **the minimum is over round-trip time, not
over the offset.** Taking the minimum *offset* is biased — with symmetric noise it
systematically undershoots — and the measurements below show it happening.

### The protocol

```
    leader  -> everyone   BEACON  tick, tempo, how late this send is
    follower -> leader    PROBE   its own send time t1
    leader  -> follower   REPLY   echoes t1, plus how long it held the probe
```

The follower knows the whole round trip **in its own clock** — no shared absolute
time anywhere. Subtracting the leader's holding time (PTP calls it residence time)
means the leader can answer from its main loop instead of from a radio callback,
which is both safer and free.

Tempo is followed **immediately**; phase is corrected **slowly and only from clean
exchanges**. A tempo is a decision somebody made; a phase is a measurement.

### Measured, on two decks on a bench

| method | mean | spread | worst | under 500 µs |
|---|---|---|---|---|
| one-way broadcast | −248 µs | 3340 µs | 1765 µs | — |
| minimum *offset* filter | — | — | biased −1 to −2 ms | — |
| round trip, no gate | +384 µs | 2491 µs | 2031 µs | 6 of 8 |
| round trip + hard RTT gate | +36 µs | 227 µs | **184 µs** | 10 of 10 |
| round trip + graduated trust, across two tempo changes | +334 µs | 3446 µs | 2322 µs | 8 of 14 |

The fourth row is the best result seen and the fifth is the honest one: the hard
gate measured beautifully in a quiet moment and then **starved**. The floor is the
fastest exchange ever seen, so one lucky probe sets a standard nothing else meets,
almost every window is discarded, and the clock coasts and drifts between the rare
accepted ones — which turned a 184 µs worst case into 2400 µs. Graduated trust
(half gain for a clean window, an eighth for a middling one, discard only a
hopeless one) never starves and is what ships.

**So: typically a few hundred microseconds, with occasional excursions to about
2 ms.** A pulse at 124 bpm is 5040 µs, so the worst case is under half a pulse and
the typical case is a few per cent of one. Well below the ~10 ms at which a
rhythmic difference is heard.

### Where the remaining variance comes from `[OPEN]`

The receive timestamp is taken in the ESP-NOW callback, which runs on the WiFi
task, so it carries that task's scheduling jitter. **ESP-NOW exposes no hardware
receive timestamp**, so this is close to the floor for this transport rather than a
bug to be fixed. Getting to tens of microseconds would need timestamping at the
radio, which means a different transport — or the wire.

Two things that came out of measuring rather than reasoning, both fixed:

- **A tempo change restarted the bar.** `seq_bpm()` set `s_tick = 0`, so `>bpm 140`
  mid-performance jumped every lane back to step one. Audible, nothing asked for
  it, and it was the single worst phase outlier in the ensemble because a follower
  took a bar reset with every tempo it followed. The grid re-anchors now and the
  position carries on.
- **Delivery is not symmetric.** Leader-to-follower measured 86 per cent of
  expected packets; follower-to-leader about 21, because the leading deck is
  transmitting eight times a second and misses receives while it does. The peer
  count flickered between none and one until the follower's probe rate went up and
  the staleness window widened.

### Ableton Link is still not implemented `[OPEN]`

Nothing here is Link, and nothing is stubbed. What this does share is Link's
*model* — a local timer corrected slowly from round-trip measurements — so if the
licence question (GPLv2+ or commercial from Ableton) is ever settled, Link replaces
the transport underneath `seq_timebase()` and `seq_nudge_by()` and nothing above
them changes.

