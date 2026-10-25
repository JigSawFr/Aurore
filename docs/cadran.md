# Cadran

Cadran is the look of [PlayGuard](https://github.com/JigSawFr/playguard) and [PlayShelf](https://github.com/JigSawFr/playshelf), now part of Aurore: navy and cool-white palettes, an icon rail for a sidebar, the app's mark and a status pill in the header, cards in place of lists, modals and action sheets, vector icons, figures in Sora, and the language, theme and text size changed while the app runs, in 15 languages.

It lives in `library/include/borealis/cadran/` and `library/lib/cadran/`, its strings in `resources/i18n/<language>/cadran.json`, its emoji in `resources/img/emoji/`, its font in `resources/font/`. It is **GPLv3** (`LICENSE.GPL`): an app built with it is GPLv3.

## Turning it on

There is nothing to do: `brls::Application::init()` and `createWindow()` start it, and every screen takes its look the first time it takes the focus. A main screen (an `AppletFrame` holding a `TabFrame`) gets the rail and the header with the app's mark, the date and the language, theme and text size; every other screen its header and its list in cards; every dialog its card.

What the app sets **before** `init()`:

```cpp
#include <borealis.hpp>
#include <borealis/cadran.hpp>

// What Cadran asks of the app (all optional), and its preferences, kept in
// the app's own settings (a ui::Prefs).
auto& h = ui::hooks::get();
h.version    = [] { return std::string("1.0.0"); };
h.rail_icons = [] { return std::vector<std::string>{ "today", "list", "", "settings" }; };
h.push_main  = [](int tab, const std::string& row) { /* push a new main screen on `tab` */ };
h.save_prefs = [] { return my_settings::save(); };
ui::use_prefs(my_settings::get().look);

brls::Application::init();
brls::Application::createWindow("my_app/title"_i18n);
```

`brls::Application::setCadran(false)`, before `init()`, keeps borealis' own look.

## What the app tells it

`ui::hooks::get()` (`cadran/ui/hooks.hpp`) holds what Cadran asks of the app: the main screen and its tabs (to go back to a tab, re-read it, title a screen opened from it), the rail's icons, the app's version, commit and logo, whether it writes nothing for now (the header's pill says so), saving the preferences, sending a report, today's date by the app's own clock, and its test flags. Each one left empty does nothing.

`ui::Prefs` (`cadran/ui/prefs.hpp`) is the language, theme, text size, effects and background glow chosen. The app keeps it in its settings and hands it over with `ui::use_prefs()`; the pickers (`ui::pick_language`, `pick_theme`, `pick_text_size`, `quick_settings`) change it there and save it through the hook. A new language or theme builds the screens again through `push_main`; without it, the screens opened from then on take the change.

## What it gives

- **Frame** (`cadran/ui/cadran_frame.hpp`): the rail, the header (mark, title, pill, date), the crumb of a screen opened from a tab, the background glow.
- **Pieces** (`cadran_kit.hpp`, `cadran_list.hpp`): cards, rows, chips, pills, bars, keys, switches, QR codes, empty states.
- **Dialogs and sheets** (`ui.hpp`, `cadran_modal.hpp`, `action_sheet.hpp`): confirmations (dangerous ones in coral), notices, checklists, hold-to-confirm, failures with a report button, lists of values (rows, grids, several marked), lists of actions with icons, details, groups and switches, the keyboard.
- **Toasts** (`ui::notify`, `ui::refuse`, `ui::error`): news, done, refused, each with its icon.
- **Views** (`cadran/view/`): `CadranIcon` (about 60 vector icons), `CadranDigits`, `FooterHints`, `HeaderSetting`, `LoadingBar`, `MarkdownView` (headings, lists, tables, alerts, emoji), `QrView`, `ScrollView`, `VersionBadge`.
- **Live preferences** (`live_prefs.hpp`, `text_size.hpp`, `effects.hpp`, `motion.hpp`): every screen built again in the new language or theme, the focus kept; larger text on a TV; lighter effects on battery or when asked.
- **Helpers** (`ui.hpp`, `format_text.hpp`, `cadran/util/`): showing and hiding views with the focus, batched layout, durations and days in the language, text and UTF-8 helpers, a Markdown parser.

## Testing an app with it

On the desktop, these environment variables (or the app's own, through `ui::hooks::get().env_flag`) help tests and screenshots: `BRLS_STILL_FOCUS=1` (the focus glow and every movement held still), `BRLS_CADRAN_NOGLOW=1` (no background glow), `BRLS_FOCUS_LOG=1` (each focused line logged by its title), `BRLS_CUT_TEXT=1` (every label cut short written to `cut_text.txt`), `BRLS_DOCKED=1` (the console on its dock: larger text) and `BRLS_BATTERY=1` (in hand: lighter effects).
