// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

// ui::prefs — what the user chose for Cadran's look: the language, the
// theme, the text size, the effects and the sidebar. The app keeps them in its own
// settings and hands them over with use_prefs() at start-up, before the
// first screen; the pickers (ui::pick_language…) change them there, then
// save them through ui::hooks::get().save_prefs.
#pragma once

#include <string>

namespace ui
{

/// What the user chose for Cadran's look. The app keeps it in its settings
/// and hands it over with use_prefs().
struct Prefs
{
    std::string language = "system"; ///< "system" or one of LANGUAGES
    std::string theme    = "dark"; ///< one of THEMES: the light or dark variant
    /// A theme file over the boards' palettes (aurore::Theme::loadFile: a
    /// preset "builtin:<id>" or a file of the app's themes folder,
    /// ui::hooks' themes_dir); "" for the boards' own. A file that styles one
    /// variant only takes it, whatever `theme` says.
    std::string theme_file;
    std::string text_size = "auto"; ///< one of TEXT_SIZES: "auto", larger on a TV
    std::string effects   = "auto"; ///< one of EFFECTS: "auto", lighter on battery
    /// One of DENSITIES: how close the screens' pieces stand (the text keeps
    /// its size). "auto": dense on a compact screen, the app's own elsewhere.
    std::string density = "auto";
    bool glow           = true; ///< the Cadran screens' background glow
    /// The main screen's sidebar: "auto" (on its icons while a tab has the
    /// focus, open over the tab while the focus is on it), "open" (always)
    /// or "icons" (always on its icons). cadran::apply_rail() after a change.
    std::string rail = "auto";
};

constexpr const char* LANGUAGES[]  = { "system", "en-US", "fr", "fr-CA", "de", "es", "es-419", "it", "nl",
     "pt", "pt-BR", "ru", "ja", "ko", "zh-Hans", "zh-Hant" };
constexpr const char* THEMES[]     = { "system", "light", "dark" };
constexpr const char* TEXT_SIZES[] = { "auto", "normal", "large", "largest" };
constexpr const char* EFFECTS[]    = { "auto", "full", "reduced" };
constexpr const char* RAILS[]      = { "auto", "open", "icons" };
constexpr const char* DENSITIES[]  = { "auto", "comfortable", "compact", "dense" };

/// The ones in use: the app's (use_prefs), else defaults of their own.
Prefs& prefs();
/// From now on, prefs() is `p`, which must outlive every screen.
void use_prefs(Prefs& p);

/// A flag of the environment, through ui::hooks::get().env_flag (its comment
/// says which).
bool env_flag(const char* name);

} // namespace ui
