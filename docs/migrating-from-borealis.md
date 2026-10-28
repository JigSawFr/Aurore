# Migrating from borealis

Aurore is borealis under a new name. An app on [xfangfang/borealis](https://github.com/xfangfang/borealis) (the `wiliwili` branch) or on natinusala's moves to Aurore by changing its submodule and a few lines of CMake, and by renaming what it took from borealis (the `brls` namespace, `<borealis.hpp>`, the `brls:` XML tags, the `BRLS_` macros): Aurore keeps none of borealis' names, and a search and replace does it (section 6, *The names*). This page lists those lines, then what looks or behaves differently afterwards.

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
- **The library's path.** The app's variable for the `library/` folder (`BOREALIS_LIBRARY` in borealis' template, any name will do) moves to the new path, and the two includes stay before `project()`:

  ```cmake
  set(AURORE_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/library/aurore/library)
  include(${AURORE_LIBRARY}/cmake/commonOption.cmake)
  include(${AURORE_LIBRARY}/cmake/toolchain.cmake)
  project(my_app)
  add_subdirectory(${AURORE_LIBRARY} ${CMAKE_BINARY_DIR}/aurore)
  ```

- **The target.** The library target is `Aurore::aurore`; `borealis` is gone, without an alias. It brings its include directories, definitions and dependencies with it, so the app's own copies of them can go:

  ```cmake
  target_link_libraries(my_app PRIVATE Aurore::aurore)
  ```

  The test harness, if the app uses it, is `Aurore::test` (`-DAURORE_BUILD_TESTS=ON`, [testing.md](testing.md)), its cases registered with `aurore_add_tests()`.
- **The options** are prefixed: `AURORE_PLATFORM`, `AURORE_WINDOW`, `AURORE_GRAPHICS`, `AURORE_LIBROMFS`, `AURORE_BUILD_TESTS`, `AURORE_DEV_TOOLS`… ([platforms.md](platforms.md#options-for-every-platform)). borealis' names are no longer read: `PLATFORM_DESKTOP=ON` is `AURORE_PLATFORM=DESKTOP`, `USE_SDL2=ON` is `AURORE_WINDOW=SDL2`, `USE_DEKO3D=ON` is `AURORE_GRAPHICS=DEKO3D`, `USE_LIBROMFS` is `AURORE_LIBROMFS`, `BRLS_BUILD_TESTS`, `BRLS_DEV_TOOLS`, `BRLS_INSPECTOR`, `BRLS_UNITY_BUILD` and `BRLS_FONTSTASH_STREAM` are `AURORE_BUILD_TESTS`…, and the variable `BRLS_RESOURCES_DIR` or `CUSTOM_RESOURCES_DIR` (the folder that holds `resources/` on a desktop) is `AURORE_RESOURCES_ROOT`. `PLATFORM_*` and `USE_*` are still set as plain variables after `commonOption.cmake`, for the app's own tests. The presets in `CMakePresets.json` are Aurore's own, for the demo.
- **The Switch** keeps devkitPro's compiler flags now (the Cortex-A57 tuning, the TLS model): an app that put them back by hand can stop. libpulsar is no longer linked: borealis' Switch sounds had been off for years, and still are.

## 3. The resources

Aurore's `resources/` has more than borealis': the library's and Cadran's strings in 15 languages (`i18n/<language>/`: `cadran.json`, `hints.json`, `a11y.json`, `themes.json`), the emoji (`img/emoji/`), the fonts (Inter, PromptFont and Sora in `font/`, Material Icons in `material/`), the theme folders (`themes/`). The app's resources go **on top of** Aurore's, at each build: copy Aurore's folder, then the app's over it (the `ASSEMBLE_RESOURCES` step of [`template/CMakeLists.txt`](../template/CMakeLists.txt) does it, and leaves the demo's own files out). A copy made once goes stale at the next update of the submodule.

## 4. The look: Cadran

Aurore draws every app in [Cadran](cadran.md)'s look, the look of PlayGuard and PlayShelf, without a line of code: an icon rail for the sidebar, cards for the lists, its dialogs and pickers, its palettes. Two choices:

- **Keep borealis' look**: `aurore::Application::setCadran(false)` before `init()`. The screens look as before, with Aurore's fixes and features.
- **Take Cadran's**: fill in what it asks of the app (`ui::hooks`: the main screen and its tabs, the rail's icons, the version) and keep the user's choices (`ui::Prefs`), as [cadran.md](cadran.md) says. Each hook left empty does nothing, so it can be done screen by screen.

**Licence.** borealis is Apache 2.0; Cadran is GPLv3, and is linked into every app built with Aurore, whichever look it shows: an app built with Aurore is GPLv3 ([`NOTICE`](../NOTICE)).

## 5. What changes on screen

Most apps build and run unchanged. What to look at after the move:

| Change | What to check |
| --- | --- |
| **Yoga 3**, with standard CSS flexbox (Yoga 2's errata dropped) | Absolute children of a padded parent placed without insets, and sizes in percent, may move a little. `boxSizing` is `borderBox` by default (sizes include padding and border); `contentBox` is there for the others |
| **Fonts off the Switch**: Inter for the text, PromptFont for the buttons, instead of Nintendo's; Chinese, Japanese and Korean from the system | Text widths on the desktop and the phones ([fonts.md](fonts.md)) |
| **Theme keys**: every colour of the core views is a theme key, changed live | An app that set colours once at start-up from `getTheme()` should keep the key (`View::setThemeColor`, `@theme/…`) to follow a theme change ([themes.md](themes.md)) |
| **Translations** in 15 languages for borealis' own words, and a language switched while the app runs (`aurore::loadTranslations(locale)`) | An app's strings that override borealis' own |
| **Links**: `Platform::openBrowser` opens `http` and `https` links only, without a shell | An app that opened other schemes or local files through it |
| **Pictures** larger than 4096×4096 pixels (or 16384 on a side) are refused before they are decoded | Large backgrounds; scale them down beforehand |
| **Focus**: a hidden box gives no focus, a destroyed view is forgotten as the focus | Screens that relied on focusing a hidden view |

The README's [*Every change and its API*](../README.md#-what-aurore-adds) lists the rest: new views, styles per state, the inspector, hot reload, the test harness.

## 6. The names

Every name borealis gave the library has an Aurore one, and only the Aurore one works: an app that still uses `brls::`, `<borealis.hpp>` or `BRLS_` no longer builds, and a `brls:` XML tag, a `brls/` theme key or a `BRLS_` environment variable is no longer read.

| borealis | Aurore |
| --- | --- |
| `namespace brls`, `brls::View` | `namespace aurore`, `aurore::View` |
| `<borealis.hpp>`, `<borealis/core/view.hpp>`… | `<aurore.hpp>`, `<aurore/core/view.hpp>`… |
| `<borealis/extern/nlohmann/json.hpp>`, `include/borealis/extern/nanovg`… | `<aurore/extern/…>`, `include/aurore/extern/nanovg`… |
| `BRLS_BIND`, `BRLS_ASSET`, `BRLS_TEST`, `BRLS_CHECK`, `BRLS_KBD_KEY_Y`… | `AURORE_BIND`, `AURORE_ASSET`, `AURORE_TEST`, `AURORE_CHECK`, `AURORE_KBD_KEY_Y`… |
| `BrlsKeyCombination`, `BrlsKeyboardScancode`, `BrlsKeyCode`… | `AuroreKeyCombination`, `AuroreKeyboardScancode`, `AuroreKeyCode`… |
| What the build defines: `BOREALIS_USE_OPENGL`, `BRLS_RESOURCES`, `BRLS_USE_FREETYPE`… | `AURORE_USE_OPENGL`, `AURORE_RESOURCES`, `AURORE_USE_FREETYPE`… |
| What an app defines: `-DBRLS_BSD_SESSIONS=…`, `-DBRLS_I18N_PREFIX=…`, `-DBRLS_CADRAN_HOST_I18N` | `-DAURORE_BSD_SESSIONS=…`, `-DAURORE_I18N_PREFIX=…`, `-DAURORE_CADRAN_HOST_I18N` |
| Environment: `BRLS_LANG`, `BOREALIS_LANG`, `BRLS_THEME`, `BRLS_INSPECTOR`, `BRLS_TEST_OUTPUT`… | `AURORE_LANG`, `AURORE_THEME`, `AURORE_INSPECTOR`, `AURORE_TEST_OUTPUT`… |
| XML: `<brls:Box>`, `<brls:Label>`, `registerXMLView("brls:…")` | `<aurore:Box>`, `<aurore:Label>`, `registerXMLView("aurore:…")` |
| Theme colours and style metrics: `brls/text`, `@theme/brls/accent`, `@style/brls/…` | `aurore/text`, `@theme/aurore/accent`, `@style/aurore/…`, in code, XML and theme files |
| CMake: `BRLS_BUILD_TESTS`, `BRLS_DEV_TOOLS`, `BRLS_RESOURCES_DIR`, `brls_add_tests()` | `AURORE_BUILD_TESTS`, `AURORE_DEV_TOOLS`, `AURORE_RESOURCES_ROOT`, `aurore_add_tests()` |
| The web page's `brls_web_button()` | `aurore_web_button()` |
| `ASYNC_RETAIN`, `ASYNC_TOKEN`, `ASYNC_RELEASE` | capture `aliveToken()` and return if it expired ([getting-started.md](getting-started.md)) |
| `Threading::getSyncFunctions()`, `getAsyncTasks()`, `new Threading()` | `Threading::isIdle()`; the application starts the threads |

Moving an app's code to the new names is a search and replace; `git grep` finds what is left:

```sh
git grep -l -E 'brls|borealis|BRLS_|BOREALIS_' -- src include resources CMakeLists.txt | xargs sed -i -E \
    -e 's/\bnamespace brls\b/namespace aurore/g' -e 's/\bbrls::/aurore::/g' -e 's/\bbrls_(add_tests|web_button)\b/aurore_\1/g' \
    -e 's/<borealis(\.hpp|\/)/<aurore\1/g' -e 's/\bBRLS_|\bBOREALIS_/AURORE_/g' -e 's/\bBrls([A-Z])/Aurore\1/g' \
    -e 's/\bbrls:([A-Z])/aurore:\1/g' -e 's/([@"\/ ])brls\//\1aurore\//g'
```

Look at the result before committing: an app's own name that starts with `BRLS_` is changed too, and `BRLS_RESOURCES_DIR` becomes `AURORE_RESOURCES_ROOT`, not `AURORE_RESOURCES_DIR` (libromfs' environment variable).

## 7. Afterwards

- Turn on the debugging aids while checking the screens: `AURORE_INSPECTOR=1` and `AURORE_HOT_RELOAD=$PWD/resources` ([environment.md](environment.md)).
- [architecture.md](architecture.md) explains the loop, the focus, ownership and threads; the API reference is built with `doxygen docs/Doxyfile`.
- Issues and fixes go to [JigSawFr/aurore](https://github.com/JigSawFr/aurore); a fix that belongs upstream is welcome there too.
