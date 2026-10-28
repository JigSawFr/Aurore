# Environment variables

Every environment variable Aurore reads, in one place. None is needed: each one turns on a debugging aid, overrides what the system says, or steers the tests. They are read on the desktop (Windows, macOS, Linux) unless the table says otherwise; the consoles have no environment to set.

A flag is on when it is set to anything but `0` (Cadran's: `1`, `true` or `yes`).

## The library

| Variable | Read by | Effect |
| --- | --- | --- |
| `BRLS_INSPECTOR` | `Application::init()` | `1` turns the [inspector](inspector.md) on (F12, or L3 + R3), `0` off, even in a build made with `-DBRLS_INSPECTOR=ON` |
| `BRLS_HOT_RELOAD` | `Application::init()` | `1` turns [hot reload](hot_reload.md) on; a folder (`$PWD/resources`) also reads the XML from there instead of the copy next to the program. Not on the consoles |
| `BRLS_PROFILE` | `Profiler` | `1` measures every frame and logs averages every 120 frames ([testing.md](testing.md#the-bench)). Not on the Switch nor in a browser |
| `BRLS_REDRAW_DEBUG` | `Application::init()` | `1` checks redraw on demand: each frame it skips is drawn anyway and compared, a difference logged as a warning (`Application::setRedrawDebug`, OpenGL only). Not on the Switch nor in a browser |
| `AURORE_RESOURCES_DIR` | libromfs | With the resources built in (`-DUSE_LIBROMFS=ON`): a folder laid out like `resources/` whose files are read before the built-in ones, so changed XML, pictures or translations need only a restart |
| `BRLS_THEME` (older name `BOREALIS_THEME`) | the desktop platform | `DARK` starts in the dark variant, anything else in the light one, instead of the system's mode. With Cadran on, the preferences' theme (`ui::Prefs::theme`, dark by default) is applied over it: the demo maps `BRLS_THEME=LIGHT` onto them. `BRLS_THEME` wins when both are set |
| `BRLS_LANG` (older name `BOREALIS_LANG`) | the desktop platform | The language, when the app did not set `Platform::APP_LOCALE_DEFAULT`: a name as Aurore spells it (`fr`, `zh-Hant`, `es-419`) or as a system does (`fr_FR.UTF-8`). It wins over the system's language under GLFW and SDL alike; `BRLS_LANG` wins when both are set |
| `LANGUAGE`, `LC_ALL`, `LC_MESSAGES`, `LANG` | the GLFW platform, on Linux | The system's language, the first one set (and not `C` or `POSIX`); in `LANGUAGE`, the first of a list (`fr:en`). Not read when `BRLS_LANG` is set. (Windows: the display language and its region; macOS: the first preferred language) |
| `SteamDeck` | the desktop platform, on Linux | Set by Steam on a Steam Deck: the screen's brightness through the Deck's backlight, links opened and the keyboard shown through Steam (GLFW) |
| `systemroot` | the desktop fonts, on Windows | Where Windows' fonts are (`C:\Windows\Fonts`), for Chinese, Japanese and Korean |

The debugging aids above (`BRLS_INSPECTOR`, `BRLS_HOT_RELOAD`, `BRLS_PROFILE`, `BRLS_REDRAW_DEBUG`) are read only in a build made with `BRLS_DEV_TOOLS`. It is `AUTO` by default: on, except in `Release` and `MinSizeRel` builds, where these variables do nothing. `-DBRLS_DEV_TOOLS=ON` reads them in a release build too (a simulator built in `Release`), `OFF` in none ([hot reload](hot_reload.md#turning-it-on)). Their functions (`Application::setInspectorEnabled`, `setHotReload`, `Profiler::setEnabled`, `setRedrawDebug`) work in every build.

## Cadran

[Cadran](cadran.md)'s flags help tests and screenshots of an app. They are read through `ui::env_flag("NAME")`, which reads `BRLS_NAME` in a build with `BRLS_DEV_TOOLS` (above: not a `Release` one, by default); an app that sets `ui::hooks::get().env_flag` answers them itself in every build (its own variables, a test's settings). On the Switch they are all off.

| Variable | Effect |
| --- | --- |
| `BRLS_STILL_FOCUS` | The focus glow and every movement held still (`Application::getReduceMotion()` is then true) |
| `BRLS_CADRAN_NOGLOW` | No background glow on the screens |
| `BRLS_FOCUS_LOG` | Each focused line logged by its title |
| `BRLS_CUT_TEXT` | Every label cut short ("…") written to `cut_text.txt` |
| `BRLS_DOCKED` | The console on its dock: larger text when the text size is "auto" |
| `BRLS_BATTERY` | The console in hand, on battery: the lighter effects when they are "auto" |

## The test harness

Read by the [test harness](testing.md) (`-DBRLS_BUILD_TESTS=ON`) and the demo's tests.

| Variable | Effect |
| --- | --- |
| `BRLS_TEST_REFERENCES` | The folder of the reference screenshots, when the test's options name none (default `references`) |
| `BRLS_TEST_OUTPUT` | The folder the screenshots and their differences are written to (default `test-output`) |
| `BRLS_TEST_UPDATE` | `1` writes every reference again from the screenshots taken, to commit after looking at them |
| `CI` | Set (as on GitHub Actions): a missing reference fails instead of being written |
| `BRLS_HEADLESS_GLFW` | `1` draws in a hidden GLFW window instead of EGL's surfaceless context (needs a display) |
| `BRLS_DECOR_SHOTS` | `tests/demo_tests.cpp` only: a folder where the case of the theme decorations saves a picture of each kind of particles |

## Names

Aurore keeps borealis' names for compatibility: the older variables start with `BOREALIS_` (`BOREALIS_LANG` and `BOREALIS_THEME` are now aliases of `BRLS_LANG` and `BRLS_THEME`), those Aurore added with `BRLS_`, and `AURORE_RESOURCES_DIR` belongs to the bundled libromfs. The same prefixes are used by CMake options (`BRLS_BUILD_TESTS`, `BRLS_INSPECTOR`) and variables (`BRLS_RESOURCES_DIR`, `CUSTOM_RESOURCES_DIR`): those are set when building, not in the environment.
