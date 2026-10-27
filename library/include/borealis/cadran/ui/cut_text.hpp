// cut_text — the desktop's check for texts cut short (PlayGuard's
// tools/cut_text_check.py): with BRLS_CUT_TEXT=1 (ui::env_flag), once a
// second, every label on the screen on top that its box cuts (drawn with "…",
// brls::Label::isTruncated) goes to cut_text.txt in the working directory,
// once each: the screen's name (ui::hooks' shown_screen,
// "… › dialog" or "… › picker" for one opened over it), a tab, then the label's whole text. Nothing on the console or in a browser.
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

namespace cut_text
{
// Once per main-loop iteration (the app's main loop); does nothing without
// the variable.
void sample();
} // namespace cut_text
