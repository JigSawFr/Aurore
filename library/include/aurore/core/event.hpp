/*
    Copyright 2020 natinusala

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#pragma once

#include <algorithm>
#include <functional>
#include <list>
#include <mutex>

namespace aurore
{

// Simple observer pattern implementation
//
// Usage:
// 1. typedef your event type
// 2. create as many events as you want using that type
// 3. call subscribe on the events with your observers
// 4. call fire when you want to fire the events
//    it wil return true if at least one subscriber exists
//    for that event
//
// Subscribing, unsubscribing and firing may happen on different threads
// (the logger fires from any); the callbacks run on the thread that fires.
template <typename... Ts>
class Event
{
  public:
    typedef std::function<void(Ts...)> Callback;
    typedef std::list<Callback> CallbacksList;
    typedef typename CallbacksList::iterator Subscription;

    Event() = default;
    // The subscribers are copied, not the lock
    Event(const Event& other)
        : callbacks(other.copyCallbacks())
    {
    }
    Event& operator=(const Event& other)
    {
        if (this != &other)
        {
            CallbacksList copy = other.copyCallbacks();
            std::lock_guard<std::mutex> guard(this->mutex);
            this->callbacks.swap(copy);
        }
        return *this;
    }

    Subscription subscribe(Callback cb);
    void unsubscribe(Subscription subscription);
    void clear();
    bool fire(Ts... args);

  private:
    CallbacksList callbacks;
    mutable std::mutex mutex;

    CallbacksList copyCallbacks() const
    {
        std::lock_guard<std::mutex> guard(this->mutex);
        return this->callbacks;
    }
};

template <typename... Ts>
typename Event<Ts...>::Subscription Event<Ts...>::subscribe(Event<Ts...>::Callback cb)
{
    std::lock_guard<std::mutex> guard(this->mutex);
    this->callbacks.push_back(std::move(cb));
    return --this->callbacks.end();
}

template <typename... Ts>
void Event<Ts...>::unsubscribe(Event<Ts...>::Subscription subscription)
{
    std::lock_guard<std::mutex> guard(this->mutex);
    if (this->callbacks.size() > 0)
        this->callbacks.erase(subscription);
}

template <typename... Ts>
void Event<Ts...>::clear()
{
    std::lock_guard<std::mutex> guard(this->mutex);
    this->callbacks.clear();
}

template <typename... Ts>
bool Event<Ts...>::fire(Ts... args)
{
    // A callback may subscribe or unsubscribe (itself or another one) while
    // the event fires: walk a copy, never the list being changed, and call
    // it without the lock held.
    std::unique_lock<std::mutex> lock(this->mutex);
    if (this->callbacks.empty())
        return false;

    if (this->callbacks.size() == 1)
    {
        Callback cb = this->callbacks.front();
        lock.unlock();
        cb(args...);
        return true;
    }

    CallbacksList snapshot = this->callbacks;
    lock.unlock();
    for (Callback& cb : snapshot)
        cb(args...);

    return true;
}

}; // namespace aurore
