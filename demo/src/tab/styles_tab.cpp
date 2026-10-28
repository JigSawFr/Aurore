/*
    Copyright 2026 JigSawFr

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "tab/styles_tab.hpp"

namespace
{
// The swatch's colours: theme keys (demo_app.cpp), so a theme can change them
const char* const SWATCHES[] = { "demo/swatch/teal", "demo/swatch/orange", "demo/swatch/red", "demo/swatch/blue" };

// The model the tab's bind= attributes read: it lives as long as the app,
// the tab may be built again (a new language or theme).
struct StylesModel
{
    aurore::Observable<int> counter { 0 };
    aurore::Observable<NVGcolor> swatch;
    aurore::Observable<bool> even { true };
    aurore::Observable<std::string> announced { aurore::getStr("demo/styles/announced_hint") };
    aurore::DataModel model;

    StylesModel()
    {
        model.add("counter", counter);
        model.add("swatch", swatch);
        model.add("even", even);
        model.add("announced", announced);

        this->updateSwatch();
        aurore::Application::getThemeChangedEvent()->subscribe([this](aurore::ThemeVariant)
            { this->updateSwatch(); });
    }

    void updateSwatch()
    {
        swatch = aurore::Application::getTheme()[SWATCHES[counter.get() % 4]];
    }
};

StylesModel& stylesModel()
{
    static StylesModel model;
    return model;
}
}

StylesTab::StylesTab()
{
    StylesModel& data = stylesModel();
    aurore::Application::registerModel("styles", &data.model);

    aurore::Application::setFocusAnnouncer([](const std::string& description)
        { stylesModel().announced = aurore::getStr("demo/styles/announced", description); });

    this->inflateFromXMLRes("xml/tabs/styles.xml");

    // pressed: needs something to press.
    aurore::View* fade = this->getView("styles/fade");
    fade->registerClickAction([](aurore::View*)
        { return true; });
    fade->addGestureRecognizer(new aurore::TapGestureRecognizer(fade));

    this->disabled->setState(aurore::ButtonState::DISABLED);
    this->toggle->registerClickAction([this](aurore::View*)
        {
        bool enabled = this->disabled->getState() == aurore::ButtonState::ENABLED;
        this->disabled->setState(enabled ? aurore::ButtonState::DISABLED : aurore::ButtonState::ENABLED);
        return true; });

    this->increment->registerClickAction([](aurore::View*)
        {
        StylesModel& data = stylesModel();
        data.counter = data.counter.get() + 1;
        data.updateSwatch();
        data.even = data.counter.get() % 2 == 0;
        return true; });

    this->reduceMotion->init(aurore::getStr("demo/styles/reduce_motion"), aurore::Application::isReduceMotionSet(), [](bool on)
        { aurore::Application::setReduceMotion(on); });
}

aurore::View* StylesTab::create()
{
    return new StylesTab();
}
