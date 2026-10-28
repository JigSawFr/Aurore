// focus_log — for an app's desktop tests (PlayGuard's tools/desktop_smoke.py):
// with AURORE_FOCUS_LOG=1 (ui::env_flag), each line that takes the focus logs its
// title ("focus: Check for updates"), so the test reaches a line by its
// words, not by counting presses. Nothing on the console or in a browser.
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

namespace focus_log
{
/// Once, at start-up; does nothing without the variable.
void install();
} // namespace focus_log
