# Overlay mode on the Switch: design note

**Status: not built.** This note says what an "Aurore Lite" overlay, drawn on a VI layer over a running game the way Tesla overlays are, would need, and why it is not a platform to add without a console to try it on. The use in mind is PlayGuard's "time left" overlay.

## How Tesla overlays draw

An overlay (`.ovl`, an NRO) is loaded by [nx-ovlloader](https://github.com/WerWolv/nx-ovlloader), a sysmodule derived from nx-hbloader that gives it a **fixed heap: 4 MB by default** in the maintained fork (ppkantorski/nx-ovlloader reads `/config/nx-ovlloader/heap_size.bin`; WerWolv's reserved 6 MB, 2 of them for the framebuffers). It is not an applet: no `appletMainLoop()` focus, no operation-mode events, no applet audio. [libtesla](https://github.com/WerWolv/libtesla) (GPL-2.0-or-later, so usable from GPLv3 code) then, in `tsl::gfx::Renderer::init()`:

1. `viInitialize(ViServiceType_Manager)`, `viOpenDefaultDisplay()`;
2. `viCreateManagedLayer(display, 0, 0, &__nx_vi_layer_id)` then `viCreateLayer()`, `viSetLayerScalingMode(FitToLayer)`, `viSetLayerZ()` above the game;
3. `viAddToLayerStack()` for the stacks it must show on (Default, Screenshot, Recording, Arbitrary, LastFrame, Null, ApplicationForDebug, Lcd), `viSetLayerSize()` (720 px tall, 448×720 scaled), `viSetLayerPosition()`;
4. `nwindowCreateFromLayer()`, then `framebufferCreate(448, 720, PIXEL_FORMAT_RGBA_4444, 2 buffers)`: about 1.3 MB.

It draws **on the CPU**: rectangles and glyphs (stb_truetype, from the shared system font) written straight into the framebuffer in its block-linear layout, between `framebufferBegin()` and `framebufferEnd()`.

Input: the overlay reads the pads and the touch screen itself (`padUpdate()`, `hidGetTouchScreenStates()`, touch coordinates mapped to the layer). While it is shown it takes the input away from the game with `hidsys` command 503 (`EnableAppletToGetInput(false, aruid)` for the application and the system applets, found through `pmdmnt`), and gives it back when hidden.

## What Aurore has today

Aurore's Switch platform (`library/lib/platforms/switch/`) is an application: it draws with deko3d (or Mesa's OpenGL) on `nwindowGetDefault()` at 1280×720 or 1920×1080, follows the operation mode through applet events, plays sounds with audren, and its resources come from romfs. Its memory is that of an application (hundreds of MB): the deko3d pools alone take 5 MB (`switch_video.cpp`: 4 MB of images, where the framebuffers and the depth buffer also go, 1 MB of data, 128 KB of code), the nanovg font atlas starts at 512×512 and grows up to 4096×4096 (16 MB in A8), and then come the textures, the Yoga nodes and the views.

## What a VI-layer platform needs

| Need | What it takes |
|---|---|
| A window | A `Platform` (`SwitchOverlayPlatform`) whose video context creates the layer as above and an `NWindow` from it, never `nwindowGetDefault()`; a fixed 448×720 window, portrait, so `Application::contentWidth` is 448 and screens must be laid out for it (Cadran's rail and header assume 1280 wide) |
| Drawing | Either deko3d on the layer's `NWindow` (`dk::SwapchainMaker` takes any `NWindow`), if an ovlloader process may open the GPU channel next to the game: **unknown, to try on a console first**; or a CPU nanovg backend: nanovgXC's `nanovg_sw.h` (zlib) is a candidate, with the output swizzled into the block-linear RGBA4444 framebuffer as libtesla does |
| Memory | Everything within 4 MB (6–8 MB with the fork's setting): the atlas capped at 512×512 (`NVG_MAX_FONTIMAGE_SIZE`), pools cut to what 448×720 needs, no image cache beyond a few icons, the system's shared font only (mapped by `pl`, not on the heap), small Yoga trees; `__nx_heap_size` is the loader's, not the app's |
| Input focus | An `InputManager` reading pads and touch directly, with the layer's offset and scale; `requestForeground(true/false)` through hidsys 503 when shown and hidden; a button combination to show it, as Tesla's L+DDOWN+RS |
| No applet | No applet hooks, operation-mode events or audren; `Application::mainLoop()` without `appletMainLoop()`; the overlay's own pacing (Tesla draws at 60 Hz while shown, sleeps while hidden) |
| Loading | A `.ovl` built like an NRO, in `/switch/.overlays/`, started from the Tesla menu (or Ultrahand), which nx-ovlloader then hands back to (`envSetNextLoad`) |

## Suggested path

1. On a console: a bare overlay that opens a deko3d device on its layer and clears it, to learn whether the GPU is usable from nx-ovlloader with a game running, and what deko3d's minimum memory is there.
2. With the answer, the renderer: deko3d on the layer, or nanovgXC's software backend behind Aurore's nanovg calls.
3. A `PLATFORM_SWITCH_OVERLAY` CMake target and the platform above, with a 448-wide Cadran layout (no rail, a header and a card).
4. PlayGuard's "time left" overlay on it; until then, libtesla itself (GPL-2.0-or-later) is the shorter way to that overlay.
