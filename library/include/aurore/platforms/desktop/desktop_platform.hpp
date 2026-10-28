/*
    Copyright 2021 natinusala
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

#pragma once

#include <aurore/core/platform.hpp>
#include <aurore/platforms/desktop/desktop_font.hpp>
#include <aurore/platforms/desktop/desktop_ime.hpp>
#if defined(__linux__) and !defined(__SDL2__)
#include <dbus/dbus.h>
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(IOS) || defined(TVOS)
#elif defined(__APPLE__)
#include <IOKit/pwr_mgt/IOPMLib.h>
#endif

namespace brls
{

class DesktopPlatform : public Platform
{
  public:
    DesktopPlatform();
    ~DesktopPlatform() override;

    /// The environment variable `BRLS_` + `name`, else `BOREALIS_` + `name`
    /// (the older name): nullptr when neither is set.
    static const char* environment(const std::string& name);

    /// The locale BRLS_LANG (or BOREALIS_LANG) asks for: a locale's name as
    /// it is ("zh-Hant", an app's own), else read as a system's
    /// ("fr_FR.UTF-8" gives "fr"), never a path. False when neither is set:
    /// the system's language is used.
    static bool localeFromEnvironmentVariable(std::string* locale);

    std::string getName() override;

    ThemeVariant getThemeVariant() override;
    void setThemeVariant(ThemeVariant theme) override;
    std::string getLocale() override;
    ImeManager* getImeManager() override;

    FontLoader* getFontLoader() override;
    bool canShowBatteryLevel() override;
    bool canShowWirelessLevel() override;
    int getBatteryLevel() override;
    bool isBatteryCharging() override;
    bool hasWirelessConnection() override;
    int getWirelessLevel() override;
    bool hasEthernetConnection() override;
    void disableScreenDimming(bool disable, const std::string& reason, const std::string& app) override;
    bool runLoop(const std::function<bool()>& runLoopImpl) override;
    bool isScreenDimmingDisabled() override;
    void setBacklightBrightness(float brightness) override;
    float getBacklightBrightness() override;
    bool canSetBacklightBrightness() override;
    std::string getIpAddress() override;
    std::string getDnsServer() override;
    bool isApplicationMode() override;
    void exitToHomeMode(bool value) override;
    void forceEnableGamePlayRecording() override;
    void openBrowser(std::string url) override;

#if (defined(__APPLE__) || defined(__linux__)) && !defined(ANDROID) && !defined(IOS) && !defined(TVOS)
    /**
     * The command openBrowser() runs for this url: xdg-open or open, with the
     * url as one argument; empty if isBrowserUrl() refuses it
     */
    static std::vector<std::string> browserCommand(const std::string& url);

    /**
     * Starts args[0], looked up in PATH, with these arguments and no shell
     * @return the child's pid, or -1
     */
    static int spawnProcess(const std::vector<std::string>& args);
#endif

    inline static bool RESTART_APP       = false;
    inline static std::string GAMEPAD_DB = BRLS_ASSET("gamepad/gamecontrollerdb.txt");

  protected:
    DesktopFontLoader* fontLoader = nullptr;
    ThemeVariant themeVariant     = ThemeVariant::LIGHT;
    DesktopImeManager* imeManager = nullptr;
    std::string locale;
    /// BRLS_LANG (or BOREALIS_LANG) gave the locale: the system's is not looked for
    bool localeFromEnvironment = false;
    bool screenDimmingDisabled = false;
#ifdef __linux__
    uint32_t inhibitCookie = 0;
#ifndef __SDL2__
    std::string inhibitRequestPath;
#endif
#elif defined(_WIN32)
    HANDLE hLCD;
#elif defined(IOS) || defined(TVOS)
#elif defined(__APPLE__)
    IOPMAssertionID assertionID = 0;
#endif
};

} // namespace brls
