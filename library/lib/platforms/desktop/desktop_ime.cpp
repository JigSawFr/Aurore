// SPDX-FileCopyrightText: 2019 WerWolv
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-FileCopyrightText: 2023 xfangfang
// SPDX-License-Identifier: Apache-2.0

#include <cstring>
#include <iostream>
#include <aurore/core/logger.hpp>
#include <aurore/platforms/desktop/desktop_ime.hpp>

namespace aurore
{

static std::string terminalInput(std::string text)
{
    printf("\033[0;94m[INPUT] \033[0;36m%s\033[0m: ", text.c_str());
    std::string line;
    std::getline(std::cin, line);
    return line;
}

bool DesktopImeManager::openForText(std::function<void(std::string)> f, std::string headerText,
    std::string subText, int maxStringLength, std::string initialText,
    int kbdDisableBitmask)
{
    std::string line = terminalInput(headerText);
    f(line);
    return true;
}

bool DesktopImeManager::openForNumber(std::function<void(long)> f, std::string headerText,
    std::string subText, int maxStringLength, std::string initialText,
    std::string leftButton, std::string rightButton,
    int kbdDisableBitmask)
{
    std::string line = terminalInput(headerText);

    try
    {
        f(stoll(line));
        return true;
    }
    catch (const std::exception& e)
    {
        Logger::error("Could not parse input, did you enter a valid integer?");
        return false;
    }
}

};