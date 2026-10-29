// SPDX-FileCopyrightText: 2024 xfangfang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <nanovg_gxm_utils.h>
#include <aurore/core/video.hpp>

namespace aurore
{

class PsvVideoContext : public VideoContext
{
  public:
    PsvVideoContext();
    ~PsvVideoContext() override;

    void clear(NVGcolor color) override;
    void resetState() override;
    void beginFrame() override;
    void endFrame() override;
    void setSwapInterval(int interval) override;
    double getScaleFactor() override;
    NVGcontext* getNVGContext() override;

    NVGXMwindow* getWindow();

  private:
    NVGcontext* nvgContext;
    NVGXMwindow* window;
};

} // namespace aurore
