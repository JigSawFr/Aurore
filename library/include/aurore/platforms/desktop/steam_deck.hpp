// SPDX-FileCopyrightText: 2024 xfangfang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>

#include <string>

namespace aurore
{

/**
 * Call Steam client internal command
 * @param cmd example: steam://close/keyboard
 */
void runSteamDeckCommand(const std::string& cmd);

/**
 * The steam://openurl command for this url, empty if
 * Platform::isBrowserUrl() refuses it
 */
std::string steamOpenUrlCommand(const std::string& url);

/**
 * return true if we are running in SteamDeck game mode
 */
bool isSteamDeck();

bool isSteamDeckBrightnessSupported();

float getSteamDeckBrightness();

void setSteamDeckBrightness(float value);

}
#endif