# Platforms

How to build Aurore's demo, and an app on Aurore, for each platform. The configure and build commands are those the CI runs (`.github/workflows/ci.yaml`, where the toolchains are pinned; `site.yml` for a browser); its artifacts are the demo built for each platform. Getting the toolchains themselves is each SDK's business: the links say where.

An app builds the same way as the demo: its `CMakeLists.txt` includes `library/aurore/library/cmake/commonOption.cmake` and `toolchain.cmake` before `project()`, then `add_subdirectory(library/aurore/library)` and links `Aurore::aurore` (see the [README](../README.md#-use-it-in-your-project) and [`template/`](../template/README.md)). The platform is chosen with `-DAURORE_PLATFORM=…`, and the packaging (an `.nro`, a `.vpk`, a `.pkg`, an app bundle) is the app's own: the demo's `CMakeLists.txt` has each one, to copy. [porting.md](porting.md) has what an app's code should mind on each.

| Platform | `AURORE_PLATFORM` | Window and input (`AURORE_WINDOW`) | Drawing (`AURORE_GRAPHICS`) | Resources |
| --- | --- | --- | --- | --- |
| Linux, macOS, Windows | `DESKTOP` | SDL3 (default), GLFW or SDL2 | OpenGL 3.2; or `GL2`, `GLES2`, `GLES3`; SDL_GPU, a prototype: Direct3D 12, Metal or Vulkan (`SDLGPU`) | files next to the program, or built in (`AURORE_LIBROMFS`) |
| Nintendo Switch | `SWITCH` | GLFW (default) or SDL2 | deko3d (`DEKO3D`) or OpenGL 4.3 | romfs, in the `.nro` |
| PS Vita | `PSV` | SDL2, or the system's | GXM (`GXM`) or OpenGL ES 2 | in the `.vpk` |
| PS4 | `PS4` | SDL2 | OpenGL ES 2 | in the `.pkg` |
| New 3DS, New 3DS XL, New 2DS XL (demo and test card) | `3DS` | libctru's | citro3d | `romfs:/` |
| Android | `ANDROID` | SDL3 (default) or SDL2 | OpenGL ES 2 (or `GLES3`) | built in (libromfs, forced) |
| iOS, tvOS | `IOS`, `TVOS` | SDL3 (default) or SDL2 | OpenGL ES 3 (or `GLES2`) | built in (libromfs, forced) |
| Browser | Emscripten (`emcmake`) | SDL3 (default) or SDL2 | OpenGL ES 3 (WebGL 2) | packaged with the page |

Every build needs CMake 3.20 or later (3.24 on macOS, iOS and tvOS with SDL2 or SDL3 built from the submodule) and a C++23 compiler, for Aurore and for the projects that link it. Clone with the submodules (`git clone --recursive`, or `git submodule update --init --recursive`).

SDL3 is the window and input everywhere it runs: the desktop, a browser, Android, iOS and tvOS. It is the submodule `library/lib/extern/SDL3` (3.4.18), the system's with `USE_SYSTEM_SDL3=ON` (`find_package(SDL3)`), a download with `AURORE_FETCH_DEPS`, Emscripten's port in a browser. On the desktop it gives what GLFW gave: the app's ID for the taskbar (`Application::setAppId`), the window's icon (`icon/icon.png`), a saved position kept only while it is on a screen, the app's controller mappings (`gamepad/gamecontrollerdb.txt`), the battery on every system SDL reads it on (`SDL_GetPowerInfo`, Linux's too; iOS keeps its own reading), and the system's light or dark theme on every system, Linux's desktop portal too, followed while the app runs until it picks one (`Platform::setThemeVariant`; `Platform::followSystemTheme` goes back to it) or `AURORE_THEME` gives it. Windows and X11 size windows in pixels: a first window, before any size is saved, is opened at the main screen's density (1280x720 is 2560x1440 at 200 %), and a window keeps its size in points when the density changes under it (moved to another screen on Windows, `Xft.dpi` changed on X11), each time within the screen's free area. Under SDL3 as under SDL2, a controller's face buttons are read by their label: A is the button marked A, on a Nintendo controller as on an Xbox one (SDL3 itself numbers them by place). An app's code that means either SDL tests `__SDL__`, defined under both; `__SDL2__` or `__SDL3__` for one only.

SDL2 stays for the consoles, which have no SDL3: devkitPro has none for the Switch, nor PacBrew for the PS4, and the Vita's OpenGL ES build would need an SDL3 built with PVR_PSP2 in its Docker image (`AURORE_WINDOW=SDL3` stops the configuration there). Elsewhere it is an option, `-DAURORE_WINDOW=SDL2` (`-PauroreSdl=2` on Android). SDL2 is the submodule `library/lib/extern/SDL` (2.32.10) on the desktop, Android, iOS and tvOS, and on the Vita without `USE_SYSTEM_SDL2`. Elsewhere it is the toolchain's: devkitPro's `switch-sdl2` on the Switch, PacBrew's SDL 2.0.18 on the PS4, Emscripten's port in a browser, and the SDL in the Docker image of the Vita's OpenGL ES build (2.32.10 with `psv/scripts/sdl2/ime.patch`). Aurore's SDL code builds with all of them.

## Linux

On Debian or Ubuntu:

```sh
sudo apt install build-essential cmake ninja-build git libdbus-1-dev libxkbcommon-dev \
  libwayland-dev wayland-protocols xorg-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release -DAURORE_PLATFORM=DESKTOP -DAURORE_LIBROMFS=ON
cmake --build build
./build/aurore_demo
```

`-DAURORE_WINDOW=GLFW` builds with GLFW instead of SDL3, `-DAURORE_WINDOW=SDL2` with SDL2; `-DAURORE_GRAPHICS=GL2`, `-DAURORE_GRAPHICS=GLES2` or `-DAURORE_GRAPHICS=GLES3` another OpenGL. Without `AURORE_LIBROMFS`, the `resources/` folder is copied next to the program at each build. The presets do the same in fewer words: `cmake --preset desktop-sdl3` (or `desktop-glfw`, `desktop-sdl` for SDL2), then `cmake --build --preset desktop-sdl3`.

The tests need EGL and Mesa as well (`libegl-dev libegl1 libegl-mesa0 libgl1-mesa-dri`), then `-DAURORE_BUILD_TESTS=ON` and `ctest --test-dir build` ([testing.md](testing.md)).

## macOS

With Xcode's command-line tools and CMake (Homebrew's, for instance):

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DAURORE_PLATFORM=DESKTOP -DBUNDLE_MACOS_APP=ON
make -C build -j$(sysctl -n hw.ncpu)
open build/aurore_demo.app
```

The build is for Apple silicon and runs from macOS 27, which needs Xcode 27; `-DCMAKE_OSX_DEPLOYMENT_TARGET` and `-DCMAKE_OSX_ARCHITECTURES` change both, down to what the C++23 code compiles for. `-DBUNDLE_MACOS_APP=ON` makes an `.app` with the resources inside; without it, a plain program with `resources/` next to it. `-DAURORE_WINDOW=GLFW` builds with GLFW, `-DAURORE_WINDOW=SDL2` with SDL2.

## Windows

With [MSYS2](https://www.msys2.org/), in its *MINGW64* shell:

```sh
pacman -S --needed git mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-cc
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DAURORE_PLATFORM=DESKTOP \
  -DAURORE_WIN32_TERMINAL=OFF -DAURORE_LIBROMFS=ON -DZLIB_USE_STATIC_LIBS=ON
cmake --build build
./build/aurore_demo.exe
```

`-DAURORE_GRAPHICS=SDLGPU` draws with Direct3D 12 instead of OpenGL ([below](#sdl_gpu-on-the-desktop-prototype); Direct3D 11 is gone). `-DAURORE_WIN32_TERMINAL=OFF` makes a program without a console window (`ON` keeps one, for the logs). The demo links the C and C++ runtimes statically (`-static` in its `CMakeLists.txt`): it runs without MSYS2's DLLs. `library/cmake/MinGWCross.cmake` is a toolchain file for building it from Linux with MinGW-w64 (not built by CI).

## Nintendo Switch

With [devkitPro](https://devkitpro.org/wiki/Getting_Started): the `switch-dev` group, and for the OpenGL build `switch-glfw`, `switch-mesa` and `switch-glm` (or `switch-sdl2` with `-DAURORE_WINDOW=SDL2`). The CI uses the `devkitpro/devkita64` image.

```sh
export DEVKITPRO=/opt/devkitpro
cmake -B build_switch -DCMAKE_BUILD_TYPE=Release -DAURORE_PLATFORM=SWITCH -DAURORE_GRAPHICS=DEKO3D
make -C build_switch aurore_demo.nro -j$(nproc)
```

`build_switch/aurore_demo.nro` goes to `/switch/` on the SD card, or straight to a console with `nxlink -s build_switch/aurore_demo.nro` (its logs come back to the terminal). Without `-DAURORE_GRAPHICS=DEKO3D`, the OpenGL driver. The presets `switch-deko3d` and `switch-gl` do the same. The `.nro` target of the demo's `CMakeLists.txt` shows how the resources go into its romfs, and why the text and button fonts are left out of it: the system's shared fonts take their place ([fonts.md](fonts.md)).

## PS Vita

With the [VitaSDK](https://vitasdk.org/) (`VITASDK` set in the environment). The CI builds in its Docker images:

```sh
# GXM, the Vita's own graphics API
docker run --rm -v $(pwd):/src/ vitasdk/vitasdk sh -c \
  "cd /src && cmake -B build -DAURORE_PLATFORM=PSV -DCMAKE_BUILD_TYPE=Release -DAURORE_GRAPHICS=GXM && cmake --build build"

# OpenGL ES 2 through SDL2 and PVR_PSP2, in an image built from psv/scripts/
docker build -t aurore-vita-gles2 psv/scripts
docker run --rm -v $(pwd):/src/ aurore-vita-gles2 \
  "cmake -B build -G Ninja -DAURORE_PLATFORM=PSV -DUSE_SYSTEM_SDL2=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build"
```

The result is `build/aurore_demo.vpk`, installed with VitaShell. The OpenGL ES build packs what is in `psv/module/` (empty in the repository: the image copies the PVR_PSP2 `.suprx` modules there, `psv/scripts/Dockerfile` shows where they come from); `-DAURORE_VITA_SHARK=ON` (GXM) compiles the shaders at run time with SceShaccCg. `psv/sce_sys/` holds the LiveArea's pictures. `-DAURORE_SIMPLE_HIGHLIGHT=ON` draws a plainer focus highlight, lighter for the Vita. Linking `Aurore::aurore` keeps 16 KiB between the text and data segments (`--defsym=__sce_headroom=0x4000`, read by VitaSDK 2026.08's linker script): `vita-elf-create` writes the module's SCE data there, and fails ("segment 1 overlaps") when the text ends just before the 64 KiB boundary the data starts at.

## PS4

With the [OpenOrbis](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain) toolchain (`OPENORBIS` set), as [PacBrew](https://github.com/PacBrew/ps4-openorbis) packages it, with its `add_self` and packaging (Linux only). PacBrew's clang and libc++ are LLVM 12's: the image built from `ps4/scripts/Dockerfile` replaces them with LLVM 23's clang and the runtimes built from its sources (libc++, libc++abi, libunwind, compiler-rt's builtins), with `libcxx-ps4.patch` for OpenOrbis' musl, so the PS4 has C++23 and its library:

```sh
docker build -t aurore-ps4 ps4/scripts
docker run --rm -v $(pwd):/src/ aurore-ps4 \
  "cmake -B build -DAURORE_PLATFORM=PS4 -DCMAKE_BUILD_TYPE=Release && make -C build -j$(nproc)"
```

The result is `build/*.pkg`; the demo's `CMakeLists.txt` copies `ps4/` (the package's `sce_sys`) and the resources into `build/ps4/`, read at `/app0/` once installed. `-DAURORE_LIBJBC=ON` asks for root access (an app that needs it).

## New 3DS, New 3DS XL, New 2DS XL

The library and a test card, drawn by citro3d on the bottom screen (the touch screen): the library lays screens out with the small screen preset (480x360 units on the 320x240 screen), and nanovg's citro3d renderer (`nanovg_citro3d.h`) draws them. The buttons are the Switch's, in the same places (A on the right, B at the bottom, L and R, ZL and ZR as LT and RT, SELECT and START as − and +); the D-pad, the Circle Pad and the C-stick move the focus, the Circle Pad is the left stick and the C-stick the right one; the bottom screen's touches are read, one finger. Text is typed on the system's keyboard (swkbd: the text one, its password mode, the number pad), which takes both screens until it closes. The Wi-Fi's strength (0 to 3 bars) and the IP address are the system's (ac, soc). The library's two threads (async tasks, image decoding) run on the New models' third core, the app's own, else on the app's core at a lower priority. No sound: Aurore has none of its own (the Switch plays the system's), and none is played. The New models only: the New 3DS, the New 3DS XL and the New 2DS XL, which share the CPU (804 MHz, the L2 cache), the memory (256 MB) and the screens' resolutions; the New 2DS XL has no stereoscopic 3D, which Aurore does not use, and is the console of reference. The platform reads the model (`CtrPlatform::getName()`, in the log and on the test card); the original 3DS, 3DS XL and 2DS run, at a third of the speed, and are not aimed at: the log says so. With [devkitPro](https://devkitpro.org/wiki/Getting_Started): the `3ds-dev` group. The CI uses the `devkitpro/devkitarm` image.

```sh
cmake -B build_3ds -DCMAKE_BUILD_TYPE=Release -DAURORE_PLATFORM=3DS
cmake --build build_3ds -j$(nproc)
```

The results are `build_3ds/aurore_demo.3dsx`, the demo, and `build_3ds/ctr/aurore_3ds_smoke.3dsx`, the test card of `ctr/`. The demo is the desktop's, split between the two screens (below); HOME leaves it. The test card is an XML layout (boxes, labels) and a view drawing each kind of paint and fill the renderer handles (a colour with a stroke, linear, radial and box gradients, a nonzero fill with a hole, an image, a scissor, text), three buttons to move the focus between and click, and a line showing what Aurore reads (the buttons held, the sticks, the touch), with the library's resources in its romfs; its top screen is the activity's top screen view: the console's model, the sizes and the last button clicked; START quits. The log goes to the terminal of `3dslink -s` when it started the app, else to `/3ds/aurore.log` on the SD card (the top screen is the app's). Either goes to `/3ds/` on the SD card, or to the console with `3dslink`. `AURORE_WINDOW` and `AURORE_GRAPHICS` stop the configuration: the window is libctru's and the renderer citro3d.

The two screens are the app's, as in the homebrews made for the console: the bottom one (the touch screen) to act, the top one (400x240) to look. The activity's content is on the bottom one; what the top one shows is the activity's too, its top screen view (`Activity::setTopScreenView()`, `aurore/core/top_screen.hpp`): the item chosen, its details. A view can give one too (`View::setTopScreenView()`), shown while it or a view inside it has the focus: a list at the bottom, the item chosen at the top (a cell's preview, or the list's own view it updates as the focus moves). The one nearest to the focus wins, then the activity's. Either is laid out at the top screen's size in the app's units (600x360 with the small screen preset: a text is as big on both screens) and drawn there only, no focus nor touch goes to it. With none, the screen shows `TopScreenHeader`: the activity's title (its AppletFrame's), the battery, the Wi-Fi and the time. The AppletFrame's header goes up there with it, giving its room to the screen below, when it shows the title alone: one with views added to it (tools, a hint view, Cadran's pill) stays below, as does a popup's, and `AppletFrame::setHeaderOnTopScreen(false)` (XML `headerOnTopScreen="false"`) keeps any below. The hints stay on the screen below, with the controls. The demo splits itself so: the tab shown, on the night with the aurora, at the top (the Welcome tab's night, name and pitch go up there, its two buttons stay below), and in the Library the game focused, its cover and details (`demo/src/view/top_screens.cpp`). On the console a frame is drawn only when something changed (`Application::setRedrawOnDemand(true)` by default there): its CPU is short for a full redraw at 60 frames a second. The platform says the screen is there (`TopScreen::setSize()`) and draws it after the app, on its own render target (`VideoContext::beginTopScreen()`); the demo's simulated New 3DS draws it in the window with `TopScreen::draw()`. Elsewhere `TopScreen::isAvailable()` is false and a top screen view is never drawn.

The renderer has no fragment shader to work with (the PICA200 has none): the paint's coordinates are computed per vertex by its vertex shader (`library/lib/extern/nanovg/citro3d/nanovg_citro3d.v.pica`, assembled by picasso and built into the library), colours are the texture combiners' constants, a linear gradient reads a shared ramp texture, and radial and box gradients read a small alpha texture of their distance field, kept in a cache. Nonzero fills go through the stencil in two passes, the scissor is the hardware's. The edges are antialiased as nanovg's GL shader does it, the coverage per pixel read from a texture instead (`nanovg_citro3d_aa.h`: 1024 places across an edge, 64 stroke widths) and multiplied in a third combiner; `tests/demo_tests.cpp` (`citro3d_coverage`) checks it against nanovg's formula. Text has its own, the glyph atlas'. Textures are padded to powers of two, at most 1024 (a larger image is scaled down).

## Android

The Gradle project in [`android-project/`](../android-project/), on SDL3's Android port, or SDL2's with `-PauroreSdl=2`:

```sh
./build_libromfs_generator.sh
cd android-project && ./gradlew assembleDebug        # or: ./gradlew assembleDebug -PauroreSdl=2
```

The APK is in `android-project/app/build/outputs/apk/debug/`. The page in `android-project/` has the SDK and NDK it needs, and how to turn it into an app of your own.

The window is fullscreen; the system bars, the display cutout and the rounded corners are the [safe area](safe-areas.md), read through `PlatformUtils.getSafeAreaInsets` (an app's own copy of `android-project/` keeps that method).

**Controllers**: a controller paired with the phone over Bluetooth (an Xbox, DualSense or Switch Pro controller, an 8BitDo…) or plugged in over USB reaches SDL through Android's input, with no permission. The two Bluetooth permissions left commented out in `AndroidManifest.xml` are for one thing only: SDL's own driver for the Steam Controller over Bluetooth LE (HIDAPI), which runs when the app sets `SDL_HINT_JOYSTICK_HIDAPI_STEAM`. An app that wants it uncomments them, and from Android 12 also asks the user for `BLUETOOTH_CONNECT` ("Nearby devices") at run time, which SDL does not do.

## iOS and tvOS

On macOS with Xcode:

```sh
./build_libromfs_generator.sh
cmake -B build -G Xcode -DAURORE_PLATFORM=IOS -DPLATFORM=OS64 -DDEPLOYMENT_TARGET=27.0
cmake --build build
```

With Xcode 27, for iOS 27 (`DEPLOYMENT_TARGET`; Xcode 27 goes down to iOS 15). The app is `build/Debug-iphoneos/aurore_demo.app`; the CI zips it unsigned. `-DIOS_CODE_SIGN_IDENTITY="Apple Development: …"` signs it, `-DIOS_GUI_IDENTIFIER=com.example.app` sets its bundle identifier (`PACKAGE_NAME` by default). `-DAURORE_WINDOW=SDL2` builds on SDL2. `library/cmake/ios.toolchain.cmake` takes the usual options of ios-cmake: `-DPLATFORM=SIMULATORARM64` builds for the simulator. `-DAURORE_PLATFORM=TVOS` builds for the Apple TV (not built by CI). The demo's `ios_bundle()` call (in its `CMakeLists.txt`) gives the app its storyboard, icons and `Info.plist` (`demo/ios/`).

The notch, the rounded corners and the home indicator (on tvOS, the TV's safe area) are the [safe area](safe-areas.md), from `UIWindow.safeAreaInsets`.

## In a browser

Aurore runs on its SDL platform under [Emscripten](https://emscripten.org/), SDL3 or SDL2 (`emcmake cmake -DAURORE_WINDOW=SDL2`): the window stays at 1280×720 (the page scales the canvas), and `aurore_web_button(button, down)`, exported to the page, presses a controller button, for an on-screen pad or keys SDL does not map. The demo builds for it under `emcmake cmake` and runs on the website, <https://aurore.boreale.dev/demo/> ([web/README.md](https://github.com/JigSawFr/Aurore/blob/main/web/README.md) says how); [PlayGuard's web simulator](https://github.com/JigSawFr/playguard/blob/main/web/README.md) is an app's complete example. In short, under `emcmake cmake`:

- set `AURORE_PLATFORM=DESKTOP`, `AURORE_WINDOW=SDL3`, `AURORE_GRAPHICS=GLES3` and `USE_SYSTEM_SDL3` before including `commonOption.cmake`, and compile with `-pthread -sUSE_SDL=3 -sUSE_ZLIB=1 -fexceptions`, SDL3 coming from Emscripten's ports (a `SDL3Config.cmake` that makes an empty `SDL3::SDL3`); for SDL2, `AURORE_WINDOW=SDL2`, `USE_SYSTEM_SDL2`, `-sUSE_SDL=2` and a `SDL2Config.cmake` that finds nothing to link (`web/cmake/` has both);
- package the resources into the page's file system (`--preload-file`) and set `AURORE_RESOURCES_ROOT` to where they land;
- run the loop with `emscripten_set_main_loop`, one `mainLoop()` per frame, since a browser needs its thread back between frames;
- start the threads with the page (`-sPTHREAD_POOL_SIZE`), which then needs the cross-origin isolation headers;
- a browser has no system font: give Chinese, Japanese and Korean a font of their own (`font/cjk.otf`, [fonts.md](fonts.md)).

## Simulated platforms

The demo shows on the desktop how it looks on a console: its screen at the console's proportions, laid out with the console's preset, the console drawn around it.

```sh
./aurore_demo --simulate 3ds      # or AURORE_SIMULATE=3ds
```

| `--simulate` | Screen | Preset | Edge antialiasing |
|---|---|---|---|
| `switch` | 1280x720 | standard | yes |
| `vita` | 960x544 | standard | yes (the console smooths with 4x MSAA instead) |
| `ps4` | 1920x1080 | standard | yes |
| `3ds` | 320x240, the top screen above it | small screen (480x360 units, dense) | yes (`nanovg_citro3d_aa.h`) |
| `phone` | 1600x720 | standard | yes |

The Home tab shows the consoles (*On every screen*, `demo::simulation::drawPreview`) and the Developers tab lists them (*Simulated platform*): choosing one while the demo runs shows it there, the screens made again for it. The consoles are Boréale's, drawn as on its sites (PlayGuard's `site.css`, `.device` and its `DEVICE_GRIPS`, the reference): a body in the night's blues with a glow of aurora under it, dark controls, the sticks' caps ringed in teal; plain shapes, without a maker's logo, name or colours. The touches and the mouse are read on the screen only, and the window is sized for the console; the New 3DS's top screen is the app's (`aurore::TopScreen`, below), drawn as the console's second target draws it. The screen is drawn at the window's resolution, not the console's pixels.

An app does the same with `aurore::ScreenFrame` (`aurore/core/screen_frame.hpp`) and `Application::setScreenFrame()`: the frame says where the screen goes in the window and draws itself around it, the app is laid out and drawn in the screen, and its input read there. `tests/demo_tests.cpp` (`simulated_consoles`) checks each console against its picture in `tests/references/`.

## SDL_GPU on the desktop (prototype)

`-DAURORE_GRAPHICS=SDLGPU` draws with SDL3's GPU API instead of OpenGL: Direct3D 12 on Windows, Metal on macOS, Vulkan on Linux (and on Windows with `SDL_GPU_DRIVER=vulkan`). It needs the SDL3 window (the default). nanovg's renderer for it is `nanovg_sdlgpu.h`, a port of the OpenGL 3 one with the same shaders and passes; `driver/sdlgpu.cpp` draws each frame into a texture of the window's size, then onto the window, and reads it back for the screenshots. Its shaders are compiled ahead into `nvg_shader/sdlgpu/sdlgpu_shaders.h`, so that building Aurore needs no shader compiler: SPIR-V by glslang, then MSL and DXIL by [SDL_shadercross](https://github.com/libsdl-org/SDL_shadercross), SDL's own translator (`tools/build_shadercross.sh` builds it at pinned versions, `tools/make_sdlgpu_shaders.py` writes the header, and CI checks it is up to date). Not there yet: mobile, and the test harness, which stays on OpenGL. `SDL_GPU_DRIVER` (`direct3d12`, `vulkan`, `metal`) picks the driver, `AURORE_GPU_DEBUG=1` turns its validation on. CI runs the demo on Mesa's software Vulkan (lavapipe) and on Windows' software Direct3D 12 (WARP), and builds it on macOS.

## Options for every platform

| Option | Effect |
| --- | --- |
| `AURORE_LIBROMFS` | Builds the resources into the program (`build_libromfs_generator.sh` first when cross-compiling) |
| `AURORE_FREETYPE`, `AURORE_FRIBIDI` | FreeType for the glyphs, FriBidi for right-to-left text ([fonts.md](fonts.md)) |
| `AURORE_INSPECTOR` | The inspector on from the start ([inspector.md](inspector.md)) |
| `AURORE_DEV_TOOLS` | `AUTO` (default), `ON` or `OFF`: the debugging aids read from the environment, left out of `Release` and `MinSizeRel` builds by default ([environment.md](environment.md)) |
| `AURORE_BUILD_TESTS` | The test harness, desktop only ([testing.md](testing.md)); with the demo, its tests (they need `AURORE_LIBROMFS`) |
| `AURORE_BUILD_FUZZERS` | With the demo's tests and clang: the libFuzzer targets of `tests/fuzz` ([testing.md](testing.md)) |
| `AURORE_UNITY_BUILD` | A unity build (faster from scratch) |
| `AURORE_SIMPLE_HIGHLIGHT` | A focus highlight without its animation |
| `AURORE_STD_THREAD`, `AURORE_BOOST_FILESYSTEM` | `std::thread` instead of pthreads; Boost's filesystem for a toolchain without `std::filesystem` |
| `AURORE_INSTALL` | Desktop: installs the library for `find_package(Aurore)` |

`library/cmake/commonOption.cmake` lists them all with their defaults. Only the `AURORE_` options are read; `commonOption.cmake` then sets the plain variables `PLATFORM_DESKTOP`, `USE_GLFW`, `USE_DEKO3D`, `USE_LIBROMFS`… from them, for an app's own tests (`if (PLATFORM_SWITCH)`).
