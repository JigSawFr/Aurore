// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// Application::setScreenRest on the headless platform: the warning, the panel
// turned off, the press that wakes it kept from the screen, the rest ending
// when it is no longer allowed. The timing itself is in tests/host.

#include <aurore.hpp>
#include <aurore/test/harness.hpp>
#include <aurore/test/headless_platform.hpp>

namespace
{
int s_presses = 0;

// A screen with one focused view that counts the A presses it gets
class PressCounter : public aurore::Activity
{
  public:
    aurore::View* createContentView() override
    {
        auto* box  = new aurore::Box(aurore::Axis::COLUMN);
        auto* cell = new aurore::Box(aurore::Axis::ROW);
        cell->setFocusable(true);
        cell->registerAction("Count", aurore::BUTTON_A, [](aurore::View*)
            {
                s_presses++;
                return true; });
        box->addView(cell);
        return box;
    }
};
} // namespace

AURORE_TEST(screen_rest_warns_darkens_and_keeps_the_waking_press)
{
    aurore::test::Harness h;
    s_presses = 0;
    h.push(new PressCounter());
    h.settle();

    int warned = 0, rested = 0, woke = 0;
    bool allowed = true;
    aurore::ScreenRestOptions rest;
    rest.idleSeconds = 2;
    rest.warnSeconds = 1;
    rest.allowed     = [&]
    { return allowed; };
    rest.onWarning = [&]
    { warned++; };
    rest.onRest = [&]
    { rested++; };
    rest.onWake = [&]
    { woke++; };
    aurore::Application::setScreenRest(rest);

    // Normal presses reach the screen
    h.tap(aurore::BUTTON_A);
    AURORE_CHECK_EQ(s_presses, 1);

    h.advance(1500);
    AURORE_CHECK_EQ(warned, 0);
    h.advance(1000);
    AURORE_CHECK_EQ(warned, 1);
    AURORE_CHECK(aurore::Application::getScreenRestPhase() == aurore::ScreenRestPhase::WARNING);

    h.advance(1200);
    AURORE_CHECK_EQ(rested, 1);
    AURORE_CHECK(aurore::Application::getScreenRestPhase() == aurore::ScreenRestPhase::REST);
    AURORE_CHECK(!h.platform()->panelOn);

    // The press that wakes it is not the screen's, nor is the one still held after it
    h.press(aurore::BUTTON_A);
    h.step(2);
    AURORE_CHECK(h.platform()->panelOn);
    AURORE_CHECK_EQ(woke, 1);
    h.step(3);
    AURORE_CHECK_EQ(s_presses, 1);
    h.release(aurore::BUTTON_A);
    h.step(3);
    AURORE_CHECK(aurore::Application::getScreenRestPhase() == aurore::ScreenRestPhase::AWAKE);

    // The screen takes input again, and the idle clock started at the wake-up
    h.tap(aurore::BUTTON_A);
    AURORE_CHECK_EQ(s_presses, 2);
    AURORE_CHECK_EQ(warned, 1);

    // No longer allowed: it never goes dark
    allowed = false;
    h.advance(10000);
    AURORE_CHECK_EQ(warned, 1);
    AURORE_CHECK(h.platform()->panelOn);

    aurore::Application::setScreenRest({});
}

AURORE_TEST(screen_rest_ends_when_it_is_no_longer_allowed)
{
    aurore::test::Harness h;
    h.push(new PressCounter());
    h.settle();

    bool allowed = true;
    aurore::ScreenRestOptions rest;
    rest.idleSeconds = 1;
    rest.warnSeconds = 1;
    rest.allowed     = [&]
    { return allowed; };
    aurore::Application::setScreenRest(rest);

    h.advance(2500);
    AURORE_CHECK(!h.platform()->panelOn);
    allowed = false;
    h.advance(300);
    AURORE_CHECK(h.platform()->panelOn);

    // Changing the options also brings it back
    allowed = true;
    h.advance(2500);
    AURORE_CHECK(!h.platform()->panelOn);
    aurore::Application::setScreenRest({});
    AURORE_CHECK(h.platform()->panelOn);
}

int main(int argc, char** argv)
{
    return aurore::test::runTests(argc, argv);
}
