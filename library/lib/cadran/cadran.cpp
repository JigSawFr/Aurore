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
    live_prefs::apply_theme();
    install_theme();
    text_size::install();
    FooterHints::install();
    VersionBadge::install();
    HeaderSetting::install();
    focus_log::install();
    install_screens();
}

}   // namespace cadran
