// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// Host tests for cadran/util/held_step.hpp: 15 minutes a press, whole hours
// once an arrow repeats for long enough, a pause or the other way starts
// again, the limits hold.
#include "check.h"

#include <aurore/cadran/util/held_step.hpp>

static void test_up_then_hours()
{
    cadran::Held held;
    int64_t t = 0;
    int m     = cadran::held_step(held, 30, 15, t);
    CHECK(m == 45);
    t += 250000; // the first repeat comes later than 150 ms: the count starts again
    m = cadran::held_step(held, m, 15, t);
    CHECK(m == 60);
    for (int i = 0; i < 4; i++)
        m = cadran::held_step(held, m, 15, t += 100000);
    CHECK(m == 120); // 15 min a press so far
    m = cadran::held_step(held, m, 15, t += 100000);
    CHECK(m == 180); // then the next whole hour
    m = cadran::held_step(held, 190, 15, t += 100000);
    CHECK(m == 240);
}

static void test_down_and_restart()
{
    cadran::Held down;
    int64_t t = 0;
    int m     = cadran::held_step(down, 190, -15, t);
    CHECK(m == 175);
    t += 250000;
    for (int i = 0; i < 5; i++)
        m = cadran::held_step(down, m, -15, t += 100000);
    CHECK(m == 100);
    m = cadran::held_step(down, m, -15, t += 100000);
    CHECK(m == 60);
    // The other way, or a pause: 15 min again.
    CHECK(cadran::held_step(down, 60, 15, t += 100000) == 75);
    CHECK(cadran::held_step(down, 75, 15, t += 1000000) == 90);
}

static void test_five_and_limits()
{
    cadran::Held five;
    int64_t t = 0;
    int m     = 0;
    for (int i = 0; i < 10; i++)
        m = cadran::held_step(five, 100, 5, t += 100000);
    CHECK(m == 105); // L / R never jump
    CHECK(cadran::held_step(five, 1435, 15, 0) == 1440);
    CHECK(cadran::held_step(five, 10, -15, 10000000) == 0);
    CHECK(cadran::held_step(five, 95, 15, 20000000, 100) == 100); // a lower limit
}

int main()
{
    test_up_then_hours();
    test_down_and_restart();
    test_five_and_limits();
    return CHECK_DONE("held_step tests passed");
}
