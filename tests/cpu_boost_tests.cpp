// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// Application::acquireCpuBoost and CpuBoostGuard on the headless platform: the platform is told
// at the first holder and the last release only. The counting itself is in tests/host.

#include <aurore.hpp>
#include <aurore/test/harness.hpp>
#include <aurore/test/headless_platform.hpp>

AURORE_TEST(cpu_boost_is_counted_and_guarded)
{
    aurore::test::Harness h;
    auto* platform   = h.platform();
    const int before = platform->cpuBoostChanges;
    AURORE_CHECK(!platform->cpuBoostOn);

    {
        aurore::CpuBoostGuard outer;
        AURORE_CHECK(platform->cpuBoostOn);
        {
            aurore::CpuBoostGuard inner;
            AURORE_CHECK_EQ(aurore::Application::getCpuBoostHolders(), 2);
        }
        AURORE_CHECK(platform->cpuBoostOn); // the outer one still holds
        aurore::CpuBoostGuard off(false); // a disabled guard holds nothing
        AURORE_CHECK_EQ(aurore::Application::getCpuBoostHolders(), 1);
    }
    AURORE_CHECK(!platform->cpuBoostOn);
    AURORE_CHECK_EQ(aurore::Application::getCpuBoostHolders(), 0);
    AURORE_CHECK_EQ((int)platform->cpuBoostChanges - before, 2); // on once, off once

    aurore::Application::releaseCpuBoost(); // nobody holds: nothing
    AURORE_CHECK_EQ((int)platform->cpuBoostChanges - before, 2);
}
