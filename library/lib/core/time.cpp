// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <aurore/core/time.hpp>

namespace aurore
{

void Ticking::updateTickings()
{
    // Update time
    static Time previousTime = 0;

    Time currentTime = getCPUTimeUsec() / 1000;
    Time delta       = previousTime == 0 ? 0 : currentTime - previousTime;

    previousTime = currentTime;

    // Update every running ticking, kill them and execute cb if they are finished
    // We have to clone the running tickings list to avoid altering it while
    // in the for loop (so if another ticking is started in a callback or during onUpdate())
    std::vector<Ticking*> tickings(Ticking::runningTickings);

    for (Ticking* ticking : tickings)
    {
        // A ticking stopped or destroyed by the callback of one before it
        // (an activity deleted at the end of its animation) is no longer
        // in the list: skip it rather than touch it.
        if (std::find(Ticking::runningTickings.begin(), Ticking::runningTickings.end(), ticking) == Ticking::runningTickings.end())
            continue;

        bool run = ticking->onUpdate(delta);

        ticking->tickCallback();

        if (!run)
            ticking->stop(true); // will remove the ticking from Ticking::runningTickings
    }
}

void Ticking::start()
{
    if (this->running)
        return;

    Ticking::runningTickings.push_back(this);

    this->running = true;

    this->onStart();
}

void Ticking::stop()
{
    this->stop(false);
}

void Ticking::stop(bool finished)
{
    if (!this->running)
        return;

    for (size_t i = 0; i < Ticking::runningTickings.size(); i++)
    {
        if (Ticking::runningTickings[i] == this)
        {
            Ticking::runningTickings.erase(Ticking::runningTickings.begin() + i);
            break;
        }
    }

    this->running = false;

    this->onStop();

    this->endCallback(finished);
}

void Ticking::setEndCallback(TickingEndCallback endCallback)
{
    this->endCallback = endCallback;
}

void Ticking::setTickCallback(TickingTickCallback tickCallback)
{
    this->tickCallback = tickCallback;
}

bool Ticking::isRunning()
{
    return this->running;
}

Ticking::~Ticking()
{
    this->stop();
}

void FiniteTicking::rewind()
{
    this->onRewind();
}

void FiniteTicking::reset()
{
    this->stop();
    this->onReset();
}

} // namespace aurore
