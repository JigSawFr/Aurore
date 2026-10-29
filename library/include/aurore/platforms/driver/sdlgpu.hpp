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
