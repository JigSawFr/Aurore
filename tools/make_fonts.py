#!/usr/bin/env python3
# Copyright 2026 JigSawFr
# Licensed under the Apache License, Version 2.0.
#
# Makes the two fonts the library draws with where the system gives none
# (every platform but the Switch, whose shared fonts are used):
#
#   aurore_text.ttf     the text: Inter 4.001 (SIL OFL 1.1), its regular
#                       weight, Latin, Greek and Cyrillic with their symbols
#   aurore_buttons.ttf  the controller buttons: glyphs of PromptFont 1.15.0
#                       (SIL OFL 1.1), at the code points of the Switch's
#                       shared button font (BUTTONS below), so that the
#                       library's and the apps' "" (A) and the others
#                       draw the same on the Switch and elsewhere
#
#     python3 tools/make_fonts.py Inter[opsz,wght].ttf promptfont.ttf resources/font
#
# Inter: https://github.com/google/fonts/tree/main/ofl/inter (the variable
# font). PromptFont: https://shinmera.com/promptfont, or the npm package
# promptfont@1.15.0. Needs fontTools. The output is the same on every run.
#
# Both have the Switch fonts' vertical metrics (ascender 0.88 em, descender
# 0.12 em): fontstash places the baseline from them, so text and buttons sit
# where they do on the console.

import sys
from pathlib import Path

from fontTools import subset
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.boundsPen import BoundsPen
from fontTools.pens.recordingPen import DecomposingRecordingPen
from fontTools.pens.transformPen import TransformPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont
from fontTools.varLib import instancer

ASCENT, DESCENT = 0.88, -0.12

# Latin, Greek, Cyrillic, their punctuation and the symbols a UI writes.
# No private use area: Inter has glyphs there, which would hide the buttons'.
TEXT_RANGES = ",".join([
    "U+0020-007E", "U+00A0-024F", "U+0250-02FF", "U+0300-036F", "U+0370-03FF", "U+0400-052F",
    "U+1E00-1EFF", "U+2000-206F", "U+20A0-20CF", "U+2100-214F", "U+2150-218F", "U+2190-21FF",
    "U+2200-22FF", "U+2300-23FF", "U+2460-24FF", "U+2500-25FF", "U+2600-27BF", "U+FB00-FB06",
])

# Switch shared font code point: (PromptFont code point, what it is).
BUTTONS = {
    0xE0E0: (0x21D3, "A"),
    0xE0E1: (0x21D2, "B"),
    0xE0E2: (0x21D0, "X"),
    0xE0E3: (0x21D1, "Y"),
    0xE0E4: (0x219C, "L"),
    0xE0E5: (0x219D, "R"),
    0xE0E6: (0x219A, "ZL"),
    0xE0E7: (0x219B, "ZR"),
    0xE0E8: (0x2203, "SL"),
    0xE0E9: (0x2204, "SR"),
    0xE0EA: (0x21A8, "the four buttons"),
    0xE0EB: (0x2200, "D-pad up"),
    0xE0EC: (0x2202, "D-pad down"),
    0xE0ED: (0x21FF, "D-pad left"),
    0xE0EE: (0x2201, "D-pad right"),
    0xE0EF: (0x21FE, "+"),
    0xE0F0: (0x21FD, "-"),
    0xE0F3: (0x23FB, "power"),
    0xE0F4: (0x21F9, "HOME"),
    0xE0F5: (0x21FA, "capture"),
    0xE100: (0x21CD, "stick"),
    0xE101: (0x21CB, "left stick"),
    0xE102: (0x21CC, "right stick"),
    0xE103: (0x21B8, "stick pressed"),
    0xE104: (0x21BA, "left stick pressed"),
    0xE105: (0x21BB, "right stick pressed"),
    0xE130: (0x1F506, "brightness"),
    0xE13C: (0x1F50A, "volume"),
    0xE150: (0x2757, "!"),
    0xE152: (0x2753, "?"),
    0xE153: (0x1F6C8, "i"),
}

BUTTON_EM = 1000
BUTTON_SIZE = 0.954  # the Switch font's round buttons, in em
BUTTON_MAX_WIDTH = 0.96
BUTTON_MAX_HEIGHT = 1.0
BUTTON_CENTRE = 0.38  # their middle, above the baseline


def names(fb, family, version, copyright_, description):
    fb.setupNameTable({
        "copyright": copyright_,
        "familyName": family,
        "styleName": "Regular",
        "uniqueFontIdentifier": f"{family.replace(' ', '')}-Regular;{version}",
        "fullName": f"{family} Regular",
        "version": f"Version {version}",
        "psName": f"{family.replace(' ', '')}-Regular",
        "description": description,
        "licenseDescription": "This Font Software is licensed under the SIL Open Font License, Version 1.1.",
        "licenseInfoURL": "https://openfontlicense.org",
    })


def make_text(source, out):
    font = TTFont(source)
    if "fvar" in font:
        axes = {a.axisTag: a.defaultValue for a in font["fvar"].axes}
        axes["wght"] = 400
        font = instancer.instantiateVariableFont(font, axes)

    options = subset.Options()
    options.layout_features = ["kern"]  # fontstash shapes nothing: kerning only
    options.hinting = False
    options.notdef_outline = True
    options.name_IDs = []
    options.glyph_names = False
    subsetter = subset.Subsetter(options)
    subsetter.populate(unicodes=subset.parse_unicodes(TEXT_RANGES))
    subsetter.subset(font)

    upm = font["head"].unitsPerEm
    ascent, descent = round(ASCENT * upm), round(DESCENT * upm)
    hhea, os2 = font["hhea"], font["OS/2"]
    hhea.ascent, hhea.descent, hhea.lineGap = ascent, descent, 0
    os2.sTypoAscender, os2.sTypoDescender, os2.sTypoLineGap = ascent, descent, 0
    os2.fsSelection |= 1 << 7  # USE_TYPO_METRICS

    name = font["name"]
    copyright_ = "Copyright 2016 The Inter Project Authors (https://github.com/rsms/inter)"
    name.names = []
    family = "Aurore Text"
    for name_id, text in {
        0: copyright_,
        1: family,
        2: "Regular",
        3: "AuroreText-Regular;4.001",
        4: f"{family} Regular",
        5: "Version 4.001",
        6: "AuroreText-Regular",
        10: "Inter 4.001, subset, with the vertical metrics of the Switch's font.",
        13: "This Font Software is licensed under the SIL Open Font License, Version 1.1.",
        14: "https://openfontlicense.org",
    }.items():
        name.setName(text, name_id, 3, 1, 0x409)
    font["head"].created = font["head"].modified = 0x0
    font.recalcTimestamp = False
    font.save(out)


def make_buttons(source, out):
    src = TTFont(source)
    cmap = src.getBestCmap()
    glyphs = src.getGlyphSet()
    scale_from = BUTTON_EM / src["head"].unitsPerEm

    # The round A button sets the scale for all: the size of the Switch's
    a = glyphs[cmap[0x21D3]]
    pen = BoundsPen(glyphs)
    a.draw(pen)
    base = BUTTON_SIZE * BUTTON_EM / ((pen.bounds[3] - pen.bounds[1]) * scale_from)

    order = [".notdef"]
    outlines = {".notdef": TTGlyphPen(None).glyph()}
    metrics = {".notdef": (BUTTON_EM, 0)}
    mapping = {}
    for code, (source_code, _) in sorted(BUTTONS.items()):
        glyph = glyphs[cmap[source_code]]
        pen = BoundsPen(glyphs)
        glyph.draw(pen)
        x0, y0, x1, y1 = (v * scale_from for v in pen.bounds)
        k = min(base, BUTTON_MAX_WIDTH * BUTTON_EM / (x1 - x0), BUTTON_MAX_HEIGHT * BUTTON_EM / (y1 - y0))
        dx = BUTTON_EM / 2 - k * (x0 + x1) / 2
        dy = BUTTON_CENTRE * BUTTON_EM - k * (y0 + y1) / 2
        k *= scale_from
        dx, dy = round(dx), round(dy)
        outline = DecomposingRecordingPen(glyphs)
        glyph.draw(outline)
        tt = TTGlyphPen(None)
        outline.replay(TransformPen(tt, (k, 0, 0, k, dx, dy)))
        name = f"uni{code:04X}"
        order.append(name)
        outlines[name] = tt.glyph()
        metrics[name] = (BUTTON_EM, 0)
        mapping[code] = name

    fb = FontBuilder(BUTTON_EM, isTTF=True)
    fb.setupGlyphOrder(order)
    fb.setupCharacterMap(mapping)
    fb.setupGlyf(outlines)
    for name, glyph in outlines.items():
        glyph.recalcBounds(fb.font["glyf"])
        metrics[name] = (BUTTON_EM, getattr(glyph, "xMin", 0))
    fb.setupHorizontalMetrics(metrics)
    ascent, descent = round(ASCENT * BUTTON_EM), round(DESCENT * BUTTON_EM)
    fb.setupHorizontalHeader(ascent=ascent, descent=descent)
    names(fb, "Aurore Buttons", "1.15.0",
          "Copyright 2018-2026, Yukari Hafner. Copyright 2011-2016, Severin Meyer.",
          "PromptFont 1.15.0's button glyphs at the code points of the Switch's shared font.")
    fb.setupOS2(version=4, sTypoAscender=ascent, sTypoDescender=descent, sTypoLineGap=0,
                usWinAscent=ascent, usWinDescent=-descent, fsSelection=0x40 | 1 << 7)
    fb.setupPost()
    fb.font["head"].created = fb.font["head"].modified = 0x0
    fb.font.recalcTimestamp = False
    fb.save(out)


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__ or "usage: make_fonts.py <Inter.ttf> <promptfont.ttf> <output folder>")
    out = Path(sys.argv[3])
    out.mkdir(parents=True, exist_ok=True)
    make_text(sys.argv[1], out / "aurore_text.ttf")
    make_buttons(sys.argv[2], out / "aurore_buttons.ttf")


if __name__ == "__main__":
    main()
