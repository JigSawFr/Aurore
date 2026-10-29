// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// The New 3DS's video context: citro3d on both screens, through nanovg's
// citro3d renderer. The app on the bottom one (the touch screen, 320x240),
// the activity's top screen view on the top one (400x240, TopScreen), each
// its render target, in the same frame (docs/platforms.md).

#pragma once

#include <3ds.h>
#include <citro3d.h>

#include <aurore/core/video.hpp>

namespace aurore
{

class CtrVideoContext : public VideoContext
{
  public:
    // The bottom screen, landscape
    static constexpr int WIDTH  = 320;
    static constexpr int HEIGHT = 240;
    // The top screen, landscape
    static constexpr int TOP_WIDTH  = 400;
    static constexpr int TOP_HEIGHT = 240;

    CtrVideoContext();
    ~CtrVideoContext() override;

    void clear(NVGcolor color) override;
    void beginFrame() override;
    void endFrame() override;
    bool beginTopScreen(NVGcolor color) override;
    void resetState() override { }
    double getScaleFactor() override { return 1.0; }
    NVGcontext* getNVGContext() override { return this->nvgContext; }
    // citro3d waits for the vertical blank at each frame
    void setSwapInterval(int interval) override { }

  private:
    NVGcontext* nvgContext      = nullptr;
    C3D_RenderTarget* target    = nullptr;
    C3D_RenderTarget* topTarget = nullptr;
    bool inFrame                = false;
};

} // namespace aurore
