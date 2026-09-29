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

THE DECK MOVES WHEN IT CHANGES MODE. After '>usb on' it comes back as a USB MIDI
device whose console is a CDC port with a new name. When the deck's port goes,
this waits for the deck to come back - at the port it was given, or at the port
whose USB serial number is the deck's own in that mode, 'deck-0001' - and
carries on there. Matching by the deck's serial number means it never picks up
another ESP32 on the same computer. The CDC console only reboots the deck on
esptool's DTR/RTS pattern, and this opens it with both low, so following the deck
there does not reset it.
"""
import argparse
import base64
import re
import sys
import time

import serial
from serial.tools import list_ports

DECK_USB_SERIAL = 'deck-0001'       # usbdev.c: the deck's serial number in USB MIDI mode


def open_deck(port):
    d = serial.Serial()
    d.port, d.baudrate, d.timeout = port, 115200, 0
    d.dtr = d.rts = False
    d.open()
    return d


def find_deck(given):
    """The port given, if it is there; else the deck's CDC console in USB MIDI mode."""
    ports = list(list_ports.comports())
    if any(p.device == given for p in ports):
        return given
    for p in ports:
        if p.serial_number == DECK_USB_SERIAL:
            return p.device
    return None

VIEW = re.compile(rb'\x1b\]view;([A-Za-z0-9+/=]+)\x07')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--deck', required=True, help="the deck's serial port")
    ap.add_argument('--view', required=True, help="the view node's serial port")
    a = ap.parse_args()

    deck = None                        # found and opened in the loop, and again after a move
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
        try:
            got = deck.read(8192) if deck is not None else b''
        except (serial.SerialException, OSError):
            print('-- deck gone (changing mode?); waiting for it to come back', flush=True)
            try:
                deck.close()
            except Exception:
                pass
            deck, got = None, b''
        if deck is None:
            port = find_deck(a.deck)
            if port is not None:
                try:
                    deck = open_deck(port)
                    buf = b''
                    print(f'-- deck on {port}', flush=True)
                except serial.SerialException:
                    deck = None
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
