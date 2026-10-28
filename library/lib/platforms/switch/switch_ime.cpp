/*
    Copyright 2019  WerWolv
    Copyright 2019  p-sam
    Copyright 2023 xfangfang

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

#include <switch.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>
#include <aurore/core/logger.hpp>
#include <aurore/platforms/switch/switch_ime.hpp>

namespace aurore
{

static SwkbdConfig createSwkbdBaseConfig(std::string headerText, std::string subText, int maxStringLength, std::string initialText)
{
    SwkbdConfig config;

    swkbdCreate(&config, 0);

    swkbdConfigMakePresetDefault(&config);
    swkbdConfigSetHeaderText(&config, headerText.c_str());
    swkbdConfigSetSubText(&config, subText.c_str());
    swkbdConfigSetStringLenMax(&config, maxStringLength);
    swkbdConfigSetInitialText(&config, initialText.c_str());
    swkbdConfigSetBlurBackground(&config, true);

    return config;
}

// Room for what swkbd writes back: maxStringLength characters in UTF-8 (4
// bytes at most each) and the NUL; 500, the longest swkbd edits, when no
// length is set
static size_t textBufferSize(int maxStringLength)
{
    size_t characters = maxStringLength > 0 ? (size_t)maxStringLength : 500;
    return std::max<size_t>(0x100, characters * 4 + 1);
}

int getSwkbdKeyDisableBitmask(int aurore_key)
{
    // translate aurore to Switch libnx values
    int ret = 0;
    if (aurore_key == aurore::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE)
        return 0;

    if (aurore_key & aurore::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_SPACE)
        // Disable space-bar
        ret |= SwkbdKeyDisableBitmask_Space;

    if (aurore_key & aurore::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_AT)
        // Disable '@'.
        ret |= SwkbdKeyDisableBitmask_At;

    if (aurore_key & aurore::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_PERCENT)
        // Disable '%'.
        ret |= SwkbdKeyDisableBitmask_Percent;

    if (aurore_key & aurore::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_FORWSLASH)
        // Disable '/'.
        ret |= SwkbdKeyDisableBitmask_ForwardSlash;

    if (aurore_key & aurore::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_BACKSLASH)
        // Disable '\'.
        ret |= SwkbdKeyDisableBitmask_Backslash;

    if (aurore_key & aurore::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NUMBERS)
        // Disable numbers.
        ret |= SwkbdKeyDisableBitmask_Numbers;

    if (aurore_key & aurore::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_DOWNLOADCODE)
        // Used for swkbdConfigMakePresetDownloadCode.
        ret |= SwkbdKeyDisableBitmask_DownloadCode;

    if (aurore_key & aurore::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_USERNAME)
        // Used for swkbdConfigMakePresetUserName. Disables '@', '%', and '\'.
        ret |= SwkbdKeyDisableBitmask_UserName;

    return ret;
}

bool SwitchImeManager::openForText(std::function<void(std::string)> f, std::string headerText,
    std::string subText, int maxStringLength, std::string initialText,
    int kbdDisableBitmask)
{
    SwkbdConfig config = createSwkbdBaseConfig(headerText, subText, maxStringLength, initialText);

    swkbdConfigSetType(&config, SwkbdType_All);
    swkbdConfigSetKeySetDisableBitmask(&config, getSwkbdKeyDisableBitmask(kbdDisableBitmask));

    std::vector<char> buffer(textBufferSize(maxStringLength));

    if (R_SUCCEEDED(swkbdShow(&config, buffer.data(), buffer.size())))
    {
        f(buffer.data());

        swkbdClose(&config);
        return true;
    }

    swkbdClose(&config);

    return false;
}

bool SwitchImeManager::openForPassword(std::function<void(std::string)> f, std::string headerText,
    std::string subText, int maxStringLength, std::string initialText,
    int kbdDisableBitmask)
{
    SwkbdConfig config = createSwkbdBaseConfig(headerText, subText, maxStringLength, initialText);

    // The system's password keyboard: what is typed is hidden
    swkbdConfigMakePresetPassword(&config);
    swkbdConfigSetHeaderText(&config, headerText.c_str());
    swkbdConfigSetSubText(&config, subText.c_str());
    swkbdConfigSetStringLenMax(&config, maxStringLength);
    swkbdConfigSetKeySetDisableBitmask(&config, getSwkbdKeyDisableBitmask(kbdDisableBitmask));

    std::vector<char> buffer(textBufferSize(maxStringLength));
    bool shown = R_SUCCEEDED(swkbdShow(&config, buffer.data(), buffer.size()));

    swkbdClose(&config);

    if (!shown)
        return false;

    f(buffer.data());
    return true;
}

bool SwitchImeManager::openForNumber(std::function<void(long)> f, std::string headerText,
    std::string subText, int maxStringLength, std::string initialText,
    std::string leftButton, std::string rightButton,
    int kbdDisableBitmask)
{
    SwkbdConfig config = createSwkbdBaseConfig(headerText, subText, maxStringLength, initialText);

    swkbdConfigSetType(&config, SwkbdType_NumPad);
    swkbdConfigSetLeftOptionalSymbolKey(&config, leftButton.c_str());
    swkbdConfigSetRightOptionalSymbolKey(&config, rightButton.c_str());
    swkbdConfigSetKeySetDisableBitmask(&config, getSwkbdKeyDisableBitmask(kbdDisableBitmask));

    std::vector<char> buffer(textBufferSize(maxStringLength));

    bool shown = R_SUCCEEDED(swkbdShow(&config, buffer.data(), buffer.size()));

    swkbdClose(&config);

    if (!shown || strcmp(buffer.data(), "") == 0)
        return false;

    // The optional symbol keys can leave a text that is not a number ("-",
    // "."), or one too large for a long
    long value;
    try
    {
        value = std::stol(buffer.data());
    }
    catch (const std::exception& e)
    {
        Logger::warning("switch ime: \"{}\" is not a number: {}", buffer.data(), e.what());
        return false;
    }

    f(value);
    return true;
}

};