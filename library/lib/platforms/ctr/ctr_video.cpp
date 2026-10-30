// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>

#define NANOVG_CITRO3D_IMPLEMENTATION
#include <nanovg_citro3d.h>

#include <aurore/core/application.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/core/top_screen.hpp>
#include <aurore/platforms/ctr/ctr_video.hpp>

namespace aurore
{

namespace
{
    // The render target's RGBA8 to the screen's RGB8, as it is
    constexpr u32 DISPLAY_TRANSFER_FLAGS = GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0)
        | GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8)
        | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO);

    // The render target's clear colour: RGBA8, R in the high byte
    u32 clearColor(NVGcolor c)
    {
        auto byte = [](float v)
        { return (u32)(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f); };
        return byte(c.r) << 24 | byte(c.g) << 16 | byte(c.b) << 8 | byte(c.a);
    }
} // namespace

CtrVideoContext::CtrVideoContext()
{
    gfxInitDefault();

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 2))
    {
        Logger::error("3DS: citro3d could not start");
        return;
    }
    // The screens are portrait: the bottom one is 240 wide and 320 high.
    // Depth and stencil for nanovg's nonzero fills.
    this->target = C3D_RenderTargetCreate(HEIGHT, WIDTH, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    if (!this->target)
    {
        Logger::error("3DS: no render target for the bottom screen");
        return;
    }
    C3D_RenderTargetSetOutput(this->target, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    // The top screen, 240 wide and 400 high, its left eye only (no
    // stereoscopic 3D): the activity's top screen view
    this->topTarget = C3D_RenderTargetCreate(TOP_HEIGHT, TOP_WIDTH, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    if (this->topTarget)
        C3D_RenderTargetSetOutput(this->topTarget, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    else
        Logger::error("3DS: no render target for the top screen");

    // The edges antialiased: a coverage texture (nanovg_citro3d_aa.h)
    this->nvgContext = nvgCreateC3D(NVG_ANTIALIAS);
    if (!this->nvgContext)
    {
        Logger::error("3DS: nanovg could not be created");
        return;
    }
    Application::setWindowSize(WIDTH, HEIGHT);
    if (this->topTarget)
        TopScreen::setSize(TOP_WIDTH, TOP_HEIGHT);
    // A frame redrawn only when something changed: the CPU is short for a
    // full redraw at 60 frames a second (docs/platforms.md)
    Application::setRedrawOnDemand(true);
}

CtrVideoContext::~CtrVideoContext()
{
    if (this->inFrame)
        C3D_FrameEnd(0);
    if (this->nvgContext)
        nvgDeleteC3D(this->nvgContext);
    if (this->target)
        C3D_RenderTargetDelete(this->target);
    if (this->topTarget)
        C3D_RenderTargetDelete(this->topTarget);
    C3D_Fini();
    gfxExit();
}

void CtrVideoContext::beginFrame()
{
    // Waits for the vertical blank and for the GPU to finish the last frame
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    this->inFrame = true;
    if (this->nvgContext)
        nvgC3DFrameBegin(this->nvgContext);
}

void CtrVideoContext::clear(NVGcolor color)
{
    if (!this->target)
        return;
    C3D_RenderTargetClear(this->target, C3D_CLEAR_ALL, clearColor(color), 0);
    // nanovg draws there at nvgEndFrame
    C3D_FrameDrawOn(this->target);
}

bool CtrVideoContext::beginTopScreen(NVGcolor color)
{
    if (!this->topTarget || !this->inFrame)
        return false;
    C3D_RenderTargetClear(this->topTarget, C3D_CLEAR_ALL, clearColor(color), 0);
    C3D_FrameDrawOn(this->topTarget);
    return true;
}

void CtrVideoContext::endFrame()
{
    if (!this->inFrame)
        return;
    C3D_FrameEnd(0);
    this->inFrame = false;
}

} // namespace aurore
