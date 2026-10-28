/*
    Copyright 2021 XITRIX

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

#include <yoga/Yoga.h>

#include <aurore/core/application.hpp>
#include <aurore/core/thread.hpp>
#include <aurore/views/debug_layer.hpp>
#include <aurore/views/label.hpp>

#ifdef PS4
#include <aurore/platforms/ps4/ps4_sysmodule.hpp>
#endif

namespace aurore
{

DebugLayer::DebugLayer()
    : Box(Axis::COLUMN)
{
    setWidth(aurore::Application::contentWidth);
    setHeight(aurore::Application::contentHeight);

    setJustifyContent(JustifyContent::FLEX_START);
    setAlignItems(AlignItems::FLEX_END);

    Box* contentView = new Box(Axis::COLUMN);
    this->addView(contentView);
    contentView->setWidth(aurore::Application::contentWidth / 2);
    contentView->setPadding(5);
    contentView->setBackgroundColor(RGBA(0, 0, 0, 160));
    YGNodeStyleSetFlexDirection(contentView->getYGNode(), YGFlexDirectionColumnReverse);
    // Lines logged from any thread, shown on the UI thread while the layer
    // is there
    logSubscription = Logger::getLogEvent()->subscribe([this, alive = aliveToken(), contentView](Logger::TimePoint now, LogLevel level, const std::string& log)
        { aurore::sync([this, alive, now, level, contentView, log]
              {
            if (alive.expired())
                return;
            uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count() % 1000;
#ifdef PS4
            OrbisDateTime lt{};
            if (sceRtcGetCurrentClockLocalTime)
                sceRtcGetCurrentClockLocalTime(&lt);
            std::string timeBase = fmt::format("{:02d}:{:02d}:{:02d}.{:03d}", lt.hour, lt.minute, lt.second, (int)ms);
#else
            std::time_t tt = std::chrono::system_clock::to_time_t(now);
            std::tm time_tm = localTime(tt);
            std::string timeBase = fmt::format("{:%H:%M:%S}.{:03d}", time_tm, (int)ms);
#endif

            auto box = new Box(Axis::ROW);
            box->setMarginTop(1);
            box->setMarginBottom(1);
            box->setPaddingTop(1);
            box->setHeight(9);
            auto timeLabel = new Label();
            timeLabel->setFontSize(8);
            timeLabel->setText(timeBase);
            timeLabel->setMinWidth(60);
            timeLabel->setTextColor(RGBA(200, 200, 200, 200));
            auto levelLabel = new Label();
            levelLabel->setFontSize(8);
            levelLabel->setMinWidth(40);
            auto label = new Label();
            label->setText(log);
            label->setFontSize(8);
            label->setTextColor(RGBA(200, 200, 200, 200));
            switch (level)
            {
                case LogLevel::LOG_INFO:
                    levelLabel->setTextColor(RGBA(94, 145, 208, 255));
                    levelLabel->setText("[INFO] ");
                    break;
                case LogLevel::LOG_WARNING:
                    levelLabel->setTextColor(RGBA(158, 139, 40, 255));
                    levelLabel->setText("[WARN] ");
                    break;
                case LogLevel::LOG_ERROR:
                    levelLabel->setTextColor(RGBA(165, 77, 69, 255));
                    levelLabel->setText("[ERROR] ");
                    break;
                case LogLevel::LOG_DEBUG:
                default:
                    levelLabel->setTextColor(RGBA(99, 138, 55, 255));
                    levelLabel->setText("[DEBUG] ");
            }
            box->addView(timeLabel);
            box->addView(levelLabel);
            box->addView(label);
            contentView->addView(box, 0);

            if (contentView->getChildren().size() > aurore::Application::contentHeight / 10 - 1)
            {
                // Not freeView(): it logs (Logger::verbose), which would add a line again, and so on
                View* old = contentView->getChildren()[contentView->getChildren().size() - 1];
                contentView->removeView(old, false);
                delete old;
            } }); });
}

DebugLayer::~DebugLayer()
{
    Logger::getLogEvent()->unsubscribe(logSubscription);
}

} // namespace aurore
