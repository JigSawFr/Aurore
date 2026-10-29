// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/input.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace aurore
{

// Input manager for GLFW gamepad and keyboard
class GLFWInputManager : public InputManager
{
  public:
    GLFWInputManager(GLFWwindow* window);

    short getControllersConnectedCount() override;

    void updateUnifiedControllerState(ControllerState* state) override;

    void updateControllerState(ControllerState* state, int controller) override;

    bool getKeyboardKeyState(AuroreKeyboardScancode state) override;

    void updateTouchStates(std::vector<RawTouchState>* states) override;

    void updateMouseStates(RawMouseState* state) override;

    void sendRumble(unsigned short controller, unsigned short lowFreqMotor, unsigned short highFreqMotor) override;

    void runloopStart() override;

    void setPointerLock(bool lock) override;

  private:
    Point scrollOffset;
    Point pointerOffset;
    Point pointerOffsetBuffer;
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void cursorCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void cursorEnterCallback(GLFWwindow* window, int entered);
    static void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    GLFWwindow* window;
    // Left, right and middle: from their events, a short click is not lost
    // between two frames
    ButtonLatch mouseLatches[3];
    bool mouseButtons[3] = {};
    bool pointerLocked   = false;
};

};
