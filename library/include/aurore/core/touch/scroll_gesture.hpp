// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/touch/pan_gesture.hpp>

namespace aurore
{

/*
 * Scroll recognizer
 *
 * Child of Pan recognizer.
 * The only difference is that Scroll recognizer will translate scrolling wheel to pan gestures.
 * If NO_TOUCH_SCROLLING=true defined, will ignore scrolling by touch.
 *
 * If mouse translation used, the only available state is MOVE.
 * Also PanGestureStatus will be returned with delta values ONLY.
 *
 * TODO: Reimplement scroll events when mouse input will be separated from touch
 */
class ScrollGestureRecognizer : public PanGestureRecognizer
{
  public:
    ScrollGestureRecognizer(PanGestureEvent::Callback respond, PanAxis axis);
    GestureState recognitionLoop(TouchState touch, MouseState mouse, View* view, Sound* soundToPlay) override;
};

} // namespace aurore
