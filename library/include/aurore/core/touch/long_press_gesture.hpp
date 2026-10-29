// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <aurore/core/gesture.hpp>
#include <aurore/core/time.hpp>

namespace aurore
{

// Long press recognizer: a touch (or the mouse's left button) held still
// on the view for `duration` ms. (The right button is B on desktop.)
// UNSURE: while it is held, not long enough yet
// START: held long enough: `respond` runs, and the other recognizers (the
//        tap under it, a scroll) are interrupted
// END: released after START
// FAILED: released too soon, or moved more than `slop` px away
class LongPressGestureRecognizer : public GestureRecognizer
{
  public:
    LongPressGestureRecognizer(View* view, std::function<void()> respond, Time duration = 500, float slop = 16);

    GestureState recognitionLoop(TouchState touch, MouseState mouse, View* view, Sound* soundToPlay) override;

  private:
    std::function<void()> respond;
    Time duration;
    float slop;

    Time pressedAt = 0;
    Point position;
};

} // namespace aurore
