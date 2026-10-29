// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore.hpp>
#include <aurore/cadran/view/scroll_view.hpp>

class AboutTab : public ScrollView
{
  public:
    AboutTab();

    static aurore::View* create();
};
