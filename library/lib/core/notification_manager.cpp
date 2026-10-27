/*
Borealis, a Nintendo Switch UI Library
Copyright (C) 2019  natinusala
Copyright (C) 2024  xfangfang

This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <borealis/core/application.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/core/notification_manager.hpp>

namespace brls
{

NotificationManager::NotificationManager()
{
    float width = Application::getStyle().getMetric("brls/notification/width");
    this->setWidth(width);
    this->setTranslationX(Application::ORIGINAL_WINDOW_WIDTH - width);
    this->setAxis(Axis::COLUMN);
}

void NotificationManager::notify(const std::string& text, View* leading)
{
    // Create the notification
    brls::Logger::debug("Showing notification \"{}\"", text);

    auto* notification = new Notification(text, leading);
    this->addView(notification, 0);

    // Where the stack sits, from the style as it is now (an app may set it
    // after the manager is made)
    auto style  = Application::getStyle();
    float width = style.getMetric("brls/notification/width");
    this->setWidth(width);
    this->setTranslationX(Application::ORIGINAL_WINDOW_WIDTH - width - style.getMetric("brls/notification/margin_right"));
    this->setTranslationY(style.getMetric("brls/notification/margin_top"));

    // Timeout timer
    float timeout = style.getMetric("brls/animations/notification_timeout");
    float show    = style.getMetric("brls/animations/notification_show");
    float slide   = style.getMetric("brls/notification/slide");
    notification->timeoutTimer.reset(slide);
    notification->timeoutTimer.addStep(0.0f, (int)show, EasingFunction::quadraticOut);
    notification->timeoutTimer.addStep(0.0f, (int)timeout, EasingFunction::linear);
    notification->timeoutTimer.addStep(slide, (int)show, EasingFunction::quadraticOut);

    notification->timeoutTimer.setTickCallback([notification, slide]()
        {
            float position = notification->timeoutTimer.getValue();
            notification->setTranslationX(position);
            notification->setAlpha(1.0f - position / slide); });

    notification->timeoutTimer.setEndCallback([this, notification](bool finished)
        { this->removeView(notification); });

    notification->timeoutTimer.start();
}

NotificationManager::~NotificationManager()
{
    std::vector<View*> views = this->getChildren();
    for (auto& view : views)
    {
        auto notification = dynamic_cast<Notification*>(view);
        if (!notification)
            continue;
        // Stopped without its end callback, which would remove the view
        // from a manager being destroyed
        notification->timeoutTimer.setEndCallback([](bool) {});
        notification->timeoutTimer.stop();
    }
}

Notification::Notification(const std::string& text, View* leading)
{
    auto style = Application::getStyle();
    this->setThemeColor("backgroundColor", "brls/notification/background");
    this->setCornerRadius(style.getMetric("brls/notification/corner_radius"));
    this->setMarginBottom(style.getMetric("brls/notification/spacing"));
    float padding = style.getMetric("brls/notification/padding");
    this->setPadding(padding);
    float width = style.getMetric("brls/notification/width");
    this->setWidth(width);
    this->label = new Label();
    this->label->setText(text);
    this->label->setThemeColor("textColor", "brls/notification/text");
    if (leading)
    {
        // The text beside it, wrapping in what is left.
        this->setAlignItems(AlignItems::CENTER);
        this->addView(leading);
        this->label->setGrow(1.0f);
        this->label->setWidth(0);
    }
    this->addView(label);
}

Notification::~Notification() = default;

}; // namespace brls
