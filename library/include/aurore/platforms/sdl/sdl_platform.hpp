// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/platforms/sdl/sdl.hpp>

#include <aurore/platforms/desktop/desktop_platform.hpp>
#include <aurore/platforms/sdl/sdl_ime.hpp>
#include <aurore/platforms/sdl/sdl_input.hpp>
#include <aurore/platforms/sdl/sdl_video.hpp>

namespace aurore
{

class SDLPlatform : public DesktopPlatform
{
  public:
    SDLPlatform();
    ~SDLPlatform() override;

    std::string getName() override;
    void createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos) override;

    void restoreWindow() override;
    void setWindowSize(uint32_t windowWidth, uint32_t windowHeight) override;
    void setWindowSizeLimits(uint32_t windowMinWidth, uint32_t windowMinHeight, uint32_t windowMaxWidth, uint32_t windowMaxHeight) override;
    void setWindowPosition(int windowXPos, int windowYPos) override;
    void setWindowState(uint32_t windowWidth, uint32_t windowHeight, int windowXPos, int windowYPos) override;
    void disableScreenDimming(bool disable, const std::string& reason, const std::string& app) override;
    bool isScreenDimmingDisabled() override;
#if !defined(IOS) && !defined(TVOS) && !defined(__EMSCRIPTEN__)
    // The battery as SDL reads it (SDL_GetPowerInfo): Linux's too, which the
    // desktop platform does not read. Not on iOS, where SDL would read
    // UIDevice off the main thread (BatteryWidget reads it on a task), nor in
    // a browser, where it would be the visitor's device's, not the app's
    bool canShowBatteryLevel() override;
    int getBatteryLevel() override;
    bool isBatteryCharging() override;
#endif

    void setWindowAlwaysOnTop(bool enable) override;
    void pasteToClipboard(const std::string& text) override;
    std::string pasteFromClipboard() override;
    bool mainLoopIteration() override;

    AudioPlayer* getAudioPlayer() override;
    VideoContext* getVideoContext() override;
    InputManager* getInputManager() override;
    ImeManager* getImeManager() override;
    bool processEvent(SDL_Event* event);
#if defined(IOS) || defined(TVOS)
    Insets getSafeAreaInsets() override;
#endif

  protected:
    NullAudioPlayer* audioPlayer  = nullptr;
    SDLVideoContext* videoContext = nullptr;
    SDLInputManager* inputManager = nullptr;
    SDLImeManager* imeManager     = nullptr;
    Event<SDL_Event*> otherEvent;

#ifdef __SDL3__
    /// The system's light or dark (systemThemeChanged)
    void readSystemTheme();
#endif

  private:
    // The safe area read again now and once the system settled (a rotation)
    void safeAreaMayHaveChanged();
    size_t safeAreaCheck = 0;
};

} // namespace aurore
