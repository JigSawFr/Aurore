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
#include <ctime>
#include <initializer_list>
#include <map>
#include <memory>
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

// How a theme's background_image fills the window (background_fit)
enum class BackgroundFit
{
    COVER, // scaled to cover it, centred, cut at the edges (the default)
    CONTAIN, // scaled to fit in it whole, centred, brls/clear around it
    TILE, // repeated from the top left, one pixel per unit of the layout
    CENTER, // as is, one pixel per unit of the layout, centred
};

// A variant's background, as a theme file sets it (docs/themes.md)
struct ThemeBackground
{
    static constexpr float DEFAULT_DIM = 0.35f;

    // background_image: its path (Theme::resolveAsset), "" without one
    std::string image;
    BackgroundFit fit = BackgroundFit::COVER;
    // background_dim: how much of brls/background veils the image (0 to 1)
    float dim = DEFAULT_DIM;
    // surface_opacity: the surfaces' opacity over the image (0 to 1), see
    // Theme::addSurfaceKey; 1 without an image
    float surfaceOpacity = 1.0f;

    bool hasImage() const { return !this->image.empty(); }
};

// A Cadran icon a theme draws as a picture (icon.<name>, docs/themes.md)
struct ThemeIcon
{
    std::string image; // its path (Theme::resolveAsset)
    // icon.<name>.tint: the picture is a mask, drawn in the icon's colour
    // (ThemeImageOptions::mask); else drawn as is
    bool tint = false;

    bool operator==(const ThemeIcon&) const = default;
};

// A variant's pictures besides its background, as a theme file sets them
// (docs/themes.md, "Logos and images"): paths made by Theme::resolveAsset
struct ThemePictures
{
    std::string logo; // logo: the app's mark, "" for the app's own
    std::string preview; // preview: the theme's thumbnail in a picker
    std::map<std::string, ThemeIcon> icons; // icon.<name>, by icon name
    std::map<std::string, std::string> images; // image.<key>, by image slot

    bool empty() const { return this->logo.empty() && this->preview.empty() && this->icons.empty() && this->images.empty(); }
    bool operator==(const ThemePictures&) const = default;
};

struct ThemeImage;

// ---------------------------------------------------------------------------
// Decorations (docs/themes.md, Decorations): pictures over the surfaces of
// a few areas, strips along the screen's edges, particles
// ---------------------------------------------------------------------------

// What a decoration picture is drawn in (View::setThemeArea)
enum class ThemeArea
{
    NONE,
    HEADER, // backdrop.header: a screen's header, its whole width
    SIDEBAR, // backdrop.sidebar: the sidebar (Cadran's rail)
    DIALOG, // backdrop.dialog: dialogs, dropdowns, Cadran's sheets
    TOP, // decor.top: a strip along the top edge of the screen
    BOTTOM, // decor.bottom: along its bottom edge
};
constexpr int THEME_AREA_COUNT = 5; // HEADER to BOTTOM

// A picture of the decorations, as a theme file sets it
struct ThemePicture
{
    std::string image; // its path (Theme::resolveAsset), "" without one
    BackgroundFit fit = BackgroundFit::COVER;
    float opacity     = 1.0f; // 0 to 1
    // decor.top_height, decor.bottom_height: the strip's height in layout
    // units; 0: the picture's at the screen's width
    float height = 0.0f;

    bool hasImage() const { return !this->image.empty(); }
};

enum class ParticleKind
{
    NONE,
    SNOW,
    LEAVES,
    PETALS,
    EMBERS,
    CONFETTI,
    SPARKLES,
    HEARTS,
    BUBBLES,
};

// particles_layer: behind the screens (under every view) or in front of
// them, fainter
enum class ParticleLayer
{
    BACK,
    FRONT,
};

struct ThemeParticles
{
    static constexpr float LOW = 0.3f, MEDIUM = 0.6f, HIGH = 1.0f;
    static constexpr int MAX_COLORS = 5;

    ParticleKind kind   = ParticleKind::NONE;
    float density       = LOW; // 0 to 1, of ThemeDecorations::MAX_PARTICLES
    float speed         = 1.0f; // 0.25 to 4
    ParticleLayer layer = ParticleLayer::BACK;
    // particles_color: up to MAX_COLORS; empty: from the palette
    std::vector<NVGcolor> colors;

    bool enabled() const { return this->kind != ParticleKind::NONE && this->density > 0.0f; }
};

// A variant's decorations: one picture per area, and particles
struct ThemeDecor
{
    ThemePicture pictures[THEME_AREA_COUNT]; // by ThemeArea, HEADER first
    ThemeParticles particles;

    const ThemePicture& picture(ThemeArea area) const { return this->pictures[(int)area - 1]; }
    bool hasPictures() const
    {
        for (const ThemePicture& picture : this->pictures)
            if (picture.hasImage())
                return true;
        return false;
    }
};

// A theme file as listThemes() or inspect() read it, without applying it.
struct ThemeInfo
{
    std::string path; // as given to loadFile(); "builtin:<id>" for a preset
    std::string id; // the file's name without its extension
    std::string name; // its `name`, else the id
    std::string author;
    std::string version;
    std::string description;
    // Its background_image: the dark variant's, else the light one's; ""
    // without one (each variant's below)
    std::string backgroundImage;
    // Its `icon`, the Cadran icon a picker shows it with ("snowflake"...),
    // "" without one
    std::string icon;

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

    // Each variant's background (image, fit, dim, surface opacity)
    ThemeBackground lightBackground;
    ThemeBackground darkBackground;
    const ThemeBackground& background(ThemeVariant variant) const
    {
        return variant == ThemeVariant::LIGHT ? this->lightBackground : this->darkBackground;
    }

    // Each variant's logo, icons, preview and image slots' pictures
    ThemePictures lightPictures;
    ThemePictures darkPictures;
    const ThemePictures& pictures(ThemeVariant variant) const
    {
        return variant == ThemeVariant::LIGHT ? this->lightPictures : this->darkPictures;
    }

    // Each variant's decorations (backdrops, strips, particles)
    ThemeDecor lightDecor;
    ThemeDecor darkDecor;
    const ThemeDecor& decor(ThemeVariant variant) const
    {
        return variant == ThemeVariant::LIGHT ? this->lightDecor : this->darkDecor;
    }

    // "line 4: #12 is not a colour"…: lines skipped, files not found.
    std::vector<std::string> errors;
    // Unknown keys and text that reads badly on its background. Text on a
    // background picture is checked only when the picture's colour is known:
    // from an explicit check (inspect or listThemes with decodePictures), or
    // once the picture was loaded in this run; a plain listing never decodes.
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
     * With "builtin:seasonal" in use, the preset of the day (by local date:
     * Halloween, Christmas, the seasons... docs/themes.md), used as soon as
     * the date enters another one. The main loop calls it at each
     * iteration; it looks at the wall clock once a minute (a console
     * resuming from sleep included).
     */
    static void followCalendar();

    /**
     * The preset "builtin:seasonal" uses at `when` (seconds since the
     * epoch, read in local time): "builtin:halloween"...
     */
    static std::string seasonalTheme(std::time_t when);

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
     * The picture the file in use puts behind every screen in the variant
     * in use (background_image = file.png, next to the theme file), ""
     * without one: drawn over brls/clear, under the screens.
     */
    static std::string getBackgroundImage();

    /**
     * The background of `variant` with the file in use: its image (none
     * without a file, or for a variant the file does not style), how it
     * fits, its veil, the surfaces' opacity.
     */
    static ThemeBackground getBackground(ThemeVariant variant);

    /**
     * The decorations of `variant` with the file in use (backdrop.*,
     * decor.*, particles*): none without a file, or for a variant the file
     * does not style. ThemeDecorations draws them.
     */
    static ThemeDecor getDecor(ThemeVariant variant);

    /**
     * A file a theme names (its background_image, and any picture a theme
     * key names), as a path: `name` is read from `directory`, the folder of
     * the theme file (or included file) that names it, and must stay in
     * `root`, the folder of the theme (a theme folder, or the folder of a
     * single .theme file). No absolute path, no "x:/", no ".." out of
     * `root`, no link out of it; "@res/..." only for a preset (`directory`
     * "builtin:") or a theme that is itself a resource. False, with the
     * reason in `error`, when it is refused. The one place theme files'
     * pictures are resolved.
     */
    static bool resolveAsset(const std::string& name, const std::string& directory, const std::string& root,
        std::string* path, std::string* error);

    /**
     * The logo, icons, preview and image slots' pictures of `variant` with
     * the file in use (none without a file, or for a variant it does not
     * style): paths, nothing loaded.
     */
    static ThemePictures getPictures(ThemeVariant variant);

    /**
     * The icons a theme may draw as pictures (`icon.<name> = file.png`): any
     * other name is a warning. Cadran adds its own (CadranIcon).
     */
    static void addIconNames(const std::vector<std::string>& names);

    /**
     * Declares an image slot: a picture of the app's that a theme may
     * replace (`image.<key> = file.png`), "playguard/loading/art". `fallback`
     * is the app's own picture ("@res/img/art.png", a file; "" for none),
     * `maxSide` the largest side it is made at (0: 1920 x 1080). Before the
     * theme file is loaded: a theme naming a slot not declared is warned
     * about. Declared again, the new fallback replaces the old one.
     */
    static void addImageSlot(const std::string& key, const std::string& fallback = "", int maxSide = 0);

    /**
     * The image slots declared, sorted.
     */
    static std::vector<std::string> getImageSlots();

    /**
     * The picture of the image slot `key` now (UI thread): the theme's for
     * the variant in use, else the slot's fallback (also when the theme's
     * cannot be read); nullptr for neither, or a slot not declared. Loaded
     * on the async thread (ThemeImages): it may still be LOADING, and
     * ThemeImages::getLoadedEvent() says when it is ready. Kept while the
     * theme and the variant stay; a view keeping it gets the new one on
     * Application::getThemeChangedEvent() (brls::Image::setThemeImage does).
     */
    static std::shared_ptr<ThemeImage> image(const std::string& key);

    /**
     * The theme's logo for the variant in use (UI thread), nullptr without
     * one: for an app that draws its logo itself (Cadran's mark draws it).
     * Made at most ThemeImages::LOGO_SIZE; ThemeImage::draw with CONTAIN.
     */
    static std::shared_ptr<ThemeImage> logo();

    /**
     * The theme's picture for the icon `name` in the variant in use (UI
     * thread), nullptr without one; `tint` says whether it is a mask, drawn
     * in the icon's colour. Made at most ThemeImages::ICON_SIZE.
     */
    static std::shared_ptr<ThemeImage> icon(const std::string& name, bool* tint = nullptr);

    /**
     * A key of a surface (a card, a panel) drawn over the screens'
     * background: with a background image and surface_opacity under 1, its
     * opacity is multiplied by it. The core's: brls/sidebar/background,
     * brls/highlight/background, the default buttons' backgrounds; Cadran
     * adds cadran/surf and cadran/surf2.
     */
    static void addSurfaceKey(const std::string& key);

    /**
     * Reads a theme file without using it: its metadata, a preview, what is
     * wrong with it. Its pictures are only looked for (an error when one is
     * not there), never decoded, unless `decodePictures`: then each is
     * decoded on the calling thread, to tell a broken file and to check the
     * text on it (for a check off the UI thread: --check-themes, a test).
     */
    static ThemeInfo inspect(const std::string& path, bool decodePictures = false);

    /**
     * Every *.theme of `directory` (a folder, or a resource folder such as
     * "@res/themes"), sorted by name: one entry per file, a broken one with
     * its errors (never skipped, never fatal). Empty when the folder does
     * not exist. From the main thread, as inspect(): the previews are made
     * from the themes in use. Cheap: colours only, no picture decoded unless
     * `decodePictures` (as inspect()).
     */
    static std::vector<ThemeInfo> listThemes(const std::string& directory, bool decodePictures = false);

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
     * brls/background: 3.1:1, under 4.5:1". With a background image whose
     * colour is known (ThemeImages), text on brls/background is checked on
     * the image's average colour under its veil, and translucent surfaces
     * over it.
     */
    static std::vector<std::string> checkContrast(ThemeVariant variant);

    /**
     * Only the pairs the background image changes (text on the screens'
     * background, translucent surfaces), on its average colour under its
     * veil; empty without an image, or while its colour is not known.
     */
    static std::vector<std::string> checkBackgroundContrast(ThemeVariant variant);

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
