// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

// What a sheet over the screen (ui::pick() and the others of action_sheet.cpp,
// the icon sheet of a forwarder) shares: closing it, and the room around its
// panel, where a touch closes it.
#pragma once

#include <functional>
#include <aurore.hpp>

namespace ui
{

/// Pops the sheet (a fade), then runs `then` (if any) on the next frame.
void close_sheet(std::function<void()> then);

/// Room around the panel, `width` wide or taking what is left (`grow`): a
/// touch there closes the sheet.
aurore::Box* sheet_outside(float width, bool grow);

} // namespace ui
