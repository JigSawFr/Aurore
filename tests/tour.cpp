/*
    Copyright 2026 JigSawFr

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

// aurore-tour: the demo's screens drawn offscreen to PNGs by the test
// harness, without a window: every tab of the rail, its content scrolled a
// page at a time, and the screens and dialogs it opens. For looking at the
// demo in one language, theme and size at once, and for the website's
// pictures.
//
//   aurore-tour [--theme dark|light|<theme file>] [--variant light|dark] [--lang <code>]
//               [--size WxH] [--small] [-o folder] [--film folder] [scene...]
//
// `--small`: a small screen's layout, 480x360 units at the dense density
// (what AURORE_DESIGN_SIZE=480x360 AURORE_DENSITY=dense does in a window),
// drawn on 960x720 pixels unless --size says otherwise.
// With scene names (as the files are named, without ".png", or a prefix of
// them), only those; `--list` lists them. Also prints the texts cut short
// ("…") on each picture. `--film folder`: the readme-tour scene writes a
// picture there every 1/12 s (.github/readme/capture.sh makes them a GIF).

#include <aurore.hpp>
#include <aurore/cadran.hpp>
#include <aurore/test/harness.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "activity/main_activity.hpp"
#include "demo_app.hpp"

namespace
{
using aurore::test::Harness;

struct Scene
{
    std::string name;
    std::function<void(Harness&)> run; // from the main screen, the rail on its first tab
};

int usage(const char* program)
{
    std::fprintf(stderr, "usage: %s [--theme dark|light|<theme file>] [--lang <code>] [--variant light|dark] [--size WxH] [--small] [-o folder] [--film folder] [--list] [scene...]\n",
        program);
    return 2;
}

aurore::TabFrame* tabs(Harness& h)
{
    return dynamic_cast<aurore::TabFrame*>(h.top()->getView("main_tabs"));
}

// The tab at `position` of the rail (separators counted), shown
void openTab(Harness& h, int position)
{
    tabs(h)->focusTab(position);
    h.settle();
}

// Into the tab, then `downs` presses down
void enter(Harness& h, int position, int downs = 0)
{
    openTab(h, position);
    h.tap(aurore::BUTTON_RIGHT);
    for (int i = 0; i < downs; i++)
        h.tap(aurore::BUTTON_DOWN);
    h.settle();
}

// The view showing the text of `key` pressed (a button, a line)
void press(Harness& h, const char* key)
{
    aurore::View* view = h.findText(aurore::getStr(key));
    if (!view)
    {
        std::printf("no view says %s\n", key);
        return;
    }
    // Focused, then A: the view may be out of sight, down the tab
    while (view && !view->isFocusable())
        view = view->hasParent() ? view->getParent() : nullptr;
    if (!view)
        return;
    aurore::Application::giveFocus(view);
    h.settle();
    h.tap(aurore::BUTTON_A);
}

// How many pictures of each tab beyond the first, a picture every how many
// presses down
int tabPages(int tab)
{
    switch (tab)
    {
        case demo::TAB_HOME:
            return 3;
        case demo::TAB_COMPONENTS:
            return 7;
        case demo::TAB_LIBRARY:
            return 1;
        default:
            return 4;
    }
}

int tabStep(int tab)
{
    switch (tab)
    {
        case demo::TAB_HOME:
            return 2;
        case demo::TAB_LIBRARY:
            return 2;
        default:
            return 3;
    }
}

// The film of readme-tour: a picture every 1/12 s of simulated time, numbered,
// into this folder (ffmpeg makes the README's tour.gif of them)
std::string filmFolder;
int filmFrame = 0;

void film(Harness& h, double ms)
{
    const double step = 1000.0 / 12;
    for (double t = 0; t < ms; t += step)
    {
        h.advance(step);
        if (filmFolder.empty())
            continue;
        char name[32];
        std::snprintf(name, sizeof(name), "/%04d.png", filmFrame++);
        h.screenshot().savePng(filmFolder + name);
    }
}

// A button pressed, then the film running on
void filmTap(Harness& h, aurore::ControllerButton button, double ms)
{
    h.press(button);
    film(h, 100);
    h.release(button);
    film(h, ms);
}

// The screens and dialogs worth a picture of their own, from their tab shown
std::vector<Scene> extraScenes()
{
    return {
        // The README's pictures and the website's gallery (.github/readme/capture.sh)
        { "readme-cadran", [](Harness& h)
            { enter(h, demo::TAB_HOME); } },
        { "readme-sheet", [](Harness& h)
            {
            enter(h, demo::TAB_COMPONENTS);
            press(h, "demo/cadran/sheet"); } },
        { "readme-picker", [](Harness& h)
            {
            enter(h, demo::TAB_COMPONENTS);
            press(h, "demo/cadran/pick"); } },
        { "readme-checklist", [](Harness& h)
            {
            enter(h, demo::TAB_COMPONENTS);
            press(h, "demo/cadran/checklist"); } },
        { "readme-markdown", [](Harness& h)
            { enter(h, demo::TAB_COMPONENTS, 30); } },
        { "readme-components", [](Harness& h)
            {
            enter(h, demo::TAB_LIBRARY);
            h.tap(aurore::BUTTON_DOWN); } },
        { "readme-palettes", [](Harness& h)
            {
            enter(h, demo::TAB_THEMES);
            press(h, "demo/themes/picker"); } },
        { "readme-tour", [](Harness& h)
            {
            // From the rail through the tabs, into a game and back
            openTab(h, demo::TAB_HOME);
            film(h, 1600);
            filmTap(h, aurore::BUTTON_RIGHT, 1200);
            filmTap(h, aurore::BUTTON_LEFT, 500);
            filmTap(h, aurore::BUTTON_DOWN, 900);
            filmTap(h, aurore::BUTTON_RIGHT, 500);
            filmTap(h, aurore::BUTTON_DOWN, 700);
            filmTap(h, aurore::BUTTON_RIGHT, 500);
            filmTap(h, aurore::BUTTON_A, 1800);
            filmTap(h, aurore::BUTTON_DOWN, 1000);
            filmTap(h, aurore::BUTTON_B, 900);
            for (int i = 0; i < 3; i++)
                filmTap(h, aurore::BUTTON_LEFT, 250);
            filmTap(h, aurore::BUTTON_DOWN, 1000);
            filmTap(h, aurore::BUTTON_RIGHT, 600);
            for (int i = 0; i < 5; i++)
                filmTap(h, aurore::BUTTON_DOWN, 450);
            filmTap(h, aurore::BUTTON_LEFT, 500);
            filmTap(h, aurore::BUTTON_DOWN, 1000);
            filmTap(h, aurore::BUTTON_RIGHT, 500);
            filmTap(h, aurore::BUTTON_A, 900);
            film(h, 900);
            filmTap(h, aurore::BUTTON_LEFT, 500);
            filmTap(h, aurore::BUTTON_DOWN, 900);
            filmTap(h, aurore::BUTTON_DOWN, 900);
            filmTap(h, aurore::BUTTON_DOWN, 900); } },
        { "library-game", [](Harness& h)
            {
            enter(h, demo::TAB_LIBRARY);
            h.tap(aurore::BUTTON_DOWN);
            h.tap(aurore::BUTTON_A); } },
        { "library-game-2", [](Harness& h)
            {
            enter(h, demo::TAB_LIBRARY);
            h.tap(aurore::BUTTON_DOWN);
            h.tap(aurore::BUTTON_A);
            h.tap(aurore::BUTTON_DOWN, 3); } },
        { "library-genre", [](Harness& h)
            {
            enter(h, demo::TAB_LIBRARY);
            h.tap(aurore::BUTTON_RIGHT);
            h.tap(aurore::BUTTON_A); } },
        { "library-menu", [](Harness& h)
            {
            enter(h, demo::TAB_LIBRARY);
            h.tap(aurore::BUTTON_DOWN);
            h.tap(aurore::BUTTON_RIGHT);
            h.tap(aurore::BUTTON_X); } },
        { "library-list", [](Harness& h)
            {
            enter(h, demo::TAB_LIBRARY);
            h.tap(aurore::BUTTON_RIGHT, 2);
            h.tap(aurore::BUTTON_A);
            h.tap(aurore::BUTTON_DOWN, 4); } },
        { "components-settings", [](Harness& h)
            {
            enter(h, demo::TAB_COMPONENTS);
            press(h, "demo/components/open_settings"); } },
        { "components-confirm", [](Harness& h)
            {
            enter(h, demo::TAB_COMPONENTS);
            press(h, "demo/cadran/confirm"); } },
        { "components-sheet", [](Harness& h)
            {
            enter(h, demo::TAB_COMPONENTS);
            press(h, "demo/cadran/sheet"); } },
        { "components-menu", [](Harness& h)
            {
            enter(h, demo::TAB_COMPONENTS);
            h.tapView(h.find("demo/more"));
            h.tap(aurore::BUTTON_A); } },
        { "components-toast", [](Harness& h)
            {
            enter(h, demo::TAB_COMPONENTS);
            press(h, "demo/cadran/toast_done");
            h.advance(400); } },
        { "motion-transform", [](Harness& h)
            {
            enter(h, demo::TAB_MOTION);
            press(h, "demo/motion/transform"); } },
        { "motion-played", [](Harness& h)
            {
            enter(h, demo::TAB_MOTION);
            h.tap(aurore::BUTTON_A);
            h.advance(500); } },
        { "text-lab", [](Harness& h)
            {
            enter(h, demo::TAB_TEXT);
            press(h, "demo/text/lab_open"); } },
        { "text-largest", [](Harness& h)
            {
            enter(h, demo::TAB_TEXT);
            press(h, "cadran/prefs/text_sizes/largest");
            h.tap(aurore::BUTTON_UP); } },
        { "input-testing", [](Harness& h)
            {
            enter(h, demo::TAB_INPUT);
            h.tap(aurore::BUTTON_A);
            h.press(aurore::BUTTON_X);
            h.press(aurore::BUTTON_RB);
            h.setAxis(aurore::LEFT_X, 0.8f);
            h.setAxis(aurore::LEFT_Y, -0.5f);
            h.step(4); } },
        { "input-touch", [](Harness& h)
            {
            enter(h, demo::TAB_INPUT);
            h.tap(aurore::BUTTON_DOWN);
            aurore::View* pad = h.findText(aurore::getStr("demo/input/touch_here"))->getParent();
            aurore::Rect frame = pad->getFrame();
            h.swipe(frame.getMinX() + 80, frame.getMinY() + 160, frame.getMaxX() - 120, frame.getMinY() + 60, 300);
            h.touchDown(frame.getMinX() + 300, frame.getMinY() + 120);
            h.step(2);
            h.touchUp();
            h.step(6); } },
        { "developers-layout", [](Harness& h)
            {
            enter(h, demo::TAB_DEVELOPERS);
            press(h, "demo/developers/layout"); } },
        { "themes-light", [](Harness& h)
            {
            enter(h, demo::TAB_THEMES);
            press(h, "cadran/tools/themes/light");
            h.tap(aurore::BUTTON_DOWN); } },
        { "themes-boreale", [](Harness& h)
            {
            ui::prefs().theme_file = "@res/themes/boreale.theme";
            live_prefs::apply_theme();
            enter(h, demo::TAB_HOME); } },
        { "themes-boreale-library", [](Harness& h)
            {
            ui::prefs().theme_file = "@res/themes/boreale.theme";
            live_prefs::apply_theme();
            enter(h, demo::TAB_LIBRARY);
            h.tap(aurore::BUTTON_DOWN); } },
    };
}

std::vector<Scene> scenes()
{
    std::vector<Scene> list;
    list.push_back({ "rail", [](Harness& h)
        { openTab(h, 0); } });
    // Each tab, from the rail, then a page further down at a time
    for (int tab = 0; tab < demo::TAB_COUNT; tab++)
    {
        if (demo::isSeparator(tab))
            continue;
        char name[32];
        std::snprintf(name, sizeof(name), "tab%02d-%s", tab, demo::tabName(tab));
        list.push_back({ name, [tab](Harness& h)
            { enter(h, tab); } });
        for (int page = 1; page <= tabPages(tab); page++)
        {
            char more[48];
            std::snprintf(more, sizeof(more), "tab%02d-%s-%d", tab, demo::tabName(tab), page);
            list.push_back({ more, [tab, page](Harness& h)
                { enter(h, tab, page * tabStep(tab)); } });
        }
    }
    for (const Scene& scene : extraScenes())
        list.push_back(scene);
    return list;
}
} // namespace

int main(int argc, char** argv)
{
    std::setvbuf(stdout, nullptr, _IOLBF, 0); // a line as each picture is written
    std::string theme = "dark", lang = "en-US", output = "tour", variant;
    int width = 1280, height = 720;
    bool list = false, small = false, sized = false;
    std::vector<std::string> only;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        auto next       = [&]()
        { return i + 1 < argc ? std::string(argv[++i]) : std::string(); };
        if (arg == "--theme")
            theme = next();
        else if (arg == "--variant")
            variant = next();
        else if (arg == "--lang")
            lang = next();
        else if (arg == "--size")
        {
            std::string size = next();
            if (std::sscanf(size.c_str(), "%dx%d", &width, &height) != 2 || width <= 0 || height <= 0)
                return usage(argv[0]);
            sized = true;
        }
        else if (arg == "--small")
            small = true;
        else if (arg == "-o")
            output = next();
        else if (arg == "--list")
            list = true;
        else if (arg == "--film")
            filmFolder = next();
        else if (arg == "-h" || arg == "--help" || arg[0] == '-')
            return usage(argv[0]);
        else
            only.push_back(arg);
    }

    std::vector<Scene> all = scenes();
    if (list)
    {
        for (const Scene& scene : all)
            std::printf("%s\n", scene.name.c_str());
        return 0;
    }
    std::filesystem::create_directories(output);
    if (!filmFolder.empty())
        std::filesystem::create_directories(filmFolder);
    aurore::Logger::setLogLevel(aurore::LogLevel::LOG_ERROR);

    // One harness per process: each scene in a child process of its own
    // would cost a start each; the main screen pushed again is enough
    static ui::Prefs prefs;
    bool themeFile = theme != "dark" && theme != "light";
    demo::installHooks();
    prefs.theme    = !variant.empty() ? variant : themeFile ? "dark"
                                                            : theme;
    prefs.language = lang;
    ui::use_prefs(prefs);

    if (small)
    {
        aurore::Application::ORIGINAL_WINDOW_WIDTH  = 480;
        aurore::Application::ORIGINAL_WINDOW_HEIGHT = 360;
        aurore::setDensity(aurore::DENSITY_DENSE);
        if (!sized)
            width = 960, height = 720;
    }

    aurore::test::Options options;
    options.width       = width;
    options.height      = height;
    options.locale      = lang;
    options.theme       = theme == "light" || variant == "light" ? aurore::ThemeVariant::LIGHT : aurore::ThemeVariant::DARK;
    options.outputDir   = output;
    options.systemFonts = true; // Chinese, Japanese and Korean
    Harness h(options);
    demo::registerViews();
    if (themeFile)
    {
        prefs.theme_file = theme;
        live_prefs::apply_theme();
    }

    int cutTotal = 0;
    for (const Scene& scene : all)
    {
        bool wanted = only.empty();
        for (const std::string& name : only)
            wanted = wanted || scene.name.rfind(name, 0) == 0;
        if (!wanted)
            continue;
        // What the scene before opened closed, then a main screen anew
        while (aurore::Application::getActivitiesStack().size() > 1)
        {
            aurore::Application::popActivity(aurore::TransitionAnimation::NONE);
            h.settle();
        }
        // The preferences a scene before changed, as they were
        prefs.text_size  = "auto";
        prefs.theme      = !variant.empty() ? variant : themeFile ? "dark"
                                                                  : theme;
        prefs.theme_file = themeFile ? theme : "";
        text_size::apply();
        live_prefs::apply_theme();
        h.push(new MainActivity());
        h.settle();
        scene.run(h);
        if (!h.settle())
            std::printf("%s: still moving after 5 s\n", scene.name.c_str());
        std::string path = output + "/" + scene.name + ".png";
        h.screenshot().savePng(path);
        std::vector<std::string> cut = h.cutTexts();
        aurore::View* focus          = h.focus();
        std::printf("%s  (focus: %s \"%s\")%s\n", path.c_str(), focus ? focus->describe().c_str() : "none",
            h.focusText().c_str(), cut.empty() ? "" : "  (cut texts below)");
        for (const std::string& text : cut)
            std::printf("    cut: %s\n", text.c_str());
        cutTotal += (int)cut.size();
    }
    std::printf("%d cut text(s)\n", cutTotal);
    std::fflush(stdout);
    std::_Exit(0);
}
