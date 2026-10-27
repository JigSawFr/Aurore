/*
    Copyright 2019 natinusala
    Copyright 2019 p-sam
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

#include <borealis/core/application.hpp>
#include <borealis/core/assets.hpp>
#include <borealis/core/hot_reload.hpp>
#include <borealis/core/i18n.hpp>
#include <borealis/core/inspector.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/core/style.hpp>
#include <borealis/core/theme.hpp>
#include <borealis/core/theme_image.hpp>
#include <borealis/core/time.hpp>
#include <borealis/core/util.hpp>
#ifdef USE_BOOST_FILESYSTEM
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#elif __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include("experimental/filesystem")
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "Failed to include <filesystem> header!"
#endif
#ifdef USE_LIBROMFS
#include <romfs/romfs.hpp>
#endif
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "theme_presets.hpp"

namespace brls
{

static const std::initializer_list<std::pair<std::string, NVGcolor>> LIGHT_COLORS = {
    // Generic values
    { "brls/clear", nvgRGB(235, 235, 235) },
    { "brls/background", nvgRGB(235, 235, 235) },
    { "brls/text", nvgRGB(45, 45, 45) },
    { "brls/text_disabled", nvgRGB(140, 140, 140) },
    { "brls/backdrop", nvgRGBA(0, 0, 0, 178) },
    { "brls/notification/background", nvgRGBA(0, 0, 0, 178) }, // as the backdrop
    { "brls/notification/text", nvgRGB(255, 255, 255) },
    { "brls/click_pulse", nvgRGBA(13, 182, 213, 38) }, // same as highlight color1 with different opacity
    { "brls/accent", nvgRGB(49, 79, 235) },

    // Highlight
    { "brls/highlight/background", nvgRGB(252, 255, 248) },
    { "brls/highlight/color1", nvgRGB(13, 182, 213) },
    { "brls/highlight/color2", nvgRGB(80, 239, 217) },

    // AppletFrame
    { "brls/applet_frame/separator", nvgRGB(45, 45, 45) },

    // Sidebar
    { "brls/sidebar/background", nvgRGB(240, 240, 240) },
    { "brls/sidebar/active_item", nvgRGB(49, 79, 235) },
    { "brls/sidebar/separator", nvgRGB(208, 208, 208) },

    // Header
    { "brls/header/border", nvgRGB(207, 207, 207) },
    { "brls/header/rectangle", nvgRGB(127, 127, 127) },
    { "brls/header/subtitle", nvgRGB(140, 140, 140) },

    // Button
    { "brls/button/primary_enabled_background", nvgRGB(50, 79, 241) },
    { "brls/button/primary_disabled_background", nvgRGB(201, 201, 209) },
    { "brls/button/primary_enabled_text", nvgRGB(255, 255, 255) },
    { "brls/button/primary_disabled_text", nvgRGB(220, 220, 228) },

    { "brls/button/default_enabled_background", nvgRGB(255, 255, 255) },
    { "brls/button/default_disabled_background", nvgRGB(255, 255, 255) },
    { "brls/button/default_enabled_text", nvgRGB(45, 45, 45) },
    { "brls/button/default_disabled_text", nvgRGB(45, 45, 45) },

    { "brls/button/highlight_enabled_text", nvgRGB(49, 79, 235) },
    { "brls/button/highlight_disabled_text", nvgRGB(49, 79, 235) },

    { "brls/button/enabled_border_color", nvgRGB(45, 45, 45) },
    { "brls/button/disabled_border_color", nvgRGB(45, 45, 45) },

    // List
    { "brls/list/listItem_value_color", nvgRGB(43, 81, 226) },

    // Slider
    { "brls/slider/pointer_color", nvgRGB(255, 255, 255) },
    { "brls/slider/pointer_border_color", nvgRGB(200, 200, 200) },
    { "brls/slider/line_filled", nvgRGB(50, 79, 241) },
    { "brls/slider/line_empty", nvgRGB(140, 140, 140) },

    // Spinner
    { "brls/spinner/bar_color", nvgRGBA(131, 131, 131, 80) },

    // Shadows: a view's (shadowType), the focus highlight's; their opacity
    // is the style's (brls/shadow/opacity, brls/highlight/shadow_opacity)
    { "brls/shadow", nvgRGB(0, 0, 0) },
    { "brls/highlight/shadow", nvgRGB(0, 0, 0) },

    // Battery widget
    { "brls/battery/level", nvgRGB(0, 0, 0) },
    { "brls/battery/charging", nvgRGB(140, 251, 79) },
};

static const std::initializer_list<std::pair<std::string, NVGcolor>> DARK_COLORS = {
    // Generic values
    { "brls/clear", nvgRGB(45, 45, 45) },
    { "brls/background", nvgRGB(45, 45, 45) },
    { "brls/text", nvgRGB(255, 255, 255) },
    { "brls/text_disabled", nvgRGB(80, 80, 80) },
    { "brls/backdrop", nvgRGBA(0, 0, 0, 178) },
    { "brls/notification/background", nvgRGBA(0, 0, 0, 178) }, // as the backdrop
    { "brls/notification/text", nvgRGB(255, 255, 255) },
    { "brls/click_pulse", nvgRGBA(25, 138, 198, 38) }, // same as highlight color1 with different opacity
    { "brls/accent", nvgRGB(0, 255, 204) },

    // Highlight
    { "brls/highlight/background", nvgRGB(31, 34, 39) },
    { "brls/highlight/color1", nvgRGB(25, 138, 198) },
    { "brls/highlight/color2", nvgRGB(137, 241, 242) },

    // AppletFrame
    { "brls/applet_frame/separator", nvgRGB(255, 255, 255) },

    // Sidebar
    { "brls/sidebar/background", nvgRGB(50, 50, 50) },
    { "brls/sidebar/active_item", nvgRGB(0, 255, 204) },
    { "brls/sidebar/separator", nvgRGB(81, 81, 81) },

    // Header
    { "brls/header/border", nvgRGB(78, 78, 78) },
    { "brls/header/rectangle", nvgRGB(160, 160, 160) },
    { "brls/header/subtitle", nvgRGB(163, 163, 163) },

    // Button
    { "brls/button/primary_enabled_background", nvgRGB(1, 255, 201) },
    { "brls/button/primary_disabled_background", nvgRGB(83, 87, 86) },
    { "brls/button/primary_enabled_text", nvgRGB(52, 41, 55) },
    { "brls/button/primary_disabled_text", nvgRGB(71, 75, 74) },

    { "brls/button/default_enabled_background", nvgRGB(80, 80, 80) },
    { "brls/button/default_disabled_background", nvgRGB(80, 80, 80) },
    { "brls/button/default_enabled_text", nvgRGB(255, 255, 255) },
    { "brls/button/default_disabled_text", nvgRGB(255, 255, 255) },

    { "brls/button/highlight_enabled_text", nvgRGB(7, 247, 198) },
    { "brls/button/highlight_disabled_text", nvgRGB(7, 247, 198) },

    { "brls/button/enabled_border_color", nvgRGB(255, 255, 255) },
    { "brls/button/disabled_border_color", nvgRGB(255, 255, 255) },

    // List
    { "brls/list/listItem_value_color", nvgRGB(88, 195, 169) },

    // Slider
    { "brls/slider/pointer_color", nvgRGB(80, 80, 80) },
    { "brls/slider/pointer_border_color", nvgRGB(120, 120, 120) },
    { "brls/slider/line_filled", nvgRGB(1, 255, 201) },
    { "brls/slider/line_empty", nvgRGB(140, 140, 140) },

    // Spinner
    { "brls/spinner/bar_color", nvgRGBA(192, 192, 192, 80) },

    // Shadows
    { "brls/shadow", nvgRGB(0, 0, 0) },
    { "brls/highlight/shadow", nvgRGB(0, 0, 0) },

    // Battery widget
    { "brls/battery/level", nvgRGB(255, 255, 255) },
    { "brls/battery/charging", nvgRGB(140, 251, 79) },
};

static uint64_t generation = 1;

// The theme file's colours again, after a base colour changed (some are
// derived from the base: the seeds a file does not give)
static void reapplyTheme();

// ---------------------------------------------------------------------------
// Colour arithmetic
// ---------------------------------------------------------------------------

static NVGcolor mix(NVGcolor a, NVGcolor b, float t)
{
    return nvgRGBAf(a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t);
}

static NVGcolor withAlpha(NVGcolor color, int alpha)
{
    color.a = alpha / 255.0f;
    return color;
}

// `top` drawn over the opaque `under`
static NVGcolor over(NVGcolor top, NVGcolor under)
{
    NVGcolor color = mix(under, top, top.a);
    color.a        = 1.0f;
    return color;
}

static bool sameColor(NVGcolor a, NVGcolor b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

// WCAG's relative luminance
static float luminance(NVGcolor c)
{
    auto channel = [](float v)
    { return v <= 0.03928f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f); };
    return 0.2126f * channel(c.r) + 0.7152f * channel(c.g) + 0.0722f * channel(c.b);
}

float Theme::contrast(NVGcolor a, NVGcolor b)
{
    float la = luminance(a), lb = luminance(b);
    return (std::max(la, lb) + 0.05f) / (std::min(la, lb) + 0.05f);
}

static bool isDark(NVGcolor background)
{
    return luminance(background) < 0.2f;
}

// `color` moved toward black or white (away from `background`) until it
// reads at `minimum`:1 on each of `surfaces` (translucent ones over
// `background`)
static NVGcolor ink(NVGcolor color, std::initializer_list<NVGcolor> surfaces, NVGcolor background, float minimum)
{
    NVGcolor target = isDark(background) ? nvgRGB(255, 255, 255) : nvgRGB(0, 0, 0);
    for (int step = 0; step <= 20; step++)
    {
        NVGcolor candidate = mix(color, target, step / 20.0f);
        candidate.a        = 1.0f;
        bool enough        = true;
        for (NVGcolor surface : surfaces)
            enough = enough && Theme::contrast(candidate, over(surface, background)) >= minimum;
        if (enough)
            return candidate;
    }
    return target;
}

// The text to write on `fill`: the darker or the lighter of `text` and
// `background`, whichever reads better, else black or white
static NVGcolor onColor(NVGcolor fill, NVGcolor text, NVGcolor background)
{
    NVGcolor best = Theme::contrast(text, fill) >= Theme::contrast(background, fill) ? text : background;
    if (Theme::contrast(best, fill) >= 4.5f)
        return best;
    NVGcolor black = nvgRGB(0, 0, 0), white = nvgRGB(255, 255, 255);
    return Theme::contrast(black, fill) >= Theme::contrast(white, fill) ? black : white;
}

using ColorMap  = std::unordered_map<std::string, NVGcolor>;
using MetricMap = std::unordered_map<std::string, float>;

static NVGcolor lookup(const ColorMap& colors, const char* key, NVGcolor fallback)
{
    auto it = colors.find(key);
    return it == colors.end() ? fallback : it->second;
}

// The accent's shades for a state (hover, pressed, disabled) and the text
// on it
static void addAccentShades(ColorMap& out, NVGcolor accent, NVGcolor background, NVGcolor text)
{
    bool dark                   = isDark(background);
    out["brls/accent/hover"]    = dark ? mix(accent, nvgRGB(255, 255, 255), 0.15f) : mix(accent, nvgRGB(0, 0, 0), 0.10f);
    out["brls/accent/pressed"]  = mix(accent, nvgRGB(0, 0, 0), 0.20f);
    out["brls/accent/disabled"] = mix(accent, background, 0.6f);
    out["brls/accent/ink"]      = onColor(accent, text, background);
}

static ThemeValues makeValues(std::initializer_list<std::pair<std::string, NVGcolor>> list)
{
    ThemeValues values(list);
    ColorMap shades;
    const ColorMap& base = values.getBaseColors();
    addAccentShades(shades, base.at("brls/accent"), base.at("brls/background"), base.at("brls/text"));
    for (auto& [name, color] : shades)
        values.addColor(name, color);
    return values;
}

static const char* SEEDS[] = { "accent", "background", "surface", "text", "warning", "danger" };

static bool isSeed(const std::string& key)
{
    for (const char* seed : SEEDS)
        if (key == seed)
            return true;
    return false;
}

// Every key a palette (seeds) changes, from `base` (the variant without a
// theme file) for the seeds it does not give. Keys the base does not have
// (Cadran's when it is off) are left out.
static ColorMap derivePalette(const ColorMap& seeds, const ColorMap& base)
{
    ColorMap out;
    auto has = [&seeds](const char* seed)
    { return seeds.count(seed) > 0; };
    NVGcolor B  = has("background") ? seeds.at("background") : lookup(base, "brls/background", nvgRGB(45, 45, 45));
    NVGcolor T  = has("text") ? seeds.at("text") : lookup(base, "brls/text", nvgRGB(255, 255, 255));
    NVGcolor S  = has("surface") ? seeds.at("surface")
                                 : lookup(base, "cadran/surf", lookup(base, "brls/highlight/background", B));
    NVGcolor S2 = has("surface") || has("text") ? mix(S, T, 0.05f) : lookup(base, "cadran/surf2", S);
    bool dark   = isDark(B);
    auto alpha  = [dark](NVGcolor c, int onDark, int onLight)
    { return withAlpha(c, dark ? onDark : onLight); };
    auto inkOn = [&](NVGcolor c, float minimum)
    { return ink(c, { B, S, S2 }, B, minimum); };

    if (has("background"))
    {
        out["brls/background"]         = B;
        out["brls/clear"]              = B;
        out["brls/sidebar/background"] = B;
    }

    if (has("surface"))
    {
        out["brls/highlight/background"]               = S;
        out["cadran/surf"]                             = S;
        out["cadran/surf2"]                            = S2;
        out["brls/button/default_enabled_background"]  = S2;
        out["brls/button/default_disabled_background"] = S2;
    }

    if (has("text") || has("background"))
    {
        NVGcolor text2                           = inkOn(mix(T, B, 0.30f), 4.5f);
        NVGcolor text3                           = inkOn(mix(T, B, 0.45f), 4.5f);
        NVGcolor line                            = alpha(T, 36, 26);
        out["brls/text"]                         = T;
        out["cadran/text"]                       = T;
        out["cadran/text2"]                      = text2;
        out["cadran/text3"]                      = text3;
        out["brls/text_disabled"]                = mix(T, B, 0.55f);
        out["brls/header/subtitle"]              = text3;
        out["brls/header/rectangle"]             = text3;
        out["brls/list/listItem_value_color"]    = text2;
        out["brls/sidebar/separator"]            = line;
        out["brls/applet_frame/separator"]       = line;
        out["brls/header/border"]                = line;
        out["cadran/line"]                       = line;
        out["cadran/track"]                      = alpha(T, 23, 26);
        out["cadran/border"]                     = withAlpha(T, 60);
        out["cadran/well"]                       = alpha(T, 46, 34);
        out["brls/button/default_enabled_text"]  = T;
        out["brls/button/default_disabled_text"] = T;
        out["brls/button/enabled_border_color"]  = withAlpha(T, 60);
        out["brls/button/disabled_border_color"] = withAlpha(T, 60);
        out["brls/slider/line_empty"]            = text3;
        out["brls/spinner/bar_color"]            = withAlpha(T, 80);
        out["cadran/key_bg"]                     = T;
        out["cadran/key_fg"]                     = B;
    }

    if (has("accent") || has("background") || has("surface"))
    {
        NVGcolor A                                     = has("accent") ? seeds.at("accent") : lookup(base, "brls/accent", T);
        NVGcolor fill                                  = has("accent") ? A : lookup(base, "cadran/teal_fill", A);
        NVGcolor railActive                            = withAlpha(fill, 36);
        NVGcolor accentInk                             = ink(fill, { B, S, S2, railActive }, B, 4.5f);
        NVGcolor ring                                  = ink(fill, { B }, B, 3.0f);
        NVGcolor glow                                  = ink(dark ? mix(fill, nvgRGB(255, 255, 255), 0.5f) : mix(fill, B, 0.25f), { B }, B, 3.0f);
        NVGcolor onFill                                = onColor(fill, T, B);
        out["brls/accent"]                             = accentInk;
        out["brls/sidebar/active_item"]                = accentInk;
        out["brls/button/highlight_enabled_text"]      = accentInk;
        out["brls/button/highlight_disabled_text"]     = accentInk;
        out["brls/highlight/color1"]                   = ring;
        out["brls/highlight/color2"]                   = glow;
        out["brls/click_pulse"]                        = withAlpha(fill, 38);
        out["brls/button/primary_enabled_background"]  = fill;
        out["brls/button/primary_enabled_text"]        = onFill;
        out["brls/button/primary_disabled_background"] = mix(fill, B, 0.6f);
        out["brls/button/primary_disabled_text"]       = mix(onFill, mix(fill, B, 0.6f), 0.5f);
        out["brls/slider/line_filled"]                 = fill;
        out["cadran/teal"]                             = accentInk;
        out["cadran/teal_fill"]                        = fill;
        out["cadran/teal_soft"]                        = withAlpha(fill, 26);
        out["cadran/glow"]                             = alpha(fill, 31, 41);
        out["cadran/glow_strong"]                      = alpha(fill, 51, 56);
        out["cadran/pill_ok"]                          = dark ? withAlpha(fill, 41) : over(withAlpha(fill, 46), S);
        out["cadran/pill_ok_ink"]                      = ink(accentInk, { B, S, out["cadran/pill_ok"] }, B, 4.5f);
        out["cadran/rail_active"]                      = railActive;
        out["cadran/hl1"]                              = ring;
        out["cadran/hl2"]                              = glow;
        addAccentShades(out, fill, B, T);
    }

    if (has("warning"))
    {
        NVGcolor W               = seeds.at("warning");
        NVGcolor surface         = dark ? withAlpha(W, 26) : over(withAlpha(W, 36), S);
        out["cadran/amber_fill"] = W;
        out["cadran/amber_surf"] = surface;
        out["cadran/amber_line"] = dark ? withAlpha(W, 90) : over(withAlpha(W, 115), S);
        out["cadran/amber"]      = ink(W, { B, S, S2, surface }, B, 4.5f);
    }

    if (has("danger"))
    {
        NVGcolor D               = seeds.at("danger");
        out["cadran/coral_fill"] = D;
        out["cadran/coral_line"] = dark ? withAlpha(D, 140) : over(withAlpha(D, 128), S);
        out["cadran/coral_soft"] = alpha(D, 36, 28);
        out["cadran/coral"]      = inkOn(D, 4.5f);
    }

    // The label on the fills (a primary button's, a confirmation's, a
    // removal's): the ink that reads best on all of them
    if (has("accent") || has("danger") || has("text") || has("background"))
    {
        NVGcolor fills[] = { has("accent") ? seeds.at("accent") : lookup(base, "cadran/teal_fill", lookup(base, "brls/button/primary_enabled_background", B)),
            has("danger") ? seeds.at("danger") : lookup(base, "cadran/coral_fill", B) };
        // The fills the file gives first: the others can be adjusted
        bool given[]    = { has("accent"), has("danger") };
        bool anyGiven   = given[0] || given[1];
        NVGcolor best   = T;
        float bestRatio = 0.0f;
        for (NVGcolor candidate : { T, B, nvgRGB(0, 0, 0), nvgRGB(255, 255, 255) })
        {
            float ratio = 21.0f;
            for (int i = 0; i < 2; i++)
                if (given[i] || !anyGiven)
                    ratio = std::min(ratio, Theme::contrast(candidate, fills[i]));
            if (ratio > bestRatio + 0.01f)
            {
                best      = candidate;
                bestRatio = ratio;
            }
        }
        bestRatio          = std::min(Theme::contrast(best, fills[0]), Theme::contrast(best, fills[1]));
        out["cadran/navy"] = best;
        // A fill the file does not give, darkened or lightened until the
        // label reads on it
        if (bestRatio < 4.5f)
        {
            const char* seeded[]  = { "accent", "danger" };
            const char* keys[][2] = { { "cadran/teal_fill", "brls/button/primary_enabled_background" }, { "cadran/coral_fill", nullptr } };
            NVGcolor away         = luminance(best) > 0.5f ? nvgRGB(0, 0, 0) : nvgRGB(255, 255, 255);
            for (int i = 0; i < 2; i++)
            {
                if (has(seeded[i]) || Theme::contrast(best, fills[i]) >= 4.5f)
                    continue;
                NVGcolor fill = fills[i];
                for (int step = 1; step <= 20 && Theme::contrast(best, fill) < 4.5f; step++)
                    fill = mix(fills[i], away, step / 20.0f);
                for (const char* key : keys[i])
                    if (key)
                        out[key] = fill;
            }
        }
    }

    for (auto it = out.begin(); it != out.end();)
        it = base.count(it->first) ? std::next(it) : out.erase(it);
    return out;
}

// ---------------------------------------------------------------------------
// ThemeValues and Theme
// ---------------------------------------------------------------------------

ThemeValues::ThemeValues(std::initializer_list<std::pair<std::string, NVGcolor>> list)
{
    for (const std::pair<std::string, NVGcolor>& color : list)
        this->base.insert(color);
    this->values = this->base;
}

void ThemeValues::addColor(const std::string& name, NVGcolor color)
{
    auto it = this->base.find(name);
    if (it != this->base.end() && sameColor(it->second, color))
        return;
    this->base[name] = color;
    if (this->overlay.count(name) == 0)
    {
        this->values[name] = color;
        generation++;
    }
    reapplyTheme();
}

NVGcolor ThemeValues::getColor(const std::string& name)
{
    auto it = this->values.find(name);
    if (it == this->values.end())
        fatal("Unknown theme value \"" + name + "\" in size: " + std::to_string(this->values.size()));
    if (Inspector::recording)
        Inspector::recordColor(name, it->second);
    return it->second;
}

bool ThemeValues::hasColor(const std::string& name) const
{
    return this->values.count(name) > 0;
}

void ThemeValues::setOverlay(const std::unordered_map<std::string, NVGcolor>& overlay)
{
    this->overlay = overlay;
    this->values  = this->base;
    for (auto& [name, color] : overlay)
        this->values[name] = color;
    generation++;
}

Theme::Theme(ThemeValues* values)
    : values(values)
{
}

NVGcolor Theme::getColor(const std::string& name)
{
    return this->values->getColor(name);
}

bool Theme::hasColor(const std::string& name)
{
    return this->values->hasColor(name);
}

void Theme::addColor(const std::string& name, NVGcolor color)
{
    this->values->addColor(name, color);
}

NVGcolor Theme::operator[](const std::string& name)
{
    return this->getColor(name);
}

Theme& Theme::getLightTheme()
{
    static ThemeValues values = makeValues(LIGHT_COLORS);
    static Theme lightTheme(&values);
    return lightTheme;
}

Theme& Theme::getDarkTheme()
{
    static ThemeValues values = makeValues(DARK_COLORS);
    static Theme darkTheme(&values);
    return darkTheme;
}

Theme& Theme::get(ThemeVariant variant)
{
    return variant == ThemeVariant::LIGHT ? getLightTheme() : getDarkTheme();
}

uint64_t Theme::getGeneration()
{
    return generation;
}

static std::string trim(const std::string& text)
{
    size_t start = text.find_first_not_of(" \t\r");
    if (start == std::string::npos)
        return "";
    size_t end = text.find_last_not_of(" \t\r");
    return text.substr(start, end - start + 1);
}

bool Theme::parseColor(const std::string& raw, NVGcolor* color)
{
    std::string text = trim(raw);
    if (!text.empty() && text[0] == '#')
    {
        std::string hex = text.substr(1);
        for (char c : hex)
            if (!std::isxdigit((unsigned char)c))
                return false;
        if (hex.size() == 3)
            hex = { hex[0], hex[0], hex[1], hex[1], hex[2], hex[2] };
        if (hex.size() != 6 && hex.size() != 8)
            return false;
        unsigned long value = std::strtoul(hex.c_str(), nullptr, 16);
        if (hex.size() == 6)
            *color = nvgRGB((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
        else
            *color = nvgRGBA((value >> 24) & 0xFF, (value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
        return true;
    }

    // rgb(r, g, b) or rgba(r, g, b, a), a from 0 to 1
    bool alpha = text.rfind("rgba(", 0) == 0;
    if (!alpha && text.rfind("rgb(", 0) != 0)
        return false;
    if (text.back() != ')')
        return false;
    std::string inside = text.substr(alpha ? 5 : 4, text.size() - (alpha ? 6 : 5));
    std::stringstream parts(inside);
    std::string part;
    float values[4] = { 0, 0, 0, 1 };
    int count       = 0;
    while (std::getline(parts, part, ','))
    {
        if (count == 4)
            return false;
        char* end;
        std::string number = trim(part);
        values[count]      = std::strtof(number.c_str(), &end);
        if (number.empty() || *end != '\0')
            return false;
        count++;
    }
    if (count != (alpha ? 4 : 3))
        return false;
    *color = nvgRGBAf(values[0] / 255.0f, values[1] / 255.0f, values[2] / 255.0f, values[3]);
    return true;
}

// ---------------------------------------------------------------------------
// Contrast
// ---------------------------------------------------------------------------

struct ContrastCheck
{
    std::string ink;
    std::string surface;
    float minimum;
};

// The pairs most of the core's text is drawn with
static std::vector<ContrastCheck> contrastChecks = {
    { "brls/text", "brls/background", 4.5f },
    { "brls/text", "brls/sidebar/background", 4.5f },
    { "brls/button/primary_enabled_text", "brls/button/primary_enabled_background", 4.5f },
};

void Theme::addContrastCheck(const std::string& ink, const std::string& surface, float minimum)
{
    for (auto& check : contrastChecks)
        if (check.ink == ink && check.surface == surface)
        {
            check.minimum = minimum;
            return;
        }
    contrastChecks.push_back({ ink, surface, minimum });
}

// The surfaces a background image shows through (surface_opacity)
static std::vector<std::string> surfaceKeys = {
    "brls/sidebar/background",
    "brls/highlight/background",
    "brls/button/default_enabled_background",
    "brls/button/default_disabled_background",
};

void Theme::addSurfaceKey(const std::string& key)
{
    if (std::find(surfaceKeys.begin(), surfaceKeys.end(), key) == surfaceKeys.end())
        surfaceKeys.push_back(key);
}

// `backdrop`, when given: what the screens' background is drawn as (a
// background image's average colour under its veil). `backdropOnly`: the
// pairs it changes only.
static std::vector<std::string> checkColors(const ColorMap& colors, const NVGcolor* backdrop = nullptr,
    bool backdropOnly = false)
{
    std::vector<std::string> warnings;
    auto background = colors.find("brls/background");
    if (background == colors.end())
        return warnings;
    NVGcolor under = backdrop ? *backdrop : over(background->second, nvgRGB(0, 0, 0));
    for (auto& check : contrastChecks)
    {
        auto ink     = colors.find(check.ink);
        auto surface = colors.find(check.surface);
        if (ink == colors.end() || surface == colors.end())
            continue;
        // Text on the screens' background sits on the image
        bool onScreen = backdrop && (check.surface == "brls/background" || check.surface == "brls/clear");
        if (backdropOnly && !onScreen && !(backdrop && surface->second.a < 1.0f))
            continue;
        NVGcolor drawnSurface = onScreen ? under : over(surface->second, under);
        float ratio           = Theme::contrast(over(ink->second, drawnSurface), drawnSurface);
        if (ratio < check.minimum)
        {
            char text[256];
            std::snprintf(text, sizeof(text), "%s on %s%s reads badly: %.1f:1, under %.1f:1", check.ink.c_str(),
                check.surface.c_str(), backdrop ? " over the background image" : "", ratio, check.minimum);
            warnings.push_back(text);
        }
    }
    return warnings;
}

// The screens' background as drawn over `background`'s image: its average
// colour under the veil of brls/background
static NVGcolor backdropOf(const ColorMap& colors, const ThemeBackground& background, NVGcolor mean)
{
    NVGcolor veil = lookup(colors, "brls/background", nvgRGB(0, 0, 0));
    veil.a *= background.dim;
    return over(veil, mean);
}

// ---------------------------------------------------------------------------
// Theme files
// ---------------------------------------------------------------------------

static const char* BUILTIN_PREFIX = "builtin:";
// A theme folder's file: <themes>/<id>/theme.theme, next to its pictures
static const char* THEME_FOLDER_FILE = "theme.theme";
static const char* SEASONAL_PATH     = "builtin:seasonal";
static const int MAX_DEPTH           = 8;

static const BuiltinTheme* findBuiltin(std::string id)
{
    if (startsWith(id, BUILTIN_PREFIX))
        id = id.substr(std::string(BUILTIN_PREFIX).size());
    if (endsWith(id, ".theme"))
        id = id.substr(0, id.size() - 6);
    for (const BuiltinTheme& theme : builtinThemeTexts())
        if (id == theme.id)
            return &theme;
    return nullptr;
}

// The text of a file, a resource ("@res/…", from hot reload's folder when
// it has the file) or a preset ("builtin:…")
static bool readThemeRaw(const std::string& name, std::string* text)
{
    std::string disk        = HotReload::diskResource(name);
    const std::string& path = disk.empty() ? name : disk;
    if (startsWith(path, BUILTIN_PREFIX))
    {
        const BuiltinTheme* theme = findBuiltin(path);
        if (!theme)
            return false;
        *text = builtinThemeText(*theme, getWallTime());
        return true;
    }
#ifdef USE_LIBROMFS
    if (startsWith(path, "@res/"))
    {
        try
        {
            *text = std::string(romfs::get(path.substr(5)).string());
            return true;
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }
#endif
    std::string file = path;
    if (startsWith(file, "@res/"))
        file = std::string(BRLS_RESOURCES) + file.substr(5);
    std::ifstream stream(file);
    if (!stream)
        return false;
    std::stringstream buffer;
    buffer << stream.rdbuf();
    *text = buffer.str();
    return true;
}

// The text as read: a resource's ends with a NUL
static bool readThemeText(const std::string& path, std::string* text)
{
    if (!readThemeRaw(path, text))
        return false;
    size_t end = text->find('\0');
    if (end != std::string::npos)
        text->resize(end);
    return true;
}

static std::string directoryOf(const std::string& path)
{
    if (startsWith(path, BUILTIN_PREFIX))
        return BUILTIN_PREFIX;
    size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? "" : path.substr(0, slash);
}

// `name` (an @include's or an inherit's) as a path from `directory`: a
// preset when no such file exists
static std::string resolvePath(const std::string& name, const std::string& directory)
{
    if (startsWith(name, BUILTIN_PREFIX) || startsWith(name, "@res/") || startsWith(name, "/")
        || name.find(":/") != std::string::npos)
        return name;
    if (directory == BUILTIN_PREFIX)
        return BUILTIN_PREFIX + name;
    std::string path = directory.empty() ? name : directory + "/" + name;
    std::string text;
    if (!readThemeText(path, &text) && findBuiltin(name))
        return BUILTIN_PREFIX + name;
    return path;
}

// `text` cut at each '/'
static std::vector<std::string> segmentsOf(const std::string& text)
{
    std::vector<std::string> segments;
    std::stringstream stream(text);
    std::string segment;
    while (std::getline(stream, segment, '/'))
        segments.push_back(segment);
    return segments;
}

bool Theme::resolveAsset(const std::string& raw, const std::string& directory, const std::string& root,
    std::string* path, std::string* error)
{
    std::string name = trim(raw);
    if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
        name = name.substr(1, name.size() - 2);
    auto refuse = [&](const std::string& why)
    {
        *error = name + ": " + why;
        return false;
    };
    if (name.empty())
        return refuse("no file named");

    // A preset's pictures, or a resource theme's, may be the app's resources
    bool trusted = root == BUILTIN_PREFIX || startsWith(root, "@res/") || root == "@res";
    if (startsWith(name, "@res/"))
    {
        if (!trusted)
            return refuse("a theme file's pictures are in its folder, not in the app's resources");
        if (name.find("..") != std::string::npos)
            return refuse("\"..\" is not allowed");
        *path = name;
        return true;
    }
    if (root == BUILTIN_PREFIX)
        return refuse("a preset's pictures are resources (@res/...)");
    if (name.front() == '/' || name.find(':') != std::string::npos || name.find('\\') != std::string::npos)
        return refuse("an absolute path is not allowed: a theme's pictures are in its folder");

    // From the root, through the including file's folder, to the file
    std::string inside = directory;
    if (inside == root)
        inside.clear();
    else if (startsWith(inside, root + "/"))
        inside = inside.substr(root.size() + 1);
    else if (!root.empty())
        return refuse("read from outside the theme's folder");
    std::vector<std::string> kept;
    for (const std::string& segment : segmentsOf(inside + (inside.empty() ? "" : "/") + name))
    {
        if (segment.empty() || segment == ".")
            continue;
        if (segment == "..")
        {
            if (kept.empty())
                return refuse("goes out of the theme's folder");
            kept.pop_back();
            continue;
        }
        kept.push_back(segment);
    }
    if (kept.empty())
        return refuse("not a file");
    std::string relative;
    for (const std::string& segment : kept)
        relative += (relative.empty() ? "" : "/") + segment;
    std::string resolved = root.empty() ? relative : root + "/" + relative;

    // A link inside the folder to a file out of it is refused too (where
    // the file system has links and can say where they go: the path is
    // already in the folder by its name)
    if (!trusted)
    {
        bool out = false;
        try
        {
            if (fs::exists(resolved))
            {
                fs::path within = fs::canonical(resolved).lexically_relative(fs::canonical(root.empty() ? "." : root));
                out             = within.empty() || *within.begin() == "..";
            }
        }
        catch (const std::exception& e)
        {
        }
        if (out)
            return refuse("a link out of the theme's folder");
    }
    *path = resolved;
    return true;
}

namespace
{
    // A background as a file writes it, for every variant it styles or one:
    // what it does not say is unset (and inherited)
    struct BackgroundSpec
    {
        std::optional<std::string> image; // its path; "" for none
        std::optional<BackgroundFit> fit;
        std::optional<float> dim;
        std::optional<float> surfaceOpacity;
    };

    // The logo, icons, preview and image slots' pictures as a file writes them,
    // for every variant it styles or one: by key ("logo", "preview",
    // "icon.<name>", "image.<slot>"), "" for none; what it does not say is
    // unset (and inherited)
    struct PictureSpec
    {
        std::map<std::string, std::string> files;
        std::map<std::string, bool> tints; // icon.<name>.tint, by icon name
    };

    // The decorations as a file writes them, as BackgroundSpec: what it does
    // not say is unset (and inherited)
    struct DecorPictureSpec
    {
        std::optional<std::string> image; // its path; "" for none
        std::optional<BackgroundFit> fit;
        std::optional<float> opacity;
        std::optional<float> height;
    };

    struct DecorSpec
    {
        DecorPictureSpec pictures[THEME_AREA_COUNT];
        std::optional<ParticleKind> kind;
        std::optional<float> density;
        std::optional<float> speed;
        std::optional<ParticleLayer> layer;
        std::optional<std::vector<NVGcolor>> colors;
    };

    // A file read, with what it includes and inherits
    struct ParsedTheme
    {
        std::string name, author, version, description;
        std::string inherit;
        std::string counterpart; // its path, from the file's folder
        std::string icon;
        // [0]: every variant the file styles, [1] the light one, [2] the dark
        BackgroundSpec background[3];
        PictureSpec pictures[3]; // as background
        DecorSpec decor[3]; // as background
        std::vector<std::string> files; // the files it includes and inherits, read
        bool lightSection = false, darkSection = false;
        bool light = false, dark = false;

        // [0]: every variant the file styles, [1] the light one, [2] the dark
        std::vector<std::pair<std::string, NVGcolor>> seeds[3];
        std::vector<std::pair<std::string, NVGcolor>> colors[3];
        std::vector<std::pair<std::string, float>> metrics;

        std::vector<std::string> errors;
    };
}

static bool parseFit(const std::string& value, BackgroundFit* fit)
{
    static const std::pair<const char*, BackgroundFit> FITS[] = { { "cover", BackgroundFit::COVER },
        { "contain", BackgroundFit::CONTAIN }, { "tile", BackgroundFit::TILE }, { "center", BackgroundFit::CENTER } };
    for (auto& [name, each] : FITS)
        if (value == name)
        {
            *fit = each;
            return true;
        }
    return false;
}

// Logos and images (docs/themes.md, "Logos and images")

static const char* ICON_PREFIX  = "icon.";
static const char* IMAGE_PREFIX = "image.";
static const char* TINT_SUFFIX  = ".tint";

static bool isPictureKey(const std::string& key)
{
    return key == "logo" || key == "preview" || (startsWith(key, ICON_PREFIX) && key.size() > 5)
        || (startsWith(key, IMAGE_PREFIX) && key.size() > 6);
}

// One line of them: false, with the reason in `error`, when it is skipped
static bool parsePictureKey(const std::string& key, const std::string& value, const std::string& directory,
    const std::string& assets, PictureSpec* out, std::string* error)
{
    if (startsWith(key, ICON_PREFIX) && endsWith(key, TINT_SUFFIX))
    {
        std::string name = key.substr(5, key.size() - 5 - 5);
        if (name.empty())
        {
            *error = "unknown key " + key;
            return false;
        }
        if (value == "true" || value == "yes" || value == "1")
            out->tints[name] = true;
        else if (value == "false" || value == "no" || value == "0")
            out->tints[name] = false;
        else
        {
            *error = key + " is true or false, not " + value;
            return false;
        }
        return true;
    }
    if (value == "none")
    {
        out->files[key] = "";
        return true;
    }
    std::string path, why;
    if (!Theme::resolveAsset(value, directory, assets, &path, &why))
    {
        *error = key + " " + why;
        return false;
    }
    out->files[key] = path;
    return true;
}

// As inheritBackground, key by key
static void inheritPictures(PictureSpec* own, const PictureSpec* parent)
{
    auto inherit = [&](auto member)
    {
        auto everyVariant = own[0].*member; // this file's, before the inherited ones
        for (auto& [key, value] : parent[0].*member)
            if (!everyVariant.count(key))
                (own[0].*member).emplace(key, value);
        for (int slot : { 1, 2 })
            for (auto& [key, value] : parent[slot].*member)
                if (!everyVariant.count(key))
                    (own[slot].*member).emplace(key, value);
    };
    inherit(&PictureSpec::files);
    inherit(&PictureSpec::tints);
}

static std::vector<std::string> iconNames; // Theme::addIconNames, sorted

namespace
{
    struct ImageSlot
    {
        std::string fallback;
        int maxSide = 0;
    };
}
static std::map<std::string, ImageSlot> imageSlots; // Theme::addImageSlot

// Variant `v`'s (0 light, 1 dark): its own lines, else those for every
// variant; what names an icon or a slot nobody declared is warned about
static ThemePictures resolvePictures(const PictureSpec* spec, int v, std::vector<std::string>* warnings)
{
    std::map<std::string, std::string> files = spec[v + 1].files;
    files.insert(spec[0].files.begin(), spec[0].files.end());
    std::map<std::string, bool> tints = spec[v + 1].tints;
    tints.insert(spec[0].tints.begin(), spec[0].tints.end());

    ThemePictures pictures;
    for (auto& [key, path] : files)
    {
        if (key == "logo")
            pictures.logo = path;
        else if (key == "preview")
            pictures.preview = path;
        else if (startsWith(key, ICON_PREFIX))
        {
            std::string name = key.substr(5);
            if (!std::binary_search(iconNames.begin(), iconNames.end(), name))
                warnings->push_back(key + ": no icon has that name (kept)");
            if (!path.empty())
                pictures.icons[name] = ThemeIcon { path, tints.count(name) && tints[name] };
        }
        else
        {
            std::string slot = key.substr(6);
            if (!imageSlots.count(slot))
            {
                std::string declared;
                for (auto& [each, value] : imageSlots)
                    declared += (declared.empty() ? "" : ", ") + each;
                warnings->push_back(key + ": the app has no such image slot (kept); "
                    + (declared.empty() ? std::string("it declares none") : "its slots: " + declared));
            }
            if (!path.empty())
                pictures.images[slot] = path;
        }
    }
    return pictures;
}

// ---------------------------------------------------------------------------
// Decorations: backdrop.<area>, decor.<edge> and particles* (docs/themes.md)
// ---------------------------------------------------------------------------

namespace
{
    // The picture keys' prefixes, by ThemeArea from HEADER
    const char* const DECOR_PICTURES[THEME_AREA_COUNT] = { "backdrop.header", "backdrop.sidebar", "backdrop.dialog",
        "decor.top", "decor.bottom" };

    const std::pair<const char*, ParticleKind> PARTICLE_KINDS[] = { { "none", ParticleKind::NONE },
        { "snow", ParticleKind::SNOW }, { "leaves", ParticleKind::LEAVES }, { "petals", ParticleKind::PETALS },
        { "embers", ParticleKind::EMBERS }, { "confetti", ParticleKind::CONFETTI }, { "sparkles", ParticleKind::SPARKLES },
        { "hearts", ParticleKind::HEARTS }, { "bubbles", ParticleKind::BUBBLES } };
}

static bool isDecorKey(const std::string& key)
{
    return startsWith(key, "backdrop.") || startsWith(key, "decor.") || key == "particles" || startsWith(key, "particles_");
}

static bool parseNumber(const std::string& value, float minimum, float maximum, float* out)
{
    char* end;
    float number = std::strtof(value.c_str(), &end);
    if (value.empty() || *end != '\0' || !(number >= minimum && number <= maximum))
        return false;
    *out = number;
    return true;
}

// "#fff, rgba(1, 2, 3, 0.5)": the colours of a list, split at the commas
// out of parentheses
static bool parseColorList(const std::string& value, std::vector<NVGcolor>* colors)
{
    colors->clear();
    int depth    = 0;
    size_t start = 0;
    for (size_t i = 0; i <= value.size(); i++)
    {
        char c = i < value.size() ? value[i] : ',';
        depth += c == '(' ? 1 : c == ')' ? -1
                                         : 0;
        if (c != ',' || depth > 0)
            continue;
        NVGcolor color;
        if (!Theme::parseColor(trim(value.substr(start, i - start)), &color))
            return false;
        colors->push_back(color);
        start = i + 1;
    }
    return !colors->empty() && colors->size() <= (size_t)ThemeParticles::MAX_COLORS;
}

// A decoration line: false, with the reason in `error`, when it cannot be
// read; its pictures are confined as the background's (Theme::resolveAsset)
static bool parseDecorLine(const std::string& key, const std::string& value, const std::string& directory,
    const std::string& assets, DecorSpec* decor, std::string* error)
{
    auto fail = [error](const std::string& why)
    {
        *error = why;
        return false;
    };
    for (int i = 0; i < THEME_AREA_COUNT; i++)
    {
        const std::string prefix = DECOR_PICTURES[i];
        if (!startsWith(key, prefix) || (key.size() > prefix.size() && key[prefix.size()] != '_'))
            continue;
        DecorPictureSpec& picture  = decor->pictures[i];
        const std::string property = key.substr(prefix.size());
        bool strip                 = i >= (int)ThemeArea::TOP - 1;
        if (property.empty())
        {
            std::string path;
            if (value == "none")
                picture.image = "";
            else if (Theme::resolveAsset(value, directory, assets, &path, error))
                picture.image = path;
            else
                return false;
        }
        else if (property == "_fit")
        {
            BackgroundFit fit;
            if (!parseFit(value, &fit))
                return fail("is cover, contain, tile or center, not " + value);
            picture.fit = fit;
        }
        else if (property == "_opacity")
        {
            float opacity;
            if (!parseNumber(value, 0.0f, 1.0f, &opacity))
                return fail("is a number from 0 to 1, not " + value);
            picture.opacity = opacity;
        }
        else if (property == "_height" && strip)
        {
            float height;
            if (!parseNumber(value, 1.0f, 720.0f, &height))
                return fail("is a height from 1 to 720, not " + value);
            picture.height = height;
        }
        else
            return fail("is not a key");
        return true;
    }

    if (key == "particles")
    {
        for (auto& [name, kind] : PARTICLE_KINDS)
            if (value == name)
            {
                decor->kind = kind;
                return true;
            }
        *error = "is none, snow, leaves, petals, embers, confetti, sparkles, hearts or bubbles, not " + value;
        return false;
    }
    if (key == "particles_density")
    {
        float density;
        if (value == "low" || value == "medium" || value == "high")
            density = value == "low" ? ThemeParticles::LOW : value == "medium" ? ThemeParticles::MEDIUM
                                                                               : ThemeParticles::HIGH;
        else if (!parseNumber(value, 0.0f, 1.0f, &density))
            return fail("is low, medium, high or a number from 0 to 1, not " + value);
        decor->density = density;
        return true;
    }
    if (key == "particles_speed")
    {
        float speed;
        if (!parseNumber(value, 0.25f, 4.0f, &speed))
            return fail("is a number from 0.25 to 4, not " + value);
        decor->speed = speed;
        return true;
    }
    if (key == "particles_layer")
    {
        if (value != "back" && value != "front")
            return fail("is back or front, not " + value);
        decor->layer = value == "back" ? ParticleLayer::BACK : ParticleLayer::FRONT;
        return true;
    }
    if (key == "particles_color")
    {
        std::vector<NVGcolor> colors;
        if (value == "auto")
            colors.clear();
        else if (!parseColorList(value, &colors))
            return fail("is up to 5 colours, or auto, not " + value);
        decor->colors = colors;
        return true;
    }
    return fail("is not a key");
}

// What a file does not say of its decorations, from the file it inherits,
// as inheritBackground
static void inheritDecor(DecorSpec* own, const DecorSpec* parent)
{
    auto inherit = [](auto* mine, const auto* theirs, auto field)
    {
        bool everyVariant = (mine[0].*field).has_value();
        if (!everyVariant)
            mine[0].*field = theirs[0].*field;
        for (int slot : { 1, 2 })
            if (!(mine[slot].*field).has_value() && !everyVariant)
                mine[slot].*field = theirs[slot].*field;
    };
    for (int i = 0; i < THEME_AREA_COUNT; i++)
    {
        DecorPictureSpec mine[3]         = { own[0].pictures[i], own[1].pictures[i], own[2].pictures[i] };
        const DecorPictureSpec theirs[3] = { parent[0].pictures[i], parent[1].pictures[i], parent[2].pictures[i] };
        inherit(mine, theirs, &DecorPictureSpec::image);
        inherit(mine, theirs, &DecorPictureSpec::fit);
        inherit(mine, theirs, &DecorPictureSpec::opacity);
        inherit(mine, theirs, &DecorPictureSpec::height);
        for (int slot = 0; slot < 3; slot++)
            own[slot].pictures[i] = mine[slot];
    }
    inherit(own, parent, &DecorSpec::kind);
    inherit(own, parent, &DecorSpec::density);
    inherit(own, parent, &DecorSpec::speed);
    inherit(own, parent, &DecorSpec::layer);
    inherit(own, parent, &DecorSpec::colors);
}

// Variant `v`'s decorations (0 light, 1 dark), as resolveBackground
static ThemeDecor resolveDecor(const DecorSpec* spec, int v)
{
    ThemeDecor decor;
    const DecorSpec& own = spec[v + 1];
    const DecorSpec& all = spec[0];
    for (int i = 0; i < THEME_AREA_COUNT; i++)
    {
        const DecorPictureSpec& mine  = own.pictures[i];
        const DecorPictureSpec& every = all.pictures[i];
        ThemePicture& picture         = decor.pictures[i];
        picture.image                 = mine.image.value_or(every.image.value_or(""));
        picture.fit                   = mine.fit.value_or(every.fit.value_or(BackgroundFit::COVER));
        picture.opacity               = mine.opacity.value_or(every.opacity.value_or(1.0f));
        picture.height                = mine.height.value_or(every.height.value_or(0.0f));
    }
    ThemeParticles& particles = decor.particles;
    particles.kind            = own.kind.value_or(all.kind.value_or(ParticleKind::NONE));
    particles.density         = own.density.value_or(all.density.value_or(ThemeParticles::LOW));
    particles.speed           = own.speed.value_or(all.speed.value_or(1.0f));
    particles.layer           = own.layer.value_or(all.layer.value_or(ParticleLayer::BACK));
    particles.colors          = own.colors.value_or(all.colors.value_or(std::vector<NVGcolor>()));
    return decor;
}

// `directory`: the folder of the file read; `assets`: the folder its
// pictures must stay in (the theme's, Theme::resolveAsset)
static void parseThemeText(const std::string& text, const std::string& directory, const std::string& assets,
    const std::string& label, int depth, bool root, ParsedTheme* out)
{
    auto where = [&label](int number)
    { return (label.empty() ? std::string("line ") : label + ":") + std::to_string(number); };

    std::stringstream lines(text);
    std::string line, section;
    bool sectionKnown = true;
    int number        = 0;
    while (std::getline(lines, line))
    {
        number++;
        // A # at the start or after a space begins a comment, unless it
        // begins the value (a colour)
        size_t equals = line.find('=');
        for (size_t i = 0; i < line.size(); i++)
        {
            if (line[i] != '#' || (i > 0 && line[i - 1] != ' ' && line[i - 1] != '\t'))
                continue;
            if (equals != std::string::npos && i > equals && trim(line.substr(equals + 1, i - equals - 1)).empty())
                continue;
            // Nor one after a comma: the next colour of a list
            std::string before = trim(line.substr(0, i));
            if (!before.empty() && before.back() == ',')
                continue;
            line = line.substr(0, i);
            break;
        }
        line = trim(line);
        if (line.empty())
            continue;

        if (startsWith(line, "@include"))
        {
            std::string name = trim(line.substr(8));
            if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
                name = name.substr(1, name.size() - 2);
            std::string path = resolvePath(name, directory);
            std::string included;
            if (depth >= MAX_DEPTH)
                out->errors.push_back(where(number) + ": @include " + name + " goes too deep (a loop?)");
            else if (!readThemeText(path, &included))
                out->errors.push_back(where(number) + ": @include " + name + ": cannot read " + path);
            else
            {
                // An include in the theme's folder keeps its pictures there
                std::string folder = directoryOf(path);
                bool inside        = folder == assets || startsWith(folder, assets + "/");
                out->files.push_back(path);
                parseThemeText(included, folder, inside ? assets : folder, name, depth + 1, false, out);
            }
            continue;
        }

        if (line.front() == '[' && line.back() == ']')
        {
            section      = trim(line.substr(1, line.size() - 2));
            sectionKnown = section == "colors" || section == "metrics" || section == "light" || section == "dark";
            if (!sectionKnown)
                out->errors.push_back(where(number) + ": unknown section [" + section + "], its lines skipped");
            out->lightSection = out->lightSection || section == "light";
            out->darkSection  = out->darkSection || section == "dark";
            continue;
        }
        if (!sectionKnown)
            continue;

        equals = line.find('=');
        if (equals == std::string::npos)
        {
            out->errors.push_back(where(number) + ": no '=' in \"" + (line.size() > 40 ? line.substr(0, 40) + "..." : line) + "\"");
            continue;
        }
        std::string key   = trim(line.substr(0, equals));
        std::string value = trim(line.substr(equals + 1));

        if (section == "metrics")
        {
            char* end;
            float metric = std::strtof(value.c_str(), &end);
            if (!value.empty() && *end == '\0')
                out->metrics.emplace_back(key, metric);
            else
                out->errors.push_back(where(number) + ": " + value + " is not a number");
            continue;
        }

        // The background, for every variant or one; an included file's is
        // the including one's too
        if ((section.empty() || section == "light" || section == "dark")
            && (key == "background_image" || key == "background_fit" || key == "background_dim" || key == "surface_opacity"))
        {
            BackgroundSpec& background = out->background[section == "light" ? 1 : section == "dark" ? 2
                                                                                                    : 0];
            if (key == "background_image")
            {
                std::string path, error;
                if (value == "none")
                    background.image = "";
                else if (Theme::resolveAsset(value, directory, assets, &path, &error))
                    background.image = path;
                else
                    out->errors.push_back(where(number) + ": background_image " + error);
            }
            else if (key == "background_fit")
            {
                BackgroundFit fit;
                if (parseFit(value, &fit))
                    background.fit = fit;
                else
                    out->errors.push_back(where(number) + ": background_fit is cover, contain, tile or center, not " + value);
            }
            else
            {
                char* end;
                float number_ = std::strtof(value.c_str(), &end);
                if (value.empty() || *end != '\0' || !(number_ >= 0.0f && number_ <= 1.0f))
                    out->errors.push_back(where(number) + ": " + key + " is a number from 0 to 1, not " + value);
                else if (key == "background_dim")
                    background.dim = number_;
                else
                    background.surfaceOpacity = number_;
            }
            continue;
        }

        // Logos and images, for every variant or one
        if ((section.empty() || section == "light" || section == "dark") && isPictureKey(key))
        {
            std::string error;
            PictureSpec& pictures = out->pictures[section == "light" ? 1 : section == "dark" ? 2
                                                                                             : 0];
            if (!parsePictureKey(key, value, directory, assets, &pictures, &error))
                out->errors.push_back(where(number) + ": " + error);
            continue;
        }

        // The decorations, as the background
        if ((section.empty() || section == "light" || section == "dark") && isDecorKey(key))
        {
            std::string error;
            DecorSpec& decor = out->decor[section == "light" ? 1 : section == "dark" ? 2
                                                                                     : 0];
            if (!parseDecorLine(key, value, directory, assets, &decor, &error))
                out->errors.push_back(where(number) + ": " + key + " " + error);
            continue;
        }

        if (section.empty() && key == "icon")
        {
            // As background_image: an included file's counts, an inherited one's too
            out->icon = value;
            continue;
        }

        if (section.empty()
            && (key == "name" || key == "author" || key == "version" || key == "description" || key == "inherit"
                || key == "counterpart"))
        {
            // An included file's own are not this one's
            if (!root)
                continue;
            if (key == "name")
                out->name = value;
            else if (key == "author")
                out->author = value;
            else if (key == "version")
                out->version = value;
            else if (key == "description")
                out->description = value;
            else if (key == "counterpart")
                out->counterpart = resolvePath(value, directory);
            else
                out->inherit = value;
            continue;
        }

        if (section.empty() && !isSeed(key) && key.find('/') == std::string::npos)
        {
            out->errors.push_back(where(number) + ": unknown key " + key);
            continue;
        }

        NVGcolor color;
        if (!Theme::parseColor(value, &color))
        {
            out->errors.push_back(where(number) + ": " + value + " is not a colour");
            continue;
        }
        int slot = section == "light" ? 1 : section == "dark" ? 2
                                                              : 0;
        (isSeed(key) ? out->seeds : out->colors)[slot].emplace_back(key, color);
    }
}

// What a file does not say of its background, from the file it inherits:
// its own lines win, those for every variant over the inherited file's for
// one variant
static void inheritBackground(BackgroundSpec* own, const BackgroundSpec* parent)
{
    auto inherit = [&](auto field)
    {
        bool everyVariant = (own[0].*field).has_value();
        if (!everyVariant)
            own[0].*field = parent[0].*field;
        for (int slot : { 1, 2 })
            if (!(own[slot].*field).has_value() && !everyVariant)
                own[slot].*field = parent[slot].*field;
    };
    inherit(&BackgroundSpec::image);
    inherit(&BackgroundSpec::fit);
    inherit(&BackgroundSpec::dim);
    inherit(&BackgroundSpec::surfaceOpacity);
}

// The file's `inherit` and the variants it styles: the inherited file's
// lines come first, this one's over them
static void resolveInherit(ParsedTheme* theme, const std::string& directory, int depth)
{
    const std::string& inherit = theme->inherit;
    if (inherit.empty() || inherit == "light" || inherit == "dark")
    {
        bool sections = theme->lightSection || theme->darkSection;
        theme->light  = theme->lightSection || inherit == "light";
        theme->dark   = theme->darkSection || inherit == "dark" || (!sections && inherit.empty());
        return;
    }

    std::string path = resolvePath(inherit, directory);
    std::string text;
    ParsedTheme parent;
    if (depth >= MAX_DEPTH)
        theme->errors.push_back("inherit " + inherit + " goes too deep (a loop?)");
    else if (!readThemeText(path, &text))
        theme->errors.push_back("inherit " + inherit + ": cannot read " + path);
    else
    {
        theme->files.push_back(path);
        parseThemeText(text, directoryOf(path), directoryOf(path), inherit, depth + 1, true, &parent);
        resolveInherit(&parent, directoryOf(path), depth + 1);
        theme->files.insert(theme->files.end(), parent.files.begin(), parent.files.end());
    }

    for (int slot = 0; slot < 3; slot++)
    {
        theme->seeds[slot].insert(theme->seeds[slot].begin(), parent.seeds[slot].begin(), parent.seeds[slot].end());
        theme->colors[slot].insert(theme->colors[slot].begin(), parent.colors[slot].begin(), parent.colors[slot].end());
    }
    theme->metrics.insert(theme->metrics.begin(), parent.metrics.begin(), parent.metrics.end());
    inheritBackground(theme->background, parent.background);
    inheritPictures(theme->pictures, parent.pictures);
    inheritDecor(theme->decor, parent.decor);
    if (theme->icon.empty())
        theme->icon = parent.icon;
    theme->errors.insert(theme->errors.begin(), parent.errors.begin(), parent.errors.end());
    theme->light = parent.light || theme->lightSection;
    theme->dark  = parent.dark || theme->darkSection;
    if (!theme->light && !theme->dark)
        theme->dark = true;
}

namespace
{
    // What a file makes of each variant
    struct ResolvedTheme
    {
        ColorMap overlay[2]; // [0] light, [1] dark; empty for a variant not styled
        MetricMap metrics;
        ThemeBackground background[2]; // as overlay; no image for a variant not styled
        ThemePictures pictures[2]; // as background
        ThemeDecor decor[2]; // as background
        std::vector<std::string> warnings;
    };
}

// Variant `v`'s background (0 light, 1 dark): its own lines, else those for
// every variant, else the defaults
static ThemeBackground resolveBackground(const BackgroundSpec* spec, int v)
{
    ThemeBackground background;
    const BackgroundSpec& own = spec[v + 1];
    const BackgroundSpec& all = spec[0];
    background.image          = own.image.value_or(all.image.value_or(""));
    background.fit            = own.fit.value_or(all.fit.value_or(BackgroundFit::COVER));
    background.dim            = own.dim.value_or(all.dim.value_or(ThemeBackground::DEFAULT_DIM));
    background.surfaceOpacity = background.hasImage() ? own.surfaceOpacity.value_or(all.surfaceOpacity.value_or(1.0f)) : 1.0f;
    return background;
}

// `decodeImages`: a background image's colour, for the contrast checks, is
// worked out here when not known yet (inspect); else (a theme used) the
// checks wait for it to be loaded (Application, checkBackgroundContrast)
static ResolvedTheme resolveTheme(const ParsedTheme& theme, bool decodeImages = false)
{
    ResolvedTheme resolved;
    for (int v = 0; v < 2; v++)
    {
        if (!(v == 0 ? theme.light : theme.dark))
            continue;
        ThemeVariant variant = v == 0 ? ThemeVariant::LIGHT : ThemeVariant::DARK;
        const ColorMap& base = Theme::get(variant).getValues()->getBaseColors();
        const char* name     = v == 0 ? "light" : "dark";

        ColorMap seeds;
        for (int slot : { 0, v + 1 })
            for (auto& [key, color] : theme.seeds[slot])
                seeds[key] = color;
        ColorMap overlay = derivePalette(seeds, base);
        for (int slot : { 0, v + 1 })
            for (auto& [key, color] : theme.colors[slot])
            {
                if (!base.count(key))
                    resolved.warnings.push_back(std::string(name) + ": unknown key " + key + " (kept)");
                overlay[key] = color;
            }

        ColorMap all = base;
        for (auto& [key, color] : overlay)
            all[key] = color;

        // Over a background image, the surfaces let it show through
        const ThemeBackground& background = resolved.background[v] = resolveBackground(theme.background, v);
        resolved.decor[v]                                          = resolveDecor(theme.decor, v);
        if (background.hasImage() && background.surfaceOpacity < 1.0f)
            for (const std::string& key : surfaceKeys)
            {
                auto it = all.find(key);
                if (it == all.end())
                    continue;
                it->second.a *= background.surfaceOpacity;
                overlay[key] = it->second;
            }

        NVGcolor mean;
        bool known = background.hasImage()
            && (decodeImages ? ThemeImages::meanColor(background.image, &mean)
                             : ThemeImages::knownMeanColor(background.image, &mean));
        NVGcolor backdrop = known ? backdropOf(all, background, mean) : NVGcolor {};
        for (const std::string& warning : checkColors(all, known ? &backdrop : nullptr))
            resolved.warnings.push_back(std::string(name) + ": " + warning);
        resolved.overlay[v]  = std::move(overlay);
        resolved.pictures[v] = resolvePictures(theme.pictures, v, &resolved.warnings);
    }
    for (auto& [key, metric] : theme.metrics)
    {
        if (!getStyle().hasMetric(key))
            resolved.warnings.push_back("unknown metric " + key + " (kept)");
        resolved.metrics[key] = metric;
    }
    // Several warnings for one unknown key styled in both variants: once
    std::sort(resolved.warnings.begin(), resolved.warnings.end());
    resolved.warnings.erase(std::unique(resolved.warnings.begin(), resolved.warnings.end()), resolved.warnings.end());
    return resolved;
}

static ParsedTheme parseTheme(const std::string& text, const std::string& directory)
{
    ParsedTheme theme;
    parseThemeText(text, directory, directory, "", 0, true, &theme);
    resolveInherit(&theme, directory, 0);
    return theme;
}

// "@i18n/…": a string of the app's catalogs (the presets' names)
static std::string localized(const std::string& value)
{
    return startsWith(value, "@i18n/") ? getStr(value.substr(6)) : value;
}

static std::string currentPath;
static std::string currentText; // its text, as read
static std::unique_ptr<ParsedTheme> currentTheme; // the file in use
static ThemeBackground currentBackground[2]; // its backgrounds, [0] light
static ThemePictures currentPictures[2]; // its logos and images, [0] light
static uint64_t picturesGeneration = 1; // bumped when they change
static ThemeDecor currentDecor[2]; // its decorations, [0] light
static std::time_t calendarMinute = -1; // when followCalendar() last looked
static bool reapplying            = false;

static void useOverlays(const ResolvedTheme& resolved)
{
    Theme::getLightTheme().getValues()->setOverlay(resolved.overlay[0]);
    Theme::getDarkTheme().getValues()->setOverlay(resolved.overlay[1]);
    getStyle().getValues()->setOverlay(resolved.metrics);
    currentBackground[0] = resolved.background[0];
    currentBackground[1] = resolved.background[1];
    if (!(currentPictures[0] == resolved.pictures[0] && currentPictures[1] == resolved.pictures[1]))
    {
        currentPictures[0] = resolved.pictures[0];
        currentPictures[1] = resolved.pictures[1];
        picturesGeneration++;
    }
    currentDecor[0] = resolved.decor[0];
    currentDecor[1] = resolved.decor[1];
    generation++;
}

static void reapplyTheme()
{
    if (!currentTheme || reapplying)
        return;
    reapplying = true;
    useOverlays(resolveTheme(*currentTheme));
    reapplying = false;
}

static bool applyTheme(const std::string& text, const std::string& directory, const std::string& path, ThemeVariant* variant)
{
    ParsedTheme theme      = parseTheme(text, directory);
    ResolvedTheme resolved = resolveTheme(theme);
    std::string label      = path.empty() ? "theme" : "theme " + path;
    for (const std::string& error : theme.errors)
        Logger::warning("{}: {}", label, error);
    for (const std::string& warning : resolved.warnings)
        Logger::warning("{}: {}", label, warning);

    useOverlays(resolved);
    currentPath  = path;
    currentText  = text;
    currentTheme = std::make_unique<ParsedTheme>(theme);

    // A file's own text is read again on a change; text given to load() is not
    std::vector<std::string> watched;
    if (!path.empty())
    {
        watched.push_back(path);
        watched.insert(watched.end(), theme.files.begin(), theme.files.end());
    }
    HotReload::setThemeFiles(watched);

    if (variant)
    {
        if (theme.light != theme.dark)
            *variant = theme.light ? ThemeVariant::LIGHT : ThemeVariant::DARK;
        else
            *variant = Application::getPlatform() ? Application::getThemeVariant() : ThemeVariant::DARK;
    }
    Logger::info("{}: {} for the {} variant{}", label, theme.name.empty() ? "read" : localized(theme.name),
        theme.light && theme.dark ? "light and dark" : theme.light ? "light"
                                                                   : "dark",
        theme.light && theme.dark ? "s" : "");
    return true;
}

bool Theme::load(const std::string& text, ThemeVariant* variant, const std::string& directory)
{
    return applyTheme(text, directory, "", variant);
}

bool Theme::loadFile(const std::string& path, ThemeVariant* variant)
{
    std::string text;
    if (!readThemeText(path, &text))
    {
        Logger::error("theme: cannot read {}", path);
        return false;
    }
    return applyTheme(text, directoryOf(path), path, variant);
}

void Theme::followCalendar()
{
    if (currentPath != SEASONAL_PATH)
        return;
    std::time_t minute = getWallTime() / 60;
    if (minute == calendarMinute)
        return;
    calendarMinute = minute;
    // The day's preset (each has both variants: the variant stays)
    std::string text;
    if (readThemeText(currentPath, &text) && text != currentText)
        applyTheme(text, directoryOf(currentPath), currentPath, nullptr);
}

void Theme::reset()
{
    currentTheme.reset();
    currentPath.clear();
    currentText.clear();
    HotReload::setThemeFiles({});
    useOverlays(ResolvedTheme());
}

bool Theme::styles(ThemeVariant variant)
{
    return !currentTheme || (variant == ThemeVariant::LIGHT ? currentTheme->light : currentTheme->dark);
}

const std::string& Theme::getCurrentPath()
{
    return currentPath;
}

ThemeBackground Theme::getBackground(ThemeVariant variant)
{
    return currentBackground[variant == ThemeVariant::LIGHT ? 0 : 1];
}

ThemeDecor Theme::getDecor(ThemeVariant variant)
{
    return currentDecor[variant == ThemeVariant::LIGHT ? 0 : 1];
}

std::string Theme::getBackgroundImage()
{
    return getBackground(Application::getPlatform() ? Application::getThemeVariant() : ThemeVariant::DARK).image;
}

std::vector<std::string> Theme::checkContrast(ThemeVariant variant)
{
    const ColorMap& colors     = Theme::get(variant).getValues()->getColors();
    ThemeBackground background = getBackground(variant);
    NVGcolor mean;
    if (background.hasImage() && ThemeImages::knownMeanColor(background.image, &mean))
    {
        NVGcolor backdrop = backdropOf(colors, background, mean);
        return checkColors(colors, &backdrop);
    }
    return checkColors(colors);
}

std::vector<std::string> Theme::checkBackgroundContrast(ThemeVariant variant)
{
    const ColorMap& colors     = Theme::get(variant).getValues()->getColors();
    ThemeBackground background = getBackground(variant);
    NVGcolor mean;
    if (!background.hasImage() || !ThemeImages::knownMeanColor(background.image, &mean))
        return {};
    NVGcolor backdrop = backdropOf(colors, background, mean);
    return checkColors(colors, &backdrop, true);
}

static ThemeInfo::Preview previewOf(const ColorMap& base, const ColorMap& overlay)
{
    auto color = [&](const char* key, const char* fallback)
    {
        for (const char* name : { key, fallback })
        {
            auto it = overlay.find(name);
            if (it != overlay.end())
                return it->second;
            it = base.find(name);
            if (it != base.end())
                return it->second;
        }
        return nvgRGB(0, 0, 0);
    };
    ThemeInfo::Preview preview;
    preview.background = color("brls/background", "brls/clear");
    preview.surface    = color("cadran/surf", "brls/highlight/background");
    preview.text       = color("brls/text", "brls/text");
    preview.accent     = color("brls/sidebar/active_item", "brls/accent");
    preview.highlight  = color("brls/highlight/color1", "brls/highlight/color1");
    return preview;
}

// `decodePictures`: its pictures decoded here, to check that they are
// pictures and the text on them (an explicit check); else only that they
// are there, and the text on those whose colour is known already
static ThemeInfo inspectText(const std::string& text, const std::string& path, bool decodePictures = false)
{
    ThemeInfo info;
    info.path = path;
    // A theme folder's file is named after its folder
    std::string named = path;
    if (named == THEME_FOLDER_FILE || endsWith(named, std::string("/") + THEME_FOLDER_FILE))
        named = directoryOf(named);
    std::string file = named.substr(named.find_last_of("/:") == std::string::npos ? 0 : named.find_last_of("/:") + 1);
    info.id          = endsWith(file, ".theme") ? file.substr(0, file.size() - 6) : file;

    ParsedTheme theme      = parseTheme(text, directoryOf(path));
    ResolvedTheme resolved = resolveTheme(theme, decodePictures);
    info.name              = theme.name.empty() ? info.id : localized(theme.name);
    info.author            = localized(theme.author);
    info.version           = theme.version;
    info.description       = localized(theme.description);
    info.light             = theme.light;
    info.dark              = theme.dark;
    info.lightBackground   = resolved.background[0];
    info.darkBackground    = resolved.background[1];
    info.backgroundImage   = info.darkBackground.hasImage() ? info.darkBackground.image : info.lightBackground.image;
    info.lightDecor        = resolved.decor[0];
    info.darkDecor         = resolved.decor[1];
    info.icon              = theme.icon;
    info.lightPictures     = resolved.pictures[0];
    info.darkPictures      = resolved.pictures[1];
    info.counterpart       = theme.counterpart;
    info.lightPreview      = previewOf(Theme::getLightTheme().getValues()->getBaseColors(), resolved.overlay[0]);
    info.darkPreview       = previewOf(Theme::getDarkTheme().getValues()->getBaseColors(), resolved.overlay[1]);
    info.errors            = theme.errors;
    info.warnings          = resolved.warnings;

    // Its pictures must be there (and be pictures, when decoded)
    for (const ThemeBackground* background : { &info.lightBackground, &info.darkBackground })
    {
        NVGcolor mean;
        std::string error = "background_image: cannot read " + background->image;
        bool readable     = decodePictures ? ThemeImages::meanColor(background->image, &mean)
                                           : ThemeImages::exists(background->image);
        if (background->hasImage() && !readable && std::find(info.errors.begin(), info.errors.end(), error) == info.errors.end())
            info.errors.push_back(error);
    }

    // So must its logos and images
    for (const ThemePictures* pictures : { &info.lightPictures, &info.darkPictures })
    {
        std::vector<std::pair<std::string, std::string>> named = { { "logo", pictures->logo }, { "preview", pictures->preview } };
        for (auto& [name, icon] : pictures->icons)
            named.emplace_back(ICON_PREFIX + name, icon.image);
        for (auto& [slot, path] : pictures->images)
            named.emplace_back(IMAGE_PREFIX + slot, path);
        for (auto& [key, path] : named)
        {
            NVGcolor mean;
            std::string error = key + ": cannot read " + path;
            bool readable     = decodePictures ? ThemeImages::meanColor(path, &mean) : ThemeImages::exists(path);
            if (!path.empty() && !readable && std::find(info.errors.begin(), info.errors.end(), error) == info.errors.end())
                info.errors.push_back(error);
        }
    }
    // Its decorations' pictures too
    for (const ThemeDecor* decor : { &info.lightDecor, &info.darkDecor })
        for (int i = 0; i < THEME_AREA_COUNT; i++)
        {
            const ThemePicture& picture = decor->pictures[i];
            if (!picture.hasImage())
                continue;
            NVGcolor mean;
            std::string error = std::string(DECOR_PICTURES[i]) + ": cannot read " + picture.image;
            bool readable     = decodePictures ? ThemeImages::meanColor(picture.image, &mean) : ThemeImages::exists(picture.image);
            if (!readable && std::find(info.errors.begin(), info.errors.end(), error) == info.errors.end())
                info.errors.push_back(error);
        }

    // The counterpart must have the variant this one has not
    if (!theme.counterpart.empty())
    {
        std::string other;
        if (theme.light == theme.dark)
            info.errors.push_back("counterpart: only for a file that styles one variant");
        else if (!readThemeText(theme.counterpart, &other))
            info.errors.push_back("counterpart: cannot read " + theme.counterpart);
        else
        {
            ParsedTheme counterpart = parseTheme(other, directoryOf(theme.counterpart));
            if (!(theme.light ? counterpart.dark : counterpart.light))
                info.errors.push_back(std::string("counterpart: ") + theme.counterpart + " has no " + (theme.light ? "dark" : "light")
                    + " variant");
        }
    }
    return info;
}

ThemeInfo Theme::inspect(const std::string& path, bool decodePictures)
{
    std::string text;
    if (!readThemeText(path, &text))
    {
        ThemeInfo info;
        info.path = path;
        info.id = info.name = path;
        info.errors.push_back("cannot read " + path);
        return info;
    }
    return inspectText(text, path, decodePictures);
}

static bool byName(const ThemeInfo& a, const ThemeInfo& b)
{
    return a.name < b.name;
}

std::vector<ThemeInfo> Theme::listThemes(const std::string& directory, bool decodePictures)
{
    std::vector<std::string> paths;
    std::string folder = directory;
    while (folder.size() > 1 && folder.back() == '/')
        folder.pop_back();
#ifdef USE_LIBROMFS
    if (startsWith(folder, "@res/"))
    {
        try
        {
            for (auto& entry : romfs::list(folder.substr(5)))
                if (endsWith(entry.filename().string(), ".theme"))
                    paths.push_back("@res/" + entry.string());
            // The theme folders' (romfs lists the files of a folder only)
            fs::path parent = folder.substr(5);
            for (auto& entry : romfs::list())
                if (entry.filename() == THEME_FOLDER_FILE && entry.parent_path().parent_path() == parent)
                    paths.push_back("@res/" + entry.generic_string());
        }
        catch (const std::exception& e)
        {
        }
    }
    else
#endif
    {
        std::string real = startsWith(folder, "@res/") ? std::string(BRLS_RESOURCES) + folder.substr(5) : folder;
        try
        {
            if (fs::is_directory(real))
                for (const auto& entry : fs::directory_iterator(real))
                {
                    std::string name = entry.path().filename().string();
                    if (fs::is_directory(entry.path()))
                    {
                        if (fs::is_regular_file(entry.path() / THEME_FOLDER_FILE))
                            paths.push_back(folder + "/" + name + "/" + THEME_FOLDER_FILE);
                    }
                    else if (endsWith(name, ".theme"))
                        paths.push_back(folder + "/" + name);
                }
        }
        catch (const std::exception& e)
        {
            Logger::warning("theme: cannot list {}: {}", directory, e.what());
        }
    }

    std::vector<ThemeInfo> themes;
    for (const std::string& path : paths)
        themes.push_back(Theme::inspect(path, decodePictures));
    std::sort(themes.begin(), themes.end(), byName);
    return themes;
}

std::string Theme::seasonalTheme(std::time_t when)
{
    return std::string(BUILTIN_PREFIX) + seasonalThemeId(when);
}

ThemePictures Theme::getPictures(ThemeVariant variant)
{
    return currentPictures[variant == ThemeVariant::LIGHT ? 0 : 1];
}

void Theme::addIconNames(const std::vector<std::string>& names)
{
    iconNames.insert(iconNames.end(), names.begin(), names.end());
    std::sort(iconNames.begin(), iconNames.end());
    iconNames.erase(std::unique(iconNames.begin(), iconNames.end()), iconNames.end());
}

void Theme::addImageSlot(const std::string& key, const std::string& fallback, int maxSide)
{
    imageSlots[key] = ImageSlot { fallback, maxSide };
    picturesGeneration++; // its fallback may have changed
}

std::vector<std::string> Theme::getImageSlots()
{
    std::vector<std::string> keys;
    for (auto& [key, slot] : imageSlots)
        keys.push_back(key);
    return keys;
}

// The pictures loaded for the theme and the variant in use, by key: dropped
// (and freed with their last holder) when either changes
static std::map<std::string, std::shared_ptr<ThemeImage>> heldPictures;
static uint64_t heldGeneration = 0;
static int heldVariant         = -1;

static std::shared_ptr<ThemeImage> heldPicture(const std::string& key, const std::string& path, const ThemeImageOptions& options)
{
    int variant = Application::getPlatform() && Application::getThemeVariant() == ThemeVariant::LIGHT ? 0 : 1;
    if (heldGeneration != picturesGeneration || heldVariant != variant)
    {
        heldPictures.clear();
        heldGeneration = picturesGeneration;
        heldVariant    = variant;
    }
    if (path.empty())
        return nullptr;
    std::shared_ptr<ThemeImage>& held = heldPictures[key];
    if (!held || held->path != path)
        held = ThemeImages::load(path, options);
    return held;
}

static const ThemePictures& picturesInUse()
{
    return currentPictures[Application::getPlatform() && Application::getThemeVariant() == ThemeVariant::LIGHT ? 0 : 1];
}

std::shared_ptr<ThemeImage> Theme::logo()
{
    ThemeImageOptions options;
    options.flags   = NVG_IMAGE_GENERATE_MIPMAPS;
    options.maxSide = ThemeImages::LOGO_SIZE;
    return heldPicture("logo", picturesInUse().logo, options);
}

std::shared_ptr<ThemeImage> Theme::icon(const std::string& name, bool* tint)
{
    const ThemePictures& pictures = picturesInUse();
    if (pictures.icons.empty())
        return nullptr;
    auto it = pictures.icons.find(name);
    if (it == pictures.icons.end())
        return nullptr;
    if (tint)
        *tint = it->second.tint;
    ThemeImageOptions options;
    options.flags   = NVG_IMAGE_GENERATE_MIPMAPS;
    options.maxSide = ThemeImages::ICON_SIZE;
    options.mask    = it->second.tint;
    return heldPicture(ICON_PREFIX + name, it->second.image, options);
}

std::shared_ptr<ThemeImage> Theme::image(const std::string& key)
{
    auto slot = imageSlots.find(key);
    if (slot == imageSlots.end())
        return nullptr;
    ThemeImageOptions options;
    options.maxSide               = slot->second.maxSide;
    const ThemePictures& pictures = picturesInUse();
    auto it                       = pictures.images.find(key);
    if (it != pictures.images.end())
    {
        std::shared_ptr<ThemeImage> image = heldPicture(IMAGE_PREFIX + key, it->second, options);
        if (image && image->state != ThemeImage::State::FAILED)
            return image;
    }
    // The app's own
    return heldPicture("fallback." + key, slot->second.fallback, options);
}

std::vector<ThemeInfo> Theme::builtinThemes()
{
    std::vector<ThemeInfo> themes;
    for (const BuiltinTheme& theme : builtinThemeTexts())
        themes.push_back(inspectText(builtinThemeText(theme, getWallTime()), std::string(BUILTIN_PREFIX) + theme.id));
    return themes;
}

} // namespace brls
