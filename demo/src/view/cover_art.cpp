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

#include "view/cover_art.hpp"

#include <aurore/cadran/ui/format_text.hpp>

#include <algorithm>
#include <cmath>
#include <functional>

namespace library
{
namespace
{
    // A small generator of its own: the same library at every start
    struct Random
    {
        uint32_t state;
        uint32_t next()
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return state;
        }
        float unit() { return (next() & 0xFFFFFF) / (float)0x1000000; }
        int between(int low, int high) { return low + (int)(next() % (uint32_t)(high - low + 1)); }
    };

    // Two words each, made up for the demo
    const char* const FIRST[]  = { "Ember", "Hollow", "Lumen", "Moss", "Tidal", "Velvet", "Iron", "Paper", "Quiet", "Copper",
         "Frost", "Solar", "Willow", "Rune", "Pixel", "Echo", "Cinder", "Marble", "Amber", "Cobalt", "Saffron", "Juniper",
         "Glass", "Thistle" };
    const char* const SECOND[] = { "Tides", "Lantern", "Keep", "Orchard", "Circuit", "Voyage", "Signal", "Harbor", "Spire",
        "Rally", "Meadow", "Atlas", "Parade", "Cascade", "Hearth", "Comet" };
} // namespace

const std::vector<Game>& games()
{
    static std::vector<Game> list = []
    {
        std::vector<Game> all;
        Random random { 0x9E3779B9u };
        const size_t pairs = std::size(FIRST) * std::size(SECOND);
        std::vector<size_t> order(pairs);
        for (size_t i = 0; i < pairs; i++)
            order[i] = i;
        for (size_t i = pairs - 1; i > 0; i--)
            std::swap(order[i], order[random.next() % (i + 1)]);
        for (size_t i = 0; i < 302; i++)
        {
            Game game;
            game.title        = std::string(FIRST[order[i] / std::size(SECOND)]) + " " + SECOND[order[i] % std::size(SECOND)];
            game.genre        = (Genre)(random.next() % GENRE_COUNT);
            game.year         = random.between(2009, 2026);
            game.minutes      = random.unit() < 0.15f ? 0 : random.between(20, 9000);
            game.players      = random.unit() < 0.6f ? 1 : random.between(2, 4);
            game.achievements = random.between(12, 60);
            game.completion   = game.minutes == 0 ? 0.0f : std::fmin(1.0f, random.unit() * 1.1f);
            game.achieved     = (int)std::lround(game.achievements * game.completion);
            game.seed         = random.next();
            game.recent       = (int)i;
            all.push_back(game);
        }
        // Played last: a random order of its own
        for (size_t i = all.size() - 1; i > 0; i--)
            std::swap(all[i].recent, all[random.next() % (i + 1)].recent);
        return all;
    }();
    return list;
}

std::string genreName(Genre genre)
{
    static const char* const KEYS[] = { "adventure", "racing", "puzzle", "platformer", "strategy", "rhythm", "space",
        "ocean" };
    return aurore::getStr(std::string("demo/library/genres/") + KEYS[genre]);
}

const char* genreIcon(Genre genre)
{
    static const char* const ICONS[] = { "flag", "bolt", "chip", "sparkle", "shield", "bell", "globe", "wave" };
    return ICONS[genre];
}

std::string playtime(int minutes)
{
    return ui::fmt_minutes((uint32_t)minutes);
}
} // namespace library

using library::Genre;

namespace
{
// A shape filling the foot of the cover, under `top(u)` (u from 0 to 1
// across, a share of the height): its bottom corners rounded with the cover
void fillFoot(NVGcontext* vg, float x, float y, float w, float h, float r, const std::function<float(float)>& top,
    NVGpaint paint, int steps = 32)
{
    nvgBeginPath(vg);
    nvgMoveTo(vg, x, y + h * std::min(top(0), 1 - r / h));
    for (int i = 1; i <= steps; i++)
    {
        float u = (float)i / steps;
        nvgLineTo(vg, x + w * u, y + h * top(u));
    }
    nvgLineTo(vg, x + w, y + h - r);
    nvgArcTo(vg, x + w, y + h, x + w - r, y + h, r);
    nvgLineTo(vg, x + r, y + h);
    nvgArcTo(vg, x, y + h, x, y + h - r, r);
    nvgClosePath(vg);
    nvgFillPaint(vg, paint);
    nvgFill(vg);
}

NVGpaint flat(NVGcontext* vg, NVGcolor color)
{
    return nvgLinearGradient(vg, 0, 0, 0, 1, color, color);
}

// The genres' hues (degrees), shifted a little for each game
const float HUES[] = { 150, 320, 45, 200, 10, 280, 230, 190 };
} // namespace

CoverArt::CoverArt()
{
    this->setFocusable(false);
}

void CoverArt::setGame(const library::Game* game)
{
    this->game = game;
    aurore::Application::requestRedraw();
}

std::string CoverArt::getDefaultAccessibilityLabel()
{
    return this->game ? this->game->title : "";
}

void CoverArt::draw(NVGcontext* vg, float x, float y, float w, float h, aurore::Style style, aurore::FrameContext* ctx)
{
    if (!this->game || w <= 0 || h <= 0)
        return;
    const library::Game& g = *this->game;
    const float r          = std::min(this->radius, std::min(w, h) / 2);
    uint32_t seed          = g.seed;
    auto unit              = [&seed]()
    {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        return (seed & 0xFFFF) / 65536.0f;
    };

    float hue      = std::fmod(HUES[g.genre] + unit() * 50 - 25 + 360, 360) / 360.0f;
    float shift    = (unit() < 0.5f ? 1 : -1) * (0.08f + unit() * 0.1f);
    NVGcolor deep  = nvgHSL(hue, 0.55f, 0.16f);
    NVGcolor light = nvgHSL(std::fmod(hue + shift + 1, 1), 0.65f, 0.42f);
    NVGcolor ink   = nvgHSL(std::fmod(hue + shift * 2 + 1, 1), 0.85f, 0.70f);
    NVGcolor glow  = nvgHSLA(std::fmod(hue + 0.5f, 1), 0.9f, 0.65f, 255);
    float s        = std::min(w, h); // the motifs' scale

    // The ground of it
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, w, h, r);
    nvgFillPaint(vg, nvgLinearGradient(vg, x, y, x + w * 0.3f, y + h, light, deep));
    nvgFill(vg);

    switch (g.genre)
    {
        case library::ADVENTURE:
        {
            // A sun behind two ranges of mountains
            float cx = x + w * (0.3f + unit() * 0.4f), cy = y + h * 0.36f;
            nvgBeginPath(vg);
            nvgCircle(vg, cx, cy, s * 0.16f);
            nvgFillColor(vg, nvgRGBAf(ink.r, ink.g, ink.b, 0.9f));
            nvgFill(vg);
            float a = unit() * 6, b = unit() * 6;
            fillFoot(vg, x, y, w, h, r, [a](float u)
                { return 0.55f + 0.12f * std::sin(u * 9 + a) * std::sin(u * 3.3f + a) - 0.05f * std::sin(u * 21 + a); }, flat(vg, nvgRGBAf(deep.r * 1.6f, deep.g * 1.6f, deep.b * 1.6f, 1)));
            fillFoot(vg, x, y, w, h, r, [b](float u)
                { return 0.68f + 0.08f * std::sin(u * 7 + b) - 0.04f * std::fabs(std::sin(u * 17 + b)); }, flat(vg, deep));
            break;
        }
        case library::RACING:
        {
            // A road to the horizon under a striped sun
            float horizon = y + h * 0.55f, cx = x + w / 2;
            nvgBeginPath(vg);
            nvgCircle(vg, cx, horizon - s * 0.02f, s * 0.24f);
            nvgFillPaint(vg, nvgLinearGradient(vg, cx, horizon - s * 0.26f, cx, horizon, ink, glow));
            nvgFill(vg);
            nvgBeginPath(vg);
            for (int i = 0; i < 4; i++)
                nvgRect(vg, cx - s * 0.26f, horizon - s * 0.03f - i * s * 0.045f, s * 0.52f, 2 + i * 0.6f);
            nvgFillColor(vg, deep);
            nvgFill(vg);
            fillFoot(vg, x, y, w, h, r, [](float)
                { return 0.55f; }, flat(vg, deep));
            nvgBeginPath(vg);
            for (int i = -4; i <= 4; i++)
            {
                nvgMoveTo(vg, cx + i * w * 0.02f, horizon);
                nvgLineTo(vg, std::clamp(cx + i * w * 0.2f, x + r, x + w - r), y + h - 1);
            }
            for (int i = 1; i <= 5; i++)
            {
                float ly = horizon + (y + h - r - horizon) * (i * i) / 25.0f;
                nvgMoveTo(vg, x + r * 0.5f, ly);
                nvgLineTo(vg, x + w - r * 0.5f, ly);
            }
            nvgStrokeWidth(vg, 1.5f);
            nvgStrokeColor(vg, nvgRGBAf(glow.r, glow.g, glow.b, 0.65f));
            nvgStroke(vg);
            break;
        }
        case library::PUZZLE:
        {
            // Blocks, a few lit
            int cols   = 4;
            float cell = (w - r * 2) / cols, top = y + h * 0.16f;
            for (int row = 0; row < 4; row++)
                for (int col = 0; col < cols; col++)
                {
                    float lit = unit();
                    if (lit < 0.25f)
                        continue;
                    nvgBeginPath(vg);
                    nvgRoundedRect(vg, x + r + col * cell + 3, top + row * cell + 3, cell - 6, cell - 6, 5);
                    nvgFillColor(vg, lit > 0.8f ? ink : nvgRGBAf(1, 1, 1, 0.10f + lit * 0.12f));
                    nvgFill(vg);
                }
            break;
        }
        case library::PLATFORMER:
        {
            // Floating ledges, coins, a ground
            for (int i = 0; i < 3; i++)
            {
                float px = x + r + unit() * (w - 2 * r - s * 0.4f), py = y + h * (0.2f + i * 0.16f);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, px, py, s * 0.4f, s * 0.07f, s * 0.035f);
                nvgFillColor(vg, nvgRGBAf(1, 1, 1, 0.85f));
                nvgFill(vg);
                nvgBeginPath(vg);
                for (int c = 0; c < 3; c++)
                    nvgCircle(vg, px + s * (0.08f + c * 0.12f), py - s * 0.07f, s * 0.035f);
                nvgFillColor(vg, ink);
                nvgFill(vg);
            }
            fillFoot(vg, x, y, w, h, r, [](float u)
                { return 0.78f + (std::fmod(u * 5, 1.0f) < 0.5f ? 0.0f : 0.03f); }, flat(vg, deep), 40);
            break;
        }
        case library::STRATEGY:
        {
            // Towers of a keep under a banner sky
            float a = unit() * 3;
            fillFoot(vg, x, y, w, h, r, [a](float u)
                {
                float step = std::floor(u * 9);
                float tall = 0.42f + 0.18f * std::fabs(std::sin(step * 1.7f + a));
                return std::fmod(u * 9, 1.0f) < 0.18f ? tall + 0.05f : tall; }, flat(vg, deep), 90);
            nvgBeginPath(vg);
            nvgMoveTo(vg, x + w * 0.5f, y + h * 0.14f);
            nvgLineTo(vg, x + w * 0.5f, y + h * 0.36f);
            nvgStrokeWidth(vg, 2);
            nvgStrokeColor(vg, nvgRGBAf(1, 1, 1, 0.8f));
            nvgStroke(vg);
            nvgBeginPath(vg);
            nvgMoveTo(vg, x + w * 0.5f, y + h * 0.14f);
            nvgLineTo(vg, x + w * 0.5f + s * 0.18f, y + h * 0.18f);
            nvgLineTo(vg, x + w * 0.5f, y + h * 0.22f);
            nvgFillColor(vg, ink);
            nvgFill(vg);
            break;
        }
        case library::RHYTHM:
        {
            // An equaliser
            int bars    = 9;
            float width = (w - r * 2) / bars, base = y + h * 0.66f;
            for (int i = 0; i < bars; i++)
            {
                float tall = h * (0.08f + 0.4f * unit());
                nvgBeginPath(vg);
                nvgRoundedRect(vg, x + r + i * width + 2, base - tall, width - 4, tall, (width - 4) / 2);
                nvgFillPaint(vg, nvgLinearGradient(vg, 0, base - h * 0.48f, 0, base, glow, ink));
                nvgFill(vg);
            }
            break;
        }
        case library::SPACE:
        {
            // A ringed planet among stars
            nvgBeginPath(vg);
            for (int i = 0; i < 24; i++)
                nvgCircle(vg, x + r + unit() * (w - 2 * r), y + r + unit() * (h * 0.7f - r), 0.6f + unit() * 1.2f);
            nvgFillColor(vg, nvgRGBAf(1, 1, 1, 0.8f));
            nvgFill(vg);
            float cx = x + w * (0.4f + unit() * 0.2f), cy = y + h * 0.42f, pr = s * 0.22f;
            nvgBeginPath(vg);
            nvgCircle(vg, cx, cy, pr);
            nvgFillPaint(vg, nvgRadialGradient(vg, cx - pr * 0.4f, cy - pr * 0.4f, pr * 0.1f, pr * 1.2f, ink, deep));
            nvgFill(vg);
            nvgSave(vg);
            nvgTranslate(vg, cx, cy);
            nvgRotate(vg, -0.35f);
            nvgScale(vg, 1, 0.28f);
            nvgBeginPath(vg);
            nvgCircle(vg, 0, 0, pr * 1.65f);
            nvgStrokeWidth(vg, 5);
            nvgStrokeColor(vg, nvgRGBAf(glow.r, glow.g, glow.b, 0.85f));
            nvgStroke(vg);
            nvgRestore(vg);
            break;
        }
        case library::OCEAN:
        default:
        {
            // A moon over waves
            nvgBeginPath(vg);
            nvgCircle(vg, x + w * 0.7f, y + h * 0.24f, s * 0.1f);
            nvgFillColor(vg, nvgRGBAf(1, 1, 1, 0.85f));
            nvgFill(vg);
            for (int i = 0; i < 3; i++)
            {
                float a = unit() * 6, level = 0.5f + i * 0.1f;
                NVGcolor c = nvgLerpRGBA(light, deep, 0.35f + i * 0.3f);
                fillFoot(vg, x, y, w, h, r, [a, level](float u)
                    { return level + 0.025f * std::sin(u * 14 + a); }, flat(vg, c), 40);
            }
            break;
        }
    }

    // The title on a shade at the foot, in the display face
    if (this->titleShown)
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, r);
        nvgFillPaint(vg, nvgLinearGradient(vg, x, y + h * 0.55f, x, y + h, nvgRGBA(0, 0, 0, 0), nvgRGBA(0, 0, 0, 170)));
        nvgFill(vg);

        int face = aurore::Application::getFont("cadran/digits");
        if (face == aurore::FONT_INVALID)
            face = aurore::Application::getFont(aurore::FONT_REGULAR);
        float size = std::clamp(s * 0.13f, 13.0f, 34.0f);
        float pad  = std::max(10.0f, s * 0.07f);
        nvgFontFaceId(vg, face);
        nvgFontSize(vg, size);
        nvgTextLineHeight(vg, 1.05f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
        float bounds[4];
        nvgTextBoxBounds(vg, 0, 0, w - pad * 2, g.title.c_str(), nullptr, bounds);
        float tall = bounds[3] - bounds[1];
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(vg, nvgRGB(0xF4, 0xF7, 0xFB));
        nvgTextBox(vg, x + pad, y + h - pad - tall, w - pad * 2, g.title.c_str(), nullptr);
    }

    // A hairline, as every card has
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x + 0.5f, y + 0.5f, w - 1, h - 1, r);
    nvgStrokeWidth(vg, 1);
    nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 28));
    nvgStroke(vg);
}
