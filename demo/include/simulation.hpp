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

// The demo's simulated platforms, on the desktop: the app laid out as on a
// console (its screen's size and proportions, the layout preset, the
// renderer's antialiasing) and the console drawn around the screen
// (aurore::ScreenFrame). Chosen with --simulate <id> or AURORE_SIMULATE, or
// on the Developers tab. The consoles are Boréale's, the one of its sites
// (PlayGuard's site.css, .device and DEVICE_GRIPS): plain shapes in the
// night's blues, no maker's logo, name nor colours.

#pragma once

#include <nanovg.h>
#include <string>
#include <vector>

namespace demo::simulation
{

struct Device
{
    const char* id; // for --simulate and AURORE_SIMULATE
    const char* name; // as the picker lists it (a platform's name, or an i18n key starting with "demo/")
    int screenWidth; // the app's screen, in the console's pixels
    int screenHeight;
    bool smallScreen; // aurore::PRESET_SMALL_SCREEN rather than PRESET_STANDARD
    bool antialias; // the console's renderer draws antialiased edges
};

/// Every platform, the desktop (no frame) first
const std::vector<Device>& devices();
/// The one with this id, nullptr when none
const Device* find(const std::string& id);
/// The one in use ("desktop" until another is chosen)
const Device& current();
/// What the picker shows for it
std::string displayName(const Device& device);
/// The console drawn in `x, y, width, height` (a picture of it, the Home
/// tab's): its screen a night with the aurora, as the frame draws the rest
void drawPreview(NVGcontext* vg, const Device& device, float x, float y, float width, float height);

/// Before Application::init(): the one from the command line or
/// AURORE_SIMULATE, its layout preset given to Aurore. `fromCommandLine` wins.
void choose(const char* fromCommandLine);
/// After Application::createWindow(): the frame of the one chosen, the
/// window sized for it
void start();
/// Another one while the app runs: its preset and frame, the screens built
/// again
void use(const std::string& id);

} // namespace demo::simulation
