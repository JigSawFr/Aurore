# App template

A minimal app on Aurore, to copy and rename: one screen from XML with two tabs, its strings in two languages, and the build of a desktop binary and a Nintendo Switch `.nro`.

```
CMakeLists.txt                  the build: Aurore as a subdirectory, resources merged
src/main.cpp                    init, the window, the main screen
src/home_tab.*                  a view of the app's: the Home tab and its button
resources/xml/activity/main.xml the main screen: a tab frame
resources/xml/tabs/home.xml     the Home tab
resources/i18n/<lang>/app.json  the strings ("app/…"); add a language with a folder
icon.jpg                        the .nro's icon (256×256 JPEG)
```

Aurore's look, Cadran, comes with it, and so does its licence: an app built with Aurore is under the **GPLv3**.

## Start an app from it

```sh
cp -r template ~/my_app && cd ~/my_app
git init
git submodule add -b main https://github.com/JigSawFr/aurore.git library/aurore
git submodule update --init --recursive
```

Rename `my_app` in `CMakeLists.txt` (`project()`) and set the author (`PROJECT_AUTHOR`) and the version (`VERSION_MAJOR`, `VERSION_MINOR`, `VERSION_ALTER`) there too; the title shown on screen is `title` in `resources/i18n/<lang>/app.json`. Replace `icon.jpg`.

Inside Aurore's own repository the template finds Aurore around it (`..`) and builds as it is; elsewhere it looks in `library/aurore`, or where `-DAURORE_DIR=…` says.

## Desktop

Linux needs a compiler with C++20, CMake 3.16 or later, Ninja, and the X11/Wayland and OpenGL development packages GLFW asks for (`libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl-dev`, and `pkg-config libdbus-1-dev`, on Debian or Ubuntu).

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DAURORE_PLATFORM=DESKTOP
cmake --build build
./build/my_app
```

`-DAURORE_WINDOW=SDL2` builds with SDL2 instead of GLFW. The resources are copied to `build/resources` at each build; the binary reads them there.

## Nintendo Switch

With [devkitPro](https://devkitpro.org/wiki/Getting_Started) and its `switch-dev` group (plus `switch-glfw switch-mesa switch-glm` for the OpenGL build):

```sh
export DEVKITPRO=/opt/devkitpro
cmake -B build-switch -G Ninja -DCMAKE_BUILD_TYPE=Release -DAURORE_PLATFORM=SWITCH -DAURORE_GRAPHICS=DEKO3D
cmake --build build-switch --target my_app.nro
```

`build-switch/my_app.nro` goes to `/switch/` on the SD card; nxlink sends it straight to a console (`nxlink -s build-switch/my_app.nro`). Without `-DAURORE_GRAPHICS=DEKO3D` it draws with OpenGL instead of deko3d.

## Where to go next

- A new screen: an XML file in `resources/xml/`, an `Activity` with `CONTENT_FROM_XML_RES`, `aurore::Application::pushActivity()`.
- A view of your own: a `aurore::Box` subclass, `aurore::Application::registerXMLView("MyView", MyView::create)` before the XML that uses it.
- Aurore's demo (`demo/` in Aurore) shows every view; [docs/cadran.md](../docs/cadran.md) what Cadran asks of an app and gives it; [docs/components.md](../docs/components.md) the grid, the context menu and the icon button.
