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

#include <cstring>
#include <aurore/core/logger.hpp>
#include <aurore/platforms/driver/sdlgpu.hpp>

#define NANOVG_SDLGPU_IMPLEMENTATION
#include <nanovg_sdlgpu.h>

namespace aurore
{

// The frame's texture: RGBA, as readPixels gives it
static const SDL_GPUTextureFormat COLOR_FORMAT = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

SDLGPUContext::SDLGPUContext(SDL_Window* window)
    : window(window)
{
    // SDL_GPU_DRIVER (vulkan, metal) picks the driver; AURORE_GPU_DEBUG=1
    // turns the driver's validation on
    const char* debug = SDL_getenv("AURORE_GPU_DEBUG");
    this->device      = SDL_CreateGPUDevice(NVG_SDLGPU_SHADER_FORMATS, debug && strcmp(debug, "1") == 0, nullptr);
    if (!this->device)
    {
        Logger::error("sdlgpu: no GPU device: {}", SDL_GetError());
        return;
    }
    if (!SDL_ClaimWindowForGPUDevice(this->device, window))
    {
        Logger::error("sdlgpu: the window cannot be drawn by the GPU device: {}", SDL_GetError());
        SDL_DestroyGPUDevice(this->device);
        this->device = nullptr;
        return;
    }
    Logger::info("sdlgpu: {} driver", this->getDriverName());

    // nanovg fills its shapes through the stencil: D24S8, or D32S8 where it
    // is not (Apple's GPUs)
    const SDL_GPUTextureFormat stencils[] = { SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
        SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT };
    for (SDL_GPUTextureFormat format : stencils)
    {
        if (SDL_GPUTextureSupportsFormat(this->device, format, SDL_GPU_TEXTURETYPE_2D,
                SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
        {
            this->depthStencilFormat = format;
            break;
        }
    }
    if (this->depthStencilFormat == SDL_GPU_TEXTUREFORMAT_INVALID)
    {
        Logger::error("sdlgpu: no depth-stencil format");
        return;
    }

    this->vg = nvgCreateSDLGPU(this->device, COLOR_FORMAT, this->depthStencilFormat, NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    if (!this->vg)
        Logger::error("sdlgpu: nanovg's renderer could not be made");
}

SDLGPUContext::~SDLGPUContext()
{
    if (!this->device)
        return;
    if (this->cmd)
        SDL_CancelGPUCommandBuffer(this->cmd);
    SDL_WaitForGPUIdle(this->device);
    if (this->vg)
        nvgDeleteSDLGPU(this->vg);
    this->releaseTargets();
    SDL_ReleaseWindowFromGPUDevice(this->device, this->window);
    SDL_DestroyGPUDevice(this->device);
}

const char* SDLGPUContext::getDriverName()
{
    return this->device ? SDL_GetGPUDeviceDriver(this->device) : "none";
}

void SDLGPUContext::releaseTargets()
{
    // Released once the GPU is done with them
    if (this->color)
        SDL_ReleaseGPUTexture(this->device, this->color);
    if (this->depthStencil)
        SDL_ReleaseGPUTexture(this->device, this->depthStencil);
    this->color        = nullptr;
    this->depthStencil = nullptr;
    this->targetWidth = this->targetHeight = 0;
}

bool SDLGPUContext::makeTargets()
{
    if (this->width <= 0 || this->height <= 0)
        SDL_GetWindowSizeInPixels(this->window, &this->width, &this->height);
    if (this->color && this->width == this->targetWidth && this->height == this->targetHeight)
        return true;
    this->releaseTargets();
    if (this->width <= 0 || this->height <= 0)
        return false;

    SDL_GPUTextureCreateInfo info {};
    info.type                 = SDL_GPU_TEXTURETYPE_2D;
    info.width                = (Uint32)this->width;
    info.height               = (Uint32)this->height;
    info.layer_count_or_depth = 1;
    info.num_levels           = 1;
    info.format               = COLOR_FORMAT;
    // Drawn into, then copied onto the swapchain (a blit samples it)
    info.usage         = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    this->color        = SDL_CreateGPUTexture(this->device, &info);
    info.format        = this->depthStencilFormat;
    info.usage         = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    this->depthStencil = SDL_CreateGPUTexture(this->device, &info);
    if (!this->color || !this->depthStencil)
    {
        Logger::error("sdlgpu: no {}x{} frame texture: {}", this->width, this->height, SDL_GetError());
        this->releaseTargets();
        return false;
    }
    this->targetWidth  = this->width;
    this->targetHeight = this->height;
    return true;
}

void SDLGPUContext::resize(int width, int height)
{
    // The textures follow at the next frame's start: one may be drawn now
    this->width  = width;
    this->height = height;
}

void SDLGPUContext::beginFrame()
{
    if (!this->vg || this->cmd || !this->makeTargets())
        return;
    this->cmd = SDL_AcquireGPUCommandBuffer(this->device);
    if (!this->cmd)
    {
        Logger::error("sdlgpu: no command buffer: {}", SDL_GetError());
        return;
    }
    nvgSDLGPUSetTarget(this->vg, this->cmd, this->color, this->depthStencil, nullptr);
}

void SDLGPUContext::clear(NVGcolor color)
{
    if (!this->cmd)
        return;
    SDL_FColor clear = { color.r, color.g, color.b, color.a };
    nvgSDLGPUSetTarget(this->vg, this->cmd, this->color, this->depthStencil, &clear);
}

void SDLGPUContext::endFrame()
{
    if (!this->cmd)
        return;
    SDL_GPUTexture* swapchain = nullptr;
    Uint32 swapWidth = 0, swapHeight = 0;
    // No texture while the window is hidden or minimized: the frame is
    // still submitted
    if (SDL_WaitAndAcquireGPUSwapchainTexture(this->cmd, this->window, &swapchain, &swapWidth, &swapHeight)
        && swapchain)
    {
        SDL_GPUBlitInfo blit {};
        blit.source.texture      = this->color;
        blit.source.w            = (Uint32)this->targetWidth;
        blit.source.h            = (Uint32)this->targetHeight;
        blit.destination.texture = swapchain;
        blit.destination.w       = swapWidth;
        blit.destination.h       = swapHeight;
        blit.load_op             = SDL_GPU_LOADOP_DONT_CARE;
        blit.filter              = SDL_GPU_FILTER_NEAREST;
        SDL_BlitGPUTexture(this->cmd, &blit);
    }
    SDL_SubmitGPUCommandBuffer(this->cmd);
    this->cmd = nullptr;
    nvgSDLGPUSetTarget(this->vg, nullptr, nullptr, nullptr, nullptr);
}

void SDLGPUContext::setSwapInterval(int interval)
{
    if (!this->device)
        return;
    // Immediate, else mailbox, when vsync is off; vsync is always there
    SDL_GPUPresentMode mode = SDL_GPU_PRESENTMODE_VSYNC;
    if (interval == 0)
    {
        if (SDL_WindowSupportsGPUPresentMode(this->device, this->window, SDL_GPU_PRESENTMODE_IMMEDIATE))
            mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
        else if (SDL_WindowSupportsGPUPresentMode(this->device, this->window, SDL_GPU_PRESENTMODE_MAILBOX))
            mode = SDL_GPU_PRESENTMODE_MAILBOX;
    }
    SDL_SetGPUSwapchainParameters(this->device, this->window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode);
}

bool SDLGPUContext::readPixels(std::vector<unsigned char>* rgba, int* width, int* height)
{
    if (!this->color)
        return false;
    const Uint32 size = (Uint32)this->targetWidth * (Uint32)this->targetHeight * 4;

    SDL_GPUTransferBufferCreateInfo transferInfo {};
    transferInfo.usage              = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    transferInfo.size               = size;
    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(this->device, &transferInfo);
    if (!transfer)
        return false;

    // In a frame, its draws come first: its command buffer is submitted
    // with the copy, and the rest of the frame goes in a new one
    bool inFrame              = this->cmd != nullptr;
    SDL_GPUCommandBuffer* cmd = inFrame ? this->cmd : SDL_AcquireGPUCommandBuffer(this->device);
    if (!cmd)
    {
        SDL_ReleaseGPUTransferBuffer(this->device, transfer);
        return false;
    }
    SDL_GPUTextureRegion region {};
    region.texture = this->color;
    region.w       = (Uint32)this->targetWidth;
    region.h       = (Uint32)this->targetHeight;
    region.d       = 1;
    SDL_GPUTextureTransferInfo destination {};
    destination.transfer_buffer = transfer;
    SDL_GPUCopyPass* copy       = SDL_BeginGPUCopyPass(cmd);
    SDL_DownloadFromGPUTexture(copy, &region, &destination);
    SDL_EndGPUCopyPass(copy);
    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    if (inFrame)
    {
        this->cmd = SDL_AcquireGPUCommandBuffer(this->device);
        nvgSDLGPUSetTarget(this->vg, this->cmd, this->color, this->depthStencil, nullptr);
    }
    bool read = false;
    if (fence && SDL_WaitForGPUFences(this->device, true, &fence, 1))
    {
        const unsigned char* pixels = (const unsigned char*)SDL_MapGPUTransferBuffer(this->device, transfer, false);
        if (pixels)
        {
            rgba->assign(pixels, pixels + size);
            SDL_UnmapGPUTransferBuffer(this->device, transfer);
            *width  = this->targetWidth;
            *height = this->targetHeight;
            read    = true;
        }
    }
    if (fence)
        SDL_ReleaseGPUFence(this->device, fence);
    SDL_ReleaseGPUTransferBuffer(this->device, transfer);
    return read;
}

} // namespace aurore
