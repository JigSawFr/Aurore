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

#include "view/aurora_view.hpp"

#include <cmath>
#include <cstdint>

// Boréale's night and its aurora (aurore.boreale.dev): the abyss and the
// night, the aurora green, the ice blue and the violet.
const NVGcolor AuroraView::SNOW = nvgRGB(0xE8, 0xEE, 0xF7);
const NVGcolor AuroraView::MIST = nvgRGB(0xA9, 0xB6, 0xCC);

namespace
{
const NVGcolor NIGHT_TOP    = nvgRGB(0x12, 0x21, 0x3F);
const NVGcolor NIGHT_BOTTOM = nvgRGB(0x0B, 0x14, 0x24);
const NVGcolor RIDGE_FAR    = nvgRGB(0x16, 0x27, 0x4A);
const NVGcolor RIDGE_NEAR   = nvgRGB(0x0A, 0x12, 0x22);

struct Curtain
{
    unsigned char r, g, b;
    float base; // the bottom edge, as a share of the height
    float tall; // its height, as a share
    float amplitude, wave, speed, phase;
    float alpha;
};

const Curtain CURTAINS[] = {
    { 0x8A, 0x5C, 0xFF, 0.40f, 0.34f, 0.07f, 1.7f, 0.21f, 0.6f, 0.34f }, // violet, behind
    { 0x33, 0xBB, 0xFF, 0.52f, 0.40f, 0.08f, 2.3f, -0.27f, 2.1f, 0.30f }, // ice
    { 0x00, 0xFF, 0xCC, 0.58f, 0.46f, 0.10f, 1.3f, 0.33f, 4.0f, 0.42f }, // aurora, in front
};

// A ridge's height at `u` (0 to 1 across), as a share of the height: a sum
// of a few slopes, the same at every frame.
float ridge(float u, float seed, float base)
{
    return base + 0.05f * std::sin(u * 7.0f + seed) + 0.03f * std::sin(u * 17.0f + seed * 2.3f)
        + 0.02f * std::sin(u * 31.0f + seed * 0.7f);
}

// A filled ridge, its bottom corners rounded with the panel's
void drawRidge(NVGcontext* vg, float x, float y, float width, float height, float radius, float seed, float base,
    NVGcolor color)
{
    const int steps = 48;
    nvgBeginPath(vg);
    nvgMoveTo(vg, x, y + height * ridge(0, seed, base));
    for (int i = 1; i <= steps; i++)
    {
        float u = (float)i / steps;
        nvgLineTo(vg, x + width * u, y + height * ridge(u, seed, base));
    }
    nvgLineTo(vg, x + width, y + height - radius);
    nvgArcTo(vg, x + width, y + height, x + width - radius, y + height, radius);
    nvgLineTo(vg, x + radius, y + height);
    nvgArcTo(vg, x, y + height, x, y + height - radius, radius);
    nvgClosePath(vg);
    nvgFillColor(vg, color);
    nvgFill(vg);
}
} // namespace

AuroraView::AuroraView()
    : aurore::Box(aurore::Axis::COLUMN)
{
    // The same sky at every start: a small generator of its own
    uint32_t state = 0x2545F491u;
    auto next      = [&state]()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return (state & 0xFFFFFF) / (float)0x1000000;
    };
    for (int i = 0; i < 64; i++)
    {
        Star star;
        star.x     = next();
        star.y     = next() * next() * 0.75f; // more near the top
        star.size  = 0.6f + next() * 1.1f;
        star.group = i % 3;
        this->stars.push_back(star);
    }
}

float AuroraView::seconds() const
{
    if (aurore::Application::getReduceMotion() || aurore::Application::isRedrawOnDemand())
        return 7.5f; // a moment where the curtains are spread
    return (float)(aurore::getCPUTimeUsec() % 3600000000ull) / 1000000.0f;
}

void AuroraView::draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
    aurore::FrameContext* ctx)
{
    const float r = std::fmin(this->radius, std::fmin(width, height) / 2);
    const float t = this->seconds();

    // The night, and a glow of each colour where the curtains are
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, width, height, r);
    nvgFillPaint(vg, nvgLinearGradient(vg, x, y, x, y + height, NIGHT_TOP, NIGHT_BOTTOM));
    nvgFill(vg);
    const struct
    {
        float cx, cy, size;
        NVGcolor color;
    } glows[] = {
        { 0.28f, 0.10f, 0.75f, nvgRGBA(0x00, 0xFF, 0xCC, 40) },
        { 0.78f, 0.05f, 0.65f, nvgRGBA(0x8A, 0x5C, 0xFF, 48) },
        { 0.55f, 0.35f, 0.50f, nvgRGBA(0x33, 0xBB, 0xFF, 26) },
    };
    for (const auto& glow : glows)
    {
        float cx = x + width * glow.cx, cy = y + height * glow.cy;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, width, height, r);
        nvgFillPaint(vg, nvgRadialGradient(vg, cx, cy, 0, width * glow.size, glow.color, nvgRGBA(0, 0, 0, 0)));
        nvgFill(vg);
    }

    // The stars, in three groups twinkling each at its pace: three paths
    for (int group = 0; group < 3; group++)
    {
        float twinkle = 0.55f + 0.35f * std::sin(t * (0.9f + group * 0.45f) + group * 2.1f);
        nvgBeginPath(vg);
        for (const Star& star : this->stars)
        {
            if (star.group != group)
                continue;
            float sx = x + r + (width - 2 * r) * star.x;
            float sy = y + r * 0.6f + (height - r) * star.y;
            nvgCircle(vg, sx, sy, star.size);
        }
        nvgFillColor(vg, nvgRGBAf(SNOW.r, SNOW.g, SNOW.b, twinkle));
        nvgFill(vg);
    }

    // The curtains: a lit lower edge that sways, fading upwards. Their tops
    // are transparent and their edges stay above the mountains, so the
    // panel's corners need no clipping; the sides are cut straight.
    nvgSave(vg);
    nvgIntersectScissor(vg, x, y, width, height);
    const int steps = 40;
    for (const Curtain& c : CURTAINS)
    {
        auto edge = [&](float u)
        {
            return c.base + c.amplitude * std::sin(u * c.wave * 2 * NVG_PI + t * c.speed + c.phase)
                + c.amplitude * 0.45f * std::sin(u * c.wave * 5.1f + t * c.speed * 1.7f);
        };
        auto top = [&](float u)
        {
            return edge(u) - c.tall * (0.75f + 0.25f * std::sin(u * 9.0f + t * 0.5f + c.phase));
        };
        float lowest = y + height * (c.base + c.amplitude * 1.45f);
        nvgBeginPath(vg);
        nvgMoveTo(vg, x, y + height * edge(0));
        for (int i = 1; i <= steps; i++)
        {
            float u = (float)i / steps;
            nvgLineTo(vg, x + width * u, y + height * edge(u));
        }
        for (int i = steps; i >= 0; i--)
        {
            float u = (float)i / steps;
            nvgLineTo(vg, x + width * u, y + height * top(u));
        }
        nvgClosePath(vg);
        nvgFillPaint(vg,
            nvgLinearGradient(vg, x, lowest, x, lowest - height * (c.tall + c.amplitude * 2), nvgRGBA(c.r, c.g, c.b, (unsigned char)(255 * c.alpha)),
                nvgRGBA(c.r, c.g, c.b, 0)));
        nvgFill(vg);

        // Its lit edge
        nvgBeginPath(vg);
        nvgMoveTo(vg, x, y + height * edge(0));
        for (int i = 1; i <= steps; i++)
        {
            float u = (float)i / steps;
            nvgLineTo(vg, x + width * u, y + height * edge(u));
        }
        nvgStrokeWidth(vg, 2.0f);
        nvgStrokeColor(vg, nvgRGBA(c.r, c.g, c.b, (unsigned char)(255 * c.alpha * 1.3f)));
        nvgStroke(vg);
    }
    nvgRestore(vg);

    // A veil on the left, where the text goes: the curtains stay behind it
    if (this->veil)
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, width, height, r);
        nvgFillPaint(vg,
            nvgLinearGradient(vg, x + width * 0.15f, y, x + width * 0.75f, y, nvgRGBA(0x0B, 0x14, 0x24, 200),
                nvgRGBA(0x0B, 0x14, 0x24, 0)));
        nvgFill(vg);
    }

    if (this->ridges)
    {
        drawRidge(vg, x, y, width, height, r, 1.3f, 0.80f, RIDGE_FAR);
        drawRidge(vg, x, y, width, height, r, 4.1f, 0.90f, RIDGE_NEAR);
    }

    // A hairline around it, as Cadran's cards have
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x + 0.5f, y + 0.5f, width - 1, height - 1, r);
    nvgStrokeWidth(vg, 1.0f);
    nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 30));
    nvgStroke(vg);

    // Every frame is drawn but with redraw on demand, where it stands still
    aurore::Box::draw(vg, x, y, width, height, style, ctx);
}

aurore::View* AuroraView::create()
{
    return new AuroraView();
}
