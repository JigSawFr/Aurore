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

// The New 3DS's video context, without a renderer yet: nanovg runs (paths,
// text measured, the font atlas made) but nothing reaches the screens. The
// citro3d renderer takes its place in a later step (docs/platforms.md).

#pragma once

#include <aurore/core/video.hpp>

namespace aurore
{

class CtrVideoContext : public VideoContext
{
  public:
    CtrVideoContext();
    ~CtrVideoContext() override;

    void clear(NVGcolor color) override { }
    void beginFrame() override { }
    void endFrame() override { }
    void resetState() override { }
    double getScaleFactor() override { return 1.0; }
    NVGcontext* getNVGContext() override { return this->nvgContext; }
    void setSwapInterval(int interval) override { }

  private:
    NVGcontext* nvgContext = nullptr;
};

} // namespace aurore
