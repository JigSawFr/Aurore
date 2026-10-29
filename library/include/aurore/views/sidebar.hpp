// SPDX-FileCopyrightText: 2019-2021 natinusala
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/bind.hpp>
#include <aurore/core/box.hpp>
#include <aurore/views/label.hpp>
#include <aurore/views/rectangle.hpp>
#include <aurore/views/scrolling_frame.hpp>

namespace aurore
{

class SidebarItem;

class SidebarSeparator : public View
{
  public:
    SidebarSeparator();

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
};

class SidebarItemGroup
{
  public:
    void add(SidebarItem* item);
    void setActive(SidebarItem* item);
    void clear();

  private:
    std::vector<SidebarItem*> items;
};

class SidebarItem : public Box
{
  public:
    SidebarItem();

    void onFocusGained() override;
    void onFocusLost() override;

    void setGroup(SidebarItemGroup* group);

    void setLabel(std::string label);

    void setActive(bool active);
    bool isActive() const { return this->active; }

    GenericEvent* getActiveEvent();

  protected:
    AccessibilityRole getDefaultAccessibilityRole() override { return AccessibilityRole::TAB; }
    std::string getDefaultAccessibilityLabel() override { return this->label->getFullText(); }
    bool getDefaultAccessibilitySelected() override { return this->active; }

  private:
    AURORE_BIND(Rectangle, accent, "aurore/sidebar/item_accent");
    AURORE_BIND(Label, label, "aurore/sidebar/item_label");

    GenericEvent activeEvent;

    SidebarItemGroup* group;

    bool active = false;
};

class Sidebar : public ScrollingFrame
{
  public:
    Sidebar();

    /**
     * Adds an item to this sidebar. The given callback will be called
     * when the item becomes active.
     */
    void addItem(std::string label, GenericEvent::Callback focusCallback);

    SidebarItem* getItem(int position);

    /**
     * Adds a separator to this sidebar.
     */
    void addSeparator();

    /**
     * Removes all items from sidebar.
     */
    void clearItems();

    static View* create();

  private:
    SidebarItemGroup group;

    Box* contentBox;
};

} // namespace aurore
