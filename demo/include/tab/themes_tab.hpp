/*
    Copyright 2026 JigSawFr

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

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
