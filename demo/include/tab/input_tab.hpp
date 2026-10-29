// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore.hpp>
#include <aurore/cadran/view/scroll_view.hpp>

class InputTab : public ScrollView
{
  public:
    InputTab();

    static aurore::View* create();
};
