/*
    Copyright 2019-2021 natinusala
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

#include <aurore/core/application.hpp>
#include <aurore/views/progress_spinner.hpp>

namespace aurore
{

ProgressSpinner::ProgressSpinner(ProgressSpinnerSize size)
    : size(size)
{
    AURORE_REGISTER_ENUM_XML_ATTRIBUTE("size", ProgressSpinnerSize, this->setSize,
        {
            { "normal", ProgressSpinnerSize::NORMAL },
            { "large", ProgressSpinnerSize::LARGE },
        });

    this->registerColorXMLAttribute("barColor", [this](NVGcolor color)
        { this->setColor(color); });
}

void ProgressSpinner::restartAnimation()
{
    Style style = Application::getStyle();

    this->animationValue.reset(0);
    this->animationValue.stop();
    this->animationValue.setEndCallback([this](bool done)
        {
        if (done)
            this->restartAnimation(); });
    float animationLength = size == NORMAL ? 8.0f : 12.0f;
    this->animationValue.addStep(animationLength, style["aurore/spinner/animation_duration"], EasingFunction::linear);
    this->animationValue.start();
}

void ProgressSpinner::animate(bool animate)
{
    if (animate)
    {
        this->animationValue.start();
    }
    else
    {
        this->animationValue.stop();
    }
}

void ProgressSpinner::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    NVGcolor baseColor = this->customBarColor ? this->barColor : Application::getTheme()["aurore/spinner/bar_color"];
    NVGcolor barColor  = baseColor;

    // Each bar of the spinner
    switch (size)
    {
        case NORMAL:
            for (int i = 0 + animationValue; i < 8 + animationValue; i++)
            {
                barColor.a = fmax((i - animationValue) / 8.0f, baseColor.a) * this->getAlpha();
                nvgSave(vg);
                nvgTranslate(vg, x + width / 2, y + height / 2);
                nvgRotate(vg, nvgDegToRad(i * 45)); // Internal angle of octagon
                nvgBeginPath(vg);
                nvgMoveTo(vg, height * style["aurore/spinner/center_gap_multiplier"], 0);
                nvgLineTo(vg, height / 2 - height * style["aurore/spinner/center_gap_multiplier"], 0);
                nvgStrokeColor(vg, barColor);
                nvgStrokeWidth(vg, height * style["aurore/spinner/bar_width_multiplier"]);
                nvgLineCap(vg, NVG_SQUARE);
                nvgStroke(vg);
                nvgRestore(vg);
            }
            break;
        case LARGE:
            for (int i = 0 + animationValue; i < 12 + animationValue; i++)
            {
                barColor.a = fmax((i - animationValue) / 12.0f, baseColor.a) * this->getAlpha();
                nvgSave(vg);
                nvgTranslate(vg, x + width / 2, y + height / 2);
                nvgRotate(vg, nvgDegToRad(i * 30)); // Internal angle of octagon
                nvgBeginPath(vg);
                nvgMoveTo(vg, height * style["aurore/spinner/center_gap_multiplier_large"], 0);
                nvgLineTo(vg, height / 2 - height * style["aurore/spinner/center_gap_multiplier_large"], 0);
                nvgStrokeColor(vg, barColor);
                nvgStrokeWidth(vg, height * style["aurore/spinner/bar_width_multiplier_large"]);
                nvgLineCap(vg, NVG_SQUARE);
                nvgStroke(vg);
                nvgRestore(vg);
            }
            break;
    }
}

void ProgressSpinner::willAppear(bool resetState)
{
    this->restartAnimation();
}

void ProgressSpinner::willDisappear(bool resetState)
{
    this->animationValue.stop();
}

void ProgressSpinner::setColor(NVGcolor color)
{
    this->barColor       = color;
    this->customBarColor = true;
}

bool ProgressSpinner::getStyleProperty(const std::string& name, StyleValue* value)
{
    if (name != "barColor")
        return View::getStyleProperty(name, value);
    *value = StyleValue::of(this->customBarColor ? this->barColor : Application::getTheme()["aurore/spinner/bar_color"]);
    return true;
}

bool ProgressSpinner::setStyleProperty(const std::string& name, const StyleValue& value)
{
    if (name != "barColor")
        return View::setStyleProperty(name, value);
    if (value.type != StyleValue::Type::COLOR)
        return false;
    this->setColor(value.color);
    return true;
}

aurore::View* ProgressSpinner::create()
{
    return new ProgressSpinner();
}

} // namespace aurore
