// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <aurore/core/application.hpp>
#include <aurore/core/bind.hpp>
#include <aurore/core/box.hpp>
#include <aurore/views/cells/cell_detail.hpp>
#include <aurore/views/recycler.hpp>

namespace aurore
{

class SelectorCell : public DetailCell
{
  public:
    SelectorCell();

    void init(std::string title, std::vector<std::string> data, int selected, Event<int>::Callback callback, Event<int>::Callback dismissCb = [](int) {});

    void setData(std::vector<std::string> data)
    {
        this->data = data;
    }

    void setSelection(int selection, bool silent = false);
    int getSelection()
    {
        return selection;
    }

    Event<int>* getEvent()
    {
        return &event;
    }

    static View* create();

  protected:
    int selection;
    std::vector<std::string> data;

    Event<int> event;
    Event<int>::Callback dismissCb;
    void updateUI();
};

} // namespace aurore
