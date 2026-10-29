// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

// focus_log — for an app's desktop tests (PlayGuard's tools/desktop_smoke.py):
// with AURORE_FOCUS_LOG=1 (ui::env_flag), each line that takes the focus logs its
// title ("focus: Check for updates"), so the test reaches a line by its
// words, not by counting presses. Nothing on the console or in a browser.
#pragma once

namespace focus_log
{
/// Once, at start-up; does nothing without the variable.
void install();
} // namespace focus_log
