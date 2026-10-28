#pragma once

#include <aurore.hpp>

// The Home tab, from resources/xml/tabs/home.xml: a tab's views exist once
// the tab is shown, so the code that uses them goes in a view of its own.
class HomeTab : public aurore::Box
{
  public:
    HomeTab();

    static aurore::View* create();

  private:
    BRLS_BIND(aurore::Button, counter, "counter");
    int clicks = 0;
};
