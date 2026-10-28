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
// AURORE_TEST_UPDATE=1 writes them again.

#include <aurore.hpp>
#include <aurore/cadran/ui/cadran_kit.hpp>
#include <aurore/cadran/ui/cadran_list.hpp>
#include <aurore/cadran/ui/cadran_modal.hpp>
#include <aurore/cadran/ui/hooks.hpp>
#include <aurore/cadran/ui/live_prefs.hpp>
#include <aurore/cadran/ui/prefs.hpp>
#include <aurore/cadran/ui/text_size.hpp>
#include <aurore/cadran/ui/ui.hpp>
#include <aurore/cadran/view/cadran_icon.hpp>
#include <aurore/core/hot_reload.hpp>
#include <aurore/core/theme_decor.hpp>
#include <aurore/core/theme_image.hpp>
#include <aurore/core/util.hpp>
#include <aurore/platforms/desktop/desktop_font.hpp>
#ifdef __GLFW__
#include <aurore/platforms/glfw/glfw_ime.hpp>
#endif
#include <aurore/platforms/desktop/desktop_platform.hpp>
#include <aurore/test/harness.hpp>
#include <aurore/views/edit_text_dialog.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <thread>
#ifndef _WIN32
#include <sys/stat.h>
#endif

#include "activity/main_activity.hpp"
#include "demo_app.hpp"

using aurore::test::Harness;

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
class Swatch : public aurore::View
{
  public:
    Swatch()
    {
        setWidth(120);
        setHeight(80);
    }

    void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style, aurore::FrameContext* ctx) override
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
    while (aurore::ThemeImages::decoding() > 0)
    {
        if (std::chrono::steady_clock::now() > deadline)
            return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

aurore::Box* screenOf(std::initializer_list<aurore::View*> views)
{
    auto* box = new aurore::Box(aurore::Axis::COLUMN);
    box->setPadding(40, 40, 40, 40);
    box->setBackgroundColor(nvgRGB(20, 20, 30));
    for (aurore::View* view : views)
        box->addView(view);
    return box;
}

} // namespace

AURORE_TEST(main_screen)
{
    DemoHarness h;
    AURORE_CHECK_EQ(h.focusText(), std::string("Cadran"));
    AURORE_CHECK(h.hasText("Aurore Demo"));
    AURORE_CHECK(h.hasText("Monday, January 5")); // the harness' wall clock
    AURORE_CHECK(h.matchesReference("main_screen"));
}

AURORE_TEST(missing_reference_fails_in_ci)
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
    AURORE_CHECK(!matched);
    AURORE_CHECK(!std::filesystem::exists(h.options().referenceDir + "/no_such_reference.png"));
}

AURORE_TEST(rail_focus_moves)
{
    DemoHarness h;
    h.clearFocusLog();
    h.tap(aurore::BUTTON_DOWN, 2);
    AURORE_CHECK_EQ(h.focusText(), std::string("Layout and alignment"));
    h.tap(aurore::BUTTON_UP);
    AURORE_CHECK_EQ(h.focusText(), std::string("Basic components"));
    std::vector<std::string> expected = { "Basic components", "Layout and alignment", "Basic components" };
    AURORE_CHECK(h.focusLog() == expected);
    AURORE_CHECK(h.focusByText("Settings"));
    AURORE_CHECK(!h.focusByText("No such tab", aurore::BUTTON_DOWN, 3));
}

AURORE_TEST(keyboard_moves_focus)
{
    DemoHarness h;
    h.tapKey(aurore::AURORE_KBD_KEY_DOWN);
    AURORE_CHECK_EQ(h.focusText(), std::string("Basic components"));
    h.tapKey(aurore::AURORE_KBD_KEY_UP);
    AURORE_CHECK_EQ(h.focusText(), std::string("Cadran"));
}

AURORE_TEST(confirm_dialog_opens_and_closes)
{
    DemoHarness h;
    h.tap(aurore::BUTTON_RIGHT); // into the tab
    h.tap(aurore::BUTTON_DOWN);
    AURORE_CHECK_EQ(h.focusText(), std::string("Confirm"));
    size_t screens = aurore::Application::getActivitiesStack().size();

    h.tap(aurore::BUTTON_A);
    AURORE_CHECK_EQ(aurore::Application::getActivitiesStack().size(), screens + 1);
    AURORE_CHECK(dynamic_cast<aurore::Dialog*>(h.top()->getContentView()) != nullptr);
    AURORE_CHECK(h.matchesReference("confirm_dialog"));

    h.tap(aurore::BUTTON_B);
    AURORE_CHECK_EQ(aurore::Application::getActivitiesStack().size(), screens);
    AURORE_CHECK_EQ(h.focusText(), std::string("Confirm"));
}

AURORE_TEST(touch_opens_a_tab)
{
    DemoHarness h;
    AURORE_CHECK(h.tapView(h.findText("Layout and alignment")));
    AURORE_CHECK(h.hasText("alignItems: center"));
    AURORE_CHECK(aurore::Application::getInputType() == aurore::InputType::TOUCH);
    AURORE_CHECK(h.matchesReference("layout_tab"));
}

AURORE_TEST(window_resized)
{
    DemoHarness h;
    h.resize(1920, 1080);
    aurore::test::Image shot = h.screenshot();
    AURORE_CHECK_EQ(shot.width, 1920);
    AURORE_CHECK_EQ(shot.height, 1080);
    AURORE_CHECK_EQ(aurore::Application::contentWidth, 1280.0f);
    AURORE_CHECK(h.matchesReference("main_screen_1080p"));
}

AURORE_TEST(cut_texts_found)
{
    Harness h;
    auto* cut = new aurore::Label();
    cut->setText("A sentence far too long for the narrow box it is given");
    cut->setSingleLine(true);
    cut->setWidth(120);
    auto* whole = new aurore::Label();
    whole->setText("Short");
    h.push(new aurore::Activity(screenOf({ cut, whole })));
    h.settle();
    std::vector<std::string> expected = { "A sentence far too long for the narrow box it is given" };
    AURORE_CHECK(h.cutTexts() == expected);
}

AURORE_TEST(timers_follow_the_simulated_clock)
{
    Harness h;
    bool fired = false;
    aurore::delay(500, [&fired]()
        { fired = true; });
    h.advance(400);
    AURORE_CHECK(!fired);
    h.advance(150);
    AURORE_CHECK(fired);

    // An animation ends after its duration on the clock, in frames
    aurore::Animatable value;
    value.reset(0.0f);
    value.addStep(1.0f, 300);
    value.start();
    h.advance(150);
    AURORE_CHECK(value.getValue() > 0.0f && value.getValue() < 1.0f);
    AURORE_CHECK(h.settle());
    AURORE_CHECK_EQ(value.getValue(), 1.0f);
}

AURORE_TEST(redraw_on_demand_skips_still_frames)
{
    Harness h;
    auto* first  = new aurore::Button();
    auto* second = new aurore::Button();
    first->setText("First");
    second->setText("Second");
    h.push(new aurore::Activity(screenOf({ first, second })));
    aurore::Application::setRedrawOnDemand(true);
    h.settle();
    h.step(3);

    // Still: no frame drawn but the one a second
    uint64_t drawn = aurore::Application::getDrawnFrameCount();
    h.advance(900);
    AURORE_CHECK(aurore::Application::getDrawnFrameCount() - drawn <= 1);

    // A press: drawn
    h.tap(aurore::BUTTON_DOWN);
    AURORE_CHECK(aurore::Application::getDrawnFrameCount() - drawn > 2);
    AURORE_CHECK_EQ(h.focusText(), std::string("Second"));
}

AURORE_TEST(theme_change_draws_at_once_on_demand)
{
    Harness h;
    h.push(new aurore::Activity(screenOf({ new aurore::Button() })));
    aurore::Application::setRedrawOnDemand(true);
    h.settle();
    h.step(3);

    // A theme loaded from code, nothing else asking for a frame: drawn at
    // the next iteration, not at the next periodic redraw
    uint64_t drawn = aurore::Application::getDrawnFrameCount();
    aurore::Theme::loadFile("builtin:high-contrast-dark");
    h.step();
    AURORE_CHECK(aurore::Application::getDrawnFrameCount() > drawn);

    drawn = aurore::Application::getDrawnFrameCount();
    aurore::Theme::reset();
    h.step();
    AURORE_CHECK(aurore::Application::getDrawnFrameCount() > drawn);
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

AURORE_TEST(seasonal_theme_picks_the_preset_of_the_date)
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
        AURORE_CHECK_EQ(aurore::Theme::seasonalTheme(noonOf(date.year, date.month, date.day)),
            std::string("builtin:") + date.preset);

    bool listed = false;
    for (const aurore::ThemeInfo& info : aurore::Theme::builtinThemes())
        if (info.path == "builtin:seasonal")
            listed = info.ok() && info.warnings.empty() && info.light && info.dark && info.name == "Seasonal";
    AURORE_CHECK(listed);
}

AURORE_TEST(seasonal_theme_changes_at_midnight)
{
    // 16 October 2026, 15 seconds before Halloween's first day
    aurore::test::Options options;
    options.wallTime = noonOf(2026, 10, 16) + 12 * 3600 - 15;
    Harness h(options);
    h.push(new aurore::Activity(screenOf({ new aurore::Button() })));
    aurore::Theme::loadFile("builtin:seasonal");
    h.settle();
    NVGcolor autumn;
    aurore::Theme::parseColor("#1a1310", &autumn);
    NVGcolor background = aurore::Theme::getDarkTheme()["aurore/background"];
    AURORE_CHECK(background.r == autumn.r && background.g == autumn.g && background.b == autumn.b);
    aurore::test::Image before = h.screenshot();

    h.advance(20000);
    NVGcolor halloween;
    aurore::Theme::parseColor("#150d1f", &halloween);
    background = aurore::Theme::getDarkTheme()["aurore/background"];
    AURORE_CHECK(background.r == halloween.r && background.g == halloween.g && background.b == halloween.b);
    AURORE_CHECK_EQ(aurore::Theme::getCurrentPath(), std::string("builtin:seasonal"));
    // Drawn again: the button and its focus in the new colours
    AURORE_CHECK(aurore::test::compareImages(h.screenshot(), before).differingPixels > 10000);
}

AURORE_TEST(every_preset_draws_the_demo_in_both_variants)
{
    DemoHarness h;
    for (const aurore::ThemeInfo& info : aurore::Theme::builtinThemes())
        for (aurore::ThemeVariant variant : { aurore::ThemeVariant::LIGHT, aurore::ThemeVariant::DARK })
        {
            if (!info.styles(variant))
                continue;
            AURORE_CHECK(aurore::Theme::loadFile(info.path));
            aurore::Application::getPlatform()->setThemeVariant(variant);
            h.step(2);
            AURORE_CHECK(aurore::Theme::checkContrast(variant).empty());
        }
    aurore::Theme::reset();
    h.step(2);
}

AURORE_TEST(theme_picker_follows_a_one_variant_palette)
{
    DemoHarness h;
    ui::prefs().theme      = "dark";
    ui::prefs().theme_file = "builtin:high-contrast-light";
    live_prefs::apply_theme();
    h.settle();
    // The palette has the light variant only: shown, and what is saved
    AURORE_CHECK(aurore::Application::getThemeVariant() == aurore::ThemeVariant::LIGHT);
    AURORE_CHECK_EQ(ui::prefs().theme, std::string("light"));

    // Dark asked for: the palette's counterpart, in the dark variant
    ui::pick_theme();
    h.settle();
    AURORE_CHECK(h.focusByText("Dark"));
    h.tap(aurore::BUTTON_A);
    h.settle();
    AURORE_CHECK_EQ(ui::prefs().theme, std::string("dark"));
    AURORE_CHECK_EQ(ui::prefs().theme_file, std::string("builtin:high-contrast-dark"));
    AURORE_CHECK(aurore::Application::getThemeVariant() == aurore::ThemeVariant::DARK);

    // A light-only palette picked: its variant
    ui::prefs().theme_file = "";
    live_prefs::apply_theme();
    ui::pick_palette();
    h.settle();
    AURORE_CHECK(h.focusByText("Sand"));
    h.tap(aurore::BUTTON_A);
    h.settle();
    AURORE_CHECK_EQ(ui::prefs().theme, std::string("light"));
    AURORE_CHECK(aurore::Application::getThemeVariant() == aurore::ThemeVariant::LIGHT);

    // Dark again: Sand has no counterpart, back to the boards' palettes
    ui::pick_theme();
    h.settle();
    AURORE_CHECK(h.focusByText("Dark"));
    h.tap(aurore::BUTTON_A);
    h.settle();
    AURORE_CHECK_EQ(ui::prefs().theme_file, std::string(""));
    AURORE_CHECK(aurore::Application::getThemeVariant() == aurore::ThemeVariant::DARK);
}

AURORE_TEST(theme_icon_is_read_and_inherited)
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

    AURORE_CHECK_EQ(aurore::Theme::inspect(own).icon, std::string("leaf"));
    AURORE_CHECK(aurore::Theme::inspect(own).warnings.empty() && aurore::Theme::inspect(own).ok());
    AURORE_CHECK_EQ(aurore::Theme::inspect(none).icon, std::string(""));
    AURORE_CHECK_EQ(aurore::Theme::inspect(inherits).icon, std::string("wave"));
    AURORE_CHECK_EQ(aurore::Theme::inspect(overrides).icon, std::string("tree"));
    AURORE_CHECK_EQ(aurore::Theme::inspect(includes).icon, std::string("gem"));
    fs::remove_all(dir);

    // Every preset has its motif
    for (const aurore::ThemeInfo& info : aurore::Theme::builtinThemes())
        AURORE_CHECK(!info.icon.empty());
}

AURORE_TEST(seasonal_palette_shows_the_motif_of_the_date)
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
        aurore::setSimulatedWallTime(noonOf(2026, date.month, date.day));
        std::string icon;
        for (const aurore::ThemeInfo& info : aurore::Theme::builtinThemes())
            if (info.path == "builtin:seasonal")
                icon = info.icon;
        AURORE_CHECK_EQ(icon, std::string(date.icon));
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
    AURORE_CHECK(h.hasText("Ocean"));
    AURORE_CHECK(h.matchesReference(reference));
}

AURORE_TEST(palette_picker_shows_swatches)
{
    pickPalette("dark", "palette_picker");
}

AURORE_TEST(palette_picker_shows_swatches_light)
{
    pickPalette("light", "palette_picker_light");
}

AURORE_TEST(redraw_debug_finds_a_missing_invalidation)
{
    Harness h;
    auto* swatch = new Swatch();
    h.push(new aurore::Activity(screenOf({ swatch })));
    aurore::Application::setRedrawOnDemand(true);
    aurore::Application::setRedrawDebug(true);
    h.settle();
    h.step(3);
    AURORE_CHECK_EQ(aurore::Application::getMissedRedrawCount(), (uint64_t)0);

    // Changed, and said so: drawn, nothing missed
    swatch->color = nvgRGB(40, 200, 40);
    aurore::Application::requestRedraw();
    h.step(3);
    AURORE_CHECK_EQ(aurore::Application::getMissedRedrawCount(), (uint64_t)0);

    // Changed without a word: the skipped frames differ, over the swatch
    swatch->color = nvgRGB(40, 40, 200);
    h.step(3);
    AURORE_CHECK(aurore::Application::getMissedRedrawCount() > 0);
    aurore::Rect area  = aurore::Application::getLastMissedRedrawArea();
    aurore::Rect frame = swatch->getFrame();
    AURORE_CHECK(std::fabs(area.getMinX() - frame.getMinX()) <= 1 && std::fabs(area.getMinY() - frame.getMinY()) <= 1);
    AURORE_CHECK(std::fabs(area.getWidth() - frame.getWidth()) <= 2 && std::fabs(area.getHeight() - frame.getHeight()) <= 2);
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
        aurore::test::Image image;
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
        aurore::test::Image image;
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

static bool hasErrorWith(const aurore::ThemeInfo& info, const std::string& text)
{
    for (const std::string& error : info.errors)
        if (error.find(text) != std::string::npos)
            return true;
    return false;
}

AURORE_TEST(theme_pictures_stay_in_their_folder)
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
    std::vector<aurore::ThemeInfo> listed = aurore::Theme::listThemes(root);
    AURORE_CHECK_EQ(listed.size(), (size_t)2);
    for (const aurore::ThemeInfo& info : listed)
        AURORE_CHECK(info.ok());
    AURORE_CHECK_EQ(listed[0].id, std::string("aurora"));
    AURORE_CHECK_EQ(listed[0].path, root + "/aurora/theme.theme");
    AURORE_CHECK_EQ(listed[0].darkBackground.image, root + "/aurora/sky/night.png");
    AURORE_CHECK_EQ(listed[1].id, std::string("single"));
    AURORE_CHECK_EQ(listed[1].backgroundImage, root + "/night.png");

    // Out of the theme's folder: refused, an error, never a crash
    std::string path, error;
    const std::string folder = root + "/aurora";
    for (const char* name : { "../outside.png", "sky/../../outside.png", "/etc/passwd", "sdmc:/x.png", "C:/x.png",
             "@res/img/demo_icon.jpg", "sky\\night.png", "..", "." })
    {
        AURORE_CHECK(!aurore::Theme::resolveAsset(name, folder, folder, &path, &error));
        AURORE_CHECK(!error.empty());
    }
    AURORE_CHECK(aurore::Theme::resolveAsset("./sky/x/../night.png", folder, folder, &path, &error));
    AURORE_CHECK_EQ(path, folder + "/sky/night.png");
    // From an included file in a subfolder, still in the theme's folder
    AURORE_CHECK(aurore::Theme::resolveAsset("../sky/night.png", folder + "/parts", folder, &path, &error));
    AURORE_CHECK_EQ(path, folder + "/sky/night.png");
    // A preset's pictures are the app's resources
    AURORE_CHECK(aurore::Theme::resolveAsset("@res/img/demo_icon.jpg", "builtin:", "builtin:", &path, &error));
    AURORE_CHECK(!aurore::Theme::resolveAsset("night.png", "builtin:", "builtin:", &path, &error));

    const std::string escapes = themes.write("aurora2/theme.theme",
        "name = Escapes\nbackground_image = ../outside.png\n[light]\nbackground_image = /etc/passwd\n");
    aurore::ThemeInfo info    = aurore::Theme::inspect(escapes);
    AURORE_CHECK(hasErrorWith(info, "goes out of the theme's folder"));
    AURORE_CHECK(hasErrorWith(info, "an absolute path is not allowed"));
    AURORE_CHECK(!info.darkBackground.hasImage() && !info.lightBackground.hasImage());

#ifndef _WIN32
    // A link in the folder to a file out of it
    std::filesystem::create_symlink(themes.dir / "outside.png", themes.dir / "aurora" / "link.png");
    AURORE_CHECK(!aurore::Theme::resolveAsset("link.png", folder, folder, &path, &error));
    AURORE_CHECK(error.find("link") != std::string::npos);
#endif

    // A picture that is not there, or not a picture
    themes.write("broken.jpg", "not a jpeg");
    AURORE_CHECK(hasErrorWith(aurore::Theme::inspect(themes.write("missing.theme", "background_image = nowhere.png\n")),
        "cannot read"));
    // A file that is not a picture: told by a check that decodes it only
    const std::string broken = themes.write("broken.theme", "background_image = broken.jpg\n");
    AURORE_CHECK(aurore::Theme::inspect(broken).ok());
    AURORE_CHECK(hasErrorWith(aurore::Theme::inspect(broken, true), "cannot read"));
    // Used anyway: nothing drawn, no crash
    AURORE_CHECK(aurore::Theme::loadFile(root + "/broken.theme"));
    h.settle();
    AURORE_CHECK(!aurore::Application::hasThemeBackground());
    aurore::Theme::reset();
}

// A file that includes itself, or many files each including the next ones
// many times: read once each at most, quickly, with errors that say where
AURORE_TEST(theme_include_fanout_is_bounded)
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

    auto start                  = std::chrono::steady_clock::now();
    aurore::ThemeInfo fanInfo   = aurore::Theme::inspect(fan);
    aurore::ThemeInfo wideInfo  = aurore::Theme::inspect(wide);
    aurore::ThemeInfo loopInfo  = aurore::Theme::inspect(loop);
    aurore::ThemeInfo oftenInfo = aurore::Theme::inspect(oftenPath);
    AURORE_CHECK(aurore::Theme::loadFile(wide));
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    AURORE_CHECK(seconds < 5.0);

    AURORE_CHECK(hasErrorWith(fanInfo, "self.inc:2: @include self.inc includes itself (a loop)"));
    AURORE_CHECK(fanInfo.errors.size() <= 10);
    AURORE_CHECK(hasErrorWith(wideInfo, "part7.inc:1: @include part8.inc goes too deep"));
    AURORE_CHECK_EQ(wideInfo.errors.size(), (size_t)101);
    AURORE_CHECK(hasErrorWith(wideInfo, "more errors"));
    AURORE_CHECK(hasErrorWith(oftenInfo, "line 66: @include leaf.inc: skipped, the theme reads more than 64 files"));
    AURORE_CHECK(!hasErrorWith(oftenInfo, "line 65:"));
    AURORE_CHECK(hasErrorWith(loopInfo, "inherit loop.theme inherits itself (a loop)"));
    aurore::Theme::reset();
}

// @include, inherit and counterpart read files of the theme's folder only, as
// its pictures; a resource theme still includes resources
AURORE_TEST(theme_includes_stay_in_their_folder)
{
    Harness h;
    ThemeFolder themes("aurore_theme_include_confined_test");
    const std::string root = themes.dir.generic_string();
    themes.picture("other/pic.png", 200, 20, 40);
    themes.write("other/base.theme", "name = Base\nbackground_image = pic.png\naccent = #ff0000\n");
    themes.write("outside.inc", "accent = #ff0000\n");
    themes.write("mine/sibling.theme", "name = Sibling\naccent = #00ff00\n");

    const std::string absolute = themes.write("mine/abs.theme", "name = Abs\n@include /etc/passwd\n");
    aurore::ThemeInfo info     = aurore::Theme::inspect(absolute);
    AURORE_CHECK(hasErrorWith(info, "line 2: @include /etc/passwd: an absolute path is not allowed"));
    AURORE_CHECK(!hasErrorWith(info, "no '='")); // nothing of the file read
    AURORE_CHECK(!hasErrorWith(info, "root:"));

    info = aurore::Theme::inspect(themes.write("mine/up.theme", "name = Up\n@include ../outside.inc\n"));
    AURORE_CHECK(hasErrorWith(info, "line 2: @include ../outside.inc: goes out of the theme's folder"));

    const std::string inherits = themes.write("mine/inherits.theme", "name = Inherits\ninherit = ../other/base.theme\n");
    info                       = aurore::Theme::inspect(inherits);
    AURORE_CHECK(hasErrorWith(info, "inherit ../other/base.theme: goes out of the theme's folder"));
    AURORE_CHECK(!info.darkBackground.hasImage());
    AURORE_CHECK(aurore::Theme::loadFile(inherits));
    AURORE_CHECK(!aurore::Theme::getBackground(aurore::ThemeVariant::DARK).hasImage());
    aurore::Theme::reset();

    info = aurore::Theme::inspect(themes.write("mine/res.theme", "name = Res\n@include @res/themes/shared.inc\n"));
    AURORE_CHECK(hasErrorWith(info, "not in the app's resources"));
    info = aurore::Theme::inspect(themes.write("mine/pair.theme", "name = Pair\ninherit = dark\ncounterpart = /etc/hostname\n"));
    AURORE_CHECK(hasErrorWith(info, "counterpart /etc/hostname: an absolute path is not allowed"));
    AURORE_CHECK(info.counterpart.empty());

    // Still fine: a sibling, a preset by name, a resource theme's includes
    AURORE_CHECK(aurore::Theme::inspect(themes.write("mine/ok.theme", "name = Ok\ninherit = sibling.theme\n")).ok());
    AURORE_CHECK(aurore::Theme::inspect(themes.write("mine/preset.theme", "name = P\ninherit = ocean\n")).ok());
    AURORE_CHECK(aurore::Theme::inspect("@res/themes/rose.theme").ok());

    // Too large, or not a file: not read
    themes.write("mine/big.inc", "# " + std::string(300 * 1024, 'x') + "\n");
    info = aurore::Theme::inspect(themes.write("mine/big.theme", "name = Big\n@include big.inc\n"));
    AURORE_CHECK(hasErrorWith(info, "@include big.inc: cannot read"));
    std::filesystem::create_directories(themes.dir / "mine" / "folder.inc");
    info = aurore::Theme::inspect(themes.write("mine/dir.theme", "name = Dir\n@include folder.inc\n"));
    AURORE_CHECK(hasErrorWith(info, "@include folder.inc: cannot read"));
#ifndef _WIN32
    // A pipe would block a reader for ever
    AURORE_CHECK(::mkfifo((themes.dir / "mine" / "pipe.inc").c_str(), 0600) == 0);
    info = aurore::Theme::inspect(themes.write("mine/pipe.theme", "name = Pipe\n@include pipe.inc\n"));
    AURORE_CHECK(hasErrorWith(info, "@include pipe.inc: cannot read"));

    // A link out of the folder, to a file or to a folder, there or not
    std::filesystem::create_symlink(themes.dir / "outside.inc", themes.dir / "mine" / "link.inc");
    std::filesystem::create_directory_symlink(themes.dir / "other", themes.dir / "mine" / "away");
    info = aurore::Theme::inspect(themes.write("mine/link.theme", "name = Link\n@include link.inc\n"));
    AURORE_CHECK(hasErrorWith(info, "a link out of the theme's folder"));
    std::string path, error;
    const std::string folder = root + "/mine";
    AURORE_CHECK(!aurore::Theme::resolveAsset("away/missing.png", folder, folder, &path, &error));
    AURORE_CHECK(error.find("a link out of the theme's folder") != std::string::npos);
    // A broken link: where it leads cannot be told, refused
    std::filesystem::create_symlink(themes.dir / "nowhere.png", themes.dir / "mine" / "broken.png");
    AURORE_CHECK(!aurore::Theme::resolveAsset("broken.png", folder, folder, &path, &error));
    // A file not there yet, in the folder: fine (inspect says it is missing)
    AURORE_CHECK(aurore::Theme::resolveAsset("later.png", folder, folder, &path, &error));
#endif
}

// Colours and metrics are finite numbers in their ranges
AURORE_TEST(theme_numbers_are_finite_and_in_range)
{
    Harness h;
    NVGcolor color;
    for (const char* bad : { "rgba(nan, 1, 1, 1)", "rgb(inf, 0, 0)", "rgb(1e39, 0, 0)", "rgb(256, 0, 0)", "rgb(-5, 0, 0)",
             "rgba(0, 0, 0, 1.5)", "rgba(0, 0, 0, -0.1)", "rgba(0, 0, 0, nan)", "rgb(0x1p9, 0, 0)" })
        AURORE_CHECK(!aurore::Theme::parseColor(bad, &color));
    AURORE_CHECK(aurore::Theme::parseColor("rgb(255, 0, 0)", &color) && color.r == 1.0f);
    AURORE_CHECK(aurore::Theme::parseColor("rgba(0, 0, 0, 0.5)", &color) && color.a == 0.5f);

    ThemeFolder themes("aurore_theme_numbers_test");
    aurore::ThemeInfo info = aurore::Theme::inspect(themes.write("nan.theme",
        "name = N\naccent = rgba(nan, 1e39, -5, inf)\n[metrics]\naurore/sidebar/width = nan\naurore/header/height = inf\n"
        "aurore/highlight/stroke_width = 1e39\naurore/sidebar/padding_top = -20000\naurore/sidebar/padding_left = 12\n"));
    AURORE_CHECK(!info.ok());
    AURORE_CHECK(hasErrorWith(info, "line 2: rgba(nan, 1e39, -5, inf) is not a colour"));
    for (const char* value : { "line 4: nan", "line 5: inf", "line 6: 1e39", "line 7: -20000" })
        AURORE_CHECK(hasErrorWith(info, value));
    AURORE_CHECK(!hasErrorWith(info, "line 8"));
}

// Errors quote the file: never a control character to the terminal, and at
// most 100 of them
AURORE_TEST(theme_errors_are_escaped_and_capped)
{
    Harness h;
    ThemeFolder themes("aurore_theme_errors_test");
    aurore::ThemeInfo info = aurore::Theme::inspect(themes.write("ansi.theme", "name = \x1b]0;title\x07\n\x1b[2Jred\naccent = \xc2\x9b"
                                                                               "31m\n"));
    AURORE_CHECK(hasErrorWith(info, "line 2: no '=' in \"\\x1B[2Jred\""));
    AURORE_CHECK(hasErrorWith(info, "\\x9B31m is not a colour"));
    for (const std::string& error : info.errors)
        for (unsigned char c : error)
            AURORE_CHECK(c >= 0x20 && c != 0x7F && c != 0x9B);
    AURORE_CHECK_EQ(aurore::escapeControlChars("a\tb\x1b"
                                               "c\xc2\x85"
                                               "d\xc3\xa9"),
        std::string("a\\x09b\\x1Bc\\x85d\xc3\xa9"));

    std::string many = "name = Many\n";
    for (int i = 0; i < 500; i++)
        many += "nonsense\n";
    info = aurore::Theme::inspect(themes.write("many.theme", many));
    AURORE_CHECK_EQ(info.errors.size(), (size_t)101);
    AURORE_CHECK_EQ(info.errors.back(), std::string("and 400 more errors"));
}

// "@res/…" stays in the resources: in hot reload's folder, when a theme is
// read and in an XML view's file paths
AURORE_TEST(resource_names_stay_in_the_resources)
{
    Harness h;
    AURORE_CHECK(aurore::isResourceNameSafe("@res/themes/rose.theme"));
    AURORE_CHECK(aurore::isResourceNameSafe("@res/a..b/c"));
    for (const char* name : { "@res/../x", "@res/themes/../../x", "@res/..", "@res/a\\..\\..\\x" })
        AURORE_CHECK(!aurore::isResourceNameSafe(name));

    ThemeFolder resources("aurore_resources_test");
    resources.write("themes/x.theme", "name = X\n");
    resources.write("secret.theme", "name = Secret\n");
    aurore::HotReload::setEnabled(true, (resources.dir / "themes").string());
    AURORE_CHECK(!aurore::HotReload::diskResource("@res/x.theme").empty());
    AURORE_CHECK(aurore::HotReload::diskResource("@res/../secret.theme").empty());
    AURORE_CHECK(!aurore::Theme::loadFile("@res/../secret.theme"));
    aurore::HotReload::setEnabled(false);

    // A file path attribute of an XML view
    AURORE_CHECK(!aurore::View::getFilePathXMLAttributeValue("@res/img/demo_icon.jpg").empty());
    AURORE_CHECK(aurore::View::getFilePathXMLAttributeValue("@res/../secret.theme").empty());
    AURORE_CHECK(aurore::View::getFilePathXMLAttributeValue("@res/img/../../secret.theme").empty());
    AURORE_CHECK_EQ(aurore::View::getFilePathXMLAttributeValue("/elsewhere/x.png"), std::string("/elsewhere/x.png"));

    // A language that is no locale's name is not a path either
    aurore::loadTranslations("../../etc");
    AURORE_CHECK_EQ(aurore::getStr("hints/ok"), std::string("OK"));
    aurore::loadTranslations(aurore::LOCALE_DEFAULT);
}

AURORE_TEST(theme_background_keys_are_read_and_inherited)
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

    aurore::ThemeInfo info = aurore::Theme::inspect(base);
    AURORE_CHECK(info.ok());
    AURORE_CHECK(info.lightBackground.image == info.darkBackground.image && info.lightBackground.hasImage());
    AURORE_CHECK(info.lightBackground.fit == aurore::BackgroundFit::TILE);
    AURORE_CHECK_EQ(info.lightBackground.dim, 0.2f);
    AURORE_CHECK_EQ(info.darkBackground.dim, 0.6f); // its own variant's line wins
    AURORE_CHECK_EQ(info.darkBackground.surfaceOpacity, 0.5f);

    info = aurore::Theme::inspect(child);
    AURORE_CHECK(info.ok());
    AURORE_CHECK(info.lightBackground.image.find("b.png") != std::string::npos);
    AURORE_CHECK(info.darkBackground.image.find("a.png") != std::string::npos);
    AURORE_CHECK(info.lightBackground.fit == aurore::BackgroundFit::CONTAIN);
    AURORE_CHECK(info.darkBackground.fit == aurore::BackgroundFit::TILE);
    AURORE_CHECK_EQ(info.darkBackground.dim, 0.6f);
    AURORE_CHECK_EQ(info.lightBackground.surfaceOpacity, 0.5f);
    AURORE_CHECK_EQ(info.darkBackground.surfaceOpacity, 0.8f);

    info = aurore::Theme::inspect(none);
    AURORE_CHECK(!info.lightBackground.hasImage() && info.darkBackground.hasImage());
    AURORE_CHECK_EQ(info.lightBackground.surfaceOpacity, 1.0f); // nothing to show through

    // The defaults, and surface_opacity without a picture: no effect
    info = aurore::Theme::inspect(plain);
    AURORE_CHECK(info.ok() && !info.darkBackground.hasImage());
    AURORE_CHECK(info.darkBackground.fit == aurore::BackgroundFit::COVER);
    AURORE_CHECK_EQ(info.darkBackground.dim, aurore::ThemeBackground::DEFAULT_DIM);
    AURORE_CHECK_EQ(info.darkBackground.surfaceOpacity, 1.0f);

    info = aurore::Theme::inspect(wrong);
    AURORE_CHECK_EQ(info.errors.size(), (size_t)3);
    AURORE_CHECK(info.darkBackground.fit == aurore::BackgroundFit::COVER);

    // Used: the surfaces let the picture through, and are opaque again after
    NVGcolor surface = aurore::Theme::getDarkTheme()["cadran/surf"];
    NVGcolor warning = aurore::Theme::getDarkTheme()["cadran/amber_surf"];
    AURORE_CHECK(aurore::Theme::loadFile(child));
    aurore::Application::getPlatform()->setThemeVariant(aurore::ThemeVariant::DARK);
    h.settle();
    AURORE_CHECK(aurore::Application::hasThemeBackground());
    AURORE_CHECK(std::fabs(aurore::Theme::getDarkTheme()["cadran/surf"].a - surface.a * 0.8f) < 0.001f);
    AURORE_CHECK(std::fabs(aurore::Theme::getDarkTheme()["aurore/sidebar/background"].a - 0.8f) < 0.001f);
    AURORE_CHECK(std::fabs(aurore::Theme::getDarkTheme()["cadran/amber_surf"].a - warning.a * 0.8f) < 0.001f);
    AURORE_CHECK(aurore::Theme::getDarkTheme()["aurore/background"].a == 1.0f);
    AURORE_CHECK(aurore::Theme::loadFile(plain));
    h.settle();
    AURORE_CHECK(!aurore::Application::hasThemeBackground());
    AURORE_CHECK_EQ(aurore::Theme::getDarkTheme()["cadran/surf"].a, 1.0f);
    aurore::Theme::reset();
    h.settle();
    AURORE_CHECK_EQ(aurore::Theme::getDarkTheme()["cadran/surf"].a, surface.a);
    AURORE_CHECK_EQ(aurore::Theme::getDarkTheme()["cadran/amber_surf"].a, warning.a);
}

AURORE_TEST(theme_text_is_checked_on_its_picture)
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

    auto onPicture = [](const aurore::ThemeInfo& info)
    {
        for (const std::string& warning : info.warnings)
            if (warning.find("aurore/text on aurore/background over the background image") != std::string::npos)
                return true;
        return false;
    };
    // A plain listing decodes nothing: no check on the picture yet
    NVGcolor mean;
    std::vector<aurore::ThemeInfo> listed = aurore::Theme::listThemes(themes.dir.generic_string());
    AURORE_CHECK_EQ(listed.size(), (size_t)2);
    for (const aurore::ThemeInfo& info : listed)
        AURORE_CHECK(info.ok() && !onPicture(info));
    AURORE_CHECK(!aurore::ThemeImages::knownMeanColor(listed[0].backgroundImage, &mean));

    // An explicit check decodes them
    AURORE_CHECK(onPicture(aurore::Theme::inspect(snow, true)));
    AURORE_CHECK(!onPicture(aurore::Theme::inspect(night, true)));
    // ...and remembers their colour, for the next listing
    AURORE_CHECK(onPicture(aurore::Theme::inspect(snow)));

    // A veil of the background colour makes it read again
    const std::string veiled = themes.write("veiled.theme", std::string(dark) + "background_image = snow.png\nbackground_dim = 0.9\n");
    AURORE_CHECK(!onPicture(aurore::Theme::inspect(veiled, true)));

    // Used, the check runs once the picture is loaded (decoded off the UI
    // thread, its colour recorded then)
    ThemeFolder fresh("aurore_theme_contrast_load_test");
    fresh.picture("sky.png", 250, 250, 250);
    const std::string loaded = fresh.write("sky.theme", std::string(dark) + "background_image = sky.png\n");
    AURORE_CHECK(!onPicture(aurore::Theme::inspect(loaded)));
    AURORE_CHECK(aurore::Theme::loadFile(loaded));
    h.settle();
    AURORE_CHECK(!aurore::Theme::checkBackgroundContrast(aurore::ThemeVariant::DARK).empty());
    AURORE_CHECK(onPicture(aurore::Theme::inspect(loaded)));
    aurore::Theme::reset();
    h.settle();
    AURORE_CHECK(aurore::Theme::loadFile(snow));
    h.settle();
    AURORE_CHECK(!aurore::Theme::checkBackgroundContrast(aurore::ThemeVariant::DARK).empty());
    aurore::Theme::reset();
    h.settle();
    AURORE_CHECK(aurore::Theme::checkBackgroundContrast(aurore::ThemeVariant::DARK).empty());
}

// The demo's Aurora theme (a folder: resources/themes/aurora), its picture
// behind the main screen
static void auroraScreen(aurore::ThemeVariant variant, const char* reference)
{
    DemoHarness h;
    // What does not move (the focus ring pulses, the glow drifts): the last
    // card and the header's right end
    auto still = [&h]()
    { return h.screenshot().crop(1000, 0, 280, 430); };
    aurore::test::Image before = still();
    auto colors                = aurore::Theme::getDarkTheme().getValues()->getColors();

    AURORE_CHECK(aurore::Theme::loadFile("@res/themes/aurora/theme.theme"));
    aurore::Application::getPlatform()->setThemeVariant(variant);
    h.step(); // the picture asked for: decoded, as many frames on any machine
    AURORE_CHECK(themeImagesDecoded());
    h.settle();
    AURORE_CHECK(aurore::Application::hasThemeBackground());
    AURORE_CHECK(aurore::Theme::checkContrast(variant).empty());
    AURORE_CHECK(h.matchesReference(reference));

    // Back to no theme, live: the screen as it was, pixel for pixel
    aurore::Theme::reset();
    aurore::Application::getPlatform()->setThemeVariant(aurore::ThemeVariant::DARK);
    h.settle();
    AURORE_CHECK(!aurore::Application::hasThemeBackground());
    auto diff = aurore::test::compareImages(still(), before, 0);
    AURORE_CHECK(diff.sameSize && diff.differingPixels == 0);
    auto again = aurore::Theme::getDarkTheme().getValues()->getColors();
    AURORE_CHECK_EQ(again.size(), colors.size());
    for (auto& [key, color] : colors)
        AURORE_CHECK(again.count(key) && std::memcmp(&again[key], &color, sizeof(NVGcolor)) == 0);
}

AURORE_TEST(theme_background_dark)
{
    auroraScreen(aurore::ThemeVariant::DARK, "theme_background_dark");
}

AURORE_TEST(theme_background_light)
{
    auroraScreen(aurore::ThemeVariant::LIGHT, "theme_background_light");
}

// Pictures still loading when the theme changes again: dropped, never
// drawn late, never freed twice (run under the sanitizers too)
AURORE_TEST(theme_background_switches_back_and_forth)
{
    DemoHarness h;
    const char* themes[] = { "@res/themes/aurora/theme.theme", "builtin:ocean", "@res/themes/aurora/theme.theme", "" };
    for (int round = 0; round < 12; round++)
    {
        const char* theme = themes[round % 4];
        if (*theme)
            AURORE_CHECK(aurore::Theme::loadFile(theme));
        else
            aurore::Theme::reset();
        aurore::Application::getPlatform()->setThemeVariant(round % 3 ? aurore::ThemeVariant::DARK : aurore::ThemeVariant::LIGHT);
        h.step(round % 2 ? 1 : 2);
    }
    AURORE_CHECK(aurore::Theme::loadFile("@res/themes/aurora/theme.theme"));
    h.settle();
    AURORE_CHECK(aurore::Application::hasThemeBackground());
    aurore::Theme::reset();
    h.settle();
    AURORE_CHECK(!aurore::Application::hasThemeBackground());
}

// Waits for the pictures being decoded (in real time), then for the screen
// (a picture that fails can ask for another, the app's own: a few rounds)
static void settlePictures(Harness& h)
{
    for (int round = 0; round < 3; round++)
    {
        h.step();
        AURORE_CHECK(themeImagesDecoded());
        h.settle();
    }
}

#ifndef _WIN32
// A theme's picture is looked at again just before it is decoded: a link
// put in its place after Theme::resolveAsset looked is refused
AURORE_TEST(theme_pictures_are_checked_again_before_they_are_read)
{
    Harness h;
    ThemeFolder themes("aurore_theme_recheck_test");
    themes.picture("outside.png", 200, 30, 30);
    themes.picture("mine/bg.png", 30, 30, 200);
    std::string path, error;
    const std::string folder = (themes.dir / "mine").generic_string();
    AURORE_CHECK(aurore::Theme::resolveAsset("bg.png", folder, folder, &path, &error));
    AURORE_CHECK(aurore::Theme::recheckAsset(path));

    std::filesystem::remove(themes.dir / "mine" / "bg.png");
    std::filesystem::create_symlink(themes.dir / "outside.png", themes.dir / "mine" / "bg.png");
    AURORE_CHECK(!aurore::Theme::recheckAsset(path, &error));
    AURORE_CHECK(error.find("a link out of the theme's folder") != std::string::npos);
    std::shared_ptr<aurore::ThemeImage> image = aurore::ThemeImages::load(path);
    settlePictures(h);
    AURORE_CHECK(image->state == aurore::ThemeImage::State::FAILED);
    NVGcolor mean;
    AURORE_CHECK(!aurore::ThemeImages::meanColor(path, &mean));

    // A path resolveAsset did not check: as it is
    AURORE_CHECK(aurore::Theme::recheckAsset((themes.dir / "outside.png").generic_string()));
}
#endif

static bool hasWarningWith(const aurore::ThemeInfo& info, const std::string& text)
{
    for (const std::string& warning : info.warnings)
        if (warning.find(text) != std::string::npos)
            return true;
    return false;
}

AURORE_TEST(theme_logos_and_images_are_read_inherited_and_confined)
{
    Harness h;
    ThemeFolder themes("aurore_theme_pictures_test");
    const std::string root = themes.dir.generic_string();
    aurore::Theme::addImageSlot("test/art", "");
    aurore::Theme::addImageSlot("test/cover", "");
    for (const char* name : { "logo.png", "logo-day.png", "preview.png", "gear.png", "art.png", "base/logo.png", "base/i.png", "base/art.png" })
        themes.picture(name, 120, 60, 200);
    const std::string base = themes.write("base/theme.theme",
        "name = Base\nlogo = logo.png\nicon.info = i.png\nicon.info.tint = true\nimage.test/art = art.png\n"
        "[light]\nicon.settings = i.png\n");
    const std::string full = themes.write("full.theme",
        "name = Full\nlogo = logo.png\npreview = preview.png\nicon.settings = gear.png\nicon.settings.tint = yes\n"
        "icon.gamepad = gear.png\nimage.test/art = art.png\n[light]\nlogo = logo-day.png\nicon.gamepad = none\n"
        "[dark]\nimage.test/cover = art.png\n");

    aurore::ThemeInfo info = aurore::Theme::inspect(full);
    AURORE_CHECK(info.ok());
    AURORE_CHECK(info.warnings.empty());
    AURORE_CHECK_EQ(info.darkPictures.logo, root + "/logo.png");
    AURORE_CHECK_EQ(info.lightPictures.logo, root + "/logo-day.png"); // its own variant's line wins
    AURORE_CHECK_EQ(info.lightPictures.preview, root + "/preview.png");
    AURORE_CHECK(info.darkPictures.icons.at("settings").tint);
    AURORE_CHECK(!info.darkPictures.icons.at("gamepad").tint);
    AURORE_CHECK(!info.lightPictures.icons.count("gamepad")); // none
    AURORE_CHECK_EQ(info.lightPictures.images.at("test/art"), root + "/art.png");
    AURORE_CHECK(info.darkPictures.images.count("test/cover") && !info.lightPictures.images.count("test/cover"));

    // Inherited: the parent's pictures stay in the parent's folder, this
    // file's lines win
    const std::string child = themes.write("child.theme",
        "inherit = base/theme.theme\nicon.settings = gear.png\n[dark]\nlogo = none\n");
    info                    = aurore::Theme::inspect(child);
    AURORE_CHECK(info.ok());
    AURORE_CHECK(info.darkPictures.logo.empty());
    AURORE_CHECK_EQ(info.lightPictures.logo, root + "/base/logo.png");
    AURORE_CHECK_EQ(info.darkPictures.icons.at("info").image, root + "/base/i.png");
    AURORE_CHECK(info.darkPictures.icons.at("info").tint);
    AURORE_CHECK_EQ(info.lightPictures.icons.at("settings").image, root + "/gear.png"); // over the parent's [light]
    AURORE_CHECK_EQ(info.darkPictures.images.at("test/art"), root + "/base/art.png");

    // Confined, read, named: errors and warnings, never a crash
    const std::string wrong = themes.write("wrong.theme",
        "logo = ../outside.png\npreview = /etc/passwd\nicon.settings = missing.png\nicon.settings.tint = maybe\n"
        "icon.nosuchicon = gear.png\nimage.test/nosuchslot = art.png\n");
    info                    = aurore::Theme::inspect(wrong);
    AURORE_CHECK(hasErrorWith(info, "logo ../outside.png: goes out of the theme's folder"));
    AURORE_CHECK(hasErrorWith(info, "preview /etc/passwd: an absolute path is not allowed"));
    AURORE_CHECK(hasErrorWith(info, "icon.settings: cannot read"));
    AURORE_CHECK(hasErrorWith(info, "icon.settings.tint is true or false"));
    AURORE_CHECK(hasWarningWith(info, "icon.nosuchicon: no icon has that name"));
    AURORE_CHECK(hasWarningWith(info, "image.test/nosuchslot: the app has no such image slot (kept); its slots: "));
    AURORE_CHECK(hasWarningWith(info, "test/art, test/cover"));
    AURORE_CHECK(info.darkPictures.logo.empty() && info.darkPictures.preview.empty());

    // Used, and back: the logo is the theme's for the variant on screen
    AURORE_CHECK(!aurore::Theme::logo());
    AURORE_CHECK(aurore::Theme::loadFile(full));
    aurore::Application::getPlatform()->setThemeVariant(aurore::ThemeVariant::DARK);
    h.settle();
    // Loaded when asked for (by whoever draws it), off the UI thread
    AURORE_CHECK(aurore::Theme::logo() && aurore::Theme::logo()->state == aurore::ThemeImage::State::LOADING);
    settlePictures(h);
    std::shared_ptr<aurore::ThemeImage> logo = aurore::Theme::logo();
    AURORE_CHECK(logo && logo->ready() && logo->path == root + "/logo.png");
    AURORE_CHECK(logo->width <= aurore::ThemeImages::LOGO_SIZE && logo->height <= aurore::ThemeImages::LOGO_SIZE);
    aurore::Application::getPlatform()->setThemeVariant(aurore::ThemeVariant::LIGHT);
    h.settle();
    AURORE_CHECK(aurore::Theme::logo() && aurore::Theme::logo()->path == root + "/logo-day.png");
    bool tint = false;
    AURORE_CHECK(aurore::Theme::icon("settings", &tint) && tint);
    AURORE_CHECK(!aurore::Theme::icon("gamepad"));
    aurore::Theme::reset();
    aurore::Application::getPlatform()->setThemeVariant(aurore::ThemeVariant::DARK);
    h.settle();
    AURORE_CHECK(!aurore::Theme::logo() && !aurore::Theme::icon("settings"));
}

AURORE_TEST(theme_pictures_are_capped_and_freed)
{
    Harness h;
    ThemeFolder themes("aurore_theme_pictures_cap_test");
    themes.picture("big.png", 200, 100, 50, 1024, 512);
    themes.write("big.theme", "logo = big.png\nicon.settings = big.png\nicon.settings.tint = true\n");
    AURORE_CHECK(aurore::Theme::loadFile((themes.dir / "big.theme").generic_string()));
    h.settle();
    std::weak_ptr<aurore::ThemeImage> logo = aurore::Theme::logo();
    std::weak_ptr<aurore::ThemeImage> icon = aurore::Theme::icon("settings");
    settlePictures(h);
    AURORE_CHECK(logo.lock() && logo.lock()->ready());
    AURORE_CHECK_EQ(logo.lock()->width, aurore::ThemeImages::LOGO_SIZE);
    AURORE_CHECK_EQ(logo.lock()->height, aurore::ThemeImages::LOGO_SIZE / 2); // its ratio kept
    AURORE_CHECK_EQ(icon.lock()->width, aurore::ThemeImages::ICON_SIZE);
    AURORE_CHECK(icon.lock()->mask);
    // Another theme: the pictures are let go of, their textures freed
    AURORE_CHECK(aurore::Theme::loadFile("builtin:ocean"));
    h.settle();
    AURORE_CHECK(!aurore::Theme::logo());
    AURORE_CHECK(logo.expired() && icon.expired());
    aurore::Theme::reset();
}

AURORE_TEST(theme_image_slots_follow_the_theme)
{
    Harness h;
    ThemeFolder themes("aurore_theme_slots_test");
    const std::string fallback = themes.picture("fallback.png", 10, 200, 10, 48, 48);
    themes.picture("art.png", 200, 10, 10, 64, 32);
    themes.write("broken.png", "not a png");
    aurore::Theme::addImageSlot("test/art", fallback, 256);
    aurore::Theme::addImageSlot("test/empty");
    AURORE_CHECK(aurore::Theme::getImageSlots() == (std::vector<std::string> { "test/art", "test/empty" }));
    const std::string art    = themes.write("art.theme", "image.test/art = art.png\nimage.test/empty = art.png\n");
    const std::string broken = themes.write("broken.theme", "image.test/art = broken.png\n");

    auto* image = new aurore::Image();
    image->setThemeImage("test/art");
    auto* empty = new aurore::Image();
    empty->setThemeImage("test/empty");
    h.push(new aurore::Activity(screenOf({ image, empty })));
    settlePictures(h);

    // The app's own, without a theme; nothing for a slot without one
    AURORE_CHECK(aurore::Theme::image("test/art") && aurore::Theme::image("test/art")->path == fallback);
    AURORE_CHECK(!aurore::Theme::image("test/empty"));
    AURORE_CHECK(!aurore::Theme::image("test/undeclared"));
    AURORE_CHECK_EQ(image->getOriginalImageWidth(), 48.0f);
    AURORE_CHECK_EQ(empty->getTexture(), 0);

    // The theme's, live
    AURORE_CHECK(aurore::Theme::loadFile(art));
    settlePictures(h);
    AURORE_CHECK_EQ(image->getOriginalImageWidth(), 64.0f);
    AURORE_CHECK_EQ(image->getTexture(), aurore::Theme::image("test/art")->texture);
    AURORE_CHECK(empty->getTexture() != 0);

    // A picture that cannot be read: the app's
    AURORE_CHECK(aurore::Theme::loadFile(broken));
    settlePictures(h);
    AURORE_CHECK_EQ(image->getOriginalImageWidth(), 48.0f);
    AURORE_CHECK_EQ(empty->getTexture(), 0);

    // Back to no theme: the app's again
    AURORE_CHECK(aurore::Theme::loadFile(art));
    settlePictures(h);
    AURORE_CHECK_EQ(image->getOriginalImageWidth(), 64.0f);
    aurore::Theme::reset();
    settlePictures(h);
    AURORE_CHECK_EQ(image->getOriginalImageWidth(), 48.0f);
    AURORE_CHECK_EQ(empty->getTexture(), 0);

    // Set another way: it stops following the theme
    image->setImageFromFile(themes.picture("own.png", 1, 2, 3, 20, 20));
    AURORE_CHECK(image->getThemeImage().empty());
    AURORE_CHECK(aurore::Theme::loadFile(art));
    settlePictures(h);
    AURORE_CHECK_EQ(image->getOriginalImageWidth(), 20.0f);
    aurore::Theme::reset();
}

// An icon replaced by a picture: as it is, or as a mask in the icon's colour
AURORE_TEST(theme_icons_are_tinted_or_drawn_as_they_are)
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
    h.push(new aurore::Activity(screenOf({ tinted, plain, ink, other })));
    h.settle();
    aurore::test::Image before = h.screenshot();

    auto middle = [&h](aurore::View* view)
    {
        aurore::Rect frame       = view->getFrame();
        aurore::test::Image shot = h.screenshot();
        const unsigned char* p   = shot.at((int)frame.getMidX(), (int)frame.getMidY());
        return std::vector<int> { p[0], p[1], p[2] };
    };
    auto corner = [&h](aurore::View* view)
    {
        aurore::Rect frame       = view->getFrame();
        aurore::test::Image shot = h.screenshot();
        const unsigned char* p   = shot.at((int)frame.getMinX() + 4, (int)frame.getMinY() + 4);
        return std::vector<int> { p[0], p[1], p[2] };
    };
    auto near = [](std::vector<int> a, std::vector<int> b)
    {
        for (int c = 0; c < 3; c++)
            if (std::abs(a[c] - b[c]) > 6)
                return false;
        return true;
    };

    AURORE_CHECK(aurore::Theme::loadFile((themes.dir / "icons.theme").generic_string()));
    settlePictures(h);
    AURORE_CHECK(near(middle(tinted), { 20, 40, 230 })); // the icon's colour
    AURORE_CHECK(near(middle(plain), { 230, 20, 20 })); // the picture's
    AURORE_CHECK(near(corner(tinted), { 20, 20, 30 })); // transparent around it
    AURORE_CHECK(near(middle(ink), { 20, 40, 230 })); // black on nothing: all ink

    // Back: the drawings, pixel for pixel
    aurore::Theme::reset();
    h.settle();
    auto diff = aurore::test::compareImages(h.screenshot(), before, 0);
    AURORE_CHECK(diff.sameSize && diff.differingPixels == 0);
}

// The demo's Prism theme (resources/themes/prism): its logo in the header,
// its icons on the rail (Settings focused: a tinted icon in the focus's
// colours, the gamepad as it is)
static void prismScreen(aurore::ThemeVariant variant, const char* reference)
{
    DemoHarness h;
    aurore::test::Image before = h.screenshot();
    AURORE_CHECK(aurore::Theme::loadFile("@res/themes/prism/theme.theme"));
    aurore::Application::getPlatform()->setThemeVariant(variant);
    settlePictures(h);
    AURORE_CHECK(aurore::Theme::logo() && aurore::Theme::logo()->ready());
    AURORE_CHECK(aurore::Theme::checkContrast(variant).empty());
    AURORE_CHECK(h.focusByText("Settings"));
    settlePictures(h);
    AURORE_CHECK(h.matchesReference(reference));

    // Back to no theme, live: the screen as it was
    AURORE_CHECK(h.focusByText("Cadran", aurore::BUTTON_UP));
    aurore::Theme::reset();
    aurore::Application::getPlatform()->setThemeVariant(aurore::ThemeVariant::DARK);
    h.settle();
    AURORE_CHECK(!aurore::Theme::logo());
    // (what does not move: the glow drifts) the mark within its corners, the
    // header's right end
    for (aurore::Rect area : { aurore::Rect(26, 29, 26, 26), aurore::Rect(1000, 0, 280, 90) })
    {
        auto crop = [&area](const aurore::test::Image& image)
        { return image.crop((int)area.getMinX(), (int)area.getMinY(), (int)area.getWidth(), (int)area.getHeight()); };
        auto diff = aurore::test::compareImages(crop(h.screenshot()), crop(before), 0);
        AURORE_CHECK(diff.sameSize && diff.differingPixels == 0);
    }
}

AURORE_TEST(theme_logo_and_icons_dark)
{
    prismScreen(aurore::ThemeVariant::DARK, "theme_images_dark");
}

AURORE_TEST(theme_logo_and_icons_light)
{
    prismScreen(aurore::ThemeVariant::LIGHT, "theme_images_light");
}

// The demo's About tab draws its image slot (demo/about/art): Prism's
AURORE_TEST(theme_image_slot_in_the_demo)
{
    DemoHarness h;
    AURORE_CHECK(h.focusByText("About Aurore"));
    settlePictures(h);
    std::shared_ptr<aurore::ThemeImage> own = aurore::Theme::image("demo/about/art");
    AURORE_CHECK(own && own->path == "@res/img/aurore_256.png");
    AURORE_CHECK(aurore::Theme::loadFile("@res/themes/prism/theme.theme"));
    settlePictures(h);
    AURORE_CHECK(aurore::Theme::image("demo/about/art")->path == "@res/themes/prism/about.png");
    AURORE_CHECK(h.matchesReference("theme_image_slot"));
    aurore::Theme::reset();
    settlePictures(h);
    AURORE_CHECK(aurore::Theme::image("demo/about/art")->path == "@res/img/aurore_256.png");
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
    for (const aurore::ThemeInfo& info : aurore::Theme::listThemes("@res/themes"))
        if (info.id == "prism")
            AURORE_CHECK_EQ(info.darkPictures.preview, preview);
    ui::pick_palette();
    // Asked for by the picker's swatch, not decoded on this thread: still loading
    aurore::ThemeImageOptions options;
    options.flags                             = NVG_IMAGE_GENERATE_MIPMAPS;
    options.maxSide                           = aurore::ThemeImages::PREVIEW_SIZE;
    std::shared_ptr<aurore::ThemeImage> image = aurore::ThemeImages::load(preview, options);
    AURORE_CHECK(image->state == aurore::ThemeImage::State::LOADING);
    AURORE_CHECK(h.focusByText("Prism", aurore::BUTTON_DOWN, 40));
    settlePictures(h);
    AURORE_CHECK(image->ready());
    AURORE_CHECK(h.matchesReference(reference));
}

AURORE_TEST(palette_picker_shows_previews)
{
    pickPrism("dark", "palette_picker_preview");
}

AURORE_TEST(palette_picker_shows_previews_light)
{
    pickPrism("light", "palette_picker_preview_light");
}

// Logos, icons and slot pictures still loading when the theme or the
// variant changes again: dropped, never drawn late, never freed twice (run
// under the sanitizers too)
AURORE_TEST(theme_pictures_switch_back_and_forth)
{
    DemoHarness h;
    AURORE_CHECK(h.focusByText("About Aurore"));
    const char* themes[] = { "@res/themes/prism/theme.theme", "@res/themes/aurora/theme.theme", "@res/themes/prism/theme.theme", "" };
    for (int round = 0; round < 16; round++)
    {
        const char* theme = themes[round % 4];
        if (*theme)
            AURORE_CHECK(aurore::Theme::loadFile(theme));
        else
            aurore::Theme::reset();
        aurore::Application::getPlatform()->setThemeVariant(round % 3 ? aurore::ThemeVariant::DARK : aurore::ThemeVariant::LIGHT);
        h.step(round % 2 ? 1 : 2);
        if (round == 9)
            ui::pick_palette(); // its previews loading too
        if (round == 12)
            h.tap(aurore::BUTTON_B);
    }
    AURORE_CHECK(aurore::Theme::loadFile("@res/themes/prism/theme.theme"));
    aurore::Application::getPlatform()->setThemeVariant(aurore::ThemeVariant::DARK);
    settlePictures(h);
    AURORE_CHECK(aurore::Theme::logo() && aurore::Theme::logo()->ready());
    AURORE_CHECK(aurore::Theme::image("demo/about/art")->path == "@res/themes/prism/about.png");
    aurore::Theme::reset();
    settlePictures(h);
    AURORE_CHECK(!aurore::Theme::logo());
    AURORE_CHECK(aurore::Theme::image("demo/about/art")->path == "@res/img/aurore_256.png");
}

// ---------------------------------------------------------------------------
// Decorations: backdrops, strips, particles
// ---------------------------------------------------------------------------

AURORE_TEST(theme_decor_keys_are_read_and_inherited)
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
        "[colors]\naurore/backdrop = rgba(0, 0, 0, 0.8) # a comment\n[dark]\nparticles_color = #ffffff, #ddeeff, #c0d0e0\n"
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

    aurore::ThemeInfo info = aurore::Theme::inspect(base);
    AURORE_CHECK(info.ok() && info.warnings.empty());
    const aurore::ThemePicture& top = info.darkDecor.picture(aurore::ThemeArea::TOP);
    AURORE_CHECK(top.image.find("base/garland.png") != std::string::npos);
    AURORE_CHECK(top.fit == aurore::BackgroundFit::TILE);
    AURORE_CHECK_EQ(top.height, 24.0f);
    AURORE_CHECK_EQ(info.darkDecor.picture(aurore::ThemeArea::SIDEBAR).opacity, 0.5f);
    AURORE_CHECK(!info.darkDecor.picture(aurore::ThemeArea::DIALOG).hasImage());
    AURORE_CHECK(info.lightDecor.picture(aurore::ThemeArea::DIALOG).fit == aurore::BackgroundFit::CENTER);
    AURORE_CHECK(info.darkDecor.particles.kind == aurore::ParticleKind::SNOW);
    AURORE_CHECK(info.lightDecor.particles.kind == aurore::ParticleKind::PETALS); // its own variant's line
    AURORE_CHECK_EQ(info.darkDecor.particles.density, aurore::ThemeParticles::MEDIUM);
    AURORE_CHECK_EQ(info.darkDecor.particles.speed, 0.5f);
    AURORE_CHECK_EQ(info.darkDecor.particles.colors.size(), (size_t)3); // its own variant's
    AURORE_CHECK_EQ(info.lightDecor.particles.colors.size(), (size_t)2);
    AURORE_CHECK(std::fabs(info.lightDecor.particles.colors[1].a - 0.8f) < 0.01f);
    AURORE_CHECK(info.darkDecor.particles.layer == aurore::ParticleLayer::BACK);

    // Inherited, and taken away
    info = aurore::Theme::inspect(child);
    AURORE_CHECK(info.ok());
    AURORE_CHECK(!info.darkDecor.picture(aurore::ThemeArea::TOP).hasImage());
    AURORE_CHECK(info.lightDecor.picture(aurore::ThemeArea::TOP).hasImage());
    AURORE_CHECK(info.lightDecor.picture(aurore::ThemeArea::SIDEBAR).hasImage());
    AURORE_CHECK_EQ(info.darkDecor.particles.density, aurore::ThemeParticles::HIGH);
    AURORE_CHECK_EQ(info.lightDecor.particles.density, aurore::ThemeParticles::MEDIUM);
    AURORE_CHECK(info.lightDecor.particles.layer == aurore::ParticleLayer::FRONT);

    // Every wrong line an error, none of them a decoration; out of the
    // folder refused as the background's
    info = aurore::Theme::inspect(wrong);
    AURORE_CHECK_EQ(info.errors.size(), (size_t)12);
    AURORE_CHECK(hasErrorWith(info, "goes out of the theme's folder"));
    AURORE_CHECK(hasErrorWith(info, "an absolute path is not allowed"));
    AURORE_CHECK(!info.darkDecor.hasPictures() && !info.darkDecor.particles.enabled());

    // None by default
    info = aurore::Theme::inspect(plain);
    AURORE_CHECK(!info.darkDecor.hasPictures() && !info.lightDecor.hasPictures());
    AURORE_CHECK(info.darkDecor.particles.kind == aurore::ParticleKind::NONE);

    // A picture that is not there
    AURORE_CHECK(hasErrorWith(aurore::Theme::inspect(themes.write("base/missing.theme", "backdrop.header = nowhere.png\n")),
        "backdrop.header: cannot read"));

    // Used, and gone with the theme
    AURORE_CHECK(aurore::Theme::loadFile(base));
    aurore::Application::getPlatform()->setThemeVariant(aurore::ThemeVariant::DARK);
    h.step();
    AURORE_CHECK(themeImagesDecoded());
    h.settle();
    AURORE_CHECK(aurore::ThemeDecorations::hasPicture(aurore::ThemeArea::TOP));
    AURORE_CHECK(aurore::ThemeDecorations::hasPicture(aurore::ThemeArea::SIDEBAR));
    AURORE_CHECK(!aurore::ThemeDecorations::hasPicture(aurore::ThemeArea::HEADER));
    AURORE_CHECK(aurore::ThemeDecorations::particleCount() > 0);
    aurore::Theme::reset();
    h.settle();
    AURORE_CHECK(!aurore::ThemeDecorations::hasPicture(aurore::ThemeArea::TOP));
    AURORE_CHECK_EQ(aurore::ThemeDecorations::particleCount(), 0);
}

// A theme of particles only, over an empty screen
static void useParticles(const std::string& preset, const std::string& kind, const std::string& more = "")
{
    AURORE_CHECK(aurore::Theme::load("inherit = builtin:" + preset + "\nparticles = " + kind + "\n" + more));
}

AURORE_TEST(theme_particles_follow_motion_and_input)
{
    Harness h;
    // Nothing that moves of its own (a focus pulses)
    auto* label = new aurore::Label();
    label->setText("Still");
    auto* screen = new aurore::Box(aurore::Axis::COLUMN); // no fill: the particles show
    screen->addView(label);
    h.push(new aurore::Activity(screen));
    aurore::Application::setRedrawOnDemand(true);
    useParticles("winter", "snow");
    h.settle();
    AURORE_CHECK(aurore::ThemeDecorations::particleCount() > 0);
    AURORE_CHECK(aurore::ThemeDecorations::particlesRunning());

    // Moving: about PARTICLE_FPS frames a second, not every frame
    uint64_t drawn = aurore::Application::getDrawnFrameCount();
    h.advance(1000);
    uint64_t frames = aurore::Application::getDrawnFrameCount() - drawn;
    AURORE_CHECK(frames >= 25 && frames <= 35);

    // Reduced motion: nothing drawn, no frame asked for but the one a second
    aurore::Application::setReduceMotion(true);
    h.step(3);
    AURORE_CHECK(!aurore::ThemeDecorations::particlesRunning());
    aurore::test::Image still = h.screenshot();
    drawn                     = aurore::Application::getDrawnFrameCount();
    h.advance(900);
    AURORE_CHECK(aurore::Application::getDrawnFrameCount() - drawn <= 1);
    // ...and not a frozen frame of them: the screen is the one without
    aurore::Theme::load("inherit = builtin:winter\n");
    h.settle();
    AURORE_CHECK_EQ(aurore::test::compareImages(h.screenshot(), still, 0).differingPixels, (size_t)0);
    useParticles("winter", "snow");
    h.settle();
    AURORE_CHECK_EQ(aurore::test::compareImages(h.screenshot(), still, 0).differingPixels, (size_t)0);

    // Motion again: they come back
    aurore::Application::setReduceMotion(false);
    h.advance(2000);
    AURORE_CHECK(aurore::ThemeDecorations::particlesRunning());
    AURORE_CHECK(aurore::test::compareImages(h.screenshot(), still, 0).differingPixels > 50);

    // Idle a minute: they fade out and stop asking for frames
    h.advance(aurore::ThemeDecorations::IDLE_SECONDS * 1000.0 + 4000);
    AURORE_CHECK(!aurore::ThemeDecorations::particlesRunning());
    drawn = aurore::Application::getDrawnFrameCount();
    h.advance(2000);
    AURORE_CHECK(aurore::Application::getDrawnFrameCount() - drawn <= 2);
    AURORE_CHECK_EQ(aurore::test::compareImages(h.screenshot(), still, 0).differingPixels, (size_t)0);

    // An input: back
    h.tap(aurore::BUTTON_DOWN);
    AURORE_CHECK(aurore::ThemeDecorations::particlesRunning());
    drawn = aurore::Application::getDrawnFrameCount();
    h.advance(1000);
    AURORE_CHECK(aurore::Application::getDrawnFrameCount() - drawn >= 25);

    // Never a view: the focus and the layout know nothing of them
    AURORE_CHECK(dynamic_cast<aurore::Box*>(h.top()->getContentView())->getChildren().size() == 1);
    aurore::Theme::reset();
    h.settle();
    AURORE_CHECK(!aurore::ThemeDecorations::particlesRunning());
}

// Every kind, mid-flight, in both variants: drawn (and saved, for a look,
// with AURORE_DECOR_SHOTS=<folder>)
AURORE_TEST(theme_particles_draw_every_kind)
{
    const struct
    {
        const char* kind;
        const char* preset;
    } kinds[]          = { { "snow", "winter" }, { "leaves", "autumn" }, { "petals", "spring" }, { "embers", "halloween" },
                 { "confetti", "new-year" }, { "sparkles", "lunar-new-year" }, { "hearts", "valentine" }, { "bubbles", "summer" } };
    const char* folder = std::getenv("AURORE_DECOR_SHOTS");
    Harness h;
    h.push(new aurore::Activity(new aurore::Box()));
    for (const auto& each : kinds)
        for (aurore::ThemeVariant variant : { aurore::ThemeVariant::DARK, aurore::ThemeVariant::LIGHT })
        {
            aurore::Application::getPlatform()->setThemeVariant(variant);
            aurore::Theme::load(std::string("inherit = builtin:") + each.preset + "\n");
            h.step(2);
            aurore::test::Image plain = h.screenshot();
            useParticles(each.preset, each.kind, "particles_density = high\n");
            h.advance(2500);
            aurore::test::Image shot = h.screenshot();
            AURORE_CHECK(aurore::test::compareImages(shot, plain, 4).differingPixels > 200);
            if (folder)
                shot.savePng(std::string(folder) + "/particles_" + each.kind
                    + (variant == aurore::ThemeVariant::DARK ? "_dark" : "_light") + ".png");
        }
    // A few frames in a row, for a moving picture
    if (folder)
        for (const char* kind : { "snow", "leaves" })
        {
            aurore::Application::getPlatform()->setThemeVariant(aurore::ThemeVariant::DARK);
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
    aurore::Theme::reset();
    h.step(2);
}

// backdrop.header: across the whole width of a screen's header, under its
// title; nothing elsewhere
AURORE_TEST(theme_decor_header_backdrop)
{
    DemoHarness h;
    aurore::Application::setReduceMotion(true);
    ThemeFolder themes("aurore_theme_header_test");
    themes.picture("red.png", 255, 0, 0);
    AURORE_CHECK(aurore::Theme::loadFile(themes.write("red.theme", "inherit = dark\nbackdrop.header = red.png\n")));
    h.step();
    AURORE_CHECK(themeImagesDecoded());
    h.settle();
    AURORE_CHECK(aurore::ThemeDecorations::hasPicture(aurore::ThemeArea::HEADER));
    aurore::test::Image shot = h.screenshot();
    for (int x : { 2, 640, 1277 })
        AURORE_CHECK(shot.at(x, 4)[0] > 200 && shot.at(x, 4)[1] < 60);
    AURORE_CHECK(shot.at(640, 300)[0] < 120); // the content: as it was
    aurore::Theme::reset();
    h.settle();
    AURORE_CHECK(h.screenshot().at(640, 4)[0] < 120);
}

// The demo's Winter theme (a folder: resources/themes/winter): its strips
// and the sidebar's frost behind the main screen, its particles held still
// (reduced motion) so the reference does not depend on when
static void winterScreen(aurore::ThemeVariant variant, const char* reference)
{
    DemoHarness h;
    aurore::Application::setReduceMotion(true);
    AURORE_CHECK(aurore::Theme::loadFile("@res/themes/winter/theme.theme"));
    aurore::Application::getPlatform()->setThemeVariant(variant);
    h.step();
    AURORE_CHECK(themeImagesDecoded());
    h.settle();
    AURORE_CHECK(aurore::ThemeDecorations::hasPicture(aurore::ThemeArea::TOP));
    AURORE_CHECK(aurore::ThemeDecorations::hasPicture(aurore::ThemeArea::BOTTOM));
    AURORE_CHECK(aurore::ThemeDecorations::hasPicture(aurore::ThemeArea::SIDEBAR));
    AURORE_CHECK(aurore::ThemeDecorations::particleCount() > 0 && !aurore::ThemeDecorations::particlesRunning());
    AURORE_CHECK(h.matchesReference(reference));
}

AURORE_TEST(theme_decor_dark)
{
    winterScreen(aurore::ThemeVariant::DARK, "theme_decor_dark");
}

AURORE_TEST(theme_decor_light)
{
    winterScreen(aurore::ThemeVariant::LIGHT, "theme_decor_light");
}

// Decorations still loading, particles running, motion going on and off
// while the theme changes: nothing drawn late, nothing freed twice (run
// under the sanitizers too)
AURORE_TEST(theme_decor_switches_back_and_forth)
{
    DemoHarness h;
    const char* themes[] = { "@res/themes/winter/theme.theme", "builtin:new-year", "@res/themes/aurora/theme.theme",
        "builtin:halloween", "" };
    for (int round = 0; round < 20; round++)
    {
        const char* theme = themes[round % 5];
        if (*theme)
            AURORE_CHECK(aurore::Theme::loadFile(theme));
        else
            aurore::Theme::reset();
        aurore::Application::getPlatform()->setThemeVariant(round % 3 ? aurore::ThemeVariant::DARK : aurore::ThemeVariant::LIGHT);
        aurore::Application::setReduceMotion(round % 4 == 3);
        h.step(round % 2 ? 1 : 3);
    }
    aurore::Application::setReduceMotion(false);
    AURORE_CHECK(aurore::Theme::loadFile("@res/themes/winter/theme.theme"));
    h.step();
    AURORE_CHECK(themeImagesDecoded());
    h.settle();
    AURORE_CHECK(aurore::ThemeDecorations::hasPicture(aurore::ThemeArea::TOP) && aurore::ThemeDecorations::particlesRunning());
    aurore::Theme::reset();
    h.settle();
    AURORE_CHECK(!aurore::ThemeDecorations::hasPicture(aurore::ThemeArea::TOP) && !aurore::ThemeDecorations::particlesRunning());
}

// A translucent dialog over a theme's picture shows the picture, not the
// screen behind it
AURORE_TEST(theme_decor_dialog_over_a_picture)
{
    DemoHarness h;
    aurore::Application::setReduceMotion(true);
    AURORE_CHECK(aurore::Theme::loadFile("@res/themes/aurora/theme.theme"));
    h.step();
    AURORE_CHECK(themeImagesDecoded());
    h.settle();
    h.tap(aurore::BUTTON_RIGHT);
    h.tap(aurore::BUTTON_DOWN);
    AURORE_CHECK_EQ(h.focusText(), std::string("Confirm"));
    h.tap(aurore::BUTTON_A);
    h.settle();
    AURORE_CHECK(h.matchesReference("theme_decor_dialog"));
}

AURORE_TEST(images_compare_with_tolerance)
{
    aurore::test::Image a;
    a.width  = 4;
    a.height = 2;
    a.rgba.assign(4 * 2 * 4, 100);
    aurore::test::Image b = a;
    b.rgba[0]             = 105; // within 8
    b.rgba[4]             = 140; // a pixel off
    auto diff             = aurore::test::compareImages(b, a, 8);
    AURORE_CHECK(diff.sameSize);
    AURORE_CHECK_EQ(diff.differingPixels, (size_t)1);
    AURORE_CHECK_EQ(diff.maxDelta, 40);
    AURORE_CHECK(!aurore::test::compareImages(b, a.crop(0, 0, 2, 2)).sameSize);
}

namespace
{

// A line of text, the same size whatever it says
aurore::Label* lineOf(const std::string& text)
{
    auto* label = new aurore::Label();
    label->setText(text);
    label->setFontSize(40);
    label->setSingleLine(true);
    label->setWidth(600);
    label->setHeight(60);
    return label;
}

// What the screen shows where a view is
aurore::test::Image shotOf(Harness& h, aurore::View* view)
{
    aurore::Rect frame = view->getFrame();
    return h.screenshot().crop((int)frame.getMinX(), (int)frame.getMinY(), (int)frame.getWidth(), (int)frame.getHeight());
}

// Two lines drawn differently: not the same glyphs
bool drawnDifferently(Harness& h, aurore::View* one, aurore::View* other)
{
    return aurore::test::compareImages(shotOf(h, one), shotOf(h, other), 8).differingPixels > 100;
}

} // namespace

AURORE_TEST(fonts_are_the_free_ones)
{
    Harness h;
    // The text and the buttons from the library's resources, the Switch's
    // fonts nowhere
    AURORE_CHECK(aurore::Application::getFont(aurore::FONT_REGULAR) != aurore::FONT_INVALID);
    AURORE_CHECK(aurore::Application::getFont(aurore::FONT_SWITCH_ICONS) != aurore::FONT_INVALID);
    AURORE_CHECK(!aurore::Application::loadFontFromFile("old", AURORE_ASSET("font/switch_font.ttf")));
    AURORE_CHECK(!aurore::Application::loadFontFromFile("old", AURORE_ASSET("font/switch_icons.ttf")));
    // The harness reads no system font: its pictures are the same everywhere
    AURORE_CHECK(aurore::Application::getFont(aurore::FONT_KOREAN_REGULAR) == aurore::FONT_INVALID);
    AURORE_CHECK(aurore::Application::getFont(aurore::FONT_JAPANESE) == aurore::FONT_INVALID);
}

AURORE_TEST(button_glyphs_drawn)
{
    Harness h;
    // At the code points of the Switch's shared font (Hint::getKeyIcon)
    auto* a       = lineOf(aurore::Hint::getKeyIcon(aurore::BUTTON_A, true) + aurore::Hint::getKeyIcon(aurore::BUTTON_LB, true));
    auto* b       = lineOf(aurore::Hint::getKeyIcon(aurore::BUTTON_B, true) + aurore::Hint::getKeyIcon(aurore::BUTTON_RB, true));
    auto* missing = lineOf("\u0378\u0378"); // no such character: the empty box
    h.push(new aurore::Activity(screenOf({ a, b, missing })));
    h.settle();
    AURORE_CHECK(drawnDifferently(h, a, b));
    AURORE_CHECK(drawnDifferently(h, a, missing));
    AURORE_CHECK(drawnDifferently(h, b, missing));
}

AURORE_TEST(asset_fonts_load_from_the_resources)
{
    // AURORE_ASSET() is "@res/..." with the resources built in: read from them
    Harness h;
    AURORE_CHECK(aurore::Application::loadFontFromFile("test/sora", AURORE_ASSET("font/sora_bold_latin.ttf")));
    AURORE_CHECK(aurore::Application::getFont("test/sora") != aurore::FONT_INVALID);
    AURORE_CHECK(!aurore::Application::loadFontFromFile("test/none", AURORE_ASSET("font/none.ttf")));
}

AURORE_TEST(cadran_display_font_loaded)
{
    // Cadran's figures in Sora, with the resources built in too
    DemoHarness h;
    AURORE_CHECK(aurore::Application::getFont("cadran/digits") != aurore::FONT_INVALID);
}

AURORE_TEST(cjk_from_the_system_fonts)
{
    aurore::test::Options options;
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
        { "ko", aurore::FONT_KOREAN_REGULAR, "\uD55C\uAD6D\uC5B4", "\u0378\u0378\u0378" }, // 한국어
        { "zh-Hans", aurore::FONT_CHINESE_SIMPLIFIED, "\u7B80\u4F53\u4E2D\u6587", "\u0378\u0378\u0378\u0378" }, // 简体中文
        { "zh-Hant", aurore::FONT_CHINESE_TRADITIONAL, "\u7E41\u9AD4\u4E2D\u6587", "\u0378\u0378\u0378\u0378" }, // 繁體中文
        { "ja", aurore::FONT_JAPANESE, "\u65E5\u672C\u8A9E\u3042", "\u0378\u0378\u0378\u0378" }, // 日本語あ
    };
    for (const Case& c : cases)
    {
        aurore::DesktopFontLoader::FontFile file = aurore::DesktopFontLoader::findSystemFont(c.language);
        if (file.path.empty())
        {
            std::printf("cjk_from_the_system_fonts: no %s font on this system, not checked\n", c.language);
            continue;
        }
        AURORE_CHECK(aurore::Application::getFont(c.font) != aurore::FONT_INVALID);
        auto* text    = lineOf(c.text);
        auto* missing = lineOf(c.missing);
        h.push(new aurore::Activity(screenOf({ text, missing })));
        h.settle();
        if (!drawnDifferently(h, text, missing))
            std::printf("cjk_from_the_system_fonts: %s drawn as missing glyphs (%s)\n", c.language, file.path.c_str());
        AURORE_CHECK(drawnDifferently(h, text, missing));
    }
}

// ---------------------------------------------------------------------------
// The focus announcer: what a screen reader would be given
// ---------------------------------------------------------------------------

static const std::string& announced()
{
    return aurore::Application::getLastAnnouncement();
}

AURORE_TEST(announcer_says_role_value_state_and_place)
{
    Harness h;
    auto* toggle = new aurore::BooleanCell();
    toggle->init("Wi-Fi", false, [](bool) {});
    auto* detail = new aurore::DetailCell();
    detail->setText("Language");
    detail->setDetailText("English");
    auto* radio = new aurore::RadioCell();
    radio->title->setText("Dark");
    radio->setSelected(true);
    auto* slider = new aurore::SliderCell();
    slider->init("Volume", 0.5f, [](float) {});
    slider->slider->setDiscreteStep(0.1f);
    std::vector<std::string> said;
    aurore::Application::setFocusAnnouncer([&said](const std::string& text)
        { said.push_back(text); });
    h.push(new aurore::Activity(screenOf({ toggle, detail, radio, slider })));
    h.settle();

    AURORE_CHECK_EQ(announced(), std::string("Wi-Fi, switch, off, 1 of 3"));
    h.tap(aurore::BUTTON_A); // flipped in place: its new state alone
    AURORE_CHECK(toggle->isOn());
    AURORE_CHECK_EQ(announced(), std::string("on"));
    h.tap(aurore::BUTTON_DOWN);
    AURORE_CHECK_EQ(announced(), std::string("Language, English, 2 of 3"));
    detail->setDetailText("Français"); // changed while focused, after a press
    AURORE_CHECK_EQ(announced(), std::string("Français"));
    h.advance(3000);
    detail->setDetailText("Deutsch"); // on its own: not read out
    AURORE_CHECK_EQ(announced(), std::string("Français"));
    h.tap(aurore::BUTTON_DOWN);
    AURORE_CHECK_EQ(announced(), std::string("Dark, selected, 3 of 3"));
    h.tap(aurore::BUTTON_DOWN);
    AURORE_CHECK_EQ(announced(), std::string("Volume, slider, 50%"));
    h.tap(aurore::BUTTON_RIGHT);
    AURORE_CHECK_EQ(announced(), std::string("60%"));
    // Gliding while held: said once, when let go
    slider->slider->setDiscreteStep(0);
    size_t before = said.size();
    h.hold(aurore::BUTTON_RIGHT, 400);
    h.step(2);
    AURORE_CHECK_EQ(said.size(), before + 1);
    AURORE_CHECK(announced() != "60%" && announced().back() == '%');
    // Each went to the announcer as it was said.
    AURORE_CHECK(!said.empty() && said.back() == announced());
    aurore::Application::setFocusAnnouncer(nullptr);
}

AURORE_TEST(announcer_says_disabled_and_explicit_values)
{
    Harness h;
    auto* first  = new aurore::Button();
    auto* second = new aurore::Button();
    first->setText("Save");
    second->setText("Quit");
    second->setStyleState(aurore::StyleState::DISABLED, true);
    auto* box = new aurore::Box();
    box->setFocusable(true);
    box->setAccessibilityLabel("Battery");
    box->setAccessibilityValue("80%");
    h.push(new aurore::Activity(screenOf({ first, second, box })));
    h.settle();
    AURORE_CHECK_EQ(announced(), std::string("Save, button, 1 of 3"));
    AURORE_CHECK_EQ(second->getAccessibilityDescription(), std::string("Quit, button, disabled, 2 of 3"));
    // No role: no place said either.
    AURORE_CHECK_EQ(box->getAccessibilityDescription(), std::string("Battery, 80%"));
}

AURORE_TEST(focusable_box_says_the_texts_in_it)
{
    Harness h;
    auto* tile = new aurore::Box(aurore::Axis::COLUMN);
    tile->setFocusable(true);
    auto* name = new aurore::Label();
    name->setText("Mario Kart");
    auto* hidden = new aurore::Label();
    hidden->setText("never said");
    hidden->setVisibility(aurore::Visibility::GONE);
    auto* time = new aurore::Label();
    time->setText("2 h 10");
    auto* inner = new aurore::Button(); // speaks for itself
    inner->setText("Play");
    tile->addView(name);
    tile->addView(hidden);
    tile->addView(time);
    tile->addView(inner);
    auto* silent = new aurore::Box();
    silent->setFocusable(true);
    silent->setWidth(40);
    silent->setHeight(40);
    h.push(new aurore::Activity(screenOf({ tile, silent })));
    h.settle();
    AURORE_CHECK_EQ(tile->getAccessibilityDescription(), std::string("Mario Kart, 2 h 10"));
    AURORE_CHECK_EQ(silent->getAccessibilityDescription(), std::string(""));
}

AURORE_TEST(toast_and_dialog_are_announced)
{
    Harness h;
    auto* button = new aurore::Button();
    button->setText("Delete");
    h.push(new aurore::Activity(screenOf({ button })));
    h.settle();
    AURORE_CHECK_EQ(announced(), std::string("Delete, button"));

    aurore::Application::notify("Profile saved");
    AURORE_CHECK_EQ(announced(), std::string("Profile saved"));

    auto* dialog = new aurore::Dialog("Delete this profile?");
    dialog->addButton("Cancel", [] {});
    dialog->addButton("Delete", [] {});
    dialog->open();
    h.settle();
    const std::string opened = announced();
    AURORE_CHECK(opened.rfind("Delete this profile?, dialog, ", 0) == 0);
    AURORE_CHECK(opened.find(", button") != std::string::npos);
    // Within the dialog, only the button.
    h.tap(aurore::BUTTON_RIGHT);
    AURORE_CHECK(announced().rfind("Delete this profile?", 0) != 0);
    h.tap(aurore::BUTTON_B);
    AURORE_CHECK_EQ(announced(), std::string("Delete, button"));
}

AURORE_TEST(dropdown_says_its_title_and_the_choice)
{
    Harness h;
    auto* selector = new aurore::SelectorCell();
    selector->init("Region", { "Europe", "America", "Asia" }, 1, [](int) {});
    h.push(new aurore::Activity(screenOf({ selector })));
    h.settle();
    AURORE_CHECK_EQ(announced(), std::string("Region, America"));
    h.tap(aurore::BUTTON_A);
    h.settle();
    AURORE_CHECK_EQ(announced(), std::string("Region, list, America, selected, 2 of 3"));
    h.tap(aurore::BUTTON_DOWN);
    AURORE_CHECK_EQ(announced(), std::string("Asia, 3 of 3"));
}

AURORE_TEST(announcer_speaks_the_app_language)
{
    Harness h;
    auto* toggle = new aurore::BooleanCell();
    toggle->init("Wi-Fi", true, [](bool) {});
    auto* other = new aurore::Button();
    other->setText("OK");
    other->setStyleState(aurore::StyleState::DISABLED, true);
    h.push(new aurore::Activity(screenOf({ toggle, other })));
    h.settle();

    aurore::loadTranslations("fr");
    AURORE_CHECK_EQ(toggle->getAccessibilityDescription(), std::string("Wi-Fi, interrupteur, activé, 1 sur 2"));
    AURORE_CHECK_EQ(other->getAccessibilityDescription(), std::string("OK, bouton, indisponible, 2 sur 2"));
    // A language that says the count first
    aurore::loadTranslations("ja");
    AURORE_CHECK_EQ(toggle->getAccessibilityDescription(), std::string("Wi-Fi, スイッチ, オン, 2項目中1番目"));
    aurore::loadTranslations("en-US");
    AURORE_CHECK_EQ(toggle->getAccessibilityDescription(), std::string("Wi-Fi, switch, on, 1 of 2"));
}

namespace
{
// The demo's hooks (Cadran's theme, pickers) without its main screen
struct CadranHarness : Demo, Harness
{
};
} // namespace

AURORE_TEST(cadran_pieces_have_roles_and_labels)
{
    CadranHarness h;
    aurore::Box* chip = cadran::kit::chip("+15 min");
    aurore::Box* card = cadran::kit::card(true, false);
    card->addView(cadran::kit::label("Today", cadran::kit::TEXT_META));
    card->addView(cadran::kit::label("2 h 10", cadran::kit::TEXT_BODY));
    auto* toggle = new aurore::BooleanCell();
    toggle->init("Notifications", true, [](bool) {});
    cadran::List::row(toggle, "bell"); // its detail hidden, a drawn switch instead
    auto* line = new aurore::DetailCell();
    line->setText("Updates");
    line->setDetailText("3 new  ›");
    cadran::List::row(line, "download");
    h.push(new aurore::Activity(screenOf({ chip, card, toggle, line })));
    h.settle();

    AURORE_CHECK_EQ(announced(), std::string("+15 min, button, 1 of 4"));
    AURORE_CHECK_EQ(card->getAccessibilityDescription(), std::string("Today, 2 h 10, 2 of 4"));
    AURORE_CHECK_EQ(toggle->getAccessibilityDescription(), std::string("Notifications, switch, on, 3 of 4"));
    AURORE_CHECK_EQ(line->getAccessibilityDescription(), std::string("Updates, 3 new, 4 of 4"));
}

namespace
{
// A screen with the applet frame's header, titled `title`
aurore::AppletFrame* titledScreen(const std::string& title, const std::string& markId = "")
{
    aurore::Box* box = screenOf({ new aurore::Button() });
    if (!markId.empty())
    {
        auto* mark = new aurore::Rectangle();
        mark->setId(markId);
        box->addView(mark);
    }
    auto* frame = new aurore::AppletFrame(box);
    frame->setTitle(title);
    return frame;
}
} // namespace

AURORE_TEST(cadran_crumb_skips_screens_of_their_own)
{
    CadranHarness h;
    h.push(new aurore::Activity(titledScreen("First")));
    h.settle();
    h.push(new aurore::Activity(titledScreen("Second")));
    h.settle();
    AURORE_CHECK(h.hasText("First  ›")); // opened from First
    // A screen of its own (a child's): no crumb, by the id in it…
    h.push(new aurore::Activity(titledScreen("Third", "cadran/standalone")));
    h.settle();
    AURORE_CHECK(h.hasText("Third"));
    AURORE_CHECK(!h.hasText("Second  ›"));
    // … or as the app's hook says
    ui::hooks::get().standalone_screen = [](aurore::View* frame)
    {
        auto* title = dynamic_cast<aurore::Label*>(frame->getView("aurore/applet_frame/title_label"));
        return title && title->getFullText() == "Fourth";
    };
    h.push(new aurore::Activity(titledScreen("Fourth")));
    h.settle();
    AURORE_CHECK(h.hasText("Fourth"));
    AURORE_CHECK(!h.hasText("Third  ›"));
    h.push(new aurore::Activity(titledScreen("Fifth")));
    h.settle();
    AURORE_CHECK(h.hasText("Fourth  ›"));
    ui::hooks::get().standalone_screen = nullptr;
}

AURORE_TEST(cadran_picker_says_its_title_and_the_choice)
{
    CadranHarness h;
    auto* button = new aurore::Button();
    button->setText("Theme");
    h.push(new aurore::Activity(screenOf({ button })));
    h.settle();
    ui::pick("Theme", { "System", "Light", "Dark" }, 2, [](int) {});
    h.settle();
    AURORE_CHECK_EQ(announced(), std::string("Theme, list, Dark, selected, 3 of 3"));
    h.tap(aurore::BUTTON_UP);
    AURORE_CHECK_EQ(announced(), std::string("Light, 2 of 3"));
}

AURORE_TEST(every_focusable_demo_view_says_something)
{
    DemoHarness h;
    AURORE_CHECK(announced().rfind("Cadran, tab, selected, 1 of ", 0) == 0);
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
        h.tap(aurore::BUTTON_DOWN);
    }
    AURORE_CHECK(seen.size() > 5);
    for (const std::string& name : silent)
        std::fprintf(stderr, "silent: %s\n", name.c_str());
    AURORE_CHECK(silent.empty());
}

int main(int argc, char** argv)
{
    return aurore::test::runTests(argc, argv);
}

AURORE_TEST(switch_off_reads_as_secondary_text)
{
    Harness h;
    auto* cell = new aurore::BooleanCell();
    cell->init("Switch", false, [](bool) {});
    h.push(new aurore::Activity(screenOf({ cell })));
    h.settle();
    for (aurore::ThemeVariant variant : { aurore::ThemeVariant::LIGHT, aurore::ThemeVariant::DARK })
    {
        aurore::Application::getPlatform()->setThemeVariant(variant);
        h.step(2);
        aurore::Theme theme = aurore::Application::getTheme();
        NVGcolor off        = cell->detail->getTextColor();
        NVGcolor want       = theme["aurore/text_secondary"];
        AURORE_CHECK(std::fabs(off.r - want.r) < 0.01f && std::fabs(off.g - want.g) < 0.01f && std::fabs(off.b - want.b) < 0.01f);
        AURORE_CHECK(aurore::Theme::contrast(off, theme["aurore/background"]) >= 4.5f);
    }
}

AURORE_TEST(every_demo_theme_passes_the_contrast_check)
{
    DemoHarness h;
    for (aurore::ThemeVariant variant : { aurore::ThemeVariant::LIGHT, aurore::ThemeVariant::DARK })
        AURORE_CHECK(aurore::Theme::checkContrast(variant).empty());
    for (const aurore::ThemeInfo& info : aurore::Theme::listThemes("@res/themes", true))
    {
        for (const std::string& warning : info.warnings)
            std::printf("%s: %s\n", info.path.c_str(), warning.c_str());
        AURORE_CHECK(info.ok() && info.warnings.empty());
    }
}

static std::string xmlError(const std::string& name, const std::string& xml)
{
    std::filesystem::path file = std::filesystem::temp_directory_path() / name;
    std::ofstream(file) << xml;
    try
    {
        aurore::View::createFromXMLFile(file.string());
    }
    catch (const std::logic_error& e)
    {
        std::filesystem::remove(file);
        return e.what();
    }
    std::filesystem::remove(file);
    return "";
}

AURORE_TEST(xml_errors_say_where_and_what_was_meant)
{
    Harness h;
    std::string typo = xmlError("aurore_typo.xml", "<aurore:Box>\n\n    <aurore:Label fontSise=\"20\"/>\n</aurore:Box>\n");
    AURORE_CHECK(typo.find("aurore_typo.xml:3: <aurore:Label>: Unknown XML attribute \"fontSise\"") != std::string::npos);
    AURORE_CHECK(typo.find("did you mean \"fontSize\"?") != std::string::npos);

    std::string value = xmlError("aurore_value.xml", "<aurore:Box\n    axis=\"colum\"/>\n");
    AURORE_CHECK(value.find("aurore_value.xml:1: <aurore:Box>: Illegal value \"colum\" for XML attribute \"axis\"; expected column, row") != std::string::npos);

    std::string tag = xmlError("aurore_tag.xml", "<aurore:Box>\n<aurore:Lable/>\n</aurore:Box>\n");
    AURORE_CHECK(tag.find("aurore_tag.xml:2: <aurore:Lable>: Unknown XML tag \"aurore:Lable\"; did you mean \"aurore:Label\"?") != std::string::npos);
}

AURORE_TEST(reduce_motion_stops_core_animations)
{
    Harness h;
    aurore::Application::setReduceMotion(true);
    auto* cell = new aurore::BooleanCell();
    cell->init("Switch", false, [](bool) {});
    h.push(new aurore::Activity(screenOf({ cell })));
    h.settle();
    h.tap(aurore::BUTTON_A);
    AURORE_CHECK(cell->isOn());
    AURORE_CHECK(aurore::Ticking::runningTickings.empty() || std::none_of(aurore::Ticking::runningTickings.begin(), aurore::Ticking::runningTickings.end(), [](aurore::Ticking* t)
                     { return dynamic_cast<aurore::FiniteTicking*>(t) != nullptr; }));

    // A text cut short stays still while focused
    auto* row   = new aurore::Box(aurore::Axis::ROW);
    auto* label = new aurore::Label();
    label->setText("A text far too long for the narrow line it is written on");
    label->setSingleLine(true);
    row->setWidth(200);
    row->setFocusable(true);
    row->addView(label);
    aurore::setHighlightAnimationStill(true); // the focus ring's pulse aside
    h.push(new aurore::Activity(screenOf({ row })));
    h.settle();
    h.step(120);
    aurore::test::Image before = h.screenshot();
    h.step(120);
    AURORE_CHECK_EQ(aurore::test::compareImages(h.screenshot(), before).differingPixels, 0);
}

AURORE_TEST(rail_names_are_not_cut)
{
    DemoHarness h;
    for (const std::string& text : h.cutTexts())
        AURORE_CHECK(text != "Basic components" && text != "Layout and alignment");
}

AURORE_TEST(hold_to_confirm_says_it_once)
{
    DemoHarness h;
    // Opened by a finger: no A key bubble
    AURORE_CHECK(h.tapView(h.findText("Cadran")));
    ui::hold_confirm(cadran::dialog_body(aurore::getStr("demo/cadran/hold_text")), aurore::getStr("demo/cadran/hold"), [] {});
    h.settle();
    AURORE_CHECK(!h.hasText("A"));
    AURORE_CHECK(!h.hasText("Reset everything?\n\nHold the button for 2 seconds."));
    AURORE_CHECK(h.matchesReference("hold_confirm_touch"));
}

AURORE_TEST(cut_text_file_written_with_cadran)
{
    std::filesystem::path dir = std::filesystem::temp_directory_path() / "aurore_cut_text";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    std::filesystem::path previous = std::filesystem::current_path();
    std::filesystem::current_path(dir);
    // Through the app's hook: AURORE_CUT_TEXT is read only with AURORE_DEV_TOOLS
    ui::hooks::get().env_flag = [](const char* name)
    { return std::string(name) == "CUT_TEXT"; };
    {
        DemoHarness h;
        auto* cut = new aurore::Label();
        cut->setText("A sentence far too long for the narrow box it is given");
        cut->setSingleLine(true);
        cut->setWidth(120);
        h.push(new aurore::Activity(screenOf({ cut })));
        h.settle();
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        h.step(2);
        std::ifstream file(dir / "cut_text.txt");
        std::string all((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        AURORE_CHECK(all.find("A sentence far too long for the narrow box it is given") != std::string::npos);
    }
    ui::hooks::get().env_flag = nullptr;
    std::filesystem::current_path(previous);
    std::filesystem::remove_all(dir);
}

AURORE_TEST(settings_tab_keeps_the_cards_margins)
{
    DemoHarness h;
    AURORE_CHECK(h.focusByText("Settings"));
    h.tap(aurore::BUTTON_RIGHT);
    h.settle();
    aurore::View* line = h.focus();
    AURORE_CHECK(line != nullptr);
    if (line)
    {
        aurore::Rect frame = line->getFrame();
        // Within the margins of Cadran's pages, the focus ring off the window's edge
        AURORE_CHECK(frame.getMaxX() <= 1280 - 40 + 1);
        AURORE_CHECK(frame.getMinY() >= 84 + 28 - 1);
    }
    AURORE_CHECK(h.matchesReference("settings_tab"));
}

AURORE_TEST(mouse_quick_click_is_not_lost)
{
    // Pressed and let go between two frames: a touchpad tap, a quick click
    DemoHarness h;
    aurore::View* tab = h.findText("Layout and alignment");
    AURORE_CHECK(tab != nullptr);
    aurore::Rect frame = tab->getFrame();
    h.mouseQuickClick(frame.getMidX(), frame.getMidY());
    AURORE_CHECK(h.hasText("alignItems: center"));
}

namespace
{

// A screen whose button counts the controller buttons it gets
struct ButtonCounter
{
    aurore::Button* button = new aurore::Button();
    std::map<aurore::ControllerButton, int> pressed;

    ButtonCounter(Harness& h)
    {
        button->setText("Count");
        for (aurore::ControllerButton b : { aurore::BUTTON_X, aurore::BUTTON_Y, aurore::BUTTON_LB, aurore::BUTTON_RB,
                 aurore::BUTTON_LT, aurore::BUTTON_RT, aurore::BUTTON_START, aurore::BUTTON_BACK })
            button->registerAction("", b, [this, b](aurore::View*)
                {
                    pressed[b]++;
                    return true; });
        h.push(new aurore::Activity(screenOf({ button })));
        h.settle();
    }
};

} // namespace

AURORE_TEST(keyboard_keys_press_controller_buttons)
{
    Harness h;
    ButtonCounter c(h);
    h.tapKey(aurore::AURORE_KBD_KEY_X);
    h.tapKey(aurore::AURORE_KBD_KEY_Y);
    h.tapKey(aurore::AURORE_KBD_KEY_L);
    h.tapKey(aurore::AURORE_KBD_KEY_R);
    h.tapKey(aurore::AURORE_KBD_KEY_P);
    h.tapKey(aurore::AURORE_KBD_KEY_KP_ADD);
    h.tapKey(aurore::AURORE_KBD_KEY_M);
    h.tapKey(aurore::AURORE_KBD_KEY_MINUS);
    AURORE_CHECK_EQ(c.pressed[aurore::BUTTON_X], 1);
    AURORE_CHECK_EQ(c.pressed[aurore::BUTTON_Y], 1);
    AURORE_CHECK_EQ(c.pressed[aurore::BUTTON_LB], 1);
    AURORE_CHECK_EQ(c.pressed[aurore::BUTTON_RB], 1);
    AURORE_CHECK_EQ(c.pressed[aurore::BUTTON_START], 2);
    AURORE_CHECK_EQ(c.pressed[aurore::BUTTON_BACK], 2);
    AURORE_CHECK_EQ(c.pressed[aurore::BUTTON_LT], 0);

    // Shift for the triggers, and not the shoulder buttons then
    h.pressKey(aurore::AURORE_KBD_KEY_LEFT_SHIFT);
    h.tapKey(aurore::AURORE_KBD_KEY_L);
    h.tapKey(aurore::AURORE_KBD_KEY_R);
    h.releaseKey(aurore::AURORE_KBD_KEY_LEFT_SHIFT);
    AURORE_CHECK_EQ(c.pressed[aurore::BUTTON_LT], 1);
    AURORE_CHECK_EQ(c.pressed[aurore::BUTTON_RT], 1);
    AURORE_CHECK_EQ(c.pressed[aurore::BUTTON_LB], 1);

    // Ctrl+X is a shortcut, not X
    h.pressKey(aurore::AURORE_KBD_KEY_LEFT_CONTROL);
    h.tapKey(aurore::AURORE_KBD_KEY_X);
    h.releaseKey(aurore::AURORE_KBD_KEY_LEFT_CONTROL);
    AURORE_CHECK_EQ(c.pressed[aurore::BUTTON_X], 1);
}

AURORE_TEST(keyboard_keys_can_be_mapped_again)
{
    Harness h;
    ButtonCounter c(h);
    aurore::Application::mapKey(aurore::AURORE_KBD_KEY_Q, aurore::BUTTON_LB);
    aurore::Application::unmapKey(aurore::AURORE_KBD_KEY_L);
    h.tapKey(aurore::AURORE_KBD_KEY_Q);
    h.tapKey(aurore::AURORE_KBD_KEY_L);
    AURORE_CHECK_EQ(c.pressed[aurore::BUTTON_LB], 1);

    // A key the app handles itself is its own
    int own = 0;
    c.button->registerAction(aurore::AuroreKeyCombination(aurore::AURORE_KBD_KEY_X), [&own](aurore::View*)
        {
            own++;
            return true; });
    h.tapKey(aurore::AURORE_KBD_KEY_X);
    AURORE_CHECK_EQ(own, 1);
    AURORE_CHECK_EQ(c.pressed[aurore::BUTTON_X], 0);

    aurore::Application::clearKeyMap();
    h.tapKey(aurore::AURORE_KBD_KEY_Y);
    AURORE_CHECK_EQ(c.pressed[aurore::BUTTON_Y], 0);
    AURORE_CHECK(aurore::Application::getKeyMap().empty());
}

AURORE_TEST(keyboard_keys_type_text_in_a_text_field)
{
    Harness h;
    ButtonCounter c(h);
    auto* dialog = new aurore::EditTextDialog();
    dialog->open();
    h.settle();
    h.tapKey(aurore::AURORE_KBD_KEY_M); // not Minus: the dialog's delete
    h.tapKey(aurore::AURORE_KBD_KEY_X);
    AURORE_CHECK_EQ(c.pressed[aurore::BUTTON_X], 0);
    AURORE_CHECK(dynamic_cast<aurore::EditTextDialog*>(h.top()->getContentView()) != nullptr);
}

AURORE_TEST(horizontal_carousel_lets_left_out)
{
    DemoHarness h;
    AURORE_CHECK(h.focusByText("Basic components"));
    h.tap(aurore::BUTTON_RIGHT);
    aurore::View* first = h.focus();
    AURORE_CHECK(first != nullptr);
    bool inCarousel = false;
    for (aurore::View* v = first; v; v = v->hasParent() ? v->getParent() : nullptr)
        inCarousel |= dynamic_cast<aurore::HScrollingFrame*>(v) != nullptr;
    AURORE_CHECK(inCarousel);
    h.tapKey(aurore::AURORE_KBD_KEY_LEFT);
    AURORE_CHECK(dynamic_cast<aurore::SidebarItem*>(h.focus()) != nullptr);
}

AURORE_TEST(focus_on_a_scrolling_frame_shows_its_place)
{
    DemoHarness h;
    AURORE_CHECK(h.focusByText("Layout and alignment"));
    h.tap(aurore::BUTTON_RIGHT);
    // Down through the tab until the focus waits on the frame itself
    aurore::ScrollingFrame* frame = nullptr;
    for (int i = 0; i < 30 && !frame; i++)
    {
        h.tap(aurore::BUTTON_DOWN);
        frame = dynamic_cast<aurore::ScrollingFrame*>(h.focus());
    }
    AURORE_CHECK(frame != nullptr);
    if (!frame)
        return;
    aurore::Rectangle* indicator = nullptr;
    for (aurore::View* child : frame->getChildren())
        if (auto* r = dynamic_cast<aurore::Rectangle*>(child))
            indicator = r;
    AURORE_CHECK(indicator != nullptr);
    if (!indicator)
        return;
    AURORE_CHECK(indicator->getAlpha() > 0.99f);
    NVGcolor want = aurore::Application::getTheme()["aurore/highlight/color1"];
    NVGcolor got  = indicator->getColor();
    AURORE_CHECK(std::fabs(got.r - want.r) < 0.01f && std::fabs(got.g - want.g) < 0.01f && std::fabs(got.b - want.b) < 0.01f);
    AURORE_CHECK(h.matchesReference("scrolling_frame_focus"));
}

#ifdef __GLFW__
// A word being composed with an input method: its focused block in [ ], and
// block sizes that do not add up to the text read no further than blockCount
AURORE_TEST(ime_preedit_text_stays_in_its_blocks)
{
    const unsigned int word[] = { 'a', 'b', 'c', 0x1F600 };
    const int sizes[]         = { 1, 2 };
    AURORE_CHECK_EQ(aurore::glfwPreeditText(3, word, 2, sizes, 1), std::string("a[bc]"));
    AURORE_CHECK_EQ(aurore::glfwPreeditText(3, word, 2, sizes, 0), std::string("[a]bc"));
    AURORE_CHECK_EQ(aurore::glfwPreeditText(4, word, 2, sizes, 5), std::string("abc\xF0\x9F\x98\x80"));
    // One block said (sizes[1] is past it): the rest is in none, and block 1
    // is never focused
    AURORE_CHECK_EQ(aurore::glfwPreeditText(3, word, 1, sizes, 1), std::string("abc"));
    AURORE_CHECK_EQ(aurore::glfwPreeditText(3, word, 1, sizes, 0), std::string("[a]bc"));
    // An empty block, and nothing at all
    const int empty[] = { 0, 3 };
    AURORE_CHECK_EQ(aurore::glfwPreeditText(3, word, 2, empty, 1), std::string("[abc]"));
    AURORE_CHECK_EQ(aurore::glfwPreeditText(0, word, 2, sizes, 0), std::string(""));
    AURORE_CHECK_EQ(aurore::glfwPreeditText(3, word, 0, sizes, 0), std::string(""));
}
#endif

// A pan recognizer that has seen no touch fires nothing: its last state is
// set from the start, not whatever its memory held
AURORE_TEST(pan_recognizer_fires_nothing_before_a_touch)
{
    Harness h;
    int fired = 0;
    alignas(aurore::PanGestureRecognizer) unsigned char memory[sizeof(aurore::PanGestureRecognizer)];
    std::memset(memory, 0x01, sizeof(memory)); // GestureState::UNSURE in every byte
    auto* pan           = new (memory) aurore::PanGestureRecognizer([&](aurore::PanGestureStatus, aurore::Sound*)
        { fired++; },
        aurore::PanAxis::ANY);
    aurore::Sound sound = aurore::SOUND_NONE;
    pan->recognitionLoop(aurore::TouchState(), aurore::MouseState(), nullptr, &sound);
    AURORE_CHECK_EQ(fired, 0);
    pan->~PanGestureRecognizer();
}

namespace
{
bool s_second_tab_broken = false;

aurore::View* tabWith(const std::string& text)
{
    auto* button = new aurore::Button();
    button->setText(text);
    return screenOf({ button });
}

// Two tabs, the second one's content throwing as a bad XML file does
class TwoTabs : public aurore::Activity
{
  public:
    aurore::View* createContentView() override
    {
        auto* frame = new aurore::TabFrame();
        frame->addTab("One", []
            { return tabWith("first"); });
        frame->addTab("Two", []
            {
            if (s_second_tab_broken)
                throw std::logic_error("two.xml:3: <aurore:Label>: Unknown XML attribute \"fontSise\"");
            return tabWith("second"); });
        return frame;
    }
};
} // namespace

AURORE_TEST(hot_reload_keeps_the_screen_when_a_tab_fails)
{
    Harness h;
    s_second_tab_broken = false;
    h.push(new TwoTabs());
    h.settle();
    AURORE_CHECK(h.focusByText("Two"));
    h.tap(aurore::BUTTON_RIGHT);
    AURORE_CHECK_EQ(h.focusText(), std::string("second"));
    aurore::View* content = h.top()->getContentView();
    aurore::View* focused = h.focus();

    s_second_tab_broken = true;
    AURORE_CHECK(!aurore::Application::rebuildActivity());
    h.settle();
    AURORE_CHECK(h.top()->getContentView() == content);
    AURORE_CHECK(h.focus() == focused);
    AURORE_CHECK(h.hasText("second"));

    s_second_tab_broken = false;
    AURORE_CHECK(aurore::Application::rebuildActivity());
    h.settle();
    AURORE_CHECK(h.top()->getContentView() != content);
    AURORE_CHECK_EQ(h.focusText(), std::string("second"));
}

// What Windows, macOS and Linux give for the language, each to one of the
// 15 catalogues
AURORE_TEST(system_locales_map_to_the_catalogues)
{
    Harness h;
    const std::pair<const char*, const char*> cases[] = {
        { "fr_CA", "fr-CA" }, { "fr_FR.UTF-8", "fr" }, { "fr-CA", "fr-CA" }, { "zh_CN", "zh-Hans" },
        { "zh-Hans", "zh-Hans" }, { "zh-Hans-CN", "zh-Hans" }, { "zh-Hant-TW", "zh-Hant" }, { "zh_TW", "zh-Hant" },
        { "zh_HK", "zh-Hant" }, { "zh_SG", "zh-Hans" }, { "es_MX", "es-419" }, { "es-419", "es-419" },
        { "es_ES", "es" }, { "es", "es" }, { "en_GB", "en-GB" }, { "en-AU", "en-GB" }, { "en_US", "en-US" },
        { "pt_BR", "pt-BR" }, { "pt-PT", "pt" }, { "de_AT@euro", "de" }, { "ja_JP", "ja" }, { "ko-KR", "ko" },
        { "nl_BE", "nl" }, { "ru_RU.KOI8-R", "ru" }, { "it_CH", "it" }, { "C", "en-US" }, { "sv_SE", "en-US" }, { "", "en-US" }
    };
    for (const auto& [system, locale] : cases)
    {
        std::string got = aurore::localeFromSystem(system);
        if (got != locale)
            std::fprintf(stderr, "%s gave %s, not %s\n", system, got.c_str(), locale);
        AURORE_CHECK_EQ(got, std::string(locale));
    }
}

// The header's language, theme and text size: a finger a little beside the
// drawing still finds them
AURORE_TEST(header_settings_take_a_wider_touch)
{
    DemoHarness h;
    aurore::View* language = h.top()->getContentView()->getView("header_language");
    aurore::View* theme    = h.top()->getContentView()->getView("header_theme");
    AURORE_CHECK(language != nullptr && theme != nullptr);
    if (!language || !theme)
        return;
    aurore::Rect frame = language->getFrame();
    AURORE_CHECK_EQ(frame.size.height, 40.0f);
    size_t screens = aurore::Application::getActivitiesStack().size();

    // 7 px above it: the language list
    h.tapAt(frame.getMidX(), frame.getMinY() - 7);
    AURORE_CHECK_EQ(aurore::Application::getActivitiesStack().size(), screens + 1);
    h.tap(aurore::BUTTON_B);
    AURORE_CHECK_EQ(aurore::Application::getActivitiesStack().size(), screens);

    // In the gap, nearer the theme: the theme's list, not the language's
    aurore::Rect next = theme->getFrame();
    h.tapAt(next.getMinX() - 3, next.getMidY());
    AURORE_CHECK_EQ(aurore::Application::getActivitiesStack().size(), screens + 1);
    AURORE_CHECK(!h.hasText("Français"));
    h.tap(aurore::BUTTON_B);

    // Farther: nothing
    h.tapAt(frame.getMidX(), frame.getMinY() - 12);
    AURORE_CHECK_EQ(aurore::Application::getActivitiesStack().size(), screens);
}

// AURORE_LANG gives the language over the system's, a locale's name as it
// is or a system's read (borealis' names: tests/legacy_names_tests.cpp)
AURORE_TEST(language_from_the_environment_wins)
{
    Harness h;
    unsetenv("AURORE_LANG");
    std::string locale = "unchanged";
    AURORE_CHECK(!aurore::DesktopPlatform::localeFromEnvironmentVariable(&locale));
    AURORE_CHECK_EQ(locale, std::string("unchanged"));

    setenv("AURORE_LANG", "de_DE.UTF-8", 1);
    AURORE_CHECK(aurore::DesktopPlatform::localeFromEnvironmentVariable(&locale));
    AURORE_CHECK_EQ(locale, std::string("de"));

    setenv("AURORE_LANG", "fr-CA", 1);
    AURORE_CHECK(aurore::DesktopPlatform::localeFromEnvironmentVariable(&locale));
    AURORE_CHECK_EQ(locale, std::string("fr-CA"));

    setenv("AURORE_LANG", "../../etc", 1);
    AURORE_CHECK(aurore::DesktopPlatform::localeFromEnvironmentVariable(&locale));
    AURORE_CHECK_EQ(locale, std::string("en-US"));

    setenv("AURORE_THEME", "LIGHT", 1);
    AURORE_CHECK_EQ(std::string(aurore::DesktopPlatform::environment("THEME")), std::string("LIGHT"));
    for (const char* name : { "AURORE_LANG", "AURORE_THEME" })
        unsetenv(name);
}

// The desktop's text field: Cadran's card with its title in it, the cursor
// shown at the end of the text, Y to take the text out
AURORE_TEST(text_field_is_a_card_with_a_cursor)
{
    DemoHarness h;
    aurore::Application::setReduceMotion(true); // the cursor still
    auto* dialog = new aurore::EditTextDialog();
    dialog->setHeaderText("Input text");
    dialog->setText("https://github.com");
    dialog->setCursor((int)aurore::CursorPosition::END);
    dialog->setCountText("18/32");
    bool cleared = false;
    dialog->getClearEvent()->subscribe([&cleared]()
        { cleared = true; });
    dialog->open();
    h.settle();

    auto* card  = dynamic_cast<aurore::Box*>(dialog->getView("aurore/dialog/applet"));
    auto* title = dialog->getView("aurore/dialog/header");
    AURORE_CHECK(card != nullptr && title != nullptr);
    if (!card || !title)
        return;
    AURORE_CHECK(title->getParent() == card);
    AURORE_CHECK(card->getView("cadran_dialog") != nullptr);
    AURORE_CHECK(h.matchesReference("text_field_cadran"));

    // The cursor is drawn: without it, the text's end differs
    aurore::test::Image with = h.screenshot();
    dialog->setCursor((int)aurore::CursorPosition::UNSET);
    h.step(2);
    AURORE_CHECK(aurore::test::compareImages(h.screenshot(), with).differingPixels > 0);

    h.tap(aurore::BUTTON_Y);
    AURORE_CHECK(cleared);
    aurore::Application::setReduceMotion(false);
}

namespace
{
// The demo in `locale`
struct LocaleHarness : Demo, Harness
{
    explicit LocaleHarness(const std::string& locale)
        : Harness(optionsFor(locale))
    {
        demo::registerViews();
        push(new MainActivity());
        settle();
    }

    static aurore::test::Options optionsFor(const std::string& locale)
    {
        aurore::test::Options options;
        options.locale = locale;
        return options;
    }
};

// Each tab of the rail in turn (focusing its item shows it), `visit` on it
template <typename H, typename F>
size_t eachDemoTab(H& h, F visit)
{
    std::vector<std::string> seen;
    for (int tab = 0; tab < 20; tab++)
    {
        std::string name = h.focusText();
        if (std::find(seen.begin(), seen.end(), name) != seen.end())
            break;
        seen.push_back(name);
        h.settle();
        visit(name);
        h.tapKey(aurore::AURORE_KBD_KEY_DOWN);
    }
    return seen.size();
}

// The texts cut on the Cadran and Settings tabs at the very large text size
std::vector<std::string> cutAtTheLargestSize(const std::string& locale)
{
    LocaleHarness h(locale);
    ui::prefs().text_size = "largest";
    text_size::apply();
    h.settle();
    AURORE_CHECK_EQ(aurore::Application::getTextScale(), text_size::LARGEST);
    std::vector<std::string> cut;
    for (const char* tab : { "demo/tabs/cadran", "demo/tabs/settings" })
    {
        if (!h.focusByText(aurore::getStr(tab), aurore::BUTTON_DOWN, 20))
            continue;
        h.tap(aurore::BUTTON_RIGHT);
        // Down the tab, its texts as they scroll into view
        for (int i = 0; i < 40; i++)
        {
            for (const std::string& text : h.cutTexts())
                if (std::find(cut.begin(), cut.end(), text) == cut.end())
                    cut.push_back(text);
            h.tap(aurore::BUTTON_DOWN);
        }
        h.tap(aurore::BUTTON_B);
    }
    for (const std::string& text : cut)
        std::fprintf(stderr, "%s, cut: %s\n", locale.c_str(), text.c_str());
    ui::prefs().text_size = "auto";
    text_size::apply();
    return cut;
}
} // namespace

// From the rail, the keyboard's arrows reach into every tab that has
// something to focus, and Escape comes back
AURORE_TEST(every_demo_screen_reachable_by_keyboard)
{
    DemoHarness h;
    // A page to read, nothing on it to act on or to scroll
    const std::string still = aurore::getStr("demo/tabs/about");
    std::vector<std::string> unreached;
    size_t tabs = eachDemoTab(h, [&](const std::string& name)
        {
        aurore::View* item = h.focus();
        h.tapKey(aurore::AURORE_KBD_KEY_RIGHT);
        if (h.focus() == item || dynamic_cast<aurore::SidebarItem*>(h.focus()))
        {
            if (name != still)
                unreached.push_back(name);
            return;
        }
        h.tapKey(aurore::AURORE_KBD_KEY_ESCAPE);
        AURORE_CHECK(h.focus() == item); });
    AURORE_CHECK(tabs > 5);
    for (const std::string& name : unreached)
        std::fprintf(stderr, "not reached: %s\n", name.c_str());
    AURORE_CHECK(unreached.empty());
}

AURORE_TEST(no_cut_text_in_de_largest)
{
    AURORE_CHECK(cutAtTheLargestSize("de").empty());
}

AURORE_TEST(no_cut_text_in_ru_largest)
{
    AURORE_CHECK(cutAtTheLargestSize("ru").empty());
}

AURORE_TEST(no_cut_text_in_fr_largest)
{
    AURORE_CHECK(cutAtTheLargestSize("fr").empty());
}

AURORE_TEST(no_cut_text_in_ja_largest)
{
    AURORE_CHECK(cutAtTheLargestSize("ja").empty());
}

// The rows of cells grow with the text: none cut at the very large size
AURORE_TEST(cells_grow_with_the_text)
{
    Harness h;
    auto* cell = new aurore::DetailCell();
    cell->setText("Brightness");
    h.push(new aurore::Activity(screenOf({ cell })));
    h.settle();
    float normal = cell->getHeight();
    aurore::Application::setTextScale(1.5f);
    h.settle();
    AURORE_CHECK(cell->getHeight() >= normal * 1.5f - 0.5f);
    aurore::Application::setTextScale(1.0f);
    h.settle();
    AURORE_CHECK_EQ(cell->getHeight(), normal);
}
