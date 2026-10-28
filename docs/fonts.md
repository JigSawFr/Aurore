# Fonts

Labels are drawn with nanovg's fontstash: a font for the text, and fallback fonts it looks a character up in when the text font has none (one level deep, in the order they were added).

## On the Switch

The system's shared fonts, mapped by `pl`: the standard one (Latin, Japanese), Simplified and Traditional Chinese, Korean, and the button symbols (`NintendoExt`). Nothing is bundled but the Material icons (`resources/material`) and Cadran's Sora. An app's build may leave out the desktop fonts below from its romfs (`font/aurore_text.ttf`, `font/aurore_buttons.ttf`), as the demo and the template do.

## Everywhere else

The `DesktopFontLoader` (desktop, Android, iOS, PS4, PS Vita, the web) loads, in this order:

| Font name | From | |
|---|---|---|
| `FONT_REGULAR` | the app's `font/font.ttf` (`FontLoader::USER_FONT_PATH`), else `font/aurore_text.ttf` | Inter 4.001, regular (SIL OFL 1.1): Latin, Greek, Cyrillic, their symbols |
| `FONT_SWITCH_ICONS` | the app's `font/icon.ttf` (`USER_ICON_PATH`), else `font/aurore_buttons.ttf` | PromptFont 1.15.0's buttons (SIL OFL 1.1) at the Switch's code points |
| `FONT_JAPANESE`, `FONT_CHINESE_SIMPLIFIED`, `FONT_CHINESE_TRADITIONAL`, `FONT_KOREAN_REGULAR` | the app's `font/cjk.ttc` (or `.ttf`, `.otf`), else the system's | see below |
| `FONT_EMOJI` | `USER_EMOJI_PATH`; with FreeType, the system's colour emoji font when it is empty | |
| `FONT_MATERIAL_ICONS` | `resources/material` | Material Icons (Apache 2.0) |

The buttons come before the other fallbacks: their code points are in the private use area, where a system font may have something else.

### The buttons

`aurore_buttons.ttf` has the glyphs at the code points of the Switch's shared font, so that a string written for the console, `aurore::Hint::getKeyIcon()` or `" OK"`, draws the same button everywhere:

| Code point | Button | Code point | Button |
|---|---|---|---|
| U+E0E0 | A | U+E0EF | + |
| U+E0E1 | B | U+E0F0 | − |
| U+E0E2 | X | U+E0F3 | power |
| U+E0E3 | Y | U+E0F4 | HOME |
| U+E0E4 | L | U+E0F5 | capture |
| U+E0E5 | R | U+E100 | stick |
| U+E0E6 | ZL | U+E101, U+E102 | left, right stick |
| U+E0E7 | ZR | U+E103 | stick pressed |
| U+E0E8, U+E0E9 | SL, SR | U+E104, U+E105 | left, right stick pressed |
| U+E0EA | the four buttons | U+E130 | brightness |
| U+E0EB | D-pad up | U+E13C | volume |
| U+E0EC | D-pad down | U+E150 | ! |
| U+E0ED | D-pad left | U+E152 | ? |
| U+E0EE | D-pad right | U+E153 | i |

The table is `BUTTONS` in `tools/make_fonts.py`, which makes both fonts from Inter's and PromptFont's releases:

```sh
python3 tools/make_fonts.py Inter[opsz,wght].ttf promptfont.ttf resources/font
```

Both have the vertical metrics of the Switch's fonts (ascender 0.88 em, descender 0.12 em): fontstash places the baseline from them, so a line sits where it does on the console. A code point not in the table falls through to the next fonts (the Material icons have some of the same code points): add it there, from PromptFont's glyphs, and make the font again.

### Chinese, Japanese and Korean

The text font has no han, kana or hangul, and a font that has them weighs megabytes: none is bundled. An app that ships one puts it in its resources as `font/cjk.ttc` (or `.ttf`, `.otf`): it is used for the four languages. Otherwise each language gets the system's font for it (`DesktopFontLoader::findSystemFont()`):

| System | Where |
|---|---|
| Linux | fontconfig's choice for `sans-serif:lang=ja` (`ko`, `zh-cn`, `zh-tw`), when it has the language's characters. fontconfig is opened at run time (`libfontconfig.so.1`): without it, Noto Sans CJK where distributions install it |
| macOS, iOS | CoreText: Hiragino Sans, Apple SD Gothic Neo, PingFang SC, PingFang TC |
| Windows | `Fonts\` of the system folder: Yu Gothic, Meiryo; Malgun Gothic; Microsoft YaHei; Microsoft JhengHei |
| Android | `/system/fonts/NotoSansCJK-Regular.ttc`, or Droid Sans Fallback |
| PS4, PS Vita, the web | none: the app's `font/cjk` only |

The face of a font collection (`.ttc`) is the language's own (Noto Sans CJK KR for Korean), and a file is read once whatever the number of languages it serves. The fallbacks are looked up in the order of the locale's language first (`Application::getLocale()` at start), then Japanese, Simplified Chinese, Traditional Chinese and Korean: a character the four share is drawn as the locale writes it.

On Linux, `fonts-noto-cjk` (Debian, Ubuntu), `noto-fonts-cjk` (Arch) or `google-noto-sans-cjk-fonts` (Fedora) gives the four; `fonts-wqy-microhei` is a lighter one.

## In tests

The test harness reads no system font unless `Options::systemFonts` is set: its pictures are the same on every machine, with the app's fonts only (`DesktopFontLoader::useSystemFonts`).

## Licences

`resources/THIRD_PARTY_NOTICES.md` lists every font and its licence; `resources/font/OFL-*.txt` are the fonts' licences. `tools/check_notices.py`, run by CI, fails on a font without an entry, or one that names a console maker.
