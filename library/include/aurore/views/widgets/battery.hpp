// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/application.hpp>
#include <aurore/core/box.hpp>
#include <aurore/views/image.hpp>
#include <aurore/views/rectangle.hpp>

namespace aurore
{

class BatteryWidget : public Box
{
  public:
    BatteryWidget();

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    void onThemeChanged() override;
    static View* create();

  private:
    // None where the platform shows no battery level
    Image* back      = nullptr;
    Rectangle* level = nullptr;
    Platform* platform;

    void applyBackTheme(ThemeVariant theme);

    void updateState();
    inline static bool isBatteryCharging = false;
    inline static float batteryLevel     = 1;
    inline static bool updating          = false; // a read is under way
};

} // namespace aurore
