/*
    Copyright 2019 natinusala
    Copyright 2019 p-sam

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

#include <borealis/core/logger.hpp>
#include <borealis/core/style.hpp>
#include <borealis/core/theme.hpp>
#include <borealis/core/util.hpp>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace brls
{

static ThemeValues lightThemeValues = {
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

};

static ThemeValues darkThemeValues = {
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
    { "brls/spinner/bar_color", nvgRGBA(192, 192, 192, 80) }, // TODO: get this right
};

ThemeValues::ThemeValues(std::initializer_list<std::pair<std::string, NVGcolor>> list)
{
    for (std::pair<std::string, NVGcolor> color : list)
        this->values.insert(color);
}

void ThemeValues::addColor(const std::string& name, NVGcolor color)
{
        this->values[name] = color;
}

NVGcolor ThemeValues::getColor(const std::string& name)
{
    if (this->values.count(name) == 0)
        fatal("Unknown theme value \"" + name + "\" in size: " + std::to_string(this->values.size()));

    return this->values[name];
}

bool ThemeValues::hasColor(const std::string& name) const
{
    return this->values.count(name) > 0;
}

Theme::Theme(ThemeValues* values)
    : values(values)
{
}

NVGcolor Theme::getColor(const std::string& name)
{
    return this->values->getColor(name);
}

void Theme::addColor(const std::string& name, NVGcolor color)
{
    return this->values->addColor(name, color);
}

NVGcolor Theme::operator[](const std::string& name)
{
    return this->getColor(name);
}

Theme& Theme::getLightTheme()
{
    static Theme lightTheme(&lightThemeValues);
    return lightTheme;
}

Theme& Theme::getDarkTheme()
{
    static Theme darkTheme(&darkThemeValues);
    return darkTheme;
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

// WCAG's relative luminance and contrast ratio
static float luminance(NVGcolor c)
{
    auto channel = [](float v) { return v <= 0.03928f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f); };
    return 0.2126f * channel(c.r) + 0.7152f * channel(c.g) + 0.0722f * channel(c.b);
}

static float contrast(NVGcolor a, NVGcolor b)
{
    float la = luminance(a), lb = luminance(b);
    return (std::max(la, lb) + 0.05f) / (std::min(la, lb) + 0.05f);
}

bool Theme::load(const std::string& text, ThemeVariant* variant)
{
    // Read whole before it is applied: `inherit` may come after colours
    ThemeVariant inherit = ThemeVariant::DARK;
    std::vector<std::pair<std::string, NVGcolor>> colors;
    std::vector<std::pair<std::string, float>> metrics;

    std::stringstream lines(text);
    std::string line, section;
    int number = 0;
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

        if (line.front() == '[' && line.back() == ']')
        {
            section = trim(line.substr(1, line.size() - 2));
            continue;
        }

        equals = line.find('=');
        if (equals == std::string::npos)
        {
            Logger::warning("theme: line {} skipped, no '=': {}", number, line);
            continue;
        }
        std::string key   = trim(line.substr(0, equals));
        std::string value = trim(line.substr(equals + 1));

        if (section.empty() && key == "inherit")
        {
            if (value == "light")
                inherit = ThemeVariant::LIGHT;
            else if (value == "dark")
                inherit = ThemeVariant::DARK;
            else
                Logger::warning("theme: line {}: inherit is light or dark, not {}", number, value);
        }
        else if (section == "colors")
        {
            NVGcolor color;
            if (parseColor(value, &color))
                colors.emplace_back(key, color);
            else
                Logger::warning("theme: line {}: {} is not a colour", number, value);
        }
        else if (section == "metrics")
        {
            char* end;
            float metric = std::strtof(value.c_str(), &end);
            if (!value.empty() && *end == '\0')
                metrics.emplace_back(key, metric);
            else
                Logger::warning("theme: line {}: {} is not a number", number, value);
        }
        else
        {
            Logger::warning("theme: line {} skipped: {}", number, line);
        }
    }

    Theme& theme = inherit == ThemeVariant::LIGHT ? getLightTheme() : getDarkTheme();
    for (auto& [name, color] : colors)
        theme.addColor(name, color);
    for (auto& [name, metric] : metrics)
        getStyle().addMetric(name, metric);

    // The pairs most text is drawn with
    const char* pairs[][2] = {
        { "brls/text", "brls/background" },
        { "brls/text", "brls/sidebar/background" },
        { "brls/button/primary_enabled_text", "brls/button/primary_enabled_background" },
    };
    for (auto& pair : pairs)
    {
        if (!theme.values->hasColor(pair[0]) || !theme.values->hasColor(pair[1]))
            continue;
        float ratio = contrast(theme.getColor(pair[0]), theme.getColor(pair[1]));
        if (ratio < 4.5f)
            Logger::warning("theme: {} on {} reads badly (contrast {:.1f}:1, at least 4.5:1)", pair[0], pair[1], ratio);
    }

    if (variant)
        *variant = inherit;
    Logger::info("theme: {} colours and {} metrics read into the {} theme", colors.size(), metrics.size(),
        inherit == ThemeVariant::LIGHT ? "light" : "dark");
    return true;
}

bool Theme::loadFile(const std::string& path, ThemeVariant* variant)
{
    std::ifstream file(path);
    if (!file)
    {
        Logger::error("theme: cannot read {}", path);
        return false;
    }
    std::stringstream text;
    text << file.rdbuf();
    return load(text.str(), variant);
}

} // namespace brls
