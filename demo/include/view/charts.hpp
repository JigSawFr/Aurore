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

#include <vector>
#include <aurore.hpp>

/// An entrance from 0 to 1, eased out, over `duration` ms: started again
/// each time its view appears (a tab shown), at its end at once with
/// reduced motion. A ticking: the test harness waits for it to settle.
class Entrance
{
  public:
    explicit Entrance(float duration = 900);
    void start();
    /// 0 to 1; `delay` and `span`, shares of the duration: a part of it for
    /// one of several things growing one after the other.
    float at(float delay = 0, float span = 1) const;

  private:
    float duration;
    aurore::Animatable value;
};

/// A ring: its track, and an arc from the top, clockwise, to `value` (0 to
/// 1) in the accent, growing when the ring first shows. Its children (a
/// figure, a caption) are centred in it.
class RingGauge : public aurore::Box
{
  public:
    explicit RingGauge(float size = 132, float thickness = 12);

    void setValue(float value);
    void willAppear(bool resetState) override;
    void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
        aurore::FrameContext* ctx) override;

  private:
    float value = 0, thickness;
    Entrance grow;
};

/// Bars side by side, rounded at the top, the last one (today) in the
/// lighter bar colour, growing one after the other when the chart shows,
/// over the theme's chart colours (cadran/bar, cadran/bar_today).
class BarChart : public aurore::View
{
  public:
    BarChart();

    void setValues(const std::vector<float>& values); ///< 0 to 1 each
    void willAppear(bool resetState) override;
    void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
        aurore::FrameContext* ctx) override;

  private:
    std::vector<float> values;
    Entrance grow;
};

/// A curve of an easing function over its square, and a dot running along
/// it while `progress` (0 to 1) moves; for the Motion tab.
class EasingPlot : public aurore::View
{
  public:
    explicit EasingPlot(aurore::EasingFunction easing);

    void setProgress(float progress);
    void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
        aurore::FrameContext* ctx) override;

  private:
    aurore::EasingFunction easing;
    float progress = 1;
};
