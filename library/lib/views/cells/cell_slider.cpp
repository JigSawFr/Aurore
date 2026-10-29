// SPDX-FileCopyrightText: 2023 xfangfang
// SPDX-License-Identifier: Apache-2.0

#include "aurore/views/cells/cell_slider.hpp"

#include <aurore/core/i18n.hpp>

using namespace aurore::literals;

namespace aurore
{

SliderCell::SliderCell()
{
    this->setFocusable(false);
    title->setGrow(0);
    detail->setWidth(50);
    slider = new Slider();
    slider->setGrow(1);
    slider->setMarginRight(16);
    slider->setFocusable(true);
    this->addView(slider, 1);
}

void SliderCell::init(const std::string& title, float init, const std::function<void(float)>& callback)
{
    this->title->setText(title);
    this->slider->setProgress(init);
    this->slider->getProgressEvent()->subscribe(callback);
}

Event<float>* SliderCell::getEvent()
{
    return this->slider->getProgressEvent();
}

View* SliderCell::getDefaultFocus()
{
    if (this->getVisibility() != Visibility::VISIBLE)
        return nullptr;

    return slider->getDefaultFocus();
}

View* SliderCell::create()
{
    return new SliderCell();
}

} // namespace aurore
