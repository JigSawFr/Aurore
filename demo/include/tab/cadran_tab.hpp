// The demo's Cadran pieces: the buttons that open Cadran's dialogs, sheets
// and toasts, its icons and a Markdown text, each built in code for the
// Components tab (tab/components_tab.cpp).
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

#include <aurore.hpp>

namespace cadran_pieces
{
/// A row of buttons, each opening one of Cadran's dialogs or sheets.
aurore::Box* dialogs();
/// A row of buttons, each posting a toast, a notification or an error.
aurore::Box* toasts();
/// Every icon of Cadran, in rows.
aurore::Box* icons();
/// A Markdown text as Cadran lays it out, added to `box`.
void markdown(aurore::Box* box);
} // namespace cadran_pieces
