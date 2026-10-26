/*
    Copyright 2019-2020 natinusala
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

#pragma once

#include <nanovg.h>

#include <initializer_list>
#include <string>
#include <unordered_map>

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

  private:
    std::unordered_map<std::string, NVGcolor> values;
};

// Simple wrapper around ThemeValues for the array operator
class Theme
{
  public:
    Theme(ThemeValues* values);
    NVGcolor operator[](const std::string& name);

    void addColor(const std::string&, NVGcolor color);
    NVGcolor getColor(const std::string& name);

    static Theme& getLightTheme();
    static Theme& getDarkTheme();

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
     * given, for Platform::setThemeVariant. A text colour that reads badly on
     * its background (contrast under 4.5:1) is logged as a warning.
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

  private:
    ThemeValues* values;
};

} // namespace brls
