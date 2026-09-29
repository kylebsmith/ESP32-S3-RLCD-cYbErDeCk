#!/usr/bin/env python3
"""Drive the view node with no deck: the engine's own frames, every mode.

  python3 tools/view_demo.py --view /dev/cu.usbmodemNODE            # all seven, in turn
  python3 tools/view_demo.py --view /dev/cu.usbmodemNODE --mode scan

docs/VIEW.md. The frames are the deck's own picture engine - viz.c, built on
this computer by tools/mock_pictures.py - at the deck's 80 x 30, playing ORBITALS'
night (the radar and >noise 1, whose sparkles are glyphs) and its orbit, a
sixteenth apart at 124 bpm, packed exactly as the deck packs them: a control
frame naming the mode (and, for the poster, its lines), then the picture. For
seeing the modes on a screen without the deck; the deck itself is what
firmware/main/view.c sends.

The packing here is a copy of firmware/main/view_wire.h, in Python. The copy
that matters is checked: tools/test_view_wire.c runs the deck's packer through
the node's reader.
"""
import argparse
import os
import struct
import subprocess
import sys
import tempfile
import threading
import time

import serial

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mock_pictures as M   # noqa: E402

MODES = ['plain', 'scan', 'phosphor', 'feedback', 'riso', 'poster', 'code']
ORB = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'pieces/orbitals.txt')
VERBS = {'send', 'bpm', 'scale', 'swing', 'play', 'stop', 'mute', 'solo', 'toggle',
         'clear', 'map', 'route'}
LANES = ['kick 9...8...9...8...', 'box 2...1...2...1...', 'disc:x 8876532111235678',
         'disc:y 568886531113', 'hat ..3...3...3...4.', 'bass .000.000.000.000']


def xor(b):
    s = 0
    for c in b:
        s ^= c
    return s


def picture(tick, w, h, cells):
    head = struct.pack('<IBB', tick, w, h) + bytes(cells)
    return b'DKV1' + head + bytes([xor(head)])


def control(tick, mode, lines):
    text = b''.join(bytes([f, t]) + s.encode()[:60] + b'\n' for s, f, t in lines)
    head = struct.pack('<IBBH', tick, mode, len(lines), len(text)) + text
    return b'DKC1' + head + bytes([xor(head)])


def poster_lines(step):
    out = [('ORBITALS', 0, 0), ('II first light', 0, 0), ('124 bpm  dmin', 0, 0)]
    for lane in LANES:
        name, pat = lane.split(' ')
        at = len(name) + 1 + step % len(pat)
        out.append((lane, at, at + 1))
    return out


def span(line, step):
    """The step a plain lane line is on - one character a step. The deck
    lights every kind; this is enough to see the code screen move."""
    if not line.startswith('>') or ' ' not in line:
        return 0, 0
    name, pat = line[1:].split(' ', 1)
    body = pat.split(' ')[0]
    if name in VERBS or '=' in pat or not body or any(c in body for c in '[]<>%_'):
        return 0, 0
    at = len(name) + 2 + step % len(body)
    return at, at + 1


def code_lines(step):
    """As the deck sends them: the document, tempo and key, then ten lines."""
    text = open(ORB).read().split('\n')
    i = next(n for n, l in enumerate(text) if l.startswith('-- II first light'))
    out = [('orbitals', 0, 0), ('124 bpm  dmin', 0, 0)]
    for l in text[max(0, i - 1):i + 9]:
        out.append((l, *span(l, step)))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--view', required=True, help="the view node's serial port")
    ap.add_argument('--mode', choices=MODES, help='one mode, instead of all in turn')
    ap.add_argument('--seconds', type=float, default=8, help='each mode, in turn')
    ap.add_argument('--for', dest='total', type=float, default=0, help='stop after this long')
    a = ap.parse_args()

    tmp = tempfile.mkdtemp(prefix='view_demo_')
    exe = M.build(tmp, False)                     # the deck's viz.c, as flashed
    d = os.path.join(tmp, 'm')
    os.makedirs(d)
    subprocess.run([exe, d, '80', '30', 'motion'], check=True)
    film = []
    for scene, n in (('night', 32), ('orbit', 48)):
        for k in range(n):
            film.append([c for row in M.load(d, f'{scene}{k:02d}', 80, 30) for c in row])

    node = serial.Serial(a.view, 115200, timeout=0.1)

    def listen():
        buf = b''
        while True:
            try:
                buf += node.read(256)
            except serial.SerialException:
                return
            while b'\n' in buf:
                line, buf = buf.split(b'\n', 1)
                print('node: ' + line.decode('utf-8', 'replace').strip(), flush=True)

    threading.Thread(target=listen, daemon=True).start()
    step, start = 0, time.time()
    period = 60.0 / 124 / 4
    try:
        while not a.total or time.time() - start < a.total:
            if a.mode:
                mode = MODES.index(a.mode)
            else:
                mode = int((time.time() - start) / a.seconds) % len(MODES)
            tick = step * 24
            lines = (poster_lines(step) if MODES[mode] == 'poster' else
                     code_lines(step) if MODES[mode] == 'code' else [])
            node.write(control(tick, mode, lines) + picture(tick, 80, 30, film[step % len(film)]))
            step += 1
            time.sleep(max(0.0, start + step * period - time.time()))
    except KeyboardInterrupt:
        pass


if __name__ == '__main__':
    main()
