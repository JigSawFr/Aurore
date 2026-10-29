// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

#include <aurore/cadran/ui/effects.hpp>

#include <string>

#include <aurore/core/application.hpp>

#include <aurore/cadran/ui/prefs.hpp>
#include <aurore/cadran/ui/text_size.hpp>

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
    // The app's own reduced motion (aurore::Application::setReduceMotion).
    if (aurore::Application::isReduceMotionSet())
        return false;
    const std::string& setting = ui::prefs().effects;
    if (setting == "reduced")
        return false;
    return setting != "auto" || !in_hand();
}

} // namespace effects
