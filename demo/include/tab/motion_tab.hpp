// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include <aurore.hpp>
#include <aurore/cadran/view/scroll_view.hpp>

class EasingPlot;

/// Movement: easings drawn and played, state styles with their
/// transitions (XML only), the focus highlight gliding and shaking, reduced
/// motion, and the transform lab.
class MotionTab : public ScrollView
{
  public:
    MotionTab();

    static aurore::View* create();

  private:
    std::vector<EasingPlot*> plots;
    aurore::Animatable progress;

    void play();
};
