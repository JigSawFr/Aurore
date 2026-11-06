// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

#include <aurore/cadran/view/loading_dial.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
constexpr float PI = 3.14159265f;
// Where the gate has it on its 1280 x 720 screen.
constexpr float CX = 1196.0f, CY = 340.0f;

constexpr const char* LINE  = "cadran/dial/line";
constexpr const char* TRACK = "cadran/dial/track";
constexpr const char* TEAL  = "cadran/brand/fill";

// `c` with its alpha scaled by `a` (0..1).
NVGcolor faded(NVGcolor c, float a)
{
    c.a *= a;
    return c;
}
} // namespace

LoadingDial::LoadingDial()
{
    this->setFocusable(false);
}

void LoadingDial::draw(NVGcontext* vg, float x, float y, float width, float height,
    aurore::Style style, aurore::FrameContext* ctx)
{
    (void)width;
    (void)height;
    (void)style;
    const float cx = x + CX, cy = y + CY;
    const NVGcolor line_ink = ctx->theme[LINE], teal = ctx->theme[TEAL];
    // A line `strength` strong (0..1).
    const auto line = [line_ink](float strength)
    {
        NVGcolor c = line_ink;
        c.a        = (float)std::lround(std::min(1.0f, strength * 2.0f * c.a) * 255.0f) / 255.0f;
        return c;
    };

    // The teal glow.
    NVGpaint glow = nvgRadialGradient(vg, cx, cy, 0.0f, 290.0f, faded(teal, 23 / 255.0f), faded(teal, 0));
    nvgBeginPath(vg);
    nvgCircle(vg, cx, cy, 290.0f);
    nvgFillPaint(vg, glow);
    nvgFill(vg);

    // Two rings.
    nvgStrokeWidth(vg, 1.5f);
    for (const auto& [r, a] : { std::pair<float, float> { 226.0f, 0.05f }, { 138.0f, 0.04f } })
    {
        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, r);
        nvgStrokeColor(vg, line(a));
        nvgStroke(vg);
    }

    // The minutes, longer every five.
    for (int i = 0; i < 60; i++)
    {
        const float ang = (float)i * 2.0f * PI / 60.0f, len = i % 5 == 0 ? 16.0f : 7.0f;
        const float sx = std::sin(ang), sy = -std::cos(ang);
        nvgBeginPath(vg);
        nvgMoveTo(vg, cx + sx * 235.0f, cy + sy * 235.0f);
        nvgLineTo(vg, cx + sx * (235.0f + len), cy + sy * (235.0f + len));
        const float a = i % 5 == 0 ? 0.12f : 0.06f;
        nvgStrokeColor(vg, line(a));
        nvgStrokeWidth(vg, 1.5f);
        nvgStroke(vg);
    }

    // The day's track, three quarters of it in teal from the top.
    nvgStrokeWidth(vg, 18.0f);
    nvgBeginPath(vg);
    nvgCircle(vg, cx, cy, 182.0f);
    nvgStrokeColor(vg, ctx->theme[TRACK]);
    nvgStroke(vg);
    nvgBeginPath(vg);
    nvgArc(vg, cx, cy, 182.0f, -PI / 2.0f, -PI / 2.0f + 1.5f * PI, NVG_CW);
    nvgStrokeColor(vg, faded(teal, 41 / 255.0f));
    nvgLineCap(vg, NVG_ROUND);
    nvgStroke(vg);
    nvgLineCap(vg, NVG_BUTT);
}

aurore::View* LoadingDial::create()
{
    return new LoadingDial();
}
