#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0

"""The demo's CJK font in a browser (web/README.md): the characters the
Japanese, Korean and Chinese catalogs use (resources/i18n/), cut out of Noto
Sans CJK (SIL Open Font License 1.1). A browser has no system font Aurore can
read, and a whole CJK font weighs megabytes: with this one, those languages
draw as they do on a console. The build packages it as font/cjk.otf
(-DAURORE_WEB_CJK_FONT=<out.otf>), which Aurore uses for the four
(docs/fonts.md).

Noto Sans CJK draws its han for one locale at a time: the Simplified Chinese
face is taken.

Usage: tools/web_cjk_font.py <NotoSansCJK-Regular.ttc> <out.otf> [repo-root]   (needs fontTools)
"""
import glob
import json
import os
import sys

from fontTools import subset
from fontTools.ttLib import TTCollection

LANGUAGES = ("ja", "ko", "zh-Hans", "zh-Hant")


def strings(node):
    if isinstance(node, dict):
        for v in node.values():
            yield from strings(v)
    elif isinstance(node, list):
        for v in node:
            yield from strings(v)
    elif isinstance(node, str):
        yield node


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    source, out = sys.argv[1], sys.argv[2]
    root = sys.argv[3] if len(sys.argv) > 3 else "."

    chars = set()
    for lang in LANGUAGES:
        for catalog in sorted(glob.glob(os.path.join(root, "resources", "i18n", lang, "*.json"))):
            with open(catalog, encoding="utf-8") as f:
                for s in strings(json.load(f)):
                    chars.update(c for c in s if ord(c) >= 0x2E80)   # CJK, kana, hangul and their punctuation
    # Every kana, for the names the catalogs do not spell (hiragana and katakana).
    chars.update(chr(c) for c in range(0x3040, 0x3100))

    collection = TTCollection(source)
    face = next((i for i, f in enumerate(collection.fonts)
                 if "SC" in f["name"].getDebugName(1) or "SC" in (f["name"].getDebugName(4) or "")), 0)
    options = subset.Options()
    options.font_number = face
    options.layout_features = ["*"]
    options.name_IDs = ["*"]
    options.notdef_outline = True
    font = subset.load_font(source, options)
    subsetter = subset.Subsetter(options)
    subsetter.populate(text="".join(sorted(chars)))
    subsetter.subset(font)
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    subset.save_font(font, out, options)
    print(f"{out}: {len(chars)} characters from {collection.fonts[face]['name'].getDebugName(4)}, "
          f"{os.path.getsize(out) // 1024} KiB")
    return 0


if __name__ == "__main__":
    sys.exit(main())
