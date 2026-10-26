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

#include <borealis/core/logger.hpp>
#include <borealis/core/profiler.hpp>

#include <cstdlib>

namespace brls
{

static const int LOG_EVERY = 120;

void Profiler::setEnabled(bool on)
{
    enabled = on;
}

void Profiler::beginFrame()
{
#if !defined(__SWITCH__) && !defined(__EMSCRIPTEN__)
    static bool fromEnvironment = []
    {
        const char* value = std::getenv("BRLS_PROFILE");
        return value && *value && *value != '0';
    }();
    if (fromEnvironment && !logging)
    {
        logging = true;
        enabled = true;
    }
#endif
    if (!enabled)
        return;
    current          = FrameProfile();
    frameStart       = getRealTimeUsec();
    phaseStart       = frameStart;
    layoutMark       = 0;
    allocationsStart = allocationCounter ? allocationCounter() : 0;
}

void Profiler::endPhase(Phase phase)
{
    if (!enabled || frameStart == 0)
        return;
    Time now = getRealTimeUsec();
    // The layout run during the phase is the layout's, not the phase's
    Time spent = (now - phaseStart) - (current.layout - layoutMark);
    if (spent < 0)
        spent = 0;
    switch (phase)
    {
        case Phase::EVENTS:
            current.events += spent;
            break;
        case Phase::INPUT:
            current.input += spent;
            break;
        case Phase::UPDATE:
            current.update += spent;
            break;
        case Phase::DRAW:
            current.draw += spent;
            break;
        case Phase::SWAP:
            current.swap += spent;
            break;
    }
    phaseStart = now;
    layoutMark = current.layout;
}

void Profiler::endFrame(bool drawn)
{
    if (!enabled || frameStart == 0)
        return;
    current.drawn       = drawn;
    current.total       = getRealTimeUsec() - frameStart;
    current.allocations = allocationCounter ? allocationCounter() - allocationsStart : 0;
    last                = current;
    frameStart          = 0;
    frameEvent.fire();

    if (!logging)
        return;
    static FrameProfile sum;
    static int frames = 0;
    static int drawnFrames = 0;
    sum.events += last.events;
    sum.input += last.input;
    sum.update += last.update;
    sum.layout += last.layout;
    sum.draw += last.draw;
    sum.swap += last.swap;
    sum.total += last.total;
    sum.layouts += last.layouts;
    sum.viewsLaidOut += last.viewsLaidOut;
    sum.allocations += last.allocations;
    drawnFrames += last.drawn ? 1 : 0;
    if (++frames < LOG_EVERY)
        return;
    double n = frames;
    Logger::info("profile ({} frames, {} drawn), us per frame: events {:.0f} input {:.0f} update {:.0f} layout {:.0f} draw {:.0f} swap {:.0f} total {:.0f}; per frame: {:.2f} layouts, {:.1f} views laid out, {:.1f} allocations",
        frames, drawnFrames, sum.events / n, sum.input / n, sum.update / n, sum.layout / n, sum.draw / n, sum.swap / n, sum.total / n,
        sum.layouts / n, sum.viewsLaidOut / n, sum.allocations / n);
    sum         = FrameProfile();
    frames      = 0;
    drawnFrames = 0;
}

void Profiler::addLayout(Time duration)
{
    if (!enabled)
        return;
    current.layouts++;
    current.layout += duration;
}

} // namespace brls
