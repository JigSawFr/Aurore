// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#include <aurore/cadran/ui/focus_log.hpp>

#include <aurore.hpp>

#include <aurore/cadran/ui/prefs.hpp>

namespace focus_log
{
void install()
{
#if !defined(__SWITCH__) && !defined(__EMSCRIPTEN__)
    static bool done = false;
    if (done || !ui::env_flag("FOCUS_LOG"))
        return;
    done = true;
    brls::Application::getGlobalFocusChangeEvent()->subscribe([](brls::View* view)
        {
        // A list's line (a sheet's too), else a dialog's button.
        if (auto* cell = dynamic_cast<brls::DetailCell*>(view)) brls::Logger::info("focus: {}", cell->title->getFullText());
        else if (auto* button = dynamic_cast<brls::Button*>(view)) brls::Logger::info("focus: {}", button->getText()); });
#endif
}
} // namespace focus_log
