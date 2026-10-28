# Aurore in 15 minutes

From nothing to a screen of your own on the Switch: build the demo, describe a screen in XML, add a view of your own, restyle it with a theme, and ship it as a `.nro`. The API reference is built from the headers (`doxygen docs/Doxyfile`, then `build_docs/html/index.html`).

## 1. Build the demo (3 minutes)

On Linux (Debian, Ubuntu), the packages first:

```sh
sudo apt install build-essential cmake ninja-build git libdbus-1-dev libxkbcommon-dev \
  libwayland-dev wayland-protocols xorg-dev
```

Then:

```sh
git clone --recursive https://github.com/JigSawFr/aurore.git
cd aurore
cmake -B build -G Ninja -DAURORE_PLATFORM=DESKTOP -DAURORE_WINDOW=GLFW -DAURORE_LIBROMFS=ON
cmake --build build
./build/aurore_demo
```

The arrows move the focus, Enter is A, Escape is B, X, Y, L and R are themselves ([the other keys](input.md)), the mouse and a controller work too. `-DAURORE_LIBROMFS=ON` builds the `resources/` folder into the program; without it, the folder is copied next to it. Press **F12**: that is the [inspector](inspector.md), showing the views on screen, the focus and what a frame costs. Press it twice more to close it.

The demo's code is in `demo/`, its screens in `resources/xml/`: they are the examples for what follows.

## 2. Your app

Add Aurore to your project as a submodule, and its resources next to yours:

```sh
git submodule add -b main https://github.com/JigSawFr/aurore.git library/aurore
git submodule update --init --recursive
cp -r library/aurore/resources resources
```

Aurore's `template/` folder (see its `README.md`) is such an app, with the `CMakeLists.txt` for the desktop and the Switch ([platforms.md](platforms.md#platforms) has the other platforms'). The program itself starts the app, pushes a first screen and runs the loop:

```cpp
#include <aurore.hpp>
#include "hello_activity.hpp"

int main(int argc, char* argv[])
{
    if (!aurore::Application::init())
        return EXIT_FAILURE;
    aurore::Application::createWindow("Hello");

    aurore::Application::pushActivity(new HelloActivity());

    while (aurore::Application::mainLoop())
        ;
    return EXIT_SUCCESS;
}
```

## 3. A screen from XML (3 minutes)

A screen is an **activity**. Its content can come from an XML file in `resources/xml/`:

```cpp
// hello_activity.hpp
#pragma once
#include <aurore.hpp>

class HelloActivity : public aurore::Activity
{
  public:
    CONTENT_FROM_XML_RES("activity/hello.xml");

    void onContentAvailable() override
    {
        this->button->registerClickAction([](aurore::View*) {
            aurore::Application::notify("Hello!");
            return true;
        });
    }

  private:
    AURORE_BIND(aurore::Button, button, "hello_button");
};
```

```xml
<!-- resources/xml/activity/hello.xml -->
<aurore:AppletFrame>
    <aurore:Box
        title="Hello"
        width="auto"
        height="auto"
        axis="column"
        alignItems="center"
        justifyContent="center"
        gap="20">

        <aurore:Label
            text="Aurore lays this out with flexbox"
            fontSize="24" />

        <aurore:Button
            id="hello_button"
            style="primary"
            width="300"
            text="Say hello" />

    </aurore:Box>
</aurore:AppletFrame>
```

Every XML tag is a view (`aurore:Box`, `aurore:Label`, `aurore:Button`, `aurore:Image`, `aurore:ScrollingFrame`, `aurore:TabFrame`…); its attributes are the yoga layout (`width`, `height`, `grow`, `margin`, `padding`, `gap`, `axis`, `alignItems`…) and the view's own (`text`, `fontSize`, `backgroundColor`, `cornerRadius`…). `@i18n/…` reads a translation, `@theme/…` a theme colour, `@style/…` a metric, `@res/…` a resource. `AURORE_BIND` finds a view by its `id` the first time it is used. A second screen is pushed with `aurore::Application::pushActivity(new OtherActivity())`, and B takes it off.

While you work on the XML, start the app with `AURORE_HOT_RELOAD=$PWD/resources ./build/my_app`: each save builds the screen again, the focus where it was ([hot reload](hot_reload.md)).

## 4. A view of your own (3 minutes)

A view of your own is a `Box` that fills itself from its own XML, and is registered under a tag name:

```cpp
class ScoreCard : public aurore::Box
{
  public:
    ScoreCard()
    {
        this->inflateFromXMLRes("xml/views/score_card.xml");
        this->forwardXMLAttribute("player", this->name, "text");
    }

    void setScore(int score) { this->score->setText(std::to_string(score)); }

    static aurore::View* create() { return new ScoreCard(); }

  private:
    AURORE_BIND(aurore::Label, name, "name");
    AURORE_BIND(aurore::Label, score, "score");
};
```

```xml
<!-- resources/xml/views/score_card.xml -->
<aurore:Box
    width="260"
    height="auto"
    axis="column"
    padding="16"
    cornerRadius="12"
    backgroundColor="@theme/aurore/highlight/background"
    focusable="true">
    <aurore:Label id="name" fontSize="18" />
    <aurore:Label id="score" fontSize="40" text="0" />
</aurore:Box>
```

Register it before the first screen that uses it, `aurore::Application::registerXMLView("ScoreCard", ScoreCard::create);`, and it can be written in any XML: `<ScoreCard id="p1" player="Ada" />`. Override `draw()` for a view that draws itself with nanovg (`demo/src/view/` has both kinds), and `onFocusGained()`, `registerAction()` or gesture recognizers to react.

Slow work (a file, the network) runs on the task thread with `aurore::async`, and its result comes back to the UI thread with `aurore::sync`; views are only touched there. The view may be gone by then: capture its `aliveToken()` and check it first.

```cpp
aurore::async([this, alive = aliveToken()] {
    std::string text = loadScore(); // the task thread: not `this`
    aurore::sync([this, alive, text] {
        if (alive.expired())
            return; // the view was deleted meanwhile
        this->score->setText(text);
    });
});
```

`ASYNC_RETAIN`, `ASYNC_TOKEN` and `ASYNC_RELEASE` (from borealis) still work and do the same; they are kept for older code.

## 5. A theme (2 minutes)

Colours and metrics come from the theme (light or dark, following the system) and can be changed without rebuilding, from a file:

```ini
# my_theme.ini
inherit = dark
[colors]
aurore/accent = #ff7a59
aurore/background = #14161c
aurore/highlight/color1 = rgb(255, 122, 89)
[metrics]
aurore/sidebar/width = 320
```

```cpp
aurore::ThemeVariant variant;
if (aurore::Theme::loadFile("my_theme.ini", &variant))
    aurore::Application::getPlatform()->setThemeVariant(variant);
```

An app adds its own values with `aurore::Theme::getDarkTheme().addColor("my_app/score", nvgRGB(…))` and `aurore::getStyle().addMetric(…)`, and its XML reads them with `@theme/my_app/score`. With hot reload on, a save of the theme file restyles the screen at once. Aurore draws everything in [Cadran](cadran.md)'s look by default; `aurore::Application::setCadran(false)` before `init()` keeps borealis' own.

## 6. Ship it on the Switch (4 minutes)

Install [devkitPro](https://devkitpro.org/wiki/Getting_Started) with the `switch-dev` group and the portlibs Aurore uses (`switch-glfw`, `switch-mesa`, `switch-glm`; or nothing more with deko3d), then:

```sh
export DEVKITPRO=/opt/devkitpro
cmake -B build_switch -G Ninja -DAURORE_PLATFORM=SWITCH -DAURORE_GRAPHICS=DEKO3D -DCMAKE_BUILD_TYPE=Release
cmake --build build_switch --target aurore_demo.nro
```

The `.nro` holds the program, its icon and its resources (built in with romfs). Copy it to `/switch/` on the SD card and start it from the Homebrew Menu, or send it to a console running the menu with `nxlink -s build_switch/aurore_demo.nro` (the app's logs come back to the terminal). Without `-DAURORE_GRAPHICS=DEKO3D` it builds the OpenGL driver instead. The inspector works there too, with **L3 + R3**.

The same project builds for Windows, macOS, PS4, PS Vita, Android, iOS and a browser: [platforms.md](platforms.md#platforms) has their toolchains and commands, [porting.md](porting.md#porting-an-app) what an app should mind on each.

## Further

- Coming from borealis: [migrating-from-borealis.md](migrating-from-borealis.md#migrating-from-borealis).
- How it works inside (the loop, layout, focus, ownership, threads): [architecture.md](architecture.md#architecture).
- Every XML tag: [views.md](views.md#views), and every attribute: [xml-attributes.md](xml-attributes.md#xml-attributes).
- Every environment variable (the inspector, hot reload, the profiler, the tests): [environment.md](environment.md#environment-variables).
