// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#include "aurore/views/cells/cell_selector.hpp"

#include "aurore/views/dropdown.hpp"

namespace aurore
{

SelectorCell::SelectorCell()
{
    detail->setThemeColor("textColor", "aurore/list/listItem_value_color");

    this->registerClickAction([this](View* view)
        {
        Dropdown* dropdown = new Dropdown(
            this->title->getFullText(), data, [this](int selected) {
                this->setSelection(selected, false);
            }, selection, this->dismissCb);
        Application::pushActivity(new Activity(dropdown));
        return true; });
}

void SelectorCell::init(std::string title, std::vector<std::string> data, int selected, Event<int>::Callback callback, Event<int>::Callback dismissCb)
{
    this->title->setText(title);
    this->data      = data;
    this->selection = selected;
    this->dismissCb = dismissCb;
    this->event.subscribe(callback);
    updateUI();
}

void SelectorCell::setSelection(int selection, bool silent)
{
    this->selection = selection;
    if (!silent)
        this->event.fire(selection);
    updateUI();
}

void SelectorCell::updateUI()
{
    auto text = data[selection];
    this->detail->setText(text);
}

View* SelectorCell::create()
{
    return new SelectorCell();
}

} // namespace aurore
