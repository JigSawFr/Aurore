/*
Copyright 2024 xfangfang

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

#include <aurore.hpp>

class TextTestTab : public aurore::Box
{
  public:
    TextTestTab();

    static aurore::View* create();

  private:
    AURORE_BIND(aurore::SliderCell, width, "width");
    AURORE_BIND(aurore::SliderCell, height, "height");
    AURORE_BIND(aurore::BooleanCell, singleLine, "singleLine");
    AURORE_BIND(aurore::SelectorCell, horizontal, "horizontal");
    AURORE_BIND(aurore::SelectorCell, vertical, "vertical");
    AURORE_BIND(aurore::Label, label, "label");
};
