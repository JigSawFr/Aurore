// SPDX-FileCopyrightText: 2024 xfangfang
// SPDX-License-Identifier: Apache-2.0

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
