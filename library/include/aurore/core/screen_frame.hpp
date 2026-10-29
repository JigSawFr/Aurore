// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// A frame around the app's screen: the window holds more than the screen,
// a console drawn around it for instance (the demo's simulated platforms).
// With one (Application::setScreenFrame), the app is laid out and drawn in
// the screen's rectangle only, the mouse and the touches are read there,
// and the frame is drawn over the rest of the window.

#pragma once

#include <nanovg.h>

#include <aurore/core/geometry.hpp>

namespace aurore
{

class ScreenFrame
{
  public:
    virtual ~ScreenFrame() = default;

    /// Where the screen goes in a window `width` x `height` pixels big
    virtual Rect layout(float width, float height) = 0;

    /// Draws the frame, in the window's pixels, after the app: over all of
    /// the window but `screen`
    virtual void draw(NVGcontext* vg, float width, float height, Rect screen) = 0;
};

} // namespace aurore
