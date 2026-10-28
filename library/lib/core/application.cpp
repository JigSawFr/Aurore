/*
    Copyright 2019-2020 natinusala
    Copyright 2019 p-sam
    Copyright 2020 WerWolv
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

#include <yoga/Yoga.h>
#include <yoga/event/event.h>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <set>
#include <stdexcept>
#include <string>
#include <aurore/core/application.hpp>
#include <aurore/core/font.hpp>
#include <aurore/core/hot_reload.hpp>
#include <aurore/core/i18n.hpp>
#include <aurore/core/inspector.hpp>
#include <aurore/core/profiler.hpp>
#include <aurore/core/theme_decor.hpp>
#include <aurore/core/theme_image.hpp>
#include <aurore/core/thread.hpp>
#include <aurore/core/time.hpp>
#include <aurore/core/util.hpp>
#include <aurore/views/bottom_bar.hpp>
#include <aurore/views/button.hpp>
#include <aurore/views/cells/cell_bool.hpp>
#include <aurore/views/cells/cell_input.hpp>
#include <aurore/views/cells/cell_radio.hpp>
#include <aurore/views/cells/cell_selector.hpp>
#include <aurore/views/cells/cell_slider.hpp>
#include <aurore/views/debug_layer.hpp>
#include <aurore/views/dialog.hpp>
#include <aurore/views/edit_text_dialog.hpp>
#include <aurore/views/h_scrolling_frame.hpp>
#include <aurore/views/header.hpp>
#include <aurore/views/hint.hpp>
#include <aurore/views/icon_button.hpp>
#include <aurore/views/image.hpp>
#include <aurore/views/progress_spinner.hpp>
#include <aurore/views/rectangle.hpp>
#include <aurore/views/recycler.hpp>
#include <aurore/views/recycling_grid.hpp>
#include <aurore/views/sidebar.hpp>
#include <aurore/views/slider.hpp>
#include <aurore/views/tab_frame.hpp>
#include <aurore/views/widgets/account.hpp>
#include <aurore/views/widgets/battery.hpp>
#include <aurore/views/widgets/wireless.hpp>

#include <chrono>
#include <set>
#include <thread>
#include <aurore/cadran.hpp>

#define BUTTOM_REPEAT_TRIGGER 250000 // 250ms
#define BUTTON_REPEAT_DELAY 100000 // 100 ms

namespace aurore
{

static std::thread::id mainThreadId;

bool Application::init()
{
    mainThreadId               = std::this_thread::get_id();
    Application::inited        = false;
    Application::quitRequested = false;

    if (Application::ORIGINAL_WINDOW_WIDTH == 0)
        Application::ORIGINAL_WINDOW_WIDTH = 1280;
    if (Application::ORIGINAL_WINDOW_HEIGHT == 0)
        Application::ORIGINAL_WINDOW_HEIGHT = 720;

    // Init platform
    Application::platform            = Platform::createPlatform();
    Application::notificationManager = new NotificationManager();

    if (!Application::platform)
    {
        fatal("Did not find a valid platform");
        return false;
    }

    Logger::info("Using platform {}", platform->getName());

    // Init i18n
    loadTranslations();
    // Cadran: the preferences' language in place of the console's
    if (Application::cadran)
        cadran::start();

    Threading::start();

    // Debugging aids, off unless asked for
#ifdef AURORE_INSPECTOR
    Inspector::setEnabled(true);
#endif
    // From the environment only in a build with AURORE_DEV_TOOLS (not a
    // release one, by default)
#ifdef AURORE_DEV_TOOLS
    if (const char* inspector = environmentVariable("INSPECTOR"))
        Inspector::setEnabled(std::strcmp(inspector, "0") != 0);
    if (const char* hotReload = environmentVariable("HOT_RELOAD"))
    {
        if (std::strcmp(hotReload, "0") != 0)
            HotReload::setEnabled(true, std::strcmp(hotReload, "1") != 0 ? hotReload : "");
    }
    // A notch or a TV's overscan seen on a desktop
    if (const char* safeAreaEnv = environmentVariable("SAFE_AREA"))
    {
        Insets insets;
        if (Insets::parse(safeAreaEnv, &insets))
            Application::setSafeAreaOverride(&insets);
        else
            Logger::warning("AURORE_SAFE_AREA: \"{}\" is not \"top,right,bottom,left\"", safeAreaEnv);
    }
#endif

#if defined(AURORE_DEV_TOOLS) && !defined(__SWITCH__) && !defined(__EMSCRIPTEN__)
    const char* redrawDebugEnv = environmentVariable("REDRAW_DEBUG");
    if (redrawDebugEnv && *redrawDebugEnv && *redrawDebugEnv != '0')
        Application::setRedrawDebug(true);
#endif

    Application::inited = true;

    return true;
}

void Application::createWindow(std::string windowTitle)
{
    if (!Application::inited)
    {
        fatal("Please call aurore::Application::init() before calling aurore::Application::createWindow().");
        return;
    }

    if (VideoContext::sizeW == 0 || VideoContext::sizeH == 0)
    {
        // Create a window with a default size and position
        Application::getPlatform()->createWindow(windowTitle, ORIGINAL_WINDOW_WIDTH, ORIGINAL_WINDOW_HEIGHT);
    }
    else
    {
        // Creates a window based on the window position and size data from the last non-full-screen mode
        Application::getPlatform()->createWindow(windowTitle, VideoContext::sizeW, VideoContext::sizeH,
            VideoContext::posX, VideoContext::posY);
    }

    // The safe area again, now that the window is there to convert it
    Application::updateSafeArea();

    // Load most commonly used sounds
    AudioPlayer* audioPlayer = Application::getAudioPlayer();
    for (enum Sound sound : {
             SOUND_FOCUS_CHANGE,
             SOUND_FOCUS_ERROR,
             SOUND_CLICK,
         })
        audioPlayer->load(sound);

    // Init rng
    std::srand(std::time(nullptr));

    // Init static variables
    Application::currentFocus = nullptr;
    Application::title        = windowTitle;

    // Init yoga
    Application::getYogaConfig();

    facebook::yoga::Event::subscribe([](YGNodeConstRef node, facebook::yoga::Event::Type eventType, facebook::yoga::Event::Data eventData)
        {
        View* view = (View*)YGNodeGetContext(node);

        if (!view)
            return;

        if (eventType == facebook::yoga::Event::NodeLayout)
        {
            Profiler::addViewLaidOut();
            view->onLayout();
        } });

    // Load fonts and setup fallbacks
    Application::platform->getFontLoader()->loadFonts();

    // Register built-in XML views
    Application::registerBuiltInXMLViews();

    // Cadran's look, from the first screen on
    if (Application::cadran)
        cadran::start_window();

    Application::getWindowCreationDoneEvent()->fire();
}

YGConfig* Application::getYogaConfig()
{
    // Created on first use: views can be built before the window
    static YGConfigRef config = []
    {
        YGConfigRef config = YGConfigNew();
        YGConfigSetUseWebDefaults(config, true);
        // Yoga's messages in the app's log, one line each
        YGConfigSetLogger(config, [](YGConfigConstRef, YGNodeConstRef, YGLogLevel level, const char* format, va_list args)
            {
            char message[512];
            int length = std::vsnprintf(message, sizeof(message), format, args);
            if (level == YGLogLevelError || level == YGLogLevelFatal)
                Logger::error("yoga: {}", message);
            else if (level == YGLogLevelWarn)
                Logger::warning("yoga: {}", message);
            else
                Logger::debug("yoga: {}", message);
            return length; });
        return config;
    }();
    return config;
}

bool Application::mainLoop()
{
    return Application::platform->runLoop(internalMainLoop);
}

bool Application::internalMainLoop()
{
    Application::updateFPS();
    Application::frameStartTime = getCPUTimeUsec();
    Application::setActiveEvent(false);
    Profiler::beginFrame();

    // Main loop callback
    if (!Application::platform->mainLoopIteration() || Application::quitRequested)
    {
        Application::getWindowShouldCloseEvent()->fire();
        Application::exit();
        return false;
    }
    Profiler::endPhase(Profiler::Phase::EVENTS);

    // Mouse and touch
    if (Application::blockInputsTokens == 0)
    {
        Application::processInput();
    }
    else
    {
        Logger::verbose("input blocked (tokens={})", Application::blockInputsTokens);
        if (!muteSounds)
            Application::getAudioPlayer()->play(Sound::SOUND_CLICK_ERROR);
    }

    Profiler::endPhase(Profiler::Phase::INPUT);

    // Animations
#ifndef SIMPLE_HIGHLIGHT
    updateHighlightAnimation();
#endif
    Ticking::updateTickings();

    // Before deciding whether to draw: a theme change asks for a redraw,
    // however it was made (the seasonal preset's at a new season included)
    Theme::followCalendar();
    Application::checkThemeChanged();
    // The theme's particles moved since the last frame drawn
    bool particlesMoved = ThemeDecorations::step(Application::frameStartTime);
    Profiler::endPhase(Profiler::Phase::UPDATE);

    // Render: every frame, or with redraw on demand only when something
    // asked for it (and at least once a second, for what changes without
    // telling: a clock, a battery...)
    bool draw = true;
    if (Application::redrawOnDemand)
    {
        if (!Ticking::runningTickings.empty())
            Application::requestRedraw();
        draw = Application::redrawFrames > 0 || particlesMoved || Application::frameStartTime - Application::lastDrawTime >= 1000000;
    }

    if (draw)
    {
        Application::frame();
        Application::lastDrawTime = Application::frameStartTime;
        Application::drawnFrames++;
        if (Application::redrawFrames > 0)
            Application::redrawFrames--;
    }
    else if (Application::redrawDebug)
    {
        Application::checkSkippedFrame();
    }

    // Run sync functions
    Threading::performSyncTasks();

    HotReload::poll();

    // Trigger RunLoop subscribers
    runLoopEvent.fire();

    // Free views deletion pool.
    // A view deletion might inserts other views to deletionPool
    // (A std::deque allocates when made: nothing is made on the frames,
    // most of them, with nothing to delete.)
    if (!Application::deletionPool.empty())
    {
        std::deque<View*> pendingViews;
        pendingViews.swap(Application::deletionPool);

        std::deque<View*> undeletedViews;
        for (auto view : pendingViews)
        {
            if (!view->isPtrLocked())
            {
                delete view;
            }
            else
            {
                undeletedViews.push_back(view);
                aurore::Logger::verbose("Application: will delete view: {}", view->describe());
            }
        }

        for (auto view : undeletedViews)
            Application::deletionPool.push_back(view);
    }

    if (!Application::currentFocus && !Application::activitiesStack.empty())
        Application::giveFocus(Application::activitiesStack.back()->getContentView());

    Profiler::endPhase(Profiler::Phase::UPDATE);
    Profiler::endFrame(draw);

    // The simulated clock (the test harness) moves on its own: nothing to
    // wait for
    if (isTimeSimulated())
        return true;

    // A frame not drawn is not presented either, so nothing waits for the
    // display: the loop waits a frame's time instead (the limiter below does
    // when it is set)
    if (!draw && Application::limitedFrameTime <= 0)
        std::this_thread::sleep_for(std::chrono::microseconds(16667));

    if (Application::limitedFrameTime > 0)
    {
        // Frames start on a schedule of their own, one every limitedFrameTime:
        // sleeping what was left of each frame let the oversleeps add up (and
        // a limit of 60 gave 57 or 58). A schedule more than 4 frames late
        // (a long load) starts again from now rather than racing to catch up.
        static Time deadline = 0;
        Time now             = getCPUTimeUsec();
        if (deadline == 0 || now - deadline > 4 * Application::limitedFrameTime)
            deadline = now;
        deadline += Application::limitedFrameTime;
        if (deadline > now)
            std::this_thread::sleep_for(std::chrono::microseconds(deadline - now));
    }

    return true;
}

void Application::updateFPS()
{
    static Time start   = getCPUTimeUsec();
    static size_t index = 0;

    index++;
    // update FPS every second, from the time that really went by (the
    // frames of slightly more than a second are not a second's worth)
    Time elapsed = Application::frameStartTime - start;
    if (elapsed > 1000000)
    {
        Application::globalFPS = (size_t)((double)index * 1000000.0 / (double)elapsed + 0.5);
        start                  = Application::frameStartTime;
        index                  = 0;
    }
}

const ControllerState& Application::getControllerState()
{
    return controllerState;
}

// A stick pushed on a diagonal turns both of its axes into navigation
// buttons, and the focus then moved twice in one frame (up, then right).
// Only the direction the stick leans to most is kept, unless the other one
// also comes from the D-pad.
static void keepDominantStickDirection(ControllerState* state)
{
    const float threshold = 0.5f;
    float x               = std::max(std::fabs(state->axes[LEFT_X]), std::fabs(state->axes[RIGHT_X]));
    float y               = std::max(std::fabs(state->axes[LEFT_Y]), std::fabs(state->axes[RIGHT_Y]));
    if (x <= threshold || y <= threshold)
        return;

    if (x > y)
    {
        if (!state->buttons[BUTTON_UP])
            state->buttons[BUTTON_NAV_UP] = false;
        if (!state->buttons[BUTTON_DOWN])
            state->buttons[BUTTON_NAV_DOWN] = false;
    }
    else
    {
        if (!state->buttons[BUTTON_LEFT])
            state->buttons[BUTTON_NAV_LEFT] = false;
        if (!state->buttons[BUTTON_RIGHT])
            state->buttons[BUTTON_NAV_RIGHT] = false;
    }
}

void Application::processInput()
{
    static ControllerState oldControllerState = {};

    // Input
    std::vector<RawTouchState> rawTouch;
    RawMouseState rawMouse;

    InputManager* inputManager = Application::platform->getInputManager();
    inputManager->runloopStart();
    inputManager->updateTouchStates(&rawTouch);
    inputManager->updateMouseStates(&rawMouse);
    inputManager->updateUnifiedControllerState(&controllerState);
    keepDominantStickDirection(&controllerState);

    if (isSwapInputKeys())
    {
        bool swapKeys[ControllerButton::_BUTTON_MAX];
        for (int i = 0; i < ControllerButton::_BUTTON_MAX; i++)
            swapKeys[i] = controllerState.buttons[InputManager::mapControllerState((ControllerButton)i)];

        for (int i = 0; i < ControllerButton::_BUTTON_MAX; i++)
            controllerState.buttons[i] = swapKeys[i];
    }

    // After the swap: the keys stay as they are named
    applyKeyMap(inputManager, &controllerState);

    // Its toggle, and the whole controller while it is browsing
    if (Inspector::isEnabled())
        Inspector::processInput(inputManager, &controllerState);

    std::vector<TouchState> touchState;
    for (auto& i : rawTouch)
    {
        auto old = std::find_if(std::begin(currentTouchState), std::end(currentTouchState), [&i](const TouchState& touch)
            { return touch.fingerId == i.fingerId; });

        if (old != std::end(currentTouchState))
        {
            touchState.push_back(InputManager::computeTouchState(i, *old));
        }
        else
        {
            TouchState state;
            state.fingerId = i.fingerId;
            touchState.push_back(InputManager::computeTouchState(i, state));
        }
    }

    for (auto& i : currentTouchState)
    {
        if (i.phase == TouchPhase::NONE)
            continue;

        auto old = std::find_if(std::begin(rawTouch), std::end(rawTouch), [&i](RawTouchState touch)
            { return touch.fingerId == i.fingerId; });

        if (old == std::end(rawTouch))
        {
            touchState.push_back(InputManager::computeTouchState(RawTouchState(), i));
        }
    }

    for (auto& i : touchState)
    {
        if (i.phase == TouchPhase::NONE)
        {
            i.view = nullptr;
            break;
        }
        else if (!i.view || i.phase == TouchPhase::START)
        {
            Point position = i.position;
            Application::setInputType(InputType::TOUCH);
            Application::setDrawCoursor(false);

            // Search for first responder, which will be the root of recognition tree
            if (!Application::activitiesStack.empty())
                i.view = Application::activitiesStack[Application::activitiesStack.size() - 1]
                             ->getContentView()
                             ->hitTest(position);
        }

        if (i.view && i.phase != TouchPhase::NONE)
        {
            Sound sound = i.view->gestureRecognizerRequest(i, MouseState(), i.view);
            float pitch = 1;
            if (sound == SOUND_TOUCH)
            {
                // Play touch sound with random pitch
                pitch = (rand() % 10) / 10.0f + 1.0f;
            }
            Application::getAudioPlayer()->play(sound, pitch);
        }
    }
    currentTouchState = touchState;
    for (auto& i : touchState)
        if (i.phase != TouchPhase::NONE)
        {
            Application::requestRedraw();
            Application::lastInputTime = getCPUTimeUsec();
        }

    MouseState mouseState = InputManager::computeMouseState(rawMouse, currentMouseState);

    if (mouseState.offset.x != 0 || mouseState.offset.y != 0 || mouseState.scroll.x != 0 || mouseState.scroll.y != 0 || mouseState.leftButton != TouchPhase::NONE || mouseState.middleButton != TouchPhase::NONE || mouseState.rightButton != TouchPhase::NONE)
    {
        Application::setInputType(InputType::TOUCH);
        Application::setDrawCoursor(true);
        Application::requestRedraw();
        Application::lastInputTime = getCPUTimeUsec();
    }

    // hover: styles follow the pointer when it moves (its offset is only
    // known while it is locked: the position is compared instead).
    static Point lastPointer = mouseState.position;
    if (mouseState.position.x != lastPointer.x || mouseState.position.y != lastPointer.y)
    {
        lastPointer = mouseState.position;
        Application::updateHover(mouseState.position, true);
    }

    if (mouseState.scroll.x == 0 && mouseState.scroll.y == 0 && mouseState.leftButton == TouchPhase::NONE && mouseState.middleButton == TouchPhase::NONE && mouseState.rightButton == TouchPhase::NONE)
        mouseState.view = nullptr;
    else if (mouseState.view == nullptr)
    {
        Point position = mouseState.position;

        // Search for first responder, which will be the root of recognition tree
        if (!Application::activitiesStack.empty())
            mouseState.view = Application::activitiesStack[Application::activitiesStack.size() - 1]
                                  ->getContentView()
                                  ->hitTest(position);
    }
    currentMouseState = mouseState;

    if (mouseState.view)
    {
        Sound sound = mouseState.view->gestureRecognizerRequest(TouchState(), mouseState, mouseState.view);
        float pitch = 1;
        if (sound == SOUND_TOUCH)
        {
            // Play touch sound with random pitch
            pitch = (rand() % 10) / 10.0f + 1.0f;
        }
        Application::getAudioPlayer()->play(sound, pitch);
    }

    // Trigger controller events
    bool repeating = false;
    Time cpuTime   = getCPUTimeUsec();

    for (int i = 0; i < _BUTTON_MAX; i++)
    {
        if (controllerState.buttons[i])
        {
            Application::lastInputTime = cpuTime;
            repeating                  = controllerState.repeatingButtonStop[i] > 0 && cpuTime > controllerState.repeatingButtonStop[i];

            if (repeating)
                controllerState.repeatingButtonStop[i] = cpuTime + BUTTON_REPEAT_DELAY;

            if (!oldControllerState.buttons[i])
                controllerState.repeatingButtonStop[i] = cpuTime + BUTTOM_REPEAT_TRIGGER;

            if (!oldControllerState.buttons[i] || repeating)
                Application::onControllerButtonPressed((enum ControllerButton)i, repeating);
        }
        else
        {
            controllerState.repeatingButtonStop[i] = 0;
        }
    }

    // A button held (a slider moving) or just let go changes the screen
    for (int i = 0; i < _BUTTON_MAX; i++)
    {
        if (controllerState.buttons[i] || oldControllerState.buttons[i])
        {
            Application::requestRedraw();
            break;
        }
    }

    oldControllerState = controllerState;

    // Trigger keyboard events
    const bool ctrlPressed  = inputManager->getKeyboardKeyState(AURORE_KBD_KEY_LEFT_CONTROL) || inputManager->getKeyboardKeyState(AURORE_KBD_KEY_RIGHT_CONTROL);
    const bool altPressed   = inputManager->getKeyboardKeyState(AURORE_KBD_KEY_LEFT_ALT) || inputManager->getKeyboardKeyState(AURORE_KBD_KEY_RIGHT_ALT);
    const bool shiftPressed = inputManager->getKeyboardKeyState(AURORE_KBD_KEY_LEFT_SHIFT) || inputManager->getKeyboardKeyState(AURORE_KBD_KEY_RIGHT_SHIFT);
    const bool metaPressed  = inputManager->getKeyboardKeyState(AURORE_KBD_KEY_LEFT_SUPER) || inputManager->getKeyboardKeyState(AURORE_KBD_KEY_RIGHT_SUPER);
    for (size_t i = 0; i < watchedKeys.size(); i++)
    {
        auto& watchedKey    = watchedKeys[i];
        auto& oldWatchedKey = oldWatchedKeys[i];

        const AuroreKeyboardScancode scancode = watchedKey.getScancode();
        if (scancode == AURORE_KBD_KEY_UNKNOWN)
        {
            continue;
        }

        watchedKey.pressed = inputManager->getKeyboardKeyState(scancode);
        if (static_cast<bool>(watchedKey.getModifiers() & AURORE_KBD_MODIFIER_SHIFT) != shiftPressed || static_cast<bool>(watchedKey.getModifiers() & AURORE_KBD_MODIFIER_CTRL) != ctrlPressed || static_cast<bool>(watchedKey.getModifiers() & AURORE_KBD_MODIFIER_ALT) != altPressed || static_cast<bool>(watchedKey.getModifiers() & AURORE_KBD_MODIFIER_META) != metaPressed)
        {
            // If the key is pressed but the modifiers are not, unpressed it
            watchedKey.pressed = false;
        }

        if (watchedKey.pressed)
        {
            Application::requestRedraw();
            repeating = watchedKey.repeatingStop > 0 && cpuTime > watchedKey.repeatingStop;

            if (repeating)
                watchedKey.repeatingStop = cpuTime + BUTTON_REPEAT_DELAY;

            if (!oldWatchedKey.pressed)
                watchedKey.repeatingStop = cpuTime + BUTTOM_REPEAT_TRIGGER;

            if (!oldWatchedKey.pressed || repeating)
                onKeyboardPressed(watchedKey.key, repeating);
        }
        else
        {
            watchedKey.repeatingStop = 0;
        }

        oldWatchedKeys[i] = watchedKeys[i];
    }
}

void Application::addToWatchedKeys(const AuroreKeyCombination key)
{
    if (key.code == AURORE_KBD_KEY_UNKNOWN)
        return;
    if (watchedKeysMap.count(key) == 0 || watchedKeysMap[key] <= 0)
    {
        watchedKeysMap[key] = 0;
        watchedKeys.emplace_back(key);
        oldWatchedKeys.emplace_back(key);
    }
    watchedKeysMap[key] = watchedKeysMap[key] + 1;
}

void Application::removeWatchedKeys(const AuroreKeyCombination key)
{
    if (key.code == AURORE_KBD_KEY_UNKNOWN)
        return;
    if (watchedKeysMap.count(key) == 0 || watchedKeysMap[key] <= 0)
    {
        // Key is not watched, nothing to do
        return;
    }
    watchedKeysMap[key] = watchedKeysMap[key] - 1;
    if (watchedKeysMap[key] > 0)
    {
        // Key is still watched, nothing to do
        return;
    }

    // Key is not watched anymore, remove it from the lists
    for (auto it = watchedKeys.begin(); it != watchedKeys.end(); ++it)
    {
        if (it->key == key)
        {
            watchedKeys.erase(it);
            break;
        }
    }
    for (auto it = oldWatchedKeys.begin(); it != oldWatchedKeys.end(); ++it)
    {
        if (it->key == key)
        {
            oldWatchedKeys.erase(it);
            break;
        }
    }
}

Platform* Application::getPlatform()
{
    return Application::platform;
}

AudioPlayer* Application::getAudioPlayer()
{
    return Application::platform->getAudioPlayer();
}

void Application::quit()
{
    Application::quitRequested = true;
}

void Application::navigate(FocusDirection direction, bool repeating)
{
    // Dismiss if repeating the same
    if (repeating && Application::repetitionOldFocus == Application::currentFocus)
        return;

    Application::repetitionOldFocus = Application::currentFocus;

    // Dismiss navigation if input type was changed
    if (Application::setInputType(InputType::GAMEPAD))
        return;

    View* currentFocus = Application::currentFocus;

    // Do nothing if there is no current focus
    if (!currentFocus)
        return;

    View* nextFocus = nullptr;

    // Handle custom navigation routes
    // By View ptr
    if (currentFocus->hasCustomNavigationRouteByPtr(direction))
    {
        nextFocus = currentFocus->getCustomNavigationRoutePtr(direction);

        if (!nextFocus)
            Logger::warning("Tried to follow a navigation route that leads to a nullptr view! (from=\"{}\", direction={})", currentFocus->describe(), std::to_string((int)direction));
    }
    // By ID
    else if (currentFocus->hasCustomNavigationRouteById(direction))
    {
        std::string id = currentFocus->getCustomNavigationRouteId(direction);
        nextFocus      = currentFocus->getNearestView(id);

        if (!nextFocus)
            Logger::warning("Tried to follow a navigation route that leads to an unknown view ID! (from=\"{}\", direction={}, targetId=\"{}\")", currentFocus->describe(), std::to_string((int)direction), id);
    }
    // Do nothing if current focus doesn't have a parent
    // (in which case there is nothing to traverse)
    else if (currentFocus->hasParent())
    {
        // Get next view to focus by traversing the views tree upwards
        nextFocus = currentFocus->getNextFocus(direction, currentFocus);
    }

    // No view to focus at the end of the traversal: wiggle and return
    if (!nextFocus)
    {
        Application::getAudioPlayer()->play(SOUND_FOCUS_ERROR);
        Application::currentFocus->shakeHighlight(direction);
        return;
    }

    // If new focus not the same as now, play sound and give it focus
    if (nextFocus->getDefaultFocus() && Application::getCurrentFocus() != nextFocus->getDefaultFocus() && nextFocus->getVisibility() == Visibility::VISIBLE)
    {
        enum Sound focusSound = nextFocus->getFocusSound();
        Application::getAudioPlayer()->play(focusSound);
        Application::giveFocus(nextFocus);
    }
    else
    {
        Application::currentFocus->shakeHighlight(direction);
    }
}

void Application::onControllerButtonPressed(enum ControllerButton button, bool repeating)
{

    // Actions
    if (Application::handleAction(ACTION_GAMEPAD, button, repeating))
        return;

    // Navigation
    // Only navigate if the button hasn't been consumed by an action
    // (allows overriding DPAD buttons using actions)
    switch (button)
    {
        case BUTTON_NAV_DOWN:
            Application::navigate(FocusDirection::DOWN, repeating);
            return;
        case BUTTON_NAV_UP:
            Application::navigate(FocusDirection::UP, repeating);
            return;
        case BUTTON_NAV_LEFT:
            Application::navigate(FocusDirection::LEFT, repeating);
            return;
        case BUTTON_NAV_RIGHT:
            Application::navigate(FocusDirection::RIGHT, repeating);
            return;
        default:
            break;
    }

    // Only play the error sound if no action applied
    Application::getAudioPlayer()->play(SOUND_CLICK_ERROR);
}

void Application::onKeyboardPressed(AuroreKeyCombination key, bool repeating)
{
    Application::handleAction(ACTION_KEYBOARD, key, repeating);
}

bool Application::setInputType(InputType type)
{
    if (type == Application::inputType)
        return false;

    Application::inputType = type;
    globalInputTypeChangeEvent.fire(type);

    if (type == InputType::GAMEPAD)
    {
        Application::setDrawCoursor(false);
        if (Application::currentFocus)
            Application::currentFocus->onFocusGained();
    }

    return true;
}

View* Application::getCurrentFocus()
{
    return Application::currentFocus;
}

void Application::setAutomaticDeactivation(bool value)
{
    Application::deactivatedBehavior = value;
}

bool Application::getAutomaticDeactivation()
{
    return Application::deactivatedBehavior;
}

bool Application::hasActiveEvent()
{
#ifdef __SWITCH__
    // Switch does not support waiting for events
    return true;
#else
    if (!Application::deactivatedBehavior || activeEvent || Application::frameStartTime - lastActiveTime < Application::deactivatedTime)
        return true;
    // The theme's particles, until they stop on their own after a while
    if (ThemeDecorations::particlesRunning())
        return true;
    // An animation under way (not a timer that never ends, a clock) keeps
    // the frames coming: idle, it was drawn at the idle rate, in jerks.
    for (Ticking* ticking : Ticking::runningTickings)
        if (dynamic_cast<FiniteTicking*>(ticking))
            return true;
    return false;
#endif
}

void Application::setRedrawOnDemand(bool enabled)
{
    Application::redrawOnDemand = enabled;
    Application::requestRedraw();
}

bool Application::isRedrawOnDemand()
{
    return Application::redrawOnDemand;
}

void Application::requestRedraw()
{
    // This frame if it is not drawn yet, and the next one, for a change made
    // after this frame's drawing
    Application::redrawFrames = 2;
}

uint64_t Application::getDrawnFrameCount()
{
    return Application::drawnFrames;
}

void Application::setActiveEvent(bool value)
{
    if (value)
        Application::requestRedraw();

#ifndef __SWITCH__
    Application::activeEvent = value;
    if (value)
    {
        lastActiveTime = getCPUTimeUsec();
    }
#endif
}

void Application::setDeactivatedTime(int millisecond)
{
    Application::deactivatedTime = millisecond * 1000;
}

void Application::setDeactivatedFPS(int value)
{
    Application::deactivatedFPS = value;
}

int Application::getDeactivatedFPS()
{
    return Application::deactivatedFPS;
}

double Application::getDeactivatedFrameTime()
{
    return 1.0 / Application::deactivatedFPS;
}

bool Application::handleAction(const ActionType type, const int button, const bool repeating)
{
    // Dismiss if input type was changed
    if (type == ACTION_GAMEPAD && button == BUTTON_A && setInputType(InputType::GAMEPAD))
        return false;

    if (Application::activitiesStack.empty())
        return false;

    View* hintParent = Application::currentFocus;
    std::set<int> consumedButtons;

    if (!hintParent)
        hintParent = Application::activitiesStack[Application::activitiesStack.size() - 1]->getContentView();

    while (hintParent)
    {
        for (auto& action : hintParent->getActions())
        {
            if (action->getType() != type)
                continue;

            if (action->getButton() != button)
                continue;

            if (consumedButtons.find(action->getButton()) != consumedButtons.end())
                continue;

            if (action->isAvailable() && (!repeating || action->isAllowRepeating()))
            {
                if (action->getActionListener()(hintParent))
                {
                    setInputType(InputType::GAMEPAD);
                    if (type == ACTION_GAMEPAD && button == BUTTON_A)
                        hintParent->playClickAnimation();

                    getAudioPlayer()->play(action->getSound());

                    consumedButtons.insert(action->getButton());
                }
            }
        }

        hintParent = hintParent->getParent();
    }

    return !consumedButtons.empty();
}

void Application::frame(bool present)
{
    VideoContext* videoContext = Application::platform->getVideoContext();

    const bool inspect = Inspector::isOpen();
    if (inspect)
        Inspector::beginFrame();

    // Frame context
    FrameContext frameContext = FrameContext();

    frameContext.pixelRatio = (float)Application::windowWidth / (float)Application::windowHeight;
    frameContext.vg         = Application::getNVGContext();
    frameContext.fontStash  = &Application::fontStash;
    frameContext.theme      = Application::getTheme();

    // Begin frame and clear
    videoContext->beginFrame();
    videoContext->clear(Application::getTheme().getColor("aurore/clear"));
    float scaleFactor = videoContext->getScaleFactor();

    nvgBeginFrame(frameContext.vg, Application::windowWidth, Application::windowHeight, scaleFactor);
    nvgScale(frameContext.vg, Application::windowScale, Application::windowScale);
    Application::drawThemeBackgroundImage(frameContext.vg, 0, 0, Application::contentWidth, Application::contentHeight);
    ThemeDecorations::drawStrips(frameContext.vg, 0, 0, Application::contentWidth, Application::contentHeight);
    ThemeDecorations::drawParticles(frameContext.vg, ParticleLayer::BACK, Application::contentWidth, Application::contentHeight);

    // Kept from frame to frame: no allocation once it is large enough
    static std::vector<View*> viewsToDraw;
    viewsToDraw.clear();

    // Draw all activities in the stack
    // until we find one that's not translucent
    for (size_t i = 0; i < Application::activitiesStack.size(); i++)
    {
        Activity* activity = Application::activitiesStack[Application::activitiesStack.size() - 1 - i];

        View* view = activity->getContentView();
        if (view)
            viewsToDraw.push_back(view);

        if (!activity->isTranslucent())
            break;
    }

    for (size_t i = 0; i < viewsToDraw.size(); i++)
    {
        View* view = viewsToDraw[viewsToDraw.size() - 1 - i];
        view->frame(&frameContext);
    }

    // Only a focus on the activity on top is highlighted: one pushed with
    // nothing focusable yet (its content still coming) would otherwise show
    // the screen below's highlight floating over it
    if (currentFocus && Application::getInputType() != InputType::TOUCH
        && (activitiesStack.empty() || currentFocus->getParentActivity() == activitiesStack.back()))
    {
        if (!Application::highlightIdleFade)
        {
            currentFocus->frameHighlight(&frameContext);
        }
        else
        {
            float idle           = (float)(getCPUTimeUsec() - Application::lastInputTime) / 1000.0f;
            float highlightAlpha = idle < 800.0f ? 1.0f : std::max(0.0f, 1.0f - (idle - 800.0f) / 400.0f);
            if (idle < 1200.0f)
                Application::requestRedraw(); // until faded out
            if (highlightAlpha > 0.0f)
            {
                nvgSave(frameContext.vg);
                nvgGlobalAlpha(frameContext.vg, highlightAlpha);
                currentFocus->frameHighlight(&frameContext);
                nvgRestore(frameContext.vg);
            }
        }
    }

    // The theme's particles in front of the screens, under the toasts
    ThemeDecorations::drawParticles(frameContext.vg, ParticleLayer::FRONT, Application::contentWidth, Application::contentHeight);

    // Notifications
    Application::notificationManager->frame(&frameContext);

    if (isDrawCursor())
    {
        getPlatform()->getInputManager()->drawCursor(frameContext.vg);
    }

    if (debuggingViewEnabled)
    {
        if (!debugLayer)
            debugLayer = new DebugLayer();

        debugLayer->frame(&frameContext);
    }

    // A frame drawn only to be compared (checkSkippedFrame) is not profiled
    if (present && Profiler::isMeasuring())
    {
        int draws = 0, fill = 0, stroke = 0, text = 0;
        nvgDebugGetStats(frameContext.vg, &draws, &fill, &stroke, &text);
        Profiler::setDrawStats(draws, fill + stroke + text);
        Profiler::endPhase(Profiler::Phase::DRAW);
    }

    if (inspect)
    {
        Inspector::draw(&frameContext);
        if (present)
            Profiler::endPhase(Profiler::Phase::INSPECTOR);
    }

    // End frame
    nvgResetTransform(Application::getNVGContext()); // scale
    nvgEndFrame(Application::getNVGContext());

    if (!present)
        return;
    Profiler::endPhase(Profiler::Phase::FLUSH);

    if (Application::redrawDebug)
        videoContext->readPixels(&Application::presentedPixels, &Application::presentedWidth, &Application::presentedHeight);

    videoContext->endFrame();
    Profiler::endPhase(Profiler::Phase::SWAP);
    postFrameEvent.fire();
}

// The views drawn over `area` with nothing of theirs drawn over it: the
// deepest ones (a label rather than the list it is in)
static void collectViewsIn(View* view, const Rect& area, std::vector<View*>* found)
{
    if (!view || view->getVisibility() != Visibility::VISIBLE)
        return;
    Rect frame = view->getFrame();
    if (frame.getMaxX() <= area.getMinX() || frame.getMinX() >= area.getMaxX() || frame.getMaxY() <= area.getMinY() || frame.getMinY() >= area.getMaxY())
        return;
    size_t before = found->size();
    if (auto* box = dynamic_cast<Box*>(view))
        for (View* child : box->getChildren())
            collectViewsIn(child, area, found);
    if (found->size() == before)
        found->push_back(view);
}

void Application::checkSkippedFrame()
{
    if (Application::presentedPixels.empty())
        return;

    // Drawn as it would be: what draw() asks for while at it is not this
    // frame's business
    int redrawFramesBefore = Application::redrawFrames;
    Application::frame(false);
    Application::redrawFrames = redrawFramesBefore;

    std::vector<unsigned char> pixels;
    int width = 0, height = 0;
    bool read = Application::platform->getVideoContext()->readPixels(&pixels, &width, &height);
    if (!read || width != Application::presentedWidth || height != Application::presentedHeight)
        return;

    int minX = width, minY = height, maxX = -1, maxY = -1;
    size_t differing = 0;
    for (int y = 0; y < height; y++)
    {
        const unsigned char* a = pixels.data() + (size_t)y * width * 4;
        const unsigned char* b = Application::presentedPixels.data() + (size_t)y * width * 4;
        for (int x = 0; x < width; x++, a += 4, b += 4)
        {
            if (std::abs(a[0] - b[0]) > 2 || std::abs(a[1] - b[1]) > 2 || std::abs(a[2] - b[2]) > 2)
            {
                differing++;
                minX = std::min(minX, x);
                minY = std::min(minY, y);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
            }
        }
    }
    if (differing == 0)
        return;

    float scale = Application::windowScale > 0 ? Application::windowScale : 1.0f;
    Rect area(minX / scale, minY / scale, (maxX + 1 - minX) / scale, (maxY + 1 - minY) / scale);
    Application::missedRedraws++;
    bool sameArea = Application::missedRedrawArea.getMinX() == area.getMinX() && Application::missedRedrawArea.getMinY() == area.getMinY()
        && Application::missedRedrawArea.getWidth() == area.getWidth() && Application::missedRedrawArea.getHeight() == area.getHeight();
    Application::missedRedrawArea = area;
    if (sameArea)
        return;

    std::vector<View*> views;
    for (size_t i = Application::activitiesStack.size(); i > 0; i--)
    {
        Activity* activity = Application::activitiesStack[i - 1];
        collectViewsIn(activity->getContentView(), area, &views);
        if (!activity->isTranslucent())
            break;
    }
    // The smallest first: the most likely to have changed on its own
    std::stable_sort(views.begin(), views.end(), [](View* a, View* b)
        { return a->getWidth() * a->getHeight() < b->getWidth() * b->getHeight(); });
    std::string described;
    if (Application::currentFocus)
    {
        Rect focus = Application::currentFocus->getFrame();
        if (focus.getMaxX() > area.getMinX() && focus.getMinX() < area.getMaxX() && focus.getMaxY() > area.getMinY() && focus.getMinY() < area.getMaxY())
            described = "the focused " + Application::currentFocus->describe() + " (its highlight?), ";
    }
    for (size_t i = 0; i < views.size() && i < 4; i++)
        described += (i ? ", " : "") + views[i]->describe();
    if (views.size() > 4)
        described += fmt::format(" and {} more", views.size() - 4);
    Logger::warning("redraw debug: a skipped frame differs from the last one presented ({} pixels in {:.0f},{:.0f} {:.0f}x{:.0f}): no redraw asked for {}",
        differing, area.getMinX(), area.getMinY(), area.getWidth(), area.getHeight(), described.empty() ? "?" : described);
}

void Application::setRedrawDebug(bool enabled)
{
    Application::redrawDebug = enabled;
    Application::presentedPixels.clear();
    Application::requestRedraw();
}

bool Application::isRedrawDebug()
{
    return Application::redrawDebug;
}

uint64_t Application::getMissedRedrawCount()
{
    return Application::missedRedraws;
}

Rect Application::missedRedrawArea;

Rect Application::getLastMissedRedrawArea()
{
    return Application::missedRedrawArea;
}

void Application::exit()
{

    exitEvent.fire();
    Logger::info("Exiting...");

    // The async thread first: a task still running must not reach views
    // or textures freed below
    Threading::stop();

    Application::clear();

    // Free views deletion pool
    while (!Application::deletionPool.empty())
    {
        std::deque<View*> pendingViews;
        pendingViews.swap(Application::deletionPool);

        for (auto view : pendingViews)
            delete view;
    }

    exitDoneEvent.fire();

    // The theme's pictures, while their context is there
    Application::themeBackground.reset();
    Application::themeBackgroundLoading.reset();
    Application::themeBackgroundSpec = ThemeBackground();
    ThemeDecorations::reset();
    ThemeImages::releaseAll();

    delete Application::notificationManager;
    delete Application::platform;
}

void Application::setInspectorEnabled(bool enabled)
{
    Inspector::setEnabled(enabled);
}

bool Application::isInspectorEnabled()
{
    return Inspector::isEnabled();
}

void Application::setHotReload(bool enabled, const std::string& resourcesDir)
{
    HotReload::setEnabled(enabled, resourcesDir);
}

bool Application::isHotReload()
{
    return HotReload::isEnabled();
}

// The positions of a view and its parents in their parents' children, from
// the root down: where to find its like in the same tree made again
static std::vector<size_t> pathInTree(View* root, View* view)
{
    std::vector<size_t> path;
    while (view && view != root)
    {
        Box* parent = view->hasParent() ? view->getParent() : nullptr;
        if (!parent)
            return {};
        auto& children = parent->getChildren();
        auto it        = std::find(children.begin(), children.end(), view);
        if (it == children.end())
            return {};
        path.insert(path.begin(), (size_t)(it - children.begin()));
        view = parent;
    }
    if (view != root)
        return {};
    return path;
}

static View* viewAtPath(View* root, const std::vector<size_t>& path)
{
    View* view = root;
    for (size_t index : path)
    {
        Box* box = dynamic_cast<Box*>(view);
        if (!box || index >= box->getChildren().size())
            return nullptr;
        view = box->getChildren()[index];
    }
    return view;
}

static void findActiveSidebarItems(View* view, std::vector<View*>* items)
{
    if (auto* item = dynamic_cast<SidebarItem*>(view))
    {
        if (item->isActive())
            items->push_back(item);
    }
    if (Box* box = dynamic_cast<Box*>(view))
        for (View* child : box->getChildren())
            findActiveSidebarItems(child, items);
}

bool Application::rebuildActivity()
{
    if (Application::activitiesStack.empty())
        return false;

    Activity* activity = Application::activitiesStack.back();
    View* old          = activity->getContentView();

    // The tabs shown, then the focus, to be found again in the new content
    std::vector<std::vector<size_t>> restore;
    if (old)
    {
        std::vector<View*> items;
        findActiveSidebarItems(old, &items);
        for (View* item : items)
            restore.push_back(pathInTree(old, item));
        if (Application::currentFocus && Application::currentFocus->getParentActivity() == activity)
            restore.push_back(pathInTree(old, Application::currentFocus));
    }

    View* fresh         = nullptr;
    HotReload::building = activity;
    try
    {
        fresh = activity->createContentView();
    }
    catch (const std::exception& e)
    {
        HotReload::building = nullptr;
        Logger::error("rebuild: {} kept as it was: {}", old ? old->describe() : "the activity", e.what());
        Application::notify(std::string("XML: ") + e.what());
        return false;
    }

    if (!fresh || fresh == old)
    {
        HotReload::building = nullptr;
        Logger::warning("rebuild: the activity on top was given its view, it cannot make it again");
        return false;
    }

    // The old content is kept until the new one has made the tabs shown (a
    // tab is made when it takes the focus, its own XML may throw then): a
    // mistake anywhere leaves the screen as it was
    View* oldFocus = Application::currentFocus && Application::currentFocus->getParentActivity() == activity ? Application::currentFocus : nullptr;
    activity->swapContentView(fresh);
    std::string error;
    try
    {
        activity->onContentAvailable();
    }
    catch (const std::exception& e)
    {
        error = std::string("onContentAvailable: ") + e.what();
    }
    HotReload::building = nullptr;
    activity->resizeToFitWindow();

    if (error.empty())
    {
        if (Application::globalQuitEnabled)
            Application::gloablQuitIdentifier = activity->registerExitAction();

        activity->willAppear(true);
        try
        {
            Application::giveFocus(activity->getDefaultFocus());
            for (const auto& path : restore)
            {
                if (path.empty())
                    continue;
                if (View* view = viewAtPath(fresh, path))
                    Application::giveFocus(view);
            }
        }
        catch (const std::exception& e)
        {
            error = e.what();
        }
    }

    if (!error.empty())
    {
        Logger::error("rebuild: {} kept as it was: {}", old ? old->describe() : "the activity", error);
        activity->swapContentView(old);
        if (oldFocus)
            Application::giveFocus(oldFocus);
        else if (old)
            Application::giveFocus(activity->getDefaultFocus());
        fresh->willDisappear();
        delete fresh;
        Application::notify("XML: " + error);
        Application::requestRedraw();
        return false;
    }

    if (old)
    {
        old->willDisappear();
        delete old;
    }

    Application::requestRedraw();
    Logger::info("rebuild: {} made again", fresh->describe());
    return true;
}

void Application::setGlobalQuit(bool enabled)
{
    Application::globalQuitEnabled = enabled;
    for (auto it = Application::activitiesStack.begin(); it != Application::activitiesStack.end(); ++it)
    {
        if (enabled)
            Application::gloablQuitIdentifier = (*it)->registerExitAction();
        else
            (*it)->unregisterAction(Application::gloablQuitIdentifier);
    }
}

void Application::setFPSStatus(bool enabled)
{
    Application::globalFPSToggleEnabled = enabled;
    if (!enabled)
        Application::globalFPS = 60;
}

bool Application::getFPSStatus()
{
    return Application::globalFPSToggleEnabled;
}

size_t Application::getFPS()
{
    return Application::globalFPS;
}

void Application::setLimitedFPS(size_t fps)
{
    Application::limitedFrameTime = fps == 0 ? 0 : 1000000.0f / fps;
}

void Application::setSwapInterval(int interval)
{
    Application::platform->getVideoContext()->setSwapInterval(interval);
}

void Application::notify(const std::string& text, View* leading)
{
    Application::notificationManager->notify(text, leading);
    Application::announce(text);
}

void Application::giveFocus(View* view)
{
    View* oldFocus = Application::currentFocus;
    View* newFocus = view ? view->getDefaultFocus() : nullptr;

    if (oldFocus != newFocus && newFocus != nullptr)
    {
        if (oldFocus)
            oldFocus->onFocusLost();

        Application::currentFocus = newFocus;
        Application::globalFocusChangeEvent.fire(newFocus);

        if (newFocus)
        {
            newFocus->onFocusGained();
            Logger::debug("Giving focus to {}", newFocus->describe());

            Application::announceFocus(oldFocus, newFocus);
        }

        Application::globalHintsUpdateEvent.fire();
    }
}

void Application::setHighlightIdleFade(bool fade)
{
    Application::highlightIdleFade = fade;
    Application::lastInputTime     = getCPUTimeUsec();
}

// Whether `view` is `ancestor` or under it.
static bool isUnder(View* view, View* ancestor)
{
    for (View* v = view; v; v = v->hasParent() ? v->getParent() : nullptr)
        if (v == ancestor)
            return true;
    return false;
}

void Application::announceFocus(View* oldFocus, View* newFocus)
{
    std::string description = newFocus->getAccessibilityDescription();

    // Entering a dialog or a list from outside: what it is first ("Delete
    // the profile?, dialog, Cancel, button, 1 of 2").
    std::string context;
    for (View* v = newFocus->hasParent() ? newFocus->getParent() : nullptr; v; v = v->hasParent() ? v->getParent() : nullptr)
    {
        AccessibilityRole role = v->getAccessibilityRole();
        if (role != AccessibilityRole::DIALOG && role != AccessibilityRole::LIST)
            continue;
        if (oldFocus && isUnder(oldFocus, v))
            continue;
        std::string entered = v->getAccessibilityDescription();
        if (!entered.empty())
            context = entered + (context.empty() ? "" : ", " + context);
    }

    if (newFocus->getAccessibilityLabel().empty() && newFocus->getAccessibilityValue().empty())
    {
        // Once per kind of view: a focusable view that says nothing but its
        // role ("button") is one a screen reader user cannot tell apart.
#ifdef NDEBUG
        bool warn = (bool)Application::focusAnnouncer;
#else
        bool warn = true;
#endif
        static std::set<std::string> warned;
        std::string name = newFocus->describe();
        if (warn && warned.insert(name).second)
            Logger::warning("accessibility: {} took the focus with no label: give it an "
                            "accessibilityLabel (or a text)",
                name);
    }

    if (!context.empty())
        description = context + (description.empty() ? "" : ", " + description);
    Application::announce(description);
}

void Application::announce(const std::string& text)
{
    if (text.empty())
        return;
    Application::lastAnnouncement = text;
    if (Application::focusAnnouncer)
        Application::focusAnnouncer(text);
}

void Application::setFocusAnnouncer(std::function<void(const std::string&)> announcer)
{
    Application::focusAnnouncer = std::move(announcer);
}

void Application::setReduceMotion(bool reduce)
{
    Application::reduceMotion = reduce;
}

bool Application::getReduceMotion()
{
    return Application::reduceMotion || (Application::cadran && !effects::motion());
}

// Each label measured again (and what else measures text), and the roots to
// lay out again: the screen's, and each detached one (a scrolling frame's
// content, laid out on its own).
static void markTextScaleChanged(View* view, std::vector<View*>& roots)
{
    view->onTextScaleChanged();
    if (view->isDetached())
        roots.push_back(view);
    if (auto* box = dynamic_cast<Box*>(view))
        for (View* child : box->getChildren())
            markTextScaleChanged(child, roots);
}

void Application::setTextScale(float scale)
{
    if (scale == Label::getFontScale())
        return;
    Label::setFontScale(scale);
    for (Activity* activity : Application::activitiesStack)
    {
        View* root = activity->getContentView();
        if (!root)
            continue;
        std::vector<View*> roots;
        markTextScaleChanged(root, roots);
        root->invalidate();
        for (View* detached : roots)
            detached->invalidate();
    }
}

float Application::getTextScale()
{
    return Label::getFontScale();
}

void Application::registerModel(const std::string& name, DataModel* model)
{
    Application::models[name] = model;
    for (Activity* activity : Application::activitiesStack)
        if (activity->getContentView())
            activity->getContentView()->resolveBindings(activity);
}

void Application::unregisterModel(const std::string& name)
{
    Application::models.erase(name);
}

DataModel* Application::getModel(const std::string& name)
{
    auto it = Application::models.find(name);
    return it == Application::models.end() ? nullptr : it->second;
}

void Application::updateHover(Point position, bool inside)
{
    View* hovered = nullptr;
    if (inside && !Application::activitiesStack.empty())
    {
        View* content = Application::activitiesStack.back()->getContentView();
        hovered       = content ? content->hitTest(position) : nullptr;
        // The nearest view that has a hover: style, the hit one or a parent.
        while (hovered && !hovered->hasStateStyle(StyleState::HOVER))
            hovered = hovered->getParent();
    }
    if (hovered == Application::hoveredView)
        return;
    if (Application::hoveredView)
        Application::hoveredView->setStyleState(StyleState::HOVER, false);
    Application::hoveredView = hovered;
    if (hovered)
        hovered->setStyleState(StyleState::HOVER, true);
}

void Application::forgetView(View* view)
{
    Inspector::forgetView(view);
    if (Application::hoveredView == view)
        Application::hoveredView = nullptr;
    if (Application::currentFocus == view)
        Application::currentFocus = nullptr;
    if (Application::repetitionOldFocus == view)
        Application::repetitionOldFocus = nullptr;
    std::replace(Application::focusStack.begin(), Application::focusStack.end(), view,
        static_cast<View*>(nullptr));
}

bool Application::popActivity(TransitionAnimation animation, std::function<void(void)> cb, bool free)
{
    if (Application::activitiesStack.size() <= 1) // never pop the first activity
        return false;

    Application::blockInputs();

    Activity* last = Application::activitiesStack[Application::activitiesStack.size() - 1];
    last->willDisappear(true);

    last->setInFadeAnimation(true);

    bool fade = animation == TransitionAnimation::FADE;

    Activity* toShow = nullptr;
    // Animate the old activity immediately
    if (Application::activitiesStack.size() > 1)
    {
        toShow = Application::activitiesStack[Application::activitiesStack.size() - 2];
        toShow->hide([]() {}, false, 0);
        toShow->onResume();
        toShow->show([]() {}, false, 0);
    }

    // Focus
    if (!Application::focusStack.empty())
    {
        View* newFocus = Application::focusStack[Application::focusStack.size() - 1];

        if (newFocus && (!toShow || newFocus->getParentActivity() == toShow))
        {
            Logger::debug("Giving focus to {}, and removing it from the focus stack", newFocus->describe());
            Application::giveFocus(newFocus);
        }
        else if (toShow)
        {
            Application::giveFocus(toShow->getContentView());
        }

        Application::focusStack.pop_back();
    }
    else if (toShow)
    {
        Application::giveFocus(toShow->getContentView());
    }

    // Hide animation (and show previous activity, if any)
    last->hide([last, cb, free]()
        {
        // last is not always the top of the stack, for example, during the animation, another activity is pushed
        for (auto i = activitiesStack.begin(); i != activitiesStack.end(); ++i) {
            if ( *i == last ) {
                Application::activitiesStack.erase(i);
                break;
            }
        }
        if (cb)
            cb();
        // Deleted after this frame's animations: this callback runs while
        // they update, and the activity owns some of them.
        if (free)
            aurore::sync([last]() { delete last; });

        Application::unblockInputs(); },
        fade, last->getShowAnimationDuration(animation));

    return true;
}

std::vector<Activity*> Application::getActivitiesStack()
{
    return activitiesStack;
}

void Application::pushActivity(Activity* activity, TransitionAnimation animation)
{
    Application::blockInputs();

    // Focus
    if (!Application::activitiesStack.empty() && Application::currentFocus != nullptr)
    {
        Logger::debug("Pushing {} to the focus stack", Application::currentFocus->describe());
        Application::focusStack.push_back(Application::currentFocus);
    }

    // Create the activity content view
    HotReload::building = activity;
    activity->setContentView(activity->createContentView());
    activity->onContentAvailable();
    HotReload::building = nullptr;
    activity->resizeToFitWindow();

    if (!Application::activitiesStack.empty())
    {
        Activity* last = Application::activitiesStack[Application::activitiesStack.size() - 1];
        last->onPause();
    }

    bool fadeIn = animation == TransitionAnimation::FADE || animation == TransitionAnimation::SLIDE_LEFT || animation == TransitionAnimation::SLIDE_RIGHT; // wait for the old activity animation to be done before showing the new one?

    if (Application::globalQuitEnabled)
        Application::gloablQuitIdentifier = activity->registerExitAction();

    // Layout and prepare activity
    activity->willAppear(true);
    Application::giveFocus(activity->getDefaultFocus());

    if (!fadeIn) // No animations
    {
        aurore::Logger::debug("push activity to the stack");
        Application::activitiesStack.push_back(activity);
        Application::unblockInputs();
    }
    else
    {
        activity->hide([]() {}, false, 0);

        aurore::Logger::debug("push activity to the stack");
        Application::activitiesStack.push_back(activity);
        float duration = activity->getShowAnimationDuration(animation);
        activity->show([]()
            { Application::unblockInputs(); },
            duration > 0, duration);
    }
}

void Application::replaceActivity(Activity* activity, TransitionAnimation animation)
{
    if (Application::activitiesStack.empty())
    {
        Application::pushActivity(activity, animation);
        return;
    }

    Activity* replaced = Application::activitiesStack.back();
    size_t focusDepth  = Application::focusStack.size();

    Application::pushActivity(activity, animation);

    // Nothing will give the focus back to the replaced activity
    if (Application::focusStack.size() > focusDepth)
        Application::focusStack.pop_back();
    replaced->willDisappear(true);

    // Drawn beneath the new one while it fades in, then removed
    auto remove = [replaced]()
    {
        auto it = std::find(Application::activitiesStack.begin(), Application::activitiesStack.end(), replaced);
        if (it == Application::activitiesStack.end())
            return;
        Application::activitiesStack.erase(it);
        delete replaced;
    };

    bool animated  = animation == TransitionAnimation::FADE || animation == TransitionAnimation::SLIDE_LEFT || animation == TransitionAnimation::SLIDE_RIGHT;
    float duration = animated ? activity->getShowAnimationDuration(animation) : 0;
    if (duration > 0)
        aurore::delay((long)duration + 1, remove);
    else
        aurore::sync(remove);
}

void Application::clear()
{
    for (Activity* activity : Application::activitiesStack)
    {
        activity->willDisappear(true);
        delete activity;
    }

    Application::focusStack.clear();
    Application::activitiesStack.clear();
    Application::focusStack.clear();
    Application::currentFocus       = nullptr;
    Application::repetitionOldFocus = nullptr;
}

Theme Application::getTheme()
{
    if (Application::getThemeVariant() == ThemeVariant::LIGHT)
        return Theme::getLightTheme();
    else
        return Theme::getDarkTheme();
}

ThemeVariant Application::getThemeVariant()
{
    return Application::platform->getThemeVariant();
}

ImeManager* Application::getImeManager()
{
    return Application::platform->getImeManager();
}

std::string Application::getLocale()
{
    return Application::getPlatform()->getLocale();
}

void Application::addToFreeQueue(View* view)
{
    if (std::find(deletionPool.cbegin(), deletionPool.cend(), view) != deletionPool.cend())
        return;

    aurore::Logger::verbose("Application::addToFreeQueue {}", view->describe());

    Application::deletionPool.push_back(view);
}

void Application::tryDeinitFirstResponder(View* view)
{
    if (!view)
        return;

    // Interrupt current gestures if presented
    for (size_t i = 0; i < currentTouchState.size(); i++)
    {
        if (currentTouchState[i].view == view)
        {
            currentTouchState[i].view->interruptGestures(false);
            currentTouchState[i].view = nullptr;
        }
    }

    if (currentMouseState.view == view)
    {
        currentMouseState.view->interruptGestures(false);
        currentMouseState.view = nullptr;
    }
}

bool Application::loadFontFromFile(std::string fontName, std::string filePath)
{
#ifdef USE_LIBROMFS
    // AURORE_ASSET() paths are in the resources built into the program
    if (filePath.rfind("@res/", 0) == 0)
    {
        try
        {
            auto& font = romfs::get(filePath.substr(5));
            if (font.valid())
                return Application::loadFontFromMemory(fontName, (void*)font.data(), font.size(), false);
        }
        catch (...)
        {
        }
        Logger::warning("Could not load the font \"{}\" ({} is not in the resources)", fontName, filePath);
        return false;
    }
#endif
    int handle = nvgCreateFont(Application::getNVGContext(), fontName.c_str(), filePath.c_str());

    if (handle == FONT_INVALID)
    {
        Logger::warning("Could not load the font \"{}\"", fontName);
        return false;
    }

    Application::fontStash[fontName] = handle;
    return true;
}

bool Application::loadFontFromMemory(std::string fontName, void* address, size_t size, bool freeData)
{
    return Application::loadFontFromMemory(fontName, address, size, freeData, 0);
}

bool Application::loadFontFromMemory(std::string fontName, void* address, size_t size, bool freeData, int faceIndex)
{
    int handle = nvgCreateFontMemAtIndex(Application::getNVGContext(), fontName.c_str(), (unsigned char*)address, size, freeData, faceIndex);

    if (handle == FONT_INVALID)
    {
        Logger::warning("Could not load the font \"{}\"", fontName);
        return false;
    }

    Application::fontStash[fontName] = handle;
    return true;
}

void Application::crash(std::string text)
{
    // A dialog that cannot be dismissed, its one button quitting. Shown from
    // the main thread, and whatever blocks the input released: a crash in
    // the middle of a transition must not leave the screen unusable.
    auto show = [text]()
    {
        Logger::error("crash: {}", text);
        Application::blockInputsTokens = 0;
        Application::muteSounds        = false;

        Dialog* dialog = new Dialog(text);
        dialog->setCancelable(false);
        dialog->addButton("hints/exit"_i18n, []()
            { Application::quit(); });
        dialog->open();
    };

    if (std::this_thread::get_id() == mainThreadId)
        show();
    else
        aurore::sync(show);
}

void Application::blockInputs(bool muteSounds)
{
    Application::muteSounds |= muteSounds;
    Application::blockInputsTokens += 1;
    getGlobalHintsUpdateEvent()->fire();
    Logger::debug("Adding an inputs block token (tokens={})", Application::blockInputsTokens);
}

void Application::unblockInputs()
{
    if (Application::blockInputsTokens > 0)
        Application::blockInputsTokens -= 1;

    if (Application::blockInputsTokens == 0)
        muteSounds = false;

    getGlobalHintsUpdateEvent()->fire();
    Logger::debug("Removing an inputs block token (tokens={})", Application::blockInputsTokens);
}

bool Application::isInputBlocks()
{
    return Application::blockInputsTokens > 0;
}

void Application::setSwapInputKeys(bool swap)
{
    swapInputKeys = swap;
    getGlobalHintsUpdateEvent()->fire();
}

// As the web simulator's keys: X, Y, L, R and +/- as written on the
// controller, Shift for the triggers
std::vector<std::pair<AuroreKeyCombination, ControllerButton>> Application::keyMap = {
    { AuroreKeyCombination(AURORE_KBD_KEY_X), BUTTON_X },
    { AuroreKeyCombination(AURORE_KBD_KEY_Y), BUTTON_Y },
    { AuroreKeyCombination(AURORE_KBD_KEY_L), BUTTON_LB },
    { AuroreKeyCombination(AURORE_KBD_KEY_R), BUTTON_RB },
    { AuroreKeyCombination(AURORE_KBD_KEY_L, AURORE_KBD_MODIFIER_SHIFT), BUTTON_LT },
    { AuroreKeyCombination(AURORE_KBD_KEY_R, AURORE_KBD_MODIFIER_SHIFT), BUTTON_RT },
    { AuroreKeyCombination(AURORE_KBD_KEY_P), BUTTON_START },
    { AuroreKeyCombination(AURORE_KBD_KEY_EQUAL, AURORE_KBD_MODIFIER_SHIFT), BUTTON_START },
    { AuroreKeyCombination(AURORE_KBD_KEY_KP_ADD), BUTTON_START },
    { AuroreKeyCombination(AURORE_KBD_KEY_M), BUTTON_BACK },
    { AuroreKeyCombination(AURORE_KBD_KEY_MINUS), BUTTON_BACK },
    { AuroreKeyCombination(AURORE_KBD_KEY_KP_SUBTRACT), BUTTON_BACK },
    { AuroreKeyCombination(AURORE_KBD_KEY_BACKSPACE), BUTTON_B },
};

void Application::mapKey(AuroreKeyCombination key, ControllerButton button)
{
    unmapKey(key);
    keyMap.emplace_back(key, button);
}

void Application::unmapKey(AuroreKeyCombination key)
{
    keyMap.erase(std::remove_if(keyMap.begin(), keyMap.end(), [key](const auto& entry)
                     { return (int)entry.first == (int)key; }),
        keyMap.end());
}

void Application::clearKeyMap()
{
    keyMap.clear();
}

const std::vector<std::pair<AuroreKeyCombination, ControllerButton>>& Application::getKeyMap()
{
    return keyMap;
}

void Application::applyKeyMap(InputManager* inputManager, ControllerState* state)
{
    if (keyMap.empty())
        return;

    // Letters typed in a text field are text
    if (!activitiesStack.empty() && dynamic_cast<EditTextDialog*>(activitiesStack.back()->getContentView()))
        return;

    int mods = 0;
    if (inputManager->getKeyboardKeyState(AURORE_KBD_KEY_LEFT_SHIFT) || inputManager->getKeyboardKeyState(AURORE_KBD_KEY_RIGHT_SHIFT))
        mods |= AURORE_KBD_MODIFIER_SHIFT;
    if (inputManager->getKeyboardKeyState(AURORE_KBD_KEY_LEFT_CONTROL) || inputManager->getKeyboardKeyState(AURORE_KBD_KEY_RIGHT_CONTROL))
        mods |= AURORE_KBD_MODIFIER_CTRL;
    if (inputManager->getKeyboardKeyState(AURORE_KBD_KEY_LEFT_ALT) || inputManager->getKeyboardKeyState(AURORE_KBD_KEY_RIGHT_ALT))
        mods |= AURORE_KBD_MODIFIER_ALT;
    if (inputManager->getKeyboardKeyState(AURORE_KBD_KEY_LEFT_SUPER) || inputManager->getKeyboardKeyState(AURORE_KBD_KEY_RIGHT_SUPER))
        mods |= AURORE_KBD_MODIFIER_META;

    for (const auto& [key, button] : keyMap)
    {
        if (key.mod != mods)
            continue;
        // An app that handles the key itself keeps it
        auto watched = watchedKeysMap.find((int)key);
        if (watched != watchedKeysMap.end() && watched->second > 0)
            continue;
        if (inputManager->getKeyboardKeyState(key.code))
            state->buttons[button] = true;
    }

    state->buttons[BUTTON_NAV_UP] |= state->buttons[BUTTON_UP];
    state->buttons[BUTTON_NAV_RIGHT] |= state->buttons[BUTTON_RIGHT];
    state->buttons[BUTTON_NAV_DOWN] |= state->buttons[BUTTON_DOWN];
    state->buttons[BUTTON_NAV_LEFT] |= state->buttons[BUTTON_LEFT];
}

NVGcontext* Application::getNVGContext()
{
    return Application::platform->getVideoContext()->getNVGContext();
}

void Application::setCommonFooter(std::string footer)
{
    Application::commonFooter = footer;
}

std::string* Application::getCommonFooter()
{
    return &Application::commonFooter;
}

void Application::setMaximumUIScale(float scale)
{
    Application::maximumUIScale = scale > 0.0f ? scale : 0.0f;
}

float Application::getMaximumUIScale()
{
    return Application::maximumUIScale;
}

void Application::setWindowSize(int width, int height)
{
    Application::windowWidth  = width;
    Application::windowHeight = height;

    // Rescale UI
    float uncappedWindowScale = (float)width / (float)ORIGINAL_WINDOW_WIDTH;
    bool uiScaleIsCapped      = Application::maximumUIScale > 0.0f
        && uncappedWindowScale > Application::maximumUIScale;

    Application::windowScale = uiScaleIsCapped ? Application::maximumUIScale : uncappedWindowScale;
    // Uncapped, the content keeps its design width exactly
    Application::contentWidth  = uiScaleIsCapped ? std::ceil((float)width / Application::windowScale) : (float)ORIGINAL_WINDOW_WIDTH;
    Application::contentHeight = std::ceil((float)height / Application::windowScale);

    // In layout units: the scale changed
    bool safeAreaChanged = Application::computeSafeArea();

    for (Activity* activity : Application::activitiesStack)
        activity->onWindowSizeChanged();

    if (safeAreaChanged)
        Application::safeAreaChangedEvent.fire(Application::safeArea);

    aurore::Application::setActiveEvent(true);
}

bool Application::computeSafeArea()
{
    Insets points;
    if (Application::hasSafeAreaOverride)
        points = Application::safeAreaOverride;
    else if (Application::platform)
        points = Application::platform->getSafeAreaInsets();

    // Window points to layout units, as a touch's position
    double factor = 1.0;
    if (Application::platform && Application::platform->getVideoContext())
        factor = Application::platform->getVideoContext()->getScaleFactor();
    if (Application::windowScale > 0)
        factor /= Application::windowScale;

    const float width = Application::contentWidth, height = Application::contentHeight;
    // At most a third of the window from each edge: a platform's nonsense
    // never leaves the content without room
    auto edge = [factor](float value, float margin, float size)
    {
        float units = std::isfinite(value) && value > 0 ? (float)(value * factor) : 0.0f;
        units       = std::max(units, margin);
        // Whole units, without a rounding error's extra one
        units = std::ceil(units - 0.01f);
        return std::clamp(units, 0.0f, std::max(0.0f, (float)std::floor(size / 3)));
    };
    const float overscanX = Application::overscan * width, overscanY = Application::overscan * height;
    Insets units {
        edge(points.top, overscanY, height),
        edge(points.right, overscanX, width),
        edge(points.bottom, overscanY, height),
        edge(points.left, overscanX, width),
    };

    if (units == Application::safeArea)
        return false;
    Logger::info("Safe area: top {}, right {}, bottom {}, left {}", units.top, units.right, units.bottom,
        units.left);
    Application::safeArea = units;
    if (!units.empty())
        Application::safeAreaInUse = true;
    return true;
}

Insets Application::getSafeArea()
{
    return Application::safeArea;
}

void Application::updateSafeArea()
{
    if (!Application::computeSafeArea())
        return;
    for (Activity* activity : Application::activitiesStack)
        activity->applySafeArea();
    Application::requestRedraw();
    Application::setActiveEvent(true);
    Application::safeAreaChangedEvent.fire(Application::safeArea);
}

Event<Insets>* Application::getSafeAreaChangedEvent()
{
    return &Application::safeAreaChangedEvent;
}

void Application::setSafeAreaOverride(const Insets* insets)
{
    Application::hasSafeAreaOverride = insets != nullptr;
    if (insets)
        Application::safeAreaOverride = *insets;
    Application::updateSafeArea();
}

void Application::setOverscan(float fraction)
{
    if (!std::isfinite(fraction))
        fraction = 0.0f;
    Application::overscan = std::clamp(fraction, 0.0f, 0.25f);
    Application::updateSafeArea();
}

float Application::getOverscan()
{
    return Application::overscan;
}

void Application::onWindowResized(int width, int height)
{
    Application::requestRedraw();
    // Trigger event when Window size is stable
    static size_t iter = 0;
    aurore::cancelDelay(iter);
    iter = aurore::delay(100, [width, height]()
        {
            Application::setWindowSize(width, height);
            Application::getWindowSizeChangedEvent()->fire();

            Logger::info("Window size changed to {}x{}, content size: {}x{} windowScale: {}",
                width, height, contentWidth, contentHeight, Application::windowScale);
            aurore::Logger::info("scale factor: {}",
                Application::getPlatform()->getVideoContext()->getScaleFactor()); });
}

void Application::setWindowPosition(int x, int y)
{
    Application::windowXPos = x;
    Application::windowYPos = y;
}

void Application::onWindowReposition(int x, int y)
{
    static size_t iter = 0;
    aurore::cancelDelay(iter);
    iter = aurore::delay(500, [x, y]()
        {
            Logger::info("Window position changed to {}x{}", x, y);
            Application::setWindowPosition(x, y); });
}

std::string Application::getTitle()
{
    return Application::title;
}

GenericEvent* Application::getGlobalFocusChangeEvent()
{
    return &Application::globalFocusChangeEvent;
}

VoidEvent* Application::getGlobalHintsUpdateEvent()
{
    return &Application::globalHintsUpdateEvent;
}

Event<InputType>* Application::getGlobalInputTypeChangeEvent()
{
    return &Application::globalInputTypeChangeEvent;
}

VoidEvent* Application::getRunLoopEvent()
{
    return &Application::runLoopEvent;
}

VoidEvent* Application::getPostFrameEvent()
{
    return &Application::postFrameEvent;
}

VoidEvent* Application::getExitEvent()
{
    return &Application::exitEvent;
}

VoidEvent* Application::getExitDoneEvent()
{
    return &Application::exitDoneEvent;
}

VoidEvent* Application::getWindowSizeChangedEvent()
{
    return &Application::windowSizeChangedEvent;
}

VoidEvent* Application::getWindowCreationDoneEvent()
{
    return &Application::windowCreationDoneEvent;
}

VoidEvent* Application::getWindowShouldCloseEvent()
{
    return &Application::windowShouldCloseEvent;
}

Event<bool>* Application::getWindowFocusChangedEvent()
{
    return &Application::windowFocusChangedEvent;
}

Event<ThemeVariant>* Application::getThemeChangedEvent()
{
    return &Application::themeChangedEvent;
}

void Application::loadThemeBackground()
{
    static bool listening = false;
    if (!listening)
    {
        ThemeImages::getLoadedEvent()->subscribe(&Application::onThemeImageLoaded);
        listening = true;
    }

    ThemeBackground background       = Theme::getBackground(Application::getThemeVariant());
    Application::themeBackgroundSpec = background;
    if (!background.hasImage())
    {
        Application::themeBackground.reset();
        Application::themeBackgroundLoading.reset();
        return;
    }
    int flags                          = background.fit == BackgroundFit::TILE ? NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY : 0;
    std::shared_ptr<ThemeImage>& shown = Application::themeBackground;
    if (shown && shown->path == background.image && shown->flags == flags)
    {
        Application::themeBackgroundLoading.reset();
        return;
    }
    std::shared_ptr<ThemeImage> image = ThemeImages::load(background.image, flags);
    if (image->state == ThemeImage::State::LOADING)
        Application::themeBackgroundLoading = image;
    else
    {
        Application::themeBackground = image;
        Application::themeBackgroundLoading.reset();
    }
}

void Application::onThemeImageLoaded(const ThemeImage* image)
{
    if (image != Application::themeBackgroundLoading.get())
        return;
    Application::themeBackground = std::move(Application::themeBackgroundLoading);
    Application::themeBackgroundLoading.reset();
    if (!image->ready())
        return;
    // Its colour is known now: the text on it is checked
    ThemeVariant variant = Application::getThemeVariant();
    for (const std::string& warning : Theme::checkBackgroundContrast(variant))
        Logger::warning("theme {}: {}: {}", Theme::getCurrentPath(), variant == ThemeVariant::LIGHT ? "light" : "dark", warning);
}

bool Application::hasThemeBackground()
{
    const ThemeImage* image = Application::themeBackground.get();
    return image && image->ready() && image->width > 0 && image->height > 0;
}

void Application::drawThemeBackground(NVGcontext* vg, float x, float y, float width, float height)
{
    if (!Application::hasThemeBackground())
        return;
    nvgBeginPath(vg);
    nvgRect(vg, x, y, width, height);
    nvgFillColor(vg, Application::getTheme().getColor("aurore/clear"));
    nvgFill(vg);
    Application::drawThemeBackgroundImage(vg, x, y, width, height);
    ThemeDecorations::drawStrips(vg, x, y, width, height);
}

void Application::drawThemeBackground(NVGcontext* vg, float x, float y, float width, float height, float radius)
{
    if (!Application::hasThemeBackground())
        return;
    if (radius <= 0.0f)
        return Application::drawThemeBackground(vg, x, y, width, height);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, width, height, radius);
    nvgFillColor(vg, Application::getTheme().getColor("aurore/clear"));
    nvgFill(vg);
    Application::drawThemeBackgroundImage(vg, x, y, width, height, radius);
    // The strips have straight edges: in the corners' square, over the image
    nvgSave(vg);
    nvgIntersectScissor(vg, x, y + radius, width, height - 2 * radius);
    ThemeDecorations::drawStrips(vg, x, y, width, height);
    nvgRestore(vg);
}

// The background image as background_fit says, then its veil, over the
// clear colour and under the screens: the part over x, y, width, height
void Application::drawThemeBackgroundImage(NVGcontext* vg, float areaX, float areaY, float areaWidth, float areaHeight, float radius)
{
    if (!Application::hasThemeBackground())
        return;
    const ThemeImage* image           = Application::themeBackground.get();
    const ThemeBackground& background = Application::themeBackgroundSpec;
    float width = Application::contentWidth, height = Application::contentHeight;
    float w = (float)image->width, h = (float)image->height;
    float x = 0, y = 0;
    switch (background.fit)
    {
        case BackgroundFit::COVER:
        case BackgroundFit::CONTAIN:
        {
            float scale = background.fit == BackgroundFit::COVER ? std::max(width / w, height / h) : std::min(width / w, height / h);
            w *= scale;
            h *= scale;
            x = (width - w) / 2;
            y = (height - h) / 2;
            break;
        }
        case BackgroundFit::CENTER:
            x = std::round((width - w) / 2);
            y = std::round((height - h) / 2);
            break;
        case BackgroundFit::TILE:
            break;
    }

    // The image where it is (all over the window when tiled), in the area
    float left = areaX, top = areaY, right = areaX + areaWidth, bottom = areaY + areaHeight;
    if (background.fit != BackgroundFit::TILE)
    {
        left   = std::max(left, x);
        top    = std::max(top, y);
        right  = std::min(right, x + w);
        bottom = std::min(bottom, y + h);
    }
    if (right > left && bottom > top)
    {
        nvgBeginPath(vg);
        if (radius > 0.0f)
            nvgRoundedRect(vg, left, top, right - left, bottom - top, radius);
        else
            nvgRect(vg, left, top, right - left, bottom - top);
        nvgFillPaint(vg, nvgImagePattern(vg, x, y, w, h, 0, image->texture, 1.0f));
        nvgFill(vg);
    }

    if (background.dim > 0.0f)
    {
        NVGcolor veil = Application::getTheme().getColor("aurore/background");
        veil.a *= background.dim;
        nvgBeginPath(vg);
        if (radius > 0.0f)
            nvgRoundedRect(vg, areaX, areaY, areaWidth, areaHeight, radius);
        else
            nvgRect(vg, areaX, areaY, areaWidth, areaHeight);
        nvgFillColor(vg, veil);
        nvgFill(vg);
    }
}

void Application::checkThemeChanged()
{
    ThemeVariant variant = Application::getThemeVariant();
    uint64_t generation  = Theme::getGeneration();
    if (generation == Application::themeGeneration && variant == Application::themeVariant)
        return;
    Application::themeGeneration = generation;
    Application::themeVariant    = variant;

    Application::loadThemeBackground();
    ThemeDecorations::update(variant);
    for (Activity* activity : Application::activitiesStack)
        if (View* view = activity->getContentView())
            view->onThemeChanged();
    Application::notificationManager->onThemeChanged();

    // The console records its clear colour once (deko3d): again
    Application::platform->setThemeVariant(variant);

    Application::themeChangedEvent.fire(variant);
    Application::requestRedraw();
}

int Application::getFont(std::string fontName)
{
    if (Application::fontStash.count(fontName) == 0)
        return FONT_INVALID;

    return Application::fontStash[fontName];
}

bool Application::addFontFallback(const std::string& font, const std::string& fallback)
{
    int base  = Application::getFont(font);
    int other = Application::getFont(fallback);
    if (base == FONT_INVALID || other == FONT_INVALID || base == other)
        return false;
    return nvgAddFallbackFontId(Application::getNVGContext(), base, other) != 0;
}

int Application::getDefaultFont()
{
    // Looked up for each label, not once: an app may register another font
    // under that name while it runs (a language with its own font), and the
    // labels made after it use it.
#ifdef __SWITCH__
    return Application::getFont(FONT_CHINESE_SIMPLIFIED);
#else
    return Application::getFont(FONT_REGULAR);
#endif
}

bool Application::XMLViewsRegisterContains(std::string name)
{
    return Application::xmlViewsRegister.count(canonicalXMLName(name)) > 0;
}

std::vector<std::string> Application::getXMLViewNames()
{
    std::vector<std::string> names;
    for (const auto& entry : Application::xmlViewsRegister)
        names.push_back(entry.first);
    return names;
}

XMLViewCreator Application::getXMLViewCreator(std::string name)
{
    return Application::xmlViewsRegister[canonicalXMLName(name)];
}

void Application::registerBuiltInXMLViews()
{
    Application::registerXMLView("aurore:Box", Box::create);
    Application::registerXMLView("aurore:Rectangle", Rectangle::create);
    Application::registerXMLView("aurore:AppletFrame", AppletFrame::create);
    Application::registerXMLView("aurore:Label", Label::create);
    Application::registerXMLView("aurore:TabFrame", TabFrame::create);
    Application::registerXMLView("aurore:Sidebar", Sidebar::create);
    Application::registerXMLView("aurore:Header", Header::create);
    Application::registerXMLView("aurore:ScrollingFrame", ScrollingFrame::create);
    Application::registerXMLView("aurore:HScrollingFrame", HScrollingFrame::create);
    Application::registerXMLView("aurore:RecyclerFrame", RecyclerFrame::create);
    Application::registerXMLView("aurore:RecyclingGrid", RecyclingGrid::create);
    Application::registerXMLView("aurore:HRecyclerFrame", HRecyclerFrame::create);
    Application::registerXMLView("aurore:Image", Image::create);
    Application::registerXMLView("aurore:Padding", Padding::create);
    Application::registerXMLView("aurore:Button", Button::create);
    Application::registerXMLView("aurore:IconButton", IconButton::create);
    Application::registerXMLView("aurore:CheckBox", CheckBox::create);
    Application::registerXMLView("aurore:Hints", Hints::create);
    Application::registerXMLView("aurore:Slider", Slider::create);
    Application::registerXMLView("aurore:BottomBar", BottomBar::create);
    Application::registerXMLView("aurore:ProgressSpinner", ProgressSpinner::create);

    // Cells
    Application::registerXMLView("aurore:DetailCell", DetailCell::create);
    Application::registerXMLView("aurore:RadioCell", RadioCell::create);
    Application::registerXMLView("aurore:BooleanCell", BooleanCell::create);
    Application::registerXMLView("aurore:SelectorCell", SelectorCell::create);
    Application::registerXMLView("aurore:InputCell", InputCell::create);
    Application::registerXMLView("aurore:InputNumericCell", InputNumericCell::create);
    Application::registerXMLView("aurore:SliderCell", SliderCell::create);

    // Widgets
    Application::registerXMLView("aurore:Account", AccountWidget::create);
    Application::registerXMLView("aurore:Battery", BatteryWidget::create);
    Application::registerXMLView("aurore:Wireless", WirelessWidget::create);
}

void Application::registerXMLView(std::string name, XMLViewCreator creator)
{
    // borealis' prefix (deprecated) still registers (and replaces) an aurore: view
    Application::xmlViewsRegister[canonicalXMLName(name)] = creator;
}

} // namespace aurore
