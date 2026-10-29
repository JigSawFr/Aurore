// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore.hpp>
#include <aurore/cadran/view/scroll_view.hpp>

/// Every control, grouped: buttons, toggles and values, progress, chips and
/// pills, dialogs and sheets, toasts, images, icons, Markdown, and a screen
/// of borealis' settings cells as Cadran dresses them.
class ComponentsTab : public ScrollView
{
  public:
    ComponentsTab();

    static aurore::View* create();
};
