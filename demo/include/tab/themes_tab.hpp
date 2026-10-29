// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <utility>
#include <vector>
#include <aurore.hpp>
#include <aurore/cadran/view/scroll_view.hpp>

/// Themes: light or dark, every palette (Cadran's, Aurore's presets, the
/// demo's theme files) used at once, a theme file in a few lines, the look.
class ThemesTab : public ScrollView
{
  public:
    ThemesTab();

    static aurore::View* create();

    /// The palette and the variant in use, outlined.
    void markChosen();

  private:
    std::vector<std::pair<aurore::Box*, std::string>> palettes;
    std::vector<std::pair<aurore::Box*, std::string>> variants;
};
