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

// The SDL platform is written with SDL3's names. Under SDL2 the ones that
// were only renamed are given their SDL2 meaning here; what changed shape
// (window events, enumerating the controllers, text input, touch) is under
// #ifdef __SDL3__ where it is used. docs/README-migration.md in SDL3's
// sources lists the changes.

#include <aurore/platforms/sdl/sdl.hpp>

// SDL3's functions return true on success, SDL2's most often 0
#ifdef __SDL3__
#define AURORE_SDL_OK(call) (call)
#else
#define AURORE_SDL_OK(call) ((call) == 0)
#endif

namespace aurore
{
// What an event watcher returns (SDL_EventFilter)
#ifdef __SDL3__
using SDLFilterResult = bool;
#else
using SDLFilterResult = int;
#endif
} // namespace aurore

#ifndef __SDL3__

// Initialization and the rest
#define SDL_INIT_GAMEPAD SDL_INIT_GAMECONTROLLER
#define SDL_ScreenSaverEnabled SDL_IsScreenSaverEnabled
#define SDL_RemoveEventWatch SDL_DelEventWatch

// Events
#define SDL_EVENT_QUIT SDL_QUIT
#define SDL_EVENT_KEY_DOWN SDL_KEYDOWN
#define SDL_EVENT_KEY_UP SDL_KEYUP
#define SDL_EVENT_MOUSE_MOTION SDL_MOUSEMOTION
#define SDL_EVENT_MOUSE_WHEEL SDL_MOUSEWHEEL
#define SDL_EVENT_MOUSE_BUTTON_DOWN SDL_MOUSEBUTTONDOWN
#define SDL_EVENT_MOUSE_BUTTON_UP SDL_MOUSEBUTTONUP
#define SDL_EVENT_TEXT_INPUT SDL_TEXTINPUT
#define SDL_EVENT_TEXT_EDITING SDL_TEXTEDITING
#define SDL_EVENT_POLL_SENTINEL SDL_POLLSENTINEL
#define SDL_EVENT_WILL_ENTER_BACKGROUND SDL_APP_WILLENTERBACKGROUND
#define SDL_EVENT_WILL_ENTER_FOREGROUND SDL_APP_WILLENTERFOREGROUND
#define SDL_EVENT_TERMINATING SDL_APP_TERMINATING
#define SDL_EVENT_GAMEPAD_ADDED SDL_CONTROLLERDEVICEADDED
#define SDL_EVENT_GAMEPAD_REMOVED SDL_CONTROLLERDEVICEREMOVED
#define SDL_EVENT_GAMEPAD_BUTTON_DOWN SDL_CONTROLLERBUTTONDOWN
#define SDL_EVENT_GAMEPAD_SENSOR_UPDATE SDL_CONTROLLERSENSORUPDATE

// Controllers
typedef SDL_GameController SDL_Gamepad;
typedef SDL_GameControllerButton SDL_GamepadButton;
typedef SDL_GameControllerAxis SDL_GamepadAxis;
#define SDL_CloseGamepad SDL_GameControllerClose
#define SDL_GetGamepadName SDL_GameControllerName
#define SDL_GetGamepadButton SDL_GameControllerGetButton
#define SDL_GetGamepadAxis SDL_GameControllerGetAxis
#define SDL_RumbleGamepad SDL_GameControllerRumble
#define SDL_GAMEPAD_AXIS_LEFTX SDL_CONTROLLER_AXIS_LEFTX
#define SDL_GAMEPAD_AXIS_LEFTY SDL_CONTROLLER_AXIS_LEFTY
#define SDL_GAMEPAD_AXIS_RIGHTX SDL_CONTROLLER_AXIS_RIGHTX
#define SDL_GAMEPAD_AXIS_RIGHTY SDL_CONTROLLER_AXIS_RIGHTY
#define SDL_GAMEPAD_AXIS_LEFT_TRIGGER SDL_CONTROLLER_AXIS_TRIGGERLEFT
#define SDL_GAMEPAD_AXIS_RIGHT_TRIGGER SDL_CONTROLLER_AXIS_TRIGGERRIGHT

// Keyboard
#define SDL_KMOD_SHIFT KMOD_SHIFT
#define SDL_KMOD_CTRL KMOD_CTRL
#define SDL_KMOD_ALT KMOD_ALT
#define SDL_KMOD_GUI KMOD_GUI

#endif
