/*
    Copyright 2021 natinusala
    Copyright 2021 XITRIX

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

#include <nanovg.h>

#include <vector>
#include <aurore/core/event.hpp>
#include <aurore/core/geometry.hpp>
#include <aurore/core/time.hpp>

#define GAMEPADS_MAX 5

namespace aurore
{

typedef enum
{
    AURORE_KBD_MODIFIER_NONE  = 0x00,
    AURORE_KBD_MODIFIER_SHIFT = 0x01,
    AURORE_KBD_MODIFIER_CTRL  = 0x02,
    AURORE_KBD_MODIFIER_ALT   = 0x04,
    AURORE_KBD_MODIFIER_META  = 0x08,
} AuroreKeyboardModifiers;

/// HidKeyboardScancode
/// Uses the same key codes as GLFW
typedef enum
{
    /* The unknown key */
    AURORE_KBD_KEY_UNKNOWN = -1,

    /* Printable keys */
    AURORE_KBD_KEY_SPACE         = 32,
    AURORE_KBD_KEY_APOSTROPHE    = 39, /* ' */
    AURORE_KBD_KEY_COMMA         = 44, /* , */
    AURORE_KBD_KEY_MINUS         = 45, /* - */
    AURORE_KBD_KEY_PERIOD        = 46, /* . */
    AURORE_KBD_KEY_SLASH         = 47, /* / */
    AURORE_KBD_KEY_0             = 48,
    AURORE_KBD_KEY_1             = 49,
    AURORE_KBD_KEY_2             = 50,
    AURORE_KBD_KEY_3             = 51,
    AURORE_KBD_KEY_4             = 52,
    AURORE_KBD_KEY_5             = 53,
    AURORE_KBD_KEY_6             = 54,
    AURORE_KBD_KEY_7             = 55,
    AURORE_KBD_KEY_8             = 56,
    AURORE_KBD_KEY_9             = 57,
    AURORE_KBD_KEY_SEMICOLON     = 59, /* ; */
    AURORE_KBD_KEY_EQUAL         = 61, /* = */
    AURORE_KBD_KEY_A             = 65,
    AURORE_KBD_KEY_B             = 66,
    AURORE_KBD_KEY_C             = 67,
    AURORE_KBD_KEY_D             = 68,
    AURORE_KBD_KEY_E             = 69,
    AURORE_KBD_KEY_F             = 70,
    AURORE_KBD_KEY_G             = 71,
    AURORE_KBD_KEY_H             = 72,
    AURORE_KBD_KEY_I             = 73,
    AURORE_KBD_KEY_J             = 74,
    AURORE_KBD_KEY_K             = 75,
    AURORE_KBD_KEY_L             = 76,
    AURORE_KBD_KEY_M             = 77,
    AURORE_KBD_KEY_N             = 78,
    AURORE_KBD_KEY_O             = 79,
    AURORE_KBD_KEY_P             = 80,
    AURORE_KBD_KEY_Q             = 81,
    AURORE_KBD_KEY_R             = 82,
    AURORE_KBD_KEY_S             = 83,
    AURORE_KBD_KEY_T             = 84,
    AURORE_KBD_KEY_U             = 85,
    AURORE_KBD_KEY_V             = 86,
    AURORE_KBD_KEY_W             = 87,
    AURORE_KBD_KEY_X             = 88,
    AURORE_KBD_KEY_Y             = 89,
    AURORE_KBD_KEY_Z             = 90,
    AURORE_KBD_KEY_LEFT_BRACKET  = 91, /* [ */
    AURORE_KBD_KEY_BACKSLASH     = 92, /* \ */
    AURORE_KBD_KEY_RIGHT_BRACKET = 93, /* ] */
    AURORE_KBD_KEY_GRAVE_ACCENT  = 96, /* ` */
    AURORE_KBD_KEY_WORLD_1       = 161, /* non-US #1 */
    AURORE_KBD_KEY_WORLD_2       = 162, /* non-US #2 */

    /* Function keys */
    AURORE_KBD_KEY_ESCAPE        = 256,
    AURORE_KBD_KEY_ENTER         = 257,
    AURORE_KBD_KEY_TAB           = 258,
    AURORE_KBD_KEY_BACKSPACE     = 259,
    AURORE_KBD_KEY_INSERT        = 260,
    AURORE_KBD_KEY_DELETE        = 261,
    AURORE_KBD_KEY_RIGHT         = 262,
    AURORE_KBD_KEY_LEFT          = 263,
    AURORE_KBD_KEY_DOWN          = 264,
    AURORE_KBD_KEY_UP            = 265,
    AURORE_KBD_KEY_PAGE_UP       = 266,
    AURORE_KBD_KEY_PAGE_DOWN     = 267,
    AURORE_KBD_KEY_HOME          = 268,
    AURORE_KBD_KEY_END           = 269,
    AURORE_KBD_KEY_CAPS_LOCK     = 280,
    AURORE_KBD_KEY_SCROLL_LOCK   = 281,
    AURORE_KBD_KEY_NUM_LOCK      = 282,
    AURORE_KBD_KEY_PRINT_SCREEN  = 283,
    AURORE_KBD_KEY_PAUSE         = 284,
    AURORE_KBD_KEY_F1            = 290,
    AURORE_KBD_KEY_F2            = 291,
    AURORE_KBD_KEY_F3            = 292,
    AURORE_KBD_KEY_F4            = 293,
    AURORE_KBD_KEY_F5            = 294,
    AURORE_KBD_KEY_F6            = 295,
    AURORE_KBD_KEY_F7            = 296,
    AURORE_KBD_KEY_F8            = 297,
    AURORE_KBD_KEY_F9            = 298,
    AURORE_KBD_KEY_F10           = 299,
    AURORE_KBD_KEY_F11           = 300,
    AURORE_KBD_KEY_F12           = 301,
    AURORE_KBD_KEY_F13           = 302,
    AURORE_KBD_KEY_F14           = 303,
    AURORE_KBD_KEY_F15           = 304,
    AURORE_KBD_KEY_F16           = 305,
    AURORE_KBD_KEY_F17           = 306,
    AURORE_KBD_KEY_F18           = 307,
    AURORE_KBD_KEY_F19           = 308,
    AURORE_KBD_KEY_F20           = 309,
    AURORE_KBD_KEY_F21           = 310,
    AURORE_KBD_KEY_F22           = 311,
    AURORE_KBD_KEY_F23           = 312,
    AURORE_KBD_KEY_F24           = 313,
    AURORE_KBD_KEY_F25           = 314,
    AURORE_KBD_KEY_KP_0          = 320,
    AURORE_KBD_KEY_KP_1          = 321,
    AURORE_KBD_KEY_KP_2          = 322,
    AURORE_KBD_KEY_KP_3          = 323,
    AURORE_KBD_KEY_KP_4          = 324,
    AURORE_KBD_KEY_KP_5          = 325,
    AURORE_KBD_KEY_KP_6          = 326,
    AURORE_KBD_KEY_KP_7          = 327,
    AURORE_KBD_KEY_KP_8          = 328,
    AURORE_KBD_KEY_KP_9          = 329,
    AURORE_KBD_KEY_KP_DECIMAL    = 330,
    AURORE_KBD_KEY_KP_DIVIDE     = 331,
    AURORE_KBD_KEY_KP_MULTIPLY   = 332,
    AURORE_KBD_KEY_KP_SUBTRACT   = 333,
    AURORE_KBD_KEY_KP_ADD        = 334,
    AURORE_KBD_KEY_KP_ENTER      = 335,
    AURORE_KBD_KEY_KP_EQUAL      = 336,
    AURORE_KBD_KEY_LEFT_SHIFT    = 340,
    AURORE_KBD_KEY_LEFT_CONTROL  = 341,
    AURORE_KBD_KEY_LEFT_ALT      = 342,
    AURORE_KBD_KEY_LEFT_SUPER    = 343,
    AURORE_KBD_KEY_RIGHT_SHIFT   = 344,
    AURORE_KBD_KEY_RIGHT_CONTROL = 345,
    AURORE_KBD_KEY_RIGHT_ALT     = 346,
    AURORE_KBD_KEY_RIGHT_SUPER   = 347,
    AURORE_KBD_KEY_MENU          = 348,

    AURORE_KBD_KEY_LAST
} AuroreKeyboardScancode;

// Abstract buttons enum - names correspond to a generic Xbox controller
// LT and RT should not be buttons but for the sake of simplicity we'll assume they are.
// Similarly, DPAD (also called HAT) is assumed to be buttons here.
enum ControllerButton
{
    BUTTON_LT = 0,
    BUTTON_LB,

    BUTTON_LSB,

    BUTTON_UP,
    BUTTON_RIGHT,
    BUTTON_DOWN,
    BUTTON_LEFT,

    BUTTON_BACK,
    BUTTON_GUIDE,
    BUTTON_START,

    BUTTON_RSB,

    BUTTON_Y,
    BUTTON_B,
    BUTTON_A,
    BUTTON_X,

    BUTTON_RB,
    BUTTON_RT,

    BUTTON_NAV_UP,
    BUTTON_NAV_RIGHT,
    BUTTON_NAV_DOWN,
    BUTTON_NAV_LEFT,

    _BUTTON_MAX,
};

// Abstract axis enum - names correspond to a generic Xbox controller
enum ControllerAxis
{
    LEFT_X,
    LEFT_Y,

    RIGHT_X, // also called 5th axis
    RIGHT_Y, // also called 4th axis

    LEFT_Z, // LT
    RIGHT_Z, // RT

    _AXES_MAX,
};

struct KeyState
{
    AuroreKeyboardScancode key;
    short mods;
    bool pressed;
};

enum class SensorEventType
{
    GYRO,
    ACCEL
};

// Represents the state of the controller's sensor
struct SensorEvent
{
    int controllerIndex;
    SensorEventType type;
    float data[3];
    uint32_t timestamp;
};

// Represents the state of the controller (a gamepad or a keyboard) in the current frame
struct ControllerState
{
    bool buttons[_BUTTON_MAX]; // true: pressed
    float axes[_AXES_MAX]; // from 0.0f to 1.0f
    Time repeatingButtonStop[_BUTTON_MAX]; // When the pressing time is greater than this value, trigger long press or repeat
};

// Represents a touch phase in the current frame
enum class TouchPhase
{
    START,
    STAY,
    END,
    NONE,
};

// Contains raw touch data, filled in by platform driver
struct RawTouchState
{
    int fingerId = 0;
    bool pressed = false;
    Point position;
};

// Contains touch data automatically filled with current phase by the library
class View;
struct TouchState
{
    int fingerId     = 0;
    TouchPhase phase = TouchPhase::NONE;
    Point position;
    View* view = nullptr;
};

// Contains raw touch data, filled in by platform driver
struct RawMouseState
{
    Point position;
    Point offset;
    Point scroll;
    bool leftButton   = false;
    bool middleButton = false;
    bool rightButton  = false;
};

// A button read once a frame from its press and release events: a press
// let go before the frame reads it still counts, for that one frame
class ButtonLatch
{
  public:
    void press()
    {
        down    = true;
        pressed = true;
    }
    void release() { down = false; }

    // Its state for the frame starting
    bool frame()
    {
        bool state = down || pressed;
        pressed    = false;
        return state;
    }

  private:
    bool down    = false;
    bool pressed = false;
};

struct MouseState
{
    Point position;
    Point offset;
    Point scroll;
    TouchPhase leftButton   = TouchPhase::NONE;
    TouchPhase middleButton = TouchPhase::NONE;
    TouchPhase rightButton  = TouchPhase::NONE;
    View* view              = nullptr;
};

// Interface responsible for reporting input state to the application - button presses,
// axis position and touch screen state
class InputManager
{
  public:
    virtual ~InputManager() { }

    virtual short getControllersConnectedCount() = 0;

    virtual void updateUnifiedControllerState(ControllerState* state) = 0;
    /**
     * Called once every frame to fill the given ControllerState struct with the controller state.
     */
    virtual void updateControllerState(ControllerState* state, int controller) = 0;

    virtual bool getKeyboardKeyState(AuroreKeyboardScancode state) = 0;

    /**
     * Called once every frame to fill the given RawTouchState struct with the raw touch data.
     */
    virtual void updateTouchStates(std::vector<RawTouchState>* states) = 0;

    /**
     * Called once every frame to fill the given RawTouchState struct with the raw touch data.
     */
    virtual void updateMouseStates(RawMouseState* state) = 0;

    /**
     * Calls to update gamepad's rumble state.
     */
    virtual void sendRumble(unsigned short controller, unsigned short lowFreqMotor, unsigned short highFreqMotor) = 0;

    /**
     * Called once every runloop cycle to perform some cleanup before new one.
     * For internal call only
     */
    virtual void runloopStart() {};

    virtual void drawCursor(NVGcontext* vg) {};

    virtual void setPointerLock(bool lock) {};

    inline Event<bool>* getMouseCusorEntered()
    {
        return &mouseCusorEntered;
    }

    inline Event<Point>* getMouseCusorOffsetChanged()
    {
        return &mouseCusorOffsetChanged;
    }

    inline Event<Point>* getMouseScrollOffsetChanged()
    {
        return &mouseScrollOffsetChanged;
    }

    inline Event<SensorEvent>* getControllerSensorStateChanged()
    {
        return &controllerSensorStateChanged;
    }

    inline Event<KeyState>* getKeyboardKeyStateChanged()
    {
        return &keyboardKeyStateChanged;
    }

    /**
     * Calculate current touch phase based on it's previous state
     */
    static TouchState computeTouchState(RawTouchState currentTouch, TouchState lastFrameState);

    /**
     * Calculate current touch phase based on it's previous state
     */
    static MouseState computeMouseState(RawMouseState currentTouch, MouseState lastFrameState);

    static ControllerButton mapControllerState(ControllerButton button);

  private:
    Event<bool> mouseCusorEntered;
    Event<Point> mouseCusorOffsetChanged;
    Event<Point> mouseScrollOffsetChanged;
    Event<KeyState> keyboardKeyStateChanged;
    Event<SensorEvent> controllerSensorStateChanged;
};

}; // namespace aurore
