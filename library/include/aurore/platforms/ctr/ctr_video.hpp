// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// The New 3DS's video context: citro3d on the bottom screen (the touch
// screen, 320x240), through nanovg's citro3d renderer. The top screen shows
// libctru's text console for now, where the log goes; a header drawn there
// comes later (docs/platforms.md).

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

    CtrVideoContext();
    ~CtrVideoContext() override;

    void clear(NVGcolor color) override;
    void beginFrame() override;
    void endFrame() override;
    void resetState() override { }
    double getScaleFactor() override { return 1.0; }
    NVGcontext* getNVGContext() override { return this->nvgContext; }
    // citro3d waits for the vertical blank at each frame
    void setSwapInterval(int interval) override { }

  private:
    NVGcontext* nvgContext   = nullptr;
    C3D_RenderTarget* target = nullptr;
    bool inFrame             = false;
};

} // namespace aurore
