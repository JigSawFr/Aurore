// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/application.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/platforms/glfw/glfw_video.hpp>

#include <cmath>

// nanovg implementation
#ifdef AURORE_USE_OPENGL
#include <glad/gl.h>
#ifdef USE_GL2
#define NANOVG_GL2_IMPLEMENTATION
#elif defined(USE_GLES2)
#define NANOVG_GLES2_IMPLEMENTATION
#elif defined(USE_GLES3)
#define NANOVG_GLES3_IMPLEMENTATION
#else
#define NANOVG_GL3_IMPLEMENTATION
#endif /* USE_GL2 */
#include <nanovg_gl.h>
#include <aurore/platforms/driver/gl_loader.hpp>
#include <aurore/platforms/driver/gl_read_pixels.hpp>
#elif defined(AURORE_USE_METAL)
static void* METAL_CONTEXT = nullptr;
#include <aurore/platforms/glfw/driver/metal.hpp>
#endif

#if defined(__linux__) || defined(_WIN32)
#include <nanovg.h>
#ifdef USE_LIBROMFS
#include <romfs/romfs.hpp>
#else
#include <unistd.h>
#endif
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace aurore
{

static double scaleFactor = 1.0;

static int mini(int x, int y)
{
    return x < y ? x : y;
}

static int maxi(int x, int y)
{
    return x > y ? x : y;
}

static GLFWmonitor* getCurrentMonitor(GLFWwindow* window)
{
    int nmonitors, i;
    int wx, wy, ww, wh;
    int mx, my, mw, mh;
    int overlap, bestoverlap;
    GLFWmonitor* bestmonitor;
    GLFWmonitor** monitors;
    const GLFWvidmode* mode;

    bestoverlap = 0;
    bestmonitor = NULL;

    glfwGetWindowPos(window, &wx, &wy);
    glfwGetWindowSize(window, &ww, &wh);
    monitors = glfwGetMonitors(&nmonitors);

    for (i = 0; i < nmonitors; i++)
    {
        mode = glfwGetVideoMode(monitors[i]);
        glfwGetMonitorPos(monitors[i], &mx, &my);
        mw = mode->width;
        mh = mode->height;

        overlap = maxi(0, mini(wx + ww, mx + mw) - maxi(wx, mx)) * maxi(0, mini(wy + wh, my + mh) - maxi(wy, my));

        if (bestoverlap < overlap)
        {
            bestoverlap = overlap;
            bestmonitor = monitors[i];
        }
    }

    return bestmonitor;
}

static GLFWmonitor* getAvailableMonitor(int index, int x, int y, int w, int h)
{
    int count;
    auto** monitors      = glfwGetMonitors(&count);
    GLFWmonitor* monitor = nullptr;
    if (index < count)
        monitor = monitors[index];
    else
        return nullptr;

    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int monitorX, monitorY;
    glfwGetMonitorPos(monitor, &monitorX, &monitorY);

    if (x < monitorX || y < monitorY || x + w > mode->width + monitorX || y + h > mode->height + monitorY)
        return nullptr;
    return monitor;
}

static void glfwWindowFramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    if (!width || !height)
        return;
#ifdef AURORE_USE_OPENGL
    glViewport(0, 0, width, height);
    int wWidth, wHeight;
    glfwGetWindowSize(window, &wWidth, &wHeight);
    scaleFactor = width * 1.0 / wWidth;
#elif defined(AURORE_USE_METAL)
    if (METAL_CONTEXT == nullptr)
    {
        return;
    }
    scaleFactor = GetMetalScaleFactor(METAL_CONTEXT);
    ResizeMetalDrawable(METAL_CONTEXT, width, height);
    int wWidth, wHeight;
    glfwGetWindowSize(window, &wWidth, &wHeight);
    // cocoa 画布大小和窗口一致
    width  = wWidth;
    height = wHeight;
#endif

    Application::onWindowResized(width, height);

    if (!VideoContext::FULLSCREEN)
    {
        VideoContext::sizeW = wWidth;
        VideoContext::sizeH = wHeight;
    }
}

static void glfwWindowPositionCallback(GLFWwindow* window, int windowXPos, int windowYPos)
{
    Application::onWindowReposition(windowXPos, windowYPos);

    if (!VideoContext::FULLSCREEN)
    {
        VideoContext::posX = (float)windowXPos;
        VideoContext::posY = (float)windowYPos;
    }
}

GLFWVideoContext::GLFWVideoContext(const std::string& windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowX, float windowY)
{
    if (!glfwInit())
    {
        fatal("glfw: failed to initialize");
    }

    // Create window
#ifdef AURORE_USE_OPENGL
#if defined(USE_GLES2)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#elif defined(USE_GLES3)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#elif defined(USE_GL2)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
#elif defined(__SWITCH__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#elif defined(__linux__) || defined(__APPLE__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(USE_EGL)
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
#endif
    glfwWindowHint(GLFW_COCOA_GRAPHICS_SWITCHING, GL_TRUE);
#endif
#else
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

#if defined(__APPLE__) || defined(__linux__) || defined(_WIN32)
    // If the window appears outside the screen, using the default settings
    GLFWmonitor* monitor = nullptr;
    if (!std::isnan(windowX) && !std::isnan(windowY))
        monitor = getAvailableMonitor(VideoContext::monitorIndex, (int)windowX, (int)windowY, (int)windowWidth, (int)windowHeight);

    if (monitor == nullptr)
    {
        windowX      = NAN;
        windowY      = NAN;
        windowWidth  = Application::ORIGINAL_WINDOW_WIDTH;
        windowHeight = Application::ORIGINAL_WINDOW_HEIGHT;
        monitor      = glfwGetPrimaryMonitor();
    }
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    glfwWindowHint(GLFW_AUTO_ICONIFY, 0);
    glfwWindowHint(GLFW_SOFT_FULLSCREEN, 1);
#endif
#if defined(__linux__) || defined(_WIN32) || defined(__APPLE__)
    // Set the application ID so desktop environments can match this window to its
    // .desktop entry (Wayland app_id / X11 WM_CLASS) — required for taskbar icons.
    const std::string& appId = Application::getAppId();
    if (!appId.empty())
    {
#ifdef GLFW_WAYLAND_APP_ID
        glfwWindowHintString(GLFW_WAYLAND_APP_ID, appId.c_str());
#endif
#ifdef GLFW_X11_CLASS_NAME
        glfwWindowHintString(GLFW_X11_CLASS_NAME, appId.c_str());
#endif
#ifdef GLFW_X11_INSTANCE_NAME
        glfwWindowHintString(GLFW_X11_INSTANCE_NAME, appId.c_str());
#endif
    }
#endif

// create window
#if defined(__linux__) || defined(_WIN32) || defined(__APPLE__)
    if (VideoContext::FULLSCREEN)
    {
        this->window = glfwCreateWindow(mode->width, mode->height, windowTitle.c_str(), monitor, nullptr);
#ifdef _WIN32
        // glfw will disable screen sleep when in full-screen mode
        // We will cancel it here and let the application handle this issue internally
        // X11 and wayland may have similar issues
        SetThreadExecutionState(ES_CONTINUOUS);
#endif
    }
    else
#endif
    {
        this->window = glfwCreateWindow((int)windowWidth, (int)windowHeight, windowTitle.c_str(), nullptr, nullptr);
    }

    if (!this->window)
    {
        glfwTerminate();
        fatal("glfw: Failed to create window");
        return;
    }

#if defined(__linux__) || defined(_WIN32)
    // Set window icon
    GLFWimage images[1];
#ifdef USE_LIBROMFS
    try
    {
        auto& icon       = romfs::get("icon/icon.png");
        images[0].pixels = nvgDecodeImageMem((const unsigned char*)icon.data(), (int)icon.size(), &images[0].width, &images[0].height);
        if (images[0].pixels)
            glfwSetWindowIcon(this->window, 1, images);
        nvgFreeImageData(images[0].pixels);
    }
    catch (...)
    {
    }
#else
    const char* icon_path = AURORE_ASSET("icon/icon.png");
    if (access(icon_path, F_OK) != -1)
    {
        images[0].pixels = nvgDecodeImage(icon_path, &images[0].width, &images[0].height);
        if (images[0].pixels)
            glfwSetWindowIcon(this->window, 1, images);
        nvgFreeImageData(images[0].pixels);
    }
#endif
#endif

#if defined(__APPLE__) || defined(__linux__) || defined(_WIN32)
    // Set window position
    if (!VideoContext::FULLSCREEN)
    {
        if (mode->width >= (int)windowWidth && mode->height >= (int)windowHeight)
        {
            if (!std::isnan(windowX) && !std::isnan(windowY))
            {
                glfwSetWindowPos(this->window, (int)windowX, (int)windowY);
            }
            else
            {
                // When there is no specified window position, center the window
                glfwSetWindowPos(this->window, (int)(mode->width - windowWidth) / 2,
                    (int)(mode->height - windowHeight) / 2);
            }
        }
        else
        {
            // When the window size is too large, reduce the size and center it
            float maxAllowedWidth  = (float)mode->width * 0.8f;
            float maxAllowedHeight = (float)mode->height * 0.8f;
            float scale            = std::min(maxAllowedWidth / (float)windowWidth, maxAllowedHeight / (float)windowHeight);
            float newWidth         = (float)windowWidth * scale;
            float newHeight        = (float)windowHeight * scale;

            glfwSetWindowSize(this->window, (int)newWidth, (int)newHeight);

            glfwSetWindowPos(this->window, (mode->width - newWidth) / 2,
                (mode->height - newHeight) / 2);
        }
    }
#endif

    // Configure window
    // The mouse buttons are read from their events (GLFWInputManager), so a
    // short click or a touchpad tap is not lost between two frames
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetFramebufferSizeCallback(window, glfwWindowFramebufferSizeCallback);
    glfwSetWindowPosCallback(window, glfwWindowPositionCallback);

#ifdef AURORE_USE_OPENGL
    glfwMakeContextCurrent(window);
    if (!loadGL((GLADloadfunc)glfwGetProcAddress, "glfw"))
        fatal("glfw: the OpenGL context cannot be used (see the log)");

    Logger::info("glfw: GL Vendor: {}", (const char*)glGetString(GL_VENDOR));
    Logger::info("glfw: GL Renderer: {}", (const char*)glGetString(GL_RENDERER));
    Logger::info("glfw: GL Version: {}", (const char*)glGetString(GL_VERSION));
#endif
    Logger::info("glfw: GLFW Version: {}.{}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);

    // Initialize nanovg
#ifdef AURORE_USE_OPENGL
#ifdef USE_GLES2
    this->nvgContext = nvgCreateGLES2(0);
#elif defined(USE_GLES3)
    this->nvgContext = nvgCreateGLES3(0);
#elif defined(USE_GL2)
    this->nvgContext = nvgCreateGL2(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
#else
    this->nvgContext = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
#endif
#elif defined(AURORE_USE_METAL)
    Logger::info("glfw: USE_METAL");
    void* ctx        = CreateMetalContext(window);
    METAL_CONTEXT    = ctx;
    this->nvgContext = nvgCreateMTL(GetMetalLayer(ctx), NVG_STENCIL_STROKES | NVG_ANTIALIAS);
    scaleFactor      = GetMetalScaleFactor(ctx);
#endif
    if (!this->nvgContext)
    {
        Logger::error("glfw: unable to init nanovg");
        glfwTerminate();
        return;
    }

    // Not a virtual call from the constructor
    GLFWVideoContext::setSwapInterval(VideoContext::swapInterval);

    // Setup window state
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    Application::setWindowSize(width, height);

    int wWidth, wHeight;
    glfwGetWindowSize(window, &wWidth, &wHeight);

    int xPos, yPos;
    glfwGetWindowPos(window, &xPos, &yPos);
    Application::setWindowPosition(xPos, yPos);

#ifndef AURORE_USE_METAL
    scaleFactor = width * 1.0 / wWidth;
    glViewport(0, 0, width, height);
#endif

    if (!VideoContext::FULLSCREEN)
    {
        VideoContext::sizeW = wWidth;
        VideoContext::sizeH = wHeight;
        VideoContext::posX  = (float)xPos;
        VideoContext::posY  = (float)yPos;
    }

#ifdef __SWITCH__
    this->monitor    = glfwGetPrimaryMonitor();
    const char* name = glfwGetMonitorName(monitor);
    aurore::Logger::info("glfw: Monitor: {}", name);
#endif
}

void GLFWVideoContext::beginFrame()
{
#ifdef __SWITCH__
    const GLFWvidmode* r = glfwGetVideoMode(monitor);

    if (oldWidth != r->width || oldHeight != r->height)
    {
        oldWidth  = r->width;
        oldHeight = r->height;

        glfwSetWindowSize(window, r->width, r->height);
    }
#endif
}

void GLFWVideoContext::endFrame()
{
#ifdef AURORE_USE_OPENGL
    glfwSwapBuffers(this->window);
#endif
}

void GLFWVideoContext::setSwapInterval(int interval)
{
    VideoContext::swapInterval = interval;
    glfwSwapInterval(interval);
}

void GLFWVideoContext::clear(NVGcolor color)
{

#ifdef AURORE_USE_OPENGL
    glClearColor(
        color.r,
        color.g,
        color.b,
        color.a);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#elif defined(AURORE_USE_METAL)
    nvgClearWithColor(nvgContext, nvgRGBAf(color.r, color.g, color.b, color.a));
#endif
}

void GLFWVideoContext::resetState()
{
#ifdef AURORE_USE_OPENGL
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
#endif
}

double GLFWVideoContext::getScaleFactor()
{
    return scaleFactor;
}

bool GLFWVideoContext::readPixels(std::vector<unsigned char>* rgba, int* width, int* height)
{
#ifdef AURORE_USE_OPENGL
    *width  = (int)Application::framebufferWidth;
    *height = (int)Application::framebufferHeight;
    return glReadPixelsTopDown(*width, *height, rgba);
#else
    return false;
#endif
}

GLFWVideoContext::~GLFWVideoContext()
{
    try
    {
        if (this->nvgContext)
        {
#ifdef AURORE_USE_OPENGL
#ifdef USE_GLES2
            nvgDeleteGLES2(this->nvgContext);
#elif defined(USE_GLES3)
            nvgDeleteGLES3(this->nvgContext);
#elif defined(USE_GL2)
            nvgDeleteGL2(this->nvgContext);
#else
            nvgDeleteGL3(this->nvgContext);
#endif
#elif defined(AURORE_USE_METAL)
            nvgDeleteMTL(this->nvgContext);
            METAL_CONTEXT = nullptr;
#endif
        }
    }
    catch (...)
    {
        Logger::error("Cannot delete nvg Context");
    }
    glfwDestroyWindow(this->window);
    glfwTerminate();
}

NVGcontext* GLFWVideoContext::getNVGContext()
{
    return this->nvgContext;
}

int GLFWVideoContext::getCurrentMonitorIndex()
{
    if (!this->window)
        return 0;

    int count;
    auto* monitor   = getCurrentMonitor(this->window);
    auto** monitors = glfwGetMonitors(&count);
    for (int i = 0; i < count; i++)
    {
        if (monitor == monitors[i])
            return i;
    }
    return 0;
}

void GLFWVideoContext::fullScreen(bool fs)
{
    VideoContext::FULLSCREEN = fs;

    aurore::Logger::info("Set fullscreen: {}", fs);
    if (fs)
    {
        GLFWmonitor* monitor    = getCurrentMonitor(this->window);
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        VideoContext::monitorIndex = getCurrentMonitorIndex();
        glfwSetWindowMonitor(this->window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
#ifdef _WIN32
        // glfw will disable screen sleep when in full-screen mode
        // We will cancel it here and let the application handle this issue internally
        // X11 and wayland may have similar issues
        if (!Application::getPlatform()->isScreenDimmingDisabled())
        {
            SetThreadExecutionState(ES_CONTINUOUS);
        }
#endif
    }
    else
    {
        GLFWmonitor* monitor = glfwGetWindowMonitor(this->window);
        // already in windowed mode
        if (monitor == nullptr)
            return;
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        int monitorX, monitorY;
        glfwGetMonitorPos(monitor, &monitorX, &monitorY);
        glfwRestoreWindow(this->window);

        if (sizeW == 0 || sizeH == 0 || posX < monitorX || posY < monitorY || posX + sizeW > mode->width + monitorX || posY + sizeH > mode->height + monitorY)
        {
            int width  = Application::ORIGINAL_WINDOW_WIDTH;
            int height = Application::ORIGINAL_WINDOW_HEIGHT;
            // If the window appears outside the screen, using the default settings
            glfwSetWindowMonitor(this->window, nullptr, fabs(mode->width - width) / 2,
                fabs(mode->height - height) / 2, width, height, GLFW_DONT_CARE);
        }
        else
        {
            // Set the window position and size
            glfwSetWindowMonitor(this->window, nullptr, (int)posX, (int)posY, (int)sizeW, (int)sizeH, mode->refreshRate);
        }
    }
#ifdef AURORE_USE_OPENGL
    glfwSwapInterval(VideoContext::swapInterval);
#endif
}

GLFWwindow* GLFWVideoContext::getGLFWWindow()
{
    return this->window;
}

} // namespace aurore
