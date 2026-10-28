// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#include <borealis/cadran.hpp>

#include <borealis/cadran/ui/cut_text.hpp>
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
    // BRLS_CUT_TEXT=1: the labels cut short, written once a second (an app
    // calling cut_text::sample() itself changes nothing)
    static bool sampling = false;
    if (!sampling)
    {
        sampling = true;
        brls::Application::getPostFrameEvent()->subscribe([]
            { cut_text::sample(); });
    }
}

} // namespace cadran
