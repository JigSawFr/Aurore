// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-FileCopyrightText: 2023 xfangfang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/platforms/desktop/desktop_platform.hpp>
#include <aurore/platforms/glfw/glfw_ime.hpp>
#include <aurore/platforms/glfw/glfw_input.hpp>
#include <aurore/platforms/glfw/glfw_video.hpp>

namespace aurore
{

class GLFWPlatform : public DesktopPlatform
{
  public:
    GLFWPlatform();
    ~GLFWPlatform() override;

    std::string getName() override;
    void createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos) override;

    void restoreWindow() override;
    void setWindowSize(uint32_t windowWidth, uint32_t windowHeight) override;
    void setWindowSizeLimits(uint32_t windowMinWidth, uint32_t windowMinHeight, uint32_t windowMaxWidth, uint32_t windowMaxHeight) override;
    void setWindowPosition(int windowXPos, int windowYPos) override;
    void setWindowState(uint32_t windowWidth, uint32_t windowHeight, int windowXPos, int windowYPos) override;

    void setWindowAlwaysOnTop(bool enable) override;
    void pasteToClipboard(const std::string& text) override;
    std::string pasteFromClipboard() override;
    bool mainLoopIteration() override;

    AudioPlayer* getAudioPlayer() override;
    VideoContext* getVideoContext() override;
    InputManager* getInputManager() override;
    ImeManager* getImeManager() override;

  private:
    NullAudioPlayer* audioPlayer   = nullptr;
    GLFWVideoContext* videoContext = nullptr;
    GLFWInputManager* inputManager = nullptr;
    GLFWImeManager* imeManager     = nullptr;
};

} // namespace aurore
