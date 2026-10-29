// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/application.hpp>
#include <aurore/core/bind.hpp>
#include <aurore/core/box.hpp>
#include <aurore/views/recycler.hpp>

namespace aurore
{

class CheckBox : public View
{
  public:
    CheckBox();
    virtual void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;

    static View* create();
};

class RadioCell : public RecyclerCell
{
  public:
    RadioCell();

    void setSelected(bool selected);
    bool getSelected();

    AURORE_BIND(Label, title, "aurore/rediocell/title");
    AURORE_BIND(CheckBox, checkbox, "aurore/rediocell/checkbox");

    static View* create();

    bool setBoundText(const std::string& text) override
    {
        this->title->setText(text);
        return true;
    }

  protected:
    AccessibilityRole getDefaultAccessibilityRole() override { return AccessibilityRole::CELL; }
    std::string getDefaultAccessibilityLabel() override { return this->title->getFullText(); }
    bool getDefaultAccessibilitySelected() override { return this->selected; }

  private:
    bool selected = false;
};

} // namespace aurore
