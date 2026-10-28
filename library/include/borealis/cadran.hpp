// Cadran — Aurore's look (docs/cadran.md): every header of borealis/cadran
// at once, and its start-up, which brls::Application runs itself unless the
// app turned it off (Application::setCadran(false)).
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

#include <borealis/cadran/ui/action_sheet.hpp>
#include <borealis/cadran/ui/cadran_frame.hpp>
#include <borealis/cadran/ui/cadran_kit.hpp>
#include <borealis/cadran/ui/cadran_list.hpp>
#include <borealis/cadran/ui/cadran_modal.hpp>
#include <borealis/cadran/ui/effects.hpp>
#include <borealis/cadran/ui/format_text.hpp>
#include <borealis/cadran/ui/hooks.hpp>
#include <borealis/cadran/ui/live_prefs.hpp>
#include <borealis/cadran/ui/motion.hpp>
#include <borealis/cadran/ui/prefs.hpp>
#include <borealis/cadran/ui/text_size.hpp>
#include <borealis/cadran/ui/ui.hpp>
#include <borealis/cadran/view/cadran_digits.hpp>
#include <borealis/cadran/view/cadran_icon.hpp>
#include <borealis/cadran/view/footer_hints.hpp>
#include <borealis/cadran/view/header_settings.hpp>
#include <borealis/cadran/view/loading_bar.hpp>
#include <borealis/cadran/view/markdown_view.hpp>
#include <borealis/cadran/view/qr_view.hpp>
#include <borealis/cadran/view/scroll_view.hpp>
#include <borealis/cadran/view/version_badge.hpp>

namespace cadran
{
/// Once borealis has started (Application::init): the console's theme and
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
