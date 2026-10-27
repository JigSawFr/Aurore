/*
    Copyright 2021 natinusala

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

#include "tab/components_tab.hpp"

#include <borealis/cadran/ui/cadran_kit.hpp>

using namespace brls::literals; // for _i18n

ComponentsTab::ComponentsTab()
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/tabs/components.xml");

    // Bind the button click to a method using the macro (just for the sake of showcasing it, it's overkill in this situation)
    BRLS_REGISTER_CLICK_BY_ID("button_primary", this->onPrimaryButtonClicked);

    // Get a handle to the button and register the action directly
    brls::Button* highlightButton = (brls::Button*)this->getView("button_highlight");
    highlightButton->registerAction(
        "Honk", brls::BUTTON_A, [](brls::View* view)
        { return true; }, false, false, brls::SOUND_HONK);

    // A context menu under the "more" button, its icons Cadran's
    this->getView("icon_menu")->registerClickAction([](brls::View* view)
        {
        auto* menu    = new brls::ContextMenu(view);
        auto chosen   = [](const std::string& what) { return [what]() { brls::Application::notify(brls::getStr("demo/components/menu_chosen", what)); }; };
        menu->setTitle("demo/components/menu_title"_i18n);
        menu->addItem("demo/components/menu_open"_i18n, chosen("demo/components/menu_open"_i18n), cadran::kit::icon("eye", 22));
        menu->addItem("demo/components/menu_favourite"_i18n, chosen("demo/components/menu_favourite"_i18n), cadran::kit::icon("heart", 22));
        menu->addItem("demo/components/menu_share"_i18n, chosen("demo/components/menu_share"_i18n), cadran::kit::icon("upload", 22));
        menu->addSeparator();
        menu->addItem("demo/components/menu_delete"_i18n, chosen("demo/components/menu_delete"_i18n), cadran::kit::icon("trash", 22, "cadran/coral"));
        menu->open();
        return true; });

    this->getView("icon_play")->registerClickAction([](brls::View* view)
        {
        brls::Application::notify("demo/components/icon_play"_i18n);
        return true; });

    progress->setText(std::to_string((int)(slider->getProgress() * 100)));
    slider->getProgressEvent()->subscribe([this](float progress)
        { this->progress->setText(std::to_string((int)(progress * 100))); });
}

int selected = 0;
bool ComponentsTab::onPrimaryButtonClicked(brls::View* view)
{
    //    brls::AppletFrame* frame = new brls::AppletFrame(PokemonView::create());
    //    frame->setFooterVisibility(brls::Visibility::GONE);
    //    brls::Application::pushActivity(new brls::Activity(frame));

    brls::Dropdown* dropdown = new brls::Dropdown(
        "Test", std::vector<std::string> { "Test 1", "Test 2", "Test 3", "Test 4", "Test 5", "Test 6", "Test 7", "Test 8", "Test 9", "Test 10", "Test 11", "Test 12", "Test 13" }, [](int _selected)
        { selected = _selected; },
        selected);
    //    brls::Dropdown* dropdown = new brls::Dropdown(
    //        "Test", std::vector<std::string> { "Test 1", "Test 2", "Test 3" }, [](int _selected) {
    //            selected = _selected;
    //        },
    //        selected);
    brls::Application::pushActivity(new brls::Activity(dropdown));
    brls::Logger::info("Clicked");
    return true;
}

brls::View* ComponentsTab::create()
{
    // Called by the XML engine to create a new ComponentsTab
    return new ComponentsTab();
}
