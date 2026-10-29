// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-FileCopyrightText: 2026 Taylor
// SPDX-License-Identifier: GPL-3.0-or-later

// ui — greying what cannot be written, showing / hiding views with the focus, tabs.
#include <aurore/cadran/ui/ui.hpp>

#include <aurore/cadran/ui/cadran_kit.hpp>
#include <aurore/cadran/ui/cadran_list.hpp>
#include <aurore/cadran/ui/hooks.hpp>

#include <yoga/Yoga.h>
#include <algorithm>

using namespace aurore::literals;

namespace ui
{

void show_writable(aurore::DetailCell* cell, bool writable, const char* title_key, const char* detail_key)
{
    const char* grey = "cadran/text_off";
    cell->title->setThemeColor("textColor", writable ? title_key : grey);
    if (auto* sw = dynamic_cast<aurore::BooleanCell*>(cell))
    {
        if (writable)
            sw->setOn(sw->isOn(), false); // its own value colours back
        else
            sw->detail->setThemeColor("textColor", grey);
        return;
    }
    cell->detail->setThemeColor("textColor", writable ? detail_key : grey);
}

void set_visible(aurore::View* view, bool visible)
{
    if (!view)
        return;
    // An empty state's label (kit::empty_around): its whole card.
    if (aurore::View* card = view->getParent(); card && card->getId() == "cadran_empty_card")
        view = card;
    const bool had_focus = !visible && view->isFocused();
    view->setVisibility(visible ? aurore::Visibility::VISIBLE : aurore::Visibility::GONE);
    if (auto* card = dynamic_cast<cadran::Card*>(view->getParent()))
        card->follow_rows();
    if (!had_focus)
        return;

    // Never leave the focus on a view that just disappeared: prefer the
    // nearest focusable sibling, then the nearest one of each enclosing box in
    // turn (the line may sit in a card of its own), then the container.
    aurore::Box* parent = view->getParent();
    if (!parent)
        return;
    for (aurore::View* node = view; node->getParent(); node = node->getParent())
    {
        const auto& children = node->getParent()->getChildren();
        const int count      = (int)children.size();
        int index            = -1;
        for (int i = 0; i < count; i++)
            if (children[i] == node)
                index = i;
        for (int distance = 1; index >= 0 && distance < count; distance++)
        {
            for (int i : { index + distance, index - distance })
            {
                if (i < 0 || i >= count)
                    continue;
                if (aurore::View* target = children[i]->getDefaultFocus())
                {
                    aurore::Application::giveFocus(target);
                    return;
                }
            }
        }
        // Not past the scrolling frame: the rest of the screen is another list.
        if (dynamic_cast<aurore::ScrollingFrame*>(node->getParent()))
            break;
    }
    aurore::Application::giveFocus(parent);
}

void set_visible_all(std::initializer_list<std::pair<aurore::View*, bool>> changes)
{
    for (const auto& c : changes)
        if (c.second)
            set_visible(c.first, true);
    for (const auto& c : changes)
        if (!c.second)
            set_visible(c.first, false);
}

void set_text(aurore::Label* label, const std::string& text)
{
    if (label && label->getFullText() != text)
        label->setText(text);
}

void set_detail(aurore::DetailCell* cell, const std::string& text)
{
    if (cell)
        set_text(cell->detail, text);
}

void batch_layout(aurore::Box* box, const std::function<void()>& fill)
{
    // Out of its parent's Yoga node only: Aurore's views, their parents and
    // the focus stay as they are, and a change inside stops at the box.
    YGNodeRef node  = box ? box->getYGNode() : nullptr;
    YGNodeRef owner = node ? YGNodeGetOwner(node) : nullptr;
    uint32_t index  = 0;
    while (owner && index < YGNodeGetChildCount(owner) && YGNodeGetChild(owner, index) != node)
        index++;
    if (!owner || index == YGNodeGetChildCount(owner))
    {
        fill();
        return;
    }
    YGNodeRemoveChild(owner, node);
    fill();
    YGNodeInsertChild(owner, node, index);
    box->invalidate();
}

void keep_focus(aurore::Activity* activity, aurore::View* fallback)
{
    // The view to get the focus back: the end of the boxes' last focused
    // views. Box::removeView leaves a removed child there: the walk stops
    // at one no longer among its box's children (never read).
    aurore::View* last = activity->getContentView();
    while (auto* box = dynamic_cast<aurore::Box*>(last))
    {
        aurore::View* next   = box->getLastFocusedView();
        const auto& children = box->getChildren();
        if (!next || std::find(children.begin(), children.end(), next) == children.end())
            break;
        last = next;
    }
    bool gone = !last || !last->isFocusable();
    for (aurore::View* v = last; v && !gone; v = v->getParent())
        gone = v->getVisibility() != aurore::Visibility::VISIBLE;
    if (gone)
        aurore::Application::giveFocus(fallback ? fallback : activity->getContentView());
}

bool has_focus(aurore::Activity* activity)
{
    aurore::View* focus = aurore::Application::getCurrentFocus();
    return focus && focus->getParentActivity() == activity;
}

bool on_top(aurore::Activity* activity)
{
    const auto& stack = aurore::Application::getActivitiesStack();
    return !stack.empty() && stack.back() == activity;
}

aurore::Activity* main_screen()
{
    const auto& is_main = hooks::get().is_main;
    if (!is_main)
        return nullptr;
    for (aurore::Activity* a : aurore::Application::getActivitiesStack())
        if (is_main(a))
            return a;
    return nullptr;
}

void open_tab(int position, const std::string& row)
{
    aurore::Activity* main = main_screen();
    if (main && hooks::get().open_tab)
        hooks::get().open_tab(main, position, row);
}

void refresh_shown_tab()
{
    if (hooks::get().refresh_shown)
        hooks::get().refresh_shown();
}

void go_to_tab(aurore::View* from, int position)
{
    aurore::TabFrame* frame = nullptr;
    for (aurore::View* v = from; v && !frame; v = v->getParent())
        frame = dynamic_cast<aurore::TabFrame*>(v);
    if (!frame)
        return;
    aurore::sync([frame, position]()
        {
        frame->focusTab(position);   // replaces the tab content
        const auto& children = frame->getChildren();
        if (children.size() >= 2) aurore::Application::giveFocus(children.back()); });
}

void open_screen(aurore::View* content, const std::string& title)
{
    auto* frame = new aurore::AppletFrame(content);
    frame->setTitle(title);
    frame->setIcon(aurore::View::getFilePathXMLAttributeValue("@res/img/app_icon.jpg"));
    aurore::Application::pushActivity(new aurore::Activity(frame));
}

} // namespace ui
