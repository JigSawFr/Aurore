// SPDX-FileCopyrightText: 2023 xfangfang
// SPDX-License-Identifier: Apache-2.0

#include <strings.h>
#include <unistd.h> /* chdir */

#include <unordered_map>
#include <aurore/core/application.hpp>
#include <aurore/core/i18n.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/core/thread.hpp>
#include <aurore/platforms/sdl/sdl_platform.hpp>

#include "sdl_compat.hpp"

#if defined(IOS) || defined(TVOS)
#include <sys/utsname.h>

#ifndef __SDL3__
namespace aurore
{
// ios_darwin.mm
Insets ios_safe_area(SDL_Window* window);
}
#endif

static bool isIPad()
{
    struct utsname systemInfo;
    uname(&systemInfo);
    return strncmp(systemInfo.machine, "iPad", 4) == 0;
}
#endif

namespace aurore
{

#if defined(__SDL3__) && (defined(IOS) || defined(ANDROID))
// SDL3 asks for SDL_EVENT_TERMINATING to be handled as it is sent, in an
// event watcher: the system may end the app before the loop polls it
static bool sdlTerminatingWatcher(void*, SDL_Event* event)
{
    if (event->type == SDL_EVENT_TERMINATING)
        aurore::Application::quit();
    return true;
}
#endif

SDLPlatform::SDLPlatform()
{
#ifdef ANDROID
    // Enable Fullscreen on Android
    VideoContext::FULLSCREEN = true;
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#ifndef __SDL3__
    // SDL3 has no accelerometer joystick
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
#endif
#elif defined(IOS) || defined(TVOS)
    // Enable Fullscreen on iOS
    VideoContext::FULLSCREEN = true;
    if (!isIPad())
    {
        SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    }
#ifndef __SDL3__
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
#endif
#elif defined(__APPLE__)
    // Same behavior as GLFW, change to the app's Resources directory if run in a ".app" bundle
    // Or to the executable's directory if run from other ways
#ifdef __SDL3__
    // SDL3 keeps the path: it is not freed
    const char* base_path = SDL_GetBasePath();
    if (base_path)
        chdir(base_path);
#else
    char* base_path = SDL_GetBasePath();
    if (base_path)
    {
        chdir(base_path);
        SDL_free(base_path);
    }
#endif
#endif

    // Init sdl (SDL3 has no timer subsystem to start)
#ifdef __SDL3__
    Uint32 initFlags = SDL_INIT_EVENTS;
#else
    Uint32 initFlags = SDL_INIT_EVENTS | SDL_INIT_TIMER;
#endif
    if (!AURORE_SDL_OK(SDL_Init(initFlags)))
    {
        Logger::error("sdl: failed to initialize");
        return;
    }
#if defined(__SDL3__) && (defined(IOS) || defined(ANDROID))
    SDL_AddEventWatch(sdlTerminatingWatcher, nullptr);
#endif

    // Platform impls
    this->audioPlayer = new NullAudioPlayer();

    // The system's language, unless AURORE_LANG gave one
    if (Platform::APP_LOCALE_DEFAULT == LOCALE_AUTO && !this->localeFromEnvironment)
    {
#ifdef __SDL3__
        // An array of pointers, ending with a null one
        SDL_Locale** locales    = SDL_GetPreferredLocales(nullptr);
        const SDL_Locale* first = locales ? locales[0] : nullptr;
#else
        SDL_Locale* locales     = SDL_GetPreferredLocales();
        const SDL_Locale* first = locales;
#endif
        if (first != nullptr && first->language != nullptr)
        {
            // Every locale of Aurore (a browser in French gave English:
            // only five were known)
            std::string lang = std::string { first->language };
            if (first->country)
            {
                lang += "_" + std::string { first->country };
            }
            this->locale = localeFromSystem(lang);
            aurore::Logger::info("Set app locale: {}", this->locale);
        }
        SDL_free(locales);
    }
}

void SDLPlatform::createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos)
{
    this->videoContext = new SDLVideoContext(windowTitle, windowWidth, windowHeight, windowXPos, windowYPos);
    this->inputManager = new SDLInputManager(this->videoContext->getSDLWindow());
    this->imeManager   = new SDLImeManager(&this->otherEvent);

#ifdef __SDL3__
    // The system's light or dark, which SDL3 reads everywhere (Linux's
    // desktop portal too), unless AURORE_THEME gave one. Its video must be
    // started first.
    if (!this->themeFromEnvironment)
    {
        this->themeFollowsSystem = true;
        this->readSystemTheme();
    }
#endif
}

#ifdef __SDL3__
void SDLPlatform::readSystemTheme()
{
    SDL_SystemTheme theme = SDL_GetSystemTheme();
    if (theme == SDL_SYSTEM_THEME_DARK)
        this->systemThemeChanged(ThemeVariant::DARK);
    else if (theme == SDL_SYSTEM_THEME_LIGHT)
        this->systemThemeChanged(ThemeVariant::LIGHT);
}
#endif

void SDLPlatform::restoreWindow()
{
    SDL_RestoreWindow(this->videoContext->getSDLWindow());
}

void SDLPlatform::setWindowAlwaysOnTop(bool enable)
{
#ifdef __SDL3__
    SDL_SetWindowAlwaysOnTop(this->videoContext->getSDLWindow(), enable);
#else
    SDL_SetWindowAlwaysOnTop(this->videoContext->getSDLWindow(), enable ? SDL_TRUE : SDL_FALSE);
#endif
}

void SDLPlatform::setWindowSize(uint32_t windowWidth, uint32_t windowHeight)
{
    if (windowWidth > 0 && windowHeight > 0)
    {
        SDL_SetWindowSize(this->videoContext->getSDLWindow(), windowWidth, windowHeight);
    }
}

void SDLPlatform::setWindowSizeLimits(uint32_t windowMinWidth, uint32_t windowMinHeight, uint32_t windowMaxWidth, uint32_t windowMaxHeight)
{
    if (windowMinWidth > 0 && windowMinHeight > 0)
        SDL_SetWindowMinimumSize(this->videoContext->getSDLWindow(), windowMinWidth, windowMinHeight);
    if ((windowMaxWidth > 0 && windowMaxHeight > 0) && (windowMaxWidth > windowMinWidth && windowMaxHeight > windowMinHeight))
        SDL_SetWindowMaximumSize(this->videoContext->getSDLWindow(), windowMaxWidth, windowMaxHeight);
}

void SDLPlatform::setWindowPosition(int windowXPos, int windowYPos)
{
    SDL_SetWindowPosition(this->videoContext->getSDLWindow(), windowXPos, windowYPos);
}

void SDLPlatform::setWindowState(uint32_t windowWidth, uint32_t windowHeight, int windowXPos, int windowYPos)
{
    if (windowWidth > 0 && windowHeight > 0)
    {
        SDL_Window* win = this->videoContext->getSDLWindow();
        SDL_RestoreWindow(win);
        SDL_SetWindowSize(win, windowWidth, windowHeight);
        SDL_SetWindowPosition(win, windowXPos, windowYPos);
    }
}

void SDLPlatform::disableScreenDimming(bool disable, const std::string& reason, const std::string& app)
{
    if (disable)
    {
        SDL_DisableScreenSaver();
    }
    else
    {
        SDL_EnableScreenSaver();
    }
}

#if !defined(IOS) && !defined(TVOS) && !defined(__EMSCRIPTEN__)
// A battery SDL can read: one on battery, charging or charged (not none,
// unknown, nor SDL3's error)
static bool sdlHasBattery(SDL_PowerState state)
{
    return state == SDL_POWERSTATE_ON_BATTERY || state == SDL_POWERSTATE_CHARGING || state == SDL_POWERSTATE_CHARGED;
}

bool SDLPlatform::canShowBatteryLevel()
{
    return sdlHasBattery(SDL_GetPowerInfo(nullptr, nullptr));
}

int SDLPlatform::getBatteryLevel()
{
    int percent = -1;
    SDL_GetPowerInfo(nullptr, &percent);
    // Charged without a percentage: full
    return percent < 0 ? 100 : percent;
}

bool SDLPlatform::isBatteryCharging()
{
    return SDL_GetPowerInfo(nullptr, nullptr) == SDL_POWERSTATE_CHARGING;
}
#endif

bool SDLPlatform::isScreenDimmingDisabled()
{
    return !SDL_ScreenSaverEnabled();
}

void SDLPlatform::pasteToClipboard(const std::string& text)
{
    SDL_SetClipboardText(text.c_str());
}

std::string SDLPlatform::pasteFromClipboard()
{
    char* str = SDL_GetClipboardText();
    if (!str)
        return "";
    std::string text { str };
    SDL_free(str);
    return text;
}

std::string SDLPlatform::getName()
{
    return "SDL";
}

bool SDLPlatform::processEvent(SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT)
    {
        return false;
    }
#ifdef __SDL3__
    else if (event->type == SDL_EVENT_WINDOW_MOUSE_LEAVE || event->type == SDL_EVENT_WINDOW_MOUSE_ENTER)
    {
        auto* manager = this->inputManager;
        if (manager)
            manager->getMouseCusorEntered()->fire(event->type == SDL_EVENT_WINDOW_MOUSE_ENTER);
    }
    else if (event->type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED)
    {
        if (this->videoContext)
            this->videoContext->displayScaleChanged();
    }
    else if (event->type == SDL_EVENT_SYSTEM_THEME_CHANGED)
    {
        // Kept for followSystemTheme when the app chose one; the next frame
        // redraws in it otherwise (Application::checkThemeChanged)
        if (!this->themeFromEnvironment)
            this->readSystemTheme();
    }
    else if (event->type == SDL_EVENT_WINDOW_SAFE_AREA_CHANGED || event->type == SDL_EVENT_WINDOW_SHOWN
        || event->type == SDL_EVENT_WINDOW_RESIZED || event->type == SDL_EVENT_WINDOW_MAXIMIZED
        || event->type == SDL_EVENT_WINDOW_RESTORED || event->type == SDL_EVENT_WINDOW_FOCUS_GAINED
        || event->type == SDL_EVENT_DISPLAY_ORIENTATION)
    {
        this->safeAreaMayHaveChanged();
    }
#else
    else if (event->type == SDL_WINDOWEVENT)
    {
        switch (event->window.event)
        {
            case SDL_WINDOWEVENT_SHOWN:
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_MAXIMIZED:
            case SDL_WINDOWEVENT_RESTORED:
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                this->safeAreaMayHaveChanged();
                break;
            default:
                break;
        }
        auto* manager = this->inputManager;
        if (event->window.event == SDL_WINDOWEVENT_LEAVE)
        {
            if (manager)
                manager->getMouseCusorEntered()->fire(false);
        }
        else if (event->window.event == SDL_WINDOWEVENT_ENTER)
        {
            if (manager)
                manager->getMouseCusorEntered()->fire(true);
        }
    }
    else if (event->type == SDL_DISPLAYEVENT)
    {
        // A turn of 180 degrees: the same size, the notch on the other side
        this->safeAreaMayHaveChanged();
    }
#endif
    else if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
    {
        auto* manager = this->inputManager;
        if (manager)
            manager->updateKeyboardState(event->key);
    }
    else if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        auto* manager = this->inputManager;
        if (manager)
            manager->updateMouseMotion(event->motion);
    }
    else if (event->type == SDL_EVENT_MOUSE_WHEEL)
    {
        auto* manager = this->inputManager;
        if (manager)
            manager->updateMouseWheel(event->wheel);
    }
    else if (event->type == SDL_EVENT_GAMEPAD_SENSOR_UPDATE)
    {
        auto* manager = this->inputManager;
#ifdef __SDL3__
        if (manager)
            manager->updateControllerSensorsUpdate(event->gsensor);
#else
        if (manager)
            manager->updateControllerSensorsUpdate(event->csensor);
#endif
    }
#if defined(IOS) || defined(ANDROID)
    else if (event->type == SDL_EVENT_WILL_ENTER_BACKGROUND)
    {
        aurore::Application::getWindowFocusChangedEvent()->fire(false);
    }
    else if (event->type == SDL_EVENT_WILL_ENTER_FOREGROUND)
    {
        aurore::Application::getWindowFocusChangedEvent()->fire(true);
    }
    else if (event->type == SDL_EVENT_TERMINATING)
    {
        // OS 正要杀我们（Android swipe-to-recents / iOS 强杀）。在这个窗口期主动走
        // 清理路径，让 MPVCore::clean() + mpv_terminate_destroy 有机会释放
        // MediaCodec / Surface 等底层资源，避免 Tegra 驱动层 slot 泄漏。
        aurore::Application::quit();
        return false;
    }
#endif
    else if (event->type != SDL_EVENT_POLL_SENTINEL)
    {
        // 其它没有处理的事件
        this->otherEvent.fire(event);
    }
    aurore::Application::setActiveEvent(true);
    return true;
}

void SDLPlatform::safeAreaMayHaveChanged()
{
#if defined(IOS) || defined(TVOS) || defined(ANDROID)
    if (!this->videoContext)
        return;
    Application::updateSafeArea();
    // The system lays the window out again after telling us
    aurore::cancelDelay(this->safeAreaCheck);
    this->safeAreaCheck = aurore::delay(300, []()
        { Application::updateSafeArea(); });
#endif
}

#if defined(IOS) || defined(TVOS)
Insets SDLPlatform::getSafeAreaInsets()
{
    if (!this->videoContext)
        return {};
#ifdef __SDL3__
    // The part of the window, in points, nothing covers
    SDL_Window* window = this->videoContext->getSDLWindow();
    SDL_Rect safe;
    int width, height;
    if (!SDL_GetWindowSafeArea(window, &safe) || !SDL_GetWindowSize(window, &width, &height))
        return {};
    return { (float)safe.y, (float)(width - safe.x - safe.w), (float)(height - safe.y - safe.h), (float)safe.x };
#else
    return ios_safe_area(this->videoContext->getSDLWindow());
#endif
}
#endif

bool SDLPlatform::mainLoopIteration()
{
    SDL_Event event;
    bool hasEvent = false;
    while (SDL_PollEvent(&event))
    {
        if (!processEvent(&event))
        {
            return false;
        }
        hasEvent = true;
    }
    if (!hasEvent && !Application::hasActiveEvent())
    {
        if (SDL_WaitEventTimeout(&event, (int)(aurore::Application::getDeactivatedFrameTime() * 1000))
            && !processEvent(&event))
        {
            return false;
        }
    }
    return true;
}

AudioPlayer* SDLPlatform::getAudioPlayer()
{
    return this->audioPlayer;
}

VideoContext* SDLPlatform::getVideoContext()
{
    return this->videoContext;
}

InputManager* SDLPlatform::getInputManager()
{
    return this->inputManager;
}

ImeManager* SDLPlatform::getImeManager()
{
    return this->imeManager;
}

SDLPlatform::~SDLPlatform()
{
#if defined(__SDL3__) && (defined(IOS) || defined(ANDROID))
    SDL_RemoveEventWatch(sdlTerminatingWatcher, nullptr);
#endif
    delete this->audioPlayer;
    delete this->inputManager;
    delete this->imeManager;
    delete this->videoContext;
}

} // namespace aurore
