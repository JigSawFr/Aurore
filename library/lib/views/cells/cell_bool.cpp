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

#include "borealis/views/cells/cell_bool.hpp"

#include <borealis/core/application.hpp>
#include <borealis/core/i18n.hpp>

using namespace brls::literals;

namespace brls
{

BooleanCell::BooleanCell()
{
    baseDetailTextSize = detail->getFontSize();
    setOn(false, false);
    this->registerClickAction([this](View* view)
        {
        this->setOn(!state);
        this->event.fire(state);
        return true; });
}

void BooleanCell::init(std::string title, bool isOn, std::function<void(bool)> callback)
{
    this->title->setText(title);
    setOn(isOn, false);
    getEvent()->subscribe(callback);
}

void BooleanCell::setOn(bool on, bool animated)
{
    this->state = on;

    // Cancel any previous bounce without letting its completion callback
    // mutate the view during a new state change. Its size is put back only
    // if a bounce was under way; otherwise the size it has now is the one to
    // keep (a style or XML may have set it since the cell was made).
    bool bouncing = scale.isRunning();
    scale.setEndCallback([](bool finished) {});
    scale.setTickCallback([]() {});
    scale.stop();
    scale.reset(1.0f);
    if (bouncing)
        detail->setFontSize(baseDetailTextSize);
    else
        baseDetailTextSize = detail->getFontSize();

    // Reduced motion: no bounce
    if (!animated || Application::getReduceMotion())
    {
        updateUI();
        return;
    }

    scale.addStep(0.8f, 60, EasingFunction::quadraticOut);
    scale.setTickCallback([this]
        { this->scaleTick(); });
    scale.setEndCallback([this](bool finished)
        {
        if (!finished)
            return;

        updateUI();

        scale.reset(0.8f);
        scale.addStep(1.0f, 60, EasingFunction::quadraticIn);
        scale.setTickCallback([this] {
            this->scaleTick();
        });
        scale.setEndCallback([this](bool finished) {
            if (!finished)
                return;

            detail->setFontSize(baseDetailTextSize);
        });
        scale.start(); });
    scale.start();
}

void BooleanCell::updateUI()
{
    detail->setText(state ? "hints/on"_i18n : "hints/off"_i18n);
    detail->setThemeColor("textColor", state ? "brls/list/listItem_value_color" : "brls/text_secondary");
}

void BooleanCell::scaleTick()
{
    detail->setFontSize(baseDetailTextSize * scale);
}

View* BooleanCell::create()
{
    return new BooleanCell();
}

} // namespace brls
