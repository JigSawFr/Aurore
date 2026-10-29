// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/application.hpp>
#include <aurore/core/bind.hpp>
#include <aurore/core/box.hpp>
#include <aurore/views/cells/cell_detail.hpp>
#include <aurore/views/recycler.hpp>

namespace aurore
{

class BooleanCell : public DetailCell
{
  public:
    BooleanCell();

    void init(std::string title, bool isOn, std::function<void(bool)> callback);

    void setOn(bool on, bool animated = true);
    bool isOn()
    {
        return state;
    }

    Event<bool>* getEvent()
    {
        return &event;
    }

    static View* create();

  private:
    bool state = false;
    float baseDetailTextSize;

    Animatable scale = 1;
    Event<bool> event;

    void updateUI();
    void scaleTick();

  protected:
    AccessibilityRole getDefaultAccessibilityRole() override { return AccessibilityRole::TOGGLE; }
    // "on" or "off", whether its detail is shown or not.
    std::string getDefaultAccessibilityValue() override;
};

} // namespace aurore
