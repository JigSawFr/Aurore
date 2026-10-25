<img src="resources/img/borealis_96.png" alt="borealis logo" width="96">

# borealis

Controller and TV oriented UI library for Android, iOS, Windows, macOS, Linux,
PS4, PS Vita and Nintendo Switch, in C++17.

This repository is a fork of [xfangfang/borealis](https://github.com/xfangfang/borealis)
(the `wiliwili` branch, itself a fork of [natinusala/borealis](https://github.com/natinusala/borealis)),
maintained for [PlayGuard](https://github.com/JigSawFr/playguard). It follows
upstream and adds:

- fixes and features picked from other forks, each keeping its author and its
  `cherry picked from` line;
- the changes PlayGuard used to apply as patches on top of upstream.

Everything else, from the platforms to the build, is upstream's. Nothing here
changes the behaviour of an app that does not ask for it, except the fixes.

## What borealis is

- Mimics the Nintendo Switch system UI, but can be restyled to look like
  anything else
- Hardware-accelerated vector graphics with automatic scaling for TV use
  (nanovg), portable to new graphics APIs through a nanovg backend
- Flexbox layout for everything (Yoga)
- Automatic focus paths for controller navigation, and touch support
- User interfaces written in XML, code only where it matters
- Built-in components to use or restyle, or your own from scratch
- Recycling lists for large amounts of data
- Internationalisation, storage, logger, animations, timers and background
  tasks built in

## What this fork adds

### Layout and drawing

| Change | API |
| --- | --- |
| Gaps between a box's children | `gap`, `rowGap`, `columnGap` (XML), `Box::setGap` / `setRowGap` / `setColumnGap` |
| Wrapping boxes, with focus that moves across their lines | `flexWrap="wrap"`, `Box::setWrap` |
| A radius for each corner | `cornerTopLeftRadius`… (XML), `View::setCornerRadius(tl, tr, br, bl)` |
| Horizontal gradient background, and gradients set from code | `background="horizontal_linear"`, `View::setBackgroundLinearGradient` |
| `imageAlign` honoured for `FIT` and `FILL` | — |
| Spinner colour | `ProgressSpinner::setColor` |
| Toasts styled by the theme, with an optional icon | `brls/notification/background`, `…/text`, `…/corner_radius`, `…/spacing`, `…/margin_top`, `…/margin_right`; `Application::notify(text, leading)` |
| A footer clock in hours and minutes | — |

### Text

| Change | API |
| --- | --- |
| A font scale for every label, with a cap per label and shrink-to-fit on one line | `Label::setFontScale`, `setFontScaled`, `setFontScaleMax`, `setShrinkToFit`, `setHintsFontScaleMax` |
| A label tells when it is cut with "…" | `Label::isTruncated` |
| A wrapped line no longer starts with a space; a no-break space (U+00A0) keeps words together | — |
| The default font is looked up for each new label, so a font registered later is used | — |

### Internationalisation

| Change | API |
| --- | --- |
| Load another language while the app runs | `brls::loadTranslations(locale)` |
| Lookups safe from other threads while the language changes | — |
| Lookups without exceptions (about 6× faster in English) | — |

### Focus, hints and input

| Change | API |
| --- | --- |
| A hidden box gives no focus; navigation skips hidden scrolling frames and sliders | — |
| A destroyed view is forgotten as the focus, and the empty focus is checked everywhere | — |
| A lost child focus reaches the parent scrolling frames | — |
| Optional footer hints, left out when they do not all fit | `setHintsOptionalOwner(view)` |
| Hints drawn steady while a screen fades | `setHintsSteady(true)` |
| The highlight's glow held still (stable screenshots) | `setHighlightAnimationStill(true)` |
| Extra unblocks of the input no longer wrap its counter | — |
| A view's id can be read back; a button's state too | `View::getId`, `Button::getState` |
| A callback when a dialog closes | `Dialog::setCloseCallback` |

### Platforms

| Change | API |
| --- | --- |
| SDL: the right controller closed when one is unplugged, and every controller read after a reconnection | — |
| SDL: runs in a browser (Emscripten), with an on-screen pad | `brls_web_button()`, only under `__EMSCRIPTEN__` |
| GLFW: Wayland `app_id` and X11 `WM_CLASS`, so the taskbar shows the app's icon | `Application::setAppId`, before `createWindow()` |
| Switch / deko3d: the command memory grows when a frame needs more, instead of aborting; image descriptors reused after a texture is freed; the applet hook removed first at shutdown | — |
| Logger: output flushed after every line | — |

### Performance

- The same text or the same size set again is not laid out again.
- A view's XML attribute handlers are registered the first time one is used,
  not in every constructor (a `Label` built in code: 3–6 ms → under 1 ms for
  200 views).
- Cached textures are freed once nothing uses them.
- Views queued while the deletion pool drains are kept, and each recycling
  list checks its own width.

The commit messages say what each change does, why, and where it comes from:
`git log 5f08b28..` lists them.

## Using it in a project

1. Build your project as C++17, without `-fno-rtti` or `-fno-exceptions`.
2. Add this repository as a submodule (or a
   [subrepo](https://github.com/ingydotnet/git-subrepo)):

   ```sh
   git submodule add -b wiliwili https://github.com/JigSawFr/borealis.git extern/borealis
   git submodule update --init --recursive
   ```

3. Copy the `resources` folder to the root of your project.

Or start from upstream's [template](https://github.com/xfangfang/borealis_template)
and point its submodule here.

## Building the demo

The demo in `demo/` builds for every platform. Upstream's wiki gives the
toolchains and the commands:

- [Windows, macOS and Linux](https://github.com/xfangfang/borealis/wiki/Windows-macOS-Linux)
- [Nintendo Switch](https://github.com/xfangfang/borealis/wiki/NintendoSwitch)
- [Android](https://github.com/xfangfang/borealis/wiki/Android)
- [iOS](https://github.com/xfangfang/borealis/wiki/iOS)
- [PS Vita](https://github.com/xfangfang/borealis/wiki/PS-Vita)
- [PS4](https://github.com/xfangfang/borealis/wiki/PS4)

On a desktop, for instance:

```sh
git clone --recursive https://github.com/JigSawFr/borealis.git
cd borealis
cmake -B build_pc -DPLATFORM_DESKTOP=ON
cmake --build build_pc
./build_pc/borealis_demo
```

The CI builds the demo for each platform on every push.

## Contributing

The default branch is `wiliwili`, as upstream's. Changes from upstream and
from other forks come in as merges or cherry-picks that keep their author;
commit messages follow [Conventional Commits](https://www.conventionalcommits.org/).
A fix that is not specific to this fork is better sent to
[upstream](https://github.com/xfangfang/borealis) too.

## Made with borealis

| Project | Description |
| --- | --- |
| [Moonlight-Switch](https://github.com/XITRIX/Moonlight-Switch) | Moonlight port for Nintendo Switch |
| [PlayGuard](https://github.com/JigSawFr/playguard) | Parental controls for Nintendo Switch |
| [SimpleModDownloader](https://github.com/PoloNX/SimpleModDownloader) | Downloads mods from GameBanana on the Switch |
| [switchfin](https://github.com/dragonflylee/switchfin) | Third-party Jellyfin client for PC, PS4 and Nintendo Switch |
| [UpcomingSwitchGames](https://github.com/PoloNX/UpcomingSwitchGames) | Upcoming Switch games |
| [wiliwili](https://github.com/xfangfang/wiliwili) | Third-party bilibili client for PC, PS4, PS Vita, Xbox and Nintendo Switch |

## Licence

Apache 2.0, see [`LICENSE`](LICENSE) and [`NOTICE`](NOTICE). The main
contributors are listed in [`AUTHORS`](AUTHORS); the bundled libraries in
`library/lib/extern` keep their own licences.
