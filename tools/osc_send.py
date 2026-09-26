#!/usr/bin/env python3
"""Send one OSC message to the deck's inputs - for '>osc in' (docs/MAP.md §9.8).

  python3 tools/osc_send.py <deck-ip> 9000 /deck/knob1 0.5     a fader, 0 to 1
  python3 tools/osc_send.py <deck-ip> 9000 /deck/knob1 93      a value, 0 to 127
  python3 tools/osc_send.py <deck-ip> 9000 /deck/pad1          a press

A float is sent as a float and a whole number as an int, the way a phone and
another deck send them; the deck takes the last number as the value. No
dependencies, so it runs on any laptop that can reach the deck.
"""
import socket
import struct
import sys


def pad(b):
    return b + b'\0' * (4 - len(b) % 4)


def message(addr, args):
    tags, data = ',', b''
    for a in args:
        if '.' in a:
            tags += 'f'
            data += struct.pack('>f', float(a))
        else:
            tags += 'i'
            data += struct.pack('>i', int(a))
    return pad(addr.encode()) + pad(tags.encode()) + data


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        sys.exit(2)
    host, port, addr, args = sys.argv[1], int(sys.argv[2]), sys.argv[3], sys.argv[4:]
    socket.socket(socket.AF_INET, socket.SOCK_DGRAM).sendto(message(addr, args),
                                                           (host, port))


if __name__ == '__main__':
    main()
