// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

// cut_text — the desktop's check for texts cut short (PlayGuard's
// tools/cut_text_check.py): with AURORE_CUT_TEXT=1 (ui::env_flag), once a
// second, every label on the screen on top that its box cuts (drawn with "…",
// aurore::Label::isTruncated) goes to cut_text.txt in the working directory,
// once each: the screen's name (ui::hooks' shown_screen,
// "… › dialog" or "… › picker" for one opened over it), a tab, then the label's whole text. Nothing on the console or in a browser.
#pragma once

namespace cut_text
{
/// Cadran calls it after each frame; an app may call it too, from its own
/// main loop. It does nothing without the variable.
void sample();
} // namespace cut_text
