#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0

"""Draws the Library's covers (resources/img/covers/<genre>_<n>.jpg): for
each of the demo's genres (demo/include/view/cover_art.hpp) a few pictures
in the way of key art, scenes of tools/cover_scenes.py in the genre's colours,
rendered as SVG in Chromium. The demo draws a game's cover from one of its
genre's, picked by the game's seed, and writes its title over it
(demo/src/view/cover_art.cpp). Made here and committed, so that no build
needs Chromium; the output is the same on every run.

    python3 tools/make_covers.py resources/img/covers

Needs Playwright (with its Chromium, or CHROMIUM=<path> for another) and
Pillow.
"""

import base64
import io
import os
import random
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cover_scenes as scenes  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
FONT = ROOT / "resources" / "font" / "sora_bold_latin.ttf"
SIZE = (300, 400)   # the game page's cover, 270 × 360 at its largest, a little to spare
QUALITY = 74        # JPEG

# Each genre, in the order of library::Genre: its file name, its hue
# (cover_art.cpp's HUES, from 0 to 1) and its scenes, one a cover. How many
# each has, cover_art.cpp's COVERS says too.
GENRES = [
    ("adventure", 150 / 360, ["sky", "summit", "desert", "dungeon"]),
    ("racing", 320 / 360, ["kart", "canyon", "snow"]),
    ("puzzle", 45 / 360, ["garden", "paper", "farm", "caves"]),
    ("platformer", 200 / 360, ["critters", "party", "space", "sky"]),
    ("strategy", 10 / 360, ["tactics", "dungeon", "paper", "village"]),
    ("rhythm", 280 / 360, ["party", "ink", "arena"]),
    ("space", 230 / 360, ["space", "kitchen", "arena"]),
    ("ocean", 190 / 360, ["lighthouse", "village", "sky"]),
]

PAGE = """<!doctype html><html><head><meta charset="utf-8"><style>
@font-face{{font-family:Sora;src:url(data:font/ttf;base64,{font}) format("truetype");font-weight:700}}
html,body{{margin:0;padding:0;background:#000;overflow:hidden}}
.stage{{position:relative;width:{w}px;height:{h}px;overflow:hidden}}
.stage>svg{{position:absolute;inset:0;width:100%;height:100%}}
</style></head><body><div class="stage">{svg}</div></body></html>"""


def palette(hue, n):
    """The cover's colours: the genre's hue, moved a little for each, and an accent beside it."""
    rnd = random.Random(f"palette/{hue:.4f}/{n}")
    a = (hue + rnd.uniform(-0.06, 0.06)) % 1.0
    b = (a + rnd.choice((-1, 1)) * rnd.uniform(0.08, 0.2)) % 1.0
    return scenes.Palette((a, b))


def main():
    if len(sys.argv) != 2:
        sys.exit("usage: make_covers.py <resources/img/covers>")
    out = Path(sys.argv[1])
    out.mkdir(parents=True, exist_ok=True)
    font = base64.b64encode(FONT.read_bytes()).decode()
    w, h = SIZE[0] * 2, SIZE[1] * 2   # drawn larger, then made smaller

    from playwright.sync_api import sync_playwright
    with sync_playwright() as pw:
        browser = pw.chromium.launch(executable_path=os.environ.get("CHROMIUM") or None)
        tab = browser.new_page(viewport={"width": w, "height": h}, device_scale_factor=1)
        total = 0
        for name, hue, arts in GENRES:
            for n, art in enumerate(arts):
                p = palette(hue, n)
                rnd = random.Random(f"{name}/{n}")
                picture = scenes.svg(art, p, rnd, n, "portrait")
                tab.set_content(PAGE.format(font=font, w=w, h=h, svg=picture), wait_until="load")
                tab.evaluate("document.fonts.ready")
                shot = Image.open(io.BytesIO(tab.screenshot(clip={"x": 0, "y": 0, "width": w, "height": h})))
                path = out / f"{name}_{n}.jpg"
                shot.convert("RGB").resize(SIZE, Image.LANCZOS).save(path, "JPEG", quality=QUALITY, optimize=True,
                                                                    progressive=False)
                total += path.stat().st_size
        browser.close()
    count = sum(len(arts) for _, _, arts in GENRES)
    print(f"make_covers: {count} covers, {total // 1024} KB, in {out}")


if __name__ == "__main__":
    main()
