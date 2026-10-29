// SPDX-FileCopyrightText: 2019 WerWolv
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-FileCopyrightText: 2023 xfangfang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <string>

namespace aurore
{

enum KeyboardKeyDisableBitmask
{
    KEYBOARD_DISABLE_NONE         = 0,
    KEYBOARD_DISABLE_SPACE        = 1,
    KEYBOARD_DISABLE_AT           = 1 << 1,
    KEYBOARD_DISABLE_PERCENT      = 1 << 2,
    KEYBOARD_DISABLE_FORWSLASH    = 1 << 3,
    KEYBOARD_DISABLE_BACKSLASH    = 1 << 4,
    KEYBOARD_DISABLE_NUMBERS      = 1 << 5,
    KEYBOARD_DISABLE_DOWNLOADCODE = 1 << 6,
    KEYBOARD_DISABLE_USERNAME     = 1 << 7,
};

class ImeManager
{
  public:
    virtual ~ImeManager() { }

    virtual bool openForText(std::function<void(std::string)> f, std::string headerText = "",
        std::string subText = "", int maxStringLength = 32, std::string initialText = "",
        int kbdDisableBitmask = KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE)
        = 0;

    /**
     * The keyboard for a password or a PIN: what is typed is not shown
     * (the Switch's password keyboard). Where the platform has no such
     * keyboard, the text one.
     */
    virtual bool openForPassword(std::function<void(std::string)> f, std::string headerText = "",
        std::string subText = "", int maxStringLength = 32, std::string initialText = "",
        int kbdDisableBitmask = KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE)
    {
        return openForText(f, headerText, subText, maxStringLength, initialText, kbdDisableBitmask);
    }

    virtual bool openForNumber(std::function<void(long)> f, std::string headerText = "",
        std::string subText = "", int maxStringLength = 18, std::string initialText = "",
        std::string leftButton = "", std::string rightButton = "",
        int kbdDisableBitmask = KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE)
        = 0;
};

};