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
#include <borealis/test/harness.hpp>

#include <cmath>

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
    BRLS_CHECK(h.tapView(h.findText("Settings")));
    BRLS_CHECK(h.hasText("Radio cell"));
    BRLS_CHECK(brls::Application::getInputType() == brls::InputType::TOUCH);
    BRLS_CHECK(h.matchesReference("settings_tab"));
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
