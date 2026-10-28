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

// brls::test::Harness runs an app's screens with no window: inputs given
// through calls, a simulated clock (frames advance without waiting, the
// same way every run), screenshots compared with references. Built with
// -DBRLS_BUILD_TESTS=ON (target Aurore::test); docs/testing.md has a guide.
//
//     brls::test::Harness h;                  // Application::init + window
//     h.push(new MainActivity());
//     h.settle();                             // animations done
//     h.tap(brls::BUTTON_DOWN);
//     BRLS_CHECK(h.focusText() == "Settings");
//     BRLS_CHECK(h.matchesReference("main_settings"));
//
// One Harness per process: borealis' Application is global.

#include <borealis/core/activity.hpp>
#include <borealis/core/geometry.hpp>
#include <borealis/core/input.hpp>
#include <borealis/core/safe_area.hpp>
#include <borealis/core/theme.hpp>
#include <borealis/core/time.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

namespace brls
{
class HeadlessPlatform;
class View;
}

namespace brls::test
{

struct Options
{
    // The framebuffer's size; the UI scales as on a window this size
    int width  = 1280;
    int height = 720;
    // Simulated time per frame
    double frameMs = 1000.0 / 60.0;
    // The platform's locale and theme (Cadran's preferences, when they say
    // "system", follow them)
    std::string locale = "en-US";
    ThemeVariant theme = ThemeVariant::DARK;
    // The wall clock at the start, in seconds since the epoch (the dates and
    // times shown; it moves on with the simulated clock), in UTC: Monday
    // 5 January 2026, 10:30. Negative: the system's clock and time zone.
    int64_t wallTime = 1767609000;
    // Window title (translated by the app if it wants)
    std::string title = "test";
    // Where matchesReference() reads references and writes what it got.
    // Empty: BRLS_TEST_REFERENCES / BRLS_TEST_OUTPUT from the environment,
    // else "references" and "test-output" in the working directory.
    std::string referenceDir;
    std::string outputDir;
    // The system's fonts (Chinese, Japanese and Korean, colour emoji) looked
    // up: off, the pictures are the same on every machine, with the app's
    // fonts only
    bool systemFonts = false;
};

// An RGBA picture, rows top-down
struct Image
{
    int width  = 0;
    int height = 0;
    std::vector<unsigned char> rgba;

    bool empty() const { return rgba.empty(); }
    // The pixel's channels, at x, y
    const unsigned char* at(int x, int y) const { return rgba.data() + ((size_t)y * width + x) * 4; }
    // The part of the picture in `x, y, w, h` (clipped to it)
    Image crop(int x, int y, int w, int h) const;

    bool savePng(const std::string& path) const;
    static Image loadPng(const std::string& path);
};

struct ImageDiff
{
    bool sameSize          = false;
    size_t differingPixels = 0; // with a channel off by more than the tolerance
    int maxDelta           = 0; // the largest channel difference
    double ratio           = 0; // differingPixels / all pixels
    // The pixels found different in red over a faded copy of `expected`
    Image map;
};

// Pixel by pixel: a channel off by more than `tolerance` (0-255) makes the
// pixel differ
ImageDiff compareImages(const Image& actual, const Image& expected, int tolerance = 0);

class Harness
{
  public:
    explicit Harness(Options options = Options());
    ~Harness();

    Harness(const Harness&)            = delete;
    Harness& operator=(const Harness&) = delete;

    const Options& options() const { return opts; }
    HeadlessPlatform* platform() { return headless; }

    // Screens

    // Application::pushActivity, then a frame
    void push(Activity* activity, TransitionAnimation animation = TransitionAnimation::NONE);
    // The activity on top (nullptr without any)
    Activity* top();
    // A view of the activity on top, by its XML id (nullptr when none)
    View* find(const std::string& id);

    // Time

    // Runs `count` frames, the clock moving one frame's time before each
    void step(int count = 1);
    // Runs frames for `ms` of simulated time
    void advance(double ms);
    // Runs frames until nothing moves: no animation running, no task queued
    // or running (sync/async), for 2 frames in a row; at most `maxMs` of
    // simulated time. Returns false when it gave up.
    bool settle(double maxMs = 5000);
    // Microseconds on the simulated clock
    Time now() const;
    uint64_t frameCount() const { return frames; }

    // Gamepad: press and release are kept until changed. tap() presses for
    // one frame, releases and settles.
    void press(ControllerButton button);
    void release(ControllerButton button);
    void tap(ControllerButton button, int times = 1);
    // Holds the button for `ms` (repeats come as on a device), then releases
    void hold(ControllerButton button, double ms);
    void setAxis(ControllerAxis axis, float value);

    // Keyboard (BRLS_KBD_KEY_*, GLFW's codes): Enter is A, Escape is B and
    // the arrows are the D-pad, as on the desktop
    void pressKey(BrlsKeyboardScancode key);
    void releaseKey(BrlsKeyboardScancode key);
    void tapKey(BrlsKeyboardScancode key, int times = 1);

    // Touch, in the views' coordinates (View::getFrame()). tapAt() and
    // tapView() touch down, wait a frame, lift and settle.
    void touchDown(float x, float y, int finger = 0);
    void touchMove(float x, float y, int finger = 0);
    void touchUp(int finger = 0);
    void tapAt(float x, float y);
    bool tapView(View* view);
    // A drag from one point to another over `ms`, then settles
    void swipe(float fromX, float fromY, float toX, float toY, double ms = 200);

    // Mouse, in the views' coordinates
    void mouseMove(float x, float y);
    void mouseClick(float x, float y);
    // Pressed and let go between two frames, as a quick click or a
    // touchpad tap
    void mouseQuickClick(float x, float y);
    void mouseScroll(float dx, float dy);

    // The next keyboard (ImeManager) opened gets `text`; without it, a
    // keyboard opened is cancelled
    void setKeyboardAnswer(const std::string& text);

    // Focus

    View* focus();
    // The text of the focused view: its own if it is a label or a button,
    // the title of a cell, else the first text found in it (empty: none)
    std::string focusText();
    // Every focus change since the start (or clearFocusLog()), as focusText()
    const std::vector<std::string>& focusLog() const { return focusChanges; }
    void clearFocusLog() { focusChanges.clear(); }
    // Moves the focus with `button` until focusText() is `text`, at most
    // `maxSteps` presses. Returns false when not reached.
    bool focusByText(const std::string& text, ControllerButton button = BUTTON_DOWN, int maxSteps = 30);

    // Text

    // The labels shown on the activities drawn whose text the box cuts
    // ("…", Label::isTruncated), their whole text. As with BRLS_CUT_TEXT, a
    // label only clipped away by its parent (a folded sidebar) still counts.
    std::vector<std::string> cutTexts();
    // Every label's text shown (visible views of the activities drawn)
    std::vector<std::string> texts();
    // True when a label shown has exactly this text
    bool hasText(const std::string& text);
    // The first label shown with exactly this text (nullptr: none), to tap
    View* findText(const std::string& text);

    // Accessibility

    // The views shown that take the focus themselves (not passing it on)
    // with nothing to tell them apart: no accessibility label nor value
    // (a role at most), as View::describe(). What the focus announcer was
    // given last: Application::getLastAnnouncement().
    std::vector<std::string> silentFocusables();

    // Pictures

    // The last frame drawn (a frame is drawn first if none was)
    Image screenshot();
    // Compares a screenshot (or the part of it in `area`, views'
    // coordinates) with the reference `<referenceDir>/<name>.png`: a pixel
    // differs when a channel is off by more than `tolerance`; it matches
    // when at most `maxRatio` of the pixels differ. Writes
    // `<outputDir>/<name>.png` (and `<name>.diff.png` when it differs).
    // A missing reference, or BRLS_TEST_UPDATE=1, writes the reference
    // instead and matches.
    bool matchesReference(const std::string& name, int tolerance = 8, double maxRatio = 0.001);
    bool matchesReference(const std::string& name, const Rect& area, int tolerance = 8, double maxRatio = 0.001);
    // What the last matchesReference() found
    const ImageDiff& lastDiff() const { return diff; }

    // Window

    // A new window size, then settles
    void resize(int width, int height);
    // The platform reports this safe area (framebuffer pixels: a notch, the
    // system's bars), then settles
    void setSafeArea(const Insets& insets);

  private:
    void walkShown(const std::function<void(View*)>& visit);

    Options opts;
    HeadlessPlatform* headless = nullptr;
    Time clock                 = 0;
    uint64_t frames            = 0;
    std::vector<std::string> focusChanges;
    ImageDiff diff;
};

// Text of a view as focusText() reads it
std::string viewText(View* view);

} // namespace brls::test

// A small test runner, for executables of test cases (tests/main.cpp):
//
//     BRLS_TEST(focus_moves) { brls::test::Harness h; ... BRLS_CHECK(...); }
//     int main(int argc, char** argv) { return brls::test::runTests(argc, argv); }
//
// `program` runs every case, each in a process of its own; `program name`
// runs one (what ctest calls, see brls_add_tests in library/cmake/AuroreTesting.cmake);
// `program --list` prints their names ("brls-test: name" lines).
namespace brls::test
{
using TestFunction = void (*)();
bool registerTest(const char* name, TestFunction function);
void reportFailure(const char* file, int line, const std::string& what);
int runTests(int argc, char** argv);
} // namespace brls::test

#define BRLS_TEST(name)                                                                                \
    static void brls_test_##name();                                                                    \
    static const bool brls_test_registered_##name = brls::test::registerTest(#name, brls_test_##name); \
    static void brls_test_##name()

#define BRLS_CHECK(condition)                                                            \
    do                                                                                   \
    {                                                                                    \
        if (!(condition))                                                                \
            brls::test::reportFailure(__FILE__, __LINE__, "BRLS_CHECK(" #condition ")"); \
    } while (0)

#define BRLS_CHECK_EQ(actual, expected)                                                                                  \
    do                                                                                                                   \
    {                                                                                                                    \
        auto brls_actual_   = (actual);                                                                                  \
        auto brls_expected_ = (expected);                                                                                \
        if (!(brls_actual_ == brls_expected_))                                                                           \
            brls::test::reportFailure(__FILE__, __LINE__,                                                                \
                std::string("BRLS_CHECK_EQ(" #actual ", " #expected "): got ") + brls::test::describeValue(brls_actual_) \
                    + ", expected " + brls::test::describeValue(brls_expected_));                                        \
    } while (0)

namespace brls::test
{
inline std::string describeValue(const std::string& value) { return "\"" + value + "\""; }
inline std::string describeValue(const char* value) { return "\"" + std::string(value) + "\""; }
template <typename T>
std::string describeValue(const T& value)
{
    if constexpr (std::is_arithmetic_v<T>)
        return std::to_string(value);
    else if constexpr (std::is_pointer_v<T>)
        return value ? "a pointer" : "nullptr";
    else
        return "(value)";
}
} // namespace brls::test
