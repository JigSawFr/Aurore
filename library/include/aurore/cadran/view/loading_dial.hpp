// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

// LoadingDial — Cadran's dial on the right of the loading screen, behind the
// icon and the bar, as the PIN gate draws it: its teal glow, two rings, the
// minutes, the day's arc. Its colours are theme keys (cadran/dial/line,
// cadran/dial/track and cadran/brand/fill), so a theme file can restyle it:
// its lines white over the navy, navy on the light theme's white. Laid out for
// a 1280 x 720 screen. An app registers `create` for its XML
// (`registerXMLView("LoadingDial", LoadingDial::create)`).
#pragma once

#include <aurore.hpp>

class LoadingDial : public aurore::View
{
  public:
    LoadingDial();

    void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
        aurore::FrameContext* ctx) override;

    static aurore::View* create();
};
