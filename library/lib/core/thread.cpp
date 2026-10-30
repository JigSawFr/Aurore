// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#include <exception>
#include <aurore/core/application.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/core/thread.hpp>

#ifdef AURORE_USE_STD_THREAD
#include <thread>
#elif defined(__3DS__)
#include <3ds.h>

#include <algorithm>
#else
#include <pthread.h>
#endif

#ifdef PS4
#include <orbis/libkernel.h>
#endif

namespace aurore
{

#ifdef AURORE_USE_STD_THREAD
static std::thread* task_loop_thread = nullptr;
static std::thread* decode_thread    = nullptr;
#elif defined(__3DS__)
static Thread task_loop_thread = nullptr;
static Thread decode_thread    = nullptr;

// A thread of the New 3DS: on its third core, its own (the app's is the
// first, the system's the second), else on the app's at a lower priority.
// The scheduler does not share a core by time: on the app's, it runs when
// the interface waits (for the screen, for input)
static Thread startCtrThread(ThreadFunc entry)
{
    s32 priority = 0x30;
    svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
    priority   = std::min<s32>(priority + 1, 0x3F);
    bool isNew = false;
    APT_CheckNew3DS(&isNew);
    Thread thread = isNew ? threadCreate(entry, nullptr, 128 * 1024, priority, 2, false) : nullptr;
    if (!thread)
        thread = threadCreate(entry, nullptr, 128 * 1024, priority, -2, false);
    return thread;
}
#else
static pthread_t task_loop_thread = pthread_t(0);
static pthread_t decode_thread    = pthread_t(0);
#endif

// asyncDecode()'s queue, as async()'s
static std::mutex decode_mutex;
static std::condition_variable decode_cv;
static std::vector<std::function<void()>> decode_tasks;
static int decode_running = 0; // under decode_mutex
static std::atomic<bool> decode_thread_running { false };

// Runs the tasks of a queue as they come, until `active` is false
static void runQueue(std::mutex& mutex, std::condition_variable& cv, std::vector<std::function<void()>>& queue,
    int& running, const std::atomic<bool>& active, const char* what)
{
    while (true)
    {
        std::vector<std::function<void()>> tasks;
        {
            // Woken as soon as a task is queued, instead of polling the
            // queue every 500 ms
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [&]
                { return !active || !queue.empty(); });
            if (!active)
                break;
            tasks.swap(queue);
            running = (int)tasks.size();
        }

        for (auto& task : tasks)
        {
            // An exception thrown by a task would end the program from
            // this thread: log it and go on with the next one
            try
            {
                task();
            }
            catch (std::exception& e)
            {
                aurore::Logger::error("error: {}: {}", what, e.what());
            }
            catch (...)
            {
                aurore::Logger::error("error: {}: unknown exception", what);
            }
            std::lock_guard<std::mutex> lock(mutex);
            running--;
        }
    }
}

void sync(std::function<void()> func)
{
    Threading::sync(std::move(func));
}

void async(std::function<void()> task)
{
    Threading::async(std::move(task));
}

size_t delay(long milliseconds, std::function<void()> func)
{
    return Threading::delay(milliseconds, std::move(func));
}

void cancelDelay(size_t iter)
{
    Threading::cancelDelay(iter);
}

void Threading::sync(std::function<void()> func)
{
    std::lock_guard<std::mutex> guard(m_sync_mutex);
    m_sync_functions.push_back(std::move(func));
}

void Threading::async(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> guard(m_async_mutex);
        m_async_tasks.push_back(std::move(task));
    }
    m_async_cv.notify_one();
}

void Threading::asyncDecode(std::function<void()> task)
{
    // No thread of its own (it could not be made): with the other tasks
    if (!decode_thread_running)
    {
        Threading::async(std::move(task));
        return;
    }
    {
        std::lock_guard<std::mutex> guard(decode_mutex);
        decode_tasks.push_back(std::move(task));
    }
    decode_cv.notify_one();
}

size_t Threading::delay(long milliseconds, std::function<void()> func)
{
    std::lock_guard<std::mutex> guard(m_delay_mutex);
    DelayOperation operation;
#ifdef PS4
    operation.startPoint        = sceKernelGetProcessTime();
    operation.delayMilliseconds = milliseconds * 1000;
#else
    operation.startPoint        = getCPUTimeUsec();
    operation.delayMilliseconds = milliseconds;
#endif
    operation.func  = std::move(func);
    operation.index = ++m_delay_index;
    m_delay_tasks.push_back(operation);
    m_delay_pending.insert(operation.index);
    return m_delay_index;
}

void Threading::cancelDelay(size_t iter)
{
    std::lock_guard<std::mutex> guard(m_delay_mutex);
    // An index run already, cancelled already or not given yet is ignored:
    // kept, it would stay in the set (or cancel a later delay)
    if (m_delay_pending.erase(iter))
        m_delay_cancel_set.insert(iter);
}

bool Threading::isIdle()
{
    {
        std::lock_guard<std::mutex> guard(m_async_mutex);
        if (!m_async_tasks.empty() || m_async_running > 0)
            return false;
    }
    {
        std::lock_guard<std::mutex> guard(decode_mutex);
        if (!decode_tasks.empty() || decode_running > 0)
            return false;
    }
    std::lock_guard<std::mutex> guard(m_sync_mutex);
    return m_sync_functions.empty();
}

void Threading::setSyncTaskBudget(uint64_t microseconds)
{
    m_sync_budget = microseconds;
}

void Threading::performSyncTasks()
{
    std::vector<std::function<void()>> local;
    m_sync_mutex.lock();
    local.swap(m_sync_functions);
    m_sync_mutex.unlock();

    // A task run from another thread's result (an image loaded...) may
    // change the screen
    if (!local.empty())
        Application::requestRedraw();

    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < local.size(); i++)
    {
        try
        {
            local[i]();
        }
        catch (std::exception& e)
        {
            aurore::Logger::error("error: performSyncTasks: {}", e.what());
        }
        catch (...)
        {
            aurore::Logger::error("error: performSyncTasks: unknown exception");
        }

        // Over the budget: the tasks left go back first in the queue
        if (m_sync_budget > 0 && i + 1 < local.size()
            && (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count() >= m_sync_budget)
        {
            std::lock_guard<std::mutex> guard(m_sync_mutex);
            m_sync_functions.insert(m_sync_functions.begin(), std::make_move_iterator(local.begin() + i + 1), std::make_move_iterator(local.end()));
            break;
        }
    }

    std::vector<DelayOperation> delay_local;
    m_delay_mutex.lock();
    delay_local.swap(m_delay_tasks);
    m_delay_mutex.unlock();

    for (auto& d : delay_local)
    {
        // Check cancel
        m_delay_mutex.lock();
        if (m_delay_cancel_set.contains(d.index))
        {
            m_delay_cancel_set.erase(d.index);
            m_delay_mutex.unlock();
            continue;
        }
        m_delay_mutex.unlock();

#ifdef PS4
        uint64_t duration = sceKernelGetProcessTime() - d.startPoint;
#else
        auto duration = (getCPUTimeUsec() - d.startPoint) / 1000;
#endif

        if (duration >= d.delayMilliseconds)
        {
            // Cancelled since the check above, or run now
            m_delay_mutex.lock();
            bool cancelled = m_delay_cancel_set.erase(d.index) > 0;
            m_delay_pending.erase(d.index);
            m_delay_mutex.unlock();
            if (cancelled)
                continue;

            Application::requestRedraw();
            try
            {
                d.func();
            }
            catch (std::exception& e)
            {
                aurore::Logger::error("error: performSyncTasks(delay): {}", e.what());
            }
            catch (...)
            {
                aurore::Logger::error("error: performSyncTasks(delay): unknown exception");
            }
        }
        else
        {
            m_delay_mutex.lock();
            m_delay_tasks.push_back(std::move(d));
            m_delay_mutex.unlock();
        }
    }
}

void Threading::start()
{
    if (task_loop_running)
        return;
    task_loop_active = true;
    start_task_loop();
}

void Threading::stop()
{
    if (!task_loop_running)
        return;
    {
        std::lock_guard<std::mutex> guard(m_async_mutex);
        task_loop_active = false;
    }
    m_async_cv.notify_all();
    {
        // Under its lock: the decode thread sees the flag or is woken
        std::lock_guard<std::mutex> guard(decode_mutex);
    }
    decode_cv.notify_all();

#ifdef AURORE_USE_STD_THREAD
    task_loop_thread->join();
    delete task_loop_thread;
    task_loop_thread = nullptr;
    if (decode_thread)
    {
        decode_thread->join();
        delete decode_thread;
        decode_thread = nullptr;
    }
#elif defined(__3DS__)
    threadJoin(task_loop_thread, U64_MAX);
    threadFree(task_loop_thread);
    task_loop_thread = nullptr;
    if (decode_thread)
    {
        threadJoin(decode_thread, U64_MAX);
        threadFree(decode_thread);
        decode_thread = nullptr;
    }
#else
    pthread_join(task_loop_thread, NULL);
    if (decode_thread_running)
        pthread_join(decode_thread, NULL);
#endif
    task_loop_running     = false;
    decode_thread_running = false;
}
void Threading::std_task_loop()
{
    task_loop(nullptr);
}
void* Threading::task_loop(void* a)
{
    runQueue(m_async_mutex, m_async_cv, m_async_tasks, m_async_running, task_loop_active, "async task");
    return NULL;
}

void* Threading::decode_loop(void* a)
{
    runQueue(decode_mutex, decode_cv, decode_tasks, decode_running, task_loop_active, "decode task");
    return NULL;
}

void Threading::start_task_loop()
{
#ifdef AURORE_USE_STD_THREAD
    task_loop_thread      = new std::thread(std_task_loop);
    task_loop_running     = true;
    decode_thread         = new std::thread([]
        { decode_loop(nullptr); });
    decode_thread_running = true;
#elif defined(__3DS__)
    task_loop_thread      = startCtrThread([](void*)
        { task_loop(nullptr); });
    task_loop_running     = task_loop_thread != nullptr;
    decode_thread         = task_loop_running ? startCtrThread([](void*)
                                            { decode_loop(nullptr); })
                                              : nullptr;
    decode_thread_running = decode_thread != nullptr;
#else
    task_loop_running     = pthread_create(&task_loop_thread, NULL, task_loop, NULL) == 0;
    decode_thread_running = task_loop_running && pthread_create(&decode_thread, NULL, decode_loop, NULL) == 0;
#endif
}

} // namespace aurore
