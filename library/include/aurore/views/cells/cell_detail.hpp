// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/application.hpp>
#include <aurore/core/bind.hpp>
#include <aurore/core/box.hpp>
#include <aurore/views/recycler.hpp>

namespace aurore
{

class DetailCell : public RecyclerCell
{
  public:
    DetailCell();

    void setText(std::string title);
    void setTextColor(NVGcolor color);
    void setDetailText(std::string title);
    void setDetailTextColor(NVGcolor color);

    AURORE_BIND(Label, title, "aurore/rediocell/title");
    AURORE_BIND(Label, detail, "aurore/rediocell/detail");

    static View* create();

    bool setBoundText(const std::string& text) override;

  protected:
    AccessibilityRole getDefaultAccessibilityRole() override { return AccessibilityRole::CELL; }
    // Its title.
    std::string getDefaultAccessibilityLabel() override;
    // Its detail, and any other text shown in it (a state, a note).
    std::string getDefaultAccessibilityValue() override;
};

} // namespace aurore
