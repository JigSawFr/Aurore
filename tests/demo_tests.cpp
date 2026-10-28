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
#include <borealis/cadran/ui/cadran_kit.hpp>
#include <borealis/cadran/ui/cadran_list.hpp>
#include <borealis/cadran/ui/cadran_modal.hpp>
#include <borealis/cadran/ui/hooks.hpp>
#include <borealis/cadran/ui/live_prefs.hpp>
#include <borealis/cadran/ui/prefs.hpp>
#include <borealis/cadran/ui/ui.hpp>
#include <borealis/cadran/view/cadran_icon.hpp>
#include <borealis/core/hot_reload.hpp>
#include <borealis/core/theme_decor.hpp>
#include <borealis/core/theme_image.hpp>
#include <borealis/core/util.hpp>
#include <borealis/platforms/desktop/desktop_font.hpp>
#include <borealis/test/harness.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <thread>
#ifndef _WIN32
#include <sys/stat.h>
#endif

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

// The pictures of a theme are decoded on a worker thread: waits for them, 10 s at most, so a stuck decode fails
// the test instead of hanging the job
bool themeImagesDecoded()
{
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (brls::ThemeImages::decoding() > 0)
    {
        if (std::chrono::steady_clock::now() > deadline)
            return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

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

BRLS_TEST(missing_reference_fails_in_ci)
{
    DemoHarness h;
    const char* ci    = std::getenv("CI");
    std::string saved = ci ? ci : "";
    setenv("CI", "true", 1);
    bool matched = h.matchesReference("no_such_reference");
    if (ci)
        setenv("CI", saved.c_str(), 1);
    else
        unsetenv("CI");
    BRLS_CHECK(!matched);
    BRLS_CHECK(!std::filesystem::exists(h.options().referenceDir + "/no_such_reference.png"));
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
    brls::delay(500, [&fired]()
        { fired = true; });
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
        { 2026, 3, 3, "lunar-new-year" }, // the Lantern Festival
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
    namespace fs       = std::filesystem;
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
        { 1, 20, "snowflake" },
        { 2, 12, "heart" },
        { 2, 17, "lantern" },
        { 4, 10, "blossom" },
        { 7, 14, "sun" },
        { 10, 1, "leaf" },
        { 10, 31, "pumpkin" },
        { 12, 24, "tree" },
        { 12, 31, "firework" },
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

    // A square of one colour in the middle of a transparent picture
    std::string spot(const std::string& name, unsigned char r, unsigned char g, unsigned char b, int size = 32)
    {
        brls::test::Image image;
        image.width  = size;
        image.height = size;
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                bool inside = x >= size / 4 && x < size * 3 / 4 && y >= size / 4 && y < size * 3 / 4;
                image.rgba.insert(image.rgba.end(), { r, g, b, (unsigned char)(inside ? 255 : 0) });
            }
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
    brls::ThemeInfo info      = brls::Theme::inspect(escapes);
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

// A file that includes itself, or many files each including the next ones
// many times: read once each at most, quickly, with errors that say where
BRLS_TEST(theme_include_fanout_is_bounded)
{
    Harness h;
    ThemeFolder themes("aurore_theme_fanout_test");
    std::string self = "accent = #3ddc97\n";
    for (int i = 0; i < 10; i++)
        self += "@include self.inc\n";
    themes.write("self.inc", self);
    const std::string fan = themes.write("fan.theme", "name = Fan\n@include self.inc\n");
    // Ten files, each including the next one ten times: 10^9 reads unbounded
    for (int f = 0; f < 10; f++)
    {
        std::string text;
        for (int i = 0; i < 10; i++)
            text += "@include part" + std::to_string(f + 1) + ".inc\n";
        themes.write("part" + std::to_string(f) + ".inc", text);
    }
    const std::string wide = themes.write("wide.theme", "name = Wide\n@include part0.inc\n");
    const std::string loop = themes.write("loop.theme", "name = Loop\ninherit = loop.theme\n");
    std::string often      = "name = Often\n";
    for (int i = 0; i < 70; i++)
        often += "@include leaf.inc\n";
    themes.write("leaf.inc", "accent = #3ddc97\n");
    const std::string oftenPath = themes.write("often.theme", often);

    auto start                = std::chrono::steady_clock::now();
    brls::ThemeInfo fanInfo   = brls::Theme::inspect(fan);
    brls::ThemeInfo wideInfo  = brls::Theme::inspect(wide);
    brls::ThemeInfo loopInfo  = brls::Theme::inspect(loop);
    brls::ThemeInfo oftenInfo = brls::Theme::inspect(oftenPath);
    BRLS_CHECK(brls::Theme::loadFile(wide));
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    BRLS_CHECK(seconds < 5.0);

    BRLS_CHECK(hasErrorWith(fanInfo, "self.inc:2: @include self.inc includes itself (a loop)"));
    BRLS_CHECK(fanInfo.errors.size() <= 10);
    BRLS_CHECK(hasErrorWith(wideInfo, "part7.inc:1: @include part8.inc goes too deep"));
    BRLS_CHECK_EQ(wideInfo.errors.size(), (size_t)101);
    BRLS_CHECK(hasErrorWith(wideInfo, "more errors"));
    BRLS_CHECK(hasErrorWith(oftenInfo, "line 66: @include leaf.inc: skipped, the theme reads more than 64 files"));
    BRLS_CHECK(!hasErrorWith(oftenInfo, "line 65:"));
    BRLS_CHECK(hasErrorWith(loopInfo, "inherit loop.theme inherits itself (a loop)"));
    brls::Theme::reset();
}

// @include, inherit and counterpart read files of the theme's folder only, as
// its pictures; a resource theme still includes resources
BRLS_TEST(theme_includes_stay_in_their_folder)
{
    Harness h;
    ThemeFolder themes("aurore_theme_include_confined_test");
    const std::string root = themes.dir.generic_string();
    themes.picture("other/pic.png", 200, 20, 40);
    themes.write("other/base.theme", "name = Base\nbackground_image = pic.png\naccent = #ff0000\n");
    themes.write("outside.inc", "accent = #ff0000\n");
    themes.write("mine/sibling.theme", "name = Sibling\naccent = #00ff00\n");

    const std::string absolute = themes.write("mine/abs.theme", "name = Abs\n@include /etc/passwd\n");
    brls::ThemeInfo info       = brls::Theme::inspect(absolute);
    BRLS_CHECK(hasErrorWith(info, "line 2: @include /etc/passwd: an absolute path is not allowed"));
    BRLS_CHECK(!hasErrorWith(info, "no '='")); // nothing of the file read
    BRLS_CHECK(!hasErrorWith(info, "root:"));

    info = brls::Theme::inspect(themes.write("mine/up.theme", "name = Up\n@include ../outside.inc\n"));
    BRLS_CHECK(hasErrorWith(info, "line 2: @include ../outside.inc: goes out of the theme's folder"));

    const std::string inherits = themes.write("mine/inherits.theme", "name = Inherits\ninherit = ../other/base.theme\n");
    info                       = brls::Theme::inspect(inherits);
    BRLS_CHECK(hasErrorWith(info, "inherit ../other/base.theme: goes out of the theme's folder"));
    BRLS_CHECK(!info.darkBackground.hasImage());
    BRLS_CHECK(brls::Theme::loadFile(inherits));
    BRLS_CHECK(!brls::Theme::getBackground(brls::ThemeVariant::DARK).hasImage());
    brls::Theme::reset();

    info = brls::Theme::inspect(themes.write("mine/res.theme", "name = Res\n@include @res/themes/shared.inc\n"));
    BRLS_CHECK(hasErrorWith(info, "not in the app's resources"));
    info = brls::Theme::inspect(themes.write("mine/pair.theme", "name = Pair\ninherit = dark\ncounterpart = /etc/hostname\n"));
    BRLS_CHECK(hasErrorWith(info, "counterpart /etc/hostname: an absolute path is not allowed"));
    BRLS_CHECK(info.counterpart.empty());

    // Still fine: a sibling, a preset by name, a resource theme's includes
    BRLS_CHECK(brls::Theme::inspect(themes.write("mine/ok.theme", "name = Ok\ninherit = sibling.theme\n")).ok());
    BRLS_CHECK(brls::Theme::inspect(themes.write("mine/preset.theme", "name = P\ninherit = ocean\n")).ok());
    BRLS_CHECK(brls::Theme::inspect("@res/themes/rose.theme").ok());

    // Too large, or not a file: not read
    themes.write("mine/big.inc", "# " + std::string(300 * 1024, 'x') + "\n");
    info = brls::Theme::inspect(themes.write("mine/big.theme", "name = Big\n@include big.inc\n"));
    BRLS_CHECK(hasErrorWith(info, "@include big.inc: cannot read"));
    std::filesystem::create_directories(themes.dir / "mine" / "folder.inc");
    info = brls::Theme::inspect(themes.write("mine/dir.theme", "name = Dir\n@include folder.inc\n"));
    BRLS_CHECK(hasErrorWith(info, "@include folder.inc: cannot read"));
#ifndef _WIN32
    // A pipe would block a reader for ever
    BRLS_CHECK(::mkfifo((themes.dir / "mine" / "pipe.inc").c_str(), 0600) == 0);
    info = brls::Theme::inspect(themes.write("mine/pipe.theme", "name = Pipe\n@include pipe.inc\n"));
    BRLS_CHECK(hasErrorWith(info, "@include pipe.inc: cannot read"));

    // A link out of the folder, to a file or to a folder, there or not
    std::filesystem::create_symlink(themes.dir / "outside.inc", themes.dir / "mine" / "link.inc");
    std::filesystem::create_directory_symlink(themes.dir / "other", themes.dir / "mine" / "away");
    info = brls::Theme::inspect(themes.write("mine/link.theme", "name = Link\n@include link.inc\n"));
    BRLS_CHECK(hasErrorWith(info, "a link out of the theme's folder"));
    std::string path, error;
    const std::string folder = root + "/mine";
    BRLS_CHECK(!brls::Theme::resolveAsset("away/missing.png", folder, folder, &path, &error));
    BRLS_CHECK(error.find("a link out of the theme's folder") != std::string::npos);
    // A broken link: where it leads cannot be told, refused
    std::filesystem::create_symlink(themes.dir / "nowhere.png", themes.dir / "mine" / "broken.png");
    BRLS_CHECK(!brls::Theme::resolveAsset("broken.png", folder, folder, &path, &error));
    // A file not there yet, in the folder: fine (inspect says it is missing)
    BRLS_CHECK(brls::Theme::resolveAsset("later.png", folder, folder, &path, &error));
#endif
}

// Colours and metrics are finite numbers in their ranges
BRLS_TEST(theme_numbers_are_finite_and_in_range)
{
    Harness h;
    NVGcolor color;
    for (const char* bad : { "rgba(nan, 1, 1, 1)", "rgb(inf, 0, 0)", "rgb(1e39, 0, 0)", "rgb(256, 0, 0)", "rgb(-5, 0, 0)",
             "rgba(0, 0, 0, 1.5)", "rgba(0, 0, 0, -0.1)", "rgba(0, 0, 0, nan)", "rgb(0x1p9, 0, 0)" })
        BRLS_CHECK(!brls::Theme::parseColor(bad, &color));
    BRLS_CHECK(brls::Theme::parseColor("rgb(255, 0, 0)", &color) && color.r == 1.0f);
    BRLS_CHECK(brls::Theme::parseColor("rgba(0, 0, 0, 0.5)", &color) && color.a == 0.5f);

    ThemeFolder themes("aurore_theme_numbers_test");
    brls::ThemeInfo info = brls::Theme::inspect(themes.write("nan.theme",
        "name = N\naccent = rgba(nan, 1e39, -5, inf)\n[metrics]\nbrls/sidebar/width = nan\nbrls/header/height = inf\n"
        "brls/highlight/stroke_width = 1e39\nbrls/sidebar/padding_top = -20000\nbrls/sidebar/padding_left = 12\n"));
    BRLS_CHECK(!info.ok());
    BRLS_CHECK(hasErrorWith(info, "line 2: rgba(nan, 1e39, -5, inf) is not a colour"));
    for (const char* value : { "line 4: nan", "line 5: inf", "line 6: 1e39", "line 7: -20000" })
        BRLS_CHECK(hasErrorWith(info, value));
    BRLS_CHECK(!hasErrorWith(info, "line 8"));
}

// Errors quote the file: never a control character to the terminal, and at
// most 100 of them
BRLS_TEST(theme_errors_are_escaped_and_capped)
{
    Harness h;
    ThemeFolder themes("aurore_theme_errors_test");
    brls::ThemeInfo info = brls::Theme::inspect(themes.write("ansi.theme", "name = \x1b]0;title\x07\n\x1b[2Jred\naccent = \xc2\x9b"
                                                                           "31m\n"));
    BRLS_CHECK(hasErrorWith(info, "line 2: no '=' in \"\\x1B[2Jred\""));
    BRLS_CHECK(hasErrorWith(info, "\\x9B31m is not a colour"));
    for (const std::string& error : info.errors)
        for (unsigned char c : error)
            BRLS_CHECK(c >= 0x20 && c != 0x7F && c != 0x9B);
    BRLS_CHECK_EQ(brls::escapeControlChars("a\tb\x1b"
                                           "c\xc2\x85"
                                           "d\xc3\xa9"),
        std::string("a\\x09b\\x1Bc\\x85d\xc3\xa9"));

    std::string many = "name = Many\n";
    for (int i = 0; i < 500; i++)
        many += "nonsense\n";
    info = brls::Theme::inspect(themes.write("many.theme", many));
    BRLS_CHECK_EQ(info.errors.size(), (size_t)101);
    BRLS_CHECK_EQ(info.errors.back(), std::string("and 400 more errors"));
}

// "@res/…" stays in the resources: in hot reload's folder and when a theme
// is read
BRLS_TEST(resource_names_stay_in_the_resources)
{
    Harness h;
    BRLS_CHECK(brls::isResourceNameSafe("@res/themes/rose.theme"));
    BRLS_CHECK(brls::isResourceNameSafe("@res/a..b/c"));
    for (const char* name : { "@res/../x", "@res/themes/../../x", "@res/..", "@res/a\\..\\..\\x" })
        BRLS_CHECK(!brls::isResourceNameSafe(name));

    ThemeFolder resources("aurore_resources_test");
    resources.write("themes/x.theme", "name = X\n");
    resources.write("secret.theme", "name = Secret\n");
    brls::HotReload::setEnabled(true, (resources.dir / "themes").string());
    BRLS_CHECK(!brls::HotReload::diskResource("@res/x.theme").empty());
    BRLS_CHECK(brls::HotReload::diskResource("@res/../secret.theme").empty());
    BRLS_CHECK(!brls::Theme::loadFile("@res/../secret.theme"));
    brls::HotReload::setEnabled(false);

    // A language that is no locale's name is not a path either
    brls::loadTranslations("../../etc");
    BRLS_CHECK_EQ(brls::getStr("hints/ok"), std::string("OK"));
    brls::loadTranslations(brls::LOCALE_DEFAULT);
}

BRLS_TEST(theme_background_keys_are_read_and_inherited)
{
    Harness h;
    ThemeFolder themes("aurore_theme_background_test");
    themes.picture("a.png", 10, 20, 40);
    themes.picture("b.png", 230, 235, 240);
    const std::string base  = themes.write("base.theme",
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
    NVGcolor warning = brls::Theme::getDarkTheme()["cadran/amber_surf"];
    BRLS_CHECK(brls::Theme::loadFile(child));
    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);
    h.settle();
    BRLS_CHECK(brls::Application::hasThemeBackground());
    BRLS_CHECK(std::fabs(brls::Theme::getDarkTheme()["cadran/surf"].a - surface.a * 0.8f) < 0.001f);
    BRLS_CHECK(std::fabs(brls::Theme::getDarkTheme()["brls/sidebar/background"].a - 0.8f) < 0.001f);
    BRLS_CHECK(std::fabs(brls::Theme::getDarkTheme()["cadran/amber_surf"].a - warning.a * 0.8f) < 0.001f);
    BRLS_CHECK(brls::Theme::getDarkTheme()["brls/background"].a == 1.0f);
    BRLS_CHECK(brls::Theme::loadFile(plain));
    h.settle();
    BRLS_CHECK(!brls::Application::hasThemeBackground());
    BRLS_CHECK_EQ(brls::Theme::getDarkTheme()["cadran/surf"].a, 1.0f);
    brls::Theme::reset();
    h.settle();
    BRLS_CHECK_EQ(brls::Theme::getDarkTheme()["cadran/surf"].a, surface.a);
    BRLS_CHECK_EQ(brls::Theme::getDarkTheme()["cadran/amber_surf"].a, warning.a);
}

BRLS_TEST(theme_text_is_checked_on_its_picture)
{
    Harness h;
    ThemeFolder themes("aurore_theme_contrast_test");
    themes.picture("snow.png", 250, 250, 250);
    themes.picture("night.png", 8, 10, 20);
    // White text on a dark theme: fine on its colour, not on a white picture
    // with no veil
    const char* dark        = "inherit = dark\nbackground = #101418\ntext = #ffffff\nbackground_dim = 0\n";
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
    auto still = [&h]()
    { return h.screenshot().crop(1000, 0, 280, 430); };
    brls::test::Image before = still();
    auto colors              = brls::Theme::getDarkTheme().getValues()->getColors();

    BRLS_CHECK(brls::Theme::loadFile("@res/themes/aurora/theme.theme"));
    brls::Application::getPlatform()->setThemeVariant(variant);
    h.step(); // the picture asked for: decoded, as many frames on any machine
    BRLS_CHECK(themeImagesDecoded());
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

// Waits for the pictures being decoded (in real time), then for the screen
// (a picture that fails can ask for another, the app's own: a few rounds)
static void settlePictures(Harness& h)
{
    for (int round = 0; round < 3; round++)
    {
        h.step();
        BRLS_CHECK(themeImagesDecoded());
        h.settle();
    }
}

static bool hasWarningWith(const brls::ThemeInfo& info, const std::string& text)
{
    for (const std::string& warning : info.warnings)
        if (warning.find(text) != std::string::npos)
            return true;
    return false;
}

BRLS_TEST(theme_logos_and_images_are_read_inherited_and_confined)
{
    Harness h;
    ThemeFolder themes("aurore_theme_pictures_test");
    const std::string root = themes.dir.generic_string();
    brls::Theme::addImageSlot("test/art", "");
    brls::Theme::addImageSlot("test/cover", "");
    for (const char* name : { "logo.png", "logo-day.png", "preview.png", "gear.png", "art.png", "base/logo.png", "base/i.png", "base/art.png" })
        themes.picture(name, 120, 60, 200);
    const std::string base = themes.write("base/theme.theme",
        "name = Base\nlogo = logo.png\nicon.info = i.png\nicon.info.tint = true\nimage.test/art = art.png\n"
        "[light]\nicon.settings = i.png\n");
    const std::string full = themes.write("full.theme",
        "name = Full\nlogo = logo.png\npreview = preview.png\nicon.settings = gear.png\nicon.settings.tint = yes\n"
        "icon.gamepad = gear.png\nimage.test/art = art.png\n[light]\nlogo = logo-day.png\nicon.gamepad = none\n"
        "[dark]\nimage.test/cover = art.png\n");

    brls::ThemeInfo info = brls::Theme::inspect(full);
    BRLS_CHECK(info.ok());
    BRLS_CHECK(info.warnings.empty());
    BRLS_CHECK_EQ(info.darkPictures.logo, root + "/logo.png");
    BRLS_CHECK_EQ(info.lightPictures.logo, root + "/logo-day.png"); // its own variant's line wins
    BRLS_CHECK_EQ(info.lightPictures.preview, root + "/preview.png");
    BRLS_CHECK(info.darkPictures.icons.at("settings").tint);
    BRLS_CHECK(!info.darkPictures.icons.at("gamepad").tint);
    BRLS_CHECK(!info.lightPictures.icons.count("gamepad")); // none
    BRLS_CHECK_EQ(info.lightPictures.images.at("test/art"), root + "/art.png");
    BRLS_CHECK(info.darkPictures.images.count("test/cover") && !info.lightPictures.images.count("test/cover"));

    // Inherited: the parent's pictures stay in the parent's folder, this
    // file's lines win
    const std::string child = themes.write("child.theme",
        "inherit = base/theme.theme\nicon.settings = gear.png\n[dark]\nlogo = none\n");
    info                    = brls::Theme::inspect(child);
    BRLS_CHECK(info.ok());
    BRLS_CHECK(info.darkPictures.logo.empty());
    BRLS_CHECK_EQ(info.lightPictures.logo, root + "/base/logo.png");
    BRLS_CHECK_EQ(info.darkPictures.icons.at("info").image, root + "/base/i.png");
    BRLS_CHECK(info.darkPictures.icons.at("info").tint);
    BRLS_CHECK_EQ(info.lightPictures.icons.at("settings").image, root + "/gear.png"); // over the parent's [light]
    BRLS_CHECK_EQ(info.darkPictures.images.at("test/art"), root + "/base/art.png");

    // Confined, read, named: errors and warnings, never a crash
    const std::string wrong = themes.write("wrong.theme",
        "logo = ../outside.png\npreview = /etc/passwd\nicon.settings = missing.png\nicon.settings.tint = maybe\n"
        "icon.nosuchicon = gear.png\nimage.test/nosuchslot = art.png\n");
    info                    = brls::Theme::inspect(wrong);
    BRLS_CHECK(hasErrorWith(info, "logo ../outside.png: goes out of the theme's folder"));
    BRLS_CHECK(hasErrorWith(info, "preview /etc/passwd: an absolute path is not allowed"));
    BRLS_CHECK(hasErrorWith(info, "icon.settings: cannot read"));
    BRLS_CHECK(hasErrorWith(info, "icon.settings.tint is true or false"));
    BRLS_CHECK(hasWarningWith(info, "icon.nosuchicon: no icon has that name"));
    BRLS_CHECK(hasWarningWith(info, "image.test/nosuchslot: the app has no such image slot (kept); its slots: "));
    BRLS_CHECK(hasWarningWith(info, "test/art, test/cover"));
    BRLS_CHECK(info.darkPictures.logo.empty() && info.darkPictures.preview.empty());

    // Used, and back: the logo is the theme's for the variant on screen
    BRLS_CHECK(!brls::Theme::logo());
    BRLS_CHECK(brls::Theme::loadFile(full));
    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);
    h.settle();
    // Loaded when asked for (by whoever draws it), off the UI thread
    BRLS_CHECK(brls::Theme::logo() && brls::Theme::logo()->state == brls::ThemeImage::State::LOADING);
    settlePictures(h);
    std::shared_ptr<brls::ThemeImage> logo = brls::Theme::logo();
    BRLS_CHECK(logo && logo->ready() && logo->path == root + "/logo.png");
    BRLS_CHECK(logo->width <= brls::ThemeImages::LOGO_SIZE && logo->height <= brls::ThemeImages::LOGO_SIZE);
    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::LIGHT);
    h.settle();
    BRLS_CHECK(brls::Theme::logo() && brls::Theme::logo()->path == root + "/logo-day.png");
    bool tint = false;
    BRLS_CHECK(brls::Theme::icon("settings", &tint) && tint);
    BRLS_CHECK(!brls::Theme::icon("gamepad"));
    brls::Theme::reset();
    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);
    h.settle();
    BRLS_CHECK(!brls::Theme::logo() && !brls::Theme::icon("settings"));
}

BRLS_TEST(theme_pictures_are_capped_and_freed)
{
    Harness h;
    ThemeFolder themes("aurore_theme_pictures_cap_test");
    themes.picture("big.png", 200, 100, 50, 1024, 512);
    themes.write("big.theme", "logo = big.png\nicon.settings = big.png\nicon.settings.tint = true\n");
    BRLS_CHECK(brls::Theme::loadFile((themes.dir / "big.theme").generic_string()));
    h.settle();
    std::weak_ptr<brls::ThemeImage> logo = brls::Theme::logo();
    std::weak_ptr<brls::ThemeImage> icon = brls::Theme::icon("settings");
    settlePictures(h);
    BRLS_CHECK(logo.lock() && logo.lock()->ready());
    BRLS_CHECK_EQ(logo.lock()->width, brls::ThemeImages::LOGO_SIZE);
    BRLS_CHECK_EQ(logo.lock()->height, brls::ThemeImages::LOGO_SIZE / 2); // its ratio kept
    BRLS_CHECK_EQ(icon.lock()->width, brls::ThemeImages::ICON_SIZE);
    BRLS_CHECK(icon.lock()->mask);
    // Another theme: the pictures are let go of, their textures freed
    BRLS_CHECK(brls::Theme::loadFile("builtin:ocean"));
    h.settle();
    BRLS_CHECK(!brls::Theme::logo());
    BRLS_CHECK(logo.expired() && icon.expired());
    brls::Theme::reset();
}

BRLS_TEST(theme_image_slots_follow_the_theme)
{
    Harness h;
    ThemeFolder themes("aurore_theme_slots_test");
    const std::string fallback = themes.picture("fallback.png", 10, 200, 10, 48, 48);
    themes.picture("art.png", 200, 10, 10, 64, 32);
    themes.write("broken.png", "not a png");
    brls::Theme::addImageSlot("test/art", fallback, 256);
    brls::Theme::addImageSlot("test/empty");
    BRLS_CHECK(brls::Theme::getImageSlots() == (std::vector<std::string> { "test/art", "test/empty" }));
    const std::string art    = themes.write("art.theme", "image.test/art = art.png\nimage.test/empty = art.png\n");
    const std::string broken = themes.write("broken.theme", "image.test/art = broken.png\n");

    auto* image = new brls::Image();
    image->setThemeImage("test/art");
    auto* empty = new brls::Image();
    empty->setThemeImage("test/empty");
    h.push(new brls::Activity(screenOf({ image, empty })));
    settlePictures(h);

    // The app's own, without a theme; nothing for a slot without one
    BRLS_CHECK(brls::Theme::image("test/art") && brls::Theme::image("test/art")->path == fallback);
    BRLS_CHECK(!brls::Theme::image("test/empty"));
    BRLS_CHECK(!brls::Theme::image("test/undeclared"));
    BRLS_CHECK_EQ(image->getOriginalImageWidth(), 48.0f);
    BRLS_CHECK_EQ(empty->getTexture(), 0);

    // The theme's, live
    BRLS_CHECK(brls::Theme::loadFile(art));
    settlePictures(h);
    BRLS_CHECK_EQ(image->getOriginalImageWidth(), 64.0f);
    BRLS_CHECK_EQ(image->getTexture(), brls::Theme::image("test/art")->texture);
    BRLS_CHECK(empty->getTexture() != 0);

    // A picture that cannot be read: the app's
    BRLS_CHECK(brls::Theme::loadFile(broken));
    settlePictures(h);
    BRLS_CHECK_EQ(image->getOriginalImageWidth(), 48.0f);
    BRLS_CHECK_EQ(empty->getTexture(), 0);

    // Back to no theme: the app's again
    BRLS_CHECK(brls::Theme::loadFile(art));
    settlePictures(h);
    BRLS_CHECK_EQ(image->getOriginalImageWidth(), 64.0f);
    brls::Theme::reset();
    settlePictures(h);
    BRLS_CHECK_EQ(image->getOriginalImageWidth(), 48.0f);
    BRLS_CHECK_EQ(empty->getTexture(), 0);

    // Set another way: it stops following the theme
    image->setImageFromFile(themes.picture("own.png", 1, 2, 3, 20, 20));
    BRLS_CHECK(image->getThemeImage().empty());
    BRLS_CHECK(brls::Theme::loadFile(art));
    settlePictures(h);
    BRLS_CHECK_EQ(image->getOriginalImageWidth(), 20.0f);
    brls::Theme::reset();
}

// An icon replaced by a picture: as it is, or as a mask in the icon's colour
BRLS_TEST(theme_icons_are_tinted_or_drawn_as_they_are)
{
    Harness h;
    ThemeFolder themes("aurore_theme_icons_test");
    themes.spot("red.png", 230, 20, 20);
    themes.picture("ink.png", 0, 0, 0, 32, 32); // no transparency: a mask by its dark parts
    themes.write("icons.theme",
        "icon.today = red.png\nicon.today.tint = true\nicon.timer = red.png\nicon.lock = ink.png\nicon.lock.tint = true\n");
    auto icon = [](const char* name)
    {
        auto* view = new CadranIcon();
        view->setIcon(name);
        view->setColor(nvgRGB(20, 40, 230));
        view->setWidth(64);
        view->setHeight(64);
        view->setMarginBottom(10);
        return view;
    };
    CadranIcon* tinted = icon("today");
    CadranIcon* plain  = icon("timer");
    CadranIcon* ink    = icon("lock");
    CadranIcon* other  = icon("shield");
    h.push(new brls::Activity(screenOf({ tinted, plain, ink, other })));
    h.settle();
    brls::test::Image before = h.screenshot();

    auto middle = [&h](brls::View* view)
    {
        brls::Rect frame       = view->getFrame();
        brls::test::Image shot = h.screenshot();
        const unsigned char* p = shot.at((int)frame.getMidX(), (int)frame.getMidY());
        return std::vector<int> { p[0], p[1], p[2] };
    };
    auto corner = [&h](brls::View* view)
    {
        brls::Rect frame       = view->getFrame();
        brls::test::Image shot = h.screenshot();
        const unsigned char* p = shot.at((int)frame.getMinX() + 4, (int)frame.getMinY() + 4);
        return std::vector<int> { p[0], p[1], p[2] };
    };
    auto near = [](std::vector<int> a, std::vector<int> b)
    {
        for (int c = 0; c < 3; c++)
            if (std::abs(a[c] - b[c]) > 6)
                return false;
        return true;
    };

    BRLS_CHECK(brls::Theme::loadFile((themes.dir / "icons.theme").generic_string()));
    settlePictures(h);
    BRLS_CHECK(near(middle(tinted), { 20, 40, 230 })); // the icon's colour
    BRLS_CHECK(near(middle(plain), { 230, 20, 20 })); // the picture's
    BRLS_CHECK(near(corner(tinted), { 20, 20, 30 })); // transparent around it
    BRLS_CHECK(near(middle(ink), { 20, 40, 230 })); // black on nothing: all ink

    // Back: the drawings, pixel for pixel
    brls::Theme::reset();
    h.settle();
    auto diff = brls::test::compareImages(h.screenshot(), before, 0);
    BRLS_CHECK(diff.sameSize && diff.differingPixels == 0);
}

// The demo's Prism theme (resources/themes/prism): its logo in the header,
// its icons on the rail (Settings focused: a tinted icon in the focus's
// colours, the gamepad as it is)
static void prismScreen(brls::ThemeVariant variant, const char* reference)
{
    DemoHarness h;
    brls::test::Image before = h.screenshot();
    BRLS_CHECK(brls::Theme::loadFile("@res/themes/prism/theme.theme"));
    brls::Application::getPlatform()->setThemeVariant(variant);
    settlePictures(h);
    BRLS_CHECK(brls::Theme::logo() && brls::Theme::logo()->ready());
    BRLS_CHECK(brls::Theme::checkContrast(variant).empty());
    BRLS_CHECK(h.focusByText("Settings"));
    settlePictures(h);
    BRLS_CHECK(h.matchesReference(reference));

    // Back to no theme, live: the screen as it was
    BRLS_CHECK(h.focusByText("Cadran", brls::BUTTON_UP));
    brls::Theme::reset();
    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);
    h.settle();
    BRLS_CHECK(!brls::Theme::logo());
    // (what does not move: the glow drifts) the mark within its corners, the
    // header's right end
    for (brls::Rect area : { brls::Rect(26, 29, 26, 26), brls::Rect(1000, 0, 280, 90) })
    {
        auto crop = [&area](const brls::test::Image& image)
        { return image.crop((int)area.getMinX(), (int)area.getMinY(), (int)area.getWidth(), (int)area.getHeight()); };
        auto diff = brls::test::compareImages(crop(h.screenshot()), crop(before), 0);
        BRLS_CHECK(diff.sameSize && diff.differingPixels == 0);
    }
}

BRLS_TEST(theme_logo_and_icons_dark)
{
    prismScreen(brls::ThemeVariant::DARK, "theme_images_dark");
}

BRLS_TEST(theme_logo_and_icons_light)
{
    prismScreen(brls::ThemeVariant::LIGHT, "theme_images_light");
}

// The demo's About tab draws its image slot (demo/about/art): Prism's
BRLS_TEST(theme_image_slot_in_the_demo)
{
    DemoHarness h;
    BRLS_CHECK(h.focusByText("About Aurore"));
    settlePictures(h);
    std::shared_ptr<brls::ThemeImage> own = brls::Theme::image("demo/about/art");
    BRLS_CHECK(own && own->path == "@res/img/aurore_256.png");
    BRLS_CHECK(brls::Theme::loadFile("@res/themes/prism/theme.theme"));
    settlePictures(h);
    BRLS_CHECK(brls::Theme::image("demo/about/art")->path == "@res/themes/prism/about.png");
    BRLS_CHECK(h.matchesReference("theme_image_slot"));
    brls::Theme::reset();
    settlePictures(h);
    BRLS_CHECK(brls::Theme::image("demo/about/art")->path == "@res/img/aurore_256.png");
}

// The palette picker with Prism's preview in its row: loaded when the
// picker opens, off the UI thread, the colours shown until then
static void pickPrism(const char* theme, const char* reference)
{
    DemoHarness h;
    ui::prefs().theme = theme;
    live_prefs::apply_theme();
    h.settle();
    const std::string preview = "@res/themes/prism/preview.png";
    // Listing decodes nothing
    for (const brls::ThemeInfo& info : brls::Theme::listThemes("@res/themes"))
        if (info.id == "prism")
            BRLS_CHECK_EQ(info.darkPictures.preview, preview);
    ui::pick_palette();
    // Asked for by the picker's swatch, not decoded on this thread: still loading
    brls::ThemeImageOptions options;
    options.flags                           = NVG_IMAGE_GENERATE_MIPMAPS;
    options.maxSide                         = brls::ThemeImages::PREVIEW_SIZE;
    std::shared_ptr<brls::ThemeImage> image = brls::ThemeImages::load(preview, options);
    BRLS_CHECK(image->state == brls::ThemeImage::State::LOADING);
    BRLS_CHECK(h.focusByText("Prism", brls::BUTTON_DOWN, 40));
    settlePictures(h);
    BRLS_CHECK(image->ready());
    BRLS_CHECK(h.matchesReference(reference));
}

BRLS_TEST(palette_picker_shows_previews)
{
    pickPrism("dark", "palette_picker_preview");
}

BRLS_TEST(palette_picker_shows_previews_light)
{
    pickPrism("light", "palette_picker_preview_light");
}

// Logos, icons and slot pictures still loading when the theme or the
// variant changes again: dropped, never drawn late, never freed twice (run
// under the sanitizers too)
BRLS_TEST(theme_pictures_switch_back_and_forth)
{
    DemoHarness h;
    BRLS_CHECK(h.focusByText("About Aurore"));
    const char* themes[] = { "@res/themes/prism/theme.theme", "@res/themes/aurora/theme.theme", "@res/themes/prism/theme.theme", "" };
    for (int round = 0; round < 16; round++)
    {
        const char* theme = themes[round % 4];
        if (*theme)
            BRLS_CHECK(brls::Theme::loadFile(theme));
        else
            brls::Theme::reset();
        brls::Application::getPlatform()->setThemeVariant(round % 3 ? brls::ThemeVariant::DARK : brls::ThemeVariant::LIGHT);
        h.step(round % 2 ? 1 : 2);
        if (round == 9)
            ui::pick_palette(); // its previews loading too
        if (round == 12)
            h.tap(brls::BUTTON_B);
    }
    BRLS_CHECK(brls::Theme::loadFile("@res/themes/prism/theme.theme"));
    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);
    settlePictures(h);
    BRLS_CHECK(brls::Theme::logo() && brls::Theme::logo()->ready());
    BRLS_CHECK(brls::Theme::image("demo/about/art")->path == "@res/themes/prism/about.png");
    brls::Theme::reset();
    settlePictures(h);
    BRLS_CHECK(!brls::Theme::logo());
    BRLS_CHECK(brls::Theme::image("demo/about/art")->path == "@res/img/aurore_256.png");
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
    BRLS_CHECK(themeImagesDecoded());
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
    drawn                   = brls::Application::getDrawnFrameCount();
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
    } kinds[]          = { { "snow", "winter" }, { "leaves", "autumn" }, { "petals", "spring" }, { "embers", "halloween" },
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
    BRLS_CHECK(themeImagesDecoded());
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

// The demo's Winter theme (a folder: resources/themes/winter): its strips
// and the sidebar's frost behind the main screen, its particles held still
// (reduced motion) so the reference does not depend on when
static void winterScreen(brls::ThemeVariant variant, const char* reference)
{
    DemoHarness h;
    brls::Application::setReduceMotion(true);
    BRLS_CHECK(brls::Theme::loadFile("@res/themes/winter/theme.theme"));
    brls::Application::getPlatform()->setThemeVariant(variant);
    h.step();
    BRLS_CHECK(themeImagesDecoded());
    h.settle();
    BRLS_CHECK(brls::ThemeDecorations::hasPicture(brls::ThemeArea::TOP));
    BRLS_CHECK(brls::ThemeDecorations::hasPicture(brls::ThemeArea::BOTTOM));
    BRLS_CHECK(brls::ThemeDecorations::hasPicture(brls::ThemeArea::SIDEBAR));
    BRLS_CHECK(brls::ThemeDecorations::particleCount() > 0 && !brls::ThemeDecorations::particlesRunning());
    BRLS_CHECK(h.matchesReference(reference));
}

BRLS_TEST(theme_decor_dark)
{
    winterScreen(brls::ThemeVariant::DARK, "theme_decor_dark");
}

BRLS_TEST(theme_decor_light)
{
    winterScreen(brls::ThemeVariant::LIGHT, "theme_decor_light");
}

// Decorations still loading, particles running, motion going on and off
// while the theme changes: nothing drawn late, nothing freed twice (run
// under the sanitizers too)
BRLS_TEST(theme_decor_switches_back_and_forth)
{
    DemoHarness h;
    const char* themes[] = { "@res/themes/winter/theme.theme", "builtin:new-year", "@res/themes/aurora/theme.theme",
        "builtin:halloween", "" };
    for (int round = 0; round < 20; round++)
    {
        const char* theme = themes[round % 5];
        if (*theme)
            BRLS_CHECK(brls::Theme::loadFile(theme));
        else
            brls::Theme::reset();
        brls::Application::getPlatform()->setThemeVariant(round % 3 ? brls::ThemeVariant::DARK : brls::ThemeVariant::LIGHT);
        brls::Application::setReduceMotion(round % 4 == 3);
        h.step(round % 2 ? 1 : 3);
    }
    brls::Application::setReduceMotion(false);
    BRLS_CHECK(brls::Theme::loadFile("@res/themes/winter/theme.theme"));
    h.step();
    BRLS_CHECK(themeImagesDecoded());
    h.settle();
    BRLS_CHECK(brls::ThemeDecorations::hasPicture(brls::ThemeArea::TOP) && brls::ThemeDecorations::particlesRunning());
    brls::Theme::reset();
    h.settle();
    BRLS_CHECK(!brls::ThemeDecorations::hasPicture(brls::ThemeArea::TOP) && !brls::ThemeDecorations::particlesRunning());
}

// A translucent dialog over a theme's picture shows the picture, not the
// screen behind it
BRLS_TEST(theme_decor_dialog_over_a_picture)
{
    DemoHarness h;
    brls::Application::setReduceMotion(true);
    BRLS_CHECK(brls::Theme::loadFile("@res/themes/aurora/theme.theme"));
    h.step();
    BRLS_CHECK(themeImagesDecoded());
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
    b.rgba[0]           = 105; // within 8
    b.rgba[4]           = 140; // a pixel off
    auto diff           = brls::test::compareImages(b, a, 8);
    BRLS_CHECK(diff.sameSize);
    BRLS_CHECK_EQ(diff.differingPixels, (size_t)1);
    BRLS_CHECK_EQ(diff.maxDelta, 40);
    BRLS_CHECK(!brls::test::compareImages(b, a.crop(0, 0, 2, 2)).sameSize);
}

namespace
{

// A line of text, the same size whatever it says
brls::Label* lineOf(const std::string& text)
{
    auto* label = new brls::Label();
    label->setText(text);
    label->setFontSize(40);
    label->setSingleLine(true);
    label->setWidth(600);
    label->setHeight(60);
    return label;
}

// What the screen shows where a view is
brls::test::Image shotOf(Harness& h, brls::View* view)
{
    brls::Rect frame = view->getFrame();
    return h.screenshot().crop((int)frame.getMinX(), (int)frame.getMinY(), (int)frame.getWidth(), (int)frame.getHeight());
}

// Two lines drawn differently: not the same glyphs
bool drawnDifferently(Harness& h, brls::View* one, brls::View* other)
{
    return brls::test::compareImages(shotOf(h, one), shotOf(h, other), 8).differingPixels > 100;
}

} // namespace

BRLS_TEST(fonts_are_the_free_ones)
{
    Harness h;
    // The text and the buttons from the library's resources, the Switch's
    // fonts nowhere
    BRLS_CHECK(brls::Application::getFont(brls::FONT_REGULAR) != brls::FONT_INVALID);
    BRLS_CHECK(brls::Application::getFont(brls::FONT_SWITCH_ICONS) != brls::FONT_INVALID);
    BRLS_CHECK(!brls::Application::loadFontFromFile("old", BRLS_ASSET("font/switch_font.ttf")));
    BRLS_CHECK(!brls::Application::loadFontFromFile("old", BRLS_ASSET("font/switch_icons.ttf")));
    // The harness reads no system font: its pictures are the same everywhere
    BRLS_CHECK(brls::Application::getFont(brls::FONT_KOREAN_REGULAR) == brls::FONT_INVALID);
    BRLS_CHECK(brls::Application::getFont(brls::FONT_JAPANESE) == brls::FONT_INVALID);
}

BRLS_TEST(button_glyphs_drawn)
{
    Harness h;
    // At the code points of the Switch's shared font (Hint::getKeyIcon)
    auto* a       = lineOf(brls::Hint::getKeyIcon(brls::BUTTON_A, true) + brls::Hint::getKeyIcon(brls::BUTTON_LB, true));
    auto* b       = lineOf(brls::Hint::getKeyIcon(brls::BUTTON_B, true) + brls::Hint::getKeyIcon(brls::BUTTON_RB, true));
    auto* missing = lineOf("\u0378\u0378"); // no such character: the empty box
    h.push(new brls::Activity(screenOf({ a, b, missing })));
    h.settle();
    BRLS_CHECK(drawnDifferently(h, a, b));
    BRLS_CHECK(drawnDifferently(h, a, missing));
    BRLS_CHECK(drawnDifferently(h, b, missing));
}

BRLS_TEST(asset_fonts_load_from_the_resources)
{
    // BRLS_ASSET() is "@res/..." with the resources built in: read from them
    Harness h;
    BRLS_CHECK(brls::Application::loadFontFromFile("test/sora", BRLS_ASSET("font/sora_bold_latin.ttf")));
    BRLS_CHECK(brls::Application::getFont("test/sora") != brls::FONT_INVALID);
    BRLS_CHECK(!brls::Application::loadFontFromFile("test/none", BRLS_ASSET("font/none.ttf")));
}

BRLS_TEST(cadran_display_font_loaded)
{
    // Cadran's figures in Sora, with the resources built in too
    DemoHarness h;
    BRLS_CHECK(brls::Application::getFont("cadran/digits") != brls::FONT_INVALID);
}

BRLS_TEST(cjk_from_the_system_fonts)
{
    brls::test::Options options;
    options.systemFonts = true;
    Harness h(options);
    struct Case
    {
        const char* language;
        const std::string& font;
        const char* text;
        const char* missing; // as many characters, none drawn
    };
    const Case cases[] = {
        { "ko", brls::FONT_KOREAN_REGULAR, "\uD55C\uAD6D\uC5B4", "\u0378\u0378\u0378" }, // 한국어
        { "zh-Hans", brls::FONT_CHINESE_SIMPLIFIED, "\u7B80\u4F53\u4E2D\u6587", "\u0378\u0378\u0378\u0378" }, // 简体中文
        { "zh-Hant", brls::FONT_CHINESE_TRADITIONAL, "\u7E41\u9AD4\u4E2D\u6587", "\u0378\u0378\u0378\u0378" }, // 繁體中文
        { "ja", brls::FONT_JAPANESE, "\u65E5\u672C\u8A9E\u3042", "\u0378\u0378\u0378\u0378" }, // 日本語あ
    };
    for (const Case& c : cases)
    {
        brls::DesktopFontLoader::FontFile file = brls::DesktopFontLoader::findSystemFont(c.language);
        if (file.path.empty())
        {
            std::printf("cjk_from_the_system_fonts: no %s font on this system, not checked\n", c.language);
            continue;
        }
        BRLS_CHECK(brls::Application::getFont(c.font) != brls::FONT_INVALID);
        auto* text    = lineOf(c.text);
        auto* missing = lineOf(c.missing);
        h.push(new brls::Activity(screenOf({ text, missing })));
        h.settle();
        if (!drawnDifferently(h, text, missing))
            std::printf("cjk_from_the_system_fonts: %s drawn as missing glyphs (%s)\n", c.language, file.path.c_str());
        BRLS_CHECK(drawnDifferently(h, text, missing));
    }
}

// ---------------------------------------------------------------------------
// The focus announcer: what a screen reader would be given
// ---------------------------------------------------------------------------

static const std::string& announced()
{
    return brls::Application::getLastAnnouncement();
}

BRLS_TEST(announcer_says_role_value_state_and_place)
{
    Harness h;
    auto* toggle = new brls::BooleanCell();
    toggle->init("Wi-Fi", false, [](bool) {});
    auto* detail = new brls::DetailCell();
    detail->setText("Language");
    detail->setDetailText("English");
    auto* radio = new brls::RadioCell();
    radio->title->setText("Dark");
    radio->setSelected(true);
    auto* slider = new brls::SliderCell();
    slider->init("Volume", 0.5f, [](float) {});
    slider->slider->setDiscreteStep(0.1f);
    std::vector<std::string> said;
    brls::Application::setFocusAnnouncer([&said](const std::string& text)
        { said.push_back(text); });
    h.push(new brls::Activity(screenOf({ toggle, detail, radio, slider })));
    h.settle();

    BRLS_CHECK_EQ(announced(), std::string("Wi-Fi, switch, off, 1 of 3"));
    h.tap(brls::BUTTON_A); // flipped in place: its new state alone
    BRLS_CHECK(toggle->isOn());
    BRLS_CHECK_EQ(announced(), std::string("on"));
    h.tap(brls::BUTTON_DOWN);
    BRLS_CHECK_EQ(announced(), std::string("Language, English, 2 of 3"));
    detail->setDetailText("Français"); // changed while focused, after a press
    BRLS_CHECK_EQ(announced(), std::string("Français"));
    h.advance(3000);
    detail->setDetailText("Deutsch"); // on its own: not read out
    BRLS_CHECK_EQ(announced(), std::string("Français"));
    h.tap(brls::BUTTON_DOWN);
    BRLS_CHECK_EQ(announced(), std::string("Dark, selected, 3 of 3"));
    h.tap(brls::BUTTON_DOWN);
    BRLS_CHECK_EQ(announced(), std::string("Volume, slider, 50%"));
    h.tap(brls::BUTTON_RIGHT);
    BRLS_CHECK_EQ(announced(), std::string("60%"));
    // Gliding while held: said once, when let go
    slider->slider->setDiscreteStep(0);
    size_t before = said.size();
    h.hold(brls::BUTTON_RIGHT, 400);
    h.step(2);
    BRLS_CHECK_EQ(said.size(), before + 1);
    BRLS_CHECK(announced() != "60%" && announced().back() == '%');
    // Each went to the announcer as it was said.
    BRLS_CHECK(!said.empty() && said.back() == announced());
    brls::Application::setFocusAnnouncer(nullptr);
}

BRLS_TEST(announcer_says_disabled_and_explicit_values)
{
    Harness h;
    auto* first  = new brls::Button();
    auto* second = new brls::Button();
    first->setText("Save");
    second->setText("Quit");
    second->setStyleState(brls::StyleState::DISABLED, true);
    auto* box = new brls::Box();
    box->setFocusable(true);
    box->setAccessibilityLabel("Battery");
    box->setAccessibilityValue("80%");
    h.push(new brls::Activity(screenOf({ first, second, box })));
    h.settle();
    BRLS_CHECK_EQ(announced(), std::string("Save, button, 1 of 3"));
    BRLS_CHECK_EQ(second->getAccessibilityDescription(), std::string("Quit, button, disabled, 2 of 3"));
    // No role: no place said either.
    BRLS_CHECK_EQ(box->getAccessibilityDescription(), std::string("Battery, 80%"));
}

BRLS_TEST(focusable_box_says_the_texts_in_it)
{
    Harness h;
    auto* tile = new brls::Box(brls::Axis::COLUMN);
    tile->setFocusable(true);
    auto* name = new brls::Label();
    name->setText("Mario Kart");
    auto* hidden = new brls::Label();
    hidden->setText("never said");
    hidden->setVisibility(brls::Visibility::GONE);
    auto* time = new brls::Label();
    time->setText("2 h 10");
    auto* inner = new brls::Button(); // speaks for itself
    inner->setText("Play");
    tile->addView(name);
    tile->addView(hidden);
    tile->addView(time);
    tile->addView(inner);
    auto* silent = new brls::Box();
    silent->setFocusable(true);
    silent->setWidth(40);
    silent->setHeight(40);
    h.push(new brls::Activity(screenOf({ tile, silent })));
    h.settle();
    BRLS_CHECK_EQ(tile->getAccessibilityDescription(), std::string("Mario Kart, 2 h 10"));
    BRLS_CHECK_EQ(silent->getAccessibilityDescription(), std::string(""));
}

BRLS_TEST(toast_and_dialog_are_announced)
{
    Harness h;
    auto* button = new brls::Button();
    button->setText("Delete");
    h.push(new brls::Activity(screenOf({ button })));
    h.settle();
    BRLS_CHECK_EQ(announced(), std::string("Delete, button"));

    brls::Application::notify("Profile saved");
    BRLS_CHECK_EQ(announced(), std::string("Profile saved"));

    auto* dialog = new brls::Dialog("Delete this profile?");
    dialog->addButton("Cancel", [] {});
    dialog->addButton("Delete", [] {});
    dialog->open();
    h.settle();
    const std::string opened = announced();
    BRLS_CHECK(opened.rfind("Delete this profile?, dialog, ", 0) == 0);
    BRLS_CHECK(opened.find(", button") != std::string::npos);
    // Within the dialog, only the button.
    h.tap(brls::BUTTON_RIGHT);
    BRLS_CHECK(announced().rfind("Delete this profile?", 0) != 0);
    h.tap(brls::BUTTON_B);
    BRLS_CHECK_EQ(announced(), std::string("Delete, button"));
}

BRLS_TEST(dropdown_says_its_title_and_the_choice)
{
    Harness h;
    auto* selector = new brls::SelectorCell();
    selector->init("Region", { "Europe", "America", "Asia" }, 1, [](int) {});
    h.push(new brls::Activity(screenOf({ selector })));
    h.settle();
    BRLS_CHECK_EQ(announced(), std::string("Region, America"));
    h.tap(brls::BUTTON_A);
    h.settle();
    BRLS_CHECK_EQ(announced(), std::string("Region, list, America, selected, 2 of 3"));
    h.tap(brls::BUTTON_DOWN);
    BRLS_CHECK_EQ(announced(), std::string("Asia, 3 of 3"));
}

BRLS_TEST(announcer_speaks_the_app_language)
{
    Harness h;
    auto* toggle = new brls::BooleanCell();
    toggle->init("Wi-Fi", true, [](bool) {});
    auto* other = new brls::Button();
    other->setText("OK");
    other->setStyleState(brls::StyleState::DISABLED, true);
    h.push(new brls::Activity(screenOf({ toggle, other })));
    h.settle();

    brls::loadTranslations("fr");
    BRLS_CHECK_EQ(toggle->getAccessibilityDescription(), std::string("Wi-Fi, interrupteur, activé, 1 sur 2"));
    BRLS_CHECK_EQ(other->getAccessibilityDescription(), std::string("OK, bouton, indisponible, 2 sur 2"));
    // A language that says the count first
    brls::loadTranslations("ja");
    BRLS_CHECK_EQ(toggle->getAccessibilityDescription(), std::string("Wi-Fi, スイッチ, オン, 2項目中1番目"));
    brls::loadTranslations("en-US");
    BRLS_CHECK_EQ(toggle->getAccessibilityDescription(), std::string("Wi-Fi, switch, on, 1 of 2"));
}

namespace
{
// The demo's hooks (Cadran's theme, pickers) without its main screen
struct CadranHarness : Demo, Harness
{
};
} // namespace

BRLS_TEST(cadran_pieces_have_roles_and_labels)
{
    CadranHarness h;
    brls::Box* chip = cadran::kit::chip("+15 min");
    brls::Box* card = cadran::kit::card(true, false);
    card->addView(cadran::kit::label("Today", cadran::kit::TEXT_META));
    card->addView(cadran::kit::label("2 h 10", cadran::kit::TEXT_BODY));
    auto* toggle = new brls::BooleanCell();
    toggle->init("Notifications", true, [](bool) {});
    cadran::List::row(toggle, "bell"); // its detail hidden, a drawn switch instead
    auto* line = new brls::DetailCell();
    line->setText("Updates");
    line->setDetailText("3 new  ›");
    cadran::List::row(line, "download");
    h.push(new brls::Activity(screenOf({ chip, card, toggle, line })));
    h.settle();

    BRLS_CHECK_EQ(announced(), std::string("+15 min, button, 1 of 4"));
    BRLS_CHECK_EQ(card->getAccessibilityDescription(), std::string("Today, 2 h 10, 2 of 4"));
    BRLS_CHECK_EQ(toggle->getAccessibilityDescription(), std::string("Notifications, switch, on, 3 of 4"));
    BRLS_CHECK_EQ(line->getAccessibilityDescription(), std::string("Updates, 3 new, 4 of 4"));
}

BRLS_TEST(cadran_picker_says_its_title_and_the_choice)
{
    CadranHarness h;
    auto* button = new brls::Button();
    button->setText("Theme");
    h.push(new brls::Activity(screenOf({ button })));
    h.settle();
    ui::pick("Theme", { "System", "Light", "Dark" }, 2, [](int) {});
    h.settle();
    BRLS_CHECK_EQ(announced(), std::string("Theme, list, Dark, selected, 3 of 3"));
    h.tap(brls::BUTTON_UP);
    BRLS_CHECK_EQ(announced(), std::string("Light, 2 of 3"));
}

BRLS_TEST(every_focusable_demo_view_says_something)
{
    DemoHarness h;
    BRLS_CHECK(announced().rfind("Cadran, tab, selected, 1 of ", 0) == 0);
    // Each tab of the rail in turn (focusing its item shows it)
    std::vector<std::string> silent;
    std::vector<std::string> seen;
    for (int tab = 0; tab < 20; tab++)
    {
        std::string name = h.focusText();
        if (std::find(seen.begin(), seen.end(), name) != seen.end())
            break;
        seen.push_back(name);
        h.settle();
        for (const std::string& view : h.silentFocusables())
            silent.push_back(name + ": " + view);
        h.tap(brls::BUTTON_DOWN);
    }
    BRLS_CHECK(seen.size() > 5);
    for (const std::string& name : silent)
        std::fprintf(stderr, "silent: %s\n", name.c_str());
    BRLS_CHECK(silent.empty());
}

int main(int argc, char** argv)
{
    return brls::test::runTests(argc, argv);
}

BRLS_TEST(switch_off_reads_as_secondary_text)
{
    Harness h;
    auto* cell = new brls::BooleanCell();
    cell->init("Switch", false, [](bool) {});
    h.push(new brls::Activity(screenOf({ cell })));
    h.settle();
    for (brls::ThemeVariant variant : { brls::ThemeVariant::LIGHT, brls::ThemeVariant::DARK })
    {
        brls::Application::getPlatform()->setThemeVariant(variant);
        h.step(2);
        brls::Theme theme = brls::Application::getTheme();
        NVGcolor off      = cell->detail->getTextColor();
        NVGcolor want     = theme["brls/text_secondary"];
        BRLS_CHECK(std::fabs(off.r - want.r) < 0.01f && std::fabs(off.g - want.g) < 0.01f && std::fabs(off.b - want.b) < 0.01f);
        BRLS_CHECK(brls::Theme::contrast(off, theme["brls/background"]) >= 4.5f);
    }
}

BRLS_TEST(every_demo_theme_passes_the_contrast_check)
{
    DemoHarness h;
    for (brls::ThemeVariant variant : { brls::ThemeVariant::LIGHT, brls::ThemeVariant::DARK })
        BRLS_CHECK(brls::Theme::checkContrast(variant).empty());
    for (const brls::ThemeInfo& info : brls::Theme::listThemes("@res/themes", true))
    {
        for (const std::string& warning : info.warnings)
            std::printf("%s: %s\n", info.path.c_str(), warning.c_str());
        BRLS_CHECK(info.ok() && info.warnings.empty());
    }
}

static std::string xmlError(const std::string& name, const std::string& xml)
{
    std::filesystem::path file = std::filesystem::temp_directory_path() / name;
    std::ofstream(file) << xml;
    try
    {
        brls::View::createFromXMLFile(file.string());
    }
    catch (const std::logic_error& e)
    {
        std::filesystem::remove(file);
        return e.what();
    }
    std::filesystem::remove(file);
    return "";
}

BRLS_TEST(xml_errors_say_where_and_what_was_meant)
{
    Harness h;
    std::string typo = xmlError("aurore_typo.xml", "<brls:Box>\n\n    <brls:Label fontSise=\"20\"/>\n</brls:Box>\n");
    BRLS_CHECK(typo.find("aurore_typo.xml:3: <brls:Label>: Unknown XML attribute \"fontSise\"") != std::string::npos);
    BRLS_CHECK(typo.find("did you mean \"fontSize\"?") != std::string::npos);

    std::string value = xmlError("aurore_value.xml", "<brls:Box\n    axis=\"colum\"/>\n");
    BRLS_CHECK(value.find("aurore_value.xml:1: <brls:Box>: Illegal value \"colum\" for XML attribute \"axis\"; expected column, row") != std::string::npos);

    std::string tag = xmlError("aurore_tag.xml", "<brls:Box>\n<brls:Lable/>\n</brls:Box>\n");
    BRLS_CHECK(tag.find("aurore_tag.xml:2: <brls:Lable>: Unknown XML tag \"brls:Lable\"; did you mean \"brls:Label\"?") != std::string::npos);
}

BRLS_TEST(reduce_motion_stops_core_animations)
{
    Harness h;
    brls::Application::setReduceMotion(true);
    auto* cell = new brls::BooleanCell();
    cell->init("Switch", false, [](bool) {});
    h.push(new brls::Activity(screenOf({ cell })));
    h.settle();
    h.tap(brls::BUTTON_A);
    BRLS_CHECK(cell->isOn());
    BRLS_CHECK(brls::Ticking::runningTickings.empty() || std::none_of(brls::Ticking::runningTickings.begin(), brls::Ticking::runningTickings.end(), [](brls::Ticking* t)
                   { return dynamic_cast<brls::FiniteTicking*>(t) != nullptr; }));

    // A text cut short stays still while focused
    auto* row   = new brls::Box(brls::Axis::ROW);
    auto* label = new brls::Label();
    label->setText("A text far too long for the narrow line it is written on");
    label->setSingleLine(true);
    row->setWidth(200);
    row->setFocusable(true);
    row->addView(label);
    brls::setHighlightAnimationStill(true); // the focus ring's pulse aside
    h.push(new brls::Activity(screenOf({ row })));
    h.settle();
    h.step(120);
    brls::test::Image before = h.screenshot();
    h.step(120);
    BRLS_CHECK_EQ(brls::test::compareImages(h.screenshot(), before).differingPixels, 0);
}

BRLS_TEST(rail_names_are_not_cut)
{
    DemoHarness h;
    for (const std::string& text : h.cutTexts())
        BRLS_CHECK(text != "Basic components" && text != "Layout and alignment");
}

BRLS_TEST(hold_to_confirm_says_it_once)
{
    DemoHarness h;
    // Opened by a finger: no A key bubble
    BRLS_CHECK(h.tapView(h.findText("Cadran")));
    ui::hold_confirm(cadran::dialog_body(brls::getStr("demo/cadran/hold_text")), brls::getStr("demo/cadran/hold"), [] {});
    h.settle();
    BRLS_CHECK(!h.hasText("A"));
    BRLS_CHECK(!h.hasText("Reset everything?\n\nHold the button for 2 seconds."));
    BRLS_CHECK(h.matchesReference("hold_confirm_touch"));
}

BRLS_TEST(cut_text_file_written_with_cadran)
{
    std::filesystem::path dir = std::filesystem::temp_directory_path() / "aurore_cut_text";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    std::filesystem::path previous = std::filesystem::current_path();
    std::filesystem::current_path(dir);
    setenv("BRLS_CUT_TEXT", "1", 1);
    {
        DemoHarness h;
        auto* cut = new brls::Label();
        cut->setText("A sentence far too long for the narrow box it is given");
        cut->setSingleLine(true);
        cut->setWidth(120);
        h.push(new brls::Activity(screenOf({ cut })));
        h.settle();
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        h.step(2);
        std::ifstream file(dir / "cut_text.txt");
        std::string all((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        BRLS_CHECK(all.find("A sentence far too long for the narrow box it is given") != std::string::npos);
    }
    unsetenv("BRLS_CUT_TEXT");
    std::filesystem::current_path(previous);
    std::filesystem::remove_all(dir);
}

BRLS_TEST(settings_tab_keeps_the_cards_margins)
{
    DemoHarness h;
    BRLS_CHECK(h.focusByText("Settings"));
    h.tap(brls::BUTTON_RIGHT);
    h.settle();
    brls::View* line = h.focus();
    BRLS_CHECK(line != nullptr);
    if (line)
    {
        brls::Rect frame = line->getFrame();
        // Within the margins of Cadran's pages, the focus ring off the window's edge
        BRLS_CHECK(frame.getMaxX() <= 1280 - 40 + 1);
        BRLS_CHECK(frame.getMinY() >= 84 + 28 - 1);
    }
    BRLS_CHECK(h.matchesReference("settings_tab"));
}
