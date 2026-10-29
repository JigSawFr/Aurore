// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/application.hpp>
#include <aurore/core/bind.hpp>
#include <aurore/core/box.hpp>
#include <aurore/views/image.hpp>
#include <aurore/views/label.hpp>

namespace aurore
{

class BottomBar : public Box
{
  public:
    BottomBar();
    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    static View* create();

  private:
    void updateText();
    std::string bottomText;
    AURORE_BIND(Box, hints, "aurore/hints");
    AURORE_BIND(Label, time, "aurore/hints/time");
    AURORE_BIND(View, battery, "aurore/battery");
    AURORE_BIND(View, wireless, "aurore/wireless");
};

} // namespace aurore
