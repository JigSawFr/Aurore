/*
    Copyright 2026 JigSawFr

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

#include <nanovg.h>

#include <cstdint>
#include <string>

namespace brls
{

// The states a view can be styled for (focus:backgroundColor="…" in XML,
// View::setStateStyle in code). When several are on, the later one in this
// list wins for a property both style: hover, focus, pressed, disabled.
enum class StyleState : uint8_t
{
    HOVER    = 1 << 0, // the mouse pointer is over the view (desktop)
    FOCUS    = 1 << 1, // the view has the focus
    PRESSED  = 1 << 2, // A or a touch is held on the view (its click animation)
    DISABLED = 1 << 3, // set by the app (View::setStyleState), or a disabled Button
};

// A style property's value: a number (borderThickness, alpha…) or a colour
// (backgroundColor, textColor…).
struct StyleValue
{
    enum class Type : uint8_t
    {
        NONE,
        NUMBER,
        COLOR,
    };

    Type type      = Type::NONE;
    float number   = 0.0f;
    NVGcolor color = nvgRGBA(0, 0, 0, 0);

    static StyleValue of(float number)
    {
        StyleValue value;
        value.type   = Type::NUMBER;
        value.number = number;
        return value;
    }

    static StyleValue of(NVGcolor color)
    {
        StyleValue value;
        value.type  = Type::COLOR;
        value.color = color;
        return value;
    }

    bool operator==(const StyleValue& other) const;
    bool operator!=(const StyleValue& other) const { return !(*this == other); }

    // From `from` (t = 0) to `to` (t = 1), each colour channel on its own.
    static StyleValue mix(const StyleValue& from, const StyleValue& to, float t);
};

// What a view is, for an accessibility description (the focus announcer).
enum class AccessibilityRole : uint8_t
{
    NONE, // nothing said for the role
    BUTTON,
    LABEL,
    HEADER,
    IMAGE,
    CELL,
    TOGGLE,
    SLIDER,
    TEXT_FIELD,
    TAB,
    LIST,
};

// "button", "toggle"… (the XML names); "" for NONE.
std::string accessibilityRoleName(AccessibilityRole role);

} // namespace brls
