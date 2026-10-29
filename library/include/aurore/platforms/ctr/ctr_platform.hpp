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

// The New 3DS, through libctru: its system's language and battery, the
// small screen preset (480x360 layout units on the 320x240 screen), and
// citro3d on the bottom screen. No input nor keyboard yet: the buttons are
// scanned each frame but not passed to Aurore (docs/platforms.md).

#pragma once

#include <aurore/core/audio.hpp>
#include <aurore/core/font.hpp>
#include <aurore/core/ime.hpp>
#include <aurore/core/input.hpp>
#include <aurore/core/platform.hpp>
#include <aurore/platforms/ctr/ctr_video.hpp>

namespace aurore
{

/// No controller nor touch read yet: every button up, no touch
class CtrInputManager : public InputManager
{
  public:
    short getControllersConnectedCount() override { return 1; }
    void updateUnifiedControllerState(ControllerState* state) override;
    void updateControllerState(ControllerState* state, int controller) override;
    bool getKeyboardKeyState(AuroreKeyboardScancode key) override { return false; }
    void updateTouchStates(std::vector<RawTouchState>* states) override { }
    void updateMouseStates(RawMouseState* state) override { }
    void sendRumble(unsigned short controller, unsigned short lowFreqMotor, unsigned short highFreqMotor) override { }
};

/// No keyboard yet (libctru's swkbd later): nothing is typed
class CtrImeManager : public ImeManager
{
  public:
    bool openForText(std::function<void(std::string)> f, std::string headerText, std::string subText,
        int maxStringLength, std::string initialText, int kbdDisableBitmask) override
    {
        return false;
    }
    bool openForNumber(std::function<void(long)> f, std::string headerText, std::string subText,
        int maxStringLength, std::string initialText, std::string leftButton, std::string rightButton,
        int kbdDisableBitmask) override
    {
        return false;
    }
};

/// The library's fonts, from the romfs
class CtrFontLoader : public FontLoader
{
  public:
    void loadFonts() override;
};

class CtrPlatform : public Platform
{
  public:
    CtrPlatform();
    ~CtrPlatform() override;

    void createWindow(std::string title, uint32_t width, uint32_t height, float windowXPos, float windowYPos) override;
    // The console's model: New 3DS, New 3DS XL or New 2DS XL (the original
    // models run, at a third of the speed, and are not aimed at)
    std::string getName() override { return this->modelName; }
    // A New model (804 MHz, the L2 cache, the extra cores)
    bool isNewModel() const { return this->newModel; }
    const LayoutPreset* getLayoutPreset() override;

    bool mainLoopIteration() override;
    ThemeVariant getThemeVariant() override { return this->themeVariant; }
    void setThemeVariant(ThemeVariant theme) override { this->themeVariant = theme; }
    // The console has no light or dark of its own: the dark it starts in
    void followSystemTheme() override { this->themeVariant = ThemeVariant::DARK; }
    std::string getLocale() override;

    int getBatteryLevel() override;
    bool isBatteryCharging() override;
    bool canShowWirelessLevel() override { return false; }
    int getWirelessLevel() override { return 0; }
    std::string getIpAddress() override { return "-"; }
    std::string getDnsServer() override { return "-"; }
    void disableScreenDimming(bool disable, const std::string& reason, const std::string& app) override { }
    bool isScreenDimmingDisabled() override { return false; }
    void setBacklightBrightness(float brightness) override { }
    float getBacklightBrightness() override { return 1.0f; }
    bool canSetBacklightBrightness() override { return false; }
    bool isApplicationMode() override { return true; }
    void exitToHomeMode(bool value) override { }
    void forceEnableGamePlayRecording() override { }
    void openBrowser(std::string url) override { }

    AudioPlayer* getAudioPlayer() override { return &this->audioPlayer; }
    VideoContext* getVideoContext() override { return this->videoContext; }
    InputManager* getInputManager() override { return &this->inputManager; }
    ImeManager* getImeManager() override { return &this->imeManager; }
    FontLoader* getFontLoader() override { return &this->fontLoader; }

  private:
    std::string modelName     = "3DS";
    bool newModel             = false;
    ThemeVariant themeVariant = ThemeVariant::DARK;
    NullAudioPlayer audioPlayer;
    CtrInputManager inputManager;
    CtrImeManager imeManager;
    CtrFontLoader fontLoader;
    CtrVideoContext* videoContext = nullptr;
};

} // namespace aurore
