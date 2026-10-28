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

// Tasks that come back to a view from another thread, the task queues and
// events fired from several threads. Also built with -fsanitize=thread or
// address (docs/testing.md).

#include <yoga/Yoga.h>
#include <borealis.hpp>
#include <borealis/test/harness.hpp>
#include <borealis/test/headless_platform.hpp>
#include <borealis/views/widgets/battery.hpp>
#include <borealis/views/widgets/wireless.hpp>

#include <atomic>
#include <cmath>
#include <fstream>
#include <future>
#include <iterator>
#include <string>
#include <thread>

using brls::test::Harness;

namespace
{

std::string readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

// Uses the older macros, which now wrap aliveToken()
class LegacyAsyncView : public brls::View
{
  public:
    explicit LegacyAsyncView(bool* ran)
        : ran(ran)
    {
    }

    void queue()
    {
        ASYNC_RETAIN
        brls::sync([ASYNC_TOKEN]()
            {
                ASYNC_RELEASE
                *this->ran = true; });
    }

    void draw(NVGcontext*, float, float, float, float, brls::Style, brls::FrameContext*) override { }

  private:
    bool* ran;
};

// The width a view is given (its layout may wait for the next frame)
float styleWidth(brls::View* view)
{
    return YGNodeStyleGetWidth(view->getYGNode()).value;
}

// The battery widget's level bar
brls::View* batteryBar(brls::View* battery)
{
    for (brls::View* child : static_cast<brls::Box*>(battery)->getChildren())
        if (dynamic_cast<brls::Rectangle*>(child))
            return child;
    return nullptr;
}

// Holds the task thread until released
struct TaskThreadGate
{
    std::promise<void> open;
    std::shared_future<void> opened = open.get_future().share();

    TaskThreadGate()
    {
        brls::async([opened = opened]
            { opened.wait(); });
    }
    void release() { open.set_value(); }
};

} // namespace

BRLS_TEST(alive_token_expires_with_the_view)
{
    Harness harness;
    brls::View* view          = new brls::Box();
    std::weak_ptr<void> token = view->aliveToken();
    BRLS_CHECK(!token.expired());
    BRLS_CHECK(!view->aliveToken().owner_before(token) && !token.owner_before(view->aliveToken()));
    delete view;
    BRLS_CHECK(token.expired());
}

BRLS_TEST(async_image_callback_twice_or_after_the_image_is_gone)
{
    Harness harness;
    std::string png = readFile(std::string(AURORE_SOURCE_RESOURCES) + "img/sys/battery_back_dark.png");
    BRLS_CHECK(png.size() > 100);

    brls::Image* image = new brls::Image();
    std::function<void(const std::string&, size_t)> done;
    image->setImageAsync([&done](std::function<void(const std::string&, size_t)> cb)
        { done = cb; });
    BRLS_CHECK(done != nullptr);

    // Called twice: the second sets the picture again
    done(png, png.size());
    harness.settle();
    BRLS_CHECK(image->getTexture() != 0);
    done(png, png.size());
    harness.settle();
    BRLS_CHECK(image->getTexture() != 0);

    // A length past the data is cut to it
    done(png, png.size() + 4096);
    harness.settle();
    BRLS_CHECK(image->getTexture() != 0);

    // After the image is deleted: nothing (the old counter was freed by the
    // first call, and read again by the next ones)
    delete image;
    done(png, png.size());
    done(std::string(), 0);
    harness.settle();
}

BRLS_TEST(legacy_async_macros_skip_a_deleted_view)
{
    Harness harness;
    bool keptRan = false, deletedRan = false;
    LegacyAsyncView* kept    = new LegacyAsyncView(&keptRan);
    LegacyAsyncView* deleted = new LegacyAsyncView(&deletedRan);
    kept->queue();
    kept->queue();
    deleted->queue();
    delete deleted;
    harness.settle();
    BRLS_CHECK(keptRan);
    BRLS_CHECK(!deletedRan);
    delete kept;
}

BRLS_TEST(cancel_delay_ignores_an_index_not_given_yet)
{
    Harness harness;
    size_t first = brls::delay(0, [] {});
    harness.step(2);

    // Cancelling the next index before it is given does not cancel it
    brls::cancelDelay(first + 1);
    bool ran      = false;
    size_t second = brls::delay(0, [&ran]
        { ran = true; });
    BRLS_CHECK_EQ(second, first + 1);
    harness.step(2);
    BRLS_CHECK(ran);

    // A pending one is cancelled; cancelling it again, or one run, is harmless
    bool cancelledRan = false;
    size_t third      = brls::delay(100, [&cancelledRan]
             { cancelledRan = true; });
    brls::cancelDelay(third);
    brls::cancelDelay(third);
    brls::cancelDelay(second);
    harness.advance(300);
    BRLS_CHECK(!cancelledRan);
}

BRLS_TEST(threading_stop_twice_then_start)
{
    Harness harness;
    brls::Threading::stop();
    brls::Threading::stop();
    brls::Threading::start();
    brls::Threading::start();

    std::atomic<bool> ran { false };
    brls::async([&ran]
        { ran = true; });
    harness.settle();
    BRLS_CHECK(ran.load());
}

BRLS_TEST(event_subscribed_while_another_thread_fires)
{
    Harness harness;
    brls::Event<int> event;
    std::atomic<int> received { 0 };
    event.subscribe([&received](int value)
        { received += value; });

    std::atomic<bool> stop { false };
    std::thread firing([&]
        {
            while (!stop)
                event.fire(1); });
    for (int i = 0; i < 20000 || received.load() < 1000; i++)
    {
        auto subscription = event.subscribe([](int) {});
        event.unsubscribe(subscription);
    }
    stop = true;
    firing.join();
    BRLS_CHECK(received.load() > 0);

    // Copied: the subscribers, not the lock
    brls::Event<int> copy = event;
    received              = 0;
    BRLS_CHECK(copy.fire(2));
    BRLS_CHECK_EQ(received.load(), 2);
}

BRLS_TEST(logging_from_the_task_thread)
{
    Harness harness;
    brls::Logger::setLogLevel(brls::LogLevel::LOG_VERBOSE);
    std::atomic<int> lines { 0 };
    auto subscription = brls::Logger::getLogEvent()->subscribe([&lines](brls::Logger::TimePoint, brls::LogLevel, std::string)
        { lines++; });
    for (int i = 0; i < 50; i++)
        brls::async([i]
            { brls::Logger::verbose("task {}", i); });
    for (int i = 0; i < 50; i++)
        brls::Logger::verbose("ui {}", i);
    harness.settle();
    brls::Logger::getLogEvent()->unsubscribe(subscription);
    BRLS_CHECK_EQ(lines.load(), 100);
}

BRLS_TEST(status_widgets_read_on_the_task_thread)
{
    Harness harness;
    harness.platform()->showStatus = true;
    // The widgets read the platform again when drawn, 5 s after the last time
    auto later = [&harness]
    {
        harness.advance(6000);
        brls::Application::requestRedraw();
        harness.step();
    };
    auto drawn = [&harness]
    {
        harness.settle();
        brls::Application::requestRedraw();
        harness.step(2);
    };

    brls::Box* box               = new brls::Box(brls::Axis::ROW);
    brls::BatteryWidget* battery = new brls::BatteryWidget();
    box->addView(battery);
    box->addView(new brls::WirelessWidget());
    harness.push(new brls::Activity(box));
    later();
    drawn();

    // The level drawn follows the platform's, read on the task thread
    brls::View* level = batteryBar(battery);
    BRLS_CHECK(level != nullptr);
    if (!level)
        return;
    float full = styleWidth(level);
    BRLS_CHECK(full > 0);
    harness.platform()->batteryLevel = 50;
    later();
    drawn();
    BRLS_CHECK(std::fabs(styleWidth(level) - full / 2) < 0.5f);

    // Deleted while a read is under way: it lands after, harmlessly
    {
        TaskThreadGate gate;
        harness.platform()->batteryLevel = 20;
        later();
        brls::Application::popActivity(brls::TransitionAnimation::NONE);
        harness.step(5);
        gate.release();
    }
    harness.settle();

    // Read by the widgets made since
    brls::BatteryWidget* next = new brls::BatteryWidget();
    harness.push(new brls::Activity(next));
    drawn();
    BRLS_CHECK(std::fabs(styleWidth(batteryBar(next)) - full / 5) < 0.5f);
}

int main(int argc, char** argv)
{
    return brls::test::runTests(argc, argv);
}
