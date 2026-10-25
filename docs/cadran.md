# Cadran

Cadran is the look of [PlayGuard](https://github.com/JigSawFr/playguard) and [PlayShelf](https://github.com/JigSawFr/playshelf), now part of Aurore: navy and cool-white palettes, an icon rail for a sidebar, the app's mark and a status pill in the header, cards in place of lists, modals and action sheets, vector icons, figures in Sora, and the language, theme and text size changed while the app runs, in 15 languages.

It lives in `library/include/borealis/cadran/` and `library/lib/cadran/`, its strings in `resources/i18n/<language>/cadran.json`, its emoji in `resources/img/emoji/`, its font in `resources/font/`. It is **GPLv3** (`LICENSE.GPL`): an app built with it is GPLv3.

For now an app turns it on itself, as below; it becomes Aurore's default look next.

## Turning it on

```cpp
#include <borealis.hpp>
#include <borealis/cadran/ui/cadran_frame.hpp>
#include <borealis/cadran/ui/hooks.hpp>
#include <borealis/cadran/ui/live_prefs.hpp>
#include <borealis/cadran/ui/prefs.hpp>
#include <borealis/cadran/ui/text_size.hpp>
#include <borealis/cadran/ui/ui.hpp>
#include <borealis/cadran/view/footer_hints.hpp>
#include <borealis/cadran/view/header_settings.hpp>
#include <borealis/cadran/view/version_badge.hpp>

// 1. What Cadran asks of the app (all optional), and its preferences, kept
//    in the app's own settings.
auto& h = ui::hooks::get();
h.version   = [] { return std::string("1.0.0"); };
h.save_prefs = [] { return my_settings::save(); };
ui::use_prefs(my_settings::get().look);   // a ui::Prefs

// 2. borealis, then the preferences' language and theme.
brls::Application::init();
live_prefs::start();
brls::Application::createWindow("my_app/title"_i18n);
ui::use_font_for(live_prefs::locale());
live_prefs::apply_theme();

// 3. The look, before the first screen.
cadran::install_theme();
text_size::install();
FooterHints::install();
VersionBadge::install();
HeaderSetting::install();
cadran::install_screens();

// 4. The main screen: an AppletFrame holding a TabFrame.
cadran::style_frame(frame, frame->getView("main_tabs"));
```

## What the app tells it

`ui::hooks::get()` (`cadran/ui/hooks.hpp`) holds what Cadran asks of the app: the main screen and its tabs (to go back to a tab, re-read it, title a screen opened from it), the rail's icons, the app's version, commit and logo, whether it writes nothing for now (the header's pill says so), saving the preferences, sending a report, today's date by the app's own clock, and its test flags. Each one left empty does nothing.

`ui::Prefs` (`cadran/ui/prefs.hpp`) is the language, theme, text size, effects and background glow chosen. The app keeps it in its settings and hands it over with `ui::use_prefs()`; the pickers (`ui::pick_language`, `pick_theme`, `pick_text_size`, `quick_settings`) change it there and save it through the hook.

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
