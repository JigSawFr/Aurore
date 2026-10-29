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

#include <deque>
#include <string>
#include <aurore.hpp>

/// A controller drawn with nanovg, its buttons lit as they are pressed and
/// its sticks leaning as they are pushed (Application::getControllerState:
/// a pad, or the keyboard's keys mapped to it). Focused, A starts a test
/// where every button is only shown, the focus staying; holding B for a
/// second ends it.
class ControllerView : public aurore::Box
{
  public:
    ControllerView();

    void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
        aurore::FrameContext* ctx) override;
    aurore::View* getNextFocus(aurore::FocusDirection direction, aurore::View* currentView) override;
    void onFocusLost() override;

    bool isTesting() const { return this->testing; }
    void setTesting(bool testing);

  protected:
    aurore::AccessibilityRole getDefaultAccessibilityRole() override { return aurore::AccessibilityRole::BUTTON; }

  private:
    bool testing            = false;
    aurore::Time bHeldSince = 0;
    aurore::Label* status;
};

/// A pad for a finger or the mouse: each touch leaves a ripple, a drag a
/// trail that fades.
class TouchPad : public aurore::Box
{
  public:
    TouchPad();

    void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
        aurore::FrameContext* ctx) override;

  private:
    struct Mark
    {
        float x, y; ///< in the pad, 0 to 1
        aurore::Time at;
        bool ripple;
    };
    std::deque<Mark> marks;

    void add(aurore::Point point, bool ripple);
};
