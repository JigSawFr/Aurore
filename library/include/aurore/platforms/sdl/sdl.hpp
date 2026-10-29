// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

// SDL2 or SDL3, whichever the library is built with (AURORE_WINDOW): the
// SDL platform's headers include this rather than one of them.
#ifdef __SDL3__
#include <SDL3/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

namespace aurore
{

// The SDL types the platform's own functions take, named the same under both
#ifdef __SDL3__
using SDLGamepadSensorEvent = SDL_GamepadSensorEvent;
#else
using SDLGamepadSensorEvent = SDL_ControllerSensorEvent;
#endif

} // namespace aurore
