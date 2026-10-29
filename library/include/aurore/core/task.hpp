// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/timer.hpp>

namespace aurore
{

// A RepeatingTask is a task executed repeatedly on the main thread at a given period in ms
class RepeatingTask : private RepeatingTimer
{
  public:
    /**
     * Create a RepeatingTask with the given period in ms.
     *
     * The task is guaranteed to wait for at least the given period of time between
     * handler runs, but it can wait for slightly longer (usually less than a ms late)
     */
    RepeatingTask(Time period);

    /**
     * Task handler executed repeatedly on the main thread at the given period.
     */
    virtual void run() = 0;

    /**
     * Starts the task.
     */
    void start();

    /**
     * Stops the task.
     */
    void stop();
};
} // namespace aurore
