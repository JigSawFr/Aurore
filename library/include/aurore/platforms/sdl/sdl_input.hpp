// SPDX-FileCopyrightText: borealis contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/input.hpp>

#include <aurore/platforms/sdl/sdl.hpp>

namespace aurore
{

// Input manager for GLFW gamepad and keyboard
class SDLInputManager : public InputManager
{
  public:
    explicit SDLInputManager(SDL_Window* window);

    ~SDLInputManager();

    short getControllersConnectedCount() override;

    void updateUnifiedControllerState(ControllerState* state) override;

    void updateControllerState(ControllerState* state, int controller) override;

    bool getKeyboardKeyState(AuroreKeyboardScancode state) override;

    void updateTouchStates(std::vector<RawTouchState>* states) override;

    void updateMouseStates(RawMouseState* state) override;

    void sendRumble(unsigned short controller, unsigned short lowFreqMotor, unsigned short highFreqMotor) override;

    void runloopStart() override;

    void setPointerLock(bool lock) override;

    void updateMouseMotion(SDL_MouseMotionEvent event);

    void updateMouseWheel(SDL_MouseWheelEvent event);

    void updateControllerSensorsUpdate(SDLGamepadSensorEvent event);

    void updateKeyboardState(SDL_KeyboardEvent event);

  private:
    Point scrollOffset;
    Point pointerOffset;
    Point pointerOffsetBuffer;
    bool pointerLocked = false;

    SDL_Window* window;
};

};
