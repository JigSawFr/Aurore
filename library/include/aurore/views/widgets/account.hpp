// SPDX-FileCopyrightText: 2021 CrustySeanPro
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/application.hpp>
#include <aurore/core/assets.hpp>
#include <aurore/core/box.hpp>
#include <aurore/views/image.hpp>

namespace aurore
{

class AccountWidget : public Box
{
  public:
    AccountWidget();

    static View* create();

  private:
#ifdef __SWITCH__
    Image* acc;
#endif
};

} // namespace aurore
