// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include "view/charts.hpp"

#include <algorithm>
#include <cmath>

Entrance::Entrance(float duration)
    : duration(duration)
{
    this->value.reset(1.0f);
}

void Entrance::start()
{
    this->value.stop();
    if (aurore::Application::getReduceMotion())
    {
        this->value.reset(1.0f);
        return;
    }
    this->value.reset(0.0f);
    this->value.addStep(1.0f, (int32_t)this->duration, aurore::EasingFunction::linear);
    this->value.setTickCallback([]
        { aurore::Application::requestRedraw(); });
    this->value.start();
}

float Entrance::at(float delay, float span) const
{
    float t = std::clamp((const_cast<aurore::Animatable&>(this->value).getValue() - delay) / span, 0.0f, 1.0f);
    return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t); // cubic out
}

// Ring

RingGauge::RingGauge(float size, float thickness)
    : aurore::Box(aurore::Axis::COLUMN)
    , thickness(thickness)
{
    this->setWidth(size);
    this->setHeight(size);
    this->setJustifyContent(aurore::JustifyContent::CENTER);
    this->setAlignItems(aurore::AlignItems::CENTER);
}

void RingGauge::setValue(float value)
{
    this->value = std::clamp(value, 0.0f, 1.0f);
    aurore::Application::requestRedraw();
}

void RingGauge::willAppear(bool resetState)
{
    aurore::Box::willAppear(resetState);
    this->grow.start();
}

void RingGauge::draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
    aurore::FrameContext* ctx)
{
    float cx = x + width / 2, cy = y + height / 2;
    float radius = std::fmin(width, height) / 2 - this->thickness / 2;

    nvgBeginPath(vg);
    nvgCircle(vg, cx, cy, radius);
    nvgStrokeWidth(vg, this->thickness);
    nvgStrokeColor(vg, ctx->theme["cadran/track"]);
    nvgStroke(vg);

    float shown = this->value * this->grow.at();
    if (shown > 0.001f)
    {
        float start = -NVG_PI / 2, end = start + shown * 2 * NVG_PI;
        nvgBeginPath(vg);
        nvgArc(vg, cx, cy, radius, start, end, NVG_CW);
        nvgLineCap(vg, NVG_ROUND);
        nvgStrokePaint(vg,
            nvgLinearGradient(vg, cx - radius, cy - radius, cx + radius, cy + radius, ctx->theme["cadran/teal_fill"],
                ctx->theme["cadran/blue"]));
        nvgStroke(vg);
        nvgLineCap(vg, NVG_BUTT);
    }

    aurore::Box::draw(vg, x, y, width, height, style, ctx);
}

// Bars

BarChart::BarChart()
    : grow(1100)
{
    this->setHeight(96);
}

void BarChart::setValues(const std::vector<float>& values)
{
    this->values = values;
    aurore::Application::requestRedraw();
}

void BarChart::willAppear(bool resetState)
{
    aurore::View::willAppear(resetState);
    this->grow.start();
}

void BarChart::draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
    aurore::FrameContext* ctx)
{
    size_t count = this->values.size();
    if (count == 0)
        return;
    const float gap = 10;
    float bar       = (width - gap * (count - 1)) / count;
    for (size_t i = 0; i < count; i++)
    {
        float grown = this->grow.at(0.06f * i, 0.6f);
        float tall  = std::fmax(bar * 0.5f, height * this->values[i] * grown);
        float bx = x + i * (bar + gap), by = y + height - tall;

        // The well, then the bar
        nvgBeginPath(vg);
        nvgRoundedRect(vg, bx, y, bar, height, std::fmin(8.0f, bar / 2));
        nvgFillColor(vg, ctx->theme["cadran/track"]);
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, bx, by, bar, tall, std::fmin(8.0f, bar / 2));
        nvgFillColor(vg, ctx->theme[i + 1 == count ? "cadran/teal_fill" : "cadran/bar"]);
        nvgFill(vg);
    }
}

// Easing curve

EasingPlot::EasingPlot(aurore::EasingFunction easing)
    : easing(easing)
{
    this->setHeight(96);
}

void EasingPlot::setProgress(float progress)
{
    this->progress = progress;
}

void EasingPlot::draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
    aurore::FrameContext* ctx)
{
    // Room above and below for the curves that go past their end (back, elastic)
    float top = y + height * 0.18f, bottom = y + height * 0.82f, span = bottom - top;

    nvgBeginPath(vg);
    nvgMoveTo(vg, x, bottom);
    nvgLineTo(vg, x + width, bottom);
    nvgMoveTo(vg, x, top);
    nvgLineTo(vg, x + width, top);
    nvgStrokeWidth(vg, 1.0f);
    nvgStrokeColor(vg, ctx->theme["cadran/line"]);
    nvgStroke(vg);

    const int steps = 48;
    nvgBeginPath(vg);
    for (int i = 0; i <= steps; i++)
    {
        float t  = (float)i / steps;
        float px = x + width * t, py = bottom - span * aurore::ease(this->easing, t);
        if (i == 0)
            nvgMoveTo(vg, px, py);
        else
            nvgLineTo(vg, px, py);
    }
    nvgStrokeWidth(vg, 2.5f);
    nvgLineJoin(vg, NVG_ROUND);
    nvgStrokeColor(vg, ctx->theme["cadran/teal"]);
    nvgStroke(vg);

    float t = std::clamp(this->progress, 0.0f, 1.0f);
    nvgBeginPath(vg);
    nvgCircle(vg, x + width * t, bottom - span * aurore::ease(this->easing, t), 6);
    nvgFillColor(vg, ctx->theme["cadran/text"]);
    nvgFill(vg);
}
