// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#include "aurore/views/widgets/battery.hpp"

#include "aurore/core/thread.hpp"

#define BATTERY_MAX_WIDTH 23.0f

namespace aurore
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
        aurore::Logger::verbose("isBatteryCharging: {}; batteryLevel: {}", isBatteryCharging, batteryLevel);
#ifdef ANDROID
        isBatteryCharging = Application::getPlatform()->isBatteryCharging();
        batteryLevel      = Application::getPlatform()->getBatteryLevel() / 100.0f;
#else
        // Read on the task thread, applied on the UI thread, which draws
        // them; one read at a time
        if (updating)
            return;
        updating = true;
        aurore::async([]()
            {
                bool charging = Application::getPlatform()->isBatteryCharging();
                float level   = Application::getPlatform()->getBatteryLevel() / 100.0f;
                aurore::sync([charging, level]()
                    {
                        isBatteryCharging = charging;
                        batteryLevel      = level;
                        updating          = false; }); });
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
    level->setColor(ctx->theme[isBatteryCharging ? "aurore/battery/charging" : "aurore/battery/level"]);

    level->setWidth(BATTERY_MAX_WIDTH * batteryLevel);
    Box::draw(vg, x, y, width, height, style, ctx);
}

View* BatteryWidget::create()
{
    return new BatteryWidget();
}

} // namespace aurore
