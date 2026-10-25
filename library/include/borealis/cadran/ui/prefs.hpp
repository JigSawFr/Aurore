// ui::prefs — what the user chose for Cadran's look: the language, the
// theme, the text size and the effects. The app keeps them in its own
// settings and hands them over with use_prefs() at start-up, before the
// first screen; the pickers (ui::pick_language…) change them there, then
// save them through ui::hooks::get().save_prefs.
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

#include <string>

namespace ui
{

struct Prefs
{
    std::string language  = "system";   // "system" or one of LANGUAGES
    std::string theme     = "dark";     // one of THEMES
    std::string text_size = "auto";     // one of TEXT_SIZES: "auto", larger on a TV
    std::string effects   = "auto";     // one of EFFECTS: "auto", lighter on battery
    bool        glow      = true;       // the Cadran screens' background glow
};

constexpr const char* LANGUAGES[]  = { "system", "en-US", "fr", "fr-CA", "de", "es", "es-419", "it", "nl",
                                       "pt", "pt-BR", "ru", "ja", "ko", "zh-Hans", "zh-Hant" };
constexpr const char* THEMES[]     = { "system", "light", "dark" };
constexpr const char* TEXT_SIZES[] = { "auto", "normal", "large" };
constexpr const char* EFFECTS[]    = { "auto", "full", "reduced" };

// The ones in use: the app's (use_prefs), else defaults of their own.
Prefs& prefs();
// From now on, prefs() is `p`, which must outlive every screen.
void use_prefs(Prefs& p);

// A flag of the environment, through ui::hooks::get().env_flag (its comment
// says which).
bool env_flag(const char* name);

}   // namespace ui
