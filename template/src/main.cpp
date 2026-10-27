// my_app: a starting point for an app built on Aurore.

#include <cstdlib>
#include <string>
#include <vector>
#include <borealis.hpp>
#include <borealis/cadran.hpp>

#include "home_tab.hpp"

using namespace brls::literals; // "key"_i18n

// The main screen, from resources/xml/activity/main.xml
class MainActivity : public brls::Activity
{
  public:
    CONTENT_FROM_XML_RES("activity/main.xml");
};

int main(int argc, char* argv[])
{
    // Cadran, Aurore's look: the icons of the rail, one per tab (before init)
    ui::hooks::get().rail_icons = []()
    { return std::vector<std::string> { "home", "info" }; };

    if (!brls::Application::init())
    {
        brls::Logger::error("Unable to init the application");
        return EXIT_FAILURE;
    }

    brls::Application::createWindow("app/title"_i18n);
    brls::Application::setGlobalQuit(true); // + quits, on every screen

    // The app's own views, before the XML that uses them
    brls::Application::registerXMLView("HomeTab", HomeTab::create);

    brls::Application::pushActivity(new MainActivity());

    while (brls::Application::mainLoop())
        ;

    return EXIT_SUCCESS;
}
