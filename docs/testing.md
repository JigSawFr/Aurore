# Testing

Aurore has a test harness: an app's screens run in a process with no window and no display, inputs are given through calls, time is simulated (frames advance without waiting, the same way every run, animations included), and screenshots are compared with references. It replaces scripts that drive a real window under Xvfb with `xdotool` and grab it with `import`.

It is built with `-DBRLS_BUILD_TESTS=ON`, on the desktop with the OpenGL 3 driver (the default). Without it, the library only has the few hooks the harness uses: a clock that can be simulated (`setSimulatedTimeUsec`, `getWallTime`), `VideoContext::readPixels` and `Platform::setFactory`.

## The harness

`library/include/borealis/test/harness.hpp`, target `borealis_test`.

```cpp
#include <borealis.hpp>
#include <borealis/test/harness.hpp>

BRLS_TEST(settings_opens)
{
    my_app::install_hooks();          // what the app does before Application::init()
    brls::test::Harness h;            // Application::init() and the window
    my_app::register_views();         // what it does after createWindow()
    h.push(new MainActivity());
    h.settle();                       // animations and tasks done

    BRLS_CHECK(h.focusByText("Settings"));   // presses Down until there
    h.tap(brls::BUTTON_A);
    BRLS_CHECK_EQ(h.focusText(), std::string("Language"));
    BRLS_CHECK(h.cutTexts().empty());         // no label cut short ("…")
    BRLS_CHECK(h.matchesReference("settings"));
}

int main(int argc, char** argv) { return brls::test::runTests(argc, argv); }
```

What it gives:

| Call | Does |
|---|---|
| `Harness(Options)` | `width`, `height`, `frameMs` (simulated time per frame), `locale`, `theme`, reference and output folders. One harness per process: `Application` is global |
| `push(activity)`, `top()`, `find(id)` | a screen pushed (without transition), the one on top, a view of it by XML id |
| `step(n)`, `advance(ms)`, `settle(maxMs)` | `n` frames; frames for `ms` of simulated time; frames until no animation runs and no `sync`/`async` task is queued or running |
| `press`, `release`, `tap(button, times)`, `hold(button, ms)`, `setAxis` | the gamepad (`tap` presses for a frame, releases and settles; `hold` gets the repeats a device gets) |
| `pressKey`, `releaseKey`, `tapKey` | the keyboard (`BRLS_KBD_KEY_*`): Enter is A, Escape is B, the arrows the D-pad, as on the desktop; watched keys (`Application::addToWatchedKeys`) and `getKeyboardKeyStateChanged()` see them |
| `touchDown`, `touchMove`, `touchUp`, `tapAt`, `tapView`, `swipe` | touch, in the views' coordinates (`View::getFrame()`) |
| `mouseMove`, `mouseClick`, `mouseScroll` | the mouse |
| `setKeyboardAnswer(text)` | the next keyboard opened (`ImeManager`) gets `text`; without one it is cancelled |
| `focus()`, `focusText()`, `focusLog()`, `focusByText(text, button)` | the focused view, its text (a label's, a button's, a cell's title, else the first text in it), every focus change so far, the focus moved until it reads `text` |
| `texts()`, `hasText(text)`, `findText(text)`, `cutTexts()` | the labels shown (one by its text, to tap it), and those their box cuts (`Label::isTruncated`): what `BRLS_FOCUS_LOG` and `BRLS_CUT_TEXT` wrote to the log and to `cut_text.txt`, as values |
| `screenshot()` | the last frame drawn, as an `Image` (RGBA, `savePng`, `loadPng`, `crop`) |
| `matchesReference(name, tolerance, maxRatio)`, also with an area | the screenshot (or a part) against `<references>/<name>.png`, see below |
| `resize(w, h)` | a new window size |
| `platform()` | the `HeadlessPlatform`: `openedUrls` (what `openBrowser` was asked), the clipboard, `requestClose()` |

`compareImages(actual, expected, tolerance)` compares two images on its own: a pixel differs when a channel is off by more than `tolerance`, and the result has the count, the ratio, the largest difference and a map of the differences.

`BRLS_TEST(name)` declares a case, `BRLS_CHECK(condition)` and `BRLS_CHECK_EQ(actual, expected)` check (a failure is printed and the case goes on), `brls::test::runTests` runs them: `program` runs every case, each in a process of its own, `program name` one, `program --list` lists them.

## Screenshots and references

`matchesReference("main")` takes a screenshot, writes it to `<output>/main.png`, and compares it with `<references>/main.png`: it matches when at most `maxRatio` of the pixels (0.1 % by default) have a channel off by more than `tolerance` (8 by default). When it does not, `<output>/main.diff.png` shows the pixels that differ in red. A missing reference is written and matches; `BRLS_TEST_UPDATE=1` writes them all again, to commit after looking at them.

The folders: `Options::referenceDir` and `outputDir`, else `BRLS_TEST_REFERENCES` and `BRLS_TEST_OUTPUT`, else `references` and `test-output` in the working directory. `brls_add_tests` (below) sets them.

The same frames are drawn on every run: the clock is simulated, the platform says the same thing everywhere (no battery or Wi-Fi shown, `127.0.0.1`, the locale and theme of `Options`), the fonts are the app's resources. On Linux, the drawing is Mesa's software renderer (llvmpipe), with no display: the references are made with it, and a tolerance absorbs what another version of it changes in the antialiasing.

## How it draws without a display

`HeadlessPlatform` (`borealis/test/headless_platform.hpp`) takes the place of GLFW or SDL (`Platform::setFactory`). Its video context draws into an offscreen framebuffer of an OpenGL 3.2 context made by EGL on Mesa's surfaceless platform: no X server, no Xvfb. Where EGL is missing (macOS, Windows), or with `BRLS_HEADLESS_GLFW=1`, the context is a hidden GLFW window's (which needs a display). On Ubuntu: `libegl-dev` to build, `libegl1` and `libgl1-mesa-dri` to run.

The clock is borealis' own: `getCPUTimeUsec()` returns the simulated time once `setSimulatedTimeUsec()` set one, which the harness moves one frame's time before each frame. Animations, timers, `brls::delay`, button repeats and the highlight read it; the main loop no longer sleeps to wait for a frame.

## In an app

```cmake
set(BRLS_BUILD_TESTS ON CACHE BOOL "" FORCE)   # or -DBRLS_BUILD_TESTS=ON
add_subdirectory(library/aurore/library)

enable_testing()
add_executable(my_app_tests tests/ui_tests.cpp ${MY_APP_SOURCES_BUT_MAIN})
target_link_libraries(my_app_tests PRIVATE borealis_test)
brls_add_tests(my_app_tests REFERENCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/references)
```

`brls_add_tests` registers each case with `ctest` (read from `--list` when `ctest` runs), with the references folder and `<build>/test-output` for what the tests got. The app's `main()` stays out of the test executable: what it does before and after `createWindow()` goes in functions the tests call too (the demo's `demo/src/demo_app.cpp`).

A smoke scenario of an Xvfb script (start, press keys, wait, grab the screen, compare) becomes a case: `h.tap(brls::BUTTON_DOWN, 3)` for the key presses, `h.settle()` for the waits, `matchesReference` for the grab and the comparison, `focusText()` and `cutTexts()` for what was read in the log and in `cut_text.txt`.

## The demo's tests

```sh
cmake -B build -G Ninja -DPLATFORM_DESKTOP=ON -DUSE_GLFW=ON -DUSE_LIBROMFS=ON -DBRLS_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

`tests/demo_tests.cpp`: focus moves through the rail and into a tab, a dialog opens and closes, keyboard and touch, the window resized, redraw on demand and its debug mode, screenshots against `tests/references/`. CI runs them (`linux (tests)`).

## Redraw on demand: finding a missing invalidation

With redraw on demand (`Application::setRedrawOnDemand(true)`), a view that changes how it looks without `invalidate()` or `requestRedraw()` stays stale on screen. `Application::setRedrawDebug(true)`, or `BRLS_REDRAW_DEBUG=1` on the desktop, draws each skipped frame all the same, without presenting it, compares it with the last frame presented, and logs a difference as a warning with its area and the views drawn there:

```
redraw debug: a skipped frame differs from the last one presented (412 pixels in 1104,20 96x28): no redraw asked for Label (id: clock)
```

`Application::getMissedRedrawCount()` and `getLastMissedRedrawArea()` give the same to a test. It reads every frame back: slow, for debugging, OpenGL drivers only (`VideoContext::readPixels`).
