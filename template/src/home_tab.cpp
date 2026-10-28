#include "home_tab.hpp"

HomeTab::HomeTab()
{
    this->inflateFromXMLRes("xml/tabs/home.xml");

    this->counter->registerClickAction([this](aurore::View*)
        {
        this->clicks++;
        this->counter->setText(aurore::getStr("app/home/clicked", this->clicks));
        return true; });
}

aurore::View* HomeTab::create()
{
    return new HomeTab();
}
