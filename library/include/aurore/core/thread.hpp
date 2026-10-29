// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <unistd.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

namespace aurore
{

struct DelayOperation
{
#ifdef PS4
    uint64_t startPoint;
#else
    // getCPUTimeUsec(): the test harness' simulated clock when it runs
    int64_t startPoint;
#endif
    long delayMilliseconds;
    size_t index;
    std::function<void()> func;
};

typedef std::vector<DelayOperation>::iterator DelayOperationIterator;

/**
 * Enqueue a function to be executed before
 * the application is redrawn the next time.
 *
 * Aurore is not thread-safe, and sync() provides a mechanism
 * for queuing up UI-related state changes from other threads.
 *
 * It's a shortcut for aurore::Application::sync(&func);
 */
extern void sync(std::function<void()> func);

/**
 * Enqueue a function to be executed in
 * parallel with application's main thread.
 *
 * It's a shortcut for aurore::Application::async(&func);
 */
extern void async(std::function<void()> func);

extern size_t delay(long milliseconds, std::function<void()> func);

extern void cancelDelay(size_t iter);

// The free functions above are the API for apps; Threading is what the
// application runs them with (start, stop, performSyncTasks each frame)
class Threading
{
  public:
    /**
     * Enqueue a function to be executed before
     * the application is redrawn the next time.
     *
     * Aurore is not thread-safe, and sync() provides a mechanism
     * for queuing up UI-related state changes from other threads.
     */
    static void sync(std::function<void()> func);

    /**
     * Enqueue a function to be executed in
     * parallel with application's main thread.
     */
    static void async(std::function<void()> func);

    /**
     * As async(), on a thread of its own for decoding pictures (ThemeImages,
     * Image::setImageFromFileAsync): a large picture does not hold back the
     * tasks of async() (the battery, the Wi-Fi, the app's own).
     */
    static void asyncDecode(std::function<void()> func);

    static size_t delay(long milliseconds, std::function<void()> func);

    static void cancelDelay(size_t iter);

    static void start();

    static void stop();

    static void performSyncTasks();

    /**
     * Bounds the time the sync tasks may take on a frame, in microseconds:
     * once it is spent, the tasks left run on the next frames, in order, so
     * a burst of results (a page of covers) does not stall a frame. 0, the
     * default, runs every task on the frame it was queued for.
     */
    static void setSyncTaskBudget(uint64_t microseconds);

    /**
     * True when no task is queued with sync(), async() or asyncDecode()
     * and none is running on their threads (delay()s are not counted). The test
     * harness waits for it before it calls a screen settled.
     */
    static bool isIdle();

  private:
    inline static std::mutex m_sync_mutex;
    inline static std::vector<std::function<void()>> m_sync_functions;
    inline static uint64_t m_sync_budget = 0;

    inline static std::mutex m_async_mutex;
    inline static std::vector<std::function<void()>> m_async_tasks;
    inline static std::condition_variable m_async_cv;
    inline static int m_async_running = 0; // under m_async_mutex

    inline static std::mutex m_delay_mutex;
    inline static std::vector<DelayOperation> m_delay_tasks;
    inline static std::set<size_t> m_delay_cancel_set;
    inline static std::set<size_t> m_delay_pending; // queued, not run nor cancelled
    inline static size_t m_delay_index = 0;

    inline static std::atomic<bool> task_loop_active { true };
    inline static bool task_loop_running = false; // started and not stopped

    static void* task_loop(void* a);
    static void std_task_loop();
    static void* decode_loop(void* a);

    static void start_task_loop();
};

} // namespace aurore
