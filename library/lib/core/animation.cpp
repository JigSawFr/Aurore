// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#include <tweeny/easing.h>
#include <algorithm>
#include <aurore/core/animation.hpp>
#include <aurore/core/application.hpp>

namespace aurore
{

// `easing` between `start` and `end` at `position` (0 to 1), Tweeny's curve
static float easeBetween(EasingFunction easing, float position, float start, float end)
{
    switch (easing)
    {
        case EasingFunction::def:
            return tweeny::easing::def(position, start, end);
        case EasingFunction::linear:
            return tweeny::easing::linear(position, start, end);
        case EasingFunction::stepped:
            return tweeny::easing::stepped(position, start, end);
        case EasingFunction::quadraticIn:
            return tweeny::easing::quadraticIn(position, start, end);
        case EasingFunction::quadraticOut:
            return tweeny::easing::quadraticOut(position, start, end);
        case EasingFunction::quadraticInOut:
            return tweeny::easing::quadraticInOut(position, start, end);
        case EasingFunction::cubicIn:
            return tweeny::easing::cubicIn(position, start, end);
        case EasingFunction::cubicOut:
            return tweeny::easing::cubicOut(position, start, end);
        case EasingFunction::cubicInOut:
            return tweeny::easing::cubicInOut(position, start, end);
        case EasingFunction::quarticIn:
            return tweeny::easing::quarticIn(position, start, end);
        case EasingFunction::quarticOut:
            return tweeny::easing::quarticOut(position, start, end);
        case EasingFunction::quarticInOut:
            return tweeny::easing::quarticInOut(position, start, end);
        case EasingFunction::quinticIn:
            return tweeny::easing::quinticIn(position, start, end);
        case EasingFunction::quinticOut:
            return tweeny::easing::quinticOut(position, start, end);
        case EasingFunction::quinticInOut:
            return tweeny::easing::quinticInOut(position, start, end);
        case EasingFunction::sinusoidalIn:
            return tweeny::easing::sinusoidalIn(position, start, end);
        case EasingFunction::sinusoidalOut:
            return tweeny::easing::sinusoidalOut(position, start, end);
        case EasingFunction::sinusoidalInOut:
            return tweeny::easing::sinusoidalInOut(position, start, end);
        case EasingFunction::exponentialIn:
            return tweeny::easing::exponentialIn(position, start, end);
        case EasingFunction::exponentialOut:
            return tweeny::easing::exponentialOut(position, start, end);
        case EasingFunction::exponentialInOut:
            return tweeny::easing::exponentialInOut(position, start, end);
        case EasingFunction::circularIn:
            return tweeny::easing::circularIn(position, start, end);
        case EasingFunction::circularOut:
            return tweeny::easing::circularOut(position, start, end);
        case EasingFunction::circularInOut:
            return tweeny::easing::circularInOut(position, start, end);
        case EasingFunction::bounceIn:
            return tweeny::easing::bounceIn(position, start, end);
        case EasingFunction::bounceOut:
            return tweeny::easing::bounceOut(position, start, end);
        case EasingFunction::bounceInOut:
            return tweeny::easing::bounceInOut(position, start, end);
        case EasingFunction::elasticIn:
            return tweeny::easing::elasticIn(position, start, end);
        case EasingFunction::elasticOut:
            return tweeny::easing::elasticOut(position, start, end);
        case EasingFunction::elasticInOut:
            return tweeny::easing::elasticInOut(position, start, end);
        case EasingFunction::backIn:
            return tweeny::easing::backIn(position, start, end);
        case EasingFunction::backOut:
            return tweeny::easing::backOut(position, start, end);
        case EasingFunction::backInOut:
            return tweeny::easing::backInOut(position, start, end);
    }
    return end;
}

float ease(EasingFunction easing, float position)
{
    return easeBetween(easing, position, 0.0f, 1.0f);
}

Animatable::Animatable(float value)
    : currentValue(value)
{
}

void Animatable::onReset()
{
    this->startValue = this->currentValue;
    this->steps.clear();
    this->elapsed = 0;
    this->total   = 0;
}

void Animatable::reset(float initialValue)
{
    this->currentValue = initialValue;
    FiniteTicking::reset();
}

void Animatable::reset()
{
    FiniteTicking::reset();
}

void Animatable::onRewind()
{
    this->elapsed      = 0;
    this->currentValue = this->startValue;
}

void Animatable::addStep(float targetValue, int32_t duration, EasingFunction easing)
{
    duration = std::max<int32_t>(duration, 0);
    this->steps.push_back({ targetValue, duration, easing });
    this->total += duration;
}

float Animatable::getProgress()
{
    if (this->total <= 0)
        return this->steps.empty() ? 0.0f : 1.0f;
    return std::min(1.0f, (float)this->elapsed / (float)this->total);
}

bool Animatable::onUpdate(retro_time_t delta)
{
    if (this->total <= 0 || this->elapsed >= this->total)
        return false;

    this->elapsed      = std::min<int64_t>(this->total, this->elapsed + std::max<int64_t>(delta, 0));
    this->currentValue = this->valueAt(this->elapsed);
    return true;
}

float Animatable::valueAt(int64_t time) const
{
    // Each step from where the previous one ends, a step of no duration at once
    float from     = this->startValue;
    int64_t offset = 0;
    for (const Step& step : this->steps)
    {
        if (time < offset + step.duration)
            return easeBetween(step.easing, (float)(time - offset) / (float)step.duration, from, step.target);
        offset += step.duration;
        from = step.target;
    }
    return from;
}

float Animatable::getValue()
{
    return this->currentValue;
}

Animatable::operator float() const
{
    return this->currentValue;
}

Animatable::operator float()
{
    return this->currentValue;
}

void Animatable::operator=(const float value)
{
    // Views set the same value on every frame (an alpha, a scroll
    // indicator): resetting cleared the steps each time, an allocation per
    // frame. Nothing to reset when it holds that value with no step pending.
    if (!this->isRunning() && this->currentValue == value && this->total == 0)
        return;

    this->reset(value);
}

bool Animatable::operator==(const float value)
{
    return this->currentValue == value;
}

#define HIGHLIGHT_SPEED 125.0f

static double highlightGradientX = 0;
static double highlightGradientY = 0;
static double highlightColor     = 0;
static bool highlightStill       = false;

void setHighlightAnimationStill(bool still)
{
    highlightStill = still;
}

void updateHighlightAnimation()
{
    if (highlightStill || !Application::hasActiveEvent())
        return;

    Time currentTime = getCPUTimeUsec() / 1000;

    // Update variables
    highlightGradientX = (cos((double)currentTime / HIGHLIGHT_SPEED / 3.0) + 1.0) / 2.0;
    highlightGradientY = (sin((double)currentTime / HIGHLIGHT_SPEED / 3.0) + 1.0) / 2.0;
    highlightColor     = (sin((double)currentTime / HIGHLIGHT_SPEED * 2.0) + 1.0) / 2.0;
}

void getHighlightAnimation(float* gradientX, float* gradientY, float* color)
{
    if (gradientX)
        *gradientX = (float)highlightGradientX;

    if (gradientY)
        *gradientY = (float)highlightGradientY;

    if (color)
        *color = (float)highlightColor;
}

} // namespace aurore
