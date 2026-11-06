// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/screen_rest.hpp>

namespace aurore
{

ScreenRestTimer::Action ScreenRestTimer::tick(int64_t nowMs, int64_t lastInputMs, bool heldNow, int idleSeconds, int warnSeconds, bool allowed)
{
    const bool enabled = allowed && idleSeconds > 0;
    // The input that woke the panel was not the screen's (its inputs are skipped while dark), so
    // lastInputMs does not show it: the idle clock starts at the wake-up at the latest.
    const int64_t idleSince = lastInputMs > wokeAt ? lastInputMs : wokeAt;
    const bool idleLong     = enabled && nowMs - idleSince >= (int64_t)idleSeconds * 1000;

    switch (state)
    {
        case ScreenRestPhase::AWAKE:
            if (idleLong && !heldNow)
            {
                state    = ScreenRestPhase::WARNING;
                warnedAt = nowMs;
                return Action::WARN;
            }
            return Action::NONE;
        case ScreenRestPhase::WARNING:
            if (!enabled || heldNow || lastInputMs > warnedAt)
            {
                state = ScreenRestPhase::AWAKE;
                return Action::CANCEL_WARNING;
            }
            if (nowMs - warnedAt >= (int64_t)warnSeconds * 1000)
            {
                state = ScreenRestPhase::REST;
                return Action::PANEL_OFF;
            }
            return Action::NONE;
        case ScreenRestPhase::REST:
            if (!enabled || heldNow)
            {
                state  = ScreenRestPhase::AWAKE;
                wokeAt = nowMs;
                return Action::PANEL_ON;
            }
            return Action::NONE;
    }
    return Action::NONE;
}

void ScreenRestTimer::reset(int64_t nowMs)
{
    state  = ScreenRestPhase::AWAKE;
    wokeAt = nowMs;
}

} // namespace aurore
