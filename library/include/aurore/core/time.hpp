/*
    Copyright 2021 natinusala

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

#pragma once

#include <libretro-common/features/features_cpu.h>
#include <libretro-common/libretro.h>

#include <atomic>
#include <ctime>

#include <functional>
#include <vector>

namespace brls
{

typedef retro_time_t Time;

namespace detail
{
    // A negative value: the real clock
    inline std::atomic<Time> simulatedTimeUsec { -1 };
    inline std::atomic<int64_t> simulatedWallTime { -1 };
    inline std::atomic<Time> simulatedWallTimeSetAt { 0 };
}

/**
 * Returns the current CPU time in microseconds: the simulated clock's while
 * one is set (setSimulatedTimeUsec()), else the system's monotonic clock.
 */
inline Time getCPUTimeUsec()
{
    Time simulated = detail::simulatedTimeUsec.load(std::memory_order_relaxed);
    return simulated >= 0 ? simulated : cpu_features_get_time_usec();
}

/**
 * Returns the system's monotonic clock in microseconds, even while a
 * simulated clock is set (to measure how long something really took).
 */
inline Time getRealTimeUsec()
{
    return cpu_features_get_time_usec();
}

/**
 * Replaces the clock read by getCPUTimeUsec() (animations, timers, delay(),
 * button repeat...) with one that only moves when told: frames then run
 * without waiting for real time, the same way every run. Used by the test
 * harness (brls::test::Harness). A negative value goes back to the real clock.
 */
inline void setSimulatedTimeUsec(Time usec)
{
    detail::simulatedTimeUsec.store(usec, std::memory_order_relaxed);
}

inline bool isTimeSimulated()
{
    return detail::simulatedTimeUsec.load(std::memory_order_relaxed) >= 0;
}

/**
 * The wall clock (the dates and times shown), in seconds since the epoch:
 * the system's, or the one set with setSimulatedWallTime(), which moves on
 * with getCPUTimeUsec().
 */
inline std::time_t getWallTime()
{
    int64_t start = detail::simulatedWallTime.load(std::memory_order_relaxed);
    if (start < 0)
        return std::time(nullptr);
    return (std::time_t)(start + (getCPUTimeUsec() - detail::simulatedWallTimeSetAt.load(std::memory_order_relaxed)) / 1000000);
}

/**
 * Sets the wall clock to `seconds` since the epoch from now on (the test
 * harness: the same date on every screenshot). A negative value goes back
 * to the system's.
 */
inline void setSimulatedWallTime(int64_t seconds)
{
    detail::simulatedWallTimeSetAt.store(getCPUTimeUsec(), std::memory_order_relaxed);
    detail::simulatedWallTime.store(seconds, std::memory_order_relaxed);
}

/**
 * std::localtime() without its shared buffer: safe on any thread (the
 * logger runs on the task thread too).
 */
inline std::tm localTime(std::time_t time)
{
    std::tm result {};
#ifdef _WIN32
    localtime_s(&result, &time);
#else
    localtime_r(&time, &result);
#endif
    return result;
}

typedef std::function<void()> TickingGenericCallback;

typedef std::function<void(bool)> TickingEndCallback;
typedef TickingGenericCallback TickingTickCallback;

// Interface representing something that "ticks" every frame for a certain amount of frames,
// like a timer, an animation, a background task...
// The library manages a list of running tickings. Each ticking is reponsible for managing its own
// lifetime by returning true or false in onUpdate.
class Ticking
{
  public:
    virtual ~Ticking();

    /**
     * Starts the ticking, pushing it to the list of running tickings.
     * If the ticking is finished, it will be restarted.
     * If the ticking is already running, this method will have no effect.
     */
    void start();

    /**
     * Stops the ticking if it was running, and executes the end callback.
     */
    void stop();

    /**
     * Sets a callback to be executed when the
     * ticking finishes.
     * The callback argument will be set to true if the ticking stopped
     * on its own, false if it was stopped early by the user.
     */
    void setEndCallback(TickingEndCallback endCallback);

    /**
     * Sets a callback to be executed at every tick
     * until the ticking finishes.
     *
     * The last animation tick will execute the tick callback
     * then the end callback.
     */
    void setTickCallback(TickingTickCallback tickCallback);

    /**
     * Returns true if the ticking is currently running.
     */
    bool isRunning();

    /**
     * Called internally by the main loop. Takes all running tickings
     * and updates them.
     */
    static void updateTickings();

    inline static std::vector<Ticking*> runningTickings;

  protected:
    /**
     * Executed every frame while the ticking lives.
     * Delta is the time difference in ms between the last frame
     * and the current one.
     * Must return false if the ticking is finished and should be
     * removed from the list of active tickings.
     * The end callback will automatically be called then.
     */
    virtual bool onUpdate(Time delta) = 0;

    /**
     * Called when the ticking becomes active.
     */
    virtual void onStart() {};

    /**
     * Called when the ticking is stopped, either by the user
     * or because it finished.
     */
    virtual void onStop() {};

  private:
    void stop(bool finished);

    bool running = false;

    TickingEndCallback endCallback   = [](bool finished) {};
    TickingTickCallback tickCallback = [] {};
};

// Represents a "finite" ticking that runs for a known amount of time
// and can be seek / reset / rewound.
class FiniteTicking : public Ticking
{
  public:
    /**
     * Rewinds the ticking to go back to the beginning
     * without losing its state (as opposed to reset() that clears
     * everything in the ticking).
     * Does not start or stop it.
     */
    void rewind();

    /**
     * Stops and resets the ticking, clearing its state
     * in the process (as opposed to rewind() that just restarts
     * the ticking from the beginning without losing the state).
     */
    void reset();

  protected:
    /**
     * Called when the ticking gets rewound.
     */
    virtual void onRewind() {};

    /**
     * Called when the ticking gets reset.
     */
    virtual void onReset() {};
};

} // namespace brls
