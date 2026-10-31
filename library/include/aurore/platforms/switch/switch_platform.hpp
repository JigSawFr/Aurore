// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef AURORE_USE_DEKO3D
#include <aurore/platforms/switch/switch_video.hpp>
#elif defined(__SDL2__)
#include <aurore/platforms/sdl/sdl_video.hpp>
#else
#include <aurore/platforms/glfw/glfw_video.hpp>
#endif
#include <aurore/core/audio.hpp>
#include <aurore/core/platform.hpp>
#include <aurore/core/theme.hpp>
#include <aurore/platforms/switch/switch_font.hpp>
#include <aurore/platforms/switch/switch_ime.hpp>
#include <aurore/platforms/switch/switch_input.hpp>

namespace aurore
{

class SwitchPlatform : public Platform
{
  public:
    SwitchPlatform();
    ~SwitchPlatform() override;

    void createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos) override;

    std::string getName() override;

    bool mainLoopIteration() override;
    ThemeVariant getThemeVariant() override;
    void setThemeVariant(ThemeVariant theme) override;
    void followSystemTheme() override;
    std::string getLocale() override;
    ImeManager* getImeManager() override;

    VideoContext* getVideoContext() override;
    AudioPlayer* getAudioPlayer() override;
    InputManager* getInputManager() override;
    FontLoader* getFontLoader() override;
    bool canShowBatteryLevel() override;
    int getBatteryLevel() override;
    bool isBatteryCharging() override;
    bool hasWirelessConnection() override;
    bool hasEthernetConnection() override;
    int getWirelessLevel() override;
    void disableScreenDimming(bool disable, const std::string& reason, const std::string& app) override;
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

    void appletCallback(AppletHookType hookType);

  private:
    ThemeVariant themeVariant;
    ThemeVariant systemThemeVariant; // the console's, read at start
    std::string locale;

    AudioPlayer* audioPlayer         = nullptr;
    SwitchInputManager* inputManager = nullptr;
    SwitchImeManager* imeManager     = nullptr;

#ifdef AURORE_USE_DEKO3D
    SwitchVideoContext* videoContext = nullptr; // set once the context is built: its own constructor reads it
#elif defined(__SDL2__)
    SDLVideoContext* videoContext = nullptr;
#else
    GLFWVideoContext* videoContext = nullptr;
#endif
    SwitchFontLoader* fontLoader = nullptr;
    AppletHookCookie applet_hook_cookie;
};

} // namespace aurore
