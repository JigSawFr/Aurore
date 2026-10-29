// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

#include <aurore/cadran/view/loading_bar.hpp>

#include <algorithm>

namespace
{
// How long the fill takes to reach a new value, in µs.
constexpr float EASE_US = 350000.0f;

float eased(float t) // ease-out cubic, 0..1
{
    const float r = 1.0f - t;
    return 1.0f - r * r * r;
}
} // namespace

LoadingBar::LoadingBar()
{
    this->setFocusable(false);
}

void LoadingBar::setProgress(float value)
{
    this->moveTo(value, EASE_US, false);
}

void LoadingBar::fillOver(float value, aurore::Time duration_us)
{
    this->moveTo(value, (float)duration_us, true);
}

void LoadingBar::reset()
{
    this->from = this->to = 0.0f;
    this->duration        = 1.0f;
    this->since           = aurore::getCPUTimeUsec();
}

void LoadingBar::moveTo(float value, float duration_us, bool at_steady_pace)
{
    // From where the fill is drawn now, not from the last value asked.
    const aurore::Time now = aurore::getCPUTimeUsec();
    this->from             = this->drawn(now);
    this->to               = std::clamp(value, 0.0f, 1.0f);
    this->since            = now;
    this->duration         = std::max(duration_us, 1.0f);
    this->steady           = at_steady_pace;
}

float LoadingBar::drawn(aurore::Time now) const
{
    if (!this->since)
        return this->to;
    const float t = std::min(1.0f, (float)(now - this->since) / this->duration);
    return this->from + (this->to - this->from) * (this->steady ? t : eased(t));
}

void LoadingBar::draw(NVGcontext* vg, float x, float y, float width, float height,
    aurore::Style style, aurore::FrameContext* ctx)
{
    (void)style;
    const float radius   = height / 2.0f;
    aurore::Theme& theme = ctx->theme;
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, width, height, radius);
    nvgFillColor(vg, this->a(theme[this->brand ? "cadran/brand/track" : "cadran/well"]));
    nvgFill(vg);

    const aurore::Time now = aurore::getCPUTimeUsec();
    const float value      = this->drawn(now);
    // Still moving: the next frames are drawn too (redraw on demand)
    if (this->since && now - this->since < (aurore::Time)this->duration)
        aurore::Application::requestRedraw();
    if (value <= 0.0f)
        return;
    const float w = std::max(height, width * value);
    // Teal, lighter toward its end: the end that moves.
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, w, height, radius);
    nvgFillPaint(vg, nvgLinearGradient(vg, x, y, x + w, y, this->a(theme[this->brand ? "cadran/brand/fill" : "cadran/teal_fill"]), this->a(theme[this->brand ? "cadran/brand/glow" : "aurore/highlight/color2"])));
    nvgFill(vg);
}

aurore::View* LoadingBar::create()
{
    return new LoadingBar();
}
