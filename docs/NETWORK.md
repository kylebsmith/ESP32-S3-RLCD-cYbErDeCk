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

**Measured since, 2026-09-25, in this firmware** (the heartbeat's free internal
heap): about **98 KB free before Wi-Fi, 15-16.5 KB with the station up, 14.2 KB
with the access point up**. The radio costs about 82 KB of internal RAM here, and
what is left is the budget an SSH session's crypto has to fit in - see *SSH, as
built* below.

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

## SSH, as built — what was wrong, what was decided `[BUILT]` `[OPEN]`

*2026-09-25, docs/NEXT.md §10: "test it, and fix the defect it has". The defect the
brief named was real, and there were three more.*

**1. The password was a word of the line.** `>ssh user@host pass ls` put it in a
document line, and a document is journalled, mirrored to the SD card and copied to
the owner's DGX - ground rule 6. `>wifi <ssid> <pass>` and `>host deck 12345678`,
both taught by the guide, did the same. **Now no command takes a password on its
line.** A command that needs one asks on the status line; the keys go to a buffer
shown as stars (`firmware/main/ask.h`), are handed to the command on Enter and wiped
on Enter and on Esc, and the document never sees them. `wifi` and `host` take one
word, and a second word is the old password: refused, and cut from the line before
autosave can keep it. `ssh` sends nothing when the answer appears in its command.
Checked on the deck: `>wifi TESTNET notreal-1` left `>wifi TESTNET` and a 20-byte
save; a passphrase typed at the prompt reached the radio and not the document.

**2. The host key was shown, not checked. Decision: keep it on first use, refuse a
change before any password is sent.** With password authentication an unchecked
key is not a cosmetic gap: whoever answers in the host's place is handed the
password - encrypted, but to them. Keeping the first key (in NVS, never a document)
is what ssh(1) does for a new host, and the alternative - typing a 43-character
fingerprint into the deck before the first connection - is a ritual nobody would
perform. The first connection is still taken on trust, so the deck prints the
fingerprint exactly as `ssh-keygen -lf` prints it, with the key's type: libssh2's
mbedTLS backend cannot use ed25519 host keys (*the four things* above), so it is
the host's ECDSA or RSA key to compare. `>ssh forget <host>` is for a key the owner
changed. The format is checked against a real key's `ssh-keygen` output in
`tools/test_ask.c`; **the check against a live server is unverified**.

**3. The session ran on the editor's task,** which a task watchdog panics after 10
seconds without a turn, while `connect()` waited out twelve SYN retries. By reading
the code, `>ssh` to an address nobody answers would have rebooted the deck
mid-set; **that was not reproduced on hardware**. Now the session is its own task,
with its stack in PSRAM because the radio leaves about 16 KB internal, `connect()`
is bounded at 5 seconds, and what the session says comes back through a message
buffer to `+out`. Measured on two decks, one hosting a test network: an address
nobody answers says `no answer in 5 seconds` at 5.0 s while the editor keeps 194
turns a second; a closed port says `connection refused`.

**4. The reply went to `+ssh`, which nothing ever showed.** It goes to `+out` now,
shown when the session ends, and Ctrl-O comes back like any other command's output.

**Against a real server, the same day, everything up to the login.** The deck on the
owner's home network, and on the laptop an OpenSSH 9.9 server started unprivileged on
a spare port with a throwaway host key and a configuration under which no login can
succeed — it admits only a user the laptop does not have, with no PAM and no keys. The
deck sent a dummy password to be refused. Measured:

- **The handshake fits.** Key exchange completed about a second after Enter, with
  mbedTLS on internal RAM only and about 14 KB of it free. Internal RAM settled 300 to
  400 bytes lower after the first session of a boot — allocations made once — and did
  not move across the other five sessions of the two boots.
- **The fingerprint is `ssh-keygen`'s**, character for character, key type included
  (`ecdsa-sha2-nistp256`).
- **The key's life:** kept the first time; recognised the second; after the server's
  key was replaced, **refused before authentication** — the server logged a connection
  and a disconnect, and no user name and no password; and after `>ssh forget`, the new
  key kept. `>ssh forget` now also says so on a line.
- The dummy password reached the server and was refused, and the deck reported
  `auth failed` and closed the session.

**Still unverified: a login that succeeds, and a command's reply in `+out`.** That
needs a server that accepts a login, which the owner's own account is not to be used
for — a throwaway account, a container with its own sshd, or key authentication
(proposed below, not built).

**Proposed, not built: key authentication.** The deck makes its own key pair,
keeps the private half in NVS and shows the public half, which is not a secret; the
owner adds it to `authorized_keys` once, and no password is typed at all. That is
the right end state for "control Claude Code on my laptop from the deck", and it
needs a reachable sshd to test before it is worth building.

**Observed once:** a deck joining another deck's open network took 44 seconds to
get an address by DHCP after associating.

---

## OSC in, and the keyboard scan that deafened the radio `[BUILT]` `[MEASURED]` 2026-09-25

**OSC in is a lane source** ([NEXT.md](NEXT.md) §8, [MAP.md](MAP.md) §9.8):
`>osc in 9000` listens, and `/deck/<name>` sets the input called `<name>` —
`>knob1 = knob`, `>pad1 = pad` — whose routes follow it. The reader
(`osc_parse.h`) takes a message, a bundle, or messages end to end, which is what this
deck's own `>osc` sends; it checks every length against the datagram, because the
datagram is from anyone, and refuses a malformed one whole.

**The first test found something bigger than OSC.** Two decks, one hosting a test
network and listening, the other sending its lanes to it: in the first thirteen
seconds 119 messages arrived; then 137 in the next 38; then **5 in 23 seconds**,
while the sender's own count said 280 went out. Neither deck's heap moved and neither
link dropped. Both decks were **scanning for a keyboard** — neither had one — and the
scan used NimBLE's defaults, a 30 ms window every 30 ms: *all* of the radio's time,
for as long as no keyboard is connected, on the radio Wi-Fi shares.

With a 30 ms window every 160 ms, the same test delivered **280 of 280** in each of
two twenty-second windows. So the scan now takes the radio only when nothing else
needs it: full time while Wi-Fi and ESP-NOW are off, the 30-in-160 ms window while
either is on (`kbd_share_radio`). This is not only OSC — it is every use of the radio
while no keyboard is paired, and the 44 seconds a deck once took to get an address
from another deck's network (*SSH, as built*) may be the same thing; that is
unverified. **Also unverified:** how much longer a keyboard now takes to reconnect
while Wi-Fi is on — no keyboard was here to time it.

**From a laptop, on a home network** (the owner's, the same day): the deck joined it,
listened, and a script on the laptop (`tools/osc_send.py`) moved `knob1`. A phone's
fader values — 0, 0.25, 0.5, 1.0 — became 0, 32, 64 and 127 on the filter, a plain
93 stayed 93, and a pad press played a kick exactly one step after a hat. But
**sending to the filter moving took 62 to 265 ms**: the deck's Wi-Fi was in power
save, dozing between the router's beacons with a listen interval of 307 ms, and the
router held every message until it woke. Pings said the same — 78 ms on average,
301 at worst, against 6 ms to the router. The ensemble already turned power save off
for this reason; OSC in did not. **Now listening turns it off**, and the same test,
twenty moves from one process: **20 of 20 with the right value, 4 ms at best, 22 ms
median, 55 ms at worst** — pings 29 ms on average. What remains above the router's 6 ms
is unexplained; the keyboard's Bluetooth link shares the radio, and that is a guess.

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
| + true send time, median of the best six, snap on tempo | +1 µs | 130 µs | 86 µs | 22 of 22 |
| the same, second run | −37 µs | 643 µs | 547 µs | 21 of 22 |
| **+ send callbacks matched by queue, power save off** | **−12 µs** | **55 µs** | **35 µs** | **22 of 22** |
| **the same, while drawing six visual lanes with the split on** | **−8 µs** | **25 µs** | **25 µs** | **10 of 10** |

Rows four and five are the story of a wrong turn: the hard gate measured beautifully
in a quiet moment and then **starved**. The floor is the fastest exchange ever seen,
so one lucky probe sets a standard nothing else meets, almost every window is
discarded, and the clock coasts and drifts between the rare accepted ones — which
turned a 184 µs worst case into 2400 µs. Graduated trust never starves, and shipped,
and was still wrong by twenty times.

**Rows eight and nine are what ships.** Six changes got there; the first three below
were found by reasoning about the protocol and the last three only by measuring, and the
last three were worth more than the first three put together. In descending order
of what each was worth:

1. **The probe's departure is measured, not assumed.** `esp_now_send` only *queues* a
   frame; what follows — the WiFi task waking, CSMA backoff waiting for a quiet channel,
   the transmit — was landing inside the measured round trip, entirely on the outbound
   leg, while halving the total spread it across both. A one-sided delay in a sum that
   gets halved puts half of itself straight into the answer. The ESP-NOW **send
   callback** fires when the frame has actually been transmitted and acknowledged, so
   the packet now carries an opaque tag and the follower reads the real departure out of
   that callback. This is where the milliseconds were.
2. **Six samples instead of one.** Keeping the single fastest exchange is right in
   principle and throws away a window of twenty-four to keep one draw, whose offset
   still carries the full jitter of the one receive timestamp behind it. Keep the best
   six by round trip and correct toward their **median**: six lightly-queued exchanges
   are six near-independent measurements of the same offset, and a median cannot be
   dragged by an outlier that slipped past the filter. Their **disagreement** then
   replaces the RTT floor as the gate, which is strictly better — the floor judges this
   window by a memory of the luckiest packet ever seen, whereas six probes agreeing
   within 300 µs is corroboration about *now* and needs no history to interpret.
3. **A tempo change is a step, so take it in one.** With the above in place, every
   steady-state window landed inside 73 µs and the only readings over 300 µs in a whole
   run were the first one after the leader changed tempo. That is not the radio:
   `seq_bpm` re-anchors the grid so the current pulse keeps its place in absolute time,
   which is right, but the two decks re-anchor at the two different moments they each
   heard about the change, and the difference modulo the new pulse is the error.
   Converging on a step in halves takes several windows, so the follower now notices the
   tempo moved, discards the window in progress — half of it was measured against a grid
   that no longer exists — and applies the next corroborated window whole.

4. **The send callback belongs to a queue, not to the latest probe.** `on_sent` read
   "the most recently handed-out tag" on the reasoning that only one probe is ever
   outstanding. The callback can be dispatched *after the next probe has been queued*,
   and it then stamped that probe's slot with the previous probe's departure — an error
   of one whole probe interval, 50 ms. It showed as a round-trip floor of 13–26 ms,
   which is a number with no physical meaning for a 26-byte frame, and that is what
   gave it away. Send callbacks are delivered one per send and in order, so a four-deep
   queue matches them exactly.
5. **WiFi power save off.** A station defaults to `WIFI_PS_MIN_MODEM`: the radio sleeps
   between beacon intervals and wakes to check for traffic. For a browser that is free
   battery life; for a clock it quantises both ends of every measurement to tens of
   milliseconds. `esp_wifi_set_ps(WIFI_PS_NONE)` is one line and it is the single
   largest timing fix in the file. Every earlier explanation for the variance was real
   and worth fixing, and all of them together were smaller than this one.
6. **An unacknowledged unicast is dropped, not trusted.** Its callback timed the last
   retry rather than whichever attempt landed. Accepting those put milliseconds into the
   answer — which was tried, and measured, and reverted.

**Broadcasting the probes was tried and reverted, and the reason is worth keeping.** A
broadcast is never acknowledged and therefore never retried, so its send callback
describes the transmit that actually happened — exactly what fix 6 is working around.
It is also, on this hardware at this range, about 90 per cent packet loss: unicast
probes drew roughly seventeen replies a second out of twenty, broadcast probes drew one
and a half. **The retries were not overhead, they were the delivery.** A clock that
measures perfectly on one exchange in thirteen is worse than one that measures well on
seventeen in twenty, because the windows in between are spent coasting — which is the
same failure the hard RTT gate produced, arrived at by a third route.

One more fix was found along the way and was not about accuracy at all: folding the
error into ±half a pulse used a `while` loop after subtracting `(their tick − our tick)
× period`. That term is an exact multiple of the pulse, so it vanished under the fold
and never affected the answer — but two decks that started playing minutes apart differ
by a hundred thousand ticks, so the loop it fed ran a hundred thousand times **inside a
radio callback**. One modulo is the same answer in constant time.

**So: 32 of 32 samples inside 500 µs, worst 35 µs, across two tempo changes and with
one deck drawing hard.** A pulse at 124 bpm is 5040 µs, so this is under one per cent of
a pulse — and about 0.3 per cent of the ~10 ms at which a rhythmic difference is heard.

The honest caveat: this is a measurement of a room, and the room varies. During one
stretch of testing the acknowledgement rate on this pair collapsed to about ten per
cent for a couple of minutes and the worst sample reached 951 µs before recovering by
itself. The estimator handled it the way it is meant to — the probes stopped agreeing,
the gain dropped, and the local clock coasted at 4 µs until the air cleared — but a
congested band is a real condition and 500 µs is not guaranteed through one. What is
guaranteed is that a bad room degrades the phase and cannot degrade the clock.

### A correction: the ticks did not follow the grid `[MEASURED]` 2026-09-25

**Every figure above measures the two decks' grids** — where each deck says its
ticks belong — and they were true. But a follower's ticks never moved. The timer was
periodic, and every correction moved only the grid: `seq_nudge` slid `s_grid_t0`
toward the ensemble while the ticks went on firing at the timer's own phase, on the
follower's own crystal. `seq.h` described the follower "trimming its own period";
nothing did.

It was found through the report [NEXT.md](NEXT.md) §2 asked to have fixed — a
following deck's `>jitter` sd of 231 µs beside a histogram that put every tick inside
100 µs. The brief read that as the sd counting deliberate grid slides, and said to fix
the reporting and not the clock. The sd *was* counting the slides — because the ticks
never made them. A mean, added to the report, showed it. Leader and follower on the
bench, `>kick x...x...` on both:

| | before | after |
|---|---|---|
| follower's ticks against its own grid, three 30 s windows | mean **−281, −212, −146 µs**, sd 19–24 | mean **+29, +29, +29 µs**, sd 4 |
| the ensemble's figure for the two grids | off by 3 µs | off by −41 µs |
| leader's ticks against its grid | mean −2 µs | mean +28 µs |
| leader after `>bpm 130` | mean **+4821 µs** | mean +28 µs |

So the follower drifted 2 µs a second against the leader — crystal against crystal,
about 7 ms an hour — and started wherever its `>play` happened to fall, anywhere
within half a pulse by the arithmetic of the fold in `seq_nudge` (not measured). The audible phase between two decks was that, not 35 µs. And a
tempo change put a deck's ticks a whole pulse behind the grid it broadcast, because
`seq_bpm` anchored the next tick "due now" and restarted a periodic timer that fired it
a period later: a follower that had followed its grid would have landed a pulse off
the leader after every tempo change.

**Now each tick is armed at the grid's due time** (`seq_clock.h`): a grid that never
moves makes that the periodic timer it replaced, and a grid that moves takes the ticks
with it, a fraction of an error at a time. A new tempo re-anchors on the last tick
that fired, so the only interval that changes is the one the tempo change is. The
+28 µs on both decks is the timer's own dispatch latency — the same on each, so it
cancels between them. `tools/test_clock.c` runs the scheduling arithmetic through an
hour with a 2 ppm crystal: the periodic clock's grids agree within 8.3 µs while its
ticks end the hour 6.0 ms apart; ticks armed on the grid stay within 8.3 µs.

**The phase between the two decks' ticks is now derived, not observed:** the ensemble's
grid figure plus each deck's measured mean against its own grid, three measured terms.
Nothing outside the decks — no scope, no audio capture — has timed the two outputs
against each other. That is unverified.

The report changed too: `>jitter` prints the **mean**, and the histogram counts a
tick's distance from the first sample by size, early or late. It compared the signed
distance, so an early tick was "<.1" however early.

### And the count: two decks shared a pulse, not a step `[MEASURED]` 2026-09-25

The phase correction folds every disagreement into ±half a pulse — "a whole-pulse
disagreement is a different bar, not a phase error" — and then nothing dealt with the
different bar: `(void)ourtick;`. Each deck counts pulses from its own `>play`, and a
step is 24 of them, so two decks started by two players shared a tempo and a pulse
within tens of microseconds while their sixteenths fell wherever the second `>play`
landed. Measured with the count added to `>sync`: **3, 4 and 14 pulses apart** on three
joins — up to 71 ms — and by the Mac's own clock, stamping each deck's console as its
kicks arrived, **the kicks were 77 ms apart**. The ensemble had never played together;
it had played at the same speed.

**Now a follower takes the leader's count** (`ens_count.h`). Each reply says which
pulse the leader was on; the follower works out how far its own count is behind,
using only replies back inside 2 ms — a slow reply can put the count a pulse out —
and when three agree, its clock moves the count at the start of a tick, keeping that
tick's time. A counted lane then waits for its next downbeat, as it does after
`>play`. A follower that presses play while the leader is playing is silent until the
count arrives — a second at most, then it plays alone — so it joins on the leader's
step rather than sounding a downbeat of its own first.

| on the bench, `>kick x...x...` on both | before | after |
|---|---|---|
| three joins at different moments: `>sync` | 3, 4, 14 pulses apart | in the leader's count, all three |
| the kicks, by the Mac's clock (median) | +77 ms | 0, −5, −1 ms |
| follower plays first, then the leader starts | — | first kick at once; then 0 ms |
| the leader restarts under a playing follower | — | back to the top with it, +3 ms |

The Mac's figures carry the console's own delay, ±25 ms a kick, so they resolve steps
and fractions of steps, not microseconds; the microseconds are the section above.
`tools/test_clock.c` checks the count arithmetic exact across 35 cases with the pulse
up to half a pulse off, and that the phase alone cannot tell 14 pulses from 38.

### Does drawing move the clock? No `[FACT]`

The question this instrument rests on, so it is measured rather than argued. A deck
running six visual lanes that all fire every other step, with the preview split on:

| | bare | drawing six lanes |
|---|---|---|
| local clock, standard deviation | **4 µs** | **5 µs** |
| ticks later than 100 µs | 0 of 5919 | 2 of 10013 |
| ticks later than 250 µs | 0 | 0 |
| phase against the other deck, worst (the grids — see the correction above) | 35 µs | 25 µs |

Re-measured with the clock that arms each tick on the grid (2026-09-25), under a
heavier load — five picture lanes, `echo`, `move` and the view streaming to HDMI:
**sd 4 µs, spread 96 µs, 6175 of 6175 ticks inside 100 µs**.

The two-core split is doing its job: the frame is generated in the main loop and the
clock dispatches on the other core, so the drawing cannot reach it. The phase figure is
no worse loaded than bare, which was *not* true before fix 4 — a deck under load looked
like it degraded to 1181 µs, and that was the mis-stamped send callback being provoked
by a slower main loop rather than the drawing touching the clock.

### Where the remaining variance comes from `[OPEN]`

The receive timestamp is still taken in the ESP-NOW callback, which runs on the WiFi
task, so it carries that task's scheduling jitter. **ESP-NOW exposes no hardware
receive timestamp.** What changed is that this is no longer the largest term — the
transmit side was, and it was removable. The remaining jitter is absorbed rather than
chased: taking the median of six lightly-queued exchanges is the JackTrip lesson
applied to a clock instead of to audio, and it is why the outliers stopped reaching the
grid.

Tightening further means either refusing more windows, which is the mistake already
made once, or timestamping at the radio, which means a different transport — or the
wire. Neither is worth doing at 35 µs.

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

### Ableton Link: not in this push, and why `[DECIDED]` 2026-09-25

**Not integrated.** [NEXT.md](NEXT.md) §7 made the licence the gate and asked for the
decision to be written here if the answer was no. It is no, for now, and the reason
is the licence rather than the engineering: Link is GPLv2+ or commercial from
Ableton, this repository is MIT, and taking the GPL makes the whole firmware GPL.
That is a decision about the project that belongs to its owner, and this push did
not make it on their behalf.

**Nothing is lost by waiting**, which is what makes "not now" the right answer rather
than a postponement:

- **Between decks**, the ESP-NOW ensemble already does what Link would be used for:
  two decks' grids within 35 µs, 32 of 32 samples inside 500 µs across tempo
  changes, no router — measured above; and since 2026-09-25 the ticks follow the
  grid and a follower plays in the leader's count (*A correction* and *And the
  count*, above).
- **With a DAW**, MIDI clock already does it over USB: 49.600 clocks a second for a
  requested 124 bpm, measured at the host (docs/OS.md).
- **The seam is ready.** The ensemble shares Link's *model* — a local timer
  corrected slowly from round trips — so if the licence question is ever settled,
  Link replaces the transport under `seq_timebase()` and `seq_nudge_by()` and nothing
  above them changes. Nothing is stubbed to look like Link, and nothing should be.

**What would reopen it:** the owner choosing GPLv2+ for the firmware, or a
commercial licence from Ableton; or a performance that needs a phone or a laptop
app that speaks only Link, which neither ESP-NOW nor MIDI clock reaches.

