#!/usr/bin/env python3
# Copyright 2026 JigSawFr
# Licensed under the Apache License, Version 2.0.
#
# Draws the backgrounds of the demo's Aurora theme (resources/themes/aurora):
# a northern-lights sky, at night (night.jpg) and at dawn (day.jpg). Made
# here so that the demo ships no third-party picture.
#
#     python3 tools/make_aurora_background.py resources/themes/aurora
#
# Needs Pillow and NumPy. The output is the same on every run (seeded).

import sys
from pathlib import Path

import numpy as np
from PIL import Image

WIDTH, HEIGHT = 1920, 1080


def smooth_noise(rng, size, scale):
    """1-D value noise: random values every `scale` pixels, eased between."""
    knots = rng.random(size // scale + 3)
    x = np.arange(size) / scale
    i = x.astype(int)
    t = x - i
    t = t * t * (3 - 2 * t)
    return knots[i] * (1 - t) + knots[i + 1] * t


def sky(top, middle, bottom):
    """A vertical gradient: top to middle (at 60 %), then to bottom."""
    y = np.linspace(0.0, 1.0, HEIGHT)
    channels = [np.interp(y, [0.0, 0.6, 1.0], [top[c], middle[c], bottom[c]]) for c in range(3)]
    column = np.stack(channels, axis=-1)[:, None, :]
    return np.repeat(column, WIDTH, axis=1)


def curtain(rng, base, amplitude, width, rise, colour, strength):
    """One band of light: bright along a wavy line, rays fading upward."""
    x = np.arange(WIDTH)
    phase = rng.random() * 6.28
    line = (base + amplitude * np.sin(x / WIDTH * 6.28 * 1.3 + phase)
            + amplitude * 0.45 * np.sin(x / WIDTH * 6.28 * 3.1 + phase * 1.7)) * HEIGHT
    rays = 0.55 + 0.45 * smooth_noise(rng, WIDTH, 9) * smooth_noise(rng, WIDTH, 37)
    glow = 0.6 + 0.4 * smooth_noise(rng, WIDTH, 260)
    y = np.arange(HEIGHT)[:, None]
    d = (y - line[None, :]) / HEIGHT
    # Sharp below the line, a long fade above it (the rays)
    below = np.exp(-((d / width) ** 2) * 3.0)
    above = np.exp(d / rise)
    shape = np.where(d > 0, below, above)
    intensity = shape * (rays * glow)[None, :] * strength
    return intensity[..., None] * np.array(colour, float)[None, None, :]


def stars(rng, image, count, brightness):
    ys = rng.integers(0, int(HEIGHT * 0.75), count)
    xs = rng.integers(0, WIDTH, count)
    values = rng.random(count) ** 3 * brightness
    for x, y, v in zip(xs, ys, values):
        image[y, x] += v
        if v > brightness * 0.5 and 0 < x < WIDTH - 1 and 0 < y < HEIGHT - 1:
            image[y - 1:y + 2, x] += v * 0.35
            image[y, x - 1:x + 2] += v * 0.35


def draw(night):
    rng = np.random.default_rng(7 if night else 11)
    if night:
        image = sky((6, 10, 26), (10, 22, 44), (14, 36, 52))
        stars(rng, image, 900, 170)
        bands = [
            (0.50, 0.07, 0.025, 0.16, (40, 230, 150), 0.85),
            (0.36, 0.05, 0.02, 0.12, (60, 190, 230), 0.45),
            (0.26, 0.04, 0.02, 0.10, (150, 90, 230), 0.40),
        ]
    else:
        image = sky((214, 228, 246), (236, 236, 246), (250, 236, 228))
        bands = [
            (0.50, 0.07, 0.025, 0.16, (-140, -20, -80), 0.85),
            (0.36, 0.05, 0.02, 0.12, (-120, -50, -10), 0.65),
            (0.26, 0.04, 0.02, 0.10, (-40, -120, -10), 0.60),
        ]
    for base, amplitude, width, rise, colour, strength in bands:
        image += curtain(rng, base, amplitude, width, rise, colour, strength)
    # A horizon: low hills, darker (or bluer at dawn)
    x = np.arange(WIDTH)
    hills = (0.86 + 0.03 * np.sin(x / WIDTH * 6.28 * 1.7 + 1.0) + 0.015 * np.sin(x / WIDTH * 6.28 * 5.3)) * HEIGHT
    ground = np.arange(HEIGHT)[:, None] > hills[None, :]
    image[ground] = (8, 14, 22) if night else (176, 190, 212)
    # A faint dither against banding
    image += rng.normal(0, 0.8, image.shape)
    return Image.fromarray(np.clip(image, 0, 255).astype(np.uint8), "RGB")


def main():
    out = Path(sys.argv[1] if len(sys.argv) > 1 else "resources/themes/aurora")
    out.mkdir(parents=True, exist_ok=True)
    for night, name in ((True, "night.jpg"), (False, "day.jpg")):
        path = out / name
        draw(night).save(path, quality=78, optimize=True, progressive=True)
        print(path, path.stat().st_size // 1024, "KB")


if __name__ == "__main__":
    main()
