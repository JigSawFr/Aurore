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

// The demo under the test harness: ctest runs each case in a process of its
// own (docs/testing.md). The references are in tests/references/;
// BRLS_TEST_UPDATE=1 writes them again.

#include <borealis.hpp>
#include <borealis/cadran/ui/live_prefs.hpp>
#include <borealis/cadran/ui/prefs.hpp>
#include <borealis/cadran/ui/ui.hpp>
#include <borealis/core/theme_decor.hpp>
#include <borealis/core/theme_image.hpp>
#include <borealis/test/harness.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <thread>

#include "activity/main_activity.hpp"
#include "demo_app.hpp"

using brls::test::Harness;

namespace
{

// What the demo's main() does before Application::init()
struct Demo
{
    Demo() { demo::installHooks(); }
};

// The harness, on the demo's main screen
struct DemoHarness : Demo, Harness
{
    DemoHarness()
    {
        demo::registerViews();
        push(new MainActivity());
        settle();
    }
};

// A square of colour, changed without asking for a redraw unless told to
class Swatch : public brls::View
{
  public:
    Swatch()
    {
        setWidth(120);
        setHeight(80);
    }

    void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) override
    {
        nvgBeginPath(vg);
        nvgRect(vg, x, y, width, height);
        nvgFillColor(vg, color);
        nvgFill(vg);
    }

    NVGcolor color = nvgRGB(200, 40, 40);
};

brls::Box* screenOf(std::initializer_list<brls::View*> views)
{
    auto* box = new brls::Box(brls::Axis::COLUMN);
    box->setPadding(40, 40, 40, 40);
    box->setBackgroundColor(nvgRGB(20, 20, 30));
    for (brls::View* view : views)
        box->addView(view);
    return box;
}

} // namespace

BRLS_TEST(main_screen)
{
    DemoHarness h;
    BRLS_CHECK_EQ(h.focusText(), std::string("Cadran"));
    BRLS_CHECK(h.hasText("Aurore Demo"));
    BRLS_CHECK(h.hasText("Monday, January 5")); // the harness' wall clock
    BRLS_CHECK(h.matchesReference("main_screen"));
}

BRLS_TEST(rail_focus_moves)
{
    DemoHarness h;
    h.clearFocusLog();
    h.tap(brls::BUTTON_DOWN, 2);
    BRLS_CHECK_EQ(h.focusText(), std::string("Layout and alignment"));
    h.tap(brls::BUTTON_UP);
    BRLS_CHECK_EQ(h.focusText(), std::string("Basic components"));
    std::vector<std::string> expected = { "Basic components", "Layout and alignment", "Basic components" };
    BRLS_CHECK(h.focusLog() == expected);
    BRLS_CHECK(h.focusByText("Settings"));
    BRLS_CHECK(!h.focusByText("No such tab", brls::BUTTON_DOWN, 3));
}

BRLS_TEST(keyboard_moves_focus)
{
    DemoHarness h;
    h.tapKey(brls::BRLS_KBD_KEY_DOWN);
    BRLS_CHECK_EQ(h.focusText(), std::string("Basic components"));
    h.tapKey(brls::BRLS_KBD_KEY_UP);
    BRLS_CHECK_EQ(h.focusText(), std::string("Cadran"));
}

BRLS_TEST(confirm_dialog_opens_and_closes)
{
    DemoHarness h;
    h.tap(brls::BUTTON_RIGHT); // into the tab
    h.tap(brls::BUTTON_DOWN);
    BRLS_CHECK_EQ(h.focusText(), std::string("Confirm"));
    size_t screens = brls::Application::getActivitiesStack().size();

    h.tap(brls::BUTTON_A);
    BRLS_CHECK_EQ(brls::Application::getActivitiesStack().size(), screens + 1);
    BRLS_CHECK(dynamic_cast<brls::Dialog*>(h.top()->getContentView()) != nullptr);
    BRLS_CHECK(h.matchesReference("confirm_dialog"));

    h.tap(brls::BUTTON_B);
    BRLS_CHECK_EQ(brls::Application::getActivitiesStack().size(), screens);
    BRLS_CHECK_EQ(h.focusText(), std::string("Confirm"));
}

BRLS_TEST(touch_opens_a_tab)
{
    DemoHarness h;
    BRLS_CHECK(h.tapView(h.findText("Layout and alignment")));
    BRLS_CHECK(h.hasText("alignItems: center"));
    BRLS_CHECK(brls::Application::getInputType() == brls::InputType::TOUCH);
    BRLS_CHECK(h.matchesReference("layout_tab"));
}

BRLS_TEST(window_resized)
{
    DemoHarness h;
    h.resize(1920, 1080);
    brls::test::Image shot = h.screenshot();
    BRLS_CHECK_EQ(shot.width, 1920);
    BRLS_CHECK_EQ(shot.height, 1080);
    BRLS_CHECK_EQ(brls::Application::contentWidth, 1280.0f);
    BRLS_CHECK(h.matchesReference("main_screen_1080p"));
}

BRLS_TEST(cut_texts_found)
{
    Harness h;
    auto* cut = new brls::Label();
    cut->setText("A sentence far too long for the narrow box it is given");
    cut->setSingleLine(true);
    cut->setWidth(120);
    auto* whole = new brls::Label();
    whole->setText("Short");
    h.push(new brls::Activity(screenOf({ cut, whole })));
    h.settle();
    std::vector<std::string> expected = { "A sentence far too long for the narrow box it is given" };
    BRLS_CHECK(h.cutTexts() == expected);
}

BRLS_TEST(timers_follow_the_simulated_clock)
{
    Harness h;
    bool fired = false;
    brls::delay(500, [&fired]() { fired = true; });
    h.advance(400);
    BRLS_CHECK(!fired);
    h.advance(150);
    BRLS_CHECK(fired);

    // An animation ends after its duration on the clock, in frames
    brls::Animatable value;
    value.reset(0.0f);
    value.addStep(1.0f, 300);
    value.start();
    h.advance(150);
    BRLS_CHECK(value.getValue() > 0.0f && value.getValue() < 1.0f);
    BRLS_CHECK(h.settle());
    BRLS_CHECK_EQ(value.getValue(), 1.0f);
}

BRLS_TEST(redraw_on_demand_skips_still_frames)
{
    Harness h;
    auto* first  = new brls::Button();
    auto* second = new brls::Button();
    first->setText("First");
    second->setText("Second");
    h.push(new brls::Activity(screenOf({ first, second })));
    brls::Application::setRedrawOnDemand(true);
    h.settle();
    h.step(3);

    // Still: no frame drawn but the one a second
    uint64_t drawn = brls::Application::getDrawnFrameCount();
    h.advance(900);
    BRLS_CHECK(brls::Application::getDrawnFrameCount() - drawn <= 1);

    // A press: drawn
    h.tap(brls::BUTTON_DOWN);
    BRLS_CHECK(brls::Application::getDrawnFrameCount() - drawn > 2);
    BRLS_CHECK_EQ(h.focusText(), std::string("Second"));
}

BRLS_TEST(theme_change_draws_at_once_on_demand)
{
    Harness h;
    h.push(new brls::Activity(screenOf({ new brls::Button() })));
    brls::Application::setRedrawOnDemand(true);
    h.settle();
    h.step(3);

    // A theme loaded from code, nothing else asking for a frame: drawn at
    // the next iteration, not at the next periodic redraw
    uint64_t drawn = brls::Application::getDrawnFrameCount();
    brls::Theme::loadFile("builtin:high-contrast-dark");
    h.step();
    BRLS_CHECK(brls::Application::getDrawnFrameCount() > drawn);

    drawn = brls::Application::getDrawnFrameCount();
    brls::Theme::reset();
    h.step();
    BRLS_CHECK(brls::Application::getDrawnFrameCount() > drawn);
}

// Noon of a date, in the harness' time zone (UTC)
static std::time_t noonOf(int year, int month, int day)
{
    std::tm date {};
    date.tm_year = year - 1900;
    date.tm_mon  = month - 1;
    date.tm_mday = day;
    date.tm_hour = 12;
    return std::mktime(&date);
}

BRLS_TEST(seasonal_theme_picks_the_preset_of_the_date)
{
    Harness h;
    const struct
    {
        int year, month, day;
        const char* preset;
    } dates[] = {
        { 2026, 1, 5, "winter" },
        { 2026, 2, 14, "valentine" },
        { 2026, 2, 16, "lunar-new-year" }, // the eve of 17 February
        { 2026, 3, 3, "lunar-new-year" },  // the Lantern Festival
        { 2026, 3, 4, "winter" },
        { 2029, 2, 13, "valentine" }, // both: the first of the table
        { 2026, 3, 20, "spring" },
        { 2026, 7, 14, "summer" },
        { 2026, 10, 1, "autumn" },
        { 2026, 10, 31, "halloween" },
        { 2026, 11, 3, "autumn" },
        { 2026, 12, 24, "christmas" },
        { 2026, 12, 31, "new-year" },
        { 2027, 1, 3, "new-year" },
        { 2040, 2, 1, "winter" }, // out of the Lunar New Year's table
    };
    for (const auto& date : dates)
        BRLS_CHECK_EQ(brls::Theme::seasonalTheme(noonOf(date.year, date.month, date.day)),
            std::string("builtin:") + date.preset);

    bool listed = false;
    for (const brls::ThemeInfo& info : brls::Theme::builtinThemes())
        if (info.path == "builtin:seasonal")
            listed = info.ok() && info.warnings.empty() && info.light && info.dark && info.name == "Seasonal";
    BRLS_CHECK(listed);
}

BRLS_TEST(seasonal_theme_changes_at_midnight)
{
    // 16 October 2026, 15 seconds before Halloween's first day
    brls::test::Options options;
    options.wallTime = noonOf(2026, 10, 16) + 12 * 3600 - 15;
    Harness h(options);
    h.push(new brls::Activity(screenOf({ new brls::Button() })));
    brls::Theme::loadFile("builtin:seasonal");
    h.settle();
    NVGcolor autumn;
    brls::Theme::parseColor("#1a1310", &autumn);
    NVGcolor background = brls::Theme::getDarkTheme()["brls/background"];
    BRLS_CHECK(background.r == autumn.r && background.g == autumn.g && background.b == autumn.b);
    brls::test::Image before = h.screenshot();

    h.advance(20000);
    NVGcolor halloween;
    brls::Theme::parseColor("#150d1f", &halloween);
    background = brls::Theme::getDarkTheme()["brls/background"];
    BRLS_CHECK(background.r == halloween.r && background.g == halloween.g && background.b == halloween.b);
    BRLS_CHECK_EQ(brls::Theme::getCurrentPath(), std::string("builtin:seasonal"));
    // Drawn again: the button and its focus in the new colours
    BRLS_CHECK(brls::test::compareImages(h.screenshot(), before).differingPixels > 10000);
}

BRLS_TEST(every_preset_draws_the_demo_in_both_variants)
{
    DemoHarness h;
    for (const brls::ThemeInfo& info : brls::Theme::builtinThemes())
        for (brls::ThemeVariant variant : { brls::ThemeVariant::LIGHT, brls::ThemeVariant::DARK })
        {
            if (!info.styles(variant))
                continue;
            BRLS_CHECK(brls::Theme::loadFile(info.path));
            brls::Application::getPlatform()->setThemeVariant(variant);
            h.step(2);
            BRLS_CHECK(brls::Theme::checkContrast(variant).empty());
        }
    brls::Theme::reset();
    h.step(2);
}

BRLS_TEST(theme_picker_follows_a_one_variant_palette)
{
    DemoHarness h;
    ui::prefs().theme      = "dark";
    ui::prefs().theme_file = "builtin:high-contrast-light";
    live_prefs::apply_theme();
    h.settle();
    // The palette has the light variant only: shown, and what is saved
    BRLS_CHECK(brls::Application::getThemeVariant() == brls::ThemeVariant::LIGHT);
    BRLS_CHECK_EQ(ui::prefs().theme, std::string("light"));

    // Dark asked for: the palette's counterpart, in the dark variant
    ui::pick_theme();
    h.settle();
    BRLS_CHECK(h.focusByText("Dark"));
    h.tap(brls::BUTTON_A);
    h.settle();
    BRLS_CHECK_EQ(ui::prefs().theme, std::string("dark"));
    BRLS_CHECK_EQ(ui::prefs().theme_file, std::string("builtin:high-contrast-dark"));
    BRLS_CHECK(brls::Application::getThemeVariant() == brls::ThemeVariant::DARK);

    // A light-only palette picked: its variant
    ui::prefs().theme_file = "";
    live_prefs::apply_theme();
    ui::pick_palette();
    h.settle();
    BRLS_CHECK(h.focusByText("Sand"));
    h.tap(brls::BUTTON_A);
    h.settle();
    BRLS_CHECK_EQ(ui::prefs().theme, std::string("light"));
    BRLS_CHECK(brls::Application::getThemeVariant() == brls::ThemeVariant::LIGHT);

    // Dark again: Sand has no counterpart, back to the boards' palettes
    ui::pick_theme();
    h.settle();
    BRLS_CHECK(h.focusByText("Dark"));
    h.tap(brls::BUTTON_A);
    h.settle();
    BRLS_CHECK_EQ(ui::prefs().theme_file, std::string(""));
    BRLS_CHECK(brls::Application::getThemeVariant() == brls::ThemeVariant::DARK);
}

BRLS_TEST(theme_icon_is_read_and_inherited)
{
    Harness h;
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "aurore_theme_icon_test";
    fs::create_directories(dir);
    auto write = [&dir](const char* name, const char* text)
    {
        std::ofstream(dir / name) << text;
        return (dir / name).string();
    };
    write("motif.inc", "icon = gem\n");
    const std::string own       = write("own.theme", "name = Own\nicon = leaf\naccent = #3ddc97\n");
    const std::string none      = write("none.theme", "name = None\naccent = #3ddc97\n");
    const std::string inherits  = write("inherits.theme", "name = Inherits\ninherit = builtin:ocean\n");
    const std::string overrides = write("overrides.theme", "inherit = own.theme\nicon = tree\n");
    const std::string includes  = write("includes.theme", "@include motif.inc\naccent = #3ddc97\n");

    BRLS_CHECK_EQ(brls::Theme::inspect(own).icon, std::string("leaf"));
    BRLS_CHECK(brls::Theme::inspect(own).warnings.empty() && brls::Theme::inspect(own).ok());
    BRLS_CHECK_EQ(brls::Theme::inspect(none).icon, std::string(""));
    BRLS_CHECK_EQ(brls::Theme::inspect(inherits).icon, std::string("wave"));
    BRLS_CHECK_EQ(brls::Theme::inspect(overrides).icon, std::string("tree"));
    BRLS_CHECK_EQ(brls::Theme::inspect(includes).icon, std::string("gem"));
    fs::remove_all(dir);

    // Every preset has its motif
    for (const brls::ThemeInfo& info : brls::Theme::builtinThemes())
        BRLS_CHECK(!info.icon.empty());
}

BRLS_TEST(seasonal_palette_shows_the_motif_of_the_date)
{
    Harness h;
    const struct
    {
        int month, day;
        const char* icon;
    } dates[] = {
        { 1, 20, "snowflake" }, { 2, 12, "heart" }, { 2, 17, "lantern" }, { 4, 10, "blossom" },
        { 7, 14, "sun" }, { 10, 1, "leaf" }, { 10, 31, "pumpkin" }, { 12, 24, "tree" }, { 12, 31, "firework" },
    };
    for (const auto& date : dates)
    {
        brls::setSimulatedWallTime(noonOf(2026, date.month, date.day));
        std::string icon;
        for (const brls::ThemeInfo& info : brls::Theme::builtinThemes())
            if (info.path == "builtin:seasonal")
                icon = info.icon;
        BRLS_CHECK_EQ(icon, std::string(date.icon));
    }
}

// The palette picker, a swatch of its colours in each row's icon place
static void pickPalette(const char* theme, const char* reference)
{
    DemoHarness h;
    ui::prefs().theme = theme;
    live_prefs::apply_theme();
    h.settle();
    ui::pick_palette();
    h.settle();
    BRLS_CHECK(h.hasText("Ocean"));
    BRLS_CHECK(h.matchesReference(reference));
}

BRLS_TEST(palette_picker_shows_swatches)
{
    pickPalette("dark", "palette_picker");
}

BRLS_TEST(palette_picker_shows_swatches_light)
{
    pickPalette("light", "palette_picker_light");
}

BRLS_TEST(redraw_debug_finds_a_missing_invalidation)
{
    Harness h;
    auto* swatch = new Swatch();
    h.push(new brls::Activity(screenOf({ swatch })));
    brls::Application::setRedrawOnDemand(true);
    brls::Application::setRedrawDebug(true);
    h.settle();
    h.step(3);
    BRLS_CHECK_EQ(brls::Application::getMissedRedrawCount(), (uint64_t)0);

    // Changed, and said so: drawn, nothing missed
    swatch->color = nvgRGB(40, 200, 40);
    brls::Application::requestRedraw();
    h.step(3);
    BRLS_CHECK_EQ(brls::Application::getMissedRedrawCount(), (uint64_t)0);

    // Changed without a word: the skipped frames differ, over the swatch
    swatch->color = nvgRGB(40, 40, 200);
    h.step(3);
    BRLS_CHECK(brls::Application::getMissedRedrawCount() > 0);
    brls::Rect area  = brls::Application::getLastMissedRedrawArea();
    brls::Rect frame = swatch->getFrame();
    BRLS_CHECK(std::fabs(area.getMinX() - frame.getMinX()) <= 1 && std::fabs(area.getMinY() - frame.getMinY()) <= 1);
    BRLS_CHECK(std::fabs(area.getWidth() - frame.getWidth()) <= 2 && std::fabs(area.getHeight() - frame.getHeight()) <= 2);
}

// A folder of theme files and pictures, removed afterwards
struct ThemeFolder
{
    std::filesystem::path dir;

    explicit ThemeFolder(const char* name)
        : dir(std::filesystem::temp_directory_path() / name)
    {
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);
    }
    ~ThemeFolder() { std::filesystem::remove_all(dir); }

    std::string write(const std::string& name, const std::string& text)
    {
        std::filesystem::create_directories((dir / name).parent_path());
        std::ofstream(dir / name) << text;
        return (dir / name).generic_string();
    }

    // A picture of one colour
    std::string picture(const std::string& name, unsigned char r, unsigned char g, unsigned char b, int width = 64,
        int height = 36)
    {
        std::filesystem::create_directories((dir / name).parent_path());
        brls::test::Image image;
        image.width  = width;
        image.height = height;
        for (int i = 0; i < width * height; i++)
            image.rgba.insert(image.rgba.end(), { r, g, b, 255 });
        image.savePng((dir / name).string());
        return (dir / name).generic_string();
    }
};

static bool hasErrorWith(const brls::ThemeInfo& info, const std::string& text)
{
    for (const std::string& error : info.errors)
        if (error.find(text) != std::string::npos)
            return true;
    return false;
}

BRLS_TEST(theme_pictures_stay_in_their_folder)
{
    Harness h;
    ThemeFolder themes("aurore_theme_assets_test");
    const std::string root = themes.dir.generic_string();
    themes.picture("night.png", 10, 20, 40);
    themes.picture("aurora/sky/night.png", 10, 20, 40);
    themes.picture("outside.png", 10, 20, 40);
    themes.write("single.theme", "name = Single\nbackground_image = night.png\n");
    themes.write("aurora/theme.theme", "name = Aurora\n@include parts/sky.inc\n");
    themes.write("aurora/parts/sky.inc", "background_image = ../sky/night.png\n");
    themes.write("aurora/notes.txt", "not a theme\n");
    themes.write("empty/readme.txt", "a folder without theme.theme\n");

    // Folder themes are listed with the single files, named after their folder
    std::vector<brls::ThemeInfo> listed = brls::Theme::listThemes(root);
    BRLS_CHECK_EQ(listed.size(), (size_t)2);
    for (const brls::ThemeInfo& info : listed)
        BRLS_CHECK(info.ok());
    BRLS_CHECK_EQ(listed[0].id, std::string("aurora"));
    BRLS_CHECK_EQ(listed[0].path, root + "/aurora/theme.theme");
    BRLS_CHECK_EQ(listed[0].darkBackground.image, root + "/aurora/sky/night.png");
    BRLS_CHECK_EQ(listed[1].id, std::string("single"));
    BRLS_CHECK_EQ(listed[1].backgroundImage, root + "/night.png");

    // Out of the theme's folder: refused, an error, never a crash
    std::string path, error;
    const std::string folder = root + "/aurora";
    for (const char* name : { "../outside.png", "sky/../../outside.png", "/etc/passwd", "sdmc:/x.png", "C:/x.png",
             "@res/img/demo_icon.jpg", "sky\\night.png", "..", "." })
    {
        BRLS_CHECK(!brls::Theme::resolveAsset(name, folder, folder, &path, &error));
        BRLS_CHECK(!error.empty());
    }
    BRLS_CHECK(brls::Theme::resolveAsset("./sky/x/../night.png", folder, folder, &path, &error));
    BRLS_CHECK_EQ(path, folder + "/sky/night.png");
    // From an included file in a subfolder, still in the theme's folder
    BRLS_CHECK(brls::Theme::resolveAsset("../sky/night.png", folder + "/parts", folder, &path, &error));
    BRLS_CHECK_EQ(path, folder + "/sky/night.png");
    // A preset's pictures are the app's resources
    BRLS_CHECK(brls::Theme::resolveAsset("@res/img/demo_icon.jpg", "builtin:", "builtin:", &path, &error));
    BRLS_CHECK(!brls::Theme::resolveAsset("night.png", "builtin:", "builtin:", &path, &error));

    const std::string escapes = themes.write("aurora2/theme.theme",
        "name = Escapes\nbackground_image = ../outside.png\n[light]\nbackground_image = /etc/passwd\n");
    brls::ThemeInfo info = brls::Theme::inspect(escapes);
    BRLS_CHECK(hasErrorWith(info, "goes out of the theme's folder"));
    BRLS_CHECK(hasErrorWith(info, "an absolute path is not allowed"));
    BRLS_CHECK(!info.darkBackground.hasImage() && !info.lightBackground.hasImage());

#ifndef _WIN32
    // A link in the folder to a file out of it
    std::filesystem::create_symlink(themes.dir / "outside.png", themes.dir / "aurora" / "link.png");
    BRLS_CHECK(!brls::Theme::resolveAsset("link.png", folder, folder, &path, &error));
    BRLS_CHECK(error.find("link") != std::string::npos);
#endif

    // A picture that is not there, or not a picture
    themes.write("broken.jpg", "not a jpeg");
    BRLS_CHECK(hasErrorWith(brls::Theme::inspect(themes.write("missing.theme", "background_image = nowhere.png\n")),
        "cannot read"));
    // A file that is not a picture: told by a check that decodes it only
    const std::string broken = themes.write("broken.theme", "background_image = broken.jpg\n");
    BRLS_CHECK(brls::Theme::inspect(broken).ok());
    BRLS_CHECK(hasErrorWith(brls::Theme::inspect(broken, true), "cannot read"));
    // Used anyway: nothing drawn, no crash
    BRLS_CHECK(brls::Theme::loadFile(root + "/broken.theme"));
    h.settle();
    BRLS_CHECK(!brls::Application::hasThemeBackground());
    brls::Theme::reset();
}

BRLS_TEST(theme_background_keys_are_read_and_inherited)
{
    Harness h;
    ThemeFolder themes("aurore_theme_background_test");
    themes.picture("a.png", 10, 20, 40);
    themes.picture("b.png", 230, 235, 240);
    const std::string base = themes.write("base.theme",
        "name = Base\nbackground_image = a.png\nbackground_fit = tile\nbackground_dim = 0.2\nsurface_opacity = 0.5\n"
        "[light]\nbackground = #f0f0f0\n[dark]\nbackground = #101010\nbackground_dim = 0.6\n");
    const std::string child = themes.write("child.theme",
        "inherit = base.theme\n[light]\nbackground_image = b.png\nbackground_fit = contain\n[dark]\nsurface_opacity = 0.8\n");
    const std::string plain = themes.write("plain.theme", "name = Plain\nsurface_opacity = 0.5\naccent = #3ddc97\n");
    const std::string none  = themes.write("none.theme", "inherit = base.theme\n[light]\nbackground_image = none\n");
    const std::string wrong = themes.write("wrong.theme",
        "background_image = a.png\nbackground_fit = stretch\nbackground_dim = 2\nsurface_opacity = half\n");

    brls::ThemeInfo info = brls::Theme::inspect(base);
    BRLS_CHECK(info.ok());
    BRLS_CHECK(info.lightBackground.image == info.darkBackground.image && info.lightBackground.hasImage());
    BRLS_CHECK(info.lightBackground.fit == brls::BackgroundFit::TILE);
    BRLS_CHECK_EQ(info.lightBackground.dim, 0.2f);
    BRLS_CHECK_EQ(info.darkBackground.dim, 0.6f); // its own variant's line wins
    BRLS_CHECK_EQ(info.darkBackground.surfaceOpacity, 0.5f);

    info = brls::Theme::inspect(child);
    BRLS_CHECK(info.ok());
    BRLS_CHECK(info.lightBackground.image.find("b.png") != std::string::npos);
    BRLS_CHECK(info.darkBackground.image.find("a.png") != std::string::npos);
    BRLS_CHECK(info.lightBackground.fit == brls::BackgroundFit::CONTAIN);
    BRLS_CHECK(info.darkBackground.fit == brls::BackgroundFit::TILE);
    BRLS_CHECK_EQ(info.darkBackground.dim, 0.6f);
    BRLS_CHECK_EQ(info.lightBackground.surfaceOpacity, 0.5f);
    BRLS_CHECK_EQ(info.darkBackground.surfaceOpacity, 0.8f);

    info = brls::Theme::inspect(none);
    BRLS_CHECK(!info.lightBackground.hasImage() && info.darkBackground.hasImage());
    BRLS_CHECK_EQ(info.lightBackground.surfaceOpacity, 1.0f); // nothing to show through

    // The defaults, and surface_opacity without a picture: no effect
    info = brls::Theme::inspect(plain);
    BRLS_CHECK(info.ok() && !info.darkBackground.hasImage());
    BRLS_CHECK(info.darkBackground.fit == brls::BackgroundFit::COVER);
    BRLS_CHECK_EQ(info.darkBackground.dim, brls::ThemeBackground::DEFAULT_DIM);
    BRLS_CHECK_EQ(info.darkBackground.surfaceOpacity, 1.0f);

    info = brls::Theme::inspect(wrong);
    BRLS_CHECK_EQ(info.errors.size(), (size_t)3);
    BRLS_CHECK(info.darkBackground.fit == brls::BackgroundFit::COVER);

    // Used: the surfaces let the picture through, and are opaque again after
    NVGcolor surface = brls::Theme::getDarkTheme()["cadran/surf"];
    BRLS_CHECK(brls::Theme::loadFile(child));
    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);
    h.settle();
    BRLS_CHECK(brls::Application::hasThemeBackground());
    BRLS_CHECK(std::fabs(brls::Theme::getDarkTheme()["cadran/surf"].a - surface.a * 0.8f) < 0.001f);
    BRLS_CHECK(std::fabs(brls::Theme::getDarkTheme()["brls/sidebar/background"].a - 0.8f) < 0.001f);
    BRLS_CHECK(brls::Theme::getDarkTheme()["brls/background"].a == 1.0f);
    BRLS_CHECK(brls::Theme::loadFile(plain));
    h.settle();
    BRLS_CHECK(!brls::Application::hasThemeBackground());
    BRLS_CHECK_EQ(brls::Theme::getDarkTheme()["cadran/surf"].a, 1.0f);
    brls::Theme::reset();
    h.settle();
    BRLS_CHECK_EQ(brls::Theme::getDarkTheme()["cadran/surf"].a, surface.a);
}

BRLS_TEST(theme_text_is_checked_on_its_picture)
{
    Harness h;
    ThemeFolder themes("aurore_theme_contrast_test");
    themes.picture("snow.png", 250, 250, 250);
    themes.picture("night.png", 8, 10, 20);
    // White text on a dark theme: fine on its colour, not on a white picture
    // with no veil
    const char* dark = "inherit = dark\nbackground = #101418\ntext = #ffffff\nbackground_dim = 0\n";
    const std::string snow  = themes.write("snow.theme", std::string(dark) + "background_image = snow.png\n");
    const std::string night = themes.write("night.theme", std::string(dark) + "background_image = night.png\n");

    auto onPicture = [](const brls::ThemeInfo& info)
    {
        for (const std::string& warning : info.warnings)
            if (warning.find("brls/text on brls/background over the background image") != std::string::npos)
                return true;
        return false;
    };
    // A plain listing decodes nothing: no check on the picture yet
    NVGcolor mean;
    std::vector<brls::ThemeInfo> listed = brls::Theme::listThemes(themes.dir.generic_string());
    BRLS_CHECK_EQ(listed.size(), (size_t)2);
    for (const brls::ThemeInfo& info : listed)
        BRLS_CHECK(info.ok() && !onPicture(info));
    BRLS_CHECK(!brls::ThemeImages::knownMeanColor(listed[0].backgroundImage, &mean));

    // An explicit check decodes them
    BRLS_CHECK(onPicture(brls::Theme::inspect(snow, true)));
    BRLS_CHECK(!onPicture(brls::Theme::inspect(night, true)));
    // ...and remembers their colour, for the next listing
    BRLS_CHECK(onPicture(brls::Theme::inspect(snow)));

    // A veil of the background colour makes it read again
    const std::string veiled = themes.write("veiled.theme", std::string(dark) + "background_image = snow.png\nbackground_dim = 0.9\n");
    BRLS_CHECK(!onPicture(brls::Theme::inspect(veiled, true)));

    // Used, the check runs once the picture is loaded (decoded off the UI
    // thread, its colour recorded then)
    ThemeFolder fresh("aurore_theme_contrast_load_test");
    fresh.picture("sky.png", 250, 250, 250);
    const std::string loaded = fresh.write("sky.theme", std::string(dark) + "background_image = sky.png\n");
    BRLS_CHECK(!onPicture(brls::Theme::inspect(loaded)));
    BRLS_CHECK(brls::Theme::loadFile(loaded));
    h.settle();
    BRLS_CHECK(!brls::Theme::checkBackgroundContrast(brls::ThemeVariant::DARK).empty());
    BRLS_CHECK(onPicture(brls::Theme::inspect(loaded)));
    brls::Theme::reset();
    h.settle();
    BRLS_CHECK(brls::Theme::loadFile(snow));
    h.settle();
    BRLS_CHECK(!brls::Theme::checkBackgroundContrast(brls::ThemeVariant::DARK).empty());
    brls::Theme::reset();
    h.settle();
    BRLS_CHECK(brls::Theme::checkBackgroundContrast(brls::ThemeVariant::DARK).empty());
}

// The demo's Aurora theme (a folder: resources/themes/aurora), its picture
// behind the main screen
static void auroraScreen(brls::ThemeVariant variant, const char* reference)
{
    DemoHarness h;
    // What does not move (the focus ring pulses, the glow drifts): the last
    // card and the header's right end
    auto still = [&h]() { return h.screenshot().crop(1000, 0, 280, 430); };
    brls::test::Image before = still();
    auto colors              = brls::Theme::getDarkTheme().getValues()->getColors();

    BRLS_CHECK(brls::Theme::loadFile("@res/themes/aurora/theme.theme"));
    brls::Application::getPlatform()->setThemeVariant(variant);
    h.step(); // the picture asked for: decoded, as many frames on any machine
    while (brls::ThemeImages::decoding() > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    h.settle();
    BRLS_CHECK(brls::Application::hasThemeBackground());
    BRLS_CHECK(brls::Theme::checkContrast(variant).empty());
    BRLS_CHECK(h.matchesReference(reference));

    // Back to no theme, live: the screen as it was, pixel for pixel
    brls::Theme::reset();
    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);
    h.settle();
    BRLS_CHECK(!brls::Application::hasThemeBackground());
    auto diff = brls::test::compareImages(still(), before, 0);
    BRLS_CHECK(diff.sameSize && diff.differingPixels == 0);
    auto again = brls::Theme::getDarkTheme().getValues()->getColors();
    BRLS_CHECK_EQ(again.size(), colors.size());
    for (auto& [key, color] : colors)
        BRLS_CHECK(again.count(key) && std::memcmp(&again[key], &color, sizeof(NVGcolor)) == 0);
}

BRLS_TEST(theme_background_dark)
{
    auroraScreen(brls::ThemeVariant::DARK, "theme_background_dark");
}

BRLS_TEST(theme_background_light)
{
    auroraScreen(brls::ThemeVariant::LIGHT, "theme_background_light");
}

// Pictures still loading when the theme changes again: dropped, never
// drawn late, never freed twice (run under the sanitizers too)
BRLS_TEST(theme_background_switches_back_and_forth)
{
    DemoHarness h;
    const char* themes[] = { "@res/themes/aurora/theme.theme", "builtin:ocean", "@res/themes/aurora/theme.theme", "" };
    for (int round = 0; round < 12; round++)
    {
        const char* theme = themes[round % 4];
        if (*theme)
            BRLS_CHECK(brls::Theme::loadFile(theme));
        else
            brls::Theme::reset();
        brls::Application::getPlatform()->setThemeVariant(round % 3 ? brls::ThemeVariant::DARK : brls::ThemeVariant::LIGHT);
        h.step(round % 2 ? 1 : 2);
    }
    BRLS_CHECK(brls::Theme::loadFile("@res/themes/aurora/theme.theme"));
    h.settle();
    BRLS_CHECK(brls::Application::hasThemeBackground());
    brls::Theme::reset();
    h.settle();
    BRLS_CHECK(!brls::Application::hasThemeBackground());
}

// ---------------------------------------------------------------------------
// Decorations: backdrops, strips, particles
// ---------------------------------------------------------------------------

BRLS_TEST(theme_decor_keys_are_read_and_inherited)
{
    Harness h;
    ThemeFolder themes("aurore_theme_decor_test");
    themes.picture("garland.png", 200, 30, 40, 128, 16);
    themes.picture("frost.png", 220, 230, 240);
    themes.picture("outside.png", 1, 2, 3);
    const std::string base = themes.write("base/theme.theme",
        "name = Base\ninherit = dark\ndecor.top = garland.png\ndecor.top_fit = tile\ndecor.top_height = 24\nbackdrop.sidebar = frost.png\n"
        "backdrop.sidebar_opacity = 0.5\nparticles = snow\nparticles_density = medium\nparticles_speed = 0.5\n"
        "particles_color = #ffffff, rgba(200, 220, 255, 0.8) # a comment\n"
        "[colors]\nbrls/backdrop = rgba(0, 0, 0, 0.8) # a comment\n[dark]\nparticles_color = #ffffff, #ddeeff, #c0d0e0\n"
        "[light]\nparticles = petals\nbackdrop.dialog = frost.png\nbackdrop.dialog_fit = center\n");
    themes.picture("base/garland.png", 200, 30, 40, 128, 16);
    themes.picture("base/frost.png", 220, 230, 240);
    const std::string child = themes.write("base/child.theme",
        "inherit = theme.theme\nparticles_layer = front\n[dark]\ndecor.top = none\nparticles_density = high\n");
    const std::string wrong = themes.write("base/wrong.theme",
        "decor.top = ../outside.png\nbackdrop.header = /etc/passwd\nbackdrop.nowhere = frost.png\nparticles = rain\n"
        "particles_density = lots\nparticles_speed = 9\nparticles_layer = middle\nparticles_color = #12\n"
        "decor.top_height = -3\nbackdrop.header_height = 20\nbackdrop.dialog_opacity = 2\ndecor.side = frost.png\n");
    const std::string plain = themes.write("plain.theme", "name = Plain\naccent = #3ddc97\n");

    brls::ThemeInfo info = brls::Theme::inspect(base);
    BRLS_CHECK(info.ok() && info.warnings.empty());
    const brls::ThemePicture& top = info.darkDecor.picture(brls::ThemeArea::TOP);
    BRLS_CHECK(top.image.find("base/garland.png") != std::string::npos);
    BRLS_CHECK(top.fit == brls::BackgroundFit::TILE);
    BRLS_CHECK_EQ(top.height, 24.0f);
    BRLS_CHECK_EQ(info.darkDecor.picture(brls::ThemeArea::SIDEBAR).opacity, 0.5f);
    BRLS_CHECK(!info.darkDecor.picture(brls::ThemeArea::DIALOG).hasImage());
    BRLS_CHECK(info.lightDecor.picture(brls::ThemeArea::DIALOG).fit == brls::BackgroundFit::CENTER);
    BRLS_CHECK(info.darkDecor.particles.kind == brls::ParticleKind::SNOW);
    BRLS_CHECK(info.lightDecor.particles.kind == brls::ParticleKind::PETALS); // its own variant's line
    BRLS_CHECK_EQ(info.darkDecor.particles.density, brls::ThemeParticles::MEDIUM);
    BRLS_CHECK_EQ(info.darkDecor.particles.speed, 0.5f);
    BRLS_CHECK_EQ(info.darkDecor.particles.colors.size(), (size_t)3); // its own variant's
    BRLS_CHECK_EQ(info.lightDecor.particles.colors.size(), (size_t)2);
    BRLS_CHECK(std::fabs(info.lightDecor.particles.colors[1].a - 0.8f) < 0.01f);
    BRLS_CHECK(info.darkDecor.particles.layer == brls::ParticleLayer::BACK);

    // Inherited, and taken away
    info = brls::Theme::inspect(child);
    BRLS_CHECK(info.ok());
    BRLS_CHECK(!info.darkDecor.picture(brls::ThemeArea::TOP).hasImage());
    BRLS_CHECK(info.lightDecor.picture(brls::ThemeArea::TOP).hasImage());
    BRLS_CHECK(info.lightDecor.picture(brls::ThemeArea::SIDEBAR).hasImage());
    BRLS_CHECK_EQ(info.darkDecor.particles.density, brls::ThemeParticles::HIGH);
    BRLS_CHECK_EQ(info.lightDecor.particles.density, brls::ThemeParticles::MEDIUM);
    BRLS_CHECK(info.lightDecor.particles.layer == brls::ParticleLayer::FRONT);

    // Every wrong line an error, none of them a decoration; out of the
    // folder refused as the background's
    info = brls::Theme::inspect(wrong);
    BRLS_CHECK_EQ(info.errors.size(), (size_t)12);
    BRLS_CHECK(hasErrorWith(info, "goes out of the theme's folder"));
    BRLS_CHECK(hasErrorWith(info, "an absolute path is not allowed"));
    BRLS_CHECK(!info.darkDecor.hasPictures() && !info.darkDecor.particles.enabled());

    // None by default
    info = brls::Theme::inspect(plain);
    BRLS_CHECK(!info.darkDecor.hasPictures() && !info.lightDecor.hasPictures());
    BRLS_CHECK(info.darkDecor.particles.kind == brls::ParticleKind::NONE);

    // A picture that is not there
    BRLS_CHECK(hasErrorWith(brls::Theme::inspect(themes.write("base/missing.theme", "backdrop.header = nowhere.png\n")),
        "backdrop.header: cannot read"));

    // Used, and gone with the theme
    BRLS_CHECK(brls::Theme::loadFile(base));
    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);
    h.step();
    while (brls::ThemeImages::decoding() > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    h.settle();
    BRLS_CHECK(brls::ThemeDecorations::hasPicture(brls::ThemeArea::TOP));
    BRLS_CHECK(brls::ThemeDecorations::hasPicture(brls::ThemeArea::SIDEBAR));
    BRLS_CHECK(!brls::ThemeDecorations::hasPicture(brls::ThemeArea::HEADER));
    BRLS_CHECK(brls::ThemeDecorations::particleCount() > 0);
    brls::Theme::reset();
    h.settle();
    BRLS_CHECK(!brls::ThemeDecorations::hasPicture(brls::ThemeArea::TOP));
    BRLS_CHECK_EQ(brls::ThemeDecorations::particleCount(), 0);
}

// A theme of particles only, over an empty screen
static void useParticles(const std::string& preset, const std::string& kind, const std::string& more = "")
{
    BRLS_CHECK(brls::Theme::load("inherit = builtin:" + preset + "\nparticles = " + kind + "\n" + more));
}

BRLS_TEST(theme_particles_follow_motion_and_input)
{
    Harness h;
    // Nothing that moves of its own (a focus pulses)
    auto* label = new brls::Label();
    label->setText("Still");
    auto* screen = new brls::Box(brls::Axis::COLUMN); // no fill: the particles show
    screen->addView(label);
    h.push(new brls::Activity(screen));
    brls::Application::setRedrawOnDemand(true);
    useParticles("winter", "snow");
    h.settle();
    BRLS_CHECK(brls::ThemeDecorations::particleCount() > 0);
    BRLS_CHECK(brls::ThemeDecorations::particlesRunning());

    // Moving: about PARTICLE_FPS frames a second, not every frame
    uint64_t drawn = brls::Application::getDrawnFrameCount();
    h.advance(1000);
    uint64_t frames = brls::Application::getDrawnFrameCount() - drawn;
    BRLS_CHECK(frames >= 25 && frames <= 35);

    // Reduced motion: nothing drawn, no frame asked for but the one a second
    brls::Application::setReduceMotion(true);
    h.step(3);
    BRLS_CHECK(!brls::ThemeDecorations::particlesRunning());
    brls::test::Image still = h.screenshot();
    drawn = brls::Application::getDrawnFrameCount();
    h.advance(900);
    BRLS_CHECK(brls::Application::getDrawnFrameCount() - drawn <= 1);
    // ...and not a frozen frame of them: the screen is the one without
    brls::Theme::load("inherit = builtin:winter\n");
    h.settle();
    BRLS_CHECK_EQ(brls::test::compareImages(h.screenshot(), still, 0).differingPixels, (size_t)0);
    useParticles("winter", "snow");
    h.settle();
    BRLS_CHECK_EQ(brls::test::compareImages(h.screenshot(), still, 0).differingPixels, (size_t)0);

    // Motion again: they come back
    brls::Application::setReduceMotion(false);
    h.advance(2000);
    BRLS_CHECK(brls::ThemeDecorations::particlesRunning());
    BRLS_CHECK(brls::test::compareImages(h.screenshot(), still, 0).differingPixels > 50);

    // Idle a minute: they fade out and stop asking for frames
    h.advance(brls::ThemeDecorations::IDLE_SECONDS * 1000.0 + 4000);
    BRLS_CHECK(!brls::ThemeDecorations::particlesRunning());
    drawn = brls::Application::getDrawnFrameCount();
    h.advance(2000);
    BRLS_CHECK(brls::Application::getDrawnFrameCount() - drawn <= 2);
    BRLS_CHECK_EQ(brls::test::compareImages(h.screenshot(), still, 0).differingPixels, (size_t)0);

    // An input: back
    h.tap(brls::BUTTON_DOWN);
    BRLS_CHECK(brls::ThemeDecorations::particlesRunning());
    drawn = brls::Application::getDrawnFrameCount();
    h.advance(1000);
    BRLS_CHECK(brls::Application::getDrawnFrameCount() - drawn >= 25);

    // Never a view: the focus and the layout know nothing of them
    BRLS_CHECK(dynamic_cast<brls::Box*>(h.top()->getContentView())->getChildren().size() == 1);
    brls::Theme::reset();
    h.settle();
    BRLS_CHECK(!brls::ThemeDecorations::particlesRunning());
}

// Every kind, mid-flight, in both variants: drawn (and saved, for a look,
// with BRLS_DECOR_SHOTS=<folder>)
BRLS_TEST(theme_particles_draw_every_kind)
{
    const struct
    {
        const char* kind;
        const char* preset;
    } kinds[] = { { "snow", "winter" }, { "leaves", "autumn" }, { "petals", "spring" }, { "embers", "halloween" },
        { "confetti", "new-year" }, { "sparkles", "lunar-new-year" }, { "hearts", "valentine" }, { "bubbles", "summer" } };
    const char* folder = std::getenv("BRLS_DECOR_SHOTS");
    Harness h;
    h.push(new brls::Activity(new brls::Box()));
    for (const auto& each : kinds)
        for (brls::ThemeVariant variant : { brls::ThemeVariant::DARK, brls::ThemeVariant::LIGHT })
        {
            brls::Application::getPlatform()->setThemeVariant(variant);
            brls::Theme::load(std::string("inherit = builtin:") + each.preset + "\n");
            h.step(2);
            brls::test::Image plain = h.screenshot();
            useParticles(each.preset, each.kind, "particles_density = high\n");
            h.advance(2500);
            brls::test::Image shot = h.screenshot();
            BRLS_CHECK(brls::test::compareImages(shot, plain, 4).differingPixels > 200);
            if (folder)
                shot.savePng(std::string(folder) + "/particles_" + each.kind
                    + (variant == brls::ThemeVariant::DARK ? "_dark" : "_light") + ".png");
        }
    // A few frames in a row, for a moving picture
    if (folder)
        for (const char* kind : { "snow", "leaves" })
        {
            brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);
            useParticles(std::string(kind) == "snow" ? "winter" : "autumn", kind, "particles_density = medium\n");
            h.advance(2000);
            for (int i = 0; i < 45; i++)
            {
                h.advance(1000.0 / 30);
                char name[64];
                std::snprintf(name, sizeof(name), "/frames_%s_%02d.png", kind, i);
                h.screenshot().crop(0, 0, 640, 360).savePng(std::string(folder) + name);
            }
        }
    brls::Theme::reset();
    h.step(2);
}

// backdrop.header: across the whole width of a screen's header, under its
// title; nothing elsewhere
BRLS_TEST(theme_decor_header_backdrop)
{
    DemoHarness h;
    brls::Application::setReduceMotion(true);
    ThemeFolder themes("aurore_theme_header_test");
    themes.picture("red.png", 255, 0, 0);
    BRLS_CHECK(brls::Theme::loadFile(themes.write("red.theme", "inherit = dark\nbackdrop.header = red.png\n")));
    h.step();
    while (brls::ThemeImages::decoding() > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    h.settle();
    BRLS_CHECK(brls::ThemeDecorations::hasPicture(brls::ThemeArea::HEADER));
    brls::test::Image shot = h.screenshot();
    for (int x : { 2, 640, 1277 })
        BRLS_CHECK(shot.at(x, 4)[0] > 200 && shot.at(x, 4)[1] < 60);
    BRLS_CHECK(shot.at(640, 300)[0] < 120); // the content: as it was
    brls::Theme::reset();
    h.settle();
    BRLS_CHECK(h.screenshot().at(640, 4)[0] < 120);
}

// A translucent dialog over a theme's picture shows the picture, not the
// screen behind it
BRLS_TEST(theme_decor_dialog_over_a_picture)
{
    DemoHarness h;
    brls::Application::setReduceMotion(true);
    BRLS_CHECK(brls::Theme::loadFile("@res/themes/aurora/theme.theme"));
    h.step();
    while (brls::ThemeImages::decoding() > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    h.settle();
    h.tap(brls::BUTTON_RIGHT);
    h.tap(brls::BUTTON_DOWN);
    BRLS_CHECK_EQ(h.focusText(), std::string("Confirm"));
    h.tap(brls::BUTTON_A);
    h.settle();
    BRLS_CHECK(h.matchesReference("theme_decor_dialog"));
}

BRLS_TEST(images_compare_with_tolerance)
{
    brls::test::Image a;
    a.width  = 4;
    a.height = 2;
    a.rgba.assign(4 * 2 * 4, 100);
    brls::test::Image b = a;
    b.rgba[0] = 105; // within 8
    b.rgba[4] = 140; // a pixel off
    auto diff = brls::test::compareImages(b, a, 8);
    BRLS_CHECK(diff.sameSize);
    BRLS_CHECK_EQ(diff.differingPixels, (size_t)1);
    BRLS_CHECK_EQ(diff.maxDelta, 40);
    BRLS_CHECK(!brls::test::compareImages(b, a.crop(0, 0, 2, 2)).sameSize);
}

int main(int argc, char** argv)
{
    return brls::test::runTests(argc, argv);
}
