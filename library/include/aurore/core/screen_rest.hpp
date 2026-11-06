// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <functional>

namespace aurore
{

/// Where the screen rest is: lit, warning that the panel is about to go dark, or dark.
enum class ScreenRestPhase
{
    AWAKE,
    WARNING,
    REST
};

/// What an app asks of Application::setScreenRest().
struct ScreenRestOptions
{
    /// Seconds without input before the warning (0: the rest is off).
    int idleSeconds = 0;
    /// Seconds the warning lasts before the panel goes dark.
    int warnSeconds = 10;
    /// Asked every frame: false while the app does not want the screen to rest (nothing long is
    /// running). Empty: always allowed.
    std::function<bool()> allowed;
    /// The warning begins (show a notice: any input cancels it).
    std::function<void()> onWarning;
    /// The panel went dark.
    std::function<void()> onRest;
    /// The panel is back on.
    std::function<void()> onWake;
};

/**
 * The screen rest's timing, with the clock given (Application drives it each frame, tests drive it
 * with their own times). Idle for the delay: a warning; the warning over: the panel is turned
 * off; an input then turns it back on and the idle clock starts again.
 */
class ScreenRestTimer
{
  public:
    enum class Action
    {
        NONE,
        WARN, ///< the warning begins
        CANCEL_WARNING, ///< someone is there (or the rest is no longer allowed): the warning goes
        PANEL_OFF, ///< the panel goes dark
        PANEL_ON ///< the panel comes back
    };

    /// `lastInputMs` is the last input the screen saw; `heldNow` an input right now read from the
    /// device itself (the screen sees nothing while the panel is dark).
    Action tick(int64_t nowMs, int64_t lastInputMs, bool heldNow, int idleSeconds, int warnSeconds, bool allowed);

    ScreenRestPhase phase() const { return state; }

    /// Back to lit, with the idle clock starting at `nowMs` (the app was brought back by something else).
    void reset(int64_t nowMs);

  private:
    ScreenRestPhase state = ScreenRestPhase::AWAKE;
    int64_t warnedAt      = 0; ///< when the warning began
    int64_t wokeAt        = 0; ///< when the panel last came back: the idle clock starts there at the latest
};

} // namespace aurore
