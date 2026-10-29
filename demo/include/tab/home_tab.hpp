// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore.hpp>
#include <aurore/cadran/view/scroll_view.hpp>

/// The first tab: Aurore in one sentence over a night with an aurora drawn
/// in nanovg, a dashboard of live cards (a ring, bars, figures) and a tile
/// for each of the other tabs.
class HomeTab : public ScrollView
{
  public:
    HomeTab();

    static aurore::View* create();
};
