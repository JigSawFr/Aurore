// my_app: a starting point for an app built on Aurore.

#include <cstdlib>
#include <string>
#include <vector>
#include <aurore.hpp>
#include <aurore/cadran.hpp>

#include "home_tab.hpp"

using namespace aurore::literals; // "key"_i18n

// The main screen, from resources/xml/activity/main.xml
class MainActivity : public aurore::Activity
{
  public:
    CONTENT_FROM_XML_RES("activity/main.xml");
};

int main(int argc, char* argv[])
{
    // Cadran, Aurore's look: the icons of the rail, one per tab (before init)
    ui::hooks::get().rail_icons = []()
    { return std::vector<std::string> { "home", "info" }; };

    if (!aurore::Application::init())
    {
        aurore::Logger::error("Unable to init the application");
        return EXIT_FAILURE;
    }

    aurore::Application::createWindow("app/title"_i18n);
    aurore::Application::setGlobalQuit(true); // + quits, on every screen

    // The app's own views, before the XML that uses them
    aurore::Application::registerXMLView("HomeTab", HomeTab::create);

    aurore::Application::pushActivity(new MainActivity());

    while (aurore::Application::mainLoop())
        ;

    return EXIT_SUCCESS;
}
