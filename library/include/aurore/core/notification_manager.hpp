// SPDX-FileCopyrightText: 2019 natinusala
// SPDX-FileCopyrightText: 2024 xfangfang
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <aurore/core/animation.hpp>
#include <aurore/core/box.hpp>
#include <aurore/views/label.hpp>

// TODO: check in HOS that the animation duration + notification timeout are correct
namespace aurore
{

class Notification : public Box
{
  public:
    // `leading` (an icon, owned by the notification) before the text.
    explicit Notification(const std::string& text, View* leading = nullptr);
    ~Notification() override;

    Animatable timeoutTimer;

  private:
    Label* label;
};

class NotificationManager : public Box
{
  public:
    NotificationManager();
    ~NotificationManager() override;

    void notify(const std::string& text, View* leading = nullptr);
};

}; // namespace aurore
