// effects — Preferences › Visual effects: "auto" (the default: in full on the
// dock, lighter in hand), "full" (the background glow, the rings' arcs on
// the move, the screens' and lists' entrances, the small answers to a
// press, in hand as on the dock) or "reduced" (no decorative movement at
// all, the glow still: for whom motion tires). Lighter: the glow, the
// ring's pulse and every other movement left out. In hand: off the dock,
// handheld or on its stand, charging or not (the console's operation mode);
// the desktop: BRLS_BATTERY=1 (ui::env_flag) (off by default, so the simulator
// draws in full as the dock does).
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

namespace effects
{
// The console is off its dock.
bool in_hand();
// What the setting and the power ask for now: the lighter effects (no glow,
// nothing moving).
bool light();
// The setting is "reduced": nothing moves for show (the glow stays).
bool reduced();
// A decorative movement may be drawn now: neither light() nor reduced(),
// not the app's reduced motion (brls::Application::setReduceMotion), and
// not the desktop's still frames (BRLS_STILL_FOCUS, for the screenshots
// that compare). Every animation of the look asks this first; the core's
// own (transition=) ask brls::Application::getReduceMotion(), which asks
// this.
bool motion();
}   // namespace effects
