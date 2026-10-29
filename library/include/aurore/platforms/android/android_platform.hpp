// SPDX-FileCopyrightText: 2023 xfangfang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/platforms/sdl/sdl_platform.hpp>

namespace aurore
{

class AndroidPlatform : public SDLPlatform
{
  public:
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
    float getBacklightBrightness() override;
    void setBacklightBrightness(float brightness) override;
    bool canSetBacklightBrightness() override;
    Insets getSafeAreaInsets() override;
};

} // namespace aurore
