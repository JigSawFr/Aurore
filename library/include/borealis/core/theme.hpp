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

// One variant's colours by key ("brls/text", "cadran/teal"…): the ones the
// code adds (the base), and over them the ones of the theme file in use.
class ThemeValues
{
  public:
    ThemeValues(std::initializer_list<std::pair<std::string, NVGcolor>> list);

    // A colour of the base: what the variant is without a theme file.
    void addColor(const std::string&, NVGcolor color);
    NVGcolor getColor(const std::string&);
    bool hasColor(const std::string&) const;

    // The theme file's colours, over the base (empty: the base alone).
    void setOverlay(const std::unordered_map<std::string, NVGcolor>& overlay);

    // Every colour as drawn now, and the base alone.
    const std::unordered_map<std::string, NVGcolor>& getColors() const { return this->values; }
    const std::unordered_map<std::string, NVGcolor>& getBaseColors() const { return this->base; }

  private:
    std::unordered_map<std::string, NVGcolor> base;
    std::unordered_map<std::string, NVGcolor> overlay;
    std::unordered_map<std::string, NVGcolor> values; // base, then overlay
};

// A theme file as listThemes() or inspect() read it, without applying it.
struct ThemeInfo
{
    std::string path; // as given to loadFile(); "builtin:<id>" for a preset
    std::string id;   // the file's name without its extension
    std::string name; // its `name`, else the id
    std::string author;
    std::string version;
    std::string description;
    std::string backgroundImage; // its background_image, "" without one

    // For a file that styles one variant, the theme file of the other one
    // (`counterpart`, as given to loadFile; "" without one): what a picker
    // switches to when the other variant is asked for.
    std::string counterpart;

    // The variants it styles: one when it inherits light or dark, both when
    // it has [light] and [dark] (it then follows the system's mode).
    bool light = false;
    bool dark  = false;

    // A few colours to show it in a picker, per variant it styles.
    struct Preview
    {
        NVGcolor background;
        NVGcolor surface;
        NVGcolor text;
        NVGcolor accent;
        NVGcolor highlight;
    };
    Preview lightPreview {};
    Preview darkPreview {};

    // "line 4: #12 is not a colour"…: lines skipped, files not found.
    std::vector<std::string> errors;
    // Unknown keys and text that reads badly on its background.
    std::vector<std::string> warnings;

    bool ok() const { return this->errors.empty(); }
    // The variant to show it in when it styles one only.
    ThemeVariant variant() const { return this->light && !this->dark ? ThemeVariant::LIGHT : ThemeVariant::DARK; }
    bool styles(ThemeVariant variant) const { return variant == ThemeVariant::LIGHT ? this->light : this->dark; }
};

/**
 * The colours views draw with, by key, for the light and the dark variant:
 * Application::getTheme() is the variant in use. Every colour of the core
 * views and of Cadran is a key (docs/themes.md lists them), so a theme file
 * restyles all of them.
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

    // A colour of the base (see ThemeValues::addColor).
    void addColor(const std::string&, NVGcolor color);
    NVGcolor getColor(const std::string& name);
    bool hasColor(const std::string& name);
    ThemeValues* getValues() { return this->values; }

    static Theme& getLightTheme();
    static Theme& getDarkTheme();
    static Theme& get(ThemeVariant variant);

    /**
     * Uses a theme file (docs/themes.md), replacing the one used before:
     *
     *     name = Ocean
     *     inherit = dark          # light, dark, another file or builtin:<id>
     *     accent = #3fa7ff        # the palette: accent, background, surface,
     *                             # text, warning, danger; the rest derived
     *     [colors]                # any key, for the variants it styles
     *     brls/sidebar/background = #0d1b2a
     *     [light] / [dark]        # the same, for one variant only
     *     [metrics]
     *     brls/highlight/stroke_width = 4
     *
     * `path` is a file, a resource ("@res/themes/x.theme") or a preset
     * ("builtin:high-contrast-dark", builtinThemes()). A line it cannot read
     * is logged and skipped; false when the file cannot be read at all. The
     * variant to show it in is put in `variant` when given (the one it
     * styles, or the one in use when it styles both), for
     * Platform::setThemeVariant. Text that reads badly on its background is
     * logged as a warning (checkContrast). The screens take it at the next
     * iteration of the main loop, which draws them again (redraw on demand
     * or not).
     */
    static bool loadFile(const std::string& path, ThemeVariant* variant = nullptr);

    /**
     * As loadFile, from the file's text; `directory` is where its `@include`
     * and inherit files are looked for.
     */
    static bool load(const std::string& text, ThemeVariant* variant = nullptr, const std::string& directory = "");

    /**
     * Back to the look without a theme file.
     */
    static void reset();

    /**
     * The file in use ("" without one), as given to loadFile.
     */
    static const std::string& getCurrentPath();

    /**
     * Whether the file in use styles `variant` (true without one): a file
     * for one variant only leaves the other as it is without a file.
     */
    static bool styles(ThemeVariant variant);

    /**
     * The picture the file in use puts behind every screen
     * (background_image = file.png, next to the theme file), "" without one:
     * drawn over brls/clear, under the screens (a view with a background
     * of its own hides it).
     */
    static std::string getBackgroundImage();

    /**
     * Reads a theme file without using it: its metadata, a preview, what is
     * wrong with it.
     */
    static ThemeInfo inspect(const std::string& path);

    /**
     * Every *.theme of `directory` (a folder, or a resource folder such as
     * "@res/themes"), sorted by name: one entry per file, a broken one with
     * its errors (never skipped, never fatal). Empty when the folder does
     * not exist. From the main thread, as inspect(): the previews are made
     * from the themes in use.
     */
    static std::vector<ThemeInfo> listThemes(const std::string& directory);

    /**
     * The presets built into Aurore ("builtin:<id>" paths).
     */
    static std::vector<ThemeInfo> builtinThemes();

    /**
     * Reads a colour as theme files write it ("#2ec4b6", "#2ec4b680",
     * "rgba(…)"); false if it is not one.
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
     * brls/background: 3.1:1, under 4.5:1".
     */
    static std::vector<std::string> checkContrast(ThemeVariant variant);

    /**
     * Changes each time a colour or a metric changes (a theme file used, a
     * colour added): the main loop compares it at each iteration to tell
     * the views and draw them again.
     */
    static uint64_t getGeneration();

  private:
    ThemeValues* values;
};

} // namespace brls
