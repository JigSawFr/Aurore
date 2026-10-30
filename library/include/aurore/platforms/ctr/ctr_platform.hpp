// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// The New 3DS, through libctru: its system's language and battery, the
// small screen preset (480x360 layout units on the 320x240 screen), and
// citro3d on both screens, its buttons, sticks and touch screen, the
// system's keyboard (swkbd), the Wi-Fi's strength and the IP address (ac,
// soc), the log on 3dslink's terminal or the SD card (docs/platforms.md).

#pragma once

#include <aurore/core/audio.hpp>
#include <aurore/core/font.hpp>
#include <aurore/core/ime.hpp>
#include <aurore/core/input.hpp>
#include <aurore/core/platform.hpp>
#include <aurore/platforms/ctr/ctr_video.hpp>

namespace aurore
{

/// The console's buttons as the Switch's (same layout), the Circle Pad as
/// the left stick, the C-stick as the right one, the bottom screen's touches
class CtrInputManager : public InputManager
{
  public:
    short getControllersConnectedCount() override { return 1; }
    void updateUnifiedControllerState(ControllerState* state) override;
    void updateControllerState(ControllerState* state, int controller) override;
    bool getKeyboardKeyState(AuroreKeyboardScancode key) override { return false; }
    void updateTouchStates(std::vector<RawTouchState>* states) override;
    void updateMouseStates(RawMouseState* state) override { }
    void sendRumble(unsigned short controller, unsigned short lowFreqMotor, unsigned short highFreqMotor) override { }
};

/// The system's keyboard (swkbd): the text one, its password mode, the
/// number pad. It takes both screens until it is closed.
class CtrImeManager : public ImeManager
{
  public:
    bool openForText(std::function<void(std::string)> f, std::string headerText, std::string subText,
        int maxStringLength, std::string initialText, int kbdDisableBitmask) override;
    bool openForPassword(std::function<void(std::string)> f, std::string headerText, std::string subText,
        int maxStringLength, std::string initialText, int kbdDisableBitmask) override;
    bool openForNumber(std::function<void(long)> f, std::string headerText, std::string subText,
        int maxStringLength, std::string initialText, std::string leftButton, std::string rightButton,
        int kbdDisableBitmask) override;
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
    bool canShowWirelessLevel() override { return true; }
    bool hasWirelessConnection() override;
    // 0 to 3 bars
    int getWirelessLevel() override;
    std::string getIpAddress() override;
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
    u32* socBuffer            = nullptr; // the sockets' memory, given to soc:U
    bool socReady             = false;
    ThemeVariant themeVariant = ThemeVariant::DARK;
    NullAudioPlayer audioPlayer;
    CtrInputManager inputManager;
    CtrImeManager imeManager;
    CtrFontLoader fontLoader;
    CtrVideoContext* videoContext = nullptr;
};

} // namespace aurore
