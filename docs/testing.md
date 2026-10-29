# Testing

Aurore has a test harness: an app's screens run in a process with no window and no display, inputs are given through calls, time is simulated (frames advance without waiting, the same way every run, animations included), and screenshots are compared with references. It replaces scripts that drive a real window under Xvfb with `xdotool` and grab it with `import`.

It is built with `-DAURORE_BUILD_TESTS=ON`, on the desktop with the OpenGL 3 driver (the default). Without it, the library only has the few hooks the harness uses: a clock that can be simulated (`setSimulatedTimeUsec`, `getWallTime`), `VideoContext::readPixels` and `Platform::setFactory`.

## The harness

`library/include/aurore/test/harness.hpp`, target `Aurore::test`.

```cpp
#include <aurore.hpp>
#include <aurore/test/harness.hpp>

AURORE_TEST(settings_opens)
{
    my_app::install_hooks();          // what the app does before Application::init()
    aurore::test::Harness h;            // Application::init() and the window
    my_app::register_views();         // what it does after createWindow()
    h.push(new MainActivity());
    h.settle();                       // animations and tasks done

    AURORE_CHECK(h.focusByText("Settings"));   // presses Down until there
    h.tap(aurore::BUTTON_A);
    AURORE_CHECK_EQ(h.focusText(), std::string("Language"));
    AURORE_CHECK(h.cutTexts().empty());         // no label cut short ("…")
    AURORE_CHECK(h.matchesReference("settings"));
}

int main(int argc, char** argv) { return aurore::test::runTests(argc, argv); }
```

What it gives:

| Call | Does |
|---|---|
| `Harness(Options)` | `width`, `height`, `frameMs` (simulated time per frame), `locale`, `theme`, reference and output folders, `systemFonts` (the system's Chinese, Japanese and Korean fonts, off by default). One harness per process: `Application` is global |
| `push(activity)`, `top()`, `find(id)` | a screen pushed (without transition), the one on top, a view of it by XML id |
| `step(n)`, `advance(ms)`, `settle(maxMs)` | `n` frames; frames for `ms` of simulated time; frames until no animation runs and no `sync`/`async` task is queued or running |
| `press`, `release`, `tap(button, times)`, `hold(button, ms)`, `setAxis` | the gamepad (`tap` presses for a frame, releases and settles; `hold` gets the repeats a device gets) |
| `pressKey`, `releaseKey`, `tapKey` | the keyboard (`AURORE_KBD_KEY_*`): Enter is A, Escape is B, the arrows the D-pad and the other keys of [input.md](input.md), as on the desktop; watched keys (`Application::addToWatchedKeys`) and `getKeyboardKeyStateChanged()` see them |
| `touchDown`, `touchMove`, `touchUp`, `tapAt`, `tapView`, `swipe` | touch, in the views' coordinates (`View::getFrame()`) |
| `mouseMove`, `mouseClick`, `mouseScroll` | the mouse |
| `setKeyboardAnswer(text)` | the next keyboard opened (`ImeManager`) gets `text`; without one it is cancelled |
| `focus()`, `focusText()`, `focusLog()`, `focusByText(text, button)` | the focused view, its text (a label's, a button's, a cell's title, else the first text in it), every focus change so far, the focus moved until it reads `text` |
| `texts()`, `hasText(text)`, `findText(text)`, `cutTexts()` | the labels shown (one by its text, to tap it), and those their box cuts (`Label::isTruncated`): what `AURORE_FOCUS_LOG` and `AURORE_CUT_TEXT` wrote to the log and to `cut_text.txt`, as values |
| `silentFocusables()`, `aurore::Application::getLastAnnouncement()` | the views shown that take the focus with no accessibility label nor value (a role at most), and what the focus announcer was given last (docs/styles.md, "Accessibility") |
| `screenshot()` | the last frame drawn, as an `Image` (RGBA, `savePng`, `loadPng`, `crop`) |
| `matchesReference(name, tolerance, maxRatio)`, also with an area | the screenshot (or a part) against `<references>/<name>.png`, see below |
| `resize(w, h)` | a new window size |
| `setSafeArea(insets)` | the platform reports this safe area (a notch, the system's bars: [safe-areas.md](safe-areas.md)), the screen laid out again |
| `platform()` | the `HeadlessPlatform`: `openedUrls` (what `openBrowser` was asked), the clipboard, `requestClose()` |

`compareImages(actual, expected, tolerance)` compares two images on its own: a pixel differs when a channel is off by more than `tolerance`, and the result has the count, the ratio, the largest difference and a map of the differences.

`AURORE_TEST(name)` declares a case, `AURORE_CHECK(condition)` and `AURORE_CHECK_EQ(actual, expected)` check (a failure is printed and the case goes on), `aurore::test::runTests` runs them: `program` runs every case, each in a process of its own, `program name` one, `program --list` lists them.

## Screenshots and references

`matchesReference("main")` takes a screenshot, writes it to `<output>/main.png`, and compares it with `<references>/main.png`: it matches when at most `maxRatio` of the pixels (0.1 % by default) have a channel off by more than `tolerance` (8 by default). When it does not, `<output>/main.diff.png` shows the pixels that differ in red. A missing reference is written and matches, except in CI (the `CI` variable set, as on GitHub Actions) where it fails: a test whose reference was not committed, or whose name has a typo, does not pass unseen. `AURORE_TEST_UPDATE=1` writes them all again, to commit after looking at them. Each case registered by `aurore_add_tests` stops after 5 minutes.

The folders: `Options::referenceDir` and `outputDir`, else `AURORE_TEST_REFERENCES` and `AURORE_TEST_OUTPUT`, else `references` and `test-output` in the working directory. `aurore_add_tests` (below) sets them.

The same frames are drawn on every run: the clock is simulated, the platform says the same thing everywhere (no battery or Wi-Fi shown, `127.0.0.1`, the locale and theme of `Options`), the fonts are the app's resources: no system font is read unless `Options::systemFonts` asks for it ([fonts](fonts.md)). On Linux, the drawing is Mesa's software renderer (llvmpipe), with no display: the references are made with it, and a tolerance absorbs what another version of it changes in the antialiasing.

## How it draws without a display

`HeadlessPlatform` (`aurore/test/headless_platform.hpp`) takes the place of GLFW or SDL (`Platform::setFactory`). Its video context draws into an offscreen framebuffer of an OpenGL 3.2 context made by EGL on Mesa's surfaceless platform: no X server, no Xvfb. Where EGL is missing (macOS, Windows), or with `AURORE_HEADLESS_WINDOW=1`, the context is a hidden window's, SDL3's or GLFW's (whichever the library is built with; it needs a display). On Ubuntu: `libegl-dev` to build, `libegl1` and `libgl1-mesa-dri` to run.

The clock is borealis' own: `getCPUTimeUsec()` returns the simulated time once `setSimulatedTimeUsec()` set one, which the harness moves one frame's time before each frame. Animations, timers, `aurore::delay`, button repeats and the highlight read it; the main loop no longer sleeps to wait for a frame.

## In an app

```cmake
set(AURORE_BUILD_TESTS ON CACHE BOOL "" FORCE)   # or -DAURORE_BUILD_TESTS=ON
add_subdirectory(library/aurore/library)

enable_testing()
add_executable(my_app_tests tests/ui_tests.cpp ${MY_APP_SOURCES_BUT_MAIN})
target_link_libraries(my_app_tests PRIVATE Aurore::test)
aurore_add_tests(my_app_tests REFERENCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/references)
```

`aurore_add_tests` registers each case with `ctest` (read from `--list` when `ctest` runs), with the references folder and `<build>/test-output` for what the tests got. The app's `main()` stays out of the test executable: what it does before and after `createWindow()` goes in functions the tests call too (the demo's `demo/src/demo_app.cpp`).

An Aurore installed with `-DAURORE_INSTALL=ON -DAURORE_BUILD_TESTS=ON` installs the harness too: after `find_package(Aurore)`, `Aurore::test` and `aurore_add_tests` are there.

A smoke scenario of an Xvfb script (start, press keys, wait, grab the screen, compare) becomes a case: `h.tap(aurore::BUTTON_DOWN, 3)` for the key presses, `h.settle()` for the waits, `matchesReference` for the grab and the comparison, `focusText()` and `cutTexts()` for what was read in the log and in `cut_text.txt`.

## The demo's tests

```sh
cmake -B build -G Ninja -DAURORE_PLATFORM=DESKTOP -DAURORE_WINDOW=GLFW -DAURORE_LIBROMFS=ON -DAURORE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

They need `AURORE_LIBROMFS=ON`, which configuring says: their reference screenshots are drawn with the resources built in, and from files the desktop build picks other fonts. With CMake 3.25 or later, `cmake --workflow --preset tests` configures, builds and runs them in `build/tests`.

`tests/demo_tests.cpp`: focus moves through the rail and into a tab, a dialog opens and closes, keyboard and touch, the window resized, redraw on demand and its debug mode, no label cut short on any tab at the largest text size (in German, Russian, French and Japanese), the Welcome, Library and Themes tabs and an action sheet on a small screen (compact width), screenshots against `tests/references/`. CI runs them (`linux (tests)`).

`aurore-tour` (`tests/tour.cpp`) draws every screen of the demo to PNGs the same way, without a window: each tab of the rail and its content a few presses further down, then the screens and dialogs it opens (a game's page, the action sheet, the controller under test…), and prints the labels cut short on each. For looking at the whole demo in one language, theme and size:

```sh
aurore-tour --lang ja --theme light --size 1280x800 -o shots   # every scene
aurore-tour --list                                            # their names
aurore-tour -o shots tab01 library-                           # those starting so
aurore-tour --small -o shots                                  # a small screen's layout
```

`--small` lays the demo out on 480x360 units at the dense density (as `AURORE_DESIGN_SIZE=480x360 AURORE_DENSITY=dense` does in a window), the compact width class, drawn on 960x720 pixels.

`tests/async_tests.cpp` covers what crosses threads: a task coming back to a deleted view (`View::aliveToken()`, the `ASYNC_*` macros), the battery and Wi-Fi widgets read on the task thread (`HeadlessPlatform::showStatus` shows them), `delay()`/`cancelDelay()`, `Threading::stop()`/`start()`, events fired from another thread, logging from the task thread. Built with ThreadSanitizer they check the locking too:

```sh
cmake -B build-tsan -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DAURORE_PLATFORM=DESKTOP -DAURORE_WINDOW=GLFW -DAURORE_LIBROMFS=ON -DAURORE_BUILD_TESTS=ON \
    -DCMAKE_CXX_FLAGS=-fsanitize=thread -DCMAKE_C_FLAGS=-fsanitize=thread -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=thread
cmake --build build-tsan --target aurore_async_tests
TSAN_OPTIONS=suppressions=$PWD/tests/tsan.supp ctest --test-dir build-tsan -R aurore_async_tests --output-on-failure
```

`tests/tsan.supp` leaves out what Mesa's software renderer, which draws the tests and is not built with the sanitizer, does in its own threads.

`tests/host/`: Cadran's plain C++ parts without a window, each suite linked only to the sources it tests: the palettes' contrast (every pair of `cadran::palette::CONTRAST`, both themes), safe text and the Markdown parser, `ui::guarded`, and the text helpers (`cadran_text`, `format_text`) over the real `cadran.json` catalogs. `ctest -R host` runs them alone.

## The demo in a browser

`tools/web_smoke.py` starts the demo's build for a browser in headless Chromium: its first screen, a key, the service worker GitHub Pages needs, the address' language and theme ([web/README.md](../web/README.md#testing-it)).

## Fuzzing

`-DAURORE_BUILD_FUZZERS=ON`, with the tests and clang (and its runtimes, `libclang-rt-18-dev` on Ubuntu), builds the libFuzzer targets of `tests/fuzz`, under AddressSanitizer and UndefinedBehaviorSanitizer:

| Target | What it reads |
| --- | --- |
| `aurore_fuzz_markdown` | Cadran's Markdown and `safe_text`, on any bytes: the bounds `markdown.hpp` promises, every string valid UTF-8, `clean()` stable |
| `aurore_fuzz_theme` | A theme file (`Theme::load`), as an app reads one from a folder its user fills, from an empty folder of its own: nothing it includes or inherits is read; and `Theme::parseColor`. The library is built for the fuzzer too, so that it follows the paths inside it: a build of its own |

```sh
CC=clang CXX=clang++ cmake -B build-fuzz -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DAURORE_PLATFORM=DESKTOP \
    -DAURORE_LIBROMFS=ON -DAURORE_BUILD_TESTS=ON -DAURORE_BUILD_FUZZERS=ON
cmake --build build-fuzz --target aurore_fuzz_markdown aurore_fuzz_theme
mkdir -p corpus/theme && build-fuzz/tests/aurore_fuzz_theme -max_total_time=60 corpus/theme resources/themes
```

CI's `linux (fuzz)` runs each for two minutes on every pull request, from the corpus the runs before it grew (and the repository's docs and themes); an input that crashes one goes up as the `fuzz-crashes` artifact, to run again with `aurore_fuzz_theme <file>`.

## The bench

`aurore_bench` (`tests/bench.cpp`) runs the demo's main screen under the harness with the `Profiler` on, over three scenarios (still, every frame drawn; moving through the rail; still with redraw on demand), and prints per frame: frames drawn, layout passes, views laid out, nanovg draw calls, allocations (a replaced `operator new`), and microseconds per phase (input, update, layout, draw, flush, swap, total).

```sh
aurore_bench                                   # the table
aurore_bench --json out.json                   # and the numbers
aurore_bench --write-baseline tests/bench_baseline.json
aurore_bench --check tests/bench_baseline.json # exit 1 on a regression
```

`--check` fails when a count is above the baseline's by more than `--margin` (25 %, and 1). Times depend on the machine: they are checked only with `--time-margin <factor>`. `ctest` runs the check.

The `Profiler` (`aurore/core/profiler.hpp`) is in every build and costs a test per phase while off. `Profiler::setEnabled(true)` turns it on (`getLastFrame()`, `getFrameEvent()`, `setAllocationCounter()`); `AURORE_PROFILE=1` in the environment of a desktop app does too (a build with `AURORE_DEV_TOOLS`, see [hot reload](hot_reload.md#turning-it-on)), and logs averages every 120 frames. The [inspector](inspector.md)'s panel shows its figures, and it measures while the panel is open. Each `FrameProfile` has the time of each phase (`events`, `input`, `update`, `layout`, `draw`, `inspector`, `flush`, `swap`), the `total` and the `interval` since the iteration before, and counts: layouts, views laid out, nanovg's draw calls and triangles, allocations.

## aurore-preview

```sh
aurore-preview resources/xml/tabs/layout.xml --theme light --lang fr --size 1920x1080 -o layout.png
```

Draws an XML layout to a PNG with the harness: `--theme dark`, `light` or a theme file (`Theme::loadFile`), `--lang` a locale, `--size` the window. The XML may use borealis', Cadran's and the demo's views; an app with views of its own builds the same tool on the harness after registering them.

## Redraw on demand: finding a missing invalidation

With redraw on demand (`Application::setRedrawOnDemand(true)`), a view that changes how it looks without `invalidate()` or `requestRedraw()` stays stale on screen. `Application::setRedrawDebug(true)`, or `AURORE_REDRAW_DEBUG=1` on the desktop (a build with `AURORE_DEV_TOOLS`), draws each skipped frame all the same, without presenting it, compares it with the last frame presented, and logs a difference as a warning with its area and the views drawn there:

```
redraw debug: a skipped frame differs from the last one presented (412 pixels in 1104,20 96x28): no redraw asked for Label (id: clock)
```

`Application::getMissedRedrawCount()` and `getLastMissedRedrawArea()` give the same to a test. It reads every frame back: slow, for debugging, OpenGL drivers only (`VideoContext::readPixels`).
