# Cadran

Cadran is the look of [PlayGuard](https://github.com/JigSawFr/playguard) and [PlayShelf](https://github.com/JigSawFr/playshelf), now part of Aurore: navy and cool-white palettes, an icon rail for a sidebar, the app's mark and a status pill in the header, cards in place of lists, modals and action sheets, vector icons, figures in Sora, and the language, theme and text size changed while the app runs, in 15 languages.

It lives in `library/include/aurore/cadran/` and `library/lib/cadran/`, its strings in `resources/i18n/<language>/cadran.json`, its emoji in `resources/img/emoji/`, its font in `resources/font/`. It is **GPLv3** (`LICENSE.GPL`): an app built with it is GPLv3.

## Turning it on

There is nothing to do: `aurore::Application::init()` and `createWindow()` start it, and every screen takes its look the first time it takes the focus. A main screen (an `AppletFrame` holding a `TabFrame`) gets the rail and the header with the app's mark, the date and the language, theme and text size; every other screen its header and its list in cards; every dialog its card.

What the app sets **before** `init()`:

```cpp
#include <aurore.hpp>
#include <aurore/cadran.hpp>

// What Cadran asks of the app (all optional), and its preferences, kept in
// the app's own settings (a ui::Prefs).
auto& h = ui::hooks::get();
h.version    = [] { return std::string("1.0.0"); };
h.rail_icons = [] { return std::vector<std::string>{ "today", "list", "", "settings" }; };
h.push_main  = [](int tab, const std::string& row) { /* push a new main screen on `tab` */ };
h.save_prefs = [] { return my_settings::save(); };
ui::use_prefs(my_settings::get().look);

aurore::Application::init();
aurore::Application::createWindow("my_app/title"_i18n);
```

`aurore::Application::setCadran(false)`, before `init()`, keeps borealis' own look.

## What the app tells it

`ui::hooks::get()` (`cadran/ui/hooks.hpp`) holds what Cadran asks of the app: the main screen and its tabs (to go back to a tab, re-read it, title a screen opened from it), the rail's icons, the app's version, commit and logo, whether it writes nothing for now and a mode of its own the header's pill says over every screen (`pill`, with what a tap opens), the icons of its own screens' lines, which screens stand on their own (`standalone_screen`: no crumb, no version badge; or a view with the id `cadran/standalone` in the screen), saving the preferences, sending a report, today's date by the app's own clock, and its test flags. Each one left empty does nothing.

`ui::Prefs` (`cadran/ui/prefs.hpp`) is the language, theme (its variant, and its file: `theme_file`), text size, density, effects, sidebar and background glow chosen. The app keeps it in its settings and hands it over with `ui::use_prefs()`; the pickers (`ui::pick_language`, `pick_theme`, `pick_palette`, `pick_text_size`, `pick_density`, `quick_settings`) change it there and save it through the hook. A new theme is drawn from the next frame on every screen, nothing built again (docs/themes.md); a new language or density builds the screens again through `push_main` (without it, the screen shown is read again). The language "system" is the platform's: on a desktop, the display language on Windows, the first of the preferred languages on macOS, `LANGUAGE`, `LC_ALL`, `LC_MESSAGES` or `LANG` on Linux (`AURORE_LANG=fr-CA` overrides them on every desktop, and `AURORE_THEME=LIGHT` or `DARK` the variant; `AURORE_LANG` and `AURORE_THEME`, the older names, still work), each matched to one of the 15 catalogues (`aurore::localeFromSystem`: `es_MX` gives `es-419`, `zh_TW` `zh-Hant`). The palette picker lists Aurore's presets and the user's theme files in `ui::hooks`' `themes_dir`, each with a swatch of its own colours and its motif (`ui::pick`'s `swatches`).

Cadran's two palettes (`cadran/ui/cadran_palette.hpp`) are its only colours: each is a theme key (`cadran/teal`, `cadran/surf`…, and the core's `aurore/…` keys it sets), so a theme file restyles every Cadran screen (docs/themes.md lists them). Cadran's pieces keep keys, not colours (`View::setThemeColor`, `kit::label(text, size, "cadran/text2")`, `Card::setFill("cadran/surf")`, `ui::show_writable(cell, writable, "cadran/coral")`); an app's own views do the same to follow a theme change.

## What it gives

- **Frame** (`cadran/ui/cadran_frame.hpp`): the rail, open, on its icons, or folding on its icons while a tab has the focus and opening over the tab to choose one (`ui::Prefs::rail`, `ui::pick_rail`), with a badge per tab (`TabBadge`); the header (mark, title, pill, date), the crumb of a screen opened from a tab, the background glow. The header's language, theme and text size are shortcuts for a finger or a mouse (a touch 56 px high finds them), not for the controller: they take no focus, so the app offers the same choices on a screen the controller reaches (its settings, with `ui::pick_language`, `pick_theme`, `pick_text_size` or `quick_settings`), as the demo's Cadran tab does with its quick settings.
- **Pieces** (`cadran_kit.hpp`, `cadran_list.hpp`): cards, rows, chips, pills, bars, keys, switches, QR codes, empty states.
- **A tab of borealis cells** (a `ScrollingFrame` or a `RecyclerFrame` of `DetailCell`, `BooleanCell`…, as the demo's Settings): padded with `@style/aurore/tab_frame/content_padding_sides` and `…/content_padding_top_bottom`, it keeps the margins of Cadran's pages (40 and 28 px), its focus ring off the window's edge and the header's line. The rail's names too long for it are drawn down to 80 % of their size rather than cut.
- **Dialogs and sheets** (`ui.hpp`, `cadran_modal.hpp`, `action_sheet.hpp`): confirmations (dangerous ones in coral), notices, checklists, hold-to-confirm, failures with a report button, lists of values (rows, grids, several marked), lists of actions with icons, details, groups and switches, the keyboard.
- **Toasts** (`ui::notify`, `ui::refuse`, `ui::error`): news (`Toast::Info`, text alone), done and refused, these two with their icon.
- **Views** (`cadran/view/`): `CadranIcon` (about 60 vector icons), `CadranDigits`, `FooterHints`, `HeaderSetting`, `LoadingBar`, `MarkdownView` (headings, lists, tables, alerts, emoji), `QrView`, `ScrollView`, `VersionBadge`.
- **Live preferences** (`live_prefs.hpp`, `text_size.hpp`, `effects.hpp`, `motion.hpp`): a new theme drawn at once; every screen built again in the new language, the focus kept; the text normal, larger (×1.25), very large (×1.5) or larger on a TV only (the default); the [density](styles.md#density-of-the-lengths) comfortable, compact, dense, or automatic (the default: dense on a [compact screen](styles.md#screen-width), the app's own elsewhere); lighter effects on battery or when asked. The text size and the reduced effects go through the core (`aurore::Application::setTextScale`, `getReduceMotion`, docs/styles.md), so views that are not Cadran's follow them too.
- **Accessibility** ([styles.md](styles.md#accessibility)): every piece says what it is when it takes the focus. A list's line its title and value ("Updates, 3 new"), a switch's line "on" or "off" (its drawn switch in place of the detail), a chip is a button with its text, a focusable card (`kit::card(true, …)`) a cell with the texts in it, an action sheet or picker its title when it opens, then its line with "selected" for the value chosen ("Theme, list, Dark, selected, 3 of 3"), a dialog its text, a toast its text; the header's language, theme and text size are buttons ("Language, English"). An app's own focusable boxes say the texts in them without anything to add; one that shows only an icon or a picture needs an `accessibilityLabel`.
- **Helpers** (`ui.hpp`, `format_text.hpp`, `cadran/util/`): showing and hiding views with the focus, batched layout, durations and days in the language, text and UTF-8 helpers, a Markdown parser.

## Testing an app with it

On the desktop, in a build with `AURORE_DEV_TOOLS` (not a `Release` one, by default: [hot reload](hot_reload.md#turning-it-on)), these environment variables (or the app's own, through `ui::hooks::get().env_flag`, in every build) help tests and screenshots: `AURORE_STILL_FOCUS=1` (the focus glow and every movement held still), `AURORE_CADRAN_NOGLOW=1` (no background glow), `AURORE_FOCUS_LOG=1` (each focused line logged by its title), `AURORE_CUT_TEXT=1` (every label cut short on the screen on top written to `cut_text.txt` in the working directory, checked once a second, with the screen's name; `Harness::silentFocusables()` is its counterpart for views that take the focus with nothing to say), `AURORE_DOCKED=1` (the console on its dock: larger text) and `AURORE_BATTERY=1` (in hand: lighter effects).
