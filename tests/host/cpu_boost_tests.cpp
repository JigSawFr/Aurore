// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// Host tests for core/cpu_boost.hpp: the counted boost, with the platform given as a callback.
#include "check.h"

#include <aurore/core/cpu_boost.hpp>

#include <thread>
#include <vector>

using aurore::CpuBoostCounter;

static void test_first_on_last_off()
{
    std::vector<bool> calls;
    CpuBoostCounter c([&](bool on)
        { calls.push_back(on); });
    c.acquire();
    c.acquire();
    CHECK(c.holders() == 2);
    CHECK(calls.size() == 1 && calls[0] == true); // the second holder changes nothing
    c.release();
    CHECK(calls.size() == 1); // one still holds
    c.release();
    CHECK(calls.size() == 2 && calls[1] == false);
    CHECK(c.holders() == 0);
}

static void test_release_without_holder()
{
    int calls = 0;
    CpuBoostCounter c([&](bool)
        { calls++; });
    c.release();
    c.releaseAll();
    CHECK(calls == 0 && c.holders() == 0);
    c.acquire();
    c.release();
    c.release(); // one too many: nothing, and the count stays right
    c.acquire();
    CHECK(c.holders() == 1 && calls == 3);
}

static void test_release_all()
{
    std::vector<bool> calls;
    CpuBoostCounter c([&](bool on)
        { calls.push_back(on); });
    c.acquire();
    c.acquire();
    c.acquire();
    c.releaseAll();
    CHECK(c.holders() == 0);
    CHECK(calls.size() == 2 && calls[1] == false);
    c.acquire(); // starts again from nothing
    CHECK(c.holders() == 1 && calls.size() == 3 && calls[2] == true);
}

static void test_no_sink()
{
    CpuBoostCounter c;
    c.acquire();
    c.release();
    CHECK(c.holders() == 0);
}

static void test_threads()
{
    int ons = 0, offs = 0;
    CpuBoostCounter c([&](bool on)
        { on ? ons++ : offs++; }); // called with the lock held: no race on these
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; t++)
        threads.emplace_back([&]
            {
                for (int k = 0; k < 1000; k++)
                {
                    c.acquire();
                    c.release();
                } });
    for (auto& t : threads)
        t.join();
    CHECK(c.holders() == 0);
    CHECK(ons == offs && ons >= 1);
}

int main()
{
    test_first_on_last_off();
    test_release_without_holder();
    test_release_all();
    test_no_sink();
    test_threads();
    CHECK_DONE("cpu_boost");
}
