// SPDX-FileCopyrightText: 2019-2021 natinusala
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/bind.hpp>
#include <aurore/core/box.hpp>
#include <aurore/views/label.hpp>

namespace aurore
{

// A simple header with a title, an optional subtitle, a rectangle on the left
// and a separator
class Header : public Box
{
  public:
    Header();

    void setTitle(std::string text);
    void setSubtitle(std::string text);

    static View* create();

  private:
    AURORE_BIND(Label, title, "aurore/header/title");
    AURORE_BIND(Label, subtitle, "aurore/header/subtitle");
};

} // namespace aurore
