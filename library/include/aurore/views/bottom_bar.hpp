/*
    Copyright 2021 XITRIX

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
    AURORE_BIND(Box, hints, "brls/hints");
    AURORE_BIND(Label, time, "brls/hints/time");
    AURORE_BIND(View, battery, "brls/battery");
    AURORE_BIND(View, wireless, "brls/wireless");
};

} // namespace aurore
