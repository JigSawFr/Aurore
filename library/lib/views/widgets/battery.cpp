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

#include "borealis/views/widgets/battery.hpp"

#include "borealis/core/thread.hpp"

#define BATTERY_MAX_WIDTH 23.0f

namespace brls
{

BatteryWidget::BatteryWidget()
{
    platform = Application::getPlatform();
    if (!platform->canShowBatteryLevel())
        return;

    setSize(Size(44, 44));

    back = new Image();
    back->setSize(Size(44, 44));
    back->setScalingType(ImageScalingType::FIT);
    back->detach();

    level = new Rectangle();
    level->setDetachedPosition(11, 18);
    level->setSize(Size(BATTERY_MAX_WIDTH, 10));
    level->detach();

    applyBackTheme(platform->getThemeVariant());

    addView(level);
    addView(back);
}

void BatteryWidget::updateState()
{
    static Time time = 0;
    Time now         = getCPUTimeUsec();
    if ((now - time) > 5000000)
    {
        brls::Logger::verbose("isBatteryCharging: {}; batteryLevel: {}", isBatteryCharging, batteryLevel);
#ifdef ANDROID
        isBatteryCharging = Application::getPlatform()->isBatteryCharging();
        batteryLevel      = Application::getPlatform()->getBatteryLevel() / 100.0f;
#else
        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN]()
            {
                ASYNC_RELEASE
                isBatteryCharging = Application::getPlatform()->isBatteryCharging();
                batteryLevel      = Application::getPlatform()->getBatteryLevel() / 100.0f; });
#endif
        time = now;
    }
}

void BatteryWidget::applyBackTheme(ThemeVariant theme)
{
    switch (theme)
    {
        case ThemeVariant::LIGHT:
            back->setImageFromRes("img/sys/battery_back_light.png");
            break;
        case ThemeVariant::DARK:
            back->setImageFromRes("img/sys/battery_back_dark.png");
            break;
    }
}

void BatteryWidget::onThemeChanged()
{
    if (back)
        applyBackTheme(platform->getThemeVariant());
    Box::onThemeChanged();
}

void BatteryWidget::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    if (!level)
        return;
    this->updateState();
    level->setColor(ctx->theme[isBatteryCharging ? "brls/battery/charging" : "brls/battery/level"]);

    level->setWidth(BATTERY_MAX_WIDTH * batteryLevel);
    Box::draw(vg, x, y, width, height, style, ctx);
}

View* BatteryWidget::create()
{
    return new BatteryWidget();
}

} // namespace brls
