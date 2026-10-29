// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <nanovg.h>

#include <cstdint>
#include <string>

namespace aurore
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
    DIALOG, // said with its text when the focus enters it
};

// "button", "toggle"… (the XML names); "" for NONE.
std::string accessibilityRoleName(AccessibilityRole role);

// The words the descriptions are made of, in the app's language
// (resources/i18n/<language>/a11y.json, English when the app has none): a
// role as said ("switch"; "" for NONE, a label, a cell), a state ("on",
// "off", "selected", "disabled"), a share of 0..1 as "40%".
std::string accessibilityRoleWord(AccessibilityRole role);
std::string accessibilityStateWord(const std::string& state);
std::string accessibilityPercent(float fraction);

} // namespace aurore
