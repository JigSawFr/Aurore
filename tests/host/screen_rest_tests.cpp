// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// Host tests for core/screen_rest.hpp: the rest's timing with the clock given.
#include "check.h"

#include <aurore/core/screen_rest.hpp>

using aurore::ScreenRestPhase;
using aurore::ScreenRestTimer;
using Action = ScreenRestTimer::Action;

static constexpr int64_t S = 1000;

static void test_off_and_not_allowed()
{
    ScreenRestTimer t;
    CHECK(t.tick(1000 * S, 0, false, 0, 10, true) == Action::NONE); // 0 seconds: off
    CHECK(t.tick(1000 * S, 0, false, 300, 10, false) == Action::NONE); // not allowed now
    CHECK(t.phase() == ScreenRestPhase::AWAKE);
}

static void test_warning_then_dark_then_back()
{
    ScreenRestTimer t;
    CHECK(t.tick(299 * S, 0, false, 300, 10, true) == Action::NONE);
    CHECK(t.tick(300 * S, 0, false, 300, 10, true) == Action::WARN && t.phase() == ScreenRestPhase::WARNING);
    CHECK(t.tick(309 * S, 0, false, 300, 10, true) == Action::NONE);
    CHECK(t.tick(310 * S, 0, false, 300, 10, true) == Action::PANEL_OFF && t.phase() == ScreenRestPhase::REST);
    CHECK(t.tick(500 * S, 0, false, 300, 10, true) == Action::NONE);
    CHECK(t.tick(501 * S, 0, true, 300, 10, true) == Action::PANEL_ON && t.phase() == ScreenRestPhase::AWAKE);
}

static void test_input_cancels_the_warning()
{
    ScreenRestTimer t;
    CHECK(t.tick(300 * S, 0, false, 300, 10, true) == Action::WARN);
    CHECK(t.tick(302 * S, 301 * S, false, 300, 10, true) == Action::CANCEL_WARNING);
    CHECK(t.phase() == ScreenRestPhase::AWAKE);
    CHECK(t.tick(303 * S, 301 * S, false, 300, 10, true) == Action::NONE);
}

static void test_not_allowed_brings_it_back()
{
    ScreenRestTimer t;
    t.tick(300 * S, 0, false, 300, 10, true);
    CHECK(t.tick(301 * S, 0, false, 300, 10, false) == Action::CANCEL_WARNING);
    t.tick(1000 * S, 0, false, 300, 10, true);
    t.tick(1010 * S, 0, false, 300, 10, true);
    CHECK(t.phase() == ScreenRestPhase::REST);
    CHECK(t.tick(1011 * S, 0, false, 300, 10, false) == Action::PANEL_ON);
    CHECK(t.tick(1012 * S, 0, false, 0, 10, true) == Action::NONE);
}

static void test_idle_clock_restarts_at_the_wake()
{
    // The waking input was not the screen's: its last input stays old, the wake counts as one.
    ScreenRestTimer t;
    t.tick(300 * S, 0, false, 300, 10, true);
    t.tick(310 * S, 0, false, 300, 10, true);
    CHECK(t.tick(2000 * S, 0, true, 300, 10, true) == Action::PANEL_ON);
    CHECK(t.tick(2001 * S, 0, false, 300, 10, true) == Action::NONE);
    CHECK(t.tick(2299 * S, 0, false, 300, 10, true) == Action::NONE);
    CHECK(t.tick(2300 * S, 0, false, 300, 10, true) == Action::WARN);
}

static void test_held_input_at_the_deadline_and_reset()
{
    ScreenRestTimer t;
    CHECK(t.tick(300 * S, 0, true, 300, 10, true) == Action::NONE);
    t.tick(301 * S, 0, false, 300, 10, true);
    CHECK(t.phase() == ScreenRestPhase::WARNING);
    t.reset(400 * S);
    CHECK(t.phase() == ScreenRestPhase::AWAKE);
    CHECK(t.tick(500 * S, 0, false, 300, 10, true) == Action::NONE);
    CHECK(t.tick(700 * S, 0, false, 300, 10, true) == Action::WARN);
}

int main()
{
    test_off_and_not_allowed();
    test_warning_then_dark_then_back();
    test_input_cancels_the_warning();
    test_not_allowed_brings_it_back();
    test_idle_clock_restarts_at_the_wake();
    test_held_input_at_the_deadline_and_reset();
    return CHECK_DONE("screen rest tests passed");
}
