// text_size — Preferences › Text size: "auto"
// (the default: larger while the console is docked, on a TV), "normal",
// "large" or "largest". Larger draws every label LARGE times its size,
// largest LARGEST times (aurore::Application::setTextScale); the rail's too,
// the header's and the footer's a little less, each as its room allows
// (ui/cadran_frame); the cells' rows grow with it (aurore::RecyclerCell).
// Applied at start-up, then again, live, when the setting changes or the
// console leaves or joins its dock: every label of every screen is laid
// out again.
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

namespace text_size
{
constexpr float LARGE   = 1.25f;
constexpr float LARGEST = 1.5f;

/// The console on its dock (the desktop: BRLS_DOCKED=1, ui::env_flag).
bool docked();
/// What the setting and the console's mode ask for now: the labels' scale,
/// and whether it is more than 1.
float scale();
bool larger();
/// aurore::Label's scale from larger(); the screens laid out again when it changed.
void apply();
/// apply() before the first view, then follows the dock. Once, at start-up.
void install();
} // namespace text_size
