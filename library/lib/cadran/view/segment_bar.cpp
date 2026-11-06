// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

#include <aurore/cadran/view/segment_bar.hpp>

#include <algorithm>

namespace
{
constexpr float EASE_US = 250000.0f; // a segment taking its colour, in µs
constexpr float GAP     = 4.0f; // between two segments, px

NVGcolor mix(NVGcolor a, NVGcolor b, float t)
{
    return nvgRGBAf(a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t);
}
} // namespace

SegmentBar::SegmentBar(size_t count)
    : states(count, State::WAITING)
    , since(count, 0)
{
    this->setHeight(12); // as kit::Bar
    this->setFocusable(false);
}

void SegmentBar::set(size_t index, State state)
{
    if (index >= this->states.size() || this->states[index] == state)
        return;
    this->states[index] = state;
    this->since[index]  = aurore::getCPUTimeUsec();
}

void SegmentBar::draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
    aurore::FrameContext* ctx)
{
    (void)style;
    (void)ctx;
    const size_t n = this->states.size();
    if (!n)
        return;
    aurore::Theme theme    = aurore::Application::getTheme();
    const aurore::Time now = aurore::getCPUTimeUsec();
    const float w          = (width - GAP * (float)(n - 1)) / (float)n;
    const float radius     = std::min(height, w) / 2.0f;
    for (size_t i = 0; i < n; i++)
    {
        const NVGcolor well = theme["cadran/well"];
        NVGcolor color      = well;
        if (this->states[i] != State::WAITING)
        {
            const float t     = std::min(1.0f, (float)(now - this->since[i]) / EASE_US);
            const NVGcolor to = this->states[i] == State::DONE ? theme["cadran/teal_fill"] : theme["cadran/coral"];
            color             = mix(well, to, t);
        }
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + (float)i * (w + GAP), y, w, height, radius);
        nvgFillColor(vg, this->a(color));
        nvgFill(vg);
    }
}
