// SPDX-FileCopyrightText: 2020-2021 natinusala
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/frame_context.hpp>
#include <aurore/core/view.hpp>

namespace aurore
{

// A solid color rectangle
class Rectangle : public View
{
  public:
    Rectangle(NVGcolor color);
    Rectangle();
    ~Rectangle() { }

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;

    void setColor(NVGcolor color);
    NVGcolor getColor() { return this->color; }

    bool getStyleProperty(const std::string& name, StyleValue* value) override;
    bool setStyleProperty(const std::string& name, const StyleValue& value) override;

    static View* create();

  private:
    NVGcolor color = nvgRGB(0, 0, 255);
};

} // namespace aurore
