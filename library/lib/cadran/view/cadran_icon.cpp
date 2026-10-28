// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#include <aurore/cadran/view/cadran_icon.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

#include <aurore/cadran/ui/effects.hpp>
#include <aurore/cadran/ui/hooks.hpp>
#include <aurore/cadran/ui/motion.hpp>

namespace
{
constexpr float POP_US = 380000.0f;
} // namespace

CadranIcon::CadranIcon()
{
    this->setThemeColor("color", "brls/text");
    this->setWidth(22);
    this->setHeight(22);
    this->setFocusable(false);
    this->registerStringXMLAttribute("icon", [this](std::string value)
        { this->setIcon(value); });
    this->registerColorXMLAttribute("color", [this](NVGcolor value)
        { this->setColor(value); });
}

void CadranIcon::setIcon(const std::string& name) { this->icon = name; }
void CadranIcon::setColor(NVGcolor value)
{
    if (!this->transitionStyle("color", brls::StyleValue::of(value)))
        this->color = value;
}

bool CadranIcon::getStyleProperty(const std::string& name, brls::StyleValue* value)
{
    if (name != "color")
        return brls::View::getStyleProperty(name, value);
    *value = brls::StyleValue::of(this->color);
    return true;
}

bool CadranIcon::setStyleProperty(const std::string& name, const brls::StyleValue& value)
{
    if (name != "color")
        return brls::View::setStyleProperty(name, value);
    if (value.type != brls::StyleValue::Type::COLOR)
        return false;
    if (!this->transitionStyle("color", value))
        this->color = value.color;
    return true;
}

void CadranIcon::pop() { this->pop_since = effects::motion() ? motion::now() : 0; }

static void mark(NVGcontext* vg, float w, float h)
{
    // The app's own logo (ui::hooks::get().mark), else Aurore's
    // (.github/branding/icon.svg, simplified): the aurora's line in the
    // focus highlight's rounded frame, on the night's navy.
    if (ui::hooks::get().mark)
    {
        ui::hooks::get().mark(vg, w, h);
        return;
    }
    nvgScale(vg, w / 256.0f, h / 256.0f);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, 0, 0, 256, 256, 56);
    nvgFillColor(vg, nvgRGB(0x0A, 0x10, 0x20));
    nvgFill(vg);
    nvgBeginPath(vg);
    nvgMoveTo(vg, 58, 150);
    nvgBezierTo(vg, 88, 118, 112, 160, 150, 128);
    nvgBezierTo(vg, 172, 110, 186, 104, 200, 108);
    nvgLineCap(vg, NVG_ROUND);
    nvgStrokeWidth(vg, 12);
    nvgStrokePaint(vg, nvgLinearGradient(vg, 58, 0, 200, 0, nvgRGB(0x00, 0xFF, 0xCC), nvgRGB(0x8A, 0x5C, 0xFF)));
    nvgStroke(vg);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, 44, 44, 168, 168, 30);
    nvgStrokeWidth(vg, 14);
    nvgStrokeColor(vg, nvgRGB(0x00, 0xFF, 0xCC));
    nvgStroke(vg);
}

void CadranIcon::draw(NVGcontext* vg, float x, float y, float width, float height,
    brls::Style style, brls::FrameContext* ctx)
{
    (void)style;
    (void)ctx;
    nvgSave(vg);
    nvgTranslate(vg, x, y);
    // Popping: scaled about its middle, from nothing past its size and back.
    if (this->pop_since)
    {
        const float t = motion::progress(this->pop_since, POP_US);
        if (t >= 1.0f)
        {
            this->pop_since = 0;
        }
        else
        {
            const float scale = motion::back_out(t);
            nvgTranslate(vg, width / 2.0f, height / 2.0f);
            nvgScale(vg, scale, scale);
            nvgTranslate(vg, -width / 2.0f, -height / 2.0f);
        }
    }
    glyph(vg, this->icon, this->color, width, height);
    nvgRestore(vg);
}

// The theme's picture in the icon's place (its logo for "mark"), fitted in
// the box: false while it has none, or it is not loaded
static bool themed(NVGcontext* vg, const std::string& i, NVGcolor color, float width, float height)
{
    bool tint                                 = false;
    std::shared_ptr<brls::ThemeImage> picture = i == "mark" ? brls::Theme::logo() : brls::Theme::icon(i, &tint);
    if (!picture || !picture->ready())
        return false;
    // The logo as it is, as the app's own; an icon at its colour's opacity
    if (i == "mark")
        return picture->draw(vg, 0, 0, width, height, brls::BackgroundFit::CONTAIN);
    return picture->draw(vg, 0, 0, width, height, brls::BackgroundFit::CONTAIN, tint ? &color : nullptr, tint ? 1.0f : color.a);
}

bool CadranIcon::glyph(NVGcontext* vg, const std::string& i, NVGcolor color, float width, float height)
{
    if (themed(vg, i, color, width, height))
        return true;
    if (i == "mark")
    {
        nvgSave(vg);
        mark(vg, width, height);
        nvgRestore(vg);
        return true;
    }
    nvgSave(vg);
    // The boards' icons: a 24-unit box, 2.2-unit round strokes.
    nvgScale(vg, width / 24.0f, height / 24.0f);
    nvgStrokeColor(vg, color);
    nvgFillColor(vg, color);
    nvgStrokeWidth(vg, 2.2f);
    nvgLineCap(vg, NVG_ROUND);
    nvgLineJoin(vg, NVG_ROUND);
    bool known = true;
    if (i == "today")
    {
        nvgBeginPath(vg);
        nvgCircle(vg, 12, 12, 9);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 10, 9);
        nvgLineTo(vg, 15, 12);
        nvgLineTo(vg, 10, 15);
        nvgClosePath(vg);
        nvgFill(vg);
        nvgStroke(vg);
    }
    else if (i == "timer")
    {
        nvgBeginPath(vg);
        nvgCircle(vg, 12, 12, 9);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 12, 7);
        nvgLineTo(vg, 12, 12);
        nvgLineTo(vg, 15, 14);
        nvgStroke(vg);
    }
    else if (i == "activity")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 5, 20);
        nvgLineTo(vg, 5, 12);
        nvgMoveTo(vg, 12, 20);
        nvgLineTo(vg, 12, 5);
        nvgMoveTo(vg, 19, 20);
        nvgLineTo(vg, 19, 14);
        nvgStroke(vg);
    }
    else if (i == "shield")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 12, 3);
        nvgLineTo(vg, 19, 6);
        nvgLineTo(vg, 19, 12);
        nvgBezierTo(vg, 19, 16.5f, 16, 19.5f, 12, 21);
        nvgBezierTo(vg, 8, 19.5f, 5, 16.5f, 5, 12);
        nvgLineTo(vg, 5, 6);
        nvgClosePath(vg);
        nvgStroke(vg);
    }
    else if (i == "lock")
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 5, 11, 14, 10, 2);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 8, 11);
        nvgLineTo(vg, 8, 8);
        nvgArc(vg, 12, 8, 4, NVG_PI, 0, NVG_CW);
        nvgLineTo(vg, 16, 11);
        nvgStroke(vg);
    }
    else if (i == "globe")
    {
        nvgBeginPath(vg);
        nvgCircle(vg, 12, 12, 9);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 3, 12);
        nvgLineTo(vg, 21, 12);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 12, 3);
        nvgBezierTo(vg, 15, 6.5f, 15, 17.5f, 12, 21);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 12, 3);
        nvgBezierTo(vg, 9, 6.5f, 9, 17.5f, 12, 21);
        nvgStroke(vg);
    }
    else if (i == "settings")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 4, 7);
        nvgLineTo(vg, 13, 7);
        nvgMoveTo(vg, 19, 7);
        nvgLineTo(vg, 20, 7);
        nvgMoveTo(vg, 4, 17);
        nvgLineTo(vg, 7, 17);
        nvgMoveTo(vg, 13, 17);
        nvgLineTo(vg, 20, 17);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, 16, 7, 2.5f);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, 10, 17, 2.5f);
        nvgStroke(vg);
    }
    else if (i == "tools")
    {
        nvgBeginPath(vg);
        nvgCircle(vg, 16.5f, 7.5f, 4);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgStrokeWidth(vg, 3);
        nvgMoveTo(vg, 13.5f, 10.5f);
        nvgLineTo(vg, 4.5f, 19.5f);
        nvgStroke(vg);
    }
    else if (i == "shelf")
    {
        // Two shelves, books standing on them.
        nvgBeginPath(vg);
        nvgMoveTo(vg, 3, 11);
        nvgLineTo(vg, 21, 11);
        nvgMoveTo(vg, 3, 21);
        nvgLineTo(vg, 21, 21);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgRect(vg, 5.5f, 4, 3, 7);
        nvgRect(vg, 10.5f, 5.5f, 3, 5.5f);
        nvgRect(vg, 7, 14, 3, 7);
        nvgRect(vg, 15, 15.5f, 3.5f, 5.5f);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 15.5f, 4.5f);
        nvgLineTo(vg, 18.5f, 11);
        nvgStroke(vg);
    }
    else if (i == "info")
    {
        nvgBeginPath(vg);
        nvgCircle(vg, 12, 12, 9);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 12, 11);
        nvgLineTo(vg, 12, 16.5f);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, 12, 8, 1.3f);
        nvgFill(vg);
    }
    else if (i == "plus")
    {
        nvgBeginPath(vg);
        nvgStrokeWidth(vg, 3);
        nvgMoveTo(vg, 12, 5);
        nvgLineTo(vg, 12, 19);
        nvgMoveTo(vg, 5, 12);
        nvgLineTo(vg, 19, 12);
        nvgStroke(vg);
    }
    else if (i == "stop")
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 5, 5, 14, 14, 3);
        nvgFill(vg);
    }
    else if (i == "eye")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 2, 12);
        nvgBezierTo(vg, 2, 12, 6, 5, 12, 5);
        nvgBezierTo(vg, 18, 5, 22, 12, 22, 12);
        nvgBezierTo(vg, 22, 12, 18, 19, 12, 19);
        nvgBezierTo(vg, 6, 19, 2, 12, 2, 12);
        nvgClosePath(vg);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, 12, 12, 3);
        nvgStroke(vg);
    }
    else if (i == "sun")
    {
        // The header's light theme: a disc and eight rays.
        nvgBeginPath(vg);
        nvgCircle(vg, 12, 12, 4);
        nvgStroke(vg);
        nvgBeginPath(vg);
        for (int k = 0; k < 8; k++)
        {
            const float a = (float)k * NVG_PI / 4;
            nvgMoveTo(vg, 12 + std::cos(a) * 7, 12 + std::sin(a) * 7);
            nvgLineTo(vg, 12 + std::cos(a) * 9.5f, 12 + std::sin(a) * 9.5f);
        }
        nvgStroke(vg);
    }
    else if (i == "tv")
    {
        // The automatic text size, larger on the TV: a screen on its stand.
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 3, 5, 18, 12, 2);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 9, 21);
        nvgLineTo(vg, 15, 21);
        nvgMoveTo(vg, 12, 17);
        nvgLineTo(vg, 12, 21);
        nvgStroke(vg);
    }
    else if (i == "text-normal" || i == "text-large")
    {
        // A text size: an A, small or large, on the same baseline (as the
        // header's button).
        nvgFontFaceId(vg, brls::Application::getDefaultFont());
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
        nvgFontSize(vg, i == "text-large" ? 28 : 18);
        nvgText(vg, 12, 20.5f, "A", nullptr);
    }
    else if (i == "text-largest")
    {
        // The large A, a plus at its shoulder
        nvgFontFaceId(vg, brls::Application::getDefaultFont());
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
        nvgFontSize(vg, 26);
        nvgText(vg, 9.5f, 21.5f, "A", nullptr);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 20, 2.5f);
        nvgLineTo(vg, 20, 9.5f);
        nvgMoveTo(vg, 16.5f, 6);
        nvgLineTo(vg, 23.5f, 6);
        nvgStroke(vg);
    }
    else if (i == "moon")
    {
        nvgBeginPath(vg);
        nvgArc(vg, 12, 12, 9, 0.11f, 4.60f, NVG_CW);
        nvgArc(vg, 16, 8, 7.07f, 3.93f, 0.785f, NVG_CCW);
        nvgClosePath(vg);
        nvgStroke(vg);
    }
    else if (i == "pause")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 9, 5);
        nvgLineTo(vg, 9, 19);
        nvgMoveTo(vg, 15, 5);
        nvgLineTo(vg, 15, 19);
        nvgStroke(vg);
    }
    else if (i == "list")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 4, 6);
        nvgLineTo(vg, 20, 6);
        nvgMoveTo(vg, 4, 12);
        nvgLineTo(vg, 20, 12);
        nvgMoveTo(vg, 4, 18);
        nvgLineTo(vg, 14, 18);
        nvgStroke(vg);
    }
    else if (i == "rail-open" || i == "rail-icons" || i == "rail-auto")
    {
        // Preferences › Sidebar: a screen with its rail, wide with its
        // names, narrow on its dots, or narrow and opening (an arrow out).
        const bool open  = i == "rail-open";
        const float edge = open ? 11.0f : 8.0f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 3, 4, 18, 16, 2);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, edge, 4);
        nvgLineTo(vg, edge, 20);
        nvgStroke(vg);
        if (open)
        {
            nvgBeginPath(vg);
            for (float r : { 8.5f, 12.0f, 15.5f })
            {
                nvgMoveTo(vg, 5.5f, r);
                nvgLineTo(vg, 8.5f, r);
            }
            nvgStroke(vg);
        }
        else
        {
            nvgBeginPath(vg);
            for (float r : { 8.5f, 12.0f, 15.5f })
                nvgCircle(vg, 5.5f, r, 0.9f);
            nvgFill(vg);
        }
        if (i == "rail-auto")
        {
            nvgBeginPath(vg);
            nvgMoveTo(vg, 11.5f, 12);
            nvgLineTo(vg, 17.5f, 12);
            nvgMoveTo(vg, 15, 9.5f);
            nvgLineTo(vg, 17.5f, 12);
            nvgLineTo(vg, 15, 14.5f);
            nvgStroke(vg);
        }
    }
    else if (i == "lines")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 3, 12);
        nvgLineTo(vg, 11, 12);
        nvgMoveTo(vg, 3, 6);
        nvgLineTo(vg, 21, 6);
        nvgMoveTo(vg, 3, 18);
        nvgLineTo(vg, 15, 18);
        nvgStroke(vg);
    }
    else if (i == "download")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 12, 3);
        nvgLineTo(vg, 12, 15);
        nvgMoveTo(vg, 7, 10);
        nvgLineTo(vg, 12, 15);
        nvgLineTo(vg, 17, 10);
        nvgMoveTo(vg, 4, 20);
        nvgLineTo(vg, 20, 20);
        nvgStroke(vg);
    }
    else if (i == "chevron-down")
    {
        nvgBeginPath(vg);
        nvgStrokeWidth(vg, 2.5f);
        nvgMoveTo(vg, 6, 9);
        nvgLineTo(vg, 12, 15);
        nvgLineTo(vg, 18, 9);
        nvgStroke(vg);
    }
    else if (i == "chevron-left")
    {
        nvgBeginPath(vg);
        nvgStrokeWidth(vg, 2.5f);
        nvgMoveTo(vg, 15, 6);
        nvgLineTo(vg, 9, 12);
        nvgLineTo(vg, 15, 18);
        nvgStroke(vg);
    }
    else if (i == "chevron-right")
    {
        nvgBeginPath(vg);
        nvgStrokeWidth(vg, 2.5f);
        nvgMoveTo(vg, 9, 6);
        nvgLineTo(vg, 15, 12);
        nvgLineTo(vg, 9, 18);
        nvgStroke(vg);
    }
    else if (i == "caret-up")
    {
        nvgBeginPath(vg);
        nvgStrokeWidth(vg, 3);
        nvgMoveTo(vg, 4, 15);
        nvgLineTo(vg, 12, 7);
        nvgLineTo(vg, 20, 15);
        nvgStroke(vg);
    }
    else if (i == "caret-down")
    {
        nvgBeginPath(vg);
        nvgStrokeWidth(vg, 3);
        nvgMoveTo(vg, 4, 9);
        nvgLineTo(vg, 12, 17);
        nvgLineTo(vg, 20, 9);
        nvgStroke(vg);
    }
    else if (i == "check")
    {
        nvgBeginPath(vg);
        nvgStrokeWidth(vg, 3);
        nvgMoveTo(vg, 4, 12);
        nvgLineTo(vg, 9, 17);
        nvgLineTo(vg, 20, 7);
        nvgStroke(vg);
    }
    else if (i == "key")
    {
        nvgBeginPath(vg);
        nvgCircle(vg, 7.5f, 15.5f, 4);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 10.5f, 12.5f);
        nvgLineTo(vg, 20, 3);
        nvgMoveTo(vg, 16.5f, 6.5f);
        nvgLineTo(vg, 19.5f, 9.5f);
        nvgMoveTo(vg, 14, 9);
        nvgLineTo(vg, 16, 11);
        nvgStroke(vg);
    }
    else if (i == "unlock")
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 5, 11, 14, 10, 2);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 8, 11);
        nvgLineTo(vg, 8, 8);
        nvgArc(vg, 12, 8, 4, NVG_PI, NVG_PI * 1.82f, NVG_CW);
        nvgStroke(vg);
    }
    else if (i == "phone")
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 7, 2, 10, 20, 2.5f);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, 12, 18, 1.1f);
        nvgFill(vg);
    }
    else if (i == "trash")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 4, 7);
        nvgLineTo(vg, 20, 7);
        nvgMoveTo(vg, 9, 7);
        nvgLineTo(vg, 9, 4);
        nvgLineTo(vg, 15, 4);
        nvgLineTo(vg, 15, 7);
        nvgMoveTo(vg, 6, 7);
        nvgLineTo(vg, 7, 20);
        nvgLineTo(vg, 17, 20);
        nvgLineTo(vg, 18, 7);
        nvgMoveTo(vg, 10, 11);
        nvgLineTo(vg, 10, 16);
        nvgMoveTo(vg, 14, 11);
        nvgLineTo(vg, 14, 16);
        nvgStroke(vg);
    }
    else if (i == "server")
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 3, 4, 18, 7, 2);
        nvgRoundedRect(vg, 3, 13, 18, 7, 2);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, 7, 7.5f, 1.1f);
        nvgCircle(vg, 7, 16.5f, 1.1f);
        nvgFill(vg);
    }
    else if (i == "pin")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 12, 21);
        nvgBezierTo(vg, 8, 17, 5, 13.5f, 5, 10);
        nvgArc(vg, 12, 10, 7, NVG_PI, 2.0f * NVG_PI, NVG_CW);
        nvgBezierTo(vg, 19, 13.5f, 16, 17, 12, 21);
        nvgClosePath(vg);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, 12, 10, 2.6f);
        nvgStroke(vg);
    }
    else if (i == "refresh" || i == "history" || i == "restore")
    {
        // An arc around the clock face, its arrow at the top: forward
        // (refresh) or back (history, restore); history adds the hands.
        const bool back = i != "refresh";
        nvgBeginPath(vg);
        if (back)
        {
            nvgArc(vg, 12, 12, 9, NVG_PI, -NVG_PI / 2.0f, NVG_CCW);
            nvgBezierTo(vg, 9.5f, 3, 7.1f, 4, 5.3f, 5.7f);
            nvgLineTo(vg, 3, 8);
            nvgMoveTo(vg, 3, 3);
            nvgLineTo(vg, 3, 8);
            nvgLineTo(vg, 8, 8);
        }
        else
        {
            nvgArc(vg, 12, 12, 9, 0, 1.5f * NVG_PI, NVG_CW);
            nvgBezierTo(vg, 14.5f, 3, 16.9f, 4, 18.7f, 5.7f);
            nvgLineTo(vg, 21, 8);
            nvgMoveTo(vg, 21, 3);
            nvgLineTo(vg, 21, 8);
            nvgLineTo(vg, 16, 8);
        }
        if (i == "history")
        {
            nvgMoveTo(vg, 12, 7);
            nvgLineTo(vg, 12, 12);
            nvgLineTo(vg, 15.5f, 14);
        }
        nvgStroke(vg);
    }
    else if (i == "save")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 5, 3);
        nvgLineTo(vg, 16, 3);
        nvgLineTo(vg, 21, 8);
        nvgLineTo(vg, 21, 19);
        nvgQuadTo(vg, 21, 21, 19, 21);
        nvgLineTo(vg, 5, 21);
        nvgQuadTo(vg, 3, 21, 3, 19);
        nvgLineTo(vg, 3, 5);
        nvgQuadTo(vg, 3, 3, 5, 3);
        nvgClosePath(vg);
        nvgMoveTo(vg, 7, 21);
        nvgLineTo(vg, 7, 14);
        nvgLineTo(vg, 17, 14);
        nvgLineTo(vg, 17, 21);
        nvgMoveTo(vg, 7, 3);
        nvgLineTo(vg, 7, 8);
        nvgLineTo(vg, 15, 8);
        nvgStroke(vg);
    }
    else if (i == "upload")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 12, 15);
        nvgLineTo(vg, 12, 3);
        nvgMoveTo(vg, 7, 8);
        nvgLineTo(vg, 12, 3);
        nvgLineTo(vg, 17, 8);
        nvgMoveTo(vg, 4, 20);
        nvgLineTo(vg, 20, 20);
        nvgStroke(vg);
    }
    else if (i == "language")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 5, 8);
        nvgLineTo(vg, 11, 14);
        nvgMoveTo(vg, 4, 14);
        nvgLineTo(vg, 10, 8);
        nvgLineTo(vg, 12, 5);
        nvgMoveTo(vg, 2, 5);
        nvgLineTo(vg, 14, 5);
        nvgMoveTo(vg, 7, 2);
        nvgLineTo(vg, 8, 2);
        nvgMoveTo(vg, 22, 22);
        nvgLineTo(vg, 17, 12);
        nvgLineTo(vg, 12, 22);
        nvgMoveTo(vg, 14, 18);
        nvgLineTo(vg, 20, 18);
        nvgStroke(vg);
    }
    else if (i == "contrast")
    {
        nvgBeginPath(vg);
        nvgCircle(vg, 12, 12, 9);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgArc(vg, 12, 12, 9, -NVG_PI / 2.0f, NVG_PI / 2.0f, NVG_CW);
        nvgClosePath(vg);
        nvgFill(vg);
    }
    else if (i == "home")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 3, 10);
        nvgLineTo(vg, 12, 3);
        nvgLineTo(vg, 21, 10);
        nvgLineTo(vg, 21, 20);
        nvgQuadTo(vg, 21, 21, 20, 21);
        nvgLineTo(vg, 4, 21);
        nvgQuadTo(vg, 3, 21, 3, 20);
        nvgClosePath(vg);
        nvgMoveTo(vg, 9, 21);
        nvgLineTo(vg, 9, 14);
        nvgLineTo(vg, 15, 14);
        nvgLineTo(vg, 15, 21);
        nvgStroke(vg);
    }
    else if (i == "bolt")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 13, 2);
        nvgLineTo(vg, 4, 14);
        nvgLineTo(vg, 12, 14);
        nvgLineTo(vg, 11, 22);
        nvgLineTo(vg, 20, 10);
        nvgLineTo(vg, 12, 10);
        nvgClosePath(vg);
        nvgStroke(vg);
    }
    else if (i == "sparkle")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 12, 3);
        nvgLineTo(vg, 13.8f, 10.2f);
        nvgLineTo(vg, 21, 12);
        nvgLineTo(vg, 13.8f, 13.8f);
        nvgLineTo(vg, 12, 21);
        nvgLineTo(vg, 10.2f, 13.8f);
        nvgLineTo(vg, 3, 12);
        nvgLineTo(vg, 10.2f, 10.2f);
        nvgClosePath(vg);
        nvgStroke(vg);
    }
    else if (i == "bell")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 6, 9);
        nvgArc(vg, 12, 9, 6, NVG_PI, 2.0f * NVG_PI, NVG_CW);
        nvgBezierTo(vg, 18, 15, 21, 17, 21, 17);
        nvgLineTo(vg, 3, 17);
        nvgBezierTo(vg, 3, 17, 6, 15, 6, 9);
        nvgMoveTo(vg, 10, 20.5f);
        nvgBezierTo(vg, 11, 21.8f, 13, 21.8f, 14, 20.5f);
        nvgStroke(vg);
    }
    else if (i == "chip")
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 6, 6, 12, 12, 2);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgRect(vg, 9.5f, 9.5f, 5, 5);
        nvgFill(vg);
        nvgBeginPath(vg);
        for (float p : { 9.0f, 15.0f })
        {
            nvgMoveTo(vg, p, 2);
            nvgLineTo(vg, p, 6);
            nvgMoveTo(vg, p, 18);
            nvgLineTo(vg, p, 22);
            nvgMoveTo(vg, 2, p);
            nvgLineTo(vg, 6, p);
            nvgMoveTo(vg, 18, p);
            nvgLineTo(vg, 22, p);
        }
        nvgStroke(vg);
    }
    else if (i == "code")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 16, 18);
        nvgLineTo(vg, 22, 12);
        nvgLineTo(vg, 16, 6);
        nvgMoveTo(vg, 8, 6);
        nvgLineTo(vg, 2, 12);
        nvgLineTo(vg, 8, 18);
        nvgStroke(vg);
    }
    else if (i == "heart")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 12, 21);
        nvgLineTo(vg, 5, 14);
        nvgBezierTo(vg, 3.5f, 12.5f, 2, 10.8f, 2, 8.5f);
        nvgBezierTo(vg, 2, 5.5f, 4.5f, 3, 7.5f, 3);
        nvgBezierTo(vg, 9.3f, 3, 10.5f, 3.5f, 12, 5);
        nvgBezierTo(vg, 13.5f, 3.5f, 14.7f, 3, 16.5f, 3);
        nvgBezierTo(vg, 19.5f, 3, 22, 5.5f, 22, 8.5f);
        nvgBezierTo(vg, 22, 10.8f, 20.5f, 12.5f, 19, 14);
        nvgClosePath(vg);
        nvgStroke(vg);
    }
    else if (i == "folder")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 4, 20);
        nvgLineTo(vg, 20, 20);
        nvgQuadTo(vg, 22, 20, 22, 18);
        nvgLineTo(vg, 22, 9);
        nvgQuadTo(vg, 22, 7, 20, 7);
        nvgLineTo(vg, 12, 7);
        nvgLineTo(vg, 10, 4);
        nvgLineTo(vg, 4, 4);
        nvgQuadTo(vg, 2, 4, 2, 6);
        nvgLineTo(vg, 2, 18);
        nvgQuadTo(vg, 2, 20, 4, 20);
        nvgClosePath(vg);
        nvgStroke(vg);
    }
    else if (i == "file")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 14, 2);
        nvgLineTo(vg, 6, 2);
        nvgQuadTo(vg, 4, 2, 4, 4);
        nvgLineTo(vg, 4, 20);
        nvgQuadTo(vg, 4, 22, 6, 22);
        nvgLineTo(vg, 18, 22);
        nvgQuadTo(vg, 20, 22, 20, 20);
        nvgLineTo(vg, 20, 8);
        nvgClosePath(vg);
        nvgMoveTo(vg, 14, 2);
        nvgLineTo(vg, 14, 8);
        nvgLineTo(vg, 20, 8);
        nvgMoveTo(vg, 8, 13);
        nvgLineTo(vg, 16, 13);
        nvgMoveTo(vg, 8, 17);
        nvgLineTo(vg, 14, 17);
        nvgStroke(vg);
    }
    else if (i == "flag")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 4, 22);
        nvgLineTo(vg, 4, 3);
        nvgQuadTo(vg, 8, 1, 12, 3);
        nvgQuadTo(vg, 16, 5, 20, 3);
        nvgLineTo(vg, 20, 15);
        nvgQuadTo(vg, 16, 17, 12, 15);
        nvgQuadTo(vg, 8, 13, 4, 15);
        nvgStroke(vg);
    }
    else if (i == "gamepad")
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 2, 7, 20, 11, 5.5f);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 6, 12.5f);
        nvgLineTo(vg, 10, 12.5f);
        nvgMoveTo(vg, 8, 10.5f);
        nvgLineTo(vg, 8, 14.5f);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, 15.5f, 13.5f, 1.1f);
        nvgCircle(vg, 18, 11, 1.1f);
        nvgFill(vg);
    }
    else if (i == "calendar")
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 3, 5, 18, 16, 2);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 3, 10);
        nvgLineTo(vg, 21, 10);
        nvgMoveTo(vg, 8, 3);
        nvgLineTo(vg, 8, 7);
        nvgMoveTo(vg, 16, 3);
        nvgLineTo(vg, 16, 7);
        nvgStroke(vg);
    }
    else if (i == "alert")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 12, 3);
        nvgLineTo(vg, 22, 20);
        nvgLineTo(vg, 2, 20);
        nvgClosePath(vg);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 12, 9);
        nvgLineTo(vg, 12, 14);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, 12, 17, 1.1f);
        nvgFill(vg);
    }
    else if (i == "pencil")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 4, 20);
        nvgLineTo(vg, 8, 20);
        nvgLineTo(vg, 19, 9);
        nvgLineTo(vg, 15, 5);
        nvgLineTo(vg, 4, 16);
        nvgClosePath(vg);
        nvgMoveTo(vg, 13, 7);
        nvgLineTo(vg, 17, 11);
        nvgStroke(vg);
    }
    else if (i == "pencil-off")
    { // read-only: the pencil, crossed out
        nvgBeginPath(vg);
        nvgMoveTo(vg, 4, 20);
        nvgLineTo(vg, 8, 20);
        nvgLineTo(vg, 19, 9);
        nvgLineTo(vg, 15, 5);
        nvgLineTo(vg, 4, 16);
        nvgClosePath(vg);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 3, 3);
        nvgLineTo(vg, 21, 21);
        nvgStroke(vg);
    }
    else if (i == "tag")
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, 3, 3);
        nvgLineTo(vg, 11, 3);
        nvgLineTo(vg, 21, 13);
        nvgLineTo(vg, 13, 21);
        nvgLineTo(vg, 3, 11);
        nvgClosePath(vg);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, 7.5f, 7.5f, 1.5f);
        nvgFill(vg);
    }
    else if (i == "user")
    {
        nvgBeginPath(vg);
        nvgCircle(vg, 12, 8, 4);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 4, 21);
        nvgQuadTo(vg, 4, 14, 12, 14);
        nvgQuadTo(vg, 20, 14, 20, 21);
        nvgStroke(vg);
    }
    else if (i == "snowflake")
    {
        // Winter: six arms, a chevron near each tip.
        nvgBeginPath(vg);
        for (int k = 0; k < 6; k++)
        {
            const float a  = (float)k * NVG_PI / 3 - NVG_PI / 2;
            const float bx = 12 + std::cos(a) * 5.5f, by = 12 + std::sin(a) * 5.5f;
            nvgMoveTo(vg, 12, 12);
            nvgLineTo(vg, 12 + std::cos(a) * 9.5f, 12 + std::sin(a) * 9.5f);
            nvgMoveTo(vg, bx + std::cos(a - 0.85f) * 3, by + std::sin(a - 0.85f) * 3);
            nvgLineTo(vg, bx, by);
            nvgLineTo(vg, bx + std::cos(a + 0.85f) * 3, by + std::sin(a + 0.85f) * 3);
        }
        nvgStroke(vg);
    }
    else if (i == "lantern")
    {
        // The Lunar New Year: a paper lantern, wider than tall, its ribs and
        // caps, the tassel below.
        nvgBeginPath(vg);
        nvgMoveTo(vg, 7.5f, 5.5f);
        nvgBezierTo(vg, 0.5f, 7, 0.5f, 16, 7.5f, 17.5f);
        nvgLineTo(vg, 16.5f, 17.5f);
        nvgBezierTo(vg, 23.5f, 16, 23.5f, 7, 16.5f, 5.5f);
        nvgClosePath(vg);
        nvgMoveTo(vg, 9.5f, 5.5f);
        nvgBezierTo(vg, 6, 8, 6, 15, 9.5f, 17.5f);
        nvgMoveTo(vg, 14.5f, 5.5f);
        nvgBezierTo(vg, 18, 8, 18, 15, 14.5f, 17.5f);
        nvgMoveTo(vg, 12, 20);
        nvgLineTo(vg, 12, 23);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 6, 2.6f, 12, 2.8f, 1);
        nvgRoundedRect(vg, 6, 17.4f, 12, 2.8f, 1);
        nvgFill(vg);
    }
    else if (i == "blossom")
    {
        // Spring: five round petals around a dot, notched between them.
        constexpr float R = 5.5f, r = 4.0f;
        const float half = NVG_PI / 5;
        // Where two petals' circles meet outside
        const float meet = R * std::cos(half) + std::sqrt(r * r - R * std::sin(half) * R * std::sin(half));
        nvgBeginPath(vg);
        for (int k = 0; k < 5; k++)
        {
            const float a  = (float)k * 2 * half - NVG_PI / 2;
            const float px = 12 + std::cos(a) * R, py = 12 + std::sin(a) * R;
            const float from = std::atan2(12 + std::sin(a - half) * meet - py, 12 + std::cos(a - half) * meet - px);
            const float to   = std::atan2(12 + std::sin(a + half) * meet - py, 12 + std::cos(a + half) * meet - px);
            nvgArc(vg, px, py, r, from, to, NVG_CW);
        }
        nvgClosePath(vg);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, 12, 12, 2);
        nvgFill(vg);
    }
    else if (i == "wave")
    {
        // The sea: three rows of waves.
        nvgBeginPath(vg);
        for (float y : { 7.0f, 12.0f, 17.0f })
        {
            nvgMoveTo(vg, 3, y);
            for (int k = 0; k < 4; k++)
                nvgQuadTo(vg, 5.25f + 4.5f * (float)k, y + (k % 2 ? 2.5f : -2.5f), 7.5f + 4.5f * (float)k, y);
        }
        nvgStroke(vg);
    }
    else if (i == "leaf")
    {
        // Autumn: a leaf, its midrib and stem.
        nvgBeginPath(vg);
        nvgMoveTo(vg, 6, 18);
        nvgBezierTo(vg, 4, 10, 10, 4, 20, 4);
        nvgBezierTo(vg, 20, 14, 14, 20, 6, 18);
        nvgClosePath(vg);
        nvgMoveTo(vg, 3, 21);
        nvgLineTo(vg, 15, 9);
        nvgStroke(vg);
    }
    else if (i == "pumpkin")
    {
        // Halloween: a squat ribbed pumpkin and its curled stem.
        nvgBeginPath(vg);
        nvgEllipse(vg, 12, 14.5f, 9.5f, 6.5f);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgEllipse(vg, 12, 14.5f, 3.8f, 6.5f);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, 12, 8);
        nvgQuadTo(vg, 12, 4.5f, 15.5f, 3.5f);
        nvgStroke(vg);
    }
    else if (i == "tree")
    {
        // Christmas: a fir of three tiers on its trunk.
        nvgBeginPath(vg);
        nvgMoveTo(vg, 12, 2);
        nvgLineTo(vg, 16, 7.5f);
        nvgLineTo(vg, 14, 7.5f);
        nvgLineTo(vg, 18, 13);
        nvgLineTo(vg, 15.5f, 13);
        nvgLineTo(vg, 19.5f, 18.5f);
        nvgLineTo(vg, 4.5f, 18.5f);
        nvgLineTo(vg, 8.5f, 13);
        nvgLineTo(vg, 6, 13);
        nvgLineTo(vg, 10, 7.5f);
        nvgLineTo(vg, 8, 7.5f);
        nvgClosePath(vg);
        nvgMoveTo(vg, 12, 18.5f);
        nvgLineTo(vg, 12, 22);
        nvgStroke(vg);
    }
    else if (i == "firework")
    {
        // New Year's Eve: a burst, long and short rays from a spark, no disc
        // (not the sun).
        nvgBeginPath(vg);
        for (int k = 0; k < 8; k++)
        {
            const float a    = (float)k * NVG_PI / 4 - NVG_PI / 2;
            const float from = k % 2 ? 4.5f : 3.5f, to = k % 2 ? 7.0f : 9.5f;
            nvgMoveTo(vg, 12 + std::cos(a) * from, 12 + std::sin(a) * from);
            nvgLineTo(vg, 12 + std::cos(a) * to, 12 + std::sin(a) * to);
        }
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, 12, 12, 1.3f);
        for (int k = 1; k < 8; k += 2)
        {
            const float a = (float)k * NVG_PI / 4 - NVG_PI / 2;
            nvgCircle(vg, 12 + std::cos(a) * 9.6f, 12 + std::sin(a) * 9.6f, 1.2f);
        }
        nvgFill(vg);
    }
    else if (i == "gem")
    {
        // Amethyst: a cut stone, its table and facets.
        nvgBeginPath(vg);
        nvgMoveTo(vg, 7, 4);
        nvgLineTo(vg, 17, 4);
        nvgLineTo(vg, 21.5f, 9.5f);
        nvgLineTo(vg, 12, 21);
        nvgLineTo(vg, 2.5f, 9.5f);
        nvgClosePath(vg);
        nvgMoveTo(vg, 2.5f, 9.5f);
        nvgLineTo(vg, 21.5f, 9.5f);
        nvgMoveTo(vg, 8, 9.5f);
        nvgLineTo(vg, 12, 4);
        nvgLineTo(vg, 16, 9.5f);
        nvgMoveTo(vg, 8, 9.5f);
        nvgLineTo(vg, 12, 21);
        nvgLineTo(vg, 16, 9.5f);
        nvgStroke(vg);
    }
    else if (i == "dune")
    {
        // Sand: two dunes in a row, the sun above them.
        nvgBeginPath(vg);
        nvgMoveTo(vg, 2, 19);
        nvgBezierTo(vg, 5, 11.5f, 9, 11, 13, 16.5f);
        nvgBezierTo(vg, 15.5f, 13.5f, 19, 13, 22, 17);
        nvgMoveTo(vg, 2, 21.5f);
        nvgLineTo(vg, 22, 21.5f);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, 17.5f, 6.5f, 2.5f);
        nvgStroke(vg);
    }
    else if (i == "palette")
    {
        // A painter's palette: colours of one's own.
        nvgBeginPath(vg);
        nvgMoveTo(vg, 12, 3);
        nvgBezierTo(vg, 17, 3, 21, 6.5f, 21, 11);
        nvgBezierTo(vg, 21, 14.5f, 18, 15, 16, 15);
        nvgBezierTo(vg, 14, 15, 13.5f, 17, 14.5f, 18.5f);
        nvgBezierTo(vg, 15.5f, 20, 14.5f, 21, 12, 21);
        nvgBezierTo(vg, 7, 21, 3, 17, 3, 12);
        nvgBezierTo(vg, 3, 7, 7, 3, 12, 3);
        nvgClosePath(vg);
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, 8, 10, 1.4f);
        nvgCircle(vg, 12, 7.3f, 1.4f);
        nvgCircle(vg, 16.2f, 9.5f, 1.4f);
        nvgCircle(vg, 7.8f, 14.8f, 1.4f);
        nvgFill(vg);
    }
    else
    {
        known = false;
    }
    nvgRestore(vg);
    return known;
}

brls::View* CadranIcon::create() { return new CadranIcon(); }

const std::vector<std::string>& CadranIcon::names()
{
    // Each name glyph() draws, but "mark" (the theme's `logo`)
    static const std::vector<std::string> NAMES = {
        "today",
        "timer",
        "activity",
        "shield",
        "lock",
        "globe",
        "settings",
        "tools",
        "shelf",
        "info",
        "plus",
        "stop",
        "eye",
        "sun",
        "tv",
        "text-normal",
        "text-large",
        "text-largest",
        "moon",
        "pause",
        "list",
        "rail-open",
        "rail-icons",
        "rail-auto",
        "lines",
        "download",
        "chevron-down",
        "chevron-left",
        "chevron-right",
        "caret-up",
        "caret-down",
        "check",
        "key",
        "unlock",
        "phone",
        "trash",
        "server",
        "pin",
        "refresh",
        "history",
        "restore",
        "save",
        "upload",
        "language",
        "contrast",
        "home",
        "bolt",
        "sparkle",
        "bell",
        "chip",
        "code",
        "heart",
        "folder",
        "file",
        "flag",
        "gamepad",
        "calendar",
        "alert",
        "pencil",
        "pencil-off",
        "tag",
        "user",
        "snowflake",
        "lantern",
        "blossom",
        "wave",
        "leaf",
        "pumpkin",
        "tree",
        "firework",
        "gem",
        "dune",
        "palette",
    };
    return NAMES;
}

CadranSwatch::CadranSwatch(Colors value)
    : colors(std::move(value))
{
    this->setWidth(36);
    this->setHeight(36);
    this->setFocusable(false);
    if (!this->colors.picture.empty())
    {
        brls::ThemeImageOptions options;
        options.flags   = NVG_IMAGE_GENERATE_MIPMAPS;
        options.maxSide = brls::ThemeImages::PREVIEW_SIZE;
        this->preview   = brls::ThemeImages::load(this->colors.picture, options);
    }
}

void CadranSwatch::draw(NVGcontext* vg, float x, float y, float width, float height,
    brls::Style style, brls::FrameContext* ctx)
{
    (void)style;
    (void)ctx;
    const float corner  = std::min(width, height) * 0.28f;
    const auto hairline = [&]
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + 0.5f, y + 0.5f, width - 1, height - 1, corner - 0.5f);
        nvgStrokeColor(vg, brls::Application::getTheme()["cadran/line"]);
        nvgStrokeWidth(vg, 1);
        nvgStroke(vg);
    };
    // The theme's own thumbnail, once loaded
    if (this->preview && this->preview->draw(vg, x, y, width, height, brls::BackgroundFit::COVER, nullptr, 1.0f, corner))
    {
        hairline();
        return;
    }
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, width, height, corner);
    nvgFillColor(vg, this->colors.background);
    nvgFill(vg);
    // The surface as a ring inside the square
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x + 3.25f, y + 3.25f, width - 6.5f, height - 6.5f, corner - 2.5f);
    nvgStrokeColor(vg, this->colors.ring);
    nvgStrokeWidth(vg, 2.5f);
    nvgStroke(vg);
    // The hairline, in the sheet's own colours
    hairline();

    const float side = std::min(width, height) * 0.55f;
    nvgSave(vg);
    nvgTranslate(vg, x + (width - side) / 2, y + (height - side) / 2);
    if (this->colors.motif.empty() || !CadranIcon::glyph(vg, this->colors.motif, this->colors.ink, side, side))
        CadranIcon::glyph(vg, "sparkle", this->colors.ink, side, side);
    nvgRestore(vg);
}
