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
// The model the tab's bind= attributes read: it lives as long as the app,
// the tab may be built again (a new language or theme).
struct StylesModel
{
    brls::Observable<int> counter { 0 };
    brls::Observable<NVGcolor> swatch { nvgRGB(46, 196, 182) };
    brls::Observable<bool> even { true };
    brls::Observable<std::string> announced { "Move the focus: what a screen reader would say shows here." };
    brls::DataModel model;

    StylesModel()
    {
        model.add("counter", counter);
        model.add("swatch", swatch);
        model.add("even", even);
        model.add("announced", announced);
    }
};

StylesModel& stylesModel()
{
    static StylesModel model;
    return model;
}

const NVGcolor SWATCHES[] = { nvgRGB(46, 196, 182), nvgRGB(255, 159, 28), nvgRGB(231, 29, 54), nvgRGB(58, 123, 213) };
}

StylesTab::StylesTab()
{
    StylesModel& data = stylesModel();
    brls::Application::registerModel("styles", &data.model);

    brls::Application::setFocusAnnouncer([](const std::string& description) {
        stylesModel().announced = "Announced: " + description;
    });

    this->inflateFromXMLRes("xml/tabs/styles.xml");

    // pressed: needs something to press.
    brls::View* fade = this->getView("styles/fade");
    fade->registerClickAction([](brls::View*) { return true; });
    fade->addGestureRecognizer(new brls::TapGestureRecognizer(fade));

    this->disabled->setState(brls::ButtonState::DISABLED);
    this->toggle->registerClickAction([this](brls::View*) {
        bool enabled = this->disabled->getState() == brls::ButtonState::ENABLED;
        this->disabled->setState(enabled ? brls::ButtonState::DISABLED : brls::ButtonState::ENABLED);
        return true;
    });

    this->increment->registerClickAction([](brls::View*) {
        StylesModel& data = stylesModel();
        data.counter = data.counter.get() + 1;
        data.swatch  = SWATCHES[data.counter.get() % 4];
        data.even    = data.counter.get() % 2 == 0;
        return true;
    });

    this->reduceMotion->init("Reduce motion", brls::Application::isReduceMotionSet(), [](bool on) {
        brls::Application::setReduceMotion(on);
    });
}

brls::View* StylesTab::create()
{
    return new StylesTab();
}
