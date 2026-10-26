/*
    Copyright 2019-2020 natinusala
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

#pragma once

#include <nanovg.h>

#include <cstdint>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

namespace brls
{

enum class ThemeVariant
{
    LIGHT,
    DARK
};

class ThemeValues
{
  public:
    ThemeValues(std::initializer_list<std::pair<std::string, NVGcolor>> list);

    void addColor(const std::string&, NVGcolor color);
    NVGcolor getColor(const std::string&);
    bool hasColor(const std::string&) const;
    const std::unordered_map<std::string, NVGcolor>& getColors() const { return this->values; }

  private:
    std::unordered_map<std::string, NVGcolor> values;
};

/**
 * The colours views draw with, by key, for the light and the dark variant:
 * Application::getTheme() is the variant in use. Every colour of the core
 * views and of Cadran is a key, so changing a key restyles all of them.
 *
 * A view keeps a key rather than a colour with View::setThemeColor (XML:
 * textColor="@theme/brls/text"), and draws with its new value when the
 * theme changes, at once: Application::getThemeChangedEvent() says so to
 * what keeps colours of its own.
 */
class Theme
{
  public:
    Theme(ThemeValues* values);
    NVGcolor operator[](const std::string& name);

    void addColor(const std::string&, NVGcolor color);
    NVGcolor getColor(const std::string& name);
    bool hasColor(const std::string& name);
    ThemeValues* getValues() { return this->values; }

    static Theme& getLightTheme();
    static Theme& getDarkTheme();
    static Theme& get(ThemeVariant variant);

    /**
     * Reads a theme file into the light or the dark theme, to restyle an app
     * without rebuilding it:
     *
     *     # a comment
     *     inherit = dark                  # the theme it changes (dark by default)
     *     [colors]
     *     brls/accent = #2ec4b6           # #rgb, #rrggbb, #rrggbbaa,
     *     cadran/well = rgba(255, 255, 255, 0.08)   # rgb() or rgba()
     *     [metrics]
     *     brls/sidebar/width = 320
     *
     * Colours and metrics not named keep their values. Read again, a file
     * changes the theme again (the screens drawn afterwards use it).
     * Returns false when the file cannot be read; a line it cannot read is
     * logged and skipped. The variant it inherits is put in `variant` when
     * given, for Platform::setThemeVariant. Text that reads badly on its
     * background is logged as a warning (checkContrast). The screens take it
     * at their next frame.
     */
    static bool loadFile(const std::string& path, ThemeVariant* variant = nullptr);

    /**
     * As loadFile, from the file's text.
     */
    static bool load(const std::string& text, ThemeVariant* variant = nullptr);

    /**
     * Reads a colour as loadFile does ("#2ec4b6", "rgba(…)"); false if it is
     * not one.
     */
    static bool parseColor(const std::string& text, NVGcolor* color);

    /**
     * WCAG's contrast ratio of two opaque colours (1 to 21).
     */
    static float contrast(NVGcolor a, NVGcolor b);

    /**
     * A pair checkContrast() looks at: `ink` drawn on `surface` (keys), at
     * least `minimum`:1. A translucent colour is first blended over
     * brls/background. The core adds its own; Cadran its palette's.
     */
    static void addContrastCheck(const std::string& ink, const std::string& surface, float minimum = 4.5f);

    /**
     * The pairs of `variant` under their minimum, as "brls/text on
     * brls/background reads badly: 3.1:1, under 4.5:1".
     */
    static std::vector<std::string> checkContrast(ThemeVariant variant);

    /**
     * Changes each time a colour changes: Application compares it each
     * frame to tell the views.
     */
    static uint64_t getGeneration();

  private:
    ThemeValues* values;
};

} // namespace brls
