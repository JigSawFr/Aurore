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
    std::string locale;

    AudioPlayer* audioPlayer;
    SwitchInputManager* inputManager;
    SwitchImeManager* imeManager;

#ifdef AURORE_USE_DEKO3D
    SwitchVideoContext* videoContext;
#elif defined(__SDL2__)
    SDLVideoContext* videoContext;
#else
    GLFWVideoContext* videoContext;
#endif
    SwitchFontLoader* fontLoader;
    AppletHookCookie applet_hook_cookie;
};

} // namespace aurore
