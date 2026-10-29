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
