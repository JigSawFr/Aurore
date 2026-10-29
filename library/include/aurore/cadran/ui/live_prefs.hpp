// live_prefs — Preferences › Theme and Language applied at once, without
// restarting. The theme: its variant and its file set, every view of the
// screens drawn in it from the next frame (View::setThemeColor,
// aurore::Application::getThemeChangedEvent). The language: its strings loaded
// again (aurore::loadTranslations), then the main screen built again on the tab
// that was shown, the focus on the line it was on (every view takes its
// words and font when it is made; Chinese has its own font, ui::use_font_for
// and the default font looked up for each label).
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

#include <string>

namespace live_prefs
{
/// Right after Aurore starts, which reads the console's own theme and
/// language (what "system" means): those kept, and the strings loaded in the
/// preferences' language.
void start();

/// The theme in the preferences: its variant (ui::Prefs::theme) and its file
/// (ui::Prefs::theme_file), the boards' palettes when it cannot be read.
/// The screens take it at their next frame.
void apply_theme();

/// The language the strings are in now: the preferences', or the console's.
std::string locale();

/// After the preferences' language changed: applied, and the screens built
/// again (ui::hooks' push_main; without it, the screen shown is read again).
void language_changed();

/// The density in the preferences (ui::Prefs::density) given to Aurore
/// (aurore::setDensity), before the first screen.
void apply_density();
/// After the preferences' density changed: applied, and the screens built
/// again as for the language (their sizes are read when they are made).
void density_changed();
} // namespace live_prefs
