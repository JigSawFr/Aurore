// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#include <borealis/cadran/ui/effects.hpp>

#include <string>

#include <borealis/core/application.hpp>

#include <borealis/cadran/ui/prefs.hpp>
#include <borealis/cadran/ui/text_size.hpp>

namespace effects
{

bool in_hand()
{
#ifdef __SWITCH__
    return !text_size::docked();
#else
    return ui::env_flag("BATTERY");
#endif
}

bool light()
{
    return ui::prefs().effects == "auto" && in_hand();
}

bool reduced()
{
    return ui::prefs().effects == "reduced";
}

bool motion()
{
    // The smoke test's screenshots compare frames: nothing may depend on when
    // they are taken (the same flag holds the focus highlight still).
    static const bool still = ui::env_flag("STILL_FOCUS");
    if (still)
        return false;
    // The app's own reduced motion (brls::Application::setReduceMotion).
    if (brls::Application::isReduceMotionSet())
        return false;
    const std::string& setting = ui::prefs().effects;
    if (setting == "reduced")
        return false;
    return setting != "auto" || !in_hand();
}

} // namespace effects
