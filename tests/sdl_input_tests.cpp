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

// The SDL driver's input, on a virtual controller (built when the library
// uses SDL2 or SDL3): a press shorter than a frame, the wheel's fractions,
// SDL's event watcher running on another thread than the frame, as on
// Android, and under SDL3 the face buttons read by their label

#include <aurore/platforms/sdl/sdl_input.hpp>
#include <aurore/test/harness.hpp>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

using aurore::ControllerState;
using aurore::SDLInputManager;

namespace
{

#ifdef __SDL3__
// The buttons the tests press, where SDL3 has them (south is A on most)
const int PAD_A = SDL_GAMEPAD_BUTTON_SOUTH;
const int PAD_B = SDL_GAMEPAD_BUTTON_EAST;
const int PAD_X = SDL_GAMEPAD_BUTTON_WEST;

void initControllers()
{
    AURORE_CHECK(SDL_Init(SDL_INIT_GAMEPAD));
}

// A controller SDL maps by itself, opened by the input manager. A vendor and
// a product make it one SDL knows (0x057e, 0x2009: Nintendo's Pro Controller).
SDL_Joystick* attachController(Uint16 vendor = 0, Uint16 product = 0)
{
    initControllers();
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type         = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes        = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons     = SDL_GAMEPAD_BUTTON_COUNT;
    desc.vendor_id    = vendor;
    desc.product_id   = product;
    desc.name         = "Aurore test controller";
    SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
    AURORE_CHECK(id != 0);
    return SDL_OpenJoystick(id);
}

void press(SDL_Joystick* joystick, int button, bool down)
{
    SDL_SetJoystickVirtualButton(joystick, button, down);
    SDL_UpdateJoysticks();
}

void closeController(SDL_Joystick* joystick)
{
    SDL_CloseJoystick(joystick);
}
#else
const int PAD_A = SDL_CONTROLLER_BUTTON_A;
const int PAD_B = SDL_CONTROLLER_BUTTON_B;
const int PAD_X = SDL_CONTROLLER_BUTTON_X;

void initControllers()
{
    AURORE_CHECK(SDL_Init(SDL_INIT_GAMECONTROLLER) == 0);
}

// A controller SDL maps by itself, opened by the input manager
SDL_Joystick* attachController()
{
    initControllers();
    SDL_VirtualJoystickDesc desc;
    SDL_zero(desc);
    desc.version  = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type     = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    desc.naxes    = SDL_CONTROLLER_AXIS_MAX;
    desc.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
    desc.name     = "Aurore test controller";
    int index     = SDL_JoystickAttachVirtualEx(&desc);
    AURORE_CHECK(index >= 0);
    return SDL_JoystickOpen(index);
}

void press(SDL_Joystick* joystick, int button, bool down)
{
    SDL_JoystickSetVirtualButton(joystick, button, down ? SDL_PRESSED : SDL_RELEASED);
    SDL_JoystickUpdate();
}

void closeController(SDL_Joystick* joystick)
{
    SDL_JoystickClose(joystick);
}
#endif

} // namespace

AURORE_TEST(sdl_press_between_two_frames_counts_once)
{
    SDL_Joystick* joystick = attachController();
    AURORE_CHECK(joystick != nullptr);
    SDLInputManager manager(nullptr);
    AURORE_CHECK_EQ(manager.getControllersConnectedCount(), 1);

    // Down and up before the frame reads the controller
    press(joystick, PAD_A, true);
    press(joystick, PAD_A, false);

    ControllerState state {};
    manager.updateUnifiedControllerState(&state);
    AURORE_CHECK(state.buttons[aurore::BUTTON_A]);
    manager.updateUnifiedControllerState(&state);
    AURORE_CHECK(!state.buttons[aurore::BUTTON_A]);

    // Held across frames: pressed until released
    press(joystick, PAD_B, true);
    manager.updateUnifiedControllerState(&state);
    AURORE_CHECK(state.buttons[aurore::BUTTON_B]);
    manager.updateUnifiedControllerState(&state);
    AURORE_CHECK(state.buttons[aurore::BUTTON_B]);
    press(joystick, PAD_B, false);
    manager.updateUnifiedControllerState(&state);
    AURORE_CHECK(!state.buttons[aurore::BUTTON_B]);

    closeController(joystick);
}

#ifdef __SDL3__
// SDL3's buttons are positional: Aurore reads A where a Nintendo controller
// prints A (east), as SDL2 did, and B under it (south)
AURORE_TEST(sdl3_reads_a_nintendo_controller_by_its_labels)
{
    SDL_Joystick* joystick = attachController(0x057e, 0x2009);
    AURORE_CHECK(joystick != nullptr);
    SDLInputManager manager(nullptr);
    AURORE_CHECK_EQ(manager.getControllersConnectedCount(), 1);

    ControllerState state {};
    press(joystick, SDL_GAMEPAD_BUTTON_EAST, true);
    manager.updateUnifiedControllerState(&state);
    AURORE_CHECK(state.buttons[aurore::BUTTON_A]);
    AURORE_CHECK(!state.buttons[aurore::BUTTON_B]);
    press(joystick, SDL_GAMEPAD_BUTTON_EAST, false);

    press(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true);
    manager.updateUnifiedControllerState(&state);
    AURORE_CHECK(state.buttons[aurore::BUTTON_B]);
    AURORE_CHECK(!state.buttons[aurore::BUTTON_A]);
    press(joystick, SDL_GAMEPAD_BUTTON_SOUTH, false);

    press(joystick, SDL_GAMEPAD_BUTTON_NORTH, true);
    manager.updateUnifiedControllerState(&state);
    AURORE_CHECK(state.buttons[aurore::BUTTON_X]);
    press(joystick, SDL_GAMEPAD_BUTTON_NORTH, false);

    closeController(joystick);
}

// An Xbox-like controller keeps the places: A at the south
AURORE_TEST(sdl3_reads_an_unknown_controller_by_place)
{
    SDL_Joystick* joystick = attachController();
    AURORE_CHECK(joystick != nullptr);
    SDLInputManager manager(nullptr);

    ControllerState state {};
    press(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true);
    manager.updateUnifiedControllerState(&state);
    AURORE_CHECK(state.buttons[aurore::BUTTON_A]);
    press(joystick, SDL_GAMEPAD_BUTTON_SOUTH, false);

    press(joystick, SDL_GAMEPAD_BUTTON_WEST, true);
    manager.updateUnifiedControllerState(&state);
    AURORE_CHECK(state.buttons[aurore::BUTTON_X]);
    press(joystick, SDL_GAMEPAD_BUTTON_WEST, false);

    closeController(joystick);
}
#endif

AURORE_TEST(sdl_wheel_gives_its_fractions)
{
    initControllers();
    SDLInputManager manager(nullptr);
    aurore::Point got { 0, 0 };
    manager.getMouseScrollOffsetChanged()->subscribe([&got](aurore::Point delta)
        { got = delta; });

    // A touchpad's small step: no whole notch
    SDL_MouseWheelEvent wheel {};
#ifdef __SDL3__
    wheel.type = SDL_EVENT_MOUSE_WHEEL;
    wheel.x    = -0.5f;
    wheel.y    = 0.25f;
#else
    wheel.type     = SDL_MOUSEWHEEL;
    wheel.preciseX = -0.5f;
    wheel.preciseY = 0.25f;
#endif
    manager.updateMouseWheel(wheel);
    AURORE_CHECK(got.x == -0.5f);
    AURORE_CHECK(got.y == 0.25f);
}

// SDL calls the event watcher with its joystick lock held (Android sends a
// pad's events from its UI thread): the frame reading the controllers at the
// same time must not wait on it while it waits on the frame
AURORE_TEST(sdl_watcher_on_another_thread_does_not_block_the_frame)
{
    SDL_Joystick* joystick = attachController();
    AURORE_CHECK(joystick != nullptr);
    SDLInputManager manager(nullptr);
#ifdef __SDL3__
    SDL_JoystickID jid = SDL_GetJoystickID(joystick);
#else
    SDL_JoystickID jid = SDL_JoystickInstanceID(joystick);
#endif

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
#ifdef __SDL3__
            if (i % 2)
            {
                event.type           = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
                event.gbutton.which  = jid;
                event.gbutton.button = PAD_X;
            }
            else
            {
                // A controller plugged in again: the watcher opens it (SDL3
                // gives its instance ID)
                event.type          = SDL_EVENT_GAMEPAD_ADDED;
                event.gdevice.which = jid;
            }
#else
            if (i % 2)
            {
                event.type           = SDL_CONTROLLERBUTTONDOWN;
                event.cbutton.which  = jid;
                event.cbutton.button = PAD_X;
            }
            else
            {
                // A controller plugged in again: the watcher opens it
                event.type         = SDL_CONTROLLERDEVICEADDED;
                event.cdevice.which = 0;
            }
#endif
            SDL_LockJoysticks();
            SDL_PushEvent(&event);
            SDL_UnlockJoysticks();
#ifdef __SDL3__
            if (i % 256 == 0)
                SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
#else
            if (i % 256 == 0)
                SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
#endif
        }
        done = true; });

    ControllerState state {};
    while (!done)
        manager.updateUnifiedControllerState(&state);

    system.join();
    watchdog.join();
    AURORE_CHECK_EQ(manager.getControllersConnectedCount(), 1);
    closeController(joystick);
}

int main(int argc, char** argv)
{
    return aurore::test::runTests(argc, argv);
}
