/*
    Copyright 2020-2021 natinusala
    Copyright 2019 p-sam

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

// Android and iOS start the app through SDL, which renames main()
#if (defined(ANDROID) || defined(IOS)) && defined(__SDL3__)
#include <SDL3/SDL_main.h>
#elif defined(ANDROID) || defined(IOS)
#include <SDL2/SDL_main.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <aurore.hpp>
#include <aurore/cadran.hpp>

#include "activity/main_activity.hpp"
#include "demo_app.hpp"
#include "simulation.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// One frame, called by the browser at each display refresh
static void webFrame()
{
    if (!aurore::Application::mainLoop())
        emscripten_cancel_main_loop();
}
#endif

#if defined(__PSV__) && defined(AURORE_USE_OPENGL)
// Needed for the OpenGL driver to work
extern "C" unsigned int sceLibcHeapSize = 2 * 1024 * 1024;
#endif

using namespace aurore::literals; // for _i18n

namespace
{
// --check-themes: each preset and each theme file of `folder`, its errors
// and its warnings (contrast included); 1 when any has some.
int checkThemeFiles(const std::string& folder)
{
    std::vector<aurore::ThemeInfo> themes = aurore::Theme::builtinThemes();
    // Off the screens: the pictures are decoded, to check them and the text on them
    for (aurore::ThemeInfo& info : aurore::Theme::listThemes(folder, true))
        themes.push_back(std::move(info));
    int status = EXIT_SUCCESS;
    for (aurore::ThemeVariant variant : { aurore::ThemeVariant::LIGHT, aurore::ThemeVariant::DARK })
        for (const std::string& warning : aurore::Theme::checkContrast(variant))
        {
            std::printf("default %s: %s\n", variant == aurore::ThemeVariant::LIGHT ? "light" : "dark", warning.c_str());
            status = EXIT_FAILURE;
        }
    for (const aurore::ThemeInfo& info : themes)
    {
        std::printf("%s (%s): %s%s%s\n", info.name.c_str(), info.path.c_str(), info.light ? "light " : "",
            info.dark ? "dark " : "", info.errors.empty() && info.warnings.empty() ? "ok" : "");
        for (const std::string& error : info.errors)
            std::printf("  error: %s\n", error.c_str());
        for (const std::string& warning : info.warnings)
            std::printf("  warning: %s\n", warning.c_str());
        if (!info.errors.empty() || !info.warnings.empty())
            status = EXIT_FAILURE;
    }
    return status;
}
}

int main(int argc, char* argv[])
{
    // We recommend to use INFO for real apps
    std::string themeFile;
    const char* checkThemes = nullptr;
    const char* simulate    = nullptr;
    for (int i = 1; i < argc; i++)
    {
        if (std::strcmp(argv[i], "-d") == 0)
        { // Set log level
            aurore::Logger::setLogLevel(aurore::LogLevel::LOG_DEBUG);
        }
        else if (std::strcmp(argv[i], "-o") == 0)
        {
            const char* path = (i + 1 < argc) ? argv[++i] : "aurore.log";
            aurore::Logger::setLogOutput(std::fopen(path, "w+"));
        }
        else if (std::strcmp(argv[i], "-v") == 0)
        {
            aurore::Application::enableDebuggingView(true);
        }
        else if (std::strcmp(argv[i], "-t") == 0 && i + 1 < argc)
        { // Theme file
            themeFile = argv[++i];
        }
        else if (std::strcmp(argv[i], "--check-themes") == 0)
        { // Check the presets and a folder, then quit
            checkThemes = (i + 1 < argc) ? argv[++i] : "@res/themes";
        }
        else if (std::strcmp(argv[i], "--simulate") == 0 && i + 1 < argc)
        { // A console's screen, drawn around it (switch, vita, ps4, 3ds, phone)
            simulate = argv[++i];
        }
        else if (std::strcmp(argv[i], "-r") == 0)
        { // Redraw on demand
            aurore::Application::setRedrawOnDemand(true);
        }
    }

    aurore::Platform::APP_LOCALE_DEFAULT = aurore::LOCALE_AUTO;

    // Before init(): Cadran reads its preferences from it on
    demo::installHooks();
    // Before init() too: the simulated console's layout preset
    demo::simulation::choose(simulate);

    // Init the app and i18n
    if (!aurore::Application::init())
    {
        aurore::Logger::error("Unable to init Aurore application");
        return EXIT_FAILURE;
    }

    aurore::Application::createWindow("demo/title"_i18n);
    demo::simulation::start();

    // Out at once: nothing of the app to close
    if (checkThemes)
    {
        int status = checkThemeFiles(checkThemes);
        std::fflush(stdout);
        std::_Exit(status);
    }

#ifdef AURORE_USE_FRIBIDI
    // The bundled font has neither Hebrew nor Arabic: a system font for the
    // scripts tab
    for (const char* path : { "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/TTF/DejaVuSans.ttf" })
    {
        if (std::FILE* file = std::fopen(path, "rb"))
        {
            std::fclose(file);
            if (aurore::Application::loadFontFromFile("rtl", path))
                nvgAddFallbackFontId(aurore::Application::getNVGContext(), aurore::Application::getFont(aurore::FONT_REGULAR), aurore::Application::getFont("rtl"));
            break;
        }
    }
#endif

    aurore::ThemeVariant variant;
    if (!themeFile.empty() && aurore::Theme::loadFile(themeFile, &variant))
        aurore::Application::getPlatform()->setThemeVariant(variant);

    // The inspector: F12, or L3 + R3 on a controller (docs/inspector.md)
    aurore::Application::setInspectorEnabled(true);

    // Have the application register an action on every activity that will quit when you press BUTTON_START
    aurore::Application::setGlobalQuit(false);

    // The demo's own views, theme colours and metrics
    demo::registerViews();

    // Create and push the main activity to the stack
    aurore::Application::pushActivity(new MainActivity());

    // Run the app
#ifdef __EMSCRIPTEN__
    // In a browser a frame hands the thread back to the page (web/README.md):
    // the browser calls one per display refresh, and main() returns at once
    emscripten_set_main_loop(webFrame, 0, false);
    return EXIT_SUCCESS;
#else
    while (aurore::Application::mainLoop())
        ;
#endif

    aurore::Logger::info("{} frames drawn", aurore::Application::getDrawnFrameCount());

    // Exit
    return EXIT_SUCCESS;
}

#ifdef __WINRT__
#include <aurore/core/main.hpp>
#endif
