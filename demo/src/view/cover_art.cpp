// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include "view/cover_art.hpp"

#include <aurore/cadran/ui/format_text.hpp>

#include <algorithm>
#include <cmath>

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

namespace
{
// The genres' hues (degrees), shifted a little for each game: the ground
// drawn while its picture loads, and if it cannot be read
const float HUES[] = { 150, 320, 45, 200, 10, 280, 230, 190 };

// The pictures each genre has in resources/img/covers (tools/make_covers.py,
// which draws them), in the order of library::Genre
const char* const NAMES[] = { "adventure", "racing", "puzzle", "platformer", "strategy", "rhythm", "space", "ocean" };
const int COVERS[]        = { 4, 3, 4, 4, 4, 3, 3, 3 };
} // namespace

CoverArt::CoverArt()
{
    this->setFocusable(false);
}

void CoverArt::setGame(const library::Game* game)
{
    this->game = game;
    if (game)
    {
        // One of its genre's pictures, and which way round: the seed's
        aurore::ThemeImageOptions options;
        options.flags    = NVG_IMAGE_GENERATE_MIPMAPS; // smooth in the list's small covers
        std::string path = std::string("@res/img/covers/") + NAMES[game->genre] + "_"
            + std::to_string((game->seed >> 8) % COVERS[game->genre]) + ".jpg";
        this->picture = aurore::ThemeImages::load(path, options);
    }
    else
        this->picture.reset();
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
    const float s          = std::min(w, h);

    // The picture, mirrored for half the games; a gradient of the genre's
    // colours until it is there
    bool drawn = false;
    if (this->picture && this->picture->ready())
    {
        nvgSave(vg);
        if (g.seed & 1)
        {
            nvgTranslate(vg, x * 2 + w, 0);
            nvgScale(vg, -1, 1);
        }
        drawn = this->picture->draw(vg, x, y, w, h, aurore::BackgroundFit::COVER, nullptr, 1.0f, r);
        nvgRestore(vg);
    }
    if (!drawn)
    {
        float hue      = std::fmod(HUES[g.genre] + (g.seed & 0xFF) / 255.0f * 50 - 25 + 360, 360) / 360.0f;
        NVGcolor deep  = nvgHSL(hue, 0.55f, 0.16f);
        NVGcolor light = nvgHSL(hue, 0.65f, 0.42f);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, r);
        nvgFillPaint(vg, nvgLinearGradient(vg, x, y, x + w * 0.3f, y + h, light, deep));
        nvgFill(vg);
    }

    // The title on a shade at the foot, in the display face
    if (this->titleShown)
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, r);
        nvgFillPaint(vg, nvgLinearGradient(vg, x, y + h * 0.5f, x, y + h, nvgRGBA(0, 0, 0, 0), nvgRGBA(0, 0, 0, 190)));
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
