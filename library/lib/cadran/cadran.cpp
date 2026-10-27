// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#include <borealis/cadran.hpp>

#include <borealis/cadran/ui/focus_log.hpp>

namespace cadran
{

void start()
{
    live_prefs::start();
}

void start_window()
{
    ui::use_font_for(live_prefs::locale());
    install_theme();
    live_prefs::apply_theme(); // a theme file over the palettes just installed
    text_size::install();
    FooterHints::install();
    VersionBadge::install();
    HeaderSetting::install();
    focus_log::install();
    install_screens();
}

} // namespace cadran
