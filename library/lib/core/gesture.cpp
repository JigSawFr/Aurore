// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/gesture.hpp>

namespace aurore
{

GestureState GestureRecognizer::recognitionLoop(TouchState touch, MouseState mouse, View* view, Sound* soundToPlay)
{
    return GestureState::FAILED;
}

void GestureRecognizer::interrupt(bool onlyIfUnsureState)
{
    if (onlyIfUnsureState && this->state != GestureState::UNSURE)
        return;

    this->state = GestureState::INTERRUPTED;
}

}
