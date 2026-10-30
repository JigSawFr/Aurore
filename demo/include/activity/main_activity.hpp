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

    // A top screen to look at (the New 3DS's): the tab shown, there
    void onContentAvailable() override;

  private:
    aurore::Event<bool>::Subscription enterSubscription;
};
