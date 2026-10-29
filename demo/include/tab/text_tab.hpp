// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <utility>
#include <vector>
#include <aurore.hpp>
#include <aurore/cadran/view/scroll_view.hpp>

/// Text and languages: the 15 languages switched live, the text sizes and
/// the type scale, scripts and emoji, long texts, and the text lab.
class TextTab : public ScrollView
{
  public:
    TextTab();

    static aurore::View* create();

  private:
    std::vector<std::pair<aurore::Box*, std::string>> sizeChoices;
};
