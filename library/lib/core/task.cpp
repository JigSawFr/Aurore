// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/task.hpp>

namespace aurore
{

RepeatingTask::RepeatingTask(Time period)
{
    this->setPeriod(period);

    this->setCallback([this]()
        { this->run(); });
}

void RepeatingTask::start()
{
    RepeatingTimer::start();
}

void RepeatingTask::stop()
{
    RepeatingTimer::stop();
}

} // namespace aurore
