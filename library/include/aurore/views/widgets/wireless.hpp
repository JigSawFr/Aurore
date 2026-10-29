// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/application.hpp>
#include <aurore/core/box.hpp>
#include <aurore/views/image.hpp>
#include <aurore/views/rectangle.hpp>

namespace aurore
{

class WirelessWidget : public Box
{
  public:
    WirelessWidget();

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    static View* create();

  private:
    Image* _0;
    Image* _1;
    Image* _2;
    Image* _3;
    Image* ethernet;
    Platform* platform;

    void applyTheme(ThemeVariant theme);

    void updateState();
    inline static bool hasWirelessConnection = false;
    inline static int wifiLevel              = 3;
    inline static bool hasEthernetConnection = false;
    inline static bool updating              = false; // a read is under way
};

} // namespace aurore
