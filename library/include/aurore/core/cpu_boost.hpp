// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <mutex>

namespace aurore
{

/**
 * A boost that several jobs ask for at once, counted: it goes on with the first holder and off with
 * the last, so two overlapping jobs do not switch it off under each other. Thread safe (the jobs
 * run on worker threads). `sink` is called with true / false only when the state changes, with the
 * lock held: it must not call back into the counter.
 */
class CpuBoostCounter
{
  public:
    explicit CpuBoostCounter(std::function<void(bool)> sink = {});

    void acquire();
    /// A release with nobody holding does nothing.
    void release();
    /// Everybody lets go at once (the app's exit).
    void releaseAll();
    int holders() const;

  private:
    mutable std::mutex mutex;
    std::function<void(bool)> sink;
    int count = 0;
};

/// Holds Application's CPU boost (Application::acquireCpuBoost) for a scope: every way out lets go.
class CpuBoostGuard
{
  public:
    /// `enabled` false holds nothing (an app setting that turns the boost off).
    explicit CpuBoostGuard(bool enabled = true);
    ~CpuBoostGuard();
    CpuBoostGuard(const CpuBoostGuard&)            = delete;
    CpuBoostGuard& operator=(const CpuBoostGuard&) = delete;

  private:
    bool held;
};

} // namespace aurore
