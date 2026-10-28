// Cadran — Aurore's look (docs/cadran.md): every header of aurore/cadran
// at once, and its start-up, which aurore::Application runs itself unless the
// app turned it off (Application::setCadran(false)).
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

#include <aurore/cadran/ui/action_sheet.hpp>
#include <aurore/cadran/ui/cadran_frame.hpp>
#include <aurore/cadran/ui/cadran_kit.hpp>
#include <aurore/cadran/ui/cadran_list.hpp>
#include <aurore/cadran/ui/cadran_modal.hpp>
#include <aurore/cadran/ui/effects.hpp>
#include <aurore/cadran/ui/format_text.hpp>
#include <aurore/cadran/ui/hooks.hpp>
#include <aurore/cadran/ui/live_prefs.hpp>
#include <aurore/cadran/ui/motion.hpp>
#include <aurore/cadran/ui/prefs.hpp>
#include <aurore/cadran/ui/text_size.hpp>
#include <aurore/cadran/ui/ui.hpp>
#include <aurore/cadran/view/cadran_digits.hpp>
#include <aurore/cadran/view/cadran_icon.hpp>
#include <aurore/cadran/view/footer_hints.hpp>
#include <aurore/cadran/view/header_settings.hpp>
#include <aurore/cadran/view/loading_bar.hpp>
#include <aurore/cadran/view/markdown_view.hpp>
#include <aurore/cadran/view/qr_view.hpp>
#include <aurore/cadran/view/scroll_view.hpp>
#include <aurore/cadran/view/version_badge.hpp>

namespace cadran
{
/// Once Aurore has started (Application::init): the console's theme and
/// language kept, the strings loaded in the preferences' language. The app
/// sets ui::hooks and calls ui::use_prefs before.
void start();
/// Once the window is there (Application::createWindow): the preferences'
/// font and theme, the palettes, the text size, and the views and screens
/// that take Cadran's look from then on: the footer's hints, the header's
/// version badge and settings, every screen's header and list, and the main
/// screen (an AppletFrame holding a TabFrame) with its rail.
void start_window();
} // namespace cadran
