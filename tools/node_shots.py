#!/usr/bin/env python3
"""The view node's screens for the docs, drawn by the node's own code.

  python3 tools/node_shots.py [OUT_DIR]      default docs/img

Builds tools/mock_frames.c (the deck's picture engine) and tools/node_sim.cpp
(view/deckview/deckview.ino on this computer), renders ORBITALS' scenes at
53 x 20 - the node's one-to-one grid - through every mode and a few settings
of the colour controllers, and writes view-modes.png (the sheet) and
view-motion.gif (the orbit turning, a sixteenth a frame, as the screen shows
it). Nothing here draws: the pixels are the node's.
"""
import os
import subprocess
import sys
import tempfile

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import deck_webfont   # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
U = '255'


def build(tmp):
    frames = os.path.join(tmp, 'mock_frames')
    subprocess.run(['cc', '-std=c11', '-O1', '-I', 'firmware/components/viz/include',
                    '-I', 'firmware/components/seq/include', '-I', 'tools/hostshim',
                    '-o', frames, 'tools/mock_frames.c', 'firmware/components/viz/viz.c'],
                   check=True, cwd=REPO)
    sim = os.path.join(tmp, 'node_sim')
    subprocess.run(['c++', '-std=c++17', '-O1', '-I', 'tools/nodesim', '-I', 'view/deckview',
                    '-o', sim, 'tools/node_sim.cpp'], check=True, cwd=REPO)
    cells = os.path.join(tmp, 'cells')
    os.makedirs(cells)
    subprocess.run([frames, cells, '53', '20', 'motion'], check=True, stdout=subprocess.DEVNULL)
    return sim, cells


def shot(sim, cells, scene, mode, out, par=()):
    subprocess.run([sim, cells, scene + '%02d-53x20.cells', mode, out] + list(par),
                   check=True, stdout=subprocess.DEVNULL)
    return Image.open(out).convert('RGB')


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, 'docs', 'img')
    tmp = tempfile.mkdtemp(prefix='node_shots_')
    sim, cells = build(tmp)
    tiles = [
        ('plain', 'orbit', 'plain', ()),
        ('scan', 'night', 'scan', ()),
        ('riso', 'orbit', 'riso', ()),
        ('poster', 'night', 'poster', ()),
        ('code', 'night', 'code', ()),
        ('plain, >day 9', 'orbit', 'plain', (U, U, U, '127', U, U, U, U)),
        ('plain, >ink 6, sat', 'night', 'plain', ('70', U, '127', U, U, '0', U, U)),
        ('plain, the grid', 'orbit', 'plain', (U, U, U, U, U, U, U, '60')),
    ]
    tw, th, cap = 640, 480, 36
    sheet = Image.new('RGB', (4 * tw + 5 * 24, 2 * (th + cap) + 3 * 24), (236, 235, 228))
    d = ImageDraw.Draw(sheet)
    deck_webfont.write(tmp)
    face = ImageFont.truetype(os.path.join(tmp, 'Deck.ttf'), 24)
    for i, (label, scene, mode, par) in enumerate(tiles):
        im = shot(sim, cells, scene, mode, os.path.join(tmp, 't%d.ppm' % i), par)
        x = 24 + (i % 4) * (tw + 24)
        y = 24 + (i // 4) * (th + cap + 24)
        sheet.paste(im, (x, y))
        d.text((x, y + th + 6), '>' + label, font=face, fill=(18, 18, 18))
    sheet.save(os.path.join(out, 'view-modes.png'))
    subprocess.run([sim, cells, 'orbit%02d-53x20.cells', 'plain', os.path.join(tmp, 'm%02d.ppm')],
                   check=True, stdout=subprocess.DEVNULL)
    frames = []
    k = 0
    while os.path.exists(os.path.join(tmp, 'm%02d.ppm' % k)):
        frames.append(Image.open(os.path.join(tmp, 'm%02d.ppm' % k)).convert('P', palette=Image.ADAPTIVE, colors=8))
        k += 1
    frames[0].save(os.path.join(out, 'view-motion.gif'), save_all=True, append_images=frames[1:],
                   duration=int(60000 / 124 / 4), loop=0)
    print('%s: view-modes.png, view-motion.gif (%d frames)' % (out, len(frames)))


if __name__ == '__main__':
    main()
