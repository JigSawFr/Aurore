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

// aurore_bench: the main loop's cost on the demo's screens, through the
// Profiler (BRLS_PROFILE), under the test harness: time per phase, layouts
// and allocations per frame, over a few scenarios.
//
//   aurore_bench [--frames N] [--json out.json]
//   aurore_bench --write-baseline tests/bench_baseline.json
//   aurore_bench --check tests/bench_baseline.json [--margin 0.25] [--time-margin 3]
//
// --check fails (exit 1) when a count (layouts, views laid out,
// allocations, frames drawn per frame) is above the baseline's by more than
// the margin (and 1). Times depend on the machine: they are checked only
// with --time-margin (a factor over the baseline's). The baseline and the
// check must run the same number of frames (--frames, 300 by default).

#include <borealis.hpp>
#include <borealis/test/harness.hpp>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <new>
#include <sstream>
#include <string>
#include <vector>

#include "activity/main_activity.hpp"
#include "demo_app.hpp"

// Every allocation of the program, counted (GCC takes malloc and free in
// these for a mismatch with new and delete)
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

static std::atomic<uint64_t> allocations { 0 };

void* operator new(std::size_t size)
{
    allocations.fetch_add(1, std::memory_order_relaxed);
    if (void* p = std::malloc(size ? size : 1))
        return p;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size)
{
    allocations.fetch_add(1, std::memory_order_relaxed);
    if (void* p = std::malloc(size ? size : 1))
        return p;
    throw std::bad_alloc();
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    allocations.fetch_add(1, std::memory_order_relaxed);
    return std::malloc(size ? size : 1);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
    allocations.fetch_add(1, std::memory_order_relaxed);
    return std::malloc(size ? size : 1);
}

void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { std::free(p); }

namespace
{

using Metrics = std::map<std::string, double>;

// What a scenario measured, over its frames
struct Run
{
    std::vector<brls::FrameProfile> frames;

    Metrics metrics() const
    {
        Metrics m;
        double n = frames.empty() ? 1 : (double)frames.size();
        auto mean = [&](auto field)
        {
            double sum = 0;
            for (auto& f : frames)
                sum += (double)field(f);
            return sum / n;
        };
        auto p95 = [&](auto field)
        {
            std::vector<double> values;
            for (auto& f : frames)
                values.push_back((double)field(f));
            if (values.empty())
                return 0.0;
            std::sort(values.begin(), values.end());
            return values[std::min(values.size() - 1, (size_t)(values.size() * 0.95))];
        };
        m["drawn_per_frame"]        = mean([](auto& f) { return f.drawn ? 1 : 0; });
        m["layouts_per_frame"]      = mean([](auto& f) { return f.layouts; });
        m["views_laid_out_per_frame"] = mean([](auto& f) { return f.viewsLaidOut; });
        m["allocations_per_frame"]  = mean([](auto& f) { return f.allocations; });
        m["us_input"]               = mean([](auto& f) { return f.input; });
        m["us_update"]              = mean([](auto& f) { return f.update; });
        m["us_layout"]              = mean([](auto& f) { return f.layout; });
        m["us_draw"]                = mean([](auto& f) { return f.draw; });
        m["us_swap"]                = mean([](auto& f) { return f.swap; });
        m["us_total"]               = mean([](auto& f) { return f.total; });
        m["us_total_p95"]           = p95([](auto& f) { return f.total; });
        return m;
    }
};

bool isTime(const std::string& metric)
{
    return metric.rfind("us_", 0) == 0;
}

// Runs `frames` frames, calling `each(i)` before each one, and records them
Run measure(brls::test::Harness& h, int frames, const std::function<void(int)>& each)
{
    Run run;
    auto subscription = brls::Profiler::getFrameEvent()->subscribe([&run]()
        { run.frames.push_back(brls::Profiler::getLastFrame()); });
    for (int i = 0; i < frames; i++)
    {
        each(i);
        h.step();
    }
    brls::Profiler::getFrameEvent()->unsubscribe(subscription);
    return run;
}

// A JSON object of objects of numbers, as written below
std::map<std::string, Metrics> readBaseline(const std::string& path)
{
    std::map<std::string, Metrics> result;
    std::ifstream file(path);
    if (!file)
        return result;
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string text = buffer.str();
    size_t pos       = 0;
    auto readString  = [&](std::string* out)
    {
        size_t start = text.find('"', pos);
        if (start == std::string::npos)
            return false;
        size_t end = text.find('"', start + 1);
        if (end == std::string::npos)
            return false;
        *out = text.substr(start + 1, end - start - 1);
        pos  = end + 1;
        return true;
    };
    pos = text.find('{');
    if (pos == std::string::npos)
        return result;
    pos++;
    std::string scenario;
    while (readString(&scenario))
    {
        size_t open  = text.find('{', pos);
        size_t close = text.find('}', open);
        if (open == std::string::npos || close == std::string::npos)
            break;
        pos = open + 1;
        std::string metric;
        while (pos < close)
        {
            size_t quote = text.find('"', pos);
            if (quote == std::string::npos || quote > close)
                break;
            readString(&metric);
            size_t colon = text.find(':', pos);
            result[scenario][metric] = std::strtod(text.c_str() + colon + 1, nullptr);
            pos = colon + 1;
        }
        pos = close + 1;
    }
    return result;
}

std::string toJson(const std::map<std::string, Metrics>& results)
{
    std::string out = "{\n";
    size_t i        = 0;
    for (auto& [scenario, metrics] : results)
    {
        out += "  \"" + scenario + "\": {\n";
        size_t j = 0;
        for (auto& [metric, value] : metrics)
        {
            char number[64];
            std::snprintf(number, sizeof(number), "%.2f", value);
            out += "    \"" + metric + "\": " + number + (++j < metrics.size() ? ",\n" : "\n");
        }
        out += std::string("  }") + (++i < results.size() ? ",\n" : "\n");
    }
    return out + "}\n";
}

} // namespace

int main(int argc, char** argv)
{
    int frames = 300;
    std::string jsonPath, baselinePath, writeBaselinePath;
    double margin     = 0.25;
    double timeMargin = 0;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        auto next       = [&]() { return i + 1 < argc ? std::string(argv[++i]) : std::string(); };
        if (arg == "--frames")
            frames = std::max(10, std::atoi(next().c_str()));
        else if (arg == "--json")
            jsonPath = next();
        else if (arg == "--check")
            baselinePath = next();
        else if (arg == "--write-baseline")
            writeBaselinePath = next();
        else if (arg == "--margin")
            margin = std::atof(next().c_str());
        else if (arg == "--time-margin")
            timeMargin = std::atof(next().c_str());
        else
        {
            std::fprintf(stderr, "usage: %s [--frames N] [--json out.json] [--check baseline.json [--margin 0.25] [--time-margin 3]] [--write-baseline path]\n", argv[0]);
            return 2;
        }
    }

    brls::Logger::setLogLevel(brls::LogLevel::LOG_WARNING);
    demo::installHooks();
    std::map<std::string, Metrics> results;
    {
        brls::test::Harness h;
        demo::registerViews();
        brls::Profiler::setAllocationCounter([]() { return allocations.load(std::memory_order_relaxed); });
        h.push(new MainActivity());
        h.settle();
        brls::Profiler::setEnabled(true);

        // The main screen, still, every frame drawn
        results["idle"] = measure(h, frames, [](int) {}).metrics();

        // Down and up through the rail: focus, highlight and scroll moving
        results["navigate"] = measure(h, frames * 2, [&](int i)
                                  {
            int phase = i % 40;
            brls::ControllerButton button = (i / 40) % 6 < 3 ? brls::BUTTON_DOWN : brls::BUTTON_UP;
            if (phase == 0)
                h.press(button);
            else if (phase == 1)
                h.release(button); })
                                  .metrics();
        h.settle();

        // Still again, with redraw on demand: frames skipped
        brls::Application::setRedrawOnDemand(true);
        h.step(3);
        results["idle_on_demand"] = measure(h, frames, [](int) {}).metrics();
        brls::Application::setRedrawOnDemand(false);

        brls::Profiler::setEnabled(false);
    }

    std::printf("%-16s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s\n", "scenario", "drawn", "layouts", "laid out", "allocs",
        "input", "update", "layout", "draw", "swap", "total");
    for (auto& [scenario, m] : results)
        std::printf("%-16s %8.2f %8.2f %8.1f %8.1f %8.0f %8.0f %8.0f %8.0f %8.0f %8.0f us\n", scenario.c_str(), m["drawn_per_frame"],
            m["layouts_per_frame"], m["views_laid_out_per_frame"], m["allocations_per_frame"], m["us_input"], m["us_update"],
            m["us_layout"], m["us_draw"], m["us_swap"], m["us_total"]);

    if (!jsonPath.empty())
        std::ofstream(jsonPath) << toJson(results);
    if (!writeBaselinePath.empty())
    {
        std::ofstream(writeBaselinePath) << toJson(results);
        std::printf("baseline written to %s\n", writeBaselinePath.c_str());
    }

    if (baselinePath.empty())
        return 0;
    auto baseline = readBaseline(baselinePath);
    if (baseline.empty())
    {
        std::fprintf(stderr, "no baseline read from %s\n", baselinePath.c_str());
        return 2;
    }
    int regressions = 0;
    for (auto& [scenario, expected] : baseline)
    {
        auto found = results.find(scenario);
        if (found == results.end())
        {
            std::fprintf(stderr, "%s: scenario missing\n", scenario.c_str());
            regressions++;
            continue;
        }
        for (auto& [metric, base] : expected)
        {
            bool time = isTime(metric);
            if (time && timeMargin <= 0)
                continue;
            double value = found->second[metric];
            double limit = time ? base * timeMargin : base * (1 + margin) + 1;
            if (value > limit)
            {
                std::fprintf(stderr, "regression: %s %s = %.2f, baseline %.2f (limit %.2f)\n", scenario.c_str(), metric.c_str(), value, base, limit);
                regressions++;
            }
        }
    }
    if (regressions)
        return 1;
    std::printf("no regression against %s\n", baselinePath.c_str());
    return 0;
}
