// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <nanovg.h>

#include <vector>
#include <aurore/platforms/sdl/sdl.hpp>

namespace aurore
{

// AURORE_GRAPHICS=SDLGPU: SDL3's GPU API (Direct3D 12, Vulkan, Metal) under the SDL3
// window. Each frame is drawn by nanovg (nanovg_sdlgpu.h) into a texture of
// the window's size in pixels, then copied onto the window's swapchain: the
// texture is what readPixels reads, as GL reads its framebuffer.
class SDLGPUContext
{
  public:
    explicit SDLGPUContext(SDL_Window* window);
    ~SDLGPUContext();

    /// The device and its nanovg renderer were made
    bool isValid() const { return this->vg != nullptr; }
    NVGcontext* getNVGContext() { return this->vg; }
    /// Vulkan, Metal…: SDL_GPU's driver
    const char* getDriverName();

    /// The window's size in pixels: the frame's texture follows it
    void resize(int width, int height);
    void beginFrame();
    void clear(NVGcolor color);
    void endFrame();
    void setSwapInterval(int interval);
    bool readPixels(std::vector<unsigned char>* rgba, int* width, int* height);

  private:
    SDL_Window* window                      = nullptr;
    SDL_GPUDevice* device                   = nullptr;
    NVGcontext* vg                          = nullptr;
    SDL_GPUTextureFormat depthStencilFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
    SDL_GPUTexture* color                   = nullptr;
    SDL_GPUTexture* depthStencil            = nullptr;
    /// The textures' size, and the window's they follow at the next frame
    int targetWidth = 0, targetHeight = 0;
    int width = 0, height = 0;
    /// The frame's, from beginFrame to endFrame
    SDL_GPUCommandBuffer* cmd = nullptr;

    bool makeTargets();
    void releaseTargets();
};

} // namespace aurore
