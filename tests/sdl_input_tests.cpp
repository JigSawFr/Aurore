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

// The SDL2 driver's input, on a virtual controller (built when the library
// uses SDL2): a press shorter than a frame, the wheel's fractions, and SDL's
// event watcher running on another thread than the frame, as on Android

#include <borealis/platforms/sdl/sdl_input.hpp>
#include <borealis/test/harness.hpp>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

using brls::ControllerState;
using brls::SDLInputManager;

namespace
{

// A controller SDL maps by itself, opened by the input manager
SDL_Joystick* attachController()
{
    BRLS_CHECK(SDL_Init(SDL_INIT_GAMECONTROLLER) == 0);
    SDL_VirtualJoystickDesc desc;
    SDL_zero(desc);
    desc.version  = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type     = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    desc.naxes    = SDL_CONTROLLER_AXIS_MAX;
    desc.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
    desc.name     = "Aurore test controller";
    int index     = SDL_JoystickAttachVirtualEx(&desc);
    BRLS_CHECK(index >= 0);
    return SDL_JoystickOpen(index);
}

} // namespace

BRLS_TEST(sdl_press_between_two_frames_counts_once)
{
    SDL_Joystick* joystick = attachController();
    BRLS_CHECK(joystick != nullptr);
    SDLInputManager manager(nullptr);
    BRLS_CHECK_EQ(manager.getControllersConnectedCount(), 1);

    // Down and up before the frame reads the controller
    SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, SDL_PRESSED);
    SDL_JoystickUpdate();
    SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, SDL_RELEASED);
    SDL_JoystickUpdate();

    ControllerState state {};
    manager.updateUnifiedControllerState(&state);
    BRLS_CHECK(state.buttons[brls::BUTTON_A]);
    manager.updateUnifiedControllerState(&state);
    BRLS_CHECK(!state.buttons[brls::BUTTON_A]);

    // Held across frames: pressed until released
    SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_B, SDL_PRESSED);
    SDL_JoystickUpdate();
    manager.updateUnifiedControllerState(&state);
    BRLS_CHECK(state.buttons[brls::BUTTON_B]);
    manager.updateUnifiedControllerState(&state);
    BRLS_CHECK(state.buttons[brls::BUTTON_B]);
    SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_B, SDL_RELEASED);
    SDL_JoystickUpdate();
    manager.updateUnifiedControllerState(&state);
    BRLS_CHECK(!state.buttons[brls::BUTTON_B]);

    SDL_JoystickClose(joystick);
}

BRLS_TEST(sdl_wheel_gives_its_fractions)
{
    BRLS_CHECK(SDL_Init(SDL_INIT_GAMECONTROLLER) == 0);
    SDLInputManager manager(nullptr);
    brls::Point got { 0, 0 };
    manager.getMouseScrollOffsetChanged()->subscribe([&got](brls::Point delta)
        { got = delta; });

    // A touchpad's small step: no whole notch
    SDL_MouseWheelEvent wheel {};
    wheel.type     = SDL_MOUSEWHEEL;
    wheel.preciseX = -0.5f;
    wheel.preciseY = 0.25f;
    manager.updateMouseWheel(wheel);
    BRLS_CHECK(got.x == -0.5f);
    BRLS_CHECK(got.y == 0.25f);
}

// SDL calls the event watcher with its joystick lock held (Android sends a
// pad's events from its UI thread): the frame reading the controllers at the
// same time must not wait on it while it waits on the frame
BRLS_TEST(sdl_watcher_on_another_thread_does_not_block_the_frame)
{
    SDL_Joystick* joystick = attachController();
    BRLS_CHECK(joystick != nullptr);
    SDLInputManager manager(nullptr);
    SDL_JoystickID jid = SDL_JoystickInstanceID(joystick);

    std::atomic<bool> done { false };
    std::thread watchdog([&done]
        {
        for (int i = 0; i < 300 && !done; i++)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!done)
        {
            std::fprintf(stderr, "the frame and the event watcher wait on each other\n");
            std::_Exit(1);
        } });

    std::thread system([&done, jid]
        {
        for (int i = 0; i < 4000; i++)
        {
            SDL_Event event {};
            if (i % 2)
            {
                event.type           = SDL_CONTROLLERBUTTONDOWN;
                event.cbutton.which  = jid;
                event.cbutton.button = SDL_CONTROLLER_BUTTON_X;
            }
            else
            {
                // A controller plugged in again: the watcher opens it
                event.type         = SDL_CONTROLLERDEVICEADDED;
                event.cdevice.which = 0;
            }
            SDL_LockJoysticks();
            SDL_PushEvent(&event);
            SDL_UnlockJoysticks();
            if (i % 256 == 0)
                SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
        }
        done = true; });

    ControllerState state {};
    while (!done)
        manager.updateUnifiedControllerState(&state);

    system.join();
    watchdog.join();
    BRLS_CHECK_EQ(manager.getControllersConnectedCount(), 1);
    SDL_JoystickClose(joystick);
}

int main(int argc, char** argv)
{
    return brls::test::runTests(argc, argv);
}
