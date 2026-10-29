/*
    Copyright 2021 natinusala
    Copyright 2023 xfangfang

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

#include <functional>
#include <string>
#include <vector>
#include <aurore/core/audio.hpp>
#include <aurore/core/font.hpp>
#include <aurore/core/i18n.hpp>
#include <aurore/core/ime.hpp>
#include <aurore/core/input.hpp>
#include <aurore/core/theme.hpp>
#include <aurore/core/video.hpp>

namespace aurore
{

struct LayoutPreset; // application.hpp

// Interface to provide everything platform specific required to run Aurore: graphics context, inputs, audio...
// The best platform is automatically selected when the application starts, and cannot be changed by the user at the moment
class Platform
{
  public:
    virtual ~Platform() = default;
    ;

    /**
     * Called on startup, right after instanciation, to create and open a window
     * with the given title and size.
     */
    virtual void createWindow(std::string title, uint32_t width, uint32_t height, float windowXPos = NAN, float windowYPos = NAN) = 0;

    /**
     *
     * This function also restores windows from maximization.
     *
     */
    virtual void restoreWindow() {};

    /**
     *
     * Set window size
     *
     */
    virtual void setWindowSize(uint32_t windowWidth, uint32_t windowHeight) {};

    /**
     *
     * Set window size limits
     *
     */
    virtual void setWindowSizeLimits(uint32_t windowMinWidth, uint32_t windowMinHeight, uint32_t windowMaxWidth, uint32_t windowMaxHeight) {};

    /**
     *
     * Set window position
     *
     */
    virtual void setWindowPosition(int windowXPos, int windowYPos) {};

    /**
     *
     * 1.restoreWindow
     * 2.Set window size
     * 3.Set window position
     *
     */
    virtual void setWindowState(uint32_t windowWidth, uint32_t windowHeight, int windowXPos, int windowYPos) {};

    /**
     * Returns the human-readable name of the platform.
     */
    virtual std::string getName() = 0;

    /**
     * The layout this platform's screen asks for (a small console screen:
     * PRESET_SMALL_SCREEN), nullptr for the usual one. Read by
     * Application::init() unless the app chose its own (Application::usePreset).
     */
    virtual const LayoutPreset* getLayoutPreset() { return nullptr; }

    /**
     * Returns true if device has wireless connection.
     */
    virtual bool hasWirelessConnection() { return false; };

    /**
     * Returns wireless quality level from 0 to 3.
     */
    virtual int getWirelessLevel() = 0;

    /**
     * Returns true if device has ethernet connection.
     */
    virtual bool hasEthernetConnection() { return false; };

    /**
     * Returns ip address.
     */
    virtual std::string getIpAddress() = 0;

    /**
     * Returns dns server address.
     */
    virtual std::string getDnsServer() = 0;

    /**
     * Returns if battery level supports.
     */
    virtual bool canShowBatteryLevel() { return true; }

    /**
     * Returns if Wireless level supports.
     */
    virtual bool canShowWirelessLevel() { return true; }

    /**
     * Returns the battery level from 0 to 100.
     */
    virtual int getBatteryLevel() = 0;

    /**
     * Returns the battery level from 0 to 100.
     */
    virtual bool isBatteryCharging() = 0;

    /**
     * Disable screen dimming/saver...
     *
     * @param disable Inhibit or not
     * @param reason If disable is true, pass Inhibited reason to the system.
     * @param app If disable is true, pass app name to the system.
     */
    virtual void disableScreenDimming(bool disable, const std::string& reason = "disable dimming", const std::string& app = "aurore") = 0;

    /**
     * Returns true if screen dimming is disabled
     */
    virtual bool isScreenDimmingDisabled() = 0;

    /**
     * Set the backlight brightness
     *
     * @param brightness 0.0 to 1.0
     */
    virtual void setBacklightBrightness(float brightness) = 0;

    /**
     * Get the backlight brightness
     * @return 0.0 to 1.0
     */
    virtual float getBacklightBrightness() = 0;

    /**
     * Returns true if the platform can set/get the backlight brightness
     */
    virtual bool canSetBacklightBrightness() = 0;

    /**
     * Set the windows always on top
     */
    virtual void setWindowAlwaysOnTop(bool enable) { }

    /**
     * Called at every iteration of the main loop.
     * Must return false if the app should continue running
     * (for example, return false if the X button was pressed on the window).
     */
    virtual bool mainLoopIteration() = 0;

    virtual bool runLoop(const std::function<bool()>& runLoopImpl) { return runLoopImpl(); }

    /**
     * Can be called at anytime to get the current theme variant: the
     * system's until setThemeVariant gives one.
     *
     * Under SDL3 it follows the system's when that changes (AURORE_THEME or
     * a setThemeVariant call stops it); elsewhere it is read once, at start.
     * The result should be cached by the platform code.
     */
    virtual ThemeVariant getThemeVariant() = 0;

    /**
     * Overwrite current theme settings: the app's choice, kept whatever the
     * system's becomes
     */
    virtual void setThemeVariant(ThemeVariant theme) = 0;

    /**
     * Back to the system's variant after setThemeVariant (a "system" choice
     * in the app's settings): the one read at start, or AURORE_THEME's, and
     * under SDL3 the system's as it changes from then on. A platform that
     * reads no system's variant keeps the last one given.
     */
    virtual void followSystemTheme() { }

    /**
     * Can be called at anytime to get the current locale
     *
     * For now, the locale is assumed to stay the same during the whole time
     * the app is running (no locale hot swap)
     *
     * As such, the result should be cached by the platform code.
     * The method should return one of the locale constants
     * defined in the i18n header file.
     */
    virtual std::string getLocale() = 0;

    /**
     * Returns the AudioPlayer for the platform.
     * Cannot return nullptr.
     */
    virtual AudioPlayer* getAudioPlayer() = 0;

    /**
     * Returns the VideoContext for the platform.
     * Cannot return nullptr.
     */
    virtual VideoContext* getVideoContext() = 0;

    /**
     * Returns the InputManager for the platform.
     * Cannot return nullptr.
     */
    virtual InputManager* getInputManager() = 0;

    /**
     * Returns the ImeManager for the platform.
     * Cannot return nullptr.
     */
    virtual ImeManager* getImeManager() = 0;

    /**
     * Returns the FontLoader for the platform.
     * Cannot return nullptr.
     */
    virtual FontLoader* getFontLoader() = 0;

    /**
     * Returns if non applet.
     */
    virtual bool isApplicationMode() = 0;

    /**
     * When the program exits, exit directly to the home menu
     */
    virtual void exitToHomeMode(bool value) = 0;

    /**
     * When the program exits, exit directly to the home menu
     */
    virtual void forceEnableGamePlayRecording() = 0;

    /**
     * Open a link in browser
     * @param url the link which will be opened; one that isBrowserUrl()
     * refuses is not opened
     */
    virtual void openBrowser(std::string url) = 0;

    /**
     * true if openBrowser() may open this url: it starts with one of
     * BROWSER_URL_SCHEMES followed by "://" and has no control character
     */
    static bool isBrowserUrl(const std::string& url);

    /**
     * The schemes openBrowser() accepts, in lower case
     */
    static inline std::vector<std::string> BROWSER_URL_SCHEMES = { "http", "https" };

    /**
     * Paste text to clipboard
     */
    virtual void pasteToClipboard(const std::string& text) {};

    /**
     * Copy text from clipboard
     */
    virtual std::string pasteFromClipboard() { return ""; };

    /**
     * Selects and returns the best platform.
     */
    static Platform* createPlatform();

    /**
     * Has createPlatform() return what `factory` makes instead of the build's
     * own platform (the test harness' headless one). To call before
     * Application::init(); an empty function goes back to the default.
     */
    static void setFactory(std::function<Platform*()> factory) { Platform::factory = std::move(factory); }

    /**
     * App locale, empty for default
     */
    // From the text itself, not LOCALE_AUTO: that is another const string in
    // each file (internal linkage), not yet built when this inline one is.
    static inline std::string APP_LOCALE_DEFAULT = "auto";

  private:
    static inline std::function<Platform*()> factory;
};

} // namespace aurore
