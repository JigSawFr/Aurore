// SPDX-FileCopyrightText: 2019-2021 natinusala
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/logger.hpp>
#include <aurore/views/rectangle.hpp>

namespace aurore
{

Rectangle::Rectangle(NVGcolor color)
{
    this->setColor(color);

    // Register XML attributes
    this->registerColorXMLAttribute("color", [this](NVGcolor color)
        { this->setColor(color); });
}

Rectangle::Rectangle()
    : Rectangle(nvgRGB(255, 255, 255))
{
    // Empty ctor for XML
}

void Rectangle::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    NVGcolor color = a(this->color);

    if (color.a == 0.0f)
        return;

    nvgFillColor(vg, color);

    nvgBeginPath(vg);

    if (this->getCornerRadius() > 0.0f)
        nvgRoundedRect(vg, x, y, width, height, this->getCornerRadius());
    else
        nvgRect(vg, x, y, width, height);

    nvgFill(vg);
}

void Rectangle::setColor(NVGcolor color)
{
    if (this->transitionStyle("color", StyleValue::of(color)))
        return;
    this->color = color;
}

bool Rectangle::getStyleProperty(const std::string& name, StyleValue* value)
{
    if (name != "color")
        return View::getStyleProperty(name, value);
    *value = StyleValue::of(this->color);
    return true;
}

bool Rectangle::setStyleProperty(const std::string& name, const StyleValue& value)
{
    if (name != "color")
        return View::setStyleProperty(name, value);
    if (value.type != StyleValue::Type::COLOR)
        return false;
    this->setColor(value.color);
    return true;
}

// void Rectangle::layout(NVGcontext* vg, Style* style, FontStash* stash)
// {
//     // Nothing to do
// }

View* Rectangle::create()
{
    return new Rectangle();
}

} // namespace aurore
