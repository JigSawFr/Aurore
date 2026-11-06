// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

// held_step — a duration set with the stick: an arrow held steps 15 minutes
// at a time, then whole hours once it has repeated for long enough; a pause or
// the other way starts again. No window, no Aurore: it is tested on its own.
#pragma once

#include <cstdint>

namespace cadran
{

/// What is kept between two presses of the same arrow.
struct Held
{
    int presses     = 0; ///< in a row, signed by their way
    int64_t last_us = 0;
};

/// The new value, 0 to `max` minutes, after a press of `delta` minutes at
/// `now_us` (a repeat comes within 150 ms of the last press). `held` is kept
/// between presses.
int held_step(Held& held, int minutes, int delta, int64_t now_us, int max = 24 * 60);

} // namespace cadran
