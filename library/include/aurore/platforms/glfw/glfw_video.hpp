// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/video.hpp>

#ifdef __PSV__
#define GLFW_INCLUDE_ES2
#else
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

namespace aurore
{

// GLFW Video Context
class GLFWVideoContext : public VideoContext
{
  public:
    GLFWVideoContext(const std::string& windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowX = NAN, float windowY = NAN);
    ~GLFWVideoContext() override;

    NVGcontext* getNVGContext() override;

    void clear(NVGcolor color) override;
    void beginFrame() override;
    void endFrame() override;
    void setSwapInterval(int interval) override;
    void resetState() override;
    double getScaleFactor() override;
    bool readPixels(std::vector<unsigned char>* rgba, int* width, int* height) override;
    void fullScreen(bool fs) override;
    int getCurrentMonitorIndex() override;

    GLFWwindow* getGLFWWindow();

  private:
    GLFWwindow* window     = nullptr;
    NVGcontext* nvgContext = nullptr;

#ifdef __SWITCH__
    int oldWidth, oldHeight;
    GLFWmonitor* monitor = nullptr;
#endif
};

} // namespace aurore
