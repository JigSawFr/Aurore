// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#include <borealis/cadran/ui/text_size.hpp>

#include <borealis.hpp>
#include <cstdlib>
#include <string>
#include <vector>
#include <yoga/Yoga.h>

#include <borealis/cadran/ui/prefs.hpp>
#include <borealis/cadran/view/cadran_digits.hpp>
#include <borealis/cadran/ui/cadran_list.hpp>
#include <borealis/cadran/ui/ui.hpp>

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

bool larger()
{
    const std::string& size = ui::prefs().text_size;
    return size == "large" || (size == "auto" && docked());
}

namespace
{
#ifdef __SWITCH__
AppletHookCookie s_cookie;

// On the dock or off it: next frame (libnx calls this inside the main loop).
void on_applet_hook(AppletHookType hook, void*)
{
    if (hook == AppletHookType_OnOperationMode) brls::sync([]() { apply(); });
}
#endif
}   // namespace

void apply()
{
    const float scale = larger() ? LARGE : 1.0f;
    if (scale == brls::Application::getTextScale()) return;
    // Every label of every screen measured and laid out again (the core's).
    brls::Application::setTextScale(scale);
    // The tab on screen, for what it lays out by the size (Today's room).
    ui::refresh_shown_tab();
}

void install()
{
    apply();
    static bool hooked = false;
    if (hooked) return;
    hooked = true;
#ifdef __SWITCH__
    appletHook(&s_cookie, on_applet_hook, nullptr);
#endif
}
}   // namespace text_size
