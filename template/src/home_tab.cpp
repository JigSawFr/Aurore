#include "home_tab.hpp"

HomeTab::HomeTab()
{
    this->inflateFromXMLRes("xml/tabs/home.xml");

    this->counter->registerClickAction([this](brls::View*) {
        this->clicks++;
        this->counter->setText(brls::getStr("app/home/clicked", this->clicks));
        return true;
    });
}

brls::View* HomeTab::create()
{
    return new HomeTab();
}
