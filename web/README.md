# The demo in a browser

Aurore's demo (`demo/`), compiled with [Emscripten](https://emscripten.org) for a browser: the real library, the same code as on a Switch, drawn with WebGL 2. The website publishes it at <https://aurore.boreale.dev/demo/>.

## Building it

With Emscripten's SDK active (`source emsdk_env.sh`; the version CI uses is pinned in `.github/workflows/site.yml`):

```sh
emcmake cmake -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
python3 -m http.server -d build-web/web 8000      # http://localhost:8000/
```

Under `emcmake`, the top `CMakeLists.txt` builds the desktop platform with SDL3 and OpenGL ES 3 from Emscripten's ports (`web/cmake/SDL3Config.cmake` gives an empty `SDL3::SDL3`; `-DAURORE_WINDOW=SDL2` takes Emscripten's SDL2 port instead, with `web/cmake/SDL2Config.cmake`), packages `resources/` into `aurore_demo.data` at `/app/resources`, and assembles the page in `build-web/web/`: `web/page/`, `aurore_demo.js`, `.wasm` and `.data`, the mark, Boréale's marks and the site's fonts.

- **Japanese, Korean and Chinese** need a font a browser does not give: `python3 tools/web_cjk_font.py /usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc build-web/cjk.otf` cuts the characters the catalogs use out of Noto Sans CJK (`fonts-noto-cjk`, and fontTools), and `-DAURORE_WEB_CJK_FONT=build-web/cjk.otf` packages it as the app's `font/cjk.otf` (about 270 KB).
- `demo/src/main.cpp` hands each frame back to the browser (`emscripten_set_main_loop`); the rest is Aurore's own: `aurore_web_button()` for the pad, the window kept at 1280×720 and scaled by the page ([docs/platforms.md](../docs/platforms.md#in-a-browser)).

## The page

As PlayGuard's and PlayShelf's simulators, in the website's look (`site/assets/site.css`): its colours, Sora for the name and Figtree for the rest (`site/assets/fonts/`, copied by CMake next to the page).

- **The header** is the website's, in Boréale's night whatever the theme: the name opens the menu of Boréale's sites (Boréale, PlayGuard, PlayShelf, Aurore; the marks of `site/assets/marks/`, copied into `marks/`), then the website's links (Features, Documentation, API, GitHub), the build (`build.json`, written by `site.yml`: its pull request or branch and commit), the language menu, the theme's menu and the settings' button. On a phone, the links and the build go; the language's button shows its flag and code, then its flag alone under 380px; the theme's menu is a short face (its icon) over the system's list.
- **The language** is the browser's, if the consoles have it (`fr-CA` for a Canadian browser, `es-419` for a Mexican one, `zh-Hant` for Taiwan…; else English), unless the address says `?lang=` or the header's menu picks another. The menu is the website's (`tools/build_site.py`, `site/assets/site.js`): a button with the page's flag and language, and under it the 15 languages in a popover, each with its flag, the page's marked (Esc or a click elsewhere closes it; ↓ on the button opens it at the language in use, the arrows, Home and End move through it). The flags are the website's `twemoji-flags.woff2` (Twemoji's 15, CC-BY 4.0, with `LICENSE-twemoji-flags.txt`), so Windows shows them too. The page's texts are in `web/page/i18n/<code>.json`, English underneath; the demo starts in the page's language (`AURORE_LANG`).
- **The theme** is *Dark* by default; the header chooses *Auto* (the browser's) or *Light* instead (kept for the next visits, and in the address as `?variant=` so a copied link shows it). The demo starts in the page's (`AURORE_THEME`); when it changes while the demo runs, under *Auto* or from the menu, the page follows at once and offers to start the demo again.
- **The settings** are in the side panel and in the address: `?simulate=3ds` lays the app out as on a console and draws the console around its screen (`AURORE_SIMULATE`: `switch`, `vita`, `ps4`, `3ds`, `phone`; `demo/include/simulation.hpp`), `?theme=winter` is a theme folder or file of `resources/themes/`, or a preset (`builtin:halloween`), passed as `-t` (`?theme=boreale` is Boréale's night). **Apply** loads the page again with them, the demo starting over; **Copy link** gives a link that opens that state; **Reset** goes back to the defaults. The panel's **Log** tab shows what the demo writes. ⚙ hides the panel (kept), or on a phone goes to it, under the screen.
- **Controls**: the keyboard reaches SDL from the screen, once clicked or focused (`SDL_EMSCRIPTEN_KEYBOARD_ELEMENT`: Esc on a menu of the page stays the page's) (the arrows, Enter for Ⓐ, Esc for Ⓑ, X, Y, L, R, F12 for the inspector), a click is a touch, and **Pad** shows the controller on screen (at once on a touch screen; the choice is kept): under the screen in portrait, on either side of it on a phone in landscape. **Full screen** (where the browser allows it) shows the screen and the pad only.
- **Tools**, under the screen (behind a button on a phone): **Screenshot** (a PNG of the screen), **Record** (a WebM video, until Stop) and **Report a problem** (a GitHub issue with the page's address, the build, the browser and the end of the log).
- **Threads** need a cross-origin isolated page, which GitHub Pages cannot make with headers: `sw.js`, a service worker, adds them (as coi-serviceworker does), and the page loads once more under it. A browser without service workers (or a private window that refuses them) is told why the demo cannot start.
- `<body data-demo>` says where the demo is: `loading`, `running`, `ready` (its first frame drawn) or `error`.
- Its Content-Security-Policy allows only its own files, WebAssembly and the workers Emscripten starts.
- Published on the site (not in a pull request's preview), it carries Umami's tracker, the site's own copy (`../assets/umami.js`, added by `tools/build_site.py`'s `demo_audience()`), which counts each visit on aurore.boreale.dev alone, without cookie; its policy then lets it send to `gateway.umami.is`. Its sites menu links to boreale.dev's privacy and legal notice.

## Testing it

`tools/web_smoke.py build-web/web build-web/smoke` starts the demo in headless Chromium (Playwright for Python, and Pillow) four ways: served with the headers threads need, served without them as GitHub Pages does (the service worker adds them), and with `?lang=fr&theme=winter` (the page in French), and with `?simulate=3ds&variant=light`. Each fails when the page throws or says the demo cannot start or has stopped, when the first screen is blank, when the down arrow changes nothing on screen, when the language menu has not the 15 languages or the menu of Boréale's sites does not open; each canvas is written to `build-web/smoke/<name>.png`, with what the demo logged. CI runs it on the SDL2 build the site publishes (`site.yml`) and on the SDL3 one (`ci.yaml`, `web (sdl3)`).
