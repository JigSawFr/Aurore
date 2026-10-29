// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore.hpp>

/// A night sky with an aurora, drawn with nanovg behind the box's children:
/// a rounded panel, three curtains of light that sway, stars that twinkle,
/// two ridges of mountains. The same night in both variants of a theme, as
/// a picture would be: its text is drawn in the snow white over it
/// (AuroraView::SNOW, MIST).
///
/// It moves only while the app wants movement: still with reduced motion
/// (Application::getReduceMotion) and with redraw on demand, where it would
/// keep the app drawing every frame for an ornament.
class AuroraView : public aurore::Box
{
  public:
    AuroraView();

    void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
        aurore::FrameContext* ctx) override;

    /// The panel's corner radius (Cadran's top card's by default).
    void setRadius(float radius) { this->radius = radius; }
    /// The mountains at the bottom.
    void setRidges(bool ridges) { this->ridges = ridges; }
    /// A veil darkening the left of the sky, for a text over it.
    void setVeil(bool veil) { this->veil = veil; }

    static const NVGcolor SNOW;
    static const NVGcolor MIST;

    static aurore::View* create();

  private:
    float radius = 22;
    bool ridges  = true;
    bool veil    = true;

    struct Star
    {
        float x, y, size;
        int group;
    };
    std::vector<Star> stars;

    float seconds() const;
};
