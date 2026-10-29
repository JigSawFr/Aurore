// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// Tasks that come back to a view from another thread, the task queues and
// events fired from several threads. Also built with -fsanitize=thread or
// address (docs/testing.md).

#include <yoga/Yoga.h>
#include <aurore.hpp>
#include <aurore/test/harness.hpp>
#include <aurore/test/headless_platform.hpp>
#include <aurore/views/widgets/battery.hpp>
#include <aurore/views/widgets/wireless.hpp>

#include <atomic>
#include <cmath>
#include <fstream>
#include <future>
#include <iterator>
#include <string>
#include <thread>

using aurore::test::Harness;

namespace
{

std::string readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

// The width a view is given (its layout may wait for the next frame)
float styleWidth(aurore::View* view)
{
    return YGNodeStyleGetWidth(view->getYGNode()).value;
}

// The battery widget's level bar
aurore::View* batteryBar(aurore::View* battery)
{
    for (aurore::View* child : static_cast<aurore::Box*>(battery)->getChildren())
        if (dynamic_cast<aurore::Rectangle*>(child))
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
        aurore::async([opened = opened]
            { opened.wait(); });
    }
    void release() { open.set_value(); }
};

} // namespace

AURORE_TEST(alive_token_expires_with_the_view)
{
    Harness harness;
    aurore::View* view        = new aurore::Box();
    std::weak_ptr<void> token = view->aliveToken();
    AURORE_CHECK(!token.expired());
    AURORE_CHECK(!view->aliveToken().owner_before(token) && !token.owner_before(view->aliveToken()));
    delete view;
    AURORE_CHECK(token.expired());
}

AURORE_TEST(async_image_callback_twice_or_after_the_image_is_gone)
{
    Harness harness;
    std::string png = readFile(std::string(AURORE_SOURCE_RESOURCES) + "img/sys/battery_back_dark.png");
    AURORE_CHECK(png.size() > 100);

    aurore::Image* image = new aurore::Image();
    std::function<void(const std::string&, size_t)> done;
    image->setImageAsync([&done](std::function<void(const std::string&, size_t)> cb)
        { done = cb; });
    AURORE_CHECK(done != nullptr);

    // Called twice: the second sets the picture again
    done(png, png.size());
    harness.settle();
    AURORE_CHECK(image->getTexture() != 0);
    done(png, png.size());
    harness.settle();
    AURORE_CHECK(image->getTexture() != 0);

    // A length past the data is cut to it
    done(png, png.size() + 4096);
    harness.settle();
    AURORE_CHECK(image->getTexture() != 0);

    // After the image is deleted: nothing (the old counter was freed by the
    // first call, and read again by the next ones)
    delete image;
    done(png, png.size());
    done(std::string(), 0);
    harness.settle();
}

AURORE_TEST(cancel_delay_ignores_an_index_not_given_yet)
{
    Harness harness;
    size_t first = aurore::delay(0, [] {});
    harness.step(2);

    // Cancelling the next index before it is given does not cancel it
    aurore::cancelDelay(first + 1);
    bool ran      = false;
    size_t second = aurore::delay(0, [&ran]
        { ran = true; });
    AURORE_CHECK_EQ(second, first + 1);
    harness.step(2);
    AURORE_CHECK(ran);

    // A pending one is cancelled; cancelling it again, or one run, is harmless
    bool cancelledRan = false;
    size_t third      = aurore::delay(100, [&cancelledRan]
             { cancelledRan = true; });
    aurore::cancelDelay(third);
    aurore::cancelDelay(third);
    aurore::cancelDelay(second);
    harness.advance(300);
    AURORE_CHECK(!cancelledRan);
}

AURORE_TEST(threading_stop_twice_then_start)
{
    Harness harness;
    aurore::Threading::stop();
    aurore::Threading::stop();
    aurore::Threading::start();
    aurore::Threading::start();

    std::atomic<bool> ran { false };
    aurore::async([&ran]
        { ran = true; });
    harness.settle();
    AURORE_CHECK(ran.load());
}

// A picture being decoded does not hold back the other tasks
AURORE_TEST(async_tasks_run_while_a_picture_decodes)
{
    Harness harness;
    std::promise<void> release;
    std::shared_future<void> released = release.get_future().share();
    std::atomic<bool> decoding { false }, decoded { false }, ran { false };
    aurore::Threading::asyncDecode([&, released]
        {
            decoding = true;
            released.wait();
            decoded = true; });
    while (!decoding)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    AURORE_CHECK(!aurore::Threading::isIdle());

    aurore::async([&ran]
        { ran = true; });
    for (int i = 0; i < 2000 && !ran; i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    AURORE_CHECK(ran.load());
    AURORE_CHECK(!decoded.load());

    release.set_value();
    harness.settle();
    AURORE_CHECK(decoded.load());
    AURORE_CHECK(aurore::Threading::isIdle());

    // Stopped and started again, as async()'s
    aurore::Threading::stop();
    aurore::Threading::start();
    std::atomic<bool> again { false };
    aurore::Threading::asyncDecode([&again]
        { again = true; });
    harness.settle();
    AURORE_CHECK(again.load());
}

AURORE_TEST(event_subscribed_while_another_thread_fires)
{
    Harness harness;
    aurore::Event<int> event;
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
    AURORE_CHECK(received.load() > 0);

    // Copied: the subscribers, not the lock
    aurore::Event<int> copy = event;
    received                = 0;
    AURORE_CHECK(copy.fire(2));
    AURORE_CHECK_EQ(received.load(), 2);
}

AURORE_TEST(logging_from_the_task_thread)
{
    Harness harness;
    aurore::Logger::setLogLevel(aurore::LogLevel::LOG_VERBOSE);
    std::atomic<int> lines { 0 };
    auto subscription = aurore::Logger::getLogEvent()->subscribe([&lines](aurore::Logger::TimePoint, aurore::LogLevel, std::string)
        { lines++; });
    for (int i = 0; i < 50; i++)
        aurore::async([i]
            { aurore::Logger::verbose("task {}", i); });
    for (int i = 0; i < 50; i++)
        aurore::Logger::verbose("ui {}", i);
    harness.settle();
    aurore::Logger::getLogEvent()->unsubscribe(subscription);
    AURORE_CHECK_EQ(lines.load(), 100);
}

AURORE_TEST(status_widgets_read_on_the_task_thread)
{
    Harness harness;
    harness.platform()->showStatus = true;
    // The widgets read the platform again when drawn, 5 s after the last time
    auto later = [&harness]
    {
        harness.advance(6000);
        aurore::Application::requestRedraw();
        harness.step();
    };
    auto drawn = [&harness]
    {
        harness.settle();
        aurore::Application::requestRedraw();
        harness.step(2);
    };

    aurore::Box* box               = new aurore::Box(aurore::Axis::ROW);
    aurore::BatteryWidget* battery = new aurore::BatteryWidget();
    box->addView(battery);
    box->addView(new aurore::WirelessWidget());
    harness.push(new aurore::Activity(box));
    later();
    drawn();

    // The level drawn follows the platform's, read on the task thread
    aurore::View* level = batteryBar(battery);
    AURORE_CHECK(level != nullptr);
    if (!level)
        return;
    float full = styleWidth(level);
    AURORE_CHECK(full > 0);
    harness.platform()->batteryLevel = 50;
    later();
    drawn();
    AURORE_CHECK(std::fabs(styleWidth(level) - full / 2) < 0.5f);

    // Deleted while a read is under way: it lands after, harmlessly
    {
        TaskThreadGate gate;
        harness.platform()->batteryLevel = 20;
        later();
        aurore::Application::popActivity(aurore::TransitionAnimation::NONE);
        harness.step(5);
        gate.release();
    }
    harness.settle();

    // Read by the widgets made since
    aurore::BatteryWidget* next = new aurore::BatteryWidget();
    harness.push(new aurore::Activity(next));
    drawn();
    AURORE_CHECK(std::fabs(styleWidth(batteryBar(next)) - full / 5) < 0.5f);
}

int main(int argc, char** argv)
{
    return aurore::test::runTests(argc, argv);
}
