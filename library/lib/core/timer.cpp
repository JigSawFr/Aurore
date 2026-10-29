// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/timer.hpp>

namespace aurore
{

void Timer::start(Time duration)
{
    this->duration = duration;
    this->start();
}

void Timer::start()
{
    Ticking::start();
}

void Timer::setDuration(Time duration)
{
    this->duration = duration;
}

void Timer::onStart()
{
    this->progress = 0;
}

bool Timer::onUpdate(Time delta)
{
    this->progress += delta;
    return this->progress < this->duration;
}

void Timer::onReset()
{
    this->progress = 0;
    this->duration = 0;
}

void Timer::onRewind()
{
    this->progress = 0;
}

void RepeatingTimer::start(Time period)
{
    this->period = period;
    this->start();
}

void RepeatingTimer::start()
{
    Ticking::start();
}

void RepeatingTimer::setPeriod(Time period)
{
    this->period = period;
}

void RepeatingTimer::setCallback(TickingGenericCallback callback)
{
    this->callback = callback;
}

void RepeatingTimer::onStart()
{
    this->progress = 0;
}

bool RepeatingTimer::onUpdate(Time delta)
{
    this->progress += delta;

    if (this->progress >= this->period)
    {
        this->callback();
        this->progress = 0;
    }

    return true; // never stop
}

} // namespace aurore
