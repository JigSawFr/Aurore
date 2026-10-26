/*
    Copyright 2020-2021 natinusala
    Copyright 2019 p-sam

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

#include "demo_app.hpp"

#include <borealis.hpp>
#include <borealis/cadran.hpp>
#include <cstdlib>
#include <cstring>
#include <string>

#include "activity/main_activity.hpp"
#include "tab/cadran_tab.hpp"
#include "tab/components_tab.hpp"
#include "tab/recycling_list_tab.hpp"
#include "tab/settings_tab.hpp"
#include "tab/styles_tab.hpp"
#include "tab/text_test_tab.hpp"
#include "tab/transform_tab.hpp"
#include "view/captioned_image.hpp"
#include "view/pokemon_view.hpp"

namespace
{
// The tab shown: its position in the sidebar, separators included.
int shownTab = 0;

// What Cadran, Aurore's look, asks of the demo: the rail's icons, and the
// main screen built again on the tab shown when the language or the theme
// changes (Language & theme, in the header or on the Cadran tab).
void installCadranHooks()
{
    static ui::Prefs prefs;   // not saved: the demo starts in the defaults
    const char* theme = std::getenv("BOREALIS_THEME");
    if (theme && !std::strcmp(theme, "LIGHT"))
        prefs.theme = "light";
    ui::use_prefs(prefs);

    ui::hooks::Hooks& h = ui::hooks::get();
    h.rail_icons        = []()
    {
        return std::vector<std::string> { "sparkle", "list", "lines", "refresh", "pencil", "contrast", "language", "", "activity", "gamepad",
            "", "settings", "", "info" };
    };
    h.shown_screen = []() { return "tab/" + std::to_string(shownTab); };
    h.main_screen  = h.shown_screen;
    h.tab_position = [](const std::string& screen)
    { return screen.rfind("tab/", 0) == 0 ? std::atoi(screen.c_str() + 4) : -1; };
    h.push_main = [](int position, const std::string&)
    {
        auto* main = new MainActivity();
        brls::Application::pushActivity(main, brls::TransitionAnimation::NONE);
        brls::sync([main, position]()
            {
            if (auto* tabs = dynamic_cast<brls::TabFrame*>(main->getView("main_tabs")))
                tabs->focusTab(position); });
    };
    brls::Application::getGlobalFocusChangeEvent()->subscribe([](brls::View* view)
        {
        auto* item = dynamic_cast<brls::SidebarItem*>(view);
        if (!item || !item->getParent())
            return;
        const auto& items = item->getParent()->getChildren();
        for (size_t i = 0; i < items.size(); i++)
            if (items[i] == item)
                shownTab = (int)i; });
}
}

namespace demo
{

void installHooks()
{
    installCadranHooks();
}

void registerViews()
{
    // Custom views (including tabs, which are views)
    brls::Application::registerXMLView("CaptionedImage", CaptionedImage::create);
    brls::Application::registerXMLView("CadranTab", CadranTab::create);
    brls::Application::registerXMLView("RecyclingListTab", RecyclingListTab::create);
    brls::Application::registerXMLView("ComponentsTab", ComponentsTab::create);
    brls::Application::registerXMLView("TransformTab", TransformTab::create);
    brls::Application::registerXMLView("TransformBox", TransformBox::create);
    brls::Application::registerXMLView("PokemonView", PokemonView::create);
    brls::Application::registerXMLView("SettingsTab", SettingsTab::create);
    brls::Application::registerXMLView("TextTestTab", TextTestTab::create);
    brls::Application::registerXMLView("StylesTab", StylesTab::create);

    // Add custom values to the theme
    brls::Theme::getLightTheme().addColor("captioned_image/caption", nvgRGB(2, 176, 183));
    brls::Theme::getDarkTheme().addColor("captioned_image/caption", nvgRGB(51, 186, 227));

    // Add custom values to the style
    brls::getStyle().addMetric("about/padding_top_bottom", 50);
    brls::getStyle().addMetric("about/padding_sides", 75);
    brls::getStyle().addMetric("about/description_margin", 50);
}

} // namespace demo
