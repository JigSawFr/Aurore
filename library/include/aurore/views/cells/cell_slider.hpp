// SPDX-FileCopyrightText: 2023 xfangfang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/application.hpp>
#include <aurore/core/bind.hpp>
#include <aurore/core/box.hpp>
#include <aurore/views/cells/cell_detail.hpp>
#include <aurore/views/recycler.hpp>
#include <aurore/views/slider.hpp>

namespace aurore
{

class SliderCell : public DetailCell
{
  public:
    SliderCell();

    void init(const std::string& title, float init, const std::function<void(float)>& callback);

    Event<float>* getEvent();

    static View* create();

    View* getDefaultFocus() override;

    Slider* slider;

  protected:
    AccessibilityRole getDefaultAccessibilityRole() override { return AccessibilityRole::SLIDER; }
};

} // namespace aurore
