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

#if defined(ANDROID) || defined(IOS)
#include <SDL2/SDL_main.h>
#endif

#include <borealis.hpp>
#include <borealis/cadran.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "activity/main_activity.hpp"
#include "demo_app.hpp"

#if defined(__PSV__) && defined(BOREALIS_USE_OPENGL)
// Needed for the OpenGL driver to work
extern "C" unsigned int sceLibcHeapSize = 2 * 1024 * 1024;
#endif

using namespace brls::literals; // for _i18n

namespace
{
// --check-themes: each preset and each theme file of `folder`, its errors
// and its warnings (contrast included); 1 when any has some.
int checkThemeFiles(const std::string& folder)
{
    std::vector<brls::ThemeInfo> themes = brls::Theme::builtinThemes();
    for (brls::ThemeInfo& info : brls::Theme::listThemes(folder))
        themes.push_back(std::move(info));
    int status = EXIT_SUCCESS;
    for (brls::ThemeVariant variant : { brls::ThemeVariant::LIGHT, brls::ThemeVariant::DARK })
        for (const std::string& warning : brls::Theme::checkContrast(variant))
        {
            std::printf("default %s: %s\n", variant == brls::ThemeVariant::LIGHT ? "light" : "dark", warning.c_str());
            status = EXIT_FAILURE;
        }
    for (const brls::ThemeInfo& info : themes)
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
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-d") == 0) { // Set log level
            brls::Logger::setLogLevel(brls::LogLevel::LOG_DEBUG);
        } else if (std::strcmp(argv[i], "-o") == 0) {
            const char* path = (i + 1 < argc) ? argv[++i] : "borealis.log";
            brls::Logger::setLogOutput(std::fopen(path, "w+"));
        } else if (std::strcmp(argv[i], "-v") == 0) {
            brls::Application::enableDebuggingView(true);
        } else if (std::strcmp(argv[i], "-t") == 0 && i + 1 < argc) { // Theme file
            themeFile = argv[++i];
        } else if (std::strcmp(argv[i], "--check-themes") == 0) { // Check the presets and a folder, then quit
            checkThemes = (i + 1 < argc) ? argv[++i] : "@res/themes";
        } else if (std::strcmp(argv[i], "-r") == 0) { // Redraw on demand
            brls::Application::setRedrawOnDemand(true);
        }
    }

    brls::Platform::APP_LOCALE_DEFAULT = brls::LOCALE_AUTO;

    // Before init(): Cadran reads its preferences from it on
    demo::installHooks();

    // Init the app and i18n
    if (!brls::Application::init())
    {
        brls::Logger::error("Unable to init Borealis application");
        return EXIT_FAILURE;
    }

    brls::Application::createWindow("demo/title"_i18n);

    // Out at once: nothing of the app to close
    if (checkThemes)
    {
        int status = checkThemeFiles(checkThemes);
        std::fflush(stdout);
        std::_Exit(status);
    }

#ifdef BRLS_USE_FRIBIDI
    // The bundled font has neither Hebrew nor Arabic: a system font for the
    // scripts tab
    for (const char* path : { "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/TTF/DejaVuSans.ttf" })
    {
        if (std::FILE* file = std::fopen(path, "rb"))
        {
            std::fclose(file);
            if (brls::Application::loadFontFromFile("rtl", path))
                nvgAddFallbackFontId(brls::Application::getNVGContext(), brls::Application::getFont(brls::FONT_REGULAR), brls::Application::getFont("rtl"));
            break;
        }
    }
#endif

    brls::ThemeVariant variant;
    if (!themeFile.empty() && brls::Theme::loadFile(themeFile, &variant))
        brls::Application::getPlatform()->setThemeVariant(variant);

    // The inspector: F12, or L3 + R3 on a controller (docs/inspector.md)
    brls::Application::setInspectorEnabled(true);

    // Have the application register an action on every activity that will quit when you press BUTTON_START
    brls::Application::setGlobalQuit(false);

    // The demo's own views, theme colours and metrics
    demo::registerViews();

    // Create and push the main activity to the stack
    brls::Application::pushActivity(new MainActivity());

    // Run the app
    while (brls::Application::mainLoop())
        ;

    brls::Logger::info("{} frames drawn", brls::Application::getDrawnFrameCount());

    // Exit
    return EXIT_SUCCESS;
}

#ifdef __WINRT__
#include <borealis/core/main.hpp>
#endif
