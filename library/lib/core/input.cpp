// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/application.hpp>
#include <aurore/core/input.hpp>

namespace aurore
{

TouchPhase getPhase(bool oldState, bool newState)
{
    if (!oldState && newState)
        return TouchPhase::START;

    if (oldState && newState)
        return TouchPhase::STAY;

    if (oldState && !newState)
        return TouchPhase::END;

    return TouchPhase::NONE;
}

TouchPhase getPhase(TouchPhase oldState, bool newState)
{
    bool old = oldState == TouchPhase::START || oldState == TouchPhase::STAY;
    return getPhase(old, newState);
}

TouchState InputManager::computeTouchState(RawTouchState currentTouch, TouchState lastFrameState)
{
    TouchState state;
    state.fingerId = lastFrameState.fingerId;
    state.view     = lastFrameState.view;
    state.phase    = getPhase(lastFrameState.phase, currentTouch.pressed);
    if (state.phase == TouchPhase::END)
        state.position = lastFrameState.position;
    else
        state.position = currentTouch.position;
    return state;
}

MouseState InputManager::computeMouseState(RawMouseState currentTouch, MouseState lastFrameState)
{
    MouseState state;
    state.view         = lastFrameState.view;
    state.position     = currentTouch.position;
    state.offset       = currentTouch.offset;
    state.scroll       = currentTouch.scroll;
    state.leftButton   = getPhase(lastFrameState.leftButton, currentTouch.leftButton);
    state.middleButton = getPhase(lastFrameState.middleButton, currentTouch.middleButton);
    state.rightButton  = getPhase(lastFrameState.rightButton, currentTouch.rightButton);
    return state;
}

ControllerButton InputManager::mapControllerState(ControllerButton button)
{
    if (!Application::isSwapInputKeys())
        return button;

    switch (button)
    {
        case BUTTON_A:
            return BUTTON_B;
        case BUTTON_B:
            return BUTTON_A;
        case BUTTON_X:
            return BUTTON_Y;
        case BUTTON_Y:
            return BUTTON_X;
        default:
            return button;
    }
}

} // namespace aurore
