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
    auto channel = [](float v) { return v <= 0.03928f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f); };
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
    bool dark                 = isDark(background);
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
    auto has = [&seeds](const char* seed) { return seeds.count(seed) > 0; };
    NVGcolor B = has("background") ? seeds.at("background") : lookup(base, "brls/background", nvgRGB(45, 45, 45));
    NVGcolor T = has("text") ? seeds.at("text") : lookup(base, "brls/text", nvgRGB(255, 255, 255));
    NVGcolor S = has("surface") ? seeds.at("surface")
                                : lookup(base, "cadran/surf", lookup(base, "brls/highlight/background", B));
    NVGcolor S2  = has("surface") || has("text") ? mix(S, T, 0.05f) : lookup(base, "cadran/surf2", S);
    bool dark    = isDark(B);
    auto alpha   = [dark](NVGcolor c, int onDark, int onLight) { return withAlpha(c, dark ? onDark : onLight); };
    auto inkOn   = [&](NVGcolor c, float minimum) { return ink(c, { B, S, S2 }, B, minimum); };

    if (has("background"))
    {
        out["brls/background"]         = B;
        out["brls/clear"]              = B;
        out["brls/sidebar/background"] = B;
    }

    if (has("surface"))
    {
        out["brls/highlight/background"]                = S;
        out["cadran/surf"]                              = S;
        out["cadran/surf2"]                             = S2;
        out["brls/button/default_enabled_background"]  = S2;
        out["brls/button/default_disabled_background"] = S2;
    }

    if (has("text") || has("background"))
    {
        NVGcolor text2 = inkOn(mix(T, B, 0.30f), 4.5f);
        NVGcolor text3 = inkOn(mix(T, B, 0.45f), 4.5f);
        NVGcolor line  = alpha(T, 36, 26);
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
        NVGcolor A    = has("accent") ? seeds.at("accent") : lookup(base, "brls/accent", T);
        NVGcolor fill = has("accent") ? A : lookup(base, "cadran/teal_fill", A);
        NVGcolor railActive = withAlpha(fill, 36);
        NVGcolor accentInk  = ink(fill, { B, S, S2, railActive }, B, 4.5f);
        NVGcolor ring      = ink(fill, { B }, B, 3.0f);
        NVGcolor glow      = ink(dark ? mix(fill, nvgRGB(255, 255, 255), 0.5f) : mix(fill, B, 0.25f), { B }, B, 3.0f);
        NVGcolor onFill    = onColor(fill, T, B);
        out["brls/accent"]                               = accentInk;
        out["brls/sidebar/active_item"]                  = accentInk;
        out["brls/button/highlight_enabled_text"]        = accentInk;
        out["brls/button/highlight_disabled_text"]       = accentInk;
        out["brls/highlight/color1"]                     = ring;
        out["brls/highlight/color2"]                     = glow;
        out["brls/click_pulse"]                          = withAlpha(fill, 38);
        out["brls/button/primary_enabled_background"]   = fill;
        out["brls/button/primary_enabled_text"]         = onFill;
        out["brls/button/primary_disabled_background"]  = mix(fill, B, 0.6f);
        out["brls/button/primary_disabled_text"]        = mix(onFill, mix(fill, B, 0.6f), 0.5f);
        out["brls/slider/line_filled"]                   = fill;
        out["cadran/teal"]                               = accentInk;
        out["cadran/teal_fill"]                          = fill;
        out["cadran/teal_soft"]                          = withAlpha(fill, 26);
        out["cadran/glow"]                               = alpha(fill, 31, 41);
        out["cadran/glow_strong"]                        = alpha(fill, 51, 56);
        out["cadran/pill_ok"]                            = dark ? withAlpha(fill, 41) : over(withAlpha(fill, 46), S);
        out["cadran/pill_ok_ink"]                        = ink(accentInk, { B, S, out["cadran/pill_ok"] }, B, 4.5f);
        out["cadran/rail_active"]                        = railActive;
        out["cadran/hl1"]                                = ring;
        out["cadran/hl2"]                                = glow;
        addAccentShades(out, fill, B, T);
    }

    if (has("warning"))
    {
        NVGcolor W           = seeds.at("warning");
        NVGcolor surface     = dark ? withAlpha(W, 26) : over(withAlpha(W, 36), S);
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
        bestRatio = std::min(Theme::contrast(best, fills[0]), Theme::contrast(best, fills[1]));
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

static std::vector<std::string> checkColors(const ColorMap& colors)
{
    std::vector<std::string> warnings;
    auto background = colors.find("brls/background");
    if (background == colors.end())
        return warnings;
    NVGcolor under = over(background->second, nvgRGB(0, 0, 0));
    for (auto& check : contrastChecks)
    {
        auto ink     = colors.find(check.ink);
        auto surface = colors.find(check.surface);
        if (ink == colors.end() || surface == colors.end())
            continue;
        NVGcolor drawnSurface = over(surface->second, under);
        float ratio           = Theme::contrast(over(ink->second, drawnSurface), drawnSurface);
        if (ratio < check.minimum)
        {
            char text[256];
            std::snprintf(text, sizeof(text), "%s on %s reads badly: %.1f:1, under %.1f:1", check.ink.c_str(),
                check.surface.c_str(), ratio, check.minimum);
            warnings.push_back(text);
        }
    }
    return warnings;
}

std::vector<std::string> Theme::checkContrast(ThemeVariant variant)
{
    return checkColors(Theme::get(variant).getValues()->getColors());
}

// ---------------------------------------------------------------------------
// Theme files
// ---------------------------------------------------------------------------

static const char* BUILTIN_PREFIX = "builtin:";
static const int MAX_DEPTH        = 8;

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
        *text = theme->text;
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

namespace
{
// A file read, with what it includes and inherits
struct ParsedTheme
{
    std::string name, author, version, description;
    std::string inherit;
    std::string counterpart;     // its path, from the file's folder
    std::string backgroundImage; // its path, from the file's folder
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

static void parseThemeText(const std::string& text, const std::string& directory, const std::string& label, int depth,
    bool root, ParsedTheme* out)
{
    auto where = [&label](int number) { return (label.empty() ? std::string("line ") : label + ":") + std::to_string(number); };

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
                out->files.push_back(path);
                parseThemeText(included, directoryOf(path), name, depth + 1, false, out);
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

        if (section.empty() && key == "background_image")
        {
            // An included file's is the including one's too
            out->backgroundImage = resolvePath(value, directory);
            if (startsWith(out->backgroundImage, BUILTIN_PREFIX))
                out->errors.push_back(where(number) + ": background_image is a file, not " + value);
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
        int slot = section == "light" ? 1 : section == "dark" ? 2 : 0;
        (isSeed(key) ? out->seeds : out->colors)[slot].emplace_back(key, color);
    }
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
        parseThemeText(text, directoryOf(path), inherit, depth + 1, true, &parent);
        resolveInherit(&parent, directoryOf(path), depth + 1);
        theme->files.insert(theme->files.end(), parent.files.begin(), parent.files.end());
    }

    for (int slot = 0; slot < 3; slot++)
    {
        theme->seeds[slot].insert(theme->seeds[slot].begin(), parent.seeds[slot].begin(), parent.seeds[slot].end());
        theme->colors[slot].insert(theme->colors[slot].begin(), parent.colors[slot].begin(), parent.colors[slot].end());
    }
    theme->metrics.insert(theme->metrics.begin(), parent.metrics.begin(), parent.metrics.end());
    if (theme->backgroundImage.empty())
        theme->backgroundImage = parent.backgroundImage;
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
    std::vector<std::string> warnings;
};
}

static ResolvedTheme resolveTheme(const ParsedTheme& theme)
{
    ResolvedTheme resolved;
    for (int v = 0; v < 2; v++)
    {
        if (!(v == 0 ? theme.light : theme.dark))
            continue;
        ThemeVariant variant  = v == 0 ? ThemeVariant::LIGHT : ThemeVariant::DARK;
        const ColorMap& base  = Theme::get(variant).getValues()->getBaseColors();
        const char* name      = v == 0 ? "light" : "dark";

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
        for (const std::string& warning : checkColors(all))
            resolved.warnings.push_back(std::string(name) + ": " + warning);
        resolved.overlay[v] = std::move(overlay);
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
    parseThemeText(text, directory, "", 0, true, &theme);
    resolveInherit(&theme, directory, 0);
    return theme;
}

// "@i18n/…": a string of the app's catalogs (the presets' names)
static std::string localized(const std::string& value)
{
    return startsWith(value, "@i18n/") ? getStr(value.substr(6)) : value;
}

static std::string currentPath;
static std::unique_ptr<ParsedTheme> currentTheme; // the file in use
static bool reapplying = false;

static void useOverlays(const ResolvedTheme& resolved)
{
    Theme::getLightTheme().getValues()->setOverlay(resolved.overlay[0]);
    Theme::getDarkTheme().getValues()->setOverlay(resolved.overlay[1]);
    getStyle().getValues()->setOverlay(resolved.metrics);
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
    ParsedTheme theme       = parseTheme(text, directory);
    ResolvedTheme resolved  = resolveTheme(theme);
    std::string label       = path.empty() ? "theme" : "theme " + path;
    for (const std::string& error : theme.errors)
        Logger::warning("{}: {}", label, error);
    for (const std::string& warning : resolved.warnings)
        Logger::warning("{}: {}", label, warning);

    useOverlays(resolved);
    currentPath  = path;
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
        theme.light && theme.dark ? "light and dark" : theme.light ? "light" : "dark", theme.light && theme.dark ? "s" : "");
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

void Theme::reset()
{
    currentTheme.reset();
    currentPath.clear();
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

std::string Theme::getBackgroundImage()
{
    return currentTheme ? currentTheme->backgroundImage : "";
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

static ThemeInfo inspectText(const std::string& text, const std::string& path)
{
    ThemeInfo info;
    info.path = path;
    std::string file = path.substr(path.find_last_of("/:") == std::string::npos ? 0 : path.find_last_of("/:") + 1);
    info.id   = endsWith(file, ".theme") ? file.substr(0, file.size() - 6) : file;

    ParsedTheme theme      = parseTheme(text, directoryOf(path));
    ResolvedTheme resolved = resolveTheme(theme);
    info.name        = theme.name.empty() ? info.id : localized(theme.name);
    info.author      = localized(theme.author);
    info.version     = theme.version;
    info.description = localized(theme.description);
    info.light       = theme.light;
    info.dark        = theme.dark;
    info.backgroundImage = theme.backgroundImage;
    info.counterpart = theme.counterpart;
    info.lightPreview = previewOf(Theme::getLightTheme().getValues()->getBaseColors(), resolved.overlay[0]);
    info.darkPreview  = previewOf(Theme::getDarkTheme().getValues()->getBaseColors(), resolved.overlay[1]);
    info.errors      = theme.errors;
    info.warnings    = resolved.warnings;

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

ThemeInfo Theme::inspect(const std::string& path)
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
    return inspectText(text, path);
}

static bool byName(const ThemeInfo& a, const ThemeInfo& b)
{
    return a.name < b.name;
}

std::vector<ThemeInfo> Theme::listThemes(const std::string& directory)
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
                    if (endsWith(name, ".theme") && !fs::is_directory(entry.path()))
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
        themes.push_back(Theme::inspect(path));
    std::sort(themes.begin(), themes.end(), byName);
    return themes;
}

std::vector<ThemeInfo> Theme::builtinThemes()
{
    std::vector<ThemeInfo> themes;
    for (const BuiltinTheme& theme : builtinThemeTexts())
        themes.push_back(inspectText(theme.text, std::string(BUILTIN_PREFIX) + theme.id));
    return themes;
}

} // namespace brls
