# Themes

Every colour Aurore draws with, the core views' and Cadran's, is a **theme key** (`brls/text`, `cadran/teal`…) with a value for the light variant and one for the dark variant. A **theme file** changes any of them, and the look changes at once: no restart, no screen built again. Aurore ships presets, an app lists the user's own files in a folder, and a theme can be three lines long:

```ini
name = Mint
inherit = dark
accent = #3ddc97
```

## The format

A theme file is text, `*.theme` by convention: `key = value` lines, `[sections]`, `#` comments (a `#` at the start of a line or after a space; `#3ddc97` right after `=` is a colour).

```ini
# Metadata: shown by a picker (Theme::listThemes)
name = Ocean
author = Aurore
version = 1
description = Deep blue, a sky-blue accent.

# What it builds on: light, dark, another file (next to this one, or
# "builtin:<id>" for a preset). Without it: dark.
inherit = dark

# The palette: a few colours, every key derived from them
background = #0a1a2f
surface = #10263f
text = #eaf2fb
accent = #3fa7ff
warning = #ffb000
danger = #ff6b6b

# A picture behind every screen, next to the file
background_image = night.jpg

# Another file's lines, here
@include shared.inc

[colors]            # any key, for every variant the file styles
brls/sidebar/background = #0d1b2a

[light]             # for the light variant only (palette or keys)
accent = #1f5fc4

[dark]              # for the dark variant only
cadran/glow = rgba(63, 167, 255, 0.15)

[metrics]           # style metrics (Application::getStyle)
brls/highlight/stroke_width = 6
```

- **Colours**: `#rgb`, `#rrggbb`, `#rrggbbaa`, `rgb(r, g, b)`, `rgba(r, g, b, a)` (a from 0 to 1).
- **Variants**: a file styles the variant it inherits (`inherit = light` or `dark`, dark by default), or both when it has `[light]` and `[dark]` sections; it then follows the system's mode (or the app's light/dark setting). The variant a file does not style stays as it is without a file.
- **`inherit`** with a file or a preset takes everything from it first (its colours, metrics, variants, background), then this file's lines over them. `@include other.inc` reads another file's lines at that place (its `name` and `inherit` are ignored). Both look next to the including file first, then in the presets; loops stop after 8 levels.
- **Later lines win**; palette values are derived first, then `[colors]`, then `[light]`/`[dark]`, so an explicit key always beats the derived one.
- **Errors never stop the file**: a line that cannot be read (a bad colour, an unknown section, a missing include) is skipped and reported, with its file and line number. An unknown key is kept (an app's own keys are fine) and reported as a warning, in case it is a typo.
- **Contrast**: after a file is read, the pairs the core and Cadran promise to keep readable are checked (`Theme::checkContrast`): `brls/text on brls/background reads badly: 3.1:1, under 4.5:1`. Text needs 4.5:1, the focus ring 3:1 (WCAG 1.4.3, 1.4.11). Warnings are logged, and listed in `ThemeInfo::warnings`.

### The palette

The palette keys (`accent`, `background`, `surface`, `text`, `warning`, `danger`) set every key that follows from them; the ones a file does not give keep the variant's own colours. Inks used as text are darkened or lightened until they read at 4.5:1 on the background and both surfaces.

| Palette key | Keys it sets |
| --- | --- |
| `background` | `brls/background`, `brls/clear`, `brls/sidebar/background` |
| `surface` | `cadran/surf`, `cadran/surf2` (5 % toward the text), `brls/highlight/background`, `brls/button/default_*_background` |
| `text` (and `background`) | `brls/text`, `cadran/text`, `cadran/text2` and `text3` (30 % and 45 % toward the background, at 4.5:1), `brls/text_disabled`, `brls/header/subtitle`, `brls/header/rectangle`, `brls/list/listItem_value_color`, every separator and line (`brls/sidebar/separator`, `brls/header/border`, `brls/applet_frame/separator`, `cadran/line`, `cadran/track`, `cadran/border`, `cadran/well`), button text and borders, `brls/slider/line_empty`, `brls/spinner/bar_color`, `cadran/key_bg`, `cadran/key_fg` |
| `accent` | `brls/accent` and the inks of the active and chosen things (`brls/sidebar/active_item`, `brls/button/highlight_*_text`, `cadran/teal`, `cadran/pill_ok_ink`), the fills (`brls/button/primary_*`, `brls/slider/line_filled`, `cadran/teal_fill`, `cadran/teal_soft`, `cadran/pill_ok`, `cadran/rail_active`), the focus ring (`brls/highlight/color1` at 3:1, `color2` lighter, `cadran/hl1`, `cadran/hl2`), `brls/click_pulse`, `cadran/glow`, `cadran/glow_strong`, and the state shades below |
| `warning` | `cadran/amber`, `cadran/amber_fill`, `cadran/amber_surf`, `cadran/amber_line` |
| `danger` | `cadran/coral`, `cadran/coral_fill`, `cadran/coral_line`, `cadran/coral_soft` |

The label on the fills (`cadran/navy`: a confirmation's, a primary button's) is the one of the text, the background, black or white that reads best on the accent and danger fills; a fill the file does not give is darkened or lightened until it reads at 4.5:1.

**State shades**: `brls/accent/hover`, `brls/accent/pressed`, `brls/accent/disabled` and `brls/accent/ink` (the text on the accent) exist in every theme, derived from `brls/accent` (from `accent` in a file), for an app's state styles: `pressed:backgroundColor="@theme/brls/accent/pressed"` (docs/styles.md).

## Every key

Without a theme file, each variant has the core's defaults, then Cadran's palettes over them (`cadran/ui/cadran_palette.hpp`, the single place they are defined) when Cadran is on (the default).

**Core** (`library/lib/core/theme.cpp`):

| Key | What |
| --- | --- |
| `brls/clear` | The window's colour, under everything |
| `brls/background` | Screens, dialogs and pickers |
| `brls/text`, `brls/text_disabled` | Labels, and what cannot be used now |
| `brls/accent`, `brls/accent/hover`, `…/pressed`, `…/disabled`, `…/ink` | The accent (a dialog's buttons) and its state shades |
| `brls/backdrop` | Behind a dialog or a picker |
| `brls/notification/background`, `brls/notification/text` | Toasts |
| `brls/click_pulse` | A press's flash |
| `brls/highlight/background`, `brls/highlight/color1`, `brls/highlight/color2`, `brls/highlight/shadow` | The focus: its fill, its ring pulsing between the two colours, its shadow |
| `brls/shadow` | A view's shadow (`shadowType`) |
| `brls/applet_frame/separator` | The header's and footer's lines |
| `brls/sidebar/background`, `brls/sidebar/active_item`, `brls/sidebar/separator` | The sidebar |
| `brls/header/border`, `brls/header/rectangle`, `brls/header/subtitle` | A list's header |
| `brls/button/{primary,default}_{enabled,disabled}_{background,text}`, `brls/button/highlight_{enabled,disabled}_text`, `brls/button/{enabled,disabled}_border_color` | Buttons, by style and state |
| `brls/list/listItem_value_color` | A cell's value |
| `brls/slider/pointer_color`, `…/pointer_border_color`, `…/line_filled`, `…/line_empty` | Sliders |
| `brls/spinner/bar_color` | The progress spinner |
| `brls/battery/level`, `brls/battery/charging` | The battery widget |

**Cadran** (`cadran/ui/cadran_palette.hpp`; it also sets the core keys above to its palettes):

| Key | What |
| --- | --- |
| `cadran/text`, `text2`, `text3` | Text, quieter text, the quietest (captions, notes) |
| `cadran/text_off` | What the app cannot do now (`ui::show_writable`) |
| `cadran/surf`, `surf2` | Cards, and the second surface (chips, code, icons' squares) |
| `cadran/line`, `cadran/border`, `cadran/track`, `cadran/well` | Hairlines, a button's border, a switch's track off, a bar's empty part |
| `cadran/teal`, `teal_fill`, `teal_soft` | The accent: as an ink, as a fill, faint |
| `cadran/amber`, `amber_fill`, `amber_surf`, `amber_line` | Warnings |
| `cadran/coral`, `coral_fill`, `coral_line`, `coral_soft` | Removals and failures |
| `cadran/blue`, `violet`, `pink` | A release note's alerts, a build's tags, the tiles' inks |
| `cadran/pill_ok`, `pill_ok_ink` | The header's pill |
| `cadran/rail_active`, `cadran/rail_shadow` | The rail's active pill, its shadow over the tab |
| `cadran/glow`, `glow_strong` | The screens' background light |
| `cadran/hl1`, `hl2` | The focus ring's two colours |
| `cadran/key_bg`, `key_fg` | A controller key's bubble |
| `cadran/navy` | The label on a fill |
| `cadran/knob`, `knob_line` | A switch's knob |
| `cadran/bar`, `bar_today` | A chart's bars |
| `cadran/avatar0` … `avatar3` | The accounts' avatars |
| `cadran/hold_fill` | A held button filling up |
| `cadran/brand/track`, `brand/fill`, `brand/glow` | The loading screen's bar, alike in both variants |
| `cadran/none` | Transparent |

Not keys, on purpose: the app's logo (`CadranIcon` "mark", the app's own drawing through `ui::hooks`' `mark`), the debug layer and the layout wireframe (`-v`), and a `Rectangle`'s default colour before the app sets one.

**Metrics** (`[metrics]`): any of `library/lib/core/style.cpp` and Cadran's (`cadran/radius/*`, `brls/tab_frame/sidebar_width`…). The focus highlight's look is there: `brls/highlight/stroke_width`, `corner_radius`, `shadow_width`, `shadow_offset`, `shadow_feather`, `shadow_opacity`. Metrics read when drawing (the highlight's) change at once; the ones read when a view is made (paddings, sizes) apply to the views made afterwards.

## Presets

Built into the library (`library/lib/core/theme_presets.cpp`), every app has them: `builtin:<id>`, listed by `Theme::builtinThemes()`, their names translated (`resources/i18n/*/themes.json`). Each passes the contrast check.

| Id | Variants | What |
| --- | --- | --- |
| `high-contrast-dark` | dark | White on black, a yellow focus ring, a thicker ring |
| `high-contrast-light` | light | Black on white, a deep blue focus ring, a thicker ring |
| `ocean` | dark | Deep blue, a sky-blue accent |
| `amethyst` | light and dark | Violet, following the system's mode |
| `sand` | light | Warm paper, a terracotta accent |

A preset is also a starting point: `inherit = builtin:ocean`, then the keys to change.

## The API

```cpp
#include <borealis.hpp>

// Use a file (or a preset), replacing the one in use; the screens take it at
// their next frame. `variant`: the one it styles (or the one in use when it
// styles both), for setThemeVariant.
brls::ThemeVariant variant;
if (brls::Theme::loadFile("sdmc:/config/myapp/themes/mint.theme", &variant))
    brls::Application::getPlatform()->setThemeVariant(variant);
brls::Theme::loadFile("builtin:high-contrast-dark");
brls::Theme::reset();                       // back to the look without a file

// A picker: metadata, a preview, errors and warnings per file, never a crash
for (const brls::ThemeInfo& info : brls::Theme::listThemes("sdmc:/config/myapp/themes"))
    if (info.ok())
        addRow(info.name, info.author, info.dark ? info.darkPreview : info.lightPreview, info.path);
std::vector<brls::ThemeInfo> presets = brls::Theme::builtinThemes();

brls::Theme::getCurrentPath();              // the file in use, "" without one
brls::Theme::styles(brls::ThemeVariant::LIGHT);   // whether it styles that variant
brls::Theme::inspect(path);                 // one file, read without using it
brls::Theme::checkContrast(variant);        // the pairs under their minimum
brls::Theme::addContrastCheck("myapp/ink", "brls/background", 4.5f);
```

`ThemeInfo` has `path`, `id`, `name`, `author`, `version`, `description`, `backgroundImage`, `light`/`dark` (the variants it styles), `lightPreview`/`darkPreview` (`background`, `surface`, `text`, `accent`, `highlight`), `errors`, `warnings`, `ok()` and `variant()`. `listThemes` takes a folder or a resource folder (`@res/themes`); a file's `@include`s are best named `*.inc`, so that they are not listed.

### Colours that follow the theme

A view drawn from a theme key follows every change. What the core and Cadran draw does; an app's own views do the same in one of three ways:

- **XML**: `textColor="@theme/brls/text"`, `backgroundColor="@theme/cadran/surf"` (any colour property of the view: docs/styles.md).
- **Code**: `label->setThemeColor("textColor", "cadran/text2")`, `box->setThemeColor("backgroundColor", "cadran/surf")`, instead of `setTextColor(theme["cadran/text2"])`, which keeps the colour of the moment. A colour set afterwards by value (`setTextColor(…)`) replaces the key; `setThemeColor(property, "")` forgets it.
- **Drawing**: in `draw()`, `ctx->theme["cadran/teal"]` is always the theme in use.

What keeps colours another way (a cached `NVGcolor`, an image per variant) refreshes them in `View::onThemeChanged()` (call the base; a `Box` goes on to its children), or listens to `Application::getThemeChangedEvent()`, fired once a frame when the variant, the file or a colour changed, after every view of the screens took the change.

## A theme picker in an app

With Cadran (PlayGuard, PlayShelf), everything is there:

1. Keep `ui::Prefs::theme_file` in the app's settings, next to `theme` ("" for the boards' palettes, `builtin:<id>`, or a path), and hand the prefs over with `ui::use_prefs()` as today.
2. Say where the user's own files are: `ui::hooks::get().themes_dir = [] { return std::string("sdmc:/config/myapp/themes"); };` (on the desktop, a folder next to the settings). The folder may not exist: the picker then offers the presets only.
3. That is all: `ui::pick_theme()` (the header's button, Preferences) ends with a **Palette** row opening `ui::pick_palette()`: Default, the presets, then the folder's readable files (a broken one is logged and left out). `ui::palette_name()` is the choice saved, for a tile. The choice is saved through `save_prefs` and applied at once (`live_prefs::apply_theme()`), a file that styles one variant only taking it.

Without Cadran, the same in a few lines: list `Theme::builtinThemes()` and `Theme::listThemes(folder)` in cells (the demo's Settings tab does it, `demo/src/tab/settings_tab.cpp`, with each theme's colours as swatches), call `Theme::loadFile(info.path, &variant)` and `setThemeVariant(variant)` on a click, save `info.path`, and load it again at start-up (after the app's own `addColor` calls).

Users write their files in the folder, starting from a preset (`inherit = builtin:ocean`) or from nothing; the demo's examples are in `resources/themes`. To check files without opening a screen: `aurore_demo --check-themes <folder>` prints each preset's and each file's errors and warnings and exits with 1 when there are some.
