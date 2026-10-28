/*
    Copyright 2023 xfangfang

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

#include <strings.h>
#include <unistd.h> /* chdir */

#include <unordered_map>
#include <aurore/core/application.hpp>
#include <aurore/core/i18n.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/core/thread.hpp>
#include <aurore/platforms/sdl/sdl_platform.hpp>

#if defined(IOS) || defined(TVOS)
#include <sys/utsname.h>

namespace aurore
{
// ios_darwin.mm
Insets ios_safe_area(SDL_Window* window);
}

static bool isIPad()
{
    struct utsname systemInfo;
    uname(&systemInfo);
    return strncmp(systemInfo.machine, "iPad", 4) == 0;
}
#endif

namespace aurore
{

SDLPlatform::SDLPlatform()
{
#ifdef ANDROID
    // Enable Fullscreen on Android
    VideoContext::FULLSCREEN = true;
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
#elif defined(IOS) || defined(TVOS)
    // Enable Fullscreen on iOS
    VideoContext::FULLSCREEN = true;
    if (!isIPad())
    {
        SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    }
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
#elif defined(__APPLE__)
    // Same behavior as GLFW, change to the app's Resources directory if run in a ".app" bundle
    // Or to the executable's directory if run from other ways
    char* base_path = SDL_GetBasePath();
    if (base_path)
    {
        chdir(base_path);
        SDL_free(base_path);
    }
#endif

    // Init sdl
    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_TIMER) < 0)
    {
        Logger::error("sdl: failed to initialize");
        return;
    }

    // Platform impls
    this->audioPlayer = new NullAudioPlayer();

    // The system's language, unless AURORE_LANG gave one
    if (Platform::APP_LOCALE_DEFAULT == LOCALE_AUTO && !this->localeFromEnvironment)
    {
        SDL_Locale* locales = SDL_GetPreferredLocales();
        if (locales != nullptr && locales->language != nullptr)
        {
            // Every locale of Aurore (a browser in French gave English:
            // only five were known)
            std::string lang = std::string { locales->language };
            if (locales->country)
            {
                lang += "_" + std::string { locales->country };
            }
            this->locale = localeFromSystem(lang);
            aurore::Logger::info("Set app locale: {}", this->locale);
            SDL_free(locales);
        }
    }
}

void SDLPlatform::createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos)
{
    this->videoContext = new SDLVideoContext(windowTitle, windowWidth, windowHeight, windowXPos, windowYPos);
    this->inputManager = new SDLInputManager(this->videoContext->getSDLWindow());
    this->imeManager   = new SDLImeManager(&this->otherEvent);
}

void SDLPlatform::restoreWindow()
{
    SDL_RestoreWindow(this->videoContext->getSDLWindow());
}

void SDLPlatform::setWindowAlwaysOnTop(bool enable)
{
    SDL_SetWindowAlwaysOnTop(this->videoContext->getSDLWindow(), enable ? SDL_TRUE : SDL_FALSE);
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

bool SDLPlatform::isScreenDimmingDisabled()
{
    return !SDL_IsScreenSaverEnabled();
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
    return std::string { str };
}

std::string SDLPlatform::getName()
{
    return "SDL";
}

bool SDLPlatform::processEvent(SDL_Event* event)
{
    if (event->type == SDL_QUIT)
    {
        return false;
    }
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
    else if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP)
    {
        auto* manager = this->inputManager;
        if (manager)
            manager->updateKeyboardState(event->key);
    }
    else if (event->type == SDL_MOUSEMOTION)
    {
        auto* manager = this->inputManager;
        if (manager)
            manager->updateMouseMotion(event->motion);
    }
    else if (event->type == SDL_MOUSEWHEEL)
    {
        auto* manager = this->inputManager;
        if (manager)
            manager->updateMouseWheel(event->wheel);
    }
    else if (event->type == SDL_CONTROLLERSENSORUPDATE)
    {
        auto* manager = this->inputManager;
        if (manager)
            manager->updateControllerSensorsUpdate(event->csensor);
    }
#if defined(IOS) || defined(ANDROID)
    else if (event->type == SDL_APP_WILLENTERBACKGROUND)
    {
        aurore::Application::getWindowFocusChangedEvent()->fire(false);
    }
    else if (event->type == SDL_APP_WILLENTERFOREGROUND)
    {
        aurore::Application::getWindowFocusChangedEvent()->fire(true);
    }
    else if (event->type == SDL_APP_TERMINATING)
    {
        // OS 正要杀我们（Android swipe-to-recents / iOS 强杀）。在这个窗口期主动走
        // 清理路径，让 MPVCore::clean() + mpv_terminate_destroy 有机会释放
        // MediaCodec / Surface 等底层资源，避免 Tegra 驱动层 slot 泄漏。
        aurore::Application::quit();
        return false;
    }
#endif
    else if (event->type != SDL_POLLSENTINEL)
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
    return ios_safe_area(this->videoContext->getSDLWindow());
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
    delete this->audioPlayer;
    delete this->inputManager;
    delete this->imeManager;
    delete this->videoContext;
}

} // namespace aurore
