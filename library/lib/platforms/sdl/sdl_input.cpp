// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#include <mutex>
#include <aurore/core/application.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/platforms/sdl/sdl_input.hpp>

#include "sdl_compat.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <algorithm>

namespace aurore
{

// The controller's buttons and axes Aurore reads, the first ones of SDL's
// lists (in the same order under SDL2 and SDL3)
static constexpr size_t AURORE_SDL_BUTTONS = 15;
static constexpr size_t AURORE_SDL_AXES    = 6;

// A key or mouse button: released, held, or released since it was last read
// while its press was not read yet (counts as pressed once more)
static constexpr int KEY_RELEASED = 0;
static constexpr int KEY_PRESSED  = 1;
static constexpr int KEY_STICKY   = 2;

static float normalizeSdlAxis(Sint16 axis)
{
    return axis < 0 ? static_cast<float>(axis) / 32768.0f : static_cast<float>(axis) / 32767.0f;
}

/// HidKeyboardScancode
/// Uses the same key codes as GLFW
static const AuroreKeyboardScancode sdlToAuroreKeyboardScancode(SDL_Scancode scancode)
{
    if (scancode == SDL_SCANCODE_UNKNOWN)
        return AURORE_KBD_KEY_UNKNOWN;

    // 1 - 9
    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9)
        return (AuroreKeyboardScancode)(AURORE_KBD_KEY_1 + scancode - SDL_SCANCODE_1);

    // KP1 - KP9
    if (scancode >= SDL_SCANCODE_KP_1 && scancode <= SDL_SCANCODE_KP_9)
        return (AuroreKeyboardScancode)(AURORE_KBD_KEY_KP_1 + scancode - SDL_SCANCODE_KP_1);

    // A - Z
    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z)
        return (AuroreKeyboardScancode)(AURORE_KBD_KEY_A + scancode - SDL_SCANCODE_A);

    // F1 - F12
    if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12)
        return (AuroreKeyboardScancode)(AURORE_KBD_KEY_F1 + scancode - SDL_SCANCODE_F1);

    switch (scancode)
    {
        case SDL_SCANCODE_UNKNOWN:
            return AURORE_KBD_KEY_UNKNOWN;

            /* Printable keys */
        case SDL_SCANCODE_SPACE:
            return AURORE_KBD_KEY_SPACE;
        case SDL_SCANCODE_APOSTROPHE:
            return AURORE_KBD_KEY_APOSTROPHE;
        case SDL_SCANCODE_COMMA:
            return AURORE_KBD_KEY_COMMA;
        case SDL_SCANCODE_MINUS:
            return AURORE_KBD_KEY_MINUS;
        case SDL_SCANCODE_PERIOD:
            return AURORE_KBD_KEY_PERIOD;
        case SDL_SCANCODE_SLASH:
            return AURORE_KBD_KEY_SLASH;
        case SDL_SCANCODE_0:
            return AURORE_KBD_KEY_0;
        case SDL_SCANCODE_SEMICOLON:
            return AURORE_KBD_KEY_SEMICOLON;
        case SDL_SCANCODE_EQUALS:
            return AURORE_KBD_KEY_EQUAL;
        case SDL_SCANCODE_LEFTBRACKET:
            return AURORE_KBD_KEY_LEFT_BRACKET;
        case SDL_SCANCODE_BACKSLASH:
            return AURORE_KBD_KEY_BACKSLASH;
        case SDL_SCANCODE_RIGHTBRACKET:
            return AURORE_KBD_KEY_RIGHT_BRACKET;
        case SDL_SCANCODE_GRAVE:
            return AURORE_KBD_KEY_GRAVE_ACCENT;
        case SDL_SCANCODE_INTERNATIONAL1:
            return AURORE_KBD_KEY_WORLD_1;

            /* Function keys */
        case SDL_SCANCODE_ESCAPE:
            return AURORE_KBD_KEY_ESCAPE;
        case SDL_SCANCODE_RETURN:
            return AURORE_KBD_KEY_ENTER;
        case SDL_SCANCODE_TAB:
            return AURORE_KBD_KEY_TAB;
        case SDL_SCANCODE_BACKSPACE:
            return AURORE_KBD_KEY_BACKSPACE;
        case SDL_SCANCODE_INSERT:
            return AURORE_KBD_KEY_INSERT;
        case SDL_SCANCODE_DELETE:
            return AURORE_KBD_KEY_DELETE;
        case SDL_SCANCODE_RIGHT:
            return AURORE_KBD_KEY_RIGHT;
        case SDL_SCANCODE_LEFT:
            return AURORE_KBD_KEY_LEFT;
        case SDL_SCANCODE_DOWN:
            return AURORE_KBD_KEY_DOWN;
        case SDL_SCANCODE_UP:
            return AURORE_KBD_KEY_UP;
        case SDL_SCANCODE_PAGEUP:
            return AURORE_KBD_KEY_PAGE_UP;
        case SDL_SCANCODE_PAGEDOWN:
            return AURORE_KBD_KEY_PAGE_DOWN;
        case SDL_SCANCODE_HOME:
            return AURORE_KBD_KEY_HOME;
        case SDL_SCANCODE_END:
            return AURORE_KBD_KEY_END;
        case SDL_SCANCODE_CAPSLOCK:
            return AURORE_KBD_KEY_CAPS_LOCK;
        case SDL_SCANCODE_SCROLLLOCK:
            return AURORE_KBD_KEY_SCROLL_LOCK;
        case SDL_SCANCODE_NUMLOCKCLEAR:
            return AURORE_KBD_KEY_NUM_LOCK;
        case SDL_SCANCODE_PRINTSCREEN:
            return AURORE_KBD_KEY_PRINT_SCREEN;
        case SDL_SCANCODE_PAUSE:
            return AURORE_KBD_KEY_PAUSE;
        case SDL_SCANCODE_KP_0:
            return AURORE_KBD_KEY_KP_0;
        case SDL_SCANCODE_KP_DECIMAL:
            return AURORE_KBD_KEY_KP_DECIMAL;
        case SDL_SCANCODE_KP_DIVIDE:
            return AURORE_KBD_KEY_KP_DIVIDE;
        case SDL_SCANCODE_KP_MULTIPLY:
            return AURORE_KBD_KEY_KP_MULTIPLY;
        case SDL_SCANCODE_KP_MINUS:
            return AURORE_KBD_KEY_KP_SUBTRACT;
        case SDL_SCANCODE_KP_PLUS:
            return AURORE_KBD_KEY_KP_ADD;
        case SDL_SCANCODE_KP_ENTER:
            return AURORE_KBD_KEY_KP_ENTER;
        case SDL_SCANCODE_KP_EQUALS:
            return AURORE_KBD_KEY_KP_EQUAL;
        case SDL_SCANCODE_LSHIFT:
            return AURORE_KBD_KEY_LEFT_SHIFT;
        case SDL_SCANCODE_LCTRL:
            return AURORE_KBD_KEY_LEFT_CONTROL;
        case SDL_SCANCODE_LALT:
            return AURORE_KBD_KEY_LEFT_ALT;
        case SDL_SCANCODE_LGUI:
            return AURORE_KBD_KEY_LEFT_SUPER;
        case SDL_SCANCODE_RSHIFT:
            return AURORE_KBD_KEY_RIGHT_SHIFT;
        case SDL_SCANCODE_RCTRL:
            return AURORE_KBD_KEY_RIGHT_CONTROL;
        case SDL_SCANCODE_RALT:
            return AURORE_KBD_KEY_RIGHT_ALT;
        case SDL_SCANCODE_RGUI:
            return AURORE_KBD_KEY_RIGHT_SUPER;
        case SDL_SCANCODE_MENU:
            return AURORE_KBD_KEY_MENU;
        default:
            return AURORE_KBD_KEY_UNKNOWN;
    }
}

static const SDL_Scancode auroreToSdlKeyboardScancode(AuroreKeyboardScancode scancode)
{
    if (scancode == AURORE_KBD_KEY_UNKNOWN)
        return SDL_SCANCODE_UNKNOWN;

    // 1 - 9
    if (scancode >= AURORE_KBD_KEY_1 && scancode <= AURORE_KBD_KEY_9)
        return (SDL_Scancode)(SDL_SCANCODE_1 + scancode - AURORE_KBD_KEY_1);

    // KP1 - KP9
    if (scancode >= AURORE_KBD_KEY_KP_1 && scancode <= AURORE_KBD_KEY_KP_9)
        return (SDL_Scancode)(SDL_SCANCODE_KP_1 + scancode - AURORE_KBD_KEY_KP_1);

    // A - Z
    if (scancode >= AURORE_KBD_KEY_A && scancode <= AURORE_KBD_KEY_Z)
        return (SDL_Scancode)(SDL_SCANCODE_A + scancode - AURORE_KBD_KEY_A);

    // F1 - F12
    if (scancode >= AURORE_KBD_KEY_F1 && scancode <= AURORE_KBD_KEY_F12)
        return (SDL_Scancode)(SDL_SCANCODE_F1 + scancode - AURORE_KBD_KEY_F1);

    switch (scancode)
    {
        case AURORE_KBD_KEY_UNKNOWN:
            return SDL_SCANCODE_UNKNOWN;

            /* Printable keys */
        case AURORE_KBD_KEY_SPACE:
            return SDL_SCANCODE_SPACE;
        case AURORE_KBD_KEY_APOSTROPHE:
            return SDL_SCANCODE_APOSTROPHE;
        case AURORE_KBD_KEY_COMMA:
            return SDL_SCANCODE_COMMA;
        case AURORE_KBD_KEY_MINUS:
            return SDL_SCANCODE_MINUS;
        case AURORE_KBD_KEY_PERIOD:
            return SDL_SCANCODE_PERIOD;
        case AURORE_KBD_KEY_SLASH:
            return SDL_SCANCODE_SLASH;
        case AURORE_KBD_KEY_0:
            return SDL_SCANCODE_0;
        case AURORE_KBD_KEY_SEMICOLON:
            return SDL_SCANCODE_SEMICOLON;
        case AURORE_KBD_KEY_EQUAL:
            return SDL_SCANCODE_EQUALS;
        case AURORE_KBD_KEY_LEFT_BRACKET:
            return SDL_SCANCODE_LEFTBRACKET;
        case AURORE_KBD_KEY_BACKSLASH:
            return SDL_SCANCODE_BACKSLASH;
        case AURORE_KBD_KEY_RIGHT_BRACKET:
            return SDL_SCANCODE_RIGHTBRACKET;
        case AURORE_KBD_KEY_GRAVE_ACCENT:
            return SDL_SCANCODE_GRAVE;
        case AURORE_KBD_KEY_WORLD_1:
            return SDL_SCANCODE_INTERNATIONAL1;

            /* Function keys */
        case AURORE_KBD_KEY_ESCAPE:
            return SDL_SCANCODE_ESCAPE;
        case AURORE_KBD_KEY_ENTER:
            return SDL_SCANCODE_RETURN;
        case AURORE_KBD_KEY_TAB:
            return SDL_SCANCODE_TAB;
        case AURORE_KBD_KEY_BACKSPACE:
            return SDL_SCANCODE_BACKSPACE;
        case AURORE_KBD_KEY_INSERT:
            return SDL_SCANCODE_INSERT;
        case AURORE_KBD_KEY_DELETE:
            return SDL_SCANCODE_DELETE;
        case AURORE_KBD_KEY_RIGHT:
            return SDL_SCANCODE_RIGHT;
        case AURORE_KBD_KEY_LEFT:
            return SDL_SCANCODE_LEFT;
        case AURORE_KBD_KEY_DOWN:
            return SDL_SCANCODE_DOWN;
        case AURORE_KBD_KEY_UP:
            return SDL_SCANCODE_UP;
        case AURORE_KBD_KEY_PAGE_UP:
            return SDL_SCANCODE_PAGEUP;
        case AURORE_KBD_KEY_PAGE_DOWN:
            return SDL_SCANCODE_PAGEDOWN;
        case AURORE_KBD_KEY_HOME:
            return SDL_SCANCODE_HOME;
        case AURORE_KBD_KEY_END:
            return SDL_SCANCODE_END;
        case AURORE_KBD_KEY_CAPS_LOCK:
            return SDL_SCANCODE_CAPSLOCK;
        case AURORE_KBD_KEY_SCROLL_LOCK:
            return SDL_SCANCODE_SCROLLLOCK;
        case AURORE_KBD_KEY_NUM_LOCK:
            return SDL_SCANCODE_NUMLOCKCLEAR;
        case AURORE_KBD_KEY_PRINT_SCREEN:
            return SDL_SCANCODE_PRINTSCREEN;
        case AURORE_KBD_KEY_PAUSE:
            return SDL_SCANCODE_PAUSE;
        case AURORE_KBD_KEY_KP_0:
            return SDL_SCANCODE_KP_0;
        case AURORE_KBD_KEY_KP_DECIMAL:
            return SDL_SCANCODE_KP_DECIMAL;
        case AURORE_KBD_KEY_KP_DIVIDE:
            return SDL_SCANCODE_KP_DIVIDE;
        case AURORE_KBD_KEY_KP_MULTIPLY:
            return SDL_SCANCODE_KP_MULTIPLY;
        case AURORE_KBD_KEY_KP_SUBTRACT:
            return SDL_SCANCODE_KP_MINUS;
        case AURORE_KBD_KEY_KP_ADD:
            return SDL_SCANCODE_KP_PLUS;
        case AURORE_KBD_KEY_KP_ENTER:
            return SDL_SCANCODE_KP_ENTER;
        case AURORE_KBD_KEY_KP_EQUAL:
            return SDL_SCANCODE_KP_EQUALS;
        case AURORE_KBD_KEY_LEFT_SHIFT:
            return SDL_SCANCODE_LSHIFT;
        case AURORE_KBD_KEY_LEFT_CONTROL:
            return SDL_SCANCODE_LCTRL;
        case AURORE_KBD_KEY_LEFT_ALT:
            return SDL_SCANCODE_LALT;
        case AURORE_KBD_KEY_LEFT_SUPER:
            return SDL_SCANCODE_LGUI;
        case AURORE_KBD_KEY_RIGHT_SHIFT:
            return SDL_SCANCODE_RSHIFT;
        case AURORE_KBD_KEY_RIGHT_CONTROL:
            return SDL_SCANCODE_RCTRL;
        case AURORE_KBD_KEY_RIGHT_ALT:
            return SDL_SCANCODE_RALT;
        case AURORE_KBD_KEY_RIGHT_SUPER:
            return SDL_SCANCODE_RGUI;
        case AURORE_KBD_KEY_MENU:
            return SDL_SCANCODE_MENU;
        default:
            return SDL_SCANCODE_UNKNOWN;
    }
}

// LT and RT do not exist here because they are axes
static const size_t SDL_BUTTONS_MAPPING[AURORE_SDL_BUTTONS] = {
    BUTTON_A, // SDL_CONTROLLER_BUTTON_A
    BUTTON_B, // SDL_CONTROLLER_BUTTON_B
    BUTTON_X, // SDL_CONTROLLER_BUTTON_X
    BUTTON_Y, // SDL_CONTROLLER_BUTTON_Y
    BUTTON_BACK, // SDL_CONTROLLER_BUTTON_BACK
    BUTTON_GUIDE, // SDL_CONTROLLER_BUTTON_GUIDE
    BUTTON_START, // SDL_CONTROLLER_BUTTON_START
    BUTTON_LSB, //    SDL_CONTROLLER_BUTTON_LEFTSTICK
    BUTTON_RSB, //    SDL_CONTROLLER_BUTTON_RIGHTSTICK
    BUTTON_LB, //    SDL_CONTROLLER_BUTTON_LEFTSHOULDER
    BUTTON_RB, //    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
    BUTTON_UP, //    SDL_CONTROLLER_BUTTON_DPAD_UP
    BUTTON_DOWN, //    SDL_CONTROLLER_BUTTON_DPAD_DOWN
    BUTTON_LEFT, //    SDL_CONTROLLER_BUTTON_DPAD_LEFT
    BUTTON_RIGHT, //    SDL_CONTROLLER_BUTTON_DPAD_RIGHT
};

#ifdef __SDL3__
// SDL3 names the four face buttons by where they sit (south, east, west,
// north). Aurore reads them by their label, as SDL2 did: the button marked A
// is A, on a Nintendo controller as on an Xbox one, and a PlayStation
// controller's cross is A. A controller SDL gives no label keeps the place.
static size_t sdlFaceButton(SDL_Gamepad* gamepad, size_t button)
{
    switch (SDL_GetGamepadButtonLabel(gamepad, (SDL_GamepadButton)button))
    {
        case SDL_GAMEPAD_BUTTON_LABEL_A:
        case SDL_GAMEPAD_BUTTON_LABEL_CROSS:
            return BUTTON_A;
        case SDL_GAMEPAD_BUTTON_LABEL_B:
        case SDL_GAMEPAD_BUTTON_LABEL_CIRCLE:
            return BUTTON_B;
        case SDL_GAMEPAD_BUTTON_LABEL_X:
        case SDL_GAMEPAD_BUTTON_LABEL_SQUARE:
            return BUTTON_X;
        case SDL_GAMEPAD_BUTTON_LABEL_Y:
        case SDL_GAMEPAD_BUTTON_LABEL_TRIANGLE:
            return BUTTON_Y;
        default:
            return SDL_BUTTONS_MAPPING[button];
    }
}
#endif

static std::unordered_map<SDL_Scancode, int> keyboardKeys {};

#ifdef __EMSCRIPTEN__
// In a browser, the page presses buttons itself (the keys it maps by their
// meaning, so on any layout, and its on-screen pad). A press seen by no
// frame (down and up before the next one) still counts for one.
static bool webButtonsHeld[_BUTTON_MAX] {};
static bool webButtonsPressed[_BUTTON_MAX] {};

extern "C" EMSCRIPTEN_KEEPALIVE void aurore_web_button(int button, int down)
{
    if (button < 0 || button >= _BUTTON_MAX)
        return;
    webButtonsHeld[button] = down != 0;
    if (down)
        webButtonsPressed[button] = true;
}

#endif

static const size_t SDL_AXIS_MAPPING[AURORE_SDL_AXES] = {
    LEFT_X,
    LEFT_Y,
    RIGHT_X,
    RIGHT_Y,
    LEFT_Z,
    RIGHT_Z,
};

std::vector<std::pair<SDL_JoystickID, SDL_Gamepad*>> controllers;
// Buttons of each controller pressed since its last read, so that a press
// and release between two frames still counts once, as a key does
static std::unordered_map<SDL_JoystickID, uint32_t> controllerButtonsPressed;
// The event watcher can run on another thread than the one reading the
// controllers, keys and mouse buttons (Android sends them from its UI thread)
static std::recursive_mutex controllersMutex;

// SDL calls the event watcher with its joystick lock held (Android's pad
// events): code that calls SDL with controllersMutex held takes that lock
// first, so that the two threads never wait on each other
struct ControllersLock
{
    ControllersLock()
    {
        SDL_LockJoysticks();
        controllersMutex.lock();
    }
    ~ControllersLock()
    {
        controllersMutex.unlock();
        SDL_UnlockJoysticks();
    }
};

static int mouseButtons[3] = { 0 };

static inline int getMouseButtonState(int buttonIndex)
{
    std::lock_guard<std::recursive_mutex> lock(controllersMutex);
    if (mouseButtons[buttonIndex - 1] == KEY_STICKY)
    {
        mouseButtons[buttonIndex - 1] = KEY_RELEASED;
        return KEY_PRESSED;
    }
    else
    {
        return mouseButtons[buttonIndex - 1];
    }
}

static inline int getKeyboardKeys(SDL_Scancode code)
{
    std::lock_guard<std::recursive_mutex> lock(controllersMutex);
    if (keyboardKeys.find(code) == keyboardKeys.end())
        return KEY_RELEASED;
    // Using KEY_STICKY to ensure that no input is lost
    // It's very helpful for devices with frame drops such as PSV
    if (keyboardKeys[code] == KEY_STICKY)
    {
        keyboardKeys[code] = KEY_RELEASED;
        return KEY_PRESSED;
    }
    else
    {
        return keyboardKeys[code];
    }
}

// Opens a controller SDL announces, unless it is in the list already
#ifdef __SDL3__
static void sdlOpenController(SDL_JoystickID jid)
{
    ControllersLock lock;
    SDL_Gamepad* controller = SDL_OpenGamepad(jid);
#else
static void sdlOpenController(int deviceIndex)
{
    ControllersLock lock;
    SDL_Gamepad* controller = SDL_GameControllerOpen(deviceIndex);
#endif
    if (!controller)
        return;
#ifndef __SDL3__
    SDL_JoystickID jid = SDL_JoystickGetDeviceInstanceID(deviceIndex);
#endif
    // Opening is reference-counted: a controller already in the list (opened
    // at start-up) is not tracked a second time.
    auto existing = std::find_if(controllers.begin(), controllers.end(), [jid](const auto& c)
        { return c.first == jid; });
    if (existing != controllers.end())
    {
        SDL_CloseGamepad(controller);
    }
    else
    {
        Logger::info("Controller connected: {}/{}", jid, SDL_GetGamepadName(controller));
        controllers.push_back({ jid, controller });
    }
}

static SDLFilterResult sdlEventWatcher(void* data, SDL_Event* event)
{
    if (event->type == SDL_EVENT_GAMEPAD_ADDED)
    {
        // SDL3 gives the instance ID, SDL2 the device's index
#ifdef __SDL3__
        sdlOpenController(event->gdevice.which);
#else
        sdlOpenController(event->cdevice.which);
#endif
    }
    else if (event->type == SDL_EVENT_GAMEPAD_REMOVED)
    {
        // The instance ID, under SDL2 too
#ifdef __SDL3__
        SDL_JoystickID jid = event->gdevice.which;
#else
        SDL_JoystickID jid = event->cdevice.which;
#endif
        Logger::info("Controller disconnected: {}", jid);
        ControllersLock lock;
        auto it = std::find_if(controllers.begin(), controllers.end(), [jid](auto x)
            { return x.first == jid; });
        if (it != controllers.end())
        {
            SDL_CloseGamepad(it->second);
            controllers.erase(it);
        }
        controllerButtonsPressed.erase(jid);
    }
    else if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
    {
#ifdef __SDL3__
        const auto& button = event->gbutton;
#else
        const auto& button = event->cbutton;
#endif
        if (button.button < AURORE_SDL_BUTTONS)
        {
            std::lock_guard<std::recursive_mutex> lock(controllersMutex);
            controllerButtonsPressed[button.which] |= 1u << button.button;
        }
    }
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        std::lock_guard<std::recursive_mutex> lock(controllersMutex);
        if (event->button.button <= 3)
            mouseButtons[event->button.button - 1] = KEY_PRESSED;
    }
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        std::lock_guard<std::recursive_mutex> lock(controllersMutex);
        if (event->button.button <= 3)
            mouseButtons[event->button.button - 1] = KEY_STICKY;
    }
    else if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
    {
        std::lock_guard<std::recursive_mutex> lock(controllersMutex);
#ifdef __SDL3__
        SDL_Scancode scancode = event->key.scancode;
#else
        SDL_Scancode scancode = event->key.keysym.scancode;
#endif
        keyboardKeys[scancode] = event->type == SDL_EVENT_KEY_DOWN ? KEY_PRESSED : KEY_STICKY;
    }
    Application::setActiveEvent(true);
    return 0;
}

SDLInputManager::SDLInputManager(SDL_Window* window)
    : window(window)
{

    Uint32 flags = SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD;
#if !defined(__WINRT__) && !defined(__EMSCRIPTEN__)
    flags |= SDL_INIT_HAPTIC;
#endif
    if (!AURORE_SDL_OK(SDL_Init(flags)))
    {
        aurore::fatal("Couldn't initialize joystick: " + std::string(SDL_GetError()));
    }

    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#if defined(__APPLE__) && !defined(PLATFORM_IOS) && !defined(PLATFORM_TVOS) && !defined(PLATFORM_VISIONOS)
#ifndef __SDL3__
    // SDL3's relative mode never warps the cursor
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "0");
#endif
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, "1");
#endif

#ifdef __SDL3__
    // SDL3 names the joysticks by their instance ID, in a list to free
    int controllersCount      = 0;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&controllersCount);
#else
    int controllersCount = SDL_NumJoysticks();
#endif
    aurore::Logger::info("joystick num: {}", controllersCount);

    {
        ControllersLock lock;
        for (int i = 0; i < controllersCount; i++)
        {
#ifdef __SDL3__
            SDL_JoystickID jid = joysticks[i];
            Logger::info("sdl: joystick {}: \"{}\"", jid, SDL_GetJoystickNameForID(jid));
            bool mapped = SDL_IsGamepad(jid);
#else
            SDL_JoystickID jid = SDL_JoystickGetDeviceInstanceID(i);
            Logger::info("sdl: joystick {}: \"{}\"", jid, SDL_JoystickNameForIndex(i));
            bool mapped = SDL_IsGameController(i);
#endif

            // A joystick SDL has no mapping for is not a game controller,
            // and opening it as one fails: it was kept as a null controller
            if (!mapped)
            {
                char guid[33];
#ifdef __SDL3__
                SDL_GUIDToString(SDL_GetJoystickGUIDForID(jid), guid, sizeof(guid));
#else
                SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i), guid, sizeof(guid));
#endif
                Logger::warning("sdl: joystick {} has no controller mapping (GUID {})", jid, guid);
                continue;
            }
#ifdef __SDL3__
            SDL_Gamepad* controller = SDL_OpenGamepad(jid);
#else
            SDL_Gamepad* controller = SDL_GameControllerOpen(i);
#endif
            if (!controller)
            {
                Logger::warning("sdl: joystick {} could not be opened: {}", jid, SDL_GetError());
                continue;
            }
            controllers.push_back({ jid, controller });
        }
    }
#ifdef __SDL3__
    SDL_free(joysticks);
#endif

    SDL_AddEventWatch(sdlEventWatcher, this->window);

    Application::getRunLoopEvent()->subscribe([this]()
        {
        if(fabs(scrollOffset.y) < 1) scrollOffset.y = 0;
        else scrollOffset.y *= 0.8;
        if(fabs(scrollOffset.x) < 1) scrollOffset.x = 0;
        else scrollOffset.x *= 0.8;

        pointerOffset.x = 0;
        pointerOffset.y = 0; });
}

SDLInputManager::~SDLInputManager()
{
    SDL_RemoveEventWatch(sdlEventWatcher, this->window);

    ControllersLock lock;
    for (auto i : controllers)
    {
        SDL_CloseGamepad(i.second);
    }
    controllers.clear();
    controllerButtonsPressed.clear();
}

short SDLInputManager::getControllersConnectedCount()
{
    ControllersLock lock;
    return controllers.size();
}

void SDLInputManager::updateUnifiedControllerState(ControllerState* state)
{
    for (bool& button : state->buttons)
        button = false;

    for (float& axe : state->axes)
        axe = 0;

    ControllersLock lock;
    // updateControllerState takes a position in the list, not an instance ID
    for (size_t index = 0; index < controllers.size(); index++)
    {
        ControllerState localState {};
        updateControllerState(&localState, (int)index);

        for (size_t i = 0; i < _BUTTON_MAX; i++)
            state->buttons[i] |= localState.buttons[i];

        for (size_t i = 0; i < _AXES_MAX; i++)
        {
            state->axes[i] += localState.axes[i];

            if (state->axes[i] < -1)
                state->axes[i] = -1;
            else if (state->axes[i] > 1)
                state->axes[i] = 1;
        }
    }

    // Add keyboard keys on top of gamepad buttons
    if (Application::isSwapInputKeys())
    {
        state->buttons[BUTTON_B] |= getKeyboardKeys(SDL_SCANCODE_KP_ENTER);
        state->buttons[BUTTON_B] |= getKeyboardKeys(SDL_SCANCODE_RETURN);
        state->buttons[BUTTON_A] |= getKeyboardKeys(SDL_SCANCODE_ESCAPE);
        state->buttons[BUTTON_A] |= getKeyboardKeys(SDL_SCANCODE_AC_BACK);
    }
    else
    {
        state->buttons[BUTTON_A] |= getKeyboardKeys(SDL_SCANCODE_KP_ENTER);
        state->buttons[BUTTON_A] |= getKeyboardKeys(SDL_SCANCODE_RETURN);
        state->buttons[BUTTON_B] |= getKeyboardKeys(SDL_SCANCODE_ESCAPE);
        state->buttons[BUTTON_B] |= getKeyboardKeys(SDL_SCANCODE_AC_BACK);
    }

    // Android tv remote control
    state->buttons[BUTTON_X] |= getKeyboardKeys(SDL_SCANCODE_MENU);

    state->buttons[BUTTON_UP] |= getKeyboardKeys(SDL_SCANCODE_UP);
    state->buttons[BUTTON_RIGHT] |= getKeyboardKeys(SDL_SCANCODE_RIGHT);
    state->buttons[BUTTON_DOWN] |= getKeyboardKeys(SDL_SCANCODE_DOWN);
    state->buttons[BUTTON_LEFT] |= getKeyboardKeys(SDL_SCANCODE_LEFT);

#ifdef __EMSCRIPTEN__
    for (int i = 0; i < _BUTTON_MAX; i++)
    {
        state->buttons[i] |= webButtonsHeld[i] || webButtonsPressed[i];
        webButtonsPressed[i] = false;
    }
#endif

    state->buttons[BUTTON_NAV_UP] |= state->buttons[BUTTON_UP];
    state->buttons[BUTTON_NAV_RIGHT] |= state->buttons[BUTTON_RIGHT];
    state->buttons[BUTTON_NAV_DOWN] |= state->buttons[BUTTON_DOWN];
    state->buttons[BUTTON_NAV_LEFT] |= state->buttons[BUTTON_LEFT];
}

void SDLInputManager::updateControllerState(ControllerState* state, int controller)
{
    ControllersLock lock;
    if ((int)controllers.size() <= controller)
        return;

    SDL_Gamepad* c = controllers[controller].second;

    uint32_t pressed = 0;
    auto latched     = controllerButtonsPressed.find(controllers[controller].first);
    if (latched != controllerButtonsPressed.end())
    {
        pressed         = latched->second;
        latched->second = 0;
    }

    for (size_t i = 0; i < AURORE_SDL_BUTTONS; i++)
    {
        // Translate SDL gamepad to Aurore controller
#ifdef __SDL3__
        size_t auroreButton = i < 4 ? sdlFaceButton(c, i) : SDL_BUTTONS_MAPPING[i];
#else
        size_t auroreButton = SDL_BUTTONS_MAPPING[i];
#endif
        state->buttons[auroreButton] = SDL_GetGamepadButton(c, (SDL_GamepadButton)i) || (pressed >> i & 1);
    }

    state->buttons[BUTTON_LT] = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > 3276.7f;
    state->buttons[BUTTON_RT] = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 3276.7f;

    state->buttons[BUTTON_NAV_UP]    = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_LEFTY) < -16383.5f || SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_RIGHTY) < -16383.5f || state->buttons[BUTTON_UP];
    state->buttons[BUTTON_NAV_RIGHT] = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_LEFTX) > 16383.5f || SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_RIGHTX) > 16383.5f || state->buttons[BUTTON_RIGHT];
    state->buttons[BUTTON_NAV_DOWN]  = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_LEFTY) > 16383.5f || SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_RIGHTY) > 16383.5f || state->buttons[BUTTON_DOWN];
    state->buttons[BUTTON_NAV_LEFT]  = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_LEFTX) < -16383.5f || SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_RIGHTX) < -16383.5f || state->buttons[BUTTON_LEFT];

    for (size_t i = 0; i < AURORE_SDL_AXES; i++)
    {
        state->axes[SDL_AXIS_MAPPING[i]] = normalizeSdlAxis(SDL_GetGamepadAxis(c, (SDL_GamepadAxis)i));
    }
}

bool SDLInputManager::getKeyboardKeyState(AuroreKeyboardScancode key)
{
    return getKeyboardKeys(auroreToSdlKeyboardScancode(key));
}

static void addTouch(std::vector<RawTouchState>* states, const SDL_Finger* finger)
{
    RawTouchState state;
    state.pressed    = true;
    state.fingerId   = (int)finger->id;
    state.position.x = Application::contentWidth * finger->x;
    state.position.y = Application::contentHeight * finger->y;
    states->push_back(state);
}

void SDLInputManager::updateTouchStates(std::vector<RawTouchState>* states)
{
#ifdef __SDL3__
    // Lists of IDs and of fingers, to free
    int devices          = 0;
    SDL_TouchID* touches = SDL_GetTouchDevices(&devices);
    for (int d = 0; d < devices; d++)
    {
        int touchesCount     = 0;
        SDL_Finger** fingers = SDL_GetTouchFingers(touches[d], &touchesCount);
        for (int touchID = 0; touchID < touchesCount; touchID++)
            addTouch(states, fingers[touchID]);
        SDL_free(fingers);
    }
    SDL_free(touches);
#else
    int devices = SDL_GetNumTouchDevices();
    if (devices == 0)
        return;

    for (int deviceID = 0; deviceID < devices; deviceID++)
    {
        SDL_TouchID device = SDL_GetTouchDevice(deviceID);
        int touchesCount   = SDL_GetNumTouchFingers(device);
        for (int touchID = 0; touchID < touchesCount; touchID++)
            addTouch(states, SDL_GetTouchFinger(device, touchID));
    }
#endif
}

void SDLInputManager::updateMouseStates(RawMouseState* state)
{
    // Whole pixels under SDL2, a fraction under SDL3
#ifdef __SDL3__
    float x, y;
#else
    int x, y;
#endif
    SDL_GetMouseState(&x, &y);

    state->leftButton   = getMouseButtonState(SDL_BUTTON_LEFT);
    state->middleButton = getMouseButtonState(SDL_BUTTON_MIDDLE);
    state->rightButton  = getMouseButtonState(SDL_BUTTON_RIGHT);

#ifdef AURORE_USE_D3D11
    // d3d11 scaleFactor 不计算在点击事件里
    state->position.x = x / Application::windowScale;
    state->position.y = y / Application::windowScale;
#else
    double scaleFactor = aurore::Application::getPlatform()->getVideoContext()->getScaleFactor();
    state->position.x  = x * scaleFactor / Application::windowScale;
    state->position.y  = y * scaleFactor / Application::windowScale;
#endif

    state->offset = pointerOffset;

    state->scroll = scrollOffset;
}

void SDLInputManager::setPointerLock(bool lock)
{
    pointerLocked       = lock;
    pointerOffset       = { 0, 0 };
    pointerOffsetBuffer = { 0, 0 };

#ifdef __SDL3__
    // Per window under SDL3
    if (!SDL_SetWindowRelativeMouseMode(this->window, lock))
    {
        aurore::Logger::warning("sdl: failed to set relative mouse mode: {}", SDL_GetError());
    }

    if (lock)
        SDL_HideCursor();
    else
        SDL_ShowCursor();
#else
    if (SDL_SetRelativeMouseMode(lock ? SDL_TRUE : SDL_FALSE) < 0)
    {
        aurore::Logger::warning("sdl: failed to set relative mouse mode: {}", SDL_GetError());
    }

    SDL_ShowCursor(lock ? SDL_FALSE : SDL_TRUE);
#endif
}

void SDLInputManager::runloopStart()
{
    pointerOffset         = pointerOffsetBuffer;
    pointerOffsetBuffer.x = 0;
    pointerOffsetBuffer.y = 0;
}

void SDLInputManager::sendRumble(unsigned short controller, unsigned short lowFreqMotor, unsigned short highFreqMotor)
{
    ControllersLock lock;
    if (controllers.size() <= controller)
        return;
    SDL_Gamepad* c = controllers[controller].second;

    SDL_RumbleGamepad(c, lowFreqMotor, highFreqMotor, 30000);
}

void SDLInputManager::updateMouseMotion(SDL_MouseMotionEvent event)
{
    if (pointerLocked)
    {
        getMouseCusorOffsetChanged()->fire(Point(float(event.xrel), float(event.yrel)));
    }
}

void SDLInputManager::updateMouseWheel(SDL_MouseWheelEvent event)
{
    // The precise deltas, as GLFW gives: the whole ones stay at 0 for a
    // touchpad's small steps (SDL3's x and y are the precise ones)
#ifdef __SDL3__
    float preciseX = event.x;
    float preciseY = event.y;
#else
    float preciseX = event.preciseX;
    float preciseY = event.preciseY;
#endif
#if defined(_WIN32) || defined(__linux__)
    this->scrollOffset.x += preciseX * 30;
    this->scrollOffset.y += preciseY * 30;
#else
    this->scrollOffset.x += preciseX * 10;
    this->scrollOffset.y += preciseY * 10;
#endif

    this->getMouseScrollOffsetChanged()->fire(Point(preciseX, preciseY));
}

void SDLInputManager::updateControllerSensorsUpdate(SDLGamepadSensorEvent event)
{
    // An unsigned ID under SDL3
    int id = (int)event.which;
    SensorEvent state;
    // In milliseconds, as SDL2 gives it (SDL3 in nanoseconds)
#ifdef __SDL3__
    uint32_t timestamp = (uint32_t)SDL_NS_TO_MS(event.timestamp);
#else
    uint32_t timestamp = event.timestamp;
#endif

    switch (event.sensor)
    {
        case SDL_SENSOR_ACCEL:
            state = SensorEvent { id, SensorEventType::ACCEL, { event.data[0], event.data[1], event.data[2] }, timestamp };
            getControllerSensorStateChanged()->fire(state);
            Application::setActiveEvent(true);
            break;
        case SDL_SENSOR_GYRO:
            state = SensorEvent { id, SensorEventType::GYRO, { event.data[0], event.data[1], event.data[2] }, timestamp };
            getControllerSensorStateChanged()->fire(state);
            Application::setActiveEvent(true);
            break;
    }
}

void SDLInputManager::updateKeyboardState(SDL_KeyboardEvent event)
{
    auto* self = (SDLInputManager*)Application::getPlatform()->getInputManager();
    KeyState state {};
#ifdef __SDL3__
    SDL_Scancode scancode = event.scancode;
    SDL_Keymod mod        = event.mod;
#else
    SDL_Scancode scancode = event.keysym.scancode;
    Uint16 mod            = event.keysym.mod;
#endif
    state.key     = sdlToAuroreKeyboardScancode(scancode);
    state.pressed = event.type == SDL_EVENT_KEY_DOWN;
    state.mods |= mod & SDL_KMOD_SHIFT ? AURORE_KBD_MODIFIER_SHIFT : 0;
    state.mods |= mod & SDL_KMOD_CTRL ? AURORE_KBD_MODIFIER_CTRL : 0;
    state.mods |= mod & SDL_KMOD_ALT ? AURORE_KBD_MODIFIER_ALT : 0;
    state.mods |= mod & SDL_KMOD_GUI ? AURORE_KBD_MODIFIER_META : 0;

    self->getKeyboardKeyStateChanged()->fire(state);
    Application::setActiveEvent(true);
}

};
