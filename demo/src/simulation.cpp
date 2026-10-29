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

#include "simulation.hpp"

#include <algorithm>
#include <cstring>
#include <ctime>
#include <aurore.hpp>
#include <aurore/cadran/ui/live_prefs.hpp>
#include <aurore/core/time.hpp>
#include <aurore/core/util.hpp>

namespace demo::simulation
{

namespace
{
    // The consoles' shapes, in their screen's pixels: the screen from (0, 0)
    // to (screenWidth, screenHeight), the body around it
    struct Margins
    {
        float left, top, right, bottom;
    };

    // Plain colours: a night-blue body, darker glass, two soft accents
    const NVGcolor BACKGROUND_TOP    = nvgRGB(0x0B, 0x10, 0x20);
    const NVGcolor BACKGROUND_BOTTOM = nvgRGB(0x16, 0x1D, 0x30);
    const NVGcolor BODY              = nvgRGB(0x23, 0x2A, 0x3B);
    const NVGcolor BODY_EDGE         = nvgRGB(0x34, 0x3D, 0x53);
    const NVGcolor GLASS             = nvgRGB(0x09, 0x0C, 0x13);
    const NVGcolor CONTROL           = nvgRGB(0x14, 0x19, 0x25);
    const NVGcolor CONTROL_EDGE      = nvgRGB(0x3C, 0x46, 0x5C);
    const NVGcolor TEAL              = nvgRGB(0x1E, 0x6B, 0x68);
    const NVGcolor VIOLET            = nvgRGB(0x4A, 0x3F, 0x8A);
    const NVGcolor LIGHT             = nvgRGB(0x2D, 0xD4, 0xBF);

    const std::vector<Device> DEVICES = {
        { "desktop", "demo/developers/simulate_desktop", 1280, 720, false, true },
        { "switch", "Switch", 1280, 720, false, true },
        { "vita", "PS Vita", 960, 544, false, true }, // 4x MSAA rather than nanovg's edges
        { "ps4", "PS4", 1920, 1080, false, true },
        { "3ds", "New 3DS", 320, 240, true, false },
        { "phone", "demo/developers/simulate_phone", 1600, 720, false, true },
    };

    const Device* chosen  = &DEVICES[0];
    int defaultAntialias  = -1; // the context's, before any simulation
    bool windowSizeChosen = false;

    // From the console's pixels to the window's
    struct Map
    {
        float x0, y0, k;
        float x(float v) const { return x0 + v * k; }
        float y(float v) const { return y0 + v * k; }
        float s(float v) const { return v * k; }
    };

    Margins marginsOf(const Device& d)
    {
        if (!std::strcmp(d.id, "switch"))
            return { 296, 46, 296, 46 };
        if (!std::strcmp(d.id, "vita"))
            return { 330, 80, 330, 80 };
        if (!std::strcmp(d.id, "ps4"))
            return { 28, 28, 28, 282 };
        if (!std::strcmp(d.id, "3ds"))
            return { 112, 378, 112, 60 };
        return { 40, 40, 40, 40 }; // phone
    }

    void roundRect(NVGcontext* vg, const Map& m, float x0, float y0, float x1, float y1, float r)
    {
        nvgRoundedRect(vg, m.x(x0), m.y(y0), m.s(x1 - x0), m.s(y1 - y0), m.s(r));
    }

    // The screen cut out of the shape just added: it stays the app's
    void screenHole(NVGcontext* vg, const aurore::Rect& screen)
    {
        nvgRect(vg, screen.getMinX(), screen.getMinY(), screen.getWidth(), screen.getHeight());
        nvgPathWinding(vg, NVG_HOLE);
    }

    void fill(NVGcontext* vg, NVGcolor color, bool edge = false)
    {
        nvgFillColor(vg, color);
        nvgFill(vg);
        if (edge)
        {
            nvgStrokeColor(vg, BODY_EDGE);
            nvgStrokeWidth(vg, 1.5f);
            nvgStroke(vg);
        }
    }

    // A round button, a little raised
    void button(NVGcontext* vg, const Map& m, float x, float y, float r, NVGcolor color = CONTROL)
    {
        nvgBeginPath(vg);
        nvgCircle(vg, m.x(x), m.y(y), m.s(r));
        nvgFillColor(vg, color);
        nvgFill(vg);
        nvgStrokeColor(vg, CONTROL_EDGE);
        nvgStrokeWidth(vg, std::max(1.0f, m.s(2)));
        nvgStroke(vg);
    }

    // A stick: its well and its cap
    void stick(NVGcontext* vg, const Map& m, float x, float y, float r)
    {
        button(vg, m, x, y, r, GLASS);
        nvgBeginPath(vg);
        nvgCircle(vg, m.x(x), m.y(y), m.s(r * 0.72f));
        nvgFillPaint(vg, nvgRadialGradient(vg, m.x(x - r * 0.2f), m.y(y - r * 0.2f), 0, m.s(r * 0.8f), nvgRGB(0x3A, 0x43, 0x58), CONTROL));
        nvgFill(vg);
    }

    // Four buttons in a diamond
    void diamond(NVGcontext* vg, const Map& m, float x, float y, float gap, float r)
    {
        button(vg, m, x, y - gap, r);
        button(vg, m, x + gap, y, r);
        button(vg, m, x, y + gap, r);
        button(vg, m, x - gap, y, r);
    }

    // A cross pad
    void cross(NVGcontext* vg, const Map& m, float x, float y, float arm, float width)
    {
        nvgBeginPath(vg);
        roundRect(vg, m, x - width / 2, y - arm, x + width / 2, y + arm, width * 0.2f);
        roundRect(vg, m, x - arm, y - width / 2, x + arm, y + width / 2, width * 0.2f);
        fill(vg, CONTROL);
    }

    void drawSwitch(NVGcontext* vg, const Map& m, const aurore::Rect& screen)
    {
        // The two controllers on the sides, then the tablet
        nvgBeginPath(vg);
        nvgRoundedRectVarying(vg, m.x(-296), m.y(-46), m.s(250), m.s(812), m.s(125), 0, 0, m.s(125));
        fill(vg, TEAL, true);
        nvgBeginPath(vg);
        nvgRoundedRectVarying(vg, m.x(1326), m.y(-46), m.s(250), m.s(812), 0, m.s(125), m.s(125), 0);
        fill(vg, VIOLET, true);
        nvgBeginPath(vg);
        roundRect(vg, m, -46, -46, 1326, 766, 34);
        screenHole(vg, screen);
        fill(vg, GLASS, true);

        stick(vg, m, -171, 160, 62);
        diamond(vg, m, -171, 430, 78, 32);
        nvgBeginPath(vg);
        roundRect(vg, m, -100, 30, -66, 42, 4); // minus
        fill(vg, CONTROL);
        nvgBeginPath(vg);
        roundRect(vg, m, -145, 585, -115, 615, 6); // capture
        fill(vg, CONTROL);

        diamond(vg, m, 1451, 160, 78, 32);
        stick(vg, m, 1451, 430, 62);
        nvgBeginPath(vg);
        roundRect(vg, m, 1366, 30, 1400, 42, 4); // plus
        roundRect(vg, m, 1377, 19, 1389, 53, 4);
        fill(vg, CONTROL);
        button(vg, m, 1410, 600, 20); // home
    }

    void drawVita(NVGcontext* vg, const Map& m, const aurore::Rect& screen)
    {
        nvgBeginPath(vg);
        roundRect(vg, m, -330, -80, 1290, 624, 352);
        screenHole(vg, screen);
        fill(vg, BODY, true);
        nvgBeginPath(vg);
        roundRect(vg, m, -20, -20, 980, 564, 14);
        screenHole(vg, screen);
        fill(vg, GLASS);

        cross(vg, m, -175, 160, 70, 46);
        stick(vg, m, -150, 420, 46);
        diamond(vg, m, 1135, 160, 62, 27);
        stick(vg, m, 1110, 420, 46);
        for (float x : { 1040.0f, 1090.0f })
        {
            nvgBeginPath(vg);
            roundRect(vg, m, x, 560, x + 34, 572, 6); // start, select
            fill(vg, CONTROL);
        }
        button(vg, m, -95, 565, 13); // home
    }

    void drawTv(NVGcontext* vg, const Map& m, const aurore::Rect& screen)
    {
        nvgBeginPath(vg);
        roundRect(vg, m, 900, 1100, 1020, 1244, 6); // neck
        roundRect(vg, m, 640, 1236, 1280, 1282, 22); // foot
        fill(vg, BODY, true);
        nvgBeginPath(vg);
        roundRect(vg, m, -28, -28, 1948, 1108, 12);
        screenHole(vg, screen);
        fill(vg, GLASS, true);
        nvgBeginPath(vg);
        nvgCircle(vg, m.x(960), m.y(1094), std::max(1.5f, m.s(4)));
        fill(vg, LIGHT);
    }

    // The top screen, until an activity gives its own: the app's name and
    // the time, as a header would show them
    void drawTopScreen(NVGcontext* vg, const Map& m)
    {
        float x = m.x(-40), y = m.y(-342), w = m.s(400), h = m.s(240);
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillPaint(vg, nvgLinearGradient(vg, x, y, x, y + h, nvgRGB(0x10, 0x18, 0x2C), nvgRGB(0x0A, 0x0F, 0x1C)));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRect(vg, x, y + h * 0.62f, w, h * 0.38f);
        nvgFillPaint(vg, nvgLinearGradient(vg, x, y + h * 0.62f, x + w, y + h, nvgRGBA(0x2D, 0xD4, 0xBF, 0x30), nvgRGBA(0xA7, 0x8B, 0xFA, 0x30)));
        nvgFill(vg);

        int font = aurore::Application::getFont(aurore::FONT_REGULAR);
        if (font < 0)
            return;
        char clock[16]  = "";
        std::time_t now = aurore::getWallTime(); // the harness' own in the tests
        if (std::tm* local = std::localtime(&now))
            std::strftime(clock, sizeof(clock), "%H:%M", local);
        nvgFontFaceId(vg, font);
        nvgFontSize(vg, m.s(20));
        nvgFillColor(vg, nvgRGB(0xF5, 0xF7, 0xFF));
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(vg, x + m.s(16), y + m.s(26), aurore::Application::getTitle().c_str(), nullptr);
        nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        nvgText(vg, x + w - m.s(16), y + m.s(26), clock, nullptr);
    }

    void drawClamshell(NVGcontext* vg, const Map& m, const aurore::Rect& screen)
    {
        // The top half, the hinge, the bottom half (the app's screen)
        nvgBeginPath(vg);
        roundRect(vg, m, -112, -378, 432, -66, 24);
        fill(vg, BODY, true);
        nvgBeginPath(vg);
        roundRect(vg, m, -52, -354, 372, -90, 8);
        fill(vg, GLASS);
        drawTopScreen(vg, m);
        nvgBeginPath(vg);
        roundRect(vg, m, -100, -70, 420, -32, 12);
        fill(vg, TEAL, true);
        nvgBeginPath(vg);
        roundRect(vg, m, -112, -36, 432, 300, 24);
        screenHole(vg, screen);
        fill(vg, BODY, true);
        nvgBeginPath(vg);
        roundRect(vg, m, -12, -12, 332, 252, 8);
        screenHole(vg, screen);
        fill(vg, GLASS);

        stick(vg, m, -58, 34, 30);
        cross(vg, m, -58, 146, 30, 20);
        diamond(vg, m, 378, 34, 27, 12);
        button(vg, m, 378, 106, 7); // the small second stick
        for (float y : { 196.0f, 222.0f })
        {
            nvgBeginPath(vg);
            roundRect(vg, m, 364, y, 392, y + 9, 4); // start, select
            fill(vg, CONTROL);
        }
        nvgBeginPath(vg);
        roundRect(vg, m, 132, 268, 188, 280, 6); // home
        fill(vg, CONTROL);
    }

    void drawPhone(NVGcontext* vg, const Map& m, const aurore::Rect& screen)
    {
        nvgBeginPath(vg);
        roundRect(vg, m, 1080, -40, 1260, -30, 4); // volume and power keys
        fill(vg, BODY);
        nvgBeginPath(vg);
        roundRect(vg, m, -34, -34, 1634, 754, 74);
        screenHole(vg, screen);
        fill(vg, GLASS, true);
        button(vg, m, -17, 360, 7); // camera
    }

    class DeviceFrame : public aurore::ScreenFrame
    {
      public:
        explicit DeviceFrame(const Device& d)
            : device(d)
            , margins(marginsOf(d))
        {
        }

        aurore::Rect layout(float width, float height) override
        {
            float w = device.screenWidth + margins.left + margins.right;
            float h = device.screenHeight + margins.top + margins.bottom;
            float k = std::min(width * 0.92f / w, height * 0.92f / h);
            float x = (width - w * k) / 2 + margins.left * k;
            float y = (height - h * k) / 2 + margins.top * k;
            return aurore::Rect(std::round(x), std::round(y), std::round(device.screenWidth * k),
                std::round(device.screenHeight * k));
        }

        void draw(NVGcontext* vg, float width, float height, aurore::Rect screen) override
        {
            nvgBeginPath(vg);
            nvgRect(vg, 0, 0, width, height);
            screenHole(vg, screen);
            nvgFillPaint(vg, nvgLinearGradient(vg, 0, 0, 0, height, BACKGROUND_TOP, BACKGROUND_BOTTOM));
            nvgFill(vg);

            Map m { screen.getMinX(), screen.getMinY(), screen.getWidth() / device.screenWidth };
            if (!std::strcmp(device.id, "switch"))
                drawSwitch(vg, m, screen);
            else if (!std::strcmp(device.id, "vita"))
                drawVita(vg, m, screen);
            else if (!std::strcmp(device.id, "ps4"))
                drawTv(vg, m, screen);
            else if (!std::strcmp(device.id, "3ds"))
                drawClamshell(vg, m, screen);
            else
                drawPhone(vg, m, screen);
        }

      private:
        const Device& device;
        Margins margins;
    };

    bool isDesktop(const Device& d)
    {
        return !std::strcmp(d.id, "desktop");
    }

    void applyPreset(const Device& d)
    {
        aurore::Application::usePreset(d.smallScreen ? aurore::PRESET_SMALL_SCREEN : aurore::PRESET_STANDARD);
    }

    // The frame, the renderer's antialiasing, and a window the console fits
    void applyFrame(const Device& d)
    {
        NVGcontext* vg = aurore::Application::getNVGContext();
        if (vg)
        {
            NVGparams* params = nvgInternalParams(vg);
            if (defaultAntialias < 0)
                defaultAntialias = params->edgeAntiAlias;
            params->edgeAntiAlias = d.antialias ? defaultAntialias : 0;
        }
        aurore::Application::setScreenFrame(isDesktop(d) ? nullptr : new DeviceFrame(d));

        auto* platform = aurore::Application::getPlatform();
        if (isDesktop(d))
        {
            if (windowSizeChosen)
                platform->setWindowSize(1280, 720);
            return;
        }
        Margins mg = marginsOf(d);
        float w    = (d.screenWidth + mg.left + mg.right) / 0.92f;
        float h    = (d.screenHeight + mg.top + mg.bottom) / 0.92f;
        float k    = std::min(1440.0f / w, 860.0f / h);
        platform->setWindowSize((uint32_t)(w * k), (uint32_t)(h * k));
        windowSizeChosen = true;
    }
} // namespace

const std::vector<Device>& devices()
{
    return DEVICES;
}

const Device* find(const std::string& id)
{
    for (const Device& d : DEVICES)
        if (id == d.id)
            return &d;
    return nullptr;
}

const Device& current()
{
    return *chosen;
}

std::string displayName(const Device& device)
{
    return std::strncmp(device.name, "demo/", 5) == 0 ? aurore::getStr(device.name) : std::string(device.name);
}

void choose(const char* fromCommandLine)
{
    const char* id = fromCommandLine ? fromCommandLine : aurore::environmentVariable("SIMULATE");
    if (!id || !*id)
        return;
    if (const Device* d = find(id))
    {
        chosen = d;
        if (!isDesktop(*d))
            applyPreset(*d);
    }
    else
        aurore::Logger::warning("simulate: \"{}\" is not a platform (desktop, switch, vita, ps4, 3ds, phone)", id);
}

void start()
{
    if (!isDesktop(*chosen))
        applyFrame(*chosen);
}

void use(const std::string& id)
{
    const Device* d = find(id);
    if (!d || d == chosen)
        return;
    chosen = d;
    applyPreset(*d);
    applyFrame(*d);
    // The sizes are read when the screens are made: made again
    live_prefs::density_changed();
}

} // namespace demo::simulation
