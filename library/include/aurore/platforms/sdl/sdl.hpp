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
