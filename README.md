<div align="center">

<img src=".github/readme/banner.png" alt="Aurore" width="100%">

### A Switch-like interface for your app, on every screen

**Android · iOS · Windows · macOS · Linux · PS4 · PS&nbsp;Vita · Nintendo&nbsp;Switch**

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](#-use-it-in-your-project)
[![upstream: xfangfang/borealis](https://img.shields.io/badge/upstream-xfangfang%2Fborealis-555.svg)](https://github.com/xfangfang/borealis)
[![license: GPLv3 · Apache 2.0](https://img.shields.io/badge/license-GPLv3%20%C2%B7%20Apache%202.0-blue.svg)](#-license-and-credits)

[**What's new**](#-what-aurore-adds) ·
[**Use&nbsp;it**](#-use-it-in-your-project) ·
[**Build&nbsp;the&nbsp;demo**](#-build-the-demo) ·
[**Upstream&nbsp;wiki**](https://github.com/xfangfang/borealis/wiki)

<img src=".github/readme/tour.gif" alt="The Aurore demo" width="100%">

</div>

---

Aurore is a C++ UI library for controllers, TVs and touch screens. It looks like the Nintendo Switch's own menus out of the box, and can be restyled into anything else.

It is [borealis](https://github.com/natinusala/borealis) under a new name: a fork of [xfangfang/borealis](https://github.com/xfangfang/borealis) (the `wiliwili` branch), maintained by [JigSawFr](https://github.com/JigSawFr) for [PlayGuard](https://github.com/JigSawFr/playguard) and [PlayShelf](https://github.com/JigSawFr/playshelf). It follows upstream and adds **fixes and features picked from other forks** and **the changes those apps need**, each one opt-in unless it is a fix. The code keeps borealis' names (`brls`, `borealis.hpp`), so a borealis app moves to Aurore by changing its submodule.

> [!IMPORTANT]
> **[Cadran](docs/cadran.md)**, the look of PlayGuard and PlayShelf (an icon rail, cards, modals, action sheets, vector icons, 15 languages, language and theme switched while the app runs), is Aurore's default look: an app gets it without a line of code, and `brls::Application::setCadran(false)` keeps borealis' own. Cadran is **GPLv3**: an app built with Aurore is GPLv3.

## ✨ Features

- 🎮 **Made for controllers** — focus moves on its own with the D-pad, touch and mouse work too.
- 🧱 **Flexbox layout** — every view is laid out by [Yoga](https://github.com/facebook/yoga), described in XML, code only where it matters.
- 🖌️ **Vector graphics** — hardware-accelerated by [nanovg](https://github.com/memononen/nanovg) (OpenGL, Direct3D 11, deko3d, GXM), scaled for a TV.
- 🧩 **Ready-made components** — tabs, lists that recycle their cells, settings cells, dialogs, toasts, sliders, images, all restyled by a theme.
- 🎨 **Themes** — every colour is a theme key; theme files and folders bring palettes, background pictures, logos, icons and decorations, switched while the app runs ([docs/themes.md](docs/themes.md)). Presets ship with it: high contrast, Ocean, Amethyst, Sand, one per season and feast, and *Seasonal*, which follows the calendar.
- 🌍 **15 languages** — one JSON catalogue per language, switched while the app runs; colour emoji and right-to-left text as options.
- 🧪 **Tools** — a test harness with simulated time and screenshot comparison, an inspector (F12), hot reload of XML and themes, a profiler and an app template.
- 🧰 **The toolbox** — logger, animations, timers, background tasks, storage.

<details>
<summary><b>📸 Screenshots</b></summary>
<br>

<p align="center">
  <img src=".github/readme/cadran.png" alt="Cadran's pieces" width="49%">
  <img src=".github/readme/sheet.png" alt="An action sheet" width="49%">
  <br><sub><b>Cadran's pieces</b> · <b>An action sheet</b></sub>
</p>

<p align="center">
  <img src=".github/readme/picker.png" alt="A list of values" width="49%">
  <img src=".github/readme/checklist.png" alt="A checklist" width="49%">
  <br><sub><b>A list of values</b> · <b>A checklist</b></sub>
</p>

<p align="center">
  <img src=".github/readme/markdown.png" alt="Icons and Markdown" width="49%">
  <img src=".github/readme/components.png" alt="The demo's other tabs in Cadran" width="49%">
  <br><sub><b>Icons and Markdown</b> · <b>The demo's other tabs in Cadran</b></sub>
</p>

<p align="center">
  <img src=".github/readme/winter.png" alt="A theme folder: Winter" width="49%">
  <img src=".github/readme/prism.png" alt="A theme folder: Prism" width="49%">
  <br><sub><b>A theme folder with decorations: Winter</b> · <b>A theme with its own logo and icons: Prism</b></sub>
</p>

<p align="center">
  <img src=".github/readme/aurora.png" alt="A background picture: Aurora" width="49%">
  <img src=".github/readme/palettes.png" alt="The palette picker" width="49%">
  <br><sub><b>A background picture: Aurora</b> · <b>The palette picker</b></sub>
</p>

<p align="center">
  <img src=".github/readme/light.png" alt="The light theme" width="49%">
  <img src=".github/readme/french.png" alt="In French, switched while the app runs" width="49%">
  <br><sub><b>The light theme</b> · <b>In French, switched while the app runs</b></sub>
</p>

</details>

## 🌟 What Aurore adds

- 🎨 **Themes** — theme files and folders: palettes from one accent, presets, background pictures, logos, icons, app image slots, backdrops, decor strips and gentle particles, all switched live, with contrast checked.
- ✨ **Styles** — per-state styles (focus, pressed, disabled, hover), transitions, data binding and accessibility labels.
- 🧩 **Views** — a recycling grid and row, context menus, icon buttons, Markdown.
- 🧪 **Tools** — test harness, inspector, hot reload, profiler, template.
- 📐 **Layout** — Yoga 3; `gap`, `rowGap`, `columnGap`; `flexWrap` with focus across the lines; `boxSizing`, `positionType="static"`; a radius per corner; horizontal gradients.
- 🔤 **Text** — a font scale for every label, shrink-to-fit, `isTruncated()`, clean line breaks.
- 🌍 **Languages** — switch language while the app runs, with lookups safe from any thread.
- 🎯 **Focus and hints** — no focus lost in hidden or destroyed views; optional and steady footer hints.
- 🖥️ **Platforms** — controllers reconnected under SDL, the browser (Emscripten), the taskbar icon on Linux, deko3d's command memory on the Switch.
- ⚡ **Speed** — no relayout for an unchanged text or size, XML attributes registered on demand, unused textures freed.

<details>
<summary><b>Every change and its API</b></summary>
<br>

**Themes** ([docs/themes.md](docs/themes.md))

| Change | API |
| --- | --- |
| Every colour by theme key, switched live without rebuilding the screens | `View::setThemeColor`, XML `@theme/…`, `View::onThemeChanged`, `Application::getThemeChangedEvent` |
| Theme files: metadata, `inherit`, `[light]`/`[dark]`, a palette from one accent, `@include` | `Theme::loadFile`, `Theme::inspect`, `Theme::listThemes` |
| Presets: high contrast, Ocean, Amethyst, Sand, nine seasons and feasts, *Seasonal* | `builtin:<id>` · `Theme::seasonalTheme`, `Theme::followCalendar` |
| Contrast checked for every theme (WCAG 4.5:1 and 3:1) | `Theme::addContrastCheck` · `aurore_demo --check-themes` |
| Theme folders with their pictures, kept inside the folder | `<id>/theme.theme` · `Theme::resolveAsset` |
| Background pictures, their fit, a veil, translucent surfaces | `background_image`, `background_fit`, `background_dim`, `surface_opacity` · `Theme::addSurfaceKey` |
| Logo, icons, app image slots, a thumbnail | `logo`, `icon.<name>`, `image.<slot>`, `preview` · `Theme::addImageSlot`, `Image::setThemeImage` |
| Backdrops, decor strips, particles (still with reduced motion) | `backdrop.*`, `decor.*`, `particles*` · `View::setThemeArea` |
| Cadran's palette picker, each palette with a swatch and a motif | `ui::pick_palette` · `icon =` |

**Styles, views and tools**

| Change | API |
| --- | --- |
| Styles per state, transitions, data binding, accessibility | [docs/styles.md](docs/styles.md) |
| Recycling grid and row, context menu, icon button | [docs/components.md](docs/components.md) |
| A test harness: no window, simulated time, inputs by call, screenshots compared | `-DBRLS_BUILD_TESTS=ON` · [docs/testing.md](docs/testing.md) |
| An inspector drawn over the app, on every platform | F12 or L3+R3 · [docs/inspector.md](docs/inspector.md) |
| Hot reload of the XML and the theme on the desktop | [docs/hot_reload.md](docs/hot_reload.md) |
| Frame profiler | `BRLS_PROFILE=1` |

**Layout and drawing**

| Change | API |
| --- | --- |
| Gaps between a box's children | `gap`, `rowGap`, `columnGap` (XML) · `Box::setGap`, `setRowGap`, `setColumnGap` |
| Wrapping boxes, focus moving across their lines | `flexWrap="wrap"` · `Box::setWrap` |
| Yoga 3.2, standard CSS flexbox | `Application::getYogaConfig()` |
| Width and height measured with or without padding and border | `boxSizing="borderBox"` (default) or `"contentBox"` · `View::setBoxSizing` |
| A view whose position offsets are ignored and that absolute children skip to find their reference | `positionType="static"` · `PositionType::STATIC` |
| A radius for each corner | `cornerTopLeftRadius`… (XML) · `View::setCornerRadius(tl, tr, br, bl)` |
| Horizontal gradient background, gradients from code | `background="horizontal_linear"` · `View::setBackgroundLinearGradient` |
| `imageAlign` honoured for `FIT` and `FILL` | — |
| Pictures larger than 4096×4096 pixels (or 16384 on a side) refused from their header, before decoding; PNG and JPEG only | `NVG_MAX_IMAGE_PIXELS`, `nvgDecodeImage`, `nvgDecodeImageMem` |
| A spinner's own colour | `ProgressSpinner::setColor` |
| Toasts styled by the theme, with an optional icon | `brls/notification/background`, `…/text`, `…/corner_radius`, `…/spacing`, `…/margin_top`, `…/margin_right` · `Application::notify(text, leading)` |
| The footer's clock in hours and minutes | — |

**Text**

| Change | API |
| --- | --- |
| A font scale for every label, a cap per label, shrink-to-fit on one line | `Label::setFontScale`, `setFontScaled`, `setFontScaleMax`, `setShrinkToFit` · `setHintsFontScaleMax` |
| A label tells when it is cut with "…" | `Label::isTruncated` |
| A wrapped line no longer starts with a space; a no-break space keeps words together | — |
| The default font looked up for each new label | — |
| Free fonts off the Switch (Inter, PromptFont's buttons at the Switch's code points); Chinese, Japanese and Korean from the system (fontconfig, CoreText…) or the app's `font/cjk` | [docs/fonts.md](docs/fonts.md) |
| FreeType instead of stb_truetype, hinted (an option) | `-DAURORE_FREETYPE=ON`: the system's FreeType on desktop, `switch-freetype` on Switch |
| stb_truetype: a glyph read within its own data, a font's tables within the font (CVE-2026-18497); fonts still from trusted sources | — |
| Colour emoji (CBDT, sbix and COLR fonts) in any label, with FreeType | `FontLoader::USER_EMOJI_PATH`; on desktop, the system's emoji font when it is empty |
| Hebrew and Arabic in reading order, Arabic letters joined (an option) | `-DAURORE_FRIBIDI=ON` · `brls::bidiVisual()` |

**Languages**

| Change | API |
| --- | --- |
| Load another language while the app runs | `brls::loadTranslations(locale)` |
| Lookups safe from other threads while the language changes | — |
| Lookups without exceptions (about 6× faster in English) | — |

**Focus, hints and input**

| Change | API |
| --- | --- |
| A hidden box gives no focus; navigation skips hidden scrolling frames and sliders | — |
| A destroyed view is forgotten as the focus; an empty focus is checked everywhere | — |
| A lost child focus reaches the parent scrolling frames | — |
| Optional footer hints, left out when they do not all fit | `setHintsOptionalOwner(view)` |
| Hints drawn steady while a screen fades | `setHintsSteady(true)` |
| The highlight's glow held still, for stable screenshots | `setHighlightAnimationStill(true)` |
| An extra unblock no longer wraps the input's counter | — |
| A view's id and a button's state read back | `View::getId` · `Button::getState` |
| A callback when a dialog closes | `Dialog::setCloseCallback` |

**Platforms**

| Change | API |
| --- | --- |
| SDL: the right controller closed when one is unplugged, every controller read after a reconnection | — |
| SDL in a browser (Emscripten), with an on-screen pad | `brls_web_button()` |
| GLFW: Wayland `app_id` and X11 `WM_CLASS`, for the taskbar's icon | `Application::setAppId`, before `createWindow()` |
| deko3d: the command memory grows instead of aborting; image descriptors reused | — |
| Switch: the applet hook removed first at shutdown | — |
| `openBrowser`: http and https links only, handed to `xdg-open`/`open` as one argument (no shell), one line on the Steam Deck pipe | `Platform::isBrowserUrl`, `Platform::BROWSER_URL_SCHEMES` |
| Logger: flushed after every line | — |

**Speed**

| Change | Gain |
| --- | --- |
| The same text or size set again is not laid out again | a refresh: 2.1 → 0.13 ms |
| XML attribute handlers registered the first time one is used | 200 labels built in code: 3–6 → under 1 ms |
| Cached textures freed once nothing uses them | — |
| Views queued while the deletion pool drains kept; each recycling list checks its own width | — |
| Text measures kept per text and style (`nvgTextBounds`, `nvgTextBoxBounds`) | a measure: 2.1 → 0.18 µs; 95% of the demo's answered from the cache |

Each commit says what it changes, why, and where it comes from: `git log 5f08b28..`.

</details>

## 📦 Use it in your project

1. Build your project as **C++20**, without `-fno-rtti` or `-fno-exceptions`.
2. Add Aurore as a submodule:

   ```sh
   git submodule add -b main https://github.com/JigSawFr/aurore.git library/aurore
   git submodule update --init --recursive
   ```

3. Copy its `resources` folder to the root of your project.
4. In your `CMakeLists.txt`, include `library/aurore/library/cmake/commonOption.cmake` and `toolchain.cmake` before `project()`, `add_subdirectory(library/aurore/library)` after it, and link `Aurore::aurore`. It brings its include directories, definitions and dependencies with it.

   The build options are prefixed: `AURORE_PLATFORM` (`DESKTOP`, `SWITCH`, `PSV`, `PS4`, `ANDROID`, `IOS`, `TVOS`), `AURORE_WINDOW` (`GLFW`, `SDL2`), `AURORE_GRAPHICS` (`GL3`, `GL2`, `GLES2`, `GLES3`, `DEKO3D`, `D3D11`, `GXM`), `AURORE_LIBROMFS`, `AURORE_FREETYPE`, `AURORE_FRIBIDI`… ([`commonOption.cmake`](library/cmake/commonOption.cmake) lists them). borealis' names (`PLATFORM_DESKTOP`, `USE_GLFW`, `USE_DEKO3D`…) still work, with a deprecation warning, and stay set as plain variables for the app's own tests.

   On the desktop, `-DAURORE_INSTALL=ON` (with `AURORE_LIBROMFS=OFF`) installs the library instead, for `find_package(Aurore)` and `Aurore::aurore`.

5. Pick a version: the submodule can follow a release tag (`vX.Y.Z`), and [`CHANGELOG.md`](CHANGELOG.md) says what each one changes. Before 1.0 a minor version may break the API. In code, `AURORE_VERSION` and `AURORE_VERSION_AT_LEAST(0, 2, 0)` come with `borealis.hpp`; an installed Aurore answers `find_package(Aurore 0.1)`.

> [!TIP]
> Starting from scratch? Copy [**`template/`**](template/README.md): a minimal app (one XML screen, its strings, an icon) whose CMake builds a desktop binary and a Switch `.nro` with Aurore as a submodule. Upstream's [template](https://github.com/xfangfang/borealis_template) has the build files for the other platforms.

## 🛠️ Build the demo

On Windows, macOS or Linux:

```sh
git clone --recursive https://github.com/JigSawFr/aurore.git
cd aurore
cmake -B build_pc -DAURORE_PLATFORM=DESKTOP
cmake --build build_pc
./build_pc/aurore_demo
```

Aurore builds with CMake only (3.16 or later): upstream's xmake files (`xmake.lua`, `uwp.lua`), untested and behind the CMake build, are gone.

With CMake 3.25 or later and Ninja, the presets in `CMakePresets.json` do the same for the usual targets, in `build/<preset>` (`cmake --list-presets`: also `desktop-debug`, `desktop-asan` and `tests`):

```sh
cmake --preset desktop-glfw        # or desktop-sdl
cmake --build --preset desktop-glfw
./build/desktop-glfw/aurore_demo
cmake --workflow --preset tests    # configure, build and run the tests

export DEVKITPRO=/opt/devkitpro    # Switch: switch-deko3d or switch-gl (OpenGL)
cmake --preset switch-deko3d
cmake --build --preset switch-deko3d   # build/switch-deko3d/aurore_demo.nro
```

With the resources built in (`AURORE_LIBROMFS`), `AURORE_RESOURCES_DIR` makes the app read a file from that folder first, falling back to the built-in copy: `AURORE_RESOURCES_DIR=resources ./build/desktop-glfw/aurore_demo` picks up changed XML, images or translations without a rebuild, only a restart.

New to it? [**Aurore in 15 minutes**](docs/getting-started.md) goes from the demo to a screen of your own on the Switch. `./build_pc/aurore_demo -t winter` (or `aurora`, `prism`, `builtin:halloween`…) starts it with a theme. `doxygen docs/Doxyfile` builds the API reference, and two debugging aids are a switch away: the [inspector](docs/inspector.md) (F12) and [hot reload](docs/hot_reload.md) of the XML and the theme.

For the other platforms, upstream's wiki has the toolchains and the commands:

- 🖥️ [Windows, macOS, Linux](https://github.com/xfangfang/borealis/wiki/Windows-macOS-Linux)
- 🎮 [Nintendo Switch](https://github.com/xfangfang/borealis/wiki/NintendoSwitch) · [PS4](https://github.com/xfangfang/borealis/wiki/PS4) · [PS Vita](https://github.com/xfangfang/borealis/wiki/PS-Vita)
- 📱 [Android](https://github.com/xfangfang/borealis/wiki/Android) · [iOS](https://github.com/xfangfang/borealis/wiki/iOS)

## 📚 Documentation

| Page | |
| --- | --- |
| [Aurore in 15 minutes](docs/getting-started.md) | From the demo to a screen of your own on the Switch |
| [Themes](docs/themes.md) | Theme files and folders, presets, pictures, decorations, keys |
| [Cadran](docs/cadran.md) | The default look and its kit |
| [Styles](docs/styles.md) | State styles, transitions, data binding, accessibility |
| [Components](docs/components.md) | The views Aurore adds |
| [Testing](docs/testing.md) | The test harness |
| [Fonts](docs/fonts.md) | The fonts on each platform, the buttons' code points, Chinese, Japanese and Korean |
| [Inspector](docs/inspector.md) · [Hot reload](docs/hot_reload.md) | Debugging aids |
| [Overlay](docs/overlay.md) | Design note on a Switch overlay mode (not built) |

## 🕹️ Made with borealis

| Project | |
| --- | --- |
| [Moonlight-Switch](https://github.com/XITRIX/Moonlight-Switch) | Moonlight game streaming for Nintendo Switch |
| [PlayGuard](https://github.com/JigSawFr/playguard) | Parental controls for Nintendo Switch, on the console |
| [PlayShelf](https://github.com/JigSawFr/playshelf) | A game library, catalogue and downloads for Nintendo Switch |
| [SimpleModDownloader](https://github.com/PoloNX/SimpleModDownloader) | GameBanana mods, downloaded on the Switch |
| [switchfin](https://github.com/dragonflylee/switchfin) | Jellyfin client for PC, PS4 and Nintendo Switch |
| [UpcomingSwitchGames](https://github.com/PoloNX/UpcomingSwitchGames) | The upcoming Switch games |
| [wiliwili](https://github.com/xfangfang/wiliwili) | bilibili client for PC, PS4, PS Vita, Xbox and Nintendo Switch |

## 🤝 Contributing

[`CONTRIBUTING.md`](CONTRIBUTING.md) has the build, the tests, the format and the rules for pull requests; [`SECURITY.md`](SECURITY.md) says how to report a vulnerability privately. The default branch is `main`; upstream's is `wiliwili`. Picks from upstream and other forks keep their author and their `cherry picked from` line; commits follow [Conventional Commits](https://www.conventionalcommits.org/).

## 📜 License and credits

borealis' code is **Apache 2.0** ([`LICENSE`](LICENSE)); Cadran is **GPLv3** ([`LICENSE.GPL`](LICENSE.GPL)), and so is an app built with it. [`NOTICE`](NOTICE) says which is which, and [`resources/THIRD_PARTY_NOTICES.md`](resources/THIRD_PARTY_NOTICES.md) lists what is bundled (the libraries in `library/lib/extern`, Inter, PromptFont, Sora, Material Icons, Twemoji) with its licence. Maintained by [JigSawFr](https://github.com/JigSawFr).

<details>
<summary>Credits</summary>
<br>

- borealis was created by [natinusala](https://github.com/natinusala/borealis) and its [contributors](AUTHORS); carried on by [xfangfang](https://github.com/xfangfang/borealis) and [dragonflylee](https://github.com/dragonflylee).
- Changes picked from the forks of Nika0000, Kelvin Wong, kawaii-flesh, Alexander Kiselev, youngzyl, Elias da Rosa, XITRIX, CostelaBR, PalindromicBreadLoaf and xlanor, each commit keeping its author.
- Built on [nanovg](https://github.com/memononen/nanovg), [Yoga](https://github.com/facebook/yoga), [GLFW](https://www.glfw.org/), [SDL](https://www.libsdl.org/), [tinyxml2](https://github.com/leethomason/tinyxml2), [fmt](https://github.com/fmtlib/fmt), [nlohmann/json](https://github.com/nlohmann/json), [tweeny](https://github.com/mobius3/tweeny) and the others in `library/lib/extern`, under their own licences.
- The text font is [Inter](https://rsms.me/inter/) and the controller buttons come from [PromptFont](https://shinmera.com/promptfont) by Shinmera (Yukari Hafner), both under the SIL Open Font License.
- The demo's logo by @MeganRoshelle; Aurore's icon and banner in [`.github/branding`](.github/branding).

</details>

<sub>Aurore is not affiliated with or endorsed by Nintendo. Nintendo Switch is a trademark of Nintendo.</sub>
