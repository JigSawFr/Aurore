// SPDX-FileCopyrightText: 2020-2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore.hpp>

class MainActivity : public aurore::Activity
{
  public:
    // Declare that the content of this activity is the given XML file
    CONTENT_FROM_XML_RES("activity/main.xml");

    MainActivity();
    ~MainActivity();

  private:
    aurore::Event<bool>::Subscription enterSubscription;
};
