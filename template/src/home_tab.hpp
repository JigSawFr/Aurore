#pragma once

#include <borealis.hpp>

// The Home tab, from resources/xml/tabs/home.xml: a tab's views exist once
// the tab is shown, so the code that uses them goes in a view of its own.
class HomeTab : public brls::Box
{
  public:
    HomeTab();

    static brls::View* create();

  private:
    BRLS_BIND(brls::Button, counter, "counter");
    int clicks = 0;
};
