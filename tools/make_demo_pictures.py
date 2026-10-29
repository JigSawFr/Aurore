#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0
#
# Draws the tiles of the demo's image views (resources/img/tiles.png). Made
# here so that the demo ships no third-party picture; the Library's covers
# are key art made by tools/make_covers.py (resources/img/covers).
#
#     python3 tools/make_demo_pictures.py resources/img
#
# Needs Pillow. The output is the same on every run.

import random
import sys
from pathlib import Path

from PIL import Image, ImageDraw

SS = 4  # drawn this many times larger, then made smaller: smooth edges

def tiles(size=128):
    # Overlapping round tiles in muted colours, row after row, from the top
    rng = random.Random(7)
    palette = [(150, 120, 120), (120, 140, 135), (175, 160, 150), (135, 125, 150), (160, 140, 110), (115, 120, 125)]
    big = size * SS
    image = Image.new("RGB", (big, big), (90, 90, 95))
    draw = ImageDraw.Draw(image)
    w = big / 6
    for row in range(-1, 14):
        y = row * w * 0.45
        offset = (row % 2) * w / 2
        for col in range(-1, 8):
            x = col * w + offset
            c = palette[rng.randrange(len(palette))]
            c = tuple(min(255, v + rng.randrange(-12, 13)) for v in c)
            draw.rounded_rectangle([x + SS, y, x + w - SS, y + w * 1.1], radius=w * 0.45, fill=c,
                                   outline=(70, 70, 75), width=SS)
    return image.resize((size, size), Image.LANCZOS)


def main():
    if len(sys.argv) != 2:
        sys.exit("usage: make_demo_pictures.py <resources/img>")
    out = Path(sys.argv[1])
    out.mkdir(parents=True, exist_ok=True)
    tiles().save(out / "tiles.png", optimize=True)


if __name__ == "__main__":
    main()
