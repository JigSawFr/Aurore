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

#include <aurore/core/application.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/test/headless_platform.hpp>

#include <glad/glad.h>
#define NANOVG_GL3 1
#include <nanovg_gl.h>
#include <aurore/platforms/driver/gl_read_pixels.hpp>

#include <cstdlib>
#include <cstring>

#if defined(BRLS_HEADLESS_EGL)
#define EGL_NO_X11
#define MESA_EGL_NO_X11_HEADERS
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

#if defined(__GLFW__)
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#endif

namespace brls
{

// HeadlessVideoContext

HeadlessVideoContext::HeadlessVideoContext(int width, int height)
    : width(width)
    , height(height)
{
    if (!createContext())
    {
        Logger::error("headless: no OpenGL context (EGL surfaceless, or a hidden GLFW window)");
        return;
    }
    Logger::info("headless: GL Renderer: {}", (const char*)glGetString(GL_RENDERER));
    Logger::info("headless: GL Version: {}", (const char*)glGetString(GL_VERSION));

    createFramebuffer();
    this->nvgContext = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
    if (!this->nvgContext)
    {
        Logger::error("headless: unable to init nanovg");
        return;
    }
    Application::setWindowSize(width, height);
}

bool HeadlessVideoContext::createContext()
{
    const char* forceGlfw = std::getenv("BRLS_HEADLESS_GLFW");
    bool useGlfw          = forceGlfw && *forceGlfw && *forceGlfw != '0';

#if defined(BRLS_HEADLESS_EGL)
    if (!useGlfw)
    {
        // Mesa's surfaceless platform: a context with no window system at all
        auto getPlatformDisplay = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
        EGLDisplay display      = EGL_NO_DISPLAY;
        if (getPlatformDisplay)
            display = getPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
        EGLint major, minor;
        if (display != EGL_NO_DISPLAY && eglInitialize(display, &major, &minor) && eglBindAPI(EGL_OPENGL_API))
        {
            const EGLint configAttributes[] = { EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_SURFACE_TYPE, 0, EGL_NONE };
            EGLConfig config                = nullptr;
            EGLint configs                  = 0;
            eglChooseConfig(display, configAttributes, &config, 1, &configs);
            const EGLint contextAttributes[] = {
                EGL_CONTEXT_MAJOR_VERSION, 3,
                EGL_CONTEXT_MINOR_VERSION, 2,
                EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                EGL_NONE
            };
            EGLContext context = eglCreateContext(display, configs > 0 ? config : (EGLConfig) nullptr, EGL_NO_CONTEXT, contextAttributes);
            if (context != EGL_NO_CONTEXT && eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context)
                && gladLoadGLLoader((GLADloadproc)eglGetProcAddress))
            {
                this->eglDisplay = display;
                this->eglContext = context;
                Logger::info("headless: EGL {}.{} surfaceless", major, minor);
                return true;
            }
            if (context != EGL_NO_CONTEXT)
                eglDestroyContext(display, context);
            eglTerminate(display);
        }
        Logger::warning("headless: EGL surfaceless unavailable, trying a hidden GLFW window");
    }
#endif

#if defined(__GLFW__)
    if (!glfwInit())
        return false;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "headless", nullptr, nullptr);
    if (!window)
        return false;
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return false;
    this->glfwWindow = window;
    return true;
#else
    (void)useGlfw;
    return false;
#endif
}

void HeadlessVideoContext::createFramebuffer()
{
    glGenFramebuffers(1, &this->framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, this->framebuffer);

    glGenRenderbuffers(1, &this->colorBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, this->colorBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, this->width, this->height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, this->colorBuffer);

    // nanovg's strokes and fills use the stencil
    glGenRenderbuffers(1, &this->depthStencilBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, this->depthStencilBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, this->width, this->height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, this->depthStencilBuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        Logger::error("headless: framebuffer incomplete");
    glViewport(0, 0, this->width, this->height);
}

void HeadlessVideoContext::destroyFramebuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (this->depthStencilBuffer)
        glDeleteRenderbuffers(1, &this->depthStencilBuffer);
    if (this->colorBuffer)
        glDeleteRenderbuffers(1, &this->colorBuffer);
    if (this->framebuffer)
        glDeleteFramebuffers(1, &this->framebuffer);
    this->framebuffer = this->colorBuffer = this->depthStencilBuffer = 0;
}

void HeadlessVideoContext::resize(int newWidth, int newHeight)
{
    if (newWidth <= 0 || newHeight <= 0 || (newWidth == this->width && newHeight == this->height))
        return;
    this->width  = newWidth;
    this->height = newHeight;
    destroyFramebuffer();
    createFramebuffer();
    Application::setWindowSize(newWidth, newHeight);
    Application::getWindowSizeChangedEvent()->fire();
}

void HeadlessVideoContext::clear(NVGcolor color)
{
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void HeadlessVideoContext::beginFrame()
{
    glBindFramebuffer(GL_FRAMEBUFFER, this->framebuffer);
    glViewport(0, 0, this->width, this->height);
}

void HeadlessVideoContext::endFrame()
{
    // Nothing to present: the frame stays in the framebuffer for readPixels()
    glFlush();
}

void HeadlessVideoContext::resetState()
{
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
}

bool HeadlessVideoContext::readPixels(std::vector<unsigned char>* rgba, int* outWidth, int* outHeight)
{
    if (!this->framebuffer)
        return false;
    glBindFramebuffer(GL_FRAMEBUFFER, this->framebuffer);
    *outWidth  = this->width;
    *outHeight = this->height;
    return glReadPixelsTopDown(this->width, this->height, rgba);
}

HeadlessVideoContext::~HeadlessVideoContext()
{
    if (this->nvgContext)
        nvgDeleteGL3(this->nvgContext);
    if (this->eglContext || this->glfwWindow)
        destroyFramebuffer();
#if defined(BRLS_HEADLESS_EGL)
    if (this->eglContext)
    {
        eglMakeCurrent((EGLDisplay)this->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext((EGLDisplay)this->eglDisplay, (EGLContext)this->eglContext);
        eglTerminate((EGLDisplay)this->eglDisplay);
    }
#endif
#if defined(__GLFW__)
    if (this->glfwWindow)
    {
        glfwDestroyWindow((GLFWwindow*)this->glfwWindow);
        glfwTerminate();
    }
#endif
}

// HeadlessInputManager

void HeadlessInputManager::updateControllerState(ControllerState* state, int controller)
{
    for (int i = 0; i < _BUTTON_MAX; i++)
        state->buttons[i] = this->buttons[i];
    for (int i = 0; i < _AXES_MAX; i++)
        state->axes[i] = this->axes[i];
}

void HeadlessInputManager::updateUnifiedControllerState(ControllerState* state)
{
    updateControllerState(state, 0);

    // The keyboard on top of the buttons, as GLFW maps it
    bool swap   = Application::isSwapInputKeys();
    bool enter  = keys.count(BRLS_KBD_KEY_ENTER) || keys.count(BRLS_KBD_KEY_KP_ENTER);
    bool escape = keys.count(BRLS_KBD_KEY_ESCAPE) > 0;
    state->buttons[swap ? BUTTON_B : BUTTON_A] |= enter;
    state->buttons[swap ? BUTTON_A : BUTTON_B] |= escape;
    state->buttons[BUTTON_UP] |= keys.count(BRLS_KBD_KEY_UP) > 0;
    state->buttons[BUTTON_RIGHT] |= keys.count(BRLS_KBD_KEY_RIGHT) > 0;
    state->buttons[BUTTON_DOWN] |= keys.count(BRLS_KBD_KEY_DOWN) > 0;
    state->buttons[BUTTON_LEFT] |= keys.count(BRLS_KBD_KEY_LEFT) > 0;

    state->buttons[BUTTON_NAV_UP] |= state->buttons[BUTTON_UP] || axes[LEFT_Y] < -0.5f || axes[RIGHT_Y] < -0.5f;
    state->buttons[BUTTON_NAV_RIGHT] |= state->buttons[BUTTON_RIGHT] || axes[LEFT_X] > 0.5f || axes[RIGHT_X] > 0.5f;
    state->buttons[BUTTON_NAV_DOWN] |= state->buttons[BUTTON_DOWN] || axes[LEFT_Y] > 0.5f || axes[RIGHT_Y] > 0.5f;
    state->buttons[BUTTON_NAV_LEFT] |= state->buttons[BUTTON_LEFT] || axes[LEFT_X] < -0.5f || axes[RIGHT_X] < -0.5f;
}

bool HeadlessInputManager::getKeyboardKeyState(BrlsKeyboardScancode key)
{
    return keys.count(key) > 0;
}

void HeadlessInputManager::updateTouchStates(std::vector<RawTouchState>* states)
{
    // A finger lifted is no longer there: the library ends its touch
    for (auto& touch : this->touches)
        states->push_back(touch);
}

void HeadlessInputManager::updateMouseStates(RawMouseState* state)
{
    this->mouse.leftButton = this->leftLatch.frame();
    *state                 = this->mouse;
    // Offsets and scrolling are per frame
    this->mouse.offset = Point();
    this->mouse.scroll = Point();
}

void HeadlessInputManager::setButton(ControllerButton button, bool pressed)
{
    this->buttons[button] = pressed;
}

void HeadlessInputManager::setAxis(ControllerAxis axis, float value)
{
    this->axes[axis] = value;
}

void HeadlessInputManager::setKey(BrlsKeyboardScancode key, bool pressed)
{
    if (pressed == (keys.count(key) > 0))
        return;
    if (pressed)
        keys.insert(key);
    else
        keys.erase(key);
    KeyState state;
    state.key     = key;
    state.mods    = 0;
    state.pressed = pressed;
    getKeyboardKeyStateChanged()->fire(state);
}

void HeadlessInputManager::setTouch(int fingerId, bool pressed, Point position)
{
    for (auto it = this->touches.begin(); it != this->touches.end(); ++it)
    {
        if (it->fingerId == fingerId)
        {
            if (pressed)
                it->position = position;
            else
                this->touches.erase(it);
            return;
        }
    }
    if (!pressed)
        return;
    RawTouchState touch;
    touch.fingerId = fingerId;
    touch.pressed  = true;
    touch.position = position;
    this->touches.push_back(touch);
}

void HeadlessInputManager::setMouse(Point position, bool leftButton)
{
    this->mouse.offset.x += position.x - this->mouse.position.x;
    this->mouse.offset.y += position.y - this->mouse.position.y;
    this->mouse.position = position;
    // As the GLFW platform: from press and release events, a click let go
    // before the next frame still counts
    if (leftButton)
        this->leftLatch.press();
    else
        this->leftLatch.release();
}

void HeadlessInputManager::scroll(Point offset)
{
    this->mouse.scroll.x += offset.x;
    this->mouse.scroll.y += offset.y;
}

// HeadlessImeManager

bool HeadlessImeManager::openForText(std::function<void(std::string)> f, std::string headerText,
    std::string subText, int maxStringLength, std::string initialText, int kbdDisableBitmask)
{
    this->lastHeader = headerText;
    if (!this->nextText)
        return false;
    std::string text = *this->nextText;
    this->nextText.reset();
    f(text);
    return true;
}

bool HeadlessImeManager::openForNumber(std::function<void(long)> f, std::string headerText,
    std::string subText, int maxStringLength, std::string initialText,
    std::string leftButton, std::string rightButton, int kbdDisableBitmask)
{
    this->lastHeader = headerText;
    if (!this->nextText)
        return false;
    std::string text = *this->nextText;
    this->nextText.reset();
    f(std::strtol(text.c_str(), nullptr, 10));
    return true;
}

// HeadlessPlatform

HeadlessPlatform::HeadlessPlatform(int width, int height, std::string locale, ThemeVariant theme)
    : width(width)
    , height(height)
{
    // Not the machine's: the same screens everywhere
    this->locale       = std::move(locale);
    this->themeVariant = theme;
}

void HeadlessPlatform::createWindow(std::string title, uint32_t, uint32_t, float, float)
{
    this->videoContext = new HeadlessVideoContext(this->width, this->height);
    if (!this->videoContext->isValid())
        fatal("headless: could not create an OpenGL context");
}

void HeadlessPlatform::setWindowSize(uint32_t newWidth, uint32_t newHeight)
{
    this->width  = (int)newWidth;
    this->height = (int)newHeight;
    if (this->videoContext)
        this->videoContext->resize((int)newWidth, (int)newHeight);
}

HeadlessPlatform::~HeadlessPlatform()
{
    delete this->videoContext;
}

} // namespace brls
