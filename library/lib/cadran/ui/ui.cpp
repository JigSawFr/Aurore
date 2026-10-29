// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-FileCopyrightText: 2026 Taylor
// SPDX-License-Identifier: GPL-3.0-or-later

// ui — toasts, the failures said in a dialog and screen changes. The other
// cadran/ui/*.cpp hold the rest of ui.hpp by concern.
#include <aurore/cadran/ui/ui.hpp>

#include <aurore/cadran/ui/cadran_modal.hpp>
#include <aurore/cadran/ui/cadran_text.hpp>
#include <aurore/cadran/ui/effects.hpp>
#include <aurore/cadran/ui/hooks.hpp>

using namespace aurore::literals;

namespace ui
{

void notify(const std::string& text, Toast kind)
{
    // Logged too: the desktop smoke test checks what the user was told.
    aurore::Logger::info("toast: {}", text);
    aurore::sync([text, kind]()
        { aurore::Application::notify(text, kind == Toast::Info ? nullptr : cadran::toast_icon(kind == Toast::Done)); });
}

// A refusal: the line that asked shakes its head (its focus ring, as the
// console does at the end of a list), with the toast that says why.
static void shake_focus()
{
    if (!effects::motion())
        return;
    if (aurore::View* focus = aurore::Application::getCurrentFocus())
        focus->shakeHighlight(aurore::FocusDirection::LEFT);
}

void refuse(const std::string& text)
{
    aurore::sync([]()
        { shake_focus(); });
    notify(text, Toast::Refused);
}

static void show_error(const std::string& text)
{
    aurore::Logger::info("error: {}", text);
    cadran_text::Failure f;
    f.reason = text;
    show_failure(f);
}

void error(const std::string& text)
{
    aurore::sync([text]()
        { show_error(text); });
}

void error(const std::string& what, const std::string& why)
{
    aurore::Logger::info("error: {}: {}", what, why);
    cadran_text::Failure f;
    f.title  = what;
    f.reason = why;
    aurore::sync([f]()
        { show_failure(f); });
}

void replace_screen(aurore::Activity* next)
{
    const auto none = aurore::TransitionAnimation::NONE;
    if (!aurore::Application::popActivity(none, [next, none]()
            { aurore::Application::pushActivity(next, none); }))
        aurore::Application::pushActivity(next, none);
}

bool save_prefs()
{
    return !hooks::get().save_prefs || hooks::get().save_prefs();
}

} // namespace ui
