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

#include <cstdlib>
#include <cstring>
#include <string>
#include <aurore.hpp>
#include <aurore/cadran.hpp>

#include "activity/main_activity.hpp"
#include "tab/cadran_tab.hpp"
#include "tab/components_tab.hpp"
#include "tab/grid_tab.hpp"
#include "tab/recycling_list_tab.hpp"
#include "tab/settings_tab.hpp"
#include "tab/styles_tab.hpp"
#include "tab/text_test_tab.hpp"
#include "tab/transform_tab.hpp"
#include "view/captioned_image.hpp"
#include "view/gem_view.hpp"

namespace
{
// The tab shown: its position in the sidebar, separators included.
int shownTab = 0;

// What Cadran, Aurore's look, asks of the demo: the rail's icons, the main
// screen built again on the tab shown when the language changes (Language &
// theme, in the header or on the Cadran tab), and the folder of the theme
// files the palette picker lists after Aurore's presets.
void installCadranHooks()
{
    static ui::Prefs prefs; // not saved: the demo starts in the defaults
    const char* theme = aurore::environmentVariable("THEME");
    if (theme && !std::strcmp(theme, "LIGHT"))
        prefs.theme = "light";
    ui::use_prefs(prefs);

    ui::hooks::Hooks& h = ui::hooks::get();
    h.themes_dir        = []()
    { return std::string("@res/themes"); };
    h.rail_icons = []()
    {
        return std::vector<std::string> { "sparkle", "list", "lines", "refresh", "pencil", "contrast", "language", "", "activity", "gamepad",
            "shelf", "", "settings", "", "info" };
    };
    h.shown_screen = []()
    { return "tab/" + std::to_string(shownTab); };
    h.main_screen  = h.shown_screen;
    h.tab_position = [](const std::string& screen)
    { return screen.rfind("tab/", 0) == 0 ? std::atoi(screen.c_str() + 4) : -1; };
    h.push_main = [](int position, const std::string&)
    {
        auto* main = new MainActivity();
        aurore::Application::pushActivity(main, aurore::TransitionAnimation::NONE);
        aurore::sync([main, position]()
            {
            if (auto* tabs = dynamic_cast<aurore::TabFrame*>(main->getView("main_tabs")))
                tabs->focusTab(position); });
    };
    aurore::Application::getGlobalFocusChangeEvent()->subscribe([](aurore::View* view)
        {
        auto* item = dynamic_cast<aurore::SidebarItem*>(view);
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
    // A picture of the demo's a theme may replace (image.demo/about/art):
    // the About tab's, drawn by a aurore::Image that follows the theme
    aurore::Theme::addImageSlot("demo/about/art", "@res/img/aurore_256.png", 512);
}

void registerViews()
{
    // Custom views (including tabs, which are views)
    aurore::Application::registerXMLView("CaptionedImage", CaptionedImage::create);
    aurore::Application::registerXMLView("CadranTab", CadranTab::create);
    aurore::Application::registerXMLView("RecyclingListTab", RecyclingListTab::create);
    aurore::Application::registerXMLView("ComponentsTab", ComponentsTab::create);
    aurore::Application::registerXMLView("GridTab", GridTab::create);
    aurore::Application::registerXMLView("TransformTab", TransformTab::create);
    aurore::Application::registerXMLView("TransformBox", TransformBox::create);
    aurore::Application::registerXMLView("GemView", GemView::create);
    aurore::Application::registerXMLView("SettingsTab", SettingsTab::create);
    aurore::Application::registerXMLView("TextTestTab", TextTestTab::create);
    aurore::Application::registerXMLView("StylesTab", StylesTab::create);

    // Add custom values to the theme
    aurore::Theme::getLightTheme().addColor("captioned_image/caption", nvgRGB(2, 176, 183));
    aurore::Theme::getDarkTheme().addColor("captioned_image/caption", nvgRGB(51, 186, 227));
    const std::pair<const char*, NVGcolor> swatches[] = { { "demo/swatch/teal", nvgRGB(46, 196, 182) },
        { "demo/swatch/orange", nvgRGB(255, 159, 28) }, { "demo/swatch/red", nvgRGB(231, 29, 54) },
        { "demo/swatch/blue", nvgRGB(58, 123, 213) } };
    for (const auto& [key, color] : swatches)
    {
        aurore::Theme::getLightTheme().addColor(key, color);
        aurore::Theme::getDarkTheme().addColor(key, color);
    }

    // Add custom values to the style
    aurore::getStyle().addMetric("about/padding_top_bottom", 50);
    aurore::getStyle().addMetric("about/padding_sides", 75);
    aurore::getStyle().addMetric("about/description_margin", 50);
}

} // namespace demo
