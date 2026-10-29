// SPDX-FileCopyrightText: 2024 xfangfang
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/application.hpp>
#include <aurore/platforms/psv/psv_video.hpp>

#define NANOVG_GXM_IMPLEMENTATION
#define NANOVG_GXM_UTILS_IMPLEMENTATION
#include <nanovg_gxm.h>

namespace aurore
{

PsvVideoContext::PsvVideoContext()
{
#ifdef USE_VITA_SHARK
    if (shark_init("app0:module/libshacccg.suprx") < 0)
    {
        sceClibPrintf("vitashark: failed to initialize");
        return;
    }
#endif

    // Initialize nanovg
    NVGXMinitOptions initOptions = {
        .msaa           = SCE_GXM_MULTISAMPLE_4X,
        .swapInterval   = VideoContext::swapInterval,
        .dumpShader     = 0,
        .scenesPerFrame = 1,
    };

    window = gxmCreateWindow(&initOptions);
    if (window == NULL)
    {
        fatal("gxm: failed to initialize");
    }

    this->nvgContext = nvgCreateGXM(window->context, window->shader_patcher, 0);
    if (!this->nvgContext)
    {
        aurore::fatal("gxm: unable to init nanovg");
    }

    Application::setWindowSize(DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

PsvVideoContext::~PsvVideoContext()
{
    try
    {
        if (this->nvgContext)
        {
            nvgDeleteGXM(this->nvgContext);
            gxmDeleteWindow(window);
        }
    }
    catch (...)
    {
        Logger::error("Cannot delete nvg Context");
    }
#ifdef USE_VITA_SHARK
    shark_end();
#endif
}

void PsvVideoContext::clear(NVGcolor color)
{
    gxmClearColor(color.r, color.g, color.b, color.a);
}

void PsvVideoContext::resetState() { }

void PsvVideoContext::beginFrame()
{
    gxmBeginFrame();
    gxmClear();
}

void PsvVideoContext::endFrame()
{
    gxmEndFrame();
    gxmDialogUpdate();
    gxmSwapBuffer();
}

void PsvVideoContext::setSwapInterval(int interval)
{
    VideoContext::swapInterval = interval;
    gxmSwapInterval(interval);
}

double PsvVideoContext::getScaleFactor()
{
    return 1.0;
}

NVGcontext* PsvVideoContext::getNVGContext()
{
    return this->nvgContext;
}

NVGXMwindow* PsvVideoContext::getWindow()
{
    return this->window;
}

}