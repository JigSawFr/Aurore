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

// The test harness' platform: no window, no display. OpenGL draws into an
// offscreen framebuffer of a context made with EGL without any surface
// (Mesa's surfaceless platform: no X server needed), or of a hidden GLFW
// window where EGL is missing (macOS, Windows) or AURORE_HEADLESS_GLFW=1.
// Inputs come from the harness only; battery, network, clipboard, browser
// and keyboard are stand-ins, so screens look the same on every machine.

#include <aurore/platforms/desktop/desktop_platform.hpp>

#include <atomic>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace aurore
{

class HeadlessVideoContext : public VideoContext
{
  public:
    HeadlessVideoContext(int width, int height);
    ~HeadlessVideoContext() override;

    NVGcontext* getNVGContext() override { return nvgContext; }
    void clear(NVGcolor color) override;
    void beginFrame() override;
    void endFrame() override;
    void setSwapInterval(int interval) override { }
    void resetState() override;
    double getScaleFactor() override { return 1.0; }
    bool readPixels(std::vector<unsigned char>* rgba, int* width, int* height) override;

    // A new framebuffer size (the window resized)
    void resize(int width, int height);

    bool isValid() const { return nvgContext != nullptr; }

  private:
    bool createContext();
    void createFramebuffer();
    void destroyFramebuffer();

    int width, height;
    NVGcontext* nvgContext = nullptr;
    unsigned framebuffer = 0, colorBuffer = 0, depthStencilBuffer = 0;
    void* eglDisplay = nullptr;
    void* eglContext = nullptr;
    void* glfwWindow = nullptr;
};

class HeadlessInputManager : public InputManager
{
  public:
    short getControllersConnectedCount() override { return 1; }
    void updateUnifiedControllerState(ControllerState* state) override;
    void updateControllerState(ControllerState* state, int controller) override;
    bool getKeyboardKeyState(AuroreKeyboardScancode key) override;
    void updateTouchStates(std::vector<RawTouchState>* states) override;
    void updateMouseStates(RawMouseState* state) override;
    void sendRumble(unsigned short controller, unsigned short lowFreqMotor, unsigned short highFreqMotor) override { }

    void setButton(ControllerButton button, bool pressed);
    void setAxis(ControllerAxis axis, float value);
    void setKey(AuroreKeyboardScancode key, bool pressed);
    void setTouch(int fingerId, bool pressed, Point position);
    void setMouse(Point position, bool leftButton);
    void scroll(Point offset);

  private:
    bool buttons[_BUTTON_MAX] = {};
    float axes[_AXES_MAX]     = {};
    std::set<AuroreKeyboardScancode> keys;
    std::vector<RawTouchState> touches;
    RawMouseState mouse;
    ButtonLatch leftLatch;
};

// Answers the keyboard with the text the harness was given, at once
class HeadlessImeManager : public ImeManager
{
  public:
    bool openForText(std::function<void(std::string)> f, std::string headerText = "",
        std::string subText = "", int maxStringLength = 32, std::string initialText = "",
        int kbdDisableBitmask = KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE) override;
    bool openForNumber(std::function<void(long)> f, std::string headerText = "",
        std::string subText = "", int maxStringLength = 18, std::string initialText = "",
        std::string leftButton = "", std::string rightButton = "",
        int kbdDisableBitmask = KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE) override;

    // The next keyboard opened gets `text` (once); none: it is cancelled
    std::optional<std::string> nextText;
    // The header of the last keyboard opened
    std::string lastHeader;
};

class HeadlessPlatform : public DesktopPlatform
{
  public:
    HeadlessPlatform(int width, int height, std::string locale, ThemeVariant theme);
    ~HeadlessPlatform() override;

    std::string getName() override { return "Headless"; }
    void createWindow(std::string title, uint32_t width, uint32_t height, float windowXPos, float windowYPos) override;
    void setWindowSize(uint32_t width, uint32_t height) override;
    bool mainLoopIteration() override { return !closeRequested; }

    AudioPlayer* getAudioPlayer() override { return &audioPlayer; }
    VideoContext* getVideoContext() override { return videoContext; }
    InputManager* getInputManager() override { return &inputManager; }
    ImeManager* getImeManager() override { return &headlessImeManager; }

    bool canShowBatteryLevel() override { return showStatus; }
    bool canShowWirelessLevel() override { return showStatus; }
    int getBatteryLevel() override { return batteryLevel; }
    bool isBatteryCharging() override { return false; }
    bool hasWirelessConnection() override { return false; }
    int getWirelessLevel() override { return 0; }
    bool hasEthernetConnection() override { return true; }
    std::string getIpAddress() override { return "127.0.0.1"; }
    std::string getDnsServer() override { return "127.0.0.1"; }
    void disableScreenDimming(bool disable, const std::string& reason, const std::string& app) override { screenDimmingOff = disable; }
    bool isScreenDimmingDisabled() override { return screenDimmingOff; }
    void openBrowser(std::string url) override { openedUrls.push_back(url); }
    void pasteToClipboard(const std::string& text) override { clipboard = text; }
    std::string pasteFromClipboard() override { return clipboard; }

    HeadlessInputManager* getHeadlessInput() { return &inputManager; }
    HeadlessImeManager* getHeadlessIme() { return &headlessImeManager; }
    HeadlessVideoContext* getHeadlessVideo() { return videoContext; }

    // Ends the main loop on its next iteration
    void requestClose() { closeRequested = true; }

    // The links openBrowser() was asked to open
    std::vector<std::string> openedUrls;

    // The battery and Wi-Fi widgets shown (the widgets made after it is set)
    bool showStatus = false;
    // What getBatteryLevel() says (read on the task thread)
    std::atomic<int> batteryLevel { 100 };

  private:
    int width, height;
    NullAudioPlayer audioPlayer;
    HeadlessVideoContext* videoContext = nullptr;
    HeadlessInputManager inputManager;
    HeadlessImeManager headlessImeManager;
    std::string clipboard;
    bool screenDimmingOff = false;
    bool closeRequested   = false;
};

} // namespace aurore
