// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdint.h>

#include <vector>
#include <aurore/core/time.hpp>

namespace aurore
{

// The easings of Robert Penner's set, as Tweeny names them (animation.cpp
// draws them with Tweeny's)
enum class EasingFunction
{
    def,
    linear,
    stepped,
    quadraticIn,
    quadraticOut,
    quadraticInOut,
    cubicIn,
    cubicOut,
    cubicInOut,
    quarticIn,
    quarticOut,
    quarticInOut,
    quinticIn,
    quinticOut,
    quinticInOut,
    sinusoidalIn,
    sinusoidalOut,
    sinusoidalInOut,
    exponentialIn,
    exponentialOut,
    exponentialInOut,
    circularIn,
    circularOut,
    circularInOut,
    bounceIn,
    bounceOut,
    bounceInOut,
    elasticIn,
    elasticOut,
    elasticInOut,
    backIn,
    backOut,
    backInOut,
};

/// `easing`'s curve at `position` (0 to 1), from 0 to 1: what an Animatable
/// moves by, for a view that draws its own motion
float ease(EasingFunction easing, float position);

// An animatable is a float which value can be animated from an initial value to a target value,
// during a given amount of time. An easing function can also be specified.
//
// Declare the animatable and then use reset(initialValue) to reset the animation.
// Add as many steps as you like by calling addStep(targetValue, duration, easing) one or multiple times.
// Then, start and stop the animation with start() and stop().
//
// setEndCallback() and setTickCallback() allow you to execute code as long as the animation runs and / or once when it finishes.
// Use .getValue() to get the current value at any time.
//
// An animatable has overloads for float conversion, comparison (==) and assignment operator (=) to allow
// basic usage as a simple float. Assignment operator is a shortcut to the reset() method.
class Animatable : public FiniteTicking
{
  public:
    /**
     * Creates an animatable with the given initial value.
     */
    Animatable(float value = 0.0f);

    /**
     * Returns the current animatable value.
     */
    float getValue();

    /**
     * Stops and resets the animation, going back to the given initial value.
     * All steps are removed.
     * If an animation was already ongoing for that animatable, its end callback
     * will be called.
     */
    void reset(float initialValue);

    /**
     * Stops and resets the animation. The value will stay where it's at.
     * All steps are removed.
     * If an animation was already ongoing for that animatable, its end callback
     * will be called.
     */
    void reset();

    /**
     * Adds an animation step to the target value, lasting the specified duration in milliseconds.
     *
     * An animation can have multiple steps. Target value can be greater and lower than the previous step (it can go forwards or backwards).
     * Easing function is optional, default is EasingFunction::linear.
     *
     * Duration is int32_t due to internal limitations, so a step cannot last for longer than 2 147 483 647ms.
     * The sum of the duration of all steps cannot exceed 71582min.
     */
    void addStep(float targetValue, int32_t duration, EasingFunction easing = EasingFunction::linear);

    /**
     * Returns the progress of the animation between 0.0f and 1.0f.
     */
    float getProgress();

    operator float() const;
    operator float();
    void operator=(const float value);
    bool operator==(const float value);

  protected:
    bool onUpdate(Time delta) override;

    void onReset() override;
    void onRewind() override;

  private:
    struct Step
    {
        float target;
        int32_t duration; // ms
        EasingFunction easing;
    };

    // The value where the steps start (reset()), the steps, the time spent
    // in them and their total
    float currentValue = 0.0f;
    float startValue   = 0.0f;
    std::vector<Step> steps;
    int64_t elapsed = 0;
    int64_t total   = 0;

    float valueAt(int64_t time) const;
};

void updateHighlightAnimation();
void getHighlightAnimation(float* gradient_x, float* gradient_y, float* color);

// Holds the focus highlight's moving glow where it is, so that a screenshot
// does not depend on when it was taken.
void setHighlightAnimationStill(bool still);

} // namespace aurore
