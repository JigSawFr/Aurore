// SPDX-FileCopyrightText: 2019 natinusala
// SPDX-FileCopyrightText: 2024 xfangfang
// SPDX-License-Identifier: GPL-3.0-or-later

#include <aurore/core/application.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/core/notification_manager.hpp>

namespace aurore
{

NotificationManager::NotificationManager()
{
    float width = Application::getStyle().getMetric("aurore/notification/width");
    this->setWidth(width);
    this->setTranslationX(Application::ORIGINAL_WINDOW_WIDTH - width);
    this->setAxis(Axis::COLUMN);
}

void NotificationManager::notify(const std::string& text, View* leading)
{
    // Create the notification
    aurore::Logger::debug("Showing notification \"{}\"", text);

    auto* notification = new Notification(text, leading);
    this->addView(notification, 0);

    // Where the stack sits, from the style as it is now (an app may set it
    // after the manager is made),
    auto style  = Application::getStyle();
    float width = style.getMetric("aurore/notification/width");
    this->setWidth(width);
    // and clear of the safe area
    const Insets safe = Application::getSafeArea();
    this->setTranslationX(Application::ORIGINAL_WINDOW_WIDTH - width - style.getMetric("aurore/notification/margin_right") - safe.right);
    this->setTranslationY(style.getMetric("aurore/notification/margin_top") + safe.top);

    // Timeout timer
    float timeout = style.getMetric("aurore/animations/notification_timeout");
    float show    = style.getMetric("aurore/animations/notification_show");
    float slide   = style.getMetric("aurore/notification/slide");
    notification->timeoutTimer.reset(slide);
    notification->timeoutTimer.addStep(0.0f, (int)show, EasingFunction::quadraticOut);
    notification->timeoutTimer.addStep(0.0f, (int)timeout, EasingFunction::linear);
    notification->timeoutTimer.addStep(slide, (int)show, EasingFunction::quadraticOut);

    // Reduced motion: the toast fades in and out where it stays
    const bool still = Application::getReduceMotion();
    notification->timeoutTimer.setTickCallback([notification, slide, still]()
        {
            float position = notification->timeoutTimer.getValue();
            notification->setTranslationX(still ? 0.0f : position);
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
    this->setThemeColor("backgroundColor", "aurore/notification/background");
    this->setCornerRadius(style.getMetric("aurore/notification/corner_radius"));
    this->setMarginBottom(style.getMetric("aurore/notification/spacing"));
    float padding = style.getMetric("aurore/notification/padding");
    this->setPadding(padding);
    float width = style.getMetric("aurore/notification/width");
    this->setWidth(width);
    this->label = new Label();
    this->label->setText(text);
    this->label->setThemeColor("textColor", "aurore/notification/text");
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

}; // namespace aurore
