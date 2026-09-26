#!/usr/bin/env python3
"""Relay the deck's picture to the HDMI view node, through this computer.

  python3 tools/viewrelay.py --deck /dev/cu.usbmodemDECK --view /dev/cu.usbmodemNODE

docs/VIEW.md. The deck is meant to drive the view node itself, over its own USB.
Until that link exists this stands in for the cable: it holds the deck's console,
prints it (so it is also a monitor - only one program may hold the port), lifts
out every frame the deck sends as an ESC ] view;<base64> BEL line, and writes the
frame's bytes to the node. It also reads the node's own reports and prints them,
because a USB serial write that nobody reads can block the node's loop.

With '>send view on' on the deck, the node should report frames and no refusals.
Opening the deck's port resets it, as every serial monitor does.
"""
import argparse
import base64
import re
import sys
import time

import serial

VIEW = re.compile(rb'\x1b\]view;([A-Za-z0-9+/=]+)\x07')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--deck', required=True, help="the deck's serial port")
    ap.add_argument('--view', required=True, help="the view node's serial port")
    a = ap.parse_args()

    deck = serial.Serial()
    deck.port, deck.baudrate, deck.timeout = a.deck, 115200, 0
    deck.dtr = deck.rts = False
    deck.open()
    node = None
    buf, nbuf = b'', b''
    frames = 0
    while True:
        if node is None:
            try:
                node = serial.Serial(a.view, 115200, timeout=0, write_timeout=0.2)
                print(f'-- view node on {a.view}', flush=True)
            except serial.SerialException:
                node = None
        got = deck.read(8192)
        if got:
            buf += got
            while b'\n' in buf:
                line, buf = buf.split(b'\n', 1)
                m = VIEW.search(line)
                if m:
                    frames += 1
                    if node is not None:
                        try:
                            node.write(base64.b64decode(m.group(1)))
                        except serial.SerialException as e:
                            print(f'-- view node lost: {e}', flush=True)
                            node = None
                    # The deck writes a frame in one piece, straight to its USB
                    # driver, so it can land inside another task's log line.
                    # Keep that text.
                    line = (line[:m.start()] + line[m.end():]).strip()
                    if not line:
                        continue
                sys.stdout.write(line.decode('utf-8', 'replace') + '\n')
                sys.stdout.flush()
        if node is not None:
            try:
                nbuf += node.read(4096)
            except serial.SerialException:
                node = None
            while b'\n' in nbuf:
                line, nbuf = nbuf.split(b'\n', 1)
                print('node: ' + line.decode('utf-8', 'replace').rstrip('\r'),
                      flush=True)
        if not got:
            time.sleep(0.005)


if __name__ == '__main__':
    main()
