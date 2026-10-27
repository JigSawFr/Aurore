#!/usr/bin/env python3
# Copyright 2026 JigSawFr
# Licensed under the Apache License, Version 2.0.
#
# Draws the demo's list and grid pictures (resources/img/gallery: nine gems,
# 400 px, and their 100 px thumbnails) and the tiles of its image views
# (resources/img/tiles.png). Made here so that the demo ships no third-party
# picture.
#
#     python3 tools/make_demo_pictures.py resources/img
#
# Needs Pillow. The output is the same on every run.

import math
import random
import sys
from pathlib import Path

from PIL import Image, ImageDraw

SS = 4  # drawn this many times larger, then made smaller: smooth edges

# name: (colour, number of sides)
GEMS = {
    "amber": ((235, 150, 40), 6),
    "coral": ((240, 110, 100), 8),
    "jade": ((60, 170, 110), 5),
    "lapis": ((50, 90, 200), 6),
    "onyx": ((70, 70, 85), 8),
    "opal": ((150, 210, 225), 7),
    "pearl": ((225, 220, 210), 12),
    "ruby": ((200, 30, 70), 4),
    "topaz": ((245, 200, 70), 5),
}


def shade(colour, factor):
    return tuple(max(0, min(255, round(c * factor))) for c in colour) + (255,)


def gem(colour, sides, size=400):
    big = size * SS
    image = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    centre = big / 2
    outer = big * 0.42
    inner = outer * 0.55
    start = -math.pi / 2
    corners = [(centre + outer * math.cos(start + 2 * math.pi * i / sides),
                centre + outer * math.sin(start + 2 * math.pi * i / sides)) for i in range(sides)]
    table = [(centre + inner * math.cos(start + 2 * math.pi * i / sides),
              centre + inner * math.sin(start + 2 * math.pi * i / sides)) for i in range(sides)]
    # A shadow, the facets lit from the top left, the table on top
    draw.ellipse([centre - outer, centre + outer * 0.86, centre + outer, centre + outer * 1.02], fill=(0, 0, 0, 50))
    for i in range(sides):
        j = (i + 1) % sides
        middle = start + 2 * math.pi * (i + 0.5) / sides
        light = 0.75 + 0.45 * math.cos(middle - (-3 * math.pi / 4))
        draw.polygon([corners[i], corners[j], table[j], table[i]], fill=shade(colour, light))
    draw.polygon(table, fill=shade(colour, 1.15))
    draw.line(corners + [corners[0]], fill=shade(colour, 0.55), width=3 * SS)
    # A highlight
    hx, hy = centre - inner * 0.35, centre - inner * 0.45
    glint = Image.new("RGBA", image.size, (0, 0, 0, 0))
    ImageDraw.Draw(glint).ellipse([hx - inner * 0.18, hy - inner * 0.1, hx + inner * 0.18, hy + inner * 0.1],
                                  fill=(255, 255, 255, 120))
    image.alpha_composite(glint)
    return image.resize((size, size), Image.LANCZOS)


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
    (out / "gallery" / "thumbnails").mkdir(parents=True, exist_ok=True)
    for name, (colour, sides) in GEMS.items():
        picture = gem(colour, sides)
        picture.save(out / "gallery" / f"{name}.png", optimize=True)
        picture.resize((100, 100), Image.LANCZOS).save(out / "gallery" / "thumbnails" / f"{name}.png", optimize=True)
    tiles().save(out / "tiles.png", optimize=True)


if __name__ == "__main__":
    main()
