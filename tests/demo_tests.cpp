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
#include <borealis/test/harness.hpp>

#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>

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
