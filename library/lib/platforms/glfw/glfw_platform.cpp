// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-FileCopyrightText: 2023 xfangfang
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/application.hpp>
#include <aurore/core/i18n.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/platforms/glfw/glfw_platform.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <strings.h>

// glfw video and input code inspired from the glfw hybrid app by fincs
// https://github.com/fincs/hybrid_app

namespace aurore
{

#if defined(__APPLE__)
std::string darwin_locale();
#endif

static void glfwErrorCallback(int errorCode, const char* description)
{
    switch (errorCode)
    {
        case GLFW_API_UNAVAILABLE:
            Logger::error("OpenGL is unavailable: {}", description);
            break;
        case GLFW_VERSION_UNAVAILABLE:
            Logger::error("OpenGL 3.2 (the minimum requirement) is not available: {}", description);
            break;
        default:
            Logger::error("GLFW {}: {}", errorCode, description);
    }
}

GLFWPlatform::GLFWPlatform()
{
    // Init glfw
    glfwSetErrorCallback(glfwErrorCallback);
    glfwInitHint(GLFW_JOYSTICK_HAT_BUTTONS, GLFW_TRUE);

    if (!glfwInit())
    {
        Logger::error("glfw: failed to initialize");
        return;
    }

    // Misc
    glfwSetTime(0.0);

    // The system's language, unless AURORE_LANG gave one
    if (Platform::APP_LOCALE_DEFAULT == LOCALE_AUTO && !this->localeFromEnvironment)
    {
#if defined(_WIN32)
        // The display language ("es" and "MX": es-419), not only its
        // primary language
        LCID lcid         = MAKELCID(GetUserDefaultUILanguage(), SORT_DEFAULT);
        char language[16] = "", region[16] = "";
        if (GetLocaleInfoA(lcid, LOCALE_SISO639LANGNAME, language, sizeof(language)) > 0)
        {
            std::string lang = language;
            if (GetLocaleInfoA(lcid, LOCALE_SISO3166CTRYNAME, region, sizeof(region)) > 0)
                lang += std::string("_") + region;
            this->locale = localeFromSystem(lang);
        }
#elif defined(__APPLE__)
        // "fr-CA", "zh-Hans-CN", "es-419": read whole
        this->locale = localeFromSystem(darwin_locale());
#elif defined(__linux__)
        // The variables in the order the C library reads them (LANGUAGE
        // may list several, "fr:en": the first one)
        const char* langEnv = nullptr;
        for (const char* var : { "LANGUAGE", "LC_ALL", "LC_MESSAGES", "LANG" })
        {
            const char* value = getenv(var);
            if (value && *value && strcmp(value, "C") != 0 && strcmp(value, "POSIX") != 0)
            {
                langEnv = value;
                break;
            }
        }
        if (langEnv)
        {
            std::string lang = langEnv;
            lang             = lang.substr(0, lang.find(':'));
            this->locale     = localeFromSystem(lang);
        }
#endif
        aurore::Logger::info("Auto detect app locale: {}", this->locale);
    }

    // Platform impls
    this->audioPlayer = new NullAudioPlayer();
}

void GLFWPlatform::createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos)
{
    this->videoContext = new GLFWVideoContext(windowTitle, windowWidth, windowHeight, windowXPos, windowYPos);
    GLFWwindow* win    = this->videoContext->getGLFWWindow();
    this->inputManager = new GLFWInputManager(win);
    this->imeManager   = new GLFWImeManager(win);
}

void GLFWPlatform::setWindowAlwaysOnTop(bool enable)
{
    glfwSetWindowAttrib(this->videoContext->getGLFWWindow(), GLFW_FLOATING, enable ? GLFW_TRUE : GLFW_FALSE);
}

void GLFWPlatform::restoreWindow()
{
    glfwRestoreWindow(this->videoContext->getGLFWWindow());
}

void GLFWPlatform::setWindowSize(uint32_t windowWidth, uint32_t windowHeight)
{
    if (windowWidth > 0 && windowHeight > 0)
    {
        glfwSetWindowSize(this->videoContext->getGLFWWindow(), windowWidth, windowHeight);
    }
}

void GLFWPlatform::setWindowSizeLimits(uint32_t windowMinWidth, uint32_t windowMinHeight, uint32_t windowMaxWidth, uint32_t windowMaxHeight)
{
    if (windowMinWidth > 0 && windowMinHeight > 0)
        glfwSetWindowSizeLimits(this->videoContext->getGLFWWindow(), windowMinWidth, windowMinHeight, GLFW_DONT_CARE, GLFW_DONT_CARE);
    if ((windowMaxWidth > 0 && windowMaxHeight > 0) && (windowMaxHeight > windowMinWidth && windowMaxHeight > windowMinHeight))
        glfwSetWindowSizeLimits(this->videoContext->getGLFWWindow(), GLFW_DONT_CARE, GLFW_DONT_CARE, windowMaxWidth, windowMaxHeight);
}

void GLFWPlatform::setWindowPosition(int windowXPos, int windowYPos)
{
    glfwSetWindowPos(this->videoContext->getGLFWWindow(), windowXPos, windowYPos);
}

void GLFWPlatform::setWindowState(uint32_t windowWidth, uint32_t windowHeight, int windowXPos, int windowYPos)
{
    if (windowWidth > 0 && windowHeight > 0)
    {
        GLFWwindow* win = this->videoContext->getGLFWWindow();
        glfwRestoreWindow(win);
        glfwSetWindowMonitor(win, nullptr, windowXPos, windowYPos, windowWidth, windowHeight, 0);
    }
}

void GLFWPlatform::pasteToClipboard(const std::string& text)
{
    glfwSetClipboardString(this->videoContext->getGLFWWindow(), text.c_str());
}

std::string GLFWPlatform::pasteFromClipboard()
{
    const char* str = glfwGetClipboardString(this->videoContext->getGLFWWindow());
    if (!str)
        return "";
    return std::string { str };
}

std::string GLFWPlatform::getName()
{
    return "GLFW";
}

bool GLFWPlatform::mainLoopIteration()
{
    bool isActive;
    do
    {
        isActive = !glfwGetWindowAttrib(this->videoContext->getGLFWWindow(), GLFW_ICONIFIED);

        if (isActive)
        {
            glfwPollEvents();
            if (!Application::hasActiveEvent())
            {
                glfwWaitEventsTimeout(Application::getDeactivatedFrameTime());
            }
        }
        else
        {
            glfwWaitEvents();
        }
    } while (!isActive);

    return !glfwWindowShouldClose(this->videoContext->getGLFWWindow());
}

AudioPlayer* GLFWPlatform::getAudioPlayer()
{
    return this->audioPlayer;
}

VideoContext* GLFWPlatform::getVideoContext()
{
    return this->videoContext;
}

InputManager* GLFWPlatform::getInputManager()
{
    return this->inputManager;
}

ImeManager* GLFWPlatform::getImeManager()
{
    return this->imeManager;
}

GLFWPlatform::~GLFWPlatform()
{
    delete this->audioPlayer;
    delete this->inputManager;
    delete this->imeManager;
    delete this->videoContext;
}

} // namespace aurore
