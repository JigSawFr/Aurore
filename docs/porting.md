# Porting an app

What to mind when an app on Aurore runs on more than the platform it was written on: a Switch homebrew going to the desktop and Android, a desktop tool going to the Switch. The build commands are in [platforms.md](platforms.md); this page is about the app's code and resources. The rule of thumb: let Aurore read the platform, and keep the app's code free of `#ifdef`s but where a platform really has something of its own.

## The build

- One `CMakeLists.txt` for every platform: `commonOption.cmake` and `toolchain.cmake` before `project()`, the platform picked with `-DAURORE_PLATFORM=…`. Make the program with `program_target(name sources)` rather than `add_executable`: on Android it is a shared library SDL's Java activity loads, on Windows a GUI program unless `AURORE_WIN32_TERMINAL`.
- Add the platform's own source where it has one: `library/lib/platforms/switch/switch_wrapper.c` on the Switch (as [`template/`](../template/CMakeLists.txt) does), `library/lib/platforms/ps4/crashlog.c` on the PS4 for a backtrace when the app crashes.
- The packaging is per platform: the `.nro` and its romfs, the `.vpk` and its LiveArea, the PS4's `.pkg`, the macOS and iOS bundles, the Android project. Copy each from the demo's `CMakeLists.txt` (and `android-project/`), with the app's name, IDs and icon.
- Keep `main()` as it is everywhere: `init()`, `createWindow()`, a first activity, `while (mainLoop())`. Android and iOS start it through SDL: include `<SDL3/SDL_main.h>` (`<SDL2/SDL_main.h>` on SDL2) before it there; UWP includes `<aurore/core/main.hpp>` after it. The demo's `demo/src/main.cpp` does both:

  ```cpp
  #if (defined(ANDROID) || defined(IOS)) && defined(__SDL3__)
  #include <SDL3/SDL_main.h>
  #elif defined(ANDROID) || defined(IOS)
  #include <SDL2/SDL_main.h>
  #endif
  // … int main(int argc, char* argv[]) { … } …
  #ifdef __WINRT__
  #include <aurore/core/main.hpp>
  #endif
  ```

  In a browser only, the loop is handed to `emscripten_set_main_loop` ([platforms.md](platforms.md#in-a-browser)).

## Resources

- Name every resource as `@res/…` in XML and `AURORE_ASSET("…")` in code: Aurore turns it into the right place on each platform (a file next to the program, the romfs, the package, the program itself with libromfs). A path relative to the working directory works on the desktop only.
- Android and iOS always build the resources in (libromfs): run `./build_libromfs_generator.sh` before the first build, and remember that a deleted file may need CMake to run again.
- Files the app writes (its settings, a cache) are not resources: each platform has its own place for them (the SD card on the Switch, the user's folders on a desktop, the app's sandbox on a phone), and Aurore does not pick one.
- On the Switch, the system's shared fonts replace the text and button fonts, which the demo leaves out of its romfs; elsewhere the free fonts in `resources/font/` are used, and Chinese, Japanese and Korean come from the system ([fonts.md](fonts.md)). A browser has no system font at all.

## Screens

- The UI is laid out 1280 units wide, whatever the screen: 720 units high on the Switch and a 16:9 window, less on a wider phone, more on a 16:10 or 4:3 screen (`Application::contentHeight`). Lay screens out with `grow`, percentages and scrolling frames rather than a fixed 720, and try the app in a window of another shape on the desktop.
- A large desktop window makes everything large; `Application::setMaximumUIScale()` caps it, and the rest of the window is given to the layout.
- The window can change size on the desktop, and the Switch changes resolution when docked: views are told (`View::onWindowSizeChanged`, `Application::getWindowSizeChangedEvent()`), activities are resized for them.

## Input

- Every platform has the controller's buttons: a pad on the consoles and desktops, the keyboard too on a desktop (the arrows, Enter for A, Escape for B, the other buttons on letters: [input.md](input.md)), SDL's controllers on phones. Register actions on buttons (`registerAction`, `registerClickAction`) rather than on keys, and the footer shows the right hints.
- Touch is the only input on a phone without a pad, and one of two on the Switch in handheld mode and the Vita: anything reached with a button must be reachable with a tap too (a button, a cell, `addGestureRecognizer`), not through a hidden action alone. `Application::getInputType()` says which was used last; the focus highlight is not drawn after a touch.
- The mouse works on a desktop (hover styles, the wheel in scrolling frames); there is nothing to hover on the other platforms.
- Text input differs: the system keyboard on the consoles and phones, a dialog of Aurore's under GLFW and SDL on a desktop (`ImeManager`, through `InputCell` or `Application::getImeManager()`).

## What the platform can do

`Application::getPlatform()` answers for the system, and some answers are "no" on some platforms: check them rather than assuming the Switch's.

| Ask | Before using |
| --- | --- |
| battery, network, IP address | `canShowBatteryLevel()`, `canShowWirelessLevel()` |
| screen brightness | `canSetBacklightBrightness()` |
| links | `openBrowser(url)` opens http and https links only, in the system's browser (the web applet on the Switch); `Platform::isBrowserUrl` says beforehand |
| clipboard | `pasteToClipboard`, `pasteFromClipboard`: GLFW and SDL only, nothing elsewhere |
| language, theme | `getLocale()`, `getThemeVariant()`: the system's, unless the app sets `Platform::APP_LOCALE_DEFAULT` or a variant ([environment.md](environment.md) for the desktop's overrides) |
| the home menu | `isApplicationMode()`, `exitToHomeMode()`: the Switch's |

When something is a platform's own (a Switch service, an Android intent), keep it behind one function of the app's, with the compiler's defines: `__SWITCH__`, `__PSV__`, `PS4`, `ANDROID`, `IOS`, `__APPLE__`, `_WIN32`, `__linux__`, `__EMSCRIPTEN__`.

## Threads and speed

- The UI belongs to the main thread on every platform ([architecture.md](architecture.md#threads)): slow work goes to `aurore::async`, its result back with `aurore::sync`. The Vita and the PS4 use pthreads; `-DAURORE_STD_THREAD=ON` uses `std::thread` where pthreads are missing.
- The consoles are slower than a desktop: long lists in a `RecyclerFrame` or `RecyclingGrid`, pictures no larger than shown (Aurore refuses those over 4096×4096), `setImageFromFileAsync` for covers. `-DAURORE_SIMPLE_HIGHLIGHT=ON` and `Application::setRedrawOnDemand(true)` save frames on a Vita or on a battery.
- Quitting differs: the Switch's apps leave through the Home menu or `+` (`setGlobalQuit`), a desktop through its window, a phone never quits on its own.

## Licence

Cadran, Aurore's look, is GPLv3: an app built with Aurore is GPLv3 on every platform, stores included.
