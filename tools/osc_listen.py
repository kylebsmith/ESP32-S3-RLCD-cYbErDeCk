#!/usr/bin/env python3
"""
A reference OSC receiver for the deck. Zero dependencies.

WHY THIS IS IN THE REPOSITORY. The deck can send OSC, and until something
receives it that claim is untestable - "the visuals don't work" with nothing in
any log on either side is exactly the failure mode the OSC packing test was
written to prevent, and this is the other half of it. Run this on the laptop,
point the deck at it, and the wire is proven end to end without a Pi existing
yet.

It is also the specification by example for whatever eventually renders: the
deck sends /deck/step with the bar position, /deck/<lane> with two ints, and
/deck/frame with a screenful of ASCII. That is the whole protocol.

    python3 tools/osc_listen.py            # listen on 0.0.0.0:9000
    python3 tools/osc_listen.py 9001

Then on the deck:
    >wifi <ssid>             (it asks for the password; or >host deck, and join it)
    >osc <this machine's ip> 9000
    >play
    >frame
"""
import socket
import struct
import sys
import time


def parse(buf):
    """Yield (address, args) for every message in a datagram."""
    i = 0
    while i < len(buf):
        j = buf.find(b"\0", i)
        if j < 0:
            return
        addr = buf[i:j].decode("ascii", "replace")
        i = j + (4 - (j - i) % 4) % 4 + 1 if False else (j + 1 + (4 - ((j + 1 - i) % 4)) % 4)
        if i >= len(buf):
            return
        j = buf.find(b"\0", i)
        if j < 0:
            return
        tags = buf[i:j].decode("ascii", "replace")
        i = j + 1 + (4 - ((j + 1 - i) % 4)) % 4
        args = []
        for t in tags.lstrip(","):
            if t == "i":
                if i + 4 > len(buf):
                    return
                args.append(struct.unpack_from(">i", buf, i)[0])
                i += 4
            elif t == "s":
                j = buf.find(b"\0", i)
                if j < 0:
                    return
                args.append(buf[i:j].decode("ascii", "replace"))
                i = j + 1 + (4 - ((j + 1 - i) % 4)) % 4
            else:
                return
        yield addr, args


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 9000
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("0.0.0.0", port))
    print(f"listening on 0.0.0.0:{port} - point the deck here with '>osc'")
    last = None
    n = 0
    while True:
        data, who = s.recvfrom(2048)
        n += 1
        now = time.perf_counter()
        gap = "" if last is None else f"  +{(now - last) * 1000:6.1f} ms"
        last = now
        for addr, args in parse(data):
            if addr == "/deck/frame":
                print(f"\n--- frame from {who[0]} ---")
                print(args[0] if args else "")
                print("--- end ---")
            else:
                print(f"{addr:20} {args}{gap}")
                gap = ""


if __name__ == "__main__":
    main()
