// SPDX-FileCopyrightText: borealis contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/input.hpp>

#include <SDL2/SDL.h>

#include <psp2common/ctrl.h>

namespace aurore
{

// Input manager for GLFW gamepad and keyboard
class PsvInputManager : public InputManager
{
  public:
    PsvInputManager();

    ~PsvInputManager();

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

    void updateControllerSensorsUpdate(SDL_ControllerSensorEvent event);

    void updateKeyboardState(SDL_KeyboardEvent event);

  private:
    SceCtrlData pad {};
    Rect screen {};
    [[nodiscard]] Point convertVitaTouchXY(float x, float y) const;
};

};
