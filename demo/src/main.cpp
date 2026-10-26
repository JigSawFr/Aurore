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

int main(int argc, char* argv[])
{
    // We recommend to use INFO for real apps
    std::string themeFile;
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

    brls::ThemeVariant variant;
    if (!themeFile.empty() && brls::Theme::loadFile(themeFile, &variant))
        brls::Application::getPlatform()->setThemeVariant(variant);

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
