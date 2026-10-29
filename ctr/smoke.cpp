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

// aurore_3ds_smoke: Aurore on the New 3DS before its renderer. A layout made
// from XML with the small screen preset, laid out by yoga, its sizes written
// on libctru's text console (the top screen). START quits. No window, no
// drawing: it shows the library builds and runs on the console
// (docs/platforms.md).

#include <3ds.h>

#include <yoga/Yoga.h>
#include <cstdio>
#include <exception>
#include <aurore.hpp>
#include <aurore/version.hpp>

namespace
{

const char* const LAYOUT = R"xml(
<aurore:Box axis="row" compact:axis="column" width="100%" padding="16dp">
    <aurore:Box id="first" grow="1" height="40dp" marginBottom="8dp" />
    <aurore:Box id="second" grow="1" height="40dp" compact:height="24dp" />
</aurore:Box>
)xml";

const char* widthClassName(aurore::WidthClass widthClass)
{
    switch (widthClass)
    {
        case aurore::WidthClass::COMPACT:
            return "compact";
        case aurore::WidthClass::MEDIUM:
            return "medium";
        default:
            return "expanded";
    }
}

void printFrame(aurore::View* view)
{
    YGNodeRef node = view->getYGNode();
    std::printf("  %-7s %5.1f %5.1f %5.1f x %5.1f\n", view->getId().c_str(), YGNodeLayoutGetLeft(node),
        YGNodeLayoutGetTop(node), YGNodeLayoutGetWidth(node), YGNodeLayoutGetHeight(node));
}

void layOut()
{
    // What the 3DS platform asks Aurore for (CtrPlatform::getLayoutPreset)
    aurore::Application::usePreset(aurore::PRESET_SMALL_SCREEN);
    std::printf("Aurore %s on the New 3DS\n\n", AURORE_VERSION);
    std::printf("Design size  %ux%u\n", aurore::Application::ORIGINAL_WINDOW_WIDTH,
        aurore::Application::ORIGINAL_WINDOW_HEIGHT);
    std::printf("Density      %.2f\n", aurore::getDensity());
    std::printf("Width class  %s\n", widthClassName(aurore::Application::getWidthClass()));
    std::printf("Text scale   %.2f\n\n", aurore::Application::getPresetTextScale());

    aurore::View* root = aurore::View::createFromXMLString(LAYOUT);
    YGNodeCalculateLayout(root->getYGNode(), (float)aurore::Application::ORIGINAL_WINDOW_WIDTH,
        (float)aurore::Application::ORIGINAL_WINDOW_HEIGHT, YGDirectionLTR);
    std::printf("Layout (x, y, width x height):\n");
    for (const char* id : { "first", "second" })
        if (aurore::View* view = root->getView(id))
            printFrame(view);
    delete root;
}

} // namespace

int main(int argc, char** argv)
{
    // The application and its main loop linked in (never run here: no
    // renderer yet), so the build checks the whole library resolves on the 3DS
    if (argc < 0)
    {
        aurore::Application::init();
        aurore::Application::createWindow("Aurore");
        while (aurore::Application::mainLoop())
            ;
    }

    gfxInitDefault();
    consoleInit(GFX_TOP, nullptr);

    try
    {
        layOut();
    }
    catch (const std::exception& e)
    {
        std::printf("\nError: %s\n", e.what());
    }
    std::printf("\nPress START to quit.\n");

    while (aptMainLoop())
    {
        hidScanInput();
        if (hidKeysDown() & KEY_START)
            break;
        gspWaitForVBlank();
        gfxSwapBuffers();
    }

    gfxExit();
    return 0;
}
