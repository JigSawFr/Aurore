# The demo in a browser

Aurore's demo (`demo/`), compiled with [Emscripten](https://emscripten.org) for a browser: the real library, the same code as on a Switch, drawn with WebGL 2. The website publishes it at <https://aurore.boreale.dev/demo/>.

## Building it

With Emscripten's SDK active (`source emsdk_env.sh`; the version CI uses is pinned in `.github/workflows/site.yml`):

```sh
emcmake cmake -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
python3 -m http.server -d build-web/web 8000      # http://localhost:8000/
```

Under `emcmake`, the top `CMakeLists.txt` builds the desktop platform with SDL2 and OpenGL ES 3 from Emscripten's ports (`web/cmake/SDL2Config.cmake` finds nothing to link; `-DAURORE_WINDOW=SDL3` takes Emscripten's SDL3 port instead, with `web/cmake/SDL3Config.cmake`), packages `resources/` into `aurore_demo.data` at `/app/resources`, and assembles the page in `build-web/web/`: `web/page/`, `aurore_demo.js`, `.wasm` and `.data`, the mark and the site's fonts.

- **Japanese, Korean and Chinese** need a font a browser does not give: `python3 tools/web_cjk_font.py /usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc build-web/cjk.otf` cuts the characters the catalogs use out of Noto Sans CJK (`fonts-noto-cjk`, and fontTools), and `-DAURORE_WEB_CJK_FONT=build-web/cjk.otf` packages it as the app's `font/cjk.otf` (about 270 KB).
- `demo/src/main.cpp` hands each frame back to the browser (`emscripten_set_main_loop`); the rest is Aurore's own: `aurore_web_button()` for the pad, the window kept at 1280×720 and scaled by the page ([docs/platforms.md](../docs/platforms.md#in-a-browser)).

## The page

- **Threads** need a cross-origin isolated page, which GitHub Pages cannot make with headers: `sw.js`, a service worker, adds them (as coi-serviceworker does), and the page loads once more under it. A browser without service workers (or a private window that refuses them) is told why the demo cannot start.
- **The theme and the language** are in the address: `?theme=winter` (a theme folder or file of `resources/themes/`, or a preset: `builtin:halloween`) passes `-t` to the demo, `?lang=fr` sets `AURORE_LANG` (the consoles' 15 languages, `fr-CA`, `es-419` and `pt` among them; `?theme=boreale` is Boréale's night). Without them, Cadran's default look and the browser's language. The header's lists change them, starting the demo again.
- **Controls**: the keyboard reaches SDL (the arrows, Enter for Ⓐ, Esc for Ⓑ, X, Y, L, R, F12 for the inspector), a click is a touch, and **Pad** shows the controller on screen (at once on a touch screen; the choice is remembered): under the screen in portrait, beside it in landscape. **Full screen** shows the screen and the pad only.
- The page is in English, in French for a French browser; it follows the website's look (`site/assets/`).
- Its Content-Security-Policy allows only its own files, WebAssembly and the workers Emscripten starts.
