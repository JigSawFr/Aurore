// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include <cmath>
#include <aurore/core/application.hpp>
#include <aurore/core/touch/long_press_gesture.hpp>

namespace aurore
{

LongPressGestureRecognizer::LongPressGestureRecognizer(View* view, std::function<void()> respond, Time duration, float slop)
    : respond(respond)
    , duration(duration)
    , slop(slop)
{
}

GestureState LongPressGestureRecognizer::recognitionLoop(TouchState touch, MouseState mouse, View* view, Sound* soundToPlay)
{
    if (!enabled)
        return GestureState::FAILED;

    TouchPhase phase = touch.phase;
    Point position   = touch.position;
    if (phase == TouchPhase::NONE)
    {
        phase    = mouse.leftButton;
        position = mouse.position;
    }

    if (phase == TouchPhase::NONE)
    {
        if (this->state != GestureState::INTERRUPTED)
            this->state = GestureState::FAILED;
        return this->state;
    }

    Time now = getCPUTimeUsec() / 1000;

    switch (phase)
    {
        case TouchPhase::START:
            this->state     = GestureState::UNSURE;
            this->pressedAt = now;
            this->position  = position;
            break;
        case TouchPhase::STAY:
            if (this->state == GestureState::START)
                this->state = GestureState::STAY;
            else if (this->state == GestureState::UNSURE)
            {
                float dx = position.x - this->position.x;
                float dy = position.y - this->position.y;
                if (std::sqrt(dx * dx + dy * dy) > this->slop)
                    this->state = GestureState::FAILED;
                else if (now - this->pressedAt >= this->duration)
                {
                    this->state = GestureState::START;
                    if (this->respond)
                        this->respond();
                }
                else
                    Application::requestRedraw(); // keeps the frames coming while it is held
            }
            break;
        case TouchPhase::END:
            this->state = (this->state == GestureState::START || this->state == GestureState::STAY)
                ? GestureState::END
                : GestureState::FAILED;
            break;
        case TouchPhase::NONE:
            this->state = GestureState::FAILED;
            break;
    }

    return this->state;
}

} // namespace aurore
