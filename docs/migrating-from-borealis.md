# Migrating from borealis

Aurore is borealis under a new name, and keeps its names: the `brls` namespace, `<borealis.hpp>`, the `brls:` XML tags, the `BRLS_` macros. An app on [xfangfang/borealis](https://github.com/xfangfang/borealis) (the `wiliwili` branch) or on natinusala's moves to Aurore by changing its submodule and a few lines of CMake. This page lists those lines, then what looks or behaves differently afterwards.

## 1. The submodule

Replace borealis with Aurore, at the same path or a new one (`library/aurore` below, as in Aurore's own docs):

```sh
git submodule deinit -f library/borealis
git rm -f library/borealis
rm -rf .git/modules/library/borealis
git submodule add -b main https://github.com/JigSawFr/aurore.git library/aurore
git submodule update --init --recursive
```

Pin a release rather than following `main`: `git -C library/aurore checkout vX.Y.Z`, and read its entry in [`CHANGELOG.md`](../CHANGELOG.md) before moving to the next one (before 1.0, a minor version may break the API). In code, `AURORE_VERSION_AT_LEAST(0, 2, 0)` tells which one is built.

## 2. The build

- **C++20.** Aurore and its headers need it: `set(CMAKE_CXX_STANDARD 20)` or `CXX_STANDARD 20` on the app's target. RTTI and exceptions stay on (no `-fno-rtti`, no `-fno-exceptions`).
- **The library's path.** `BOREALIS_LIBRARY` still names the `library/` folder, and the two includes stay before `project()`:

  ```cmake
  set(BOREALIS_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/library/aurore/library)
  include(${BOREALIS_LIBRARY}/cmake/commonOption.cmake)
  include(${BOREALIS_LIBRARY}/cmake/toolchain.cmake)
  project(my_app)
  add_subdirectory(${BOREALIS_LIBRARY} ${CMAKE_BINARY_DIR}/aurore)
  ```

- **The target.** The library target is `Aurore::aurore`; `borealis` is gone, without an alias. It brings its include directories, definitions and dependencies with it, so the app's own copies of them can go:

  ```cmake
  target_link_libraries(my_app PRIVATE Aurore::aurore)
  ```

  The test harness, if the app uses it, is `Aurore::test` (`-DBRLS_BUILD_TESTS=ON`, [testing.md](testing.md)).
- **The options** keep their names (`PLATFORM_*`, `USE_GLFW`, `USE_SDL2`, `USE_DEKO3D`, `USE_LIBROMFS`…), and a few are new ([platforms.md](platforms.md#options-for-every-platform)). The presets in `CMakePresets.json` are Aurore's own, for the demo.
- **The Switch** keeps devkitPro's compiler flags now (the Cortex-A57 tuning, the TLS model): an app that put them back by hand can stop. libpulsar is no longer linked: borealis' Switch sounds had been off for years, and still are.

## 3. The resources

Aurore's `resources/` has more than borealis': the library's and Cadran's strings in 15 languages (`i18n/<language>/`: `cadran.json`, `hints.json`, `a11y.json`, `themes.json`), the emoji (`img/emoji/`), the fonts (Inter, PromptFont and Sora in `font/`, Material Icons in `material/`), the theme folders (`themes/`). The app's resources go **on top of** Aurore's, at each build: copy Aurore's folder, then the app's over it (the `ASSEMBLE_RESOURCES` step of [`template/CMakeLists.txt`](../template/CMakeLists.txt) does it, and leaves the demo's own files out). A copy made once goes stale at the next update of the submodule.

## 4. The look: Cadran

Aurore draws every app in [Cadran](cadran.md)'s look, the look of PlayGuard and PlayShelf, without a line of code: an icon rail for the sidebar, cards for the lists, its dialogs and pickers, its palettes. Two choices:

- **Keep borealis' look**: `brls::Application::setCadran(false)` before `init()`. The screens look as before, with Aurore's fixes and features.
- **Take Cadran's**: fill in what it asks of the app (`ui::hooks`: the main screen and its tabs, the rail's icons, the version) and keep the user's choices (`ui::Prefs`), as [cadran.md](cadran.md) says. Each hook left empty does nothing, so it can be done screen by screen.

**Licence.** borealis is Apache 2.0; Cadran is GPLv3, and is linked into every app built with Aurore, whichever look it shows: an app built with Aurore is GPLv3 ([`NOTICE`](../NOTICE)).

## 5. What changes on screen

Most apps build and run unchanged. What to look at after the move:

| Change | What to check |
| --- | --- |
| **Yoga 3**, with standard CSS flexbox (Yoga 2's errata dropped) | Absolute children of a padded parent placed without insets, and sizes in percent, may move a little. `boxSizing` is `borderBox` by default (sizes include padding and border); `contentBox` is there for the others |
| **Fonts off the Switch**: Inter for the text, PromptFont for the buttons, instead of Nintendo's; Chinese, Japanese and Korean from the system | Text widths on the desktop and the phones ([fonts.md](fonts.md)) |
| **Theme keys**: every colour of the core views is a theme key, changed live | An app that set colours once at start-up from `getTheme()` should keep the key (`View::setThemeColor`, `@theme/…`) to follow a theme change ([themes.md](themes.md)) |
| **Translations** in 15 languages for borealis' own words, and a language switched while the app runs (`brls::loadTranslations(locale)`) | An app's strings that override borealis' own (`brls/…`) |
| **Links**: `Platform::openBrowser` opens `http` and `https` links only, without a shell | An app that opened other schemes or local files through it |
| **Pictures** larger than 4096×4096 pixels (or 16384 on a side) are refused before they are decoded | Large backgrounds; scale them down beforehand |
| **Focus**: a hidden box gives no focus, a destroyed view is forgotten as the focus | Screens that relied on focusing a hidden view |

The README's [*Every change and its API*](../README.md#-what-aurore-adds) lists the rest: new views, styles per state, the inspector, hot reload, the test harness.

## 6. Afterwards

- Turn on the debugging aids while checking the screens: `BRLS_INSPECTOR=1` and `BRLS_HOT_RELOAD=$PWD/resources` ([environment.md](environment.md)).
- [architecture.md](architecture.md) explains the loop, the focus, ownership and threads; the API reference is built with `doxygen docs/Doxyfile`.
- Issues and fixes go to [JigSawFr/aurore](https://github.com/JigSawFr/aurore); a fix that belongs upstream is welcome there too.
