// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/platforms/sdl/sdl.hpp>

#include <aurore/core/video.hpp>

namespace aurore
{

// SDL Video Context
class SDLVideoContext : public VideoContext
{
  public:
    SDLVideoContext(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos);
    ~SDLVideoContext() override;

    NVGcontext* getNVGContext() override;

    void clear(NVGcolor color) override;
    void beginFrame() override;
    void endFrame() override;
    void setSwapInterval(int interval) override;
    void resetState() override;
    void fullScreen(bool fs) override;

    SDL_Window* getSDLWindow();
    /// SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: the window keeps its size in
    /// points where SDL3 sizes it in pixels (Windows, X11)
    void displayScaleChanged();

    double getScaleFactor() override;
    bool readPixels(std::vector<unsigned char>* rgba, int* width, int* height) override;

  private:
    SDL_Window* window     = nullptr;
    NVGcontext* nvgContext = nullptr;
    float displayScale     = 0.0f; // SDL_GetWindowDisplayScale, last seen
};

} // namespace aurore
