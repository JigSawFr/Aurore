// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

#include <aurore/cadran/util/held_step.hpp>

#include <algorithm>
#include <cstdlib>

namespace cadran
{

int held_step(Held& held, int minutes, int delta, int64_t now_us, int max)
{
    // A repeat comes within 150 ms of the last press; a longer pause starts
    // the count again.
    constexpr int64_t REPEAT_US = 150000;
    constexpr int HOURS_AFTER   = 5; // repeats in a row
    const bool up               = delta > 0;
    const bool repeat           = now_us - held.last_us < REPEAT_US && held.presses != 0 && (held.presses > 0) == up;
    held.presses                = repeat ? held.presses + (up ? 1 : -1) : (up ? 1 : -1);
    held.last_us                = now_us;
    int next                    = minutes + delta;
    if (std::abs(delta) == 15 && std::abs(held.presses) > HOURS_AFTER)
        next = up ? (minutes / 60 + 1) * 60 : ((minutes + 59) / 60 - 1) * 60; // the next whole hour that way
    return std::max(0, std::min(max, next));
}

} // namespace cadran
