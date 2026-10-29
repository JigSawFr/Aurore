// SPDX-FileCopyrightText: 2024 xfangfang
// SPDX-License-Identifier: Apache-2.0

#ifdef __linux__

#include <aurore/core/logger.hpp>
#include <aurore/core/platform.hpp>
#include <aurore/platforms/desktop/steam_deck.hpp>

namespace aurore
{

#define STEAM_CLIENT_PIPE_PATH "/home/deck/.steam/steam.pipe"
#define STEAM_DECK_BACKLIGHT_PATH "/sys/class/backlight/amdgpu_bl0/brightness"
#define STEAM_DECK_MAX_BRIGHTNESS 4095

void runSteamDeckCommand(const std::string& cmd)
{
    if (!isSteamDeck())
        return;
    // One command per line: a line break or control character inside would
    // start another one
    std::string line = cmd;
    if (!line.empty() && line.back() == '\n')
        line.pop_back();
    if (line.empty())
        return;
    for (unsigned char c : line)
    {
        if (c < 0x20 || c == 0x7f)
        {
            Logger::warning("steam.pipe: refused a command with a control character");
            return;
        }
    }
    line += '\n';
    int fd = open(STEAM_CLIENT_PIPE_PATH, O_WRONLY | O_NONBLOCK);
    if (fd < 0)
    {
        Logger::warning("Cannot open steam.pipe");
    }
    else
    {
        write(fd, line.c_str(), line.size());
        close(fd);
    }
}

std::string steamOpenUrlCommand(const std::string& url)
{
    if (!Platform::isBrowserUrl(url))
        return "";
    return "steam://openurl/" + url + "\n";
}

bool isSteamDeck()
{
    static bool isSteamDeck = getenv("SteamDeck");
    return isSteamDeck;
}

bool isSteamDeckBrightnessSupported()
{
    if (!isSteamDeck())
        return false;
    static bool tested = false, value = false;
    if (tested)
        return value;
    int fd = open(STEAM_DECK_BACKLIGHT_PATH, O_RDWR | O_NONBLOCK);
    if (fd < 0)
    {
        Logger::warning("Backlight control is not supported");
        tested = true;
        value  = false;
        return false;
    }
    else
    {
        close(fd);
        Logger::info("Backlight control is supported");
        tested = true;
        value  = true;
        return true;
    }
}

float getSteamDeckBrightness()
{
    if (!isSteamDeck())
        return 0.0f;
    int fd = open(STEAM_DECK_BACKLIGHT_PATH, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        Logger::warning("Backlight control is not supported");
        return 0.0f;
    }
    else
    {
        char brightness[16];
        read(fd, brightness, sizeof(brightness));
        close(fd);
        return atoi(brightness) * 1.0f / STEAM_DECK_MAX_BRIGHTNESS;
    }
}

void setSteamDeckBrightness(float value)
{
    if (!isSteamDeck())
        return;
    int fd = open(STEAM_DECK_BACKLIGHT_PATH, O_WRONLY | O_NONBLOCK);
    if (fd < 0)
    {
        Logger::warning("Backlight control is not supported");
    }
    else
    {
        int b                  = value * STEAM_DECK_MAX_BRIGHTNESS;
        std::string brightness = std::to_string(b);
        write(fd, brightness.c_str(), brightness.size());
        close(fd);
    }
}

}

#endif