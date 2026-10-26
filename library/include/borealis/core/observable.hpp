/*
    Copyright 2026 JigSawFr

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

#include <cstddef>
#include <functional>
#include <type_traits>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace brls
{

// What subscribe() returns: the subscriber stays subscribed as long as the
// Subscription lives. Destroying it (or calling cancel()) unsubscribes; it
// can outlive the Observable (it then does nothing). A view keeps the
// subscriptions of its bindings (View::keepSubscription), so they end with it.
class Subscription
{
  public:
    Subscription() = default;
    explicit Subscription(std::function<void()> cancel)
        : cancelFunction(std::move(cancel))
    {
    }

    Subscription(Subscription&& other) noexcept
        : cancelFunction(std::move(other.cancelFunction))
    {
        other.cancelFunction = nullptr;
    }

    Subscription& operator=(Subscription&& other) noexcept
    {
        if (this != &other)
        {
            this->cancel();
            this->cancelFunction = std::move(other.cancelFunction);
            other.cancelFunction = nullptr;
        }
        return *this;
    }

    Subscription(const Subscription&)            = delete;
    Subscription& operator=(const Subscription&) = delete;

    ~Subscription()
    {
        this->cancel();
    }

    void cancel()
    {
        if (this->cancelFunction)
        {
            auto function        = std::move(this->cancelFunction);
            this->cancelFunction = nullptr;
            function();
        }
    }

    bool active() const
    {
        return this->cancelFunction != nullptr;
    }

  private:
    std::function<void()> cancelFunction;
};

// A value that tells its subscribers when it changes:
//
//     brls::Observable<std::string> name { "Ada" };
//     label->bindText(name);   // the label shows "Ada"
//     name = "Grace";          // ... then "Grace"
//
// set() notifies only when the value changes (operator== when T has one).
// Subscribers are called on the thread that calls set(): set it from the UI
// thread (brls::sync from a worker).
template <typename T>
class Observable
{
  public:
    using Subscriber = std::function<void(const T&)>;

    Observable()
        : state(std::make_shared<State>())
    {
    }

    Observable(T value)
        : state(std::make_shared<State>())
    {
        this->state->value = std::move(value);
    }

    // An Observable is a place: copying it would leave the subscribers on
    // the original.
    Observable(const Observable&)            = delete;
    Observable& operator=(const Observable&) = delete;

    const T& get() const
    {
        return this->state->value;
    }

    operator const T&() const
    {
        return this->state->value;
    }

    void set(T value)
    {
        if constexpr (std::is_invocable_r_v<bool, std::equal_to<>, const T&, const T&>)
        {
            if (this->state->value == value)
                return;
        }
        this->state->value = std::move(value);
        this->notify();
    }

    Observable& operator=(T value)
    {
        this->set(std::move(value));
        return *this;
    }

    // Calls every subscriber with the value, changed or not (after an
    // in-place change of a value set() cannot compare).
    void notify()
    {
        // A copy: a subscriber may subscribe or unsubscribe while called.
        std::shared_ptr<State> keep = this->state;
        std::vector<std::pair<size_t, Subscriber>> subscribers = keep->subscribers;
        for (auto& [id, subscriber] : subscribers)
        {
            if (keep->contains(id))
                subscriber(keep->value);
        }
    }

    // `subscriber` called with the new value at each change, until the
    // returned Subscription is destroyed.
    [[nodiscard]] Subscription subscribe(Subscriber subscriber)
    {
        size_t id = ++this->state->lastId;
        this->state->subscribers.emplace_back(id, std::move(subscriber));
        std::weak_ptr<State> weak = this->state;
        return Subscription([weak, id] {
            if (auto state = weak.lock())
                state->remove(id);
        });
    }

    size_t getSubscriberCount() const
    {
        return this->state->subscribers.size();
    }

  private:
    struct State
    {
        T value {};
        size_t lastId = 0;
        std::vector<std::pair<size_t, Subscriber>> subscribers;

        bool contains(size_t id) const
        {
            for (auto& entry : this->subscribers)
                if (entry.first == id)
                    return true;
            return false;
        }

        void remove(size_t id)
        {
            for (auto it = this->subscribers.begin(); it != this->subscribers.end(); ++it)
            {
                if (it->first == id)
                {
                    this->subscribers.erase(it);
                    return;
                }
            }
        }
    };

    std::shared_ptr<State> state;
};

// Observables named for XML bindings (bind="text: profile.name"). A model is
// registered under a name on the Application (every screen) or on an
// Activity (its own screen, looked up first); its observables must outlive
// it, or be removed first.
//
//     brls::Observable<std::string> name { "Ada" };
//     brls::Observable<bool> online { true };
//     brls::DataModel profile;
//     profile.add("name", name);
//     profile.add("online", online);
//     brls::Application::registerModel("profile", &profile);
class DataModel
{
  public:
    template <typename T>
    void add(const std::string& key, Observable<T>& observable)
    {
        this->entries.insert_or_assign(key, Entry { std::type_index(typeid(T)), &observable });
    }

    void remove(const std::string& key)
    {
        this->entries.erase(key);
    }

    // The observable under `key` when it holds a T, else nullptr.
    template <typename T>
    Observable<T>* get(const std::string& key) const
    {
        auto it = this->entries.find(key);
        if (it == this->entries.end() || it->second.type != std::type_index(typeid(T)))
            return nullptr;
        return static_cast<Observable<T>*>(it->second.observable);
    }

    bool has(const std::string& key) const
    {
        return this->entries.count(key) > 0;
    }

  private:
    struct Entry
    {
        std::type_index type;
        void* observable;
    };

    std::unordered_map<std::string, Entry> entries;
};

} // namespace brls
