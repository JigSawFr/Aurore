// ui — toasts, the failures said in a dialog and screen changes. The other
// cadran/ui/*.cpp hold the rest of ui.hpp by concern.
// Copyright (C) 2026 JigSawFr, (C) 2026 Taylor.  GPLv3-or-later (see LICENSE.GPL).
#include <aurore/cadran/ui/ui.hpp>

#include <aurore/cadran/ui/cadran_modal.hpp>
#include <aurore/cadran/ui/cadran_text.hpp>
#include <aurore/cadran/ui/effects.hpp>
#include <aurore/cadran/ui/hooks.hpp>

using namespace brls::literals;

namespace ui
{

void notify(const std::string& text, Toast kind)
{
    // Logged too: the desktop smoke test checks what the user was told.
    brls::Logger::info("toast: {}", text);
    brls::sync([text, kind]()
        { brls::Application::notify(text, kind == Toast::Info ? nullptr : cadran::toast_icon(kind == Toast::Done)); });
}

// A refusal: the line that asked shakes its head (its focus ring, as the
// console does at the end of a list), with the toast that says why.
static void shake_focus()
{
    if (!effects::motion())
        return;
    if (brls::View* focus = brls::Application::getCurrentFocus())
        focus->shakeHighlight(brls::FocusDirection::LEFT);
}

void refuse(const std::string& text)
{
    brls::sync([]()
        { shake_focus(); });
    notify(text, Toast::Refused);
}

static void show_error(const std::string& text)
{
    brls::Logger::info("error: {}", text);
    cadran_text::Failure f;
    f.reason = text;
    show_failure(f);
}

void error(const std::string& text)
{
    brls::sync([text]()
        { show_error(text); });
}

void error(const std::string& what, const std::string& why)
{
    brls::Logger::info("error: {}: {}", what, why);
    cadran_text::Failure f;
    f.title  = what;
    f.reason = why;
    brls::sync([f]()
        { show_failure(f); });
}

void replace_screen(brls::Activity* next)
{
    const auto none = brls::TransitionAnimation::NONE;
    if (!brls::Application::popActivity(none, [next, none]()
            { brls::Application::pushActivity(next, none); }))
        brls::Application::pushActivity(next, none);
}

bool save_prefs()
{
    return !hooks::get().save_prefs || hooks::get().save_prefs();
}

} // namespace ui
