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

#include "aurore/views/cells/cell_input.hpp"

#include "aurore/views/dropdown.hpp"

#include <algorithm>

namespace aurore
{

InputCell::InputCell()
{
    detail->setThemeColor("textColor", "aurore/list/listItem_value_color");

    this->registerClickAction([this](View* view)
        {
            auto done = [this](std::string text) { this->setValue(text); };
            if (this->secure)
                Application::getImeManager()->openForPassword(done,
                    this->title->getFullText(), this->hint, this->maxInputLength, "", this->kbdDisableBitmask);
            else
                Application::getImeManager()->openForText(done,
                    this->title->getFullText(), this->hint, this->maxInputLength, this->value, this->kbdDisableBitmask);

        return true; });

    this->registerBoolXMLAttribute("secure", [this](bool value)
        { this->setSecure(value); });
}

void InputCell::setSecure(bool secure)
{
    this->secure = secure;
    updateUI();
}

void InputCell::init(std::string title, std::string value, Event<std::string>::Callback callback, std::string placeholder, std::string hint, int maxInputLength, int kbdDisableBitmask)
{
    this->hint  = hint;
    this->value = value;
    this->title->setText(title);
    this->placeholder       = placeholder;
    this->maxInputLength    = maxInputLength;
    this->kbdDisableBitmask = kbdDisableBitmask;
    this->event.subscribe(callback);
    updateUI();
}

void InputCell::setValue(std::string value)
{
    this->event.fire(value);
    this->value = value;
    updateUI();
}

void InputCell::setPlaceholder(std::string placeholder)
{
    this->placeholder = placeholder;
    updateUI();
}

void InputCell::updateUI()
{
    if (this->value.empty())
    {
        this->detail->setText(placeholder);
        this->detail->setThemeColor("textColor", "aurore/text_secondary");
    }
    else
    {
        if (this->secure)
        {
            // One dot a character (a code point), at most 12
            size_t chars = 0;
            for (unsigned char c : this->value)
                if ((c & 0xC0) != 0x80)
                    chars++;
            std::string dots;
            for (size_t i = 0; i < std::min<size_t>(chars, 12); i++)
                dots += "\u2022";
            this->detail->setText(dots);
        }
        else
        {
            this->detail->setText(value);
        }
        this->detail->setThemeColor("textColor", "aurore/list/listItem_value_color");
    }
}

View* InputCell::create()
{
    return new InputCell();
}

InputNumericCell::InputNumericCell()
{
    detail->setThemeColor("textColor", "aurore/list/listItem_value_color");

    this->registerClickAction([this](View* view)
        {
        Application::getImeManager()->openForNumber([&](long number) {
            this->setValue(number);
        },
            this->title->getFullText(), this->hint, this->maxInputLength, std::to_string(this->value), "", "", this->kbdDisableBitmask);

        return true; });
}

void InputNumericCell::init(std::string title, long value, Event<long>::Callback callback, std::string hint, int maxInputLength, int kbdDisableBitmask)
{
    this->hint  = hint;
    this->value = value;
    this->title->setText(title);
    this->maxInputLength    = maxInputLength;
    this->kbdDisableBitmask = kbdDisableBitmask;
    this->event.subscribe(callback);
    updateUI();
}

void InputNumericCell::setValue(long value)
{
    this->event.fire(value);
    this->value = value;
    updateUI();
}

void InputNumericCell::updateUI()
{
    this->detail->setText(std::to_string(value));
    this->detail->setThemeColor("textColor", "aurore/list/listItem_value_color");
}

View* InputNumericCell::create()
{
    return new InputNumericCell();
}

} // namespace aurore
