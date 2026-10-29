// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#include <aurore.hpp>

namespace aurore
{

ScrollGestureRecognizer::ScrollGestureRecognizer(PanGestureEvent::Callback respond, PanAxis axis)
    : PanGestureRecognizer(respond, axis)
{
}

GestureState ScrollGestureRecognizer::recognitionLoop(TouchState touch, MouseState mouse, View* view, Sound* soundToPlay)
{
    if (!enabled)
        return GestureState::FAILED;

    if (touch.phase != TouchPhase::NONE)
        return PanGestureRecognizer::recognitionLoop(touch, mouse, view, soundToPlay);

    GestureState result;
    if (mouse.scroll.x != 0 || mouse.scroll.y != 0)
    {
        result = GestureState::STAY;
        PanGestureStatus status {
            .state         = GestureState::STAY,
            .position      = Point(),
            .startPosition = Point(),
            .delta         = mouse.scroll,
            .deltaOnly     = true,
        };
        this->getPanGestureEvent().fire(status, soundToPlay);
    }
    else
    {
#ifdef NO_TOUCH_SCROLLING
        result = GestureState::FAILED;
#else
        result = PanGestureRecognizer::recognitionLoop(touch, mouse, view, soundToPlay);
#endif
    }

    return result;
}

};
