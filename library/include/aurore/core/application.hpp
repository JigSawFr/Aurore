// SPDX-FileCopyrightText: 2019-2021 natinusala
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <aurore/core/activity.hpp>
#include <aurore/core/audio.hpp>
#include <aurore/core/font.hpp>
#include <aurore/core/frame_context.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/core/notification_manager.hpp>
#include <aurore/core/observable.hpp>
#include <aurore/core/platform.hpp>
#include <aurore/core/screen_frame.hpp>
#include <aurore/core/style.hpp>
#include <aurore/core/theme.hpp>
#include <aurore/core/view.hpp>
#include <aurore/views/label.hpp>

#ifdef __WINRT__
#ifdef main
#undef main
#endif
#define main SDL_main
#endif

namespace aurore
{

/// Input types for entire app
enum class InputType
{
    GAMEPAD, ///< Gamepad or keyboard
    TOUCH, ///< Touch screen
};

class DebugLayer;
class EditTextDialog;
struct ThemeImage;

/**
 * How wide the screen is, in layout units (Application::contentWidth): what a
 * layout picks its variant from (XML `compact:`, `medium:`, `expanded:`).
 */
enum class WidthClass
{
    COMPACT, ///< under WIDTH_MEDIUM: a phone, a small console screen (one column)
    MEDIUM, ///< under WIDTH_EXPANDED
    EXPANDED, ///< the 1280-unit design width and more
};

inline constexpr float WIDTH_MEDIUM   = 720.0f; ///< The narrowest MEDIUM width
inline constexpr float WIDTH_EXPANDED = 1080.0f; ///< The narrowest EXPANDED width

/// The class of a width in layout units.
WidthClass widthClassOf(float width);

/**
 * A screen's layout in one go: the design size its screens are laid out on,
 * the density of their lengths and the scale of their text
 * (Application::usePreset).
 */
struct LayoutPreset
{
    uint32_t designWidth; ///< ORIGINAL_WINDOW_WIDTH
    uint32_t designHeight; ///< ORIGINAL_WINDOW_HEIGHT
    float density; ///< aurore::setDensity
    float textScale; ///< Application::setTextScale, the text size settings' base
};

/// The usual one: 1280x720 at density 1, the text as written.
inline constexpr LayoutPreset PRESET_STANDARD = { 1280, 720, DENSITY_COMFORTABLE, 1.0f };
/// A small screen, 320x240 pixels for instance: 480x360 units (a compact
/// width), dense, the text 1.25 times larger (12 to 17 pixels).
inline constexpr LayoutPreset PRESET_SMALL_SCREEN = { 480, 360, DENSITY_DENSE, 1.25f };

/// Makes a new view for an XML tag (Application::registerXMLView).
typedef std::function<View*(void)> XMLViewCreator;

/// The app: its window, its stack of activities, the focus, the inputs and the main loop.
/// Every member is static: there is one app per process, run from the main thread.
class Application
{
  public:
    friend class EditTextDialog;

    static inline uint32_t ORIGINAL_WINDOW_WIDTH  = 1280; ///< The design width, in layout units
    static inline uint32_t ORIGINAL_WINDOW_HEIGHT = 720; ///< The design height
    inline static std::string appId               = ""; ///< See setAppId().

    /**
     * Inits the Aurore application.
     * Returns true if it succeeded, false otherwise.
     */
    static bool init();

    /**
     * Sets the application ID reported to the windowing system
     * (Wayland app_id / X11 WM_CLASS). Desktop environments use it to
     * associate the window with its .desktop entry, which is required
     * for showing the correct icon in the taskbar.
     * Must be called before createWindow().
     */
    static void setAppId(std::string appId) { Application::appId = std::move(appId); }
    /// The application ID given to setAppId(), "" by default.
    static const std::string& getAppId() { return Application::appId; }

    /**
     * Creates the application window with the given title.
     * Must be called after calling init().
     */
    static void createWindow(std::string title);

    /**
     * Application main loop iteration.
     * Must be called in an infinite loop until it returns false.
     */
    static bool mainLoop();

    /// The platform the app runs on (window, inputs, video, audio, fonts), made by init().
    static Platform* getPlatform();
    /// The platform's sound player.
    static AudioPlayer* getAudioPlayer();

    /**
     * The Yoga configuration every view's node is created with
     * (web defaults, yoga's messages in the log).
     */
    static YGConfig* getYogaConfig();

    /// The nanovg context every view draws with.
    static NVGcontext* getNVGContext();
    /// The size of the UI in layout units: ORIGINAL_WINDOW_WIDTH wide, as high as the window's ratio gives.
    inline static float contentWidth, contentHeight;
    inline static float presetTextScale = 1.0f;
    inline static bool presetChosen     = false; ///< usePreset() called by the app

    /// The app's screen's size in pixels: the window's, or the screen's in
    /// it when there is a ScreenFrame.
    inline static unsigned windowWidth, windowHeight;
    /// The window's whole size in pixels (what is drawn and read back).
    inline static unsigned framebufferWidth, framebufferHeight;

    /**
     * A frame drawn around the app's screen (the demo's simulated consoles):
     * the app laid out and drawn in its screen, the frame over the rest of
     * the window. Kept by the application (deleted when replaced); nullptr
     * for none, the window all screen.
     */
    static void setScreenFrame(ScreenFrame* frame);
    static ScreenFrame* getScreenFrame();
    /// The screen's rectangle in the window, in pixels.
    static Rect getScreenRect();

    /// The window's position on the screen (desktop).
    inline static int windowXPos, windowYPos;

    /**
     * Called by the video context when the content window is resized
     */
    static void onWindowResized(int width, int height);

    /**
     * Do not call this function, use it internally.
     * Called when the video context is ready (to setup the initial content scaling).
     */
    static void setWindowSize(int width, int height);

    /**
     * Called by the video context when the content window is Repositioned
     */
    static void onWindowReposition(int xPos, int yPos);

    /**
     * Do not call this function, use it internally.
     * Called when the video context is ready (to setup the initial window position).
     */
    static void setWindowPosition(int xPos, int yPos);

    /// A copy of the stack of activities, the first pushed first.
    static std::vector<Activity*> getActivitiesStack();

    /**
     * Pushes a view on this applications's view stack.
     *
     * The view will automatically be resized to take
     * the whole screen.
     *
     * The view will gain focus if applicable.
     *
     * The first activity to be pushed cannot be popped.
     */
    static void pushActivity(Activity* view, TransitionAnimation animation = TransitionAnimation::FADE);

    /**
     * Pushes the activity in place of the one on top, in one transition:
     * the one replaced is not shown again on the way, nor given the focus
     * back later, and is deleted once the new one is shown (a wizard's next
     * step, a sign-in screen giving way to the home screen). With an empty
     * stack it is a push.
     */
    static void replaceActivity(Activity* activity, TransitionAnimation animation = TransitionAnimation::FADE);

    /**
     * Pops the last pushed activity from the stack
     * and gives focus back where it was before.
     *
     * return false if no actifity to pop.
     */
    static bool popActivity(
        TransitionAnimation animation = TransitionAnimation::FADE, std::function<void(void)> cb = [] {}, bool free = true);

    /**
     * Gives the focus to the given view
     * or clears the focus if given nullptr.
     */
    static void giveFocus(View* view);

    /**
     * Drops every reference the focus system holds to a view
     * that is being destroyed.
     */
    static void forgetView(View* view);

    /**
     * When on, the focus highlight fades out after a moment without any
     * button pressed (0.8 s, then 0.4 s), and comes back at the next one:
     * for a screen watched more than browsed (a player, a slideshow). Off
     * by default.
     */
    static void setHighlightIdleFade(bool fade);

    /**
     * Called with the focused view's description (View::
     * getAccessibilityDescription: "Save, button") each time the focus moves:
     * where a screen reader, a text-to-speech engine or a log plugs in.
     * Nothing by default; an empty function removes it.
     */
    static void setFocusAnnouncer(std::function<void(const std::string&)> announcer);

    /**
     * Gives `text` to the focus announcer, outside of a focus move: a value
     * changed in place (View::announceAccessibilityValue), a toast (notify
     * says its text). Nothing for an empty text. getLastAnnouncement() is
     * the last text given, announcer or not (for tests).
     */
    static void announce(const std::string& text);
    static const std::string& getLastAnnouncement() { return Application::lastAnnouncement; }

    /**
     * Reduced motion: the movements that are only there for show are skipped
     * (the transition= of views, the highlight's glide, the scrolling of
     * lists and of texts cut short, the toasts' and dropdowns' slide, the
     * switch cells' bounce; Cadran's entrances, glides and breathing glow).
     * Off by default. getReduceMotion(), which
     * transitions read, is also true while Cadran's effects leave movement
     * out (Preferences › Visual effects "reduced", the lighter effects in
     * hand, AURORE_STILL_FOCUS); the highlight's glide follows the app's
     * setting only.
     */
    static void setReduceMotion(bool reduce);
    static bool getReduceMotion();
    /// The app's own setting alone, without Cadran's.
    static bool isReduceMotionSet() { return Application::reduceMotion; }

    /**
     * When the last input came (a button, a key, a touch, the mouse), on the
     * clock of getCPUTimeUsec(): what the theme's particles stop after.
     */
    static Time getLastInputTime() { return Application::lastInputTime; }

    /**
     * Every Label drawn `scale` times its size (Label::setFontScale), and the
     * screens laid out again for it at once (every view told through
     * View::onTextScaleChanged). 1 by default; Cadran's text size setting
     * goes through here.
     */
    static void setTextScale(float scale);
    static float getTextScale();

    /**
     * The density of the lengths (aurore::setDensity): DENSITY_COMFORTABLE
     * by default, DENSITY_COMPACT, DENSITY_DENSE for a small screen. The text
     * keeps its size (setTextScale). Read when the views are made: set it
     * before the first screen; AURORE_DENSITY gives it on the desktop.
     */
    static void setDensity(float density) { aurore::setDensity(density); }
    static float getDensity() { return aurore::getDensity(); }

    /**
     * The class of the screen's width now (widthClassOf(contentWidth)). XML
     * reads it when a view is made; a view that lays itself out in code
     * reads it there too, and the window size changed event says when it may
     * have changed. ORIGINAL_WINDOW_WIDTH sets it for a small screen (480
     * on a 320-pixel one); AURORE_DESIGN_SIZE=480x360 tries it on the desktop.
     */
    static WidthClass getWidthClass();

    /**
     * Lays every screen out for `preset` (PRESET_SMALL_SCREEN on a small
     * screen): its design size, density and text scale. Before init(); a
     * platform may give one (Platform::getLayoutPreset), the app's wins, and
     * AURORE_PRESET (small or standard) wins over both on the desktop.
     */
    static void usePreset(const LayoutPreset& preset);
    /// The text scale of the preset in use (1 without one): what Cadran's text
    /// size setting multiplies.
    static float getPresetTextScale() { return Application::presetTextScale; }

    /**
     * The data models XML bindings read (View::bind, bind="text: name.key"),
     * by name, for every screen. An Activity's own (Activity::registerModel)
     * come first. The model must outlive its registration.
     */
    static void registerModel(const std::string& name, DataModel* model);
    static void unregisterModel(const std::string& name);
    static DataModel* getModel(const std::string& name);

    /**
     * The view under the mouse pointer that has a hover: style (desktop), or
     * nullptr.
     */
    static View* getHoveredView() { return Application::hoveredView; }

    /// The metrics in use (aurore::getStyle()).
    inline static Style getStyle()
    {
        return aurore::getStyle();
    }

    /// The colours of the variant in use (light or dark), with the theme file's over them.
    static Theme getTheme();
    /// The variant in use, as the platform reports it.
    static ThemeVariant getThemeVariant();

    /**
     * Fired when the theme changed: another variant
     * (Platform::setThemeVariant), a theme file (Theme::loadFile,
     * Theme::reset) or a colour changed (Theme::addColor). Checked once a
     * frame, before it is drawn; by then every view of the screens took the
     * change (View::onThemeChanged). For what keeps colours of its own.
     */
    static Event<ThemeVariant>* getThemeChangedEvent();

    /**
     * Whether the theme in use draws a picture behind the screens in the
     * variant in use (its background_image, loaded).
     */
    static bool hasThemeBackground();

    /**
     * Draws what is behind the screens over `x, y, width, height`: the
     * clear colour, the theme's background image and its veil. For a panel
     * drawn over other views in a translucent surface colour (a drawer over
     * a page), so that it hides them and still shows the picture. Nothing
     * without a background image.
     */
    static void drawThemeBackground(NVGcontext* vg, float x, float y, float width, float height);

    /**
     * The same in a shape with round corners of `radius`, and the strips
     * of the theme's decorations (decor.top, decor.bottom) over it: under a
     * translucent dialog or sheet (View::setThemeArea).
     */
    static void drawThemeBackground(NVGcontext* vg, float x, float y, float width, float height, float radius);

    /// The platform's text input (the keyboard shown for an EditTextDialog).
    static ImeManager* getImeManager();

    /**
     * Loads a font from a given file and stores it in the font stash.
     * A AURORE_ASSET() path ("@res/...") is read from the resources built
     * into the program when they are (USE_LIBROMFS).
     * Returns true if the operation succeeded.
     */
    static bool loadFontFromFile(std::string fontName, std::string filePath);

    /**
     * Loads a font from a given memory buffer and stores it in the font stash.
     * Returns true if the operation succeeded.
     */
    static bool loadFontFromMemory(std::string fontName, void* data, size_t size, bool freeData);

    /**
     * The same, with the face at `faceIndex` of a font collection (.ttc).
     */
    static bool loadFontFromMemory(std::string fontName, void* data, size_t size, bool freeData, int faceIndex);

    /**
     * Returns the nanovg handle to the given font name, or FONT_INVALID if
     * no such font is currently loaded.
     */
    static int getFont(std::string fontName);

    /**
     * Glyphs missing from the font loaded as `font` are looked up in the one
     * loaded as `fallback`. False when either is not loaded.
     */
    static bool addFontFallback(const std::string& font, const std::string& fallback);

    /// The font a new Label draws with: the regular font (the Chinese one on the Switch).
    static int getDefaultFont();

    /// Shows a toast with `text`, and `leading` (an icon) before it when given; the
    /// focus announcer says the text.
    static void notify(const std::string& text, View* leading = nullptr);

    /// Handles a controller button as the main loop does: the focused view's actions first,
    /// then the focus moves. For the platforms and the test harness.
    static void onControllerButtonPressed(enum ControllerButton button, bool repeating);

    /// Handles a key as the main loop does (the actions registered on keys).
    static void onKeyboardPressed(AuroreKeyCombination key, bool repeating);

    /**
     * "Crashes" the app (displays a fullscreen CrashFrame)
     */
    /**
     * Shows `text` in a dialog that cannot be dismissed, with one button that
     * quits the app: for an error the app cannot go on after. Can be called
     * from any thread.
     */
    static void crash(std::string text);

    /// Asks the main loop to stop: the next mainLoop() returns false, after the exit events.
    static void quit();
    /// Deletes every activity on the stack and clears the focus.
    static void clear();

    /**
     * Blocks any and all user inputs
     */
    static void blockInputs(bool muteSounds = false);

    /**
     * Unblocks inputs after a call to
     * blockInputs()
     */
    static void unblockInputs();

    /// Whether blockInputs() is in force.
    static bool isInputBlocks();

    /**
     * True when the screen has stopped moving: no animation running nor
     * Timer waiting (a RepeatingTimer does not count: a clock's refresh runs
     * as long as its screen is open), no task queued or running on the
     * threads (Threading::isIdle()), inputs not blocked (a screen opening or
     * closing), and no redraw asked for (a view fading on the clock). What a
     * test waits for before it presses the next button or takes a screenshot.
     * With AURORE_DEV_TOOLS, AURORE_SETTLE_FILE (a file's path) has it
     * written there on every loop, for a test driving the app from outside
     * (docs/testing.md).
     */
    static bool isSettled();

    /// The controller's buttons and sticks as read this frame.
    static const ControllerState& getControllerState();

    /// A text the app keeps for its footers (the library does not draw it).
    static void setCommonFooter(std::string footer);
    static std::string* getCommonFooter();

    /// Pixels per layout unit (windowWidth / ORIGINAL_WINDOW_WIDTH, capped by setMaximumUIScale()).
    inline static float windowScale;
    inline static float maximumUIScale = 0.0f; ///< See setMaximumUIScale().

    /**
     * Caps how large the UI is allowed to grow relative to the design size.
     * Set to 0 to disable the cap.
     */
    static void setMaximumUIScale(float scale);
    static float getMaximumUIScale();

    /**
     * The safe area, in layout units (docs/safe-areas.md): how far from each
     * edge of the window content keeps clear of the display cutouts, rounded
     * corners and system bars the platform reports (Platform::getSafeAreaInsets),
     * or of the overscan margin, whichever is larger. Zero on desktops and
     * consoles, unless set with setSafeAreaOverride() or setOverscan().
     */
    static Insets getSafeArea();

    /**
     * Reads the platform's safe area again; when it changed, the activities'
     * views take it (Box::setSafeArea) and getSafeAreaChangedEvent() fires.
     * The platforms call it when it may have changed (a rotation); a resize
     * reads it too.
     */
    static void updateSafeArea();

    /// Fired with the new safe area (layout units) when it changed.
    static Event<Insets>* getSafeAreaChangedEvent();

    /// True once a safe area other than zero was set: until then the views
    /// do no work for it.
    static bool isSafeAreaInUse() { return Application::safeAreaInUse; }

    /**
     * In place of what the platform reports, these insets, in the unit of the
     * window's size: a notch or a TV seen on a desktop, a test. nullptr goes
     * back to the platform's. In a build with AURORE_DEV_TOOLS, the
     * environment variable AURORE_SAFE_AREA sets it at init ("t,r,b,l", as a
     * CSS padding).
     */
    static void setSafeAreaOverride(const Insets* insets);

    /**
     * A TV's overscan: a margin at each edge, as a fraction of the window's
     * size (0.05: 5 %, a TV's title safe area; 0 by default, at most 0.25),
     * that the safe area is never smaller than. For a setting of the app on
     * consoles and TV boxes plugged into a set that crops the picture.
     */
    static void setOverscan(float fraction);
    static float getOverscan();

    /**
     * Sets whether BUTTON_START will globally be used to close the application.
     */
    static void setGlobalQuit(bool enabled);

    /// Whether getFPS() is counted (60 when off).
    static void setFPSStatus(bool enabled);
    static bool getFPSStatus();
    /// The frames per second over the last second.
    static size_t getFPS();

    /**
     * Set the FPS limit
     * @param fps 0 to disable limit
     */
    static void setLimitedFPS(size_t fps);

    /**
     * Set the swap interval
     * Must be called after createWindow. On some platforms, it is impossible to set it back
     * to 0 after setting it to a non-zero values, so you can use
     * `VideoContext::swapInterval = 0;` (`#include "aurore/core/video.hpp"`) before creating the window.
     *
     * Test results:
     *
     * ## Platforms that always vsync (interval: 1):
     * iOS18/GLES3/SDL
     * PS4/GLES2/SDL
     *
     * ## Platforms that only support enable and disable vsync (interval: 0,1):
     * Android9/GLES2/SDL
     * macOS13.0+/GL3.2/GLFW (will fixed to 120fps with vsync enabled, https://github.com/glfw/glfw/pull/2277)
     * Windows11/GL3.2/GLFW
     * Linux Wayland/GL3.2/GLFW
     *
     * ## Platforms that not support disable vsync (interval: 1,2,3,4):
     * Windows11/D3D11/GLFW
     * Switch/DEKO3D
     *
     * ## Platforms that support arbitrary values (interval: 0,1,2,3,4):
     * PsVita/GXM
     * PsVita/GLES2/SDL
     * macOS13.0+/GL3.2/SDL
     * Linux X11/GL3.2/GLFW
     * Linux X11/GL3.2/SDL
     * Linux Wayland/GL3.2/SDL
     * Switch/GL4.3/GLFW
     * Switch/GL4.3/SDL
     *
     * @param interval 0 to disable vsync
     */
    static void setSwapInterval(int interval);

    /**
     * If the value is set to true, the program will limit FPS to Application::DeactivatedFPS
     * after Application::DeactivatedTime milliseconds of inactivity.
     *
     * default is false;
     */
    static void setAutomaticDeactivation(bool value);
    /// See setAutomaticDeactivation().
    static bool getAutomaticDeactivation();
    /// Whether the app is active: an input lately, an animation under way, or no automatic
    /// deactivation. Always true on the Switch.
    static bool hasActiveEvent();
    /// Marks the app active now (and asks for a redraw), or no longer.
    static void setActiveEvent(bool value);
    /// After this long without an input, the app is drawn at the deactivated FPS.
    static void setDeactivatedTime(int millisecond);
    /// The frame rate once deactivated (5 by default).
    static void setDeactivatedFPS(int value);
    static int getDeactivatedFPS();
    /// A frame's time once deactivated, in seconds.
    static double getDeactivatedFrameTime();

    /**
     * Redraw on demand: a frame is drawn only when something may have changed
     * on screen (an input, a running animation, a sync or delayed task that
     * ran, a view's layout invalidated, a window resized, or requestRedraw()),
     * and at least once a second. On the other frames the inputs are still
     * read, but nothing is drawn or presented, which saves most of the CPU
     * and GPU time of an idle screen.
     *
     * A view that changes how it looks without any of these (a colour set
     * from a timer of its own...) calls requestRedraw().
     *
     * default is false: every frame is drawn.
     */
    static void setRedrawOnDemand(bool enabled);
    /// See setRedrawOnDemand().
    static bool isRedrawOnDemand();

    /**
     * Asks for the next frames to be drawn, with redraw on demand.
     */
    static void requestRedraw();

    /**
     * Number of frames drawn since the start (frames skipped by redraw on
     * demand are not counted).
     */
    static uint64_t getDrawnFrameCount();

    /**
     * Debugging redraw on demand: each frame it skips is drawn all the same,
     * without being presented, and compared with the last frame presented.
     * A difference is something that changed on screen without asking for a
     * redraw (a missing invalidate() or requestRedraw()): it is logged as a
     * warning, with the area and the views drawn there (once per area).
     * Reads every frame back: slow, and OpenGL drivers only. Off by default;
     * AURORE_REDRAW_DEBUG=1 in the environment turns it on (desktop).
     */
    static void setRedrawDebug(bool enabled);
    /// See setRedrawDebug().
    static bool isRedrawDebug();

    /**
     * With setRedrawDebug(): the number of skipped frames found different
     * from the last frame presented, and the last such area, in the views'
     * coordinates.
     */
    static uint64_t getMissedRedrawCount();
    static Rect getLastMissedRedrawArea();

    /// Fired with the new focus each time the focus moves.
    static GenericEvent* getGlobalFocusChangeEvent();
    /// Fired when the footer's hints must be read again (the focus moved, keys swapped).
    static VoidEvent* getGlobalHintsUpdateEvent();
    /// Fired when the input type changes (a touch after the buttons, or the reverse).
    static Event<InputType>* getGlobalInputTypeChangeEvent();
    /// Fired once per main loop iteration, after the frame and the sync tasks.
    static VoidEvent* getRunLoopEvent();
    /// Fired after each frame presented.
    static VoidEvent* getPostFrameEvent();
    /// Fired when the app starts to exit, before the activities are deleted.
    static VoidEvent* getExitEvent();
    /// Fired once the activities and the views left are deleted.
    static VoidEvent* getExitDoneEvent();
    /// Fired when the window's size changed.
    static VoidEvent* getWindowSizeChangedEvent();
    /// Fired at the end of createWindow().
    static VoidEvent* getWindowCreationDoneEvent();
    /// Fired when the window is closed or quit() was asked, before the exit.
    static VoidEvent* getWindowShouldCloseEvent();
    /// Fired with true or false when the window gains or loses the system's focus.
    static Event<bool>* getWindowFocusChangedEvent();

    /// The view that has the focus, or nullptr.
    static View* getCurrentFocus();

    /// The window's title, as given to createWindow().
    static std::string getTitle();

    /**
     * Registers a view to be created from XML. You must give the name of the XML node as well
     * as a function that creates the view.
     *
     * If you need attributes, register them with the given functions in the view
     * class constructor directly. They will be called one by one after the view is instantiated.
     *
     * You should not add any children in the function, it is already taken care of.
     */
    static void registerXMLView(std::string name, XMLViewCreator creator);

    /// Whether an XML tag is registered (registerXMLView()).
    static bool XMLViewsRegisterContains(std::string name);

    /// The XML tags registered (registerXMLView).
    static std::vector<std::string> getXMLViewNames();
    /// The function registered for an XML tag.
    static XMLViewCreator getXMLViewCreator(std::string name);

    /**
     * Returns the current system locale.
     */
    static std::string getLocale();

    /// Deletes `view` at the end of the current main loop iteration (once no ptrLock()
    /// holds it). View::freeView() calls it.
    static void addToFreeQueue(View* view);

    /**
     * Returns the current input type.
     */
    inline static InputType getInputType()
    {
        return inputType;
    }

    /**
     * The built-in inspector (docs/inspector.md): F12 on a keyboard, or
     * both sticks pressed in together (L3 + R3) on a controller, opens it
     * over the screen with the view tree, the focus chain, the yoga box and
     * the theme values of a view, and the frame's timings. Off by default;
     * the AURORE_INSPECTOR CMake option, or AURORE_INSPECTOR=1 in the
     * environment on a desktop, turns it on at init(). Closed, it only
     * watches for its toggle.
     */
    static void setInspectorEnabled(bool enabled);
    /// See setInspectorEnabled().
    static bool isInspectorEnabled();

    /**
     * Hot reload, on a desktop (docs/hot_reload.md): the XML files the
     * activity on top was built from, and the files read by
     * Theme::loadFile(), are watched, and the activity is built again when
     * one of them changes (the focus is put back where it was when it can
     * be). `resourcesDir`, when given, is a resources folder (the app's
     * sources' `resources/`) the XML files are read from in place of the
     * ones built in (libromfs) or copied next to the app. Off by default;
     * `AURORE_HOT_RELOAD=1`, or `AURORE_HOT_RELOAD=<resources folder>`, in the
     * environment turns it on at init(). Does nothing on the consoles.
     */
    static void setHotReload(bool enabled, const std::string& resourcesDir = "");
    /// See setHotReload().
    static bool isHotReload();

    /// Builds the content of the activity on top again from its
    /// createContentView(), as hot reload does. False when it cannot (an
    /// activity given its view in its constructor) or the new content
    /// throws while being made (its XML, onContentAvailable(), or a tab
    /// shown, made when it takes the focus): the old one then stays, with
    /// the focus, and the error is shown in a notification.
    static bool rebuildActivity();

    /// Draws the debug layer (DebugLayer) over the app.
    inline static void enableDebuggingView(bool enable)
    {
        debuggingViewEnabled = enable;
    }

    inline static bool isDebuggingViewEnabled()
    {
        return debuggingViewEnabled;
    }

    /// Swaps A and B (and X and Y) for the app's actions, as on a Japanese console.
    static void setSwapInputKeys(bool swap);

    /// The keyboard keys that press a controller button, on top of Enter (A),
    /// Escape (B) and the arrows. By default X, Y, L, R, Shift+L (ZL),
    /// Shift+R (ZR), P or + (Plus), M or - (Minus) and Backspace (B); keys
    /// are named after their place on a US keyboard (docs/input.md).
    /// A key a view registers an action for keeps that action only, and
    /// none applies while a text is being typed.
    /// @{
    static void mapKey(AuroreKeyCombination key, ControllerButton button);
    static void unmapKey(AuroreKeyCombination key);
    static void clearKeyMap();
    static const std::vector<std::pair<AuroreKeyCombination, ControllerButton>>& getKeyMap();
    /// @}

    inline static bool isSwapInputKeys()
    {
        return swapInputKeys;
    }

    /// Draws the mouse pointer with nanovg (for platforms with no system pointer).
    inline static void setDrawCoursor(bool draw)
    {
        drawCoursor = draw;
    }

    inline static bool isDrawCursor()
    {
        return drawCoursor;
    }

    /// See setHintsLiteMode().
    inline static bool isHintsLiteMode()
    {
        return hintsLiteMode;
    }

    /// Set hints lite mode, which will only show hints for A button and B button
    inline void static setHintsLiteMode(const bool value)
    {
        hintsLiteMode = value;
    }

    /// Stops the touch gestures `view` is tracking, before it is removed.
    static void tryDeinitFirstResponder(View* view);

    /// Reads `key` each frame from now on (counted: one removeWatchedKeys() per add).
    static void addToWatchedKeys(const AuroreKeyCombination key);

    /// See addToWatchedKeys().
    static void removeWatchedKeys(const AuroreKeyCombination key);

  private:
    static void updateHover(Point position, bool inside);
    static void announceFocus(View* oldFocus, View* newFocus);

    inline static std::function<void(const std::string&)> focusAnnouncer;
    inline static std::string lastAnnouncement;
    inline static bool reduceMotion = false;
    inline static std::unordered_map<std::string, DataModel*> models;
    inline static View* hoveredView = nullptr;

    inline static bool inited               = false;
    inline static bool quitRequested        = false;
    inline static bool debuggingViewEnabled = false;
    inline static bool swapInputKeys        = false;
    inline static bool drawCoursor          = false;

    inline static Platform* platform = nullptr;

    inline static std::string title;

    inline static FontStash fontStash;

    inline static std::vector<Activity*> activitiesStack;
    inline static std::vector<View*> focusStack;
    inline static std::deque<View*> deletionPool;

    inline static View* currentFocus = nullptr;
    inline static std::vector<TouchState> currentTouchState;
    inline static MouseState currentMouseState;
    inline static NotificationManager* notificationManager;

    /// Return true if input type was changed
    static bool setInputType(InputType type);

    inline static InputType inputType = InputType::GAMEPAD;
    inline static std::vector<AuroreKeyState> watchedKeys;
    inline static std::vector<AuroreKeyState> oldWatchedKeys;
    inline static std::unordered_map<int, int> watchedKeysMap;
    static std::vector<std::pair<AuroreKeyCombination, ControllerButton>> keyMap;
    static void applyKeyMap(InputManager* inputManager, ControllerState* state);
    inline static ControllerState controllerState = {};

    inline static void processInput();
    inline static bool internalMainLoop();

    inline static void updateFPS();

    inline static unsigned blockInputsTokens = 0; ///< any value > 0 means inputs are blocked
    inline static bool muteSounds            = false;

    inline static std::string commonFooter;

    inline static bool globalQuitEnabled                = false;
    inline static ActionIdentifier gloablQuitIdentifier = ACTION_NONE;
    inline static bool globalFPSToggleEnabled           = false;
    inline static size_t globalFPS                      = 60;
    inline static Time limitedFrameTime                 = 0;
    inline static Time frameStartTime                   = 0;
    inline static bool hintsLiteMode                    = false;

    inline static bool deactivatedBehavior = false;
    inline static bool activeEvent         = false;
    inline static Time lastActiveTime      = 0;
    inline static Time lastInputTime       = 0;
    inline static bool highlightIdleFade   = false;
    inline static bool redrawOnDemand      = false;
    inline static int redrawFrames         = 2; ///< frames still to draw
    inline static Time lastDrawTime        = 0;
    inline static uint64_t drawnFrames     = 0;
    inline static bool redrawDebug         = false;
    inline static uint64_t missedRedraws   = 0;
    static Rect missedRedrawArea;
    inline static std::unique_ptr<ScreenFrame> screenFrame;
    inline static Rect screenRect;
    inline static std::vector<unsigned char> presentedPixels;
    inline static int presentedWidth = 0, presentedHeight = 0;
    inline static int deactivatedFPS  = 5; ///< FPS 5
    inline static int deactivatedTime = 5000000; ///< 5s

    inline static View* repetitionOldFocus = nullptr;

    inline static GenericEvent globalFocusChangeEvent;
    inline static VoidEvent globalHintsUpdateEvent;
    inline static Event<InputType> globalInputTypeChangeEvent;
    inline static VoidEvent runLoopEvent;
    inline static VoidEvent postFrameEvent;
    inline static VoidEvent exitEvent;
    inline static VoidEvent exitDoneEvent;
    inline static VoidEvent windowSizeChangedEvent;
    inline static Event<Insets> safeAreaChangedEvent;
    inline static Insets safeArea;
    inline static Insets safeAreaOverride;
    inline static bool hasSafeAreaOverride = false;
    inline static float overscan           = 0.0f;
    // A safe area other than zero was ever set: the views take it as they
    // are added (none of that work until then)
    inline static bool safeAreaInUse = false;
    // The safe area from the platform's, the override and the overscan, in
    // layout units: true when it changed
    static bool computeSafeArea();
    inline static VoidEvent windowCreationDoneEvent;
    inline static VoidEvent windowShouldCloseEvent;
    inline static Event<bool> windowFocusChangedEvent;
    inline static Event<ThemeVariant> themeChangedEvent;

    /// The theme the screens were last told about
    inline static uint64_t themeGeneration  = 0;
    inline static ThemeVariant themeVariant = ThemeVariant::DARK;
    static void checkThemeChanged();

    /// The theme file's background in the variant in use: the image drawn,
    /// and the one replacing it once loaded (the one drawn stays meanwhile)
    inline static ThemeBackground themeBackgroundSpec;
    inline static std::shared_ptr<ThemeImage> themeBackground;
    inline static std::shared_ptr<ThemeImage> themeBackgroundLoading;
    static void loadThemeBackground();
    static void onThemeImageLoaded(const ThemeImage* image);
    static void drawThemeBackgroundImage(NVGcontext* vg, float x, float y, float width, float height, float radius = 0.0f);

    inline static std::unordered_map<std::string, XMLViewCreator> xmlViewsRegister;

    static void navigate(FocusDirection direction, bool repeating);

    /// present: false draws the frame without presenting it (redraw debug)
    static void frame(bool present = true);
    static void checkSkippedFrame();
    static void exit();

    /**
     * Handles actions for the currently focused view and
     * the given button
     * Returns true if at least one action has been fired
     */
    static bool handleAction(ActionType type, int button, bool repeating);

    static void registerBuiltInXMLViews();

    inline static DebugLayer* debugLayer = nullptr;
};

} // namespace aurore
