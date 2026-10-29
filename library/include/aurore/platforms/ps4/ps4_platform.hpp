// SPDX-FileCopyrightText: 2023 xfangfang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/platforms/sdl/sdl_platform.hpp>

namespace aurore
{

class Ps4Platform : public SDLPlatform
{
  public:
    Ps4Platform();
    ~Ps4Platform() override;
    std::string getName() override;
    void createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos) override;
    bool canShowWirelessLevel() override;
    bool hasWirelessConnection() override;
    int getWirelessLevel() override;
    bool hasEthernetConnection() override;
    std::string getIpAddress() override;
    std::string getDnsServer() override;
    void openBrowser(std::string url) override;
    ImeManager* getImeManager() override;

    // ps4 specific
    int loadStartModuleFromSandbox(const std::string& name);
    int loadStartModule(const std::string& path);
    int moduleDlsym(int handle, const std::string& name, void** func);

#ifdef USE_JBC
  private:
    static void grantRootAccess();
    static void exitRootAccess();
    static bool hasRootAccess();
#endif

  private:
    ImeManager* imeManager = nullptr;
};

} // namespace aurore
