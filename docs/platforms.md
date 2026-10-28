# Platforms

How to build Aurore's demo, and an app on Aurore, for each platform. The configure and build commands are those the CI runs (`.github/workflows/ci.yaml`, where the toolchains are pinned), but for a browser, which it does not build; its artifacts are the demo built for each platform. Getting the toolchains themselves is each SDK's business: the links say where.

An app builds the same way as the demo: its `CMakeLists.txt` includes `library/aurore/library/cmake/commonOption.cmake` and `toolchain.cmake` before `project()`, then `add_subdirectory(library/aurore/library)` and links `Aurore::aurore` (see the [README](../README.md#-use-it-in-your-project) and [`template/`](../template/README.md)). The platform is chosen with `-DAURORE_PLATFORM=…` (borealis' `-DPLATFORM_…=ON` still works, with a deprecation warning), and the packaging (an `.nro`, a `.vpk`, a `.pkg`, an app bundle) is the app's own: the demo's `CMakeLists.txt` has each one, to copy. [porting.md](porting.md) has what an app's code should mind on each.

| Platform | `AURORE_PLATFORM` | Window and input (`AURORE_WINDOW`) | Drawing (`AURORE_GRAPHICS`) | Resources |
| --- | --- | --- | --- | --- |
| Linux, macOS, Windows | `DESKTOP` | GLFW (default) or SDL2 | OpenGL 3.2; or `GL2`, `GLES2`, `GLES3`; Direct3D 11 on Windows (`D3D11`) | files next to the program, or built in (`AURORE_LIBROMFS`) |
| Nintendo Switch | `SWITCH` | GLFW (default) or SDL2 | deko3d (`DEKO3D`) or OpenGL 4.3 | romfs, in the `.nro` |
| PS Vita | `PSV` | SDL2, or the system's | GXM (`GXM`) or OpenGL ES 2 | in the `.vpk` |
| PS4 | `PS4` | SDL2 | OpenGL ES 2 | in the `.pkg` |
| Android | `ANDROID` | SDL2 | OpenGL ES 2 (or `GLES3`) | built in (libromfs, forced) |
| iOS, tvOS | `IOS`, `TVOS` | SDL2 | OpenGL ES 3 (or `GLES2`) | built in (libromfs, forced) |
| Browser | Emscripten (`emcmake`) | SDL2 | OpenGL ES 3 (WebGL 2) | packaged with the page |

Every build needs CMake 3.16 or later and a C++20 compiler. Clone with the submodules (`git clone --recursive`, or `git submodule update --init --recursive`).

## Linux

On Debian or Ubuntu:

```sh
sudo apt install build-essential cmake ninja-build git libdbus-1-dev libxkbcommon-dev \
  libwayland-dev wayland-protocols xorg-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release -DAURORE_PLATFORM=DESKTOP -DAURORE_LIBROMFS=ON
cmake --build build
./build/aurore_demo
```

`-DAURORE_WINDOW=SDL2` builds with SDL2 instead of GLFW; `-DAURORE_GRAPHICS=GL2`, `-DAURORE_GRAPHICS=GLES2` or `-DAURORE_GRAPHICS=GLES3` another OpenGL. Without `AURORE_LIBROMFS`, the `resources/` folder is copied next to the program at each build. The presets do the same in fewer words: `cmake --preset desktop-glfw` (or `desktop-sdl`), then `cmake --build --preset desktop-glfw`.

The tests need EGL and Mesa as well (`libegl-dev libegl1 libegl-mesa0 libgl1-mesa-dri`), then `-DBRLS_BUILD_TESTS=ON` and `ctest --test-dir build` ([testing.md](testing.md)).

## macOS

With Xcode's command-line tools and CMake (Homebrew's, for instance):

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DAURORE_PLATFORM=DESKTOP -DBUNDLE_MACOS_APP=ON
make -C build -j$(sysctl -n hw.ncpu)
open build/aurore_demo.app
```

The build is universal (arm64 and x86_64) and runs from macOS 10.15 (11 on Apple silicon). `-DBUNDLE_MACOS_APP=ON` makes an `.app` with the resources inside; without it, a plain program with `resources/` next to it. `-DAURORE_WINDOW=SDL2` builds with SDL2.

## Windows

With [MSYS2](https://www.msys2.org/), in its *MINGW64* shell:

```sh
pacman -S --needed git mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-cc
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DAURORE_PLATFORM=DESKTOP \
  -DAURORE_WIN32_TERMINAL=OFF -DAURORE_LIBROMFS=ON -DZLIB_USE_STATIC_LIBS=ON
cmake --build build
./build/aurore_demo.exe
```

`-DAURORE_GRAPHICS=D3D11` draws with Direct3D 11 instead of OpenGL. `-DAURORE_WIN32_TERMINAL=OFF` makes a program without a console window (`ON` keeps one, for the logs). The demo links the C and C++ runtimes statically (`-static` in its `CMakeLists.txt`): it runs without MSYS2's DLLs. `library/cmake/MinGWCross.cmake` is a toolchain file for building it from Linux with MinGW-w64 (not built by CI).

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

# OpenGL ES 2 through SDL2 and PVR_PSP2
docker run --rm -v $(pwd):/src/ xfangfang/vitasdk_sdl2_gles2 \
  "cmake -B build -G Ninja -DAURORE_PLATFORM=PSV -DUSE_SYSTEM_SDL2=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build"
```

The result is `build/aurore_demo.vpk`, installed with VitaShell. The OpenGL ES build packs what is in `psv/module/` (empty in the repository: the PVR_PSP2 `.suprx` modules go there, `psv/scripts/Dockerfile` shows where they come from); `-DAURORE_VITA_SHARK=ON` (GXM) compiles the shaders at run time with SceShaccCg. `psv/sce_sys/` holds the LiveArea's pictures. `-DAURORE_SIMPLE_HIGHLIGHT=ON` draws a plainer focus highlight, lighter for the Vita.

## PS4

With the [OpenOrbis](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain) toolchain (`OPENORBIS` set), through pacbrew's Docker image, which has its `add_self` and packaging (Linux only):

```sh
docker run --rm -v $(pwd):/src/ xfangfang/pacbrew \
  "cmake -B build -DAURORE_PLATFORM=PS4 -DCMAKE_BUILD_TYPE=Release && make -C build -j$(nproc)"
```

The result is `build/*.pkg`; the demo's `CMakeLists.txt` copies `ps4/` (the package's `sce_sys`) and the resources into `build/ps4/`, read at `/app0/` once installed. `-DAURORE_LIBJBC=ON` asks for root access (an app that needs it).

## Android

The Gradle project in [`android-project/`](../android-project/), on SDL2's Android port:

```sh
./build_libromfs_generator.sh
cd android-project && ./gradlew assembleDebug
```

The APK is in `android-project/app/build/outputs/apk/debug/`. The page in `android-project/` has the SDK and NDK it needs, and how to turn it into an app of your own.

The window is fullscreen; the system bars, the display cutout and the rounded corners are the [safe area](safe-areas.md), read through `PlatformUtils.getSafeAreaInsets` (an app's own copy of `android-project/` keeps that method).

## iOS and tvOS

On macOS with Xcode:

```sh
./build_libromfs_generator.sh
cmake -B build -G Xcode -DAURORE_PLATFORM=IOS -DPLATFORM=OS64 -DDEPLOYMENT_TARGET=13.0
cmake --build build
```

The app is `build/Debug-iphoneos/aurore_demo.app`; the CI zips it unsigned. `-DIOS_CODE_SIGN_IDENTITY="Apple Development: …"` signs it, `-DIOS_GUI_IDENTIFIER=com.example.app` sets its bundle identifier (`PACKAGE_NAME` by default). `library/cmake/ios.toolchain.cmake` takes the usual options of ios-cmake: `-DPLATFORM=SIMULATORARM64` builds for the simulator. `-DAURORE_PLATFORM=TVOS` builds for the Apple TV (not built by CI). The demo's `ios_bundle()` call (in its `CMakeLists.txt`) gives the app its storyboard, icons and `Info.plist` (`demo/ios/`).

The notch, the rounded corners and the home indicator (on tvOS, the TV's safe area) are the [safe area](safe-areas.md), from `UIWindow.safeAreaInsets`.

## In a browser

Aurore runs on its SDL2 platform under [Emscripten](https://emscripten.org/): the window stays at 1280×720 (the page scales the canvas), and `brls_web_button(button, down)`, exported to the page, presses a controller button, for an on-screen pad or keys SDL does not map. Aurore's own CI does not build the demo for the web; [PlayGuard's web simulator](https://github.com/JigSawFr/playguard/blob/main/web/README.md) is a complete example. In short, under `emcmake cmake`:

- set `AURORE_PLATFORM=DESKTOP`, `AURORE_WINDOW=SDL2`, `AURORE_GRAPHICS=GLES3` and `USE_SYSTEM_SDL2` before including `commonOption.cmake`, and compile with `-pthread -sUSE_SDL=2 -sUSE_ZLIB=1 -fexceptions`, SDL2 coming from Emscripten's ports (a `SDL2Config.cmake` that finds nothing to link);
- package the resources into the page's file system (`--preload-file`) and set `BRLS_RESOURCES_DIR` to where they land;
- run the loop with `emscripten_set_main_loop`, one `mainLoop()` per frame, since a browser needs its thread back between frames;
- start the threads with the page (`-sPTHREAD_POOL_SIZE`), which then needs the cross-origin isolation headers;
- a browser has no system font: give Chinese, Japanese and Korean a font of their own (`font/cjk.otf`, [fonts.md](fonts.md)).

## Options for every platform

| Option | Effect |
| --- | --- |
| `AURORE_LIBROMFS` | Builds the resources into the program (`build_libromfs_generator.sh` first when cross-compiling) |
| `AURORE_FREETYPE`, `AURORE_FRIBIDI` | FreeType for the glyphs, FriBidi for right-to-left text ([fonts.md](fonts.md)) |
| `BRLS_INSPECTOR` | The inspector on from the start ([inspector.md](inspector.md)) |
| `BRLS_DEV_TOOLS` | `AUTO` (default), `ON` or `OFF`: the debugging aids read from the environment, left out of `Release` and `MinSizeRel` builds by default ([environment.md](environment.md)) |
| `BRLS_BUILD_TESTS` | The test harness, desktop only ([testing.md](testing.md)); with the demo, its tests (they need `AURORE_LIBROMFS`) |
| `BRLS_BUILD_FUZZERS` | With the demo's tests and clang: the libFuzzer targets of `tests/fuzz` ([testing.md](testing.md)) |
| `BRLS_UNITY_BUILD` | A unity build (faster from scratch) |
| `AURORE_SIMPLE_HIGHLIGHT` | A focus highlight without its animation |
| `AURORE_STD_THREAD`, `AURORE_BOOST_FILESYSTEM` | `std::thread` instead of pthreads; Boost's filesystem for a toolchain without `std::filesystem` |
| `AURORE_INSTALL` | Desktop: installs the library for `find_package(Aurore)` |

`library/cmake/commonOption.cmake` lists them all with their defaults. borealis' names (`PLATFORM_DESKTOP`, `USE_GLFW`, `USE_DEKO3D`, `USE_LIBROMFS`…) still work, with a deprecation warning, and are set as plain variables after `commonOption.cmake` for an app's own tests (`if (PLATFORM_SWITCH)`).
