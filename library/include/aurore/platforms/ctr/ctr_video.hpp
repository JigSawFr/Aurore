/*
    Copyright 2026 JigSawFr

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
