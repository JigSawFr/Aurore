#!/usr/bin/env python3
# Copyright 2026 JigSawFr
# Licensed under the Apache License, Version 2.0.
#
# Draws the pictures of the demo's Prism theme (resources/themes/prism): its
# logo for each variant, its icons (two masks the theme tints, one drawn as
# is), its preview in a picker, and its picture for the demo's image slot.
# Made here so that the demo ships no third-party picture.
#
#     python3 tools/make_prism_pictures.py resources/themes/prism
#
# Needs Pillow. The output is the same on every run.

import math
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

SS = 4  # drawn this many times larger, then made smaller: smooth edges

SPECTRUM = [(255, 92, 120), (255, 170, 70), (250, 226, 90), (90, 220, 140), (70, 170, 255), (160, 110, 255)]


def canvas(width, height, colour=(0, 0, 0, 0)):
    image = Image.new("RGBA", (width * SS, height * SS), colour)
    return image, ImageDraw.Draw(image)


def done(image, width, height):
    return image.resize((width, height), Image.LANCZOS)


def s(points):
    return [(x * SS, y * SS) for x, y in points]


def prism(draw, cx, cy, size, outline, fill, width):
    """A triangle standing on its base, the light entering on its left."""
    h = size * math.sqrt(3) / 2
    top = (cx, cy - h * 0.6)
    left = (cx - size / 2, cy + h * 0.4)
    right = (cx + size / 2, cy + h * 0.4)
    draw.polygon(s([top, left, right]), fill=fill)
    draw.line(s([top, left, right, top, left]), fill=outline, width=int(width * SS), joint="curve")
    return top, left, right


def rays(draw, start, angle_from, angle_to, length, width):
    for i, colour in enumerate(SPECTRUM):
        a = math.radians(angle_from + (angle_to - angle_from) * i / (len(SPECTRUM) - 1))
        end = (start[0] + math.cos(a) * length, start[1] + math.sin(a) * length)
        draw.line(s([start, end]), fill=colour + (255,), width=int(width * SS))


def logo(dark):
    """The mark: a prism splitting a beam into the spectrum, on a rounded tile."""
    size = 256
    image, draw = canvas(size, size)
    tile = (24, 20, 40, 255) if dark else (250, 248, 255, 255)
    edge = (180, 140, 255, 255) if dark else (106, 63, 209, 255)
    draw.rounded_rectangle([0, 0, size * SS - 1, size * SS - 1], radius=56 * SS, fill=tile)
    beam = (240, 240, 255, 255) if dark else (60, 50, 90, 255)
    draw.line(s([(30, 150), (104, 128)]), fill=beam, width=9 * SS)
    top, left, right = prism(draw, 120, 140, 110, edge, (0, 0, 0, 0), 11)
    exit_point = (146, 126)
    rays(draw, exit_point, -14, 20, 96, 8)
    # The prism over the rays' start
    prism(draw, 120, 140, 110, edge, tile, 11)
    return done(image, size, size)


def gear_mask():
    """Settings: a cog, white on transparent (the theme tints it)."""
    size = 128
    image, draw = canvas(size, size)
    c, outer, inner, teeth = 64, 52, 38, 8
    points = []
    for i in range(teeth * 4):
        a = 2 * math.pi * i / (teeth * 4)
        r = outer if i % 4 in (0, 1) else inner
        points.append((c + math.cos(a) * r, c + math.sin(a) * r))
    draw.polygon(s(points), fill=(255, 255, 255, 255))
    draw.ellipse(s([(c - 16, c - 16), (c + 16, c + 16)]), fill=(0, 0, 0, 0))
    return done(image, size, size)


def info_mask():
    """Info: a hexagon, a cut gem, with an "i" cut out of it, white on transparent."""
    size = 128
    image, draw = canvas(size, size)
    points = [(64 + math.cos(math.radians(30 + 60 * k)) * 58, 64 + math.sin(math.radians(30 + 60 * k)) * 58) for k in range(6)]
    draw.polygon(s(points), fill=(255, 255, 255, 255))
    clear = (0, 0, 0, 0)
    draw.ellipse(s([(56, 30), (72, 46)]), fill=clear)
    draw.rounded_rectangle(s([(56, 54), (72, 98)]), radius=6 * SS, fill=clear)
    return done(image, size, size)


def gamepad_colour():
    """Gamepad: in colour, drawn as is (not tinted)."""
    size = 128
    image, draw = canvas(size, size)
    draw.rounded_rectangle(s([(6, 30), (122, 98)]), radius=34 * SS, fill=(124, 82, 230, 255))
    draw.rounded_rectangle(s([(10, 34), (118, 94)]), radius=30 * SS, outline=(190, 160, 255, 255), width=3 * SS)
    white = (250, 248, 255, 255)
    draw.rounded_rectangle(s([(26, 58), (54, 70)]), radius=3 * SS, fill=white)
    draw.rounded_rectangle(s([(34, 50), (46, 78)]), radius=3 * SS, fill=white)
    for (x, y), colour in zip([(88, 52), (100, 64), (88, 76), (76, 64)], SPECTRUM[:4]):
        draw.ellipse(s([(x - 7, y - 7), (x + 7, y + 7)]), fill=colour + (255,))
    return done(image, size, size)


def preview():
    """The picker's thumbnail: the theme's night, its prism and spectrum."""
    size = 128
    image, draw = canvas(size, size)
    for y in range(size * SS):
        t = y / (size * SS)
        colour = tuple(int(a + (b - a) * t) for a, b in zip((38, 26, 66), (16, 13, 28))) + (255,)
        draw.line([(0, y), (size * SS, y)], fill=colour)
    draw.line(s([(0, 82), (52, 70)]), fill=(240, 240, 255, 255), width=4 * SS)
    rays(draw, (72, 66), -18, 22, 70, 5)
    prism(draw, 62, 76, 52, (180, 140, 255, 255), (24, 20, 40, 255), 5)
    return done(image, size, size)


def about_art():
    """The demo's image slot (demo/about/art): a wide prism and its rays."""
    width, height = 480, 270
    image, draw = canvas(width, height)
    glow = Image.new("RGBA", image.size, (0, 0, 0, 0))
    gdraw = ImageDraw.Draw(glow)
    gdraw.ellipse(s([(160, 75), (400, 215)]), fill=(150, 100, 255, 90))
    glow = glow.filter(ImageFilter.GaussianBlur(28 * SS))
    image.alpha_composite(glow)
    draw = ImageDraw.Draw(image)
    draw.line(s([(10, 170), (170, 140)]), fill=(240, 240, 255, 255), width=8 * SS)
    rays(draw, (250, 128), -26, 26, 200, 9)  # a transparent edge all around
    prism(draw, 220, 150, 150, (190, 150, 255, 255), (30, 24, 52, 255), 10)
    return done(image, width, height)


def main():
    folder = Path(sys.argv[1] if len(sys.argv) > 1 else "resources/themes/prism")
    folder.mkdir(parents=True, exist_ok=True)
    (folder / "icons").mkdir(exist_ok=True)
    logo(True).save(folder / "logo-dark.png", optimize=True)
    logo(False).save(folder / "logo-light.png", optimize=True)
    gear_mask().save(folder / "icons" / "settings.png", optimize=True)
    info_mask().save(folder / "icons" / "info.png", optimize=True)
    gamepad_colour().save(folder / "icons" / "gamepad.png", optimize=True)
    preview().save(folder / "preview.png", optimize=True)
    about_art().save(folder / "about.png", optimize=True)


if __name__ == "__main__":
    main()
