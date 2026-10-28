// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
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
    const std::string& size = ui::prefs().text_size;
    if (size == "largest")
        return LARGEST;
    return size == "large" || (size == "auto" && docked()) ? LARGE : 1.0f;
}

bool larger()
{
    return scale() > 1.0f;
}

namespace
{
#ifdef __SWITCH__
    AppletHookCookie s_cookie;

    // On the dock or off it: next frame (libnx calls this inside the main loop).
    void on_applet_hook(AppletHookType hook, void*)
    {
        if (hook == AppletHookType_OnOperationMode)
            brls::sync([]()
                { apply(); });
    }
#endif
} // namespace

void apply()
{
    const float wanted = scale();
    if (wanted == brls::Application::getTextScale())
        return;
    // Every label of every screen measured and laid out again (the core's).
    brls::Application::setTextScale(wanted);
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
