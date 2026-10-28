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
