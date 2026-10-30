// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include "simulation.hpp"

#include <algorithm>
#include <cstring>
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

    // The colours of the console on Boréale's sites (PlayGuard's site.css,
    // .device and its DEVICE_GRIPS, the reference): a body in the night's
    // blues, a glow of aurora under it, dark controls, the sticks' caps
    // ringed in teal
    const NVGcolor BACKGROUND_TOP    = nvgRGB(0x0B, 0x10, 0x20);
    const NVGcolor BACKGROUND_BOTTOM = nvgRGB(0x14, 0x1C, 0x31);
    const NVGcolor BODY_LIGHT        = nvgRGB(0x1B, 0x2C, 0x52);
    const NVGcolor BODY_DARK         = nvgRGB(0x0A, 0x14, 0x28);
    const NVGcolor BODY_RING         = nvgRGB(0x22, 0x32, 0x4F);
    const NVGcolor BODY_HIGHLIGHT    = nvgRGBA(0xE8, 0xEE, 0xF7, 0x24);
    const NVGcolor CONTROL           = nvgRGB(0x0A, 0x13, 0x24);
    const NVGcolor CONTROL_EDGE      = nvgRGBA(0xA9, 0xB6, 0xCC, 0x38);
    const NVGcolor CONTROL_MARK      = nvgRGBA(0xA9, 0xB6, 0xCC, 0x73);
    const NVGcolor STICK_WELL        = nvgRGB(0x07, 0x0E, 0x1B);
    const NVGcolor STICK_CAP         = nvgRGB(0x13, 0x22, 0x40);
    const NVGcolor STICK_RING        = nvgRGBA(0x00, 0xFF, 0xCC, 0x40);
    const NVGcolor SCREEN_RING       = nvgRGB(0x07, 0x0D, 0x19);
    const NVGcolor SCREEN_EDGE       = nvgRGB(0x05, 0x0A, 0x14);
    const NVGcolor GLOW_CYAN         = nvgRGBA(0x33, 0xBB, 0xFF, 0x52);
    const NVGcolor GLOW_VIOLET       = nvgRGBA(0x8A, 0x5C, 0xFF, 0x26);
    const NVGcolor LIGHT             = nvgRGB(0x00, 0xFF, 0xCC);

    const std::vector<Device> DEVICES = {
        { "desktop", "demo/developers/simulate_desktop", 1280, 720, false, true },
        { "switch", "Switch", 1280, 720, false, true },
        { "vita", "PS Vita", 960, 544, false, true }, // 4x MSAA rather than nanovg's edges
        { "ps4", "PS4", 1920, 1080, false, true },
        { "3ds", "New 3DS", 320, 240, true, true },
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

    // The handheld of the sites: the screen 71% of its width, 14.5% on each
    // side, 3.4% above and under, its sides drawn from DEVICE_GRIPS' 140 by
    // 470 view box
    constexpr float SWITCH_WIDTH = 1280 / 0.71f;
    constexpr float SWITCH_SIDE  = SWITCH_WIDTH * 0.145f;
    constexpr float SWITCH_EDGE  = SWITCH_WIDTH * 0.034f;
    constexpr float SWITCH_GRIP  = (720 + 2 * SWITCH_EDGE) / 470; // the view box's scale

    Margins marginsOf(const Device& d)
    {
        // Under the body, room for its glow
        if (!std::strcmp(d.id, "switch"))
            return { SWITCH_SIDE, SWITCH_EDGE, SWITCH_SIDE, SWITCH_EDGE + 110 };
        if (!std::strcmp(d.id, "vita"))
            return { 330, 80, 330, 160 };
        if (!std::strcmp(d.id, "ps4"))
            return { 28, 28, 28, 300 };
        if (!std::strcmp(d.id, "3ds"))
            return { 112, 378, 112, 96 };
        return { 40, 40, 40, 120 }; // phone
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

    float hairline(const Map& m, float units)
    {
        return std::max(1.0f, m.s(units));
    }

    // The glow of aurora under a body, as site.css's .device::after (the
    // screen left out: the frame is drawn over the app)
    void glow(NVGcontext* vg, const Map& m, float x0, float y0, float x1, float y1, const aurore::Rect& screen)
    {
        float w = x1 - x0, h = y1 - y0;
        float gx = m.x(x0 + w * 0.1f), gy = m.y(y1 - h * 0.18f), gw = m.s(w * 0.8f), gh = m.s(h * 0.32f);
        nvgBeginPath(vg);
        nvgRect(vg, gx - gw * 0.2f, gy - gh * 0.4f, gw * 1.4f, gh * 1.8f);
        screenHole(vg, screen);
        nvgFillPaint(vg, nvgBoxGradient(vg, gx, gy, gw, gh, gh / 2, gh * 0.9f, GLOW_CYAN, nvgRGBA(0x33, 0xBB, 0xFF, 0)));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRect(vg, gx - gw * 0.3f, gy - gh * 0.5f, gw * 1.6f, gh * 2.0f);
        screenHole(vg, screen);
        nvgFillPaint(vg, nvgBoxGradient(vg, gx - gw * 0.05f, gy, gw * 1.1f, gh, gh / 2, gh * 1.3f, GLOW_VIOLET, nvgRGBA(0x8A, 0x5C, 0xFF, 0)));
        nvgFill(vg);
    }

    // A body: the night's gradient, lit from the top left, its ring, a light
    // on its top edge; the screen cut out when it is on it
    void body(NVGcontext* vg, const Map& m, float x0, float y0, float x1, float y1, float r, const aurore::Rect* screen)
    {
        float w = x1 - x0, h = y1 - y0;
        nvgBeginPath(vg);
        roundRect(vg, m, x0, y0, x1, y1, r);
        if (screen)
            screenHole(vg, *screen);
        // 160 degrees: down, a little to the right
        nvgFillPaint(vg, nvgLinearGradient(vg, m.x(x0 + w * 0.33f), m.y(y0), m.x(x0 + w * 0.67f), m.y(y1), BODY_LIGHT, BODY_DARK));
        nvgFill(vg);
        nvgStrokeColor(vg, BODY_RING);
        nvgStrokeWidth(vg, hairline(m, 2));
        nvgStroke(vg);

        nvgSave(vg);
        nvgScissor(vg, m.x(x0), m.y(y0), m.s(w), m.s(std::min(h * 0.4f, r * 1.5f)));
        nvgBeginPath(vg);
        nvgRoundedRect(vg, m.x(x0) + 1.5f, m.y(y0) + 1.5f, m.s(w) - 3, m.s(h) - 3, std::max(0.0f, m.s(r) - 1.5f));
        nvgStrokeColor(vg, BODY_HIGHLIGHT);
        nvgStrokeWidth(vg, 1.0f);
        nvgStroke(vg);
        nvgRestore(vg);
    }

    // The screen in its glass: a dark ring around it, and over it the
    // sheen of the sites' screen, the aurora in its corner
    void glass(NVGcontext* vg, const aurore::Rect& screen, float ring)
    {
        float x = screen.getMinX(), y = screen.getMinY(), w = screen.getWidth(), h = screen.getHeight();
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x - ring, y - ring, w + 2 * ring, h + 2 * ring, ring * 1.6f);
        screenHole(vg, screen);
        nvgFillColor(vg, SCREEN_RING);
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRect(vg, x - 0.5f, y - 0.5f, w + 1, h + 1);
        nvgStrokeColor(vg, SCREEN_EDGE);
        nvgStrokeWidth(vg, 1.0f);
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillPaint(vg, nvgLinearGradient(vg, x, y, x + w * 0.36f, y + h * 0.36f, nvgRGBA(255, 255, 255, 0x17), nvgRGBA(255, 255, 255, 0)));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillPaint(vg, nvgLinearGradient(vg, x + w * 0.7f, y + h * 0.7f, x + w, y + h, nvgRGBA(0, 0xFF, 0xCC, 0), nvgRGBA(0, 0xFF, 0xCC, 0x12)));
        nvgFill(vg);
    }

    // A button: dark, its edge faintly lit
    void button(NVGcontext* vg, const Map& m, float x, float y, float r)
    {
        nvgBeginPath(vg);
        nvgCircle(vg, m.x(x), m.y(y), m.s(r));
        nvgFillColor(vg, CONTROL);
        nvgFill(vg);
        nvgStrokeColor(vg, CONTROL_EDGE);
        nvgStrokeWidth(vg, hairline(m, 2));
        nvgStroke(vg);
    }

    // A stick: its well, and its cap ringed in teal
    void stick(NVGcontext* vg, const Map& m, float x, float y, float r)
    {
        nvgBeginPath(vg);
        nvgCircle(vg, m.x(x), m.y(y), m.s(r));
        nvgFillColor(vg, STICK_WELL);
        nvgFill(vg);
        nvgStrokeColor(vg, CONTROL_EDGE);
        nvgStrokeWidth(vg, hairline(m, 2));
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, m.x(x), m.y(y), m.s(r * 22 / 34));
        nvgFillColor(vg, STICK_CAP);
        nvgFill(vg);
        nvgStrokeColor(vg, STICK_RING);
        nvgStroke(vg);
    }

    // Four buttons in a cross, as the sites draw the pad and the face buttons
    void cross(NVGcontext* vg, const Map& m, float x, float y, float gap, float r)
    {
        button(vg, m, x, y - gap, r);
        button(vg, m, x + gap, y, r);
        button(vg, m, x, y + gap, r);
        button(vg, m, x - gap, y, r);
    }

    // A small mark (minus, plus, start…): a lit line
    void mark(NVGcontext* vg, const Map& m, float x0, float y0, float x1, float y1, float width)
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, m.x(x0), m.y(y0));
        nvgLineTo(vg, m.x(x1), m.y(y1));
        nvgLineCap(vg, NVG_ROUND);
        nvgStrokeColor(vg, CONTROL_MARK);
        nvgStrokeWidth(vg, hairline(m, width));
        nvgStroke(vg);
    }

    void drawSwitch(NVGcontext* vg, const Map& m, const aurore::Rect& screen)
    {
        float x0 = -SWITCH_SIDE, y0 = -SWITCH_EDGE, x1 = 1280 + SWITCH_SIDE, y1 = 720 + SWITCH_EDGE;
        glow(vg, m, x0, y0, x1, y1, screen);
        body(vg, m, x0, y0, x1, y1, SWITCH_WIDTH * 0.095f, &screen);
        glass(vg, screen, m.s(10));

        // DEVICE_GRIPS, its view box centred on each side
        float k     = SWITCH_GRIP;
        float inset = (SWITCH_SIDE - 140 * k) / 2;
        Map left { m.x(x0 + inset), m.y(y0), m.k * k };
        Map right { m.x(1280 + inset), m.y(y0), m.k * k };
        mark(vg, left, 86, 44, 106, 44, 4); // minus
        stick(vg, left, 70, 128, 34);
        cross(vg, left, 70, 296, 34, 12);
        mark(vg, right, 34, 44, 54, 44, 4); // plus
        mark(vg, right, 44, 34, 44, 54, 4);
        cross(vg, right, 70, 126, 34, 12);
        stick(vg, right, 70, 300, 34);
    }

    void drawVita(NVGcontext* vg, const Map& m, const aurore::Rect& screen)
    {
        glow(vg, m, -330, -80, 1290, 624, screen);
        body(vg, m, -330, -80, 1290, 624, 352, &screen);
        glass(vg, screen, m.s(18));

        cross(vg, m, -175, 165, 58, 22);
        stick(vg, m, -150, 425, 46);
        cross(vg, m, 1135, 165, 58, 22);
        stick(vg, m, 1110, 425, 46);
        mark(vg, m, 1040, 566, 1066, 566, 8); // start, select
        mark(vg, m, 1090, 566, 1116, 566, 8);
        button(vg, m, -95, 566, 12); // home
    }

    void drawTv(NVGcontext* vg, const Map& m, const aurore::Rect& screen)
    {
        glow(vg, m, 0, 0, 1920, 1300, screen);
        body(vg, m, 900, 1100, 1020, 1250, 6, nullptr); // neck
        body(vg, m, 620, 1238, 1300, 1290, 26, nullptr); // foot
        body(vg, m, -28, -28, 1948, 1108, 14, &screen);
        glass(vg, screen, m.s(6));
        nvgBeginPath(vg);
        nvgCircle(vg, m.x(960), m.y(1094), std::max(1.5f, m.s(4)));
        nvgFillColor(vg, LIGHT);
        nvgFill(vg);
    }

    // The top screen: the activity's top screen view (aurore::TopScreen),
    // as the console's second target draws it; a night with the aurora low
    // on it in a picture of the console
    void drawTopScreen(NVGcontext* vg, const Map& m, bool live)
    {
        aurore::Rect top(m.x(-40), m.y(-342), m.s(400), m.s(240));
        float x = top.getMinX(), y = top.getMinY(), w = top.getWidth(), h = top.getHeight();
        if (live && aurore::TopScreen::isAvailable())
        {
            aurore::TopScreen::draw(vg, top);
        }
        else
        {
            nvgBeginPath(vg);
            nvgRect(vg, x, y, w, h);
            nvgFillPaint(vg, nvgLinearGradient(vg, x, y, x, y + h, nvgRGB(0x10, 0x18, 0x2C), nvgRGB(0x0A, 0x0F, 0x1C)));
            nvgFill(vg);
            nvgBeginPath(vg);
            nvgRect(vg, x, y + h * 0.62f, w, h * 0.38f);
            nvgFillPaint(vg, nvgLinearGradient(vg, x, y + h * 0.62f, x + w, y + h, nvgRGBA(0x00, 0xFF, 0xCC, 0x30), nvgRGBA(0x8A, 0x5C, 0xFF, 0x30)));
            nvgFill(vg);
        }
        glass(vg, top, m.s(10));
    }

    void drawClamshell(NVGcontext* vg, const Map& m, const aurore::Rect& screen, bool live)
    {
        // The top half, the hinge, the bottom half (the app's screen)
        glow(vg, m, -112, -378, 432, 300, screen);
        body(vg, m, -112, -378, 432, -66, 24, nullptr);
        drawTopScreen(vg, m, live);
        nvgBeginPath(vg);
        roundRect(vg, m, -100, -72, 420, -30, 14);
        nvgFillColor(vg, CONTROL);
        nvgFill(vg);
        nvgStrokeColor(vg, BODY_RING);
        nvgStrokeWidth(vg, hairline(m, 1));
        nvgStroke(vg);
        body(vg, m, -112, -36, 432, 300, 24, &screen);
        glass(vg, screen, m.s(10));

        stick(vg, m, -58, 36, 28);
        cross(vg, m, -58, 146, 24, 10);
        cross(vg, m, 378, 36, 24, 10);
        button(vg, m, 378, 106, 6); // the small second stick
        mark(vg, m, 368, 200, 388, 200, 6); // start, select
        mark(vg, m, 368, 224, 388, 224, 6);
        mark(vg, m, 140, 276, 180, 276, 8); // home
    }

    void drawPhone(NVGcontext* vg, const Map& m, const aurore::Rect& screen)
    {
        glow(vg, m, -34, -34, 1634, 754, screen);
        mark(vg, m, 1090, -36, 1250, -36, 6); // volume and power keys
        body(vg, m, -34, -34, 1634, 754, 74, &screen);
        glass(vg, screen, m.s(4));
        stick(vg, m, -17, 360, 7); // the camera
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
            float k = std::min(width * 0.94f / w, height * 0.94f / h);
            float x = (width - w * k) / 2 + margins.left * k;
            float y = (height - h * k) / 2 + margins.top * k;
            // The New 3DS's top screen, in the pixels of the app's screen as
            // the window shows it
            if (!std::strcmp(device.id, "3ds"))
                aurore::TopScreen::setSize((int)std::round(400 * k), (int)std::round(240 * k));
            return aurore::Rect(std::round(x), std::round(y), std::round(device.screenWidth * k), std::round(device.screenHeight * k));
        }

        void draw(NVGcontext* vg, float width, float height, aurore::Rect screen) override
        {
            nvgBeginPath(vg);
            nvgRect(vg, 0, 0, width, height);
            screenHole(vg, screen);
            nvgFillPaint(vg, nvgLinearGradient(vg, 0, 0, 0, height, BACKGROUND_TOP, BACKGROUND_BOTTOM));
            nvgFill(vg);

            Map m { screen.getMinX(), screen.getMinY(), screen.getWidth() / device.screenWidth };
            drawConsole(vg, m, screen, true);
        }

        // The console alone, around `screen`; `live`, the app's own second
        // screen on it, else a picture
        void drawConsole(NVGcontext* vg, const Map& m, const aurore::Rect& screen, bool live = false)
        {
            if (!std::strcmp(device.id, "switch"))
                drawSwitch(vg, m, screen);
            else if (!std::strcmp(device.id, "vita"))
                drawVita(vg, m, screen);
            else if (!std::strcmp(device.id, "ps4"))
                drawTv(vg, m, screen);
            else if (!std::strcmp(device.id, "3ds"))
                drawClamshell(vg, m, screen, live);
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
        // No top screen, but the New 3DS's (DeviceFrame::layout())
        aurore::TopScreen::setSize(0, 0);

        auto* platform = aurore::Application::getPlatform();
        if (isDesktop(d))
        {
            if (windowSizeChosen)
                platform->setWindowSize(1280, 720);
            return;
        }
        Margins mg = marginsOf(d);
        float w    = (d.screenWidth + mg.left + mg.right) / 0.94f;
        float h    = (d.screenHeight + mg.top + mg.bottom) / 0.94f;
        float k    = std::min(1440.0f / w, 860.0f / h);
        platform->setWindowSize((uint32_t)(w * k), (uint32_t)(h * k));
        windowSizeChosen = true;
    }
} // namespace

void drawPreview(NVGcontext* vg, const Device& device, float x, float y, float width, float height)
{
    if (isDesktop(device))
        return;
    DeviceFrame frame(device);
    aurore::Rect screen = frame.layout(width, height);
    screen              = aurore::Rect(x + screen.getMinX(), y + screen.getMinY(), screen.getWidth(), screen.getHeight());
    nvgSave(vg);
    nvgIntersectScissor(vg, x, y, width, height);
    // The screen: a night, the aurora low on it
    float sx = screen.getMinX(), sy = screen.getMinY(), sw = screen.getWidth(), sh = screen.getHeight();
    nvgBeginPath(vg);
    nvgRect(vg, sx, sy, sw, sh);
    nvgFillPaint(vg, nvgLinearGradient(vg, sx, sy, sx, sy + sh, nvgRGB(0x10, 0x18, 0x2C), nvgRGB(0x0A, 0x0F, 0x1C)));
    nvgFill(vg);
    nvgBeginPath(vg);
    nvgRect(vg, sx, sy + sh * 0.55f, sw, sh * 0.45f);
    nvgFillPaint(vg, nvgLinearGradient(vg, sx, sy + sh * 0.55f, sx + sw, sy + sh, nvgRGBA(0x00, 0xFF, 0xCC, 0x48), nvgRGBA(0x8A, 0x5C, 0xFF, 0x48)));
    nvgFill(vg);
    // The rest as the frame draws it, without its window's background
    Map m { sx, sy, sw / device.screenWidth };
    frame.drawConsole(vg, m, screen);
    nvgRestore(vg);
}

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
