// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

#include <aurore/cadran/ui/text_size.hpp>

#include <yoga/Yoga.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <aurore.hpp>

#include <aurore/cadran/ui/cadran_list.hpp>
#include <aurore/cadran/ui/prefs.hpp>
#include <aurore/cadran/ui/ui.hpp>
#include <aurore/cadran/view/cadran_digits.hpp>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace text_size
{
bool docked()
{
#ifdef __SWITCH__
    return appletGetOperationMode() == AppletOperationMode_Console;
#else
    return ui::env_flag("DOCKED");
#endif
}

float scale()
{
    // Over the layout preset's own (a small screen's text is larger already)
    const float base        = aurore::Application::getPresetTextScale();
    const std::string& size = ui::prefs().text_size;
    if (size == "largest")
        return base * LARGEST;
    return base * (size == "large" || (size == "auto" && docked()) ? LARGE : 1.0f);
}

bool larger()
{
    return scale() > aurore::Application::getPresetTextScale();
}

namespace
{
#ifdef __SWITCH__
    AppletHookCookie s_cookie;

    // On the dock or off it: next frame (libnx calls this inside the main loop).
    void on_applet_hook(AppletHookType hook, void*)
    {
        if (hook == AppletHookType_OnOperationMode)
            aurore::sync([]()
                { apply(); });
    }
#endif
} // namespace

void apply()
{
    const float wanted = scale();
    if (wanted == aurore::Application::getTextScale())
        return;
    // Every label of every screen measured and laid out again (the core's).
    aurore::Application::setTextScale(wanted);
    // The tab on screen, for what it lays out by the size (Today's room).
    ui::refresh_shown_tab();
}

void install()
{
    apply();
    static bool hooked = false;
    if (hooked)
        return;
    hooked = true;
#ifdef __SWITCH__
    appletHook(&s_cookie, on_applet_hook, nullptr);
#endif
}
} // namespace text_size
