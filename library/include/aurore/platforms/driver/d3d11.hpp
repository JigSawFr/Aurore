// SPDX-FileCopyrightText: 2023 zeromake
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <d3d11.h>
#include <d3d11_1.h>
#include <nanovg.h>

#ifdef __GLFW__
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#elif defined(__SDL__)
#include <aurore/platforms/sdl/sdl.hpp>
#endif

namespace aurore
{

class D3D11Context
{
  public:
#ifdef __GLFW__
    D3D11Context(GLFWwindow* window, int width, int height);
#elif defined(__SDL__)
    D3D11Context(SDL_Window* window, int width, int height);
#endif
    ~D3D11Context();

    double getScaleFactor();
    void clear(NVGcolor color);
    void beginFrame();
    void endFrame();
    void setSwapInterval(int value);

    bool onFramebufferSize(int width, int height, bool init = false);

    ID3D11Device* getDevice() { return this->device; }

    IDXGISwapChain* getSwapChain() { return this->swapChain; }

  private:
    ID3D11Device* device                     = nullptr;
    ID3D11DeviceContext* deviceContext       = nullptr;
    IDXGISwapChain1* swapChain               = nullptr;
    ID3D11RenderTargetView* renderTargetView = nullptr;
    ID3D11DepthStencilView* depthStencilView = nullptr;
    int interval                             = 1;

    UINT(WINAPI* GetDpiForWindow)
    (HWND);

    HWND hWnd = nullptr;

    bool initDX(HWND window, IUnknown* coreWindow, int width, int height);
    void unInitDX();
};

}
