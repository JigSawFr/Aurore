// SPDX-FileCopyrightText: 2019 natinusala
// SPDX-FileCopyrightText: 2019 psam
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <nanovg.h>

#include <aurore/core/font.hpp>
#include <aurore/core/style.hpp>
#include <aurore/core/theme.hpp>

namespace aurore
{

class FrameContext
{
  public:
    NVGcontext* vg       = nullptr;
    float pixelRatio     = 0.0;
    FontStash* fontStash = nullptr;
    Theme theme          = nullptr;
};

} // namespace aurore
