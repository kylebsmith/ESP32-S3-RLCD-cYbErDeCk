#!/usr/bin/env python3
"""The deck's clock against the tempo on its screen, over minutes, at the host.

  python3 tools/clock_audit.py capture SECONDS OUT.json [--port NAME]
  python3 tools/clock_audit.py report OUT.json BPM [--loop BARS]

Every jitter number this project had before 2026-09-29 was measured against the
deck's OWN grid - '>jitter' against the grid it schedules from, the host captures
against a grid fitted to the notes - and a grid fitted to the notes absorbs any
tempo error whole. That is how a deck playing 124.008 bpm, 64 parts per million
fast, passed every check while its loops walked away from a DAW at 124.

This measures something else. It records the deck's MIDI clock (turn it on:
'>sync on') and every note with CoreMIDI's own receive timestamps - rtmidi's
deltas, not the time a Python callback happened to run - and reports:

  tempo      the clock against the NOMINAL tempo: the error in parts per million
             and in milliseconds a minute, which is what an unsynced DAW hears
             as loops slipping. The crystal alone is a few ppm; more is a bug.
  stability  the Allan deviation of the clock at one beat, one bar, 4, 16 and 64
             bars: loop-to-loop wander, the thing a steady rms cannot show. A
             clean clock falls as 1/tau (the USB frame's white noise averaging
             out); wander flattens it or turns it up.
  bars       every bar's length against the nominal bar: the worst loop.
  loop       every note against the clock's grid, by its place in the loop and
             by how many notes share its tick - so a moment that is late only
             when the texture is thick shows up as the place it is late.

One thing in that last table is the host's, not the deck's: macOS stamps the
events of one USB packet about 0.09 ms apart, in order, so the 'at once' rows
rise by that much a note even when the deck sends a step whole (measured
2026-09-29: the same with the step in one transfer and in several). The deck's
own side of it is '>jitter'.
"""
import json
import math
import sys
import time


def capture(seconds, out, want='cyberdeck'):
    import rtmidi
    mi = rtmidi.MidiIn()
    names = mi.get_ports()
    idx = [i for i, n in enumerate(names) if want.lower() in n.lower()]
    if not idx:
        sys.exit('no MIDI port matching %r in %r' % (want, names))
    mi.open_port(idx[0])
    mi.ignore_types(sysex=True, timing=False, active_sense=True)
    ev = []
    t = [0.0]

    def cb(msg, data=None):
        bytes_, delta = msg
        t[0] += delta                     # CoreMIDI's receive time, accumulated
        ev.append((t[0], list(bytes_)))
    mi.set_callback(cb)
    t0 = time.time()
    while time.time() - t0 < seconds:
        time.sleep(0.5)
    mi.cancel_callback()
    mi.close_port()
    with open(out, 'w') as f:
        json.dump({'port': names[idx[0]], 'seconds': seconds, 'events': ev}, f)
    print('%d events in %.0f s from %s -> %s' % (len(ev), seconds, names[idx[0]], out))


def linfit(xs, ys):
    n = len(xs)
    mx, my = sum(xs) / n, sum(ys) / n
    sxx = sum((x - mx) ** 2 for x in xs)
    sxy = sum((x - mx) * (y - my) for x, y in zip(xs, ys))
    b = sxy / sxx if sxx else 0.0
    return my - b * mx, b


def adev(phase, tau0, m):
    """Overlapping Allan deviation from phase samples (seconds) at tau = m*tau0."""
    n = len(phase)
    if n < 2 * m + 1:
        return None
    s = 0.0
    k = 0
    for i in range(n - 2 * m):
        d = phase[i + 2 * m] - 2 * phase[i + m] + phase[i]
        s += d * d
        k += 1
    return math.sqrt(s / (2.0 * k * (m * tau0) ** 2))


def pct(v, p):
    v = sorted(v)
    return v[min(len(v) - 1, int(p / 100.0 * len(v)))]


def report(path, bpm, loop_bars=8):
    ev = json.load(open(path))['events']
    clk = [t for t, b in ev if b and b[0] == 0xF8]
    if len(clk) < 24 * 8:
        sys.exit('only %d clock pulses: is ">sync on" set, and the deck playing?' % len(clk))
    P = 60.0 / (bpm * 24.0)                      # one MIDI clock pulse, nominal
    # the pulses as a phase against the nominal grid from the first, counted
    # pulse to pulse: a slow drift past half a pulse must not be read as a
    # pulse gained (it was, the first time this ran, and made a sawtooth)
    idx = [0]
    for x, y in zip(clk, clk[1:]):
        idx.append(idx[-1] + max(1, round((y - x) / P)))
    e = [t - clk[0] - i * P for t, i in zip(clk, idx)]
    a, b = linfit(clk, e)
    ppm = -b * 1e6                                # e falling = deck running fast
    mins = (clk[-1] - clk[0]) / 60.0
    resid = [x - (a + b * t) for x, t in zip(e, clk)]
    print('=== %s: %d clock pulses over %.1f min, nominal %d bpm' % (path, len(clk), mins, bpm))
    print('tempo      %.4f bpm played: %+.1f ppm, %+.2f ms a minute against %d exactly'
          % (bpm * (1 + ppm / 1e6), ppm, ppm * 60e-3, bpm))
    print('           over this run the downbeat moved %+.2f ms off the nominal grid'
          % (-(e[-1] - e[0]) * 1e3))
    ar = [abs(r) for r in resid]
    print('jitter     about the deck\'s own tempo: rms %.3f ms, p99 %.3f, p99.9 %.3f, worst %.3f ms'
          % (math.sqrt(sum(r * r for r in resid) / len(resid)) * 1e3,
             pct(ar, 99) * 1e3, pct(ar, 99.9) * 1e3, max(ar) * 1e3))
    # Allan deviation at a beat, a bar, 4, 16, 64 bars
    phase = [t - clk[0] for t in clk]
    phase = [p - i * P for p, i in zip(phase, idx)]
    out = []
    for label, m in (('beat', 24), ('bar', 96), ('4 bars', 384), ('16 bars', 1536), ('64 bars', 6144)):
        d = adev(phase, P, m)
        if d is not None:
            out.append('%s %.2g' % (label, d))
    print('stability  Allan deviation (fractional): ' + ', '.join(out))
    # every bar's length against the nominal bar
    bar = 96
    starts = [clk[k] for k in range(0, len(clk), bar)]
    lens = [(y - x) - bar * P for x, y in zip(starts, starts[1:])]
    if lens:
        print('bars       %d bars: longest %+.3f ms, shortest %+.3f ms, sd %.3f ms off nominal'
              % (len(lens), max(lens) * 1e3, min(lens) * 1e3,
                 math.sqrt(sum(x * x for x in lens) / len(lens)) * 1e3))
    # notes against the clock's own grid (96ths from the pulse fit), by loop place
    notes = [(t, b) for t, b in ev if len(b) == 3 and (b[0] & 0xF0) == 0x90 and b[2] > 0]
    if notes:
        q = P / 4.0                               # one of the deck's 96 PPQN ticks
        by_tick = {}
        for t, b in notes:
            k = round((t - clk[0] - a) * (1 + ppm / 1e6) / q)
            by_tick.setdefault(k, []).append((t, b))
        rows = {}
        dens = {}
        for k, lst in by_tick.items():
            ideal = clk[0] + a + k * q / (1 + ppm / 1e6)
            for t, b in lst:
                r = t - ideal
                place = (k // 24) % (16 * loop_bars)  # the sixteenth in the loop
                rows.setdefault(place, []).append(r)
                dens.setdefault(len(lst), []).append(r)
        allr = [r for v in rows.values() for r in v]
        print('notes      %d note-ons against the clock grid: rms %.3f ms, worst %+.3f ms'
              % (len(allr), math.sqrt(sum(r * r for r in allr) / len(allr)) * 1e3,
                 max(allr, key=abs) * 1e3))
        print('by texture notes at one tick -> mean, worst (ms):')
        for n in sorted(dens):
            v = dens[n]
            print('           %2d at once: %5d notes, mean %+.3f, worst %+.3f'
                  % (n, len(v), sum(v) / len(v) * 1e3, max(v, key=abs) * 1e3))
        worst = sorted(rows.items(), key=lambda kv: -max(abs(x) for x in kv[1]))[:5]
        print('by place   the %d-bar loop\'s latest sixteenths (bar.step: mean, worst ms):' % loop_bars)
        for place, v in worst:
            print('           %d.%-2d  %d notes, mean %+.3f, worst %+.3f'
                  % (place // 16 + 1, place % 16 + 1, len(v), sum(v) / len(v) * 1e3,
                     max(v, key=abs) * 1e3))


if __name__ == '__main__':
    a = sys.argv[1:]
    if len(a) >= 3 and a[0] == 'capture':
        port = a[a.index('--port') + 1] if '--port' in a else 'cyberdeck'
        capture(float(a[1]), a[2], port)
    elif len(a) >= 3 and a[0] == 'report':
        loop = int(a[a.index('--loop') + 1]) if '--loop' in a else 8
        report(a[1], int(a[2]), loop)
    else:
        print(__doc__)
        sys.exit(2)
