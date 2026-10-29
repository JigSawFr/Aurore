// SPDX-FileCopyrightText: 2023 xfangfang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef AURORE_USE_GXM
#include <aurore/platforms/desktop/desktop_platform.hpp>
#include <aurore/platforms/psv/psv_ime.hpp>
#include <aurore/platforms/psv/psv_input.hpp>
#include <aurore/platforms/psv/psv_video.hpp>
#else
#include <aurore/platforms/sdl/sdl_platform.hpp>
#endif

namespace aurore
{

#ifdef AURORE_USE_GXM
class PsvPlatform : public DesktopPlatform
#else
class PsvPlatform : public SDLPlatform
#endif
{
  public:
    PsvPlatform();
    ~PsvPlatform() override;
    std::string getName() override;
    void createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos) override;
    bool canShowBatteryLevel() override;
    bool canShowWirelessLevel() override;
    int getBatteryLevel() override;
    bool isBatteryCharging() override;
    bool hasWirelessConnection() override;
    int getWirelessLevel() override;
    bool hasEthernetConnection() override;
    std::string getIpAddress() override;
    std::string getDnsServer() override;
    void openBrowser(std::string url) override;
    void setBacklightBrightness(float brightness) override;
    float getBacklightBrightness() override;
    bool canSetBacklightBrightness() override;
    bool isScreenDimmingDisabled() override;
    void disableScreenDimming(bool disable, const std::string& reason, const std::string& app) override;
    bool mainLoopIteration() override;
    InputManager* getInputManager() override;
    ImeManager* getImeManager() override;
    VideoContext* getVideoContext() override;
    AudioPlayer* getAudioPlayer() override;

  private:
    bool suspendDisabled {};
#ifdef AURORE_USE_GXM
    PsvInputManager* inputManager = nullptr;
    PsvImeManager* imeManager     = nullptr;
    PsvVideoContext* videoContext = nullptr;
    NullAudioPlayer* audioPlayer  = nullptr;
#endif
};

} // namespace aurore
