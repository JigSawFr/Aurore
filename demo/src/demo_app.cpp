// SPDX-FileCopyrightText: 2020-2021 natinusala
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-License-Identifier: Apache-2.0

#include "demo_app.hpp"

#include <cstdlib>
#include <cstring>
#include <string>
#include <aurore.hpp>
#include <aurore/cadran.hpp>

#include "activity/main_activity.hpp"
#include "tab/about_tab.hpp"
#include "tab/components_tab.hpp"
#include "tab/developers_tab.hpp"
#include "tab/home_tab.hpp"
#include "tab/input_tab.hpp"
#include "tab/library_tab.hpp"
#include "tab/motion_tab.hpp"
#include "tab/text_tab.hpp"
#include "tab/themes_tab.hpp"
#include "tab/transform_tab.hpp"
#include "view/aurora_view.hpp"

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
        return std::vector<std::string> { "home", "shelf", "sparkle", "bolt", "language", "palette", "gamepad", "", "code", "",
            "info" };
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

int shownTab()
{
    return ::shownTab;
}

bool isSeparator(int position)
{
    return position == 7 || position == 9;
}

const char* tabName(int position)
{
    switch (position)
    {
        case TAB_HOME:
            return "home";
        case TAB_LIBRARY:
            return "library";
        case TAB_COMPONENTS:
            return "components";
        case TAB_MOTION:
            return "motion";
        case TAB_TEXT:
            return "text";
        case TAB_THEMES:
            return "themes";
        case TAB_INPUT:
            return "input";
        case TAB_DEVELOPERS:
            return "developers";
        case TAB_ABOUT:
            return "about";
        default:
            return "";
    }
}

void openTab(aurore::View* from, int position)
{
    ui::go_to_tab(from, position);
}

void pushScreen(aurore::View* content, const std::string& title, bool dressed)
{
    if (dressed)
        content->setId("cadran_list"); // Cadran leaves its lines as they are
    auto* frame = new aurore::AppletFrame(content);
    frame->setTitle(title);
    aurore::Application::pushActivity(new aurore::Activity(frame), aurore::TransitionAnimation::FADE);
}

void installHooks()
{
    installCadranHooks();
    // A picture of the demo's a theme may replace (image.demo/about/art):
    // the About tab's, drawn by a aurore::Image that follows the theme
    aurore::Theme::addImageSlot("demo/about/art", "@res/img/aurore_256.png", 512);
}

void registerViews()
{
    // The demo's views, for its XML (tabs are views too)
    aurore::Application::registerXMLView("HomeTab", HomeTab::create);
    aurore::Application::registerXMLView("LibraryTab", LibraryTab::create);
    aurore::Application::registerXMLView("ComponentsTab", ComponentsTab::create);
    aurore::Application::registerXMLView("MotionTab", MotionTab::create);
    aurore::Application::registerXMLView("TextTab", TextTab::create);
    aurore::Application::registerXMLView("ThemesTab", ThemesTab::create);
    aurore::Application::registerXMLView("InputTab", InputTab::create);
    aurore::Application::registerXMLView("DevelopersTab", DevelopersTab::create);
    aurore::Application::registerXMLView("AboutTab", AboutTab::create);
    aurore::Application::registerXMLView("AuroraView", AuroraView::create);
    aurore::Application::registerXMLView("TransformBox", TransformBox::create);
    aurore::Application::registerXMLView("ScrollView", ScrollView::create);
    aurore::Application::registerXMLView("CadranIcon", CadranIcon::create);
    aurore::Application::registerXMLView("CadranDigits", CadranDigits::create);
}

} // namespace demo
