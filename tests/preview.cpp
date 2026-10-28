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

// aurore-preview: an XML layout drawn offscreen to a PNG, by the test
// harness, without a window.
//
//   aurore-preview layout.xml [--theme dark|light|<theme file>] [--lang <code>]
//                  [--size WxH] [-o out.png]
//
// The XML may use borealis' views, Cadran's and the demo's (an app with
// views of its own builds the same tool with aurore::test::Harness, after
// registering them). Resources are the demo's: "@res/..." and i18n strings.

#include <aurore.hpp>
#include <aurore/cadran.hpp>
#include <aurore/test/harness.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <string>

#include "demo_app.hpp"

static int usage(const char* program)
{
    std::fprintf(stderr,
        "usage: %s layout.xml [--theme dark|light|<theme file>] [--lang <code>] [--size WxH] [-o out.png]\n", program);
    return 2;
}

int main(int argc, char** argv)
{
    std::string xml, theme = "dark", lang = "en-US", output;
    int width = 1280, height = 720;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        auto next       = [&]()
        { return i + 1 < argc ? std::string(argv[++i]) : std::string(); };
        if (arg == "--theme")
            theme = next();
        else if (arg == "--lang")
            lang = next();
        else if (arg == "--size")
        {
            std::string size = next();
            if (std::sscanf(size.c_str(), "%dx%d", &width, &height) != 2 || width <= 0 || height <= 0)
                return usage(argv[0]);
        }
        else if (arg == "-o")
            output = next();
        else if (arg == "-h" || arg == "--help")
            return usage(argv[0]);
        else if (xml.empty() && arg[0] != '-')
            xml = arg;
        else
            return usage(argv[0]);
    }
    if (xml.empty())
        return usage(argv[0]);
    if (!std::filesystem::exists(xml))
    {
        std::fprintf(stderr, "%s: no such file\n", xml.c_str());
        return 1;
    }
    if (output.empty())
        output = std::filesystem::path(xml).stem().string() + ".png";

    aurore::Logger::setLogLevel(aurore::LogLevel::LOG_WARNING);

    // Cadran's preferences: the theme and language asked for
    static ui::Prefs prefs;
    bool themeFile = theme != "dark" && theme != "light";
    prefs.theme    = themeFile ? "dark" : theme;
    prefs.language = lang;
    ui::use_prefs(prefs);
    demo::installHooks();
    ui::use_prefs(prefs); // after the demo's, which sets its own

    aurore::test::Options options;
    options.width  = width;
    options.height = height;
    options.locale = lang;
    options.theme  = theme == "light" ? aurore::ThemeVariant::LIGHT : aurore::ThemeVariant::DARK;
    aurore::test::Harness h(options);
    demo::registerViews();

    if (themeFile)
    {
        aurore::ThemeVariant variant;
        if (!aurore::Theme::loadFile(theme, &variant))
        {
            std::fprintf(stderr, "%s: not a theme file\n", theme.c_str());
            return 1;
        }
        aurore::Application::getPlatform()->setThemeVariant(variant);
    }

    // A mistake in the file: its message (file:line, what was expected) and
    // the exit code 1, not an abort
    aurore::View* view = nullptr;
    try
    {
        view = aurore::View::createFromXMLFile(xml);
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }
    if (!view)
    {
        std::fprintf(stderr, "%s: could not be read\n", xml.c_str());
        return 1;
    }
    h.push(new aurore::Activity(view));
    h.settle();

    aurore::test::Image image = h.screenshot();
    if (!image.savePng(output))
    {
        std::fprintf(stderr, "%s: could not be written\n", output.c_str());
        return 1;
    }
    std::printf("%s (%dx%d)\n", output.c_str(), image.width, image.height);
    return 0;
}
