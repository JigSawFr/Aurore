/*
    Copyright 2026 JigSawFr

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

#include <borealis/core/event.hpp>
#include <borealis/core/time.hpp>

#include <cstdint>
#include <functional>

namespace brls
{

// What one iteration of the main loop cost, measured while the Profiler is
// enabled. Times are real microseconds, even under the test harness'
// simulated clock.
struct FrameProfile
{
    Time events    = 0; // the platform's event pump (Platform::mainLoopIteration, which may wait)
    Time input     = 0; // touch, mouse, buttons and keys handled
    Time update    = 0; // highlight, tickings, sync tasks, run loop event, views deleted
    Time layout    = 0; // yoga layout, wherever it ran (not counted in the phase it ran in)
    Time draw      = 0; // the views, highlight and notifications drawn into nanovg
    Time inspector = 0; // the inspector's panel drawn, while it is open
    Time flush     = 0; // nanovg's flush (nvgEndFrame)
    Time swap      = 0; // VideoContext::endFrame (buffer swap, present)
    Time total     = 0;
    Time interval  = 0; // since the iteration before started (0 for the first one measured)

    uint32_t layouts      = 0; // layout passes (a tree laid out from its root)
    uint32_t viewsLaidOut = 0; // views given a new layout (yoga NodeLayout events)
    uint32_t drawCalls    = 0; // nanovg's draw calls for the views (the inspector's not counted)
    uint32_t triangles    = 0; // and their triangles
    uint64_t allocations  = 0; // through the allocation counter, 0 without one
    bool drawn            = false; // false: redraw on demand skipped the frame
};

// Per-frame cost of the main loop: time per phase, layouts, allocations.
// Off by default; on with setEnabled(true) or BRLS_PROFILE=1 in the
// environment (desktop), which also logs averages every 120 frames. The
// inspector's panel shows it, and measures while it is open.
// The bench (tests/bench.cpp) reads it frame by frame.
class Profiler
{
  public:
    static void setEnabled(bool enabled);
    static bool isEnabled() { return enabled; }

    // Enabled, or the inspector open
    static bool isMeasuring();

    // The last main-loop iteration measured
    static const FrameProfile& getLastFrame() { return last; }

    // Fired after each iteration measured; getLastFrame() has it
    static Event<>* getFrameEvent() { return &frameEvent; }

    // A function returning the number of allocations made so far. Counting
    // them is the program's business (a replaced global operator new, as
    // the bench does); without one, allocations stay 0.
    static void setAllocationCounter(std::function<uint64_t()> counter) { allocationCounter = std::move(counter); }

    // The library's own calls
    enum class Phase
    {
        EVENTS,
        INPUT,
        UPDATE,
        DRAW,
        INSPECTOR,
        FLUSH,
        SWAP,
    };
    static void beginFrame();
    static void endPhase(Phase phase);
    static void endFrame(bool drawn);
    static void addLayout(Time duration);
    static void addViewLaidOut();
    static void setDrawStats(uint32_t drawCalls, uint32_t triangles);

  private:
    inline static bool enabled = false;
    inline static bool logging = false;
    inline static FrameProfile current;
    inline static FrameProfile last;
    inline static Time phaseStart  = 0;
    inline static Time frameStart  = 0;
    inline static Time lastStart   = 0; // frameStart of the iteration before
    inline static Time layoutMark  = 0; // current.layout when the phase started
    inline static uint64_t allocationsStart = 0;
    inline static Event<> frameEvent;
    inline static std::function<uint64_t()> allocationCounter;
};

} // namespace brls
