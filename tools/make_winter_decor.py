#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0
#
# Draws the decorations of the demo's Winter theme (resources/themes/winter):
# icicles along the top of the screen, a snowdrift along its bottom, and a
# frost tile for the sidebar and the dialogs, each at night (the dark
# variant) and by day (the light one). Made here so that the demo ships no
# third-party picture.
#
#     python3 tools/make_winter_decor.py resources/themes/winter
#
# Needs Pillow and NumPy. The output is the same on every run (seeded).

import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

WIDTH = 1280      # the layout's width: a strip drawn one pixel per unit
SCALE = 3         # drawn larger, then made smaller: smooth edges

# Per variant: the ice's light and shadow, the snow's, the frost's
LOOKS = {
    "night": dict(ice=(214, 236, 255), ice_shadow=(120, 170, 220), ice_alpha=0.9,
                  snow=(236, 244, 252), snow_shadow=(150, 180, 214), frost=(200, 225, 255), frost_alpha=0.05),
    "day": dict(ice=(120, 170, 214), ice_shadow=(52, 104, 158), ice_alpha=0.75,
                snow=(255, 255, 255), snow_shadow=(170, 196, 222), frost=(42, 100, 150), frost_alpha=0.05),
}


def icicles(rng, look, height=34):
    """A rim of snow along the top, icicles of every length hanging from it."""
    w, h = WIDTH * SCALE, height * SCALE
    image = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    ice = look["ice"] + (int(255 * look["ice_alpha"]),)
    shadow = look["ice_shadow"] + (int(255 * look["ice_alpha"]),)
    x = 0.0
    while x < w:
        width = rng.uniform(7, 16) * SCALE
        # Mostly short, now and then a long one
        length = (rng.uniform(0.22, 0.45) if rng.random() < 0.8 else rng.uniform(0.6, 0.97)) * h
        left, right = x, x + width
        tip = x + width * rng.uniform(0.4, 0.6)
        draw.polygon([(left, 0), (right, 0), (tip, length)], fill=shadow)
        # The lit side, a little narrower
        draw.polygon([(left, 0), (left + width * 0.62, 0), (tip - width * 0.05, length * 0.96)], fill=ice)
        x += width * rng.uniform(0.8, 2.0)
    # The snow's rim they hang from, wavy
    rim = look["snow"] + (255,)
    xs = np.arange(0, w + 1, 4)
    ys = (5 + 2.2 * np.sin(xs / (61.0 * SCALE)) + 1.4 * np.sin(xs / (23.0 * SCALE) + 1.3)) * SCALE
    draw.polygon([(0, 0)] + list(zip(xs.tolist(), ys.tolist())) + [(w, 0)], fill=rim)
    image = image.filter(ImageFilter.GaussianBlur(SCALE * 0.35))
    return image.resize((WIDTH, height), Image.LANCZOS)


def drift(rng, look, height=22):
    """Soft mounds of snow, a shadow under each crest."""
    w, h = WIDTH * SCALE, height * SCALE
    xs = np.arange(w)
    crest = np.zeros(w)
    for _ in range(9):
        centre = rng.uniform(-0.1, 1.1) * w
        spread = rng.uniform(0.08, 0.2) * w
        crest += rng.uniform(0.25, 0.6) * np.exp(-((xs - centre) / spread) ** 2)
    crest = h * (1.0 - np.clip(0.12 + crest * 0.8, 0, 0.85))
    y = np.arange(h)[:, None]
    depth = (y - crest[None, :]) / h            # under the crest: snow
    inside = np.clip(depth * h / (1.0 * SCALE), 0, 1)
    shade = np.clip(1.0 - depth * 3.0, 0, 1) ** 2  # a cool shadow just under the crest
    snow = np.array(look["snow"], float)
    shadow = np.array(look["snow_shadow"], float)
    rgb = snow[None, None, :] * (1 - 0.35 * shade[..., None]) + shadow[None, None, :] * 0.35 * shade[..., None]
    alpha = inside * 255
    image = Image.fromarray(np.dstack([rgb, alpha]).clip(0, 255).astype(np.uint8), "RGBA")
    return image.resize((WIDTH, height), Image.LANCZOS)


def frost(rng, look, size=160):
    """A tile of faint six-armed crystals, seamless."""
    s = size * SCALE
    image = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    colour = look["frost"] + (int(255 * look["frost_alpha"] * 2.2),)
    for _ in range(5):
        cx, cy = rng.uniform(0, s), rng.uniform(0, s)
        r = rng.uniform(9, 22) * SCALE
        turn = rng.uniform(0, math.pi / 3)
        for ox in (-s, 0, s):          # drawn again across the edges: it tiles
            for oy in (-s, 0, s):
                x0, y0 = cx + ox, cy + oy
                for k in range(6):
                    a = turn + k * math.pi / 3
                    x1, y1 = x0 + r * math.cos(a), y0 + r * math.sin(a)
                    draw.line([(x0, y0), (x1, y1)], fill=colour, width=SCALE)
                    for f in (0.45, 0.7):  # the little branches
                        bx, by = x0 + r * f * math.cos(a), y0 + r * f * math.sin(a)
                        for side in (-1, 1):
                            b = a + side * math.pi / 4
                            draw.line([(bx, by), (bx + r * 0.25 * math.cos(b), by + r * 0.25 * math.sin(b))],
                                      fill=colour, width=SCALE)
    return image.resize((size, size), Image.LANCZOS)


def main():
    if len(sys.argv) != 2:
        print(__doc__ if __doc__ else "usage: make_winter_decor.py <folder>")
        sys.exit(2)
    folder = Path(sys.argv[1])
    folder.mkdir(parents=True, exist_ok=True)
    for name, look in LOOKS.items():
        rng = np.random.default_rng(7)
        icicles(rng, look).quantize(64, method=Image.FASTOCTREE).save(folder / f"icicles-{name}.png", optimize=True)
        drift(rng, look).save(folder / f"drift-{name}.png", optimize=True)
        frost(rng, look).save(folder / f"frost-{name}.png", optimize=True)


if __name__ == "__main__":
    main()
